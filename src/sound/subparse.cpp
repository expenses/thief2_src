#undef bool
#include <string>
#include <map>
#include <list>
#include <vector>
#include <algorithm>
#define bool sbool

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <windows.h>
#include <lg.h>
#include <mprintf.h>
#include <lglog.h>
#include <config.h>
#include <storeapi.h>

#include <comtools.h>
#include <appagg.h>
#include <resapi.h>
#include <reslock.h>
#include <subtitle.h>

#define SUB_MAX_NAME 256
#define SUB_SEARCH_FLAGS 0x6000

enum eSubBlock
{
   kSubBlockFile,
   kSubBlockType,
   kSubBlockSub,
   kSubBlockMulti,
   kSubBlockSkip
};

typedef std::map<std::string, int> tSubTypeMap;

struct sSubParse
{
   char *pName;
   char *pType;
   char *pDescr;
   char *pTime;
   char *pLength;
   char *pText;
   BOOL bMulti;
   BOOL bSkip;
   ulong flags;
   ulong color;
   int maxDist;
   cSubtitle *pMulti;
   uint lastTime;
   const char *pFile;
   const char *pPath;
   IStore *pStore;
   tSubTypeMap *pHidden;
};

cSubtitleStore *g_pSubtitles;

struct sSubTiming
{
   int maxCount;
   int extraTime;
};

extern "C" ISearchPath *gContextPath;

static BOOL g_bSubtitles;
static IRes *g_pSubFont;

static std::string ConfigString(const std::string &var, const std::string &def)
{
   char *pVal = NULL;
   int cnt = 1;

   if (!config_get_value(var.c_str(), CONFIG_STRING_TYPE, &pVal, &cnt) || cnt <= 0)
      return def;

   std::string val(pVal);
   Free(pVal);
   return val;
}

static int ConfigInt(const std::string &var, int def)
{
   int val = def;
   int cnt = 1;
   config_get_value(var.c_str(), CONFIG_INT_TYPE, &val, &cnt);
   return val;
}

static IRes *LoadSubtitleFont()
{
   std::string dir = ConfigString("subtitles_font_dir", "intrface\\");
   std::string name = ConfigString("subtitles_font_name", "textfont");

   AutoAppIPtr(ResMan);
   IRes *pRes = pResMan->Bind(name.c_str(), "Font", NULL, dir.c_str(), 0);
   if (!pRes)
      _LogWriteMsg(_LogFmt("Failed to locate subtitle font '%s' in '%s', no subtitles will be displayed", name.c_str(), dir.c_str()));
   return pRes;
}

static sSubTiming GetTiming()
{
   sSubTiming timing;
   timing.maxCount = ConfigInt("subtitles_max_count", 5);
   if (timing.maxCount < 0)
      timing.maxCount = 0;
   timing.extraTime = ConfigInt("subtitles_extra_time", 3000);
   return timing;
}

static sSubParse g_parse;
static char g_savedChar;
static int g_line;
static int g_defMaxDist;

#define SubLog0(p, sev, kind, msg) \
   do { \
      mprintf("LOG: subtitles> file '%s' in path '%s'> " kind " (line %d): " msg "\n", (p)->pFile, (p)->pPath, g_line + 1); \
      _LogWriteMsg(_LogFmt(sev ": subtitle file '%s' in path '%s'> " kind " (line %d): " msg, (p)->pFile, (p)->pPath, g_line + 1)); \
   } while (0)

#define SubLog1(p, sev, kind, msg, a) \
   do { \
      mprintf("LOG: subtitles> file '%s' in path '%s'> " kind " (line %d): " msg "\n", (p)->pFile, (p)->pPath, g_line + 1, a); \
      _LogWriteMsg(_LogFmt(sev ": subtitle file '%s' in path '%s'> " kind " (line %d): " msg, (p)->pFile, (p)->pPath, g_line + 1, a)); \
   } while (0)

#define SubLog2(p, sev, kind, msg, a, b) \
   do { \
      mprintf("LOG: subtitles> file '%s' in path '%s'> " kind " (line %d): " msg "\n", (p)->pFile, (p)->pPath, g_line + 1, a, b); \
      _LogWriteMsg(_LogFmt(sev ": subtitle file '%s' in path '%s'> " kind " (line %d): " msg, (p)->pFile, (p)->pPath, g_line + 1, a, b)); \
   } while (0)

#define SubSyntaxError0(p, msg)         SubLog0(p, "ERROR", "SYNTAX ERROR", msg)
#define SubSyntaxError1(p, msg, a)      SubLog1(p, "ERROR", "SYNTAX ERROR", msg, a)
#define SubSyntaxWarning0(p, msg)       SubLog0(p, "WARNING", "SYNTAX WARNING", msg)
#define SubSyntaxWarning1(p, msg, a)    SubLog1(p, "WARNING", "SYNTAX WARNING", msg, a)
#define SubSyntaxWarning2(p, msg, a, b) SubLog2(p, "WARNING", "SYNTAX WARNING", msg, a, b)
#define SubWarning1(p, msg, a)          SubLog1(p, "WARNING", "WARNING", msg, a)
#define SubWarning2(p, msg, a, b)       SubLog2(p, "WARNING", "WARNING", msg, a, b)

cSubtitle::cSubtitle(const char *pDescr, const char *pText, ulong color, int length, ulong flags, uint time, int maxDist)
   : m_text(pText, color)
{
   m_length = length;
   m_maxDist = maxDist;
   m_flags = flags;
   m_time = time;

   if (pDescr && *pDescr)
   {
      m_descr = pDescr;
      if (pText && *pText)
         m_descr += ": ";
   }
}

cSubtitle::~cSubtitle()
{
   while (!m_parts.empty())
   {
      delete m_parts.front();
      m_parts.pop_front();
   }
}

cSubtitleStore::~cSubtitleStore()
{
   for (tSubMap::iterator it = m_subs.begin(); it != m_subs.end(); ++it)
      delete it->second;
}

void cSubtitleStore::Add(const std::string &name, cSubtitle *pSub)
{
   std::string key(name);
   std::transform(key.begin(), key.end(), key.begin(), tolower);
   m_subs[key] = pSub;
}

cSubtitle *cSubtitleStore::Find(const char *pName)
{
   char key[MAX_PATH];
   char *p = key;

   while (*pName)
      *p++ = tolower(*pName++);
   *p = '\0';

   tSubMap::iterator it = m_subs.find(key);
   if (it == m_subs.end())
      return NULL;
   return it->second;
}

static void LoadHiddenTypes(tSubTypeMap &hidden)
{
   char buf[2048];

   if (!config_get_raw("subtitles_hide_types", buf, sizeof(buf)))
      return;

   char *p = buf;
   while (*p && isspace((uchar)*p))
      p++;
   if (!*p)
      return;

   std::transform(p, p + strlen(p), p, tolower);

   while (*p)
   {
      char *pSep = strchr(p, '+');
      if (!pSep)
      {
         if (*p)
            hidden[p] = TRUE;
         break;
      }
      *pSep = '\0';
      if (*p)
         hidden[p] = TRUE;
      p = pSep + 1;
   }
}

static ulong TypeFlags(tSubTypeMap *pHidden, const char *pType)
{
   if (!pType || !*pType)
      return 0;

   char key[SUB_MAX_NAME];
   int i;

   for (i = 0; *pType && i < SUB_MAX_NAME - 1; i++)
      key[i] = tolower((uchar)*pType++);
   key[i] = '\0';

   if (pHidden->find(key) != pHidden->end())
      return 0;
   return kSubShown;
}

static void SkipWhitespace(char **ppCur)
{
   while (**ppCur && isspace((uchar)**ppCur))
   {
      if (**ppCur == '\n')
         g_line++;
      (*ppCur)++;
   }
}

static BOOL CheckInt(sSubParse *pParse, const char *pKey, const char *pVal)
{
   for (const char *p = pVal; *p; p++)
   {
      if (!isdigit((uchar)*p))
      {
         SubSyntaxWarning2(pParse, "'%s' expected a positive int val but found '%s', skipping", pKey, pVal);
         return FALSE;
      }
   }
   return TRUE;
}

static void Unescape(char *pText)
{
   if (!pText)
      return;

   char *pDst = strchr(pText, '\\');
   if (!pDst || !pDst[1])
      return;

   char *pSrc = pDst;
   while (*pSrc)
   {
      if (*pSrc == '\\')
      {
         if (pSrc[1] == 'n')
            *pDst = '\n';
         else if (pSrc[1] == 't')
            *pDst = '\t';
         else
            *pDst = pSrc[1];
         pSrc += 2;
      }
      else
         *pDst = *pSrc++;
      pDst++;
   }
   *pDst = '\0';
}

static int ConfigMaxDist(const char *pVar)
{
   int dist = g_defMaxDist;
   int cnt = 1;
   config_get_value(pVar, CONFIG_INT_TYPE, &dist, &cnt);
   return dist;
}

static ulong ConfigColor(const char *pVar)
{
   int rgb[3];
   int cnt = 3;

   if (pVar && *pVar && config_get_value(pVar, CONFIG_INT_TYPE, rgb, &cnt) && cnt == 3)
   {
      for (int i = 0; i < 3; i++)
      {
         if (rgb[i] < 0)
            rgb[i] = 0;
         else if (rgb[i] > 255)
            rgb[i] = 255;
      }
      return SUB_COLOR_SET | (rgb[2] << 16) | (rgb[1] << 8) | rgb[0];
   }
   return SUB_COLOR_DEFAULT;
}

static void ClearSub(sSubParse *pParse)
{
   if (!pParse->bMulti)
      pParse->pName = NULL;
   pParse->pTime = NULL;
   pParse->pLength = NULL;
   pParse->pText = NULL;
}

static void ClearMultiSub(sSubParse *pParse)
{
   pParse->pName = NULL;
   pParse->pTime = NULL;
   pParse->pLength = NULL;
   pParse->pText = NULL;
   pParse->bMulti = FALSE;
   pParse->bSkip = FALSE;
   pParse->pMulti = NULL;
}

static void ResetParser()
{
   g_savedChar = '\0';
   g_line = 0;
   g_defMaxDist = 0;
   int cnt = 1;
   config_get_value("subtitles_max_dist", CONFIG_INT_TYPE, &g_defMaxDist, &cnt);
   if (g_defMaxDist < 0)
      g_defMaxDist = 0;
}

static BOOL GetToken(sSubParse *pParse, char **ppCur, char **ppToken, BOOL *pQuoted)
{
   char *p;

   *ppToken = NULL;

   if (g_savedChar == '{')
   {
      *ppToken = "{";
      g_savedChar = '\0';
      return TRUE;
   }
   if (g_savedChar == '}')
   {
      *ppToken = "}";
      g_savedChar = '\0';
      return TRUE;
   }
   if (g_savedChar == '"')
   {
      g_savedChar = '\0';
      goto quoted;
   }

   if (g_savedChar == '\n')
      g_line++;
   g_savedChar = '\0';

   SkipWhitespace(ppCur);
   while (**ppCur)
   {
      p = *ppCur;
      if (p[0] == '/' && p[1] == '/')
      {
         *ppCur = p + 2;
         while (**ppCur && **ppCur != '\r' && **ppCur != '\n')
            (*ppCur)++;
      }
      else if (p[0] == '/' && p[1] == '*')
      {
         do
         {
            *ppCur = p + 2;
            while (**ppCur && **ppCur != '*')
            {
               if (**ppCur == '\n')
                  g_line++;
               (*ppCur)++;
            }
            p = *ppCur;
         } while (*p && p[1] != '/');
         if (*p == '*')
            *ppCur = p + 2;
      }
      else
      {
         if (*p == '"')
         {
            *ppCur = p + 1;
            goto quoted;
         }
         if (*p == '{')
         {
            *ppToken = "{";
            *ppCur = p + 1;
            return TRUE;
         }
         if (*p == '}')
         {
            *ppToken = "}";
            *ppCur = p + 1;
            return TRUE;
         }
         while (**ppCur && !isspace((uchar)**ppCur) && **ppCur != '{' && **ppCur != '}' && **ppCur != '"')
            (*ppCur)++;
         g_savedChar = **ppCur;
         **ppCur = '\0';
         (*ppCur)++;
         *ppToken = p;
         return TRUE;
      }
      SkipWhitespace(ppCur);
   }
   return FALSE;

quoted:
   if (pQuoted)
      *pQuoted = TRUE;

   p = *ppCur;
   while (**ppCur)
   {
      char c = **ppCur;
      if (c == '"')
         break;
      if (c == '\n' || c == '\r')
      {
         SubSyntaxError0(pParse, "newline in quoted string");
         throw "newline in quoted string";
      }
      if (c == '\\' && (*ppCur)[1] == '"')
         (*ppCur)++;
      (*ppCur)++;
   }
   if (**ppCur)
   {
      **ppCur = '\0';
      (*ppCur)++;
      *ppToken = p;
      return TRUE;
   }
   return FALSE;
}

static BOOL TerminateLine(char **ppCur)
{
   if (!**ppCur)
      return FALSE;

   char *pEnd = strchr(*ppCur, '\n');
   if (!pEnd)
   {
      pEnd = strchr(*ppCur, '\r');
      if (!pEnd)
         return TRUE;
   }
   if (pEnd > *ppCur && pEnd[-1] == '\r')
      pEnd[-1] = '\0';
   else
      *pEnd = '\0';
   *ppCur = pEnd + 1;
   return TRUE;
}

static void CommitSub(sSubParse *pParse)
{
   cSubtitleStore *pSubs = g_pSubtitles;

   if (!pParse->flags || (!pParse->pDescr && !pParse->pText))
      return;

   uint time = 0;
   int length = 0;

   if (pParse->pTime && CheckInt(pParse, "Time", pParse->pTime))
      time = atoi(pParse->pTime);
   if (pParse->pLength && CheckInt(pParse, "Length", pParse->pLength))
      length = atoi(pParse->pLength);

   if (!pParse->bMulti)
   {
      if (!pSubs->Find(pParse->pName))
      {
         Unescape(pParse->pText);
         cSubtitle *pSub = new cSubtitle(pParse->pDescr, pParse->pText, pParse->color, length, pParse->flags, time, pParse->maxDist);
         pSubs->Add(pParse->pName, pSub);
      }
   }
   else if (!pParse->pLength)
   {
      SubWarning1(pParse, "'skipping MultiSub \"%s\"' entry because 'Length' is missing/invalid", pParse->pName);
   }
   else if (!pParse->bSkip)
   {
      Unescape(pParse->pText);
      if (!pParse->pMulti)
      {
         pParse->lastTime = time;
         if (pSubs->Find(pParse->pName))
         {
            pParse->bSkip = TRUE;
            return;
         }
         pParse->pMulti = new cSubtitle(pParse->pDescr, pParse->pText, pParse->color, length, pParse->flags, time, pParse->maxDist);
         pSubs->Add(pParse->pName, pParse->pMulti);
      }
      else
      {
         if (time < pParse->lastTime)
            SubWarning1(pParse, "'MultiSub \"%s\"' entry with out-of-order 'Time' value", pParse->pName);
         else
            pParse->lastTime = time;
         pParse->pMulti->m_parts.push_back(new cSubtitle(pParse->pDescr, pParse->pText, pParse->color, length, pParse->flags, time, pParse->maxDist));
      }
   }
}

static BOOL LoadSrt(sSubParse *pParse, const char *pFile)
{
   IStore *pStore = pParse->pStore;
   IStoreStream *pStream = NULL;

   if (pStore && pStore->Exists())
   {
      ResManLock();
      pStream = pStore->OpenStream(pFile, 0);
      ResManUnlock();
   }
   if (!pStream)
   {
      SubWarning1(pParse, "Failed to open SRT subtitle file \"%s\"", pFile);
      return TRUE;
   }

   long size = pStream->GetSize();
   if (size < 1)
   {
      pStream->Close();
      pStream->Release();
      return TRUE;
   }

   std::vector<char> buf(size + 2);

   if (pStream->Read(size, &buf[0]) != size)
   {
      SubWarning1(pParse, "Failed to read SRT subtitle file \"%s\"", pFile);
      pStream->Close();
      pStream->Release();
      return TRUE;
   }
   pStream->Close();
   pStream->Release();
   buf[size] = '\0';
   buf[size + 1] = '\0';

   cSubtitleStore *pSubs = g_pSubtitles;
   char *pCur = &buf[0];
   std::string text;
   BOOL bFirst = TRUE;
   BOOL bComma;

   while (pCur && TerminateLine(&pCur))
   {
      char *pTimes = pCur;
      if (!pCur || !TerminateLine(&pCur))
         return TRUE;

      if (bFirst)
      {
         bComma = strchr(pTimes, ',') != NULL;
         bFirst = FALSE;
      }

      uint startHour = 0, startMin = 0, startSec = 0, startMs = 0;
      uint endHour = 0, endMin = 0, endSec = 0, endMs = 0;

      sscanf(pTimes, bComma ? "%u:%u:%u,%u --> %u:%u:%u,%u" : "%u:%u:%u.%u --> %u:%u:%u.%u",
             &startHour, &startMin, &startSec, &startMs, &endHour, &endMin, &endSec, &endMs);

      uint time = ((startHour * 60 + startMin) * 60 + startSec) * 1000 + startMs;
      int length = ((endHour * 60 + endMin) * 60 + endSec) * 1000 - time + endMs;

      text = "";
      while (pCur && *pCur)
      {
         char *pLine = pCur;
         char *pEnd = strchr(pLine, '\n');
         if (!pEnd)
            pEnd = strchr(pLine, '\r');
         if (pEnd)
         {
            if (pLine < pEnd && pEnd[-1] == '\r')
               pEnd[-1] = '\0';
            else
               *pEnd = '\0';
            pCur = pEnd + 1;
         }
         if (!*pLine)
            break;
         if (!text.empty())
            text += "\n";
         text += pLine;
      }

      if (!text.empty())
      {
         if (!pParse->pMulti)
         {
            pParse->lastTime = time;
            pParse->pMulti = new cSubtitle(pParse->pDescr, text.c_str(), pParse->color, length, pParse->flags, time, pParse->maxDist);
            pSubs->Add(pParse->pName, pParse->pMulti);
         }
         else
         {
            if (time < pParse->lastTime)
               SubWarning2(pParse, "'SrtSub \"%s\"' SRT file \"%s\" contains entry with out-of-order time values", pParse->pName, pFile);
            else
               pParse->lastTime = time;
            pParse->pMulti->m_parts.push_back(new cSubtitle(pParse->pDescr, text.c_str(), pParse->color, length, pParse->flags, time, pParse->maxDist));
         }
      }
   }
   return TRUE;
}

static BOOL CheckSubName(sSubParse *pParse, const char *pKey, char *pName)
{
   if (!pName)
   {
      SubSyntaxWarning1(pParse, "'%s' has empty name parameter, skipping", pKey);
      return FALSE;
   }
   if (strlen(pName) >= SUB_MAX_NAME)
   {
      SubSyntaxWarning1(pParse, "'%s' has too long name parameter, possible syntax error", pKey);
      pName[SUB_MAX_NAME - 1] = '\0';
   }
   if (!pParse->pType)
   {
      SubSyntaxWarning1(pParse, "'%s' has no 'Type' defined, skipping", pKey);
      return FALSE;
   }
   return TRUE;
}

static void BeginSub(sSubParse *pParse, ulong flags)
{
   char var[256];

   pParse->flags = TypeFlags(pParse->pHidden, pParse->pType) | flags;
   sprintf(var, "subtitles_color_%s", pParse->pType);
   pParse->color = ConfigColor(var);
   sprintf(var, "subtitles_max_dist_%s", pParse->pType);
   pParse->maxDist = ConfigMaxDist(var);
}

static BOOL ParseBlock(sSubParse *pParse, char **ppCur, int depth, int block, BOOL bOpened)
{
   char *pKey;
   char *pArg;
   char *pName;
   char *pSrtFile;

   if (!bOpened)
   {
      if (!GetToken(pParse, ppCur, &pKey, NULL))
         return TRUE;
      if (*pKey != '{')
      {
         SubSyntaxError0(pParse, "expected '{'");
         return FALSE;
      }
      if (block == kSubBlockFile)
         block = kSubBlockType;
   }

   BOOL bBlockOk = TRUE;
   BOOL bHaveSubs = FALSE;
   ulong flags = kSub2D | kSub3D;

   while (**ppCur && GetToken(pParse, ppCur, &pKey, NULL) && *pKey != '}')
   {
      if (*pKey == '{')
      {
         if (block == kSubBlockMulti)
         {
            if (!ParseBlock(pParse, ppCur, depth + 1, kSubBlockSub, TRUE))
               return FALSE;
            CommitSub(pParse);
            ClearSub(pParse);
            continue;
         }
         if (!bBlockOk)
            SubSyntaxWarning0(pParse, "unexpected '{', trying to skip block");
         if (!ParseBlock(pParse, ppCur, depth + 1, kSubBlockSkip, TRUE))
            return FALSE;
         continue;
      }

      BOOL bQuoted = FALSE;

      if (stricmp(pKey, "Singleton") != 0 && stricmp(pKey, "NoVolume") != 0)
      {
         if (!GetToken(pParse, ppCur, &pArg, &bQuoted))
         {
            SubSyntaxError1(pParse, "unexpected EOF after '%s'", pKey);
            return FALSE;
         }
         if (!bQuoted && *pArg == '{')
         {
            SubSyntaxWarning1(pParse, "unexpected '{' after '%s', trying to skip block", pKey);
            if (!ParseBlock(pParse, ppCur, depth + 1, kSubBlockSkip, TRUE))
               return FALSE;
            continue;
         }
         pName = (pArg && *pArg) ? pArg : NULL;
      }
      else
      {
         pArg = NULL;
         pName = NULL;
      }

      bBlockOk = FALSE;

      if (block == kSubBlockType)
      {
         BOOL bLate = FALSE;

         if (stricmp(pKey, "Type") == 0)
         {
            if (pParse->pType)
               SubSyntaxWarning1(pParse, "skipping duplicate '%s' declaration in block", pKey);
            else if (bHaveSubs)
               bLate = TRUE;
            else
               pParse->pType = pName;
         }
         else if (stricmp(pKey, "Descr") == 0)
         {
            if (pParse->pDescr)
               SubSyntaxWarning1(pParse, "skipping duplicate '%s' declaration in block", pKey);
            else if (bHaveSubs)
               bLate = TRUE;
            else
               pParse->pDescr = pArg;
         }
         else if (stricmp(pKey, "Singleton") == 0)
         {
            if (bHaveSubs)
               bLate = TRUE;
            else
               flags |= kSubSingleton;
         }
         else if (stricmp(pKey, "NoVolume") == 0)
         {
            if (bHaveSubs)
               bLate = TRUE;
            else
               flags |= kSubNoVolume;
         }
         else if (stricmp(pKey, "OnlySoundType") == 0)
         {
            if (bHaveSubs)
               bLate = TRUE;
            else if (!pName)
               SubSyntaxWarning1(pParse, "'%s' has empty name parameter, skipping", pKey);
            else
            {
               if ((flags & (kSub2D | kSub3D)) != (kSub2D | kSub3D))
                  SubSyntaxWarning1(pParse, "multiple '%s' parameters in block, only the last one will have any effect", pKey);
               if (stricmp(pArg, "2D") == 0)
                  flags = (flags & ~kSub3D) | kSub2D;
               else if (stricmp(pArg, "3D") == 0)
                  flags = (flags & ~kSub2D) | kSub3D;
               else
                  SubSyntaxWarning2(pParse, "'%s' has bad parameter '%s', skipping", pKey, pArg);
            }
         }
         else if (stricmp(pKey, "Sub") == 0)
         {
            bHaveSubs = TRUE;
            if (CheckSubName(pParse, pKey, pName))
            {
               pParse->pName = pArg;
               BeginSub(pParse, flags);
               if (!ParseBlock(pParse, ppCur, depth + 1, kSubBlockSub, FALSE))
               {
                  ClearSub(pParse);
                  return FALSE;
               }
               CommitSub(pParse);
               ClearSub(pParse);
               continue;
            }
            ClearSub(pParse);
            if (!ParseBlock(pParse, ppCur, depth + 1, kSubBlockSkip, FALSE))
               return FALSE;
         }
         else if (stricmp(pKey, "MultiSub") == 0)
         {
            bHaveSubs = TRUE;
            if (CheckSubName(pParse, pKey, pName))
            {
               pParse->pName = pArg;
               pParse->bMulti = TRUE;
               BeginSub(pParse, flags);
               BOOL bResult = ParseBlock(pParse, ppCur, depth + 1, kSubBlockMulti, FALSE);
               ClearMultiSub(pParse);
               if (!bResult)
                  return FALSE;
               continue;
            }
            ClearMultiSub(pParse);
            if (!ParseBlock(pParse, ppCur, depth + 1, kSubBlockSkip, FALSE))
               return FALSE;
         }
         else if (stricmp(pKey, "SrtSub") == 0)
         {
            bHaveSubs = TRUE;
            if (CheckSubName(pParse, pKey, pName))
            {
               if (!GetToken(pParse, ppCur, &pSrtFile, NULL) || !*pSrtFile)
               {
                  SubSyntaxError1(pParse, "'%s' is missing SRT filename parameter", pKey);
                  return FALSE;
               }
               if (!g_pSubtitles->Find(pArg))
               {
                  pParse->pName = pArg;
                  pParse->bMulti = TRUE;
                  BeginSub(pParse, flags);
                  if (!LoadSrt(pParse, pSrtFile))
                  {
                     ClearMultiSub(pParse);
                     return FALSE;
                  }
               }
               ClearMultiSub(pParse);
               continue;
            }
            ClearMultiSub(pParse);
            if (!ParseBlock(pParse, ppCur, depth + 1, kSubBlockSkip, FALSE))
               return FALSE;
         }
         else
         {
            bBlockOk = TRUE;
            SubSyntaxWarning1(pParse, "skipping unknown or misplaced '%s' declaration", pKey);
         }

         if (bLate)
         {
            if (!bQuoted)
               SubSyntaxWarning2(pParse, "skipping '%s %s' because it was not specified before any 'Sub'/'MultiSub'/'SrtSub' declarations", pKey, pArg);
            else
               SubSyntaxWarning2(pParse, "skipping '%s \"%s\"' because it was not specified before any 'Sub'/'MultiSub'/'SrtSub' declarations", pKey, pArg);
         }
      }
      else if (block == kSubBlockSub)
      {
         if (stricmp(pKey, "Text") == 0)
            pParse->pText = pName ? pName : "";
         else if (stricmp(pKey, "Time") == 0)
            pParse->pTime = pName;
         else if (stricmp(pKey, "Length") == 0)
            pParse->pLength = pName;
         else
         {
            bBlockOk = TRUE;
            SubSyntaxWarning1(pParse, "skipping unknown or misplaced '%s' declaration", pKey);
         }
      }
      else if (block == kSubBlockMulti)
      {
         bBlockOk = TRUE;
         SubSyntaxWarning1(pParse, "skipping unknown or misplaced '%s' declaration", pKey);
      }
      else if (block != kSubBlockSkip)
      {
         SubSyntaxError0(pParse, "unknown");
         return FALSE;
      }
   }
   return TRUE;
}

static BOOL ParseFile(char *pData, const char *pFile, const char *pPath, tSubTypeMap *pHidden, IStore *pStore)
{
   if (!pData || !*pData)
      return TRUE;

   ResetParser();

   if (strncmp(pData, "SUB1", 4) != 0)
   {
      g_parse.pFile = pFile;
      g_parse.pPath = pPath;
      SubSyntaxError0(&g_parse, "invalid file header");
      return FALSE;
   }

   pData += 4;
   SkipWhitespace(&pData);

   while (pData && *pData)
   {
      memset(&g_parse, 0, sizeof(g_parse));
      g_parse.pFile = pFile;
      g_parse.pHidden = pHidden;
      g_parse.pStore = pStore;
      g_parse.pPath = pPath;
      if (!ParseBlock(&g_parse, &pData, 0, kSubBlockFile, FALSE))
         return FALSE;
   }
   return TRUE;
}

void SubtitlesLoad(ISearchPath *pPath)
{
   void *pCookie = pPath->BeginContents("*.sub", SUB_SEARCH_FLAGS, "subtitles\\");
   if (!pCookie)
      return;

   tSubTypeMap hidden;
   LoadHiddenTypes(hidden);

   IStore *pStore = NULL;
   IStore *pCanonStore = NULL;
   char name[MAX_PATH + 1] = "";

   while (pPath->Next(pCookie, &pStore, name, &pCanonStore))
   {
      if (!name[0])
         continue;

      int len = strlen(name);
      if (len <= 3 || name[len - 4] != '.' || stricmp(&name[len - 3], "sub") != 0)
         continue;

      IStoreStream *pStream = NULL;
      if (pStore && pStore->Exists())
      {
         ResManLock();
         pStream = pStore->OpenStream(name, 0);
         ResManUnlock();
      }

      std::string path(pStore->GetFullPathName());
      pCanonStore->Release();

      if (!pStream)
      {
         pStore->Release();
         _LogWriteMsg(_LogFmt("WARNING: Failed to open subtitle file '%s' in path '%s'", name, path.c_str()));
         continue;
      }

      long size = pStream->GetSize();
      if (size < 1)
      {
         pStore->Release();
         pStream->Close();
         pStream->Release();
         continue;
      }

      std::vector<char> buf(size + 2);

      if (pStream->Read(size, &buf[0]) != size)
      {
         pStore->Release();
         _LogWriteMsg(_LogFmt("WARNING: Failed to read subtitle file '%s' in path '%s'", name, path.c_str()));
         pStream->Close();
         pStream->Release();
         continue;
      }
      pStream->Close();
      pStream->Release();
      buf[size] = '\0';
      buf[size + 1] = '\0';

      try
      {
         ParseFile(&buf[0], name, path.c_str(), &hidden, pStore);
      }
      catch (const char *)
      {
         _LogWriteMsg(_LogFmt("ERROR: Partially or completely failed to load subtitle file '%s' in path '%s'", name, path.c_str()));
      }
      pStore->Release();
   }

   pPath->EndContents(pCookie);
}

static void SubtitlesLoadPath(ISearchPath *pPath)
{
   if (pPath)
   {
      AutoAppIPtr(ResMan);
      SubtitlesLoad(pPath);
   }
}

extern "C" void SubtitlesTerm()
{
   if (g_pSubtitles)
   {
      delete g_pSubtitles;
      g_pSubtitles = NULL;
   }
   if (g_pSubFont)
      g_pSubFont->Release();
   g_bSubtitles = FALSE;
   g_pSubFont = NULL;
}

extern "C" void SubtitlesInit()
{
   ISearchPath *pPath = gContextPath;

   g_bSubtitles = config_is_nonzero("enable_subtitles");
   if (!g_bSubtitles)
      return;

   g_pSubtitles = new cSubtitleStore;
   try
   {
      SubtitlesLoadPath(pPath);
   }
   catch (...)
   {
   }

   if (g_pSubtitles->IsEmpty())
   {
      SubtitlesTerm();
      return;
   }

   g_pSubFont = LoadSubtitleFont();
   if (!g_pSubFont)
   {
      SubtitlesTerm();
      return;
   }
}

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <lg.h>
#include <mprintf.h>
#include <math.h>
#include <comtools.h>
#include <appagg.h>
#include <resapi.h>
#include <imgrstyp.h>
#include <storeapi.h>
#include <tmpalloc.h>
#include <config.h>
#include <dev2d.h>
#include <tmgr.h>
#include <resmtl.h>

void MtlSkipSpace(char **pp)
{
   while (**pp && iswspace(**pp))
      (*pp)++;
}

void MtlEndToken(char **pp)
{
   while (**pp && !iswspace(**pp))
      (*pp)++;
   if (**pp)
   {
      **pp = '\0';
      (*pp)++;
   }
}

void MtlTrim(char **pp)
{
   MtlSkipSpace(pp);
   if (!**pp)
      return;
   char *pEnd = *pp + strlen(*pp) - 1;
   while (iswspace(*pEnd))
      pEnd--;
   if (**pp == '"' && *pEnd == '"')
   {
      (*pp)++;
      *pEnd = '\0';
   }
}

char *MtlNextToken(char **pp)
{
   MtlSkipSpace(pp);
   if (!**pp)
      return NULL;
   if (**pp == '"')
   {
      (*pp)++;
      char *pStart = *pp;
      while (**pp && **pp != '"')
         (*pp)++;
      **pp = '\0';
      (*pp)++;
      return pStart;
   }
   MtlSkipSpace(pp);
   char *pStart = *pp;
   MtlEndToken(pp);
   return pStart;
}

void MtlParseFlag(char **pp, ulong flag, ulong *pFlags)
{
   MtlTrim(pp);
   if (!**pp || atoi(*pp))
      *pFlags |= flag;
   else
      *pFlags &= ~flag;
}

BOOL MtlParseBlendFunc(const char *pszMaterial, const char *pszName, int *pFunc)
{
   const char *apszNames[] =
   {
      "ZERO", "ONE", "SRC_ALPHA", "INV_SRC_ALPHA", "SRC_COLOR", "INV_SRC_COLOR",
      "DST_ALPHA", "INV_DST_ALPHA", "DST_COLOR", "INV_DST_COLOR", NULL
   };
   for (int i = 0; apszNames[i]; i++)
   {
      if (!stricmp(apszNames[i], pszName))
      {
         *pFunc = i;
         return TRUE;
      }
   }
   mprintf("WARNING: skipping unknown blend function '%s' in material '%s'\n", pszName, pszMaterial);
   return FALSE;
}

BOOL MtlParseUVSource(const char *pszMaterial, const char *pszName, int *pSource)
{
   const char *apszNames[] = { "TEXTURE", "LIGHTMAP", "ENVIRONMENT", "PROJECTION", NULL };
   for (int i = 0; apszNames[i]; i++)
   {
      if (!stricmp(apszNames[i], pszName))
      {
         *pSource = i;
         return TRUE;
      }
   }
   mprintf("WARNING: skipping unknown uv source '%s' in material '%s'\n", pszName, pszMaterial);
   return FALSE;
}

void MtlParseFloat(char **pp, float *pf)
{
   char *pTok = *pp;
   MtlEndToken(pp);
   MtlTrim(&pTok);
   *pf = (float)atof(pTok);
}

void MtlParseFloat2(char **pp, float *pf1, float *pf2)
{
   char *pTok1 = *pp;
   MtlEndToken(pp);
   MtlTrim(&pTok1);
   char *pTok2 = *pp;
   MtlEndToken(pp);
   MtlTrim(&pTok2);
   *pf1 = (float)atof(pTok1);
   *pf2 = (float)atof(pTok2);
}

void MtlParseInt2(char **pp, int *pn1, int *pn2)
{
   char *pTok1 = *pp;
   MtlEndToken(pp);
   MtlTrim(&pTok1);
   char *pTok2 = *pp;
   MtlEndToken(pp);
   MtlTrim(&pTok2);
   *pn1 = atoi(pTok1);
   *pn2 = atoi(pTok2);
}

EXTERN BOOL UseD3DDisplay();

#define MTL_MAX_IF_DEPTH 32

enum eMtlIfState
{
   kMtlIfSkip,
   kMtlIfActive,
   kMtlIfDead,
};

static int g_nMtlIfDepth;
static int g_aMtlIfState[MTL_MAX_IF_DEPTH + 1];

static void MtlPreprocIf(BOOL bIfDef, const char *pszMaterial, const char *pszArg)
{
   const char *pszCmd = bIfDef ? "ifdef" : "ifndef";
   if (g_nMtlIfDepth >= MTL_MAX_IF_DEPTH)
   {
      mprintf("WARNING: too deep nesting of '.%s' in material '%s'\n", pszCmd, pszMaterial);
      return;
   }
   if (!pszArg || !*pszArg)
   {
      mprintf("WARNING: '.%s' argument is missing in material '%s'\n", pszCmd, pszMaterial);
      return;
   }

   BOOL bDefined = FALSE;
   if (!strcmp(pszArg, "DX6"))
      bDefined = !UseD3DDisplay();
   else if (!strcmp(pszArg, "DX9"))
      bDefined = UseD3DDisplay();
   else if (*pszArg == '$' && pszArg[1])
      bDefined = config_is_nonzero(pszArg + 1);
   else if (!strcmp(pszArg, "0"))
      bDefined = FALSE;
   else if (!strcmp(pszArg, "1"))
      bDefined = TRUE;
   else
      mprintf("WARNING: invalid argument \"%s\" for '.%s' in material '%s'\n", pszArg, pszCmd, pszMaterial);

   if (g_nMtlIfDepth && g_aMtlIfState[g_nMtlIfDepth] != kMtlIfActive)
   {
      g_aMtlIfState[g_nMtlIfDepth + 1] = kMtlIfDead;
      g_nMtlIfDepth++;
      return;
   }
   g_nMtlIfDepth++;
   g_aMtlIfState[g_nMtlIfDepth] = (bDefined == bIfDef);
}

static void MtlPreprocElse(const char *pszMaterial)
{
   if (!g_nMtlIfDepth)
   {
      mprintf("WARNING: non-matching '.else' in material '%s'\n", pszMaterial);
      return;
   }
   if (g_aMtlIfState[g_nMtlIfDepth] != kMtlIfDead)
      g_aMtlIfState[g_nMtlIfDepth] = (g_aMtlIfState[g_nMtlIfDepth] == kMtlIfSkip);
}

BOOL MtlPreprocess(char **pp, const char *pszMaterial)
{
   if (**pp != '.')
      return FALSE;
   char *pszCmd = ++(*pp);
   while (**pp && !isspace((uchar)**pp))
      (*pp)++;
   char *pszArg = *pp;
   while (**pp && **pp != '\r' && **pp != '\n')
      (*pp)++;
   if (**pp)
   {
      **pp = '\0';
      do
         (*pp)++;
      while (**pp == '\r' || **pp == '\n');
   }
   *pszArg = '\0';
   if (pszArg < *pp)
      while (pszArg[1] && isspace((uchar)pszArg[1]))
         pszArg++;
   pszArg++;

   if (!strcmp(pszCmd, "ifdef"))
   {
      MtlTrim(&pszArg);
      MtlPreprocIf(TRUE, pszMaterial, pszArg);
      return TRUE;
   }
   if (!strcmp(pszCmd, "ifndef"))
   {
      MtlTrim(&pszArg);
      MtlPreprocIf(FALSE, pszMaterial, pszArg);
      return TRUE;
   }
   if (!strcmp(pszCmd, "else"))
   {
      MtlPreprocElse(pszMaterial);
      return TRUE;
   }
   if (strcmp(pszCmd, "endif"))
   {
      mprintf("WARNING: unknown preprocessor command '.%s' in material '%s'\n", pszCmd, pszMaterial);
      return TRUE;
   }
   if (g_nMtlIfDepth)
   {
      g_nMtlIfDepth--;
      return TRUE;
   }
   mprintf("WARNING: non-matching '.endif' in material '%s'\n", pszMaterial);
   return TRUE;
}

BOOL MtlIsActive(void)
{
   if (g_nMtlIfDepth && g_aMtlIfState[g_nMtlIfDepth] != kMtlIfActive)
      return FALSE;
   return TRUE;
}

void MtlConcatUVXform(const sMtlUVXform *pA, const sMtlUVXform *pB, sMtlUVXform *pOut)
{
   pOut->m[0][0] = pA->m[0][1] * pB->m[1][0] + pA->m[0][0] * pB->m[0][0];
   pOut->m[0][1] = pA->m[0][0] * pB->m[0][1] + pB->m[1][1] * pA->m[0][1];
   pOut->m[0][2] = pB->m[0][2] * pA->m[0][0] + pB->m[1][2] * pA->m[0][1] + pA->m[0][2];
   pOut->m[1][0] = pA->m[1][1] * pB->m[1][0] + pA->m[1][0] * pB->m[0][0];
   pOut->m[1][1] = pA->m[1][0] * pB->m[0][1] + pB->m[1][1] * pA->m[1][1];
   pOut->m[1][2] = pB->m[0][2] * pA->m[1][0] + pB->m[1][2] * pA->m[1][1] + pA->m[1][2];
}

int MtlMakeFramePattern(char *pszPattern, const char *pszName, BOOL bAppendIndex, const char **ppszExt)
{
   strncpy(pszPattern, pszName, 0x104);
   pszPattern[0x103] = '\0';
   if (ppszExt)
      *ppszExt = "";

   char *pEnd = strrchr(pszPattern, '.');
   if (pEnd)
   {
      *pEnd = '\0';
      if (ppszExt)
         *ppszExt = pszName + (pEnd - pszPattern);
   }
   else
      pEnd = pszPattern + strlen(pszPattern);

   char *p = pEnd - 1;
   if (!bAppendIndex)
   {
      int nDigits = 0;
      while (p >= pszPattern && isdigit((uchar)*p))
      {
         p--;
         nDigits++;
      }
      if (nDigits)
      {
         if (p < pszPattern || !isdigit((uchar)*p))
            p++;
         int nFirst = atoi(p);
         if (nDigits < 2)
            strcpy(p, "%d");
         else
            sprintf(p, "%%0%dd", nDigits);
         if (ppszExt)
            strcat(p, "%s");
         return nFirst;
      }
      strcat(pszPattern, "%d");
   }
   else if (*p != '_')
      strcat(pszPattern, "_%d");
   else
      strcat(pszPattern, "%d");
   if (ppszExt)
      strcat(pszPattern, "%s");
   return 0;
}

int MtlLookupName(const char **ppszNames, const char *pszName)
{
   for (int i = 0; ppszNames[i]; i++)
      if (!stricmp(pszName, ppszNames[i]))
         return i;
   return -1;
}

void MtlLayerUpdateXform(sMtlTexLayer *pLayer)
{
   sMtlUVXform scale, rot;
   float s, c;

   scale.m[0][0] = pLayer->scale[0];
   scale.m[0][1] = 0.0;
   scale.m[0][2] = -pLayer->pivot[0] * pLayer->scale[0];
   scale.m[1][0] = 0.0;
   scale.m[1][1] = pLayer->scale[1];
   scale.m[1][2] = -pLayer->pivot[1] * pLayer->scale[1];
   if (pLayer->rotation != 0.0)
   {
      s = sin(pLayer->rotation);
      c = cos(pLayer->rotation);
   }
   else
   {
      c = 1.0;
      s = 0.0;
   }
   rot.m[0][0] = c;
   rot.m[0][1] = -s;
   rot.m[0][2] = pLayer->offset[0] + pLayer->pivot[0];
   rot.m[1][0] = s;
   rot.m[1][1] = c;
   rot.m[1][2] = pLayer->offset[1] + pLayer->pivot[1];
   MtlConcatUVXform(&rot, &scale, &pLayer->xform);
}

void MtlLayerInit(sMtlTexLayer *pLayer)
{
   memset(pLayer, 0, sizeof(*pLayer));
   pLayer->rgb[0] = 1.0;
   pLayer->rgb[1] = 1.0;
   pLayer->srcBlend = 2;
   pLayer->rgb[2] = 1.0;
   pLayer->dstBlend = 3;
   pLayer->alpha = 1.0;
   pLayer->packedColor = 0xFFFFFFFF;
   pLayer->pivot[0] = 0.5;
   pLayer->pivot[1] = 0.5;
   pLayer->scale[0] = 1.0;
   pLayer->scale[1] = 1.0;
   pLayer->xform.m[0][0] = 1.0;
   pLayer->xform.m[1][1] = 1.0;
}

static ulong MtlColorByte(float f)
{
   return (ulong)(f * 255.0);
}

static void MtlClamp01(float *pf)
{
   if (*pf < 0.0)
      *pf = 0.0;
   else if (*pf > 1.0)
      *pf = 1.0;
}

void MtlLayerFinish(sMtlTexLayer *pLayer)
{
   if (pLayer->nFrames < 2)
   {
      pLayer->aniRate = 0;
      pLayer->aniMode = 0;
   }
   if (pLayer->rgbFunc == kMtlFuncWave || pLayer->alphaFunc == kMtlFuncWave
       || pLayer->uvFunc[0] > 1 || pLayer->uvFunc[1] > 1
       || pLayer->uvFunc[2] > 0 || pLayer->uvFunc[3] > 0 || pLayer->uvFunc[4] > 0)
      pLayer->flags |= kMtlLayerAnimated;
   if (pLayer->srcBlend != 1 || pLayer->dstBlend != 0)
   {
      pLayer->flags |= kMtlLayerTranslucent;
      if (pLayer->srcBlend == 2 || pLayer->srcBlend == 3 || pLayer->dstBlend == 2 || pLayer->dstBlend == 3)
         pLayer->flags |= kMtlLayerAdditive;
   }
   MtlClamp01(&pLayer->rgb[0]);
   MtlClamp01(&pLayer->rgb[1]);
   MtlClamp01(&pLayer->rgb[2]);
   MtlClamp01(&pLayer->alpha);
   pLayer->packedColor = (((MtlColorByte(pLayer->alpha) << 8 | MtlColorByte(pLayer->rgb[0])) << 8
                          | MtlColorByte(pLayer->rgb[1])) << 8) | MtlColorByte(pLayer->rgb[2]);
   MtlLayerUpdateXform(pLayer);
}

static void MtlReleaseRes(IRes *pRes)
{
   pRes->Unlock();
   pRes->Drop();
   pRes->Release();
}

void MtlLayerFreeTextures(sMtlTexLayer *pLayer)
{
   pLayer->flags &= ~(kMtlLayerTexture | kMtlLayerLightmap);
   if (pLayer->pFrames)
   {
      if (pLayer->nFrames > 1)
      {
         sMtlFrame *pFrame = (sMtlFrame *)pLayer->pFrames;
         for (int i = 0; i < pLayer->nFrames; i++, pFrame++)
         {
            if (pFrame->pRes && pFrame->pRes != (IRes *)1)
               MtlReleaseRes(pFrame->pRes);
         }
         delete [] (sMtlFrame *)pLayer->pFrames;
         pLayer->pFrames = NULL;
         pLayer->pBitmap = NULL;
         return;
      }
      MtlReleaseRes((IRes *)pLayer->pFrames);
      pLayer->pFrames = NULL;
   }
   pLayer->pBitmap = NULL;
}

EXTERN BOOL g_bMtlTexLoadDither;
EXTERN BOOL g_bMtlTexLoadNoMip;
EXTERN BOOL g_bMtlTexLoadNoPal;
int g_nTexEdgePadding;
BOOL g_bMaterialsEnabled = TRUE;

BOOL MtlLayerLoadTexture(sMtlTexLayer *pLayer, const char *pszName)
{
   if (!stricmp(pszName, "$TEXTURE"))
   {
      MtlLayerFreeTextures(pLayer);
      pLayer->flags |= kMtlLayerTexture;
      return TRUE;
   }
   if (!stricmp(pszName, "$LIGHTMAP"))
   {
      MtlLayerFreeTextures(pLayer);
      pLayer->flags |= kMtlLayerLightmap;
      return TRUE;
   }
   AutoAppIPtr(ResMan);
   IRes *pRes = pResMan->Bind(pszName, RESTYPE_IMAGE, NULL, NULL, 0);
   if (pRes)
   {
      MtlLayerFreeTextures(pLayer);
      BOOL bSave = g_bMtlTexLoadDither;
      g_bMtlTexLoadDither = FALSE;
      grs_bitmap *pBm = (grs_bitmap *)pRes->Lock();
      g_bMtlTexLoadDither = bSave;
      pLayer->pBitmap = pBm;
      if (pBm)
      {
         pLayer->pFrames = pRes;
         if (!(pBm->flags & BMF_NO_MIPMAP))
            pBm->flags |= BMF_MIPMAP;
         return TRUE;
      }
      pRes->Release();
   }
   return FALSE;
}

BOOL MtlLoadFrame(IResMan *pResMan, const char *pszName, sMtlFrame *pFrame)
{
   if (!stricmp(pszName, "$TEXTURE"))
   {
      pFrame->pBitmap = NULL;
      pFrame->pRes = NULL;
      return TRUE;
   }
   if (!stricmp(pszName, "$LIGHTMAP"))
   {
      pFrame->pBitmap = NULL;
      pFrame->pRes = (IRes *)1;
      return TRUE;
   }
   IRes *pRes = pResMan->Bind(pszName, RESTYPE_IMAGE, NULL, NULL, 0);
   if (pRes)
   {
      g_bMtlTexLoadDither = FALSE;
      grs_bitmap *pBm = (grs_bitmap *)pRes->Lock();
      pFrame->pBitmap = pBm;
      g_bMtlTexLoadDither = FALSE;
      if (pBm)
      {
         pFrame->pRes = pRes;
         if (!(pBm->flags & BMF_NO_MIPMAP))
            pBm->flags |= BMF_MIPMAP;
         return TRUE;
      }
      pRes->Release();
   }
   return FALSE;
}

BOOL MtlLayerLoadLUT(const char *pszName, sMtlTexLayer *pLayer, int n)
{
   AutoAppIPtr(ResMan);
   BOOL bSaveNoMip = g_bMtlTexLoadNoMip;
   BOOL bSaveNoPal = g_bMtlTexLoadNoPal;
   g_bMtlTexLoadNoMip = TRUE;
   g_bMtlTexLoadNoPal = TRUE;
   int nSavePad = g_nTexEdgePadding;
   g_nTexEdgePadding = 0;
   BOOL bSaveDither = g_bMtlTexLoadDither;
   g_bMaterialsEnabled = FALSE;
   g_bMtlTexLoadDither = FALSE;
   IRes *pRes = pResMan->Bind(pszName, RESTYPE_IMAGE, NULL, NULL, 0);
   grs_bitmap *pBm = NULL;
   if (pRes)
      pBm = (grs_bitmap *)pRes->Lock();
   g_bMaterialsEnabled = TRUE;
   g_bMtlTexLoadDither = bSaveDither;
   g_bMtlTexLoadNoPal = bSaveNoPal;
   g_bMtlTexLoadNoMip = bSaveNoMip;
   g_nTexEdgePadding = nSavePad;
   if (!pRes)
      return FALSE;
   if (pLayer->apLUT[n])
   {
      MtlReleaseRes(pLayer->apLUTRes[n]);
      pLayer->apLUTRes[n] = NULL;
      pLayer->apLUT[n] = NULL;
   }
   pLayer->apLUT[n] = pBm;
   if (!pBm)
      return FALSE;
   if (!(pBm->flags & BMF_NO_MIPMAP) && pBm->type == BMT_FLAT32 && pBm->w > 1)
   {
      pLayer->apLUTRes[n] = pRes;
      return TRUE;
   }
   pLayer->apLUT[n] = NULL;
   pRes->Unlock();
   pRes->Drop();
   return FALSE;
}

BOOL g_bLoadLowPrioMaterials;
int g_nTexMaxEdgePadding = 1;
short g_aMtlNoise[512];

void MtlConfigInit(void)
{
   int n;
   g_bMaterialsEnabled = !config_is_defined("disable_material_loading");
   g_bLoadLowPrioMaterials = config_is_nonzero("load_lowprio_materials");
   n = 1;
   if (config_get_value("tex_edge_padding", CONFIG_INT_TYPE, (config_valptr)&g_nTexEdgePadding, &n) && n > 0)
   {
      if (g_nTexEdgePadding < 0)
         g_nTexEdgePadding = 0;
      else if (g_nTexEdgePadding > 2)
         g_nTexEdgePadding = 2;
   }
   n = 1;
   if (config_get_value("tex_max_edge_padding", CONFIG_INT_TYPE, (config_valptr)&g_nTexMaxEdgePadding, &n) && n > 0
       && g_nTexMaxEdgePadding < 1)
      g_nTexMaxEdgePadding = 1;
   ulong seed = 0;
   for (int i = 0; i < 512; i++)
   {
      seed = seed * 1664525 + 12345;
      g_aMtlNoise[i] = (short)(seed >> 1);
   }
}

static const char *g_apszMtlWaveNames[] =
{
   "SINE", "ABS_SINE", "TRIANGLE", "SQUARE", "SAWTOOTH", "INV_SAWTOOTH", "TURB", NULL
};

#define kMtlTurbMaxPhase 256.0

static char *MtlNextArg(char **pp)
{
   MtlSkipSpace(pp);
   char *p = *pp;
   MtlEndToken(pp);
   return p;
}

BOOL MtlParseWave(char **pp, sMtlWave *pWave, const char *pszParam, const char *pszMaterial)
{
   MtlSkipSpace(pp);
   char *pszType = *pp;
   MtlEndToken(pp);
   MtlTrim(pp);
   char *pszAmp = MtlNextArg(pp);
   char *pszBase = MtlNextArg(pp);
   char *pszPhase = MtlNextArg(pp);
   char *pszFreq = MtlNextArg(pp);
   MtlTrim(pp);

   int type = MtlLookupName(g_apszMtlWaveNames, pszType);
   if (type < 0)
   {
      mprintf("WARNING: skipping unknown wave function type '%s' for 'render_pass' param '%s' in material '%s'\n",
              pszType, pszParam, pszMaterial);
      return FALSE;
   }
   int period = atoi(pszFreq);
   if (period == 0)
   {
      mprintf("WARNING: frequency is 0, skipping '%s' wave function for 'render_pass' param '%s' in material '%s'\n",
              pszType, pszParam, pszMaterial);
      return FALSE;
   }
   double phase = atof(pszPhase);
   double maxPhase = (type == kMtlWaveTurb) ? kMtlTurbMaxPhase : 1.0;
   if (phase <= 0.0 || phase > maxPhase)
      phase = 0.0;

   pWave->type = type;
   pWave->period = period;
   pWave->phase = (int)((unsigned)period * phase + 0.5);
   pWave->base = (float)atof(pszBase);
   pWave->amplitude = (float)atof(pszAmp);
   pWave->stepScale = 0;
   pWave->steps = 0;
   if (type == kMtlWaveSquare)
      return TRUE;

   int nSteps = **pp ? atoi(*pp) : 0;
   int nAbs = abs(nSteps);
   pWave->steps = (float)nAbs;
   if (nAbs < 2)
   {
      pWave->steps = 0;
      pWave->stepScale = 0;
      return TRUE;
   }
   pWave->stepScale = (float)nAbs;
   switch (type)
   {
      case kMtlWaveSine:
      case kMtlWaveAbsSine:
      case kMtlWaveTriangle:
         pWave->phase += ((nAbs * 2 - 1) * pWave->phase) / (unsigned)(nAbs * 2);
      case kMtlWaveTurb:
         if (nSteps < 1)
            pWave->stepScale = (float)(nAbs - 1);
         break;
      case kMtlWaveSawtooth:
      case kMtlWaveInvSawtooth:
         if (nSteps >= 1)
            pWave->stepScale = (float)(nAbs + 1);
         break;
   }
   return TRUE;
}

static const char *g_apszMtlLayerAniModes[] = { "NORMAL", "REVERSE", "RAND", "PINGPONG", NULL };

static void MtlSkipLine(char **pp)
{
   while (**pp && **pp != '\r' && **pp != '\n')
      (*pp)++;
   while (**pp == '\r' || **pp == '\n')
      (*pp)++;
}

static char *MtlCutLine(char *p)
{
   while (*p && *p != '\r' && *p != '\n')
      p++;
   if (*p)
      *p++ = '\0';
   return p;
}

static int MtlRateToPeriod(float rate)
{
   return (int)(1000.0 / rate);
}

static BOOL MtlLayerLoadAniTextures(sMtlTexLayer *pLayer, char **pp, const char *pszMaterial)
{
   sMtlFrame frames[kMtlMaxAniFrames];
   int nFrames = 0;
   char *pszTok;

   MtlSkipSpace(pp);
   if (**pp == '{')
   {
      BOOL bWarned = FALSE;
      (*pp)++;
      MtlSkipSpace(pp);
      IResMan *pResMan = AppGetObj(IResMan);
      sMtlFrame *pFrame = frames;
      while ((pszTok = MtlNextToken(pp)) != NULL && *pszTok && *pszTok != '}')
      {
         if (nFrames < kMtlMaxAniFrames)
         {
            pFrame->pBitmap = NULL;
            pFrame->pRes = NULL;
            if (MtlLoadFrame(pResMan, pszTok, pFrame))
            {
               nFrames++;
               pFrame++;
            }
            else
               mprintf("WARNING: 'texture' param in 'render_pass' block in material '%s' failed to load animation frame \"%s\"\n",
                       pszMaterial, pszTok);
         }
         else if (!bWarned)
         {
            bWarned = TRUE;
            mprintf("WARNING: 'texture' param in 'render_pass' block in material '%s' exceeds maximum number of ani frames (%d)\n",
                    pszMaterial, kMtlMaxAniFrames);
         }
      }
      SafeRelease(pResMan);
   }
   else if (**pp == '*')
   {
      (*pp)++;
      BOOL bUnderscore = (**pp == '_');
      if (bUnderscore)
         (*pp)++;
      MtlSkipSpace(pp);
      char *pszCount = *pp;
      MtlEndToken(pp);
      MtlTrim(pp);
      MtlTrim(pp);
      if (pLayer->pBitmap || (pLayer->flags & (kMtlLayerTexture | kMtlLayerLightmap)))
      {
         mprintf("WARNING: skipping multiple 'texture' params in 'render_pass' block in material '%s'\n", pszMaterial);
         return TRUE;
      }
      int nCount = atoi(pszCount);
      if (nCount == 1 || nCount < 0)
         return FALSE;
      if (nCount == 0 || nCount > kMtlMaxAniFrames)
         nCount = kMtlMaxAniFrames;
      char szPattern[256];
      char szName[256];
      const char *pszExt;
      int nIndex = MtlMakeFramePattern(szPattern, *pp, bUnderscore, &pszExt);
      IResMan *pResMan = AppGetObj(IResMan);
      for (; nFrames < nCount; nFrames++, nIndex++)
      {
         const char *pszName = *pp;
         frames[nFrames].pBitmap = NULL;
         frames[nFrames].pRes = NULL;
         if (nFrames)
         {
            sprintf(szName, szPattern, nIndex, pszExt);
            pszName = szName;
         }
         if (!MtlLoadFrame(pResMan, pszName, &frames[nFrames]))
         {
            if (nFrames == 0)
               mprintf("WARNING: 'texture' param in 'render_pass' block in material '%s' failed to load ani texture \"%s\"\n",
                       pszMaterial, pszName);
            break;
         }
      }
      SafeRelease(pResMan);
   }
   else
      return FALSE;

   if (nFrames == 1)
   {
      if (frames[0].pRes == NULL)
      {
         pLayer->flags |= kMtlLayerTexture;
         pLayer->pBitmap = NULL;
         pLayer->pFrames = NULL;
      }
      else if (frames[0].pRes == (IRes *)1)
      {
         pLayer->pBitmap = NULL;
         pLayer->pFrames = NULL;
         pLayer->flags |= kMtlLayerLightmap;
      }
      else
      {
         pLayer->pBitmap = frames[0].pBitmap;
         pLayer->pFrames = frames[0].pRes;
      }
   }
   else if (nFrames > 1)
   {
      pLayer->flags |= kMtlLayerAnimated;
      if (pLayer->aniRate == 0)
         pLayer->aniRate = kMtlDefAniRate;
      pLayer->nFrames = nFrames;
      pLayer->pFrames = new sMtlFrame[nFrames];
      memcpy(pLayer->pFrames, frames, nFrames * sizeof(sMtlFrame));
      pLayer->pBitmap = frames[0].pBitmap;
      if (frames[0].pRes == NULL)
         pLayer->flags |= kMtlLayerTexture;
      else if (frames[0].pRes == (IRes *)1)
         pLayer->flags |= kMtlLayerLightmap;
   }
   return TRUE;
}

struct sMtlScroll
{
   int uPeriod;
   int vPeriod;
   float uSteps;
   float vSteps;
};

struct sMtlSpin
{
   int period;
   float steps;
};

struct sMtlIncidence
{
   float scale;
};

static void MtlParseScroll(sMtlTexLayer *pLayer, char **pp)
{
   float uRate, vRate;
   int uSteps, vSteps;
   MtlParseFloat2(pp, &uRate, &vRate);
   MtlParseInt2(pp, &uSteps, &vSteps);
   uSteps = abs(uSteps);
   vSteps = abs(vSteps);
   sMtlScroll *pScroll = (sMtlScroll *)&pLayer->waves[kMtlWaveUOffset];
   pScroll->uPeriod = (uRate == 0.0) ? 0 : MtlRateToPeriod(uRate);
   pScroll->vPeriod = (vRate == 0.0) ? 0 : MtlRateToPeriod(vRate);
   pScroll->uSteps = (pScroll->uPeriod && uSteps > 1) ? (float)uSteps : 0.0f;
   pScroll->vSteps = (pScroll->vPeriod && vSteps > 1) ? (float)vSteps : 0.0f;
   pLayer->uvFunc[2] = (pScroll->uPeriod != 0);
   pLayer->uvFunc[3] = (pScroll->vPeriod != 0);
}

BOOL MtlParseRenderPass(sMaterial *pMtl, char **pp, const char *pszMaterial)
{
   MtlSkipSpace(pp);
   char *pszTok = *pp;
   MtlEndToken(pp);
   if (pszTok[0] != '{' || pszTok[1] != '\0')
   {
      mprintf("ERROR: invalid 'render_pass' syntax, expected '{' found '%s', in material '%s'\n", pszTok, pszMaterial);
      return FALSE;
   }
   if (pMtl->nLayers >= kMtlMaxLayers)
   {
      mprintf("WARNING: too many texture layers, skipping 'render_pass' in material '%s'\n", pszMaterial);
      int depth = 1;
      while (**pp && depth)
      {
         if (**pp == '{')
            depth++;
         else if (**pp == '}')
            depth--;
         (*pp)++;
      }
      return TRUE;
   }

   sMtlTexLayer *pLayer = &pMtl->layers[pMtl->nLayers++];
   BOOL bPivotSet = FALSE;
   MtlLayerInit(pLayer);
   while (**pp)
   {
      MtlSkipSpace(pp);
      if (MtlPreprocess(pp, pszMaterial) || !MtlIsActive() || **pp == '#' || (**pp == '/' && (*pp)[1] == '/'))
      {
         MtlSkipLine(pp);
         continue;
      }
      if (!**pp)
         break;
      char *pszName = *pp;
      MtlEndToken(pp);
      char *pszNext = MtlCutLine(*pp);
      if (*pszName == '}')
      {
         *pp = pszNext;
         MtlLayerFinish(pLayer);
         return TRUE;
      }
      if (!stricmp(pszName, "texture"))
      {
         if (!MtlLayerLoadAniTextures(pLayer, pp, pszMaterial))
         {
            MtlTrim(pp);
            if (pLayer->pBitmap || (pLayer->flags & (kMtlLayerTexture | kMtlLayerLightmap)))
               mprintf("WARNING: skipping multiple 'texture' params in 'render_pass' block in material '%s'\n", pszMaterial);
            else if (!MtlLayerLoadTexture(pLayer, *pp))
               mprintf("WARNING: 'texture' param in 'render_pass' block in material '%s' failed to load texture \"%s\"\n",
                       pszMaterial, *pp);
         }
         if (pLayer->pBitmap && (pLayer->pBitmap->flags & BMF_DEV_MUST_BLIT) && !bPivotSet)
         {
            pLayer->pivot[0] = 0.0;
            pLayer->pivot[1] = 0.0;
         }
      }
      else if (!stricmp(pszName, "mipmap_bias"))
      {
         pLayer->mipmapBias = atoi(*pp);
         if (pLayer->mipmapBias < -16)
            pLayer->mipmapBias = -16;
         else if (pLayer->mipmapBias > 16)
            pLayer->mipmapBias = 16;
      }
      else if (!stricmp(pszName, "ani_rate"))
      {
         pLayer->aniRate = atoi(*pp);
         if (pLayer->aniRate < 1)
            pLayer->aniRate = kMtlDefAniRate;
      }
      else if (!stricmp(pszName, "ani_mode"))
      {
         char *pszMode = MtlNextToken(pp);
         int mode = MtlLookupName(g_apszMtlLayerAniModes, pszMode);
         if (mode < 0)
            mprintf("WARNING: skipping unknown ani_mode '%s' for 'render_pass' in material '%s'\n", pszMode, pszMaterial);
         else
            pLayer->aniMode = mode;
      }
      else if (!stricmp(pszName, "shaded"))
         MtlParseFlag(pp, kMtlLayerShaded, &pLayer->flags);
      else if (!stricmp(pszName, "use_location_envtex"))
      {
         int n = atoi(*pp);
         if (n == 0)
            pLayer->flags &= ~kMtlLayerLocEnvTex;
         else
         {
            pLayer->flags |= kMtlLayerLocEnvTex;
            if (n > 1)
               pLayer->flags |= kMtlLayerLocEnvTex2;
         }
      }
      else if (!stricmp(pszName, "blend"))
      {
         char *pszSrc = MtlNextArg(pp);
         char *pszDst = MtlNextArg(pp);
         MtlParseBlendFunc(pszMaterial, pszSrc, &pLayer->srcBlend);
         MtlParseBlendFunc(pszMaterial, pszDst, &pLayer->dstBlend);
      }
      else if (!stricmp(pszName, "alpha"))
      {
         char *pszVal = MtlNextArg(pp);
         if (!stricmp(pszVal, "func"))
         {
            char *pszFunc = MtlNextToken(pp);
            if (!stricmp(pszFunc, "INCIDENCE"))
            {
               char *pszA = *pp;
               MtlEndToken(pp);
               char *pszB = MtlNextArg(pp);
               if (!MtlLayerLoadLUT(*pp, pLayer, 0))
                  mprintf("WARNING: could not load/use LUT texture '%s' for 'render_pass' param 'alpha func INCIDENCE' in material '%s'\n",
                          *pp, pszMaterial);
               else
               {
                  pLayer->alphaFunc = kMtlFuncIncidence;
                  pLayer->alpha = (float)atof(pszA);
                  float f = (float)atof(pszB);
                  ((sMtlIncidence *)&pLayer->waves[kMtlWaveAlpha])->scale = (f <= 0.0) ? 1.0f : f;
               }
            }
            else if (!stricmp(pszFunc, "WAVE"))
            {
               if (MtlParseWave(pp, &pLayer->waves[kMtlWaveAlpha], pszName, pszMaterial))
                  pLayer->alphaFunc = kMtlFuncWave;
            }
            else
               mprintf("WARNING: skipping unknown 'func' type '%s' for 'render_pass' param '%s' in material '%s'\n",
                       pszFunc, pszName, pszMaterial);
         }
         else
         {
            pLayer->alphaFunc = kMtlFuncConst;
            pLayer->alpha = (float)atof(pszVal);
         }
      }
      else if (!stricmp(pszName, "replace_alpha"))
         MtlParseFlag(pp, kMtlLayerReplaceAlpha, &pLayer->flags);
      else if (!stricmp(pszName, "rgb"))
      {
         char *pszVal = MtlNextArg(pp);
         if (!stricmp(pszVal, "func"))
         {
            char *pszFunc = MtlNextToken(pp);
            if (!stricmp(pszFunc, "INCIDENCE"))
            {
               char *pszR = MtlNextArg(pp);
               char *pszG = MtlNextArg(pp);
               char *pszB = MtlNextArg(pp);
               char *pszScale = MtlNextArg(pp);
               if (!MtlLayerLoadLUT(*pp, pLayer, 1))
                  mprintf("WARNING: could not load/use LUT texture '%s' for 'render_pass' param 'rgb func INCIDENCE' in material '%s'\n",
                          *pp, pszMaterial);
               else
               {
                  pLayer->rgbFunc = kMtlFuncIncidence;
                  pLayer->rgb[0] = (float)atof(pszR);
                  pLayer->rgb[1] = (float)atof(pszG);
                  pLayer->rgb[2] = (float)atof(pszB);
                  float f = (float)atof(pszScale);
                  ((sMtlIncidence *)&pLayer->waves[kMtlWaveRGB])->scale = (f <= 0.0) ? 1.0f : f;
               }
            }
            else if (!stricmp(pszFunc, "WAVE"))
            {
               if (MtlParseWave(pp, &pLayer->waves[kMtlWaveRGB], pszName, pszMaterial))
                  pLayer->rgbFunc = kMtlFuncWave;
            }
            else
               mprintf("WARNING: skipping unknown 'func' type '%s' for 'render_pass' param '%s' in material '%s'\n",
                       pszFunc, pszName, pszMaterial);
         }
         else
         {
            char *pszG = MtlNextArg(pp);
            char *pszB = MtlNextArg(pp);
            pLayer->rgbFunc = kMtlFuncConst;
            pLayer->rgb[0] = (float)atof(pszVal);
            pLayer->rgb[1] = (float)atof(pszG);
            pLayer->rgb[2] = (float)atof(pszB);
         }
      }
      else if (!stricmp(pszName, "uv_source"))
      {
         char *pszSrc = MtlNextArg(pp);
         MtlParseUVSource(pszMaterial, pszSrc, &pLayer->uvSource);
      }
      else if (!stricmp(pszName, "uv_clamp"))
      {
         ulong f = 0;
         MtlParseFlag(pp, 1, &f);
         pLayer->bUVClamp = (f != 0);
      }
      else if (!stricmp(pszName, "uv_mod"))
      {
         char *pszFunc = MtlNextArg(pp);
         if (!stricmp(pszFunc, "SCALE"))
            MtlParseFloat2(pp, &pLayer->scale[0], &pLayer->scale[1]);
         else if (!stricmp(pszFunc, "PIVOT"))
         {
            MtlParseFloat2(pp, &pLayer->pivot[0], &pLayer->pivot[1]);
            bPivotSet = TRUE;
         }
         else if (!stricmp(pszFunc, "OFFSET"))
            MtlParseFloat2(pp, &pLayer->offset[0], &pLayer->offset[1]);
         else if (!stricmp(pszFunc, "ROTATE"))
         {
            MtlParseFloat(pp, &pLayer->rotation);
            pLayer->rotation = (float)(fmod((double)pLayer->rotation, 360.0) * (3.14159265358979 / 180.0));
         }
         else if (!stricmp(pszFunc, "SCROLL"))
            MtlParseScroll(pLayer, pp);
         else if (!stricmp(pszFunc, "SPIN"))
         {
            float rate;
            MtlParseFloat(pp, &rate);
            int nSteps = abs(atoi(MtlNextArg(pp)));
            sMtlSpin *pSpin = (sMtlSpin *)&pLayer->waves[kMtlWaveRotate];
            pSpin->period = (rate == 0.0) ? 0 : MtlRateToPeriod(rate);
            pSpin->steps = (pSpin->period && nSteps > 1) ? (float)nSteps : 0.0f;
            if (pSpin->period)
               pLayer->uvFunc[4] = 1;
         }
         else if (!stricmp(pszFunc, "SCALE_WAVE"))
         {
            if (MtlParseWave(pp, &pLayer->waves[kMtlWaveUScale], pszName, pszMaterial))
            {
               pLayer->uvFunc[0] = kMtlFuncWave;
               pLayer->uvFunc[1] = kMtlFuncWave;
               pLayer->waves[kMtlWaveVScale] = pLayer->waves[kMtlWaveUScale];
               pLayer->waves[kMtlWaveVScale].type = -1;
            }
         }
         else if (!stricmp(pszFunc, "USCALE_WAVE"))
         {
            if (MtlParseWave(pp, &pLayer->waves[kMtlWaveUScale], pszName, pszMaterial))
               pLayer->uvFunc[0] = kMtlFuncWave;
         }
         else if (!stricmp(pszFunc, "VSCALE_WAVE"))
         {
            if (MtlParseWave(pp, &pLayer->waves[kMtlWaveVScale], pszName, pszMaterial))
               pLayer->uvFunc[1] = kMtlFuncWave;
         }
         else if (!stricmp(pszFunc, "UOFFSET_WAVE"))
         {
            if (MtlParseWave(pp, &pLayer->waves[kMtlWaveUOffset], pszName, pszMaterial))
               pLayer->uvFunc[2] = kMtlFuncWave;
         }
         else if (!stricmp(pszFunc, "VOFFSET_WAVE"))
         {
            if (MtlParseWave(pp, &pLayer->waves[kMtlWaveVOffset], pszName, pszMaterial))
               pLayer->uvFunc[3] = kMtlFuncWave;
         }
         else if (!stricmp(pszFunc, "ROTATE_WAVE"))
         {
            if (MtlParseWave(pp, &pLayer->waves[kMtlWaveRotate], pszName, pszMaterial))
               pLayer->uvFunc[4] = kMtlFuncWave;
         }
         else
            mprintf("WARNING: skipping unknown function type '%s' for 'render_pass' param '%s' in material '%s'\n",
                    pszFunc, pszName, pszMaterial);
      }
      else
         mprintf("WARNING: skipping unknown 'render_pass' sub-param '%s' in material '%s'\n", pszName, pszMaterial);
      *pp = pszNext;
   }
   mprintf("ERROR: unexptected EOF in 'render_pass' block in material '%s'\n", pszMaterial);
   return FALSE;
}

#define kMtlMaxIncludeDepth 8

static const char *g_apszMtlAniModes[] = { "WRAP", "RAND", "REVERSE", "PINGPONG", NULL };

BOOL MtlReadFile(IRes *pRes, const char *pszName, char **ppText)
{
   *ppText = NULL;
   IStore *pStore = pRes->GetStore();
   if (!pStore)
      return FALSE;
   IStoreStream *pStream = pStore->OpenStream(pszName, 0);
   pStore->Release();
   if (!pStream)
      return FALSE;
   long nSize = pStream->GetSize();
   if (nSize)
   {
      char *pText = new char[nSize + 2];
      pStream->Read(nSize, pText);
      pStream->Close();
      pStream->Release();
      pText[nSize] = '\0';
      pText[nSize + 1] = '\0';
      *ppText = pText;
      return TRUE;
   }
   pStream->Close();
   pStream->Release();
   return FALSE;
}

static void MtlParseSize(char **pp, int *pX, int *pY)
{
   char *pszX = *pp;
   MtlEndToken(pp);
   if (**pp)
   {
      *pX = atoi(pszX);
      *pY = atoi(*pp);
   }
   else
      *pX = *pY = atoi(pszX);
}

static sMtlTexLayer *MtlAddLayer(sMaterial *pMtl, const char *pszParam, const char *pszMaterial)
{
   if (pMtl->nLayers >= kMtlMaxLayers)
   {
      mprintf("WARNING: too many texture layers, skipping '%s' in material '%s'\n", pszParam, pszMaterial);
      return NULL;
   }
   sMtlTexLayer *pLayer = &pMtl->layers[pMtl->nLayers];
   MtlLayerInit(pLayer);
   return pLayer;
}

BOOL MtlParseText(sMaterial *pMtl, char *pText, const char *pszMaterial, IRes *pRes, int depth)
{
   char *p = pText;
   while (*p)
   {
      MtlSkipSpace(&p);
      if (MtlPreprocess(&p, pszMaterial) || !MtlIsActive() || *p == '#' || (*p == '/' && p[1] == '/'))
      {
         MtlSkipLine(&p);
         continue;
      }
      if (!*p)
         return TRUE;
      char *pszName = p;
      MtlEndToken(&p);
      if (!stricmp(pszName, "render_pass"))
      {
         if (!MtlParseRenderPass(pMtl, &p, pszMaterial))
            return FALSE;
         continue;
      }
      char *pszNext = MtlCutLine(p);
      if (!stricmp(pszName, "include"))
      {
         if (depth < kMtlMaxIncludeDepth)
         {
            char *pText2;
            MtlTrim(&p);
            if (!MtlReadFile(pRes, p, &pText2))
               return FALSE;
            if (pText2)
            {
               BOOL bOk = MtlParseText(pMtl, pText2, pszMaterial, pRes, depth + 1);
               delete [] pText2;
               if (!bOk)
                  return FALSE;
            }
         }
      }
      else if (!stricmp(pszName, "legacy_chroma_key"))
         MtlParseFlag(&p, kMtlLegacyChromaKey, &pMtl->flags);
      else if (!stricmp(pszName, "chroma_key_to_alpha"))
         MtlParseFlag(&p, kMtlChromaKeyToAlpha, &pMtl->flags);
      else if (!stricmp(pszName, "force_opaque"))
         MtlParseFlag(&p, kMtlForceOpaque, &pMtl->flags);
      else if (!stricmp(pszName, "force_full_alpha"))
         MtlParseFlag(&p, kMtlForceFullAlpha, &pMtl->flags);
      else if (!stricmp(pszName, "force_alpha_key"))
         MtlParseFlag(&p, kMtlForceAlphaKey, &pMtl->flags);
      else if (!stricmp(pszName, "force_local_pal"))
         MtlParseFlag(&p, kMtlForceLocalPal, &pMtl->flags);
      else if (!stricmp(pszName, "no_mipmap"))
         MtlParseFlag(&p, kMtlNoMipmap, &pMtl->flags);
      else if (!stricmp(pszName, "uv_clamp"))
         MtlParseFlag(&p, kMtlUVClamp, &pMtl->flags);
      else if (!stricmp(pszName, "edge_padding"))
      {
         MtlParseFlag(&p, kMtlEdgePadding, &pMtl->flags);
         if (!(pMtl->flags & kMtlEdgePadding))
            pMtl->flags |= kMtlNoEdgePadding;
      }
      else if (!stricmp(pszName, "ani_rate"))
      {
         pMtl->aniRate = atoi(p);
         pMtl->flags |= kMtlAniRate;
      }
      else if (!stricmp(pszName, "ani_mode"))
      {
         ulong mode = 0;
         while (*p)
         {
            MtlSkipSpace(&p);
            char *pszMode = p;
            MtlEndToken(&p);
            int n = MtlLookupName(g_apszMtlAniModes, pszMode);
            if (n < 0)
               mprintf("WARNING: skipping unknown ani_mode '%s' in material '%s'\n", pszMode, pszMaterial);
            else
               mode |= 1 << n;
         }
         if (mode)
            pMtl->aniMode = mode & ~8;
         pMtl->flags |= kMtlAniRate;
      }
      else if (!stricmp(pszName, "ani_frames"))
      {
         pMtl->aniFrames = atoi(p);
         if (pMtl->aniFrames < 1)
            pMtl->aniFrames = -1;
         else
            pMtl->flags |= kMtlAniRate;
      }
      else if (!stricmp(pszName, "force_ani_settings"))
         MtlParseFlag(&p, kMtlForceAniSettings, &pMtl->flags);
      else if (!stricmp(pszName, "tile_factor"))
      {
         int x, y;
         MtlParseSize(&p, &x, &y);
         if (abs(x) < 1 || abs(x) > 4096 || abs(y) < 1 || abs(y) > 4096)
            mprintf("WARNING: invalid tile_factor '%dx%d' in material '%s'\n", x, y, pszMaterial);
         else
         {
            pMtl->tileFactor[0] = x;
            pMtl->tileFactor[1] = y;
         }
      }
      else if (!stricmp(pszName, "terrain_scale"))
      {
         int x, y;
         MtlParseSize(&p, &x, &y);
         if ((unsigned)(x - 1) < 4096 && (unsigned)(y - 1) < 4096)
         {
            pMtl->flags |= kMtlTerrainScale;
            pMtl->terrainScale[0] = x;
            pMtl->terrainScale[1] = y;
         }
         else
            mprintf("WARNING: invalid terrain_scale '%dx%d' in material '%s'\n", x, y, pszMaterial);
      }
      else if (!stricmp(pszName, "render_material_only"))
         MtlParseFlag(&p, kMtlRenderMtlOnly, &pMtl->flags);
      else if (!stricmp(pszName, "env_map"))
      {
         sMtlTexLayer *pLayer = MtlAddLayer(pMtl, pszName, pszMaterial);
         if (pLayer)
         {
            int mode = 0;
            char *pszAlpha = p;
            MtlEndToken(&p);
            char *pszScale = p;
            MtlEndToken(&p);
            if (*p == '*')
            {
               mode = 1;
               p++;
               MtlTrim(&p);
               if (!*p)
                  mode = 2;
            }
            else
               MtlTrim(&p);
            float alpha = (float)atof(pszAlpha);
            float scale = (float)atof(pszScale);
            if (!MtlLayerLoadTexture(pLayer, p))
            {
               memset(pLayer, 0, sizeof(*pLayer));
               mprintf("WARNING: 'env_map' param in material '%s' failed to load texture \"%s\"\n", pszMaterial, p);
            }
            else
            {
               pMtl->nLayers++;
               pLayer->flags = 0;
               if (mode)
               {
                  pLayer->flags = kMtlLayerLocEnvTex;
                  if (mode == 2)
                     pLayer->flags = kMtlLayerLocEnvTex | kMtlLayerLocEnvTex2;
               }
               pLayer->alpha = alpha;
               pLayer->dstBlend = 1;
               pLayer->scale[0] = scale;
               pLayer->uvSource = 2;
               pLayer->scale[1] = scale;
               pLayer->srcBlend = 2;
               MtlLayerFinish(pLayer);
            }
         }
      }
      else if (!stricmp(pszName, "illum_map"))
      {
         sMtlTexLayer *pLayer = MtlAddLayer(pMtl, pszName, pszMaterial);
         if (pLayer)
         {
            char *pszAlpha = p;
            MtlEndToken(&p);
            MtlTrim(&p);
            float alpha = (float)atof(pszAlpha);
            if (!MtlLayerLoadTexture(pLayer, p))
            {
               memset(pLayer, 0, sizeof(*pLayer));
               mprintf("WARNING: 'illum_map' param in material '%s' failed to load texture \"%s\"\n", pszMaterial, p);
            }
            else
            {
               pMtl->nLayers++;
               pLayer->alpha = alpha;
               pLayer->dstBlend = 1;
               pLayer->flags = 0;
               pLayer->uvSource = 0;
               pLayer->srcBlend = 2;
               MtlLayerFinish(pLayer);
            }
         }
      }
      else
         mprintf("WARNING: skipping unknown param '%s' in material '%s'\n", pszName, pszMaterial);
      p = pszNext;
   }
   return TRUE;
}

void MtlMaterialInit(sMaterial *pMtl)
{
   memset(pMtl, 0, sizeof(*pMtl));
   pMtl->aniFrames = -1;
   pMtl->tileFactor[0] = 1;
   pMtl->tileFactor[1] = 1;
}

sMaterial *MtlLoadForRes(IRes *pRes)
{
   char szName[260];
   strcpy(szName, pRes->GetName());
   char *pExt = strrchr(szName, '.');
   if (pExt)
      strcpy(pExt + 1, "mtl");
   else
      strcat(szName, ".mtl");

   char *pText;
   if (!MtlReadFile(pRes, szName, &pText) || !pText)
      return NULL;

   sMaterial *pMtl = new sMaterial;
   MtlMaterialInit(pMtl);
   MtlParseText(pMtl, pText, szName, pRes, 0);
   delete [] pText;

   if ((pMtl->flags & kMtlForceAniSettings) && !(pMtl->flags & kMtlAniRate))
      pMtl->flags &= ~kMtlForceAniSettings;
   if (pMtl->nLayers)
   {
      pMtl->flags |= kMtlHasLayers;
      for (int i = 0; i < pMtl->nLayers; i++)
         if (pMtl->layers[i].flags & kMtlLayerAnimated)
            pMtl->flags |= kMtlAnimated;
   }
   if ((pMtl->flags & kMtlRenderMtlOnly) && !(pMtl->flags & kMtlHasLayers))
      pMtl->flags &= ~kMtlRenderMtlOnly;
   return pMtl;
}

template <class T, ulong alphaMask>
void MtlEdgePad(grs_bitmap *pBm, int nPasses)
{
   int w = pBm->w;
   int h = pBm->h;
   T *pBits = (T *)pBm->bits;
   uchar *pMark = (uchar *)temp_malloc(w * h);
   int nEmpty = 0;
   int x, y, pass;

   for (y = 0; y < h; y++)
      for (x = 0; x < w; x++)
      {
         BOOL bOpaque = (pBits[y * w + x] & alphaMask) != 0;
         pMark[y * w + x] = bOpaque;
         if (!bOpaque)
            nEmpty++;
      }

   for (pass = 1; nEmpty && pass < nPasses; pass++)
   {
      for (x = 0; x < w && nEmpty; x++)
         for (y = 1; y < h && nEmpty; y++)
         {
            int i = y * w + x;
            if (pMark[i - w] && !pMark[i])
            {
               pBits[i] = pBits[i - w] & (T)~alphaMask;
               pMark[i] = 1;
               nEmpty--;
            }
            else if (!pMark[i - w] && pMark[i])
            {
               pBits[i - w] = pBits[i] & (T)~alphaMask;
               pMark[i - w] = 1;
               nEmpty--;
            }
         }
      for (y = 0; y < h && nEmpty; y++)
         for (x = 1; x < w && nEmpty; x++)
         {
            int i = y * w + x;
            if (pMark[i - 1] && !pMark[i])
            {
               pBits[i] = pBits[i - 1] & (T)~alphaMask;
               pMark[i] = 1;
               nEmpty--;
            }
            else if (!pMark[i - 1] && pMark[i])
            {
               pBits[i - 1] = pBits[i] & (T)~alphaMask;
               pMark[i - 1] = 1;
               nEmpty--;
            }
         }
   }
   temp_free(pMark);
}

void MtlTexEdgePad(sTexBitmap *pTex)
{
   grs_bitmap *pBm = &pTex->bm;
   if (pTex->texFlags & (kTexBmPadded | kTexBmNoPad))
      return;
   sMaterial *pMtl = pTex->pMtl;
   if (pMtl && (pMtl->flags & kMtlNoEdgePadding))
   {
      pTex->texFlags |= kTexBmPadded;
      return;
   }
   if (pBm->w <= 4 && pBm->h <= 4)
      return;
   BOOL bForced = pMtl && (pMtl->flags & kMtlEdgePadding);
   if (bForced || g_nTexEdgePadding == 2)
   {
      if (!(pBm->flags & BMF_TRANS))
         return;
   }
   else
   {
      if (g_nTexEdgePadding != 1 || !(pBm->flags & BMF_TRANS) || (pBm->flags & 0xF00))
         return;
   }
   if (!bForced && (pBm->w > 256 || pBm->h > 256))
      return;

   int nLog = 0;
   for (short w = pBm->w, h = pBm->h; w | h; w >>= 1, h >>= 1)
      nLog++;
   nLog -= 4;
   if (nLog < 0)
      nLog = 0;
   int nPasses = 1 << nLog;
   if (nPasses >= g_nTexMaxEdgePadding)
      nPasses = g_nTexMaxEdgePadding;

   switch (pBm->type)
   {
      case BMT_FLAT8:
         if ((pBm->flags & 0xF00) == BMF_RGB_1232)
         {
            MtlEdgePad<uchar, 0x80>(pBm, nPasses);
            pTex->texFlags |= kTexBmPadded;
         }
         break;
      case BMT_FLAT16:
         if ((pBm->flags & 0xF00) == BMF_RGB_1555)
         {
            MtlEdgePad<ushort, 0x8000>(pBm, nPasses);
            pTex->texFlags |= kTexBmPadded;
         }
         else if ((pBm->flags & 0xF00) == BMF_RGB_4444)
         {
            MtlEdgePad<ushort, 0xF000>(pBm, nPasses);
            pTex->texFlags |= kTexBmPadded;
         }
         break;
      case BMT_FLAT32:
         MtlEdgePad<ulong, 0xFF000000>(pBm, nPasses);
         pTex->texFlags |= kTexBmPadded;
         break;
   }
}

void MtlMaterialUnloadTextures(sMaterial *pMtl)
{
   if (!pMtl || !(pMtl->flags & kMtlHasLayers))
      return;
   for (int i = 0; i < pMtl->nLayers; i++)
   {
      sMtlTexLayer *pLayer = &pMtl->layers[i];
      if (!pLayer->pFrames)
         continue;
      if (pLayer->nFrames < 2)
      {
         grs_bitmap *pBm = pLayer->pBitmap;
         if (pBm && !(pBm->flags & BMF_NO_MIPMAP) && g_tmgr)
            g_tmgr->unload_texture(pBm);
      }
      else
      {
         sMtlFrame *pFrames = (sMtlFrame *)pLayer->pFrames;
         for (int j = 0; j < pLayer->nFrames; j++)
         {
            if (pFrames[j].pRes && pFrames[j].pRes != (IRes *)1)
            {
               grs_bitmap *pBm = pFrames[j].pBitmap;
               if (pBm && !(pBm->flags & BMF_NO_MIPMAP) && g_tmgr)
                  g_tmgr->unload_texture(pBm);
            }
         }
      }
   }
}

void MtlMaterialFree(sMaterial *pMtl)
{
   if (pMtl->pBitmap)
      ((sTexBitmap *)pMtl->pBitmap)->pMtl = NULL;
   for (int i = 0; i < pMtl->nLayers; i++)
   {
      sMtlTexLayer *pLayer = &pMtl->layers[i];
      MtlLayerFreeTextures(pLayer);
      for (int j = 0; j < 2; j++)
         if (pLayer->apLUTRes[j])
            MtlReleaseRes(pLayer->apLUTRes[j]);
   }
}

#ifndef __SUBTITLE_H
#define __SUBTITLE_H

#undef bool
#include <string>
#include <map>
#include <list>
#define bool sbool

#include <lg.h>
#include <storeapi.h>

enum eSubtitleFlags
{
   kSubShown     = 0x00001,
   kSubSingleton = 0x10000,
   kSub2D        = 0x20000,
   kSub3D        = 0x40000,
   kSubNoVolume  = 0x80000
};

#define SUB_COLOR_SET     0x1000000
#define SUB_COLOR_DEFAULT (SUB_COLOR_SET | 0xffffff)

class cSubtitleObj
{
public:
   virtual ~cSubtitleObj() {}
};

class cSubtitleText : public cSubtitleObj
{
public:
   cSubtitleText(const char *pText, ulong color)
      : m_color(color)
   {
      if (pText)
         m_text = pText;
   }

   virtual std::string GetText() { return m_text; }
   virtual ulong GetColor()      { return m_color; }

private:
   std::string m_text;
   ulong m_color;
};

class cSubtitle : public cSubtitleObj
{
public:
   cSubtitle(const char *pDescr, const char *pText, ulong color, int length, ulong flags, uint time, int maxDist);
   virtual ~cSubtitle();

   virtual const std::string &GetDescr() { return m_descr; }
   virtual cSubtitleText &GetText()      { return m_text; }

   std::string m_descr;
   cSubtitleText m_text;
   int m_maxDist;
   int m_length;
   ulong m_flags;
   uint m_time;
   std::list<cSubtitle *> m_parts;
};

class cSubtitleStore : public cSubtitleObj
{
public:
   virtual ~cSubtitleStore();

   virtual bool IsEmpty() { return m_subs.empty(); }
   virtual void Add(const std::string &name, cSubtitle *pSub);
   virtual cSubtitle *Find(const char *pName);

private:
   typedef std::map<std::string, cSubtitle *> tSubMap;

   tSubMap m_subs;
};

extern cSubtitleStore *g_pSubtitles;

void SubtitlesLoad(ISearchPath *pPath);
extern "C" void SubtitlesInit();
extern "C" void SubtitlesTerm();

#endif

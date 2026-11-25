/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/txtrpal.c,v 1.16 2000/02/19 13:13:39 toml Exp $

#include <windows.h>
#include <d3d9.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <mprintf.h>
#include <dev2d.h>
#include <gadbox.h>
#include <gadblist.h>
#include <config.h>
#include <appagg.h>
#include <dynfunc.h>
#include <hashset.h>
#include <hshsttem.h>
#include <str.h>
#include <storeapi.h>
#include <resapi.h>
#include <imgrstyp.h>

#include <vumanui.h>
#include <vmwincfg.h>
#include <editbr.h>
#include <editbr_.h>
#include <brlist.h>
#include <gedit.h>
#include <cmdbutts.h>
#include <command.h>
#include <txtrpal.h>
#include <txtpnp.h>
#include <texmem.h>
#include <textarch.h>
#include <family.h>
#include <family_.h>
#include <anim_txt.h>
#include <_animtxt.h>
#include <resdata.h>
#include <palmgr.h>
#include <wrdb.h>
#include <wrdbrend.h>
#include <wrfunc.h>
#include <gen_bind.h>
#include <wd3dhw2d.h>
#include <resmtl.h>
#include <darktexq.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

EXTERN BOOL drawTexturetoCanvas(int mt, editBrush *br, BOOL active);
EXTERN ISearchPath *gContextPath;
EXTERN uchar kbd_country;
EXTERN BOOL vmSoloed;
void ResRefreshStorage();

static struct _TextPal
{
   LGadRoot* root;  // grabbed from view manager
   LGadButtonList list;
   DrawElement* elems;
   int *xtra;       // to hold callback data, for now
   Rect* rects;
   TexturePalCall update;
   int numbutts;
} TextPal;

BOOL g_bNewTexPal;
static BOOL g_bNewTexPalOpen;
static BOOL phoenix_rising=FALSE;
static BOOL g_bTexPalWasSoloed;

#define TP_EXTRA_BUTTONS 4
#define TP_MAX_TEXTURES  WATERIN_IDX
#define TP_DONE_BUILD_INDEX(palptr)   ((palptr)->numbutts-5+TPAL_XTRA_DONE)
#define TP_DETACH_BUILD_INDEX(palptr) ((palptr)->numbutts-5+TPAL_XTRA_DETACH)
#define TP_REMOVE_BUILD_INDEX(palptr) ((palptr)->numbutts-5+TPAL_XTRA_REMOVE)
#define TP_SKY_BUILD_INDEX(palptr)    ((palptr)->numbutts-5+TPAL_XTRA_SKY)

#define TexIdxValid(idx) (texmemValidIdx(idx) || ((idx) >= WATERIN_IDX && (idx) <= WATERIN_IDX + 8))

static int TexPalButtonCount(void)
{
   return min(texmemGetMax(), TP_MAX_TEXTURES) + TP_EXTRA_BUTTONS;
}

////////////////////////////////////////

static int LGAPI texpal_woe(sTexPalCmd *)
{
   CriticalMsg("Could not load dialog!");
   return -1;
}

DeclDynFunc_(int, LGAPI, TexturePalette, (sTexPalCmd *));
ImplDynFunc(TexturePalette, "darkdlgs.dll", "_TexturePalette@4", texpal_woe);

#define TexturePalette (DynFunc(TexturePalette).GetProcAddress())

////////////////////////////////////////

class cTexNameSet : public cStrIHashSet<cAnsiStr *>
{
public:
   ~cTexNameSet()
   {
      DestroyAll();
   }

   tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)(const char *)*((cAnsiStr *)p);
   }
};

////////////////////////////////////////

class cDarkTexQuery : public IDarkTexQuery
{
public:
   cDarkTexQuery()
      : m_pfnUpdate(NULL), m_nLastCount(-1), m_bThumbHW(FALSE)
   {
   }

   virtual void FreeMem(void *p);
   virtual int GetNumTextures();
   virtual BOOL GetTexture(int idx, sDarkTexInfo *pInfo);
   virtual BOOL GetTextureAnim(int idx, sDarkTexAnimInfo *pInfo);
   virtual uchar *GetTexturePalette(int idx);
   virtual BOOL IsTextureBitmap(int idx, grs_bitmap *bm);
   virtual BOOL RenderTexture(int idx, int w, int h, uchar *pBits, int row);
   virtual void Select(int val);
   virtual void Detach();
   virtual void Close();
   virtual BOOL ExecCommand(const char *pszCmd, const char *pszArgs);
   virtual void CountTextureUse(int *pTable, int nSize, int flags);
   virtual const char *GetCurrentFaceName();
   virtual void EnumFamilies(tDarkTexFamilyCallback pfnCallback, void *pData);
   virtual void EnumFamilyTextures(const char *pszFamily, tDarkTexTextureCallback pfnCallback, void *pData);
   virtual void RefreshStorage();
   virtual IStoreStream *OpenMaterial(int idx);

   TexturePalCall m_pfnUpdate;
   int m_nLastCount;
   BOOL m_bThumbHW;
};

static cDarkTexQuery g_DarkTexQuery;

void cDarkTexQuery::FreeMem(void *p)
{
   free(p);
}

int cDarkTexQuery::GetNumTextures()
{
   return texmemGetMax();
}

BOOL cDarkTexQuery::GetTexture(int idx, sDarkTexInfo *pInfo)
{
   if (!TexIdxValid(idx))
   {
      CriticalMsg("invalid idx");
      return FALSE;
   }
   pInfo->pBitmap = texmem_tm[idx];
   pInfo->pRes = texmem_hnd[idx];
   pInfo->loadid = texmem_loadid[idx];
   pInfo->archetype = BuildOneTextureArchetype(idx);
   return TRUE;
}

BOOL cDarkTexQuery::GetTextureAnim(int idx, sDarkTexAnimInfo *pInfo)
{
   if (!TexIdxValid(idx))
   {
      CriticalMsg("invalid idx");
      return FALSE;
   }
   IRes *pRes = texmem_hnd[idx];
   if (!pRes)
      return FALSE;
   ectsAnimInfo *pAnim = (ectsAnimInfo *)GetResAnimData(pRes);
   if (!pAnim)
      return FALSE;
   pInfo->nFrames = pAnim->cnt;
   pInfo->ppFrames = pAnim->frame_hnd;
   pInfo->flags = pAnim->flags;
   pInfo->rate = pAnim->rate;
   return TRUE;
}

uchar *cDarkTexQuery::GetTexturePalette(int idx)
{
   if (!TexIdxValid(idx))
      return NULL;
   return palmgr_get_pal(texmem_tm[idx]->align);
}

BOOL cDarkTexQuery::IsTextureBitmap(int idx, grs_bitmap *bm)
{
   AssertMsg(idx >= 0 && bm != NULL, "idx >= 0 && bm != NULL");
   if (!TexIdxValid(idx))
      return FALSE;
   return texmem_tm[idx] == bm;
}

BOOL cDarkTexQuery::RenderTexture(int idx, int w, int h, uchar *pBits, int row)
{
   grs_bitmap *pSrc;
   grs_bitmap bm;
   grs_canvas cnv;
   BOOL bHW;

   if (!TexIdxValid(idx))
      return FALSE;
   pSrc = texmem_tm[idx];
   if (!pSrc)
      return FALSE;
   bHW = FALSE;
   if (pSrc->type == BMT_DXT)
   {
      if (!g_pHW2D)
         return FALSE;
      bHW = TRUE;
   }
   else if (m_bThumbHW && g_pHW2D)
      bHW = TRUE;

   gr_init_bitmap(&bm, pBits, BMT_FLAT32, BMF_RGB_8888, w, h);
   bm.row = row;

   if (!bHW)
   {
      gr_make_canvas(&bm, &cnv);
      gr_push_canvas(&cnv);
      gr_scale_bitmap(pSrc, 0, 0, grd_canvas->bm.w, grd_canvas->bm.h);
      gr_pop_canvas();
      gr_close_canvas(&cnv);
      gr_close_bitmap(&bm);
      return TRUE;
   }

   if (!g_pHW2D->ReadBack(pSrc, &bm))
   {
      uchar *p = bm.bits;
      for (int y = 0; y < bm.h; y++)
      {
         memset(p, 0, bm.w * 4);
         p += bm.row;
      }
      gr_close_bitmap(&bm);
   }
   gr_close_bitmap(&bm);
   return TRUE;
}

void cDarkTexQuery::Select(int val)
{
   if (m_pfnUpdate)
      m_pfnUpdate(val);
}

void cDarkTexQuery::Detach()
{
   if (m_pfnUpdate)
   {
      AssertMsg(!txtPnP_allow_attach, "!txtPnP_allow_attach");
      m_pfnUpdate(-TPAL_XTRA_DETACH);
   }
}

void cDarkTexQuery::Close()
{
   DestroyTexturePalette();
}

BOOL cDarkTexQuery::ExecCommand(const char *pszCmd, const char *pszArgs)
{
   AssertMsg(pszCmd != NULL, "cmd != NULL");
   if (!pszArgs)
      pszArgs = "";
   Command *pCmd = CommandFind((char *)pszCmd, strlen(pszCmd));
   if (!pCmd)
      return FALSE;
   CommandExecuteParsed(pCmd, (char *)pszArgs);
   return TRUE;
}

void cDarkTexQuery::CountTextureUse(int *pTable, int nSize, int flags)
{
   int i, j;

   AssertMsg(pTable && nSize, "table && size");
   memset(pTable, 0, nSize * sizeof(int));
   if (nSize >= TEXMEM_MAX)
      nSize = TEXMEM_MAX;

   if (flags & kDarkTexCountBrushes)
   {
      int hIter;
      editBrush *br;
      for (br = blistIterStart(&hIter); br; br = blistIterNext(hIter))
      {
         if (br->media > 0 || br->media == 0)
         {
            int nDefault = 0;
            for (i = 0; i < br->num_faces; i++)
            {
               int tx = br->txs[i].tx_id;
               if (tx == -1)
                  nDefault++;
               else if ((unsigned)tx < (unsigned)nSize)
                  pTable[tx]++;
            }
            if (nDefault && (unsigned)br->tx_id < (unsigned)nSize)
               pTable[br->tx_id] += nDefault;
         }
      }
      return;
   }

   for (i = 0; i < wr_num_cells; i++)
   {
      PortalCell *pCell = wr_cell[i];
      if (!pCell)
         continue;
      for (j = 0; j < pCell->num_render_polys; j++)
      {
         int tx = pCell->render_list[j].texture_id;
         if ((unsigned)tx < (unsigned)nSize)
            pTable[tx]++;
      }
   }
}

const char *cDarkTexQuery::GetCurrentFaceName()
{
   if (txtPnP_br)
      return txtPnP_get_brush_face_name(txtPnP_br, txtPnP_br->cur_face + 1);
   return NULL;
}

static char *TexPalBaseName(char *pszPath)
{
   char *pBack = strrchr(pszPath, '\\');
   char *pFwd = strrchr(pszPath, '/');
   if (pFwd)
   {
      if (pFwd > pBack)
         return pFwd + 1;
      return pBack + 1;
   }
   if (pBack)
      return pBack + 1;
   return pszPath;
}

void cDarkTexQuery::EnumFamilies(tDarkTexFamilyCallback pfnCallback, void *pData)
{
   cTexNameSet names;
   char szName[256];
   IStore *pStore;
   IStore *pCanon;
   void *pCookie;
   cAnsiStr *pName;
   tHashSetHandle h;

   AssertMsg(pfnCallback != NULL, "callback != NULL");

   pCookie = gContextPath->BeginContents(NULL, SEARCH_STORAGES, "fam\\");
   if (pCookie)
   {
      while (gContextPath->Next(pCookie, &pStore, szName, &pCanon))
      {
         int len = strlen(szName);
         if (szName[len - 1] == '\\' || szName[len - 1] == '/')
            szName[len - 1] = '\0';
         char *pBase = TexPalBaseName(szName);
         if (strcmp(pBase, "sky") && strcmp(pBase, "skyhw")
             && strcmp(pBase, "water") && strcmp(pBase, "waterhw"))
         {
            if (!names.Search(pBase))
               names.Insert(new cAnsiStr(pBase));
         }
         SafeRelease(pCanon);
         SafeRelease(pStore);
      }
      gContextPath->EndContents(pCookie);
   }

   while ((pName = names.GetFirst(h)) != NULL)
   {
      pfnCallback(*pName, _fam_lookup_record((char *)(const char *)*pName) != NULL, pData);
      names.Remove(pName);
      delete pName;
   }
}

void cDarkTexQuery::EnumFamilyTextures(const char *pszFamily, tDarkTexTextureCallback pfnCallback, void *pData)
{
   cTexNameSet names;
   char szRelPath[64];
   char szName[256];
   IStore *pStore;
   IStore *pCanon;
   void *pCookie;
   cAnsiStr *pName;
   tHashSetHandle h;

   AssertMsg(pszFamily && pfnCallback, "family && callback");
   sprintf(szRelPath, "fam\\%s\\", pszFamily);

   AutoAppIPtr(ResMan);
   IResType *pImgType = pResMan->GetResType(RESTYPE_IMAGE);
   AssertMsg(pImgType != NULL, "pImgType != NULL");

   pCookie = gContextPath->BeginContents("*.*", 0, szRelPath);
   if (pCookie)
   {
      while (gContextPath->Next(pCookie, &pStore, szName, &pCanon))
      {
         char *pBase = TexPalBaseName(szName);
         char *pExt = strrchr(pBase, '.');
         if (pExt && pImgType->IsLegalExt(pExt))
         {
            *pExt = '\0';
            if (!names.Search(pBase) && strcmp(pBase, "full")
                && !ectsAnimTxtIgnore(pBase, szRelPath))
               names.Insert(new cAnsiStr(pBase));
         }
         SafeRelease(pCanon);
         SafeRelease(pStore);
      }
      gContextPath->EndContents(pCookie);
   }

   while ((pName = names.GetFirst(h)) != NULL)
   {
      pfnCallback(*pName, texmemGetSinglebyName(szRelPath, (char *)(const char *)*pName) != -1, pData);
      names.Remove(pName);
      delete pName;
   }
}

void cDarkTexQuery::RefreshStorage()
{
   ResRefreshStorage();
}

IStoreStream *cDarkTexQuery::OpenMaterial(int idx)
{
   char szName[256];
   char szPath[256];
   char *pPath;
   char *pDot;
   grs_bitmap *bm;
   IRes *pRes;

   if (!TexIdxValid(idx))
      return NULL;
   bm = texmem_tm[idx];
   pRes = texmem_hnd[idx];
   if (!bm || !pRes || !((sTexBitmap *)bm)->pMtl)
      return NULL;

   strcpy(szName, pRes->GetName());
   pDot = strrchr(szName, '.');
   if (pDot)
      strcpy(pDot + 1, "mtl");
   else
      strcat(szName, ".mtl");

   pPath = NULL;
   pRes->GetCanonPath(&pPath);
   strcpy(szPath, pPath ? pPath : "");
   if (pPath)
      free(pPath);

   return ResOpenConfigFile(szName, *szPath ? szPath : NULL, NULL);
}

////////////////////////////////////////

static void textpal_draw_cb(DrawElement* elem, DrawElemState state)
{
   int n=(int)elem->draw_data2;

   if ((n >= 0)&&(n<TextPal.numbutts-TP_EXTRA_BUTTONS))
      drawTexturetoCanvas(n,NULL,state==dsDEPRESSED);
   else
      gr_clear(0);
}

////////////////////////////////////////

#pragma off(unreferenced)
static void destroy_me(void* arg)
{
   DestroyTexturePalette();
}

static bool textpal_button_cb(ushort action, int button, void* data, LGadBox* vb)
{
   bool destroy = (action & LGADBUTT_LDOUBLE) != 0;
   int cb_val=TextPal.xtra[button];

   if (TpalGetExtraCode(cb_val)==TPAL_XTRA_DONE)
      destroy = TRUE;
   else
      if (action & (LGADBUTT_LCLICK|LGADBUTT_LDOUBLE))
         if (TextPal.update != NULL)
            destroy = TextPal.update(cb_val) || destroy;
   if (destroy)
      uiDefer(destroy_me,NULL);
   return TRUE;
}
#pragma on(unreferenced)

////////////////////////////////////////

#define DIMS_RATIO_X 3
#define DIMS_RATIO_Y 1

void CreateTexturePalette(TexturePalCall update)
{
   struct _TextPal* pal = &TextPal;
   int n;
   int i;

   if (g_bNewTexPal)
   {
      sTexPalCmd cmd;

      if (g_bNewTexPalOpen)
         return;
      g_DarkTexQuery.m_pfnUpdate = update;
      g_DarkTexQuery.m_bThumbHW = config_is_nonzero("new_texpal_thumb_hw");
      memset(&cmd, 0, sizeof(cmd));
      cmd.cmd = kTexPalCreate;
      cmd.arg[0] = txtPnP_texture;
      cmd.arg[1] = (int)(IDarkTexQuery *)&g_DarkTexQuery;
      cmd.arg[2] = config_is_defined("new_texpal_toplevel");
      cmd.arg[3] = (int)g_pInputBinder;
      cmd.arg[4] = kbd_country;
      g_bNewTexPalOpen = TexturePalette(&cmd) == 0;
      return;
   }

   n = TexPalButtonCount();
   if (texmemGetMax() > TP_MAX_TEXTURES)
      mprintf("WARNING: there are more textures than the old texture palette can display, use the new one\n");

   if (!phoenix_rising)
   {
      g_bTexPalWasSoloed = vmSoloed;
      if (g_bTexPalWasSoloed)
         vmToggleGUIFull();
   }

   pal->update = update;

   pal->root = vmGrabViews(vmGrabTwoWide);
   if (pal->root == NULL)
      return;

   pal->rects = (Rect *)Malloc(n*sizeof(*pal->rects));
   pal->elems = (DrawElement *)Malloc(n*sizeof(*pal->elems));
   pal->xtra  = (int *)Malloc(n*sizeof(*pal->xtra));
   pal->numbutts = n;

   for (i = 0; i < n; i++)
   {
      DrawElement* elem = &pal->elems[i];
      ElementClear(elem);
      if (i < n-TP_EXTRA_BUTTONS)
      {
         elem->draw_type  = DRAWTYPE_CALLBACK;
         elem->draw_data  = textpal_draw_cb;
         elem->draw_data2 = (void*)i;
         pal->xtra[i]=i;
      }
      else
      {
         elem->draw_type  = DRAWTYPE_TEXT;
         pal->xtra[i]=(-(1+i-(n-TP_EXTRA_BUTTONS)));

         if (i==TP_DONE_BUILD_INDEX(pal))
            elem->draw_data = "Done";
         else if (i==TP_SKY_BUILD_INDEX(pal))
            elem->draw_data = "Sky";
         else if (i==TP_REMOVE_BUILD_INDEX(pal))
            elem->draw_data = "Remove\nfrom\nLevel";
         else if (i==TP_DETACH_BUILD_INDEX(pal))
         {
            if (txtPnP_allow_attach)
               elem->draw_data = "detach\nattach";
            else
               elem->draw_data = "put on\nbrush";
         }
         else
            elem->draw_data = "Bug!\nReport\nMe";
         elem->draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
      }
   }

   // compute rectangles
   {
      Rect* box = LGadBoxRect(pal->root);
      Rect bounds = { {0,0},};
      Point ratio;
      Point dims;
      int xr = 0 ,yr = 0;
      bounds.lr = MakePoint(RectWidth(box),RectHeight(box));

      for (i = 0; i < n - TP_EXTRA_BUTTONS; i++)
      {
         grs_bitmap *bm=texmemGetTexture(i);
         xr += bm->w;
         yr += bm->h;
      }
      for (i = 0; i < TP_EXTRA_BUTTONS; i++)
       { xr += 32; yr += 32; }   // hack to try and get rid of tiny tiny palette??
      ratio.x = (short)max(RectWidth(box)*n/xr,1);
      ratio.y = (short)max(RectHeight(box)*n/yr,1);
      dims = ratio;

      // scale dimensions until we have enough
      while(dims.x * dims.y < n)
      {
         dims.x++;
         dims.y = dims.x*ratio.y/ratio.x;
      }
      LayoutRectangles(&bounds,pal->rects,n,dims,MakePoint(0,0));
   }

   // build the button list finally
   {
      LGadButtonListDesc desc;
      memset(&desc,0,sizeof(desc));
      desc.num_buttons = n;
      desc.button_rects = pal->rects;
      desc.button_elems = pal->elems;
      desc.cb = textpal_button_cb;
      desc.flags=BUTTONLIST_RADIO_FLAG;

      LGadCreateButtonListDesc(&pal->list,pal->root,&desc);
   }
   pal->numbutts = n;
   LGadDrawBox(VB(pal->root),NULL);
}

////////////////////////////////////////

void DestroyTexturePalette(void)
{
   if (g_bNewTexPal)
   {
      sTexPalCmd cmd;

      if (!g_bNewTexPalOpen)
         return;
      memset(&cmd, 0, sizeof(cmd));
      cmd.cmd = kTexPalDestroy;
      TexturePalette(&cmd);
      g_bNewTexPalOpen = FALSE;
      if (!phoenix_rising)
         txtPnP_attach();
      return;
   }

   if (TextPal.root == NULL)
      return;
   LGadHideBox(VB(TextPal.root),TRUE);
   LGadDestroyButtonList(&TextPal.list);
   Free(TextPal.elems);
   Free(TextPal.rects);
   Free(TextPal.xtra);

   vmReleaseViews(TextPal.root);
   TextPal.root = NULL;
   if (!phoenix_rising)
   {
      txtPnP_attach();     // if we got detached, reattach to brush
      if (g_bTexPalWasSoloed)
         vmToggleGUIFull();
   }
}

////////////////////////////////////////

void ToggleTexturePalette(TexturePalCall update)
{
   if (TexturePaletteVisible())
      DestroyTexturePalette();
   else
      CreateTexturePalette(update);
}

////////////////////////////////////////

bool TexturePaletteVisible(void)
{
   if (g_bNewTexPal)
      return g_bNewTexPalOpen;
   return TextPal.root != NULL;
}

////////////////////////////////////////

void TexturePaletteSelect(int texture)
{
   if (g_bNewTexPalOpen)
   {
      sTexPalCmd cmd;
      memset(&cmd, 0, sizeof(cmd));
      cmd.cmd = kTexPalSelect;
      cmd.arg[0] = texture;
      TexturePalette(&cmd);
      return;
   }
   LGadRadioButtonSelect(&TextPal.list,texture);
   LGadDrawBox(VB(TextPal.root),NULL);
}

////////////////////////////////////////

static void rise_like_phoenix(void* update)
{
   phoenix_rising=TRUE;
   DestroyTexturePalette();
   CreateTexturePalette((TexturePalCall)update);
   phoenix_rising=FALSE;
}

void TexturePaletteUpdate(GFHUpdateOp op)
{
   if (g_bNewTexPalOpen)
   {
      int i, nCount = 1;
      for (i = 1; i < texmemGetMax(); i++)
         if ((i < WATERIN_IDX || i > 255) && texmem_hnd[i])
            nCount++;
      if (texmem_hnd[WATERIN_IDX])
         nCount++;
      if (texmem_hnd[WATEROUT_IDX])
         nCount++;
      if (op != GFH_FORCE && g_DarkTexQuery.m_nLastCount == nCount)
         return;
      g_DarkTexQuery.m_nLastCount = nCount;

      sTexPalCmd cmd;
      memset(&cmd, 0, sizeof(cmd));
      cmd.cmd = kTexPalUpdate;
      cmd.arg[0] = txtPnP_texture;
      TexturePalette(&cmd);
      return;
   }

   if (TextPal.root != NULL)
   {
      if (op == GFH_FORCE || TextPal.numbutts != TexPalButtonCount())
      {
         TexturePalCall update = TextPal.update;
         uiDefer(rise_like_phoenix,update);
      }
   }
}

/////////////////////////////////////////

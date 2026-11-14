#include <dev2d.h>
#include <memall.h>
#include <tmpalloc.h>
#include "prim.h"
#include "primface.h"
#include <clipprim.h>

extern r3s_point **r3_get_vpl_from_index(int n, short *index_list);
extern r3ixs_info *r3_get_info_from_index(int n, short *index_list);
extern r3ixs_info *r3_get_clipped_info_from_index(int n, r3s_phandle *vpl, int m, r3s_phandle *cvpl, short *index_list);

int r3ix_check_clip_any(int n, short *vi, r3s_texture bm)
{
   r3s_phandle *vp, *cvp;
   int m, retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   if (r3_check_facing_n(n, vp)) {
      m = r3_raw_clip_polygon(n, vp, &cvp);
      if (m) {
         r3_force_texture(bm);
         if (vp == cvp)
            info = r3_get_info_from_index(n, vi);
         else
            info = r3_get_clipped_info_from_index(n, vp, m, cvp, vi);
         retval = r3d_glob.prim_tab.convert_poly_indexed(m, cvp, info);
         temp_free(info);
      } else
         retval = CLIP_ALL;
   }
   temp_free(vp);
   return retval;
}

int r3ix_check_any(int n, short *vi, r3s_texture bm)
{
   r3s_phandle *vp;
   int retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   if (r3_check_facing_n(n, vp)) {
      r3_force_texture(bm);
      info = r3_get_info_from_index(n, vi);
      retval = r3d_glob.prim_tab.convert_poly_indexed(n, vp, info);
      temp_free(info);
   } else
      retval = CLIP_ALL;
   temp_free(vp);
   return retval;
}

int r3ix_clip_any(int n, short *vi, r3s_texture bm)
{
   r3s_phandle *vp, *cvp;
   int m, retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   m = r3_raw_clip_polygon(n, vp, &cvp);
   if (m) {
      r3_force_texture(bm);
      if (vp == cvp)
         info = r3_get_info_from_index(n, vi);
      else
         info = r3_get_clipped_info_from_index(n, vp, m, cvp, vi);
      retval = r3d_glob.prim_tab.convert_poly_indexed(m, cvp, info);
      temp_free(info);
   } else
      retval = CLIP_ALL;
   temp_free(vp);
   return retval;
}

int r3ix_any(int n, short *vi, r3s_texture bm)
{
   r3s_phandle *vp;
   int retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   info = r3_get_info_from_index(n, vi);
   r3_force_texture(bm);
   retval = r3d_glob.prim_tab.convert_poly_indexed(n, vp, info);
   temp_free(info);
   temp_free(vp);
   return retval;
}

int r3ix_check_clip_implicit_any(int n, short *vi)
{
   r3s_phandle *vp, *cvp;
   int m, retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   if (r3_check_facing_n(n, vp)) {
      m = r3_raw_clip_polygon(n, vp, &cvp);
      if (m) {
         if (vp == cvp)
            info = r3_get_info_from_index(n, vi);
         else
            info = r3_get_clipped_info_from_index(n, vp, m, cvp, vi);
         retval = r3d_glob.prim_tab.convert_poly_indexed(m, cvp, info);
         temp_free(info);
      } else
         retval = CLIP_ALL;
   }
   temp_free(vp);
   return retval;
}

int r3ix_check_implicit_any(int n, short *vi)
{
   r3s_phandle *vp;
   int retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   if (r3_check_facing_n(n, vp)) {
      info = r3_get_info_from_index(n, vi);
      retval = r3d_glob.prim_tab.convert_poly_indexed(n, vp, info);
      temp_free(info);
   } else
      retval = CLIP_ALL;
   temp_free(vp);
   return retval;
}

int r3ix_clip_implicit_any(int n, short *vi)
{
   r3s_phandle *vp, *cvp;
   int m, retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   m = r3_raw_clip_polygon(n, vp, &cvp);
   if (m) {
      if (vp == cvp)
         info = r3_get_info_from_index(n, vi);
      else
         info = r3_get_clipped_info_from_index(n, vp, m, cvp, vi);
      retval = r3d_glob.prim_tab.convert_poly_indexed(m, cvp, info);
      temp_free(info);
   } else
      retval = CLIP_ALL;
   temp_free(vp);
   return retval;
}

int r3ix_implicit_any(int n, short *vi)
{
   r3s_phandle *vp;
   int retval;
   r3ixs_info *info;

   vp = r3_get_vpl_from_index(n, vi);
   info = r3_get_info_from_index(n, vi);
   retval = r3d_glob.prim_tab.convert_poly_indexed(n, vp, info);
   temp_free(info);
   temp_free(vp);
   return retval;
}

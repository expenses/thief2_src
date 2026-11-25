// $Header: x:/prj/tech/libsrc/r3d/RCS/primd3d.c 1.7 1998/06/30 18:45:49 buzzard Exp $

#include <dev2d.h>
#include <lgd3d.h>

#include <prim.h>
#include <primfunc.h>
#include <primcon_.h>
#include <primtab_.h>

#define lgd3d_poly_setup_null lgd3d_poly_setup
static poly_setup_func poly_lgd3d_setup_funcs[] =
{
   // 0
   lgd3d_poly_setup,         //      solid unlit
   lgd3d_poly_setup,         //      solid unlit clipped
   lgd3d_spoly_setup,        //      solid gouraud
   lgd3d_spoly_setup,        //      solid gouraud clipped
   lgd3d_rgb_poly_setup,     //      solid rgb gouraud
   lgd3d_rgb_poly_setup,     //      solid rgb gouraud clipped
   lgd3d_rgba_poly_setup,    //      solid rgba gouraud
   lgd3d_rgba_poly_setup,    //      solid rgba gouraud clipped
   lgd3d_rgba_poly_setup,    //      solid rgba fog gouraud
   lgd3d_rgba_poly_setup,    //      solid rgba fog gouraud clipped
   lgd3d_rgba_poly_setup,    //      solid diffuse specular
   lgd3d_rgba_poly_setup,    //      solid diffuse specular clipped

   // 12
   lgd3d_poly_setup_null,    //      transluc unlit
   lgd3d_poly_setup_null,    //      transluc unlit clipped
   lgd3d_poly_setup_null,    //      transluc gouraud
   lgd3d_poly_setup_null,    //      transluc gouraud clipped
   lgd3d_poly_setup_null,    //      transluc rgb gouraud
   lgd3d_poly_setup_null,    //      transluc rgb gouraud clipped
   lgd3d_poly_setup_null,    //      transluc rgba gouraud
   lgd3d_poly_setup_null,    //      transluc rgba gouraud clipped
   lgd3d_poly_setup_null,    //      transluc rgba fog gouraud
   lgd3d_poly_setup_null,    //      transluc rgba fog gouraud clipped
   lgd3d_poly_setup_null,    //      transluc diffuse specular
   lgd3d_poly_setup_null,    //      transluc diffuse specular clipped

   // 24
   lgd3d_tmap_setup,         //      texture unlit
   lgd3d_tmap_setup,         //      texture unlit clipped
   lgd3d_lit_tmap_setup,     //      texture gouraud
   lgd3d_lit_tmap_setup,     //      texture gouraud clipped
   lgd3d_rgblit_tmap_setup,  //      texture rgb gouraud
   lgd3d_rgblit_tmap_setup,  //      texture rgb gouraud clipped
   lgd3d_rgbalit_tmap_setup, //      texture rgba gouraud
   lgd3d_rgbalit_tmap_setup, //      texture rgba gouraud clipped
   lgd3d_rgbafoglit_tmap_setup,//      texture rgba fog gouraud
   lgd3d_rgbafoglit_tmap_setup,//      texture rgba fog gouraud clipped
   lgd3d_diffspecular_tmap_setup,//      texture diffuse specular
   lgd3d_diffspecular_tmap_setup,//      texture diffuse specular clipped

   // 36
   lgd3d_tmap_setup,         //      linear unlit
   lgd3d_tmap_setup,         //      linear unlit clipped
   lgd3d_lit_tmap_setup,     //      linear gouraud
   lgd3d_lit_tmap_setup,     //      linear gouraud clipped
   lgd3d_rgblit_tmap_setup,  //      linear rgb gouraud
   lgd3d_rgblit_tmap_setup,  //      linear rgb gouraud clipped
   lgd3d_rgbalit_tmap_setup, //      linear rgba gouraud
   lgd3d_rgbalit_tmap_setup, //      linear rgba gouraud clipped
   lgd3d_rgbafoglit_tmap_setup,//      linear rgba fog gouraud
   lgd3d_rgbafoglit_tmap_setup,//      linear rgba fog gouraud clipped
   lgd3d_diffspecular_tmap_setup,//      linear diffuse specular
   lgd3d_diffspecular_tmap_setup //      linear diffuse specular clipped
};

static int lgd3d_convert(int n, r3s_phandle *vp)
{
   if (r3d_do_setup) {
      r3_2d_poly_setup_func(r3d_bm);
      r3d_do_setup = FALSE;
   }
   return lgd3d_draw_poly_func(n, vp);
}

static int lgd3d_convert_indexed(int n, r3s_phandle *vp, struct r3ixs_info *info)
{
   if (r3d_do_setup) {
      r3_2d_poly_setup_func(r3d_bm);
      r3d_do_setup = FALSE;
   }
   return lgd3d_draw_poly_indexed_func(n, vp, info);
}

static int lgd3d_line(r3s_phandle p0, r3s_phandle p1)
{
   lgd3d_draw_line(p0, p1);
   return CLIP_NONE;
}

extern texture_manager *r3d_tmgr;
extern void (**ppfReleaseIP)(void);
extern void (*lgd3d_release_ip_func)(void);

void r3_use_lgd3d(void)
{
   r3_setup_prim_funcs(poly_lgd3d_setup_funcs, lgd3d_convert,
      lgd3d_convert_indexed,
      lgd3d_line, lgd3d_draw_point);
   r3d_using_lgd3d = TRUE;
   r3d_using_g2 = FALSE;
   r3_set_prim();

   // this is a really gruesome hack
   ppfReleaseIP = &lgd3d_release_ip_func;
   r3d_tmgr = g_tmgr;
}

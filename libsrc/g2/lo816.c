// $Header: x:/prj/tech/libsrc/g2/RCS/lo816.c 1.2 1998/04/28 14:31:19 KEVIN Exp $
#include <genrast.h>
#include <trirast.h>
#include <g2clip.h>
#include <tmapd.h>
#include <g2tm.h>
#include <lgassert.h>

extern g2il_func gen_flat8_lit_il;
extern g2raster_func opaque_lit_8to16_raster_asm;
extern void opaque_lit_8to16_il_init(grs_bitmap *bm);

#pragma off(unreferenced)
void opaque_lit_8to16_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;

   if (grd_ltab816_list)
      grd_ltab816 = grd_ltab816_list[bm->align];
   else {
      pixpal = (void *)(grd_pal16_list[bm->align]);
      AssertMsg(pixpal!=NULL, "opaque_lit_8to16_setup(): setting NULL palette!");
   }

   if (POW2(bm)&&(grd_ltab816!=NULL)) {
      g2d_pp.tri_func = gen_uv_triangle;
      g2d_pp.grad_func = iuv_triangle_gradients;
      g2d_pp.canvas_row = grd_bm.row;
      g2d_pp.raster_func = opaque_lit_8to16_raster_asm;
      opaque_lit_8to16_il_init(bm);
   } else {
      g2d_pp.tri_func = gen_triangle_raster;
      g2d_pp.raster_func = gen_raster_loop;
      g2d_pp.dy = 1;
      g2d_pp.pix_func = gd_upix8_expose(0,0,0);
      g2d_pp.lpix_func = gd_lpix8_expose(0,0);
      g2d_pp.inner_loop = gen_flat8_lit_il;
   }
}
#pragma on(unreferenced)

extern g2raster_func opaque_clut_lit_8to16_raster_asm;
extern void opaque_clut_lit_8to16_il_init(grs_bitmap *bm);

#pragma off(unreferenced)
void opaque_clut_lit_8to16_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;

   if (grd_ltab816_list)
      grd_ltab816 = grd_ltab816_list[bm->align];
   else {
      pixpal = (void *)(grd_pal16_list[bm->align]);
      AssertMsg(pixpal!=NULL, "opaque_lit_8to16_setup(): setting NULL palette!");
   }

   if (POW2(bm)&&(grd_ltab816!=NULL)) {
      g2d_pp.tri_func = gen_uv_triangle;
      g2d_pp.grad_func = iuv_triangle_gradients;
      g2d_pp.canvas_row = grd_bm.row;
      g2d_pp.raster_func = opaque_clut_lit_8to16_raster_asm;
      opaque_clut_lit_8to16_il_init(bm);
   } else {
      g2d_pp.tri_func = gen_triangle_raster;
      g2d_pp.raster_func = gen_raster_loop;
      g2d_pp.dy = 1;
      g2d_pp.pix_func = gd_upix8_expose(0,0,0);
      g2d_pp.lpix_func = gd_lpix8_expose(0,0);
      g2d_pp.inner_loop = gen_flat8_lit_il;
   }
}
#pragma on(unreferenced)

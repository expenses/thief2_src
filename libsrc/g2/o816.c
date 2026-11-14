

#include <genrast.h>
#include <tmapd.h>
#include <g2tm.h>
#include <lgassert.h>

extern g2il_func gen_flat8_il;
extern g2raster_func opaque_8to16_raster_asm;
extern void opaque_8to16_il_init(grs_bitmap *bm);

#pragma off(unreferenced)
void opaque_8to16_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   g2d_pp.right_edge_func = gen_right_edge;
   pixpal = (void *)(grd_pal16_list[bm->align]);
   AssertMsg(pixpal!=NULL, "opaque_8to16_setup(): setting NULL palette!");

   if (POW2(bm)) {
      g2d_pp.left_edge_func = gen_uv_left_edge;
      g2d_pp.tri_func = gen_uv_triangle;
      g2d_pp.grad_func = uv_triangle_gradients;
      g2d_pp.canvas_row = grd_bm.row;
      g2d_pp.raster_func = opaque_8to16_raster_asm;
      opaque_8to16_il_init(bm);
   } else {
      g2d_pp.left_edge_func = gen_left_edge;
      g2d_pp.tri_func = gen_triangle_raster;
      g2d_pp.inner_loop = gen_flat8_il;
      g2d_pp.pix_func = gd_upix8_expose(0,0,0);
      g2d_pp.dy = 1;
      g2d_pp.raster_func = gen_raster_loop;
   }
}
#pragma on(unreferenced)

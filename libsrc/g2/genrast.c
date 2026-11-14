// $Header: x:/prj/tech/libsrc/g2/RCS/genrast.c 1.3 1997/11/21 14:00:29 KEVIN Exp $
//
// Generic edge and rasterization functions.

#include <g2spoint.h>
#include <plyparam.h>
#include <trirast.h>
#include <tmapd.h>
#include <genrast.h>
#include <mathmac.h>

#include <lgassert.h>

g2s_point *g2d_v0, *g2d_v1, *g2d_v2;
fix *g2d_p_edge_x, *g2d_p_edge_dx;

static void float_left_edge(void);
extern void gen_right_edge_asm(void);
extern void gen_uv_left_edge_asm(void);
extern int triangle_raster_asm(void);

g2edge_func *gen_left_edge = float_left_edge;
g2edge_func *gen_right_edge = gen_right_edge_asm;
g2edge_func *gen_uv_left_edge = gen_uv_left_edge_asm;
g2tri_func *gen_uv_triangle = triangle_raster_asm;

static void fix_left_edge(void);
static void fix_right_edge(void);

void g2_set_old_poly_funcs(void)
{
   gen_uv_left_edge = fix_left_edge;
   gen_left_edge = fix_left_edge;
   gen_right_edge = fix_right_edge;
   gen_uv_triangle = gen_triangle_raster;
}

void g2_set_new_poly_funcs(void)
{
   gen_left_edge = float_left_edge;
   gen_right_edge = gen_right_edge_asm;
   gen_uv_left_edge = gen_uv_left_edge_asm;
   gen_uv_triangle = triangle_raster_asm;
}

extern void gen_calc_edge_(void);

// call once for each left edge.
static void float_left_edge(void)
{
   g2s_poly_params *tp = g2d_ppp;
   g2s_point *prev = g2d_v0;
   fix x, dx;
   float fdx, fdy;
   int i;
   ulong mask, flags;

   g2d_p_edge_x = &x;
   g2d_p_edge_dx = &dx;
   gen_calc_edge_();

   fdx = fix_float(fix_floor(x) - prev->sx);
   fdy = fix_float(fix_ceil(prev->sy) - prev->sy);
   tp->left.dx = fix_int(dx);
   tp->left.dx_frac = dx<<16;
   tp->left.x = fix_int(x);
   tp->left.x_frac = x<<16;

   flags = tp->flags&PPF_MASK;
   if (flags == 0) return;
   for (i=0, mask=1; ; mask+=mask, i++) {
      if (!(mask&flags))
         continue;
      tp->left.coord_val[i] = tp->scale[i]*prev->coord[i]*65536.0 + fdy*tp->dcy[i] + fdx*tp->dcx[i];
      tp->left.coord_delta[i] = tp->dcy[i] + tp->left.dx * tp->dcx[i];
      if ((flags -= mask)==0)
         break;
   }
}

static void fix_left_edge(void)
{
   g2s_point *prev = g2d_v0;
   g2s_point *next = g2d_v1;
   g2s_poly_params *tp = g2d_ppp;
   fix dx, dy, x, delta_y;
   float fdx, fdy;
   int i;
   ulong mask, flags;

   dy = next->sy - prev->sy;
   dx = next->sx - prev->sx;



   AssertMsg(dy > 0, "gen_left_edge(): Hey... edge not valid!\n");
   dx = fix_div(dx, dy);
   delta_y = fix_ceil(prev->sy) - prev->sy;
   // add 0xffff because we're actually doing ceil's...
#line 99
   x = prev->sx + 0xffff + fix_mul(dx, delta_y);

   tp->left.dx = fix_int(dx);
   tp->left.dx_frac = dx<<16;
   tp->left.x = fix_int(x);
   tp->left.x_frac = x<<16;

   flags = tp->flags&PPF_MASK;
   if (flags == 0) return;

   fdx = fix_float(fix_floor(x) - prev->sx);
   fdy = fix_float(delta_y);
   for (i=0, mask=1; ; mask+=mask, i++) {
      if (!(mask&flags))
         continue;
      tp->left.coord_val[i] = tp->scale[i]*prev->coord[i]*65536.0 + tp->dcy[i]*fdy + tp->dcx[i]*fdx;
      tp->left.coord_delta[i] = tp->dcy[i] + tp->left.dx * tp->dcx[i];
      if ((flags -= mask)==0)
         break;
   }
}

static void fix_right_edge(void)
{
   g2s_point *prev = g2d_v0;
   g2s_point *next = g2d_v1;
   g2s_poly_params *tp = g2d_ppp;
   fix dx, dy, delta_y;
   dx = next->sx - prev->sx;
   dy = next->sy - prev->sy;



   AssertMsg(dy > 0, "gen_right_edge(): Hey... edge not valid!\n");
   tp->right.dx = dx = fix_div(dx, dy);
   delta_y = fix_ceil(prev->sy) - prev->sy;
   // add 0xffff because we're actually doing ceil's...
#line 135
   tp->right.x = prev->sx + 0xffff + fix_mul(dx, delta_y);
}


// n is the number of scanlines for the shorter edge.
void gen_raster_loop(void)
{
   g2s_poly_params *tp = g2d_ppp;
   int n = tp->num_scanlines;
   g2s_raster r;
   do {
      ulong mask, flags = tp->flags&PPF_MASK;
      int i;
      long carry;

      r.p = tp->p;
      tp->p += tp->canvas_row;
      r.x = tp->left.x;
      r.n = fix_int(tp->right.x) - tp->left.x;
      tp->right.x += tp->right.dx;
      carry = ulong_add_get_carry(&tp->left.x_frac, tp->left.dx_frac); // (0 or -1)
      tp->left.x += tp->left.dx - carry;
      if (flags!=0)
         for (i=0, mask=1; ; mask += mask, i++) {
            if (!(mask&flags))
               continue;
            r.coord[i] = tp->left.coord_val[i];
            tp->left.coord_val[i] += tp->left.coord_delta[i] + (tp->dcx[i]&carry);
            if ((flags -= mask)==0)
               break;
         }
      if (r.n > 0)
         tp->inner_loop(&r, tp);
   } while (--n > 0);
}


int gen_triangle_raster(void)
{
   g2s_poly_params *tp = g2d_ppp;
   g2s_point *vtop, *vright, *vleft;
   int ytop, yright, yleft, n;

   gen_triangle_gradients();

   if (g2d_v0->sy < g2d_v1->sy) {
      if (g2d_v0->sy < g2d_v2->sy) {
         vtop = g2d_v0; vright = g2d_v1; vleft = g2d_v2;
      } else {
         vtop = g2d_v2; vright = g2d_v0; vleft = g2d_v1;
      }
   } else if (g2d_v1->sy < g2d_v2->sy) {
      vtop = g2d_v1; vright = g2d_v2; vleft = g2d_v0;
   } else {
      vtop = g2d_v2; vright = g2d_v0; vleft = g2d_v1;
   }

   ytop = fix_cint(vtop->sy);
   yleft = fix_cint(vleft->sy);
   yright = fix_cint(vright->sy);

   if (tp->dy == 1)
      tp->y = ytop;
   else
      tp->p = grd_bm.bits + ytop*tp->canvas_row;

   if (yright>yleft) {     // left elbow
      n = yright-ytop;

      g2d_v0 = vtop; g2d_v1 = vright;
      tp->right_edge_func();
      if (yleft>ytop) {    // do we have a flat top?
         g2d_v1 = vleft;
         tp->left_edge_func();
         tp->num_scanlines = yleft-ytop;
         tp->raster_func();
      }
                           // can't have flat bottom since yright>yleft
      g2d_v0 = vleft; g2d_v1 = vright;
      tp->left_edge_func();
      tp->num_scanlines = yright-yleft;
      tp->raster_func();
   } else {                // right elbow
      n = yleft-ytop;
      if (n==0)
         return 0;

      g2d_v0 = vtop; g2d_v1 = vleft;
      tp->left_edge_func();
      if (yright>ytop) {   // do we have a flat top?
         g2d_v1 = vright;
         tp->right_edge_func();
         tp->num_scanlines = yright-ytop;
         tp->raster_func();
      }
      if (yleft>yright) {  // do we have a flat bottom?
         g2d_v0 = vright; g2d_v1 = vleft;
         tp->right_edge_func();
         tp->num_scanlines = yleft-yright;
         tp->raster_func();
      }
   }
   return n;
}

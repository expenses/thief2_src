// $Header: x:/prj/tech/libsrc/g2/RCS/tf.c 1.4 1998/04/28 14:29:47 KEVIN Exp $
#include <trirast.h>
#include <plyshell.h>
#include <tmapd.h>

int g2_umap(int n, g2s_point **vpl)
{
   int i;

   g2d_v0 = vpl[0];
   g2d_v1 = vpl[1];
   g2d_v2 = vpl[2];
   g2d_ppp->tri_func();
   for (i=3; i<n; i++) {
      g2d_v0 = vpl[0];
      g2d_v1 = vpl[i-1];
      g2d_v2 = vpl[i];
      g2d_ppp->tri_func();
   }
   return CLIP_NONE;
}

void g2_trifan(g2s_poly_params *pp, int n, g2s_point **vpl)
{
   g2d_ppp = pp;
   g2_umap(n, vpl);
   g2d_ppp = &g2d_pp;
}

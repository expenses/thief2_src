/*
 * $Source: x:/prj/tech/libsrc/md/RCS/render.c $
 * $Revision: 1.28 $
 * $Date: 1998/06/30 18:56:08 $
 *
 * Model Library rendering routines
 *
 */

#include <string.h>

#include <md.h>
#include <md_.h>
#include <r3d.h>
#include <lg.h>
#include <mprintf.h>
#include <g2.h> // g2s_point

extern void (*g_fp_md_set_alpha_data)(int, float);
extern void (*g_fp_md_unset_alpha_data)(void);
extern mdf_pgon_cback render_pgon_callback;

// The main polygon render function
void md_render_pgon(mds_pgon *p)
{
   int i;
   int j;
   ulong flag=0;
   ulong type;

   // Bail if it isn't facing you
   // use the cached normal dot prod
   if ((mdd.buff_norms[p->norm] + p->d) <= 0.0) return;
   
   // Set vlist to contain the list of verts
   for (i=0;i<p->num;++i) {
      mdd_vlist[i] = md_buff_point(p->verts[i]);
   }

   // Set the type, accounting for rendering mode,
   // ie, no lighting, or a particular prim type
   type = (p->type&mdd_type_and)|mdd_type_or;

   // lighting on
   if (type&MD_PGON_LIGHT_ON) {
      // Set lighting values
      if (!mdd_rgb_lighting) {
         flag |= R3_PL_GOURAUD;
         for (i=0,j=p->num;i<p->num;++i,++j) {
            mdd_vlist[i]->grp.i = mdd.buff_lights[p->verts[j]];
         }
      } else {
         flag |= R3_PL_RGB_GOURAUD;
         for (i=0,j=p->num;i<p->num;++i,++j) {
            int offset = p->verts[j] * 3;
            g2s_point *g2p = (g2s_point *) &mdd_vlist[i]->grp;
            g2p->i = mdd.buff_lights[offset];
            g2p->h = mdd.buff_lights[offset+1];
            g2p->d = mdd.buff_lights[offset+2];
         }
      }
   }

   // Alpha handling
   if (mdd.use_alpha) {
      int ofs = ((type & MD_PGON_PRIM_MASK) == MD_PGON_PRIM_TMAP) ? 3 : 2;

      float trans = md_amat(mdd.model, (uchar)p->verts[ofs * p->num])->trans;

      g_fp_md_set_alpha_data(trans > 0.0f, trans * mdd.alpha_scale);
   }

   switch (type&MD_PGON_COLOR_MASK) {
      case MD_PGON_COLOR_PAL:
      {
         // punt stupid encoding
         r3_set_color(p->data);
         break;
      }   
      case MD_PGON_COLOR_VCOL:
      {
         r3_set_color(mdd_vcolor_tab[p->data]);
         break;
      }
   }

   switch(type&MD_PGON_PRIM_MASK) {
      case MD_PGON_PRIM_SOLID:
      {
         r3_set_polygon_context(flag | R3_PL_POLYGON);
         r3_draw_poly(p->num,mdd_vlist);
         break;
      }
      case MD_PGON_PRIM_WIRE:
      {
         j = p->num-1;
         for (i=0;i<p->num;++i) {
            r3_draw_line(mdd_vlist[j],mdd_vlist[i]);
            j = i;
         }
         break;
      }
      case MD_PGON_PRIM_TMAP: 
      {   
         for (i=0,j=2*p->num;i<p->num;++j,++i) {
            mdd_vlist[i]->grp.u = mdd.uvs[p->verts[j]].u;
            mdd_vlist[i]->grp.v = mdd.uvs[p->verts[j]].v;
         }
         // Set the requested texture map and context
         r3_set_texture(mdd_vtext_tab[p->data]);
         r3_set_polygon_context(flag | R3_PL_POLYGON | mdd_tmap_mode);
         r3_draw_poly(p->num,mdd_vlist);
         break;
      }
   }

   if (mdd.use_alpha) {
      g_fp_md_unset_alpha_data();
   }
}

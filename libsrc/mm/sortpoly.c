// $Header: x:/prj/tech/libsrc/mm/RCS/sortpoly.c 1.1 1997/08/22 19:38:48 kate Exp $
// this module contains the various mesh polygon sorting routines.

#include <sortpoly.h>
#include <mm.h>
#include <mm_.h>
#include <stdlib.h>
#include <string.h>
#include <r3d.h>

#define Z_DEPTH(index) (mmd_buff_point(index)->p.z)

extern mms_polysort_elt *_mmd_polysort_elts;
extern int _mmd_num_polysort;

static int pgon_compare(const void *a, const void *b)
{
   mms_polysort_elt *pA=(mms_polysort_elt *)a;
   mms_polysort_elt *pB=(mms_polysort_elt *)b;

   return (pA->depth < pB->depth) ? 1 : -1;
}

void mm_polysort_global(mms_model *m)
{
   int num_polys = m->pgons;
   int num_attachments = mmd_custom_data->num_attachments;
   mms_pgon *poly = mmd_pgons;
   mms_attach_trans *trans = (mms_attach_trans *)mmd_buff_attach_trans;
   int i;

   _mmd_num_polysort = 0;
   for (i = 0; i < num_polys; ++i)
   {
      if (mmd_buff_norms[poly->norm] + poly->d > 0.0)
      {
         _mmd_polysort_elts[_mmd_num_polysort].depth =
            Z_DEPTH(poly->v[0]) + Z_DEPTH(poly->v[1]) + Z_DEPTH(poly->v[2]);
         _mmd_polysort_elts[_mmd_num_polysort].index = i;
         _mmd_polysort_elts[_mmd_num_polysort++].kind = 1;
      }
      ++poly;
   }
   for (i = 0; i < num_attachments; ++i)
   {
      _mmd_polysort_elts[_mmd_num_polysort].depth = trans->point.p.z * 3.0;
      _mmd_polysort_elts[_mmd_num_polysort].index = i;
      _mmd_polysort_elts[_mmd_num_polysort++].kind = 2;
      ++trans;
   }
   qsort(_mmd_polysort_elts, _mmd_num_polysort, sizeof(mms_polysort_elt), pgon_compare);
}

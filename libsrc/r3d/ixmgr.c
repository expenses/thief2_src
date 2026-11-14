
#include <memall.h>
#include <lgassert.h>
#include <tmpalloc.h>
#include <r3ds.h>
#include <ctxts.h>
#include <xform.h>
#include <g2spoint.h>

static int *offset_stack;
static int offset_stack_size;
static int offset_stack_depth;
static int r3dix_offset;
static int r3dix_total;
static int r3dix_total_inuse;
static int heap_next;
static int r3dix_max;
static int stride;
static uchar *r3dix_point_list;
static r3ixs_info *r3dix_info_list;

extern void (**ppfReleaseIP)(void);

void r3_reset_index(void)
{
   r3dix_total = 0;
   r3dix_total_inuse = 0;
   heap_next = 0;
   if (ppfReleaseIP && *ppfReleaseIP)
      (*ppfReleaseIP)();
}

void r3_index_free_lists(void)
{
   if (r3dix_point_list) {
      Free(r3dix_point_list);
      r3dix_point_list = NULL;
   }
   if (r3dix_info_list) {
      Free(r3dix_info_list);
      r3dix_info_list = NULL;
   }
   r3dix_max = 0;
   stride = r3d_glob.cur_stride;
   r3_reset_index();
}

void r3_index_init(void)
{
   if (offset_stack) {
      Free(offset_stack);
      offset_stack = NULL;
   }
   offset_stack_size = 0;
   offset_stack_depth = 0;
   r3dix_offset = 0;
   r3_index_free_lists();
}

void r3_index_term(void)
{
   r3_index_init();
}

void r3_start_indexed_block(void)
{
   if (offset_stack_depth >= offset_stack_size) {
      offset_stack_size += 64;
      if (offset_stack)
         offset_stack = Realloc(offset_stack, offset_stack_size);
      else
         offset_stack = Malloc(offset_stack_size);
   }
   offset_stack[offset_stack_depth++] = r3dix_offset;
   r3dix_offset = r3dix_total;
}

void r3_end_indexed_block(void)
{
#ifdef DEBUG
   int i;
   for (i = r3dix_offset; i < r3dix_total; i++)
      AssertMsg1((r3dix_info_list[i].flags & 2) == 0, "indexed point %d unfreed", i);
#endif
   r3dix_offset = offset_stack[--offset_stack_depth];
}

int r3_alloc_indexed(int start, int n)
{
   int num_reused = 0;
   int i, max, end;

   if (start < 0)
      start = r3dix_total;
   else
      start += r3dix_offset;
   end = start + n;
   if (r3d_glob.cur_stride != stride) {
#line 133
      AssertMsg(!r3dix_total, "r3_alloc_indexed(): Stride changed while points still in use!");
      r3_index_free_lists();
   }
   if (end > r3dix_max) {
      do {
         r3dix_max += 256;
      } while (end > r3dix_max);
      if (!r3dix_point_list) {
         if (r3dix_max < 512)
            r3dix_max = 512;
         r3dix_point_list = Malloc(r3dix_max * stride);
         r3dix_info_list = Malloc(r3dix_max * sizeof(r3ixs_info));
      } else {
         r3dix_point_list = Realloc(r3dix_point_list, r3dix_max * stride);
         r3dix_info_list = Realloc(r3dix_info_list, r3dix_max * sizeof(r3ixs_info));
      }
   }
   if (start > r3dix_total) {
      for (i = r3dix_total; i < start; i++) {
         r3dix_info_list[i].index = -1;
         r3dix_info_list[i].flags = 1;
      }
      r3dix_total = start;
   }
   if (end > r3dix_total) {
      max = r3dix_total;
      r3dix_total = end;
   } else
      max = end;
   for (i = start; i < max; i++) {
      if (r3dix_info_list[i].flags & 1) {
         r3dix_info_list[i].index = heap_next++;
         r3dix_info_list[i].flags &= ~1;
      }
      if (r3dix_info_list[i].flags & 2)
         num_reused++;
      else
         r3dix_info_list[i].flags |= 2;
   }
   for (; i < end; i++) {
      r3dix_info_list[i].flags = 2;
      r3dix_info_list[i].index = heap_next++;
   }
   r3dix_total_inuse += n - num_reused;
   return start - r3dix_offset;
}

void r3_free_indexed_range(int start, int n)
{
   int i, end;

   start += r3dix_offset;
   end = start + n;
   for (i = start; i < end; i++) {
      if (r3dix_info_list[i].flags & 2)
         r3dix_total_inuse--;
      r3dix_info_list[i].index = -1;
      r3dix_info_list[i].flags = 1;
   }
#line 221
   AssertMsg1(r3dix_total_inuse >= 0, "r3dix: total in use: %i!", r3dix_total_inuse);
   if (r3dix_total_inuse == 0)
      r3_reset_index();
}

int r3_transform_block_indexed(int start, int n, mxs_vector *verts)
{
   start = r3_alloc_indexed(start, n);
   r3_transform_block(n, (r3s_point *)(r3dix_point_list + (start + r3dix_offset) * stride), verts);
   return start;
}

r3s_point *r3_get_indexed_phandle(int id)
{
   return (r3s_point *)(r3dix_point_list + (id + r3dix_offset) * stride);
}

void r3_set_strided_uvs_indexed(int start, int n, float *uvs, int uv_stride)
{
   int i, end;
   r3s_point *p;

   start += r3dix_offset;
   end = start + n;
   p = (r3s_point *)(r3dix_point_list + start * stride);
   for (i = start; i < end; i++) {
      if (r3dix_info_list[i].flags & 1) {
         r3dix_info_list[i].flags &= ~1;
         r3dix_info_list[i].index = heap_next++;
      }
      p->grp.u = uvs[0];
      p->grp.v = uvs[1];
      uvs = (float *)((uchar *)uvs + uv_stride);
      p = (r3s_point *)((uchar *)p + stride);
   }
}

void r3_set_strided_is_indexed(int start, int n, float *is, int i_stride)
{
   int i, end;
   r3s_point *p;

   start += r3dix_offset;
   end = start + n;
   p = (r3s_point *)(r3dix_point_list + start * stride);
   for (i = start; i < end; i++) {
      if (r3dix_info_list[i].flags & 1) {
         r3dix_info_list[i].flags &= ~1;
         r3dix_info_list[i].index = heap_next++;
      }
      p->grp.i = is[0];
      is = (float *)((uchar *)is + i_stride);
      p = (r3s_point *)((uchar *)p + stride);
   }
}

void r3_set_strided_rgbs_indexed(int start, int n, float *rgbs, int rgb_stride)
{
   int i, end;
   r3s_point *p;

   start += r3dix_offset;
   end = start + n;
   p = (r3s_point *)(r3dix_point_list + start * stride);
   for (i = start; i < end; i++) {
      g2s_point *g2p = (g2s_point *)&p->grp;
      if (r3dix_info_list[i].flags & 1) {
         r3dix_info_list[i].flags &= ~1;
         r3dix_info_list[i].index = heap_next++;
      }
      g2p->r = rgbs[0];
      g2p->g = rgbs[1];
      g2p->b = rgbs[2];
      rgbs = (float *)((uchar *)rgbs + rgb_stride);
      p = (r3s_point *)((uchar *)p + stride);
   }
}

r3s_point **r3_get_vpl_from_index(int n, short *index_list)
{
   r3s_point **vpl;
   int i;

   vpl = (r3s_point **)temp_malloc(n * sizeof(r3s_point *));
   for (i = 0; i < n; i++) {
      int j = r3dix_offset + index_list[i];
#line 335
      AssertMsg((j >= 0) && (j < r3dix_total), "r3_get_vpl_from_index(): vertex index out of range!");
      vpl[i] = (r3s_point *)(r3dix_point_list + j * stride);
   }
   return vpl;
}
r3ixs_info *r3_get_info_from_index(int n, short *index_list)
{
   int i;
   r3ixs_info *info;
   info = (r3ixs_info *)temp_malloc(n * sizeof(r3ixs_info));
   for (i = 0; i < n; i++) {
      int j = r3dix_offset + index_list[i];

      AssertMsg((r3dix_info_list[j].flags & 2) != 0,
                "r3_get_info_from_index(): trying to use invalid vertex!\n");
      info[i] = r3dix_info_list[j];
      r3dix_info_list[j].flags |= 1;
   }
   return info;
}

void r3_set_strided_rgbas_indexed(int start, int n, float *rgbas, int rgba_stride)
{
   int i, end;
   r3s_point *p;

   start += r3dix_offset;
   end = start + n;
   p = (r3s_point *)(r3dix_point_list + start * stride);
   for (i = start; i < end; i++) {
      g2s_point *g2p = (g2s_point *)&p->grp;
      if (r3dix_info_list[i].flags & 1) {
         r3dix_info_list[i].flags &= ~1;
         r3dix_info_list[i].index = heap_next++;
      }
      g2p->r = rgbas[0];
      g2p->g = rgbas[1];
      g2p->b = rgbas[2];
      g2p->a = rgbas[3];
      rgbas = (float *)((uchar *)rgbas + rgba_stride);
      p = (r3s_point *)((uchar *)p + stride);
   }
}

r3ixs_info *r3_get_clipped_info_from_index(int n, r3s_phandle *vpl, int m, r3s_phandle *cvpl, short *index_list)
{
   r3ixs_info *info;
   int i;

   info = (r3ixs_info *)temp_malloc(m * sizeof(r3ixs_info));
   for (i = 0; i < m; i++) {
      int j;
      r3s_point *p = cvpl[i];
      for (j = 0; j < n; j++)
         if (p == vpl[j])
            break;
      if (j == n) {
         info[i].index = heap_next++;
         info[i].flags = 0;
      } else {
         int k = r3dix_offset + index_list[j];
#line 378
         AssertMsg(r3dix_info_list[k].flags & 2, "r3_get_clipped_info_from_index(): trying to use invalid vertex!\n");
         info[i] = r3dix_info_list[k];
         r3dix_info_list[k].flags |= 1;
      }
   }
   return info;
}

#pragma once

EXTERN void r3_reset_index(void);
EXTERN void r3_index_free_lists(void);
EXTERN void r3_index_init(void);
EXTERN void r3_index_term(void);
EXTERN void r3_start_indexed_block(void);
EXTERN void r3_end_indexed_block(void);
EXTERN int r3_alloc_indexed(int start, int n);
EXTERN void r3_free_indexed_range(int start, int n);
EXTERN void r3_transform_block_indexed(int start, int n, mxs_vector *verts);
EXTERN void r3_set_strided_uvs_indexed(int start, int n, float *uvs, int uv_stride);
EXTERN void r3_set_strided_is_indexed(int start, int n, float *is, int i_stride);
EXTERN void r3_set_strided_rgbs_indexed(int start, int n, float *rgbs, int rgb_stride);
EXTERN r3s_point *r3_get_indexed_phandle(int id);
EXTERN r3s_point **r3_get_vpl_from_index(int n, short *index_list);
EXTERN r3ixs_info *r3_get_info_from_index(int n, short *index_list);
EXTERN r3ixs_info *r3_get_clipped_info_from_index(int n, r3s_phandle *vpl, int m, r3s_phandle *cvpl, short *index_list);

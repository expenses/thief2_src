#include <dev2d.h>
#include <gensf.h>
#include <fl8sf.h>
#include <swarn.h>

void (*flat8_scale_func[])() =
{
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	opaque_8to8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_OPAQUE) */
	trans_8to8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_TRANS) */
	opaque_clut_8to8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_OPAQUE) */
	trans_clut_8to8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_TRANS) */
	gen_flat8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_OPAQUE) */
	gen_flat8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_TRANS) */
	gen_flat8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_OPAQUE) */
	gen_flat8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_TRANS) */
	gen_flat8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_OPAQUE) */
	gen_flat8_scale,			/* MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_TRANS) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_OPAQUE) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_TRANS) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_OPAQUE) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_TRANS) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_OPAQUE) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_TRANS) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_OPAQUE) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_TRANS) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_OPAQUE) */
	gen_rsd8_scale,			/* MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_TRANS) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
};

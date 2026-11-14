#include <dev2d.h>
#include <gentf.h>
#include <fl8tf.h>
#include <tftype.h>

tmap_setup_func* flat8_lit_ulmap_setup_func[] =
{
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	opaque_lit_8to8_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_OPAQUE) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_NORM, BMF_TRANS) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_OPAQUE) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_CLUT, BMF_TRANS) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_OPAQUE) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_XOR, BMF_TRANS) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_OPAQUE) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_BLEND, BMF_TRANS) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_OPAQUE) */
	gen_flat8_lit_ulmap_setup,			/* MAKE_INDEX(BMT_FLAT8, FILL_SOLID, BMF_TRANS) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_OPAQUE) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_NORM, BMF_TRANS) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_OPAQUE) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_CLUT, BMF_TRANS) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_OPAQUE) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_XOR, BMF_TRANS) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_OPAQUE) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_BLEND, BMF_TRANS) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_OPAQUE) */
	gen_rsd8_ulmap_setup,			/* MAKE_INDEX(BMT_RSD8, FILL_SOLID, BMF_TRANS) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
};

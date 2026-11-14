
#include <dev2d.h>
#include "icanvas.h"
#include "general.h"
#include "flat16.h"

void (*g2d_default_flat16_canvas_table[G2C_TYPES])() =
{
	gen_uline,			/* G2C_ULINE */
	gen_uline_expose,			/* G2C_ULINE_EXPOSE */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gen_uscale,			/* G2C_USCALE */
	gen_uscale_expose,			/* G2C_USCALE_EXPOSE */
	gen_scale,			/* G2C_SCALE */
	gen_scale_expose,			/* G2C_SCALE_EXPOSE */
	gen_upoly_setup,			/* G2C_UPOLY */
	gr_null,				/* (empty slot) */
	gen_uspoly_setup,			/* G2C_USPOLY */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	flat16_ulmap_setup,			/* G2C_ULMAP */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	flat16_upmap_setup,			/* G2C_UPMAP */
	gr_null,				/* (empty slot) */
	flat16_lit_ulmap_setup,			/* G2C_LIT_ULMAP */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	flat16_lit_upmap_setup,			/* G2C_LIT_UPMAP */
	gr_null,				/* (empty slot) */
};

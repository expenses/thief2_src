#include <dev2d.h>
#include "icanvas.h"
#include "general.h"
#include "flat8.h"

void (*g2d_default_flat8_canvas_table[G2C_TYPES])() =
{
	g2_flat8_uline,			/* G2C_ULINE */
	g2_flat8_uline_expose,			/* G2C_ULINE_EXPOSE */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	flat8_uscale,			/* G2C_USCALE */
	flat8_uscale_expose,			/* G2C_USCALE_EXPOSE */
	flat8_scale,			/* G2C_SCALE */
	flat8_scale_expose,			/* G2C_SCALE_EXPOSE */
	gen_upoly_setup,			/* G2C_UPOLY */
	gr_null,				/* (empty slot) */
	flat8_uspoly_setup,			/* G2C_USPOLY */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	flat8_ulmap_setup,			/* G2C_ULMAP */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	flat8_upmap_setup,			/* G2C_UPMAP */
	gr_null,				/* (empty slot) */
	flat8_lit_ulmap_setup,			/* G2C_LIT_ULMAP */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	flat8_lit_upmap_setup,			/* G2C_LIT_UPMAP */
	gr_null,				/* (empty slot) */
};

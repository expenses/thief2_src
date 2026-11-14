#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <flat8.h>

void (*gdd_default_flat8_canvas_table[])() =
{
	flat8_upix8,			/* GDC_UPIX */
	flat8_upix8_expose,			/* GDC_UPIX_EXPOSE */
	flat8_upix8,			/* GDC_UPIX8 */
	flat8_upix8_expose,			/* GDC_UPIX8_EXPOSE */
	flat8_upix16,			/* GDC_UPIX16 */
	flat8_upix16_expose,			/* GDC_UPIX16_EXPOSE */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	flat8_lpix8,			/* GDC_LPIX */
	flat8_lpix8_expose,			/* GDC_LPIX_EXPOSE */
	flat8_lpix8,			/* GDC_LPIX8 */
	flat8_lpix8_expose,			/* GDC_LPIX8_EXPOSE */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	flat8_ugpix8,			/* GDC_UGPIX8 */
	flat8_ugpix8_expose,			/* GDC_UGPIX8_EXPOSE */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	flat8_ubitmap,			/* GDC_UBITMAP */
	flat8_ubitmap_expose,			/* GDC_UBITMAP_EXPOSE */
	flat8_uhline,			/* GDC_UHLINE */
	flat8_uhline_expose,			/* GDC_UHLINE_EXPOSE */
	flat8_uvline,			/* GDC_UVLINE */
	flat8_uvline_expose,			/* GDC_UVLINE_EXPOSE */
	gr_scream,				/* GDC_CANVAS_FUNCS */
};

#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <ddevblt.h>

void (*gdd_default_dispdev_canvas_table[])() =
{
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	dispdev_ubitmap,			/* GDC_UBITMAP */
	dispdev_ubitmap_expose,			/* GDC_UBITMAP_EXPOSE */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* (empty slot) */
	gr_scream,				/* GDC_CANVAS_FUNCS */
};

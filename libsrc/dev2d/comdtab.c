
#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*com_device_table[])() =
{
	com_init,			/* GDC_INIT_DEVICE */
	com_close,			/* GDC_CLOSE_DEVICE */
	com_set_mode,			/* GDC_SET_MODE */
	com_get_mode,			/* GDC_GET_MODE */
	com_save_state,			/* GDC_SAVE_STATE */
	com_restore_state,			/* GDC_RESTORE_STATE */
	com_stat_htrace,			/* GDC_STAT_HTRACE */
	com_stat_vtrace,			/* GDC_STAT_VTRACE */
	com_set_pal,			/* GDC_SET_PAL */
	com_get_pal,			/* GDC_GET_PAL */
	com_set_width,			/* GDC_SET_WIDTH */
	gr_scream,				/* (empty slot) */
	com_set_focus,			/* GDC_SET_FOCUS */
	com_get_focus,			/* GDC_GET_FOCUS */
	gr_scream,				/* (empty slot) */
	com_get_rgb_bitmask,			/* GDC_GET_RGB_BITMASK */
	gr_scream,				/* GDC_DEVICE_FUNCS */
};

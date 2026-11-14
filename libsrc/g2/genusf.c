#include <dev2d.h>
#include <gensf.h>
#include <swarn.h>

void (*gen_uscale_func[])() =
{
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gen_flat8_uscale,			/* BMT_FLAT8 */
	gen_flat16_uscale,		/* BMT_FLAT16 */
	gen_rsd8_uscale,			/* BMT_RSD8 */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
	gr_warn,				/* (empty slot) */
};

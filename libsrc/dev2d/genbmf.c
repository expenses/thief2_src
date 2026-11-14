#include <bitmap.h>
#include <genbm.h>
#include <grnull.h>

void (*gen_bitmap_func[])() =
{
	gr_null,			/* BMT_MODEX */
	gen_mono_bitmap,			/* BMT_MONO */
	gen_flat8_bitmap,			/* BMT_FLAT8 */
	gen_flat16_bitmap,			/* BMT_FLAT16 */
	gen_rsd8_bitmap,			/* BMT_RSD8 */
	gr_null,				/* (empty slot) */
	gen_flat8_bitmap,			/* BMT_BANK8 */
	gr_null,				/* (empty slot) */
	gr_null,				/* (empty slot) */
	gen_flat8_bitmap,			/* BMT_TLUC8 */
};

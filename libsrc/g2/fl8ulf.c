#include <dev2d.h>
#include <fl8lf.h>
#include <lftype.h>

g2ul_func* flat8_uline_func[FILL_TYPES] =
{
	flat8_uline_norm,			/* FILL_NORM */
	flat8_uline_clut,			/* FILL_CLUT */
	flat8_uline_xor,			/* FILL_XOR */
	flat8_uline_blend,			/* FILL_BLEND */
	flat8_uline_solid,			/* FILL_SOLID */
};

/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8p16f.tbl $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:12:20 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <fill.h>
#include <grnull.h>
#include <fl8pix.h>

void (*flat8_upix16_func[FILL_TYPES])() =
{
	gr_null,				/* FILL_NORM */
	gr_null,				/* FILL_CLUT */
	gr_null,				/* FILL_XOR */
	gr_null,				/* FILL_BLEND */
	gr_null,				/* FILL_SOLID */
};
// $Header: x:/prj/tech/libsrc/palmgr/RCS/palmgr.h 1.1 1998/11/02 10:49:44 KEVIN Exp $

#ifndef __PALMGR_H
#define __PALMGR_H

#define PALMGRF_SHARE         0x01     // OK to share this slot

#define PALMGRF_PALDATA       0x02     // this slot has actual data

#define PALMGRF_PAL16         0x04     // calculate 16 bit palette for this slot when necessary

#define PALMGRF_LTAB816       0x08     // calculate 8to16 lighting table for this slot

// setup lgd3d palette slot; enable opaque texture support.
#define PALMGRF_LGD3D_OPAQUE  0x10     

// setup lgd3d palette slot; enable trans texture support with alpha blending.
#define PALMGRF_LGD3D_TRANS   0x20     

#define PALMGRF_IPAL          0x40     // calc ipal for this slot

#define PALMGRF_STORE_MASK    0xffff   // up to 16 persistent flags

#define PALMGRF_LGD3D PALMGRF_LGD3D_OPAQUE|PALMGRF_LGD3D_TRANS
#define PALMGRF_DEFAULT PALMGRF_LTAB816|PALMGRF_LGD3D|PALMGRF_PALDATA|PALMGRF_PAL16

// The following flags are temporary and not stored by the palette manager

// update flags for this slot (for use with palmgr_set_pal/update_slot() only)
#define PALMGRF_SETFLAGS      0x10000  

// use the persistent flags for this slot to update (for use with palmgr_set_pal/update_slot() only)
#define PALMGRF_USEFLAGS      0x20000  

EXTERN void palmgr_init(int shade_rows, int bright_rows, int rgb_lighting);
EXTERN void palmgr_shutdown(void);

// either finds matching palette and increases ref count or 
// allocates new palette slot.
EXTERN int palmgr_alloc_slot(uchar *pal, int flags);

// Recompute one slot acording to the flags specified.
// If PALMGRF_SETFLAGS is specified, the new flags will be recorded and
// any unused data is freed.
EXTERN void palmgr_update_slot(int slot, int flags);

// just bump ref count for this slot...
EXTERN void palmgr_add_ref_slot(int slot);

// decrease ref count for this slot
EXTERN void palmgr_release_slot(int slot);

// Free all data associated with this slot, right now.
// Set ref count to zero.
EXTERN void palmgr_free_slot(int slot);

// change data for this slot and invalidates lighting tables.
// does _not_ affect ref count.
EXTERN void palmgr_set_pal_slot(int slot, int start, int n, uchar *pal);

// set flags for this slot, do no computation
EXTERN void palmgr_set_flags_slot(int slot, int flags);

// Return ipal for given slot.  Creates if necessary.
EXTERN uchar *palmgr_get_ipal_slot(int slot);

// just returns the palette in this slot
EXTERN uchar *palmgr_get_pal_slot(int slot);

// return flags for this slot
EXTERN int palmgr_get_flags_slot(int slot);

// essentially does
// palmgr_update_slot(slot, flags);
// palmgr_set_flags(palmgr_get_flags(slot)|flags);
EXTERN void palmgr_add_flags_slot(int slot, int flags);

// Frees memory associated with flags and clears those flags.
EXTERN void palmgr_remove_flags_slot(int slot, int flags);

// recompute 16 bit palettes and lighting tables, etc. as appropriate
EXTERN void palmgr_recompute_all(void);

// tell palmgr screen mode has changed.
EXTERN void palmgr_mode_changed(void);

// tell palmgr whether to use lgd3d
EXTERN void palmgr_use_lgd3d(BOOL use_lgd3d);  

// get the number of currently allocated palettes; non-ship only
EXTERN int palmgr_get_current_count(BOOL mono_print);

#endif

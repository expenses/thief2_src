// $Header: x:/prj/tech/libsrc/object/objedit/RCS/linkedst.h 1.1 1999/04/07 14:39:40 TOML Exp $

#ifndef __LINKEDST_H
#define __LINKEDST_H


//
// Link Editor Flags (part of the editor descriptor)
//

enum _eLinkEditorFlags 
{
   kLinkEditDoneButton     = (1 << 0),  // Add a "Done" button
   kLinkEditAddButton      = (1 << 1),     // Add an "Add" button
   kLinkEditDeleteButton   = (1 << 2),  // Add a  "Delete" button
   kLinkEditEditButton     = (1 << 3),  // Add an "Edit" button, support link editing on dclick

   kLinkEditNoIDs          = (1 << 8),    // Don't show id's
   kLinkEditNoFlavors      = (1 << 9),    // Don't show flavors
   kLinkEditNoSources      = (1 << 10),   // Don't show sources 
   kLinkEditNoDests        = (1 << 11),   // Don't show destinations
   kLinkEditNoData         = (1 << 12),   // Don't show data

   kLinkEditShowAll        = (1 << 16),   // Don't filter out internal links
   kLinkEditModeless       = (1 << 17),   // Edit modelessly.  

   kLinkEditAllButtons = kLinkEditDoneButton|kLinkEditAddButton|kLinkEditDeleteButton|kLinkEditEditButton
};

typedef ulong eLinkEditorFlags;


// 
// Link Editor Descriptor
//

struct sLinkEditorDesc 
{
   char title[64]; // Title string appearing at top of gadget;
   eLinkEditorFlags flags;
};



#endif // __LINKEDST_H

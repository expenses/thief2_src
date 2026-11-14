#include <comtools.h>
#include <initguid.h>

#include <lgsndid.h>

// Instantiate DirectSound guids which we need to QueryInterface on, which
// otherwise would be unresolved because we don't statically link to dsound

// copied from dsound.h
// Yes, I shall roast a while longer in Hell for this one - patmc

//DirectSound3DListener 279afa84-4981-11ce-a521-0020af0be560


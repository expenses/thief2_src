#pragma once

#include <storeapi.h>
#include <resapi.h>
#include <grs.h>

grs_bitmap* ResTgaReadImage(IStoreStream* pStream, IResMemOverride* pResMem);
void* ResTgaReadPalette(IStoreStream* pStream, IResMemOverride* pResMem);
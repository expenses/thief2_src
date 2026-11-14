#pragma once

#include <storeapi.h>
#include <resapi.h>
#include <grs.h>

grs_bitmap* ResCelReadImage(IStoreStream* pStream, IResMemOverride* pResMem);
void* ResCelReadPalette(IStoreStream* pStream, IResMemOverride* pResMem);
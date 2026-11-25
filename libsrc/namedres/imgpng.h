#pragma once

#include <storeapi.h>
#include <resapi.h>
#include <grs.h>

grs_bitmap* ResPngReadImage(IStoreStream* pStream, IResMemOverride* pResMem);

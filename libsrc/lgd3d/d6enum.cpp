// $Header: x:/prj/tech/libsrc/lgd3d/RCS/enum.cpp 1.7 1998/02/16 15:08:00 KEVIN Exp $
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

#include <comtools.h>
#include <dddynf.h>
#include <lgassert.h>
#include <memall.h>
#include <dev2d.h>
#include <d6check.h>
#include <lgd3d.h>
#include <setup.h>
#include <dbg.h>

#ifndef SHIP
#include <mprintf.h>
#define spew(x) mprint(x)
#else
#define spew(x)
#pragma off(unreferenced)
#endif


#define MAX_DEVICES 10
#define TEXTURE_FLAGS (              \
   D3DDEVCAPS_TEXTURENONLOCALVIDMEM| \
   D3DDEVCAPS_TEXTURESYSTEMMEMORY|   \
   D3DDEVCAPS_TEXTUREVIDEOMEMORY)

typedef struct lgd3ds_device_enum_info {
   LPGUID   p_ddraw_guid;
   int      device_number;
   char *   p_ddraw_desc;
   short    supported_modes[GRD_MODES];
   int      num_supported;
   ulong    requested_flags;
} lgd3ds_device_enum_info;

static int num_devices = -2;
static lgd3ds_device_info *psDeviceList[MAX_DEVICES];
static BOOL no_d3d = TRUE;
static int bDepthBuffer;
static int bWindowed;

static BOOL PASCAL c_DDEnumCallback(LPGUID lpGUID, char *lpDriverDescription, char *lpDriverName, void *lpContext);
static void GetDevices(lgd3ds_device_enum_info *info);
static HRESULT WINAPI c_EnumDisplayModesCallback(LPDDSURFACEDESC pSD, LPVOID data);
static HRESULT WINAPI c_EnumDevicesCallback(LPGUID lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC pDeviceDesc, LPD3DDEVICEDESC lpD3DHELDeviceDesc, LPVOID lpUserArg);

int lgd3d_enumerate_devices(void)
{
    return lgd3d_enumerate_devices_capable_of(0);
}

int lgd3d_enumerate_devices_capable_of(DWORD flags)
{
    if (num_devices < -1)
    {
        if (LoadDirectDraw())
        {
            lgd3ds_device_enum_info sDEinfo;
            sDEinfo.device_number = 0;
            sDEinfo.requested_flags = flags;
            DynDirectDrawEnumerate(c_DDEnumCallback, &sDEinfo);
            if (no_d3d && sDEinfo.device_number == 0)
            {
                num_devices = -1;
            }
            else
            {
                num_devices = sDEinfo.device_number;
            }
        }
        else
        {
            num_devices = -1;
        }
    }
    return num_devices;
}

extern "C" void lgd3d_unenumerate_devices(void)
{
   int i;
   for (i = 0; i < num_devices; i++) {

      free(psDeviceList[i]);
      psDeviceList[i] = NULL;
   }
   num_devices = -2;
}


lgd3ds_device_info *lgd3d_get_device_info(int device_number)
{
#line 144
   AssertMsg1(num_devices > device_number, "Invalid device number: %d", device_number);
   return psDeviceList[device_number];
}

#ifdef SHIP
#define EnumSpew(x)
#else
#define EnumSpew(x) mprintf x
#endif

static BOOL PASCAL c_DDEnumCallback(LPGUID lpGUID, char *lpDriverDescription, char *lpDriverName, void *lpContext)
{
   lgd3ds_device_enum_info *info = (lgd3ds_device_enum_info *)lpContext;

   info->p_ddraw_guid = lpGUID;
   info->p_ddraw_desc = lpDriverDescription;
   GetDevices(info);

   return DDENUMRET_OK;
}

static void GetDevices(lgd3ds_device_enum_info *info)
{
   HRESULT hResult;
   LPDIRECTDRAW lpdd;
   LPDIRECTDRAW4 lpdd4;
   LPDIRECT3D3 lpd3d;
   DDCAPS ddcaps;

   hResult = DynDirectDrawCreate(info->p_ddraw_guid, &lpdd, NULL);
   if (hResult >= 0)
   {
      hResult = lpdd->QueryInterface(IID_IDirectDraw4, (void**)&lpdd4);
      if (hResult == 0)
      {
         memset(&ddcaps, 0, sizeof(ddcaps));
         ddcaps.dwSize = sizeof(ddcaps);
         hResult = lpdd4->GetCaps(&ddcaps, NULL);
         if (hResult == 0 && (ddcaps.dwCaps & 1) != 0)
         {
            bDepthBuffer = (ddcaps.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) != 0;
            bWindowed = (ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED) != 0;
            hResult = lpdd4->QueryInterface(IID_IDirect3D3, (void**)&lpd3d);
            if (hResult == 0)
            {
               no_d3d = FALSE;
               info->num_supported = 0;
               ((IDirectDraw*)lpdd4)->EnumDisplayModes(0, NULL, info, c_EnumDisplayModesCallback);
               if (info->num_supported > 0) {
                  hResult = lpd3d->EnumDevices(c_EnumDevicesCallback, info);
                  if (hResult != 0) {
                     Warning(("EnumDevices failed."));
                  }
               }
               else {
                  Warning(("Couldn't find any display modes"));
               }
               if (lpd3d) {
                  lpd3d->Release();
               }
               lpd3d = NULL;
            }
            else {
               Warning(("Can't obtain D3D interface."));
            }
         }
         else {
            Warning(("Not a 3d accelerator."));
         }
         if (lpdd4) {
            lpdd4->Release();
         }
         lpdd4 = NULL;
      }
      else {
         Warning(("Can't obtain DDraw4 interface."));
      }
      if (lpdd) {
         lpdd->Release();
      }
      lpdd = NULL;
   }
   else {
      Warning(("Can't create ddraw object."));
   }
}


static HRESULT WINAPI c_EnumDisplayModesCallback(LPDDSURFACEDESC pSD, LPVOID data)
{
   lgd3ds_device_enum_info *info = (lgd3ds_device_enum_info *)data;
   int mode = gr_mode_from_info(pSD->dwWidth, pSD->dwHeight, 
      pSD->ddpfPixelFormat.dwRGBBitCount);

   if (mode < 0)
      return D3DENUMRET_OK;

   // first make sure we haven't already listed this mode as supported
   int i;
   for (i=0; i<info->num_supported; i++)
      if (info->supported_modes[i] == mode) break;

   if (i<info->num_supported)
      return D3DENUMRET_OK;

   info->supported_modes[info->num_supported++] = (short )mode;
   return D3DENUMRET_OK;
}

static HRESULT WINAPI c_EnumDevicesCallback(LPGUID lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC pDeviceDesc, LPD3DDEVICEDESC lpD3DHELDeviceDesc, LPVOID lpUserArg)
{
    lgd3ds_device_enum_info* info = (lgd3ds_device_enum_info*)lpUserArg;
    lgd3ds_device_info* device_info;
    ulong capabilities_flags;
    int malloc_size;

    if (pDeviceDesc->dcmColorModel == 0)
        return D3DENUMRET_OK;

    capabilities_flags = 0;
    if (!(pDeviceDesc->dwDeviceRenderBitDepth & DDBD_16))
    {
        EnumSpew(("No 16 bit.\n"));
        return D3DENUMRET_OK;
    }
    if (!(pDeviceDesc->dcmColorModel & D3DCOLOR_RGB))
    {
        EnumSpew(("device not RGB.\n"));
        return D3DENUMRET_OK;
    }
    if (!(pDeviceDesc->dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB))
    {
        EnumSpew(("no color gouraud shading.\n"));
        return D3DENUMRET_OK;
    }
    if (!(pDeviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_ALPHA))
    {
        EnumSpew(("no alpha blending.\n"));
        return D3DENUMRET_OK;
    }
    if (!(pDeviceDesc->dwDevCaps & TEXTURE_FLAGS))
    {
        EnumSpew(("no texture mapping.\n"));
        return D3DENUMRET_OK;
    }

    if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
        pDeviceDesc->dpcTriCaps.dwRasterCaps &= ~D3DPRASTERCAPS_FOGTABLE;

    if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
        capabilities_flags = LGD3DF_CAN_DO_TABLE_FOG;
    else if (info->requested_flags & LGD3DF_TABLE_FOG)
    {
        EnumSpew(("no table fog\n"));
        return D3DENUMRET_OK;
    }

    if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX)
        capabilities_flags |= LGD3DF_CAN_DO_VERTEX_FOG;
    else if (info->requested_flags & LGD3DF_VERTEX_FOG)
    {
        EnumSpew(("no vertex fog\n"));
        return D3DENUMRET_OK;
    }

    if (pDeviceDesc->dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDBLEND)
        capabilities_flags |= LGD3DF_CAN_DO_ITERATE_ALPHA;

    if (bDepthBuffer)
    {
        capabilities_flags |= LGD3DF_CAN_DO_ZBUFFER;
        if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_WBUFFER)
            capabilities_flags |= LGD3DF_CAN_DO_WBUFFER;
    }

    if (bWindowed)
        capabilities_flags |= LGD3DF_CAN_DO_WINDOWED;

    if (pDeviceDesc->wMaxTextureBlendStages >= 2 && pDeviceDesc->wMaxSimultaneousTextures >= 2
        && (WORD)pDeviceDesc->dwFVFCaps >= 2)
        capabilities_flags |= LGD3DF_CAN_DO_SINGLE_PASS_MT;

    if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER)
        capabilities_flags |= LGD3DF_CAN_DO_DITHER;

    if (pDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT)
        capabilities_flags |= LGD3DF_CAN_DO_ANTIALIAS;

#line 416
    AssertMsg(info->device_number < MAX_DEVICES, "Too many d3d devices available.");
    malloc_size = sizeof(lgd3ds_device_info) + strlen(info->p_ddraw_desc) + 1
        + sizeof(D3DDEVICEDESC) + (info->num_supported + 1) * sizeof(short);
    if (info->p_ddraw_guid)
    {
        device_info = (lgd3ds_device_info*)malloc(malloc_size + sizeof(GUID));
        device_info->p_ddraw_guid = (GUID*)((char*)device_info + malloc_size);
        *device_info->p_ddraw_guid = *info->p_ddraw_guid;
    }
    else
    {
        device_info = (lgd3ds_device_info*)malloc(malloc_size);
        device_info->p_ddraw_guid = NULL;
    }

    device_info->device_guid = *lpGuid;
    device_info->supported_modes = (short*)(device_info + 1);
    if (info->num_supported > 0)
        memcpy(device_info->supported_modes, info->supported_modes, info->num_supported * sizeof(short));
    device_info->supported_modes[info->num_supported] = -1;
    device_info->device_desc = (DevDesc*)(device_info->supported_modes + info->num_supported + 1);
    *device_info->device_desc = *pDeviceDesc;
    device_info->p_ddraw_desc = (char*)(device_info->device_desc + 1);
    strcpy(device_info->p_ddraw_desc, info->p_ddraw_desc);
    device_info->flags = capabilities_flags;

    psDeviceList[info->device_number++] = device_info;
    return D3DENUMRET_OK;
}


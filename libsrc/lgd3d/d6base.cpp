#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <lgd3d.h>
#include <d6states.h>
static HRESULT g_lgd3d_error_code = 0;
static long g_hD3DError = 0;

void SetLGD3DErrorCode(unsigned long code, long hError)
{
    g_lgd3d_error_code = code;
    g_hD3DError = hError;
}

char *GetLgd3dErrorCode(unsigned long code)
{
    switch (code)
    {
    case LGD3D_EC_DD_KAPUT: return "Could not obtain DirectDraw from display\0";
    case LGD3D_EC_RESTORE_ALL_SURFS: return "Could not restore all surfaces\0";
    case LGD3D_EC_QUERY_D3D: return "QueryInterface for D3D failed\0";
    case LGD3D_EC_GET_DD_CAPS: return " GetCaps for DirectDraw failed\0";
    case LGD3D_EC_ZBUFF_ENUMERATION: return " Could not enumerate Zbuffer formats\0";
    case LGD3D_EC_CREATE_3D_DEVICE: return "Create D3D device failed\0";
    case LGD3D_EC_CREATE_VIEWPORT: return "Creation of thr viewport failed\0";
    case LGD3D_EC_ADD_VIEWPORT: return " Addition of the viewport failed\0";
    case LGD3D_EC_SET_VIEWPORT: return "Setting of the viewport2 failed\0";
    case LGD3D_EC_SET_CURR_VP: return "Setting of current viewport failed\0";
    case LGD3D_EC_CREATE_BK_MATERIAL: return "Creation of the background material failed\0";
    case LGD3D_EC_SET_BK_MATERIAL: return "Setting of the background material failed\0";
    case LGD3D_EC_GET_BK_MAT_HANDLE: return "Could not obtain a material handle\0";
    case LGD3D_EC_GET_SURF_DESC: return "Could not get surface description\0";
    case LGD3D_EC_GET_3D_CAPS: return "Could not get D3D device caps\0";
    case LGD3D_EC_VD_MPASS_MT: return "ValidateDevice for multipass lightmaps failed\0";
    case LGD3D_EC_VD_S_DEFAULT: return "ValidateDevice for simple states failed\0";
    case LGD3D_EC_VD_SPASS_MT: return "ValidateDevice for single pass lightmaps failed\0";
    case LGD3D_EC_VD_M_DEFAULT: return "ValidateDevice for multitexture states failed\0";
    default: return "Unknown error\0";
    }
}

char* GetDDErrorMsg(long hr)
{
    switch (hr)
    {
    case DD_OK: return "I am OK\0";
    case DDERR_GENERIC: return "Generic failure.\0";
    case DDERR_ALREADYINITIALIZED: return "This object is already initialized.\0";
    case DDERR_CANNOTATTACHSURFACE: return "This surface can not be attached to the requested surface.\0";
    case DDERR_CURRENTLYNOTAVAIL: return "Support is currently not available.\0";
    case DDERR_EXCEPTION: return "An exception was encountered while performing the requested operation.\0";
    case DDERR_HEIGHTALIGN: return "Height of rectangle provided is not a multiple of reqd alignment.\0";
    case DDERR_INCOMPATIBLEPRIMARY: return "Unable to match primary surface creation request with existing primary surface.\0";
    case DDERR_INVALIDCAPS: return "One or more of the caps bits passed to the callback are incorrect.\0";
    case DDERR_INVALIDCLIPLIST: return "DirectDraw does not support the provided cliplist.\0";
    case DDERR_INVALIDMODE: return "DirectDraw does not support the requested mode.\0";
    case DDERR_INVALIDOBJECT: return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
    case DDERR_INVALIDPARAMS: return "One or more of the parameters passed to the function are incorrect.\0";
    case DDERR_INVALIDPIXELFORMAT: return "The pixel format was invalid as specified.\0";
    case DDERR_INVALIDRECT: return "Rectangle provided was invalid.\0";
    case DDERR_LOCKEDSURFACES: return "Operation could not be carried out because one or more surfaces are locked.\0";
    case DDERR_NO3D: return "There is no 3D present.\0";
    case DDERR_NOALPHAHW: return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
    case DDERR_NOCLIPLIST: return "No cliplist available.\0";
    case DDERR_NOCOLORCONVHW: return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
    case DDERR_NOCOOPERATIVELEVELSET: return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
    case DDERR_NOCOLORKEY: return "Surface doesn't currently have a color key\0";
    case DDERR_NOCOLORKEYHW: return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
    case DDERR_NODIRECTDRAWSUPPORT: return "No DirectDraw support possible with current display driver.\0";
    case DDERR_NOEXCLUSIVEMODE: return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
    case DDERR_NOFLIPHW: return "Flipping visible surfaces is not supported.\0";
    case DDERR_NOGDI: return "There is no GDI present.\0";
    case DDERR_NOMIRRORHW: return "Operation could not be carried out because there is no hardware present or available.\0";
    case DDERR_NOTFOUND: return "Requested item was not found.\0";
    case DDERR_NOOVERLAYHW: return "Operation could not be carried out because there is no overlay hardware present or available.\0";
    case DDERR_OVERLAPPINGRECTS: return "Operation could not be carried out because the source and destination rectangles are on the same surface and overlap each other.\0";
    case DDERR_NORASTEROPHW: return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
    case DDERR_NOROTATIONHW: return "Operation could not be carried out because there is no rotation hardware present or available.\0";
    case DDERR_NOSTRETCHHW: return "Operation could not be carried out because there is no hardware support for stretching.\0";
    case DDERR_NOT4BITCOLOR: return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
    case DDERR_NOT4BITCOLORINDEX: return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
    case DDERR_NOT8BITCOLOR: return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
    case DDERR_NOTEXTUREHW: return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
    case DDERR_NOVSYNCHW: return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
    case DDERR_NOZBUFFERHW: return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
    case DDERR_NOZOVERLAYHW: return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
    case DDERR_OUTOFCAPS: return "The hardware needed for the requested operation has already been allocated.\0";
    case DDERR_OUTOFMEMORY: return "DirectDraw does not have enough memory to perform the operation.\0";
    case DDERR_OUTOFVIDEOMEMORY: return "DirectDraw does not have enough video memory to perform the operation.\0";
    case DDERR_OVERLAYCANTCLIP: return "The hardware does not support clipped overlays.\0";
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "Can only have ony color key active at one time for overlays.\0";
    case DDERR_OVERLAYNOTVISIBLE: return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
    case DDERR_PALETTEBUSY: return "Access to this palette is being refused because the palette is already locked by another thread.\0";
    case DDERR_COLORKEYNOTSET: return "No src color key specified for this operation.\0";
    case DDERR_SURFACEALREADYATTACHED: return "This surface is already attached to the surface it is being attached to.\0";
    case DDERR_SURFACEALREADYDEPENDENT: return "This surface is already a dependency of the surface it is being made a dependency of.\0";
    case DDERR_SURFACEBUSY: return "Access to this surface is being refused because the surface is already locked by another thread.\0";
    case DDERR_CANTLOCKSURFACE: return "Access to this surface is being refused because no driver exists which can supply a pointer to the surface.\0";
    case DDERR_SURFACEISOBSCURED: return "Access to surface refused because the surface is obscured.\0";
    case DDERR_SURFACELOST: return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
    case DDERR_SURFACENOTATTACHED: return "The requested surface is not attached.\0";
    case DDERR_TOOBIGHEIGHT: return "Height requested by DirectDraw is too large.\0";
    case DDERR_TOOBIGSIZE: return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
    case DDERR_TOOBIGWIDTH: return "Width requested by DirectDraw is too large.\0";
    case DDERR_UNSUPPORTED: return "Action not supported.\0";
    case DDERR_UNSUPPORTEDFORMAT: return "FOURCC format requested is unsupported by DirectDraw.\0";
    case DDERR_UNSUPPORTEDMASK: return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
    case DDERR_INVALIDSTREAM: return "The specified stream contains invalid data.\0";
    case DDERR_VERTICALBLANKINPROGRESS: return "Vertical blank is in progress.\0";
    case DDERR_WASSTILLDRAWING: return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
    case DDERR_XALIGN: return "Rectangle provided was not horizontally aligned on required boundary.\0";
    case DDERR_INVALIDDIRECTDRAWGUID: return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
    case DDERR_DIRECTDRAWALREADYCREATED: return "A DirectDraw object representing this driver has already been created for this process.\0";
    case DDERR_NODIRECTDRAWHW: return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
    case DDERR_PRIMARYSURFACEALREADYEXISTS: return "This process already has created a primary surface.\0";
    case DDERR_NOEMULATION: return "Software emulation not available.\0";
    case DDERR_REGIONTOOSMALL: return "Region passed to Clipper::GetClipList is too small.\0";
    case DDERR_CLIPPERISUSINGHWND: return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
    case DDERR_NOCLIPPERATTACHED: return "No clipper object attached to surface object.\0";
    case DDERR_NOHWND: return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
    case DDERR_HWNDSUBCLASSED: return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
    case DDERR_HWNDALREADYSET: return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
    case DDERR_NOPALETTEATTACHED: return "No palette object attached to this surface.\0";
    case DDERR_NOPALETTEHW: return "No hardware support for 16 or 256 color palettes.\0";
    case DDERR_BLTFASTCANTCLIP: return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
    case DDERR_NOBLTHW: return "No blitter hardware present.\0";
    case DDERR_NODDROPSHW: return "No DirectDraw ROP hardware.\0";
    case DDERR_NOOVERLAYDEST: return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
    case DDERR_INVALIDPOSITION: return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
    case DDERR_NOTAOVERLAYSURFACE: return "Returned when an overlay member is called for a non-overlay surface.\0";
    case DDERR_EXCLUSIVEMODEALREADYSET: return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
    case DDERR_NOTFLIPPABLE: return "An attempt has been made to flip a surface that is not flippable.\0";
    case DDERR_CANTDUPLICATE: return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
    case DDERR_NOTLOCKED: return "Surface was not locked. An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
    case DDERR_CANTCREATEDC: return "or a DC was requested for a palette-indexed surface when the surface had no palette AND the display mode was not palette-indexed.\0";
    case DDERR_NODC: return "No DC was ever created for this surface.\0";
    case DDERR_WRONGMODE: return "This surface can not be restored because it was created in a different mode.\0";
    case DDERR_IMPLICITLYCREATED: return "This surface can not be restored because it is an implicitly created surface.\0";
    case DDERR_NOTPALETTIZED: return "The surface being used is not a palette-based surface.\0";
    case DDERR_UNSUPPORTEDMODE: return "The display is currently in an unsupported mode.\0";
    case DDERR_NOMIPMAPHW: return "Operation could not be carried out because there is no mip-map texture mapping hardware present or available.\0";
    case DDERR_INVALIDSURFACETYPE: return "The requested action could not be performed because the surface was of the wrong type.\0";
    case DDERR_NOOPTIMIZEHW: return "Device does not support optimized surfaces, therefore no video memory optimized surfaces.\0";
    case DDERR_NOTLOADED: return "Surface is an optimized surface, but has not yet been allocated any memory.\0";
    case DDERR_NOFOCUSWINDOW: return "Attempt was made to create or set a device window without first setting the focus window.\0";
    case DDERR_DCALREADYCREATED: return "A DC has already been returned for this surface. Only one DC can be retrieved per surface.\0";
    case DDERR_NONONLOCALVIDMEM: return "An attempt was made to allocate non-local video memory from a device that does not support non-local video memory.\0";
    case DDERR_CANTPAGELOCK: return "The attempt to page lock a surface failed.\0";
    case DDERR_CANTPAGEUNLOCK: return "The attempt to page unlock a surface failed.\0";
    case DDERR_NOTPAGELOCKED: return "An attempt was made to page unlock a surface with no outstanding page locks.\0";
    case DDERR_MOREDATA: return "There is more data available than the specified buffer size could hold.\0";
    case DDERR_EXPIRED: return "The data has expired and is therefore no longer valid.\0";
    case DDERR_VIDEONOTACTIVE: return "The video port is not active.\0";
    case DDERR_DEVICEDOESNTOWNSURFACE: return "Surfaces created by one direct draw device cannot be used directly by another direct draw device.\0";
    case DDERR_NOTINITIALIZED: return "An attempt was made to invoke an interface member of a DirectDraw object created by CoCreateInstance() before it was initialized.\0";
    case D3DERR_BADMAJORVERSION: return "D3DERR_BADMAJORVERSION\0";
    case D3DERR_BADMINORVERSION: return "D3DERR_BADMINORVERSION\0";
    case D3DERR_INVALID_DEVICE: return "D3DERR_INVALID_DEVICE\0";
    case D3DERR_INITFAILED: return "D3DERR_INITFAILED\0";
    case D3DERR_DEVICEAGGREGATED: return "D3DERR_DEVICEAGGREGATED\0";
    case D3DERR_EXECUTE_CREATE_FAILED: return "D3DERR_EXECUTE_CREATE_FAILED\0";
    case D3DERR_EXECUTE_DESTROY_FAILED: return "D3DERR_EXECUTE_DESTROY_FAILED\0";
    case D3DERR_EXECUTE_LOCK_FAILED: return "D3DERR_EXECUTE_LOCK_FAILED\0";
    case D3DERR_EXECUTE_UNLOCK_FAILED: return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
    case D3DERR_EXECUTE_LOCKED: return "D3DERR_EXECUTE_LOCKED\0";
    case D3DERR_EXECUTE_NOT_LOCKED: return "D3DERR_EXECUTE_NOT_LOCKED\0";
    case D3DERR_EXECUTE_FAILED: return "D3DERR_EXECUTE_FAILED\0";
    case D3DERR_EXECUTE_CLIPPED_FAILED: return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
    case D3DERR_TEXTURE_NO_SUPPORT: return "D3DERR_TEXTURE_NO_SUPPORT\0";
    case D3DERR_TEXTURE_CREATE_FAILED: return "D3DERR_TEXTURE_CREATE_FAILED\0";
    case D3DERR_TEXTURE_DESTROY_FAILED: return "D3DERR_TEXTURE_DESTROY_FAILED\0";
    case D3DERR_TEXTURE_LOCK_FAILED: return "D3DERR_TEXTURE_LOCK_FAILED\0";
    case D3DERR_TEXTURE_UNLOCK_FAILED: return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
    case D3DERR_TEXTURE_LOAD_FAILED: return "D3DERR_TEXTURE_LOAD_FAILED\0";
    case D3DERR_TEXTURE_SWAP_FAILED: return "D3DERR_TEXTURE_SWAP_FAILED\0";
    case D3DERR_TEXTURE_LOCKED: return "D3DERR_TEXTURE_LOCKED\0";
    case D3DERR_TEXTURE_NOT_LOCKED: return "D3DERR_TEXTURE_NOT_LOCKED\0";
    case D3DERR_TEXTURE_GETSURF_FAILED: return "D3DERR_TEXTURE_GETSURF_FAILED\0";
    case D3DERR_MATRIX_CREATE_FAILED: return "D3DERR_MATRIX_CREATE_FAILED\0";
    case D3DERR_MATRIX_DESTROY_FAILED: return "D3DERR_MATRIX_DESTROY_FAILED\0";
    case D3DERR_MATRIX_SETDATA_FAILED: return "D3DERR_MATRIX_SETDATA_FAILED\0";
    case D3DERR_MATRIX_GETDATA_FAILED: return "D3DERR_MATRIX_GETDATA_FAILED\0";
    case D3DERR_SETVIEWPORTDATA_FAILED: return "D3DERR_SETVIEWPORTDATA_FAILED\0";
    case D3DERR_INVALIDCURRENTVIEWPORT: return "D3DERR_INVALIDCURRENTVIEWPORT\0";
    case D3DERR_INVALIDPRIMITIVETYPE: return "D3DERR_INVALIDPRIMITIVETYPE\0";
    case D3DERR_INVALIDVERTEXTYPE: return "D3DERR_INVALIDVERTEXTYPE\0";
    case D3DERR_TEXTURE_BADSIZE: return "D3DERR_TEXTURE_BADSIZE\0";
    case D3DERR_INVALIDRAMPTEXTURE: return "D3DERR_INVALIDRAMPTEXTURE\0";
    case D3DERR_MATERIAL_CREATE_FAILED: return "D3DERR_MATERIAL_CREATE_FAILED\0";
    case D3DERR_MATERIAL_DESTROY_FAILED: return "D3DERR_MATERIAL_DESTROY_FAILED\0";
    case D3DERR_MATERIAL_SETDATA_FAILED: return "D3DERR_MATERIAL_SETDATA_FAILED\0";
    case D3DERR_MATERIAL_GETDATA_FAILED: return "D3DERR_MATERIAL_GETDATA_FAILED\0";
    case D3DERR_INVALIDPALETTE: return "D3DERR_INVALIDPALETTE\0";
    case D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY: return "D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY\0";
    case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY: return "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY\0";
    case D3DERR_SURFACENOTINVIDMEM: return "D3DERR_SURFACENOTINVIDMEM\0";
    case D3DERR_LIGHT_SET_FAILED: return "D3DERR_LIGHT_SET_FAILED\0";
    case D3DERR_LIGHTHASVIEWPORT: return "D3DERR_LIGHTHASVIEWPORT\0";
    case D3DERR_LIGHTNOTINTHISVIEWPORT: return "D3DERR_LIGHTNOTINTHISVIEWPORT\0";
    case D3DERR_SCENE_IN_SCENE: return "D3DERR_SCENE_IN_SCENE\0";
    case D3DERR_SCENE_NOT_IN_SCENE: return "D3DERR_SCENE_NOT_IN_SCENE\0";
    case D3DERR_SCENE_BEGIN_FAILED: return "D3DERR_SCENE_BEGIN_FAILED\0";
    case D3DERR_SCENE_END_FAILED: return "D3DERR_SCENE_END_FAILED\0";
    case D3DERR_INBEGIN: return "D3DERR_INBEGIN\0";
    case D3DERR_NOTINBEGIN: return "D3DERR_NOTINBEGIN\0";
    case D3DERR_NOVIEWPORTS: return "D3DERR_NOVIEWPORTS\0";
    case D3DERR_VIEWPORTDATANOTSET: return "D3DERR_VIEWPORTDATANOTSET\0";
    case D3DERR_VIEWPORTHASNODEVICE: return "D3DERR_VIEWPORTHASNODEVICE\0";
    case D3DERR_NOCURRENTVIEWPORT: return "D3DERR_NOCURRENTVIEWPORT\0";
    case D3DERR_INVALIDVERTEXFORMAT: return "D3DERR_INVALIDVERTEXFORMAT\0";
    case D3DERR_COLORKEYATTACHED: return "Attempted to CreateTexture on a surface that had a color key.\0";
    case D3DERR_VERTEXBUFFEROPTIMIZED: return "D3DERR_VERTEXBUFFEROPTIMIZED\0";
    case D3DERR_VBUF_CREATE_FAILED: return "D3DERR_VBUF_CREATE_FAILED\0";
    case D3DERR_VERTEXBUFFERLOCKED: return "D3DERR_VERTEXBUFFERLOCKED\0";
    case D3DERR_ZBUFFER_NOTPRESENT: return "D3DERR_ZBUFFER_NOTPRESENT\0";
    case D3DERR_STENCILBUFFER_NOTPRESENT: return "D3DERR_STENCILBUFFER_NOTPRESENT\0";
    case D3DERR_WRONGTEXTUREFORMAT: return "D3DERR_WRONGTEXTUREFORMAT\0";
    case D3DERR_UNSUPPORTEDCOLOROPERATION: return "D3DERR_UNSUPPORTEDCOLOROPERATION\0";
    case D3DERR_UNSUPPORTEDCOLORARG: return "D3DERR_UNSUPPORTEDCOLORARG\0";
    case D3DERR_UNSUPPORTEDALPHAOPERATION: return "D3DERR_UNSUPPORTEDALPHAOPERATION\0";
    case D3DERR_UNSUPPORTEDALPHAARG: return "D3DERR_UNSUPPORTEDALPHAARG\0";
    case D3DERR_TOOMANYOPERATIONS: return "D3DERR_TOOMANYOPERATIONS\0";
    case D3DERR_CONFLICTINGTEXTUREFILTER: return "D3DERR_CONFLICTINGTEXTUREFILTER\0";
    case D3DERR_UNSUPPORTEDFACTORVALUE: return "D3DERR_UNSUPPORTEDFACTORVALUE\0";
    case D3DERR_CONFLICTINGRENDERSTATE: return "D3DERR_CONFLICTINGRENDERSTATE\0";
    case D3DERR_UNSUPPORTEDTEXTUREFILTER: return "D3DERR_UNSUPPORTEDTEXTUREFILTER\0";
    case D3DERR_TOOMANYPRIMITIVES: return "D3DERR_TOOMANYPRIMITIVES\0";
    case D3DERR_INVALIDMATRIX: return "D3DERR_INVALIDMATRIX\0";
    case D3DERR_TOOMANYVERTICES: return "D3DERR_TOOMANYVERTICES\0";
    case D3DERR_CONFLICTINGTEXTUREPALETTE: return "D3DERR_CONFLICTINGTEXTUREPALETTE\0";
    default: return "Unrecognized error value.\0";
    }
}


BOOL lgd3d_get_error(DWORD* pdwCode, DWORD* phResult)
{
    if (g_lgd3d_error_code != 0)
    {
        *pdwCode = g_lgd3d_error_code;
        *phResult = g_hD3DError;
        return 1;
    }
    return 0;
}



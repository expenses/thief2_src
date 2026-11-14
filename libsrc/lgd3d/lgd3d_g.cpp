#include <windows.h>
#include <d3d.h>
#include <lgd3d.h>
#include <r3d.h>
#include <d6states.h>

double z_near = 1.0;
double z_far = 200.0;
double inv_z_far = 1.0 / 200.0;
double z1 = 200.0 / 199.0;
double z2 = 200.0 / 199.0;
double z2d = 1.0;
double w2d = 1.0;

cliprect lgd3d_clip;
int lgd3d_g_bInitialized = FALSE;
double zbias = 0.0;
uchar *texture_clut = NULL;
uchar *lgd3d_clut = NULL;
int zlinear = FALSE;
int bSpewOn = FALSE;
extern "C" {
BOOL lgd3d_punt_buffer = FALSE;
BOOL lgd3d_punt_d3d = FALSE;
}

extern "C" void lgd3d_set_zlinear(BOOL lin)
{
   zlinear = lin;
}

#include <lgd3d.h>
#include <md.h>

extern "C"
{

extern void (*g_fp_md_set_alpha_data)(int, float);
extern void (*g_fp_md_unset_alpha_data)(void);
extern void (*g_fp_md_set_mdf_pgon_cback)(void);
extern mdf_pgon_cback pgon_callback;

static int bAlphaWasOn;

static void md_lgd3d_set_alpha_data(int on, float alpha);
static void md_lgd3d_unset_alpha_data(void);
static void md_lgd3d_set_mdf_pgon_cback(void);

void md_use_lgd3d(void)
{
    g_fp_md_set_alpha_data = md_lgd3d_set_alpha_data;
    g_fp_md_unset_alpha_data = md_lgd3d_unset_alpha_data;
    g_fp_md_set_mdf_pgon_cback = md_lgd3d_set_mdf_pgon_cback;
}

static void md_lgd3d_set_alpha_data(int on, float alpha)
{
    bAlphaWasOn = on;
    if (on)
    {
        lgd3d_set_alpha(alpha);
        lgd3d_set_blend(1);
    }
}

static void md_lgd3d_unset_alpha_data(void)
{
    if (bAlphaWasOn)
    {
        lgd3d_set_alpha(1.0f);
        lgd3d_set_blend(0);
    }
}

static void md_lgd3d_set_mdf_pgon_cback(void)
{
    pgon_callback = md_render_pgon;
}

}

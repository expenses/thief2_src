#include <mprintf.h>
#include <matrix.h>
#include <r3d.h>
#include <md.h>
#include <md_.h>
#include <math.h>

// Just set these directly
mxs_vector mdd_sun_vec;
float mdd_lt_amb;    // ambient value
float mdd_lt_diff;   // diffuse value
float mdd_lt_spec;   // specular value

// sun vector in object space
static mxs_vector obj_sun;
static mxs_vector obj_view;
static bool light_invalid = TRUE;
static float amb;
static uchar ltype;

void md_light_set_type(uchar type)
{
   ltype = type;
}

// Recomputes constants based on type
void md_light_recompute()
{
   float scale;

   light_invalid = FALSE;

   // transform into object space
   r3_rotate_w2o(&obj_sun,&mdd_sun_vec);

   obj_view = *r3_get_view_in_obj();
   mx_normeq_vec(&obj_view);

   // set amb
   amb = mdd_lt_amb;
}


static float CalcSpecularTerm(mxs_vector *norm)
{
   mxs_vector half;
   float val;

   if (mx_dot_vec(norm,&obj_view) <= 0.0f)
      return 0.0f;

   mx_add_vec(&half,&obj_sun,&obj_view);
   mx_normeq_vec(&half);

   val = mx_dot_vec(&half,norm);
   if (val < 0.0f)
      val = 0.0f;

   return (float)pow(val,mdd_lt_spec) * (mdd_lt_spec * 0.5 + 1.0);
}


// starkly light from the side
void md_light_cback(int num,float *ivals,mds_light *lts)
{
   mds_model *m;
   int i;
   mxs_vector norm;
   float diffuse;

   // punt if no lighting set.
   if (ltype == MD_LT_NONE) return;

   if (light_invalid)
      md_light_recompute();

   if (!mdd.use_illum) {
      if (ltype & (MD_LT_DIFF | MD_LT_SPEC)) {
         // Diffuse and/or specular
         for (i = 0; i < num; i++) {
            norm.x = X_NORM(lts[i].norm);
            norm.y = Y_NORM(lts[i].norm);
            norm.z = Z_NORM(lts[i].norm);

            if (ltype & MD_LT_SPEC) {
               diffuse = CalcSpecularTerm(&norm);
            } else {
               diffuse = mx_dot_vec(&norm,&obj_sun);
               if (diffuse < 0.0f)
                  diffuse = 0.0f;
            }

            ivals[i] = (diffuse *= mdd_lt_diff) + amb;
         }
      } else {
         // Just ambient
         for (i = 0; i < num; i++) {
            ivals[i] = amb;
         }
      }
   } else {
      m = mdd.model;
      if (ltype & (MD_LT_DIFF | MD_LT_SPEC)) {
         for (i = 0; i < num; i++) {
            norm.x = X_NORM(lts[i].norm);
            norm.y = Y_NORM(lts[i].norm);
            norm.z = Z_NORM(lts[i].norm);

            if (ltype & MD_LT_SPEC) {
               diffuse = CalcSpecularTerm(&norm);
            } else {
               diffuse = mx_dot_vec(&norm,&obj_sun);
               if (diffuse < 0.0f)
                  diffuse = 0.0f;
            }

            ivals[i] = (diffuse *= mdd_lt_diff) + amb + md_amat(m, lts[i].mat)->illum;
         }
      } else {
         for (i = 0; i < num; i++) {
            ivals[i] = amb;
            ivals[i] += md_amat(m, lts[i].mat)->illum;
         }
      }
   }
}


void md_mesh_light_cback(int num,float *ivals,mds_light *lights)
{
   mds_model *m;
   int i;
   mxs_vector norm;
   float diffuse;

   // punt if no lighting set.
   if (ltype == '\0')
      return;

   if (light_invalid)
      md_light_recompute();

   if (!mdd.use_illum) {
      if (ltype & (MD_LT_DIFF | MD_LT_SPEC)) {
         // Diffuse and/or specular
         for (i = 0; i < num; i++) {
            norm.x = X_NORM(lights[i].norm);
            norm.y = Y_NORM(lights[i].norm);
            norm.z = Z_NORM(lights[i].norm);

            if (ltype & MD_LT_SPEC) {
               diffuse = CalcSpecularTerm(&norm);
            } else {
               diffuse = mx_dot_vec(&norm,&obj_sun);
               if (diffuse < 0.0f)
                  diffuse = 0.0f;
            }

            ivals[i] = (diffuse *= mdd_lt_diff) + amb;
         }
      } else {
         // Just ambient
         for (i = 0; i < num; i++) {
            ivals[i] = amb;
         }
      }
   } else {
      m = mdd.model;
      if (ltype & (MD_LT_DIFF | MD_LT_SPEC)) {
         for (i = 0; i < num; i++) {
            norm.x = X_NORM(lights[i].norm);
            norm.y = Y_NORM(lights[i].norm);
            norm.z = Z_NORM(lights[i].norm);

            if (ltype & MD_LT_SPEC) {
               diffuse = CalcSpecularTerm(&norm);
            } else {
               diffuse = mx_dot_vec(&norm,&obj_sun);
               if (diffuse < 0.0f)
                  diffuse = 0.0f;
            }

            ivals[i] = (diffuse *= mdd_lt_diff) + amb + md_amat(m, lights[i].mat)->illum;
         }
      } else {
         for (i = 0; i < num; i++) {
            ivals[i] = amb;
            ivals[i] += md_amat(m, lights[i].mat)->illum;
         }
      }
   }
}


// Recomputes light vals
void md_light_obj(int which)
{
   light_invalid=TRUE;
}  


// Initial lighting conditions
void md_light_init()
{
   // install the lighting routine
   md_set_light_callback((mdf_light_cback)md_light_cback);
   md_set_light_obj_callback(md_light_obj);

   // Make default lighting be a soothing 50/50
   mdd_lt_amb  = .5;
   mdd_lt_diff = .5;
   mdd_lt_spec = 0;

   md_light_set_type(MD_LT_DIFF|MD_LT_AMB);

   // sun straight up
   mx_mk_vec(&mdd_sun_vec,0,0,1);
}

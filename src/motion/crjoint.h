////////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/motion/RCS/crjoint.h 1.2 1999/08/26 13:26:28 ccarollo Exp $
//
// Creature joint accessor header
//

#ifndef __CRJOINT_H
#define __CRJOINT_H

#include <objtype.h>
#include <matrixs.h>


// List of joints you may request
enum eCreatureJoints
{
   kCJ_Invalid, 
   kCJ_Head,
   kCJ_Neck,
   kCJ_Abdomen,
   kCJ_Butt,

   kCJ_LShoulder,
   kCJ_RShoulder,
   kCJ_LElbow,
   kCJ_RElbow,
   kCJ_LWrist,
   kCJ_RWrist,
   kCJ_LFingers,
   kCJ_RFingers,

   kCJ_LHip,
   kCJ_RHip,
   kCJ_LKnee,
   kCJ_RKnee,
   kCJ_LAnkle,
   kCJ_RAnkle,
   kCJ_LToe,
   kCJ_RToe,
   kCJ_Tail,

   kCJ_NumCreatureJoints,
};

EXTERN char *g_pJointNames[];

EXTERN int  GetCreatureJointID(ObjID creature, int joint);

#ifdef __cplusplus
EXTERN const mxs_vector &GetCreatureJointPos(ObjID creature, int joint);
EXTERN const mxs_matrix &GetCreatureJointOrient(ObjID creature, int joint);
#endif

EXTERN void GetCreatureJointPosC(ObjID creature, int joint, mxs_vector *vec);
EXTERN void GetCreatureJointOrientC(ObjID creature, int joint, mxs_matrix *mat);

#endif

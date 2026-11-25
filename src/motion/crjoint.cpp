/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/motion/crjoint.cpp,v 1.3 1999/12/09 20:16:43 BODISAFA Exp $
//
// Creature joint accessor 
//

#include <matrix.h>

#include <creatur_.h>
#include <creature.h>
#include <crjoint.h>

#include <objedit.h> // for nice naming in Assert messages

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

// For sdescs and stuff that want string names of joints
char *g_pJointNames[kCJ_NumCreatureJoints + 1] = 
{
   "N/A",
   "Head",
   "Neck",
   "Abdomen",
   "Butt",
   "Left Shoulder",
   "Right Shoulder",
   "Left Elbow",
   "Right Elbow",
   "Left Wrist",
   "Right Wrist",
   "Left Fingers",
   "Right Fingers",
   "Left Hip",
   "Right Hip",
   "Left Knee",
   "Right Knee",
   "Left Ankle",
   "Right Ankle",
   "Left Toe",
   "Right Toe",
   "Tail",
};

////////////////////////////////////////

int GetCreatureJointID(ObjID creature, int joint)
{
   cCreature *pCreature = CreatureFromObj(creature);
   AssertMsg2(pCreature,"GetCreatureJointId: pCreature NULL for %s joint %d\n",ObjEditName(creature),joint);
   AssertMsg1((joint >= 0 && joint < kCJ_NumCreatureJoints), "GetCreatureJointID: invalid joint index %d\n", joint);

   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();

   return pCDesc->pJointMap[joint];
}

////////////////////////////////////////

const mxs_vector &GetCreatureJointPos(ObjID creature, int joint)
{
   cCreature *pCreature = CreatureFromObj(creature);
   AssertMsg2(pCreature,"GetCreatureJointPos: pCreature NULL for %s joint %d\n",ObjEditName(creature),joint);
   AssertMsg1((joint >= 0 && joint < kCJ_NumCreatureJoints), "GetCreatureJointPos: invalid joint index %d\n", joint);
   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();

   if (pCDesc->pJointMap[joint] == -1)
   {
      Warning(("Invalid joint %d for creature %s\n", joint, ObjWarnName(creature)));
      return pCreature->GetJoint(0);
   }
       
   return pCreature->GetJoint(pCDesc->pJointMap[joint]);
}

////////////////////////////////////////

const mxs_matrix &GetCreatureJointOrient(ObjID creature, int joint)
{
   cCreature *pCreature = CreatureFromObj(creature);
   AssertMsg2(pCreature,"GetCreatureJointOrient: pCreature NULL for %s joint %d\n",ObjEditName(creature),joint);
   AssertMsg1((joint >= 0 && joint < kCJ_NumCreatureJoints), "GetCreatureJointOrient: invalid joint index %d\n", joint);

   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();
   Assert_(pCDesc);
   if ((joint == kCJ_Head) && pCDesc->canHeadTrack)
      return pCreature->GetHeadOrient();
   else
   {
      if (pCDesc->pJointMap[joint] == -1)
      {
         Warning(("Invalid joint %d for creature %s\n", joint, ObjWarnName(creature)));
         return pCreature->GetJointOrient(0);
      }
               
      return pCreature->GetJointOrient(pCDesc->pJointMap[joint]);
   }
}

////////////////////////////////////////

void GetCreatureJointPosC(ObjID creature, int joint, mxs_vector *vec)
{
   *vec = GetCreatureJointPos(creature, joint);
}

////////////////////////////////////////

void  GetCreatureJointOrientC(ObjID creature, int joint, mxs_matrix *mat)
{
   *mat = GetCreatureJointOrient(creature, joint);
}

////////////////////////////////////////

void GetCreatureJointPosAndOrient(ObjID creature, int joint, mxs_vector *pos, mxs_matrix *orient)
{
   cCreature *pCreature = CreatureFromObj(creature);
   AssertMsg2(pCreature,"GetCreatureJointPosAndOrient: pCreature NULL for %s joint %d\n",ObjEditName(creature),joint);
   AssertMsg1(joint >= 0 && joint < kCJ_NumCreatureJoints, "GetCreatureJointPosAndOrient: invalid joint index %d\n", joint);

   if (!pCreature)
   {
      mx_identity_mat(orient);
      mx_zero_vec(pos);
      return;
   }

   const sCreatureDesc *pCDesc = pCreature->GetCreatureDesc();

   if (joint == kCJ_Neck && pCDesc->canHeadTrack)
   {
      *pos = pCreature->GetJoint(pCDesc->pJointMap[kCJ_Neck]);
      *orient = pCreature->GetHeadOrient();
      return;
   }

   if (joint == kCJ_Head && pCDesc->canHeadTrack)
   {
      int neck = pCDesc->pJointMap[kCJ_Neck];
      int head = pCDesc->pJointMap[kCJ_Head];
      mxs_matrix rel;

      mx_mult_mat(&rel, &pCreature->GetHeadOrient(), &pCreature->GetJointOrient(neck));
      mx_sub_vec(pos, &pCreature->GetJoint(head), &pCreature->GetJoint(neck));
      mx_mat_muleq_vec(&rel, pos);
      mx_addeq_vec(pos, &pCreature->GetJoint(neck));
      mx_mul_mat(orient, &rel, &pCreature->GetJointOrient(head));
      return;
   }

   int id = pCDesc->pJointMap[joint];
   if (id == -1)
   {
      *pos = pCreature->GetJoint(0);
      *orient = pCreature->GetJointOrient(0);
      return;
   }
   *pos = pCreature->GetJoint(id);
   *orient = pCreature->GetJointOrient(id);
}

////////////////////////////////////////




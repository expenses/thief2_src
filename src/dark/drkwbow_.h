// $Header: r:/prj/cam/src/dark/RCS/drkwbow_.h 1.1 1998/05/01 11:59:31 kate Exp $

#ifndef __DRKWBOW__H
#define __DRKWBOW__H


// this must be in same order as skills in static array
enum ePlayerBowSkill
{
   kPBS_Idle,
   kPBS_Aim,
   kPBS_Draw,
   kPBS_Fire,
   kPBS_Relax,
   kPBS_Begin,
   kPBS_End,
   kPBS_NumSkills,
};

EXTERN sPlayerSkillFullDesc g_BowSkillDesc[kPBS_NumSkills];

#endif

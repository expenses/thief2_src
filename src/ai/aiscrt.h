///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/ai/RCS/aiscrt.h 1.4 1998/11/05 01:44:44 TJS Exp $
//
// Base Ai scripting types
//

#ifndef __AISCRT_H
#define __AISCRT_H

///////////////////////////////////////////////////////////////////////////////
//
// Enumerations
//

///////////////////////////////////////
//
// Script action priorities
//

enum eAIActionPriority
{
   kLowPriorityAction,
   kNormalPriorityAction,
   kHighPriorityAction,

   kActionPrioritySizer = 0xffffffff
};

///////////////////////////////////////
//
// Alertness representation
//

enum eAIScriptAlertLevel
{
   kNoAlert,
   kLowAlert,
   kModerateAlert,
   kHighAlert,

   kAIAlertSizer = 0xffffffff
};

///////////////////////////////////////
//
// Speed representation
//

enum eAIScriptSpeed
{
   kSlow,
   kNormalSpeed,
   kFast,

   kAISpeedSizer = 0xffffffff
};


///////////////////////////////////////
//
// Conversation structures
//

enum eAIConvEventKind
{
   kMotion,
   kSchema,
   kTweqAll,
   kVoiceover,
   kEnd,        // closes down conversation

   eAIEventKindFiller = 0xffffffff
};


struct AIConvEvent
{
   // durString is a string name of a duration expressed as text.
   // Why do this?  So we can internationalize our conversations
   // in the few days before ship without totally re-writing everything,
   // by going from hard-coded durations to hard-coded references to
   // language-dependent durations, that's why.  TJS 11/3/98
   eAIConvEventKind eventKind;
   char eventString[32];        // name of sound or motion
   char durString[32];
   int actor;                   // comes from an AIConversation link

   AIConvEvent()
   {
   }

   AIConvEvent(eAIConvEventKind new_eventKind,
               char *new_eventString,
               char *new_durString,
               int new_actor)
      : eventKind(new_eventKind),
        actor(new_actor)
   {
      strncpy(eventString, new_eventString, 31);
      eventString[31] = 0;
      strncpy(durString, new_durString, 31);
      durString[31] = 0;
   }

};


///////////////////////////////////////
//
// Extra flags to let scripts control other aspects of AI state
//

#define kSpeechOff     (1<<0)
#define kMotionOff     (1<<1)
#define kCombatOffHACK (1<<2)


///////////////////////////////////////////////////////////////////////////////

#endif /* !__AISCRT_H */

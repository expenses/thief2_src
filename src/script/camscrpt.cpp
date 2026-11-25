/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>
#include <appagg.h>
#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>
#include <scrptmsg.h>
#include <objscrt.h>
#include <osystype.h>
#include <matrix.h>
#include <camera.h>
#include <camscrs.h>
#include <playrobj.h>

#include <rendprop.h> //to unrender playerarm
#include <plycbllm.h> //to get playerarm
#include <command.h> //to execute clear_weapon command

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Camera script service 
//


#define ANG2DEGREES(x) (((mxs_real)(x) * 180)/MX_ANG_PI)

DECLARE_SCRIPT_SERVICE_IMPL(cCameraSrv,Camera)
{
public:

   STDMETHOD(StaticAttach)(object attachee)
      {
	 ObjID plyrobj = PlayerObject();
	 ObjID plyrarm = PlayerArm();
#ifdef THIEF
//other games don't have the "clear_weapon" command, and making
//it not render without unequipping would be very bad.
	 if (plyrarm)
	   ObjSetRenderType(plyrarm,kRenderNotAtAll);	 
	 CommandExecute("clear_weapon");
#endif
	 Camera *playercam = PlayerCamera();
	 CameraView(playercam, attachee);
         return S_OK; 
      }

   STDMETHOD(DynamicAttach)(object attachee)
      {
	 ObjID plyrobj = PlayerObject();
	 ObjID plyrarm = PlayerArm();
#ifdef THIEF
//other games don't have the "clear_weapon" command, and making
//it not render without unequipping would be very bad.
	 if (plyrarm)
	   ObjSetRenderType(plyrarm,kRenderNotAtAll);	 
	 CommandExecute("clear_weapon");
#endif
	 Camera *playercam = PlayerCamera();
	 CameraRemote(playercam, attachee);
         return S_OK; 
      }

   STDMETHOD(CameraReturn)(object attachee)
      {
	 Camera *playercam = PlayerCamera();
	 object actualattach = CameraGetObjID(playercam);
	 if (attachee == actualattach)
	   {
	     CameraAttach(playercam, PlayerObject());
             return S_OK; 
	   }
	 else
	   return S_FALSE;
      }

   STDMETHOD(ForceCameraReturn)()
     {
       Camera *playercam = PlayerCamera();
       CameraAttach(playercam,PlayerObject());
       return S_OK;
     }

   STDMETHOD_(object,GetCameraParent)()
     {
       Camera *playercam = PlayerCamera();
       return playercam ? CameraGetObjID(playercam) : OBJ_NULL;
     }

   STDMETHOD_(boolean, IsRemote)()
     {
       Camera *playercam = PlayerCamera();
       return !(playercam && CameraGetObjID(playercam) == PlayerObject());
     }

   STDMETHOD_(vector,GetPosition)()
     {
       Camera *playercam = PlayerCamera();
       if (!playercam)
         return vector(0,0,0);
       return playercam->pos;
     }

   STDMETHOD_(vector,GetFacing)()
     {
       Camera *playercam = PlayerCamera();
       if (!playercam)
         return vector(0,0,0);
       return vector(ANG2DEGREES(playercam->ang.tx),
                     ANG2DEGREES(playercam->ang.ty),
                     ANG2DEGREES(playercam->ang.tz));
     }

   STDMETHOD_(vector,CameraToWorld)(const vector ref local_pos)
     {
       Camera *playercam = PlayerCamera();
       if (!playercam)
         return local_pos;
       vector result(0,0,0);
       mxs_matrix m1, m2;
       mxs_trans t;
       mx_mk_rot_z_mat(&m1, playercam->ang.tz);
       mx_rot_y_mat(&m2, &m1, playercam->ang.ty);
       mx_rot_x_mat(&t.mat, &m2, playercam->ang.tx);
       t.vec = playercam->pos;
       mx_trans_mul_vec(&result, &t, &local_pos);
       return result;
     }

   STDMETHOD_(vector,WorldToCamera)(const vector ref world_pos)
     {
       Camera *playercam = PlayerCamera();
       if (!playercam)
         return world_pos;
       vector result(0,0,0);
       mxs_matrix m1, m2;
       mxs_trans t;
       mx_mk_rot_z_mat(&m1, playercam->ang.tz);
       mx_rot_y_mat(&m2, &m1, playercam->ang.ty);
       mx_rot_x_mat(&t.mat, &m2, playercam->ang.tx);
       t.vec = playercam->pos;
       mx_trans_tmul_vec(&result, &t, &world_pos);
       return result;
     }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cCameraSrv, Camera); 

struct sCameraScrMsg : public sScrMsg
{
   sCameraScrMsg(ObjID to = 0, const char *pszMsg = "")
      : sScrMsg(to, pszMsg)
   {}

   DECLARE_SCRMSG_PERSISTENT();
};

IMPLEMENT_SCRMSG_PERSISTENT(sCameraScrMsg)
{
   PersistenceHeader(sScrMsg, 1);
   return TRUE;
}

void CameraSendAttachMessages(ObjID oldObj, ObjID newObj)
{
   if (oldObj == newObj)
      return;
   AutoAppIPtr(ScriptMan);
   if (oldObj)
   {
      sCameraScrMsg msg(oldObj, "CameraDetach");
      pScriptMan->SendMessage(&msg, NULL);
   }
   if (newObj)
   {
      sCameraScrMsg msg(newObj, "CameraAttach");
      pScriptMan->SendMessage(&msg, NULL);
   }
}

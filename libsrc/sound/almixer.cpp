#include <windows.h>
#include <math.h>
#include <lg.h>
#include <lglog.h>
#include <mprintf.h>
#include <config.h>
#include <str.h>
#include <appagg.h>
#include <dispapi.h>
#include <wdispapi.h>
#include <dlgsndi.h>
#include <mixerlck.h>
#include <storeapi.h>
#include <resapi.h>

#define AL_NO_PROTOTYPES
#define ALC_NO_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/alext.h>

#define AL_MAX_DEVICES 128

#define AL_FUNC(type, name) static type p##name;
#define AL_FUNCS \
   AL_FUNC(LPALENABLE, alEnable) \
   AL_FUNC(LPALDISABLE, alDisable) \
   AL_FUNC(LPALISENABLED, alIsEnabled) \
   AL_FUNC(LPALGETBOOLEAN, alGetBoolean) \
   AL_FUNC(LPALGETINTEGER, alGetInteger) \
   AL_FUNC(LPALGETFLOAT, alGetFloat) \
   AL_FUNC(LPALGETDOUBLE, alGetDouble) \
   AL_FUNC(LPALGETBOOLEANV, alGetBooleanv) \
   AL_FUNC(LPALGETINTEGERV, alGetIntegerv) \
   AL_FUNC(LPALGETFLOATV, alGetFloatv) \
   AL_FUNC(LPALGETDOUBLEV, alGetDoublev) \
   AL_FUNC(LPALGETSTRING, alGetString) \
   AL_FUNC(LPALGETERROR, alGetError) \
   AL_FUNC(LPALISEXTENSIONPRESENT, alIsExtensionPresent) \
   AL_FUNC(LPALGETPROCADDRESS, alGetProcAddress) \
   AL_FUNC(LPALGETENUMVALUE, alGetEnumValue) \
   AL_FUNC(LPALLISTENERI, alListeneri) \
   AL_FUNC(LPALLISTENERF, alListenerf) \
   AL_FUNC(LPALLISTENER3F, alListener3f) \
   AL_FUNC(LPALLISTENERFV, alListenerfv) \
   AL_FUNC(LPALGETLISTENERI, alGetListeneri) \
   AL_FUNC(LPALGETLISTENERF, alGetListenerf) \
   AL_FUNC(LPALGETLISTENER3F, alGetListener3f) \
   AL_FUNC(LPALGETLISTENERFV, alGetListenerfv) \
   AL_FUNC(LPALGENSOURCES, alGenSources) \
   AL_FUNC(LPALDELETESOURCES, alDeleteSources) \
   AL_FUNC(LPALISSOURCE, alIsSource) \
   AL_FUNC(LPALSOURCEI, alSourcei) \
   AL_FUNC(LPALSOURCE3I, alSource3i) \
   AL_FUNC(LPALSOURCEIV, alSourceiv) \
   AL_FUNC(LPALSOURCEF, alSourcef) \
   AL_FUNC(LPALSOURCE3F, alSource3f) \
   AL_FUNC(LPALSOURCEFV, alSourcefv) \
   AL_FUNC(LPALGETSOURCEI, alGetSourcei) \
   AL_FUNC(LPALGETSOURCEF, alGetSourcef) \
   AL_FUNC(LPALGETSOURCEFV, alGetSourcefv) \
   AL_FUNC(LPALSOURCEPLAYV, alSourcePlayv) \
   AL_FUNC(LPALSOURCESTOPV, alSourceStopv) \
   AL_FUNC(LPALSOURCEPLAY, alSourcePlay) \
   AL_FUNC(LPALSOURCEPAUSE, alSourcePause) \
   AL_FUNC(LPALSOURCESTOP, alSourceStop) \
   AL_FUNC(LPALGENBUFFERS, alGenBuffers) \
   AL_FUNC(LPALDELETEBUFFERS, alDeleteBuffers) \
   AL_FUNC(LPALISBUFFER, alIsBuffer) \
   AL_FUNC(LPALBUFFERDATA, alBufferData) \
   AL_FUNC(LPALGETBUFFERI, alGetBufferi) \
   AL_FUNC(LPALGETBUFFERF, alGetBufferf) \
   AL_FUNC(LPALBUFFERI, alBufferi) \
   AL_FUNC(LPALSOURCEQUEUEBUFFERS, alSourceQueueBuffers) \
   AL_FUNC(LPALSOURCEUNQUEUEBUFFERS, alSourceUnqueueBuffers) \
   AL_FUNC(LPALDISTANCEMODEL, alDistanceModel) \
   AL_FUNC(LPALDOPPLERFACTOR, alDopplerFactor) \
   AL_FUNC(LPALDOPPLERVELOCITY, alDopplerVelocity) \
   AL_FUNC(LPALCGETSTRING, alcGetString) \
   AL_FUNC(LPALCGETINTEGERV, alcGetIntegerv) \
   AL_FUNC(LPALCOPENDEVICE, alcOpenDevice) \
   AL_FUNC(LPALCCLOSEDEVICE, alcCloseDevice) \
   AL_FUNC(LPALCCREATECONTEXT, alcCreateContext) \
   AL_FUNC(LPALCMAKECONTEXTCURRENT, alcMakeContextCurrent) \
   AL_FUNC(LPALCPROCESSCONTEXT, alcProcessContext) \
   AL_FUNC(LPALCGETCURRENTCONTEXT, alcGetCurrentContext) \
   AL_FUNC(LPALCGETCONTEXTSDEVICE, alcGetContextsDevice) \
   AL_FUNC(LPALCSUSPENDCONTEXT, alcSuspendContext) \
   AL_FUNC(LPALCDESTROYCONTEXT, alcDestroyContext) \
   AL_FUNC(LPALCGETERROR, alcGetError) \
   AL_FUNC(LPALCISEXTENSIONPRESENT, alcIsExtensionPresent) \
   AL_FUNC(LPALCGETPROCADDRESS, alcGetProcAddress) \
   AL_FUNC(LPALCGETENUMVALUE, alcGetEnumValue)

#define EFX_FUNCS \
   AL_FUNC(LPALGENEFFECTS, alGenEffects) \
   AL_FUNC(LPALDELETEEFFECTS, alDeleteEffects) \
   AL_FUNC(LPALISEFFECT, alIsEffect) \
   AL_FUNC(LPALEFFECTI, alEffecti) \
   AL_FUNC(LPALEFFECTIV, alEffectiv) \
   AL_FUNC(LPALEFFECTF, alEffectf) \
   AL_FUNC(LPALEFFECTFV, alEffectfv) \
   AL_FUNC(LPALGETEFFECTI, alGetEffecti) \
   AL_FUNC(LPALGETEFFECTIV, alGetEffectiv) \
   AL_FUNC(LPALGETEFFECTF, alGetEffectf) \
   AL_FUNC(LPALGETEFFECTFV, alGetEffectfv) \
   AL_FUNC(LPALGENFILTERS, alGenFilters) \
   AL_FUNC(LPALDELETEFILTERS, alDeleteFilters) \
   AL_FUNC(LPALISFILTER, alIsFilter) \
   AL_FUNC(LPALFILTERI, alFilteri) \
   AL_FUNC(LPALFILTERIV, alFilteriv) \
   AL_FUNC(LPALFILTERF, alFilterf) \
   AL_FUNC(LPALFILTERFV, alFilterfv) \
   AL_FUNC(LPALGETFILTERI, alGetFilteri) \
   AL_FUNC(LPALGETFILTERIV, alGetFilteriv) \
   AL_FUNC(LPALGETFILTERF, alGetFilterf) \
   AL_FUNC(LPALGETFILTERFV, alGetFilterfv) \
   AL_FUNC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots) \
   AL_FUNC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots) \
   AL_FUNC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot) \
   AL_FUNC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti) \
   AL_FUNC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv) \
   AL_FUNC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf) \
   AL_FUNC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv) \
   AL_FUNC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti) \
   AL_FUNC(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv) \
   AL_FUNC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf) \
   AL_FUNC(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv)

AL_FUNCS
EFX_FUNCS
#undef AL_FUNC

typedef ALboolean (AL_APIENTRY *LPEAXSETBUFFERMODE)(ALsizei n, ALuint *buffers, ALint value);
typedef ALenum (AL_APIENTRY *LPEAXGETBUFFERMODE)(ALuint buffer, ALint *value);

static LPEAXSETBUFFERMODE pEAXSetBufferMode;
static LPEAXGETBUFFERMODE pEAXGetBufferMode;
static ALenum eXRAMSize;
static ALenum eXRAMFree;
static ALenum eXRAMAuto;
static ALenum eXRAMHardware;
static ALenum eXRAMAccessible;

static HMODULE g_hOpenAL;

int g_nALStreamBufferLen = 250;

static void ALLogResolveFailure(const char *pszFmt, const char *pszName)
{
   LogMsg(_LogFmt(pszFmt, pszName));
}

#define AL_RESOLVE(type, name) \
   p##name = (type)GetProcAddress(g_hOpenAL, #name); \
   if (!p##name) \
   { \
      ALLogResolveFailure("Failed to resolve AL call: %s", #name); \
      return FALSE; \
   }

#define EFX_RESOLVE(type, name) \
   p##name = (type)palGetProcAddress(#name); \
   if (!p##name) \
   { \
      ALLogResolveFailure("Failed to resolve AL extension call: %s", #name); \
      return FALSE; \
   }

static BOOL ALInitEAXRAM(void)
{
   pEAXSetBufferMode = (LPEAXSETBUFFERMODE)palGetProcAddress("EAXSetBufferMode");
   pEAXGetBufferMode = (LPEAXGETBUFFERMODE)palGetProcAddress("EAXGetBufferMode");
   return pEAXSetBufferMode && pEAXGetBufferMode;
}

static BOOL ALInitXRAMEnums(void)
{
   eXRAMSize = palGetEnumValue("AL_EAX_RAM_SIZE");
   eXRAMFree = palGetEnumValue("AL_EAX_RAM_FREE");
   eXRAMAuto = palGetEnumValue("AL_STORAGE_AUTOMATIC");
   eXRAMHardware = palGetEnumValue("AL_STORAGE_HARDWARE");
   eXRAMAccessible = palGetEnumValue("AL_STORAGE_ACCESSIBLE");
   return eXRAMSize && eXRAMFree && eXRAMAuto && eXRAMHardware && eXRAMAccessible;
}

static BOOL ALInitEFX(void)
{
   EFX_RESOLVE(LPALGENEFFECTS, alGenEffects)
   EFX_RESOLVE(LPALDELETEEFFECTS, alDeleteEffects)
   EFX_RESOLVE(LPALISEFFECT, alIsEffect)
   EFX_RESOLVE(LPALEFFECTI, alEffecti)
   EFX_RESOLVE(LPALEFFECTIV, alEffectiv)
   EFX_RESOLVE(LPALEFFECTF, alEffectf)
   EFX_RESOLVE(LPALEFFECTFV, alEffectfv)
   EFX_RESOLVE(LPALGETEFFECTI, alGetEffecti)
   EFX_RESOLVE(LPALGETEFFECTIV, alGetEffectiv)
   EFX_RESOLVE(LPALGETEFFECTF, alGetEffectf)
   EFX_RESOLVE(LPALGETEFFECTFV, alGetEffectfv)
   EFX_RESOLVE(LPALGENFILTERS, alGenFilters)
   EFX_RESOLVE(LPALDELETEFILTERS, alDeleteFilters)
   EFX_RESOLVE(LPALISFILTER, alIsFilter)
   EFX_RESOLVE(LPALFILTERI, alFilteri)
   EFX_RESOLVE(LPALFILTERIV, alFilteriv)
   EFX_RESOLVE(LPALFILTERF, alFilterf)
   EFX_RESOLVE(LPALFILTERFV, alFilterfv)
   EFX_RESOLVE(LPALGETFILTERI, alGetFilteri)
   EFX_RESOLVE(LPALGETFILTERIV, alGetFilteriv)
   EFX_RESOLVE(LPALGETFILTERF, alGetFilterf)
   EFX_RESOLVE(LPALGETFILTERFV, alGetFilterfv)
   EFX_RESOLVE(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots)
   EFX_RESOLVE(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots)
   EFX_RESOLVE(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot)
   EFX_RESOLVE(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti)
   EFX_RESOLVE(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv)
   EFX_RESOLVE(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf)
   EFX_RESOLVE(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv)
   EFX_RESOLVE(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti)
   EFX_RESOLVE(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv)
   EFX_RESOLVE(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf)
   EFX_RESOLVE(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv)
   return TRUE;
}

static BOOL ALHasLowPassFilter(void)
{
   ALuint filter;
   palGetError();
   palGenFilters(1, &filter);
   if (palGetError() != AL_NO_ERROR)
      return FALSE;
   if (palIsFilter(filter))
   {
      palFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
      if (palGetError() != AL_NO_ERROR)
      {
         palDeleteFilters(1, &filter);
         return FALSE;
      }
   }
   palDeleteFilters(1, &filter);
   return TRUE;
}

struct sEAXReverbProps
{
   unsigned long ulEnvironment;
   float flEnvironmentSize;
   float flEnvironmentDiffusion;
   long lRoom;
   long lRoomHF;
   long lRoomLF;
   float flDecayTime;
   float flDecayHFRatio;
   float flDecayLFRatio;
   long lReflections;
   float flReflectionsDelay;
   float vReflectionsPan[3];
   long lReverb;
   float flReverbDelay;
   float vReverbPan[3];
   float flEchoTime;
   float flEchoDepth;
   float flModulationTime;
   float flModulationDepth;
   float flAirAbsorptionHF;
   float flHFReference;
   float flLFReference;
   float flRoomRolloffFactor;
   unsigned long ulFlags;
};

struct sEFXReverbProps
{
   float flDensity;
   float flDiffusion;
   float flGain;
   float flGainHF;
   float flDecayTime;
   float flDecayHFRatio;
   float flReflectionsGain;
   float flReflectionsDelay;
   float flLateReverbGain;
   float flLateReverbDelay;
   float flAirAbsorptionGainHF;
   float flRoomRolloffFactor;
   int iDecayHFLimit;
};

struct sEFXEAXReverbProps
{
   float flDensity;
   float flDiffusion;
   float flGain;
   float flGainHF;
   float flGainLF;
   float flDecayTime;
   float flDecayHFRatio;
   float flDecayLFRatio;
   float flReflectionsGain;
   float flReflectionsDelay;
   float flReflectionsPan[3];
   float flLateReverbGain;
   float flLateReverbDelay;
   float flLateReverbPan[3];
   float flEchoTime;
   float flEchoDepth;
   float flModulationTime;
   float flModulationDepth;
   float flAirAbsorptionGainHF;
   float flHFReference;
   float flLFReference;
   float flRoomRolloffFactor;
   int iDecayHFLimit;
};

#define AL_NUM_REVERB_PRESETS 26

static const sEAXReverbProps g_EAXReverbPresets[AL_NUM_REVERB_PRESETS] =
{
   { 0x0, 7.5f, 1.0f, -1000, -100, 0, 1.49f, 0.83f, 1.0f, -2602, 0.007f, { 0.0f, 0.0f, 0.0f }, 200, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x1, 1.4f, 1.0f, -1000, -6000, 0, 0.17f, 0.1f, 1.0f, -1204, 0.001f, { 0.0f, 0.0f, 0.0f }, 207, 0.002f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x2, 1.9f, 1.0f, -1000, -454, 0, 0.4f, 0.83f, 1.0f, -1646, 0.002f, { 0.0f, 0.0f, 0.0f }, 53, 0.003f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x3, 1.4f, 1.0f, -1000, -1200, 0, 1.49f, 0.54f, 1.0f, -370, 0.007f, { 0.0f, 0.0f, 0.0f }, 1030, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x4, 2.5f, 1.0f, -1000, -6000, 0, 0.5f, 0.1f, 1.0f, -1376, 0.003f, { 0.0f, 0.0f, 0.0f }, -1104, 0.004f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x5, 11.6f, 1.0f, -1000, -300, 0, 2.31f, 0.64f, 1.0f, -711, 0.012f, { 0.0f, 0.0f, 0.0f }, 83, 0.017f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x6, 21.6f, 1.0f, -1000, -476, 0, 4.32f, 0.59f, 1.0f, -789, 0.02f, { 0.0f, 0.0f, 0.0f }, -289, 0.03f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x7, 19.6f, 1.0f, -1000, -500, 0, 3.92f, 0.7f, 1.0f, -1230, 0.02f, { 0.0f, 0.0f, 0.0f }, -2, 0.029f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x8, 14.6f, 1.0f, -1000, 0, 0, 2.91f, 1.3f, 1.0f, -602, 0.015f, { 0.0f, 0.0f, 0.0f }, -302, 0.022f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x1f },
   { 0x9, 36.2f, 1.0f, -1000, -698, 0, 7.24f, 0.33f, 1.0f, -1166, 0.02f, { 0.0f, 0.0f, 0.0f }, 16, 0.03f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0xa, 50.3f, 1.0f, -1000, -1000, 0, 10.05f, 0.23f, 1.0f, -602, 0.02f, { 0.0f, 0.0f, 0.0f }, 198, 0.03f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0xb, 1.9f, 1.0f, -1000, -4000, 0, 0.3f, 0.1f, 1.0f, -1831, 0.002f, { 0.0f, 0.0f, 0.0f }, -1630, 0.03f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0xc, 1.8f, 1.0f, -1000, -300, 0, 1.49f, 0.59f, 1.0f, -1219, 0.007f, { 0.0f, 0.0f, 0.0f }, 441, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0xd, 13.5f, 1.0f, -1000, -237, 0, 2.7f, 0.79f, 1.0f, -1214, 0.013f, { 0.0f, 0.0f, 0.0f }, 395, 0.02f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0xe, 7.5f, 0.3f, -1000, -270, 0, 1.49f, 0.86f, 1.0f, -1204, 0.007f, { 0.0f, 0.0f, 0.0f }, -4, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.125f, 0.95f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0xf, 38.0f, 0.3f, -1000, -3300, 0, 1.49f, 0.54f, 1.0f, -2560, 0.162f, { 0.0f, 0.0f, 0.0f }, -229, 0.088f, { 0.0f, 0.0f, 0.0f }, 0.125f, 1.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x10, 7.5f, 0.5f, -1000, -800, 0, 1.49f, 0.67f, 1.0f, -2273, 0.007f, { 0.0f, 0.0f, 0.0f }, -1691, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x11, 100.0f, 0.27f, -1000, -2500, 0, 1.49f, 0.21f, 1.0f, -2780, 0.3f, { 0.0f, 0.0f, 0.0f }, -1434, 0.1f, { 0.0f, 0.0f, 0.0f }, 0.25f, 1.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x1f },
   { 0x12, 17.5f, 1.0f, -1000, -1000, 0, 1.49f, 0.83f, 1.0f, -10000, 0.061f, { 0.0f, 0.0f, 0.0f }, 500, 0.025f, { 0.0f, 0.0f, 0.0f }, 0.125f, 0.7f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x13, 42.5f, 0.21f, -1000, -2000, 0, 1.49f, 0.5f, 1.0f, -2466, 0.179f, { 0.0f, 0.0f, 0.0f }, -1926, 0.1f, { 0.0f, 0.0f, 0.0f }, 0.25f, 1.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x14, 8.3f, 1.0f, -1000, 0, 0, 1.65f, 1.5f, 1.0f, -1363, 0.008f, { 0.0f, 0.0f, 0.0f }, -1153, 0.012f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x1f },
   { 0x15, 1.7f, 0.8f, -1000, -1000, 0, 2.81f, 0.14f, 1.0f, 429, 0.014f, { 0.0f, 0.0f, 0.0f }, 1023, 0.021f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 0.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x16, 1.8f, 1.0f, -1000, -4000, 0, 1.49f, 0.1f, 1.0f, -449, 0.007f, { 0.0f, 0.0f, 0.0f }, 1700, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 1.18f, 0.348f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x3f },
   { 0x17, 1.9f, 0.5f, -1000, 0, 0, 8.39f, 1.39f, 1.0f, -115, 0.002f, { 0.0f, 0.0f, 0.0f }, 985, 0.03f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 0.25f, 1.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x1f },
   { 0x18, 1.8f, 0.6f, -1000, -400, 0, 17.23f, 0.56f, 1.0f, -1713, 0.02f, { 0.0f, 0.0f, 0.0f }, -613, 0.03f, { 0.0f, 0.0f, 0.0f }, 0.25f, 1.0f, 0.81f, 0.31f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x1f },
   { 0x19, 1.0f, 0.5f, -1000, -151, 0, 7.56f, 0.91f, 1.0f, -626, 0.02f, { 0.0f, 0.0f, 0.0f }, 774, 0.03f, { 0.0f, 0.0f, 0.0f }, 0.25f, 0.0f, 4.0f, 1.0f, -5.0f, 5000.0f, 250.0f, 0.0f, 0x1f },
};

static sEAXReverbProps g_EAXCustomReverbPresets[AL_NUM_REVERB_PRESETS];
static sEFXEAXReverbProps g_EFXEAXReverbPresets[AL_NUM_REVERB_PRESETS];
static sEFXReverbProps g_EFXReverbPresets[AL_NUM_REVERB_PRESETS];

static BOOL ALLoadReverbPresets(void)
{
   IStoreStream *pStream = ResOpenConfigFile("al_reverb.ini", NULL, NULL);
   if (!pStream)
      return FALSE;
   int nSize = pStream->GetSize();
   char *pText = (char *)malloc(nSize + 1);
   int nRead = pStream->Read(nSize, pText);
   pStream->Close();
   pStream->Release();
   if (nRead != nSize)
   {
      free(pText);
      mprintf("WARNING: failed to load al_reverb.ini\n");
      return FALSE;
   }
   pText[nSize] = '\0';
   memcpy(g_EAXCustomReverbPresets, g_EAXReverbPresets, sizeof(g_EAXCustomReverbPresets));

   char *p = pText;
   sEAXReverbProps *pPreset = g_EAXCustomReverbPresets;
   for (int nPreset = 0; *p && nPreset < AL_NUM_REVERB_PRESETS; )
   {
      while (*p && iswspace(*p))
         p++;
      if (*p == '#')
      {
         while (*p && *p != '\r' && *p != '\n')
            p++;
         while (*p == '\r' || *p == '\n')
            p++;
         continue;
      }
      char *pLine = p;
      while (*p && *p != '\r' && *p != '\n')
         p++;
      char *pEnd = p;
      while (*p == '\r' || *p == '\n')
         p++;
      *pEnd = '\0';

      char *apField[26];
      for (int i = 0; i < 26; i++)
      {
         while (*pLine && iswspace(*pLine))
            pLine++;
         apField[i] = pLine;
         char *pComma = strchr(pLine, ',');
         if (!pComma)
         {
            free(pText);
            mprintf("WARNING: syntax error in al_reverb.ini, preset %d\n", nPreset + 1);
            return FALSE;
         }
         *pComma = '\0';
         pLine = pComma + 1;
      }
      while (*pLine && iswspace(*pLine))
         pLine++;

      pPreset->flEnvironmentSize = (float)atof(apField[0]);
      pPreset->flEnvironmentDiffusion = (float)atof(apField[1]);
      pPreset->lRoom = atoi(apField[2]);
      pPreset->lRoomHF = atoi(apField[3]);
      pPreset->lRoomLF = atoi(apField[4]);
      pPreset->flDecayTime = (float)atof(apField[5]);
      pPreset->flDecayHFRatio = (float)atof(apField[6]);
      pPreset->flDecayLFRatio = (float)atof(apField[7]);
      pPreset->lReflections = atoi(apField[8]);
      pPreset->flReflectionsDelay = (float)atof(apField[9]);
      pPreset->vReflectionsPan[0] = (float)atof(apField[10]);
      pPreset->vReflectionsPan[1] = (float)atof(apField[11]);
      pPreset->vReflectionsPan[2] = (float)atof(apField[12]);
      pPreset->lReverb = atoi(apField[13]);
      pPreset->flReverbDelay = (float)atof(apField[14]);
      pPreset->vReverbPan[0] = (float)atof(apField[15]);
      pPreset->vReverbPan[1] = (float)atof(apField[16]);
      pPreset->vReverbPan[2] = (float)atof(apField[17]);
      pPreset->flEchoTime = (float)atof(apField[18]);
      pPreset->flEchoDepth = (float)atof(apField[19]);
      pPreset->flModulationTime = (float)atof(apField[20]);
      pPreset->flModulationDepth = (float)atof(apField[21]);
      pPreset->flAirAbsorptionHF = (float)atof(apField[22]);
      pPreset->flHFReference = (float)atof(apField[23]);
      pPreset->flLFReference = (float)atof(apField[24]);
      pPreset->flRoomRolloffFactor = (float)atof(apField[25]);
      pPreset->ulFlags = strtoul(pLine, NULL, 16);
      pPreset++;
      nPreset++;
   }
   free(pText);
   return TRUE;
}

static float ALmBToGain(double mB)
{
   if (mB == -10000.0)
      return 0.0f;
   return (float)pow(10.0, mB / 2000.0);
}

static void ALConvertEAXReverb(const sEAXReverbProps *pEAX, sEFXEAXReverbProps *pEFX)
{
   float flDensity = (float)(pow((double)pEAX->flEnvironmentSize, 3.0) / 16.0);
   if (flDensity > 1.0f)
      flDensity = 1.0f;
   pEFX->flDensity = flDensity;
   pEFX->flDiffusion = pEAX->flEnvironmentDiffusion;
   pEFX->flGain = ALmBToGain(pEAX->lRoom);
   pEFX->flGainHF = ALmBToGain(pEAX->lRoomHF);
   pEFX->flGainLF = ALmBToGain(pEAX->lRoomLF);
   pEFX->flDecayTime = pEAX->flDecayTime;
   pEFX->flDecayHFRatio = pEAX->flDecayHFRatio;
   pEFX->flDecayLFRatio = pEAX->flDecayLFRatio;
   pEFX->flReflectionsGain = ALmBToGain(pEAX->lReflections);
   pEFX->flReflectionsDelay = pEAX->flReflectionsDelay;
   pEFX->flReflectionsPan[0] = pEAX->vReflectionsPan[0];
   pEFX->flReflectionsPan[1] = pEAX->vReflectionsPan[1];
   pEFX->flReflectionsPan[2] = pEAX->vReflectionsPan[2];
   pEFX->flLateReverbGain = ALmBToGain(pEAX->lReverb);
   pEFX->flLateReverbDelay = pEAX->flReverbDelay;
   pEFX->flLateReverbPan[0] = pEAX->vReverbPan[0];
   pEFX->flLateReverbPan[1] = pEAX->vReverbPan[1];
   pEFX->flLateReverbPan[2] = pEAX->vReverbPan[2];
   pEFX->flEchoTime = pEAX->flEchoTime;
   pEFX->flEchoDepth = pEAX->flEchoDepth;
   pEFX->flModulationTime = pEAX->flModulationTime;
   pEFX->flModulationDepth = pEAX->flModulationDepth;
   pEFX->flAirAbsorptionGainHF = ALmBToGain(pEAX->flAirAbsorptionHF);
   pEFX->flHFReference = pEAX->flHFReference;
   pEFX->flLFReference = pEAX->flLFReference;
   pEFX->flRoomRolloffFactor = pEAX->flRoomRolloffFactor;
   pEFX->iDecayHFLimit = (pEAX->ulFlags >> 5) & 1;
}

static void ALConvertEAXReverbStd(const sEAXReverbProps *pEAX, sEFXReverbProps *pEFX)
{
   float flDensity = (float)(pow((double)pEAX->flEnvironmentSize, 3.0) / 16.0);
   if (flDensity > 1.0f)
      flDensity = 1.0f;
   pEFX->flDensity = flDensity;
   pEFX->flDiffusion = pEAX->flEnvironmentDiffusion;
   pEFX->flGain = ALmBToGain(pEAX->lRoom);
   pEFX->flGainHF = ALmBToGain(pEAX->lRoomHF);
   pEFX->flDecayTime = pEAX->flDecayTime;
   pEFX->flDecayHFRatio = pEAX->flDecayHFRatio;
   pEFX->flReflectionsGain = ALmBToGain(pEAX->lReflections);
   pEFX->flReflectionsDelay = pEAX->flReflectionsDelay;
   pEFX->flLateReverbGain = ALmBToGain(pEAX->lReverb);
   pEFX->flLateReverbDelay = pEAX->flReverbDelay;
   pEFX->flAirAbsorptionGainHF = ALmBToGain(pEAX->flAirAbsorptionHF);
   pEFX->flRoomRolloffFactor = pEAX->flRoomRolloffFactor;
   pEFX->iDecayHFLimit = (pEAX->ulFlags >> 5) & 1;
}

static BOOL ALLoad(void)
{
   if (g_hOpenAL)
      return TRUE;
   g_hOpenAL = LoadLibraryA("openal32.dll");
   if (!g_hOpenAL)
      return FALSE;

   const sEAXReverbProps *pPresets = ALLoadReverbPresets() ? g_EAXCustomReverbPresets : g_EAXReverbPresets;
   for (int i = 0; i < AL_NUM_REVERB_PRESETS; i++)
   {
      ALConvertEAXReverb(&pPresets[i], &g_EFXEAXReverbPresets[i]);
      ALConvertEAXReverbStd(&pPresets[i], &g_EFXReverbPresets[i]);
   }

   AL_RESOLVE(LPALENABLE, alEnable)
   AL_RESOLVE(LPALDISABLE, alDisable)
   AL_RESOLVE(LPALISENABLED, alIsEnabled)
   AL_RESOLVE(LPALGETBOOLEAN, alGetBoolean)
   AL_RESOLVE(LPALGETINTEGER, alGetInteger)
   AL_RESOLVE(LPALGETFLOAT, alGetFloat)
   AL_RESOLVE(LPALGETDOUBLE, alGetDouble)
   AL_RESOLVE(LPALGETBOOLEANV, alGetBooleanv)
   AL_RESOLVE(LPALGETINTEGERV, alGetIntegerv)
   AL_RESOLVE(LPALGETFLOATV, alGetFloatv)
   AL_RESOLVE(LPALGETDOUBLEV, alGetDoublev)
   AL_RESOLVE(LPALGETSTRING, alGetString)
   AL_RESOLVE(LPALGETERROR, alGetError)
   AL_RESOLVE(LPALISEXTENSIONPRESENT, alIsExtensionPresent)
   AL_RESOLVE(LPALGETPROCADDRESS, alGetProcAddress)
   AL_RESOLVE(LPALGETENUMVALUE, alGetEnumValue)
   AL_RESOLVE(LPALLISTENERI, alListeneri)
   AL_RESOLVE(LPALLISTENERF, alListenerf)
   AL_RESOLVE(LPALLISTENER3F, alListener3f)
   AL_RESOLVE(LPALLISTENERFV, alListenerfv)
   AL_RESOLVE(LPALGETLISTENERI, alGetListeneri)
   AL_RESOLVE(LPALGETLISTENERF, alGetListenerf)
   AL_RESOLVE(LPALGETLISTENER3F, alGetListener3f)
   AL_RESOLVE(LPALGETLISTENERFV, alGetListenerfv)
   AL_RESOLVE(LPALGENSOURCES, alGenSources)
   AL_RESOLVE(LPALDELETESOURCES, alDeleteSources)
   AL_RESOLVE(LPALISSOURCE, alIsSource)
   AL_RESOLVE(LPALSOURCEI, alSourcei)
   AL_RESOLVE(LPALSOURCE3I, alSource3i)
   AL_RESOLVE(LPALSOURCEIV, alSourceiv)
   AL_RESOLVE(LPALSOURCEF, alSourcef)
   AL_RESOLVE(LPALSOURCE3F, alSource3f)
   AL_RESOLVE(LPALSOURCEFV, alSourcefv)
   AL_RESOLVE(LPALGETSOURCEI, alGetSourcei)
   AL_RESOLVE(LPALGETSOURCEF, alGetSourcef)
   AL_RESOLVE(LPALGETSOURCEFV, alGetSourcefv)
   AL_RESOLVE(LPALSOURCEPLAYV, alSourcePlayv)
   AL_RESOLVE(LPALSOURCESTOPV, alSourceStopv)
   AL_RESOLVE(LPALSOURCEPLAY, alSourcePlay)
   AL_RESOLVE(LPALSOURCEPAUSE, alSourcePause)
   AL_RESOLVE(LPALSOURCESTOP, alSourceStop)
   AL_RESOLVE(LPALGENBUFFERS, alGenBuffers)
   AL_RESOLVE(LPALDELETEBUFFERS, alDeleteBuffers)
   AL_RESOLVE(LPALISBUFFER, alIsBuffer)
   AL_RESOLVE(LPALBUFFERDATA, alBufferData)
   AL_RESOLVE(LPALGETBUFFERI, alGetBufferi)
   AL_RESOLVE(LPALGETBUFFERF, alGetBufferf)
   AL_RESOLVE(LPALBUFFERI, alBufferi)
   AL_RESOLVE(LPALSOURCEQUEUEBUFFERS, alSourceQueueBuffers)
   AL_RESOLVE(LPALSOURCEUNQUEUEBUFFERS, alSourceUnqueueBuffers)
   AL_RESOLVE(LPALDISTANCEMODEL, alDistanceModel)
   AL_RESOLVE(LPALDOPPLERFACTOR, alDopplerFactor)
   AL_RESOLVE(LPALDOPPLERVELOCITY, alDopplerVelocity)
   AL_RESOLVE(LPALCGETSTRING, alcGetString)
   AL_RESOLVE(LPALCGETINTEGERV, alcGetIntegerv)
   AL_RESOLVE(LPALCOPENDEVICE, alcOpenDevice)
   AL_RESOLVE(LPALCCLOSEDEVICE, alcCloseDevice)
   AL_RESOLVE(LPALCCREATECONTEXT, alcCreateContext)
   AL_RESOLVE(LPALCMAKECONTEXTCURRENT, alcMakeContextCurrent)
   AL_RESOLVE(LPALCPROCESSCONTEXT, alcProcessContext)
   AL_RESOLVE(LPALCGETCURRENTCONTEXT, alcGetCurrentContext)
   AL_RESOLVE(LPALCGETCONTEXTSDEVICE, alcGetContextsDevice)
   AL_RESOLVE(LPALCSUSPENDCONTEXT, alcSuspendContext)
   AL_RESOLVE(LPALCDESTROYCONTEXT, alcDestroyContext)
   AL_RESOLVE(LPALCGETERROR, alcGetError)
   AL_RESOLVE(LPALCISEXTENSIONPRESENT, alcIsExtensionPresent)
   AL_RESOLVE(LPALCGETPROCADDRESS, alcGetProcAddress)
   AL_RESOLVE(LPALCGETENUMVALUE, alcGetEnumValue)
   return TRUE;
}

static BOOL ALGetFileVersion(const char *pszFile, WORD *pVersion)
{
   typedef BOOL (WINAPI *tVerQueryValue)(LPCVOID, LPCSTR, LPVOID *, PUINT);
   typedef BOOL (WINAPI *tGetFileVersionInfo)(LPCSTR, DWORD, DWORD, LPVOID);
   typedef DWORD (WINAPI *tGetFileVersionInfoSize)(LPCSTR, LPDWORD);

   HMODULE hVersion = LoadLibraryA("version.dll");
   if (!hVersion)
      return FALSE;
   tVerQueryValue pVerQueryValue = (tVerQueryValue)GetProcAddress(hVersion, "VerQueryValueA");
   tGetFileVersionInfo pGetFileVersionInfo = (tGetFileVersionInfo)GetProcAddress(hVersion, "GetFileVersionInfoA");
   tGetFileVersionInfoSize pGetFileVersionInfoSize = (tGetFileVersionInfoSize)GetProcAddress(hVersion, "GetFileVersionInfoSizeA");
   BOOL bResult = FALSE;
   if (pVerQueryValue && pGetFileVersionInfo && pGetFileVersionInfoSize)
   {
      DWORD dwHandle = 0;
      DWORD dwSize = pGetFileVersionInfoSize(pszFile, &dwHandle);
      if (dwSize)
      {
         void *pData = operator new(dwSize);
         if (pGetFileVersionInfo(pszFile, dwHandle, dwSize, pData))
         {
            VS_FIXEDFILEINFO *pInfo;
            UINT nLen;
            if (pVerQueryValue(pData, "\\", (void **)&pInfo, &nLen))
            {
               pVersion[0] = HIWORD(pInfo->dwFileVersionMS);
               pVersion[1] = LOWORD(pInfo->dwFileVersionMS);
               pVersion[2] = HIWORD(pInfo->dwFileVersionLS);
               pVersion[3] = LOWORD(pInfo->dwFileVersionLS);
               bResult = TRUE;
            }
         }
         operator delete(pData);
      }
   }
   FreeLibrary(hVersion);
   return bResult;
}

struct sALDeviceInfo
{
   cAnsiStr strName;
   int nMajorVersion;
   int nMinorVersion;
   int n3DSources;
   int n2DSources;
   BOOL bCapture;
   BOOL bEFX;
   BOOL bOffset;
   BOOL bLinearDistance;
   BOOL bExponentDistance;
   BOOL bEAX2;
   BOOL bEAX3;
   BOOL bEAX4;
   BOOL bEAX5;
   BOOL bXRAM;
   BOOL bOcclusion;
   ALCdevice *pDevice;
   ALCcontext *pContext;
   sALDeviceInfo *pNext;

   sALDeviceInfo()
      : pDevice(NULL), pContext(NULL), pNext(NULL)
   {
   }

   sALDeviceInfo &operator=(const sALDeviceInfo &info)
   {
      strName = info.strName;
      nMajorVersion = info.nMajorVersion;
      nMinorVersion = info.nMinorVersion;
      n3DSources = info.n3DSources;
      n2DSources = info.n2DSources;
      bCapture = info.bCapture;
      bEFX = info.bEFX;
      bOffset = info.bOffset;
      bLinearDistance = info.bLinearDistance;
      bExponentDistance = info.bExponentDistance;
      bEAX2 = info.bEAX2;
      bEAX3 = info.bEAX3;
      bEAX4 = info.bEAX4;
      bEAX5 = info.bEAX5;
      bXRAM = info.bXRAM;
      bOcclusion = info.bOcclusion;
      pDevice = info.pDevice;
      pContext = info.pContext;
      return *this;
   }
};

class cALDeviceList
{
public:
   cALDeviceList(const sSndAttribs &attribs);
   ~cALDeviceList();

   void Enumerate();
   BOOL OpenDevice(const char *pszName, BOOL bKeepOpen, sALDeviceInfo **ppInfo);
   sALDeviceInfo *Find(const char *pszName);
   int GetIndex(const char *pszName);
   void Clear();

   int Count() const
   {
      return m_nDevices;
   }

   sALDeviceInfo *Get(int i) const
   {
      return (unsigned)i < m_nDevices ? m_apDevices[i] : NULL;
   }

   int DefaultIndex() const
   {
      return m_iDefault;
   }

private:
   sALDeviceInfo *m_pHead;
   sALDeviceInfo *m_apDevices[AL_MAX_DEVICES];
   unsigned m_nDevices;
   int m_iDefault;
   BOOL m_bEnumerated;
   sSndAttribs m_Attribs;
};

static void ALFreeDeviceInfo(sALDeviceInfo *pInfo)
{
   delete pInfo;
}

void cALDeviceList::Clear()
{
   while (m_pHead)
   {
      sALDeviceInfo *pNext = m_pHead->pNext;
      delete m_pHead;
      m_pHead = pNext;
   }
}

cALDeviceList::cALDeviceList(const sSndAttribs &attribs)
{
   m_Attribs = attribs;
   m_pHead = NULL;
   memset(m_apDevices, 0, sizeof(m_apDevices));
   m_nDevices = 0;
   m_iDefault = -1;
   m_bEnumerated = FALSE;

   if (!palcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
      return;

   if (config_is_defined("snd_oal_full_enum"))
   {
      Enumerate();
      return;
   }

   LogMsg("Enumerating devices without validation...");
   LogIncIndent();
   const char *pszDevice = palcGetString(NULL, ALC_DEVICE_SPECIFIER);
   if (pszDevice)
   {
      int nFound = 0;
      while (*pszDevice)
      {
         LogMsg(_LogFmt("Device: %s", pszDevice));
         pszDevice += strlen(pszDevice) + 1;
         nFound++;
      }
      LogMsg(_LogFmt("found %d unvalidated device(s)", nFound));
   }
   LogDecIndent();

   if (!palcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
      return;

   LogMsg("Enumerating extended devices without validation...");
   LogIncIndent();
   pszDevice = palcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
   if (pszDevice)
   {
      int nFound = 0;
      while (*pszDevice)
      {
         LogMsg(_LogFmt("Device: %s", pszDevice));
         pszDevice += strlen(pszDevice) + 1;
         nFound++;
      }
      LogMsg(_LogFmt("found %d unvalidated extended device(s)", nFound));
   }
   LogDecIndent();
}

cALDeviceList::~cALDeviceList()
{
   Clear();
}

void cALDeviceList::Enumerate()
{
   if (m_bEnumerated)
      return;

   Clear();
   m_pHead = NULL;
   memset(m_apDevices, 0, sizeof(m_apDevices));
   m_nDevices = 0;
   m_iDefault = -1;

   LogMsg("Enumerating devices...");
   LogIncIndent();
   const char *pszDevice = palcGetString(NULL, ALC_DEVICE_SPECIFIER);
   if (!pszDevice)
   {
      m_bEnumerated = TRUE;
      LogDecIndent();
      return;
   }
   while (*pszDevice)
   {
      OpenDevice(pszDevice, FALSE, NULL);
      pszDevice += strlen(pszDevice) + 1;
   }
   LogMsg(_LogFmt("found %d validated device(s)", m_nDevices));
   LogDecIndent();
   m_bEnumerated = TRUE;
}

sALDeviceInfo *cALDeviceList::Find(const char *pszName)
{
   if (!pszName || !*pszName)
      return NULL;
   for (sALDeviceInfo *pInfo = m_pHead; pInfo; pInfo = pInfo->pNext)
      if (!_stricmp(pInfo->strName, pszName))
         return pInfo;
   return NULL;
}

int cALDeviceList::GetIndex(const char *pszName)
{
   if (!pszName || !*pszName)
      return -1;
   for (int i = 0; i < m_nDevices; i++)
      if (!_stricmp(m_apDevices[i]->strName, pszName))
         return i;
   return -1;
}

static const char *ALSpeakerConfigName(int nConfig, ALCenum *pFormat)
{
   switch (nConfig)
   {
      case 1:
         *pFormat = ALC_MONO_SOFT;
         return "Mono";
      case 4:
         *pFormat = ALC_QUAD_SOFT;
         return "Quadraphonic";
      case 6:
      case 9:
         *pFormat = ALC_5POINT1_SOFT;
         return "5.1 Surround";
      case 7:
         *pFormat = ALC_6POINT1_SOFT;
         return "6.1 Surround";
      case 8:
      case 10:
         *pFormat = ALC_7POINT1_SOFT;
         return "7.1 Surround";
   }
   *pFormat = ALC_STEREO_SOFT;
   return "Stereo";
}

BOOL cALDeviceList::OpenDevice(const char *pszName, BOOL bKeepOpen, sALDeviceInfo **ppInfo)
{
   int iIndex = GetIndex(pszName);
   sALDeviceInfo *pExisting = Get(iIndex);
   if (!pExisting && m_nDevices == AL_MAX_DEVICES)
      return FALSE;
   if (pExisting)
   {
      if (ppInfo)
         *ppInfo = pExisting;
      if (!bKeepOpen || pExisting->pDevice)
         return TRUE;
   }

   ALCdevice *pDevice = palcOpenDevice(pszName);
   if (!pDevice)
   {
      LogMsg(_LogFmt("Skipping device \"%s\", failed to open", pszName));
      return FALSE;
   }

   int n2DChannels = 8;
   if (config_get_int("snd_oal_2d_channels", &n2DChannels))
   {
      if (n2DChannels < 4)
         n2DChannels = 4;
      else if (n2DChannels > 64)
         n2DChannels = 64;
   }

   ALCint aDefaultAttribs[] =
   {
      ALC_FREQUENCY, m_Attribs.sampleRate,
      ALC_STEREO_SOURCES, n2DChannels,
      0, 0
   };

   ALCcontext *pContext = NULL;
   int nSpeakers = 0;
   config_get_int("snd_speaker_config", &nSpeakers);
   if (nSpeakers != 3 && nSpeakers > 1 && nSpeakers <= 10
       && palcIsExtensionPresent(pDevice, "ALC_SOFT_loopback"))
   {
      ALCenum eChannels = palcGetEnumValue(pDevice, "ALC_FORMAT_CHANNELS_SOFT");
      if (!eChannels || eChannels == -1)
         LogMsg(_LogFmt("Current AL device does not support selecting speaker configuration, speaker setting %d ignored\n", nSpeakers));
      else
      {
         ALCenum eFormat;
         const char *pszConfig = ALSpeakerConfigName(nSpeakers, &eFormat);
         ALCint aAttribs[] =
         {
            ALC_FREQUENCY, m_Attribs.sampleRate,
            ALC_STEREO_SOURCES, n2DChannels,
            eChannels, eFormat,
            0, 0
         };
         palcGetError(pDevice);
         pContext = palcCreateContext(pDevice, aAttribs);
         if (!pContext)
         {
            palcGetError(pDevice);
            ALCint aAttribs2[] =
            {
               ALC_FREQUENCY, m_Attribs.sampleRate,
               eChannels, eFormat,
               0, 0
            };
            pContext = palcCreateContext(pDevice, aAttribs2);
            if (!pContext)
               LogMsg(_LogFmt("WARNING: Current AL device could not initialize with user defined speaker config \"%s\" (%d), trying with default (err 0x%X)", pszConfig, nSpeakers, palcGetError(pDevice)));
         }
      }
   }

   if (!pContext)
   {
      pContext = palcCreateContext(pDevice, aDefaultAttribs);
      if (!pContext)
      {
         ALCint aMinAttribs[] = { ALC_FREQUENCY, m_Attribs.sampleRate, 0, 0 };
         pContext = palcCreateContext(pDevice, aMinAttribs);
         if (!pContext)
         {
            palcCloseDevice(pDevice);
            LogMsg(_LogFmt("Skipping device \"%s\", failed to create context", pszName));
            return FALSE;
         }
      }
   }

   BOOL bValid = FALSE;
   palcMakeContextCurrent(pContext);
   if (pExisting && m_bEnumerated)
   {
      pExisting->pContext = pContext;
      pExisting->pDevice = pDevice;
      return TRUE;
   }

   const char *pszDeviceName = palcGetString(pDevice, ALC_DEVICE_SPECIFIER);
   if (!Find(pszDeviceName))
   {
      sALDeviceInfo *pInfo = pExisting ? pExisting : new sALDeviceInfo;
      pInfo->strName = pszName;
      palcGetIntegerv(pDevice, ALC_MAJOR_VERSION, sizeof(int), &pInfo->nMajorVersion);
      palcGetIntegerv(pDevice, ALC_MINOR_VERSION, sizeof(int), &pInfo->nMinorVersion);
      pInfo->bCapture = palcIsExtensionPresent(pDevice, "ALC_EXT_CAPTURE");
      pInfo->bEFX = palcIsExtensionPresent(pDevice, "ALC_EXT_EFX") && ALInitEFX();
      pInfo->bOffset = palIsExtensionPresent("AL_EXT_OFFSET");
      pInfo->bLinearDistance = palIsExtensionPresent("AL_EXT_LINEAR_DISTANCE");
      pInfo->bExponentDistance = palIsExtensionPresent("AL_EXT_EXPONENT_DISTANCE");
      pInfo->bEAX2 = palIsExtensionPresent("EAX2.0");
      pInfo->bEAX3 = palIsExtensionPresent("EAX3.0");
      pInfo->bEAX4 = palIsExtensionPresent("EAX4.0");
      pInfo->bEAX5 = palIsExtensionPresent("EAX5.0");
      pInfo->bXRAM = palIsExtensionPresent("EAX-RAM") && ALInitEAXRAM();
      pInfo->bOcclusion = pInfo->bEFX && ALHasLowPassFilter();

      ALCint n3D = 0;
      palcGetIntegerv(pDevice, ALC_MONO_SOURCES, 1, &n3D);
      if (n3D < 2)
         n3D = 16;
      pInfo->n3DSources = n3D;
      ALCint n2D = 0;
      palcGetIntegerv(pDevice, ALC_STEREO_SOURCES, 1, &n2D);
      if (n2D < 2)
         n2D = 6;
      pInfo->n2DSources = n2D;

      if (pInfo->nMajorVersion < 0 || (pInfo->nMajorVersion == 1 && pInfo->nMinorVersion < 1))
      {
         LogMsg(_LogFmt("Skipping device \"%s\", does not support minimum requirements", (const char *)pInfo->strName));
         if (!pExisting)
            ALFreeDeviceInfo(pInfo);
      }
      else
      {
         if (!pExisting)
         {
            if (!m_pHead)
               m_pHead = pInfo;
            else
            {
               sALDeviceInfo *pTail = m_pHead;
               while (pTail->pNext)
                  pTail = pTail->pNext;
               pTail->pNext = pInfo;
            }
            pInfo->pNext = NULL;
            const char *pszDefault = palcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
            if (pszDefault && !_stricmp(pszName, pszDefault))
               m_iDefault = m_nDevices;
            m_apDevices[m_nDevices++] = pInfo;
         }
         bValid = TRUE;
         if (bKeepOpen)
         {
            pInfo->pDevice = pDevice;
            pInfo->pContext = pContext;
         }
         if (ppInfo)
            *ppInfo = pInfo;

         int iDevice = pExisting ? iIndex : m_nDevices - 1;
         LogMsg(_LogFmt("Device %d%s", iDevice, iDevice == m_iDefault ? " (default)" : ""));
         LogIncIndent();
         LogMsg(_LogFmt("name       : %s", (const char *)pInfo->strName));
         LogMsg(_LogFmt("version    : %d.%d", pInfo->nMajorVersion, pInfo->nMinorVersion));
         const char *psz = palGetString(AL_VERSION);
         if (psz)
            LogMsg(_LogFmt("version str: %s", psz));
         psz = palGetString(AL_VENDOR);
         if (psz)
            LogMsg(_LogFmt("vendor     : %s", psz));
         psz = palGetString(AL_RENDERER);
         if (psz)
            LogMsg(_LogFmt("renderer   : %s", psz));
         LogMsg(_LogFmt("3D sources : %d", n3D));
         LogMsg(_LogFmt("2D sources : %d", n2D));
         LogMsg(_LogFmt("EFX        : %d", pInfo->bEFX));
         LogMsg(_LogFmt("EAX2       : %d", pInfo->bEAX2));
         LogMsg(_LogFmt("EAX3       : %d", pInfo->bEAX3));
         LogMsg(_LogFmt("EAX4       : %d", pInfo->bEAX4));
         LogMsg(_LogFmt("EAX5       : %d", pInfo->bEAX5));
         LogMsg(_LogFmt("XRAM       : %d", pInfo->bXRAM));
         LogMsg(_LogFmt("occlusion  : %d", pInfo->bOcclusion));
         LogDecIndent();
      }
   }

   if (!bValid || !bKeepOpen)
   {
      palcMakeContextCurrent(NULL);
      palcDestroyContext(pContext);
      palcCloseDevice(pDevice);
   }
   return bValid;
}

static BOOL ALOpenDevice(ALCdevice **ppDevice, ALCcontext **ppContext, sALDeviceInfo *pInfoOut, const sSndAttribs &attribs)
{
   ALCdevice *pDevice = NULL;
   ALCcontext *pContext = NULL;
   BOOL bOpened = FALSE;
   if (ppDevice)
      *ppDevice = NULL;
   if (ppContext)
      *ppContext = NULL;

   if (!ALLoad())
   {
      LogMsg("OpenAL not found on system or failed to load, install AL redist package or chose other sound driver");
      return FALSE;
   }

   if (LogGetOptions() & kLogToAll)
   {
      WORD aVersion[4];
      if (ALGetFileVersion("openal32.dll", aVersion))
         LogMsg(_LogFmt("OpenAL32.dll: %d.%d.%d.%d", aVersion[0], aVersion[1], aVersion[2], aVersion[3]));
      if (ALGetFileVersion("wrap_oal.dll", aVersion))
         LogMsg(_LogFmt("wrap_oal.dll: %d.%d.%d.%d", aVersion[0], aVersion[1], aVersion[2], aVersion[3]));
      const char *pszDefault = palcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
      if (pszDefault)
         LogMsg(_LogFmt("Default device: %s", pszDefault));
   }


   cALDeviceList devices(attribs);
   sALDeviceInfo *pInfo = NULL;
   char szDevice[256];
   memset(szDevice, 0, sizeof(szDevice));

   if (config_get_raw("snd_oal_device", szDevice, sizeof(szDevice)) && szDevice[0])
   {
      LogMsg(_LogFmt("Opening user defined OpenAL device \"%s\"...", szDevice));
      LogIncIndent();
      if (devices.OpenDevice(szDevice, TRUE, &pInfo))
      {
         pDevice = pInfo->pDevice;
         pContext = pInfo->pContext;
         pInfo->pDevice = NULL;
         bOpened = TRUE;
         pInfo->pContext = NULL;
      }
      else
         LogMsg("failed to open device");
      LogDecIndent();
   }

   if (!bOpened)
   {
      const char *pszDefault = palcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
      if (pszDefault && strcmp(szDevice, pszDefault))
      {
         LogMsg(_LogFmt("Opening default OpenAL device \"%s\"...", pszDefault));
         LogIncIndent();
         if (devices.OpenDevice(pszDefault, TRUE, &pInfo))
         {
            pDevice = pInfo->pDevice;
            pContext = pInfo->pContext;
            pInfo->pDevice = NULL;
            bOpened = TRUE;
            pInfo->pContext = NULL;
         }
         else
            LogMsg("failed to open device");
         LogDecIndent();
      }
   }

   if (!bOpened)
   {
      devices.Enumerate();
      if (!devices.Count())
      {
         LogMsg("No usable devices found");
         return FALSE;
      }
      int iUser = szDevice[0] ? devices.GetIndex(szDevice) : -1;
      for (int i = 0; i < devices.Count(); i++)
      {
         if (i == devices.DefaultIndex() || i == iUser)
            continue;
         pInfo = devices.Get(i);
         LogMsg(_LogFmt("Opening OpenAL device \"%s\"...", (const char *)pInfo->strName));
         LogIncIndent();
         if (devices.OpenDevice(pInfo->strName, TRUE, &pInfo))
         {
            pDevice = pInfo->pDevice;
            pContext = pInfo->pContext;
            pInfo->pDevice = NULL;
            bOpened = TRUE;
            pInfo->pContext = NULL;
            LogDecIndent();
            break;
         }
         LogMsg("failed to open device");
         LogDecIndent();
      }
      if (!bOpened)
      {
         LogMsg("Failed to find or open any device");
         return FALSE;
      }
   }

   if (pInfo->bEFX)
      ALInitEFX();
   if (pInfo->bXRAM)
      ALInitEAXRAM();
   LogMsg(_LogFmt("Opened device '%s'", (const char *)pInfo->strName));
   if (pInfoOut)
      *pInfoOut = *pInfo;
   if (ppDevice)
      *ppDevice = pDevice;
   if (ppContext)
      *ppContext = pContext;

   int nBufLen = g_nALStreamBufferLen;
   if (config_get_int("snd_oal_streambufflen", &nBufLen))
   {
      if (nBufLen < 25)
         nBufLen = 25;
      else if (nBufLen > 2000)
         nBufLen = 2000;
      g_nALStreamBufferLen = nBufLen;
      LogMsg(_LogFmt("Using custom stream buffer length %d", g_nALStreamBufferLen));
   }
   return TRUE;
}

class cALSndSample;

class cALSndMixer : public cSndMixer
{
public:
   cALSndMixer();
   virtual ~cALSndMixer();

   STDMETHOD_(eSndError, Init)(sSndSetup *setup, uint32 numChannels, sSndAttribs *attr);
   STDMETHOD_(ISndSample *, CreateRawSample)(eSndSampleCreateFlagSet flags, void *pData,
                                             uint32 dlen, uint32 nsamps, sSndAttribs *attribs);
   STDMETHOD_(void, Pause)(void);
   STDMETHOD_(void, Resume)(void);
   STDMETHOD_(void, Set3DPosition)(sSndVector *pPosition);
   STDMETHOD_(void, Set3DOrientation)(sSndVector *pFront, sSndVector *pTop);
   STDMETHOD_(void, Set3DVelocity)(sSndVector *pVelocity);
   STDMETHOD_(void, Set3DEnvironment)(sSndEnvironment *pEnv);
   STDMETHOD_(void, Set3DMethod)(eSnd3DMethod method);
   STDMETHOD_(void, Get3DMethodCapabilities)(uint32 *pMethods);
   STDMETHOD_(void, Set3DDeferMode)(BOOL deferOn);
   STDMETHOD_(void, FreeHWChannelCount)(int32 *pHWChans, int32 *p3DHWChans);
   STDMETHOD_(BOOL, Have3DReverb)(void);
   STDMETHOD_(BOOL, CanDo3DReverb)(void);
   STDMETHOD_(BOOL, Set3DReverbSettings)(ReverbSettings *pReverbSettings);
   STDMETHOD_(BOOL, Get3DReverbSettings)(ReverbSettings *pReverbSettings);
   STDMETHOD_(BOOL, Have3DOcclusion)(void);
   STDMETHOD_(int32, Kludge)(int kludgeSelector, void *pKludgeStruct, int32 sizeKludgeStruct);

   virtual BOOL InitReverb(void);
   virtual void ShutdownReverb(void);

   BOOL UseEAXReverb() const
   {
      return mbEAXReverb;
   }

   BOOL HaveOcclusion() const
   {
      return mbOcclusion;
   }

   ALuint ReverbSlot() const
   {
      return mReverbSlot;
   }

   float DistanceFactor() const
   {
      return m3DEnvironment.distanceFactor;
   }

   float RolloffFactor() const
   {
      return m3DEnvironment.rolloffFactor;
   }

protected:
   BOOL mbFirstUpdate;
   ALCdevice *mpDevice;
   ALCcontext *mpContext;

public:
   BOOL mbPanVolInUse;

protected:
   ALuint mReverbEffect;
   ALuint mReverbSlot;
   BOOL mbEFX;
   BOOL mbEAXReverb;
   BOOL mbXRAM;
   BOOL mbOcclusion;
};


cALSndMixer::cALSndMixer()
{
   mbFirstUpdate = TRUE;
   mpDevice = NULL;
   mpContext = NULL;
   m3DDeferFlag = 0;
   mbPanVolInUse = FALSE;
   mbEFX = FALSE;
   mbEAXReverb = FALSE;
   mbXRAM = FALSE;
   mbOcclusion = FALSE;
   mReverbEffect = (ALuint)-1;
   mReverbSlot = (ALuint)-1;
}

cALSndMixer::~cALSndMixer()
{
   int cookie = mpDisplayDevice ? BlockDisplay() : 0;
   Destroy();
   ShutdownReverb();
   if (mpContext)
   {
      palcMakeContextCurrent(NULL);
      palcDestroyContext(mpContext);
      mpContext = NULL;
   }
   if (mpDevice)
   {
      palcCloseDevice(mpDevice);
      mpDevice = NULL;
   }
   if (mpDisplayDevice)
      ReleaseDisplay(cookie);
}

STDMETHODIMP_(eSndError) cALSndMixer::Init(sSndSetup *setup, uint32 numChannels, sSndAttribs *pAttribs)
{
   if (mpDevice)
      return kSndDeviceAlready;

   if (pAttribs == NULL)
   {
      mPrimaryAttribs.sampleRate = 22050;
      mPrimaryAttribs.bitsPerSample = 16;
      mPrimaryAttribs.nChannels = 2;
   }
   else
      mPrimaryAttribs = *pAttribs;

   LogMsg("OpenAL Init...");
   LogIncIndent();

   sALDeviceInfo info;
   if (!ALOpenDevice(&mpDevice, &mpContext, &info, mPrimaryAttribs))
   {
      LogDecIndent();
      return kSndCantCreateDevice;
   }

   mbEFX = info.bEFX;
   mbEAXReverb = info.bEFX && info.bEAX2 && !config_is_defined("disable_oal_eax_reverb");
   mbXRAM = info.bXRAM && ALInitXRAMEnums();
   mbOcclusion = info.bOcclusion;

   LogIncIndent();
   if (mbEAXReverb)
      LogMsg("EAX reverb support enabled");
   else if (mbEFX)
      LogMsg("EFX reverb support enabled");
   if (mbOcclusion)
      LogMsg("occlusion support enabled");
   LogDecIndent();

   mpAudibleListHead = NULL;
   mpInaudibleListHead = NULL;

   uint32 nAvail = info.n3DSources + info.n2DSources;
   if (numChannels > nAvail)
   {
      LogMsg(_LogFmt("AL mixer requested %d channels, but device only supports %d", numChannels, nAvail));
      numChannels = nAvail;
   }
   mNumMixerChans = numChannels;
   mNumFreeMixerChans = numChannels;

   mpIterNext = NULL;
   mIterGroup = 1;

   for (int i = 0; i < (kSndNumGroups + 1); i++)
   {
      mGroupVolumes[i] = 0;
      mpGroupFades[i] = NULL;
   }
   mGroupFadesActive = FALSE;
   mMasterVolume = 0;
   mTimerState = eSndTimerUnknown;

   mpDisplayDevice = AppGetObj(IDisplayDevice);
   mpWinDisplayDevice = AppGetObj(IWinDisplayDevice);

   BOOL bDefer = m3DDeferFlag;
   Set3DDeferMode(TRUE);
   Set3DPosition(&m3DPosition);
   Set3DOrientation(&m3DFrontVector, &m3DTopVector);
   Set3DVelocity(&m3DVelocity);
   Set3DEnvironment(&m3DEnvironment);
   Set3DDeferMode(bDefer);

   LogDecIndent();
   return kSndOk;
}

class cALSndSample : public cSndSample
{
   DECLARE_UNAGGREGATABLE();

public:
   cALSndSample(cALSndMixer *pMixer, eSndSampleCreateFlagSet flags);
   virtual ~cALSndSample();

   STDMETHOD_(void, SetPan)(int32 pan);
   STDMETHOD_(void, SetFrequency)(uint32 freq);
   STDMETHOD_(uint32, AvailToWrite)(void);
   STDMETHOD_(void, CheckStream)(void);
   STDMETHOD_(BOOL, BufferReady)(uint32 len);
   STDMETHOD_(eSndError, LoadBufferIndirect)(SndLoadFunction funk, void *pData, uint32 len);
   STDMETHOD_(void, SilenceFill)(uint32 nBytes);
   STDMETHOD_(void, Set3DPosition)(sSndVector *pPosition);
   STDMETHOD_(void, Set3DVelocity)(sSndVector *pVelocity);
   STDMETHOD_(void, Set3DConeAngles)(uint32 inside, uint32 outside);
   STDMETHOD_(void, Set3DConeOrientation)(sSndVector *pOrientation);
   STDMETHOD_(void, Set3DDistanceRange)(float minDist, float maxDist);
   STDMETHOD_(void, Set3DMode)(eSnd3DMode mode);
   STDMETHOD_(void, SetAmbientVolume)(int32 vol);
   STDMETHOD_(void, Set3DMethod)(eSnd3DMethod method);
   STDMETHOD_(void, Set3DReverbMix)(float mix);
   STDMETHOD_(void, Set3DOcclusion)(int32 occlusion);

   virtual BOOL IsPlaying(void);
   virtual BOOL MakeAudible();

   ALuint Source() const
   {
      return mSource;
   }

   cALSndMixer *ALMixer() const
   {
      return (cALSndMixer *)mpMixer;
   }

   void Reapply3D();

protected:
   virtual void LLStop();
   virtual void LLRelease();
   virtual void LLInit();
   virtual HRESULT LLStart();
   virtual void LLPause();
   virtual void LLResume();
   virtual void LLUnMute();
   virtual void LLSetPosition(uint32 pos);
   virtual uint32 LLGetPosition();
   virtual void LLSetVolume(int32 vol);
   virtual void SetPanVol3D();

   BOOL CreateFilters();
   void UpdateFilters();
   ALuint GetFreeBuffer();
   void FlushFill();
   void *ApplyPan(void *pData, uint32 *pLen);
   BOOL StreamQueued();

   ALuint mBuffers[2];
   ALuint mSource;
   ALuint mDirectFilter;
   ALuint mSendFilter;
   uint32 mQueuedMask;
   float mReverbMix;
   int32 mOcclusion;
   ALenum mFormat;
   uint32 mStreamBufLen;
   uint32 mStreamRate;
   BOOL mb3D;
   int mPanMode;
   char *mpStreamBuf;
   char *mpPanBuf;
   BOOL mbFillPending;
   uint32 mFillBytes;
   BOOL mbPlayed;

public:
   BOOL mbPausedByMixer;
};

static ISndSample *ALStoreSourceIfPlaying(ISndMixer *, ISndSample *pSample, void *pData)
{
   cALSndSample *pAL = (cALSndSample *)pSample;
   if (pAL->Source() != (ALuint)-1)
   {
      ALint state;
      palGetSourcei(pAL->Source(), AL_SOURCE_STATE, &state);
      if (state == AL_PLAYING)
      {
         ALuint *pList = (ALuint *)pData;
         pList[++pList[0]] = pAL->Source();
         pAL->mbPausedByMixer = TRUE;
      }
   }
   return pSample;
}

static ISndSample *ALStoreSourceIfPaused(ISndMixer *, ISndSample *pSample, void *pData)
{
   cALSndSample *pAL = (cALSndSample *)pSample;
   if (pAL->mbPausedByMixer)
   {
      ALuint *pList = (ALuint *)pData;
      pList[++pList[0]] = pAL->Source();
      pAL->mbPausedByMixer = FALSE;
   }
   return pSample;
}

STDMETHODIMP_(void) cALSndMixer::Pause(void)
{
   ALuint aSources[512];
   aSources[0] = 0;
   DoForAllSamples(ALStoreSourceIfPlaying, aSources);
   for (ALuint i = 1; i <= aSources[0]; i++)
      palSourcePause(aSources[i]);
}

STDMETHODIMP_(void) cALSndMixer::Resume(void)
{
   ALuint aSources[512];
   aSources[0] = 0;
   DoForAllSamples(ALStoreSourceIfPaused, aSources);
   if (aSources[0])
      palSourcePlayv(aSources[0], &aSources[1]);
}

STDMETHODIMP_(void) cALSndMixer::Set3DPosition(sSndVector *pPosition)
{
   m3DPosition = *pPosition;
   if (m3DMixMethod)
   {
      float f = m3DEnvironment.distanceFactor;
      palListener3f(AL_POSITION, -pPosition->y * f, pPosition->z * f, -pPosition->x * f);
      if (mbPanVolInUse && mpAudibleListHead)
         mbPanVolInUse = ((cDSndSample *)mpAudibleListHead)->UpdatePanVol3D();
   }
}

STDMETHODIMP_(void) cALSndMixer::Set3DOrientation(sSndVector *pFront, sSndVector *pTop)
{
   m3DFrontVector = *pFront;
   m3DTopVector = *pTop;
   if (m3DMixMethod)
   {
      float aOrient[6];
      aOrient[0] = -pFront->y;
      aOrient[1] = pFront->z;
      aOrient[2] = -pFront->x;
      aOrient[3] = -pTop->y;
      aOrient[4] = pTop->z;
      aOrient[5] = -pTop->x;
      palListenerfv(AL_ORIENTATION, aOrient);
      if (mbPanVolInUse && mpAudibleListHead)
         mbPanVolInUse = ((cDSndSample *)mpAudibleListHead)->UpdatePanVol3D();
   }
}

STDMETHODIMP_(void) cALSndMixer::Set3DVelocity(sSndVector *pVelocity)
{
   float f = m3DEnvironment.distanceFactor;
   palListener3f(AL_VELOCITY, -pVelocity->y * f, pVelocity->z * f, -pVelocity->x * f);
   m3DVelocity = *pVelocity;
}

static ISndSample *ALReapplySample3D(ISndMixer *, ISndSample *pSample, void *)
{
   ((cALSndSample *)pSample)->Reapply3D();
   return pSample;
}

static ISndSample *ALApplySampleRolloff(ISndMixer *, ISndSample *pSample, void *)
{
   cALSndSample *pAL = (cALSndSample *)pSample;
   if (pAL->Source() != (ALuint)-1)
      palSourcef(pAL->Source(), AL_ROLLOFF_FACTOR, pAL->ALMixer()->RolloffFactor());
   return pSample;
}

STDMETHODIMP_(void) cALSndMixer::Set3DEnvironment(sSndEnvironment *pEnv)
{
   palGetError();
   BOOL bChanged = pEnv->distanceFactor != m3DEnvironment.distanceFactor
                   || pEnv->rolloffFactor != m3DEnvironment.rolloffFactor;
   BOOL bDefer = m3DDeferFlag;
   if (bChanged)
      Set3DDeferMode(TRUE);
   if (pEnv->dopplerFactor != m3DEnvironment.dopplerFactor)
      palDopplerFactor(pEnv->dopplerFactor);
   if (pEnv->distanceFactor != m3DEnvironment.distanceFactor)
   {
      Set3DPosition(&m3DPosition);
      Set3DVelocity(&m3DVelocity);
      DoForAllSamples(ALReapplySample3D, NULL);
   }
   if (pEnv->rolloffFactor != m3DEnvironment.rolloffFactor)
      DoForAllSamples(ALApplySampleRolloff, NULL);
   if (bChanged)
      Set3DDeferMode(bDefer);
   m3DEnvironment = *pEnv;
}

STDMETHODIMP_(void) cALSndMixer::Set3DMethod(eSnd3DMethod method)
{
   m3DMixMethod = method;
}

STDMETHODIMP_(void) cALSndMixer::Get3DMethodCapabilities(uint32 *pMethods)
{
   *pMethods = kSnd3DMethodPanVol | kSnd3DMethodSoftware | kSnd3DMethodHardware;
}

STDMETHODIMP_(void) cALSndMixer::Set3DDeferMode(BOOL deferOn)
{
   if (m3DDeferFlag != (uint32)deferOn)
      m3DDeferFlag = deferOn;
}

STDMETHODIMP_(void) cALSndMixer::FreeHWChannelCount(int32 *pHWChans, int32 *p3DHWChans)
{
   if (pHWChans)
      *pHWChans = mNumFreeMixerChans;
   if (p3DHWChans)
      *p3DHWChans = mNumFreeMixerChans;
}

STDMETHODIMP_(BOOL) cALSndMixer::Have3DReverb(void)
{
   return mReverbEffect != (ALuint)-1;
}

STDMETHODIMP_(BOOL) cALSndMixer::CanDo3DReverb(void)
{
   if (mReverbEffect == (ALuint)-1)
   {
      if (!InitReverb())
         return FALSE;
      ShutdownReverb();
   }
   return TRUE;
}

static float ALReverbLevelToGain(float level)
{
   return ALmBToGain((1.0f - level) * -10000.0f);
}

#define AL_REVERB_PRESET_INDEX(i) ((i) > AL_NUM_REVERB_PRESETS - 1 ? 0 : (i))

STDMETHODIMP_(BOOL) cALSndMixer::Set3DReverbSettings(ReverbSettings *pSettings)
{
   if (mReverbEffect == (ALuint)-1)
      return FALSE;
   DWORD flags = pSettings->flags & 0xf;
   if (!flags)
      return TRUE;

   BOOL bDefer = m3DDeferFlag;
   if ((flags & (flags - 1)) || (pSettings->flags & 1))
      Set3DDeferMode(TRUE);

   if (!mbEAXReverb)
   {
      if (pSettings->flags & 1)
      {
         const sEFXReverbProps *p = &g_EFXReverbPresets[AL_REVERB_PRESET_INDEX(pSettings->type)];
         palEffectf(mReverbEffect, AL_REVERB_DENSITY, p->flDensity);
         palEffectf(mReverbEffect, AL_REVERB_DIFFUSION, p->flDiffusion);
         palEffectf(mReverbEffect, AL_REVERB_GAIN, p->flGain);
         palEffectf(mReverbEffect, AL_REVERB_GAINHF, p->flGainHF);
         palEffectf(mReverbEffect, AL_REVERB_DECAY_TIME, p->flDecayTime);
         palEffectf(mReverbEffect, AL_REVERB_DECAY_HFRATIO, p->flDecayHFRatio);
         palEffectf(mReverbEffect, AL_REVERB_REFLECTIONS_GAIN, p->flReflectionsGain);
         palEffectf(mReverbEffect, AL_REVERB_REFLECTIONS_DELAY, p->flReflectionsDelay);
         palEffectf(mReverbEffect, AL_REVERB_LATE_REVERB_GAIN, p->flLateReverbGain);
         palEffectf(mReverbEffect, AL_REVERB_LATE_REVERB_DELAY, p->flLateReverbDelay);
         palEffectf(mReverbEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, p->flAirAbsorptionGainHF);
         palEffectf(mReverbEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, p->flRoomRolloffFactor);
         palEffecti(mReverbEffect, AL_REVERB_DECAY_HFLIMIT, p->iDecayHFLimit);
      }
      if (pSettings->flags & 2)
         palEffectf(mReverbEffect, AL_REVERB_LATE_REVERB_GAIN, ALReverbLevelToGain(pSettings->level));
      if (pSettings->flags & 4)
         palEffectf(mReverbEffect, AL_REVERB_DECAY_TIME, pSettings->decay);
      if (pSettings->flags & 8)
         palEffectf(mReverbEffect, AL_REVERB_DECAY_HFRATIO, pSettings->damping);
   }
   else
   {
      if (pSettings->flags & 1)
      {
         const sEFXEAXReverbProps *p = &g_EFXEAXReverbPresets[AL_REVERB_PRESET_INDEX(pSettings->type)];
         palEffectf(mReverbEffect, AL_EAXREVERB_DENSITY, p->flDensity);
         palEffectf(mReverbEffect, AL_EAXREVERB_DIFFUSION, p->flDiffusion);
         palEffectf(mReverbEffect, AL_EAXREVERB_GAIN, p->flGain);
         palEffectf(mReverbEffect, AL_EAXREVERB_GAINHF, p->flGainHF);
         palEffectf(mReverbEffect, AL_EAXREVERB_GAINLF, p->flGainLF);
         palEffectf(mReverbEffect, AL_EAXREVERB_DECAY_TIME, p->flDecayTime);
         palEffectf(mReverbEffect, AL_EAXREVERB_DECAY_HFRATIO, p->flDecayHFRatio);
         palEffectf(mReverbEffect, AL_EAXREVERB_DECAY_LFRATIO, p->flDecayLFRatio);
         palEffectf(mReverbEffect, AL_EAXREVERB_REFLECTIONS_GAIN, p->flReflectionsGain);
         palEffectf(mReverbEffect, AL_EAXREVERB_REFLECTIONS_DELAY, p->flReflectionsDelay);
         palEffectfv(mReverbEffect, AL_EAXREVERB_REFLECTIONS_PAN, p->flReflectionsPan);
         palEffectf(mReverbEffect, AL_EAXREVERB_LATE_REVERB_GAIN, p->flLateReverbGain);
         palEffectf(mReverbEffect, AL_EAXREVERB_LATE_REVERB_DELAY, p->flLateReverbDelay);
         palEffectfv(mReverbEffect, AL_EAXREVERB_LATE_REVERB_PAN, p->flLateReverbPan);
         palEffectf(mReverbEffect, AL_EAXREVERB_ECHO_TIME, p->flEchoTime);
         palEffectf(mReverbEffect, AL_EAXREVERB_ECHO_DEPTH, p->flEchoDepth);
         palEffectf(mReverbEffect, AL_EAXREVERB_MODULATION_TIME, p->flModulationTime);
         palEffectf(mReverbEffect, AL_EAXREVERB_MODULATION_DEPTH, p->flModulationDepth);
         palEffectf(mReverbEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, p->flAirAbsorptionGainHF);
         palEffectf(mReverbEffect, AL_EAXREVERB_HFREFERENCE, p->flHFReference);
         palEffectf(mReverbEffect, AL_EAXREVERB_LFREFERENCE, p->flLFReference);
         palEffectf(mReverbEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, p->flRoomRolloffFactor);
         palEffecti(mReverbEffect, AL_EAXREVERB_DECAY_HFLIMIT, p->iDecayHFLimit);
      }
      if (pSettings->flags & 2)
         palEffectf(mReverbEffect, AL_EAXREVERB_LATE_REVERB_GAIN, ALReverbLevelToGain(pSettings->level));
      if (pSettings->flags & 4)
         palEffectf(mReverbEffect, AL_EAXREVERB_DECAY_TIME, pSettings->decay);
      if (pSettings->flags & 8)
         palEffectf(mReverbEffect, AL_EAXREVERB_DECAY_HFRATIO, pSettings->damping);
   }

   if (pSettings->flags)
      palAuxiliaryEffectSloti(mReverbSlot, AL_EFFECTSLOT_EFFECT, mReverbEffect);
   Set3DDeferMode(bDefer);
   return TRUE;
}

STDMETHODIMP_(BOOL) cALSndMixer::Get3DReverbSettings(ReverbSettings *pSettings)
{
   CriticalMsg1("Call to unimplemented function %s!", "cALSndMixer::Get3DReverbSettings");
   return FALSE;
}

STDMETHODIMP_(BOOL) cALSndMixer::Have3DOcclusion(void)
{
   return mbOcclusion;
}

STDMETHODIMP_(int32) cALSndMixer::Kludge(int kludgeSelector, void *pKludgeStruct, int32 sizeKludgeStruct)
{
   if (kludgeSelector && kludgeSelector == 1)
      return (int32)mpDevice;
   return 0;
}

BOOL cALSndMixer::InitReverb(void)
{
   if (!mbEFX)
      return FALSE;
   ShutdownReverb();

   palGetError();
   palGenAuxiliaryEffectSlots(1, &mReverbSlot);
   if (palGetError() != AL_NO_ERROR)
   {
      ShutdownReverb();
      return FALSE;
   }
   palGenEffects(1, &mReverbEffect);
   if (palGetError() != AL_NO_ERROR || !palIsEffect(mReverbEffect))
   {
      ShutdownReverb();
      return FALSE;
   }
   palGetError();
   if (mbEAXReverb)
   {
      palEffecti(mReverbEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
      if (palGetError() != AL_NO_ERROR)
         mbEAXReverb = FALSE;
   }
   if (!mbEAXReverb)
   {
      palEffecti(mReverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
      if (palGetError() != AL_NO_ERROR)
      {
         ShutdownReverb();
         return FALSE;
      }
   }
   if (palIsAuxiliaryEffectSlot(mReverbSlot))
   {
      palGetError();
      palAuxiliaryEffectSloti(mReverbSlot, AL_EFFECTSLOT_EFFECT, mReverbEffect);
      if (palGetError() != AL_NO_ERROR)
      {
         ShutdownReverb();
         return FALSE;
      }
   }
   return TRUE;
}

void cALSndMixer::ShutdownReverb(void)
{
   if (mReverbSlot != (ALuint)-1)
      palAuxiliaryEffectSloti(mReverbSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECTSLOT_NULL);
   if (mReverbEffect != (ALuint)-1)
   {
      palDeleteEffects(1, &mReverbEffect);
      mReverbEffect = (ALuint)-1;
   }
   if (mReverbSlot != (ALuint)-1)
   {
      palDeleteAuxiliaryEffectSlots(1, &mReverbSlot);
      mReverbSlot = (ALuint)-1;
   }
}

STDMETHODIMP_(ISndSample *) cALSndMixer::CreateRawSample(eSndSampleCreateFlagSet flags, void *pData,
                                                         uint32 dataLen, uint32 nSamples, sSndAttribs *pAttribs)
{
   cALSndSample *smp = new cALSndSample(this, flags);

   if (smp == NULL)
      return NULL;

   if (smp->Init(pAttribs, pData, dataLen, nSamples) == FALSE)
   {
      SafeRelease(smp);
   }
   else
   {
      switch (flags)
      {
         case kSndSampleStream:
            if (mpThread != NULL)
               mpThread->CallWorker(kThreadNewStream);
            break;
         case kSndSampleInternal:
            break;
         default:
            if (pData != NULL)
            {
               if (smp->MakeAudible())
                  smp->LoadBuffer((uint8 *)pData, dataLen);
               else
                  SafeRelease(smp);
            }
            break;
      }
      if (smp != NULL)
         smp->SetGroupVolume(mMasterVolume + mGroupVolumes[0]);
   }

   return smp;
}

BOOL SndCreateALMixer(ISndMixer **ppMixer, IUnknown *pOuter)
{
   *ppMixer = NULL;

   if (pOuter != NULL)
      return FALSE;

   *ppMixer = new cALSndMixer;

   return (*ppMixer != NULL);
}

#define MIXER_MUTEX \
   cMixerAutoLock __mixer_lock__( mpMixer->MutexRef() )

struct sALOcclusion
{
   int32 level;
   float lfRatio;
   float hfRatio;
   float roomRatio;
};

static void ALOcclusionGains(const sALOcclusion *pOcc, float *pGains)
{
   float ratio = pOcc->roomRatio + pOcc->lfRatio - 1.0f;
   float prod = pOcc->roomRatio * pOcc->lfRatio;
   if (prod > ratio)
      ratio = prod;
   pGains[0] = (ratio * pOcc->level == -10000.0f) ? 0.0f : (float)pow(10.0, ratio * pOcc->level / 2000.0);
   if (pOcc->level * pOcc->roomRatio != -10000.0f)
      pGains[1] = (float)pow(10.0, pOcc->level * pOcc->roomRatio / 2000.0);
   else
      pGains[1] = 0.0f;
}

static void ALOcclusionSendGains(const sALOcclusion *pOcc, float *pGains)
{
   float ratio = pOcc->hfRatio + pOcc->lfRatio - 1.0f;
   float prod = pOcc->hfRatio * pOcc->lfRatio;
   if (prod > ratio)
      ratio = prod;
   pGains[0] = (ratio * pOcc->level == -10000.0f) ? 0.0f : (float)pow(10.0, ratio * pOcc->level / 2000.0);
   if (pOcc->level * pOcc->hfRatio != -10000.0f)
      pGains[1] = (float)pow(10.0, pOcc->level * pOcc->hfRatio / 2000.0);
   else
      pGains[1] = 0.0f;
}

static BOOL ALCreateLowPassFilter(ALuint *pFilter)
{
   palGetError();
   palGenFilters(1, pFilter);
   if (palGetError() == AL_NO_ERROR && palIsFilter(*pFilter))
   {
      palFilteri(*pFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
      if (palGetError() == AL_NO_ERROR)
         return TRUE;
   }
   palDeleteFilters(1, pFilter);
   *pFilter = (ALuint)-1;
   return FALSE;
}

static float ALmBToGainClamped(int32 mB)
{
   if (mB < kSndMinVolume)
      mB = kSndMinVolume;
   else if (mB > kSndMaxVolume)
      mB = kSndMaxVolume;
   return ALmBToGain(mB);
}

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cALSndSample, ISndSample);

cALSndSample::cALSndSample(cALSndMixer *pMixer, eSndSampleCreateFlagSet flags)
   : cSndSample(pMixer, flags)
{
   mReverbMix = 0.5f;
   mBuffers[0] = (ALuint)-1;
   mBuffers[1] = (ALuint)-1;
   mSource = (ALuint)-1;
   mDirectFilter = (ALuint)-1;
   mSendFilter = (ALuint)-1;
   mQueuedMask = 0;
   mOcclusion = 0;
   m3DMethod = kSnd3DMethodNone;
   mPanMode = 0;
   mpStreamBuf = NULL;
   mpPanBuf = NULL;
   mbFillPending = FALSE;
   mFillBytes = 0;
   mbPlayed = FALSE;
   mbPausedByMixer = FALSE;
   mFormat = 0;
}

cALSndSample::~cALSndSample()
{
   MIXER_MUTEX;

   if (mDirectFilter != (ALuint)-1)
   {
      palSourcei(mSource, AL_DIRECT_FILTER, AL_FILTER_NULL);
      palDeleteFilters(1, &mDirectFilter);
   }
   if (mSendFilter != (ALuint)-1)
   {
      palSource3i(mSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
      palDeleteFilters(1, &mSendFilter);
   }
   if (mSource != (ALuint)-1)
   {
      palDeleteSources(1, &mSource);
      mpMixer->FreeChannel(this);
   }
   if (mBuffers[0] != (ALuint)-1)
   {
      mpMixer->DoTrace((void *)mBufferLen, kSndBufferFree);
      palDeleteBuffers(mBuffers[1] != (ALuint)-1 ? 2 : 1, mBuffers);
   }
   if (mpStreamBuf)
      delete mpStreamBuf;
   if (mpPanBuf)
      delete mpPanBuf;
}

BOOL cALSndSample::CreateFilters()
{
   cALSndMixer *pMixer = ALMixer();
   if (!ALCreateLowPassFilter(&mDirectFilter))
      return FALSE;
   if (pMixer->Have3DReverb() && !ALCreateLowPassFilter(&mSendFilter))
   {
      palDeleteFilters(1, &mDirectFilter);
      mDirectFilter = (ALuint)-1;
      return FALSE;
   }
   palGetError();
   palSourcei(mSource, AL_DIRECT_FILTER, mDirectFilter);
   if (palGetError() == AL_NO_ERROR)
   {
      if (mSendFilter == (ALuint)-1)
         palSource3i(mSource, AL_AUXILIARY_SEND_FILTER, pMixer->ReverbSlot(), 0, AL_FILTER_NULL);
      else
         palSource3i(mSource, AL_AUXILIARY_SEND_FILTER, pMixer->ReverbSlot(), 0, mSendFilter);
      if (palGetError() == AL_NO_ERROR)
         return TRUE;
   }
   palSourcei(mSource, AL_DIRECT_FILTER, AL_FILTER_NULL);
   palSource3i(mSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
   if (mSendFilter == (ALuint)-1)
   {
      palDeleteFilters(1, &mDirectFilter);
      mDirectFilter = (ALuint)-1;
      return FALSE;
   }
   palDeleteFilters(2, &mDirectFilter);
   mDirectFilter = (ALuint)-1;
   mSendFilter = (ALuint)-1;
   return FALSE;
}

void cALSndSample::UpdateFilters()
{
   if (mSource == (ALuint)-1 || mDirectFilter == (ALuint)-1)
      return;

   sALOcclusion occ;
   occ.level = mOcclusion;
   occ.lfRatio = 0.5f;
   occ.hfRatio = 0.5f;
   occ.roomRatio = 1.0f;

   float directGain;
   float sendGain;
   if (mReverbMix > 0.5f)
   {
      directGain = 1.0f - (mReverbMix - 0.5f) * 2.0f;
      sendGain = 1.0f;
   }
   else
   {
      directGain = 1.0f;
      sendGain = (mReverbMix < 0.5f) ? mReverbMix * 2.0f : 1.0f;
   }

   float gains[2];
   ALOcclusionGains(&occ, gains);
   palFilterf(mDirectFilter, AL_LOWPASS_GAIN, gains[0] * directGain);
   palFilterf(mDirectFilter, AL_LOWPASS_GAINHF, gains[1]);
   palSourcei(mSource, AL_DIRECT_FILTER, mDirectFilter);

   if (mSendFilter != (ALuint)-1)
   {
      ALOcclusionSendGains(&occ, gains);
      palFilterf(mSendFilter, AL_LOWPASS_GAIN, gains[0] * sendGain);
      palFilterf(mSendFilter, AL_LOWPASS_GAINHF, gains[1]);
      palSource3i(mSource, AL_AUXILIARY_SEND_FILTER, ALMixer()->ReverbSlot(), 0, mSendFilter);
   }
}

BOOL cALSndSample::MakeAudible()
{
   MIXER_MUTEX;

   if (mStateFlags & kSndFlagAudible)
      return TRUE;

   if (mpMixer->AllocChannel(this, mFlags, m3DMethod) == FALSE)
      return FALSE;
   mbPausedByMixer = FALSE;

   palGetError();
   palGenBuffers((mFlags & kSndSampleStream) ? 2 : 1, mBuffers);
   if (palGetError() != AL_NO_ERROR)
   {
      mpMixer->FreeChannel(this);
      return FALSE;
   }

   int nBits = mAttribs.bitsPerSample;
   int nChannels = mAttribs.nChannels;
   if (nChannels == 1)
      mFormat = (nBits == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
   else
      mFormat = (nBits == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;

   mStreamBufLen = mBufferLen;
   mStreamRate = mAttribs.sampleRate;
   if (mFlags & kSndSampleStream)
   {
      uint32 len = (((mAttribs.sampleRate * g_nALStreamBufferLen) / 1000) * (nBits / 8) * nChannels + 15) & ~15;
      if ((int)(len * 2) <= (int)mBufferLen)
         mStreamBufLen = len;
   }

   mb3D = (m3DMethod == kSnd3DMethodSoftware || m3DMethod == kSnd3DMethodHardware);
   mPanMode = (nChannels != 1) + 1;
   mFormat = (nBits == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;

   mpMixer->DoTrace((void *)mStreamBufLen, kSndBufferAllocate);

   palGetError();
   palGenSources(1, &mSource);
   if (palGetError() != AL_NO_ERROR)
   {
      LLRelease();
      return FALSE;
   }

   MoveToList(mpMixer->AudibleHead());
   mStateFlags |= kSndFlagAudible;

   if (mpStreamBuf == NULL)
      mpStreamBuf = new char[mStreamBufLen];

   mGroupVolume = mpMixer->GetGroupVolume(mGroup) + mpMixer->GetMasterVolume();

   switch (m3DMethod)
   {
      case kSnd3DMethodNone:
         Set3DMode(kSnd3DModeNone);
         SetPan(mPan);
         SetFrequency(mFrequency);
         break;
      case kSnd3DMethodPanVol:
         SetPanVol3D();
         ((cALSndMixer *)mpMixer)->mbPanVolInUse = TRUE;
         break;
      case kSnd3DMethodSoftware:
      case kSnd3DMethodHardware:
         Set3DPosition(&m3DPosition);
         Set3DVelocity(&m3DVelocity);
         Set3DConeAngles(m3DConeInnerAngle, m3DConeOuterAngle);
         Set3DConeOrientation(&m3DConeOrientation);
         Set3DDistanceRange(m3DMinDistance, m3DMaxDistance);
         Set3DMode(m3DMode);
         SetAmbientVolume(mAmbientVolume);
         ALApplySampleRolloff(mpMixer, this, NULL);
         if (mpMixer->Have3DReverb())
         {
            CreateFilters();
            UpdateFilters();
         }
         break;
   }

   SetVolume(mVolume);
   mLastWrite = 0;
   return TRUE;
}

void cALSndSample::LLStop()
{
   palSourceStop(mSource);
   mbPlayed = FALSE;
}

void cALSndSample::LLRelease()
{
   palDeleteSources(1, &mSource);
   mSource = (ALuint)-1;
   palDeleteBuffers(mBuffers[1] != (ALuint)-1 ? 2 : 1, mBuffers);
   mBuffers[0] = (ALuint)-1;
   mBuffers[1] = (ALuint)-1;
}

void cALSndSample::LLInit()
{
}

HRESULT cALSndSample::LLStart()
{
   palSourcei(mSource, AL_LOOPING, ((mStateFlags & kSndFlagLooped) && !(mStateFlags & kSndFlagStream)) ? AL_TRUE : AL_FALSE);

   if (fnFillCB)
   {
      uint32 avail = AvailToWrite();
      if (avail)
      {
         mbFillPending = TRUE;
         mFillBytes = 0;
         fnFillCB(this, mpFillCBData, avail);
         FlushFill();
      }
   }

   mStateFlags &= ~kSndFlagPleaseStop;
   if (mInitBuffPos > 0)
   {
      SetPosition(mInitBuffPos);
      mInitBuffPos = -1;
   }

   palGetError();
   palSourcePlay(mSource);
   if (palGetError() != AL_NO_ERROR)
      return E_FAIL;
   mbPlayed = TRUE;
   return S_OK;
}

void cALSndSample::LLPause()
{
   palSourcePause(mSource);
}

void cALSndSample::LLResume()
{
   palSourcePlay(mSource);
}

void cALSndSample::LLUnMute()
{
   if (!IsPlaying())
      return;
   mbFillPending = TRUE;
   if (mStateFlags & kSndFlagStream)
   {
      mFillBytes = 0;
      fnFillCB(this, mpFillCBData, AvailToWrite());
      FlushFill();
      return;
   }
   uint32 pos = mBasePos;
   mBasePos = 0;
   mFillBytes = 0;
   fnFillCB(this, mpFillCBData, mStreamBufLen);
   FlushFill();
   SetPosition(pos);
}

BOOL cALSndSample::IsPlaying(void)
{
   if (mSource == (ALuint)-1)
      return FALSE;
   if ((mStateFlags & kSndFlagStream) && mbPausedByMixer)
      return TRUE;
   ALint state;
   palGetSourcei(mSource, AL_SOURCE_STATE, &state);
   return state == AL_PLAYING;
}

BOOL cALSndSample::StreamQueued()
{
   if (mSource == (ALuint)-1)
      return FALSE;
   ALint state;
   palGetSourcei(mSource, AL_SOURCE_STATE, &state);
   return state == AL_PLAYING;
}

void cALSndSample::LLSetVolume(int32 vol)
{
   if (mSource == (ALuint)-1)
      return;
   if (m3DMethod == kSnd3DMethodPanVol)
   {
      SetPanVol3D();
      return;
   }
   palSourcef(mSource, AL_GAIN, ALmBToGainClamped(vol));
}

void cALSndSample::SetPanVol3D()
{
   if (mSource == (ALuint)-1)
      return;
   int32 vol;
   int32 pan;
   mpMixer->Get3DPositionPanVolume(&m3DPosition, &pan, &vol);
   palSourcef(mSource, AL_GAIN, ALmBToGainClamped(vol + mGroupVolume + mVolume));
}

STDMETHODIMP_(void) cALSndSample::SetPan(int32 pan)
{
   mPan = pan;
}

STDMETHODIMP_(void) cALSndSample::SetFrequency(uint32 freq)
{
   if (freq < 100)
      freq = 100;
   else if (freq > 100000)
      freq = 100000;

   if (!(mStateFlags & kSndFlagAudible))
   {
      if (!(mStateFlags & kSndFlagPaused) && mState == kSndStatePlaying)
      {
         DWORD now = timeGetTime();
         mBasePos = (uint32)(mBasePos + ((float)(now - mBaseTime) * mFrequency) / 1000.0f);
         mFrequency = freq;
         mBaseTime = now;
         return;
      }
   }
   else
   {
      palBufferi(mBuffers[0], AL_FREQUENCY, freq);
      if (mBuffers[1] != (ALuint)-1)
         palBufferi(mBuffers[1], AL_FREQUENCY, freq);
   }
   mFrequency = freq;
}

void cALSndSample::LLSetPosition(uint32 pos)
{
   palSourcei(mSource, AL_BYTE_OFFSET, pos);
}

uint32 cALSndSample::LLGetPosition()
{
   ALint offset;
   palGetSourcei(mSource, AL_SAMPLE_OFFSET, &offset);
   return (offset + mBaseOffset) / mBytesPerSample;
}

void *cALSndSample::ApplyPan(void *pData, uint32 *pLen)
{
   int32 pan = mPan;
   if (pan == -1)
      pan = 0;
   else if (pan < kSndPanLeft)
      pan = kSndPanLeft;
   else if (pan > kSndPanRight)
      pan = kSndPanRight;

   if (mPanMode == 2 && pan == 0)
      return pData;

   if (mpPanBuf == NULL)
      mpPanBuf = new char[(mPanMode == 1 ? 2 : 1) * mStreamBufLen];

   float gain = (pan < 1) ? ALmBToGain(pan) : ALmBToGain(-pan);
   uint32 nSamples = *pLen / mBytesPerSample;

   if (mPanMode == 1)
   {
      *pLen *= 2;
      if (mAttribs.bitsPerSample == 8)
      {
         uchar *pSrc = (uchar *)pData;
         uchar *pDst = (uchar *)mpPanBuf;
         if (pan == 0)
         {
            for (; nSamples; nSamples--, pSrc++, pDst += 2)
               pDst[0] = pDst[1] = *pSrc;
         }
         else if (pan < 1)
         {
            int scale = (int)(gain * 256.0f);
            for (; nSamples; nSamples--, pSrc++, pDst += 2)
            {
               pDst[0] = *pSrc;
               pDst[1] = (uchar)((((int)*pSrc - 0x80) * scale >> 8) + 0x80);
            }
         }
         else
         {
            int scale = (int)(gain * 256.0f);
            for (; nSamples; nSamples--, pSrc++, pDst += 2)
            {
               pDst[0] = (uchar)((((int)*pSrc - 0x80) * scale >> 8) + 0x80);
               pDst[1] = *pSrc;
            }
         }
      }
      else
      {
         short *pSrc = (short *)pData;
         short *pDst = (short *)mpPanBuf;
         if (pan == 0)
         {
            for (; nSamples; nSamples--, pSrc++, pDst += 2)
               pDst[0] = pDst[1] = *pSrc;
         }
         else if (pan < 1)
         {
            int scale = (int)(gain * 65536.0f);
            for (; nSamples; nSamples--, pSrc++, pDst += 2)
            {
               pDst[0] = *pSrc;
               pDst[1] = (short)((unsigned)(*pSrc * scale) >> 16);
            }
         }
         else
         {
            int scale = (int)(gain * 65536.0f);
            for (; nSamples; nSamples--, pSrc++, pDst += 2)
            {
               pDst[0] = (short)((unsigned)(*pSrc * scale) >> 16);
               pDst[1] = *pSrc;
            }
         }
      }
   }
   else if (mAttribs.bitsPerSample == 8)
   {
      uchar *pSrc = (uchar *)pData;
      uchar *pDst = (uchar *)mpPanBuf;
      int scale = (int)(gain * 256.0f);
      if (pan < 1)
      {
         for (; nSamples; nSamples--, pSrc += 2, pDst += 2)
         {
            pDst[0] = pSrc[0];
            pDst[1] = (uchar)((((int)pSrc[1] - 0x80) * scale >> 8) + 0x80);
         }
      }
      else
      {
         for (; nSamples; nSamples--, pSrc += 2, pDst += 2)
         {
            pDst[0] = (uchar)((((int)pSrc[0] - 0x80) * scale >> 8) + 0x80);
            pDst[1] = pSrc[1];
         }
      }
   }
   else
   {
      short *pSrc = (short *)pData;
      short *pDst = (short *)mpPanBuf;
      int scale = (int)(gain * 32768.0f);
      if (pan < 1)
      {
         for (; nSamples; nSamples--, pSrc += 2, pDst += 2)
         {
            pDst[0] = pSrc[0];
            pDst[1] = (short)(pSrc[1] * scale >> 15);
         }
      }
      else
      {
         for (; nSamples; nSamples--, pSrc += 2, pDst += 2)
         {
            pDst[0] = (short)(pSrc[0] * scale >> 15);
            pDst[1] = pSrc[1];
         }
      }
   }
   return mpPanBuf;
}

ALuint cALSndSample::GetFreeBuffer()
{
   if (mBuffers[1] == (ALuint)-1)
   {
      if (!(mQueuedMask & 1))
         return mBuffers[0];
   }
   else if (mQueuedMask != 3)
   {
      if (mQueuedMask & 1)
         return mBuffers[1];
      return mBuffers[0];
   }

   ALint nProcessed = 0;
   palGetSourcei(mSource, AL_BUFFERS_PROCESSED, &nProcessed);
   if (nProcessed > 0)
   {
      ALuint buffer;
      palGetError();
      palSourceUnqueueBuffers(mSource, 1, &buffer);
      if (palGetError() == AL_NO_ERROR)
      {
         mQueuedMask &= ~(1 << (mBuffers[0] != buffer));
         return buffer;
      }
   }
   return (ALuint)-1;
}

void cALSndSample::FlushFill()
{
   if (!mbFillPending)
      return;
   uint32 len = mFillBytes;
   mbFillPending = FALSE;
   mFillBytes = 0;
   if (!len)
      return;
   ALuint buffer = GetFreeBuffer();
   if (buffer == (ALuint)-1)
      return;
   void *pData = mpStreamBuf;
   if (mPanMode)
      pData = ApplyPan(pData, &len);
   palGetError();
   palBufferData(buffer, mFormat, pData, len, mFrequency);
   if (palGetError() != AL_NO_ERROR)
   {
      return;
   }
   palSourceQueueBuffers(mSource, 1, &buffer);
   if (palGetError() == AL_NO_ERROR)
      mQueuedMask |= 1 << (mBuffers[0] != buffer);
}

STDMETHODIMP_(eSndError) cALSndSample::LoadBufferIndirect(SndLoadFunction funk, void *pData, uint32 len)
{
   uint32 nBytes = len;
   if (!(mStateFlags & kSndFlagStream))
   {
      if (len == 0)
      {
         return kSndUnknownError;
      }
   }
   else if (len == 0)
   {
      if (mbFillPending)
         FlushFill();
      mbPlayed = FALSE;
      if (mState == kSndStatePlaying || mState == kSndStateStopped)
      {
         mFinalWrite = mLastWrite;
         mStateFlags |= kSndFlagEndOfData;
      }
      return kSndOk;
   }

   void *pSrc;
   if (!mbFillPending)
   {
      ALuint buffer = GetFreeBuffer();
      if (buffer == (ALuint)-1)
         return kSndUnknownError;
      pSrc = funk(pData, mpStreamBuf, nBytes);
      uint32 bufLen = nBytes;
      if (mPanMode)
         pSrc = ApplyPan(pSrc, &bufLen);
      palGetError();
      palBufferData(buffer, mFormat, pSrc, bufLen, mFrequency);
      if (palGetError() != AL_NO_ERROR)
      {
         return kSndUnknownError;
      }
      palSourceQueueBuffers(mSource, 1, &buffer);
      if (palGetError() != AL_NO_ERROR)
         return kSndUnknownError;
      mQueuedMask |= 1 << (mBuffers[0] != buffer);
   }
   else
   {
      char *pDst = mpStreamBuf + mFillBytes;
      pSrc = funk(pData, pDst, nBytes);
      if (pDst != pSrc)
         memcpy(pDst, pSrc, nBytes);
      mFillBytes += nBytes;
   }

   if (mDumpFile)
      fwrite(pSrc, 1, nBytes, mDumpFile);
   mLastWrite += nBytes;
   return kSndOk;
}

STDMETHODIMP_(BOOL) cALSndSample::BufferReady(uint32 len)
{
   if (!len)
      return TRUE;
   if (mBuffers[1] == (ALuint)-1)
   {
      if (!(mQueuedMask & 1))
         return TRUE;
   }
   else if (mQueuedMask != 3)
      return TRUE;
   ALint nProcessed = 0;
   palGetSourcei(mSource, AL_BUFFERS_PROCESSED, &nProcessed);
   return nProcessed > 0;
}

STDMETHODIMP_(uint32) cALSndSample::AvailToWrite(void)
{
   if (!(mStateFlags & kSndFlagAudible))
      return 0;
   if (mBuffers[1] == (ALuint)-1)
   {
      if (!(mQueuedMask & 1))
         return mStreamBufLen;
   }
   else if (mQueuedMask != 3)
      return mStreamBufLen;
   ALint nProcessed = 0;
   palGetSourcei(mSource, AL_BUFFERS_PROCESSED, &nProcessed);
   return nProcessed > 0 ? mStreamBufLen : 0;
}

STDMETHODIMP_(void) cALSndSample::CheckStream(void)
{
   uint32 flags = mStateFlags;
   if (!(flags & kSndFlagMuted))
   {
      uint32 avail = AvailToWrite();
      if ((uint32)(mState - 1) < 2 && !(mStateFlags & kSndFlagEndOfData) && fnFillCB && avail)
      {
         mbFillPending = TRUE;
         mFillBytes = 0;
         fnFillCB(this, mpFillCBData, avail);
         FlushFill();
      }
      if (!mbPausedByMixer && mbPlayed && !StreamQueued())
         palSourcePlay(mSource);
   }
   else if (!(flags & kSndFlagPaused) && flags == kSndFlagMuted)
   {
      timeGetTime();
      if (mNumSamples <= GetPosition())
         DeferredStop();
   }
}

STDMETHODIMP_(void) cALSndSample::SilenceFill(uint32 nBytes)
{
   if (!nBytes)
      return;
   ALuint buffer = GetFreeBuffer();
   if (buffer == (ALuint)-1)
      return;
   memset(mpStreamBuf, (mAttribs.bitsPerSample == 8) ? 0x80 : 0, nBytes);
   palGetError();
   palBufferData(buffer, mFormat, mpStreamBuf, nBytes, mFrequency);
   if (palGetError() != AL_NO_ERROR)
   {
      return;
   }
   palSourceQueueBuffers(mSource, 1, &buffer);
   if (palGetError() != AL_NO_ERROR)
      return;
   mQueuedMask |= 1 << (mBuffers[0] != buffer);
   if (mDumpFile)
      fwrite(mpStreamBuf, 1, nBytes, mDumpFile);
   mLastWrite += nBytes;
}

void cALSndSample::Reapply3D()
{
   Set3DPosition(&m3DPosition);
   Set3DVelocity(&m3DVelocity);
   Set3DDistanceRange(m3DMinDistance, m3DMaxDistance);
}

STDMETHODIMP_(void) cALSndSample::Set3DPosition(sSndVector *pPosition)
{
   m3DPosition = *pPosition;
   switch (m3DMethod)
   {
      case kSnd3DMethodNone:
         break;
      case kSnd3DMethodPanVol:
         SetPanVol3D();
         break;
      default:
         if (mSource != (ALuint)-1)
         {
            float f = ALMixer()->DistanceFactor();
            palSource3f(mSource, AL_POSITION, -pPosition->y * f, pPosition->z * f, -pPosition->x * f);
         }
         break;
   }
}

STDMETHODIMP_(void) cALSndSample::Set3DVelocity(sSndVector *pVelocity)
{
   if (mSource != (ALuint)-1)
   {
      float f = ALMixer()->DistanceFactor();
      palSource3f(mSource, AL_VELOCITY, -pVelocity->y * f, pVelocity->z * f, -pVelocity->x * f);
   }
   m3DVelocity = *pVelocity;
}

STDMETHODIMP_(void) cALSndSample::Set3DConeAngles(uint32 inside, uint32 outside)
{
   if (mSource != (ALuint)-1)
   {
      palSourcei(mSource, AL_CONE_INNER_ANGLE, inside);
      palSourcei(mSource, AL_CONE_OUTER_ANGLE, outside);
   }
   m3DConeInnerAngle = inside;
   m3DConeOuterAngle = outside;
}

STDMETHODIMP_(void) cALSndSample::Set3DConeOrientation(sSndVector *pOrientation)
{
   if (mSource != (ALuint)-1)
   {
      float f = ALMixer()->DistanceFactor();
      palSource3f(mSource, AL_DIRECTION, -pOrientation->y * f, pOrientation->z * f, -pOrientation->x * f);
   }
   m3DConeOrientation = *pOrientation;
}

STDMETHODIMP_(void) cALSndSample::Set3DDistanceRange(float minDist, float maxDist)
{
   if (mSource != (ALuint)-1)
   {
      float f = ALMixer()->DistanceFactor();
      palSourcef(mSource, AL_REFERENCE_DISTANCE, minDist * f);
      palSourcef(mSource, AL_MAX_DISTANCE, maxDist * f);
   }
   m3DMinDistance = minDist;
   m3DMaxDistance = maxDist;
}

STDMETHODIMP_(void) cALSndSample::Set3DMode(eSnd3DMode mode)
{
   if (mSource != (ALuint)-1)
      palSourcei(mSource, AL_SOURCE_RELATIVE, mode != kSnd3DModeNormal);
   m3DMode = mode;
}

STDMETHODIMP_(void) cALSndSample::SetAmbientVolume(int32 vol)
{
   if (mSource != (ALuint)-1)
      palSourcef(mSource, AL_CONE_OUTER_GAIN, ALmBToGain(vol));
   mAmbientVolume = vol;
}

STDMETHODIMP_(void) cALSndSample::Set3DMethod(eSnd3DMethod method)
{
   m3DMethod = method;
   if (method == kSnd3DMethodPanVol)
      ((cALSndMixer *)mpMixer)->mbPanVolInUse = TRUE;
}

STDMETHODIMP_(void) cALSndSample::Set3DReverbMix(float mix)
{
   if (!mpMixer->Have3DReverb())
      return;
   if (mix == mReverbMix)
      return;
   if (mix < 0.0f)
      mix = 0.0f;
   else if (mix > 1.0f)
      mix = 1.0f;
   mReverbMix = mix;
   UpdateFilters();
}

STDMETHODIMP_(void) cALSndSample::Set3DOcclusion(int32 occlusion)
{
   mOcclusion = occlusion;
   UpdateFilters();
}

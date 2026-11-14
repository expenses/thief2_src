#line 72
#include "net.h"
#include <dplobby.h>
#include <appagg.h>
#include <aggmemb.h>
#include <msgdelay.h>
#include <prikind.h>
#include <lg.h>
#include <mprintf.h>
#include <config.h>
#include <memall.h>
#include <string.h>
#include <memory.h>
#include <initguid.h>
#include <netiid.h>

#ifdef SHIP
#define NetSpew(x)
#else
#define NetSpew(x) mprintf x
#endif

HRESULT CreateServiceProviderAddress(IDirectPlayLobby3 *, void **, DWORD *, char *);

struct sHostData
{
   double scale;
   double delta;
   double time_of_delta;
   double delta2;
   double time_of_delta2;
   double first_delta;
   double time_of_first_delta;
};

struct sLocalData
{
   sHostData *hostData;
   void *userData;
};

struct sAddrBuf
{
   char *buffer;
   int size;
};

struct SessCon
{
   unsigned char none;
   char *name;
};

struct PlayCon
{
   unsigned int id;
   char *name;
};

class cNet : public cCTDelegating<INet>, public cCTAggregateMemberControl<0>
{
public:
   cNet(IUnknown *pOuter)
      : cCTDelegating<INet>(),
        cCTAggregateMemberControl<0>()
   {
      MI_INIT_AGGREGATION_1(pOuter, INet, kPriorityNormal, NULL);
   }

#ifdef NET_ALLOW_SIMULATION
   cMessageDelayQueue m_MessageDelayQueue;
   int m_SimulateInternet;
#endif

private:
   static IDirectPlay4 *gDP;
   static DPSESSIONDESC2 Session;
   static char session_buffer[80];

public:
   int GetLocalPlayerData(unsigned int player, sLocalData *ldata)
   {
      unsigned int lsize;
      HRESULT hr;

      lsize = 8;
      hr = gDP->GetPlayerData(player, ldata, (LPDWORD)&lsize, 1);
      if (hr >= 0)
      {
         if (lsize == 0)
         {
            ResetPlayerData(player, 0);
            lsize = 8;
            hr = gDP->GetPlayerData(player, ldata, (LPDWORD)&lsize, 1);
         }
#line 191
         AssertMsg(lsize == 8, "Corrupt local player data");
      }
      return hr;
   }
   sHostData *GetHostData(unsigned int player)
   {
      sLocalData ldata;
      if (GetLocalPlayerData(player, &ldata) >= 0)
         return ldata.hostData;
      return 0;
   }

   STDMETHODIMP Init()
   {
      HRESULT hr;

      hr = CoInitialize(0);
      if (hr < 0)
      {
         MessageBoxA(NULL, "Failed to initialize COM", NULL, 0);
         return hr;
      }

      hr = CoCreateInstance(CLSID_DirectPlay, 0, 1, IID_IDirectPlay4A, (LPVOID *)&gDP);
      if (hr < 0)
      {
         MessageBoxA(NULL, "Direct play 4a failed to load", NULL, 0);
         return hr;
      }

#ifdef NET_ALLOW_SIMULATION
      m_SimulateInternet = 0;
      m_MessageDelayQueue.Init();
#endif
      return S_OK;
   }

   STDMETHODIMP End()
   {
      int numLeft;
      numLeft = gDP->Release();
      if (numLeft > 0)
         NetSpew(("There are %d refs to DirectPlay left over!\n", numLeft));
      CoUninitialize();
      return S_OK;
   }

   STDMETHODIMP JoinOrCreate(char *media, char *sessionName, BOOL *created, void *pAddress)
   {
      HRESULT hr;
      SessCon sc;


      hr = EnumConnections(NULL, ConnCallback, media, 1);

      memset(&Session, 0, sizeof(DPSESSIONDESC2));
      Session.dwSize = sizeof(DPSESSIONDESC2);
      Session.guidApplication = GUID_NULL;

      sc.none = 0;
      sc.name = sessionName;
      hr = EnumSessions(&Session, 0, SessionCallback, &sc, 0);

      if (sc.none != 0)
      {
         // Session not found — create new
         Session.dwSize = sizeof(DPSESSIONDESC2);
         Session.dwFlags = DPSESSION_MIGRATEHOST;
         Session.guidApplication = GUID_NULL;
         Session.dwMaxPlayers = 100;
         Session.dwCurrentPlayers = 0;
         Session.lpszSessionNameA = sessionName;
         Session.dwUser1 = 666;
         Session.dwUser2 = 667;

         hr = Open(&Session, 2);
         if (created)
            *created = 1;
         if (hr >= 0)
         {
            NetSpew(("successfully created %s\n", sessionName));
            return 1;
         }
         NetSpew(("failed to create %s\n", sessionName));
         return 0;
      }

      // Session found — join it
      hr = Open(&Session, 1);
      if (created)
         *created = 0;
      if (hr >= 0)
      {
         NetSpew(("successfully joined %s\n", sessionName));
         return 1;
      }

      NetSpew(("failed to join %s with %s [%x]\n", sessionName, DPlayErrorAsString(hr), hr));
      return 0;
   }

private:
   static BOOL FAR PASCAL ConnCallback(LPCGUID guid, LPVOID lpConnection, DWORD dwSize, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
   {
      int hr;
      void *data;
      char *ctx;
      char buf[256];

      ctx = (char *)lpContext;
      if (strnicmp(ctx, lpName->lpszShortNameA, strlen((char *)lpContext)) != 0)
         return 1;

      data = Malloc(dwSize);
      memcpy(data, lpConnection, dwSize);
      hr = gDP->InitializeConnection(data, 0);
      switch (hr)
      {
      case DPERR_INVALIDFLAGS:
         NetSpew(("Status DPERR_INVALIDFLAGS!\n"));
         break;
      case DPERR_INVALIDPARAMS:
         NetSpew(("Status DPERR_INVALIDPARAMS!\n"));
         break;
      case DPERR_ALREADYINITIALIZED:
         NetSpew(("Status DPERR_ALREADYINITIALIZED!\n"));
         break;
      case DP_OK:
         NetSpew(("Status OK!\n"));
         break;
      case DPERR_UNAVAILABLE:
         sprintf(buf, "You do not have the right hardware & drivers to use %s", ctx);
         MessageBoxA(NULL, buf, NULL, 0);
         NetSpew(("Status DPERR_UNAVAILABLE!\n"));
         break;
      default:
         NetSpew(("Direct play InitializeConnection returned an unknown failure [%x]!\n", hr));
         break;
      }
      return 1;
   }
public:

private:
   static BOOL FAR PASCAL SessionCallback(LPCDPSESSIONDESC2 lpThisSD, LPDWORD lpdwTimeOut, DWORD dwFlags, LPVOID lpContext)
   {
      SessCon *sc = (SessCon *)lpContext;
      if (!lpThisSD)
      {
         sc->none = 1;
         return 0;
      }

      if (sc->name)
      {
         if (stricmp(sc->name, lpThisSD->lpszSessionNameA) != 0)
            return 1;
      }

      sc->none = 0;
      memcpy(&Session, lpThisSD, lpThisSD->dwSize);
      strcpy(session_buffer, lpThisSD->lpszSessionNameA);
      Session.lpszSessionNameA = session_buffer;
      return 0;
   }
public:

   STDMETHODIMP_(BOOL) Host(char *media, char *sessionName)
   {
      HRESULT hr;
      IDirectPlayLobby3 *pLobby;
      void *pAddress;
      DWORD addressSize;

      hr = CoCreateInstance(CLSID_DirectPlayLobby, 0, 1, IID_IDirectPlayLobby3A, (LPVOID *)&pLobby);
      if (hr < 0)
      {
         NetSpew(("Couldn't create Lobby Interface - %s\n", DPlayErrorAsString(hr)));
         return 0;
      }

      hr = CreateServiceProviderAddress(pLobby, &pAddress, &addressSize, "");
      if (hr < 0)
      {
         NetSpew(("Couldn't create host address!\n"));
         return 0;
      }

      if (pLobby) pLobby->Release();
      pLobby = 0;

      hr = gDP->InitializeConnection(pAddress, 0);
      Free(pAddress);
      if (hr < 0 && hr != DPERR_ALREADYINITIALIZED)
      {
         NetSpew(("Couldn't initialize connection - %s\n", DPlayErrorAsString(hr)));
         return 0;
      }

      Session.dwSize = sizeof(DPSESSIONDESC2);
      Session.dwFlags = DPSESSION_DIRECTPLAYPROTOCOL | DPSESSION_OPTIMIZELATENCY;

      if (!config_get_raw("net_no_keepalive", 0, 0))
         Session.dwFlags |= DPSESSION_KEEPALIVE;

      Session.guidApplication = GUID_NULL;
      Session.dwMaxPlayers = 100;
      Session.dwCurrentPlayers = 0;
      Session.lpszSessionNameA = sessionName;
      Session.dwUser1 = 666;
      Session.dwUser2 = 667;

      hr = Open(&Session, 2);
      if (hr >= 0)
      {
         NetSpew(("successfully created %s\n", sessionName));
         return 1;
      }

      NetSpew(("failed to create %s\n", sessionName));
      return 0;
   }

   STDMETHODIMP_(BOOL) Join(char *media, char *sessionName, char *IPAddress)
   {
      HRESULT hr;
      IDirectPlayLobby3 *pLobby;
      void *pAddress = 0;
      DWORD addressSize;
      SessCon sc;

      hr = CoCreateInstance(CLSID_DirectPlayLobby, 0, 1, IID_IDirectPlayLobby3A, (LPVOID *)&pLobby);
      if (hr < 0)
      {
         NetSpew(("Couldn't create Lobby Interface - %s\n", DPlayErrorAsString(hr)));
         return 0;
      }

      hr = CreateServiceProviderAddress(pLobby, &pAddress, &addressSize, IPAddress);
      if (hr < 0)
      {
         NetSpew(("Error creating CSAddress - %s\n", DPlayErrorAsString(hr)));
         if (pLobby) pLobby->Release();
         pLobby = 0;
         return 0;
      }

      if (pLobby) pLobby->Release();
      pLobby = 0;

      hr = gDP->InitializeConnection(pAddress, 0);
      Free(pAddress);
      if (hr < 0 && hr != DPERR_ALREADYINITIALIZED)
      {
         NetSpew(("Couldn't initialize connection - %s\n", DPlayErrorAsString(hr)));
         return 0;
      }

      memset(&Session, 0, sizeof(DPSESSIONDESC2));
      Session.dwSize = sizeof(DPSESSIONDESC2);
      Session.guidApplication = GUID_NULL;

      sc.none = 0;
      sc.name = sessionName;
      hr = EnumSessions(&Session, 0, SessionCallback, &sc, 0);
      if (sc.none != 0)
      {
         NetSpew(("session %s not found (sc.none is TRUE)\n", sessionName));
         return 0;
      }

      hr = Open(&Session, 1);
      if (hr >= 0)
      {
         NetSpew(("successfully joined %s\n", sessionName));
         return 1;
      }

      NetSpew(("failed to join %s with %s [%x]\n", sessionName, DPlayErrorAsString(hr), hr));
      return 0;
   }

   STDMETHODIMP GetPlayerAddress(DPID player, char *pBuffer, int bufSize)
   {
      HRESULT hr;
      DWORD size = 0;
      LPVOID pTmp;
      IDirectPlayLobby3 *pLobby;
      sAddrBuf addrBuf;

      hr = GetPlayerAddress(player, NULL, (LPDWORD)&size);
      if (size == 0)
         return E_FAIL;

      pTmp = malloc(size);
      hr = GetPlayerAddress(player, pTmp, (LPDWORD)&size);
      if (hr < 0)
      {
         free(pTmp);
         return E_FAIL;
      }

      hr = CoCreateInstance(CLSID_DirectPlayLobby, 0, 1, IID_IDirectPlayLobby3A, (LPVOID *)&pLobby);
      if (hr < 0)
      {
         mprintf("Couldn't create Lobby Interface - %s\n", DPlayErrorAsString(hr));
         return E_FAIL;
      }

      addrBuf.buffer = pBuffer;
      addrBuf.size = bufSize;
      hr = pLobby->EnumAddress(addressCallback, pTmp, size, &addrBuf);
      free(pTmp);
      if (hr < 0)
      {
         mprintf("Couldn't get player address - %s\n", DPlayErrorAsString(hr));
         return E_FAIL;
      }

      if (pLobby)
         pLobby->Release();
      pLobby = 0;
      return 0;
   }

   static BOOL FAR PASCAL addressCallback(REFGUID guid, DWORD dataSize, LPCVOID data, LPVOID context)
   {
      unsigned int copySize;
      unsigned int tmpSize;
      sAddrBuf *addrBuf;

      if (IsEqualGUID(guid, DPAID_INet))
      {
         addrBuf = (sAddrBuf *)context;
         if ((unsigned int)addrBuf->size < dataSize)
            copySize = addrBuf->size;
         else
            copySize = dataSize;

         tmpSize = copySize;
         memcpy(addrBuf->buffer, data, tmpSize);
         return 0;
      }
      return 1;
   }

   STDMETHODIMP_(DPID) SimpleCreatePlayer(char *name)
   {
      DPID dpId;
      HRESULT hr;
      DPNAME dpName;

      dpId = 0;
      dpName.dwSize = sizeof(dpName);
      dpName.dwFlags = 0;
      dpName.lpszShortNameA = name;
      dpName.lpszLongNameA = name;
      hr = CreatePlayer(&dpId, &dpName, 0, 0, 0, 0);
      if (hr == 0)
      {
         ResetPlayerData(dpId, 0);
         NetSpew(("created %s okay\n", name));
      }
      else
         NetSpew(("Couldn't create %s, exiting\n", name));
      return dpId;
   }

   STDMETHODIMP_(DPID) IdForName(char *name)
   {
      PlayCon pc;

      pc.name = name;
      pc.id = 0;
      EnumPlayers(NULL, PlayerCallback, &pc, 0);
      return pc.id;
   }

private:
   static BOOL FAR PASCAL PlayerCallback(DPID dpId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
   {
      PlayCon *pc = (PlayCon *)lpContext;
      if (stricmp(lpName->lpszShortNameA, pc->name) == 0)
      {
         pc->id = dpId;
         return 0;
      }
      return 1;
   }
public:

#ifdef NET_ALLOW_SIMULATION
   STDMETHODIMP_(void) UseInternetSimulation(BOOL value)
   {
      m_SimulateInternet = value;
   }

   STDMETHODIMP_(BOOL) IsSimulatingInternet()
   {
      return m_SimulateInternet;
   }

   STDMETHODIMP_(void) SetFullInternetParameters(double minLatency, double aveLatency, double maxLatency, ulong packetLoss, ulong duplicates, ulong truncation)
   {
      m_MessageDelayQueue.SetFullInternetParameters(packetLoss, (ulong)minLatency, (ulong)aveLatency, (ulong)maxLatency, duplicates, truncation);
   }

   STDMETHODIMP_(void) SetInternetParameters(double minLatency, double aveLatency, double maxLatency, ulong packetLoss)
   {
      m_MessageDelayQueue.SetInternetParameters(packetLoss, (ulong)minLatency, (ulong)aveLatency, (ulong)maxLatency);
   }
#endif

   STDMETHODIMP_(void) ResetPlayerData(DPID a, void *b)
   {
      sHostData *hd = new sHostData;
      sLocalData ldata;
      unsigned int lsize;

      ldata.hostData = hd;
      ldata.userData = b;
      lsize = 8;
      memset(hd, 0, sizeof(sHostData));
      hd->scale = 1.0;
      gDP->SetPlayerData(a, &ldata, 8, 1);
   }

#ifdef NET_ALLOW_TIME_SYNCH
   STDMETHODIMP_(double) ToThisGameTime(double player_time, DPID player)
   {
      sHostData *hd = GetHostData(player);
      if (!hd)
         return player_time;
      return (player_time - hd->delta) / hd->scale;
   }

   STDMETHODIMP_(double) PlayerTimeScale(DPID player)
   {
      sHostData *hd = GetHostData(player);
      return (hd) ? hd->scale : 1.0;
   }

   STDMETHODIMP_(double) PlayerTimeDelta(DPID player)
   {
      sHostData *hd = GetHostData(player);
      return (hd) ? hd->delta : 0.0;
   }

   STDMETHODIMP_(void) UpdatePlayerTime(double game_time, double player_time, DPID player)
   {
      sHostData *hd = GetHostData(player);
      double delta = player_time - game_time;
      if (!hd)
         return;
      if (hd->time_of_delta2 == 0.0 || (game_time - hd->time_of_delta2) >= 5.0 || delta > hd->delta2)
      {
         hd->delta2 = delta;
         hd->time_of_delta2 = game_time;
      }
      if (hd->time_of_delta == 0.0 || delta > hd->delta)
      {
         hd->delta = delta;
         hd->time_of_delta = game_time;
      }
      else if ((game_time - hd->time_of_delta) > 10.0)
      {
         hd->delta = hd->delta2;
         hd->time_of_delta = hd->time_of_delta2;
      }
      hd->scale = 1.0;
   }
#endif

   STDMETHODIMP Send(DPID a, DPID b, DWORD c, LPVOID d, DWORD e)
   {
#line 873
      AssertMsg1(e > 0, "Nonpositive send size %d", e);
#ifdef NET_ALLOW_SIMULATION
      if (!m_SimulateInternet)
         return gDP->Send(a, b, c, d, e);

      void *buf = new char[e];
      memcpy(buf, d, e);
      m_MessageDelayQueue.DelayMessage(a, b, c, buf, e);
      return 0;
#else
      return gDP->Send(a, b, c, d, e);
#endif
   }

   STDMETHODIMP SendEx(DPID a, DPID b, DWORD c, LPVOID d, DWORD e, DWORD f, DWORD g, LPVOID h, LPDWORD i)
   {
#ifdef NET_ALLOW_SIMULATION
      if (!m_SimulateInternet)
         return gDP->SendEx(a, b, c, d, e, f, g, h, i);

      void *buf = new char[e];
      memcpy(buf, d, e);
      m_MessageDelayQueue.DelayMessage(a, b, c, buf, e);
      return 0;
#else
      return gDP->SendEx(a, b, c, d, e, f, g, h, i);
#endif
   }

   STDMETHODIMP Receive(LPDPID a, LPDPID b, DWORD c, LPVOID d, LPDWORD e)
   {
      HRESULT hr;





      hr = gDP->Receive(a, b, c, d, e);
#ifdef NET_ALLOW_SIMULATION
      if (m_SimulateInternet)
      {
         DWORD to, from, size;
         void *data;
         DWORD flags;
         HRESULT sendResult;
         do
         {
            m_MessageDelayQueue.NextReadyMessage(&from, &to, &flags, &data, &size);
            if (data == NULL) break;
            sendResult = gDP->SendEx(from, to, flags, data, size, 0, 0, 0, 0);
            delete data;
         }
         while (sendResult >= 0);
      }
#endif
      if (hr == 0 && *a == 0 && ((DWORD *)d)[0] == 5)
      {
         sLocalData *ldata = (sLocalData *)((DWORD *)d)[3];
         if (ldata)
         {
            ((DWORD *)d)[3] = (DWORD)ldata->userData;
            ((DWORD *)d)[4] = 4;
         }
      }
      return hr;
   }

   STDMETHODIMP Open(LPDPSESSIONDESC2 a, DWORD b)
   {
      HRESULT hr = gDP->Open(a, b);
      if (hr >= 0)
         EnumPlayers(NULL, InitExistingPlayer, this, 0);
      return hr;
   }



private:
   static BOOL FAR PASCAL InitExistingPlayer(DPID dpId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
   {
      void *ctx = lpContext;
      ((INet *)ctx)->ResetPlayerData(dpId, NULL);
      return 1;
   }
public:

   STDMETHODIMP CreatePlayer(LPDPID a, LPDPNAME b, HANDLE c, LPVOID d, DWORD e, DWORD f)
   {
      HRESULT hr = gDP->CreatePlayer(a, b, c, d, e, f);
      if (hr >= 0)
         ResetPlayerData(*a, 0);
      else
         Warning(("Couldn't create player: %s\n", DPlayErrorAsString(hr)));
      return hr;
   }

   STDMETHODIMP SetPlayerData(DPID a, LPVOID b, DWORD c, DWORD d)
   {
      if (!(d & 1))
         return gDP->SetPlayerData(a, b, c, d);

#line 996
      AssertMsg(c == 4, "SetPlayerData for local data can only take a size of 4 bytes");
      sLocalData ldata;
      HRESULT hr = GetLocalPlayerData(a, &ldata);
      if (hr >= 0)
      {
         ldata.userData = *(void **)b;
         return gDP->SetPlayerData(a, &ldata, 8, 1);
      }
      return hr;
   }

   STDMETHODIMP GetPlayerData(DPID a, LPVOID b, LPDWORD c, DWORD d)
   {
      HRESULT hr;
      sLocalData ldata;

      if (!(d & 1))
         return gDP->GetPlayerData(a, b, c, d);

      AssertMsg(*c == 4, "SetPlayerData for local data can only take a size of 4 bytes");
      hr = GetLocalPlayerData(a, &ldata);
      if (hr >= 0)
         *(DWORD *)b = (DWORD)ldata.userData;
      return hr;
   }

   STDMETHODIMP_(const char *) DPlayErrorAsString(HRESULT a)
   {
#ifdef SHIP
      switch (a)
      {
         case DPERR_SESSIONLOST:
            return "DPERR_SESSIONLOST";
         case DPERR_INVALIDOBJECT:
            return "DPERR_INVALIDOBJECT";
         case DPERR_BUFFERTOOSMALL:
            return "DPERR_BUFFERTOOSMALL";
         case DPERR_ALREADYINITIALIZED:
            return "DPERR_ALREADYINITIALIZED";
         case DPERR_ACCESSDENIED:
            return "DPERR_ACCESSDENIED";
         case DPERR_ACTIVEPLAYERS:
            return "DPERR_ACTIVEPLAYERS";
         case DPERR_INVALIDPARAM:
            return "DPERR_INVALIDPARAM";
         case DPERR_GENERIC:
            return "DPERR_GENERIC";
         case DPERR_UNSUPPORTED:
            return "DPERR_UNSUPPORTED";
         case DPERR_NOINTERFACE:
            return "DPERR_NOINTERFACE";
         case DPERR_NOMEMORY:
            return "DPERR_NOMEMORY";
         case DPERR_CANTADDPLAYER:
            return "DPERR_CANTADDPLAYER";
         case DPERR_CANTCREATEGROUP:
            return "DPERR_CANTCREATEGROUP";
         case DPERR_CANTCREATEPLAYER:
            return "DPERR_CANTCREATEPLAYER";
         case DPERR_CANTCREATESESSION:
            return "DPERR_CANTCREATESESSION";
         case DPERR_CAPSNOTAVAILABLEYET:
            return "DPERR_CAPSNOTAVAILABLEYET";
         case DPERR_EXCEPTION:
            return "DPERR_EXCEPTION";
         case DPERR_INVALIDFLAGS:
            return "DPERR_INVALIDFLAGS";
         case DPERR_INVALIDPLAYER:
            return "DPERR_INVALIDPLAYER";
         case DPERR_INVALIDGROUP:
            return "DPERR_INVALIDGROUP";
         case DPERR_NOCAPS:
            return "DPERR_NOCAPS";
         case DPERR_NOCONNECTION:
            return "DPERR_NOCONNECTION";
         case DPERR_NOMESSAGES:
            return "DPERR_NOMESSAGES";
         case DPERR_NONAMESERVERFOUND:
            return "DPERR_NONAMESERVERFOUND";
         case DPERR_NOPLAYERS:
            return "DPERR_NOPLAYERS";
         case DPERR_NOSESSIONS:
            return "DPERR_NOSESSIONS";
         case DPERR_SENDTOOBIG:
            return "DPERR_SENDTOOBIG";
         case DPERR_TIMEOUT:
            return "DPERR_TIMEOUT";
         case DPERR_UNAVAILABLE:
            return "DPERR_UNAVAILABLE";
         case DPERR_BUSY:
            return "DPERR_BUSY";
         case DPERR_USERCANCEL:
            return "DPERR_USERCANCEL";
         case DPERR_CANNOTCREATESERVER:
            return "DPERR_CANNOTCREATESERVER";
         case DPERR_PLAYERLOST:
            return "DPERR_PLAYERLOST";
         case DPERR_NOSERVICEPROVIDER:
            return "DPERR_NOSERVICEPROVIDER";
         case DPERR_INVALIDPRIORITY:
            return "DPERR_INVALIDPRIORITY";
         case DPERR_UNINITIALIZED:
            return "DPERR_UNINITIALIZED";
         case DPERR_NONEWPLAYERS:
            return "DPERR_NONEWPLAYERS";
         case DPERR_INVALIDPASSWORD:
            return "DPERR_INVALIDPASSWORD";
         case DPERR_CONNECTING:
            return "DPERR_CONNECTING";
         case DPERR_CONNECTIONLOST:
            return "DPERR_CONNECTIONLOST";
         case DPERR_UNKNOWNMESSAGE:
            return "DPERR_UNKNOWNMESSAGE";
         case DPERR_CANCELFAILED:
            return "DPERR_CANCELFAILED";
         case DPERR_BUFFERTOOLARGE:
            return "DPERR_BUFFERTOOLARGE";
         case DPERR_NOTHANDLED:
            return "DPERR_NOTHANDLED";
         case DPERR_CANCELLED:
            return "DPERR_CANCELLED";
         case DPERR_ABORTED:
            return "DPERR_ABORTED";
         case DPERR_CANTCREATEPROCESS:
            return "DPERR_CANTCREATEPROCESS";
         case DPERR_APPNOTSTARTED:
            return "DPERR_APPNOTSTARTED";
         case DPERR_INVALIDINTERFACE:
            return "DPERR_INVALIDINTERFACE";
         case DPERR_ENCRYPTIONFAILED:
            return "DPERR_ENCRYPTIONFAILED";
         case DPERR_ALREADYREGISTERED:
            return "DPERR_ALREADYREGISTERED";
         case DPERR_UNKNOWNAPPLICATION:
            return "DPERR_UNKNOWNAPPLICATION";
         case DPERR_NOTLOBBIED:
            return "DPERR_NOTLOBBIED";
         case DPERR_SERVICEPROVIDERLOADED:
            return "DPERR_SERVICEPROVIDERLOADED";
         case DPERR_NOTREGISTERED:
            return "DPERR_NOTREGISTERED";
         case DPERR_AUTHENTICATIONFAILED:
            return "DPERR_AUTHENTICATIONFAILED";
         case DPERR_CANTLOADSSPI:
            return "DPERR_CANTLOADSSPI";
         case DPERR_CANTLOADCAPI:
            return "DPERR_CANTLOADCAPI";
         case DPERR_SIGNFAILED:
            return "DPERR_SIGNFAILED";
         case DPERR_CANTLOADSECURITYPACKAGE:
            return "DPERR_CANTLOADSECURITYPACKAGE";
         case DPERR_ENCRYPTIONNOTSUPPORTED:
            return "DPERR_ENCRYPTIONNOTSUPPORTED";
         case DPERR_NOTLOGGEDIN:
            return "DPERR_NOTLOGGEDIN";
         case DPERR_LOGONDENIED:
            return "DPERR_LOGONDENIED";
         case 0:
            return "DP_OK";
      }
      return "Unknown DPlay Error";
#else
      return "Dplay Error";
#endif
   }

   STDMETHODIMP AddPlayerToGroup(DPID a, DPID b)
   {
      return gDP->AddPlayerToGroup(a, b);
   }

   STDMETHODIMP Close()
   {
      return gDP->Close();
   }

   STDMETHODIMP CreateGroup(LPDPID a, LPDPNAME b, LPVOID c, DWORD d, DWORD e)
   {
      return gDP->CreateGroup(a, b, c, d, e);
   }

   STDMETHODIMP DeletePlayerFromGroup(DPID a, DPID b)
   {
      return gDP->DeletePlayerFromGroup(a, b);
   }

   STDMETHODIMP DestroyGroup(DPID a)
   {
      return gDP->DestroyGroup(a);
   }

   STDMETHODIMP DestroyPlayer(DPID a)
   {
      return gDP->DestroyPlayer(a);
   }

   STDMETHODIMP EnumGroupPlayers(DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e)
   {
      return gDP->EnumGroupPlayers(a, b, c, d, e);
   }

   STDMETHODIMP EnumGroups(LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d)
   {
      return gDP->EnumGroups(a, b, c, d);
   }

   STDMETHODIMP EnumPlayers(LPGUID a, LPDPENUMPLAYERSCALLBACK2 b, LPVOID c, DWORD d)
   {
      return gDP->EnumPlayers(a, b, c, d);
   }

   STDMETHODIMP EnumSessions(LPDPSESSIONDESC2 a, DWORD b, LPDPENUMSESSIONSCALLBACK2 c, LPVOID d, DWORD e)
   {
      return gDP->EnumSessions(a, b, c, d, e);
   }

   STDMETHODIMP GetCaps(LPDPCAPS a, DWORD b)
   {
      return gDP->GetCaps(a, b);
   }

   STDMETHODIMP GetGroupData(DPID a, LPVOID b, LPDWORD c, DWORD d)
   {
      return gDP->GetGroupData(a, b, c, d);
   }

   STDMETHODIMP GetGroupName(DPID a, LPVOID b, LPDWORD c)
   {
      return gDP->GetGroupName(a, b, c);
   }

   STDMETHODIMP GetMessageCount(DPID a, LPDWORD b)
   {
      return gDP->GetMessageCount(a, b);
   }

   STDMETHODIMP GetPlayerAddress(DPID a, LPVOID b, LPDWORD c)
   {
      return gDP->GetPlayerAddress(a, b, c);
   }

   STDMETHODIMP GetPlayerCaps(DPID a, LPDPCAPS b, DWORD c)
   {
      return gDP->GetPlayerCaps(a, b, c);
   }

   STDMETHODIMP GetPlayerName(DPID a, LPVOID b, LPDWORD c)
   {
      return gDP->GetPlayerName(a, b, c);
   }

   STDMETHODIMP GetSessionDesc(LPVOID a, LPDWORD b)
   {
      return gDP->GetSessionDesc(a, b);
   }

   STDMETHODIMP Initialize(LPGUID a)
   {
      return gDP->Initialize(a);
   }

   STDMETHODIMP SetGroupData(DPID a, LPVOID b, DWORD c, DWORD d)
   {
      return gDP->SetGroupData(a, b, c, d);
   }

   STDMETHODIMP SetGroupName(DPID a, LPDPNAME b, DWORD c)
   {
      return gDP->SetGroupName(a, b, c);
   }

   STDMETHODIMP SetPlayerName(DPID a, LPDPNAME b, DWORD c)
   {
      return gDP->SetPlayerName(a, b, c);
   }

   STDMETHODIMP SetSessionDesc(LPDPSESSIONDESC2 a, DWORD b)
   {
      return gDP->SetSessionDesc(a, b);
   }

   STDMETHODIMP AddGroupToGroup(DPID a, DPID b)
   {
      return gDP->AddGroupToGroup(a, b);
   }

   STDMETHODIMP CreateGroupInGroup(DPID a, LPDPID b, LPDPNAME c, LPVOID d, DWORD e, DWORD f)
   {
      return gDP->CreateGroupInGroup(a, b, c, d, e, f);
   }

   STDMETHODIMP DeleteGroupFromGroup(DPID a, DPID b)
   {
      return gDP->DeleteGroupFromGroup(a, b);
   }

   STDMETHODIMP EnumConnections(LPCGUID a, LPDPENUMCONNECTIONSCALLBACK b, LPVOID c, DWORD d)
   {
      return gDP->EnumConnections(a, b, c, d);
   }

   STDMETHODIMP EnumGroupsInGroup(DPID a, LPGUID b, LPDPENUMPLAYERSCALLBACK2 c, LPVOID d, DWORD e)
   {
      return gDP->EnumGroupsInGroup(a, b, c, d, e);
   }

   STDMETHODIMP GetGroupConnectionSettings(DWORD a, DPID b, LPVOID c, LPDWORD d)
   {
      return gDP->GetGroupConnectionSettings(a, b, c, d);
   }

   STDMETHODIMP InitializeConnection(LPVOID a, DWORD b)
   {
      return gDP->InitializeConnection(a, b);
   }

   STDMETHODIMP SecureOpen(LPCDPSESSIONDESC2 a, DWORD b, LPCDPSECURITYDESC c, LPCDPCREDENTIALS d)
   {
      return gDP->SecureOpen(a, b, c, d);
   }

   STDMETHODIMP SendChatMessage(DPID a, DPID b, DWORD c, LPDPCHAT d)
   {
      return gDP->SendChatMessage(a, b, c, d);
   }

   STDMETHODIMP SetGroupConnectionSettings(DWORD a, DPID b, LPDPLCONNECTION c)
   {
      return gDP->SetGroupConnectionSettings(a, b, c);
   }

   STDMETHODIMP StartSession(DWORD a, DPID b)
   {
      return gDP->StartSession(a, b);
   }

   STDMETHODIMP GetGroupFlags(DPID a, LPDWORD b)
   {
      return gDP->GetGroupFlags(a, b);
   }

   STDMETHODIMP GetGroupParent(DPID a, LPDPID b)
   {
      return gDP->GetGroupParent(a, b);
   }

   STDMETHODIMP GetPlayerAccount(DPID a, DWORD b, LPVOID c, LPDWORD d)
   {
      return gDP->GetPlayerAccount(a, b, c, d);
   }

   STDMETHODIMP GetPlayerFlags(DPID a, LPDWORD b)
   {
      return gDP->GetPlayerFlags(a, b);
   }

   STDMETHODIMP GetGroupOwner(DPID a, LPDPID b)
   {
      return gDP->GetGroupOwner(a, b);
   }

   STDMETHODIMP SetGroupOwner(DPID a, DPID b)
   {
      return gDP->SetGroupOwner(a, b);
   }

   STDMETHODIMP GetMessageQueue(DPID a, DPID b, DWORD c, LPDWORD d, LPDWORD e)
   {
      return gDP->GetMessageQueue(a, b, c, d, e);
   }

   STDMETHODIMP CancelMessage(DWORD a, DWORD b)
   {
      return gDP->CancelMessage(a, b);
   }

   STDMETHODIMP CancelPriority(DWORD a, DWORD b, DWORD c)
   {
      return gDP->CancelPriority(a, b, c);
   }
};

IDirectPlay4 *cNet::gDP;
char cNet::session_buffer[80];
DPSESSIONDESC2 cNet::Session;

HRESULT CreateServiceProviderAddress(IDirectPlayLobby3 *pIDirectPlayLobby3A, void **ppDPlayAddress, DWORD *puDPlayAddressSize, char *IPAddress)
{
   DWORD tempSize;
   GUID tcpipGuid;
   unsigned short port;
   DPCOMPOUNDADDRESSELEMENT elements[3];
   char ipBuffer[52];
   int i;
   int result;
   int hr;

   result = 0;
   tempSize = 0;

   // Copy DPSPGUID_TCPIP (dead store, originally done by the Thief code)
   tcpipGuid = DPSPGUID_TCPIP;

   port = 0;

   // Element 0: Service Provider (TCP/IP) - guidDataType identifies the type as
   // DPAID_ServiceProvider, and lpData points to the actual service provider GUID.
   i = 0;
   elements[i].guidDataType = DPAID_ServiceProvider;
   elements[i].dwDataSize = sizeof(GUID);
   elements[i].lpData = (LPVOID)&DPSPGUID_TCPIP;
   i++;

   // Copy IP address
   lstrcpyA(ipBuffer, IPAddress);

   // Element 1: IP Address (DPAID_INet)
   elements[i].guidDataType = DPAID_INet;
   elements[i].dwDataSize = lstrlenA(ipBuffer) + 1;
   elements[i].lpData = ipBuffer;
   i++;

   // Check for custom port
   if (config_get_single_value("net_port", CONFIG_INT_TYPE, &port))
   {
      if (port > 0)
      {
         // Element 2: Port (DPAID_INetPort)
         elements[i].guidDataType = DPAID_INetPort;
         elements[i].dwDataSize = 2;
         elements[i].lpData = &port;
         i++;
      }
   }

   // Create the compound address (first call with NULL to get required size)
   result = pIDirectPlayLobby3A->CreateCompoundAddress(elements, i, NULL, &tempSize);
   if (result != DPERR_BUFFERTOOSMALL)
      return result;

   result = (int)(size_t)Malloc(tempSize);
   if (result == NULL)
      return DPERR_OUTOFMEMORY;

   hr = pIDirectPlayLobby3A->CreateCompoundAddress(elements, i, (void*)result, &tempSize);
   if (hr < 0)
      return hr;

   *ppDPlayAddress = (void*)result;
   *puDPlayAddressSize = tempSize;
   return 0;
}

void NetCreate(void)
{
   IUnknown *pOuter;
   cNet *pNet;

   pOuter = _AppGetAggregated(IID_IUnknown);
#ifdef DBG_ON
   pNet = new(__FILE__, __LINE__) cNet(pOuter);
#else
   pNet = new cNet(pOuter);
#endif
}

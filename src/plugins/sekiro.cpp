//
// Copyright 2019 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Sekiro Fix"

#pragma region DEATH_TO_WINSOCK
#undef AF_IPX
#undef AF_MAX
#undef SO_DONTLINGER
#undef IN_CLASSA

#define WSA_WAIT_IO_COMPLETION  (WAIT_IO_COMPLETION)
#define WSAPROTOCOL_LEN  255
#define FD_MAX_EVENTS    10

#define MAX_PROTOCOL_CHAIN 7

#define BASE_PROTOCOL      1
#define LAYERED_PROTOCOL   0

#pragma pack (push,8)
typedef struct _WSAPROTOCOLCHAIN {
    int ChainLen;                                 /* the length of the chain,     */
                                                  /* length = 0 means layered protocol, */
                                                  /* length = 1 means base protocol, */
                                                  /* length > 1 means protocol chain */
    DWORD ChainEntries[MAX_PROTOCOL_CHAIN];       /* a list of dwCatalogEntryIds */
} WSAPROTOCOLCHAIN, FAR * LPWSAPROTOCOLCHAIN;

typedef struct _WSAPROTOCOL_INFOW {
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    int iVersion;
    int iAddressFamily;
    int iMaxSockAddr;
    int iMinSockAddr;
    int iSocketType;
    int iProtocol;
    int iProtocolMaxOffset;
    int iNetworkByteOrder;
    int iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    WCHAR  szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOW, FAR * LPWSAPROTOCOL_INFOW;

typedef unsigned int             GROUP;

typedef WSAPROTOCOL_INFOW WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOW LPWSAPROTOCOL_INFO;

typedef int socklen_t;
#define WSAAPI                  FAR PASCAL
#define WSAEVENT                HANDLE
#define LPWSAEVENT              LPHANDLE
#define WSAOVERLAPPED           OVERLAPPED
typedef struct _OVERLAPPED *    LPWSAOVERLAPPED;

typedef struct _WSANETWORKEVENTS {
       long lNetworkEvents;
       int iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, FAR * LPWSANETWORKEVENTS;
#pragma pack (pop)

#include <SpecialK/nvapi.h>

sk::ParameterBool* disable_netcode = nullptr;
sk::ParameterBool* uncap_framerate = nullptr;
sk::ParameterBool* kill_limiter    = nullptr;

static bool disable_network_code   = true;
static bool no_frame_limit         = false;
static bool kill_limit             = false;

typedef int (WSAAPI *WSAStartup_pfn)
( WORD      wVersionRequired,
  LPWSADATA lpWSAData );

WSAStartup_pfn
WSAStartup_Original = nullptr;


typedef SOCKET (WSAAPI *WSASocketW_pfn)
( int                 af,
  int                 type,
  int                 protocol,
  LPWSAPROTOCOL_INFOW lpProtocolInfo,
  GROUP               g,
  DWORD               dwFlags );

WSASocketW_pfn
WSASocketW_Original = nullptr;

typedef DWORD (WSAAPI *WSAWaitForMultipleEvents_pfn)
(       DWORD     cEvents,
  const WSAEVENT* lphEvents,
        BOOL      fWaitAll,
        DWORD     dwTimeout,
        BOOL      fAlertable );

WSAWaitForMultipleEvents_pfn
WSAWaitForMultipleEvents_Original = nullptr;

typedef INT (WSAAPI *getnameinfo_pfn)
( const SOCKADDR* pSockaddr,
        socklen_t SockaddrLength,
        PCHAR     pNodeBuffer,
        DWORD     NodeBufferSize,
        PCHAR     pServiceBuffer,
        DWORD     ServiceBufferSize,
        INT       Flags );

getnameinfo_pfn
getnameinfo_Original = nullptr;

typedef int (WSAAPI *WSAEnumNetworkEvents_pfn)
( SOCKET             s,
  WSAEVENT           hEventObject,
  LPWSANETWORKEVENTS lpNetworkEvents );
#pragma endregion DEATH_TO_WINSOCK // This whole thing sucks, drown it in a region!

INT
WSAAPI getnameinfo_Detour (
  const SOCKADDR* pSockaddr,
        socklen_t SockaddrLength,
        PCHAR     pNodeBuffer,
        DWORD     NodeBufferSize,
        PCHAR     pServiceBuffer,
        DWORD     ServiceBufferSize,
        INT       Flags )
{
  SK_LOG_FIRST_CALL

  INT iRet =
    getnameinfo_Original ( pSockaddr, SockaddrLength, pNodeBuffer,
                           NodeBufferSize, pServiceBuffer, ServiceBufferSize,
                           Flags );

  extern void SK_ImGui_Warning (const wchar_t* wszMessage);

  SK_RunOnce (SK_ImGui_Warning (SK_UTF8ToWideChar (pNodeBuffer).c_str ()));

  dll_log->Log (L"(Sekiro) : getnameinfo [%hs]", pNodeBuffer);

  return iRet;
}


DWORD
WSAAPI WSAWaitForMultipleEvents_Detour (
        DWORD     cEvents,
  const WSAEVENT* lphEvents,
        BOOL      fWaitAll,
        DWORD     dwTimeout,
        BOOL      fAlertable )
{
  SK_LOG_FIRST_CALL

  if (! disable_network_code)
  {
    dll_log->Log ( L" >> Calling Thread for Network Activity: %s",
                     SK_Thread_GetName (SK_Thread_GetCurrentId ()).c_str () );
  }

  UNREFERENCED_PARAMETER (cEvents);
  UNREFERENCED_PARAMETER (lphEvents);
  UNREFERENCED_PARAMETER (fWaitAll);
  UNREFERENCED_PARAMETER (dwTimeout);
  UNREFERENCED_PARAMETER (fAlertable);

  if (disable_network_code)
  {
    return
      WSA_WAIT_IO_COMPLETION;
  }

  return
    WSAWaitForMultipleEvents_Original ( cEvents,  lphEvents,
                                        fWaitAll, dwTimeout,
                                        fAlertable );
}

SOCKET
WSAAPI WSASocketW_Detour (
  int                 af,
  int                 type,
  int                 protocol,
  LPWSAPROTOCOL_INFOW lpProtocolInfo,
  GROUP               g,
  DWORD               dwFlags)
{
  SK_LOG_FIRST_CALL

  if (! disable_network_code)
  {
    dll_log->Log ( L" >> Calling Thread for Network Activity: %s",
                     SK_Thread_GetName (SK_Thread_GetCurrentId () ).c_str () );
  }

  UNREFERENCED_PARAMETER (af);
  UNREFERENCED_PARAMETER (type);
  UNREFERENCED_PARAMETER (protocol);
  UNREFERENCED_PARAMETER (lpProtocolInfo);
  UNREFERENCED_PARAMETER (g);
  UNREFERENCED_PARAMETER (dwFlags);

  if (disable_network_code)
  {
    return
      WSAENETDOWN;
  }

  return
    WSASocketW_Original ( af, type, protocol, lpProtocolInfo,
                          g,  dwFlags );
}

int
WSAAPI WSAStartup_Detour (
  WORD      wVersionRequired,
  LPWSADATA lpWSAData )
{
  SK_LOG_FIRST_CALL

  if (disable_network_code)
  {
    return WSASYSNOTREADY;
  }

  return
    WSAStartup_Original (wVersionRequired, lpWSAData);
}


constexpr
  uint8_t
    _FrameLockPattern0 [] =
      { 0x48, 0x8B, 0xC4, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
        0x48, 0x8D, 0x68, 0xA1, 0x48, 0x81, 0xEC, 0xC8, 0x00, 0x00, 0x00, 0x48, 0xC7, 0x44, 0x24,
        0x20, 0xFE };

constexpr
  uint8_t
    _FrameLockPattern1 [] =
      { 0x00, 0x88, 0x88, 0x3C, 0x4C, 0x89, 0xAB, 0x00 };

constexpr
  uint8_t
    _FrameLockMask1 [] =
      { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

constexpr
  uint8_t
    _RunSpeedPattern [] =
      //{ 0xF3, 0x0F, 0x59, 0x05, 0x00, 0x30, 0x92, 0x02, 0x0F, 0x2F, 0xF8 };
        { 0xF3, 0x0F, 0x58, 0x00, 0x0F, 0xC6, 0x00, 0x00, 0x0F, 0x51, 0x00, 0xF3, 0x0F, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x2F };

constexpr
  uint8_t
    _RunSpeedMask [] =
      //{ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        { 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };

static void* _SK_Sekiro_FrameLockAddr0_Optional = nullptr;
static void* _SK_Sekiro_FrameLockAddr1          = (void *)0x141161FD0;
static void* _SK_Sekiro_RunSpeedAddr            = nullptr;//(void *)(uintptr_t)(0x00000001407D4F3D - 1);//nullptr;

bool SK_Sekiro_KillLimiter (bool set)
{
  // Optional Extra Bit of Oomph
  //if (_SK_Sekiro_FrameLockAddr0_Optional != nullptr)
  //{
  //  DWORD dwProtect;
  //
  //  VirtualProtect (_SK_Sekiro_FrameLockAddr0_Optional, 4, PAGE_EXECUTE_READWRITE, &dwProtect);
  //
  //  if (set)
  //    WriteULongRelease ((volatile DWORD *)_SK_Sekiro_FrameLockAddr0_Optional,
  //                              *((DWORD *)"\xC3\x90\x90\x55"));
  //  else
  //    WriteULongRelease ((volatile DWORD *)_SK_Sekiro_FrameLockAddr0_Optional,
  //                              *((DWORD *)"\x48\x8B\xC4\x55"));
  //
  //  VirtualProtect (_SK_Sekiro_FrameLockAddr0_Optional, 4, dwProtect, &dwProtect);
  //
  //  return set;
  //}

  return set;
}

bool SK_Sekiro_UnlimitFramerate (bool set, long double target)
{
  if (_SK_Sekiro_FrameLockAddr1 == (void *)0x141161FD0)
  {
    dll_log->Log (L"%f", *(float *)_SK_Sekiro_FrameLockAddr1);
  }

  __try {
  if (_SK_Sekiro_FrameLockAddr1 != nullptr)
  {
    DWORD dwProtect;

    VirtualProtect (_SK_Sekiro_FrameLockAddr1, 8, PAGE_EXECUTE_READWRITE, &dwProtect);
  //VirtualProtect (_SK_Sekiro_RunSpeedAddr,   1, PAGE_EXECUTE_READWRITE, &dwProtect);

    static uint8_t orig_bytes [4] = { };
    if (set)
    {
      if (orig_bytes [1] == 0x0)
      {
        WriteULongRelease ( (volatile DWORD *)orig_bytes,
                                   *((DWORD *)_SK_Sekiro_FrameLockAddr1) );
      }

      if (target != 0.0)
      {
        float target_delta =
          gsl::narrow_cast <float> ((1000.0 / (target * 1.05)) / 1000.0);

        WriteULongRelease ( (volatile DWORD *)_SK_Sekiro_FrameLockAddr1,
                  *(reinterpret_cast <DWORD *>(&target_delta)));

        int speed = 144 + gsl::narrow_cast <int> (std::ceil (((target - 60.0) / 16.0))) * 8;
        if (speed > 248)
            speed = 248;

        if (_SK_Sekiro_RunSpeedAddr != nullptr)
        {
          WriteUCharRelease ((volatile BYTE *)_SK_Sekiro_RunSpeedAddr,
                    gsl::narrow_cast <uint8_t> (speed));
        }
      }

      else
      {
        WriteULongRelease ((volatile DWORD *)_SK_Sekiro_FrameLockAddr1,
                                  *((DWORD *)"\x00\x00\x00\x00") );

        if (_SK_Sekiro_RunSpeedAddr != nullptr)
        {
          WriteUCharRelease ((volatile BYTE *)_SK_Sekiro_RunSpeedAddr, 0xF8);
        }
      }
    }

    else
    {
      WriteULongRelease ( (volatile DWORD *)_SK_Sekiro_FrameLockAddr1,
                                 *((DWORD *)orig_bytes) );

      if (_SK_Sekiro_RunSpeedAddr != nullptr)
      {
        WriteUCharRelease ( (volatile  BYTE *)_SK_Sekiro_RunSpeedAddr,
                    gsl::narrow_cast <uint8_t> (0x90) );
      }
    }

  //VirtualProtect (_SK_Sekiro_RunSpeedAddr,   1, dwProtect, &dwProtect);
    VirtualProtect (_SK_Sekiro_FrameLockAddr1, 8, dwProtect, &dwProtect);

    return set;
  }

  else
    return false;
  }
  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH  )
  {
    // Oops
    return false;
  }
}


bool
SK_Sekiro_PlugInCfg (void);

HRESULT
STDMETHODCALLTYPE
SK_Sekiro_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (Flags);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (pSwapChain);

  DWORD dwProcId;

  PostMessage (game_window.hWnd, WM_ACTIVATEAPP, FALSE,       GetWindowThreadProcessId (game_window.hWnd, &dwProcId));
  PostMessage (game_window.hWnd, WM_ACTIVATE,    WA_INACTIVE, (LPARAM)game_window.hWnd);

  SK_Thread_CreateEx ([] (LPVOID) ->
  DWORD
  {
    ULONG64 ulFrame = SK_GetFramesDrawn ();

    auto& rb =
      SK_GetCurrentRenderBackend ();


    while (SK_GetFramesDrawn () < (ulFrame + 150))
    {
      SK_Sleep (15UL);
    }

    if (rb.isHDRCapable ())
    {
      SK_LOG0 ((L"Attempting to fix HDR mode at startup (Triggering Redundant Fullscreen Mode Switch)"),
        __SK_SUBSYSTEM__);

      rb.requestFullscreenMode ();
    }

    //_SK_Sekiro_FrameLockAddr0_Optional =
    //  SK_ScanAligned (_FrameLockPattern0, 32, nullptr, 32);
    //
    //_SK_Sekiro_FrameLockAddr1 =
    //  SK_ScanAligned (_FrameLockPattern1, 8, _FrameLockMask1);
    //
    //_SK_Sekiro_RunSpeedAddr =
    //  SK_Scan (_RunSpeedPattern, 21, _RunSpeedMask);
    //
    //if (_SK_Sekiro_RunSpeedAddr != nullptr)
    //{
    //  _SK_Sekiro_RunSpeedAddr =
    //    (void *)((uintptr_t)_SK_Sekiro_RunSpeedAddr + 15);
    //}

    dll_log->Log (L"FrameLock: %p, Framelock1: %p, RunSpeed: %p",
      _SK_Sekiro_FrameLockAddr0_Optional, _SK_Sekiro_FrameLockAddr1, _SK_Sekiro_RunSpeedAddr);

    uncap_framerate = (sk::ParameterBool*)
      g_ParameterFactory->create_parameter <bool> (L"Remove Framerate Limit");

    uncap_framerate->register_to_ini (
      SK_GetDLLConfig (),
        L"Sekiro.Framerate",
          L"Unlimited"
    );

    if (! uncap_framerate->load (no_frame_limit))
    {                            no_frame_limit = false;
      uncap_framerate->store (false);
    }

    kill_limiter = (sk::ParameterBool*)
    g_ParameterFactory->create_parameter <bool> (L"Fixed-Timestep");

    kill_limiter->register_to_ini (
      SK_GetDLLConfig (),
        L"Sekiro.Framerate",
          L"KillRemainingLimiter"
    );

    if (! kill_limiter->load (kill_limit))
    {                         kill_limit = false;
          kill_limiter->store (false);
    }

    SK_Sekiro_UnlimitFramerate (no_frame_limit, kill_limit ?
       std::max (0.0, SK::Framerate::GetLimiter (SK_GetCurrentRenderBackend ().swapchain.p)->get_limit ()) : 0.0);
    SK_Sekiro_KillLimiter      (kill_limit);

    DWORD dwProcId;

    PostMessage (game_window.hWnd, WM_ACTIVATEAPP, TRUE,      GetWindowThreadProcessId (game_window.hWnd, &dwProcId));
    PostMessage (game_window.hWnd, WM_ACTIVATE,    WA_ACTIVE, (LPARAM)game_window.hWnd);

    SK_Thread_CloseSelf ();

    return 0;
  }, L"[SK] Sekiro Delayed Init");

  return S_OK;
}

bool
SK_Sekiro_PlugInCfg (void)
{
  ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
  ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
  ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

  if (ImGui::CollapsingHeader ((const char *)u8R"(Sekiro™: Shadows Die Twice)", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::BeginGroup ();
    if (ImGui::Checkbox ("Disable Network Access to Game", &disable_network_code))
    {
      disable_netcode->store (disable_network_code);

      SK_GetDLLConfig   ()->write (
        SK_GetDLLConfig ()->get_filename ()
      );
    }

    if (nvapi_init && sk::NVAPI::nv_hardware)
    {
      static bool orig_nvapi_hdr_state =
        config.apis.NvAPI.disable_hdr;

      if (ImGui::Checkbox ("Disable NVIDIA driver-based HDR", &config.apis.NvAPI.disable_hdr))
      {
        SK_SaveConfig ();
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Disabling this will resolve multi-monitor performance issues on HDR-capable systems.");

      if (orig_nvapi_hdr_state != config.apis.NvAPI.disable_hdr)
      {
        ImGui::BulletText ("Game Restart Required");
      }
    }
    ImGui::EndGroup ();

    if (_SK_Sekiro_FrameLockAddr1 != nullptr)
    {
      ImGui::SameLine ();

      ImGui::BeginGroup ();

      bool toggle_cap =
        ImGui::Checkbox ("Uncap Framerate", &no_frame_limit);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Smoothest Gameplay Possible");
        ImGui::Separator    ();
        ImGui::BulletText   ("Requires Special K's framerate limit be engaged");
        ImGui::Bullet       ();
        ImGui::SameLine     ();
        ImGui::TextColored  (ImColor::HSV (0.27f, 1.f, 1.f),
                             "If you change the framerate limit, turn this off and back on.");
        ImGui::EndTooltip   ();
      }

      if (toggle_cap)
      {
        no_frame_limit
          = SK_Sekiro_UnlimitFramerate (no_frame_limit, kill_limit ?
                                std::max (0.0, SK::Framerate::GetLimiter (SK_GetCurrentRenderBackend ().swapchain.p)->get_limit ()) : 0.0);

        uncap_framerate->store (no_frame_limit);

        SK_GetDLLConfig   ()->write (
          SK_GetDLLConfig ()->get_filename ()
        );
      }

      if (no_frame_limit)
      {
        auto& rb =
          SK_GetCurrentRenderBackend ();

        static UINT                num_modes   = 0;
        static std::vector <DXGI_MODE_DESC>
                                   modes;
        static std::string         combo_str;
        static std::vector <float> nominal_refresh;
        static std::vector <
                  std::pair <UINT,UINT>
                           >       nominal_ratio;
        static int                 current_item = -1;

        if (num_modes == 0)
        {
          SK_ComPtr   <IDXGIOutput>    pContainer;
          SK_ComQIPtr <IDXGISwapChain> pSwapChain (rb.swapchain);

          if (SUCCEEDED (pSwapChain->GetContainingOutput (&pContainer)))
          {
            DXGI_SWAP_CHAIN_DESC  swapDesc = { };
            pSwapChain->GetDesc (&swapDesc);

            pContainer->GetDisplayModeList ( swapDesc.BufferDesc.Format, 0x0,
                                             &num_modes, nullptr );

            modes.resize (num_modes);

            if ( SUCCEEDED ( pContainer->GetDisplayModeList ( swapDesc.BufferDesc.Format, 0x0,
                                                              &num_modes, modes.data () ) ) )
            {
              int idx = 1;

              combo_str += "Don't Care";
              combo_str += '\0';

              for ( auto mode : modes )
              {
                if (/*mode.Format == swapDesc.BufferDesc.Format &&*/
                      mode.Width  == swapDesc.BufferDesc.Width  &&
                      mode.Height == swapDesc.BufferDesc.Height )
                {
                  ///dll_log->Log ( L" ( %lux%lu -<+| %ws |+>- ) @ %f Hz",
                  ///                             mode.Width,
                  ///                             mode.Height,
                  ///        SK_DXGI_FormatToStr (mode.Format).c_str (),
                  ///              ( (long double)mode.RefreshRate.Numerator /
                  ///                (long double)mode.RefreshRate.Denominator )
                  ///             );

                  combo_str +=
                    SK_FormatString ("%7.03f Hz", gsl::narrow_cast <double> (mode.RefreshRate.Numerator) /
                                                  gsl::narrow_cast <double> (mode.RefreshRate.Denominator));
                  combo_str += '\0';

                  nominal_refresh.push_back (
                    gsl::narrow_cast <float> (
                      std::ceil (
                        gsl::narrow_cast <double> (mode.RefreshRate.Numerator) /
                        gsl::narrow_cast <double> (mode.RefreshRate.Denominator)
                      )
                    )
                  );

                  nominal_ratio.emplace_back (
                    std::make_pair (mode.RefreshRate.Numerator,
                                    mode.RefreshRate.Denominator)
                  );

                  if ( config.render.framerate.refresh_rate >= (nominal_refresh.back () - 1.0f) &&
                       config.render.framerate.refresh_rate <= (nominal_refresh.back () + 1.0f) )
                  {
                    current_item = idx;
                  }

                  ++idx;
                }
              }

              combo_str += '\0';
            }
          }
        }

        static int orig_item = current_item;

        ImGui::SameLine ();

        bool refresh_change =
          ImGui::Combo ("Refresh Rate Override###Sekiro_Refresh", &current_item, combo_str.c_str ());

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("You should also enable Special K's framerate limiter...");

        if (refresh_change)
        {
          if (current_item > 0)
          {
            float override_rate =
              nominal_refresh [current_item - 1];

            config.render.framerate.refresh_rate = override_rate;
            config.render.framerate.rescan_ratio =
              SK_FormatStringW ( L"%lu/%lu",
                                   nominal_ratio [current_item - 1].first,
                                   nominal_ratio [current_item - 1].second );
            config.render.framerate.rescan_.Denom     = nominal_ratio [current_item - 1].second;
            config.render.framerate.rescan_.Numerator = nominal_ratio [current_item - 1].second;
          }

          else
          {
            config.render.framerate.refresh_rate =   -1.0f;
            config.render.framerate.rescan_ratio = L"-1/1";
            config.render.framerate.rescan_.Denom     =  1;
            config.render.framerate.rescan_.Numerator =
                                                  UINT (-1);
          }

          SK_SaveConfig ();
        }

        //bool limit_toggle =
        //  ImGui::Checkbox ("Fixed Timestep Mode", &kill_limit);
        //
        //if (limit_toggle)
        //{
        //  kill_limit
        //    = SK_Sekiro_KillLimiter (kill_limit);
        //
        //  kill_limiter->store (kill_limit);
        //
        //  SK_GetDLLConfig ()->write (
        //    SK_GetDLLConfig ()->get_filename ()
        //  );
        //
        //  kill_limit =
        //  SK_Sekiro_KillLimiter      (kill_limit);
        //  no_frame_limit =
        //  SK_Sekiro_UnlimitFramerate (no_frame_limit, kill_limit ?
        //     std::max (0.0, SK::Framerate::GetLimiter ()->get_limit ()) : 0.0);
        //}

        if (orig_item != current_item)
        {
          if ( config.render.framerate.refresh_rate != -1 )
          {
            rb.requestFullscreenMode ();

            orig_item = current_item;
          }

          else
            ImGui::BulletText ("Game Restart Required for Refresh Rate Override");
        }
      }

      ImGui::EndGroup ();
    }

    ImGui::TreePop  (  );
  }

  ImGui::PopStyleColor (3);

  return true;
}


void
SK_Sekiro_InitPlugin (void)
{
  plugin_mgr->config_fns.emplace      (SK_Sekiro_PlugInCfg);
  plugin_mgr->first_frame_fns.emplace (SK_Sekiro_PresentFirstFrame);

  //void* pFOVBase =
  //  SK_Scan ( "\xF3\x0F\x10\x08\xF3\x0F\x59\x0D\x0C\xE7\x9B\x02", strlen (
  //            "\xF3\x0F\x10\x08\xF3\x0F\x59\x0D\x0C\xE7\x9B\x02"         ), nullptr );

  disable_netcode = (sk::ParameterBool*)
    g_ParameterFactory->create_parameter <bool> (L"Disable Netcode");

  disable_netcode->register_to_ini (
    SK_GetDLLConfig (),
      L"Sekiro.Network",
        L"DisableNetcode" );

  if (! disable_netcode->load  (disable_network_code))
  {                             disable_network_code = true;
        disable_netcode->store (true);
  }

  SK_CreateDLLHook2 (L"Ws2_32.dll", "getnameinfo",
                                     getnameinfo_Detour,              (void **)&getnameinfo_Original);
  SK_CreateDLLHook2 (L"Ws2_32.dll", "WSAWaitForMultipleEvents",
                                     WSAWaitForMultipleEvents_Detour, (void **)&WSAWaitForMultipleEvents_Original);
  SK_CreateDLLHook2 (L"Ws2_32.dll", "WSASocketW",
                                     WSASocketW_Detour,               (void **)&WSASocketW_Original);
  SK_CreateDLLHook2 (L"Ws2_32.dll", "WSAStartup",
                                     WSAStartup_Detour,               (void **)&WSAStartup_Original);

  SK_ApplyQueuedHooks ();
}
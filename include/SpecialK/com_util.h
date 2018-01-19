/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <SpecialK/SpecialK.h>

SK_INCLUDE_START (COM_UTIL)

#include <stdlib.h>
#include <SpecialK/core.h>
#include <SpecialK/thread.h>

class SK_AutoCOMInit
{
public:
  SK_AutoCOMInit (void) {
    HRESULT hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED (hr))
      success = true;
  }

  ~SK_AutoCOMInit (void) {
    if (success)
      CoUninitialize ();
  }

private:
  bool success = false;
};



#define _WIN32_DCOM
#include <Wbemidl.h>

namespace COM {
  struct Base {
    volatile   LONG    init            = FALSE;

    struct WMI {
      volatile LONG    init            = 0;

      IWbemServices*   pNameSpace      = nullptr;
      IWbemLocator*    pWbemLocator    = nullptr;
      BSTR             bstrNameSpace   = nullptr;

      HANDLE           hServerThread   = INVALID_HANDLE_VALUE;
      HANDLE           hShutdownServer = nullptr;

      void Lock         (void);
      void Unlock       (void);
    } wmi;
  } extern base;
};



using CoCreateInstance_pfn = HRESULT (WINAPI *)(
  _In_  REFCLSID  rclsid,
  _In_  LPUNKNOWN pUnkOuter,
  _In_  DWORD     dwClsContext,
  _In_  REFIID    riid,
  _Out_ LPVOID   *ppv );

using CoCreateInstanceEx_pfn = HRESULT (STDAPICALLTYPE *)(
  _In_    REFCLSID     rclsid,
  _In_    IUnknown     *punkOuter,
  _In_    DWORD        dwClsCtx,
  _In_    COSERVERINFO *pServerInfo,
  _In_    DWORD        dwCount,
  _Inout_ MULTI_QI     *pResults );

extern CoCreateInstance_pfn   CoCreateInstance_Original;
extern CoCreateInstanceEx_pfn CoCreateInstanceEx_Original;



static
auto SK_WaitForInit_WMI = [&](void) ->
void
{
  SK_Thread_SpinUntilFlagged (&COM::base.wmi.init);
};


SK_INCLUDE_END (COM_UTIL)
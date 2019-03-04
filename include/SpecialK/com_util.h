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

struct IUnknown;
#include <Unknwnbase.h>

#include <SpecialK/SpecialK.h>
#include <SpecialK/core.h>

#include <objbase.h>

#include <cstdlib>

SK_INCLUDE_START (COM_UTIL)

#include <SpecialK/thread.h>

class SK_AutoCOMInit
{
public:
  SK_AutoCOMInit (DWORD dwCoInit = COINIT_MULTITHREADED) :
           init_flags_ (dwCoInit)
  {
    if (_assert_not_dllmain ())
    {
      const HRESULT hr =
        CoInitializeEx (nullptr, init_flags_);

      if (SUCCEEDED (hr))
        success_ = true;
      else
        init_flags_ = ~init_flags_;
    }
  }

  ~SK_AutoCOMInit (void)
  {
    if (success_)
      CoUninitialize ();
  }

  bool  isInit       (void) { return success_;    }
  DWORD getInitFlags (void) { return init_flags_; }

protected:
  static bool _assert_not_dllmain (void) ;

private:
  DWORD init_flags_ = COINIT_MULTITHREADED;
  bool  success_    = false;
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

      HANDLE           hServerThread = INVALID_HANDLE_VALUE;
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



bool SK_WMI_Init     (void);
void SK_WMI_Shutdown (void);

static
auto SK_WMI_WaitForInit = [&](void) ->
void
{
  SK_Thread_SpinUntilFlagged (&COM::base.wmi.init);
};



extern "C++"
{
  template <class T>
  void
  SK_COM_SafeRelease (T **ppT)
  {
    if (*ppT != nullptr)
    {
      IUnknown_AtomicRelease (
        reinterpret_cast <void **> (ppT)
      );
    }
  }
  
  template <class T>
  bool
  SK_COM_PromoteInterface ( T        **ppT,
                            IUnknown  *pPolymorph )
  {
    if (pPolymorph == nullptr)
      return false;
  
    SK_COM_SafeRelease (ppT);
                       *ppT =
     reinterpret_cast <T *> (
       pPolymorph
     );
  
    return
      ( *ppT == reinterpret_cast <T *> (pPolymorph) );
  }


  template <class T>
  class SK_ComPtr :
      public CComPtrBase <T>
  {
  public:
    SK_ComPtr (void) throw ()
    {
    }
    SK_ComPtr (_Inout_opt_ T* lp) throw () :
      CComPtrBase <T> (lp)
    {
    }
    SK_ComPtr (_Inout_ const SK_ComPtr <T>& lp) throw () :
      CComPtrBase<T>(lp.p)
    {
    }
    T* operator= (_Inout_opt_ T* lp) throw ()
    {
      if (*this != lp)
      {
        SK_ComPtr (lp).Swap (*this);
      }
      return *this;
    }
    template <typename Q>
    T* operator= (_Inout_ const SK_ComPtr <Q>& lp) throw ()
    {
      if (! this->IsEqualObject (lp))
      {
        AtlComQIPtrAssign2 ((IUnknown**)&this->p, lp, __uuidof (T));
      }
      return *this;
    }
    T* operator= (_Inout_ const SK_ComPtr <T>& lp) throw ()
    {
      if (*this != lp)
      {
        SK_ComPtr (lp).Swap (*this);
      }
      return *this;
    }
    SK_ComPtr (_Inout_ SK_ComPtr <T>&& lp) throw () :
      CComPtrBase <T> ()
    {
      lp.Swap (*this);
    }
    T* operator= (_Inout_ SK_ComPtr <T>&& lp) throw ()
    {
      if (*this != lp)
      {
        SK_ComPtr (static_cast <SK_ComPtr&&> (lp)).Swap (*this);
      }
  
      return *this;
    }
  };

  template <class T>
  using SK_ComQIPtr = CComQIPtr <T>;
  
  namespace std
  {
    template <class T>
    struct hash < SK_ComPtr <T> >
    {
      std::size_t operator()(const SK_ComPtr <T>& key) const
      {
        return
          std::hash <T*>()(key.p);
      }
    };
  }
}


SK_INCLUDE_END (COM_UTIL)
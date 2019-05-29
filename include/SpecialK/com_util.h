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

#pragma once

struct IUnknown;
//#include <Unknwnbase.h>

#include <SpecialK/SpecialK.h>
#include <SpecialK/core.h>

#define _WIN32_DCOM
#include <Wbemidl.h>
#include <objbase.h>

#include <cstdlib>

SK_INCLUDE_START (COM_UTIL)

#include <SpecialK/thread.h>

bool SK_COM_TestInit (void);

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

  bool  isInit       (void) noexcept { return success_;    }
  DWORD getInitFlags (void) noexcept { return init_flags_; }

protected:
  static bool _assert_not_dllmain (void);

private:
  DWORD init_flags_ = COINIT_MULTITHREADED;
  bool  success_    = false;
};

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


static __inline
void
SK_WMI_WaitForInit (void)
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




  //SK_ComPtrBase provides the basis for all other smart pointers
//The other smartpointers add their own constructors and operators
template <class T>
class SK_ComPtrBase
{
protected:
    SK_ComPtrBase() throw()
    {
        p = nullptr;
    }
    SK_ComPtrBase(_Inout_opt_ T* lp) throw()
    {
      __try
      {
             p = lp;
        if  (p != nullptr)
             p->AddRef ();
      } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
      {
        p = nullptr;
        return;
      }
    }
    void Swap(SK_ComPtrBase& other) throw ()
    {
        T* pTemp = p;
        p = other.p;
        other.p = pTemp;
    }
public:
    typedef T _PtrClass;
    ~SK_ComPtrBase() throw()
    {
      __try {
        if (p)
            p->Release();
      } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
      {
            p = nullptr;
        return;
      }
    }
    operator T*() const throw()
    {
        return p;
    }
    T& operator*() const
    {
        ATLENSURE(p!=nullptr);
        return *p;
    }
    //The assert on operator& usually indicates a bug.  If this is really
    //what is needed, however, take the address of the p member explicitly.
    T** operator&() throw()
    {
        ATLASSERT(p==nullptr);
        return &p;
    }
    _NoAddRefReleaseOnCComPtr<T>* operator->() const throw()
    {
        ATLASSERT(p!=nullptr);
        return (_NoAddRefReleaseOnCComPtr<T>*)p;
    }
    bool operator!() const throw()
    {
        return (p == nullptr);
    }
    bool operator<(_In_opt_ T* pT) const throw()
    {
        return p < pT;
    }
    bool operator!=(_In_opt_ T* pT) const
    {
        return !operator==(pT);
    }
    bool operator==(_In_opt_ T* pT) const throw()
    {
        return p == pT;
    }

    // Release the interface and set to NULL
    void Release() throw()
    {
        T* pTemp = p;
        if (pTemp)
        {
            p = nullptr;
            pTemp->Release();
        }
    }
    // Compare two objects for equivalence
    inline bool IsEqualObject(_Inout_opt_ IUnknown* pOther) throw();

    // Attach to an existing interface (does not AddRef)
    void Attach(_In_opt_ T* p2) throw()
    {
        if (p)
        {
            ULONG ref = p->Release();
            (ref);
            // Attaching to the same object only works if duplicate references are being coalesced.  Otherwise
            // re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
            ATLASSERT(ref != 0 || p2 != p);
        }
        p = p2;
    }
    // Detach the interface (does not Release)
    T* Detach() throw()
    {
        T* pt = p;
        p = nullptr;
        return pt;
    }
    _Check_return_ HRESULT CopyTo(_COM_Outptr_result_maybenull_ T** ppT) throw()
    {
        ATLASSERT(ppT != nullptr);
        if (ppT == nullptr)
            return E_POINTER;
        *ppT = p;
        if (p)
            p->AddRef();
        return S_OK;
    }
    _Check_return_ HRESULT SetSite(_Inout_opt_ IUnknown* punkParent) throw()
    {
        return AtlSetChildSite(p, punkParent);
    }
    _Check_return_ HRESULT Advise(
        _Inout_ IUnknown* pUnk,
        _In_ const IID& iid,
        _Out_ LPDWORD pdw) throw()
    {
        return AtlAdvise(p, pUnk, iid, pdw);
    }
    _Check_return_ HRESULT CoCreateInstance(
        _In_ REFCLSID rclsid,
        _Inout_opt_ LPUNKNOWN pUnkOuter = nullptr,
        _In_ DWORD dwClsContext = CLSCTX_ALL) throw()
    {
        ATLASSERT(p == nullptr);
        return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
    }
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    _Check_return_ HRESULT CoCreateInstance(
        _In_z_ LPCOLESTR szProgID,
        _Inout_opt_ LPUNKNOWN pUnkOuter = nullptr,
        _In_ DWORD dwClsContext = CLSCTX_ALL) throw()
    {
        CLSID clsid;
        HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
        ATLASSERT(p == nullptr);
        if (SUCCEEDED(hr))
            hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
        return hr;
    }
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    template <class Q>
    _Check_return_ HRESULT QueryInterface(_Outptr_ Q** pp) const throw()
    {
        ATLASSERT(pp != nullptr);
        return p->QueryInterface(__uuidof(Q), (void**)pp);
    }
    T* p;
};

template <class T>
class SK_ComPtr :
    public SK_ComPtrBase<T>
{
public:
    SK_ComPtr() throw()
    {
    }
    SK_ComPtr(_Inout_opt_ T* lp) throw() :
        SK_ComPtrBase<T>(lp)
    {
    }
    SK_ComPtr(_Inout_ const SK_ComPtr<T>& lp) throw() :
        SK_ComPtrBase<T>(lp.p)
    {
    }
    T* operator=(_Inout_opt_ T* lp) throw()
    {
        if(*this!=lp)
        {
            SK_ComPtr(lp).Swap(*this);
        }
        return *this;
    }
    template <typename Q>
    T* operator=(_Inout_ const SK_ComPtr<Q>& lp) throw()
    {
        if( !this->IsEqualObject(lp) )
        {
            AtlComQIPtrAssign2((IUnknown**)&this->p, lp, __uuidof(T));
        }
        return *this;
    }
    T* operator=(_Inout_ const SK_ComPtr<T>& lp) throw()
    {
        if(*this!=lp)
        {
            SK_ComPtr(lp).Swap(*this);
        }
        return *this;
    }
    SK_ComPtr(_Inout_ SK_ComPtr<T>&& lp) throw() :
        SK_ComPtrBase<T>()
    {
        lp.Swap(*this);
    }
    T* operator=(_Inout_ SK_ComPtr<T>&& lp) throw()
    {
        if (*this != lp)
        {
            SK_ComPtr(static_cast<SK_ComPtr&&>(lp)).Swap(*this);
        }
        return *this;
    }
};

//specialization for IDispatch
template <>
class SK_ComPtr<IDispatch> :
    public SK_ComPtrBase<IDispatch>
{
public:
    SK_ComPtr() throw()
    {
    }
    SK_ComPtr(_Inout_opt_ IDispatch* lp) throw() :
        SK_ComPtrBase<IDispatch>(lp)
    {
    }
    SK_ComPtr(_Inout_ const SK_ComPtr<IDispatch>& lp) throw() :
        SK_ComPtrBase<IDispatch>(lp.p)
    {
    }
    IDispatch* operator=(_Inout_opt_ IDispatch* lp) throw()
    {
        if(*this!=lp)
        {
            SK_ComPtr(lp).Swap(*this);
        }
        return *this;
    }
    IDispatch* operator=(_Inout_ const SK_ComPtr<IDispatch>& lp) throw()
    {
        if(*this!=lp)
        {
            SK_ComPtr(lp).Swap(*this);
        }
        return *this;
    }
    SK_ComPtr(_Inout_ SK_ComPtr<IDispatch>&& lp) throw() :
        SK_ComPtrBase<IDispatch>()
    {
        this->p = lp.p;
        lp.p = nullptr;
    }
    IDispatch* operator=(_Inout_ SK_ComPtr<IDispatch>&& lp) throw()
    {
        SK_ComPtr(static_cast<SK_ComPtr&&>(lp)).Swap(*this);
        return *this;
    }
// IDispatch specific stuff
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    _Check_return_ HRESULT GetPropertyByName(
        _In_z_ LPCOLESTR lpsz,
        _Out_ VARIANT* pVar) throw()
    {
        ATLASSERT(this->p);
        ATLASSERT(pVar);
        DISPID dwDispID;
        HRESULT hr = GetIDOfName(lpsz, &dwDispID);
        if (SUCCEEDED(hr))
            hr = GetProperty(dwDispID, pVar);
        return hr;
    }
    _Check_return_ HRESULT GetProperty(
        _In_ DISPID dwDispID,
        _Out_ VARIANT* pVar) throw()
    {
        return GetProperty(this->p, dwDispID, pVar);
    }
    _Check_return_ HRESULT PutPropertyByName(
        _In_z_ LPCOLESTR lpsz,
        _In_ VARIANT* pVar) throw()
    {
        ATLASSERT(this->p);
        ATLASSERT(pVar);
        DISPID dwDispID;
        HRESULT hr = GetIDOfName(lpsz, &dwDispID);
        if (SUCCEEDED(hr))
            hr = PutProperty(dwDispID, pVar);
        return hr;
    }
    _Check_return_ HRESULT PutProperty(
        _In_ DISPID dwDispID,
        _In_ VARIANT* pVar) throw()
    {
        return PutProperty(this->p, dwDispID, pVar);
    }
    _Check_return_ HRESULT GetIDOfName(
        _In_z_ LPCOLESTR lpsz,
        _Out_ DISPID* pdispid) throw()
    {
        return this->p->GetIDsOfNames(IID_NULL, const_cast<LPOLESTR*>(&lpsz), 1, LOCALE_USER_DEFAULT, pdispid);
    }
    // Invoke a method by DISPID with no parameters
    _Check_return_ HRESULT Invoke0(
        _In_ DISPID dispid,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw()
    {
        DISPPARAMS dispparams = { nullptr, nullptr, 0, 0};
        return this->p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, nullptr, nullptr);
    }
    // Invoke a method by name with no parameters
    _Check_return_ HRESULT Invoke0(
        _In_z_ LPCOLESTR lpszName,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw()
    {
        HRESULT hr;
        DISPID dispid;
        hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = Invoke0(dispid, pvarRet);
        return hr;
    }
    // Invoke a method by DISPID with a single parameter
    _Check_return_ HRESULT Invoke1(
        _In_ DISPID dispid,
        _In_ VARIANT* pvarParam1,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw()
    {
        DISPPARAMS dispparams = { pvarParam1, nullptr, 1, 0};
        return this->p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, nullptr, nullptr);
    }
    // Invoke a method by name with a single parameter
    _Check_return_ HRESULT Invoke1(
        _In_z_ LPCOLESTR lpszName,
        _In_ VARIANT* pvarParam1,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw()
    {
        DISPID dispid;
        HRESULT hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = Invoke1(dispid, pvarParam1, pvarRet);
        return hr;
    }
    // Invoke a method by DISPID with two parameters
    _Check_return_ HRESULT Invoke2(
        _In_ DISPID dispid,
        _In_ VARIANT* pvarParam1,
        _In_ VARIANT* pvarParam2,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw();
    // Invoke a method by name with two parameters
    _Check_return_ HRESULT Invoke2(
        _In_z_ LPCOLESTR lpszName,
        _In_ VARIANT* pvarParam1,
        _In_ VARIANT* pvarParam2,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw()
    {
        DISPID dispid;
        HRESULT hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = Invoke2(dispid, pvarParam1, pvarParam2, pvarRet);
        return hr;
    }
    // Invoke a method by DISPID with N parameters
    _Check_return_ HRESULT InvokeN(
        _In_ DISPID dispid,
        _In_ VARIANT* pvarParams,
        _In_ int nParams,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw()
    {
        DISPPARAMS dispparams = {pvarParams, nullptr, (unsigned int)nParams, 0};
        return this->p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, nullptr, nullptr);
    }
    // Invoke a method by name with Nparameters
    _Check_return_ HRESULT InvokeN(
        _In_z_ LPCOLESTR lpszName,
        _In_ VARIANT* pvarParams,
        _In_ int nParams,
        _Out_opt_ VARIANT* pvarRet = nullptr) throw()
    {
        HRESULT hr;
        DISPID dispid;
        hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = InvokeN(dispid, pvarParams, nParams, pvarRet);
        return hr;
    }

    _Check_return_ static HRESULT PutProperty(
        _In_ IDispatch* pDispatch,
        _In_ DISPID dwDispID,
        _In_ VARIANT* pVar) throw()
    {
        ATLASSERT(pDispatch);
        ATLASSERT(pVar != nullptr);
        if (pVar == nullptr)
            return E_POINTER;

        if (pDispatch == nullptr)
            return E_INVALIDARG;

        ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::PutProperty\n"));
        DISPPARAMS dispparams = {nullptr, nullptr, 1, 1};
        dispparams.rgvarg = pVar;
        DISPID dispidPut = DISPID_PROPERTYPUT;
        dispparams.rgdispidNamedArgs = &dispidPut;

        if (pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH ||
            (pVar->vt & VT_ARRAY) || (pVar->vt & VT_BYREF))
        {
            HRESULT hr = pDispatch->Invoke(dwDispID, IID_NULL,
                LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF,
                &dispparams, nullptr, nullptr, nullptr);
            if (SUCCEEDED(hr))
                return hr;
        }
        return pDispatch->Invoke(dwDispID, IID_NULL,
                LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
                &dispparams, nullptr, nullptr, nullptr);
    }

    _Check_return_ static HRESULT GetProperty(
        _In_ IDispatch* pDispatch,
        _In_ DISPID dwDispID,
        _Out_ VARIANT* pVar) throw()
    {
        ATLASSERT(pDispatch);
        ATLASSERT(pVar != nullptr);
        if (pVar == nullptr)
            return E_POINTER;

        if (pDispatch == nullptr)
            return E_INVALIDARG;

        ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::GetProperty\n"));
        DISPPARAMS dispparamsNoArgs = {nullptr, nullptr, 0, 0};
        return pDispatch->Invoke(dwDispID, IID_NULL,
                LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                &dispparamsNoArgs, pVar, nullptr, nullptr);
    }
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
};

template <class T>
inline bool SK_ComPtrBase<T>::IsEqualObject(_Inout_opt_ IUnknown* pOther) throw()
{
    if (p == nullptr && pOther == nullptr)
        return true;	// They are both NULL objects

    if (p == nullptr || pOther == nullptr)
        return false;	// One is NULL the other is not

    SK_ComPtr<IUnknown> punk1;
    SK_ComPtr<IUnknown> punk2;
    p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
    pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);
    return punk1 == punk2;
}

template <class T, const IID* piid = &__uuidof(T)>
class SK_ComQIPtr :
    public SK_ComPtr<T>
{
public:
    SK_ComQIPtr() throw()
    {
    }
    SK_ComQIPtr(decltype(__nullptr)) throw()
    {
    }
    SK_ComQIPtr(_Inout_opt_ T* lp) throw() :
        SK_ComPtr<T>(lp)
    {
    }
    SK_ComQIPtr(_Inout_ const SK_ComQIPtr<T,piid>& lp) throw() :
        SK_ComPtr<T>(lp.p)
    {
    }
    SK_ComQIPtr(_Inout_opt_ IUnknown* lp) throw()
    {
        if (lp != nullptr)
        {
            if (FAILED(lp->QueryInterface(*piid, (void **)&this->p)))
                this->p = NULL;
        }
    }
    T* operator=(decltype(__nullptr)) throw()
    {
        SK_ComQIPtr(nullptr).Swap(*this);
        return nullptr;
    }
    T* operator=(_Inout_opt_ T* lp) throw()
    {
        if(*this!=lp)
        {
            SK_ComQIPtr(lp).Swap(*this);
        }
        return *this;
    }
    T* operator=(_Inout_ const SK_ComQIPtr<T,piid>& lp) throw()
    {
        if(*this!=lp)
        {
            SK_ComQIPtr(lp).Swap(*this);
        }
        return *this;
    }
    T* operator=(_Inout_opt_ IUnknown* lp) throw()
    {
        if(*this!=lp)
        {
            AtlComQIPtrAssign2((IUnknown**)&this->p, lp, *piid);
        }
        return *this;
    }
};

//Specialization to make it work
template<>
class SK_ComQIPtr<IUnknown, &IID_IUnknown> :
    public SK_ComPtr<IUnknown>
{
public:
    SK_ComQIPtr() throw()
    {
    }
    SK_ComQIPtr(_Inout_opt_ IUnknown* lp) throw()
    {
        //Actually do a QI to get identity
        if (lp != NULL)
        {
            if (FAILED(lp->QueryInterface(__uuidof(IUnknown), (void **)&this->p)))
                this->p = NULL;
        }
    }
    SK_ComQIPtr(_Inout_ const SK_ComQIPtr<IUnknown,&IID_IUnknown>& lp) throw() :
        SK_ComPtr<IUnknown>(lp.p)
    {
    }
    IUnknown* operator=(_Inout_opt_ IUnknown* lp) throw()
    {
        if(*this!=lp)
        {
            //Actually do a QI to get identity
            AtlComQIPtrAssign2((IUnknown**)&this->p, lp, __uuidof(IUnknown));
        }
        return *this;
    }

    IUnknown* operator=(_Inout_ const SK_ComQIPtr<IUnknown,&IID_IUnknown>& lp) throw()
    {
        if(*this!=lp)
        {
            SK_ComQIPtr(lp).Swap(*this);
        }
        return *this;
    }
};

typedef SK_ComQIPtr<IDispatch, &__uuidof(IDispatch)> SK_ComDispatchDriver;

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
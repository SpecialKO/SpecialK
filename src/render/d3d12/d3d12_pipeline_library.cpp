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

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"D12PipeLib"

#include <SpecialK/render/d3d12/d3d12_pipeline_library.h>

D3D12PipelineLibrary_StorePipeline_pfn
D3D12PipelineLibrary_StorePipeline_Original = nullptr;

D3D12PipelineLibrary_LoadGraphicsPipeline_pfn
D3D12PipelineLibrary_LoadGraphicsPipeline_Original = nullptr;

MIDL_INTERFACE("80eabf42-2568-4e5e-bd82-c37f86961dc3")
SK_IWrapD3D12PipelineLibrary : public ID3D12PipelineLibrary
{
public:
  SK_IWrapD3D12PipelineLibrary ( ID3D12Device          *pDevice,
                                 ID3D12PipelineLibrary *pPipelineLibrary ) :
                                                      pReal (pPipelineLibrary),
                                                      pDev  (pDevice),
                                                      ver_  (0)
  {
    if (! pPipelineLibrary)
      return;

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (QueryInterface (IID_ID3D12PipelineLibrary1, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 1;
    }

    if (ver_ != 0)
    {
      SK_LOG0 ( ( L"Promoted ID3D12PipelineLibrary to ID3D12PipelineLibrary%lu", ver_),
                  __SK_SUBSYSTEM__ );
    }

    //SetPrivateDataInterface (IID_IUnwrappedDXGISwapChain, pReal);
  }

  virtual ~SK_IWrapD3D12PipelineLibrary (void)
  {
  }

  SK_IWrapD3D12PipelineLibrary            (const SK_IWrapD3D12PipelineLibrary &) = delete;
  SK_IWrapD3D12PipelineLibrary &operator= (const SK_IWrapD3D12PipelineLibrary &) = delete;

  #pragma region IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppvObj) override; // 0
  virtual ULONG   STDMETHODCALLTYPE AddRef         (void)                       override; // 1
  virtual ULONG   STDMETHODCALLTYPE Release        (void)                       override; // 2
  #pragma endregion


  HRESULT STDMETHODCALLTYPE GetPrivateData          (REFGUID guid, UINT *pDataSize,       void *pData) override { return pReal->GetPrivateData (guid, pDataSize, pData); };
  HRESULT STDMETHODCALLTYPE SetPrivateData          (REFGUID guid, UINT   DataSize, const void* pData) override { return pReal->SetPrivateData (guid, DataSize,  pData); };
  HRESULT STDMETHODCALLTYPE SetPrivateDataInterface (REFGUID guid, const IUnknown *pData)              override { return pReal->SetPrivateDataInterface (guid, pData);   };
  HRESULT STDMETHODCALLTYPE SetName                 (LPCWSTR Name)                                     override { return pReal->SetName                 (Name);          };
  HRESULT STDMETHODCALLTYPE GetDevice               (REFIID  riid, void **ppvDevice)                   override
  {
    SK_ReleaseAssert (pDev.p != nullptr);

    if (pDev.p == nullptr && pReal != nullptr)
    {
      return
        pReal->GetDevice (riid, ppvDevice);
    }

    if (pDev.p != nullptr)
      return
        pDev->QueryInterface (riid, ppvDevice);

    return E_NOINTERFACE;
  }

  HRESULT STDMETHODCALLTYPE StorePipeline (
           LPCWSTR              pName, 
           ID3D12PipelineState* pPipeline ) override
  {
    SK_LOG_FIRST_CALL

    if (pName != nullptr) SK_LOG0 ( ( L"StorePipeline (%ws)", pName ), __SK_SUBSYSTEM__ );

    return pReal->StorePipeline (pName, pPipeline);
  };
  
  HRESULT STDMETHODCALLTYPE LoadGraphicsPipeline (
           LPCWSTR                             pName,
     const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
           REFIID                              riid,
           void                              **ppPipelineState ) override
  {
    SK_LOG_FIRST_CALL
      
    if (pName != nullptr) SK_LOG0 ( ( L"LoadGraphicsPipeline (%ws)", pName ), __SK_SUBSYSTEM__ );

    return pReal->LoadGraphicsPipeline (pName, pDesc, riid, ppPipelineState);
  }
  
  HRESULT STDMETHODCALLTYPE LoadComputePipeline ( 
           LPCWSTR                            pName,
     const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
           REFIID                             riid,
           void                             **ppPipelineState ) override
  {
    return pReal->LoadComputePipeline (pName, pDesc, riid, ppPipelineState);
  }
  
  SIZE_T  STDMETHODCALLTYPE GetSerializedSize (void) override { return pReal->GetSerializedSize (); }
  HRESULT STDMETHODCALLTYPE Serialize         (
           void *pData,
           SIZE_T DataSizeInBytes )                  override { return pReal->Serialize (pData, DataSizeInBytes); };

  HRESULT STDMETHODCALLTYPE LoadPipeline (LPCWSTR                           pName,
                                    const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
                                          REFIID                            riid,
                                          void                            **ppPipelineState)
  {
    return static_cast <ID3D12PipelineLibrary1 *> (pReal)->LoadPipeline (pName, pDesc, riid, ppPipelineState);
  }

  volatile LONG          refs_ = 1;
  ID3D12PipelineLibrary *pReal;
  SK_ComPtr <IUnknown>   pDev;
  unsigned int           ver_;
};

// IDXGISwapChain
HRESULT
STDMETHODCALLTYPE
SK_IWrapD3D12PipelineLibrary::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
  }

  if (
    riid == __uuidof (IUnknown)              ||
    riid == __uuidof (ID3D12Object)          ||
    riid == __uuidof (ID3D12DeviceChild)     ||
    riid == __uuidof (ID3D12PipelineLibrary) ||
    riid == __uuidof (ID3D12PipelineLibrary1) )
  {
    auto _GetVersion = [](REFIID riid) ->
    UINT
    {
      if (riid == __uuidof (ID3D12PipelineLibrary))  return 0;
      if (riid == __uuidof (ID3D12PipelineLibrary1)) return 1;

      return 0;
    };

    UINT required_ver =
      _GetVersion (riid);

    if (ver_ < required_ver)
    {
      IUnknown* pPromoted = nullptr;

      if ( FAILED (
             pReal->QueryInterface ( riid,
                           (void **)&pPromoted )
                  ) || pPromoted == nullptr
         )
      {
        return E_NOINTERFACE;
      }

      ver_ =
        SK_COM_PromoteInterface (&pReal, pPromoted) ?
                                       required_ver : ver_;
    }

    else
    {
      AddRef ();
    }

    *ppvObj = this;

    return S_OK;
  }

  HRESULT hr =
    pReal->QueryInterface (riid, ppvObj);

  if ( riid != IID_IUnknown &&
       riid != IID_ID3DUserDefinedAnnotation )
  {
    static
      concurrency::concurrent_unordered_set <std::wstring> reported_guids;

    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    bool once =
      reported_guids.count (wszGUID) > 0;

    if (! once)
    {
      reported_guids.insert (wszGUID);

      SK_LOG0 ( ( L"QueryInterface on wrapped D3D12 PipelineLibrary for Mystery UUID: %s",
                      wszGUID ), L"   DXGI   " );
    }
  }

  return hr;
}

ULONG
STDMETHODCALLTYPE
SK_IWrapD3D12PipelineLibrary::AddRef (void)
{
  InterlockedIncrement (&refs_);

  return
    pReal->AddRef ();
}

ULONG
STDMETHODCALLTYPE
SK_IWrapD3D12PipelineLibrary::Release (void)
{
  ULONG xrefs =
    InterlockedDecrement (&refs_);

  std::ignore = xrefs;

  ULONG refs =
    pReal->Release ();

  if (refs == 0)
  {
    SK_LOG0 ( ( L"(-) Releasing wrapped PipelineLibrary (%08ph)... device=%08ph", pReal, pDev.p),
             __SK_SUBSYSTEM__);

    pDev.Release ();

    //delete this;
  }

  return refs;
}

HRESULT
STDMETHODCALLTYPE
D3D12PipelineLibrary_LoadGraphicsPipeline_Detour (
              ID3D12PipelineLibrary              *This,
_In_          LPCWSTR                             pName,
_In_    const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
              REFIID                              riid,
_COM_Outptr_  void                              **ppPipelineState )
{
  SK_LOG_FIRST_CALL

  if (ppPipelineState == nullptr)
    return E_INVALIDARG;

  *ppPipelineState = nullptr;

  SK_LOG0 ( ( L"LoadGraphicsPipeline (%ws)", pName ), __SK_SUBSYSTEM__ );

  return
    D3D12PipelineLibrary_LoadGraphicsPipeline_Original (This, pName, pDesc, riid, ppPipelineState);
}

HRESULT
STDMETHODCALLTYPE
D3D12PipelineLibrary_StorePipeline_Detour (
         ID3D12PipelineLibrary *This,
_In_opt_ LPCWSTR                pName,
_In_     ID3D12PipelineState   *pPipeline )
{
  SK_LOG_FIRST_CALL

  SK_LOG0 ( ( L"StorePipeline (%ws)", pName ), __SK_SUBSYSTEM__ );

  return
    D3D12PipelineLibrary_StorePipeline_Original (This, pName, pPipeline);
}

D3D12Device1_CreatePipelineLibrary_pfn
D3D12Device1_CreatePipelineLibrary_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D12Device1_CreatePipelineLibrary_Detour (
        ID3D12Device1 *This,
  const void   *pLibraryBlob,
        SIZE_T          BlobLength,
        REFIID          riid,
        void       **ppPipelineLibrary )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    D3D12Device1_CreatePipelineLibrary_Original (
                        This, pLibraryBlob,
                                      BlobLength, riid,
                      ppPipelineLibrary );

  if (FAILED (hr))
  {
    BlobLength = 0;

    SK_LOG0 ( ( L"CreatePipelineLibrary failed with HRESULT=%x", hr ),
                __SK_SUBSYSTEM__ );

    if ( hr == D3D12_ERROR_ADAPTER_NOT_FOUND ||
         hr == D3D12_ERROR_DRIVER_VERSION_MISMATCH ||
         hr == E_INVALIDARG )
    {
      SK_LOG0 ( ( L" * Recovering from Invalid Cache => initializing empty..." ),
                  __SK_SUBSYSTEM__ );

      hr =
        D3D12Device1_CreatePipelineLibrary_Original (
                            This, pLibraryBlob,
                                             0, riid,
                          ppPipelineLibrary );

      return hr;
    }
  }

  // This stuff doesn't work anyway, seems D3D12 changes the vftable
#if 0
  if (SUCCEEDED (hr) && ppPipelineLibrary != nullptr && *ppPipelineLibrary != nullptr)
  {
    SK_RunOnce (
      SK_CreateVFTableHook  ( L"ID3D12PipelineLibrary::StorePipeline",
                        *(void ***)*ppPipelineLibrary, 8,
                                 D3D12PipelineLibrary_StorePipeline_Detour,
                       (void **)&D3D12PipelineLibrary_StorePipeline_Original )
    );
    SK_RunOnce (
      SK_CreateVFTableHook  ( L"ID3D12PipelineLibrary::LoadGraphicsPipeline",
                        *(void ***)*ppPipelineLibrary, 9,
                                 D3D12PipelineLibrary_LoadGraphicsPipeline_Detour,
                       (void **)&D3D12PipelineLibrary_LoadGraphicsPipeline_Original )
    );
  }
#endif
  
  return hr;
}

void
SK_D3D12_HookPipelineLibrary (ID3D12Device1* pDevice1)
{
  return;

  if (pDevice1 == nullptr)
    return;

  // ID3D12Device1
  //---------------
  // 44 CreatePipelineLibrary
  // 45 SetEventOnMultipleFenceCompletion
  // 46 SetResidencyPriority

  SK_CreateVFTableHook2 ( L"ID3D12Device1::CreatePipelineLibrary",
                         *(void ***)*(&pDevice1), 44,
                          D3D12Device1_CreatePipelineLibrary_Detour,
                (void **)&D3D12Device1_CreatePipelineLibrary_Original );

  //  3 GetPrivateData
  //  4 SetPrivateData
  //  5 SetPrivateDataInterface
  //  6 SetName
  //  7 GetDevice
  //  8 StorePipeline
  //  9 LoadGraphicsPipeline
  // 10 LoadComputePipeline
  // 11 GetSerializedSize
  // 12 Serialize
}
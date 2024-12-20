// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#define __SK_SUBSYSTEM__ L" DStorage "

#include <SpecialK/render/dstorage/dstorage_queue.h>

extern bool SK_DStorage_UsingGDeflate;

bool
SK_DStorage_IsWrappableQueueType (REFIID riid)
{
  return
    ( riid == __uuidof (IDStorageQueue)  ||
      riid == __uuidof (IDStorageQueue1) ||
      riid == __uuidof (IDStorageQueue2) );
}


HRESULT
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
  if (ppvObject == nullptr)
  {
    return E_POINTER;
  }

  if ( riid == __uuidof (IUnknown)        ||
       riid == __uuidof (IDStorageQueue)  ||
       riid == __uuidof (IDStorageQueue1) ||
       riid == __uuidof (IDStorageQueue2) )
  {
    auto _GetVersion = [](REFIID riid) ->
    UINT
    {
      if (riid == __uuidof (IDStorageQueue))  return 0;
      if (riid == __uuidof (IDStorageQueue1)) return 1;
      if (riid == __uuidof (IDStorageQueue2)) return 2;

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

    *ppvObject = this;

    return S_OK;
  }

  HRESULT hr =
    pReal->QueryInterface (riid, ppvObject);

  static
    concurrency::concurrent_unordered_set <std::wstring> reported_guids;

  wchar_t                wszGUID [41] = { };
  StringFromGUID2 (riid, wszGUID, 40);

  bool once =
    reported_guids.count (wszGUID) > 0;

  if (! once)
  {
    reported_guids.insert (wszGUID);

    SK_LOG0 ( ( L"QueryInterface on wrapped DStorage Queue for Mystery UUID: %s",
                    wszGUID ), L" DStorage " );
  }

  return hr;
}

ULONG
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::AddRef (void)
{
  InterlockedIncrement (&refs_);

  return
    pReal->AddRef ();
}
  
ULONG
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::Release (void)
{
  ULONG xrefs =
    InterlockedDecrement (&refs_),
         refs = pReal->Release ();

  if (xrefs == 0)
  {
    if (refs != 0)
    {
      SK_LOG0 ( ( L"Inconsistent reference count for IDStorageQueue; expected=%d, actual=%d",
                  xrefs, refs ),
                  L" DStorage " );
    }

    else
      delete this;
  }

  return
    refs;
}


void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::EnqueueRequest (const DSTORAGE_REQUEST *request)
{
  if (request == nullptr)
    return;

  if (request->Options.CompressionFormat == DSTORAGE_COMPRESSION_FORMAT_GDEFLATE)
  {
    SK_DStorage_UsingGDeflate = true;
  }

  return
    pReal->EnqueueRequest (request);
}

void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::EnqueueStatus (IDStorageStatusArray *statusArray, UINT32 index)
{
  return
    pReal->EnqueueStatus (statusArray, index);
}

void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::EnqueueSignal (ID3D12Fence *fence, UINT64 value)
{
  return
    pReal->EnqueueSignal (fence, value);
}

void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::Submit (void)
{
  return
    pReal->Submit ();
}

void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::CancelRequestsWithTag (UINT64 mask, UINT64 value)
{
  return
    pReal->CancelRequestsWithTag (mask, value);
}

void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::Close (void)
{
  return
    pReal->Close ();
}

HANDLE
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::GetErrorEvent (void)
{
  return
    pReal->GetErrorEvent ();
}

void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::RetrieveErrorRecord (_Out_ DSTORAGE_ERROR_RECORD *record)
{
  return
    pReal->RetrieveErrorRecord (record);
}

void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::Query (_Out_ DSTORAGE_QUEUE_INFO *info)
{
  return
    pReal->Query (info);
}



void
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::EnqueueSetEvent (HANDLE handle)
{
  SK_ReleaseAssert (ver_ >= 1);

  return
    static_cast <IDStorageQueue1 *> (pReal)->EnqueueSetEvent (handle);
}



DSTORAGE_COMPRESSION_SUPPORT
STDMETHODCALLTYPE
SK_IWrapDStorageQueue::GetCompressionSupport (DSTORAGE_COMPRESSION_FORMAT format)
{
  SK_ReleaseAssert (ver_ >= 2);

  auto support =
    static_cast <IDStorageQueue2 *> (pReal)->GetCompressionSupport (format);

  if (config.render.dstorage.disable_gpu_decomp)
  {
    support &=
      ~( DSTORAGE_COMPRESSION_SUPPORT_GPU_FALLBACK |
         DSTORAGE_COMPRESSION_SUPPORT_GPU_OPTIMIZED );

    support |= DSTORAGE_COMPRESSION_SUPPORT_CPU_FALLBACK;
  }
  
  return support;
}
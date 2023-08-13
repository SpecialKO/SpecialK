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

#ifndef __SK__DSTORAGE_QUEUE_H__
#define __SK__DSTORAGE_QUEUE_H__

#include <SpecialK/stdafx.h>

#include <SpecialK/render/dstorage/dstorage.h>
#include <SpecialK/render/dstorage/dstorage_queue.h>

bool SK_DStorage_IsWrappableQueueType (REFIID riid);

class SK_IWrapDStorageQueue : IDStorageQueue2
{
public:
  SK_IWrapDStorageQueue (IDStorageQueue *pQueue) : pReal (pQueue),
                                                   ver_ (0)
  {
    if (pQueue == nullptr)
      return;

    IUnknown *pPromotion = nullptr;

    if (     SUCCEEDED (QueryInterface (__uuidof (IDStorageQueue2), reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 2;
    }

    else if (SUCCEEDED (QueryInterface (__uuidof (IDStorageQueue1), reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 1;
    }

    if (ver_ != 0)
    {
      SK_LOG0 ( ( L"Promoted IDstorageQueue to IDStorageQueue%lu", ver_),
                  L" DStorage " );
    }
  }

#pragma region IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface        (REFIID riid, void **ppvObject) override;
  virtual ULONG   STDMETHODCALLTYPE AddRef                (void)                          override;
  virtual ULONG   STDMETHODCALLTYPE Release               (void)                          override;
#pragma endregion

#pragma region IDStorageQueue
  virtual void    STDMETHODCALLTYPE EnqueueRequest        (const DSTORAGE_REQUEST *request)                 override;
  virtual void    STDMETHODCALLTYPE EnqueueStatus         (IDStorageStatusArray *statusArray, UINT32 index) override;
  virtual void    STDMETHODCALLTYPE EnqueueSignal         (ID3D12Fence *fence, UINT64 value)                override;
  virtual void    STDMETHODCALLTYPE Submit                (void)                                            override;
  virtual void    STDMETHODCALLTYPE CancelRequestsWithTag (UINT64 mask, UINT64 value)                       override;
  virtual void    STDMETHODCALLTYPE Close                 (void)                                            override;
  virtual HANDLE  STDMETHODCALLTYPE GetErrorEvent         (void)                                            override;
  virtual void    STDMETHODCALLTYPE RetrieveErrorRecord   (_Out_ DSTORAGE_ERROR_RECORD *record)             override;
  virtual void    STDMETHODCALLTYPE Query                 (_Out_ DSTORAGE_QUEUE_INFO *info)                 override;
#pragma endregion

#pragma region IDStorageQueue1
  virtual void    STDMETHODCALLTYPE EnqueueSetEvent       (HANDLE handle) override;
#pragma endregion

#pragma region IDStorageQueue2
  virtual DSTORAGE_COMPRESSION_SUPPORT
                  STDMETHODCALLTYPE GetCompressionSupport (DSTORAGE_COMPRESSION_FORMAT format) override;
#pragma endregion

private:
  volatile LONG   refs_ = 1;
  IDStorageQueue *pReal;
  unsigned int    ver_  = 0;
};

#endif /* __SK__DSTORAGE_QUEUE_H__ */
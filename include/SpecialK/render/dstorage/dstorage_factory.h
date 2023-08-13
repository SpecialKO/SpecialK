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

#ifndef __SK__DSTORAGE_FACTORY_H__
#define __SK__DSTORAGE_FACTORY_H__

#include <SpecialK/render/dstorage/dstorage.h>

extern DSTORAGE_COMPRESSION_SUPPORT SK_DStorage_GDeflateSupport;
//
// Compression support is technically per-queue, but for the time being they all have
//   the same capabilities.
// 
// * This may change if different queue types are added...
//

class SK_IWrapDStorageFactory : IDStorageFactory
{
public:
  SK_IWrapDStorageFactory (IDStorageFactory *pFactory)
  {
    pReal = pFactory;
  }

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
  {
    return
      pReal->QueryInterface (riid, ppvObject);
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void) override
  {
    return
      pReal->AddRef ();
  }
  
  virtual ULONG STDMETHODCALLTYPE Release (void) override
  {
    return
      pReal->Release ();
  }

  virtual HRESULT STDMETHODCALLTYPE CreateQueue (const DSTORAGE_QUEUE_DESC *desc, REFIID riid, _COM_Outptr_ void **ppv) override;

  virtual HRESULT STDMETHODCALLTYPE OpenFile (_In_z_ const WCHAR *path, REFIID riid, _COM_Outptr_ void **ppv) override
  {
    return
      pReal->OpenFile (path, riid, ppv);
  }

  virtual HRESULT STDMETHODCALLTYPE CreateStatusArray (UINT32 capacity, _In_opt_ PCSTR name, REFIID riid, _COM_Outptr_ void **ppv) override
  {
    return
      pReal->CreateStatusArray (capacity, name, riid, ppv);
  }

  virtual void STDMETHODCALLTYPE SetDebugFlags (UINT32 flags) override
  {
    return
      pReal->SetDebugFlags (flags);
  }
  
  virtual HRESULT STDMETHODCALLTYPE SetStagingBufferSize (UINT32 size) override
  {
    return
      pReal->SetStagingBufferSize (size);
  }

private:
  IDStorageFactory *pReal = nullptr;
};

#endif /* __SK__DSTORAGE_FACTORY_H__ */
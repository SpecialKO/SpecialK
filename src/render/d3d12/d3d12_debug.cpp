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
#define __SK_SUBSYSTEM__ L"D3D12Debug"

#define D3D12_IGNORE_SDK_LAYERS
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d12/d3d12_interfaces.h>
template <typename _T>
HRESULT
SK_D3D12_GET_OBJECT_NAME_N ( ID3D12Object *pObject,
                             UINT         *pBytes,
                             _T           *pName = nullptr );

template <>
HRESULT
SK_D3D12_GET_OBJECT_NAME_N ( ID3D12Object *pObject,
                             UINT         *pBytes,
                             char         *pName )
{
  if ( (! pObject) ||
       (! pBytes ) )
    return E_POINTER;

  return
    pObject->GetPrivateData (
      WKPDID_D3DDebugObjectName,
        pBytes,
          pName );
}

template <>
HRESULT
SK_D3D12_GET_OBJECT_NAME_N ( ID3D12Object *pObject,
                             UINT         *pBytes,
                             wchar_t      *pName )
{
  if ( (! pObject) ||
       (! pBytes ) )
    return E_POINTER;

  return
    pObject->GetPrivateData (
      WKPDID_D3DDebugObjectNameW,
        pBytes,
          pName );
}

bool
SK_D3D12_HasDebugName (ID3D12Object* pD3D12Obj)
{
  if (pD3D12Obj == nullptr)
    return false;

  UINT uiNameLen = 0;

  if ( FAILED ( pD3D12Obj->GetPrivateData (WKPDID_D3DDebugObjectNameW, &uiNameLen, nullptr) ) &&
       FAILED ( pD3D12Obj->GetPrivateData (WKPDID_D3DDebugObjectName,  &uiNameLen, nullptr) ) )
    return false;

  return
    ( uiNameLen > 0 );
}

template <typename _T>
std::basic_string <_T>
SK_D3D12_GetDebugName (ID3D12Object* pD3D12Obj)
{
  if (pD3D12Obj != nullptr)
  {
    UINT bufferLen = 0;

    if ( SUCCEEDED (
           SK_D3D12_GET_OBJECT_NAME_N <_T> ( pD3D12Obj,
                   &bufferLen, nullptr )
         )
       )
    {
      if (bufferLen >= sizeof (_T))
      {
        std::basic_string <_T>
          name (
            bufferLen / sizeof (_T),
                               (_T)0 );

        if ( SUCCEEDED (
               SK_D3D12_GET_OBJECT_NAME_N <_T> ( pD3D12Obj,
                       &bufferLen, name.data () )
             )
           )
        {
          return name;
        }
      }
    }
  }

  return
    std::basic_string <_T> (0);
}

void
SK_D3D12_SetDebugName (       ID3D12Object* pD3D12Obj,
                        const std::wstring&     kName )
{
  if (pD3D12Obj != nullptr && kName.size () > 0)
  {
#if 1
    D3D_SET_OBJECT_NAME_N_W ( pD3D12Obj,
                   static_cast <UINT> ( kName.size () ),
                                        kName.data ()
                            );
    std::string utf8_copy =
      SK_WideCharToUTF8 (kName);

    D3D_SET_OBJECT_NAME_N_A ( pD3D12Obj,
                   static_cast <UINT> ( (utf8_copy).size () ),
                                        (utf8_copy).data ()
                            );

    SK_LOGi1 (
      L"Created D3D12 Object: %ws", kName.c_str ()
    );
#else
    pD3D12Obj->SetName ( kName.c_str () );
#endif
  }
}
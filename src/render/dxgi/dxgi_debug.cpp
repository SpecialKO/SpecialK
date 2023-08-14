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
#define __SK_SUBSYSTEM__ L"DXGI Debug"
                         
#include <SpecialK/render/dxgi/dxgi_interfaces.h>

void
SK_DXGI_SetDebugName ( IDXGIObject* pDXGIObject,
           const std::wstring&      kName )
{
  if (! pDXGIObject)
    return;

  SK_LOGi2 (
    L"Created DXGI Object: %ws", kName.c_str ()
  );

  //
  // D3D has separate Private Data GUIDs for Wide and ANSI names,
  //   and various parts of the debug layer use one or the other.
  //
  //  * D3D11 mostly uses ANSI, but it is best to just store
  //      all possible text encodings and cover all bases.
  //

  D3D_SET_OBJECT_NAME_N_A (pDXGIObject, 0, nullptr);
  D3D_SET_OBJECT_NAME_N_W (pDXGIObject, 0, nullptr);

  D3D_SET_OBJECT_NAME_N_W ( pDXGIObject,
                   static_cast <UINT> ( kName.length () ),
                                        kName.data   ()
                          );

  std::string utf8_copy =
    SK_WideCharToUTF8 (kName);

  D3D_SET_OBJECT_NAME_N_A ( pDXGIObject,
                 static_cast <UINT> ( (utf8_copy).length () ),
                                      (utf8_copy).data   ()
                          );
}

template <typename _T>
HRESULT
SK_DXGI_GET_OBJECT_NAME_N ( IDXGIObject *pObject,
                            UINT        *pBytes,
                            _T          *pName = nullptr );

template <>
HRESULT
SK_DXGI_GET_OBJECT_NAME_N ( IDXGIObject *pObject,
                            UINT        *pBytes,
                            char        *pName )
{
  if ( (! pObject) &&
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
SK_DXGI_GET_OBJECT_NAME_N ( IDXGIObject *pObject,
                            UINT        *pBytes,
                            wchar_t     *pName )
{
  if ( (! pObject) &&
       (! pBytes ) )
    return E_POINTER;

  return
    pObject->GetPrivateData (
      WKPDID_D3DDebugObjectNameW,
        pBytes,
          pName );
}

bool
SK_DXGI_HasDebugName (IDXGIObject* pDXGIObj)
{
  UINT uiNameLen = 0;

  HRESULT hr =
    pDXGIObj->GetPrivateData (WKPDID_D3DDebugObjectNameW, &uiNameLen, nullptr);

  if (FAILED (hr) && hr != DXGI_ERROR_MORE_DATA)
  {
    uiNameLen = 0;

    hr =
      pDXGIObj->GetPrivateData (WKPDID_D3DDebugObjectName, &uiNameLen, nullptr);

    if (FAILED (hr) && hr != DXGI_ERROR_MORE_DATA)
      return false;
  }

  return
    ( uiNameLen > 0 );
}

template <typename _T>
std::basic_string <_T>
SK_DXGI_GetDebugName (IDXGIObject* pDXGIObj)
{
  if (pDXGIObj != nullptr)
  {
    UINT bufferLen = 0;

    HRESULT hr =
      SK_DXGI_GET_OBJECT_NAME_N <_T> ( pDXGIObj,
                 &bufferLen, nullptr );

    if (SUCCEEDED (hr) || bufferLen > 0)
    {
      if (bufferLen >= sizeof (_T))
      {
        std::basic_string <_T>
          name (
            bufferLen / sizeof (_T),
                               (_T)0 );

        if ( SUCCEEDED (
               SK_DXGI_GET_OBJECT_NAME_N <_T> ( pDXGIObj,
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
    std::basic_string <_T> (reinterpret_cast <_T *> (L""));
}

std::wstring
SK_DXGI_GetDebugNameW (IDXGIObject* pDXGIObj)
{
  return
    SK_DXGI_GetDebugName <wchar_t> (pDXGIObj);
}

std::string
SK_DXGI_GetDebugNameA (IDXGIObject* pDXGIObj)
{
  return
    SK_DXGI_GetDebugName <char> (pDXGIObj);
}

std::string
SK_DXGI_GetDebugNameUTF8 (IDXGIObject* pDXGIObj)
{
  auto wide_name =
    SK_DXGI_GetDebugName <wchar_t> (pDXGIObj);

  if (wide_name.empty ())
  {
    auto name =
      SK_DXGI_GetDebugName <char> (pDXGIObj);

    if (! name.empty ())
      return name;

    return "Unnamed";
  }

  return
    SK_WideCharToUTF8 (wide_name);
}
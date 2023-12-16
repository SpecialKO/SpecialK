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
#define __SK_SUBSYSTEM__ L"D3D11Debug"

void
SK_D3D11_SetDebugName (       ID3D11DeviceChild* pDevChild,
                        const std::wstring&      kName )
{
  if (! pDevChild)
    return;

  static int
      __SK_D3D11_DebugLayerActive  = -1;
  if (__SK_D3D11_DebugLayerActive == -1)
  {
    SK_ComPtr <ID3D11Device> pDev;
    pDevChild->GetDevice (  &pDev.p );

    SK_ComQIPtr <ID3D11Debug> pDebug (pDev);
    __SK_D3D11_DebugLayerActive =
                              pDebug.p != nullptr ?
                                                1 : 0;
  }

  // Store object names only if we are logging stuff or if the
  //   D3D11 debug layer is active
  if (kName.empty () /*|| ( __SK_D3D11_DebugLayerActive == 0 &&
                                 config.system.log_level < 2 )*/)
  { // Ignore the comment above,
    return; // debug names are useful in SK's built-in debugger :)
  }

  SK_LOGi2 (
    L"Created D3D11 Object: %ws", kName.c_str ()
  );

  //
  // D3D has separate Private Data GUIDs for Wide and ANSI names,
  //   and various parts of the debug layer use one or the other.
  //
  //  * D3D11 mostly uses ANSI, but it is best to just store
  //      all possible text encodings and cover all bases.
  //

  D3D_SET_OBJECT_NAME_N_A (pDevChild, 0, nullptr);
  D3D_SET_OBJECT_NAME_N_W (pDevChild, 0, nullptr);

  D3D_SET_OBJECT_NAME_N_W ( pDevChild,
                   static_cast <UINT> ( kName.length () ),
                                        kName.data   ()
                          );

  std::string utf8_copy =
    SK_WideCharToUTF8 (kName);

  D3D_SET_OBJECT_NAME_N_A ( pDevChild,
                 static_cast <UINT> ( (utf8_copy).length () ),
                                      (utf8_copy).data   ()
                          );
}

template <typename _T>
HRESULT
SK_D3D11_GET_OBJECT_NAME_N ( ID3D11DeviceChild *pObject,
                             UINT              *pBytes,
                             _T                *pName = nullptr );

template <>
HRESULT
SK_D3D11_GET_OBJECT_NAME_N ( ID3D11DeviceChild *pObject,
                             UINT              *pBytes,
                             char              *pName )
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
SK_D3D11_GET_OBJECT_NAME_N ( ID3D11DeviceChild *pObject,
                             UINT              *pBytes,
                             wchar_t           *pName )
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
SK_D3D11_HasDebugName (ID3D11DeviceChild* pD3D11Obj)
{
  if (pD3D11Obj == nullptr)
    return false;

  UINT uiNameLen = 0;

  HRESULT hr =
    pD3D11Obj->GetPrivateData (WKPDID_D3DDebugObjectNameW, &uiNameLen, nullptr);

  if (FAILED (hr) && hr != DXGI_ERROR_MORE_DATA)
  {
    uiNameLen = 0;

    hr =
      pD3D11Obj->GetPrivateData (WKPDID_D3DDebugObjectName, &uiNameLen, nullptr);

    if (FAILED (hr) && hr != DXGI_ERROR_MORE_DATA)
      return false;
  }

  return
    ( uiNameLen > 0 );
}

template <typename _T>
std::basic_string <_T>
SK_D3D11_GetDebugName (ID3D11DeviceChild* pD3D11Obj)
{
  if (pD3D11Obj != nullptr)
  {
    UINT bufferLen = 0;

    HRESULT hr =
      SK_D3D11_GET_OBJECT_NAME_N <_T> ( pD3D11Obj,
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
               SK_D3D11_GET_OBJECT_NAME_N <_T> ( pD3D11Obj,
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
SK_D3D11_GetDebugNameW (ID3D11DeviceChild* pD3D11Obj)
{
  return
    SK_D3D11_GetDebugName <wchar_t> (pD3D11Obj);
}

std::string
SK_D3D11_GetDebugNameA (ID3D11DeviceChild* pD3D11Obj)
{
  return
    SK_D3D11_GetDebugName <char> (pD3D11Obj);
}

std::string
SK_D3D11_GetDebugNameUTF8 (ID3D11DeviceChild* pD3D11Obj)
{
  auto wide_name =
    SK_D3D11_GetDebugName <wchar_t> (pD3D11Obj);

  if (wide_name.empty ())
  {
    auto name =
      SK_D3D11_GetDebugName <char> (pD3D11Obj);

    if (! name.empty ())
      return name;

    return "Unnamed";
  }

  return
    SK_WideCharToUTF8 (wide_name);
}
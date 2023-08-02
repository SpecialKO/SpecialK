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

#pragma comment (lib,    "comctl32.lib")
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HRESULT WINAPI
SK_TaskDialogIndirect ( _In_      const TASKDIALOGCONFIG* pTaskConfig,
                        _Out_opt_       int*              pnButton,
                        _Out_opt_       int*              pnRadioButton,
                        _Out_opt_       BOOL*             pfVerificationFlagChecked )
{
  InitMUILanguage (LANG_USER_DEFAULT);

  using TaskDialogIndirect_pfn   = HRESULT (WINAPI*)(_In_ const TASKDIALOGCONFIG* pTaskConfig, _Out_opt_ int* pnButton, _Out_opt_ int* pnRadioButton, _Out_opt_ BOOL* pfVerificationFlagChecked);
  using InitCommonControlsEx_pfn = BOOL    (WINAPI*)(_In_ const INITCOMMONCONTROLSEX * picce);

  // SK links to the Import Library (Side-by-Side Assembly: 6.0),
  //   so just grab the module handle from our Activation Context.
  static HMODULE hModComctl32_Isolated =
    GetModuleHandleW (L"comctl32.dll");

  static auto InitCommonControlsEx_SanityCheck =
             (InitCommonControlsEx_pfn)
    SK_GetProcAddress ( hModComctl32_Isolated,
             "InitCommonControlsEx" );

  // ^^^ InitCommonControlsEx will be exported _by name_ if the SxS load
  //       was successful.

  static auto SXS_TaskDialogIndirect =
                 (TaskDialogIndirect_pfn)nullptr;

  if ( InitCommonControlsEx_SanityCheck != nullptr &&
       SXS_TaskDialogIndirect           == nullptr )
  {
    // Common Control Sex (Protected)
    static INITCOMMONCONTROLSEX ccsex = {
      sizeof (INITCOMMONCONTROLSEX),
      0x7ff
    };

    if (InitCommonControlsEx (&ccsex) != FALSE)
    {
#define COMCTL32_TASKDIALOGINDIRECT_ORDINAL (const char *)345

      // TaskDialogIndirect is exported by ordinal only
                         SXS_TaskDialogIndirect =
                            (TaskDialogIndirect_pfn)
      SK_GetProcAddress ( hModComctl32_Isolated,
                            COMCTL32_TASKDIALOGINDIRECT_ORDINAL );
    }
  }

  if (SXS_TaskDialogIndirect != nullptr)
  {
    return
      SXS_TaskDialogIndirect ( pTaskConfig, pnButton,
                                 pnRadioButton, pfVerificationFlagChecked );
  }

  SK_LOG0 ( ( L" (!) SxS Load of Comctl32.dll v 6.0.x Failed" ),
              L"DllIsolate" );

  return
    E_NOTIMPL;
}
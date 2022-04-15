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
#ifndef __SK__Plugin__Manager_H__
#define __SK__Plugin__Manager_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <string>

extern bool isArkhamKnight;
extern bool isTalesOfZestiria;
extern bool isFallout4;
extern bool isNieRAutomata;
extern bool isDarkSouls3;
extern bool isDivinityOrigSin;

void
__stdcall
SK_SetPluginName (std::wstring name);

void
__stdcall
SKX_SetPluginName (const wchar_t* wszName);

std::wstring&
__stdcall
SK_GetPluginName (void);

bool
__stdcall
SK_HasPlugin (void);


using SK_EndFrame_pfn                  = void   (__stdcall *)( void        );
using SK_EndFrameEx_pfn                = void   (__stdcall *)( BOOL
                                                               bWaitOnFail );
using SK_BeginFrame_pfn                = void   (__stdcall *)( void        );
using SK_ReleaseGfx_pfn                = void   (__stdcall *)( void        );
using SK_PlugIn_ControlPanelWidget_pfn = void   (__stdcall *)( void        );
using SK_PlugIn_KeyPress_pfn           = void   (__stdcall *)( BOOL Control,
                                                               BOOL Shift,
                                                               BOOL Alt,
                                                               BYTE vkCode );

using SK_PlugIn_PresentFirstFrame_pfn = HRESULT (__stdcall *)( IUnknown*,
                                                               UINT, UINT  );
using SK_PlugIn_Init_pfn              = void    (          *)( void        );
using SK_PlugIn_ControlPanelCfg_pfn   = bool    (          *)( void        );


struct SK_PluginRegistry
{
  //Stupid Hack, rewrite me... (IN PROGRESS - see isPlugin below)
  bool isArkhamKnight    = false;
  bool isTalesOfZestiria = false;
  bool isFallout4        = false;
  bool isNieRAutomata    = false;
  bool isDarkSouls3      = false;
  bool isDivinityOrigSin = false;

  bool isPlugin          = false;

  std::set <SK_PlugIn_Init_pfn>                     init_fns;
  std::set <SK_PlugIn_ControlPanelCfg_pfn>        config_fns;
  std::set <SK_PlugIn_PresentFirstFrame_pfn> first_frame_fns;
  std::set <SK_EndFrame_pfn>                   end_frame_fns;
  std::set <SK_BeginFrame_pfn>               begin_frame_fns;
  std::set <SK_ReleaseGfx_pfn>               release_gfx_fns;

  std::wstring plugin_name;
};
extern SK_LazyGlobal <SK_PluginRegistry> plugin_mgr;

#include <Windows.h>
#include <render/dxgi/dxgi_backend.h>


// As a general rule, plug-ins are only _built-in_ to the 64-bit DLL
#ifdef _WIN64
void
SK_DGPU_InitPlugin (void);

void
SK_IT_InitPlugin (void);

void
SK_NNK2_InitPlugin (void);

void
SK_TVFix_InitPlugin (void);

bool SK_FO4_UseFlipMode        (void);
bool SK_FO4_IsFullscreen       (void);
bool SK_FO4_IsBorderlessWindow (void);

HRESULT __stdcall
     SK_FO4_PresentFirstFrame  (IUnknown *, UINT, UINT);


// TODO: Get this stuff out of here, it's breaking what little design work there is.
void SK_DS3_InitPlugin         (void);
bool SK_DS3_UseFlipMode        (void);
bool SK_DS3_IsBorderless       (void);

HRESULT __stdcall
     SK_DS3_PresentFirstFrame  (IUnknown *, UINT, UINT);

HRESULT __stdcall
     SK_FAR_PresentFirstFrame  (IUnknown *, UINT, UINT);

HRESULT __stdcall
     SK_IT_PresentFirstFrame   (IUnknown *, UINT, UINT);

HRESULT __stdcall
     SK_DGPU_PresentFirstFrame (IUnknown *, UINT, UINT);

HRESULT __stdcall
     SK_TVFIX_PresentFirstFrame (IUnknown *, UINT, UINT);

HRESULT __stdcall
     SK_Sekiro_PresentFirstFrame (IUnknown *, UINT, UINT);

HRESULT __stdcall
     SK_OPT_PresentFirstFrame    (IUnknown *, UINT, UINT);

void SK_Yakuza0_PlugInInit (void);
bool SK_Yakuza0_PlugInCfg  (void);

void SK_Perona4_InitPlugin (void);
void SK_YS8_InitPlugin     (void);
void SK_ER_InitPlugin      (void);
void SK_ELEX2_InitPlugin   (void);

#else
HRESULT __stdcall
     SK_SOM_PresentFirstFrame (IDXGISwapChain *, UINT, UINT);

void
SK_SOM_InitPlugin (void);

void
SK_Persona4_DrawHandler ( ID3D11DeviceContext* pDevCtx,
                          uint32_t             current_vs,
                          uint32_t             current_ps );
void
SK_Persona4_EndFrame    ( SK_TLS* pTLS );

void
SK_Persona4_InitPlugin  ( void );
void SK_CC_InitPlugin   ( void );
#endif



enum class SK_PlugIn_Type
{
  ThirdParty  = 1,
  Custom      = 2,
  Unspecified = 0x0fffffff
};

const wchar_t*
SK_GetPlugInDirectory ( SK_PlugIn_Type =
                        SK_PlugIn_Type::Unspecified );



#include <SpecialK/parameter.h>
#include <SpecialK/config.h>
#include <SpecialK/ini.h>


sk::iParameter*
_CreateConfigParameter ( std::type_index type,
                         const wchar_t*  wszSection,
                         const wchar_t*  wszKey,
                                  void*  pBackingStore,
                         const wchar_t*  wszDescription    = L"No Description",
                         const wchar_t*  wszOldSectionName = nullptr,
                         const wchar_t*  wszOldKeyName     = nullptr );

sk::ParameterFloat*
_CreateConfigParameterFloat ( const wchar_t* wszSection,
                              const wchar_t* wszKey,
                                      float& backingStore,
                              const wchar_t* wszDescription    = L"No Description",
                              const wchar_t* wszOldSectionName = nullptr,
                              const wchar_t* wszOldKeyName     = nullptr );

sk::ParameterBool*
_CreateConfigParameterBool  ( const wchar_t* wszSection,
                              const wchar_t* wszKey,
                              bool& backingStore,
                              const wchar_t* wszDescription    = L"No Description",
                              const wchar_t* wszOldSectionName = nullptr,
                              const wchar_t* wszOldKeyName     = nullptr );

sk::ParameterInt*
_CreateConfigParameterInt   ( const wchar_t* wszSection,
                              const wchar_t* wszKey,
                              int& backingStore,
                              const wchar_t* wszDescription    = L"No Description",
                              const wchar_t* wszOldSectionName = nullptr,
                              const wchar_t* wszOldKeyName     = nullptr );

sk::ParameterInt64*
_CreateConfigParameterInt64 ( const wchar_t* wszSection,
                              const wchar_t* wszKey,
                              int64_t& backingStore,
                              const wchar_t* wszDescription    = L"No Description",
                              const wchar_t* wszOldSectionName = nullptr,
                              const wchar_t* wszOldKeyName     = nullptr );

#endif /* __SK__Plugin__Manager_H__ */



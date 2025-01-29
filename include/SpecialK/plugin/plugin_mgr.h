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
using SK_ExitGame_pfn                 = void    (__stdcall *)( void        );
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
using SK_AchievementUnlock_pfn        = void    (          *)
                                       (SK_AchievementManager::Achievement*);


struct SK_PluginRegistry
{
  //Stupid Hack, rewrite me... (IN PROGRESS - see isPlugin below)
  bool isArkhamKnight    = false;
  bool isTalesOfZestiria = false;
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
  std::set <SK_ExitGame_pfn>                   exit_game_fns;
  std::set <SK_AchievementUnlock_pfn> achievement_unlock_fns;

  std::wstring plugin_name;
};
extern SK_LazyGlobal <SK_PluginRegistry> plugin_mgr;

#include <Windows.h>
#include <render/dxgi/dxgi_backend.h>


// As a general rule, plug-ins are only _built-in_ to the 64-bit DLL
#ifdef _WIN64
void SK_DGPU_InitPlugin      (void);
void SK_IT_InitPlugin        (void);
void SK_NNK2_InitPlugin      (void);
void SK_TVFix_InitPlugin     (void);
bool SK_TVFix_SharpenShadows (void);
void SK_TVFix_CreateTexture2D(D3D11_TEXTURE2D_DESC *pDesc);

// TODO: Get this stuff out of here, it's breaking what little design work there is.
void SK_DS3_InitPlugin         (void);
bool SK_DS3_UseFlipMode        (void);
bool SK_DS3_IsBorderless       (void);
bool WINAPI
     SK_DS3_ShutdownPlugin (const wchar_t* backend);

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

void SK_Yakuza0_PlugInInit         (void);
bool SK_Yakuza0_PlugInCfg          (void);

extern bool __SK_Yakuza_TrackRTVs;
extern bool __SK_Y0_SafetyLeak;
extern bool __SK_Y0_FixAniso;
extern bool __SK_Y0_ClampLODBias;
extern int  __SK_Y0_ForceAnisoLevel;

void SK_Persona4_InitPlugin        (void);
void SK_YS8_InitPlugin             (void);
void SK_ER_InitPlugin              (void);
void SK_ELEX2_InitPlugin           (void);

void __stdcall
     SK_HatsuneMiku_BeginFrame     (void);

void SK_BGS_InitPlugin             (void);
void SK_LOTF2_InitPlugin           (void);
void SK_OPT_InitPlugin             (void);
void SK_ACV_InitPlugin             (void);

bool SK_NIER_RAD_PlugInCfg         (void);
bool SK_Okami_PlugInCfg            (void);
void SK_Okami_LoadConfig           (void);
bool SK_LSBTS_PlugInCfg            (void);
bool SK_POE2_PlugInCfg             (void);
bool SK_SO4_PlugInCfg              (void);
void SK_ACO_PlugInInit             (void);
bool SK_ACO_PlugInCfg              (void);
void SK_MHW_PlugInInit             (void);
void SK_DQXI_PlugInInit            (void);
void SK_SM_PlugInInit              (void);
bool SK_SM_PlugInCfg               (void);
void SK_NIER_RAD_InitPlugin        (void);
void SK_FF7R_InitPlugin            (void);
void SK_Sekiro_InitPlugin          (void);
void SK_FFXVI_InitPlugin           (void);
void SK_FFXV_InitPlugin            (void);
bool SK_FFXV_PlugInCfg             (void);
void SK_FFXV_SetupThreadPriorities (void);
bool SK_FarCry6_PlugInCfg          (void);

void SK_Metaphor_InitPlugin        (void);

void SK_SO2R_InitPlugin            (void);
bool SK_SO2R_PlugInCfg             (void);
bool SK_SO2R_DrawHandler           (ID3D11DeviceContext *pDevCtx, uint32_t current_ps, int num_verts);

void SK_MHW_PlugIn_Shutdown (void);
extern bool __SK_MHW_KillAntiDebug;

bool __stdcall SK_FAR_IsPlugIn      (void);
void __stdcall SK_FAR_ControlPanel  (void);

extern volatile LONG __SK_SHENMUE_FinishedButNotPresented;
extern volatile LONG __SK_SHENMUE_FullAspectCutscenes;
extern float         __SK_SHENMUE_ClockFuzz;

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
void SK_CC_DrawHandler  ( ID3D11DeviceContext* pDevCtx,
                          uint32_t             current_vs,
                          uint32_t             current_ps );
extern float             __SK_CC_ResMultiplier;
extern ID3D11SamplerState* SK_CC_NearestSampler;
#endif

bool SK_GalGun_PlugInCfg (void);

extern void SK_SEH_LaunchEldenRing         (void);
extern void SK_SEH_LaunchArmoredCoreVI     (void);
extern void SK_SEH_LaunchLordsOfTheFallen2 (void);

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

enum class SK_Import_LoadOrder {
  Undefined = -1,
  Early     =  0,
  PlugIn    =  1,
  Lazy      =  2
};

bool
SK_ImGui_SavePlugInPreference ( iSK_INI* ini, bool enable, const wchar_t* import_name,
                                const wchar_t* role, SK_Import_LoadOrder order, const wchar_t* path,
                                const wchar_t* mode );

void
SK_ImGui_PlugInDisclaimer     ( void );

bool
SK_ImGui_PlugInSelector       ( iSK_INI* ini, const std::string& name, const wchar_t* path,
                                const wchar_t* import_name, bool& enable, SK_Import_LoadOrder& order,
                                SK_Import_LoadOrder default_order = SK_Import_LoadOrder::PlugIn );

#endif /* __SK__Plugin__Manager_H__ */



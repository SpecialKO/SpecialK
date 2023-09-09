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
#include <string>

static iSK_INI* game_ini       = nullptr;
static iSK_INI* gameCustom_ini = nullptr;

#ifdef _WIN64

static sk::ParameterFloat* sf_1stFOV = nullptr;
static sk::ParameterFloat* sf_3rdFOV = nullptr;

static float* pf1stFOV = nullptr;
static float* pf3rdFOV = nullptr;

static sk::ParameterBool* __SK_SF_BasicRemastering    = nullptr;
static sk::ParameterBool* __SK_SF_ExtendedRemastering = nullptr;

static bool bRemasterBasicRTs    = true;
static bool bRemasterExtendedRTs = false;

static uintptr_t pBaseAddr = 0;

uintptr_t CalculateOffset(uintptr_t uAddr) {
    if (pBaseAddr == 0)
        pBaseAddr = reinterpret_cast<uintptr_t>(SK_Debug_GetImageBaseAddr());
    return pBaseAddr + uAddr - 0x140000000;
}

bool SK_SF_PlugInCfg() {
    std::string  utf8VersionString = SK_WideCharToUTF8(SK_GetDLLVersionStr(SK_GetHostApp()));

    if (ImGui::CollapsingHeader(utf8VersionString.data(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.90f, 0.40f, 0.40f, 0.45f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.90f, 0.45f, 0.45f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.87f, 0.53f, 0.53f, 0.80f));
        ImGui::TreePush("");

        if (ImGui::CollapsingHeader ("Render Quality"))
        {
          bool changed = false;

          ImGui::TreePush ("");
          changed |= ImGui::Checkbox ("Upgrade Base RTs to 16-bit color",       &bRemasterBasicRTs);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Eliminates banding on UI at the cost of (negligible) extra VRAM");

          // Only works in the Steam version right now
          if (GetModuleHandle (L"steam_api64.dll"))
          {
            ImGui::SameLine ();

            changed |= ImGui::Checkbox ("Upgrade Most 8-bit RTs to 16-bit color", &bRemasterExtendedRTs);

            if (ImGui::IsItemHovered ())
            {
              ImGui::SetTooltip ("May further reduce banding and improve HDR, but at high memory cost");
            }
          }

          ImGui::TreePop ();

          static bool restart_needed = false;

          if (changed)
          {
            restart_needed = true;

            __SK_SF_BasicRemastering->store    (bRemasterBasicRTs);
            __SK_SF_ExtendedRemastering->store (bRemasterExtendedRTs);

            extern iSK_INI *dll_ini;
            dll_ini->write ();
          }

          if (restart_needed)
          {
            ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
            ImGui::BulletText     ("Game Restart Required");
            ImGui::PopStyleColor  ();
          }
        }

        if (pf1stFOV != nullptr)
        {
          bool changed = false;

          if (ImGui::CollapsingHeader("Field of View"))
          {
            ImGui::TreePush("");

            changed |= ImGui::SliderFloat("1st Person FOV", pf1stFOV, 1, 120, "%.0f");
            changed |= ImGui::SliderFloat("3rd Person FOV", pf3rdFOV, 1, 120, "%.0f");

            ImGui::TreePop();
          }

          if (changed)
          {
            sf_1stFOV->store(*pf1stFOV);
            sf_3rdFOV->store(*pf3rdFOV);

            gameCustom_ini->write();
          }
        }

        ImGui::Separator();
        ImGui::PopStyleColor(3);
        ImGui::TreePop();
    }

    return true;
}
#else
typedef HRESULT (WINAPI * D3DXCreateCubeTextureFromFileInMemoryEx_pfn)(
    LPDIRECT3DDEVICE9, LPCVOID, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR, LPCVOID, LPCVOID, LPCVOID
);

typedef HRESULT(WINAPI * D3DXCreateTextureFromFileInMemoryEx_pfn)(
    LPDIRECT3DDEVICE9, LPCVOID, UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR, LPCVOID, LPCVOID, LPCVOID
);

typedef HRESULT(WINAPI * D3DXCreateVolumeTextureFromFileInMemoryEx_pfn)(
    LPDIRECT3DDEVICE9, LPCVOID, UINT, UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR, LPCVOID, LPCVOID, LPCVOID
);

D3DXCreateCubeTextureFromFileInMemoryEx_pfn BGS_CreateCube;
D3DXCreateTextureFromFileInMemoryEx_pfn BGS_CreateTexture;
D3DXCreateVolumeTextureFromFileInMemoryEx_pfn BGS_CreateVolume;

// Code by karut https://github.com/carxt
HRESULT __stdcall CreateCubeTextureFromFileInMemoryHookForD3D9(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcData, UINT SrcDataSize, LPDIRECT3DCUBETEXTURE9* ppCubeTexture) {
    return BGS_CreateCube(pDevice, pSrcData, SrcDataSize, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppCubeTexture);
}
HRESULT __stdcall CreateTextureFromFileInMemoryHookForD3D9(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcData, UINT SrcDataSize, LPDIRECT3DTEXTURE9* ppTexture) {
    return BGS_CreateTexture(pDevice, pSrcData, SrcDataSize, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppTexture);
}
HRESULT __stdcall CreateVolumeTextureFromFileInMemoryHookForD3D9(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcFile, UINT SrcData, LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture) {
    return BGS_CreateVolume(pDevice, pSrcFile, SrcData, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppVolumeTexture);
}
#endif

void SK_SEH_InitStarfieldRTs (void)
{
  __try
  {
    if (bRemasterBasicRTs)
    {
      void *scan =
        SK_ScanAlignedEx ("\xC6\x45\x68\x01\x8B\x05", 7, "\xff\xff\xff\xff\xff\xff", nullptr, 8);

      SK_LOGs0 (L"Starfield ", L"Scanned Address 0: %p", scan);

      auto      offset         = *reinterpret_cast < int32_t *>((uintptr_t)scan + 6);
	    uint32_t* frameBufferPtr =  reinterpret_cast <uint32_t *>((uintptr_t)scan + 10 + offset);  // 507A290

      *frameBufferPtr = 77;

      scan =
        SK_ScanAlignedEx ("\x44\x8B\x05\x00\x00\x00\x00\x89\x55\xFB", 10, "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF", scan, 8);

      SK_LOGs0 (L"Starfield ", L"Scanned Address 1: %p", scan);

      offset                                = *reinterpret_cast < int32_t *>((uintptr_t)scan + 3);
      uint32_t *imageSpaceBufferPtr         =  reinterpret_cast <uint32_t *>((uintptr_t)scan + 7 + offset);            // 5079A70
      uint32_t *scaleformCompositeBufferPtr =  reinterpret_cast <uint32_t *>((uintptr_t)imageSpaceBufferPtr + 0x280);  // 5079CF0

      *imageSpaceBufferPtr         = 77;
      *scaleformCompositeBufferPtr = 77;
    }

    if (bRemasterExtendedRTs)
    {
      enum class BS_DXGI_FORMAT
      {
        BS_DXGI_FORMAT_UNKNOWN0 = 0,
        BS_DXGI_FORMAT_R8_UNORM1 = 1,
        BS_DXGI_FORMAT_R8_SNORM2 = 2,
        BS_DXGI_FORMAT_R8_UINT3,
        BS_DXGI_FORMAT_R8_SINT4,
        BS_DXGI_FORMAT_UNKNOWN5,
        BS_DXGI_FORMAT_UNKNOWN6,
        BS_DXGI_FORMAT_B4G4R4A4_UNORM7,
        BS_DXGI_FORMAT_UNKNOWN8,
        BS_DXGI_FORMAT_UNKNOWN9,
        BS_DXGI_FORMAT_B5G6R5_UNORM10,
        BS_DXGI_FORMAT_B5G6R5_UNORM11,
        BS_DXGI_FORMAT_UNKNOWN12,
        BS_DXGI_FORMAT_B5G5R5A1_UNORM13,
        BS_DXGI_FORMAT_R8G8_UNORM14,
        BS_DXGI_FORMAT_R8G8_SNORM15,
        BS_DXGI_FORMAT_UNKNOWN16,
        BS_DXGI_FORMAT_UNKNOWN17,
        BS_DXGI_FORMAT_R8G8_UINT18,
        BS_DXGI_FORMAT_R8G8_SINT19,
        BS_DXGI_FORMAT_UNKNOWN20,
        BS_DXGI_FORMAT_R16_UNORM21,
        BS_DXGI_FORMAT_R16_SNORM22,
        BS_DXGI_FORMAT_R16_UINT23,
        BS_DXGI_FORMAT_R16_SINT24,
        BS_DXGI_FORMAT_R16_FLOAT25,
        BS_DXGI_FORMAT_UNKNOWN26,
        BS_DXGI_FORMAT_UNKNOWN27,
        BS_DXGI_FORMAT_UNKNOWN28,
        BS_DXGI_FORMAT_UNKNOWN29,
        BS_DXGI_FORMAT_UNKNOWN30,
        BS_DXGI_FORMAT_UNKNOWN31,
        BS_DXGI_FORMAT_UNKNOWN32,
        BS_DXGI_FORMAT_UNKNOWN33,
        BS_DXGI_FORMAT_UNKNOWN34,
        BS_DXGI_FORMAT_UNKNOWN35,
        BS_DXGI_FORMAT_R8G8B8A8_UNORM36,
        BS_DXGI_FORMAT_R8G8B8A8_SNORM37,
        BS_DXGI_FORMAT_R8G8B8A8_UINT38,
        BS_DXGI_FORMAT_R8G8B8A8_SINT39,
        BS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB40,
        BS_DXGI_FORMAT_B8G8R8A8_UNORM41,
        BS_DXGI_FORMAT_UNKNOWN42,
        BS_DXGI_FORMAT_UNKNOWN43,
        BS_DXGI_FORMAT_UNKNOWN44,
        BS_DXGI_FORMAT_B8G8R8A8_UNORM_SRGB45,
        BS_DXGI_FORMAT_UNKNOWN46,
        BS_DXGI_FORMAT_B8G8R8X8_UNORM47,
        BS_DXGI_FORMAT_R16G16_UNORM48,
        BS_DXGI_FORMAT_UNKNOWN49,
        BS_DXGI_FORMAT_R16G16_SNORM50,
        BS_DXGI_FORMAT_UNKNOWN51,
        BS_DXGI_FORMAT_R16G16_UINT52,
        BS_DXGI_FORMAT_R16G16_SINT53,
        BS_DXGI_FORMAT_R16G16_FLOAT54,
        BS_DXGI_FORMAT_R32_UINT55,
        BS_DXGI_FORMAT_R32_SINT56,
        BS_DXGI_FORMAT_R32_FLOAT57,
        BS_DXGI_FORMAT_UNKNOWN58,
        BS_DXGI_FORMAT_UNKNOWN59,
        BS_DXGI_FORMAT_UNKNOWN60,
        BS_DXGI_FORMAT_UNKNOWN61,
        BS_DXGI_FORMAT_R10G10B10A2_UNORM62,
        BS_DXGI_FORMAT_R10G10B10A2_UINT63,
        BS_DXGI_FORMAT_UNKNOWN64,
        BS_DXGI_FORMAT_UNKNOWN65,
        BS_DXGI_FORMAT_R11G11B10_FLOAT66,
        BS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP67,
        BS_DXGI_FORMAT_UNKNOWN68,
        BS_DXGI_FORMAT_UNKNOWN69,
        BS_DXGI_FORMAT_UNKNOWN70,
        BS_DXGI_FORMAT_UNKNOWN71,
        BS_DXGI_FORMAT_UNKNOWN72,
        BS_DXGI_FORMAT_R16G16B16A16_UNORM73,
        BS_DXGI_FORMAT_R16G16B16A16_SNORM74,
        BS_DXGI_FORMAT_R16G16B16A16_UINT75,
        BS_DXGI_FORMAT_R16G16B16A16_SINT76,
        BS_DXGI_FORMAT_R16G16B16A16_FLOAT77,
        BS_DXGI_FORMAT_R32G32_UINT78,
        BS_DXGI_FORMAT_R32G32_SINT79,
        BS_DXGI_FORMAT_R32G32_FLOAT80,
        BS_DXGI_FORMAT_R32G32B32_UINT81,
        BS_DXGI_FORMAT_R32G32B32_SINT82,
        BS_DXGI_FORMAT_R32G32B32_FLOAT83,
        BS_DXGI_FORMAT_R32G32B32A32_UINT84,
        BS_DXGI_FORMAT_R32G32B32A32_SINT85,
        BS_DXGI_FORMAT_R32G32B32A32_FLOAT86,
        BS_DXGI_FORMAT_UNKNOWN87,
        BS_DXGI_FORMAT_UNKNOWN88,
        BS_DXGI_FORMAT_UNKNOWN89,
        BS_DXGI_FORMAT_UNKNOWN90,
        BS_DXGI_FORMAT_UNKNOWN91,
        BS_DXGI_FORMAT_UNKNOWN92,
        BS_DXGI_FORMAT_UNKNOWN93,
        BS_DXGI_FORMAT_UNKNOWN94,
        BS_DXGI_FORMAT_UNKNOWN95,
        BS_DXGI_FORMAT_UNKNOWN96,
        BS_DXGI_FORMAT_UNKNOWN97,
        BS_DXGI_FORMAT_UNKNOWN98,
        BS_DXGI_FORMAT_D16_UNORM99,
        BS_DXGI_FORMAT_D24_UNORM_S8_UINT100,
        BS_DXGI_FORMAT_D32_FLOAT101,
        BS_DXGI_FORMAT_D24_UNORM_S8_UINT102,
        BS_DXGI_FORMAT_D24_UNORM_S8_UINT103,
        BS_DXGI_FORMAT_D32_FLOAT_S8X24_UINT104,
        BS_DXGI_FORMAT_BC1_UNORM105,
        BS_DXGI_FORMAT_BC1_UNORM_SRGB106,
        BS_DXGI_FORMAT_BC1_UNORM107,
        BS_DXGI_FORMAT_BC1_UNORM_SRGB108,
        BS_DXGI_FORMAT_BC2_UNORM109,
        BS_DXGI_FORMAT_BC2_UNORM_SRGB110,
        BS_DXGI_FORMAT_BC3_UNORM111,
        BS_DXGI_FORMAT_BC3_UNORM_SRGB112,
        BS_DXGI_FORMAT_BC4_UNORM113,
        BS_DXGI_FORMAT_BC4_SNORM114,
        BS_DXGI_FORMAT_BC5_UNORM115,
        BS_DXGI_FORMAT_BC5_SNORM116,
        BS_DXGI_FORMAT_BC6H_UF16_117,
        BS_DXGI_FORMAT_BC6H_SF16_118,
        BS_DXGI_FORMAT_BC7_UNORM119,
        BS_DXGI_FORMAT_BC7_UNORM_SRGB120
      };

      struct BufferDefinition
      {
        BS_DXGI_FORMAT format;
        uint32_t       unk04;
        const char *bufferName;
        uint16_t       unk10;
        uint16_t       unk12;
        uint32_t       unk14;
        uint16_t       unk18;
        uint16_t       unk1A;
        uint32_t       unk1C;
        uint16_t       unk20;
        uint16_t       unk22;
        uint32_t       unk24;
        uint16_t       unk28;
        uint16_t       unk2A;
        uint32_t       unk2C;
        uint32_t       unk30;
        float          unk34;
        float          unk38;
        float          unk3C;
        uint32_t       unk40;
        uint32_t       unk44;
        uint32_t       unk48;
        uint32_t       unk4C;
      };

      BufferDefinition **buffer_defs =
        (BufferDefinition **)((uintptr_t)CalculateOffset (0x144718E40));

      const char *buffers_to_remaster [] =
      {
      "NativeResolutionColorBuffer01",
      "ImageSpaceBuffer",
      "HDRImagespaceBuffer",
      "ImageSpaceBufferR10G10B10A2",
      "ImageSpaceBufferB10G11R11",
      "ImageSpaceBufferE5B9G9R9",
      "GBuffer_Normal_EmissiveIntensity",

      "FrameBuffer",
      "SF_ColorBuffer",

      "TAA_idTech7HistoryColorTarget",

      "EnvBRDF",

      "GBuffer_AlbedoMisc",
      "GBuffer_AO_Rough_Metal",
      "GBuffer_Optional",
      "LightingBufferUV",
      "SAORawAO",
      "DownsampleOutputPrevFrame",
      "DownsampleOutput",
      "SobelOutput",
      "SpaceGlareBlur",
      "SeparableSSSBufferUV",
//FSR2_RESAMPLED_LUMA_HISTORY' (113) using FP16
      "ThinGBuffer_Albedo",
      "ThinGBuffer_Optional",
      "ThinGBuffer_AlbedoArray",
      "ThinGBuffer_OptionalArray",
      "SkyCubemapThinGBuffer_Albedo",
      "SkyCubemapThinGBuffer_Optional",
      "CelestialBodyThinGBuffer_Albedo",
      "CelestialBodyThinGBuffer_Optional",
      "EpipolarExtinction",
      "ImageProcessColorTarget"
      };

      for (UINT i = 0 ; i < 200 ; ++i)
      {
        __try
        {
          for (auto remaster : buffers_to_remaster)
          {
            if (StrStrIA (buffer_defs [i]->bufferName, remaster) != 0)
            {
              buffer_defs [i]->format = BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT77;

              SK_LOGs0 (L"Starfield ", L"Remastered Buffer: '%hs' (%d) using FP16", buffer_defs [i]->bufferName, i);
              break;
            }
          }
        }

        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        };
      }
    }
  }
  
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    SK_LOGs0 (L"Starfield ", L"Structured Exception During HDR Init");
  };
}

void
SK_BGS_InitPlugin(void)
{
    SK_GAME_ID gameID = SK_GetCurrentGameID();

#ifdef _WIN64
    if (gameID == SK_GAME_ID::Starfield)
    {
      __SK_SF_BasicRemastering =
        _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                     L"BasicRTUpgrades", bRemasterBasicRTs,
                                                          L"Promote Simple RTs to FP16" );

      __SK_SF_ExtendedRemastering =
        _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                     L"ExtendedRTUpgrades", bRemasterExtendedRTs,
                                                            L"Promote All 8-bit RTs to FP16" );

      if (bRemasterBasicRTs || bRemasterExtendedRTs)
      {
        SK_SEH_InitStarfieldRTs ();
      }

      if (SK_GetModuleHandle (L"steam_api64.dll"))
      {
        if (game_ini == nullptr) {
            game_ini = SK_CreateINI(LR"(.\Starfield.ini)");
        }

        if (gameCustom_ini == nullptr) {
            gameCustom_ini = SK_CreateINI((SK_GetDocumentsDir() + LR"(\My Games\Starfield\StarfieldCustom.ini)").c_str());
        }

        game_ini->set_encoding       (iSK_INI::INI_UTF8NOBOM);
        gameCustom_ini->set_encoding (iSK_INI::INI_UTF8NOBOM);

        sf_1stFOV = dynamic_cast <sk::ParameterFloat*> (g_ParameterFactory->create_parameter <float>(L"First Person FOV"));
        sf_3rdFOV = dynamic_cast <sk::ParameterFloat*> (g_ParameterFactory->create_parameter <float>(L"Third Person FOV"));
        
        sf_1stFOV->register_to_ini(gameCustom_ini, L"Camera", L"fFPWorldFOV");
        sf_3rdFOV->register_to_ini(gameCustom_ini, L"Camera", L"fTPWorldFOV");

        pf1stFOV = reinterpret_cast<float*>(CalculateOffset(0x14557B930) + 8);
        pf3rdFOV = reinterpret_cast<float*>(CalculateOffset(0x14557B910) + 8);
      }

      plugin_mgr->config_fns.emplace(SK_SF_PlugInCfg);
    }
#else

    // Forces D3DPOOL_DEFAULT pool in order to allow texture debugging and caching
    if (config.textures.d3d9_mod) {
        BGS_CreateCube      = reinterpret_cast<D3DXCreateCubeTextureFromFileInMemoryEx_pfn>  (SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateCubeTextureFromFileInMemoryEx"));
        BGS_CreateTexture   = reinterpret_cast<D3DXCreateTextureFromFileInMemoryEx_pfn>      (SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateTextureFromFileInMemoryEx"));
        BGS_CreateVolume    = reinterpret_cast<D3DXCreateVolumeTextureFromFileInMemoryEx_pfn>(SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateVolumeTextureFromFileInMemoryEx"));

        if (BGS_CreateCube && BGS_CreateTexture && BGS_CreateVolume) {
            uintptr_t baseTexture   = 0;
            uintptr_t cubeTexture   = 0;
            uintptr_t volumeTexture = 0;

            switch (gameID) {
            case SK_GAME_ID::FalloutNewVegas:
                baseTexture   = 0xFDF3FC;
                cubeTexture   = 0xFDF400;
                volumeTexture = 0xFDF404;
                break;
            case SK_GAME_ID::Fallout3:
                baseTexture   = 0xD9B3F0;
                cubeTexture   = 0xD9B3F4;
                volumeTexture = 0xD9B3FC;
                break;
            case SK_GAME_ID::Oblivion:
                baseTexture   = 0xA28364;
                cubeTexture   = 0xA28368;
                volumeTexture = 0xA2836C;
                break;
            }

            if (baseTexture != 0) {
                DWORD oldProtect;

                if (VirtualProtect(reinterpret_cast<void*>(baseTexture), 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    *reinterpret_cast<UINT32*>(baseTexture) = reinterpret_cast<UINT32>(CreateTextureFromFileInMemoryHookForD3D9);
                    VirtualProtect(reinterpret_cast<void*>(baseTexture), 4, oldProtect, &oldProtect);
                }

                if (VirtualProtect(reinterpret_cast<void*>(cubeTexture), 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    *reinterpret_cast<UINT32*>(cubeTexture) = reinterpret_cast<UINT32>(CreateCubeTextureFromFileInMemoryHookForD3D9);
                    VirtualProtect(reinterpret_cast<void*>(cubeTexture), 4, oldProtect, &oldProtect);
                }

                if (VirtualProtect(reinterpret_cast<void*>(volumeTexture), 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    *reinterpret_cast<UINT32*>(volumeTexture) = reinterpret_cast<UINT32>(CreateVolumeTextureFromFileInMemoryHookForD3D9);
                    VirtualProtect(reinterpret_cast<void*>(volumeTexture), 4, oldProtect, &oldProtect);
                }
            }
        }
    }
#endif


}
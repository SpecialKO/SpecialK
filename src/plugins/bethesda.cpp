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

        if (ImGui::CollapsingHeader("Field of View")) {
            ImGui::TreePush("");
            ImGui::SliderFloat("1st Person FOV", pf1stFOV, 1, 120, "%.0f");
            ImGui::SliderFloat("3rd Person FOV", pf3rdFOV, 1, 120, "%.0f");

            ImGui::TreePop();
        }

        if (ImGui::Button("Save Settings", ImVec2(100.0f, 0))) {
            sf_1stFOV->store(*pf1stFOV);
            sf_3rdFOV->store(*pf3rdFOV);

            gameCustom_ini->write();
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

void SK_SEH_InitStarfieldHDR (void)
{
  __try
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
      extern bool __SK_HDR_16BitSwap;

      if (__SK_HDR_16BitSwap)
      {
        //auto threads =
        //  SK_SuspendAllOtherThreads ();

        SK_SEH_InitStarfieldHDR ();

        //SK_ResumeThreads (threads);
      }

      if (SK_GetModuleHandle (L"steam_api64.dll"))
      {
        if (game_ini == nullptr) {
            game_ini = SK_CreateINI(LR"(.\Starfield.ini)");
        }

        if (gameCustom_ini == nullptr) {
            gameCustom_ini = SK_CreateINI((SK_GetDocumentsDir() + LR"(\My Games\Starfield\StarfieldCustom.ini)").c_str());
        }

        game_ini->set_encoding       (iSK_INI::INI_UTF8);
        gameCustom_ini->set_encoding (iSK_INI::INI_UTF8);

        sf_1stFOV = dynamic_cast <sk::ParameterFloat*> (g_ParameterFactory->create_parameter <float>(L"First Person FOV"));
        sf_3rdFOV = dynamic_cast <sk::ParameterFloat*> (g_ParameterFactory->create_parameter <float>(L"Third Person FOV"));
        
        sf_1stFOV->register_to_ini(gameCustom_ini, L"Camera", L"fFPWorldFOV");
        sf_3rdFOV->register_to_ini(gameCustom_ini, L"Camera", L"fTPWorldFOV");

        pf1stFOV = reinterpret_cast<float*>(CalculateOffset(0x14557B930) + 8);
        pf3rdFOV = reinterpret_cast<float*>(CalculateOffset(0x14557B910) + 8);

        plugin_mgr->config_fns.emplace(SK_SF_PlugInCfg);
      }
    }
#else

    // Forces D3DPOOL_DEFAULT pool in order to allow texture debugging and caching
    if (config.textures.d3d9_mod) {
        BGS_CreateCube      = reinterpret_cast<D3DXCreateCubeTextureFromFileInMemoryEx_pfn>(SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateCubeTextureFromFileInMemoryEx"));
        BGS_CreateTexture   = reinterpret_cast<D3DXCreateTextureFromFileInMemoryEx_pfn>(SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateTextureFromFileInMemoryEx"));
        BGS_CreateVolume    = reinterpret_cast<D3DXCreateVolumeTextureFromFileInMemoryEx_pfn>(SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateVolumeTextureFromFileInMemoryEx"));

        if (BGS_CreateCube && BGS_CreateTexture && BGS_CreateVolume) {
            uintptr_t baseTexture = 0;
            uintptr_t cubeTexture = 0;
            uintptr_t volumeTexture = 0;

            switch (gameID) {
            case SK_GAME_ID::FalloutNewVegas:
                baseTexture = 0xFDF3FC;
                cubeTexture = 0xFDF400;
                volumeTexture = 0xFDF404;
                break;
            case SK_GAME_ID::Fallout3:
                baseTexture = 0xD9B3F0;
                cubeTexture = 0xD9B3F4;
                volumeTexture = 0xD9B3FC;
                break;
            case SK_GAME_ID::Oblivion:
                baseTexture = 0xA28364;
                cubeTexture = 0xA28368;
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
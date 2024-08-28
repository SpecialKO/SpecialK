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
#define __SK_SUBSYSTEM__ L"DX12Shader"

namespace SK          {
  namespace D3D11On12 {
//---------------------
class VertexShader { };
class PixelShader  { };
//----------------------
                      };
                      };

static std::string _SpecialNowhere;

template <typename _ShaderType>
struct ShaderBase
{
  _ShaderType *shader     = nullptr;
  ID3D10Blob  *shaderBlob = nullptr;

  bool assembleShaderString ( const char*    szShaderString,
                              const wchar_t* wszShaderName,
                              const char*    szEntryPoint,
                              const char*    szShaderModel,
                                    bool     recompile =
                                               false,
                                std::string& error_log =
                                               _SpecialNowhere )
  {
    UNREFERENCED_PARAMETER (recompile);
    UNREFERENCED_PARAMETER (error_log);

    SK_ComPtr <ID3D10Blob>
              messageBlob;

    HRESULT hr =
      SK_D3D_Compile ( szShaderString,
               strlen (szShaderString),
                 nullptr, nullptr, nullptr,
                   szEntryPoint, szShaderModel,
                            0, 0, &shaderBlob,
                                 &messageBlob );

    if (FAILED (hr))
    {
      if ( messageBlob != nullptr     &&
           messageBlob->GetBufferSize () > 0 )
      {
        std::string
          err;
          err.reserve (  messageBlob->GetBufferSize    () );
          err = ((char *)messageBlob->GetBufferPointer () );

        if (! err.empty ())
        {
          dll_log->LogEx (
            true, L"SK D3D11 Shader (SM=%hs) [%ws]: %hs",
                    szShaderModel,
                    wszShaderName, err.c_str () );
        }
      }

      return false;
    }

    return true;

#if 0
    HRESULT hrCompile =
      DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

    SK_ComQIPtr <ID3D11Device>
                      pDev (
                        SK_GetCurrentRenderBackend ().device
                      );

    if ( pDev != nullptr &&
         std::type_index ( typeid (    _ShaderType   ) ) ==
         std::type_index ( typeid (ID3D11VertexShader) ) )
    {
      hrCompile =
        pDev->CreateVertexShader (
           static_cast <DWORD *> ( shaderBlob->GetBufferPointer () ),
                                   shaderBlob->GetBufferSize    (), nullptr,
           reinterpret_cast <ID3D11VertexShader **>
                                       (&shader)
                                 );
    }

    else if ( pDev != nullptr &&
              std::type_index ( typeid (   _ShaderType   ) ) ==
              std::type_index ( typeid (ID3D11PixelShader) ) )
    {
      hrCompile =
        pDev->CreatePixelShader  (
           static_cast <DWORD *> ( shaderBlob->GetBufferPointer () ),
                                   shaderBlob->GetBufferSize    (), nullptr,
           reinterpret_cast <ID3D11PixelShader **>
                                      (&shader)
                                 );
    }

    else
    {
#pragma warning (push)
#pragma warning (disable: 4130) // No @#$% sherlock
     SK_ReleaseAssert ("WTF?!" == nullptr)
#pragma warning (pop)
    }

    return
      SUCCEEDED (hrCompile);
#endif
  }

  bool compileShaderString ( const char*    szShaderString,
                             const wchar_t* wszShaderName,
                             const char*    szEntryPoint,
                             const char*    szShaderModel,
                                   bool     recompile =
                                              false,
                               std::string& error_log =
                                              _SpecialNowhere )
  {
    UNREFERENCED_PARAMETER (recompile);
    UNREFERENCED_PARAMETER (error_log);

    SK_ComPtr <ID3D10Blob>
      compilerOutput;

    HRESULT hr =
      SK_D3D_Compile ( szShaderString,
               strlen (szShaderString),
                 nullptr, nullptr, nullptr,
                   szEntryPoint, szShaderModel,
                     0, 0,
                       &shaderBlob.p,
                         &compilerOutput.p );

    if (FAILED (hr))
    {
      if ( compilerOutput != nullptr     &&
           compilerOutput->GetBufferSize () > 0 )
      {
        std::string
          err;
          err.reserve (
            compilerOutput->GetBufferSize ()
          );
          err =
        ( (char *)compilerOutput->GetBufferPointer () );

        if (! err.empty ())
        {
          dll_log->LogEx ( true,
                             L"SK D3D12 Shader (SM=%hs) [%ws]: %hs",
                               szShaderModel, wszShaderName,
                                 err.c_str ()
                         );
        }
      }

      return false;
    }

    HRESULT hrCompile =
      DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

    SK_ComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

    if ( pDev != nullptr &&
         std::type_index ( typeid (    _ShaderType   ) ) ==
         std::type_index ( typeid (ID3D11VertexShader) ) )
    {
      hrCompile =
        pDev->CreateVertexShader (
           static_cast <DWORD *> ( shaderBlob->GetBufferPointer () ),
                                   shaderBlob->GetBufferSize    (),
                                     nullptr,
           reinterpret_cast <ID3D11VertexShader **>(&shader)
                                 );
    }

    else if ( pDev != nullptr &&
              std::type_index ( typeid (   _ShaderType   ) ) ==
              std::type_index ( typeid (ID3D11PixelShader) ) )
    {
      hrCompile =
        pDev->CreatePixelShader  (
           static_cast <DWORD *> ( shaderBlob->GetBufferPointer () ),
                                   shaderBlob->GetBufferSize    (),
                                     nullptr,
           reinterpret_cast <ID3D11PixelShader **>(&shader)
                                 );
    }

    else
    {
#pragma warning (push)
#pragma warning (disable: 4130) // No @#$% sherlock
      SK_ReleaseAssert ("WTF?!" == nullptr);
#pragma warning (pop)
    }

    return
      SUCCEEDED (hrCompile);
  }

  bool compileShaderFile ( const wchar_t* wszShaderFile,
                           const    char* szEntryPoint,
                           const    char* szShaderModel,
                                    bool  recompile =
                                            false,
                             std::string& error_log =
                                            _SpecialNowhere )
  {
    std::wstring wstr_file =
      wszShaderFile;

    SK_ConcealUserDir (
      wstr_file.data ()
    );

    if ( GetFileAttributesW (wszShaderFile) == INVALID_FILE_ATTRIBUTES )
    {
      static
        Concurrency::concurrent_unordered_set
               <std::wstring>  warned_shaders;

      if (! warned_shaders.count  (wszShaderFile) )
      {     warned_shaders.insert (wszShaderFile);

        SK_LOG0 ( (  L"Shader Compile Failed: File '%ws' Is Not Valid!",
                                                     wstr_file.c_str ()
                  ), L"D3DCompile" );
      }

      return false;
    }

    FILE* fShader =
      _wfsopen ( wszShaderFile, L"rtS",
                   _SH_DENYNO );

    if (fShader != nullptr)
    {
      uint64_t     size =
        SK_File_GetSize (wszShaderFile);

      auto data =
        std::make_unique <char> (static_cast <size_t> (size) + 32);

      memset ( data.get (),             0,
        sk::narrow_cast <size_t> (size) + 32 );

      fread  ( data.get (),             1,
        sk::narrow_cast <size_t> (size) + 2,
                                     fShader );

      fclose (fShader);

      const bool result =
        compileShaderString (
               data.get   (),
          wstr_file.c_str (), szEntryPoint, szShaderModel,
                                 recompile,  error_log
                            );

      return result;
    }

    SK_LOG0 ( (L"Cannot Compile Shader Because of Filesystem Issues"),
               L"D3DCompile" );

    return  false;
  }

  void releaseResources (void)
  {
    if (shader     != nullptr) {     shader->Release (); shader     = nullptr; }
    if (shaderBlob != nullptr) { shaderBlob->Release (); shaderBlob = nullptr; }
  }
};

using namespace SK::D3D11On12;

namespace SK::D3D11On12::ShaderTask {
  class SDR_to_HDR
  {
    public:
      SDR_to_HDR ( const std::wstring& _name,
                         uint32_t      vs_crc32c,
                         uint32_t      ps_crc32c ) : name (_name)
      {
        *(&vertexOverride._Myfirst._Val) = vs_crc32c;
        *(&pixelOverride._Myfirst._Val)  = ps_crc32c;
      }
  
      std::wstring name;
    protected:
      std::tuple <
         uint32_t,  VertexShader,
       ID3D10Blob*> vertexOverride;
  
      std::tuple <
         uint32_t,  PixelShader,
       ID3D10Blob*> pixelOverride;
  
    private:
      SK_ComPtr <ID3D11VertexShader> pVS;
      SK_ComPtr <ID3D11PixelShader>  pPS;
  };
};


#define  STEAM_OVERLAY_VS_CRC32C   0xf48cf597
#define  STEAM_OVERLAY_PS_CRC32C   0xdeadbeef

#define  RTSS_OVERLAY_VS_CRC32C    0x671afc2f
#define  RTSS_OVERLAY_PS_CRC32C    0x995f6505
#define  RTSS_OVERLAY_PS1_CRC32C   0x4777629e // Simpler pixel shader, just uses vtx colors

#define  DISCORD_OVERLAY_VS_CRC32C 0xdeadbeef
#define  DISCORD_OVERLAY_PS_CRC32C 0xdeadbeef

#define  UPLAY_OVERLAY_VS_CRC32C   0xdeadbeef
#define  UPLAY_OVERLAY_PS_CRC32C   0xdeadbeef


SK::D3D11On12::ShaderTask::SDR_to_HDR
                     _SK_Steam_to_HDR_D3D12 ( L"Steam Overlay HDR Fix",
                                                STEAM_OVERLAY_VS_CRC32C,
                                                STEAM_OVERLAY_PS_CRC32C );
SK::D3D11On12::ShaderTask::SDR_to_HDR
                      _SK_RTSS_to_HDR_D3D12 ( L"RTSS Overlay HDR Fix",
                                                RTSS_OVERLAY_VS_CRC32C,
                                                RTSS_OVERLAY_PS_CRC32C );

SK::D3D11On12::ShaderTask::SDR_to_HDR
                   _SK_DISCORD_to_HDR_D3D12 ( L"Discord Overlay HDR Fix",
                                                DISCORD_OVERLAY_VS_CRC32C,
                                                DISCORD_OVERLAY_PS_CRC32C );

SK::D3D11On12::ShaderTask::SDR_to_HDR
                   _SK_UPLAY_to_HDR_D3D12 ( L"uPlay Overlay HDR Fix",
                                              UPLAY_OVERLAY_VS_CRC32C,
                                              UPLAY_OVERLAY_PS_CRC32C );

std::string
__SK_MakeSteamPS ( bool  hdr10,
                   bool  scRGB,
                   float max_luma )
{
  std::string out =
    "\n"
    "#pragma warning ( disable : 3571 )              \n"
    "struct PS_INPUT                                 \n"
    "{                                               \n"
    "  float4 pos : SV_POSITION;                     \n"
    "  float4 col : COLOR;                           \n"
    "  float2 uv  : TEXCOORD0;                       \n"
    "};                                              \n"
    "                                                \n"
    "sampler   PS_QUAD_Sampler   : register (s0);    \n"
    "Texture2D PS_QUAD_Texture2D : register (t0);    \n"
    "                                                \n"
    "float3                                          \n"
    "RemoveGammaExp (float3 x, float exp)            \n"
    "{                                               \n"
    "  return     sign (x) *                         \n"
    "         pow (abs (x), exp);                    \n"
    "}                                               \n"
    "                                                \n"
    "float                                           \n"
    "RemoveAlphaGammaExp (float a, float exp)        \n"
    "{                                               \n"
    "  return  sign (a) *                            \n"
    "      pow (abs (a), exp);                       \n"
    "}                                               \n"
    "                                                \n";

    if (hdr10)
    { out +=
      "float3 ApplyREC2084Curve (float3 L, float maxLuminance)\n"
      "{                                                      \n"
      "  L = max (L, 0.0);                                    \n"
      "                                                       \n"
      "  float m1 = 2610.0 / 4096.0 / 4;                      \n"
      "  float m2 = 2523.0 / 4096.0 * 128;                    \n"
      "  float c1 = 3424.0 / 4096.0;                          \n"
      "  float c2 = 2413.0 / 4096.0 * 32;                     \n"
      "  float c3 = 2392.0 / 4096.0 * 32;                     \n"
      "                                                       \n"
      "  float maxLuminanceScale = maxLuminance / 10000.0f;   \n"
      "   L *= maxLuminanceScale;                             \n"
      "                                                       \n"
      "  float3 Lp = pow (L, m1);                             \n"
      "                                                       \n"
      "  return pow ((c1 + c2 * Lp) / (1 + c3 * Lp), m2);     \n"
      "}                                                      \n"
      "                                                       \n"
      "float3 REC709toREC2020 (float3 RGB709)                 \n"
      "{                                                      \n"
      "  static const float3x3 ConvMat =                      \n"
      "  {                                                    \n"
      "    0.627402, 0.329292, 0.043306,                      \n"
      "    0.069095, 0.919544, 0.011360,                      \n"
      "    0.016394, 0.088028, 0.895578                       \n"
      "  };                                                   \n"
      "  return mul (ConvMat, RGB709);                        \n"
      "}                                                      \n"
      "                                                       \n";
    }


    out +=
    "                                                         \n"
    "float4 main (PS_INPUT input) : SV_Target                 \n"
    "{                                                        \n"
    "  float4 out_col =                                       \n"
    "    PS_QUAD_Texture2D.Sample (PS_QUAD_Sampler, input.uv);\n"
    "                                                         \n"
    "  out_col =                                              \n"
    "    saturate (                                           \n"
    "      float4 (                                           \n"
    "    RemoveGammaExp ( input.col.rgb * out_col.rgb, 2.2f ),\n"
    "                     input.col.a   * out_col.a )         \n"
    "             );                                          \n"
    "                                                         \n";

    if (hdr10)
    { out +=
      "                                                       \n"
      "  out_col.rgb =                                        \n"
      "    ApplyREC2084Curve (                                \n"
      "         REC709toREC2020 ( out_col.rgb ),              \n";

      out    += std::to_string (max_luma);
      out    +=                                              "\n";

      out    +=
      "                    );                                 \n"
      "  out_col.a   =                                        \n"
      "     RemoveAlphaGammaExp ( out_col.a, 2.2f );          \n"
      "                                                       \n";
    }

    if (scRGB)
    {
      out += "  out_col *= float4 (" +
                   std::to_string (max_luma) + ", " +
                   std::to_string (max_luma) + ", " +
                   std::to_string (max_luma) + ", 1.0);      \n";
             "                                               \n";
    }

    out +=
    "  return                                                \n"
    "    float4 (           out_col.rgb,                     \n"
    "             saturate (out_col.a)                       \n"
    "           );                                           \n"
    "}                                                       \n";

  return out;
}

ID3D10Blob*
__SK_MakeSteamPS_Bytecode ( bool  hdr10,
                            bool  scRGB,
                            float max_luma )
{
  static ShaderBase <ID3D11PixelShader> steam_hdr;
                                        steam_hdr.releaseResources ();

  std::string steam_ps =
        __SK_MakeSteamPS (hdr10, scRGB, max_luma);

  steam_hdr.assembleShaderString (
              steam_ps.c_str (),
            L"Steam HDR PS",
               "main", "ps_5_0", true
  );

  return
    steam_hdr.shaderBlob;
}

std::string
__SK_MakeEpicPS ( bool  hdr10,
                  bool  scRGB,
                  float max_luma )
{
  std::string out =
    "\n"
    "#pragma warning ( disable : 3571 )              \n"
    "cbuffer PS_CONST : register (b0)                \n"
    "{                                               \n"
    "                                                \n"
    "  float4 ColorMultiplier;                       \n"
    "  float4 VertexTx;                              \n"
    "  float4 TextureTx;                             \n"
    "  int    bFlipY;                                \n"
    "}                                               \n"
    "                                                \n"
    "struct PS_INPUT                                 \n"
    "{                                               \n"
    "  float4 pos   : SV_POSITION;                   \n"
    "  float4 color : COLOR;                         \n"
    "  float2 uv    : TEXCOORD0;                     \n"
    "};                                              \n"
    "                                                \n"
    "sampler   Sampler : register (s0);              \n"
    "Texture2D Texture : register (t0);              \n"
    "                                                \n"
    "float3                                          \n"
    "RemoveGammaExp (float3 x, float exp)            \n"
    "{                                               \n"
    "  return     sign (x) *                         \n"
    "         pow (abs (x), exp);                    \n"
    "}                                               \n"
    "                                                \n"
    "float                                           \n"
    "RemoveAlphaGammaExp (float a, float exp)        \n"
    "{                                               \n"
    "  return  sign (a) *                            \n"
    "      pow (abs (a), exp);                       \n"
    "}                                               \n"
    "                                                \n";

    if (hdr10)
    { out +=
      "float3 ApplyREC2084Curve (float3 L, float maxLuminance)\n"
      "{                                                      \n"
      "  L = max (L, 0.0);                                    \n"
      "                                                       \n"
      "  float m1 = 2610.0 / 4096.0 / 4;                      \n"
      "  float m2 = 2523.0 / 4096.0 * 128;                    \n"
      "  float c1 = 3424.0 / 4096.0;                          \n"
      "  float c2 = 2413.0 / 4096.0 * 32;                     \n"
      "  float c3 = 2392.0 / 4096.0 * 32;                     \n"
      "                                                       \n"
      "  float maxLuminanceScale = maxLuminance / 10000.0f;   \n"
      "   L *= maxLuminanceScale;                             \n"
      "                                                       \n"
      "  float3 Lp = pow (L, m1);                             \n"
      "                                                       \n"
      "  return pow ((c1 + c2 * Lp) / (1 + c3 * Lp), m2);     \n"
      "}                                                      \n"
      "                                                       \n"
      "float3 REC709toREC2020 (float3 RGB709)                 \n"
      "{                                                      \n"
      "  static const float3x3 ConvMat =                      \n"
      "  {                                                    \n"
      "    0.627402, 0.329292, 0.043306,                      \n"
      "    0.069095, 0.919544, 0.011360,                      \n"
      "    0.016394, 0.088028, 0.895578                       \n"
      "  };                                                   \n"
      "  return mul (ConvMat, RGB709);                        \n"
      "}                                                      \n"
      "                                                       \n";
    }


    out +=
    "                                                         \n"
    "float4 main (PS_INPUT input) : SV_Target                 \n"
    "{                                                        \n"
    "  float4 out_col =                                       \n"
    "    Texture.Sample ( Sampler, float2 ( input.uv.x,       \n"
    "                        bFlipY ? 1.0 - input.uv.y        \n"
    "                               :       input.uv.y ) );   \n"
    "                                                         \n"
    "  out_col =                                              \n"
    "    saturate (                                           \n"
    "      float4 (                                           \n"
    "    RemoveGammaExp ( input.col.rgb * out_col.rgb, 2.2f ),\n"
    "                     input.col.a   * out_col.a )         \n"
    "             );                                          \n"
    "                                                         \n";

    if (hdr10)
    { out +=
      "                                                       \n"
      "  out_col.rgb =                                        \n"
      "    ApplyREC2084Curve (                                \n"
      "         REC709toREC2020 ( out_col.rgb ),              \n";

      out    += std::to_string (max_luma);
      out    +=                                              "\n";

      out    +=
      "                    );                                 \n"
      "  out_col.a   =                                        \n"
      "     RemoveAlphaGammaExp ( out_col.a, 2.2f );          \n"
      "                                                       \n";
    }

    if (scRGB)
    {
      out += "  out_col *= float4 (" +
                   std::to_string (max_luma) + ", " +
                   std::to_string (max_luma) + ", " +
                   std::to_string (max_luma) + ", 1.0);      \n";
             "                                               \n";
    }

    out +=
    "  return                                                \n"
    "    float4 (           out_col.rgb,                     \n"
    "             saturate (out_col.a)                       \n"
    "           );                                           \n"
    "}                                                       \n";

  return out;
}

ID3D10Blob*
__SK_MakeEpicPS_Bytecode ( bool  hdr10,
                           bool  scRGB,
                           float max_luma )
{
  static ShaderBase <ID3D11PixelShader> epic_hdr;
                                        epic_hdr.releaseResources ();

  std::string epic_ps =
        __SK_MakeEpicPS (hdr10, scRGB, max_luma);

  epic_hdr.assembleShaderString (
              epic_ps.c_str (),
            L"Epic HDR PS",
               "main", "ps_4_0", true
  );

  return
    epic_hdr.shaderBlob;
}

std::string
__SK_MakeRTSSPS ( bool  hdr10,
                  bool  scRGB,
                  float max_luma )
{
  std::string out =
    "\n"
    "#pragma warning ( disable : 3571 )              \n"
    "struct PS_INPUT                                 \n"
    "{                                               \n"
    "  float4 pos   : SV_POSITION;                   \n"
    "  float4 color : COLOR;                         \n"
    "  float2 uv    : TEXCOORD0;                     \n"
    "};                                              \n"
    "                                                \n"
    "float3                                          \n"
    "RemoveGammaExp (float3 x, float exp)            \n"
    "{                                               \n"
    "  return     sign (x) *                         \n"
    "         pow (abs (x), exp);                    \n"
    "}                                               \n"
    "                                                \n"
    "float                                           \n"
    "RemoveAlphaGammaExp (float a, float exp)        \n"
    "{                                               \n"
    "  return  sign (a) *                            \n"
    "      pow (abs (a), exp);                       \n"
    "}                                               \n"
    "                                                \n";

    if (hdr10)
    { out +=
      "float3 ApplyREC2084Curve (float3 L, float maxLuminance)\n"
      "{                                                      \n"
      "  L = max (L, 0.0);                                    \n"
      "                                                       \n"
      "  float m1 = 2610.0 / 4096.0 / 4;                      \n"
      "  float m2 = 2523.0 / 4096.0 * 128;                    \n"
      "  float c1 = 3424.0 / 4096.0;                          \n"
      "  float c2 = 2413.0 / 4096.0 * 32;                     \n"
      "  float c3 = 2392.0 / 4096.0 * 32;                     \n"
      "                                                       \n"
      "  float maxLuminanceScale = maxLuminance / 10000.0f;   \n"
      "   L *= maxLuminanceScale;                             \n"
      "                                                       \n"
      "  float3 Lp = pow (L, m1);                             \n"
      "                                                       \n"
      "  return pow ((c1 + c2 * Lp) / (1 + c3 * Lp), m2);     \n"
      "}                                                      \n"
      "                                                       \n"
      "float3 REC709toREC2020 (float3 RGB709)                 \n"
      "{                                                      \n"
      "  static const float3x3 ConvMat =                      \n"
      "  {                                                    \n"
      "    0.627402, 0.329292, 0.043306,                      \n"
      "    0.069095, 0.919544, 0.011360,                      \n"
      "    0.016394, 0.088028, 0.895578                       \n"
      "  };                                                   \n"
      "  return mul (ConvMat, RGB709);                        \n"
      "}                                                      \n"
      "                                                       \n";
    }


    out +=
    "                                                         \n"
    "float4 main (PS_INPUT input) : SV_Target                 \n"
    "{                                                        \n"
    "  float4 out_col = input.color;                          \n"
    "                                                         \n"
    "  out_col =                                              \n"
    "    saturate (                                           \n"
    "      float4 (                                           \n"
    "                    RemoveGammaExp ( out_col.rgb, 2.2f ),\n"
    "                                     out_col.a )         \n"
    "             );                                          \n"
    "                                                         \n";

    if (hdr10)
    { out +=
      "                                                       \n"
      "  out_col.rgb =                                        \n"
      "    ApplyREC2084Curve (                                \n"
      "         REC709toREC2020 ( out_col.rgb ),              \n";

      out    += std::to_string (max_luma);
      out    +=                                              "\n";

      out    +=
      "                    );                                 \n"
      "  out_col.a   =                                        \n"
      "     RemoveAlphaGammaExp ( out_col.a, 2.2f );          \n"
      "                                                       \n";
    }

    if (scRGB)
    {
      out += "  out_col *= float4 (" +
                   std::to_string (max_luma) + ", " +
                   std::to_string (max_luma) + ", " +
                   std::to_string (max_luma) + ", 1.0);      \n";
             "                                               \n";
    }

    out +=
    "  return                                                \n"
    "    float4 (           out_col.rgb,                     \n"
    "             saturate (out_col.a)                       \n"
    "           );                                           \n"
    "}                                                       \n";

  return out;
}

ID3D10Blob*
__SK_MakeRTSS_PS_Bytecode ( bool  hdr10,
                            bool  scRGB,
                            float max_luma )
{
  static ShaderBase <ID3D11PixelShader> rtss_hdr;
                                        rtss_hdr.releaseResources ();

  std::string rtss_ps =
        __SK_MakeSteamPS (hdr10, scRGB, max_luma);

  rtss_hdr.assembleShaderString (
              rtss_ps.c_str (),
            L"RTSS HDR PS",
               "main", "ps_5_0", true
  );

  return
    rtss_hdr.shaderBlob;
}

ID3D10Blob*
__SK_MakeRTSS_PS1_Bytecode ( bool  hdr10,
                             bool  scRGB,
                             float max_luma )
{
  static ShaderBase <ID3D11PixelShader> rtss1_hdr;
                                        rtss1_hdr.releaseResources ();

  std::string rtss1_ps =
        __SK_MakeRTSSPS (hdr10, scRGB, max_luma);

  rtss1_hdr.assembleShaderString (
              rtss1_ps.c_str (),
            L"RTSS HDR PS (Vtx Color Only)",
               "main", "ps_5_0", true
  );

  return
    rtss1_hdr.shaderBlob;
}
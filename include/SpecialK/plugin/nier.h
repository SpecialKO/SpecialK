//
// Copyright 2017  Andon  "Kaldaien" Coleman,
//                 Niklas "DrDaxxy"  Kielblock,
//                 Peter  "Durante"  Thoman
//
//        Francesco149, Idk31, Smithfield, and GitHub contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
#ifndef __SK__PlugIn__NieR_H__
#define __SK__PlugIn__NieR_H__

#include <minwindef.h>

using vec3_t = float [3]; // X,Z,Y


using SK_PlugIn_ControlPanelWidget_pfn = void (__stdcall         *)(void);
using SK_EndFrame_pfn                  = void (STDMETHODCALLTYPE *)(void);
using SK_PluginKeyPress_pfn            = void (CALLBACK          *)( BOOL Control,
                                                                     BOOL Shift,
                                                                     BOOL Alt,
                                                                     BYTE vkCode );


extern LPVOID __SK_base_img_addr;
extern LPVOID __SK_end_img_addr;


extern void*
__stdcall
SK_Scan                  ( const uint8_t* pattern,
                                 size_t   len,
                           const uint8_t* mask );

extern void
STDMETHODCALLTYPE
SK_BeginBufferSwap       (void);

extern BOOL
__stdcall
SK_DrawExternalOSD       ( std::string app_name,
                           std::string text );

extern void
__stdcall
SK_ImGui_DrawEULA_PlugIn (LPVOID reserved);

extern void
__stdcall
SK_SetPluginName         (std::wstring name);

extern bool
__stdcall
SK_FetchVersionInfo (const wchar_t* wszProduct);

extern HRESULT
__stdcall
SK_UpdateSoftware   (const wchar_t* wszProduct);



using D3D11Dev_CreateBuffer_pfn = HRESULT (WINAPI *)(
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer
);
using D3D11Dev_CreateShaderResourceView_pfn = HRESULT (WINAPI *)(
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView
);

using D3D11_DrawInstanced_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
using D3D11_DrawInstancedIndirect_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);
using D3D11Dev_CreateTexture2D_pfn = HRESULT (WINAPI *)(
  _In_            ID3D11Device            *This,
  _In_      const D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D
);
using D3D11_DrawIndexed_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation
);
using D3D11_Draw_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation
);
using D3D11_DrawIndexedInstanced_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
using D3D11_DrawIndexedInstancedIndirect_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);
using D3D11_DrawInstanced_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation
);
using D3D11_DrawInstancedIndirect_pfn = void (WINAPI *)(
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs
);


extern
HRESULT
WINAPI
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer );

extern
HRESULT
WINAPI
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView );

extern
void
STDMETHODCALLTYPE
D3D11_PSSetConstantBuffers_Override (
  _In_     ID3D11DeviceContext*  This,
  _In_     UINT                  StartSlot,
  _In_     UINT                  NumBuffers,
  _In_opt_ ID3D11Buffer *const  *ppConstantBuffers );

extern
void
WINAPI
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation );

extern
void
WINAPI
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

extern
void
WINAPI
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation );

extern
void
WINAPI
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

extern
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource_Override (
  _In_           ID3D11DeviceContext* This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch);

extern 
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

extern
void
WINAPI
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation );

extern
void
WINAPI
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation );



#endif /* __SK__PlugIn__NieR_H__ */
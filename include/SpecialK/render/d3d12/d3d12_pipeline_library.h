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
#pragma once

#include <SpecialK/render/d3d12/d3d12_interfaces.h>

using D3D12Device1_CreatePipelineLibrary_pfn =
HRESULT (STDMETHODCALLTYPE *)(ID3D12Device1*,const void*,SIZE_T,REFIID,void**);

extern D3D12Device1_CreatePipelineLibrary_pfn
       D3D12Device1_CreatePipelineLibrary_Original;

void SK_D3D12_HookPipelineLibrary (ID3D12Device1 *pDevice1);
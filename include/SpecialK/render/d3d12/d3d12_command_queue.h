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

using
D3D12CommandQueue_ExecuteCommandLists_pfn = void
(STDMETHODCALLTYPE *)(ID3D12CommandQueue*, UINT,
                      ID3D12CommandList* const *);

extern D3D12CommandQueue_ExecuteCommandLists_pfn
       D3D12CommandQueue_ExecuteCommandLists_Original;

bool SK_D3D12_InstallCommandQueueHooks (ID3D12Device *pDev12);
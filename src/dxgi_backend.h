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

#ifndef __SK__DXGI_BACKEND_H__
#define __SK__DXGI_BACKEND_H__

#include <string>

#include <d3d11.h>

namespace SK
{
  namespace DXGI
  {
    bool Startup  (void);
    bool Shutdown (void);

    std::wstring getPipelineStatsDesc (void);

    struct PipelineStatsD3D11 {
      struct StatQueryD3D11 {
        ID3D11Query* async  = nullptr;
        bool         active = false;
      } query;

      D3D11_QUERY_DATA_PIPELINE_STATISTICS
                 last_results;
    } extern pipeline_stats_d3d11;

    //extern HMODULE hModD3D10;
    extern HMODULE hModD3D11;
    //extern HMODULE hModD3D12;
  }
}

#endif /* __SK__DXGI_BACKEND_H__ */
// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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

#ifndef __SK__Plugin__Arknights_Endfield__
#define __SK__Plugin__Arknights_Endfield__

/*
  ---------------- SHARED MEMORY STRUCTURE FOR AKEF INJECTION ----------------
*/
namespace AKEF_SHM
{
  struct SharedMemory
  {
    volatile LONG has_pid         = 0; // 0/1 flag
    DWORD         akef_launch_pid = 0;
    DWORD         padding[6]      = { };
  };

  SharedMemory* GetView (void);

  void PublishPid           (DWORD pid);
  void ResetPid             (void);
  bool TryGetPid            (DWORD* out_pid);
  void CleanupSharedMemory  (void);
}

void SK_AKEF_PublishPid           (DWORD pid);
bool SK_AKEF_TryGetPid            (DWORD* out_pid);
void SK_AKEF_ResetPid             (void);
void SK_AKEF_CleanupSharedMemory  (void);
/* --------------------------------------------------------------------------- */

struct AKEF_WindowInfo
{
  HWND         hwnd = nullptr;
  std::wstring title;
  std::wstring class_name;
};

struct AKEF_WindowData
{
  DWORD                        pid = 0;
  std::vector<AKEF_WindowInfo> windows;
};

void AKEF_ApplyUnityTargetFrameRate (float targetFrameRate);
#endif /* __SK__Plugin__Arknights_Endfield__ */
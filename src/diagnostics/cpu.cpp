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

#include <Windows.h>

#include <SpecialK/diagnostics/cpu.h>
#include <SpecialK/utility.h>
#include <SpecialK/config.h>
#include <SpecialK/hooks.h>

typedef void (WINAPI *GetSystemInfo_pfn)(LPSYSTEM_INFO);
                      GetSystemInfo_pfn
                      GetSystemInfo_Original = nullptr;

typedef BOOL (WINAPI *GetLogicalProcessorInformation_pfn)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);
                      GetLogicalProcessorInformation_pfn
                      GetLogicalProcessorInformation_Original = nullptr;

const std::vector <uintptr_t>&
SK_CPU_GetLogicalCorePairs (void);

#include <SpecialK/log.h>
#define __SK_SUBSYSTEM__ "  CPUMgr  "

BOOL
WINAPI
GetLogicalProcessorInformation_Detour (
    _Out_writes_bytes_to_opt_ (*ReturnedLvength, *ReturnedLength) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
    _Inout_                                                       PDWORD                                ReturnedLength )
{
  SK_LOG_FIRST_CALL

  if (Buffer == nullptr || config.render.framerate.override_num_cpus == -1)
  {
    return
      GetLogicalProcessorInformation_Original ( Buffer, ReturnedLength );
  }

  const BOOL bRet = 
    GetLogicalProcessorInformation_Original ( Buffer, ReturnedLength );

  if (bRet)
  {
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi =
      Buffer;

    DWORD dwOffset = 0;

    while (dwOffset + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= *ReturnedLength)
    {
      switch (lpi->Relationship) 
      {
        case RelationProcessorCore:
        {
          static const std::vector <uintptr_t>& pairs =
            SK_CPU_GetLogicalCorePairs ();

          static const std::set <uintptr_t>& masks =
            std::set <uintptr_t> ( pairs.cbegin (), pairs.cend () );

          if (pairs.size () != masks.size ())
          {
            while (lpi->ProcessorMask >= 2)
            {
              lpi->ProcessorMask >>= 1;
            }
          }
        } break;

        default:
          break;
      }

      dwOffset += sizeof SYSTEM_LOGICAL_PROCESSOR_INFORMATION;
      lpi++;
    }
  }
  
  return bRet;
}

void
WINAPI
GetSystemInfo_Detour (
  _Out_ LPSYSTEM_INFO lpSystemInfo )
{
  GetSystemInfo_Original (lpSystemInfo);

  if (config.render.framerate.override_num_cpus == -1)
    return;

  lpSystemInfo->dwActiveProcessorMask = 0xff;
  lpSystemInfo->dwNumberOfProcessors  = config.render.framerate.override_num_cpus;
}

void
SK_GetSystemInfo (LPSYSTEM_INFO lpSystemInfo)
{
  if (GetSystemInfo_Original != nullptr)
  {
    return
      GetSystemInfo_Original (lpSystemInfo);
  }

  // We haven't hooked that function yet, so call the original.
  return
    GetSystemInfo (lpSystemInfo);
}


void
SK_CPU_InstallHooks (void)
{
  //SK_CreateDLLHook2 (      L"Kernel32",
  //                          "GetLogicalProcessorInformation",
  //                           GetLogicalProcessorInformation_Detour,
  //  static_cast_p2p <void> (&GetLogicalProcessorInformation_Original) );
  
  SK_CreateDLLHook2 (      L"Kernel32",
                            "GetSystemInfo",
                             GetSystemInfo_Detour,
    static_cast_p2p <void> (&GetSystemInfo_Original) );
  
#ifdef SK_AGGRESSIVE_HOOKS
  SK_ApplyQueuedHooks ();
#endif
}


const std::vector <uintptr_t>&
SK_CPU_GetLogicalCorePairs (void)
{
  static std::vector <uintptr_t> logical_proc_siblings;

  if (! logical_proc_siblings.empty ())
    return logical_proc_siblings;

  DWORD dwNeededBytes = 0;

  // We're not hooking anything, so use the regular import
  if (! GetLogicalProcessorInformation_Original)
        GetLogicalProcessorInformation_Original = GetLogicalProcessorInformation;

  if (! GetLogicalProcessorInformation_Original (nullptr, &dwNeededBytes))
  {
    if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
    {
      SYSTEM_LOGICAL_PROCESSOR_INFORMATION   *pLogProcInfo =
       (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)
          new uint8_t [dwNeededBytes] { };

      DWORD dwOffset = 0;

      if (GetLogicalProcessorInformation_Original (pLogProcInfo, &dwNeededBytes))
      {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi =
          pLogProcInfo;

        while (dwOffset + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= dwNeededBytes)
        {
          switch (lpi->Relationship) 
          {
            case RelationProcessorCore:
            {
              //auto CountSetBits = [](ULONG_PTR bitMask) -> DWORD
              //{
              //  DWORD     LSHIFT      = sizeof (ULONG_PTR) * 8 - 1;
              //  DWORD     bitSetCount = 0;
              //  ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    
              //  DWORD     i;
              //
              //  for (i = 0; i <= LSHIFT; ++i)
              //  {
              //    bitSetCount += ((bitMask & bitTest) ? 1 : 0);
              //    bitTest /= 2;
              //  }
              //
              //  return bitSetCount;
              //};
              //
              //logical_cores += CountSetBits (lpi->ProcessorMask);

#if defined (_WIN64) && defined (_DEBUG)
              // Don't want to bother with server hardware! No gaming machine needs this many cores (2018)
              ////assert ((lpi->ProcessorMask & 0xFFFFFFFF00000000) == 0);
#endif
              logical_proc_siblings.push_back (lpi->ProcessorMask);
            } break;

            default:
              break;
          }

          dwOffset += sizeof SYSTEM_LOGICAL_PROCESSOR_INFORMATION;
          lpi++;
        }
      }

      delete [] pLogProcInfo;
    }
  }

  return logical_proc_siblings;
}
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
#include <SpecialK/hooks.h>

typedef void (WINAPI *GetSystemInfo_pfn)(LPSYSTEM_INFO);
                      GetSystemInfo_pfn
                      GetSystemInfo_Original = nullptr;

typedef BOOL (WINAPI *GetLogicalProcessorInformation_pfn)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);
                      GetLogicalProcessorInformation_pfn
                      GetLogicalProcessorInformation_Original = nullptr;
BOOL
WINAPI
GetLogicalProcessorInformation_Detour (
    _Out_writes_bytes_to_opt_ (*ReturnedLength, *ReturnedLength) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
    _Inout_                                                      PDWORD                                ReturnedLength )
{
  if (Buffer == nullptr)
  {
    return
      GetLogicalProcessorInformation_Original ( Buffer, ReturnedLength );
  }

  BOOL bRet = 
    GetLogicalProcessorInformation_Original ( Buffer, ReturnedLength );

  if (bRet)
  {
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi =
      Buffer;

    int mask = 1;

    DWORD dwOffset = 0;

    while (dwOffset + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= *ReturnedLength)
    {
      switch (lpi->Relationship) 
      {
        case RelationProcessorCore:
        {
          lpi->ProcessorMask = ( mask <<= 1 ) | ( mask <<= 1 ) | ( mask <<= 1 ) | ( mask <<= 1 );
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

  lpSystemInfo->dwActiveProcessorMask = 0xff;
  lpSystemInfo->dwNumberOfProcessors  = 16;
}


void
SK_CPU_InstallHooks (void)
{
  //SK_CreateDLLHook2 (      L"Kernel32",
  //                          "GetLogicalProcessorInformation",
  //                           GetLogicalProcessorInformation_Detour,
  //  static_cast_p2p <void> (&GetLogicalProcessorInformation_Original) );
  //
  //SK_CreateDLLHook2 (      L"Kernel32",
  //                          "GetSystemInfo",
  //                           GetSystemInfo_Detour,
  //  static_cast_p2p <void> (&GetSystemInfo_Original) );
  //
  //SK_ApplyQueuedHooks ();
}


const std::vector <uintptr_t>&
SK_CPU_GetLogicalCorePairs (void)
{
  static std::vector <uintptr_t> logical_proc_siblings;

  if (! logical_proc_siblings.empty ())
    return logical_proc_siblings;

  DWORD dwNeededBytes = 0;

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
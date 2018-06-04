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

#include <SpecialK/diagnostics/cpu.h>

#include <Windows.h>

const std::vector <uintptr_t>&
SK_CPU_GetLogicalCorePairs (void)
{
  static std::vector <uintptr_t> logical_proc_siblings;

  if (! logical_proc_siblings.empty ())
    return logical_proc_siblings;

  DWORD dwNeededBytes = 0;

  if (! GetLogicalProcessorInformation (nullptr, &dwNeededBytes))
  {
    if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
    {
      SYSTEM_LOGICAL_PROCESSOR_INFORMATION   *pLogProcInfo =
       (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)
          new uint8_t [dwNeededBytes] { };

      DWORD dwOffset = 0;

      if (GetLogicalProcessorInformation (pLogProcInfo, &dwNeededBytes))
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
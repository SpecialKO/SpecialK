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

#include <SpecialK/diagnostics/cpu.h>

typedef void (WINAPI *GetSystemInfo_pfn)(LPSYSTEM_INFO);
                      GetSystemInfo_pfn
                      GetSystemInfo_Original = nullptr;

typedef BOOL (WINAPI *GetLogicalProcessorInformation_pfn)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);
                      GetLogicalProcessorInformation_pfn
                      GetLogicalProcessorInformation_Original = nullptr;

typedef BOOL (WINAPI *GetLogicalProcessorInformationEx_pfn)(LOGICAL_PROCESSOR_RELATIONSHIP,PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,PDWORD);
                       GetLogicalProcessorInformationEx_pfn
                       GetLogicalProcessorInformationEx_Original = nullptr;

const std::vector <uintptr_t>&
SK_CPU_GetLogicalCorePairs (void);

size_t
SK_CPU_CountPhysicalCores (void);

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ "  CPUMgr  "

BOOL
WINAPI
GetLogicalProcessorInformation_Detour (
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
  PDWORD                                ReturnedLength )
{
  SK_LOG_FIRST_CALL

  static const
    std::vector <uintptr_t>& pairs =
      SK_CPU_GetLogicalCorePairs ();

  return
    GetLogicalProcessorInformation_Original ( Buffer, ReturnedLength );
}

BOOL
WINAPI
GetLogicalProcessorInformationEx_Detour (
  LOGICAL_PROCESSOR_RELATIONSHIP           RelationshipType,
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer,
  PDWORD                                   ReturnedLength )
{
  SK_LOG_FIRST_CALL

  static const
    int core_count =
      static_cast <int> (SK_CPU_CountPhysicalCores ());

  int extra_cores =
    config.render.framerate.override_num_cpus == -1 ? 0 :
    config.render.framerate.override_num_cpus - core_count;

  ////dll_log->Log (L"Allocating %lu extra CPU cores", extra_cores);

  if (extra_cores > 0 && RelationshipType == RelationAll)
  {
    auto getFakeProcessorCount = []() -> std::pair <char*, DWORD>
    {
      int extra_cores =
        config.render.framerate.override_num_cpus - core_count;

      DWORD len    = 0;
      char* buffer = NULL;

      if (FALSE == GetLogicalProcessorInformationEx_Original (RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer, &len))
      {
		    if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
        {
          size_t extraAlloc = 0;
          size_t extraCores = extra_cores;

			    buffer = (char *)malloc (len);

          if ( buffer != nullptr &&
               GetLogicalProcessorInformationEx_Original (
                 RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer,
                   &len )
             )
          {
				    char* ptr = buffer;

				    while (ptr != nullptr && ptr < buffer + len)
            {
					    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

              if (pi->Relationship == RelationProcessorCore)
              {
                while (extraCores > 0)
                {
                  extraCores--;
                  extraAlloc += pi->Size;
                }
					    }
					    ptr            += pi->Size;
				    }
			    }

          if (buffer != nullptr)
            free (buffer);

          buffer = (char *)malloc (len + extraAlloc);

          if ( buffer != nullptr &&
               GetLogicalProcessorInformationEx_Original (
                 RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer,
                   &len )
             )
          {
            char *cores_ = (char *)malloc (extraAlloc);

				    char* ptr = buffer;

            extraCores = extra_cores;

				    while (ptr != nullptr && ptr < buffer + len)
            {
					    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

              if (pi->Relationship == RelationProcessorCore)
              {
                char* pExtra =
                  cores_;

                while (extraCores > 0)
                {
                  memcpy (pExtra, pi, pi->Size);
                          pExtra +=   pi->Size;

                  extraCores--;
                }
					    }
					    ptr += pi->Size;
				    }

            memcpy (ptr, cores_, extraAlloc);

            len += (DWORD)extraAlloc;
          }

          return std::make_pair (buffer, len);
        }
      }

      return std::make_pair (nullptr, 0);
    };

    static std::pair <char *, DWORD> spoof =
      getFakeProcessorCount ();

    if ( ReturnedLength != nullptr && *ReturnedLength < spoof.second )
    {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);

      *ReturnedLength = spoof.second;

      return FALSE;
    }

    else if ( ReturnedLength != nullptr && *ReturnedLength >= spoof.second )
    {
      if (Buffer != nullptr)
      {
        memcpy (Buffer, spoof.first, spoof.second);
             *ReturnedLength =       spoof.second;

        int cores   = 0;
        int logical = 0;

        char*  ptr = (char *)Buffer;
				while (ptr < (char *)Buffer + spoof.second)
        {
				  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

          if (pi->Relationship == RelationProcessorCore)
          {
				  	cores++;

            for (size_t g = 0; g < pi->Processor.GroupCount; ++g)
            {
				  		logical +=
                CountSetBits (pi->Processor.GroupMask [g].Mask);
				  	}
				  }
				  ptr += pi->Size;
				}

        ////dll_log->Log (L"Returning %lu cores, %lu logical", cores, logical);
      }

      return TRUE;
    }
  }

  return
    GetLogicalProcessorInformationEx_Original ( RelationshipType, Buffer, ReturnedLength );
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
  SK_CreateDLLHook2 (      L"Kernel32",
                            "GetLogicalProcessorInformation",
                             GetLogicalProcessorInformation_Detour,
    static_cast_p2p <void> (&GetLogicalProcessorInformation_Original) );

  ///////SK_CreateDLLHook2 (      L"Kernel32",
  ///////                          "GetLogicalProcessorInformationEx",
  ///////                           GetLogicalProcessorInformationEx_Detour,
  ///////  static_cast_p2p <void> (&GetLogicalProcessorInformationEx_Original) );

  SK_CreateDLLHook2 (     L"Kernel32",
                            "GetSystemInfo",
                             GetSystemInfo_Detour,
    static_cast_p2p <void> (&GetSystemInfo_Original) );
}


size_t
SK_CPU_CountPhysicalCores (void)
{
  static std::set <ULONG_PTR> logical_proc_siblings;

  if (! logical_proc_siblings.empty ())
    return logical_proc_siblings.size ();

  DWORD dwNeededBytes = 0;

  SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pLogProcInfo = nullptr;

  // We're not hooking anything, so use the regular import
  if (! GetLogicalProcessorInformation_Original)
        GetLogicalProcessorInformation_Original = GetLogicalProcessorInformation;

  if (! GetLogicalProcessorInformation_Original (nullptr, &dwNeededBytes))
  {
    if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
    {
      try {
        pLogProcInfo = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)
          new uint8_t [dwNeededBytes];
      }

      catch (const std::bad_alloc&)
      {
                          pLogProcInfo  = nullptr;
        SK_ReleaseAssert (pLogProcInfo != nullptr);
      }

      if (pLogProcInfo != nullptr)
      {
        if (GetLogicalProcessorInformation_Original (pLogProcInfo, &dwNeededBytes))
        {
          PSYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi =
            pLogProcInfo;

          DWORD  dwOffset = 0;
          while (dwOffset + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= dwNeededBytes)
          {
            switch (lpi->Relationship)
            {
              case RelationProcessorCore:
              {
                logical_proc_siblings.emplace (lpi->ProcessorMask);
              } break;

              default:
                break;
            }

            dwOffset +=
              sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            lpi++;
          }
        }

        delete [] (uint8_t *)pLogProcInfo;
      }
    }
  }

  return
    logical_proc_siblings.size ();
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
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION *pLogProcInfo =
       (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)
                              new uint8_t [dwNeededBytes] { };

      if (GetLogicalProcessorInformation_Original (pLogProcInfo, &dwNeededBytes))
      {
        DWORD                                 dwOffset = 0;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi      = pLogProcInfo;

        while (dwOffset + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= dwNeededBytes)
        {
          switch (lpi->Relationship)
          {
            case RelationProcessorCore:
            {
#if defined (_WIN64) && defined (_DEBUG)
              // Don't want to bother with server hardware! No gaming machine needs this many cores (2018)
              ////assert ((lpi->ProcessorMask & 0xFFFFFFFF00000000) == 0);
#endif
              logical_proc_siblings.push_back (lpi->ProcessorMask);
            } break;

            default:
              break;
          }

          dwOffset +=
            sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
          lpi++;
        }
      }

      delete [] pLogProcInfo;
    }
  }

  return logical_proc_siblings;
}

size_t
SK_CPU_CountLogicalCores (void)
{
  static size_t
    logical_cores = 0;
  static BOOL
    has_logical_processors = 2;

  if (has_logical_processors == 2)
  {
    auto& pairs =
      SK_CPU_GetLogicalCorePairs ();

    for ( auto it : pairs )
    {
      int cores_in_set =
        CountSetBits (it);

      if (cores_in_set > 1)
      {
        has_logical_processors = TRUE;
        logical_cores += cores_in_set;
      }
    }

    if (has_logical_processors == 2)
    {
      logical_cores          = 0;
      has_logical_processors = FALSE;
    }
  }

  if ( has_logical_processors )
    return logical_cores;

  return 0;
}

void
SK_FPU_LogPrecision (void)
{
  UINT x87_control_word = 0x0,
      sse2_control_word = 0x0;

#ifdef _M_IX86
  __control87_2 ( 0, 0, &x87_control_word,
                       &sse2_control_word );
#else
  x87_control_word  =
    _control87 (0, 0);
  sse2_control_word =
   x87_control_word;
#endif

  auto _LogPrecision = [&](UINT cw, LPCWSTR name)
  {
    SK_LOG0 ( ( L" %s FPU Control Word: %s", name,
                  (cw & _PC_24) == _PC_24 ? L"Single Precision (24-bit)" :
                  (cw & _PC_53) == _PC_53 ? L"Double Precision (53-bit)" :
                  (cw & _PC_64) == _PC_64 ? L"Double Extended Precision (64-bit)" :
                      SK_FormatStringW ( L"Unknown (cw=%x)",
                                    cw ).c_str ()
              ), L" FPU Mode " );
  };

  _LogPrecision (x87_control_word,  L" x87");
  _LogPrecision (sse2_control_word, L"SSE2");
}

SK_FPU_ControlWord
SK_FPU_SetControlWord (UINT mask, SK_FPU_ControlWord *pNewControl)
{
  SK_FPU_ControlWord orig_cw = { };

  #ifdef _M_IX86
  __control87_2 (0, 0, &orig_cw.x87, &orig_cw.sse2);
  __control87_2 (pNewControl->x87,  mask, &pNewControl->x87, nullptr);
  __control87_2 (pNewControl->sse2, mask, nullptr,           &pNewControl->sse2);
#else
  orig_cw.sse2 = _control87 (0, 0);
  orig_cw.x87  = orig_cw.sse2;
  _control87 (pNewControl->x87, mask);
#endif

  return orig_cw;
};

SK_FPU_ControlWord
SK_FPU_SetPrecision (UINT precision)
{
  SK_FPU_ControlWord cw_to_set = {
    precision, precision
  };

  return
    SK_FPU_SetControlWord (_MCW_PC, &cw_to_set);
};
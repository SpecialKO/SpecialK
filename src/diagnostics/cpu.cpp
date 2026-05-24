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

#include <SpecialK/stdafx.h>

#include <SpecialK/diagnostics/cpu.h>

#include <TlHelp32.h>

typedef void (WINAPI *GetSystemInfo_pfn)(LPSYSTEM_INFO);
                      GetSystemInfo_pfn
                      GetSystemInfo_Original = nullptr;

typedef void (WINAPI *GetNativeSystemInfo_pfn)(LPSYSTEM_INFO);
                      GetNativeSystemInfo_pfn
                      GetNativeSystemInfo_Original = nullptr;

typedef BOOL (WINAPI *GetLogicalProcessorInformation_pfn)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);
                      GetLogicalProcessorInformation_pfn
                      GetLogicalProcessorInformation_Original = nullptr;

typedef BOOL (WINAPI *GetLogicalProcessorInformationEx_pfn)(LOGICAL_PROCESSOR_RELATIONSHIP,PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,PDWORD);
                       GetLogicalProcessorInformationEx_pfn
                       GetLogicalProcessorInformationEx_Original = nullptr;

typedef DWORD (WINAPI *GetActiveProcessorCount_pfn)(WORD);
                       GetActiveProcessorCount_pfn
                       GetActiveProcessorCount_Original = nullptr;

typedef WORD (WINAPI *GetActiveProcessorGroupCount_pfn)(void);
                      GetActiveProcessorGroupCount_pfn
                      GetActiveProcessorGroupCount_Original = nullptr;

typedef DWORD (WINAPI *GetMaximumProcessorCount_pfn)(WORD);
                       GetMaximumProcessorCount_pfn
                       GetMaximumProcessorCount_Original = nullptr;

typedef WORD (WINAPI *GetMaximumProcessorGroupCount_pfn)(void);
                      GetMaximumProcessorGroupCount_pfn
                      GetMaximumProcessorGroupCount_Original = nullptr;

typedef BOOL (WINAPI *GetProcessAffinityMask_pfn)(HANDLE,PDWORD_PTR,PDWORD_PTR);
                       GetProcessAffinityMask_pfn
                       GetProcessAffinityMask_Original = nullptr;

typedef VOID (WINAPI *SetThreadpoolThreadMaximum_pfn)(PTP_POOL,DWORD);
                       SetThreadpoolThreadMaximum_pfn
                       SetThreadpoolThreadMaximum_Original = nullptr;

typedef BOOL (WINAPI *SetThreadpoolThreadMinimum_pfn)(PTP_POOL,DWORD);
                       SetThreadpoolThreadMinimum_pfn
                       SetThreadpoolThreadMinimum_Original = nullptr;

typedef unsigned int (__cdecl *MSVCP_Thrd_hardware_concurrency_pfn)(void);
                              MSVCP_Thrd_hardware_concurrency_pfn
                              MSVCP_Thrd_hardware_concurrency_Original = nullptr;

typedef FARPROC (WINAPI *SK_CPU_Early_GetProcAddress_pfn)(HMODULE,LPCSTR);
typedef HMODULE (WINAPI *SK_CPU_Early_LoadLibraryW_pfn)(LPCWSTR);
typedef HMODULE (WINAPI *SK_CPU_Early_LoadLibraryA_pfn)(LPCSTR);
typedef HMODULE (WINAPI *SK_CPU_Early_LoadLibraryExW_pfn)(LPCWSTR,HANDLE,DWORD);
typedef HMODULE (WINAPI *SK_CPU_Early_LoadLibraryExA_pfn)(LPCSTR,HANDLE,DWORD);

BOOL  WINAPI GetLogicalProcessorInformation_Detour   (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);
BOOL  WINAPI GetLogicalProcessorInformationEx_Detour (LOGICAL_PROCESSOR_RELATIONSHIP,PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,PDWORD);
void  WINAPI GetSystemInfo_Detour                    (LPSYSTEM_INFO);
void  WINAPI GetNativeSystemInfo_Detour              (LPSYSTEM_INFO);
DWORD WINAPI GetActiveProcessorCount_Detour          (WORD);
WORD  WINAPI GetActiveProcessorGroupCount_Detour     (void);
DWORD WINAPI GetMaximumProcessorCount_Detour         (WORD);
WORD  WINAPI GetMaximumProcessorGroupCount_Detour    (void);
BOOL  WINAPI GetProcessAffinityMask_Detour           (HANDLE,PDWORD_PTR,PDWORD_PTR);

size_t
SK_CPU_CountPhysicalCores (void);

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  CPUMgr  "

#ifndef LTP_PC_SMT
#define LTP_PC_SMT 0x1
#endif

#ifndef ALL_PROCESSOR_GROUPS
#define ALL_PROCESSOR_GROUPS 0xffff
#endif

namespace
{
  SK_CPU_Early_GetProcAddress_pfn
    SK_CPU_Early_GetProcAddress_Original = nullptr;

  SK_CPU_Early_LoadLibraryW_pfn
    SK_CPU_Early_LoadLibraryW_Original = nullptr;

  SK_CPU_Early_LoadLibraryA_pfn
    SK_CPU_Early_LoadLibraryA_Original = nullptr;

  SK_CPU_Early_LoadLibraryExW_pfn
    SK_CPU_Early_LoadLibraryExW_Original = nullptr;

  SK_CPU_Early_LoadLibraryExA_pfn
    SK_CPU_Early_LoadLibraryExA_Original = nullptr;

  volatile LONG
    SK_CPU_EarlyTopologySpoof_Patching = FALSE;

  wchar_t
    SK_CPU_EarlyTopologySpoof_WindowsDir [MAX_PATH] = { };

  constexpr DWORD
  SK_CPU_Spoof_MaxLogicalProcessors (void)
  {
    return static_cast <DWORD> (sizeof (ULONG_PTR) * 8);
  }

  bool
  SK_CPU_Spoof_IsEnabled (void)
  {
    return
      config.priority.cpu_spoof_enable &&
      ( config.priority.cpu_spoof_logical  > 0 ||
        config.priority.cpu_spoof_physical > 0 );
  }

  DWORD
  SK_CPU_Spoof_LogicalProcessors (void)
  {
    int logical =
      config.priority.cpu_spoof_logical;

    if (logical <= 0)
      logical =
        config.priority.cpu_spoof_physical;

    logical =
      std::clamp ( logical, 1,
        static_cast <int> (SK_CPU_Spoof_MaxLogicalProcessors ()) );

    return
      static_cast <DWORD> (logical);
  }

  DWORD
  SK_CPU_Spoof_PhysicalCores (void)
  {
    int physical =
      config.priority.cpu_spoof_physical;

    if (physical <= 0)
      physical =
        config.priority.cpu_spoof_logical;

    physical =
      std::clamp ( physical, 1,
        static_cast <int> (SK_CPU_Spoof_LogicalProcessors ()) );

    return
      static_cast <DWORD> (physical);
  }

  KAFFINITY
  SK_CPU_Spoof_MaskForCount (DWORD count)
  {
    count =
      std::min (count, SK_CPU_Spoof_MaxLogicalProcessors ());

    if (count == 0)
      return 0;

    if (count >= SK_CPU_Spoof_MaxLogicalProcessors ())
      return
        ~static_cast <KAFFINITY> (0);

    return
      (static_cast <KAFFINITY> (1) << count) - 1;
  }

  void
  SK_CPU_Spoof_LogCall (
    const wchar_t* api,
    LPVOID         caller,
    const wchar_t* details = L"" )
  {
    if (! SK_CPU_Spoof_IsEnabled ())
      return;

    if (! dll_log.isAllocated ())
      return;

    const HMODULE calling_module =
      SK_GetCallingDLL (caller);

    if (calling_module == nullptr || calling_module == SK_GetDLL ())
      return;

    static volatile LONG calls = 0;

    if (InterlockedIncrement (&calls) > 128)
      return;

    SK_LOGi0 (
      L"CPU topology spoof: %ws => logical=%lu, physical=%lu%ws%ws [caller=%ws]",
      api,
      SK_CPU_Spoof_LogicalProcessors (),
      SK_CPU_Spoof_PhysicalCores     (),
      (details != nullptr && *details != L'\0') ? L", " : L"",
      (details != nullptr) ? details : L"",
      SK_SummarizeCaller (caller).c_str ()
    );
  }

  std::vector <KAFFINITY>
  SK_CPU_Spoof_CoreMasks (void)
  {
    std::vector <KAFFINITY> masks;

    const DWORD logical =
      SK_CPU_Spoof_LogicalProcessors ();

    const DWORD physical =
      SK_CPU_Spoof_PhysicalCores ();

    const DWORD base_threads  = logical / physical;
    const DWORD extra_threads = logical % physical;

    DWORD bit = 0;

    masks.reserve (physical);

    for (DWORD core = 0; core < physical; ++core)
    {
      const DWORD threads =
        std::max (1UL, base_threads + (core < extra_threads ? 1UL : 0UL));

      KAFFINITY mask = 0;

      for (DWORD thread = 0; thread < threads && bit < logical; ++thread, ++bit)
      {
        mask |=
          static_cast <KAFFINITY> (1) << bit;
      }

      masks.emplace_back (mask);
    }

    return masks;
  }

  void
  SK_CPU_Spoof_AppendProcessorRelationship (
          std::vector <BYTE>&             out,
          LOGICAL_PROCESSOR_RELATIONSHIP  relationship,
          KAFFINITY                       mask )
  {
    const DWORD size =
      FIELD_OFFSET ( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
                     Processor.GroupMask ) + sizeof (GROUP_AFFINITY);

    std::vector <BYTE> record (size, 0);

    auto* cpu_info =
      reinterpret_cast <PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX> (
        record.data ()
      );

    cpu_info->Relationship             = relationship;
    cpu_info->Size                     = size;
    cpu_info->Processor.Flags          =
      ((mask & (mask - 1)) != 0) ? LTP_PC_SMT : 0;
    cpu_info->Processor.EfficiencyClass = 0;
    cpu_info->Processor.GroupCount     = 1;
    cpu_info->Processor.GroupMask [0].Mask  = mask;
    cpu_info->Processor.GroupMask [0].Group = 0;

    out.insert (out.end (), record.begin (), record.end ());
  }

  void
  SK_CPU_Spoof_AppendNumaRelationship (std::vector <BYTE>& out)
  {
    const DWORD size =
      FIELD_OFFSET ( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
                     NumaNode.GroupMask ) + sizeof (GROUP_AFFINITY);

    std::vector <BYTE> record (size, 0);

    auto* cpu_info =
      reinterpret_cast <PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX> (
        record.data ()
      );

    cpu_info->Relationship          = RelationNumaNode;
    cpu_info->Size                  = size;
    cpu_info->NumaNode.NodeNumber   = 0;
    cpu_info->NumaNode.GroupMask.Mask  =
      SK_CPU_Spoof_MaskForCount (SK_CPU_Spoof_LogicalProcessors ());
    cpu_info->NumaNode.GroupMask.Group = 0;

    out.insert (out.end (), record.begin (), record.end ());
  }

  void
  SK_CPU_Spoof_AppendGroupRelationship (std::vector <BYTE>& out)
  {
    const DWORD size =
      FIELD_OFFSET ( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
                     Group.GroupInfo ) + sizeof (PROCESSOR_GROUP_INFO);

    std::vector <BYTE> record (size, 0);

    auto* cpu_info =
      reinterpret_cast <PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX> (
        record.data ()
      );

    const DWORD logical =
      SK_CPU_Spoof_LogicalProcessors ();

    cpu_info->Relationship                           = RelationGroup;
    cpu_info->Size                                   = size;
    cpu_info->Group.MaximumGroupCount                = 1;
    cpu_info->Group.ActiveGroupCount                 = 1;
    cpu_info->Group.GroupInfo [0].MaximumProcessorCount =
      static_cast <BYTE> (logical);
    cpu_info->Group.GroupInfo [0].ActiveProcessorCount  =
      static_cast <BYTE> (logical);
    cpu_info->Group.GroupInfo [0].ActiveProcessorMask   =
      SK_CPU_Spoof_MaskForCount (logical);

    out.insert (out.end (), record.begin (), record.end ());
  }

  std::vector <BYTE>
  SK_CPU_Spoof_BuildProcessorInformationEx (
    LOGICAL_PROCESSOR_RELATIONSHIP relationship )
  {
    std::vector <BYTE> out;

    if ( relationship != RelationProcessorCore    &&
         relationship != RelationProcessorPackage &&
         relationship != RelationNumaNode         &&
         relationship != RelationGroup            &&
         relationship != RelationAll )
    {
      return out;
    }

    const KAFFINITY full_mask =
      SK_CPU_Spoof_MaskForCount (SK_CPU_Spoof_LogicalProcessors ());

    if ( relationship == RelationProcessorCore ||
         relationship == RelationAll )
    {
      for (const KAFFINITY mask : SK_CPU_Spoof_CoreMasks ())
        SK_CPU_Spoof_AppendProcessorRelationship (
          out, RelationProcessorCore, mask
        );
    }

    if ( relationship == RelationNumaNode ||
         relationship == RelationAll )
    {
      SK_CPU_Spoof_AppendNumaRelationship (out);
    }

    if ( relationship == RelationProcessorPackage ||
         relationship == RelationAll )
    {
      SK_CPU_Spoof_AppendProcessorRelationship (
        out, RelationProcessorPackage, full_mask
      );
    }

    if ( relationship == RelationGroup ||
         relationship == RelationAll )
    {
      SK_CPU_Spoof_AppendGroupRelationship (out);
    }

    return out;
  }

  BOOL
  SK_CPU_Spoof_CopyBuffer (
          const std::vector <BYTE>& data,
          void*                     buffer,
          PDWORD                    returned_length )
  {
    if (returned_length == nullptr)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    const DWORD required =
      static_cast <DWORD> (data.size ());

    if (buffer == nullptr || *returned_length < required)
    {
      *returned_length = required;
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
    }

    memcpy (buffer, data.data (), required);

    *returned_length = required;

    return TRUE;
  }

  BOOL
  SK_CPU_Spoof_CopyProcessorInformation (
          PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer,
          PDWORD                                returned_length )
  {
    if (returned_length == nullptr)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    const auto masks =
      SK_CPU_Spoof_CoreMasks ();

    const DWORD record_count =
      static_cast <DWORD> (masks.size () + 2);

    const DWORD required =
      record_count * sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

    if (buffer == nullptr || *returned_length < required)
    {
      *returned_length = required;
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
    }

    memset (buffer, 0, required);

    for (size_t i = 0; i < masks.size (); ++i)
    {
      buffer [i].ProcessorMask        = masks [i];
      buffer [i].Relationship         = RelationProcessorCore;
      buffer [i].ProcessorCore.Flags  =
        ((masks [i] & (masks [i] - 1)) != 0) ? LTP_PC_SMT : 0;
    }

    const KAFFINITY full_mask =
      SK_CPU_Spoof_MaskForCount (SK_CPU_Spoof_LogicalProcessors ());

    buffer [masks.size ()].ProcessorMask       = full_mask;
    buffer [masks.size ()].Relationship        = RelationNumaNode;
    buffer [masks.size ()].NumaNode.NodeNumber = 0;

    buffer [masks.size () + 1].ProcessorMask = full_mask;
    buffer [masks.size () + 1].Relationship  = RelationProcessorPackage;

    *returned_length = required;

    return TRUE;
  }

  void
  SK_CPU_Spoof_ApplySystemInfo (LPSYSTEM_INFO info)
  {
    if (info == nullptr || ! SK_CPU_Spoof_IsEnabled ())
      return;

    info->dwActiveProcessorMask =
      SK_CPU_Spoof_MaskForCount (SK_CPU_Spoof_LogicalProcessors ());

    info->dwNumberOfProcessors =
      SK_CPU_Spoof_LogicalProcessors ();
  }

  bool
  SK_CPU_Spoof_IsGroupZeroOrAll (WORD group_number)
  {
    return
      group_number == 0 || group_number == ALL_PROCESSOR_GROUPS;
  }

  bool
  SK_CPU_HookTargetAlreadyQueued (
    const std::vector <void*>& targets,
    void*                     target )
  {
    return
      std::find (targets.begin (), targets.end (), target) != targets.end ();
  }

  MH_STATUS
  SK_CPU_CreateDLLHook2Unique (
    const wchar_t*        module,
    const char*           proc_name,
    void*                 detour,
    void**                original,
    std::vector <void*>&  targets )
  {
    void* target =
      reinterpret_cast <void*> (SK_GetProcAddress (module, proc_name));

    if (target == nullptr)
      return MH_ERROR_FUNCTION_NOT_FOUND;

    if (SK_CPU_HookTargetAlreadyQueued (targets, target))
      return MH_OK;

    void* hooked_target = nullptr;

    MH_STATUS status =
      SK_CreateDLLHook2 (module, proc_name, detour, original, &hooked_target);

    if (status == MH_OK && hooked_target != nullptr)
      targets.emplace_back (hooked_target);

    return status;
  }

  bool
  SK_CPU_EarlyTopologySpoof_ParseBool (const wchar_t* value)
  {
    return
      value != nullptr &&
      ( _wcsicmp (value, L"true") == 0 ||
        _wcsicmp (value, L"yes")  == 0 ||
        _wcsicmp (value, L"on")   == 0 ||
        wcscmp   (value, L"1")    == 0 );
  }

  bool
  SK_CPU_EarlyTopologySpoof_ReadConfig (const wchar_t* ini_path)
  {
    if (ini_path == nullptr || *ini_path == L'\0')
      return false;

    if (GetFileAttributesW (ini_path) == INVALID_FILE_ATTRIBUTES)
      return false;

    wchar_t enabled [32] = { };

    GetPrivateProfileStringW (
      L"Scheduler.CPUSpoof", L"Enable", L"",
        enabled, static_cast <DWORD> (std::size (enabled)), ini_path
    );

    if (! SK_CPU_EarlyTopologySpoof_ParseBool (enabled))
      return false;

    int logical =
      GetPrivateProfileIntW (
        L"Scheduler.CPUSpoof", L"LogicalProcessors", 0, ini_path
      );

    int physical =
      GetPrivateProfileIntW (
        L"Scheduler.CPUSpoof", L"PhysicalCores", 0, ini_path
      );

    if (logical <= 0 && physical <= 0)
      return false;

    if (logical <= 0)
      logical = physical;

    if (physical <= 0)
      physical = logical;

    logical =
      std::clamp (logical, 1,
        static_cast <int> (SK_CPU_Spoof_MaxLogicalProcessors ()));

    physical =
      std::clamp (physical, 1, logical);

    config.priority.cpu_spoof_enable   = true;
    config.priority.cpu_spoof_logical  = logical;
    config.priority.cpu_spoof_physical = physical;

    return true;
  }

  bool
  SK_CPU_EarlyTopologySpoof_BuildIniPath (
          const wchar_t* ini_name,
          wchar_t*       ini_path,
          size_t         ini_path_count )
  {
    const wchar_t* config_path =
      SK_GetConfigPath ();

    if ( config_path == nullptr || *config_path == L'\0' ||
         ini_name    == nullptr || *ini_name    == L'\0' ||
         ini_path    == nullptr ||  ini_path_count == 0 )
    {
      return false;
    }

    if (wcscpy_s (ini_path, ini_path_count, config_path) != 0)
      return false;

    const size_t len =
      wcslen (ini_path);

    if ( len > 0 &&
         ini_path [len - 1] != L'\\' &&
         ini_path [len - 1] != L'/' )
    {
      if (wcscat_s (ini_path, ini_path_count, L"\\") != 0)
        return false;
    }

    return
      wcscat_s (ini_path, ini_path_count, ini_name) == 0;
  }

  bool
  SK_CPU_EarlyTopologySpoof_BuildProfileIniPath (
          const wchar_t* profile_name,
          wchar_t*       ini_path,
          size_t         ini_path_count )
  {
    const wchar_t* install_path =
      SK_GetInstallPath ();

    if ( install_path  == nullptr || *install_path  == L'\0' ||
         profile_name  == nullptr || *profile_name  == L'\0' ||
         ini_path      == nullptr ||  ini_path_count == 0 )
    {
      return false;
    }

    if (wcscpy_s (ini_path, ini_path_count, install_path) != 0)
      return false;

    const size_t len =
      wcslen (ini_path);

    if ( len > 0 &&
         ini_path [len - 1] != L'\\' &&
         ini_path [len - 1] != L'/' )
    {
      if (wcscat_s (ini_path, ini_path_count, L"\\") != 0)
        return false;
    }

    return
      wcscat_s (ini_path, ini_path_count, L"Profiles\\") == 0 &&
      wcscat_s (ini_path, ini_path_count, profile_name)   == 0 &&
      wcscat_s (ini_path, ini_path_count, L"\\SpecialK.ini") == 0;
  }

  bool
  SK_CPU_EarlyTopologySpoof_ReadRegistryProfileConfig (void)
  {
    wchar_t app_path [MAX_PATH * 2] = { };

    if (GetModuleFileNameW (nullptr, app_path, MAX_PATH * 2) == 0)
      return false;

    HKEY profile_key = nullptr;

    if ( RegOpenKeyExW ( HKEY_CURRENT_USER,
                         LR"(Software\Kaldaien\Special K\Profiles)",
                         0, KEY_READ, &profile_key ) != ERROR_SUCCESS )
    {
      return false;
    }

    bool found = false;

    wchar_t query_path [MAX_PATH * 2] = { };
    wcscpy_s (query_path, std::size (query_path), app_path);

    while (*query_path != L'\0')
    {
      wchar_t profile_name [MAX_PATH + 2] = { };
      DWORD   profile_type                = REG_SZ;
      DWORD   profile_bytes               = sizeof (profile_name);

      if ( RegQueryValueExW ( profile_key, query_path, nullptr,
                              &profile_type,
                              reinterpret_cast <LPBYTE> (profile_name),
                              &profile_bytes ) == ERROR_SUCCESS &&
           profile_type == REG_SZ &&
           profile_name [0] != L'\0' )
      {
        wchar_t ini_path [MAX_PATH * 2] = { };

        found =
          SK_CPU_EarlyTopologySpoof_BuildProfileIniPath (
            profile_name, ini_path, std::size (ini_path)
          ) &&
          SK_CPU_EarlyTopologySpoof_ReadConfig (ini_path);

        break;
      }

      wchar_t* slash =
        wcsrchr (query_path, L'\\');

      if (slash == nullptr)
        break;

      *slash = L'\0';
    }

    RegCloseKey (profile_key);

    return found;
  }

  bool
  SK_CPU_EarlyTopologySpoof_ReadProfileConfig (void)
  {
    wchar_t ini_path [MAX_PATH * 2] = { };

    if ( SK_CPU_EarlyTopologySpoof_BuildIniPath (
           L"SpecialK.ini", ini_path, std::size (ini_path)
         ) &&
         SK_CPU_EarlyTopologySpoof_ReadConfig (ini_path) )
    {
      return true;
    }

    if (SK_CPU_EarlyTopologySpoof_ReadRegistryProfileConfig ())
      return true;

    wchar_t self_path [MAX_PATH + 2] = { };

    if (GetModuleFileNameW (SK_GetDLL (), self_path, MAX_PATH) == 0)
      return false;

    const wchar_t* filename =
      wcsrchr (self_path, L'\\');

    filename =
      (filename != nullptr) ? filename + 1 : self_path;

    wchar_t module_ini [MAX_PATH + 2] = { };

    if (wcscpy_s (module_ini, std::size (module_ini), filename) != 0)
      return false;

    if (wchar_t* ext = wcsrchr (module_ini, L'.'); ext != nullptr)
      *ext = L'\0';

    if ( module_ini [0] == L'\0' ||
         _wcsicmp (module_ini, L"SpecialK64") == 0 ||
         _wcsicmp (module_ini, L"SpecialK32") == 0 )
    {
      return false;
    }

    if (wcscat_s (module_ini, std::size (module_ini), L".ini") != 0)
      return false;

    RtlZeroMemory (ini_path, sizeof (ini_path));

    return
      SK_CPU_EarlyTopologySpoof_BuildIniPath (
        module_ini, ini_path, std::size (ini_path)
      ) &&
      SK_CPU_EarlyTopologySpoof_ReadConfig (ini_path);
  }

  bool
  SK_CPU_EarlyTopologySpoof_PatchLoadedModules (void);

  FARPROC WINAPI
  SK_CPU_EarlyTopologySpoof_GetProcAddress_Detour (
    HMODULE hModule,
    LPCSTR  lpProcName );

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryW_Detour (LPCWSTR lpLibFileName);

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryA_Detour (LPCSTR lpLibFileName);

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryExW_Detour (
    LPCWSTR lpLibFileName,
    HANDLE  hFile,
    DWORD   dwFlags );

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryExA_Detour (
    LPCSTR lpLibFileName,
    HANDLE hFile,
    DWORD  dwFlags );

  void*
  SK_CPU_EarlyTopologySpoof_DetourForName (
          const char*  import_name,
          void***      original )
  {
    if (import_name == nullptr || original == nullptr)
      return nullptr;

    if (_stricmp (import_name, "GetLogicalProcessorInformationEx") == 0)
    {
      *original = reinterpret_cast <void**> (
        &GetLogicalProcessorInformationEx_Original
      );
      return reinterpret_cast <void*> (
        GetLogicalProcessorInformationEx_Detour
      );
    }

    if (_stricmp (import_name, "GetLogicalProcessorInformation") == 0)
    {
      *original = reinterpret_cast <void**> (
        &GetLogicalProcessorInformation_Original
      );
      return reinterpret_cast <void*> (
        GetLogicalProcessorInformation_Detour
      );
    }

    if (_stricmp (import_name, "GetActiveProcessorCount") == 0)
    {
      *original =
        reinterpret_cast <void**> (&GetActiveProcessorCount_Original);
      return reinterpret_cast <void*> (GetActiveProcessorCount_Detour);
    }

    if (_stricmp (import_name, "GetActiveProcessorGroupCount") == 0)
    {
      *original =
        reinterpret_cast <void**> (&GetActiveProcessorGroupCount_Original);
      return reinterpret_cast <void*> (GetActiveProcessorGroupCount_Detour);
    }

    if (_stricmp (import_name, "GetMaximumProcessorCount") == 0)
    {
      *original =
        reinterpret_cast <void**> (&GetMaximumProcessorCount_Original);
      return reinterpret_cast <void*> (GetMaximumProcessorCount_Detour);
    }

    if (_stricmp (import_name, "GetMaximumProcessorGroupCount") == 0)
    {
      *original =
        reinterpret_cast <void**> (&GetMaximumProcessorGroupCount_Original);
      return reinterpret_cast <void*> (GetMaximumProcessorGroupCount_Detour);
    }

    if (_stricmp (import_name, "GetSystemInfo") == 0)
    {
      *original = reinterpret_cast <void**> (&GetSystemInfo_Original);
      return reinterpret_cast <void*> (GetSystemInfo_Detour);
    }

    if (_stricmp (import_name, "GetNativeSystemInfo") == 0)
    {
      *original = reinterpret_cast <void**> (&GetNativeSystemInfo_Original);
      return reinterpret_cast <void*> (GetNativeSystemInfo_Detour);
    }

    if (_stricmp (import_name, "GetProcAddress") == 0)
    {
      *original =
        reinterpret_cast <void**> (&SK_CPU_Early_GetProcAddress_Original);
      return reinterpret_cast <void*> (
        SK_CPU_EarlyTopologySpoof_GetProcAddress_Detour
      );
    }

    if (_stricmp (import_name, "LoadLibraryW") == 0)
    {
      *original =
        reinterpret_cast <void**> (&SK_CPU_Early_LoadLibraryW_Original);
      return reinterpret_cast <void*> (
        SK_CPU_EarlyTopologySpoof_LoadLibraryW_Detour
      );
    }

    if (_stricmp (import_name, "LoadLibraryA") == 0)
    {
      *original =
        reinterpret_cast <void**> (&SK_CPU_Early_LoadLibraryA_Original);
      return reinterpret_cast <void*> (
        SK_CPU_EarlyTopologySpoof_LoadLibraryA_Detour
      );
    }

    if (_stricmp (import_name, "LoadLibraryExW") == 0)
    {
      *original =
        reinterpret_cast <void**> (&SK_CPU_Early_LoadLibraryExW_Original);
      return reinterpret_cast <void*> (
        SK_CPU_EarlyTopologySpoof_LoadLibraryExW_Detour
      );
    }

    if (_stricmp (import_name, "LoadLibraryExA") == 0)
    {
      *original =
        reinterpret_cast <void**> (&SK_CPU_Early_LoadLibraryExA_Original);
      return reinterpret_cast <void*> (
        SK_CPU_EarlyTopologySpoof_LoadLibraryExA_Detour
      );
    }

    return nullptr;
  }

  bool
  SK_CPU_EarlyTopologySpoof_PatchPointer (
          void** slot,
          void*  replacement,
          void** original )
  {
    if (slot == nullptr || replacement == nullptr || *slot == replacement)
      return false;

    DWORD old_protect = 0;

    if (! VirtualProtect (slot, sizeof (void*), PAGE_READWRITE, &old_protect))
      return false;

    if (original != nullptr && *original == nullptr)
      *original = *slot;

    *slot = replacement;

    FlushInstructionCache (GetCurrentProcess (), slot, sizeof (void*));

    DWORD ignored = 0;
    VirtualProtect (slot, sizeof (void*), old_protect, &ignored);

    return true;
  }

  bool
  SK_CPU_EarlyTopologySpoof_PatchModuleIAT (HMODULE module)
  {
    if (module == nullptr)
      return false;

    auto* base =
      reinterpret_cast <BYTE*> (module);

    auto* dos =
      reinterpret_cast <IMAGE_DOS_HEADER*> (base);

    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
      return false;

    auto* nt =
      reinterpret_cast <IMAGE_NT_HEADERS*> (base + dos->e_lfanew);

    if (nt->Signature != IMAGE_NT_SIGNATURE)
      return false;

    const auto& import_dir =
      nt->OptionalHeader.DataDirectory [IMAGE_DIRECTORY_ENTRY_IMPORT];

    if (import_dir.VirtualAddress == 0 || import_dir.Size == 0)
      return false;

    bool patched = false;

    auto* desc =
      reinterpret_cast <IMAGE_IMPORT_DESCRIPTOR*> (
        base + import_dir.VirtualAddress
      );

    for (; desc->Name != 0; ++desc)
    {
      if (desc->FirstThunk == 0 || desc->OriginalFirstThunk == 0)
        continue;

      auto* thunk =
        reinterpret_cast <IMAGE_THUNK_DATA*> (base + desc->FirstThunk);

      auto* orig =
        reinterpret_cast <IMAGE_THUNK_DATA*> (base + desc->OriginalFirstThunk);

      for (; orig->u1.AddressOfData != 0 && thunk->u1.Function != 0;
             ++orig, ++thunk)
      {
        if (IMAGE_SNAP_BY_ORDINAL (orig->u1.Ordinal))
          continue;

        auto* by_name =
          reinterpret_cast <IMAGE_IMPORT_BY_NAME*> (
            base + orig->u1.AddressOfData
          );

        void** original = nullptr;
        void*  detour   =
          SK_CPU_EarlyTopologySpoof_DetourForName (
            reinterpret_cast <const char*> (by_name->Name),
            &original
          );

        if (detour == nullptr)
          continue;

        patched |=
          SK_CPU_EarlyTopologySpoof_PatchPointer (
            reinterpret_cast <void**> (&thunk->u1.Function),
            detour,
            original
          );
      }
    }

    return patched;
  }

  bool
  SK_CPU_EarlyTopologySpoof_IsWindowsModule (const wchar_t* path)
  {
    if ( path == nullptr || *path == L'\0' ||
         SK_CPU_EarlyTopologySpoof_WindowsDir [0] == L'\0' )
    {
      return false;
    }

    const size_t len =
      wcslen (SK_CPU_EarlyTopologySpoof_WindowsDir);

    return
      _wcsnicmp (path, SK_CPU_EarlyTopologySpoof_WindowsDir, len) == 0 &&
      ( path [len] == L'\\' ||
        path [len] == L'/'  ||
        path [len] == L'\0' );
  }

  bool
  SK_CPU_EarlyTopologySpoof_PatchLoadedModules (void)
  {
    if (InterlockedExchange (&SK_CPU_EarlyTopologySpoof_Patching, TRUE) != FALSE)
      return false;

    bool patched = false;

    HANDLE snapshot =
      CreateToolhelp32Snapshot (
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        GetCurrentProcessId ()
      );

    if (snapshot != INVALID_HANDLE_VALUE)
    {
      MODULEENTRY32W entry = { };
      entry.dwSize = sizeof (entry);

      if (Module32FirstW (snapshot, &entry))
      {
        do
        {
          if ( entry.hModule != SK_GetDLL () &&
              !SK_CPU_EarlyTopologySpoof_IsWindowsModule (entry.szExePath) )
          {
            patched |=
              SK_CPU_EarlyTopologySpoof_PatchModuleIAT (entry.hModule);
          }
        } while (Module32NextW (snapshot, &entry));
      }

      CloseHandle (snapshot);
    }

    InterlockedExchange (&SK_CPU_EarlyTopologySpoof_Patching, FALSE);

    return patched;
  }

  void
  SK_CPU_EarlyTopologySpoof_ResolveOriginals (void)
  {
    HMODULE kernel32 =
      GetModuleHandleW (L"kernel32.dll");

    if (kernel32 == nullptr)
      return;

    auto resolve =
      [kernel32](const char* proc_name) -> FARPROC
      {
        return
          ::GetProcAddress (kernel32, proc_name);
      };

    if (GetLogicalProcessorInformationEx_Original == nullptr)
      GetLogicalProcessorInformationEx_Original =
        reinterpret_cast <GetLogicalProcessorInformationEx_pfn> (
          resolve ("GetLogicalProcessorInformationEx")
        );

    if (GetLogicalProcessorInformation_Original == nullptr)
      GetLogicalProcessorInformation_Original =
        reinterpret_cast <GetLogicalProcessorInformation_pfn> (
          resolve ("GetLogicalProcessorInformation")
        );

    if (GetActiveProcessorCount_Original == nullptr)
      GetActiveProcessorCount_Original =
        reinterpret_cast <GetActiveProcessorCount_pfn> (
          resolve ("GetActiveProcessorCount")
        );

    if (GetActiveProcessorGroupCount_Original == nullptr)
      GetActiveProcessorGroupCount_Original =
        reinterpret_cast <GetActiveProcessorGroupCount_pfn> (
          resolve ("GetActiveProcessorGroupCount")
        );

    if (GetMaximumProcessorCount_Original == nullptr)
      GetMaximumProcessorCount_Original =
        reinterpret_cast <GetMaximumProcessorCount_pfn> (
          resolve ("GetMaximumProcessorCount")
        );

    if (GetMaximumProcessorGroupCount_Original == nullptr)
      GetMaximumProcessorGroupCount_Original =
        reinterpret_cast <GetMaximumProcessorGroupCount_pfn> (
          resolve ("GetMaximumProcessorGroupCount")
        );

    if (GetSystemInfo_Original == nullptr)
      GetSystemInfo_Original =
        reinterpret_cast <GetSystemInfo_pfn> (
          resolve ("GetSystemInfo")
        );

    if (GetNativeSystemInfo_Original == nullptr)
      GetNativeSystemInfo_Original =
        reinterpret_cast <GetNativeSystemInfo_pfn> (
          resolve ("GetNativeSystemInfo")
        );

    if (SK_CPU_Early_GetProcAddress_Original == nullptr)
      SK_CPU_Early_GetProcAddress_Original =
        reinterpret_cast <SK_CPU_Early_GetProcAddress_pfn> (
          resolve ("GetProcAddress")
        );

    if (SK_CPU_Early_LoadLibraryW_Original == nullptr)
      SK_CPU_Early_LoadLibraryW_Original =
        reinterpret_cast <SK_CPU_Early_LoadLibraryW_pfn> (
          resolve ("LoadLibraryW")
        );

    if (SK_CPU_Early_LoadLibraryA_Original == nullptr)
      SK_CPU_Early_LoadLibraryA_Original =
        reinterpret_cast <SK_CPU_Early_LoadLibraryA_pfn> (
          resolve ("LoadLibraryA")
        );

    if (SK_CPU_Early_LoadLibraryExW_Original == nullptr)
      SK_CPU_Early_LoadLibraryExW_Original =
        reinterpret_cast <SK_CPU_Early_LoadLibraryExW_pfn> (
          resolve ("LoadLibraryExW")
        );

    if (SK_CPU_Early_LoadLibraryExA_Original == nullptr)
      SK_CPU_Early_LoadLibraryExA_Original =
        reinterpret_cast <SK_CPU_Early_LoadLibraryExA_pfn> (
          resolve ("LoadLibraryExA")
        );
  }

  FARPROC WINAPI
  SK_CPU_EarlyTopologySpoof_GetProcAddress_Detour (
    HMODULE hModule,
    LPCSTR  lpProcName )
  {
    if (lpProcName != nullptr &&
        (reinterpret_cast <uintptr_t> (lpProcName) >> 16) != 0)
    {
      void** original = nullptr;
      void*  detour   =
        SK_CPU_EarlyTopologySpoof_DetourForName (lpProcName, &original);

      if (detour != nullptr)
        return
          reinterpret_cast <FARPROC> (detour);
    }

    return
      SK_CPU_Early_GetProcAddress_Original != nullptr ?
        SK_CPU_Early_GetProcAddress_Original (hModule, lpProcName) :
        nullptr;
  }

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryW_Detour (LPCWSTR lpLibFileName)
  {
    HMODULE module =
      SK_CPU_Early_LoadLibraryW_Original != nullptr ?
        SK_CPU_Early_LoadLibraryW_Original (lpLibFileName) :
        nullptr;

    SK_CPU_EarlyTopologySpoof_PatchLoadedModules ();

    return module;
  }

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryA_Detour (LPCSTR lpLibFileName)
  {
    HMODULE module =
      SK_CPU_Early_LoadLibraryA_Original != nullptr ?
        SK_CPU_Early_LoadLibraryA_Original (lpLibFileName) :
        nullptr;

    SK_CPU_EarlyTopologySpoof_PatchLoadedModules ();

    return module;
  }

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryExW_Detour (
    LPCWSTR lpLibFileName,
    HANDLE  hFile,
    DWORD   dwFlags )
  {
    HMODULE module =
      SK_CPU_Early_LoadLibraryExW_Original != nullptr ?
        SK_CPU_Early_LoadLibraryExW_Original (lpLibFileName, hFile, dwFlags) :
        nullptr;

    SK_CPU_EarlyTopologySpoof_PatchLoadedModules ();

    return module;
  }

  HMODULE WINAPI
  SK_CPU_EarlyTopologySpoof_LoadLibraryExA_Detour (
    LPCSTR lpLibFileName,
    HANDLE hFile,
    DWORD  dwFlags )
  {
    HMODULE module =
      SK_CPU_Early_LoadLibraryExA_Original != nullptr ?
        SK_CPU_Early_LoadLibraryExA_Original (lpLibFileName, hFile, dwFlags) :
        nullptr;

    SK_CPU_EarlyTopologySpoof_PatchLoadedModules ();

    return module;
  }

  struct SK_CPU_AppCompatSDB_Writer
  {
    std::vector <BYTE> data;

    uint32_t
    offset (void) const
    {
      return
        static_cast <uint32_t> (data.size ());
    }

    void
    u16 (uint16_t value)
    {
      data.emplace_back (static_cast <BYTE> ( value        & 0xff));
      data.emplace_back (static_cast <BYTE> ((value >>  8) & 0xff));
    }

    void
    u32 (uint32_t value)
    {
      for (int i = 0; i < 4; ++i)
        data.emplace_back (static_cast <BYTE> ((value >> (i * 8)) & 0xff));
    }

    void
    i64 (int64_t value)
    {
      const uint64_t bits =
        static_cast <uint64_t> (value);

      for (int i = 0; i < 8; ++i)
        data.emplace_back (static_cast <BYTE> ((bits >> (i * 8)) & 0xff));
    }

    void
    bytes (const void* ptr, size_t size)
    {
      if (ptr == nullptr || size == 0)
        return;

      const auto* byte_ptr =
        static_cast <const BYTE*> (ptr);

      data.insert (data.end (), byte_ptr, byte_ptr + size);
    }

    void
    list_tag (uint16_t tag, const std::vector <BYTE>& payload)
    {
      u16   (tag);
      u32   (static_cast <uint32_t> (payload.size ()));
      bytes (payload.data (), payload.size ());
    }

    void
    string_ref_tag (uint16_t tag, uint32_t string_ref)
    {
      u16 (tag);
      u32 (string_ref);
    }

    void
    dword_tag (uint16_t tag, uint32_t value)
    {
      u16 (tag);
      u32 (value);
    }

    void
    qword_tag (uint16_t tag, int64_t value)
    {
      u16 (tag);
      i64 (value);
    }

    void
    binary_tag (uint16_t tag, const void* ptr, size_t size)
    {
      u16   (tag);
      u32   (static_cast <uint32_t> (size));
      bytes (ptr, size);
    }
  };

  struct SK_CPU_AppCompatSDB_StringRef
  {
    std::wstring value;
    uint32_t     ref = 0;
  };

  struct SK_CPU_AppCompatSDB_Game
  {
    std::wstring exe;
    std::wstring app;
    std::wstring param;

    uint32_t exe_ref             = 0;
    uint32_t app_ref             = 0;
    uint32_t param_ref           = 0;
    uint32_t relative_exe_offset = 0;
  };

  std::wstring
  SK_CPU_AppCompatSDB_SanitizeFileName (const std::wstring& name)
  {
    std::wstring sanitized = name;

    for (wchar_t& ch : sanitized)
    {
      switch (ch)
      {
        case L'\\':
        case L'/':
        case L':':
        case L'*':
        case L'?':
        case L'"':
        case L'<':
        case L'>':
        case L'|':
          ch = L'_';
          break;

        default:
          break;
      }
    }

    return
      sanitized.empty () ? L"Application" : sanitized;
  }

  std::wstring
  SK_CPU_AppCompatSDB_StemFromExe (const std::wstring& exe_name)
  {
    std::wstring stem =
      SK_CPU_AppCompatSDB_SanitizeFileName (exe_name);

    const size_t dot =
      stem.find_last_of (L'.');

    if (dot != std::wstring::npos && dot > 0)
      stem.resize (dot);

    return
      stem;
  }

  uint32_t
  SK_CPU_AppCompatSDB_AddString (
    SK_CPU_AppCompatSDB_Writer&                  writer,
    std::vector <SK_CPU_AppCompatSDB_StringRef>& refs,
    const std::wstring&                          value )
  {
    for (const auto& ref : refs)
    {
      if (ref.value == value)
        return ref.ref;
    }

    const uint32_t content_offset =
      writer.offset () + 6;

    std::vector <BYTE> utf16;
    utf16.reserve ((value.length () + 1) * sizeof (wchar_t));

    for (wchar_t ch : value)
    {
      const uint16_t code_unit =
        static_cast <uint16_t> (ch);

      utf16.emplace_back (static_cast <BYTE> ( code_unit       & 0xff));
      utf16.emplace_back (static_cast <BYTE> ((code_unit >> 8) & 0xff));
    }

    utf16.emplace_back (static_cast <BYTE> (0));
    utf16.emplace_back (static_cast <BYTE> (0));

    writer.u16   (0x8801);
    writer.u32   (static_cast <uint32_t> (utf16.size ()));
    writer.bytes (utf16.data (), utf16.size ());

    refs.emplace_back (SK_CPU_AppCompatSDB_StringRef { value, content_offset });

    return
      content_offset;
  }

  uint64_t
  SK_CPU_AppCompatSDB_HashString (const std::wstring& value, uint64_t seed)
  {
    uint64_t hash =
      1469598103934665603ULL ^ seed;

    for (wchar_t ch : value)
    {
      const uint16_t code_unit =
        static_cast <uint16_t> (::towlower (ch));

      hash ^= ( code_unit       & 0xff);
      hash *= 1099511628211ULL;
      hash ^= ((code_unit >> 8) & 0xff);
      hash *= 1099511628211ULL;
    }

    return
      hash;
  }

  GUID
  SK_CPU_AppCompatSDB_GuidFor (const std::wstring& exe_name, uint64_t salt)
  {
    const uint64_t hash_a =
      SK_CPU_AppCompatSDB_HashString (exe_name, salt);

    const uint64_t hash_b =
      SK_CPU_AppCompatSDB_HashString (exe_name, salt ^ 0x9e3779b97f4a7c15ULL);

    GUID guid =
      { 0x9d09992a, 0x2ca0, 0x4cec,
        { 0x9e, 0x4e, 0x8d, 0xb4, 0x03, 0x35, 0xcc, 0x2b } };

    guid.Data1 ^= static_cast <DWORD> (hash_a);
    guid.Data2 ^= static_cast <WORD>  (hash_a >> 32);
    guid.Data3 ^= static_cast <WORD>  (hash_a >> 48);

    for (int i = 0; i < 8; ++i)
      guid.Data4 [i] ^=
        static_cast <BYTE> ((hash_b >> (i * 8)) & 0xff);

    return
      guid;
  }

  int64_t
  SK_CPU_AppCompatSDB_CurrentFileTime (void)
  {
    FILETIME file_time = { };
    GetSystemTimeAsFileTime (&file_time);

    ULARGE_INTEGER large = { };
    large.LowPart  = file_time.dwLowDateTime;
    large.HighPart = file_time.dwHighDateTime;

    return
      static_cast <int64_t> (large.QuadPart);
  }

  void
  SK_CPU_AppCompatSDB_WriteGuidTag (
    SK_CPU_AppCompatSDB_Writer& writer,
    uint16_t                    tag,
    const GUID&                 guid )
  {
    writer.binary_tag (tag, &guid, sizeof (guid));
  }

  void
  SK_CPU_AppCompatSDB_WriteIndexKey (
    SK_CPU_AppCompatSDB_Writer& writer,
    const std::wstring&         exe_name )
  {
    BYTE key [8] = { };

    for (int i = 0; i < 8; ++i)
    {
      wchar_t ch =
        (i < static_cast <int> (exe_name.length ())) ?
          ::towupper (exe_name [i]) : L' ';

      key [7 - i] =
        (ch >= 0 && ch <= 0x7f) ? static_cast <BYTE> (ch) : static_cast <BYTE> ('?');
    }

    writer.bytes (key, sizeof (key));
  }

  std::vector <BYTE>
  SK_CPU_AppCompatSDB_BuildProcessorCountLie (
    const std::wstring& exe_name,
    const std::wstring& app_name,
    int                 processor_count )
  {
    std::vector <SK_CPU_AppCompatSDB_Game> games;
    games.emplace_back ();

    games [0].exe   = exe_name;
    games [0].app   = app_name.empty () ? SK_CPU_AppCompatSDB_StemFromExe (exe_name) : app_name;
    games [0].param =
      std::to_wstring (std::clamp (processor_count, 1, 64));

    SK_CPU_AppCompatSDB_Writer                  strings;
    std::vector <SK_CPU_AppCompatSDB_StringRef> refs;

    const uint32_t version_ref =
      SK_CPU_AppCompatSDB_AddString (strings, refs, L"1.0.0.0");

    const uint32_t db_name_ref =
      SK_CPU_AppCompatSDB_AddString (
        strings, refs,
        SK_FormatStringW ( L"Special K AMD AppCompat ProcessorCountLie (%ws)",
                           exe_name.c_str () )
      );

    const uint32_t vendor_ref =
      SK_CPU_AppCompatSDB_AddString (strings, refs, L"<Unknown>");

    const uint32_t match_all_ref =
      SK_CPU_AppCompatSDB_AddString (strings, refs, L"*");

    const uint32_t shim_ref =
      SK_CPU_AppCompatSDB_AddString (strings, refs, L"ProcessorCountLie");

    for (auto& game : games)
    {
      game.exe_ref =
        SK_CPU_AppCompatSDB_AddString (strings, refs, game.exe);
      game.app_ref =
        SK_CPU_AppCompatSDB_AddString (strings, refs, game.app);
      game.param_ref =
        SK_CPU_AppCompatSDB_AddString (strings, refs, game.param);
    }

    SK_CPU_AppCompatSDB_Writer db;
    db.qword_tag      (0x5001, SK_CPU_AppCompatSDB_CurrentFileTime ());
    db.string_ref_tag (0x6022, version_ref);
    db.string_ref_tag (0x6001, db_name_ref);
    db.dword_tag      (0x4023, 4);
    db.dword_tag      (0x4021, 2);

    const GUID database_guid =
      SK_CPU_AppCompatSDB_GuidFor (exe_name, 0);

    SK_CPU_AppCompatSDB_WriteGuidTag (db, 0x9007, database_guid);
    db.list_tag (0x7002, { });

    for (size_t i = 0; i < games.size (); ++i)
    {
      auto& game =
        games [i];

      game.relative_exe_offset =
        db.offset ();

      SK_CPU_AppCompatSDB_Writer exe;
      exe.string_ref_tag (0x6001, game.exe_ref);
      exe.string_ref_tag (0x6006, game.app_ref);
      exe.string_ref_tag (0x6005, vendor_ref);

      const GUID exe_guid =
        SK_CPU_AppCompatSDB_GuidFor (game.exe, 1 + i);
      const GUID layer_guid =
        SK_CPU_AppCompatSDB_GuidFor (game.exe, 0x100 + i);

      SK_CPU_AppCompatSDB_WriteGuidTag (exe, 0x9004, exe_guid);
      SK_CPU_AppCompatSDB_WriteGuidTag (exe, 0x9011, layer_guid);

      SK_CPU_AppCompatSDB_Writer matching;
      matching.string_ref_tag (0x6001, match_all_ref);
      exe.list_tag (0x7008, matching.data);

      SK_CPU_AppCompatSDB_Writer shim;
      shim.string_ref_tag (0x6001, shim_ref);
      shim.string_ref_tag (0x6008, game.param_ref);
      exe.list_tag (0x7009, shim.data);

      db.list_tag (0x7007, exe.data);
    }

    SK_CPU_AppCompatSDB_Writer index_payload;
    index_payload.u16       (0x3802);
    index_payload.u16       (0x7007);
    index_payload.u16       (0x3803);
    index_payload.u16       (0x6001);
    index_payload.dword_tag (0x4016, 1);
    index_payload.u16       (0x9801);
    index_payload.u32       (static_cast <uint32_t> (12 * games.size ()));

    const uint32_t index_total_length =
      12 + (6 + (20 + static_cast <uint32_t> (12 * games.size ()))) + 6;

    const uint32_t database_payload_start =
      index_total_length + 6;

    for (const auto& game : games)
    {
      SK_CPU_AppCompatSDB_WriteIndexKey (index_payload, game.exe);
      index_payload.u32 (database_payload_start + game.relative_exe_offset);
    }

    SK_CPU_AppCompatSDB_Writer indexes;
    indexes.list_tag (0x7803, index_payload.data);

    SK_CPU_AppCompatSDB_Writer root;
    root.u32 (2);
    root.u32 (3);
    root.bytes ("sdbf", 4);
    root.list_tag (0x7802, indexes.data);
    root.list_tag (0x7001, db.data);
    root.list_tag (0x7801, strings.data);

    return
      root.data;
  }

  bool
  SK_CPU_AppCompatSDB_WriteFile (
    const std::wstring&        path,
    const std::vector <BYTE>&  data )
  {
    if (path.empty () || data.empty ())
      return false;

    const size_t slash =
      path.find_last_of (L"\\/");

    if (slash != std::wstring::npos)
    {
      const std::wstring dir =
        path.substr (0, slash + 1);

      SK_CreateDirectoriesEx (dir.c_str (), false);
    }

    HANDLE hFile =
      CreateFileW ( path.c_str (), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, nullptr );

    if (hFile == INVALID_HANDLE_VALUE)
      return false;

    DWORD written_total = 0;
    BOOL  ok            = TRUE;

    while (written_total < data.size () && ok)
    {
      DWORD written_now = 0;
      ok =
        WriteFile ( hFile, data.data () + written_total,
                    static_cast <DWORD> (
                      std::min <size_t> (data.size () - written_total, MAXDWORD)
                    ),
                    &written_now, nullptr );

      if (written_now == 0)
        break;

      written_total +=
        written_now;
    }

    CloseHandle (hFile);

    return
      ok && written_total == data.size ();
  }
}

void
SK_CPU_EarlyTopologySpoof_Install (void)
{
  static volatile LONG installed = FALSE;

  if (ReadAcquire (&installed) != FALSE)
    return;

  if (! SK_CPU_EarlyTopologySpoof_ReadProfileConfig ())
    return;

  if (InterlockedCompareExchange (&installed, TRUE, FALSE) != FALSE)
    return;

  GetWindowsDirectoryW (
    SK_CPU_EarlyTopologySpoof_WindowsDir,
    static_cast <UINT> (std::size (SK_CPU_EarlyTopologySpoof_WindowsDir))
  );

  SK_CPU_EarlyTopologySpoof_ResolveOriginals ();
  SK_CPU_EarlyTopologySpoof_PatchLoadedModules ();
}

std::wstring
SK_CPU_AppCompatSDB_GetPath (void)
{
  const wchar_t* config_path =
    SK_GetConfigPath ();

  const wchar_t* host_app =
    SK_GetHostApp ();

  if (config_path == nullptr || *config_path == L'\0')
    return { };

  const std::wstring exe_name =
    (host_app != nullptr && *host_app != L'\0') ? host_app : L"Application.exe";

  return
    SK_FormatStringW ( LR"(%ws\AppCompat\%ws_CPU_Topology.sdb)",
                       config_path,
                       SK_CPU_AppCompatSDB_StemFromExe (exe_name).c_str () );
}

bool
SK_CPU_AppCompatSDB_WriteProcessorCountLie (
  const wchar_t* exe_name,
  const wchar_t* app_name,
  int            processor_count,
  std::wstring*  out_path )
{
  const std::wstring resolved_exe =
    (exe_name != nullptr && *exe_name != L'\0') ? exe_name :
      (SK_GetHostApp () != nullptr ? SK_GetHostApp () : L"Application.exe");

  const std::wstring resolved_app =
    (app_name != nullptr) ? app_name : L"";

  const std::wstring sdb_path =
    SK_CPU_AppCompatSDB_GetPath ();

  if (out_path != nullptr)
    *out_path = sdb_path;

  if (sdb_path.empty ())
    return false;

  const auto data =
    SK_CPU_AppCompatSDB_BuildProcessorCountLie (
      resolved_exe,
      resolved_app,
      processor_count
    );

  return
    SK_CPU_AppCompatSDB_WriteFile (sdb_path, data);
}

bool
SK_CPU_AppCompatSDB_RunInstaller (
  const wchar_t* sdb_path,
  bool           uninstall,
  bool           update )
{
  if (sdb_path == nullptr || *sdb_path == L'\0')
    return false;

  if (GetFileAttributesW (sdb_path) == INVALID_FILE_ATTRIBUTES)
    return false;

  wchar_t system_dir [MAX_PATH + 2] = { };

  if (GetSystemDirectoryW (system_dir, MAX_PATH) == 0)
    return false;

  const std::wstring sdbinst =
    SK_FormatStringW (LR"(%ws\sdbinst.exe)", system_dir);

  if (GetFileAttributesW (sdbinst.c_str ()) == INVALID_FILE_ATTRIBUTES)
    return false;

  if (update && ! uninstall)
  {
    const std::wstring cmd =
      SK_FormatStringW (LR"(%ws\cmd.exe)", system_dir);

    const std::wstring params =
      SK_FormatStringW (
        LR"(/c ""%ws" -q -u "%ws" >nul 2>nul & "%ws" -q "%ws"")",
        sdbinst.c_str (), sdb_path, sdbinst.c_str (), sdb_path
      );

    HINSTANCE hInst =
      SK_ShellExecuteW (
        nullptr, L"runas", cmd.c_str (), params.c_str (), nullptr, SW_HIDE
      );

    return
      reinterpret_cast <intptr_t> (hInst) > 32;
  }

  const std::wstring params =
    uninstall ?
      SK_FormatStringW (LR"(-q -u "%ws")", sdb_path) :
      SK_FormatStringW (LR"(-q "%ws")",    sdb_path);

  HINSTANCE hInst =
    SK_ShellExecuteW (
      nullptr, L"runas", sdbinst.c_str (), params.c_str (), nullptr, SW_HIDE
    );

  return
    reinterpret_cast <intptr_t> (hInst) > 32;
}

BOOL
WINAPI
GetLogicalProcessorInformation_Detour (
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
  PDWORD                                ReturnedLength )
{
  if (dll_log.isAllocated ())
  {
    SK_LOG_FIRST_CALL
  }

  if (SK_CPU_Spoof_IsEnabled ())
  {
    if (dll_log.isAllocated ())
    {
      const DWORD length_in =
        (ReturnedLength != nullptr) ? *ReturnedLength : 0UL;

      SK_CPU_Spoof_LogCall (
        L"GetLogicalProcessorInformation",
        _ReturnAddress (),
        SK_FormatStringW (
          L"buffer=%p, length_in=%lu",
          Buffer,
          length_in
        ).c_str ()
      );
    }

    return
      SK_CPU_Spoof_CopyProcessorInformation (Buffer, ReturnedLength);
  }

  static const
    std::vector <uintptr_t>& pairs =
      SK_CPU_GetLogicalCorePairs ();

  BOOL bRet =
    GetLogicalProcessorInformation_Original ( Buffer, ReturnedLength );

  if ( bRet != FALSE && ReturnedLength != nullptr )
  {
    struct {
      int physical = 0;
      int logical  = 0;
    } cores;

    if (config.render.framerate.override_num_cpus != SK_NoPreference)
    {
      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr        = Buffer;
      DWORD                                 byteOffset = 0;

      while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= *ReturnedLength)
      {
        switch (ptr->Relationship)
        {
          case RelationProcessorCore:
          {
            cores.physical++;
            cores.logical += CountSetBits (ptr->ProcessorMask);

            if (     cores.physical > config.render.framerate.override_num_cpus)
              ptr->Relationship = RelationAll;
            else if (cores.logical  > config.render.framerate.override_num_cpus)
              ptr->ProcessorMask = 0x0;
          } break;

          default:
            break;
        }

        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
      }
    }
  }

  return
    bRet;
}

BOOL
WINAPI
GetLogicalProcessorInformationEx_Detour (
  LOGICAL_PROCESSOR_RELATIONSHIP           RelationshipType,
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer,
  PDWORD                                   ReturnedLength )
{
  if (dll_log.isAllocated ())
  {
    SK_LOG_FIRST_CALL
  }

  if (SK_CPU_Spoof_IsEnabled ())
  {
    if (dll_log.isAllocated ())
    {
      const DWORD length_in =
        (ReturnedLength != nullptr) ? *ReturnedLength : 0UL;

      SK_CPU_Spoof_LogCall (
        L"GetLogicalProcessorInformationEx",
        _ReturnAddress (),
        SK_FormatStringW (
          L"relationship=%d, buffer=%p, length_in=%lu",
          static_cast <int> (RelationshipType),
          Buffer,
          length_in
        ).c_str ()
      );
    }

    auto data =
      SK_CPU_Spoof_BuildProcessorInformationEx (RelationshipType);

    if (! data.empty ())
      return
        SK_CPU_Spoof_CopyBuffer (data, Buffer, ReturnedLength);
  }

  static const
    int core_count =
      static_cast <int> (SK_CPU_CountPhysicalCores ());

  int extra_cores =
    config.render.framerate.override_num_cpus == SK_NoPreference ? 0 :
    config.render.framerate.override_num_cpus - core_count;

  if (config.render.framerate.override_num_cpus != SK_NoPreference)
    dll_log->Log (L"Allocating %lu extra CPU cores", extra_cores);

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

          buffer =
            (char *)malloc (len);

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
              ptr            += (size_t)pi->Size;
            }
          }

          std::free   (buffer);
                       buffer = (char *)
          std::malloc (len + extraAlloc);

          if ( buffer != nullptr &&
               GetLogicalProcessorInformationEx_Original (
                 RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer,
                   &len )
             )
          {
            auto cores_ =
              (char *)malloc (extraAlloc),
                 ptr    = buffer;

            if (cores_ != nullptr)
            {
              extraCores = extra_cores;

              while (ptr < buffer + len)
              {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pi =
               (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

                if (pi->Relationship == RelationProcessorCore)
                {
                  char *pExtra =
                    cores_;

                  while (extraCores > 0)
                  {
                    memcpy (pExtra, pi,       pi->Size);
                            pExtra += (size_t)pi->Size;

                    extraCores--;
                  }
                }

                ptr += (size_t)pi->Size;
              }

              memcpy (ptr, cores_, extraAlloc);

              len += (DWORD)extraAlloc;
            }
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

#ifdef LOG_CORE_DISTRIBUTION
        size_t cores   = 0;
        size_t logical = 0;
#endif

        char*  ptr = (char *)Buffer;
        while (ptr < (char *)Buffer + spoof.second)
        {
          PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

          if (pi->Relationship == RelationProcessorCore)
          {
#ifdef LOG_CORE_DISTRIBUTION
            cores++;
#endif

            for (size_t g = 0; g < pi->Processor.GroupCount; ++g)
            {
#ifdef LOG_CORE_DISTRIBUTION
              logical +=
                CountSetBits (pi->Processor.GroupMask [g].Mask);
#endif
            }
          }
          ptr += (size_t)pi->Size;
        }

#ifdef LOG_CORE_DISTRIBUTION
        dll_log->Log (L"Returning %lu cores, %lu logical", cores, logical);
#endif
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

  if (SK_CPU_Spoof_IsEnabled ())
  {
    SK_CPU_Spoof_LogCall (
      L"GetSystemInfo",
      _ReturnAddress ()
    );

    SK_CPU_Spoof_ApplySystemInfo (lpSystemInfo);
    return;
  }

  if (config.render.framerate.override_num_cpus == SK_NoPreference)
    return;

  lpSystemInfo->dwActiveProcessorMask = 0x0;

  for ( uintptr_t i = 0 ; i < (uintptr_t)config.render.framerate.override_num_cpus; ++i )
  {
    lpSystemInfo->dwActiveProcessorMask |= (((uintptr_t)1) << i);
  }

  lpSystemInfo->dwNumberOfProcessors = config.render.framerate.override_num_cpus;
}

void
WINAPI
GetNativeSystemInfo_Detour (
  _Out_ LPSYSTEM_INFO lpSystemInfo )
{
  if (GetNativeSystemInfo_Original != nullptr)
    GetNativeSystemInfo_Original (lpSystemInfo);
  else if (GetSystemInfo_Original != nullptr)
    GetSystemInfo_Original (lpSystemInfo);

  SK_CPU_Spoof_LogCall (
    L"GetNativeSystemInfo",
    _ReturnAddress ()
  );

  SK_CPU_Spoof_ApplySystemInfo (lpSystemInfo);
}

DWORD
WINAPI
GetActiveProcessorCount_Detour (WORD GroupNumber)
{
  if (dll_log.isAllocated ())
  {
    SK_LOG_FIRST_CALL
  }

  if (SK_CPU_Spoof_IsEnabled ())
  {
    if (dll_log.isAllocated ())
    {
      SK_CPU_Spoof_LogCall (
        L"GetActiveProcessorCount",
        _ReturnAddress (),
        SK_FormatStringW (L"group=%hu", GroupNumber).c_str ()
      );
    }

    return
      SK_CPU_Spoof_IsGroupZeroOrAll (GroupNumber) ?
        SK_CPU_Spoof_LogicalProcessors () :
        0;
  }

  return
    GetActiveProcessorCount_Original != nullptr ?
      GetActiveProcessorCount_Original (GroupNumber) :
      0;
}

WORD
WINAPI
GetActiveProcessorGroupCount_Detour (void)
{
  if (dll_log.isAllocated ())
  {
    SK_LOG_FIRST_CALL
  }

  if (SK_CPU_Spoof_IsEnabled ())
  {
    if (dll_log.isAllocated ())
    {
      SK_CPU_Spoof_LogCall (
        L"GetActiveProcessorGroupCount",
        _ReturnAddress ()
      );
    }

    return 1;
  }

  return
    GetActiveProcessorGroupCount_Original != nullptr ?
      GetActiveProcessorGroupCount_Original () :
      1;
}

DWORD
WINAPI
GetMaximumProcessorCount_Detour (WORD GroupNumber)
{
  if (dll_log.isAllocated ())
  {
    SK_LOG_FIRST_CALL
  }

  if (SK_CPU_Spoof_IsEnabled ())
  {
    if (dll_log.isAllocated ())
    {
      SK_CPU_Spoof_LogCall (
        L"GetMaximumProcessorCount",
        _ReturnAddress (),
        SK_FormatStringW (L"group=%hu", GroupNumber).c_str ()
      );
    }

    return
      SK_CPU_Spoof_IsGroupZeroOrAll (GroupNumber) ?
        SK_CPU_Spoof_LogicalProcessors () :
        0;
  }

  return
    GetMaximumProcessorCount_Original != nullptr ?
      GetMaximumProcessorCount_Original (GroupNumber) :
      0;
}

WORD
WINAPI
GetMaximumProcessorGroupCount_Detour (void)
{
  if (dll_log.isAllocated ())
  {
    SK_LOG_FIRST_CALL
  }

  if (SK_CPU_Spoof_IsEnabled ())
  {
    if (dll_log.isAllocated ())
    {
      SK_CPU_Spoof_LogCall (
        L"GetMaximumProcessorGroupCount",
        _ReturnAddress ()
      );
    }

    return 1;
  }

  return
    GetMaximumProcessorGroupCount_Original != nullptr ?
      GetMaximumProcessorGroupCount_Original () :
      1;
}

BOOL
WINAPI
GetProcessAffinityMask_Detour (
  _In_  HANDLE     hProcess,
  _Out_ PDWORD_PTR lpProcessAffinityMask,
  _Out_ PDWORD_PTR lpSystemAffinityMask )
{
  SK_LOG_FIRST_CALL

  const BOOL ret =
    GetProcessAffinityMask_Original != nullptr ?
      GetProcessAffinityMask_Original (
        hProcess,
        lpProcessAffinityMask,
        lpSystemAffinityMask
      ) :
      FALSE;

  if (ret && SK_CPU_Spoof_IsEnabled ())
  {
    const DWORD_PTR mask =
      static_cast <DWORD_PTR> (
        SK_CPU_Spoof_MaskForCount (SK_CPU_Spoof_LogicalProcessors ())
      );

    if (lpProcessAffinityMask != nullptr)
      *lpProcessAffinityMask = mask;

    if (lpSystemAffinityMask != nullptr)
      *lpSystemAffinityMask = mask;

    SK_CPU_Spoof_LogCall (
      L"GetProcessAffinityMask",
      _ReturnAddress (),
      SK_FormatStringW (L"mask=%p", reinterpret_cast <void*> (mask)).c_str ()
    );
  }

  return ret;
}

VOID
WINAPI
SetThreadpoolThreadMaximum_Detour (
  _Inout_ PTP_POOL ptpp,
  _In_    DWORD    cthrdMost )
{
  SK_LOG_FIRST_CALL

  DWORD applied_max =
    cthrdMost;

  if (SK_CPU_Spoof_IsEnabled ())
  {
    const DWORD logical =
      SK_CPU_Spoof_LogicalProcessors ();

    if (applied_max == 0 || applied_max > logical)
      applied_max = logical;

    SK_CPU_Spoof_LogCall (
      L"SetThreadpoolThreadMaximum",
      _ReturnAddress (),
      SK_FormatStringW (L"requested=%lu, applied=%lu", cthrdMost, applied_max).c_str ()
    );
  }

  if (SetThreadpoolThreadMaximum_Original != nullptr)
    SetThreadpoolThreadMaximum_Original (ptpp, applied_max);
}

BOOL
WINAPI
SetThreadpoolThreadMinimum_Detour (
  _Inout_ PTP_POOL ptpp,
  _In_    DWORD    cthrdMic )
{
  SK_LOG_FIRST_CALL

  DWORD applied_min =
    cthrdMic;

  if (SK_CPU_Spoof_IsEnabled ())
  {
    const DWORD logical =
      SK_CPU_Spoof_LogicalProcessors ();

    if (applied_min > logical)
      applied_min = logical;

    SK_CPU_Spoof_LogCall (
      L"SetThreadpoolThreadMinimum",
      _ReturnAddress (),
      SK_FormatStringW (L"requested=%lu, applied=%lu", cthrdMic, applied_min).c_str ()
    );
  }

  return
    SetThreadpoolThreadMinimum_Original != nullptr ?
      SetThreadpoolThreadMinimum_Original (ptpp, applied_min) :
      FALSE;
}

unsigned int
__cdecl
MSVCP_Thrd_hardware_concurrency_Detour (void)
{
  SK_LOG_FIRST_CALL

  if (SK_CPU_Spoof_IsEnabled ())
  {
    SK_CPU_Spoof_LogCall (
      L"MSVCP140!_Thrd_hardware_concurrency",
      _ReturnAddress ()
    );

    return
      static_cast <unsigned int> (SK_CPU_Spoof_LogicalProcessors ());
  }

  return
    MSVCP_Thrd_hardware_concurrency_Original != nullptr ?
      MSVCP_Thrd_hardware_concurrency_Original () :
      0U;
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
  SK_PROFILE_FIRST_CALL

  static std::vector <void*> hooked_targets;

  for (const wchar_t* module : { L"Kernel32", L"KernelBase" })
  {
    SK_CPU_CreateDLLHook2Unique (
      module, "GetLogicalProcessorInformation",
      GetLogicalProcessorInformation_Detour,
      static_cast_p2p <void> (&GetLogicalProcessorInformation_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetLogicalProcessorInformationEx",
      GetLogicalProcessorInformationEx_Detour,
      static_cast_p2p <void> (&GetLogicalProcessorInformationEx_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetSystemInfo",
      GetSystemInfo_Detour,
      static_cast_p2p <void> (&GetSystemInfo_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetNativeSystemInfo",
      GetNativeSystemInfo_Detour,
      static_cast_p2p <void> (&GetNativeSystemInfo_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetActiveProcessorCount",
      GetActiveProcessorCount_Detour,
      static_cast_p2p <void> (&GetActiveProcessorCount_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetActiveProcessorGroupCount",
      GetActiveProcessorGroupCount_Detour,
      static_cast_p2p <void> (&GetActiveProcessorGroupCount_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetMaximumProcessorCount",
      GetMaximumProcessorCount_Detour,
      static_cast_p2p <void> (&GetMaximumProcessorCount_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetMaximumProcessorGroupCount",
      GetMaximumProcessorGroupCount_Detour,
      static_cast_p2p <void> (&GetMaximumProcessorGroupCount_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "GetProcessAffinityMask",
      GetProcessAffinityMask_Detour,
      static_cast_p2p <void> (&GetProcessAffinityMask_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "SetThreadpoolThreadMaximum",
      SetThreadpoolThreadMaximum_Detour,
      static_cast_p2p <void> (&SetThreadpoolThreadMaximum_Original),
      hooked_targets
    );

    SK_CPU_CreateDLLHook2Unique (
      module, "SetThreadpoolThreadMinimum",
      SetThreadpoolThreadMinimum_Detour,
      static_cast_p2p <void> (&SetThreadpoolThreadMinimum_Original),
      hooked_targets
    );
  }

  if (GetModuleHandleW (L"MSVCP140.dll") != nullptr)
  {
    SK_CPU_CreateDLLHook2Unique (
      L"MSVCP140.dll", "_Thrd_hardware_concurrency",
      MSVCP_Thrd_hardware_concurrency_Detour,
      static_cast_p2p <void> (&MSVCP_Thrd_hardware_concurrency_Original),
      hooked_targets
    );
  }
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
          new uint8_t [(size_t)dwNeededBytes];
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

          size_t dwOffset = 0;
          while (dwOffset + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= (size_t)dwNeededBytes)
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
                              new uint8_t [(size_t)dwNeededBytes] { };

      if (GetLogicalProcessorInformation_Original (pLogProcInfo, &dwNeededBytes))
      {
        size_t                                dwOffset = 0;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi      = pLogProcInfo;

        while (dwOffset + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= (size_t)dwNeededBytes)
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


std::atomic <EFFECTIVE_POWER_MODE> SK_Power_EffectiveMode = (EFFECTIVE_POWER_MODE)EffectivePowerModeNone;

VOID
WINAPI
SK_Power_EffectiveModeCallback (
  _In_     EFFECTIVE_POWER_MODE  Mode,
  _In_opt_ VOID                 *Context )
{
  UNREFERENCED_PARAMETER (Context);

  SK_Power_EffectiveMode.store (Mode);
};

EFFECTIVE_POWER_MODE
SK_Power_GetCurrentEffectiveMode (void)
{
  return
    SK_Power_EffectiveMode.load ();
}

const char*
SK_Power_GetEffectiveModeStr (EFFECTIVE_POWER_MODE mode)
{
  switch (mode)
  {
    case EffectivePowerModeBatterySaver:
      return "Battery Saver";

    case EffectivePowerModeBetterBattery:
      return "Better Battery";

    case EffectivePowerModeBalanced:
      return "Balanced";

    case EffectivePowerModeHighPerformance:
      return "High Performance";

    case EffectivePowerModeMaxPerformance:
      return "Max Performance";

    case EffectivePowerModeGameMode:
      return "Game Mode";

    case EffectivePowerModeMixedReality:
      return "Mixed Reality";

    default:
      return "Unknown Mode";
  }
}

void* SK_Power_EffectiveMode_Notification = nullptr;

using PowerUnregisterFromEffectivePowerModeNotifications_pfn =
  HRESULT (WINAPI *)(VOID *RegistrationHandle);

   PowerUnregisterFromEffectivePowerModeNotifications_pfn
SK_PowerUnregisterFromEffectivePowerModeNotifications = nullptr;

bool
SK_Power_StopEffectiveModeCallbacks (void)
{
  // WINE will no doubt complain that these are stubs and crash...
  if (config.compatibility.using_wine)
    return false;

  // Already shutdown
  if (SK_Power_EffectiveMode_Notification == nullptr)
    return true;

  if (SK_PowerUnregisterFromEffectivePowerModeNotifications != nullptr)
  {
    HRESULT hr =
      SK_PowerUnregisterFromEffectivePowerModeNotifications (SK_Power_EffectiveMode_Notification);

    if (SUCCEEDED (hr))
    {
      SK_Power_EffectiveMode_Notification = nullptr;
      return true;
    }
  }

  return false;
}

bool
SK_Power_InitEffectiveModeCallbacks (void)
{
  // WINE will no doubt complain that these are stubs and crash...
  if (config.compatibility.using_wine)
    return false;

  // Already initialized
  if (SK_Power_EffectiveMode_Notification != nullptr)
    return true;

  using PowerRegisterForEffectivePowerModeNotifications_pfn =
    HRESULT (WINAPI *)( ULONG                          Version,
                        EFFECTIVE_POWER_MODE_CALLBACK *Callback,
                        VOID                          *Context,
                        VOID                         **RegistrationHandle );

  static PowerRegisterForEffectivePowerModeNotifications_pfn
      SK_PowerRegisterForEffectivePowerModeNotifications =
        (PowerRegisterForEffectivePowerModeNotifications_pfn)GetProcAddress (LoadLibraryEx (L"powrprof.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32),
        "PowerRegisterForEffectivePowerModeNotifications");

  if (SK_PowerRegisterForEffectivePowerModeNotifications      != nullptr)
  {
    SK_PowerUnregisterFromEffectivePowerModeNotifications =
      (PowerUnregisterFromEffectivePowerModeNotifications_pfn)GetProcAddress (LoadLibraryEx (L"powrprof.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32),
      "PowerUnregisterFromEffectivePowerModeNotifications");

    if (SK_PowerUnregisterFromEffectivePowerModeNotifications != nullptr)
    {
      HRESULT hr =
        SK_PowerRegisterForEffectivePowerModeNotifications ( EFFECTIVE_POWER_MODE_V2,
                                                          SK_Power_EffectiveModeCallback, nullptr,
                                                         &SK_Power_EffectiveMode_Notification );

      if (SUCCEEDED (hr))
      {
        return true;
      }
    }
  }

  return false;
}


// Some CPUIDs are lying about their capabilities,
//   try the instruction and see if it's illegal...
bool SK_CPU_TestForMWAITX (void)
{
  static bool supported = true;
  auto        handler   = // Workaround CAPCOM DRM
  SK_AddVectoredExceptionHandler (1, [](_EXCEPTION_POINTERS *ExceptionInfo)->LONG
  {
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
      supported = false;
    }

#ifdef _AMD64_
    ExceptionInfo->ContextRecord->Rip++;
#else
    ExceptionInfo->ContextRecord->Eip++;
#endif

    return EXCEPTION_CONTINUE_EXECUTION;
  });

  static alignas(64)
        uint64_t monitor = 0ULL;
  _mm_monitorx (&monitor, 0, 0);
  _mm_mwaitx   (0x2,      0, 1);

  RemoveVectoredExceptionHandler (handler);

  return supported;
}

// The underlying API is only available on Windows 10, but SK still supports Windows 8.1, so load this function at runtime (if available)
BOOL
WINAPI
SK_GetSystemCpuSetInformation (_Out_writes_bytes_to_opt_(BufferLength, *ReturnedLength) PSYSTEM_CPU_SET_INFORMATION Information, _In_ ULONG BufferLength, _Always_(_Out_) PULONG ReturnedLength, _In_opt_ HANDLE Process, _Reserved_ ULONG Flags)
{
  using  GetSystemCpuSetInformation_pfn = BOOL (WINAPI *)(PSYSTEM_CPU_SET_INFORMATION,ULONG,PULONG,HANDLE,ULONG);
  static GetSystemCpuSetInformation_pfn
        _GetSystemCpuSetInformation =
        (GetSystemCpuSetInformation_pfn)SK_GetProcAddress (L"kernel32.dll", "GetSystemCpuSetInformation");

  if (! _GetSystemCpuSetInformation)
    return FALSE;

  return
    _GetSystemCpuSetInformation (Information, BufferLength, ReturnedLength, Process, Flags);
}

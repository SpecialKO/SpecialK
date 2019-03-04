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

#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/log.h>
#include <SpecialK/sound.h>
#include <SpecialK/resource.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/tls.h>

#include <SpecialK/diagnostics/debug_utils.h>

#include <Windows.h>
#include <intsafe.h>
#include <Shlwapi.h>
#include <algorithm>
#include <memory>
#include <ctime>

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>

#include <diagnostics/compatibility.h>
#include <diagnostics/modules.h>


#include <SpecialK/diagnostics/load_library.h>
#include <SpecialK/diagnostics/crash_handler.h>


//#define STRICT_COMPLIANCE

// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _IMAGEHLP_SOURCE_
#include <dbghelp.h>


#ifdef _WIN64
#define SK_StackWalk          StackWalk64
#define SK_SymLoadModule      SymLoadModule64
#define SK_SymUnloadModule    SymUnloadModule64
#define SK_SymGetModuleBase   SymGetModuleBase64
#define SK_SymGetLineFromAddr SymGetLineFromAddr64
#else
#define SK_StackWalk          StackWalk
#define SK_SymLoadModule      SymLoadModule
#define SK_SymUnloadModule    SymUnloadModule
#define SK_SymGetModuleBase   SymGetModuleBase
#define SK_SymGetLineFromAddr SymGetLineFromAddr
#endif

extern const wchar_t*
__stdcall
SK_GetDebugSymbolPath (void);

extern volatile LONG __SK_Init;

void
SK_SymSetOpts (void)
{
  std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

  SymSetSearchPathW ( GetCurrentProcess (), SK_GetDebugSymbolPath () );
  SymSetOptions     ( SYMOPT_LOAD_LINES           | SYMOPT_NO_PROMPTS        |
                      SYMOPT_UNDNAME              | SYMOPT_DEFERRED_LOADS    |
                      SYMOPT_OMAP_FIND_NEAREST    | SYMOPT_FAVOR_COMPRESSED  |
                      SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_UNQUALIFIED_LOADS);
}


// Set to true during abnormal program termination;
//   used primarily for prognostics in the global injector.
static volatile LONG __SK_Crashed = 0;

bool SK_Debug_IsCrashing (void)
{
  bool ret = true;

  auto orig_se =
  _set_se_translator (SK_BasicStructuredExceptionTranslator);
  try                                    { ret = ReadAcquire (&__SK_Crashed) != 0; }
  catch (const SK_SEH_IgnoredException&) { }
  _set_se_translator (orig_se);

  return ret;
}


struct sk_crash_sound_s {
  HGLOBAL             ref        = nullptr;
  uint8_t*            buf        = nullptr;
  ISimpleAudioVolume* volume_ctl = nullptr;

  bool play (void);
} crash_sound;


bool
SK_Crash_PlaySound (void)
{
  bool ret = false;

  // Rare WinMM (SDL/DOSBox) crashes may prevent this from working, so...
  //   don't create another top-level exception.
  auto orig_se =
  _set_se_translator (SK_BasicStructuredExceptionTranslator);
  try {
    PlaySound ( reinterpret_cast <LPCWSTR> (crash_sound.buf),
                  nullptr,
                    SND_SYNC |
                    SND_MEMORY );

    ret = true;
  }
  
  catch (const SK_SEH_IgnoredException&)
  {
  }
  _set_se_translator (orig_se);

  return ret;
}

bool
sk_crash_sound_s::play (void)
{
  // Reverse Volume Ducking the stupid way ;)
  //
  //  * Crash sound is quite loud and PlaySound has no means of volume modulation
  //
  if (volume_ctl == nullptr)
  {
    volume_ctl =
      SK_WASAPI_GetVolumeControl (GetCurrentProcessId ());
  }

  BOOL  muted       = FALSE;
  float orig_volume = 1.0f;

  if (volume_ctl != nullptr)
  {
    if (SUCCEEDED (volume_ctl->GetMasterVolume (&orig_volume))) {
                   volume_ctl->GetMute         (&muted);
                   volume_ctl->SetMasterVolume (0.4f, nullptr);

      // < 0.1% is effectively muted
      muted = ( muted || orig_volume < 0.001f );
    }
  }

  bool played_sound =
    (! muted) && SK_Crash_PlaySound ();

  if (volume_ctl != nullptr)
  {
      volume_ctl->SetMasterVolume (orig_volume, nullptr);
      volume_ctl->Release ();
  }

  return played_sound;
}


LONG
WINAPI
SK_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo );

using namespace SK::Diagnostics;

using SetUnhandledExceptionFilter_pfn = LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *)(
    _In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
);
      SetUnhandledExceptionFilter_pfn
      SetUnhandledExceptionFilter_Original = nullptr;

LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter_Detour (_In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
  UNREFERENCED_PARAMETER (lpTopLevelExceptionFilter);

  SetUnhandledExceptionFilter_Original (lpTopLevelExceptionFilter);

  return
    SetUnhandledExceptionFilter_Original (SK_TopLevelExceptionFilter);
}


void
CrashHandler::Reinstall (void)
{
  static volatile LPVOID   pOldHook   = nullptr;
  if ((uintptr_t)InterlockedCompareExchangePointer (&pOldHook, (PVOID)1, nullptr) > (uintptr_t)1)
  {
    if (MH_OK == SK_RemoveHook (pOldHook))
    {
      InterlockedExchangePointer ((LPVOID *)&pOldHook, nullptr);
    }

    InterlockedCompareExchangePointer ((LPVOID *)&pOldHook, nullptr, (PVOID)1);
  }


  if (! InterlockedCompareExchangePointer ((LPVOID *)&pOldHook, (PVOID)1, nullptr))
  {
    LPVOID pHook = nullptr;

    if ( MH_OK ==
           SK_CreateDLLHook   (       L"kernel32",
                                       "SetUnhandledExceptionFilter",
                                        SetUnhandledExceptionFilter_Detour,
               static_cast_p2p <void> (&SetUnhandledExceptionFilter_Original),
                                       &pHook) )
    {
      if ( MH_OK == MH_EnableHook  ( pHook ) )
      {
        InterlockedExchangePointer ((LPVOID *)&pOldHook, pHook);
      }
    }
  }


  SetErrorMode (SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);


  // Bypass the hook if we have a trampoline
  //
  if (SetUnhandledExceptionFilter_Original != nullptr)
      SetUnhandledExceptionFilter_Original (SK_TopLevelExceptionFilter);

  else
    SetUnhandledExceptionFilter (SK_TopLevelExceptionFilter);
}


void
CrashHandler::Init (void)
{
  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    SK_Thread_Create (
      [](LPVOID) ->
        DWORD
        {
          SetCurrentThreadDescription (         L"[SK] Crash Handler Init");
          SetThreadPriority           (
                        SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST    );

          HRSRC   default_sound =
            FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_CRASH), L"WAVE");

          if (default_sound != nullptr)
          {
            crash_sound.ref   =
              LoadResource (SK_GetDLL (), default_sound);

            if (crash_sound.ref != nullptr)
            {
              crash_sound.buf =
                static_cast <uint8_t *> (LockResource (crash_sound.ref));
            }
          }

          if (! crash_log.initialized)
          {
            crash_log.flush_freq = 0;
            crash_log.lockless   = true;
            crash_log.init       (L"logs/crash.log", L"wt+,ccs=UTF-8");
          }

          Reinstall ();

          InterlockedIncrement (&init);

          SK_Thread_CloseSelf ();

          return 0;
        }
    );
  }
}

void
CrashHandler::Shutdown (void)
{
//SK_SymCleanup (GetCurrentProcess ());

  // Strip the blank line and cause empty-file deletion to happen
  if (crash_log.lines == 1)
      crash_log.lines =  0;

  crash_log.close ();
}



std::string
SK_GetSymbolNameFromModuleAddr (HMODULE hMod, uintptr_t addr)
{
  std::string ret = "";

  HANDLE hProc =
    SK_GetCurrentProcess ();

  MODULEINFO mod_info = { };

  GetModuleInformation (
    GetCurrentProcess (), hMod, &mod_info, sizeof (mod_info)
  );

#ifdef _WIN64
  auto BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else
  auto BaseAddr   = (DWORD)  mod_info.lpBaseOfDll;
#endif

  char szModName [MAX_PATH + 2] = { };

  GetModuleFileNameA   ( hMod, szModName, MAX_PATH );

  char* szDupName    = _strdup (szModName);
  char* pszShortName = szDupName;

  PathStripPathA (pszShortName);

  SK_SymLoadModule ( hProc,
                       nullptr,
                         pszShortName,
                           nullptr,
                             BaseAddr,
                               mod_info.SizeOfImage );

  SYMBOL_INFO_PACKAGE sip                 = {                };
                      sip.si.SizeOfStruct = sizeof SYMBOL_INFO;
                      sip.si.MaxNameLen   = sizeof sip.name;

  DWORD64 Displacement = 0;

  if ( SymFromAddr ( hProc,
         static_cast <DWORD64> (addr),
                         &Displacement,
                           &sip.si ) )
  {
    ret = sip.si.Name;
  }

  else
  {
    ret = "UNKNOWN";
  }

  free (szDupName);

  return ret;
}

iSK_Logger crash_log;

extern iSK_Logger steam_log;
extern iSK_Logger budget_log;
extern iSK_Logger game_debug;
extern iSK_Logger tex_log;

std::wstring
SK_SEH_SummarizeException (_In_ struct _EXCEPTION_POINTERS* ExceptionInfo, bool crash_handled)
{
  std::wstring log_entry;
  log_entry.reserve (16384);

  const wchar_t* desc = L"";

    HMODULE hModSource               = nullptr;
  char    szModName [MAX_PATH + 2] = { };
  HANDLE  hProc                    = SK_GetCurrentProcess ();

#ifdef _WIN64
  DWORD64  ip = ExceptionInfo->ContextRecord->Rip;
#else
  DWORD    ip = ExceptionInfo->ContextRecord->Eip;
#endif

  if (GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCWSTR)ip,
                              &hModSource )) {
    GetModuleFileNameA (hModSource, szModName, MAX_PATH);
  }

#ifdef _WIN64
  DWORD64 BaseAddr =
#else
  DWORD   BaseAddr =
#endif
    SK_SymGetModuleBase ( hProc, ip );

  char       szDupName [MAX_PATH * 2 + 1] = { };
  strncpy_s (szDupName, MAX_PATH * 2, szModName, _TRUNCATE);
  char* pszShortName =
             szDupName;
  
  PathStripPathA (pszShortName);

#define log_entry_format log_entry.append (SK_FormatStringW

  if (crash_handled)
  {
    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
      case EXCEPTION_ACCESS_VIOLATION:
        desc = L"\t<< EXCEPTION_ACCESS_VIOLATION >>";
               //L"The thread tried to read from or write to a virtual address "
               //L"for which it does not have the appropriate access.";
        break;

      case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        desc = L"\t<< EXCEPTION_ARRAY_BOUNDS_EXCEEDED >>";
               //L"The thread tried to access an array element that is out of "
               //L"bounds and the underlying hardware supports bounds checking.";
        break;

      case EXCEPTION_BREAKPOINT:
        desc = L"\t<< EXCEPTION_BREAKPOINT >>";
               //L"A breakpoint was encountered.";
        break;

      case EXCEPTION_DATATYPE_MISALIGNMENT:
        desc = L"\t<< EXCEPTION_DATATYPE_MISALIGNMENT >>";
               //L"The thread tried to read or write data that is misaligned on "
               //L"hardware that does not provide alignment.";
        break;

      case EXCEPTION_FLT_DENORMAL_OPERAND:
        desc = L"\t<< EXCEPTION_FLT_DENORMAL_OPERAND >>";
               //L"One of the operands in a floating-point operation is denormal.";
        break;

      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        desc = L"\t<< EXCEPTION_FLT_DIVIDE_BY_ZERO >>";
               //L"The thread tried to divide a floating-point value by a "
               //L"floating-point divisor of zero.";
        break;

      case EXCEPTION_FLT_INEXACT_RESULT:
        desc = L"\t<< EXCEPTION_FLT_INEXACT_RESULT >>";
               //L"The result of a floating-point operation cannot be represented "
               //L"exactly as a decimal fraction.";
        break;

      case EXCEPTION_FLT_INVALID_OPERATION:
        desc = L"\t<< EXCEPTION_FLT_INVALID_OPERATION >>";
        break;

      case EXCEPTION_FLT_OVERFLOW:
        desc = L"\t<< EXCEPTION_FLT_OVERFLOW >>";
               //L"The exponent of a floating-point operation is greater than the "
               //L"magnitude allowed by the corresponding type.";
        break;

      case EXCEPTION_FLT_STACK_CHECK:
        desc = L"\t<< EXCEPTION_FLT_STACK_CHECK >>";
               //L"The stack overflowed or underflowed as the result of a "
               //L"floating-point operation.";
        break;

      case EXCEPTION_FLT_UNDERFLOW:
        desc = L"\t<< EXCEPTION_FLT_UNDERFLOW >>";
               //L"The exponent of a floating-point operation is less than the "
               //L"magnitude allowed by the corresponding type.";
        break;

      case EXCEPTION_ILLEGAL_INSTRUCTION:
        desc = L"\t<< EXCEPTION_ILLEGAL_INSTRUCTION >>";
               //L"The thread tried to execute an invalid instruction.";
        break;

      case EXCEPTION_IN_PAGE_ERROR:
        desc = L"\t<< EXCEPTION_IN_PAGE_ERROR >>";
               //L"The thread tried to access a page that was not present, "
               //L"and the system was unable to load the page.";
        break;

      case EXCEPTION_INT_DIVIDE_BY_ZERO:
        desc = L"\t<< EXCEPTION_INT_DIVIDE_BY_ZERO >>";
               //L"The thread tried to divide an integer value by an integer "
               //L"divisor of zero.";
        break;

      case EXCEPTION_INT_OVERFLOW:
        desc = L"\t<< EXCEPTION_INT_OVERFLOW >>";
               //L"The result of an integer operation caused a carry out of the "
               //L"most significant bit of the result.";
        break;

      case EXCEPTION_INVALID_DISPOSITION:
        desc = L"\t<< EXCEPTION_INVALID_DISPOSITION >>";
               //L"An exception handler returned an invalid disposition to the "
               //L"exception dispatcher.";
        break;

      case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        desc = L"\t<< EXCEPTION_NONCONTINUABLE_EXCEPTION >>";
               //L"The thread tried to continue execution after a noncontinuable "
               //L"exception occurred.";
        break;

      case EXCEPTION_PRIV_INSTRUCTION:
        desc = L"\t<< EXCEPTION_PRIV_INSTRUCTION >>";
               //L"The thread tried to execute an instruction whose operation is "
               //L"not allowed in the current machine mode.";
        break;

      case EXCEPTION_SINGLE_STEP:
        desc = L"\t<< EXCEPTION_SINGLE_STEP >>";
               //L"A trace trap or other single-instruction mechanism signaled "
               //L"that one instruction has been executed.";
        break;

      case EXCEPTION_STACK_OVERFLOW:
        desc = L"\t<< EXCEPTION_STACK_OVERFLOW >>";
               //L"The thread used up its stack.";
        break;
    }

    log_entry.append (L"-----------------------------------------------------------\n");
    log_entry_format (L"[! Except !] %s\n", desc));
    log_entry.append (L"-----------------------------------------------------------\n");
  }

  wchar_t* wszThreadDescription = nullptr;

  if (SUCCEEDED (GetCurrentThreadDescription (&wszThreadDescription)))
  {
    if (         wszThreadDescription != nullptr &&
         wcslen (wszThreadDescription)              )
    {
      log_entry_format ( L"[  Thread  ]  ~ Name.....: \"%s\"\n",
                           wszThreadDescription));
                LocalFree (wszThreadDescription);
    }
  }

  log_entry_format (L"[ FaultMod ]  # File.....: '%hs'\n",  szModName));
#ifndef _WIN64
  log_entry_format (L"[ FaultMod ]  * EIP Addr.: %hs+%08Xh\n", pszShortName, ip-BaseAddr));

  log_entry_format ( L"[StackFrame] <-> Eip=%08xh, Esp=%08xh, Ebp=%08xh\n",
                       ip,
                         ExceptionInfo->ContextRecord->Esp,
                           ExceptionInfo->ContextRecord->Ebp ));
  log_entry_format ( L"[StackFrame] >-< Esi=%08xh, Edi=%08xh\n",
                       ExceptionInfo->ContextRecord->Esi,
                         ExceptionInfo->ContextRecord->Edi ));

  log_entry_format ( L"[  GP Reg  ]   eax:    0x%08x    ebx:    0x%08x\n",
                     ExceptionInfo->ContextRecord->Eax,
                     ExceptionInfo->ContextRecord->Ebx ));
  log_entry_format ( L"[  GP Reg  ]   ecx:    0x%08x    edx:    0x%08x\n",
                     ExceptionInfo->ContextRecord->Ecx,
                     ExceptionInfo->ContextRecord->Edx ));
  log_entry_format ( L"[ GP Flags ]   EFlags: 0x%08x\n",
                     ExceptionInfo->ContextRecord->EFlags ));
#else
  log_entry_format ( L"[ FaultMod ]  * RIP Addr.: %hs+%ph\n",
                       pszShortName, (LPVOID)((uintptr_t)ip-(uintptr_t)BaseAddr)));

  log_entry_format ( L"[StackFrame] <-> Rip=%012llxh, Rsp=%012llxh, Rbp=%012llxh\n",
                     ip,
                       ExceptionInfo->ContextRecord->Rsp,
                         ExceptionInfo->ContextRecord->Rbp ));
  log_entry_format ( L"[StackFrame] >-< Rsi=%012llxh, Rdi=%012llxh\n",
                     ExceptionInfo->ContextRecord->Rsi,
                       ExceptionInfo->ContextRecord->Rdi ));

  log_entry_format ( L"[  GP Reg  ]   rax:    0x%012llx    rbx:    0x%012llx\n",
                     ExceptionInfo->ContextRecord->Rax,
                     ExceptionInfo->ContextRecord->Rbx ));
  log_entry_format ( L"[  GP Reg  ]   rcx:    0x%012llx    rdx:    0x%012llx\n",
                     ExceptionInfo->ContextRecord->Rcx,
                     ExceptionInfo->ContextRecord->Rdx ));
  log_entry_format ( L"[  GP Reg  ]   r8:     0x%012llx    r9:     0x%012llx\n",
                     ExceptionInfo->ContextRecord->R8,
                     ExceptionInfo->ContextRecord->R9 ));
  log_entry_format ( L"[  GP Reg  ]   r10:    0x%012llx    r11:    0x%012llx\n",
                     ExceptionInfo->ContextRecord->R10,
                     ExceptionInfo->ContextRecord->R11 ));
  log_entry_format ( L"[  GP Reg  ]   r12:    0x%012llx    r13:    0x%012llx\n",
                     ExceptionInfo->ContextRecord->R12,
                     ExceptionInfo->ContextRecord->R13 ));
  log_entry_format ( L"[  GP Reg  ]   r14:    0x%012llx    r15:    0x%012llx\n",
                     ExceptionInfo->ContextRecord->R14,
                     ExceptionInfo->ContextRecord->R15 ));
  log_entry_format ( L"[ GP Flags ]   EFlags: 0x%08x\n",
                  ExceptionInfo->ContextRecord->EFlags ));

  if ( ExceptionInfo->ContextRecord->Dr0 != 0x0 ||
       ExceptionInfo->ContextRecord->Dr1 != 0x0 )
  {
    log_entry_format ( L"[ DebugReg ]   dr0:    0x%012llx    dr1:    0x%012llx\n",
                    ExceptionInfo->ContextRecord->Dr0,
                    ExceptionInfo->ContextRecord->Dr1 ));
  }

  if ( ExceptionInfo->ContextRecord->Dr2 != 0x0 ||
       ExceptionInfo->ContextRecord->Dr3 != 0x0 )
  {
    log_entry_format ( L"[ DebugReg ]   dr2:    0x%012llx    dr3:    0x%012llx\n",
                    ExceptionInfo->ContextRecord->Dr2,
                    ExceptionInfo->ContextRecord->Dr3 ));
  }

  if ( ExceptionInfo->ContextRecord->Dr6 != 0x0 ||
       ExceptionInfo->ContextRecord->Dr7 != 0x0 )
  {
    log_entry_format ( L"[ DebugReg ]   dr6:    0x%012llx    dr7:    0x%012llx\n",
                    ExceptionInfo->ContextRecord->Dr6,
                    ExceptionInfo->ContextRecord->Dr7 ));
  }
#endif

  log_entry.append (
    L"-----------------------------------------------------------\n");

//SK_SymUnloadModule (hProc, BaseAddr);

  CONTEXT ctx (*ExceptionInfo->ContextRecord);


#ifdef _WIN64
  STACKFRAME64 stackframe = { };
               stackframe.AddrStack.Offset = ctx.Rsp;
               stackframe.AddrFrame.Offset = ctx.Rbp;
#else
  STACKFRAME   stackframe = { };
               stackframe.AddrStack.Offset = ctx.Esp;
               stackframe.AddrFrame.Offset = ctx.Ebp;
#endif

  stackframe.AddrPC.Mode   = AddrModeFlat;
  stackframe.AddrPC.Offset = ip;

  stackframe.AddrStack.Mode = AddrModeFlat;
  stackframe.AddrFrame.Mode = AddrModeFlat;


  //char szTopFunc [512] = { };

  BOOL ret = TRUE;

  
  size_t max_symbol_len = 0,
         max_module_len = 0,
         max_file_len   = 0;

  struct stack_entry_s {
    stack_entry_s ( char* szShortName, char* szSymbolName,
                    char* szFileName,  int   uiLineNum )
    {
      strncpy_s ( short_mod_name, 64,
                  szShortName,    _TRUNCATE );
      strncpy_s ( symbol_name,    512,
                  szSymbolName,   _TRUNCATE );
      strncpy_s ( file_name,      MAX_PATH,
                  szFileName,     _TRUNCATE );

      mod_len  = strlen (short_mod_name);
      sym_len  = strlen (symbol_name);
      file_len = strlen (file_name);

      line_number = uiLineNum;
    }

    stack_entry_s ( char* szShortName, char* szSymbolName )
    {
      strncpy_s ( short_mod_name, 64,
                  szShortName,    _TRUNCATE );
      strncpy_s ( symbol_name,    512,
                  szSymbolName,   _TRUNCATE );

      mod_len  = strlen (short_mod_name);
      sym_len  = strlen (symbol_name);

     *file_name   = '\0';
      file_len    = 0;
      line_number = 0;
    }

    char     short_mod_name [64];           size_t mod_len;
    char     symbol_name    [512];          size_t sym_len;
    char     file_name      [MAX_PATH + 1]; size_t file_len;
    unsigned line_number;
  };

  std::vector <stack_entry_s> stack_entries;
  stack_entries.reserve (16);

  do
  {
    ip = stackframe.AddrPC.Offset;

    if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
              reinterpret_cast <LPCWSTR> (ip),
                                 &hModSource ) )
    {
      GetModuleFileNameA (hModSource, szModName, MAX_PATH);
    }

    MODULEINFO mod_info = { };

    GetModuleInformation (
      GetCurrentProcess (), hModSource, &mod_info, sizeof (mod_info)
    );

#ifdef _WIN64
    BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else
    BaseAddr = (DWORD)  mod_info.lpBaseOfDll;
#endif

    strncpy_s     (szDupName, MAX_PATH * 2, szModName, _TRUNCATE);
    pszShortName = szDupName;

    PathStripPathA (pszShortName);


    SK_SymLoadModule ( hProc,
                         nullptr,
                          pszShortName,
                            nullptr,
                              BaseAddr,
                                mod_info.SizeOfImage );

    SYMBOL_INFO_PACKAGE sip = { };

    sip.si.SizeOfStruct = sizeof SYMBOL_INFO;
    sip.si.MaxNameLen   = sizeof sip.name;

    DWORD64 Displacement = 0;

    if ( SymFromAddr ( hProc,
             static_cast <DWORD64> (ip),
                           &Displacement,
                             &sip.si ) )
    {
      DWORD Disp = 0x00UL;

#ifdef _WIN64
      IMAGEHLP_LINE64 ihl              = {                    };
                      ihl.SizeOfStruct = sizeof IMAGEHLP_LINE64;
#else
      IMAGEHLP_LINE   ihl              = {                  };
                      ihl.SizeOfStruct = sizeof IMAGEHLP_LINE;
#endif
      BOOL bFileAndLine =
        SK_SymGetLineFromAddr ( hProc, ip, &Disp, &ihl );

      auto AddStackEntry =
        [&](void) -> void
      {
        stack_entries.emplace_back (
          stack_entry_s (
            pszShortName, sip.si.Name
          )
        );

        max_symbol_len =
          std::max (stack_entries.back ().sym_len, max_symbol_len);
        max_module_len =
          std::max (stack_entries.back ().mod_len, max_module_len);
      };

      auto AddStackEntryWithFileAndLine =
      [&](void) -> void
      {
        stack_entries.emplace_back (
          stack_entry_s (
            pszShortName, sip.si.Name,
              ihl.FileName, ihl.LineNumber
          )
        );

        auto& entry =
          stack_entries.back ();

        max_symbol_len =
          std::max (entry.sym_len, max_symbol_len);
        max_module_len =
          std::max (entry.mod_len, max_module_len);
        max_file_len =
          std::max (entry.file_len, max_file_len);
      };

      if (bFileAndLine)
      {
        AddStackEntryWithFileAndLine ();
      }

      else
      {
        AddStackEntry ();
      }

      ////if (StrStrIA (sip.si.Name, "Scaleform"))
      ////  scaleform = true;

      //if (*szTopFunc == '\0')
      //{
      //  strncpy_s ( szTopFunc,     512,
      //                sip.si.Name, _TRUNCATE );
      //}
    }

  //SK_SymUnloadModule (hProc, BaseAddr);

    ret =
      SK_StackWalk ( SK_RunLHIfBitness ( 32, IMAGE_FILE_MACHINE_I386,
                                             IMAGE_FILE_MACHINE_AMD64 ),
                       hProc,
                         SK_GetCurrentThread (),
                           &stackframe,
                             &ctx,
                               nullptr, nullptr,
                                 nullptr, nullptr );
  } while (ret != FALSE);

  for (auto& stack_entry : stack_entries)
  {
    if (stack_entry.line_number == 0)
    {
      log_entry_format ( L" %*hs >  %#*hs\n",
                           max_module_len,
                           stack_entry.short_mod_name,
                             max_symbol_len,
                             stack_entry.symbol_name ));
    }

    else
    {
      log_entry_format
              ( L" %*hs >  %#*hs  <%*hs:%lu>\n",
               max_module_len,   stack_entry.short_mod_name,
                 max_symbol_len, stack_entry.symbol_name,
                    max_file_len, stack_entry.file_name,
                      stack_entry.line_number ));
    }
  }

  log_entry.append (L"-----------------------------------------------------------\n\n");

  return log_entry;
}

LONG
WINAPI
SK_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo )
{
  // Sadly, if this ever happens, there's no way to report the problem, so just
  //   terminate with exit code = -666.
  if ( ReadAcquire (&__SK_DLL_Ending) != 0 )
  {
    if (SK_IsDebuggerPresent ())
    {
      __debugbreak ();
    }

    else
    {
      SK_SelfDestruct     (   );
      SK_TerminateProcess (0x0);
      SK_ExitProcess      (0x0);
    }
  }

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  CONTEXT&          last_ctx    = pTLS->debug.last_ctx;
  EXCEPTION_RECORD& last_exc    = pTLS->debug.last_exc;


  //if ( (ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) &&
  //     SK_IsDebuggerPresent () )
  //{
  //  __debugbreak ();
  //}

  std::wstring log_entry =
    SK_SEH_SummarizeException (ExceptionInfo, true);

  crash_log.Log   (L"\n\tUnhandled Top-Level Exception (%x):\n",
                   ExceptionInfo->ExceptionRecord->ExceptionCode);


  crash_log.LogEx  (false, L"%ws", log_entry.c_str ());

  fflush (crash_log.fLog);


  // On second chance it's pretty clear that no exception handler exists,
  //   terminate the software.
  const bool repeated = ( !memcmp (&last_ctx, ExceptionInfo->ContextRecord,   sizeof CONTEXT)         ) &&
                        ( !memcmp (&last_exc, ExceptionInfo->ExceptionRecord, sizeof EXCEPTION_RECORD) );
  const bool non_continue = ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;

  if ( (repeated || non_continue) /*&&
      (ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT)*/ )
  {
    if (! config.system.handle_crashes)
    {
      SK_TerminateProcess (0xdeadbeef);
    }

    bool& last_chance = pTLS->debug.last_chance;
          last_chance = true;

    WIN32_FIND_DATA fd     = {                  };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;

    wchar_t   wszFindPattern [MAX_PATH * 2] = { };
    lstrcatW (wszFindPattern, SK_GetConfigPath ());
    lstrcatW (wszFindPattern,    LR"(logs\*.log)");

    hFind =
      FindFirstFileW (wszFindPattern, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      wchar_t wszBaseDir [MAX_PATH * 2] = { };
      wchar_t wszOutDir  [MAX_PATH * 2] = { };
      wchar_t wszTime    [MAX_PATH + 2] = { };

      lstrcatW (wszBaseDir, SK_GetConfigPath ( ));
      lstrcatW (wszBaseDir, LR"(logs\)");

      wcscpy   (wszOutDir, wszBaseDir);
      lstrcatW (wszOutDir, LR"(crash\)");

             time_t now = { };
      struct tm     now_tm;

                      time (&now);
      localtime_s (&now_tm, &now);

      static const wchar_t* wszTimestamp =
        L"%m-%d-%Y__%H'%M'%S\\";

      wcsftime (wszTime, MAX_PATH, wszTimestamp, &now_tm);
      lstrcatW (wszOutDir, wszTime);

      wchar_t wszOrigPath [MAX_PATH * 2 + 1] = { };
      wchar_t wszDestPath [MAX_PATH * 2 + 1] = { };

      size_t files = 0UL;

      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          *wszOrigPath = L'\0';
          *wszDestPath = L'\0';

          lstrcatW (wszOrigPath, wszBaseDir);
          lstrcatW (wszOrigPath, fd.cFileName);

          lstrcatW (wszDestPath, wszOutDir);
          lstrcatW (wszDestPath, fd.cFileName);

          SK_CreateDirectories (wszDestPath);

          if (! StrStrIW (wszOrigPath, L"installer.log"))
          {
            iSK_Logger*       log_file = nullptr;

            if (dll_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              log_file = &dll_log;
            }

            else if (steam_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              log_file = &steam_log;
            }

            else if (crash_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              log_file = &crash_log;
            }

            else if (game_debug.name.find (fd.cFileName) != std::wstring::npos)
            {
              log_file = &game_debug;
            }

            else if (tex_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              log_file = &tex_log;
            }

            else if (budget_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              log_file = &budget_log;
            }

            // There's a small chance that we may crash prior to loading CEGUI's DLLs, in which case
            //   trying to grab a static reference to the Logger Singleton would blow stuff up.
            //
            //   Avoid this by counting the number of frames actually drawn.
            if (config.cegui.enable && StrStrW (fd.cFileName, L"CEGUI.log") && SK_GetFramesDrawn () > 120)
            {
              const wchar_t* wszLogFile = L"CEGUI.log";

              // File has been relocated yet
              if (GetFileAttributesW (L"CEGUI.log") == INVALID_FILE_ATTRIBUTES)
              {
                wszLogFile = wszOrigPath;
              }

              SK_File_FullCopy (wszLogFile, wszDestPath);

              CEGUI::Logger::getDllSingleton ().
                setLogFilename (
                  reinterpret_cast <const CEGUI::utf8 *> (
                    SK_WideCharToUTF8 (wszDestPath).c_str ()
                  ),
                    true
                );
            }

            else
            {
              ++files;

              bool locked = false;

              if ( log_file       != nullptr &&
                   log_file->fLog != nullptr &&
                   log_file       != &crash_log )
              {
                if (! InterlockedCompareExchange (&log_file->relocated, 1, 0))
                {
                                            log_file->lockless = false;
                  fflush (log_file->fLog);  log_file->lock ();
                  fclose (log_file->fLog);

                  locked = true;
                }
                else
                  SK_Thread_SpinUntilAtomicMin (&log_file->relocated, 2);
              }

              SK_File_FullCopy         (wszOrigPath, wszDestPath);
              SK_File_SetNormalAttribs (wszDestPath);

              if ( log_file != nullptr &&
                   log_file->fLog      &&
                   log_file != &crash_log )
              {
                log_file->name = wszDestPath;
                log_file->fLog = _wfopen (log_file->name.c_str (), L"a");

                if (locked)
                {
                  locked = false;
                  log_file->unlock ();
                }

                InterlockedIncrement (&log_file->relocated);
              }
            }
          }

          if (! DeleteFileW (wszOrigPath))
          {
            //SK_File_SetHidden (wszOrigPath, true);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    crash_log.silent = true;
    crash_log.lines++;

    //if (! (crash_log.initialized && crash_log.silent))
    {
      if (! crash_sound.play ())
      {
        // If we cannot play the sound, then give a message box so we don't
        //   appear to have vanished without a trace...
        SK_MessageBox ( L"Application Crashed (Unable to Play Sound)!",
                          L"Special K Crash Handler [Abnormal Termination]",
                            MB_OK | MB_ICONERROR );
      }
    }

    InterlockedExchange (&__SK_Crashed, 1);

    // CEGUI is potentially the reason we crashed, so ...
    //   set it back to its original state.
    if (! config.cegui.orig_enable)
      config.cegui.enable = false;

    if ( config.cegui.enable && config.cegui.frames_drawn < 90 &&
         SK_GetFramesDrawn () > config.cegui.frames_drawn )
    {
      SK_LOG0 ( ( L"*** CEGUI is the suspected cause of this crash,"
                  L" disabling..." ),
                    L"Prognostic" );
      config.cegui.enable = false;
    }

    // Shutdown the module gracefully
    SK_SelfDestruct ();

    return EXCEPTION_EXECUTE_HANDLER;
  }

  last_ctx = *ExceptionInfo->ContextRecord;
  last_exc = *ExceptionInfo->ExceptionRecord;


  if ( ExceptionInfo->ExceptionRecord->ExceptionFlags == 0 )
  {
    return EXCEPTION_CONTINUE_EXECUTION;
  }

  else
  {
    return EXCEPTION_EXECUTE_HANDLER;
  }
}

ULONG
SK_GetSymbolNameFromModuleAddr ( HMODULE hMod,   uintptr_t addr,
                                 char*   pszOut, ULONG     ulLen )
{
  ULONG ret = 0;

  if (! dbghelp_callers.count (hMod))
  {
    MODULEINFO mod_info = { };

    GetModuleInformation (
      GetCurrentProcess (), hMod, &mod_info, sizeof (mod_info)
    );

    auto  BaseAddr =
      (DWORD64)mod_info.lpBaseOfDll;

    char szModName [MAX_PATH + 2] = {  };

    GetModuleFileNameA  ( hMod,
                            szModName,
                              MAX_PATH );

    char* pszShortName = szModName;

    PathStripPathA (pszShortName);

    SymLoadModule64 ( GetCurrentProcess (),
                        nullptr,
                          pszShortName,
                            nullptr,
                              BaseAddr,
                                mod_info.SizeOfImage );

    dbghelp_callers.insert (hMod);
  }

  HANDLE hProc =
    SK_GetCurrentProcess ();

  DWORD64                           ip;

  UIntPtrToInt64 (addr, (int64_t *)&ip);

  SYMBOL_INFO_PACKAGE sip                 = {                };
                      sip.si.SizeOfStruct = sizeof SYMBOL_INFO;
                      sip.si.MaxNameLen   = sizeof sip.name;

  DWORD64 Displacement = 0;

  if ( SymFromAddr ( hProc,
                       ip,
                         &Displacement,
                           &sip.si ) )
  {
    *pszOut = '\0';

    strncat             ( pszOut, sip.si.Name,
                            std::min (ulLen, sip.si.NameLen) );
    ret =
      static_cast <ULONG> (strlen (pszOut));
  }

  else
  {
    *pszOut = '\0';
    ret     = 0;
  }

  return ret;
}

void
WINAPI
SK_SymRefreshModuleList ( HANDLE hProc )
{
  std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

  SymRefreshModuleList (hProc);
}

using SteamAPI_SetBreakpadAppID_pfn = void (__cdecl *)( uint32_t unAppID );
using SteamAPI_UseBreakpadCrashHandler_pfn = void (__cdecl *)( char const *pchVersion, char const *pchDate, 
                                                              char const *pchTime,    bool        bFullMemoryDumps,
                                                              void       *pvContext,  LPVOID      m_pfnPreMinidumpCallback );

SteamAPI_SetBreakpadAppID_pfn        SteamAPI_SetBreakpadAppID_NEVER        = nullptr;
SteamAPI_UseBreakpadCrashHandler_pfn SteamAPI_UseBrakepadCrashHandler_NEVER = nullptr;

void
__cdecl
SteamAPI_SetBreakpadAppID_Detour ( uint32_t unAppId )
{
  UNREFERENCED_PARAMETER (unAppId);
}

void
__cdecl
SteamAPI_UseBreakpadCrashHandler_Detour ( char const *pchVersion,
                                          char const *pchDate,
                                          char const *pchTime,
                                          bool        bFullMemoryDumps,
                                          void       *pvContext,
                                               LPVOID m_pfnPreMinidumpCallback )
{
  UNREFERENCED_PARAMETER (pchVersion);
  UNREFERENCED_PARAMETER (pchDate);
  UNREFERENCED_PARAMETER (pchTime);
  UNREFERENCED_PARAMETER (bFullMemoryDumps);
  UNREFERENCED_PARAMETER (pvContext);
  UNREFERENCED_PARAMETER (m_pfnPreMinidumpCallback);
}

void
SK_BypassSteamCrashHandler (void)
{
  if (! config.steam.silent)
  {
    const wchar_t* wszSteamDLL =
      SK_RunLHIfBitness (64, L"steam_api64.dll",
                             L"steam_api.dll");

    if (SK_File_GetSize (wszSteamDLL) > 0)
    {
      if (SK_Modules.LoadLibraryLL (wszSteamDLL))
      {
        crash_log.Log (L"Disabling Steam Breakpad...");

        SK_CreateDLLHook2 (       wszSteamDLL,
                                  "SteamAPI_UseBreakpadCrashHandler",
                                   SteamAPI_UseBreakpadCrashHandler_Detour,
          static_cast_p2p <void> (&SteamAPI_UseBrakepadCrashHandler_NEVER) );
      
        SK_CreateDLLHook2 (       wszSteamDLL,
                                  "SteamAPI_SetBreakpadAppID",
                                   SteamAPI_SetBreakpadAppID_Detour,
          static_cast_p2p <void> (&SteamAPI_SetBreakpadAppID_NEVER) );
      }
    }
  }
}


extern void SK_DbgHlp_Init (void);

void
CrashHandler::InitSyms (void)
{
  static volatile LONG               init = 0L;
  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SymCleanup (SK_GetCurrentProcess ());

    SymInitialize (
      SK_GetCurrentProcess (),
        nullptr,
          FALSE );

    SK_RunOnce (SK_SymSetOpts ());

    SymRefreshModuleList (SK_GetCurrentProcess ());

    Init ();

    ///if (config.system.handle_crashes)
    ///{
    ///  if (! config.steam.silent)
    ///    SK_BypassSteamCrashHandler ();
    ///}
  }
}
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
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <SpecialK/diagnostics/crash_handler.h>

#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/log.h>
#include <SpecialK/resource.h>
#include <SpecialK/utility.h>
#include <SpecialK/tls.h>

#include <Windows.h>
#include <intsafe.h>
#include <Shlwapi.h>
#include <algorithm>
#include <memory>
#include <ctime>

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>

//#define STRICT_COMPLIANCE

// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _NO_CVCONST_H
#include <dbghelp.h>

#pragma comment( lib, "dbghelp.lib" )

extern HMODULE __stdcall SK_GetDLL (void);

using namespace SK::Diagnostics;

typedef LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *SetUnhandledExceptionFilter_pfn)(
    _In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
);

SetUnhandledExceptionFilter_pfn SetUnhandledExceptionFilter_Original = nullptr;


LONG
WINAPI
SK_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo );

LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter_Detour (_In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
  UNREFERENCED_PARAMETER (lpTopLevelExceptionFilter);

  return SetUnhandledExceptionFilter_Original (SK_TopLevelExceptionFilter);
}


void
CrashHandler::Reinstall (void)
{
  SetErrorMode                (SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);
  SetUnhandledExceptionFilter (SK_TopLevelExceptionFilter);
}


struct {
  HGLOBAL  ref = 0;
  uint8_t* buf = nullptr;
} static crash_sound;


void
CrashHandler::Init (void)
{
  HRSRC   default_sound =
    FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_CRASH), L"WAVE");

  if (default_sound != nullptr)
  {
    crash_sound.ref   =
      LoadResource (SK_GetDLL (), default_sound);

    if (crash_sound.ref != 0)
    {
      crash_sound.buf =
        static_cast <uint8_t *> (LockResource (crash_sound.ref));
    }
  }

  if (! crash_log.initialized)
  {
    crash_log.flush_freq = 0;
    crash_log.lockless   = true;
    crash_log.init (L"logs/crash.log", L"w");
  }

  SK_CreateDLLHook2 (       L"kernel32.dll",
                             "SetUnhandledExceptionFilter",
                              SetUnhandledExceptionFilter_Detour,
reinterpret_cast <LPVOID *> (&SetUnhandledExceptionFilter_Original) );

  SymSetOptions ( SYMOPT_CASE_INSENSITIVE | SYMOPT_LOAD_LINES    | SYMOPT_UNDNAME |
                  SYMOPT_NO_PROMPTS       | SYMOPT_DEFERRED_LOADS );

  SymRefreshModuleList (GetCurrentProcess ());

  //SymInitialize (
  //  GetCurrentProcess (),
  //    NULL,
  //      TRUE );

  Reinstall ();
}

void
CrashHandler::Shutdown (void)
{
  crash_log.close ();
}



std::string
SK_GetSymbolNameFromModuleAddr (HMODULE hMod, uintptr_t addr)
{
  std::string ret = "";

  HANDLE hProc =
    GetCurrentProcess ();

#ifdef _WIN64
  DWORD64  ip = addr;
#else
  DWORD    ip = addr;
#endif

#ifdef _WIN64
  DWORD64 BaseAddr =
    SymGetModuleBase64 ( hProc, ip );
#else
  DWORD BaseAddr =
    SymGetModuleBase   ( hProc, ip );
#endif

  char szModName [MAX_PATH + 2] = { };

  GetModuleFileNameA   ( hMod, szModName, MAX_PATH );

  char* szDupName    = _strdup (szModName);
  char* pszShortName = szDupName + lstrlenA (szDupName);

  while (  pszShortName      >  szDupName &&
         *(pszShortName - 1) != '\\')
    --pszShortName;

#ifdef _WIN64
  SymLoadModule64 ( hProc,
                      nullptr,
                        pszShortName,
                          nullptr,
                            BaseAddr,
                              0 );
#else
  SymLoadModule ( hProc,
                    nullptr,
                      pszShortName,
                        nullptr,
                          BaseAddr,
                            0 );
#endif

  SYMBOL_INFO_PACKAGE sip;
  sip.si.SizeOfStruct = sizeof SYMBOL_INFO;
  sip.si.MaxNameLen   = sizeof sip.name;

  DWORD64 Displacement = 0;

  if ( SymFromAddr ( hProc,
         static_cast <DWORD64> (ip),
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

LONG
WINAPI
SK_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo )
{
  bool scaleform = false;

#if 1
  SymSetOptions ( SYMOPT_CASE_INSENSITIVE | SYMOPT_LOAD_LINES    | SYMOPT_UNDNAME |
                  SYMOPT_NO_PROMPTS       | SYMOPT_DEFERRED_LOADS );
#else
  SymSetOptions ( SYMOPT_ALLOW_ZERO_ADDRESS | SYMOPT_LOAD_LINES |
                  SYMOPT_LOAD_ANYTHING      | SYMOPT_UNDNAME    |
                  SYMOPT_DEFERRED_LOADS );
#endif


  SymRefreshModuleList (GetCurrentProcess ());
  //SymInitialize (
  //  GetCurrentProcess (),
  //    NULL,
  //      TRUE );

  bool&             last_chance = SK_TLS_Bottom ()->debug.last_chance;

  CONTEXT&          last_ctx    = SK_TLS_Bottom ()->debug.last_ctx;
  EXCEPTION_RECORD& last_exc    = SK_TLS_Bottom ()->debug.last_exc;

  std::wstring desc;

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

  HMODULE hModSource = nullptr;
  char    szModName [MAX_PATH] = { };
  HANDLE  hProc                = GetCurrentProcess ();

  SymRefreshModuleList ( hProc );

#ifdef _WIN64
  DWORD64  ip = ExceptionInfo->ContextRecord->Rip;
#else
  DWORD    ip = ExceptionInfo->ContextRecord->Eip;
#endif

  if (GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            (LPCWSTR)ip,
                              &hModSource )) {
    GetModuleFileNameA (hModSource, szModName, MAX_PATH);
  }

#ifdef _WIN64
  DWORD64 BaseAddr =
    SymGetModuleBase64 ( hProc, ip );
#else
  DWORD BaseAddr =
    SymGetModuleBase   ( hProc, ip );
#endif

  char* szDupName    = _strdup (szModName);
  char* pszShortName = szDupName + lstrlenA (szDupName);

  while (  pszShortName      >  szDupName &&
         *(pszShortName - 1) != '\\')
    --pszShortName;

  crash_log.Log (L"-----------------------------------------------------------");
  crash_log.Log (L"[! Except !] %s", desc.c_str ());
  crash_log.Log (L"-----------------------------------------------------------");
  crash_log.Log (L"[ FaultMod ]  # File.....: '%hs'",  szModName);
#ifndef _WIN64
  crash_log.Log (L"[ FaultMod ]  * EIP Addr.: %hs+%08Xh", pszShortName, ip-BaseAddr);

  crash_log.Log ( L"[StackFrame] <-> Eip=%08xh, Esp=%08xh, Ebp=%08xh",
                  ip,
                    ExceptionInfo->ContextRecord->Esp,
                      ExceptionInfo->ContextRecord->Ebp );
  crash_log.Log ( L"[StackFrame] >-< Esi=%08xh, Edi=%08xh",
                  ExceptionInfo->ContextRecord->Esi,
                    ExceptionInfo->ContextRecord->Edi );

  crash_log.Log ( L"[  GP Reg  ]       eax:     0x%08x",
                  ExceptionInfo->ContextRecord->Eax );
  crash_log.Log ( L"[  GP Reg  ]       ebx:     0x%08x",
                  ExceptionInfo->ContextRecord->Ebx );
  crash_log.Log ( L"[  GP Reg  ]       ecx:     0x%08x",
                  ExceptionInfo->ContextRecord->Ecx );
  crash_log.Log ( L"[  GP Reg  ]       edx:     0x%08x",
                  ExceptionInfo->ContextRecord->Edx );
  crash_log.Log ( L"[ GP Flags ]       EFlags:  0x%08x",
                  ExceptionInfo->ContextRecord->EFlags );
#else
  crash_log.Log (L"[ FaultMod ]  * RIP Addr.: %hs+%ph", pszShortName, (LPVOID)((uintptr_t)ip-(uintptr_t)BaseAddr));

  crash_log.Log ( L"[StackFrame] <-> Rip=%012llxh, Rsp=%012llxh, Rbp=%012llxh",
                  ip,
                    ExceptionInfo->ContextRecord->Rsp,
                      ExceptionInfo->ContextRecord->Rbp );
  crash_log.Log ( L"[StackFrame] >-< Rsi=%012llxh, Rdi=%012llxh",
                  ExceptionInfo->ContextRecord->Rsi,
                    ExceptionInfo->ContextRecord->Rdi );

  crash_log.Log ( L"[  GP Reg  ]       rax:     0x%012llx",
                  ExceptionInfo->ContextRecord->Rax );
  crash_log.Log ( L"[  GP Reg  ]       rbx:     0x%012llx",
                  ExceptionInfo->ContextRecord->Rbx );
  crash_log.Log ( L"[  GP Reg  ]       rcx:     0x%012llx",
                  ExceptionInfo->ContextRecord->Rcx );
  crash_log.Log ( L"[  GP Reg  ]       rdx:     0x%012llx",
                  ExceptionInfo->ContextRecord->Rdx );
  crash_log.Log ( L"[  GP Reg  ]        r8:     0x%012llx       r9:      0x%012llx",
                  ExceptionInfo->ContextRecord->R8,
                  ExceptionInfo->ContextRecord->R9 );
  crash_log.Log ( L"[  GP Reg  ]       r10:     0x%012llx      r11:      0x%012llx",
                  ExceptionInfo->ContextRecord->R10,
                  ExceptionInfo->ContextRecord->R11 );
  crash_log.Log ( L"[  GP Reg  ]       r12:     0x%012llx      r13:      0x%012llx",
                  ExceptionInfo->ContextRecord->R12,
                  ExceptionInfo->ContextRecord->R13 );
  crash_log.Log ( L"[  GP Reg  ]       r14:     0x%012llx      r15:      0x%012llx",
                  ExceptionInfo->ContextRecord->R14,
                  ExceptionInfo->ContextRecord->R15 );
  crash_log.Log ( L"[ GP Flags ]       EFlags:  0x%08x",
                  ExceptionInfo->ContextRecord->EFlags );
#endif

  crash_log.Log (
    L"-----------------------------------------------------------");

#ifdef _WIN64
  SymUnloadModule64 (hProc, BaseAddr);
#else
  SymUnloadModule   (hProc, BaseAddr);
#endif

  free (szDupName);

  CONTEXT ctx (*ExceptionInfo->ContextRecord);

#ifdef _WIN64
  STACKFRAME64 stackframe;
#else
  STACKFRAME   stackframe;
#endif

  stackframe.AddrPC.Mode   = AddrModeFlat;
  stackframe.AddrPC.Offset = ip;

  stackframe.AddrStack.Mode = AddrModeFlat;
#ifdef _WIN64
  stackframe.AddrStack.Offset = ctx.Rsp;
#else
  stackframe.AddrStack.Offset = ctx.Esp;
#endif

  stackframe.AddrFrame.Mode = AddrModeFlat;
#ifdef _WIN64
  stackframe.AddrFrame.Offset = ctx.Rbp;
#else
  stackframe.AddrFrame.Offset = ctx.Ebp;
#endif

  std::string top_func = "";

  BOOL ret = TRUE;

  do
  {
    SymRefreshModuleList ( hProc );

    ip = stackframe.AddrPC.Offset;

    if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
              reinterpret_cast <LPCWSTR> (ip),
                                 &hModSource ) )
    {
      GetModuleFileNameA (hModSource, szModName, MAX_PATH);
    }

#ifdef _WIN64
    BaseAddr =
      SymGetModuleBase64 ( hProc, ip );
#else
    BaseAddr =
      SymGetModuleBase   ( hProc, ip );
#endif

    szDupName    = _strdup (szModName);
    pszShortName = szDupName + lstrlenA (szDupName);

    while (  pszShortName      >  szDupName &&
           *(pszShortName - 1) != '\\')
      --pszShortName;

#ifdef _WIN64
    SymLoadModule64 ( hProc,
                        nullptr,
                          pszShortName,
                            nullptr,
                              BaseAddr,
                                0 );
#else
    SymLoadModule ( hProc,
                      nullptr,
                        pszShortName,
                          nullptr,
                            BaseAddr,
                              0 );
#endif

    SYMBOL_INFO_PACKAGE sip;
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
      IMAGEHLP_LINE64 ihl64;
      ihl64.SizeOfStruct = sizeof IMAGEHLP_LINE64;

      BOOL  bFileAndLine =
        SymGetLineFromAddr64 ( hProc, ip, &Disp, &ihl64 );

      if (bFileAndLine)
      {
        crash_log.Log ( L"[-(Source)-] [!] {%24hs} %#64hs  <%hs:%lu> ",
                        pszShortName,
                          sip.si.Name,
                            ihl64.FileName,
                              ihl64.LineNumber );
#else
      IMAGEHLP_LINE ihl              = {                  };
                    ihl.SizeOfStruct = sizeof IMAGEHLP_LINE;

      const BOOL bFileAndLine =
        SymGetLineFromAddr ( hProc, ip, &Disp, &ihl );

      if (bFileAndLine)
      {
        crash_log.Log ( L"[-(Source)-] [!] {%24hs}  %#64hs  <%hs:%lu>",
                        pszShortName,
                          sip.si.Name,
                            ihl.FileName,
                              ihl.LineNumber );
#endif
      }

      else
      {
        crash_log.Log ( L"[--(Name)--] [!] {%24hs}  %#64hs",
                        pszShortName,
                          sip.si.Name );
      }

      if (StrStrIA (sip.si.Name, "Scaleform"))
        scaleform = true;

      if (top_func == "")
        top_func = sip.si.Name;

      free (szDupName);
    }

#ifdef _WIN64
    SymUnloadModule64 (hProc, BaseAddr);
#else
    SymUnloadModule   (hProc, BaseAddr);
#endif

#ifdef _WIN64
    ret =
      StackWalk64 ( IMAGE_FILE_MACHINE_AMD64,
                      hProc,
                        GetCurrentThread (),
                          &stackframe,
                            &ctx,
                              nullptr, nullptr,
                                nullptr, nullptr );
#else
    ret =
      StackWalk ( IMAGE_FILE_MACHINE_I386,
                    hProc,
                      GetCurrentThread (),
                        &stackframe,
                          &ctx,
                            nullptr, nullptr,
                              nullptr, nullptr );

#endif
  } while (ret == TRUE);

  crash_log.Log (L"-----------------------------------------------------------");


  // On second chance it's pretty clear that no exception handler exists,
  //   terminate the software.
  const bool repeated = ( !memcmp (&last_ctx, ExceptionInfo->ContextRecord,   sizeof CONTEXT)         ) &&
                        ( !memcmp (&last_exc, ExceptionInfo->ExceptionRecord, sizeof EXCEPTION_RECORD) );
  const bool non_continue = ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;

  if ( (repeated || non_continue) && (! scaleform) && desc.length () )
  {
    if (! config.system.handle_crashes)
      TerminateProcess (GetCurrentProcess (), 0xdeadbeef);

    SK_AutoClose_Log (crash_log);

    last_chance = true;

    WIN32_FIND_DATA fd     = {                  };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    size_t          files  =   0UL;
    LARGE_INTEGER   liSize = { 0ULL };

    wchar_t wszFindPattern [MAX_PATH * 2] = { };

    lstrcatW (wszFindPattern, SK_GetConfigPath ());
    lstrcatW (wszFindPattern, L"logs\\*.log");

    hFind =
      FindFirstFileW (wszFindPattern, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      wchar_t wszBaseDir [MAX_PATH * 2] = { };
      wchar_t wszOutDir  [MAX_PATH * 2] = { };
      wchar_t wszTime    [MAX_PATH    ] = { };

      lstrcatW (wszBaseDir, SK_GetConfigPath ( ));
      lstrcatW (wszBaseDir, L"logs\\");

      wcscpy (wszOutDir, wszBaseDir);
      lstrcatW (wszOutDir, L"crash\\");

             time_t now = { };
      struct tm*    now_tm;

                    time (&now);
      now_tm = localtime (&now);

      const wchar_t* wszTimestamp = L"%m-%d-%Y__%H'%M'%S\\";

      wcsftime (wszTime, MAX_PATH, wszTimestamp, now_tm);
      lstrcatW (wszOutDir, wszTime);

      wchar_t wszOrigPath [MAX_PATH * 2 + 1] = { };
      wchar_t wszDestPath [MAX_PATH * 2 + 1] = { };

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
            if (dll_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              dll_log.close ();
            }

            //if (crash_log.name.find (fd.cFileName) != std::wstring::npos)
            //{
            //  crash_log.close ();
            //}

            if (steam_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              steam_log.close ();
            }

            if (game_debug.name.find (fd.cFileName) != std::wstring::npos)
            {
              game_debug.close ();
            }

            if (budget_log.name.find (fd.cFileName) != std::wstring::npos)
            {
              budget_log.close ();
            }

            // There's a small chance that we may crash prior to loading CEGUI's DLLs, in which case
            //   trying to grab a static reference to the Logger Singleton would blow stuff up.
            //
            //   Avoid this by counting the number of frames actually drawn.
            if (StrStrW (fd.cFileName, L"CEGUI.log") && SK_GetFramesDrawn () > 120)
            {
              CopyFileExW ( L"CEGUI.log", wszDestPath,
                              nullptr, nullptr, nullptr,
                                0x00 );
              CEGUI::Logger::getDllSingleton ().
                setLogFilename (SK_WideCharToUTF8 (wszDestPath).c_str (), true);
            }

            else if (CopyFileExW (wszOrigPath, wszDestPath, nullptr, nullptr,nullptr, 0x00))
            {
              ++files;

              if (dll_log.name.find (fd.cFileName) != std::wstring::npos)
              {
                dll_log.init  (wszDestPath, L"a");
              }

              if (steam_log.name.find (fd.cFileName) != std::wstring::npos)
              {
                steam_log.init  (wszDestPath, L"a");
              }

              if (game_debug.name.find (fd.cFileName) != std::wstring::npos)
              {
                game_debug.init  (wszDestPath, L"a");
              }

              if (budget_log.name.find (fd.cFileName) != std::wstring::npos)
              {
                budget_log.init  (wszDestPath, L"a");
              }
            }

            DeleteFileW (wszOrigPath);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    if (crash_log.fLog && (! crash_log.silent))
      PlaySound ( reinterpret_cast <LPCWSTR> (crash_sound.buf),
                    nullptr,
                      SND_SYNC |
                      SND_MEMORY );


    // Shutdown the module gracefully
    SK_SelfDestruct ();

    return EXCEPTION_EXECUTE_HANDLER;
  }

  last_ctx = *ExceptionInfo->ContextRecord;
  last_exc = *ExceptionInfo->ExceptionRecord;


  if ( ExceptionInfo->ExceptionRecord->ExceptionFlags == 0 ||
       (! desc.length ())
     )
  {
    return EXCEPTION_CONTINUE_EXECUTION;
  }

  else
  {
    return EXCEPTION_EXECUTE_HANDLER;
  }
}


extern CRITICAL_SECTION cs_dbghelp;

ULONG
SK_GetSymbolNameFromModuleAddr ( HMODULE hMod,   uintptr_t addr,
                                 char*   pszOut, ULONG     ulLen )
{
  ULONG ret = 0;

  //if (config.system.strict_compliance)
    EnterCriticalSection (&cs_dbghelp);

  HANDLE hProc =
    GetCurrentProcess ();

  DWORD64                           ip;

  UIntPtrToInt64 (addr, (int64_t *)&ip);

  DWORD64 BaseAddr =
    SymGetModuleBase64 ( hProc, ip );

  char szModName [MAX_PATH + 2] = {  };

  int len =
    GetModuleFileNameA  ( hMod,
                            szModName,
                              MAX_PATH );

  char* pszShortName = szModName + len- 1;

  while (  pszShortName      >  szModName &&
         *(pszShortName - 1) != '\\')
    --pszShortName;

  SymLoadModule64 ( hProc,
                      nullptr,
                        pszShortName,
                          nullptr,
                            BaseAddr,
                              0 );

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

  //if (config.system.strict_compliance)
    LeaveCriticalSection (&cs_dbghelp);

  return ret;
}

void
WINAPI
SK_SymRefreshModuleList ( HANDLE hProc = GetCurrentProcess () )
{
  //if (config.system.strict_compliance)
    EnterCriticalSection (&cs_dbghelp);

  SymRefreshModuleList (hProc);

  //if (config.system.strict_compliance)
    LeaveCriticalSection (&cs_dbghelp);
}

typedef void (__cdecl *SteamAPI_SetBreakpadAppID_pfn)( uint32_t unAppID );
typedef void (__cdecl *SteamAPI_UseBreakpadCrashHandler_pfn)( char const *pchVersion, char const *pchDate, 
                                                              char const *pchTime,    bool        bFullMemoryDumps,
                                                              void       *pvContext,  LPVOID      m_pfnPreMinidumpCallback );

SteamAPI_SetBreakpadAppID_pfn        SteamAPI_SetBreakpadAppID_NEVER        = nullptr;
SteamAPI_UseBreakpadCrashHandler_pfn SteamAPI_UseBrakepadCrashHandler_NEVER = nullptr;

void
__cdecl
SteamAPI_SetBreakpadAppID_Detour ( uint32_t unAppId )
{
  UNREFERENCED_PARAMETER (unAppId);

  return;
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

  return;
}

#include <diagnostics/compatibility.h>

void
SK_BypassSteamCrashHandler (void)
{
  if (! config.steam.silent)
  {
#ifdef _WIN64
    const wchar_t* wszSteamDLL = L"steam_api64.dll";
#else
    const wchar_t* wszSteamDLL = L"steam_api.dll";
#endif

    if (SK_GetFileSize (wszSteamDLL) > 0)
    {
      HMODULE hMod = LoadLibraryW_Original (wszSteamDLL);

      if (hMod)
      {
        crash_log.Log (L"Disabling Steam Breakpad...");

        SK_CreateDLLHook2 (  wszSteamDLL,
                             "SteamAPI_UseBreakpadCrashHandler",
                              SteamAPI_UseBreakpadCrashHandler_Detour,
reinterpret_cast <LPVOID *> (&SteamAPI_UseBrakepadCrashHandler_NEVER) );
      
        SK_CreateDLLHook2 (  wszSteamDLL,
                             "SteamAPI_SetBreakpadAppID",
                              SteamAPI_SetBreakpadAppID_Detour,
reinterpret_cast <LPVOID *> (&SteamAPI_SetBreakpadAppID_NEVER) );

        SK_ApplyQueuedHooks ();
      }
    }
  }
}



void
CrashHandler::InitSyms (void)
{
  //if (config.system.strict_compliance)
    EnterCriticalSection (&cs_dbghelp);

  static volatile ULONG init = 0UL;

  if (! InterlockedExchange (&init, 1))
  {
    if (config.system.handle_crashes)
    {
      HRSRC   default_sound =
        FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_CRASH), L"WAVE");

      if (default_sound != nullptr)
      {
        crash_sound.ref   =
          LoadResource (SK_GetDLL (), default_sound);

        if (crash_sound.ref != 0)
        {
          crash_sound.buf =
            reinterpret_cast <uint8_t *> (LockResource (crash_sound.ref));
        }
      }

      if (! config.steam.silent)
        SK_BypassSteamCrashHandler ();
    }

    SymSetOptions ( SYMOPT_CASE_INSENSITIVE | SYMOPT_LOAD_LINES    | SYMOPT_UNDNAME |
                    SYMOPT_NO_PROMPTS       | SYMOPT_DEFERRED_LOADS );

    SymInitialize (
      GetCurrentProcess (),
        NULL,
          TRUE );
  }

  //if (config.system.strict_compliance)
    LeaveCriticalSection (&cs_dbghelp);
}
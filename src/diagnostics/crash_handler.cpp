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
#include "crash_handler.h"

#include "../config.h"
#include "../core.h"
#include "../log.h"
#include "../resource.h"

#include <Windows.h>

#define _NO_CVCONST_H
#include <dbghelp.h>

#pragma comment( lib, "dbghelp.lib" )

extern HMODULE hModSelf;

using namespace SK::Diagnostics;

LONG
WINAPI
SK_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo );

typedef LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *SetUnhandledExceptionFilter_pfn)(
    _In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
);

SetUnhandledExceptionFilter_pfn SetUnhandledExceptionFilter_Original = nullptr;

LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter_Detour (_In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
  return SetUnhandledExceptionFilter_Original (SK_TopLevelExceptionFilter);
}

void
CrashHandler::Reinstall (void)
{
  SetErrorMode (SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);
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
    FindResource (hModSelf, MAKEINTRESOURCE (IDR_CRASH), L"WAVE");

  if (default_sound != nullptr) {
    crash_sound.ref   =
      LoadResource (hModSelf, default_sound);

    if (crash_sound.ref != 0)
      crash_sound.buf = (uint8_t *)LockResource (crash_sound.ref);
  }

  if (! crash_log.initialized)
    crash_log.init ("logs/crash.log", "w");

  //SK_CreateDLLHook ( L"kernel32.dll", "SetUnhandledExceptionFilter",
                     //SetUnhandledExceptionFilter_Detour,
          //(LPVOID *)&SetUnhandledExceptionFilter_Original );

  SymInitialize (
    GetCurrentProcess (),
      NULL,
        TRUE );

  SymRefreshModuleList (GetCurrentProcess ());

  Reinstall ();
}

void
CrashHandler::Shutdown (void)
{
  crash_log.close ();
}



sk_logger_t crash_log;

LONG
WINAPI
SK_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo )
{
  static bool             last_chance = false;

  static CONTEXT          last_ctx = { 0 };
  static EXCEPTION_RECORD last_exc = { 0 };

  if (last_chance)
    return 0;

  // On second chance it's pretty clear that no exception handler exists,
  //   terminate the software.
  if (! memcmp (&last_ctx, ExceptionInfo->ContextRecord, sizeof CONTEXT)) {
    if (! memcmp (&last_exc, ExceptionInfo->ExceptionRecord, sizeof EXCEPTION_RECORD)) {
      extern HMODULE       hModSelf;
      extern BOOL APIENTRY DllMain (HMODULE hModule,
                                    DWORD   ul_reason_for_call,
                                    LPVOID  /* lpReserved */);

      sk_logger_t::AutoClose close_me =
        crash_log.auto_close ();

      last_chance = true;

      // Shutdown the module gracefully
      DllMain (hModSelf, DLL_PROCESS_DETACH, nullptr);

      PlaySound ( (LPCWSTR)crash_sound.buf,
                    nullptr,
                      SND_SYNC | SND_MEMORY );
    }
  }

  last_ctx = *ExceptionInfo->ContextRecord;
  last_exc = *ExceptionInfo->ExceptionRecord;

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

  HMODULE hModSource;
  char    szModName [MAX_PATH] = { '\0' };
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

  char* szDupName    = strdup (szModName);
  char* pszShortName = szDupName + lstrlenA (szDupName);

  while (  pszShortName      >  szDupName &&
    *(pszShortName - 1) != '\\')
    --pszShortName;

  crash_log.Log (L"-----------------------------------------------------------");
  crash_log.Log (L"[! Except !] %s", desc.c_str ());
  crash_log.Log (L"-----------------------------------------------------------");
  crash_log.Log (L"[ FaultMod ]  # File.....: '%hs'",  szModName);
#ifndef _WIN64
  crash_log.Log (L"[ FaultMod ]  * EIP Addr.: %hs+%ph", pszShortName, ip-BaseAddr);

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
  crash_log.Log (L"[ FaultMod ]  * RIP Addr.: %hs+%ph", pszShortName, ip-BaseAddr);

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
                       (DWORD64)ip,
                         &Displacement,
                           &sip.si ) ) {
    crash_log.Log (
      L"-----------------------------------------------------------");

    DWORD Disp;
#ifdef _WIN64
    IMAGEHLP_LINE64 ihl64;
    ihl64.SizeOfStruct = sizeof IMAGEHLP_LINE64;

    BOOL  bFileAndLine =
      SymGetLineFromAddr64 ( hProc, ip, &Disp, &ihl64 );

    if (bFileAndLine) {
      crash_log.Log ( L"[-(Source)-] [!] %hs  <%hs:%lu>",
                      sip.si.Name,
                        ihl64.FileName,
                          ihl64.LineNumber );
#else
    IMAGEHLP_LINE ihl;
    ihl.SizeOfStruct = sizeof IMAGEHLP_LINE;

    BOOL  bFileAndLine =
      SymGetLineFromAddr ( hProc, ip, &Disp, &ihl );

    if (bFileAndLine) {
      crash_log.Log ( L"[-(Source)-] [!] %hs  <%hs:%lu>",
                      sip.si.Name,
                        ihl.FileName,
                          ihl.LineNumber );
#endif
    } else {
      crash_log.Log ( L"[--(Name)--] [!] %hs",
                      sip.si.Name );
    }
  }
  crash_log.Log (L"-----------------------------------------------------------");

#ifdef _WIN64
  SymUnloadModule64 (hProc, BaseAddr);
#else
  SymUnloadModule   (hProc, BaseAddr);
#endif

  free (szDupName);

  if (ExceptionInfo->ExceptionRecord->ExceptionFlags != EXCEPTION_NONCONTINUABLE)
    return EXCEPTION_CONTINUE_EXECUTION;
  else {
    return UnhandledExceptionFilter (ExceptionInfo);
  }
}

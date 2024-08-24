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
#include <SpecialK/resource.h>

#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>

#define _L2(w)  L ## w
#define  _L(w) _L2(w)

#ifdef _M_AMD64
# define SK_DBGHELP_STUB(__proto) __proto##64
#else
# define SK_DBGHELP_STUB(__proto) __proto
#endif
#define SK_DBGHELP_STUB_(__proto) __proto

#define SK_StackWalk          SK_DBGHELP_STUB  (StackWalk)
#define SK_SymLoadModule      SK_DBGHELP_STUB  (SymLoadModule)
#define SK_SymUnloadModule    SK_DBGHELP_STUB  (SymUnloadModule)
#define SK_SymGetModuleBase   SK_DBGHELP_STUB  (SymGetModuleBase)
#define SK_SymGetLineFromAddr SK_DBGHELP_STUB  (SymGetLineFromAddr)
#define SK_SymGetTypeInfo     SK_DBGHELP_STUB_ (SymGetTypeInfo)

extern const wchar_t*
__stdcall
SK_GetDebugSymbolPath (void);

std::string
SK_GetSymbolNameFromModuleAddr (HMODULE hMod, uintptr_t addr);

void
SK_SymSetOpts_Once (void)
{
  DWORD dwSymFlags =
    SYMOPT_LOAD_LINES           | SYMOPT_NO_PROMPTS        |
    SYMOPT_UNDNAME              | SYMOPT_DEFERRED_LOADS    |
    SYMOPT_OMAP_FIND_NEAREST    | SYMOPT_FAVOR_COMPRESSED  |
    SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_UNQUALIFIED_LOADS |
    SYMOPT_LOAD_ANYTHING;

  SymSetSearchPathW ( GetCurrentProcess (), SK_GetDebugSymbolPath () );
  SymSetOptions     ( dwSymFlags );

  if (! config.system.handle_crashes)
    SymSetExtendedOption (SYMOPT_EX_NEVERLOADSYMBOLS, TRUE);

  //
  // Avoid deferred loads, they have the potential to deadlock if other threads are
  //   calling LoadLibrary (...) at the same time as symbols in a deferred module
  //     are used for the first time.
  //
}
void
SK_SymSetOpts (void)
{
  if (cs_dbghelp != nullptr)
  {
    SK_RunOnce (SK_SymSetOpts_Once ());
  }

  else SK_LOG0 ( ( L"SK_SymSetOpts (...) called before initializing "
                   L"critical section!" ),
                   L"CrashHandle" );
}


// Set to true during abnormal program termination;
//   used primarily for prognostics in the global injector.
static volatile LONG __SK_Crashed = 0;

bool SK_Debug_IsCrashing (void)
{
  bool ret = true;

  __try {
    ret =
      ReadAcquire (&__SK_Crashed) != 0;
  }

  __except (EXCEPTION_EXECUTE_HANDLER) {
  }

  return ret;
}


struct sk_crash_sound_s {
  HGLOBAL               ref        = nullptr;
  uint8_t*              buf        = nullptr;
  SK_ISimpleAudioVolume volume_ctl = nullptr;

  bool play (void);
};

SK_LazyGlobal <sk_crash_sound_s> crash_sound;


bool
SK_Crash_PlaySound (void)
{
  if (SK_GetFramesDrawn () == 0)
    return true;

  bool ret = false;

  // Rare WinMM (SDL/DOSBox) crashes may prevent this from working, so...
  //   don't create another top-level exception.
  __try {
    extern bool SK_ImGui_WantExit;          // Mute crash sound while exiting
    if (0 == ReadAcquire (&__SK_DLL_Ending) && (! SK_ImGui_WantExit))
    {
      if ((SK_GetAsyncKeyState (VK_MENU)  & 0x8000) == 0x0 &&
          (SK_GetAsyncKeyState (VK_LMENU) & 0x8000) == 0x0 &&
          (SK_GetAsyncKeyState (VK_RMENU) & 0x8000) == 0x0 &&
          (SK_GetAsyncKeyState (VK_F4)    & 0x8000) == 0x0)
      {
        SK_PlaySound ( reinterpret_cast <LPCWSTR> (crash_sound->buf),
                         nullptr,
                           SND_SYNC |
                           SND_MEMORY );
      }
    }

    ret = true;
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }

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
      volume_ctl = nullptr;
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

  //SetUnhandledExceptionFilter_Original (lpTopLevelExceptionFilter);

  return
    SetUnhandledExceptionFilter_Original (SK_TopLevelExceptionFilter);
}


void
CrashHandler::Reinstall (void)
{
  if (! config.system.handle_crashes)
    return;

  static volatile LPVOID   pOldHook   = nullptr;
  if ((uintptr_t)InterlockedCompareExchangePointer (&pOldHook, (PVOID)1, nullptr) > (uintptr_t)1)
  {
    if (MH_OK == SK_RemoveHook (ReadPointerAcquire (&pOldHook)))
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


  SetErrorMode (SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT);


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

  if (FALSE == InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    SK_Thread_CreateEx (
      [](LPVOID) ->
        DWORD
        {
          InitSyms ();

          SetThreadPriority (SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST);

          HRSRC   default_sound =
            FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_CRASH), L"WAVE");

          if (default_sound != nullptr)
          {
            crash_sound->ref   =
              LoadResource (SK_GetDLL (), default_sound);

            if (crash_sound->ref != nullptr)
            {
              crash_sound->buf =
                static_cast <uint8_t *> (LockResource (crash_sound->ref));
            }
          }

          if (! crash_log->initialized)
          {
            crash_log->flush_freq = 0;
            crash_log->lockless   = true;
            crash_log->init       (L"logs/crash.log", L"wt+,ccs=UTF-8");
          }

          Reinstall ();

          InterlockedIncrement (&init);

          SK_Thread_CloseSelf ();

          return 0;
        },
      L"[SK] Crash Handler Init"
    );
  }
}

void
CrashHandler::Shutdown (void)
{
  SymCleanup (GetCurrentProcess ());

  // Strip the blank line and cause empty-file deletion to happen
  if (crash_log->lines <= 4)
      crash_log->lines  = 0;

  crash_log->close ();
}



std::string
SK_GetSymbolNameFromModuleAddr (HMODULE hMod, uintptr_t addr)
{
  std::string ret;

  HANDLE hProc =
    SK_GetCurrentProcess ();

  MODULEINFO mod_info = { };

  GetModuleInformation (
    hProc,  hMod,
            &mod_info,
     sizeof (mod_info)
  );

#ifdef _M_AMD64
  auto BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else /* _M_IX86 */
  auto BaseAddr   = (DWORD)  mod_info.lpBaseOfDll;
#endif

  char szModName [MAX_PATH + 2] = { };

  GetModuleFileNameA   ( hMod, szModName, MAX_PATH );

  char* szDupName    = _strdup (szModName);
  char* pszShortName = szDupName;

  PathStripPathA (pszShortName);

  if ( dbghelp_callers.find (hMod) ==
       dbghelp_callers.cend (    ) && cs_dbghelp != nullptr )
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    if ( dbghelp_callers.find (hMod) ==
         dbghelp_callers.cend (    )  )
    {
      SK_SymLoadModule ( hProc,
                           nullptr,
                             pszShortName,
                               nullptr,
                                 BaseAddr,
                                   mod_info.SizeOfImage );

      dbghelp_callers.insert (hMod);
    }
  }

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


enum BasicType  // Stolen from CVCONST.H in the DIA 2.0 SDK
{
  btNoType = 0,
  btVoid = 1,
  btChar = 2,
  btWChar = 3,
  btInt = 6,
  btUInt = 7,
  btFloat = 8,
  btBCD = 9,
  btBool = 10,
  btLong = 13,
  btULong = 14,
  btCurrency = 25,
  btDate = 26,
  btVariant = 27,
  btComplex = 28,
  btBit = 29,
  btBSTR = 30,
  btHresult = 31,
};

enum SymbolType
{
	stParameter = 0x1,
	stLocal = 0x2,
	stGlobal = 0x4
};

enum SymTagEnum // Stolen from DIA SDK
{
  SymTagNull,
  SymTagExe,
  SymTagCompiland,
  SymTagCompilandDetails,
  SymTagCompilandEnv,
  SymTagFunction,
  SymTagBlock,
  SymTagData,
  SymTagAnnotation,
  SymTagLabel,
  SymTagPublicSymbol,
  SymTagUDT,
  SymTagEnum,
  SymTagFunctionType,
  SymTagPointerType,
  SymTagArrayType,
  SymTagBaseType,
  SymTagTypedef,
  SymTagBaseClass,
  SymTagFriend,
  SymTagFunctionArgType,
  SymTagFuncDebugStart,
  SymTagFuncDebugEnd,
  SymTagUsingNamespace,
  SymTagVTableShape,
  SymTagVTable,
  SymTagCustom,
  SymTagThunk,
  SymTagCustomType,
  SymTagManagedType,
  SymTagDimension
};

BasicType
GetBasicType (DWORD typeIndex, DWORD64 modBase)
{
  HANDLE    hProc     = GetCurrentProcess ();
  BasicType basicType = { };

  if ( SK_SymGetTypeInfo ( hProc, modBase, typeIndex,
                             TI_GET_BASETYPE, &basicType ) )
  {
    return basicType;
  }

  DWORD typeId = { };

  if ( SK_SymGetTypeInfo (hProc, modBase, typeIndex, TI_GET_TYPEID, &typeId) )
  {
    if ( SK_SymGetTypeInfo ( hProc, modBase, typeId, TI_GET_BASETYPE,
                               &basicType ) )
    {
      return basicType;
    }
  }

  return btNoType;
}

std::wstring
SK_SEH_SummarizeException (_In_ struct _EXCEPTION_POINTERS* ExceptionInfo, bool crash_handled)
{
  if (ExceptionInfo == nullptr)
    return L"(nullptr)";

  std::wstring
    log_entry;
    log_entry.reserve (16384);

  HMODULE hModSource               = nullptr;
  char    szModName [MAX_PATH + 2] = { };
  HANDLE  hProc                    = SK_GetCurrentProcess ();

#ifdef _M_AMD64
  DWORD64  ip = ExceptionInfo->ContextRecord->Rip;
#else /* _M_IX86 */
  DWORD    ip = ExceptionInfo->ContextRecord->Eip;
#endif

  if (GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            (LPCWSTR)ip,
                              &hModSource )) {
    GetModuleFileNameA (hModSource, szModName, MAX_PATH);
  }

#ifdef _M_AMD64
  DWORD64 BaseAddr =
#else /* _M_IX86 */
  DWORD   BaseAddr =
#endif
    SK_SymGetModuleBase ( hProc, ip );

  char       szDupName [MAX_PATH + 2] = { };
  strncpy_s (szDupName, MAX_PATH, szModName, _TRUNCATE);
  char* pszShortName =
             szDupName;

  PathStripPathA (pszShortName);

#define log_entry_format log_entry.append (SK_FormatStringW

  if (crash_handled)
  {
#ifdef _M_IX86
    if (config.compatibility.auto_large_address_patch)
    {
      if (! SK_PE32_IsLargeAddressAware ())
      {     SK_PE32_MakeLargeAddressAwareCopy ();

        // Turn this off to prevent a deathloop of unsuccessful patches in games with launchers :)
        config.compatibility.auto_large_address_patch = false;

        SK_SaveConfig  ();
        SK_MessageBox  ( L"Applied 32-bit LAA (Large Address Aware) Patch in Response to Crash",
                           L"[Special K Patch]", MB_ICONEXCLAMATION | MB_APPLMODAL | MB_OK );
        SK_RestartGame ();
      }
    }
#endif
    const wchar_t* desc = L"";

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

    if (hModSource)
      SK_FreeLibrary (hModSource);
  }

  wchar_t* wszThreadDescription = nullptr;

  if (SUCCEEDED (GetCurrentThreadDescription (&wszThreadDescription)))
  {
    if ( wszThreadDescription     != nullptr &&
         wszThreadDescription [0] != L'\0' )
    {
      log_entry_format ( L"[  Thread  ]  ~ Name.....: \"%s\"\n",
                           wszThreadDescription));
    }
  }

  LocalFree (wszThreadDescription);

  log_entry_format (L"[ FaultMod ]  # File.....: '%hs'\n",  szModName));
#ifdef _M_IX86
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
#else /* _M_AMD64 */
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


#ifdef _M_AMD64
  STACKFRAME64 stackframe = { };
               stackframe.AddrStack.Offset = ctx.Rsp;
               stackframe.AddrFrame.Offset = ctx.Rbp;
#else /* _M_IX86 */
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

    char     short_mod_name [64]           = { }; size_t mod_len  = 0;
    char     symbol_name    [512]          = { }; size_t sym_len  = 0;
    char     file_name      [MAX_PATH + 2] = { }; size_t file_len = 0;
    unsigned line_number                   =  0 ;
  };

  std::vector <stack_entry_s> stack_entries;
  stack_entries.reserve (16);

  do
  {
    ip = stackframe.AddrPC.Offset;

    if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
              reinterpret_cast <LPCWSTR> (ip),
                                 &hModSource ) )
    {
      GetModuleFileNameA (hModSource, szModName, MAX_PATH);
    }

    MODULEINFO mod_info = { };

    if (! GetModuleInformation (
            GetCurrentProcess (), hModSource, &mod_info, sizeof (mod_info)
       )                       )
    {
      stack_entries.emplace_back (
        stack_entry_s (
          "Unknown Module", "Unknown Function"
        )
      );
    }

    else
    {
#ifdef _M_AMD64
      BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else /* _M_IX86 */
      BaseAddr = (DWORD)  mod_info.lpBaseOfDll;
#endif

      strncpy_s     (szDupName, MAX_PATH, szModName, _TRUNCATE);
      pszShortName = szDupName;

      PathStripPathA (pszShortName);

      if ( dbghelp_callers.find (hModSource) ==
           dbghelp_callers.cend (          ) && cs_dbghelp != nullptr )
      {
        std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

        if ( dbghelp_callers.find (hModSource) ==
             dbghelp_callers.cend (          )  )
        {
          SK_SymLoadModule ( hProc,
                               nullptr,
                                pszShortName,
                                  nullptr,
                                    BaseAddr,
                                      mod_info.SizeOfImage );

          dbghelp_callers.insert (hModSource);
        }
      }

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

#ifdef _M_AMD64
        IMAGEHLP_LINE64 ihl              = {                    };
                        ihl.SizeOfStruct = sizeof IMAGEHLP_LINE64;
#else /* _M_IX86 */
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

      if (hModSource)
        SK_FreeLibrary (hModSource);
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

#ifdef _WIN64
# include <../depends/src/SKinHook/hde/hde64.h>
#else
# include <../depends/src/SKinHook/hde/hde32.h>
#endif

LPVOID
SKX_GetNextInstruction (LPVOID addr)
{
#ifdef _WIN64
  hde64s               disasm = { };
  hde64_disasm (addr, &disasm);
#else
  hde32s               disasm = { };
  hde32_disasm (addr, &disasm);
#endif

  if (    ( disasm.flags & (F_ERROR | F_ERROR_LENGTH) ) ||
       0 == disasm.len
     )      disasm.len = 1;

  return
    (LPVOID)((uintptr_t)addr + disasm.len);
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
      // Notify anything that was waiting for injection into this game,
      //   we didn't quite live that long :)
      SK_RunOnce (SK_Inject_BroadcastExitNotify ());

      SK_SelfDestruct     (   );
      SK_TerminateProcess (0x0);
      SK_ExitProcess      (0x0);
    }
  }

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  ////CONTEXT          last_ctx = pTLS != nullptr ? pTLS->debug.last_ctx : CONTEXT          { };
  ////EXCEPTION_RECORD last_exc = pTLS != nullptr ? pTLS->debug.last_exc : EXCEPTION_RECORD { };


  //if ( (ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) &&
  //     SK_IsDebuggerPresent () )
  //{
  //  __debugbreak ();
  //}

  static bool          run_once = false;
  if (! std::exchange (run_once,  true))
  {
    std::wstring hw_status =
      L"==================================================================================================\n";

    MEMORYSTATUSEX
      msex          = {           };// Mmmm, sex.
      msex.dwLength = sizeof (msex);

    GlobalMemoryStatusEx (&msex);

    PROCESS_MEMORY_COUNTERS_EX
      pmc_ex    = {             };
      pmc_ex.cb = sizeof (pmc_ex);

#if   0
    if (SK_WMI_CPUStats->num_cpus > 1)
    {
      log_entry.append (
        SK_FormatStringW (
          L"<+> Aggregate CPU Load: %lu%%", ReadAcquire (&SK_WMI_CPUStats->cpus [SK_WMI_CPUStats->num_cpus].
          )
        )
      );

      double dTemp = (double)
        SK_WMI_CPUStats->cpus [SK_WMI_CPUStats->num_cpus].temp_c;s

      if (dTemp > 0.0)
      {
        extern std::string
        SK_FormatTemperature (double in_temp, SK_UNITS in_unit, SK_UNITS out_unit, SK_TLS* pTLS);

        log_entry.append (
          std::wstring (L", Temperature: ") +
            SK_UTF8ToWideChar (
              SK_FormatTemperature (
                dTemp,
                  Celsius,
                    config.system.prefer_fahrenheit ? Fahrenheit :
                                                      Celsius, pTLS )
            )
        );
      }

      log_entry.append (L"\n");
    }
#endif

    if ( GetProcessMemoryInfo (
           GetCurrentProcess ( ), (PROCESS_MEMORY_COUNTERS *)&pmc_ex,
                                                              pmc_ex.cb ) )
    {
      hw_status.append (
        SK_FormatStringW (
          L" <+> System Memory Load....: %lu%%\n", msex.dwMemoryLoad
        )
      );

      hw_status.append (
        SK_FormatStringW (
          L"  # Current Working Set....: %6.3f GiB  (Max Used: %6.3f GiB out of Total Physical: %6.3f GiB)\n",
            (float)((double)pmc_ex.WorkingSetSize     / (1024.0 * 1024.0 * 1024.0)),
            (float)((double)pmc_ex.PeakWorkingSetSize / (1024.0 * 1024.0 * 1024.0)),
            (float)((double)msex.ullTotalPhys         / (1024.0 * 1024.0 * 1024.0))
        )
      );
      hw_status.append (
        SK_FormatStringW (
          L"  # Current Virtual Memory.: %6.3f GiB  (Max Used: %6.3f GiB)\n\n",
            (float)((double)pmc_ex.PrivateUsage      / (1024.0 * 1024.0 * 1024.0)),
            (float)((double)pmc_ex.PeakPagefileUsage  / (1024.0 * 1024.0 * 1024.0))
        )
      );

      if (SK_GPU_GetVRAMUsed (0) > 0)
      {
        if (SK_GPU_GetGPULoad (0) > 0.0f)
        {
          hw_status.append (
            SK_FormatStringW (
              L" <+> GPU0 Load.............: %5.2f%%", SK_GPU_GetGPULoad (0)
            )
          );
          if (SK_GPU_GetTempInC (0) > 0.0f)
          {
            hw_status.append (
              SK_FormatStringW (
                L" at %5.2f°C", SK_GPU_GetTempInC (0)
              )
            );
          }
          hw_status.append (L"\n");
        }
        if (config.textures.d3d11.cache && SK_D3D11_Textures->Entries_2D.load () > 0)
        {
          hw_status.append (
            SK_FormatStringW (
              L"  - D3D11 Textures Cached..: %lu (%6.3f GiB Used, Quota: %6.3f GiB); %lu Evictions / %lu Hits\n",
                        SK_D3D11_Textures->Entries_2D.load        (),
        (float)((double)SK_D3D11_Textures->AggregateSize_2D.load  () / (1024.0 * 1024.0 * 1024.0)),
        (float)((double)config.textures.cache.max_size               / (                  1024.0)),
                        SK_D3D11_Textures->Evicted_2D.load        (),
                        SK_D3D11_Textures->RedundantLoads_2D.load ()
            )
          );
        }
        hw_status.append (
          SK_FormatStringW (
            L"  - Current VRAM In Use....: %6.3f GiB / Max Useable VRAM: %6.3f GiB\n",
            (float)((double)SK_GPU_GetVRAMUsed     (0) / (1024.0 * 1024.0 * 1024.0)),
            (float)((double)SK_GPU_GetVRAMCapacity (0) / (1024.0 * 1024.0 * 1024.0 * 1024.0))
          )
        );
      }
    }

    hw_status.append (
      L"=================================================================================================="
    );

    crash_log->LogEx (false, L"\n%ws\n\n", hw_status.c_str ());
  }

#ifdef    _M_AMD64
#  define _IP       Rip
#  define _IP_TEXT "RIP"
#else
#  define _IP       Eip
#  define _IP_TEXT "EIP"
#endif

  bool first_time_for_tid =
    ( pTLS == nullptr ||
      pTLS->debug.last_ctx._IP == 0 );

  CONTEXT          dummy_ctx = { };
  EXCEPTION_RECORD dummy_exc = { };
  LONG             dummy_seq =  0 ;

  auto& last_ctx        = pTLS != nullptr ? pTLS->debug.last_ctx          : dummy_ctx;
  auto& last_exc        = pTLS != nullptr ? pTLS->debug.last_exc          : dummy_exc;
  auto& repeat_sequence = pTLS != nullptr ? pTLS->debug.exception_repeats : dummy_seq;

  bool  repeated        = false;

  if (first_time_for_tid)
  {
    last_ctx        = *ExceptionInfo->ContextRecord;
    last_exc        = *ExceptionInfo->ExceptionRecord;
    repeat_sequence = 0;
  }

  else
  {
    repeated =
      ( last_exc.ExceptionAddress/*_IP*/ == ExceptionInfo->ExceptionRecord->ExceptionAddress/*ContextRecord->_IP*/ );
  }

  // On second chance it's pretty clear that no exception handler exists,
  //   terminate the software.
//const bool repeated = ( 0 == memcmp (&last_ctx, ExceptionInfo->ContextRecord,   sizeof CONTEXT)         ) &&
//                      ( 0 == memcmp (&last_exc, ExceptionInfo->ExceptionRecord, sizeof EXCEPTION_RECORD) );
  const bool non_continue =                      (ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) ==
                                                                                                   EXCEPTION_NONCONTINUABLE;

  repeat_sequence =
    ( repeated ? ++repeat_sequence
               :   0 );

  last_ctx = *ExceptionInfo->ContextRecord;
  last_exc = *ExceptionInfo->ExceptionRecord;

  if (config.system.suppress_crashes && repeat_sequence < 64 && pTLS != nullptr)
  {
    LPVOID pExceptionAddr =
      *reinterpret_cast <LPVOID *> (&ExceptionInfo->ContextRecord->_IP);

    bool new_exception =
        pTLS->debug.suppressed_addrs.emplace (pExceptionAddr).second;

    if (pTLS->debug.suppressed_addrs.size () > 3)
      SK_RunOnce (MessageBeep (MB_ICONEXCLAMATION));

    ExceptionInfo->ExceptionRecord->
      ExceptionFlags &= ~EXCEPTION_NONCONTINUABLE;

    if (new_exception)
    {
      auto base_addr =
        static_cast <uintptr_t> (
          SK_SymGetModuleBase (GetCurrentProcess (), (DWORD_PTR)pExceptionAddr)
        );

      std::wstring callsite =
        SK_FormatStringW (L"Unhandled Exception ("
                     _L(_IP_TEXT)  L": %ws+%08xh) Ignored; "//  < Best Guess Symbol Name: \"%hs\" >"
                          "Safety Not Guaranteed (!!)",
                          SK_GetModuleName (
                            SK_GetModuleFromAddr (pExceptionAddr) ).c_str (),
                    reinterpret_cast <uintptr_t> (pExceptionAddr) - base_addr//,
//SK_GetSymbolNameFromModuleAddr (SK_GetModuleFromAddr (pExceptionAddr),
                        //reinterpret_cast <uintptr_t> (pExceptionAddr) - base_addr).c_str ()
        );

      crash_log->Log   (        L"   Unhandled Top-Level Exception (%x):\n",
                                   ExceptionInfo->ExceptionRecord->ExceptionCode );
      crash_log->LogEx ( false, L"%ws",
        SK_SEH_SummarizeException (ExceptionInfo, true).c_str ()
                       );

      fflush (crash_log->fLog);

      SK_ImGui_Warning (callsite.c_str ());
    }

    struct thread_rewrite_s
    {
    public:
      thread_rewrite_s (PCONTEXT pContext)
      {
        context = *pContext;

        auto* _InstPtr =
          reinterpret_cast <uintptr_t*> (
            &pContext->_IP
          );

        using  NtContinue_pfn = NTSTATUS (NTAPI*)(PCONTEXT, BOOLEAN);
        static NtContinue_pfn
               NtContinue = (NtContinue_pfn)SK_GetProcAddress (L"NtDll", "NtContinue");

        if (NT_SUCCESS (NtContinue (&context, FALSE)))
                        *pContext =  context;
        else
        {
          *_InstPtr =
            reinterpret_cast <std::remove_pointer <decltype       ( _InstPtr)>::type>
              ( SKX_GetNextInstruction (reinterpret_cast <LPVOID> (*_InstPtr)) );
        }

#if 1
        if ( DuplicateHandle ( GetCurrentProcess (), GetCurrentThread (),
                               GetCurrentProcess (), &hThread, THREAD_SUSPEND_RESUME |
                                                               THREAD_SET_CONTEXT,
                                                                 FALSE, 0x0
                             )
           )
        {
          WritePointerRelease (
            (volatile PVOID *)&_this, this
          );                          this->context =
                                          *pContext;

          static std::atomic_intmax_t
                   __ignored_exceptions;

          HANDLE
            hContextManipulator =
              SK_Thread_CreateEx
              ([](LPVOID user) -> DWORD
                {
                  auto *pRewrite =
                    static_cast <thread_rewrite_s *> (user);

                  if (pRewrite->hThread != 0)
                  {
                    CONTEXT origContext = {};

                    SuspendThread    (pRewrite->hThread);

                    if (GetThreadContext (pRewrite->hThread, &origContext))
                        SetThreadContext (pRewrite->hThread, &pRewrite->context);

                    delete
                      (thread_rewrite_s *)ReadPointerAcquire ((volatile PVOID *)&pRewrite->_this);

                    ResumeThread   (pRewrite->hThread);
                    SK_CloseHandle (pRewrite->hThread);

                    ++__ignored_exceptions;
                  }

                  SK_Thread_CloseSelf ();

                  return 0;
                }, SK_FormatStringW (
                      L"[SK] Crash Suppressor ([%02lu]::tid=%x)",
                        __ignored_exceptions.load (),
                            SK_GetCurrentThreadId ()
                                    )   .   c_str (),
                this
              );

         // Suspend ourself and expect to magically jump somewhere, as if nothing happend ;)

          //  This Code Never Finishes
          // --------------------------
          //
          //   Context is re-written while this thread is suspended in order to move execution out
          //     of here and back into the original call site that raised the unhandled exception.
          //
          SK_WaitForSingleObject (hContextManipulator, 1500);

          // Failed?!  Wake up in 33 ms and briefly leak memory ( ...leak ends with a crash! :P )

          DWORD                                        dwExitCode = DWORD_MAX;
          if (GetExitCodeThread (hContextManipulator, &dwExitCode))
          {
            // Is thread still active for some reason?
            if (dwExitCode == STILL_ACTIVE)
              WritePointerRelease ((volatile LPVOID *)&_this, nullptr); // Do not delete this
          }

          //  ==>  Sayonara, time to die.  ( Memory Leak Solved In Firey Explosion, hurrah! )
          *pContext = context;
        }
#else
        context = *pContext;

        if (pExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_NONCONTINUABLE_EXCEPTION)
        {
          using  NtContinue_pfn = NTSTATUS (NTAPI*)(PCONTEXT, BOOLEAN);
          static NtContinue_pfn
                 NtContinue = (NtContinue_pfn)SK_GetProcAddress (L"NtDll", "NtContinue");

          if (NT_SUCCESS (NtContinue (&context, FALSE)))
                          *pContext =  context;
          else
          {
            auto* _InstPtr =
              reinterpret_cast <uintptr_t*> (
                &pExceptionInfo->ContextRecord->_IP
              );

            *_InstPtr =
              reinterpret_cast <std::remove_pointer <decltype       ( _InstPtr)>::type>
                ( SKX_GetNextInstruction (reinterpret_cast <LPVOID> (*_InstPtr)) );
          }
        }
#endif
      }

    private :
      CONTEXT  context;
      HANDLE   hThread = INVALID_HANDLE_VALUE;
      volatile  thread_rewrite_s*
              _this    = nullptr;
      DWORD    dwTid   = SK_Thread_GetCurrentId ();
    };

    ////// Leaks memory if rewrite's spawned crash suppression thread fails
    auto rewrite =
      new thread_rewrite_s (
        ExceptionInfo->ContextRecord
      );

    UNREFERENCED_PARAMETER (rewrite);

    // ... if execution reaches here, it is well and truly Game Over! :(
    return
      EXCEPTION_CONTINUE_EXECUTION;
  }


  crash_log->Log   (        L"   Unhandled Top-Level Exception (%x):\n",
                               ExceptionInfo->ExceptionRecord->ExceptionCode );
  crash_log->LogEx ( false, L"%ws",
    SK_SEH_SummarizeException (ExceptionInfo, true).c_str ()
                   );

  fflush (crash_log->fLog);


  if ( (non_continue && repeat_sequence > 1) ||
                        repeat_sequence > 2 ) /*&&
      (ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT)*/ //)
  {
    // Stop injection on crash
    if (SK_GetFramesDrawn () > 1)
    { 
      SK_Inject_BroadcastExitNotify ();
    }

    if ( ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION ||
         ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION )
    {
      if (
        SK_GetModuleHandleW (
          SK_RunLHIfBitness (64, L"RTSSHooks64.dll",
                                 L"RTSSHooks.dll") )
         )
      {
        SK_RunOnce (
          SK_MessageBox (
            L"The game has crashed, and the suspected cause is RivaTuner Statistics Server.\r\n\r\n"
            L"\tPlease ensure that RTSS is configured to use 'Detours Compatible' mode.",
              L"Illegal / Privileged Instruction", MB_OK | MB_ICONWARNING
          )
        );
      }
    }

    if (! config.system.handle_crashes)
    {
      SK_TerminateProcess (0xdeadbeef);
    }

    if (pTLS != nullptr)
        pTLS->debug.last_chance = true;

    wchar_t   wszFindPattern [MAX_PATH + 2] = { };
    lstrcatW (wszFindPattern, SK_GetConfigPath ());
    lstrcatW (wszFindPattern,    LR"(logs\*.log)");

    WIN32_FIND_DATA fd    = { };
    HANDLE          hFind =
      FindFirstFileW (wszFindPattern, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      wchar_t wszBaseDir [MAX_PATH + 2] = { };
      wchar_t wszOutDir  [MAX_PATH + 2] = { };
      wchar_t wszTime    [MAX_PATH + 2] = { };

      lstrcatW (wszBaseDir, SK_GetConfigPath ( ));
      lstrcatW (wszBaseDir, LR"(logs\)");

      wcscpy   (wszOutDir, wszBaseDir);
      lstrcatW (wszOutDir, LR"(crash\)");

             time_t now    = { };
      struct tm     now_tm = { };

                      time (&now);
      localtime_s (&now_tm, &now);

      static const wchar_t* wszTimestamp =
        L"%Y-%m-%d__%H'%M'%S\\";

      wcsftime (wszTime, MAX_PATH, wszTimestamp, &now_tm);
      lstrcatW (wszOutDir, wszTime);

      wchar_t wszOrigPath [MAX_PATH + 2] = { };
      wchar_t wszDestPath [MAX_PATH + 2] = { };

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

          SK_CreateDirectoriesEx (wszDestPath, false);

          if (! StrStrIW (wszOrigPath, L"installer.log"))
          {
            iSK_Logger*       log_file = nullptr;

            std::array <iSK_Logger *, 7> logs {
              dll_log.getPtr (), steam_log.getPtr (),
                                  epic_log.getPtr (), crash_log.getPtr (),
                                                     game_debug.getPtr (),
                                   tex_log.getPtr (),
                                budget_log.getPtr ()
            };

            for ( auto log : logs )
            {
              if (log->name.find (fd.cFileName) != std::wstring::npos)
              {
                log_file = log;
                break;
              }
            }

            ++files;

            bool locked = false;

            if ( log_file       != nullptr &&
                 log_file->fLog != nullptr &&
                 log_file       != crash_log.getPtr () )
            {
              if (0 == InterlockedCompareExchange (&log_file->relocated, 1, 0))
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
                 log_file != crash_log.getPtr () )
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

            if ( log_file       != nullptr &&
                (log_file->fLog == nullptr ||
                                   locked  ||
                 log_file       == crash_log.getPtr ()) )
            {
              if (locked)
              {
                locked = false;
                log_file->unlock ();
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

    crash_log->lines++;

    if (! config.system.silent_crash )
    {
      if (! crash_sound->play ())
      {
        // If we cannot play the sound, then give a message box so we don't
        //   appear to have vanished without a trace...
        SK_MessageBox ( L"Application Crashed (Unable to Play Sound)!",
                          L"Special K Crash Handler [Abnormal Termination]",
                            MB_OK | MB_ICONERROR );
      }
    }

    InterlockedExchange (&__SK_Crashed, 1);

    crash_log->unlock ();

    // Shutdown the module gracefully
    SK_SelfDestruct     (   );
    SK_TerminateProcess (0x3);

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
SK_GetSymbolNameFromModuleAddr (      HMODULE     hMod,   uintptr_t addr,
                                 gsl::span <char> pszOut, ULONG     ulLen )
{
  ULONG ret = 0;

  if ( dbghelp_callers.find (hMod) ==
       dbghelp_callers.cend (    )  )
  {
    MODULEINFO mod_info = { };

    if (! GetModuleInformation (
            GetCurrentProcess (), hMod, &mod_info, sizeof (mod_info) )
       ) return 0;

    const auto BaseAddr =
      (DWORD64)mod_info.lpBaseOfDll;

    char szModName [MAX_PATH + 2] = {  };

    if (!  GetModuleFileNameA ( hMod,
                                  szModName,
                                    MAX_PATH )
       ) return 0;

    char* pszShortName = szModName;

    PathStripPathA (pszShortName);

    if ( dbghelp_callers.find (hMod) ==
         dbghelp_callers.cend (    )  )
    {
      if (! SymLoadModule64 ( GetCurrentProcess (),
                                nullptr,
                                  pszShortName,
                                    nullptr,
                                      BaseAddr,
                                        mod_info.SizeOfImage )
         ) return 0;

      dbghelp_callers.insert (hMod);
    }
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
    pszOut [0] = '\0';

    strncat             ( pszOut.data (), sip.si.Name,
                            std::min (ulLen, sip.si.NameLen) );
    ret =
      sk::narrow_cast <ULONG> (strlen (pszOut.data ()));
  }

  else
  {
    pszOut [0] = '\0';
    ret        = 0;
  }

  return ret;
}

void
WINAPI
SK_SymRefreshModuleList ( HANDLE hProc )
{
  SymRefreshModuleList (hProc);
}

using SteamAPI_SetBreakpadAppID_pfn        = void (__cdecl *)( uint32_t unAppID );
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
  if (! config.platform.silent)
  {
    const wchar_t *wszSteamDLL =
      SK_Steam_GetDLLPath ();

    if (PathFileExistsW (wszSteamDLL) && SK::SteamAPI::AppID () > 0)
    {
      if (SK_Modules->LoadLibraryLL (wszSteamDLL))
      {
        crash_log->Log (L"Disabling Steam Breakpad...");

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
    if (config.system.handle_crashes)
    {
      SymCleanup    (SK_GetCurrentProcess ());
      SymInitialize (
        SK_GetCurrentProcess (),
          nullptr,
            FALSE );

      SymRefreshModuleList (SK_GetCurrentProcess ());
    }

    Init ();

    //if (config.system.handle_crashes)
    //{
    //  if (! config.platform.silent)
    //    SK_BypassSteamCrashHandler ();
    //}
  }
}
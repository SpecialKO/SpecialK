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
#include <SpecialK/plugin/plugin_mgr.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Watch_Dogs"

#include <sysinfoapi.h>

namespace SK {

  // RAII wrapper for SK_SuspendAllOtherThreads
  class SuspendHelper {
  public:
    inline explicit SuspendHelper() noexcept : suspension_ctx_(SK_SuspendAllOtherThreads()) {};
    SuspendHelper(const SuspendHelper&) = delete;
    SuspendHelper(SuspendHelper&&) = delete;
    SuspendHelper& operator=(const SuspendHelper&) = delete;
    SuspendHelper& operator=(SuspendHelper&&) = delete;

    inline ~SuspendHelper() noexcept { SK_ResumeThreads(suspension_ctx_); };

  private:
    SK_ThreadSuspension_Ctx suspension_ctx_;
  };

  namespace WatchDogs {
    // Restore NtProtectVirtualMemory from NTDLL on disk.
    static bool UnhookVirtualProtect(void)
    {
      const auto hNtdll = SK_GetModuleHandleW(L"NtDll.dll");
      if (!hNtdll) return false;
      const auto pfnNtProtectVirtualMemory = SK_GetProcAddress(hNtdll, "NtProtectVirtualMemory");
      if (!pfnNtProtectVirtualMemory) return false;

      const auto va_base = std::bit_cast<uintptr_t>(hNtdll);
      const auto va_func = std::bit_cast<uintptr_t>(pfnNtProtectVirtualMemory);
      const ptrdiff_t offset = va_func - va_base;
      assert(offset > 0);

      const uint8_t syscall_prologue[] = {
        0x4C, 0x8B, 0xD1, // mov r10, rcx
        0xB8,             // mov eax, {imm32}
      };

      if (std::memcmp(pfnNtProtectVirtualMemory, &syscall_prologue[0], sizeof(syscall_prologue)) == 0) {
        // Prologue unmodified, no need to unhook.
        return true;
      }

      SK_LOGi0(L"Detect VMProtect hooked NtProtectVirtualMemory, unhooking...");

      const auto ntdll_path = std::filesystem::path{ SK_GetSystemDirectory() } / L"ntdll.dll";

      std::ifstream ntdll(ntdll_path, std::ios::binary);
      ntdll.seekg(offset + sizeof(syscall_prologue));
      char buf[4] = {};
      ntdll.read(&buf[0], sizeof(buf));

      const auto dwNtProtectVirtualMemoryNR = std::bit_cast<DWORD>(buf);

      // VMProtect's inline hook looks like this:
      // 1. rel32 jump (E9 xx xx xx xx) on function entry
      // 2. allocate an executable page just after ntdllto fit address in 32-bit offset range
      // 3. the usual `jmp qword [rip]` trampoline (FF 25 00 00 00 00 + {absolute addr}) in the newly allocated page
      // So we at least need to restore the first 5 bytes of function entry.
      // For sanity, also restore the whole syscall NR.

      SuspendHelper suspend{};

      DWORD dwOldProtect;
      if (!::VirtualProtect(
        pfnNtProtectVirtualMemory,
        sizeof(syscall_prologue) + sizeof(dwNtProtectVirtualMemoryNR),
        PAGE_EXECUTE_READWRITE,
        &dwOldProtect
      )) return false;

      std::memcpy(pfnNtProtectVirtualMemory, &syscall_prologue[0], sizeof(syscall_prologue));
      std::memcpy(
        std::bit_cast<void*>(
          std::bit_cast<uintptr_t>(pfnNtProtectVirtualMemory) + sizeof(syscall_prologue)
        ),
        &dwNtProtectVirtualMemoryNR,
        sizeof(dwNtProtectVirtualMemoryNR)
      );

      DWORD dwDontCare;
      ::VirtualProtect(pfnNtProtectVirtualMemory, sizeof(syscall_prologue) + sizeof(dwNtProtectVirtualMemoryNR), dwOldProtect, &dwDontCare);

      SK_LOGi0(L"Successfully unhooked NtProtectVirtualMemory.");

      return true;
    }

    enum class GameVersion {
      Ubisoft,
      Steam,
      Asia,
      CompleteEdition,
      Unknown = -1,
    };

    static GameVersion CheckGameVersion(HMODULE hModule) {
      static GameVersion version = GameVersion::Unknown;
      if (version != GameVersion::Unknown) return version;

      static const auto pe_base = std::bit_cast<uintptr_t>(hModule);
      // HACK: Check .text+0xC, since this is VMP-mutated so we could use this as a hash.
      // TODO: Use IMAGE_FILE_HEADER.TimeDateStamp?
      auto lpVersionCheck = std::bit_cast<void*>(pe_base + 0x100C);
      uint32_t func_bytes;
      std::memcpy(&func_bytes, lpVersionCheck, sizeof(func_bytes));
      switch (func_bytes) {
      case 0x08238fe8:
        SK_LOGi0(L"Found Game Version: Ubisoft");
        version = GameVersion::Ubisoft;
        break;
      case 0x0e335fe8:
        SK_LOGi0(L"Found Game Version: Steam");
        version = GameVersion::Steam;
        break;
      case 0xd98b4802:
        SK_LOGi0(L"Found Game Version: Asia");
        version = GameVersion::Asia;
        break;
      case 0xcccccccc:
        SK_LOGi0(L"Found Game Version: Complete Edition");
        version = GameVersion::CompleteEdition;
        break;
      case 0x0:
        SK_LOGi0(L"Engine DLL is not unpacked yet. This is a bug and should be reported.");
      default:
        SK_LOGi0(L"Unknown Game Version (%08x)", func_bytes);
        return GameVersion::Unknown;
      }

      return version;
    }

    namespace Game {
      // About class member functions:
      // MSVC does not support the non-standard member function pointer to function pointer cast. (-Wpmf-conversion)
      // Emulate this with a static member function.

      class Vec2f {
      public:
        inline constexpr Vec2f(float x, float y) : x_(x * -1.f), y_(y) {}
        inline constexpr float x() const { return x_; }
        inline constexpr float y() const { return y_; }
        inline void SetX(float x) { x_ = -1.f * x; } // inverted
        inline void SetY(float y) { y_ = y; }

      private:
        float x_;
        float y_;
      };
      static_assert(sizeof(Vec2f) == 0x8, "struct size mismatch");

      class MouseMgr {
        char RESERVED0[0x120];
      public:
        Vec2f mouse_sens_;

      private:
        MouseMgr() = delete;
        MouseMgr(const MouseMgr&) = delete;
        MouseMgr(MouseMgr&&) = delete;
        MouseMgr& operator=(const MouseMgr&) = delete;
        MouseMgr& operator=(MouseMgr&&) = delete;
        ~MouseMgr() = delete;
      };
      static_assert(offsetof(MouseMgr, mouse_sens_) == 0x120, "struct offset mismatch");

      class MouseSmoothingResult;

      class MouseInput {
      public:
        uint32_t type_id_;
        Vec2f mouse_delta_;

        // FIXME: function prototype
        // FIXME: should this be here?
        static MouseSmoothingResult* ProcessMouseSmoothing(MouseInput* thiz, uintptr_t a2);

      private:
        friend class CameraMgr;
        // cam_look_py_mouse
        static inline constexpr uint32_t kCamLookPyMouse = 0x7936d51d;

        MouseInput() = delete;
        MouseInput(const MouseInput&) = delete;
        MouseInput(MouseInput&&) = delete;
        MouseInput& operator=(const MouseInput&) = delete;
        MouseInput& operator=(MouseInput&&) = delete;
        ~MouseInput() = delete;
      };
      static_assert(offsetof(MouseInput, mouse_delta_) == 0x4, "struct offset mismatch");

      class InputMgr {
        char RESERVED0[0x90];
      public:
        MouseInput mouse_input_;

      private:
        InputMgr() = delete;
        InputMgr(const InputMgr&) = delete;
        InputMgr(InputMgr&&) = delete;
        InputMgr& operator=(const InputMgr&) = delete;
        InputMgr& operator=(InputMgr&&) = delete;
        ~InputMgr() = delete;
      };
      static_assert(offsetof(InputMgr, mouse_input_) + offsetof(MouseInput, mouse_delta_) == 0x94, "struct offset mismatch");


      class MouseInputResult;

      class CameraMgr {
        char RESERVED0[0x8];
      public:
        InputMgr inputmgr_;

        static MouseInputResult* ProcessMouseInput(CameraMgr* thiz, MouseInput& mouse_input);

      private:
        CameraMgr() = delete;
        CameraMgr(const CameraMgr&) = delete;
        CameraMgr(CameraMgr&&) = delete;
        CameraMgr& operator=(const CameraMgr&) = delete;
        CameraMgr& operator=(CameraMgr&&) = delete;
        ~CameraMgr() = delete;
      };
      static_assert(offsetof(CameraMgr, inputmgr_) == 0x8, "struct offset mismatch");
    }

    using pfnProcessMouseInput = decltype(&Game::CameraMgr::ProcessMouseInput);
    using pfnProcessMouseSmoothing = decltype(&Game::MouseInput::ProcessMouseSmoothing);
    using lpMouseMgr = Game::MouseMgr*;

    static struct Offsets {
      pfnProcessMouseInput ProcessMouseInput;
      pfnProcessMouseSmoothing ProcessMouseSmoothing;
      lpMouseMgr g_MouseMgr;
    } g_Offsets;

    static pfnProcessMouseInput ProcessMouseInput_Orig = nullptr;
    static pfnProcessMouseSmoothing ProcessMouseSmoothing_Orig = nullptr;
    static lpMouseMgr const g_MouseMgr = nullptr;

    static bool DisableMouseAccel = false;

    Game::MouseSmoothingResult* Game::MouseInput::ProcessMouseSmoothing(Game::MouseInput* thiz, uintptr_t a2) {
      SK_LOG_FIRST_CALL;

      if (DisableMouseAccel)
        return nullptr;

      return ProcessMouseSmoothing_Orig(thiz, a2);
    }

    Game::MouseInputResult* Game::CameraMgr::ProcessMouseInput(Game::CameraMgr* thiz, Game::MouseInput& mouse_input) {
      SK_LOG_FIRST_CALL;

      using namespace Game;

      assert(ProcessMouseInput_Orig);
      assert(g_MouseMgr);

      auto result = ProcessMouseInput_Orig(thiz, mouse_input);

      if (DisableMouseAccel)
        if (mouse_input.type_id_ == MouseInput::kCamLookPyMouse) {
          auto sens = g_MouseMgr->mouse_sens_;
          sens.SetX((sens.x() / 100.f) * (5.f - .1f) + .1f);
          sens.SetY((sens.y() / 100.f) * (5.f - .1f) + .1f);
          auto& delta = thiz->inputmgr_.mouse_input_.mouse_delta_;
          delta.SetX(mouse_input.mouse_delta_.x());
          delta.SetY(mouse_input.mouse_delta_.y());
        }

      return result;
    }
  }

  class PtrHelper {
  public:
    explicit PtrHelper(HMODULE hMod) : base_(std::bit_cast<uintptr_t>(hMod)) {};
    template <typename Ty>
    inline Ty FromOffset(ptrdiff_t offset) const
      requires std::is_pointer_v<Ty>
    {
      return std::bit_cast<Ty>(base_ + offset);
    }

  private:
    uintptr_t base_;
  };

  template <typename Fn>
  inline MH_STATUS CreateFuncHook(const wchar_t* func_name, Fn* target, Fn& detour, Fn*& original)
    requires std::is_function_v<Fn>
  {
    return SK_CreateFuncHook(func_name, target, detour, reinterpret_cast<void**>(&original));
  }
  template <typename Fn>
  inline MH_STATUS QueueEnableHook(Fn* target)
    requires std::is_function_v<Fn>
  {
    return MH_QueueEnableHook(target);
  }
}

static bool SK_WatchDogs_PlugInCfg(void) {
  auto dll_ini = SK_GetDLLConfig();

  bool changed = false;

  if (ImGui::CollapsingHeader("Watch_Dogs", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Checkbox("Disable Mouse Acceleration", &SK::WatchDogs::DisableMouseAccel)) {
      changed = true;
    }
  }

  if (changed)
    dll_ini->write();

  return true;
}

void SK_WatchDogs_InitPlugin(void) {
  SK_Thread_CreateEx([] (LPVOID) -> DWORD {
    using namespace SK::WatchDogs;

    auto hMod = SK_GetModuleHandle(L"Disrupt_b64.dll");
    if (!hMod) {
      SK_LOGi0(L"Game engine DLL not found??!");
      return 1;
    }
    auto version = CheckGameVersion(hMod);

    auto h = SK::PtrHelper(hMod);
    switch (version) {
    case GameVersion::Ubisoft:
      g_Offsets = {
        .ProcessMouseInput = h.FromOffset<pfnProcessMouseInput>(0x179BB60),
        .ProcessMouseSmoothing = h.FromOffset<pfnProcessMouseSmoothing>(0x15EC6C0),
        .g_MouseMgr = h.FromOffset<lpMouseMgr>(0x3B8BF20),
      };
      break;
    case GameVersion::Steam:
      g_Offsets = {
        .ProcessMouseInput = h.FromOffset<pfnProcessMouseInput>(0x18EBAF0),
        .ProcessMouseSmoothing = h.FromOffset<pfnProcessMouseSmoothing>(0x173CDC0),
        .g_MouseMgr = h.FromOffset<lpMouseMgr>(0x3B72F30),
      };
      break;
    case GameVersion::Asia:
      g_Offsets = {
        .ProcessMouseInput = h.FromOffset<pfnProcessMouseInput>(0x108B630),
        .ProcessMouseSmoothing = h.FromOffset<pfnProcessMouseSmoothing>(0x1049A60),
        .g_MouseMgr = h.FromOffset<lpMouseMgr>(0x3A3F2E8),
      };
      break;
    case GameVersion::CompleteEdition:
      g_Offsets = {
        .ProcessMouseInput = h.FromOffset<pfnProcessMouseInput>(0xD36670),
        .ProcessMouseSmoothing = h.FromOffset<pfnProcessMouseSmoothing>(0xB80EB0),
        .g_MouseMgr = h.FromOffset<lpMouseMgr>(0x3BC94C0),
      };
      break;
    default:
      return 1;
    }

    if (!UnhookVirtualProtect()) return 1;

#define CREATE_HOOK(func, target, original) \
    if (SK::CreateFuncHook(L"" #func, (target), (func), (original)) != MH_OK) {                   \
      SK_LOGi0(L"Hooking " #func " failed");                                                      \
      return 1;                                                                                   \
    }

    CREATE_HOOK(Game::CameraMgr::ProcessMouseInput, g_Offsets.ProcessMouseInput, ProcessMouseInput_Orig);
    CREATE_HOOK(Game::MouseInput::ProcessMouseSmoothing, g_Offsets.ProcessMouseSmoothing, ProcessMouseSmoothing_Orig);

#undef CREATE_HOOK

    SK_LOGi0(L"All functions ready for hook.");

    SK::QueueEnableHook(g_Offsets.ProcessMouseInput);
    SK::QueueEnableHook(g_Offsets.ProcessMouseSmoothing);

    SK_Thread_CloseSelf();

    return 0;
  }, L"[SK] Watch_Dogs Delayed Init");

  static sk::ParameterBool* SK_DisableMouseAccel = dynamic_cast<sk::ParameterBool*>(
      g_ParameterFactory->create_parameter<bool>(L"Disable Mouse Acceleration")
      );

  if (SK_DisableMouseAccel != nullptr) {
    auto dll_ini = SK_GetDLLConfig();

    SK_DisableMouseAccel->register_to_ini(dll_ini, L"Watch_Dogs", L"DisableMouseAccel");
    SK_DisableMouseAccel->load(SK::WatchDogs::DisableMouseAccel);
  }

  plugin_mgr->config_fns.emplace (SK_WatchDogs_PlugInCfg);
}

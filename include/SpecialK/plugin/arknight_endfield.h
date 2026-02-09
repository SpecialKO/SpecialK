#ifndef __SK__Plugin__Arknight_Endfield__
#define __SK__Plugin__Arknight_Endfield__

/*
  ---------------- SHARED MEMORY STRUCTURE FOR AKEF INJECTION ----------------
*/
namespace AKEF_Shared
{
  struct SharedMemory
  {
    volatile LONG has_pid         = 0; // 0/1 flag
    DWORD         akef_launch_pid = 0;
    DWORD         padding[6]      = { };
  };

  SharedMemory* GetView (void);

  void PublishPid           (DWORD pid);
  void ResetPid             (void);
  bool TryGetPid            (DWORD* out_pid);
  void CleanupSharedMemory  (void);
}

void AKEF_Shared_publishPid           (DWORD pid);
bool AKEF_Shared_tryGetPid            (DWORD* out_pid);
void AKEF_Shared_resetPid             (void);
void AKEF_Shared_cleanupSharedMemory  (void);
/* --------------------------------------------------------------------------- */

struct AKEF_WindowInfo
{
  HWND         hwnd = nullptr;
  std::wstring title;
  std::wstring class_name;
};

struct AKEF_WindowData
{
  DWORD                        pid = 0;
  std::vector<AKEF_WindowInfo> windows;
};

void SK_AKEF_ApplyUnityTargetFrameRate (float targetFrameRate);
#endif /* __SK__Plugin__Arknight_Endfield__ */
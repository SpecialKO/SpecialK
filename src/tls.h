#pragma once

#include <Windows.h>

struct SK_TLS {
  struct {
    bool texinject_thread = false;
  } d3d11;
};

extern volatile DWORD __SK_TLS_INDEX;

SK_TLS* __stdcall SK_GetTLS (void);
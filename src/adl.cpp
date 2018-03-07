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

#include <SpecialK/adl.h>
#include <SpecialK/log.h>

#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>

BOOL      ADL_init = ADL_FALSE;
HINSTANCE hADL_DLL;

ADL_ADAPTER_NUMBEROFADAPTERS_GET   ADL_Adapter_NumberOfAdapters_Get;
ADL_ADAPTER_ADAPTERINFO_GET        ADL_Adapter_AdapterInfo_Get;
ADL_ADAPTER_ACTIVE_GET             ADL_Adapter_Active_Get;

ADL_MAIN_CONTROL_CREATE            ADL_Main_Control_Create;
ADL_MAIN_CONTROL_DESTROY           ADL_Main_Control_Destroy;

ADL_OVERDRIVE5_TEMPERATURE_GET     ADL_Overdrive5_Temperature_Get;
ADL_OVERDRIVE5_FANSPEED_GET        ADL_Overdrive5_FanSpeed_Get;

ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;

// Memory allocation function
void* __stdcall ADL_Main_Memory_Alloc ( int iSize )
{
  void* lpBuffer = malloc ( iSize );
  return lpBuffer;
}

// Optional Memory de-allocation function
void __stdcall ADL_Main_Memory_Free ( void** lpBuffer )
{
  if ( NULL != *lpBuffer )
  {
    free ( *lpBuffer );
    *lpBuffer = NULL;
  }
}

AdapterInfo adl_adapters [ADL_MAX_ADAPTERS] = { };
AdapterInfo adl_active   [ADL_MAX_ADAPTERS] = { };

BOOL
SK_InitADL (void)
{
  if (ADL_init != ADL_FALSE) {
    return ADL_init;
  }

  for (int i = 0; i < ADL_MAX_ADAPTERS; i++)
  {
    adl_adapters [i].iSize = sizeof (AdapterInfo);
    adl_active   [i].iSize = sizeof (AdapterInfo);
  }

  hADL_DLL = SK_Modules.LoadLibraryLL (L"atiadlxx.dll");
  if (hADL_DLL == nullptr) {
    // A 32 bit calling application on 64 bit OS will fail to LoadLibrary.
    // Try to load the 32 bit library (atiadlxy.dll) instead
    hADL_DLL = SK_Modules.LoadLibraryLL (L"atiadlxy.dll");
  }

  if (hADL_DLL == nullptr) {
    ADL_init = ADL_FALSE - 1;
    return FALSE;
  }
  else {
    ADL_Main_Control_Create            =
      (ADL_MAIN_CONTROL_CREATE)GetProcAddress (
        hADL_DLL, "ADL_Main_Control_Create"
      );
    ADL_Main_Control_Destroy           =
      (ADL_MAIN_CONTROL_DESTROY)GetProcAddress (
         hADL_DLL, "ADL_Main_Control_Destroy"
       );

    ADL_Adapter_NumberOfAdapters_Get   =
     (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress (
       hADL_DLL, "ADL_Adapter_NumberOfAdapters_Get"
     );

    ADL_Adapter_AdapterInfo_Get        =
     (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress (
       hADL_DLL, "ADL_Adapter_AdapterInfo_Get"
     );

    ADL_Adapter_Active_Get        =
     (ADL_ADAPTER_ACTIVE_GET)GetProcAddress (
       hADL_DLL, "ADL_Adapter_Active_Get"
     );

    ADL_Overdrive5_Temperature_Get     =
      (ADL_OVERDRIVE5_TEMPERATURE_GET)GetProcAddress (
        hADL_DLL, "ADL_Overdrive5_Temperature_Get"
      );

    ADL_Overdrive5_FanSpeed_Get        =
      (ADL_OVERDRIVE5_FANSPEED_GET)GetProcAddress (
        hADL_DLL, "ADL_Overdrive5_FanSpeed_Get"
      );

    ADL_Overdrive5_CurrentActivity_Get =
      (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress (
        hADL_DLL, "ADL_Overdrive5_CurrentActivity_Get"
      );

    if (ADL_Main_Control_Create            != nullptr &&
        ADL_Main_Control_Destroy           != nullptr &&
        ADL_Adapter_NumberOfAdapters_Get   != nullptr &&
        ADL_Adapter_Active_Get             != nullptr &&
        ADL_Adapter_AdapterInfo_Get        != nullptr &&
        ADL_Overdrive5_Temperature_Get     != nullptr &&
        ADL_Overdrive5_FanSpeed_Get        != nullptr &&
        ADL_Overdrive5_CurrentActivity_Get != nullptr) {
      if (ADL_OK == ADL_Main_Control_Create (ADL_Main_Memory_Alloc, 0)) {
        ADL_init = ADL_TRUE;
        return TRUE;
      }
    }
  }

  ADL_init = ADL_FALSE - 1;
  return FALSE;
}

int
SK_ADL_CountPhysicalGPUs (void)
{
  int num_gpus = 0;

  if (ADL_Adapter_NumberOfAdapters_Get (&num_gpus) == ADL_OK) {
    return num_gpus;
  }

  return 0;
}

// Also populates a list of active adapters
int
SK_ADL_CountActiveGPUs (void)
{
  int active_count  = 0;
  int adapter_count = SK_ADL_CountPhysicalGPUs ();

  //dll_log.Log (L"[DisplayLib] Adapter Count: %d", adapter_count);

  ADL_Adapter_AdapterInfo_Get (
    adl_adapters,
      adapter_count * sizeof (AdapterInfo)
  );

  for (int i = 0; i < adapter_count; i++) {
    int adapter_status = ADL_FALSE;

    ADL_Adapter_Active_Get (adl_adapters [i].iAdapterIndex, &adapter_status);

    if (adapter_status == ADL_TRUE) {
      memcpy ( &adl_active     [active_count++],
                 &adl_adapters [i],
                   sizeof AdapterInfo );
    }
  }

  return active_count;
}

AdapterInfo*
SK_ADL_GetActiveAdapter (int idx)
{
  if (idx < ADL_MAX_ADAPTERS)
    return &adl_active [idx];

  return nullptr;
}
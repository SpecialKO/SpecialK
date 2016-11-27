#ifndef __SK__ADL_H__
#define __SK__ADL_H__

#include <adl/adl_structures.h>
#include <adl/adl_sdk.h>

typedef int ( *ADL_MAIN_CONTROL_CREATE )(ADL_MAIN_MALLOC_CALLBACK, int );
typedef int ( *ADL_MAIN_CONTROL_DESTROY )();

typedef int ( *ADL_ADAPTER_NUMBEROFADAPTERS_GET ) ( int* );
typedef int ( *ADL_ADAPTER_ADAPTERINFO_GET ) ( LPAdapterInfo, int );
typedef int ( *ADL_ADAPTER_ACTIVE_GET ) ( int, int* );

typedef int ( *ADL_OVERDRIVE5_TEMPERATURE_GET ) (int iAdapterIndex, int iThermalControllerIndex, ADLTemperature *lpTemperature);
typedef int ( *ADL_OVERDRIVE5_CURRENTACTIVITY_GET ) (int iAdapterIndex, ADLPMActivity *lpActivity);
typedef int ( *ADL_OVERDRIVE5_FANSPEED_GET ) (int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedValue *lpFanSpeedValue);
typedef int ( *ADL_OVERDRIVE5_FANSPEEDINFO_GET ) (int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedInfo *lpFanSpeedInfo);
typedef int ( *ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET ) (int iAdapterIndex, int iDefault, ADLODPerformanceLevels *lpOdPerformanceLevels); 

typedef int ( *ADL_OVERDRIVE6_FANSPEED_GET )(int iAdapterIndex, ADLOD6FanSpeedInfo *lpFanSpeedInfo);

#include <Windows.h>

BOOL SK_InitADL               (void);
int  SK_ADL_CountPhysicalGPUs (void);
int  SK_ADL_CountActiveGPUs   (void);

AdapterInfo*
     SK_ADL_GetActiveAdapter  (int idx);


extern BOOL ADL_init;

extern ADL_ADAPTER_NUMBEROFADAPTERS_GET   ADL_Adapter_NumberOfAdapters_Get;
extern ADL_ADAPTER_ACTIVE_GET             ADL_Adapter_Active_Get;
extern ADL_ADAPTER_ADAPTERINFO_GET        ADL_Adapter_AdapterInfo_Get;

extern ADL_OVERDRIVE5_TEMPERATURE_GET     ADL_Overdrive5_Temperature_Get;
extern ADL_OVERDRIVE5_FANSPEED_GET        ADL_Overdrive5_FanSpeed_Get;

extern ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;

#endif /* _SK__ADL_H__ */
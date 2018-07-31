//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                     Copyright 2007-2009 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------------------
//
// Version Information
//
//-----------------------------------------------------------------------------

#define OLS_MAJOR_VERSION				1
#define OLS_MINOR_VERSION				3
#define OLS_REVISION				    0
#define OLS_RELESE					   18

#define OLS_VERSION	((OLS_MAJOR_VERSION << 24) | (OLS_MINOR_VERSION << 16) |\
		(OLS_REVISION << 8) | OLS_RELESE) 

//-----------------------------------------------------------------------------
//
// Defines
//
//-----------------------------------------------------------------------------

#define OLS_DRIVER_FILE_NAME_WIN_9X			_T("WinRing0.vxd")
#define OLS_DRIVER_FILE_NAME_WIN_NT			_T("WinRing0.sys")
#define OLS_DRIVER_FILE_NAME_WIN_NT_X64		_T("WinRing0x64.sys")
#define OLS_DRIVER_FILE_NAME_WIN_NT_IA64	_T("WinRing0ia64.sys")  // Reserved

//-----------------------------------------------------------------------------
//
// Prototypes
//
//-----------------------------------------------------------------------------

DWORD Initialize();
void Deinitialize();
DWORD InitDriverInfo();

BOOL OpenDriver();
BOOL LoadDriver(TCHAR *DriverFileName, TCHAR *DriverId);
BOOL UnloadDriver(TCHAR *DriverId);
DWORD GetRefCount();

BOOL IsNT();
BOOL IsWow64();
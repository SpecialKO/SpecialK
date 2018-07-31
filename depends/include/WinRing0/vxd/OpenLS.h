//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                     Copyright 2007-2008 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

#include <devioctl.h>

//-----------------------------------------------------------------------------
//
// typedef/define
//
//-----------------------------------------------------------------------------

typedef DWORD NTSTATUS;
typedef BYTE  BOOLEAN;
typedef DIOCPARAMETERS *LPDIOC;

#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)

//-----------------------------------------------------------------------------
//
// Function Prototypes
//
//-----------------------------------------------------------------------------

DWORD _stdcall W32_DeviceIOControl(DWORD, DWORD, DWORD, LPDIOC);

DWORD _stdcall Dynamic_Init(void)
{
    return (VXD_SUCCESS);
}

DWORD _stdcall Dynamic_Exit(void)
{
    return (VXD_SUCCESS);
}

DWORD _stdcall CleanUp(void)
{
    return (VXD_SUCCESS);
}

extern void Exec_VxD_Int_rap(void);
#define CallPciBios() Exec_VxD_Int_rap()

//-----------------------------------------------------------------------------
//
// Function Prototypes for Control Code
//
//-----------------------------------------------------------------------------

NTSTATUS	ReadMsr(
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);

NTSTATUS	WriteMsr(
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);
			
NTSTATUS	ReadPmc(
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);

NTSTATUS	ReadIoPort(
				ULONG ioControlCode,
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);

NTSTATUS	WriteIoPort(
				ULONG ioControlCode,
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);

NTSTATUS	ReadPciConfig(
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);

NTSTATUS	WritePciConfig(
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);

NTSTATUS	ReadMemory(
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);

NTSTATUS	WriteMemory(
				void *lpInBuffer, 
				ULONG nInBufferSize, 
				void *lpOutBuffer, 
				ULONG nOutBufferSize, 
				ULONG *lpBytesReturned
			);


//-----------------------------------------------------------------------------
//
// Support Function Prototypes
//
//-----------------------------------------------------------------------------

NTSTATUS pciConfigRead(ULONG pciAddress, ULONG offset, void *data, int length);
NTSTATUS pciConfigWrite(ULONG pciAddress, ULONG offset, void *data, int length);

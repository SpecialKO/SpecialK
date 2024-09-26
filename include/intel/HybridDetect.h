///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021, Intel Corporation
// Permission is hereby granted, free of charge, to any person obtaining a   copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is    furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of 
// the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A    PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>
#include <array>
#include <vector>
#include <string>
#include <stdio.h>
#include <bitset>
#include <map>
#include <assert.h>
#include <thread>

#ifdef _WIN32
#define HYBRIDDETECT_OS_WIN
#else
#pragma message("Warning: " __FILE__ " not yet tested on non-Windows systems")
#endif

#ifdef HYBRIDDETECT_OS_WIN
#include <Windows.h>
#include <powrprof.h>
#include <VersionHelpers.h>
#include <intrin.h>
#include <malloc.h>

#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wreserved-identifier"
	#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
	#pragma clang diagnostic ignored "-Wsign-conversion"
	#pragma clang diagnostic ignored "-Wsign-compare"
	#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
	#pragma clang diagnostic ignored "-Wunused-parameter"
	#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#pragma comment(lib, "Powrprof.lib")
#else
typedef unsigned long ULONG;
typedef unsigned long long ULONG64;
typedef size_t SIZE_T;
typedef unsigned char BYTE;
typedef void* HANDLE;
#endif

#ifndef HYBRID_DETECT_TRACE_ENABLED_VOLUME
#define HYBRID_DETECT_TRACE_ENABLED_VOLUME 0 // Scale of 1-10, 10 being most verbose
#endif
#if HYBRID_DETECT_TRACE_ENABLED_VOLUME
#define HYBRID_DETECT_TRACE(vol, fmt, ...) if (vol <= HYBRID_DETECT_TRACE_ENABLED_VOLUME){ \
	printf("[%s][vol=%2d]: "##fmt##"\n", __FUNCTION__, vol, ##__VA_ARGS__); \
	fflush(stdout); \
}
#else
#define HYBRID_DETECT_TRACE(vol, fmt, ...)
#endif

namespace HybridDetect
{

#ifdef _WIN32
// Simple wrapper for __cpuid intrinsic, created for cross platform support e.g. WIN32
#define CPUID(registers, function) __cpuid((int*)registers, (int)function);
#define CPUIDEX(registers, function, extFunction) __cpuidex((int*)registers, (int)function, (int)extFunction);
#define XGETBV(xcrReg) _xgetbv(xcrReg)
#else 
// Linux Stuff
#define CPUID(registers, function) asm volatile ("cpuid" : "=a" (registers[0]), "=b" (registers[1]), "=c" (registers[2]), "=d" (registers[3]) : "a" (function), "c" (0));
#define CPUIDEX(registers, function, extFunction) asm volatile ("cpuid" : "=a" (registers[0]), "=b" (registers[1]), "=c" (registers[2]), "=d" (registers[3]) : "a" (function), "c" (extFunction));
#define XGETBV(xcrReg) (0) // TODO
#endif

// Enables/Disables Hybrid Detect
#define ENABLE_HYBRID_DETECT

// Tells the application to treat the target system as a heterogeneous software proxy.
//#define ENABLE_SOFTWARE_PROXY	

// Enables/Disables Run On API
#define ENABLE_RUNON

// Enables/Disables ThreadPriority Based on Core-Type
//#define ENABLE_RUNON_PRIORITY

// Enables/Disables SetThreadInformation Memory Priority Based on Core-Type
//#define ENABLE_RUNON_MEMORY_PRIORITY

// Enables/Disables SetThreadInformation Execution Speed based on Core-Type
//#define ENABLE_RUNON_EXECUTION_SPEED

// Enables CPU-Sets and Disables ThreadAffinityMasks
#define ENABLE_CPU_SETS

// No current CPUs have ISA support that varies between cores
#ifndef ENABLE_PER_LOGICAL_CPUID_ISA_DETECTION
#define ENABLE_PER_LOGICAL_CPUID_ISA_DETECTION 1 // Default to enabled for backward compatibility
#endif

// Simple conversion from an ordinal, n, to a set bit at position n
#define IndexToMask(n)  (1UL << n)

// Macros to store values for CPUID register ordinals
#define CPUID_EAX								0
#define CPUID_EBX								1
#define CPUID_ECX								2
#define CPUID_EDX								3

#define LEAF_CPUID_BASIC						0x00        // Basic CPUID Information
#define LEAF_THERMAL_POWER						0x06        // Thermal and Power Management Leaf
#define LEAF_EXTENDED_FEATURE_FLAGS				0x07        // Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value)
#define LEAF_EXTENDED_STATE						0x0D        // Processor Extended State Enumeration Main Leaf (EAX = 0DH, ECX = 0)
#define LEAF_FREQUENCY_INFORMATION				0x16        // Processor Frequency Information Leaf  function 0x16 only works on Skylake or newer.
#define LEAF_HYBRID_INFORMATION					0x1A        // Hybrid Information Sub - leaf(EAX = 1AH, ECX = 0)
#define LEAF_EXTENDED_INFORMATION_0				0x80000000  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_1				0x80000001  // Extended Function CPUID Information
#define LEAF_EXTENDED_BRAND_STRING_1			0x80000002  // Extended Function CPUID Information
#define LEAF_EXTENDED_BRAND_STRING_2			0x80000003  // Extended Function CPUID Information
#define LEAF_EXTENDED_BRAND_STRING_3			0x80000004  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_5				0x80000005  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_6				0x80000006  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_7				0x80000007  // Extended Function CPUID Information
#define LEAF_EXTENDED_INFORMATION_8				0x80000008  // Extended Function CPUID Information

enum CoreTypes
{
	ANY = -1,
	NONE = 0x00,
	RESERVED0 = 0x10,
	INTEL_ATOM = 0x20,
	RESERVED1 = 0x30,
	INTEL_CORE = 0x40,
};

// Struct to store information for each Cache.
typedef struct _CACHE_INFO
{
	unsigned                            group = 0;
	std::bitset<64>						processorMask = 0;
	unsigned							level = 0;
	unsigned							size = 0;
	unsigned							lineSize = 0;
	unsigned							type = 0; // PROCESSOR_CACHE_TYPE
	unsigned							associativity = 0;
} CACHE_INFO, * PCACHE_INFO;

// Struct to store Power information for a logical processor.
typedef struct _LOGICAL_PROCESSOR_POWER_INFORMATION {
	ULONG								number = 0;
	ULONG								maxMhz = 0;
	ULONG								currentMhz = 0;
	ULONG								mhzLimit = 0;
	ULONG								maxIdleState = 0;
	ULONG								currentIdleState = 0;
} LOGICAL_PROCESSOR_POWER_INFORMATION, * PLOGICAL_PROCESSOR_POWER_INFORMATION;

// Struct to store Power information for a logical processor.
typedef struct _LOGICAL_PROCESSOR_INFO
{
	unsigned							id = 0;
	unsigned							group = 0;
	unsigned							node = 0;
	unsigned							coreIndex = 0;
	unsigned							logicalProcessorIndex = 0;
	std::bitset<64>						processorMask = 0;
	unsigned							baseFrequency = 0;
	unsigned							currentFrequency = 0;
	unsigned							maximumFrequency = 0;
	unsigned							busFrequency = 0;
#if ENABLE_PER_LOGICAL_CPUID_ISA_DETECTION
	unsigned							SSE : 1;
	unsigned							AVX : 1;
	unsigned							AVX2 : 1;
	unsigned							AVX512 : 1;
	unsigned							AVX512F : 1;
	unsigned							AVX512DQ : 1;
	unsigned							AVX512PF : 1;
	unsigned							AVX512ER : 1;
	unsigned							AVX512CD : 1;
	unsigned							AVX512BW : 1;
	unsigned							AVX512VL : 1;
	unsigned							AVX512_IFMA : 1;
	unsigned							AVX512_VBMI : 1;
	unsigned							AVX512_VBMI2 : 1;
	unsigned							AVX512_VNNI : 1;
	unsigned							AVX512_BITALG : 1;
	unsigned							AVX512_VPOPCNTDQ : 1;
	unsigned							AVX512_4VNNIW : 1;
	unsigned							AVX512_4FMAPS : 1;
	unsigned							AVX512_VP2INTERSECT : 1;
	unsigned							SGX : 1;
	unsigned							SHA : 1;
#endif // ENABLE_PER_LOGICAL_CPUID_ISA_DETECTION
	unsigned							parked : 1;
	unsigned							allocated : 1;
	unsigned							allocatedToTargetProcess : 1;
	unsigned							realTime : 1;
	ULONG64								allocationTag = 0;
	unsigned							efficiencyClass = 0;
	unsigned							schedulingClass = 0;
	CoreTypes							coreType = CoreTypes::NONE;
	LOGICAL_PROCESSOR_POWER_INFORMATION powerInformation;
} LOGICAL_PROCESSOR_INFO, * PLOGICAL_PROCESSOR_INFO;

typedef struct _GROUP_INFO
{
	unsigned							activeGroupCount = 0;
	unsigned							maximumGroupCount = 0;
	unsigned							activeProcessorCount = 0;
	unsigned							maximumProcessorCount = 0;
	ULONG64								activeProcessorMask = 0;
}	GROUP_INFO, * PGROUP_INFO;

typedef struct _NUMA_NODE_INFO
{
	unsigned							nodeNumber = 0;
	unsigned							group = 0;
	ULONG64								mask = 0;
}	NUMA_NODE_INFO, * PNUMA_NODE_INFO;

// Feature flags
struct FeatureFlags
{
	// CPUID.1:ECX
	unsigned SSE3 : 1;		// 0, All x64 CPUs have at least SSE3
	unsigned PCLMULQDQ : 1; // 1
	unsigned SSSE3 : 1;		// 9
	unsigned FMA : 1;		// 12, See 14.5.3 - On top of AVX
	unsigned SSE4_1 : 1;	// 19
	unsigned SSE4_2 : 1;	// 20
	unsigned AES : 1;		// 25
	unsigned XSAVE : 1;		// 26, Safe to call xgetbv with ecx=0
	unsigned OSXSAVE : 1;	// 27
	unsigned AVX : 1;		// 28. See 14.3
	unsigned F16C : 1;		// 29
	unsigned RDRAND : 1;	// 30

	//AVX2: CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5]=1
	unsigned AVX2 : 1; // Check OSXSAVE and AVX and OS_Supports_YMM
	//Verify both CPUID.0x7.0:EBX.AVX512F[bit 16] = 1
	unsigned AVX512F : 1;	// 16
	unsigned AVX512DQ : 1;	// 17
	unsigned AVX512_IFMA : 1; // 21
	unsigned AVX512CD : 1;	// 28
	unsigned AVX512BW : 1;	// 30
	unsigned AVX512VL : 1;	// 31


	// Derived
	unsigned OS_Supports_YMM : 1;
	unsigned OS_Supports_ZMM : 1;

	bool AVX_Supported() const
	{
		return AVX && OS_Supports_YMM;
	}
	bool F16C_Supported() const
	{
		return AVX_Supported() && F16C;
	}
	bool AVX2_Supported() const
	{
		return AVX_Supported() && AVX2;
	}
	bool AVX512_State_Supported() const
	{
		return OS_Supports_ZMM;
	}
	// Initial set of AVX512 features supported on SkylakeX
	bool AVX512_SKX_Supported() const
	{
		return OS_Supports_ZMM && AVX512F && AVX512VL && AVX512BW && AVX512DQ && AVX512CD;
	}

};

// Struct to store Processor information
typedef struct _PROCESSOR_INFO
{
	char								vendorID[13];
	char								brandString[64];
	unsigned							numGroups = 0;
	unsigned							numNUMANodes = 0;
	unsigned							numProcessorPackages = 0;
	unsigned							numPhysicalCores = 0;
	unsigned							numLogicalCores = 0;
	unsigned							numL1Caches = 0;
	unsigned							numL2Caches = 0;
	unsigned							numL3Caches = 0;
	bool								hybrid = false;
	bool								turboBoost = false;
	bool								turboBoost3_0 = false;
	std::vector<GROUP_INFO>				groups;
	std::vector<NUMA_NODE_INFO>			nodes;
	std::vector<CACHE_INFO>				caches;
	std::vector<LOGICAL_PROCESSOR_INFO>	cores;

	// Store map of logical processors returned from GLPI. 
	// short = Core Type, ULONG64 = 64-bit processor mask 
	std::map<short, ULONG64>			coreMasks;
#ifdef ENABLE_CPU_SETS

	// Store map of logical processors returned from GetSystemCPUSetInformation. 
	// unsined custom logical cluster key, use short to store EffeciencyClass as a key 
	// std::vector<ULONG> = list of CPU Set IDs 
	std::map<unsigned, std::vector<ULONG>>	cpuSets;

#endif
	// TODO: These are not initialized
	//unsigned							osMajorVersion;
	//unsigned							osMinorVersion;
	//unsigned							osBuildNumber;
	bool IsIntel()    const { return !strcmp("GenuineIntel", vendorID); }
	bool IsAMD()      const { return !strcmp("AuthenticAMD", vendorID); }

	inline int GetCoreTypeCount(CoreTypes coreType)
	{
#ifdef ENABLE_CPU_SETS
		return (int)cpuSets[(UINT)coreType].size();
#else
		std::bitset<64> bits = coreMasks[coreType];

		return (int)bits.count();
#endif
	}

	FeatureFlags flags{};

} PROCESSOR_INFO, * PPROCESSOR_INFO;

// Type to String Conversion Helper Function
inline const char* CoreTypeString(CoreTypes type)
{
	switch (type)
	{
	case CoreTypes::NONE:
		return "None";
	case CoreTypes::RESERVED0:
		return "Reserved_0";
	case CoreTypes::INTEL_ATOM:
		return "E-Core";
	case CoreTypes::RESERVED1:
		return "Reserved_1";
	case CoreTypes::INTEL_CORE:
		return "P-Core";
	default:
		return "Any";
	}
}

#ifdef HYBRIDDETECT_OS_WIN
// Cache to String Conversion Helper Function
inline const char* CacheTypeString(int type)
{
	switch (type)
	{
	case CacheUnified:
		return "Unified";
	case CacheInstruction:
		return "Instruction";
	case CacheData:
		return "Data";
	case CacheTrace:
		return "Trace";
	}
	return "Any";
}
#endif

// Helper function to Call the CPUID intrinsic
inline bool CallCPUID(unsigned function, std::array<unsigned, 4>& registers, unsigned extFunction = 0, unsigned CPUIDFunctionMax = LEAF_EXTENDED_INFORMATION_8)
{
	HYBRID_DETECT_TRACE(10, ">>> (0x%.8x)", function);
	if (function > CPUIDFunctionMax) return false;
	CPUIDEX(registers.data(), function, extFunction)
	HYBRID_DETECT_TRACE(10, "<<<");
	return true;
}

template<typename T>
T* AdvanceBytes(T* p, SIZE_T cb)
{
	return reinterpret_cast<T*>(reinterpret_cast<BYTE*>(p) + cb);
}

#ifdef HYBRIDDETECT_OS_WIN
class EnumLogicalProcessorInformation
{
	// Based On: https://devblogs.microsoft.com/oldnewthing/20131028-00/?p=2823
public:
	EnumLogicalProcessorInformation(LOGICAL_PROCESSOR_RELATIONSHIP Relationship)
		: m_pinfoBase(nullptr), m_pinfoCurrent(nullptr), m_cbRemaining(0)
	{
		DWORD cb = 0;
		if (GetLogicalProcessorInformationEx(Relationship,
			nullptr, &cb)) return;
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;
		m_pinfoBase =
			reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>
			(LocalAlloc(LMEM_FIXED, cb));
		if (!m_pinfoBase) return;
		if (!GetLogicalProcessorInformationEx(Relationship,
			m_pinfoBase, &cb)) return;
		m_pinfoCurrent = m_pinfoBase;
		m_cbRemaining = cb;
	}
	~EnumLogicalProcessorInformation() { LocalFree(m_pinfoBase); }
	void MoveNext()
	{
		if (m_pinfoCurrent) {
			m_cbRemaining -= m_pinfoCurrent->Size;
			if (m_cbRemaining) {
				m_pinfoCurrent = AdvanceBytes(m_pinfoCurrent,
					m_pinfoCurrent->Size);
			}
			else {
				m_pinfoCurrent = nullptr;
			}
		}
	}
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Current()
	{
		return m_pinfoCurrent;
	}
private:
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* m_pinfoBase;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* m_pinfoCurrent;
	DWORD m_cbRemaining;
};

inline void PrintMask(KAFFINITY Mask)
{
#if 0
	printf("0x%.16llx", Mask);
#else
	for (int i = (sizeof(Mask) * 8) - 1; i >= 0; i--) {
		if (Mask & (static_cast<KAFFINITY>(1) << i))
			printf("%d", 1);
		else
			printf("%d", 0);
	}
#endif
}

inline bool IsHybridOSEx()
{
	OSVERSIONINFOEX versionInfo;
	DWORDLONG conditionMask = 0;
	int operation = VER_GREATER_EQUAL;

	ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOEX));

	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	versionInfo.dwMajorVersion = 10;
	versionInfo.dwMinorVersion = 0;
	versionInfo.dwBuildNumber = 22000;
	//versionInfo.wServicePackMajor = 0;
	//versionInfo.wServicePackMinor = 0;

	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, operation);
	VER_SET_CONDITION(conditionMask, VER_MINORVERSION, operation);
	VER_SET_CONDITION(conditionMask, VER_BUILDNUMBER, operation);
	//VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMAJOR, operation);
	//VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMINOR, operation);

	return VerifyVersionInfo(&versionInfo,
		VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, conditionMask);
}

// Calls CPUID & GetLogicalProcessors to fill in PROCESSOR_INFO Caches & Cores
inline bool GetLogicalProcessors(PROCESSOR_INFO& procInfo)
{
	HYBRID_DETECT_TRACE(7, ">>>");

#ifdef ENABLE_HYBRID_DETECT
#ifdef ENABLE_CPU_SETS
	unsigned long bufferSize;
	// Get the Current Process Handle.
	HANDLE curProc = GetCurrentProcess();

	// Get total number (size) of elements in the data structure.
	GetSystemCpuSetInformation(nullptr, 0, &bufferSize, curProc, 0);

	// Allocate data structures based on size returned from first call.
	auto buffer = std::make_unique<uint8_t[]>(bufferSize);

	// Get all of the CPUSet elements 
	if(!GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.get()), bufferSize, &bufferSize, curProc, 0))
	{
		return false;
	}

	uint8_t* cpuSetPtr = buffer.get();

	for (ULONG cpuSetSize = 0; cpuSetSize < bufferSize; )
	{
		auto nextCPUSet = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(cpuSetPtr);

		if (nextCPUSet->Type == CPU_SET_INFORMATION_TYPE::CpuSetInformation)
		{
			// Store Logical Processor Information for Later Use.
			LOGICAL_PROCESSOR_INFO core;
			core.id = nextCPUSet->CpuSet.Id;
			core.group = nextCPUSet->CpuSet.Group;
			core.node = nextCPUSet->CpuSet.NumaNodeIndex;
			core.logicalProcessorIndex = nextCPUSet->CpuSet.LogicalProcessorIndex;
			core.coreIndex = nextCPUSet->CpuSet.CoreIndex;
			core.realTime = nextCPUSet->CpuSet.RealTime;
			core.parked = nextCPUSet->CpuSet.Parked;
			core.allocated = nextCPUSet->CpuSet.Allocated;
			core.allocatedToTargetProcess = nextCPUSet->CpuSet.AllocatedToTargetProcess;
			core.allocationTag = nextCPUSet->CpuSet.AllocationTag;
			core.efficiencyClass = nextCPUSet->CpuSet.EfficiencyClass;
			core.schedulingClass = nextCPUSet->CpuSet.SchedulingClass;
			procInfo.cores.push_back(core);
			procInfo.numLogicalCores++;

			// Bin logical processors based on efficiency class here
			// ...
		}

		cpuSetPtr += nextCPUSet->Size;
		cpuSetSize += nextCPUSet->Size;
		
	}
	HYBRID_DETECT_TRACE(7, "<<<");

	return true;
#else
	// Based On: https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation?redirectedfrom=MSDN
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfoStart = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION currProcInfo = NULL;
	DWORD length = 0;

	while (true)
	{
		if (!GetLogicalProcessorInformation(procInfoStart, &length))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				//TODO: need hooks for malloc/free
				if (procInfoStart)
					free(procInfoStart);

				procInfoStart = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(length);

				if (procInfoStart == NULL)
					return false;
			}
			else
			{
				return false;
			}
		}
		else
		{
			break;
		}
	}

	currProcInfo = procInfoStart;

	for (DWORD offset = 0; offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= length; offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION), currProcInfo++)
	{
		switch (currProcInfo->Relationship)
		{
		case RelationNumaNode:
			// Non-NUMA systems report a single record of this type.
			procInfo.numNUMANodes++;
			break;
		case RelationProcessorCore:
			// A hyperthreaded core supplies more than one logical processor.
			procInfo.numPhysicalCores++;
			procInfo.numLogicalCores += std::bitset<64>(currProcInfo->ProcessorMask).count();
			break;
		case RelationProcessorPackage:
			// Logical processors share a physical package.
			procInfo.numProcessorPackages++;
			break;
		default:
			break;
		}
	}

	//TODO: need hooks for malloc/free
	free(procInfoStart);
	return true;
#endif
#else
	return false;
#endif

}

// Calls CPUID & GetLogicalProcessors to fill in PROCESSOR_INFO Caches & Cores
inline bool GetLogicalProcessorsEx(PROCESSOR_INFO& procInfo)
{
	HYBRID_DETECT_TRACE(7, ">>>");

#ifdef ENABLE_HYBRID_DETECT
	for (EnumLogicalProcessorInformation enumInfo(RelationGroup);
		auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
		GROUP_INFO group;
		procInfo.numGroups++;
		group.activeGroupCount = pinfo->Group.ActiveGroupCount;
		group.maximumGroupCount = pinfo->Group.MaximumGroupCount;
		group.activeProcessorCount = pinfo->Group.GroupInfo->ActiveProcessorCount;
		group.maximumProcessorCount = pinfo->Group.GroupInfo->MaximumProcessorCount;
		group.activeProcessorMask = (ULONG)pinfo->Group.GroupInfo->ActiveProcessorMask;
		HYBRID_DETECT_TRACE(5, "=== group %d: ActiveProcessorCount = %d, MaximumProcessorCount = %d", procInfo.numGroups - 1, pinfo->Group.GroupInfo->ActiveProcessorCount, pinfo->Group.GroupInfo->MaximumProcessorCount);
		procInfo.groups.push_back(group);
	}

	for (EnumLogicalProcessorInformation enumInfo(RelationNumaNode);
		auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
		NUMA_NODE_INFO node;
		procInfo.numNUMANodes++;
		node.nodeNumber = pinfo->NumaNode.NodeNumber;
		node.group = pinfo->NumaNode.GroupMask.Group;
		node.mask = (ULONG)pinfo->NumaNode.GroupMask.Mask;
		procInfo.nodes.push_back(node);
	}

	for (EnumLogicalProcessorInformation enumInfo(RelationProcessorPackage);
		auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
		++procInfo.numProcessorPackages;
	}

	for (EnumLogicalProcessorInformation enumInfo(RelationProcessorCore);
		auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
		procInfo.numPhysicalCores++;
	}

	for (EnumLogicalProcessorInformation enumInfo(RelationCache);
		auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
		CACHE_INFO cacheInfo;

		switch (pinfo->Cache.Level) {
		case 1: procInfo.numL1Caches++; break;
		case 2: procInfo.numL2Caches++; break;
		case 3: procInfo.numL3Caches++; break;
		}

		cacheInfo.group = pinfo->Cache.GroupMask.Group;
		cacheInfo.processorMask = pinfo->Cache.GroupMask.Mask;
		cacheInfo.level = pinfo->Cache.Level;
		cacheInfo.size = pinfo->Cache.CacheSize;
		cacheInfo.lineSize = pinfo->Cache.LineSize;
		cacheInfo.associativity = pinfo->Cache.Associativity;
		cacheInfo.type = pinfo->Cache.Type;

		procInfo.caches.push_back(cacheInfo);
	}
	HYBRID_DETECT_TRACE(7, "<<<");

	return true;
#else
	return false;
#endif
}

inline void UpdateProcessorInfo(PROCESSOR_INFO& procInfo)
{
#ifdef HYBRIDDETECT_OS_WIN
	// TODO: Not sure how this works with processor groups
	// Query Current Frequency for each Logical Processor using CallNTPowerInformation
	// https://docs.microsoft.com/en-us/windows/win32/api/powerbase/nf-powerbase-callntpowerinformation
	std::vector<LOGICAL_PROCESSOR_POWER_INFORMATION> pwrInfo(procInfo.numLogicalCores);
	DWORD size = sizeof(LOGICAL_PROCESSOR_POWER_INFORMATION) * procInfo.numLogicalCores;
	CallNtPowerInformation(ProcessorInformation, nullptr, 0, &pwrInfo[0], size);

	for (int i = 0; i < procInfo.cores.size(); i++)
	{
		procInfo.cores[i].currentFrequency = pwrInfo[i].currentMhz;
		procInfo.cores[i].powerInformation = pwrInfo[i];
	}
#endif
}
#endif // HYBRIDDETECT_OS_WIN

// Calls CPUID & GetLogicalProcessors & CallNTPowerInformation to fill in PROCESSOR_INFO
inline void GetProcessorInfo(PROCESSOR_INFO& procInfo)
{
	HYBRID_DETECT_TRACE(7, ">>>");
#ifdef ENABLE_HYBRID_DETECT
	// https://software.intel.com/content/www/us/en/develop/download/intel-architecture-instruction-set-extensions-programming-reference.html

		// Store registers from CPUID
	std::array<unsigned, 4>  cpuInfo;

	// Maximum leaf ordinal Reported by CPUID
	unsigned            CPUIDFunctionMax;

	std::bitset<32>     bits;

	procInfo.coreMasks.emplace(CoreTypes::ANY, 0);
	procInfo.coreMasks.emplace(CoreTypes::NONE, 0);
	procInfo.coreMasks.emplace(CoreTypes::RESERVED0, 0);
	procInfo.coreMasks.emplace(CoreTypes::INTEL_ATOM, 0);
	procInfo.coreMasks.emplace(CoreTypes::RESERVED1, 0);
	procInfo.coreMasks.emplace(CoreTypes::INTEL_CORE, 0);

#ifdef ENABLE_CPU_SETS
	procInfo.cpuSets.emplace(CoreTypes::ANY, std::vector<ULONG>());
	procInfo.cpuSets.emplace(CoreTypes::NONE, std::vector<ULONG>());
	procInfo.cpuSets.emplace(CoreTypes::RESERVED0, std::vector<ULONG>());
	procInfo.cpuSets.emplace(CoreTypes::INTEL_ATOM, std::vector<ULONG>());
	procInfo.cpuSets.emplace(CoreTypes::RESERVED1, std::vector<ULONG>());
	procInfo.cpuSets.emplace(CoreTypes::INTEL_CORE, std::vector<ULONG>());
#endif

	//Basic CPUID Information
	CallCPUID(LEAF_CPUID_BASIC, cpuInfo);

	// Store highest function number.
	CPUIDFunctionMax = cpuInfo[CPUID_EAX];

	// Read Vendor ID from CPUID
	memcpy(procInfo.vendorID + 0, &cpuInfo[CPUID_EBX], sizeof(cpuInfo[CPUID_EBX]));
	memcpy(procInfo.vendorID + 4, &cpuInfo[CPUID_EDX], sizeof(cpuInfo[CPUID_EDX]));
	memcpy(procInfo.vendorID + 8, &cpuInfo[CPUID_ECX], sizeof(cpuInfo[CPUID_ECX]));
	procInfo.vendorID[12] = '\0';

	CallCPUID(1, cpuInfo);
	bits = cpuInfo[CPUID_ECX];
	procInfo.flags.SSE3 = bits[0];
	procInfo.flags.PCLMULQDQ = bits[1];
	procInfo.flags.SSSE3 = bits[9];
	procInfo.flags.FMA = bits[12];
	procInfo.flags.SSE4_1 = bits[19];
	procInfo.flags.SSE4_2 = bits[20];
	procInfo.flags.AES = bits[25];
	procInfo.flags.XSAVE = bits[26];
	procInfo.flags.OSXSAVE = bits[27];
	procInfo.flags.AVX = bits[28];
	procInfo.flags.F16C = bits[29];
	procInfo.flags.RDRAND = bits[30];

	if (procInfo.flags.OSXSAVE)
	{
		uint64_t xcr0 = XGETBV(0);
		if (xcr0 & 0x6)
		{
			procInfo.flags.OS_Supports_YMM = 1;
			if (xcr0 & 0xe0)
			{
				procInfo.flags.OS_Supports_ZMM = 1;
			}
		}
	}

	CallCPUID(LEAF_EXTENDED_FEATURE_FLAGS, cpuInfo, 0, CPUIDFunctionMax);
	{
		bits = cpuInfo[CPUID_EBX];
		procInfo.flags.AVX2 = bits[5];
		procInfo.flags.AVX512F = bits[16];
		procInfo.flags.AVX512DQ = bits[17];
		procInfo.flags.AVX512_IFMA = bits[21];
		procInfo.flags.AVX512CD = bits[28];
		procInfo.flags.AVX512BW = bits[30];
		procInfo.flags.AVX512VL = bits[31];
	}

	// Read Brand String from Extended CPUID information
	CallCPUID(LEAF_EXTENDED_BRAND_STRING_1, cpuInfo);
	memcpy(procInfo.brandString + 00, cpuInfo.data(), sizeof(cpuInfo));
	CallCPUID(LEAF_EXTENDED_BRAND_STRING_2, cpuInfo);
	memcpy(procInfo.brandString + 16, cpuInfo.data(), sizeof(cpuInfo));
	CallCPUID(LEAF_EXTENDED_BRAND_STRING_3, cpuInfo);
	memcpy(procInfo.brandString + 32, cpuInfo.data(), sizeof(cpuInfo));

	// Structured Extended Feature Flags Enumeration Leaf 
	// (Output depends on ECX input value)
	CallCPUID(LEAF_EXTENDED_FEATURE_FLAGS, cpuInfo);
	{
		bits = cpuInfo[CPUID_EDX];
#ifndef ENABLE_SOFTWARE_PROXY
		procInfo.hybrid = bits[15];
#else
		procInfo.hybrid = true;
#endif
	}

	// Thermal and Power Management Leaf
	CallCPUID(LEAF_THERMAL_POWER, cpuInfo);
	{
		bits = cpuInfo[CPUID_EAX];
		procInfo.turboBoost = bits[1];
		procInfo.turboBoost3_0 = bits[14];
	}

#ifdef HYBRIDDETECT_OS_WIN
	DWORD_PTR           affinityMask;
	DWORD_PTR           processAffinityMask;
	DWORD_PTR           sysAffinityMask;

	// What does the process & system allow
	GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &sysAffinityMask);

	GetLogicalProcessorsEx(procInfo);

	// Fill in Logical Processors, short circuit if error.
	if (GetLogicalProcessors(procInfo))
	{
		HYBRID_DETECT_TRACE(5, "=== procInfo.numLogicalCores = %d, procInfo.numGroups = %d", procInfo.numLogicalCores, procInfo.numGroups);

		// Query Current Frequency for each Logical Processor using CallNTPowerInformation
		// https://docs.microsoft.com/en-us/windows/win32/api/powerbase/nf-powerbase-callntpowerinformation
		std::vector<LOGICAL_PROCESSOR_POWER_INFORMATION> pwrInfo(procInfo.numLogicalCores);
		DWORD size = sizeof(LOGICAL_PROCESSOR_POWER_INFORMATION) * procInfo.numLogicalCores;
		CallNtPowerInformation(ProcessorInformation, nullptr, 0, &pwrInfo[0], size);

		for (unsigned group = 0; group < procInfo.numGroups; group++)
		{
			// TODO Use Thread Group Affinity https://docs.microsoft.com/en-us/windows/win32/api/processtopologyapi/nf-processtopologyapi-setthreadgroupaffinity
			GROUP_AFFINITY nextGroup;
			GROUP_AFFINITY prevGroup;
			nextGroup.Group = group;
			nextGroup.Mask = 0xffffffff;

			SetThreadGroupAffinity(GetCurrentThread(), &nextGroup, &prevGroup);

			HYBRID_DETECT_TRACE(5, "=== group = %d, procInfo.groups[group].maximumProcessorCount = %d, procInfo.groups[group].activeProcessorCount = %d", group,
				procInfo.groups[group].maximumProcessorCount, procInfo.groups[group].activeProcessorCount);

			// Enumerate each logical core. Need active or maximum processor count?
			for (unsigned core = 0; core < procInfo.groups[group].activeProcessorCount; core++)
			{
				HYBRID_DETECT_TRACE(5, "=== core = %d", core);

				// Logical Processor Info struct for storage.
#ifdef ENABLE_CPU_SETS
				LOGICAL_PROCESSOR_INFO& logicalCore = procInfo.cores[core];
#else
				LOGICAL_PROCESSOR_INFO					logicalCore;
				logicalCore.logicalProcessorIndex = core;
#endif
				// Get the ID of the current processor;
				//int                                     prevProcessor = GetCurrentProcessorNumber();

				// Convert the oridinal position to an affinity mask.
				affinityMask = (DWORD)IndexToMask(core);

				// Thread Affinity Mask is enough to switch to current thread immediatlely. 
				SetThreadAffinityMask(GetCurrentThread(), processAffinityMask & affinityMask);

				logicalCore.processorMask = std::bitset<64>(affinityMask);

#if ENABLE_PER_LOGICAL_CPUID_ISA_DETECTION
				// Processor Extended State Enumeration Main Leaf (EAX = 0DH, ECX = 0)
				CallCPUID(LEAF_EXTENDED_STATE, cpuInfo);
				{
					bits = cpuInfo[CPUID_EAX];
					logicalCore.SSE = bits[1];
					logicalCore.AVX = bits[2];
					logicalCore.AVX512 = bits[5];
				}

				// Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value)
				CallCPUID(LEAF_EXTENDED_FEATURE_FLAGS, cpuInfo);
				{
					bits = cpuInfo[CPUID_EBX];
					logicalCore.AVX2 = bits[5];
					logicalCore.AVX512F = bits[16];
					logicalCore.AVX512DQ = bits[17];
					logicalCore.AVX512_IFMA = bits[21];
					logicalCore.AVX512PF = bits[26];
					logicalCore.AVX512ER = bits[27];
					logicalCore.AVX512CD = bits[28];
					logicalCore.SHA = bits[29];
					logicalCore.SGX = bits[2];
					logicalCore.AVX512BW = bits[30];
					logicalCore.AVX512VL = bits[31];

					bits = cpuInfo[CPUID_ECX];
					logicalCore.AVX512_VBMI = bits[1];
					logicalCore.AVX512_VBMI2 = bits[6];
					logicalCore.AVX512_VNNI = bits[11];
					logicalCore.AVX512_BITALG = bits[12];
					logicalCore.AVX512_VPOPCNTDQ = bits[14];

					bits = cpuInfo[CPUID_EDX];
					logicalCore.AVX512_4VNNIW = bits[2];
					logicalCore.AVX512_4FMAPS = bits[3];
					logicalCore.AVX512_VP2INTERSECT = bits[8];
				}
#endif // ENABLE_PER_LOGICAL_CPUID_ISA_DETECTION

				// Processor Frequency Information Leaf  function 0x16 only works on Sky-lake or newer.
				CallCPUID(LEAF_FREQUENCY_INFORMATION, cpuInfo);
				{
					logicalCore.baseFrequency = cpuInfo[CPUID_EAX];
					logicalCore.maximumFrequency = cpuInfo[CPUID_EBX];
					logicalCore.busFrequency = cpuInfo[CPUID_ECX];

					// Fall-back to ProcessorPowerInfo for older CPUs
					if (logicalCore.baseFrequency == 0)
					{
						logicalCore.baseFrequency = pwrInfo[core].mhzLimit;
						logicalCore.maximumFrequency = pwrInfo[core].mhzLimit;
					}

					logicalCore.currentFrequency = pwrInfo[core].currentMhz;
					logicalCore.powerInformation = pwrInfo[core];
				}

				// Hybrid Information Sub - leaf(EAX = 1AH, ECX = 0)
				CallCPUID(LEAF_HYBRID_INFORMATION, cpuInfo);
				{
#ifndef ENABLE_SOFTWARE_PROXY
					std::bitset<8> coreTypeBits;    // Bits 31 - 24: Core type
					bits = cpuInfo[CPUID_EAX];
					coreTypeBits[0] = bits[24];
					coreTypeBits[1] = bits[25];
					coreTypeBits[2] = bits[26];
					coreTypeBits[3] = bits[27];
					coreTypeBits[4] = bits[28];
					coreTypeBits[5] = bits[29];
					coreTypeBits[6] = bits[30];
					coreTypeBits[7] = bits[31];
					logicalCore.coreType = (CoreTypes)coreTypeBits.to_ulong();
#else
					std::string s(procInfo.brandString);

					if (s.find("Platinum 8268") != std::string::npos)
					{
						logicalCore.coreType = (core < 12) ? CoreTypes::INTEL_CORE : CoreTypes::INTEL_ATOM;
					}
					else if (s.find("i9-9980XE") != std::string::npos)
					{
						logicalCore.coreType = (core < 16) ? CoreTypes::INTEL_CORE : CoreTypes::INTEL_ATOM;
					}
					else
					{
						// Split homogeneous cores into logical heterogeneous 50/50 clusters
						logicalCore.coreType = (core < (procInfo.numLogicalCores / 2) ? CoreTypes::INTEL_CORE : CoreTypes::INTEL_ATOM);

						// Useful for Testing Asymetric Hybrid Configurations (2+8 (4 HT Cores + 8 Atom Cores)
						//logicalCore.coreType = (core < 4 ? CoreTypes::INTEL_CORE : CoreTypes::INTEL_ATOM);
					}
#endif
				}

				// Heterogeneous processor clusters
				procInfo.coreMasks[CoreTypes::ANY] |= affinityMask;

				// Homogeneous processor clusters
				procInfo.coreMasks[logicalCore.coreType] |= affinityMask;

#ifdef ENABLE_CPU_SETS
				procInfo.cpuSets[CoreTypes::ANY].push_back(logicalCore.id);
				procInfo.cpuSets[logicalCore.coreType].push_back(logicalCore.id);
#else
				procInfo.cores.push_back(logicalCore);
#endif
				//pwrInfo.clear();
			}

			

			// Reset Group Affinity
			SetThreadGroupAffinity(GetCurrentThread(), &prevGroup, nullptr);
		}

		// Reset Process Affinity Mask
		SetThreadAffinityMask(GetCurrentThread(), processAffinityMask);
	}
#endif // HYBRIDDETECT_OS_WIN
#endif
	HYBRID_DETECT_TRACE(7, "<<< ");
}

#ifdef HYBRIDDETECT_OS_WIN
inline bool SetMemoryPriority(HANDLE threadHandle, UINT memoryPriority)
{
	MEMORY_PRIORITY_INFORMATION memoryPriorityInfo;
	ZeroMemory(&memoryPriorityInfo, sizeof(memoryPriorityInfo));

	memoryPriorityInfo.MemoryPriority = memoryPriority;

	return SetThreadInformation(threadHandle, ThreadMemoryPriority,
		&memoryPriorityInfo, sizeof(memoryPriorityInfo));
}

inline bool EnablePowerThrottling(HANDLE threadHandle)
{
	THREAD_POWER_THROTTLING_STATE throttlingState;
	RtlZeroMemory(&throttlingState, sizeof(throttlingState));

	throttlingState.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
	throttlingState.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
	throttlingState.StateMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;

	return SetThreadInformation(threadHandle, ThreadPowerThrottling,
		&throttlingState, sizeof(throttlingState));
}

inline bool DisablePowerThrottling(HANDLE threadHandle)
{
	THREAD_POWER_THROTTLING_STATE throttlingState;
	RtlZeroMemory(&throttlingState, sizeof(throttlingState));

	throttlingState.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
	throttlingState.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
	throttlingState.StateMask = 0;

	return SetThreadInformation(threadHandle, ThreadPowerThrottling,
		&throttlingState, sizeof(throttlingState));
}

inline bool AutoPowerThrottling(HANDLE threadHandle)
{
	THREAD_POWER_THROTTLING_STATE throttlingState;
	RtlZeroMemory(&throttlingState, sizeof(throttlingState));

	throttlingState.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
	throttlingState.ControlMask = 0;
	throttlingState.StateMask = 0;

	return SetThreadInformation(threadHandle, ThreadPowerThrottling,
		&throttlingState, sizeof(throttlingState));
}
#endif

#ifdef ENABLE_CPU_SETS

inline short RunOnCPUSet(PROCESSOR_INFO& procInfo, HANDLE threadHandle, std::vector<ULONG> cpuSet, std::vector<ULONG> fallbackSet = {})
{
#ifdef ENABLE_RUNON
	if (cpuSet.size() > 0)
	{
		if (SetThreadSelectedCpuSets(threadHandle, &cpuSet[0], cpuSet.size()))
		{
			return 1;
		}
	}

	if (fallbackSet.size() > 0)
	{
		if (SetThreadSelectedCpuSets(threadHandle, &fallbackSet[0], fallbackSet.size()))
		{
			return 0;
		}
	}

	std::vector<ULONG> anySet = procInfo.cpuSets[CoreTypes::ANY];

	if (anySet.size() > 0)
	{
		if (SetThreadSelectedCpuSets(threadHandle, &anySet[0], anySet.size()))
		{
			return -1;
		}
	}

#endif
	return -1;
}

// Run A Thread On the Atom or Core Logical Processor Cluster
inline short RunOn(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const CoreTypes type, const std::vector<ULONG> fallbackSet = {})
{
#ifdef ENABLE_RUNON_PRIORITY
	switch (type)
	{
	case INTEL_ATOM:
		SetThreadPriority(threadHandle, THREAD_PRIORITY_BELOW_NORMAL);
		break;
	case INTEL_CORE:
		SetThreadPriority(threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);
		break;
	default:
		SetThreadPriority(threadHandle, THREAD_PRIORITY_NORMAL);
		break;
	}
#endif

#ifdef ENABLE_RUNON_MEMORY_PRIORITY
	switch (type)
	{
	case INTEL_ATOM:
		SetMemoryPriority(threadHandle, MEMORY_PRIORITY_NORMAL);
		break;
	case INTEL_CORE:
		SetMemoryPriority(threadHandle, MEMORY_PRIORITY_BELOW_NORMAL);
		break;
	default:
		SetMemoryPriority(threadHandle, MEMORY_PRIORITY_BELOW_NORMAL);
		break;
	}
#endif

#ifdef ENABLE_RUNON_EXECUTION_SPEED
	switch (type)
	{
	case INTEL_ATOM:
		EnablePowerThrottling(threadHandle);
		break;
	case INTEL_CORE:
		DisablePowerThrottling(threadHandle);
		break;
	default:
		AutoPowerThrottling(threadHandle);
		break;
	}
#endif

#ifdef ENABLE_RUNON
	//assert(procInfo.coreMasks.size());

	if (procInfo.cpuSets.size() > 0)
	{
		std::vector<ULONG> cpuSet = procInfo.cpuSets[type];
		return RunOnCPUSet(procInfo, threadHandle, cpuSet, fallbackSet);
	}

	return RunOnCPUSet(procInfo, threadHandle, fallbackSet);
#else
	return 0;
#endif
}

// Run The Current Thread On Atom or Core Logical Processor Cluster
inline short RunOn(PROCESSOR_INFO& procInfo, const CoreTypes type, const std::vector<ULONG> fallbackSet = {})
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOn(procInfo, threadHandle, type, fallbackSet);
#else
	return 0;
#endif
}

// Run A Thread On Any Logical Processor
inline short RunOnAny(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const std::vector<ULONG> fallbackSet = {})
{
#ifdef ENABLE_RUNON
	return RunOn(procInfo, threadHandle, CoreTypes::ANY, fallbackSet);
#else
	return 0;
#endif 
}

// Run The Current Thread On Any Logical Processor
inline short RunOnAny(PROCESSOR_INFO& procInfo, const std::vector<ULONG> fallbackSet = {})
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnAny(procInfo, threadHandle, fallbackSet);
#else
	return 0;
#endif
}

// Run A Thread On One Logical Processor
inline short RunOnOne(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const short coreID, const std::vector<ULONG> fallbackSet = {})
{
#ifdef ENABLE_RUNON
	bool succeeded = false;

#ifdef _DEBUG
	// Where are we starting?
	//int startedOn = GetCurrentProcessorNumber();
#endif
	if (coreID < procInfo.numLogicalCores)
	{
		std::vector<ULONG> coreSet = { procInfo.cores[coreID].id };
		short succeeded = RunOnCPUSet(procInfo, threadHandle, coreSet, fallbackSet);

#ifdef _DEBUG
		// Check to see if the core is masked
		//int iterations = 0;
		int runningOn = -1;
		do {
			// Surrender time-slice 
			Sleep(0);
			// Where Are We?
			runningOn = GetCurrentProcessorNumber();
			// Count Loop Iterations
			//iterations++;
			// Loop while the current thread is not scheduled on a logical processor matching the affinity mask.
		} while (runningOn != coreID);
		// Assert In Debug
		assert(runningOn == coreID);
		//Where did we land?
		int finishedOn = GetCurrentProcessorNumber();

		// Did we finish where we wanted?
		assert(runningOn == finishedOn);
#endif       

		return succeeded;
	}

	return RunOnCPUSet(procInfo, threadHandle, fallbackSet, procInfo.cpuSets[CoreTypes::ANY]);
#else
	return -1;
#endif
}

// Run The Current Thread On One Logical Processor
inline bool RunOnOne(PROCESSOR_INFO& procInfo, const short coreID, const std::vector<ULONG> fallbackSet = {})
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnOne(procInfo, threadHandle, coreID, fallbackSet);
#else
	return false;
#endif
}

#else

// Run A Current Thread On A Custom Logical Processor Cluster
inline short RunOnMask(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const ULONG64 mask, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	DWORD_PTR  processAffinityMask;
	DWORD_PTR  systemAffinityMask;

	// Get the system and process affinity mask
	GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);

	ULONG64 threadAffinityMask = mask;

	//assert((systemAffinityMask & threadAffinityMask));
	//assert((processAffinityMask & threadAffinityMask));

	// Is the thread-mask allowed in this system/process?
	if ((systemAffinityMask & threadAffinityMask) && (processAffinityMask & threadAffinityMask))
	{
		// Run On Thread In Process
		SetThreadAffinityMask(threadHandle, processAffinityMask & threadAffinityMask);
		return 1;
	}

	//assert(systemAffinityMask & fallbackMask);
	//assert(processAffinityMask & fallbackMask);

	// Is fall-back thread-mask allowed in this system/process?
	if ((systemAffinityMask & fallbackMask) && (processAffinityMask & fallbackMask))
	{
		SetThreadAffinityMask(threadHandle, processAffinityMask & fallbackMask);
		return 0;
	}
	else
	{
		// Fallback to process affinity mask!
		SetThreadAffinityMask(threadHandle, processAffinityMask);
		return -1;
	}
#endif
	return -1;
}

// Run The Current Thread On A Custom Logical Processor Cluster
inline short RunOnMask(PROCESSOR_INFO& procInfo, const ULONG64 mask, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnMask(procInfo, threadHandle, mask, fallbackMask);
#else
	return 0;
#endif
}

// Run A Thread On the Atom or Core Logical Processor Cluster
inline short RunOn(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const CoreTypes type, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON_PRIORITY
	switch (type)
	{
	case INTEL_ATOM:
		SetThreadPriority(threadHandle, THREAD_PRIORITY_BELOW_NORMAL);
		break;
	case INTEL_CORE:
		SetThreadPriority(threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);
		break;
	default:
		SetThreadPriority(threadHandle, THREAD_PRIORITY_NORMAL);
		break;
	}
#endif
#ifdef ENABLE_RUNON
	//assert(procInfo.coreMasks.size());

	if (procInfo.coreMasks.size())
	{
		ULONG64 threadMask = procInfo.coreMasks[type];
		return RunOnMask(procInfo, threadHandle, threadMask, fallbackMask);
	}

	return RunOnMask(procInfo, threadHandle, fallbackMask);
#else
	return 0;
#endif
}

// Run The Current Thread On Atom or Core Logical Processor Cluster
inline short RunOn(PROCESSOR_INFO& procInfo, const CoreTypes type, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOn(procInfo, threadHandle, type, fallbackMask);
#else
	return 0;
#endif
}

// Run A Thread On Any Logical Processor
inline short RunOnAny(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	return RunOn(procInfo, threadHandle, CoreTypes::ANY, fallbackMask);
#else
	return 0;
#endif 
}

// Run The Current Thread On Any Logical Processor
inline short RunOnAny(PROCESSOR_INFO& procInfo, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnAny(procInfo, threadHandle, fallbackMask);
#else
	return 0;
#endif
}

// Run A Thread On One Logical Processor
inline bool RunOnOne(PROCESSOR_INFO& procInfo, HANDLE threadHandle, const short coreID, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	bool succeeded = false;

#ifdef _DEBUG
	// Where are we starting?
	int startedOn = GetCurrentProcessorNumber();
#endif
	if (coreID < procInfo.numLogicalCores)
	{
		bool succeeded = RunOnMask(procInfo, threadHandle, IndexToMask(coreID), fallbackMask);

#ifdef _DEBUG
		// Check to see if the core is masked
		int iterations = 0;
		int runningOn = -1;
		do {
			// Surrender time-slice 
			Sleep(0);
			// Where Are We?
			runningOn = GetCurrentProcessorNumber();
			// Count Loop Iterations
			iterations++;
			// Loop while the current thread is not scheduled on a logical processor matching the affinity mask.
		} while (runningOn != coreID);
		// Assert In Debug
		assert(runningOn == coreID);
		//Where did we land?
		int finishedOn = GetCurrentProcessorNumber();

		// Did we finish where we wanted?
		assert(runningOn == finishedOn);
#endif       

		return succeeded;
	}

	return RunOnMask(procInfo, threadHandle, IndexToMask(coreID), fallbackMask);
#else
	return false;
#endif
}

// Run The Current Thread On One Logical Processor
inline bool RunOnOne(PROCESSOR_INFO& procInfo, const short coreID, const ULONG64 fallbackMask = 0xffffffff)
{
#ifdef ENABLE_RUNON
	HANDLE threadHandle = GetCurrentThread();
	return RunOnOne(procInfo, threadHandle, coreID, fallbackMask);
#else
	return false;
#endif
}

#endif

} // namespace HybridDetect
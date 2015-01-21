/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  PlatformCpuInfo.h

Abstract:

  GUID used for Platform CPU Info Data entries in the HOB list.

--*/

#ifndef _PLATFORM_CPU_INFO_GUID_H_
#define _PLATFORM_CPU_INFO_GUID_H_

#include "CpuType.h"
#include <Library/CpuIA32.h>

#define EFI_PLATFORM_CPU_INFO_GUID \
  {\
    0xbb9c7ab7, 0xb8d9, 0x4bf3, 0x9c, 0x29, 0x9b, 0xf3, 0x41, 0xe2, 0x17, 0xbc \
  }

extern EFI_GUID gEfiPlatformCpuInfoGuid;
extern CHAR16   EfiPlatformCpuInfoVariable[];

//
// Tri-state for feature capabilities and enable/disable.
// [0] clear=feature isn't capable
// [0] set  =feature is capable
// [1] clear=feature is disabled
// [1] set  =feature is enabled
//
#define CPU_FEATURES_CAPABLE      BIT0
#define CPU_FEATURES_ENABLE       BIT1

#define MAX_CACHE_DESCRIPTORS     64
#define MAXIMUM_CPU_BRAND_STRING_LENGTH 48

#pragma pack(1)

typedef struct {
  UINT32              FullCpuId;                // [31:0] & 0x0FFF0FFF
  UINT32              FullFamilyModelId;        // [31:0] & 0x0FFF0FF0
  UINT8               ExtendedFamilyId;         // [27:20]
  UINT8               ExtendedModelId;          // [19:16]
  UINT8               ProcessorType;            // [13:11]
  UINT8               FamilyId;                 // [11:8]
  UINT8               Model;                    // [7:4]
  UINT8               SteppingId;               // [3:0]
} EFI_CPU_VERSION_INFO; // CPUID.1.EAX

typedef struct {
  UINT32              L1InstructionCacheSize;
  UINT32              L1DataCacheSize;
  UINT32              L2CacheSize;
  UINT32              L3CacheSize;
  UINT32              TraceCacheSize;
  UINT8               CacheDescriptor[MAX_CACHE_DESCRIPTORS];
} EFI_CPU_CACHE_INFO; // CPUID.2.EAX

typedef struct {
  UINT8               PhysicalPackages;
  UINT8               LogicalProcessorsPerPhysicalPackage;
  UINT8               CoresPerPhysicalPackage;
  UINT8               ThreadsPerCore;
} EFI_CPU_PACKAGE_INFO; // CPUID.4.EAX

typedef struct {
  UINT32              RegEdx;                   // CPUID.5.EAX
  UINT8               MaxCState;
  UINT8               C0SubCStatesMwait;        // EDX [3:0]
  UINT8               C1SubCStatesMwait;        // EDX [7:4]
  UINT8               C2SubCStatesMwait;        // EDX [11:8]
  UINT8               C3SubCStatesMwait;        // EDX [15:12]
  UINT8               C4SubCStatesMwait;        // EDX [19:16]
  UINT8               C5SubCStatesMwait;        // EDX [23:20]
  UINT8               C6SubCStatesMwait;        // EDX [27:24]
  UINT8               C7SubCStatesMwait;        // EDX [31:28]
  UINT8               MonitorMwaitSupport;      // ECX [0]
  UINT8               InterruptsBreakMwait;     // ECX [1]
} EFI_CPU_CSTATE_INFO; // CPUID.5.EAX

typedef struct {
  UINT8               Turbo;                    // EAX [1]
  UINT8               PECI;                     // EAX [0]
  UINT8               NumIntThresholds;         // EBX [3:0]
  UINT8               HwCoordinationFeedback;   // ECX [0]
} EFI_CPU_POWER_MANAGEMENT; // CPUID.6.EAX

//
// IMPORTANT: Each CPU feature enabling entry is assumed a tri-state variable.
//   - Keep the respective feature entry variable as default value (0x00)
//     if the CPU is not capable for the feature.
//   - Use the specially defined programming convention to update the variable
//     to indicate capable, enable or disable.
//     ie. F_CAPABLE for feature available
//         F_ENABLE for feature enable
//         F_DISABLE for feature disable
//
typedef struct {
  EFI_CPUID_REGISTER  Regs;                     // CPUID.1.EAX
  UINT8               Xapic;                    // ECX [21]
  UINT8               SSE4_2;                   // ECX [20]
  UINT8               SSE4_1;                   // ECX [19]
  UINT8               Dca;                      // ECX [18]
  UINT8               SupSSE3;                  // ECX [9]
  UINT8               Tm2;                      // ECX [8]
  UINT8               Eist;                     // ECX [7]
  UINT8               Lt;                       // ECX [6]
  UINT8               Vt;                       // ECX [5]
  UINT8               Mwait;                    // ECX [3]
  UINT8               SSE3;                     // ECX [0]
  UINT8               Tcc;                      // EDX [29]
  UINT8               Mt;                       // EDX [28]
  UINT8               SSE2;                     // EDX [26]
  UINT8               SSE;                      // EDX [25]
  UINT8               MMX;                      // EDX [23]
  EFI_CPUID_REGISTER  ExtRegs;                  // CPUID.80000001.EAX
  UINT8               ExtLahfSahf64;            // ECX [0]
  UINT8               ExtIntel64;               // EDX [29]
  UINT8               ExtXd;                    // EDX [20]
  UINT8               ExtSysCallRet64;          // EDX [11]
  UINT16              Ht;                       // CPUID.0B.EAX EBX [15:0]
} EFI_CPU_FEATURES; // CPUID.1.EAX, CPUID.0B.EAX, CPUID.80000001.EAX

typedef struct {
  UINT8               PhysicalBits;
  UINT8               VirtualBits;
} EFI_CPU_ADDRESS_BITS; // CPUID.80000008.EAX

typedef struct {
  UINT8               PlatformID;               // MSR 0x17 [52:50]
  UINT32              MicrocodeRevision;        // MSR 0x8B [63:32]
  UINT8               MaxEfficiencyRatio;       // MSR 0xCE [47:40]
  UINT8               DdrRatioUnlockCap;        // MSR 0xCE [30]
  UINT8               TdcTdpLimitsTurbo;        // MSR 0xCE [29]
  UINT8               RatioLimitsTurbo;         // MSR 0xCE [28]
  UINT8               PreProduction;            // MSR 0xCE [27]
  UINT8               DcuModeSelect;            // MSR 0xCE [26]
  UINT8               MaxNonTurboRatio;         // MSR 0xCE [15:8]
  UINT8               Emrr;                     // MSR 0xFE [12]
  UINT8               Smrr;                     // MSR 0xFE [11]
  UINT8               VariableMtrrCount;        // MSR 0xFE [7:0]
  UINT16              PState;                   // MSR 0x198 [15:0]
  UINT8               TccActivationTemperature; // MSR 0x1A2 [23:16]
  UINT8               TemperatureControlOffset; // MSR 0x1A2 [15:8]
  UINT32              PCIeBar;                  // MSR 0x300 [39:20]
  UINT8               PCIeBarSizeMB;            // MSR 0x300 [3:1]
} EFI_MSR_FEATURES;

typedef struct {
  BOOLEAN                            IsIntelProcessor;
  UINT8                              BrandString[MAXIMUM_CPU_BRAND_STRING_LENGTH + 1];
  UINT32                             CpuidMaxInputValue;
  UINT32                             CpuidMaxExtInputValue;
  EFI_CPU_UARCH                      CpuUarch;
  EFI_CPU_FAMILY                     CpuFamily;
  EFI_CPU_PLATFORM                   CpuPlatform;
  EFI_CPU_TYPE                       CpuType;
  EFI_CPU_VERSION_INFO               CpuVersion;
  EFI_CPU_CACHE_INFO                 CpuCache;
  EFI_CPU_FEATURES                   CpuFeatures;
  EFI_CPU_CSTATE_INFO                CpuCState;
  EFI_CPU_PACKAGE_INFO               CpuPackage;
  EFI_CPU_POWER_MANAGEMENT           CpuPowerManagement;
  EFI_CPU_ADDRESS_BITS               CpuAddress;
  EFI_MSR_FEATURES                   Msr;
} EFI_PLATFORM_CPU_INFO;

#pragma pack()

#endif

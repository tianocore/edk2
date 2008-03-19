/** @file

Copyright (c) 2006, Intel Corporation. All rights reserved.
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  CpuIA32.h

Abstract:

  Basic Definition for IA32 Architecture.

**/

#ifndef _CPU_IA32_H_
#define _CPU_IA32_H_

typedef struct {
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;
} EFI_CPUID_REGISTER;

#pragma pack(1)
//
// Definition for IA32 microcode format
//
typedef struct {
  UINT32  HeaderVersion;
  UINT32  UpdateRevision;
  UINT32  Date;
  UINT32  ProcessorId;
  UINT32  Checksum;
  UINT32  LoaderRevision;
  UINT32  ProcessorFlags;
  UINT32  DataSize;
  UINT32  TotalSize;
  UINT8   Reserved[12];
} EFI_CPU_MICROCODE_HEADER;

typedef struct {
  UINT32  ExtendedSignatureCount;
  UINT32  ExtendedTableChecksum;
  UINT8   Reserved[12];
} EFI_CPU_MICROCODE_EXTENDED_TABLE_HEADER;

typedef struct {
  UINT32  ProcessorSignature;
  UINT32  ProcessorFlag;
  UINT32  ProcessorChecksum;
} EFI_CPU_MICROCODE_EXTENDED_TABLE;

//
// The MS compiler doesn't handle QWORDs very well.  So break
// them into DWORDs to circumvent the problem.
//
typedef union _MSR_REGISTER {
  UINT64  Qword;

  struct _DWORDS {
    UINT32  Low;
    UINT32  High;
  } Dwords;

  struct _BYTES {
    UINT8 FirstByte;
    UINT8 SecondByte;
    UINT8 ThirdByte;
    UINT8 FouthByte;
    UINT8 FifthByte;
    UINT8 SixthByte;
    UINT8 SeventhByte;
    UINT8 EighthByte;
  } Bytes;

} MSR_REGISTER;

#pragma pack()

//
// Definition for CPUID Index
//
#define EFI_CPUID_SIGNATURE                   0x0
#define EFI_CPUID_VERSION_INFO                0x1
#define EFI_CPUID_CACHE_INFO                  0x2
#define EFI_CPUID_SERIAL_NUMBER               0x3
#define EFI_CPUID_EXTENDED_FUNCTION           0x80000000
#define EFI_CPUID_EXTENDED_CPU_SIG            0x80000001
#define EFI_CPUID_BRAND_STRING1               0x80000002
#define EFI_CPUID_BRAND_STRING2               0x80000003
#define EFI_CPUID_BRAND_STRING3               0x80000004
#define EFI_CPUID_ADDRESS_SIZE                0x80000008

//
// Definition for MSR address
//
#define EFI_MSR_IA32_PLATFORM_ID              0x17
#define EFI_MSR_IA32_APIC_BASE                0x1B
#define EFI_MSR_EBC_HARD_POWERON              0x2A
#define EFI_MSR_EBC_SOFT_POWERON              0x2B
#define EFI_MSR_EBC_FREQUENCY_ID              0x2C
#define MSR_IA32_FEATURE_CONTROL              0x3A
#define EFI_MSR_IA32_BIOS_UPDT_TRIG           0x79
#define EFI_MSR_IA32_BIOS_SIGN_ID             0x8B
#define EFI_MSR_PSB_CLOCK_STATUS              0xCD
#define MSR_EXT_CONFIG                        0xEE
#define EFI_IA32_MCG_CAP                      0x179
#define EFI_IA32_MCG_CTL                      0x17B

#define EFI_MSR_IA32_PERF_STS                 0x198
#define EFI_MSR_IA32_PERF_CTL                 0x199
#define EFI_MSR_IA32_CLOCK_MODULATION         0x19A
#define MSR_IA32_THERMAL_INTERRUPT            0x19B
#define EFI_MSR_IA32_THERM_STATUS             0x19C
#define EFI_MSR_GV_THERM                      0x19D
#define MSR_IA32_MISC_ENABLE                  0x1A0
#define MSR_PIC_SENS_CFG                      0x1AA

#define EFI_IA32_MC0_CTL                      0x400
#define EFI_IA32_MC0_STATUS                   0x401
#define MSR_PECI_CONTROL                      0x5A0

//
// Definition for MTRR address and related values
//
#define EFI_IA32_MTRR_FIX64K_00000            0x250
#define EFI_IA32_MTRR_FIX16K_80000            0x258
#define EFI_IA32_MTRR_FIX16K_A0000            0x259
#define EFI_IA32_MTRR_FIX4K_C0000             0x268
#define EFI_IA32_MTRR_FIX4K_C8000             0x269
#define EFI_IA32_MTRR_FIX4K_D0000             0x26A
#define EFI_IA32_MTRR_FIX4K_D8000             0x26B
#define EFI_IA32_MTRR_FIX4K_E0000             0x26C
#define EFI_IA32_MTRR_FIX4K_E8000             0x26D
#define EFI_IA32_MTRR_FIX4K_F0000             0x26E
#define EFI_IA32_MTRR_FIX4K_F8000             0x26F
#define EFI_CACHE_VARIABLE_MTRR_BASE          0x200
#define EFI_CACHE_VARIABLE_MTRR_END           0x20F
#define EFI_CACHE_IA32_MTRR_DEF_TYPE          0x2FF

#define EFI_CACHE_VALID_ADDRESS               0xFFFFFF000
#define EFI_MSR_VALID_MASK                    0xFFFFFFFFF
#define EFI_CACHE_MTRR_VALID                  0x800
#define EFI_CACHE_FIXED_MTRR_VALID            0x400

#define EFI_CACHE_UNCACHEABLE                 0
#define EFI_CACHE_WRITECOMBINING              1
#define EFI_CACHE_WRITETHROUGH                4
#define EFI_CACHE_WRITEPROTECTED              5
#define EFI_CACHE_WRITEBACK                   6

//
// Definition for Local APIC registers and related values
//
#define LOCAL_APIC_LVT_TIMER                  0x320
#define LOCAL_APIC_TIMER_INIT_COUNT           0x380
#define LOCAL_APIC_TIMER_COUNT                0x390
#define LOCAL_APIC_TIMER_DIVIDE               0x3E0


#define DELIVERY_MODE_FIXED                   0x0
#define DELIVERY_MODE_LOWEST_PRIORITY         0x1
#define DELIVERY_MODE_SMI                     0x2
#define DELIVERY_MODE_REMOTE_READ             0x3
#define DELIVERY_MODE_NMI                     0x4
#define DELIVERY_MODE_INIT                    0x5
#define DELIVERY_MODE_SIPI                    0x6

#define TRIGGER_MODE_EDGE                     0x0
#define TRIGGER_MODE_LEVEL                    0x1

//
// CPU System Memory Map Definition
//
#define CPU_MSI_MEMORY_BASE                   0xFEE00000
#define CPU_MSI_MEMORY_SIZE                   0x100000


#endif

/** @file
  Internal header file for CPU Cache info Library.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _INTERNAL_CPU_CACHE_INFO_LIB_H_
#define _INTERNAL_CPU_CACHE_INFO_LIB_H_

#include <PiPei.h>
#include <Register/Cpuid.h>
#include <Ppi/MpServices2.h>
#include <Protocol/MpService.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CpuCacheInfoLib.h>

typedef struct {
  //
  // Package ID, the information comes from
  // EFI_CPU_PHYSICAL_LOCATION.Package
  //
  UINT32                    Package;
  //
  // APIC ID, the information comes from
  // EFI_PROCESSOR_INFORMATION.ProcessorId
  //
  UINT32                    ApicId;
  //
  // Core type of logical processor.
  // Value = CPUID.1Ah:EAX[31:24]
  //
  UINT8                     CoreType;
} CPUID_PROCESSOR_INFO;

typedef struct {
  //
  // Level of the cache.
  // Value = CPUID.04h:EAX[07:05]
  //
  UINT8                     CacheLevel : 3;
  //
  // Type of the cache.
  // Value = CPUID.04h:EAX[04:00]
  //
  UINT8                     CacheType : 5;
  //
  // Ways of associativity.
  // Value = CPUID.04h:EBX[31:22]
  //
  UINT16                    CacheWays;
  //
  // Cache share bits.
  // Value = CPUID.04h:EAX[25:14]
  //
  UINT16                    CacheShareBits;
  //
  // Size of single cache.
  // Value = (CPUID.04h:EBX[31:22] + 1) * (CPUID.04h:EBX[21:12] + 1) *
  //         (CPUID.04h:EBX[11:00] + 1) * (CPUID.04h:ECX[31:00] + 1)
  //
  UINT32                    CacheSizeinKB;
} CPUID_CACHE_DATA;

typedef union {
  EDKII_PEI_MP_SERVICES2_PPI    *Ppi;
  EFI_MP_SERVICES_PROTOCOL      *Protocol;
} MP_SERVICES;

typedef struct {
  MP_SERVICES               MpServices;
  CPUID_PROCESSOR_INFO      *ProcessorInfo;
  CPUID_CACHE_DATA          *CacheData;
} COLLECT_CPUID_CACHE_DATA_CONTEXT;


/*
  Defines the maximum count of Deterministic Cache Parameters Leaf of all APs and BSP.
  To save boot time, skip starting up all APs to calculate each AP's count of Deterministic
  Cache Parameters Leaf, so use a definition instead.
  Anyway, definition value will be checked in CpuCacheInfoCollectCoreAndCacheData function.
*/
#define MAX_NUM_OF_CACHE_PARAMS_LEAF    6

/*
  Defines the maximum count of packages.
*/
#define MAX_NUM_OF_PACKAGE              100

/**
  Get EDKII_PEI_MP_SERVICES2_PPI or EFI_MP_SERVICES_PROTOCOL pointer.

  @param[out] MpServices    A pointer to the buffer where EDKII_PEI_MP_SERVICES2_PPI or
                            EFI_MP_SERVICES_PROTOCOL is stored

  @retval EFI_SUCCESS       EDKII_PEI_MP_SERVICES2_PPI or EFI_MP_SERVICES_PROTOCOL interface is returned
  @retval EFI_NOT_FOUND     EDKII_PEI_MP_SERVICES2_PPI or EFI_MP_SERVICES_PROTOCOL interface is not found
**/
EFI_STATUS
CpuCacheInfoGetMpServices (
  OUT MP_SERVICES           *MpServices
  );

/**
  Activate all of the logical processors.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[in]  Procedure           A pointer to the function to be run on enabled logical processors.
  @param[in]  ProcedureArgument   The parameter passed into Procedure for all enabled logical processors.
**/
VOID
CpuCacheInfoStartupAllCPUs (
  IN MP_SERVICES            MpServices,
  IN EFI_AP_PROCEDURE       Procedure,
  IN VOID                   *ProcedureArgument
  );

/**
  Get detailed information of the requested logical processor.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[in]  ProcessorNum        The requested logical processor number.
  @param[out] ProcessorInfo       A pointer to the buffer where the processor information is stored
**/
VOID
CpuCacheInfoGetProcessorInfo (
  IN MP_SERVICES                MpServices,
  IN UINTN                      ProcessorNum,
  OUT EFI_PROCESSOR_INFORMATION *ProcessorInfo
  );

/**
  Get the logical processor number.

  @param[in]  MpServices          MP_SERVICES structure.

  @retval  Return the logical processor number.
**/
UINT32
CpuCacheInfoWhoAmI (
  IN MP_SERVICES            MpServices
  );

/**
  Get the total number of logical processors in the platform.

  @param[in]  MpServices          MP_SERVICES structure.

  @retval  Return the total number of logical processors.
**/
UINT32
CpuCacheInfoGetNumberOfProcessors (
  IN MP_SERVICES            MpServices
  );
#endif

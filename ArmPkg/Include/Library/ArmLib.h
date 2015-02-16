/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_LIB__
#define __ARM_LIB__

#include <Uefi/UefiBaseType.h>

#ifdef MDE_CPU_ARM
  #ifdef ARM_CPU_ARMv6
    #include <Chipset/ARM1176JZ-S.h>
  #else
    #include <Chipset/ArmV7.h>
  #endif
#elif defined(MDE_CPU_AARCH64)
  #include <Chipset/AArch64.h>
#else
 #error "Unknown chipset."
#endif

typedef enum {
  ARM_CACHE_TYPE_WRITE_BACK,
  ARM_CACHE_TYPE_UNKNOWN
} ARM_CACHE_TYPE;

typedef enum {
  ARM_CACHE_ARCHITECTURE_UNIFIED,
  ARM_CACHE_ARCHITECTURE_SEPARATE,
  ARM_CACHE_ARCHITECTURE_UNKNOWN
} ARM_CACHE_ARCHITECTURE;

typedef struct {
  ARM_CACHE_TYPE          Type;
  ARM_CACHE_ARCHITECTURE  Architecture;
  BOOLEAN                 DataCachePresent;
  UINTN                   DataCacheSize;
  UINTN                   DataCacheAssociativity;
  UINTN                   DataCacheLineLength;
  BOOLEAN                 InstructionCachePresent;
  UINTN                   InstructionCacheSize;
  UINTN                   InstructionCacheAssociativity;
  UINTN                   InstructionCacheLineLength;
} ARM_CACHE_INFO;

/**
 * The UEFI firmware must not use the ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_* attributes.
 *
 * The Non Secure memory attribute (ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_*) should only
 * be used in Secure World to distinguished Secure to Non-Secure memory.
 */
typedef enum {
  ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED = 0,
  ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED,
  ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK,
  ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_BACK,
  ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH,
  ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_WRITE_THROUGH,
  ARM_MEMORY_REGION_ATTRIBUTE_DEVICE,
  ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE
} ARM_MEMORY_REGION_ATTRIBUTES;

#define IS_ARM_MEMORY_REGION_ATTRIBUTES_SECURE(attr) ((UINT32)(attr) & 1)

typedef struct {
  EFI_PHYSICAL_ADDRESS          PhysicalBase;
  EFI_VIRTUAL_ADDRESS           VirtualBase;
  UINT64                        Length;
  ARM_MEMORY_REGION_ATTRIBUTES  Attributes;
} ARM_MEMORY_REGION_DESCRIPTOR;

typedef VOID (*CACHE_OPERATION)(VOID);
typedef VOID (*LINE_OPERATION)(UINTN);

//
// ARM Processor Mode
//
typedef enum {
  ARM_PROCESSOR_MODE_USER       = 0x10,
  ARM_PROCESSOR_MODE_FIQ        = 0x11,
  ARM_PROCESSOR_MODE_IRQ        = 0x12,
  ARM_PROCESSOR_MODE_SUPERVISOR = 0x13,
  ARM_PROCESSOR_MODE_ABORT      = 0x17,
  ARM_PROCESSOR_MODE_HYP        = 0x1A,
  ARM_PROCESSOR_MODE_UNDEFINED  = 0x1B,
  ARM_PROCESSOR_MODE_SYSTEM     = 0x1F,
  ARM_PROCESSOR_MODE_MASK       = 0x1F
} ARM_PROCESSOR_MODE;

//
// ARM Cpu IDs
//
#define ARM_CPU_IMPLEMENTER_MASK          (0xFFU << 24)
#define ARM_CPU_IMPLEMENTER_ARMLTD        (0x41U << 24)
#define ARM_CPU_IMPLEMENTER_DEC           (0x44U << 24)
#define ARM_CPU_IMPLEMENTER_MOT           (0x4DU << 24)
#define ARM_CPU_IMPLEMENTER_QUALCOMM      (0x51U << 24)
#define ARM_CPU_IMPLEMENTER_MARVELL       (0x56U << 24)

#define ARM_CPU_PRIMARY_PART_MASK         (0xFFF << 4)
#define ARM_CPU_PRIMARY_PART_CORTEXA5     (0xC05 << 4)
#define ARM_CPU_PRIMARY_PART_CORTEXA7     (0xC07 << 4)
#define ARM_CPU_PRIMARY_PART_CORTEXA8     (0xC08 << 4)
#define ARM_CPU_PRIMARY_PART_CORTEXA9     (0xC09 << 4)
#define ARM_CPU_PRIMARY_PART_CORTEXA15    (0xC0F << 4)

//
// ARM MP Core IDs
//
#define ARM_CORE_AFF0         0xFF
#define ARM_CORE_AFF1         (0xFF << 8)
#define ARM_CORE_AFF2         (0xFF << 16)
#define ARM_CORE_AFF3         (0xFFULL << 32)

#define ARM_CORE_MASK         ARM_CORE_AFF0
#define ARM_CLUSTER_MASK      ARM_CORE_AFF1
#define GET_CORE_ID(MpId)     ((MpId) & ARM_CORE_MASK)
#define GET_CLUSTER_ID(MpId)  (((MpId) & ARM_CLUSTER_MASK) >> 8)
#define GET_MPID(ClusterId, CoreId)   (((ClusterId) << 8) | (CoreId))
#define PRIMARY_CORE_ID       (PcdGet32(PcdArmPrimaryCore) & ARM_CORE_MASK)

ARM_CACHE_TYPE
EFIAPI
ArmCacheType (
  VOID
  );

ARM_CACHE_ARCHITECTURE
EFIAPI
ArmCacheArchitecture (
  VOID
  );

VOID
EFIAPI
ArmCacheInformation (
  OUT ARM_CACHE_INFO  *CacheInfo
  );

BOOLEAN
EFIAPI
ArmDataCachePresent (
  VOID
  );

UINTN
EFIAPI
ArmDataCacheSize (
  VOID
  );

UINTN
EFIAPI
ArmDataCacheAssociativity (
  VOID
  );

UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  );

BOOLEAN
EFIAPI
ArmInstructionCachePresent (
  VOID
  );

UINTN
EFIAPI
ArmInstructionCacheSize (
  VOID
  );

UINTN
EFIAPI
ArmInstructionCacheAssociativity (
  VOID
  );

UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  );

UINTN
EFIAPI
ArmIsArchTimerImplemented (
  VOID
  );

UINTN
EFIAPI
ArmReadIdPfr0 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdPfr1 (
  VOID
  );

UINTN
EFIAPI
ArmCacheInfo (
  VOID
  );

BOOLEAN
EFIAPI
ArmIsMpCore (
  VOID
  );

VOID
EFIAPI
ArmInvalidateDataCache (
  VOID
  );


VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  );

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  );

VOID
EFIAPI
ArmCleanDataCacheToPoU (
  VOID
  );

VOID
EFIAPI
ArmInvalidateInstructionCache (
  VOID
  );

VOID
EFIAPI
ArmInvalidateDataCacheEntryByMVA (
  IN  UINTN   Address
  );

VOID
EFIAPI
ArmCleanDataCacheEntryByMVA (
  IN  UINTN   Address
  );

VOID
EFIAPI
ArmCleanInvalidateDataCacheEntryByMVA (
  IN  UINTN   Address
  );

VOID
EFIAPI
ArmInvalidateDataCacheEntryBySetWay (
  IN  UINTN  SetWayFormat
  );

VOID
EFIAPI
ArmCleanDataCacheEntryBySetWay (
  IN  UINTN  SetWayFormat
  );

VOID
EFIAPI
ArmCleanInvalidateDataCacheEntryBySetWay (
  IN  UINTN   SetWayFormat
  );

VOID
EFIAPI
ArmEnableDataCache (
  VOID
  );

VOID
EFIAPI
ArmDisableDataCache (
  VOID
  );

VOID
EFIAPI
ArmEnableInstructionCache (
  VOID
  );

VOID
EFIAPI
ArmDisableInstructionCache (
  VOID
  );

VOID
EFIAPI
ArmEnableMmu (
  VOID
  );

VOID
EFIAPI
ArmDisableMmu (
  VOID
  );

VOID
EFIAPI
ArmEnableCachesAndMmu (
  VOID
  );

VOID
EFIAPI
ArmDisableCachesAndMmu (
  VOID
  );

VOID
EFIAPI
ArmEnableInterrupts (
  VOID
  );

UINTN
EFIAPI
ArmDisableInterrupts (
  VOID
  );

BOOLEAN
EFIAPI
ArmGetInterruptState (
  VOID
  );

VOID
EFIAPI
ArmEnableAsynchronousAbort (
  VOID
  );

UINTN
EFIAPI
ArmDisableAsynchronousAbort (
  VOID
  );

VOID
EFIAPI
ArmEnableIrq (
  VOID
  );

UINTN
EFIAPI
ArmDisableIrq (
  VOID
  );

VOID
EFIAPI
ArmEnableFiq (
  VOID
  );

UINTN
EFIAPI
ArmDisableFiq (
  VOID
  );

BOOLEAN
EFIAPI
ArmGetFiqState (
  VOID
  );

/**
 * Invalidate Data and Instruction TLBs
 */
VOID
EFIAPI
ArmInvalidateTlb (
  VOID
  );

VOID
EFIAPI
ArmUpdateTranslationTableEntry (
  IN  VOID     *TranslationTableEntry,
  IN  VOID     *Mva
  );

VOID
EFIAPI
ArmSetDomainAccessControl (
  IN  UINT32  Domain
  );

VOID
EFIAPI
ArmSetTTBR0 (
  IN  VOID  *TranslationTableBase
  );

VOID *
EFIAPI
ArmGetTTBR0BaseAddress (
  VOID
  );

RETURN_STATUS
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                         **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize  OPTIONAL
  );

BOOLEAN
EFIAPI
ArmMmuEnabled (
  VOID
  );

VOID
EFIAPI
ArmEnableBranchPrediction (
  VOID
  );

VOID
EFIAPI
ArmDisableBranchPrediction (
  VOID
  );

VOID
EFIAPI
ArmSetLowVectors (
  VOID
  );

VOID
EFIAPI
ArmSetHighVectors (
  VOID
  );

VOID
EFIAPI
ArmDrainWriteBuffer (
  VOID
  );

VOID
EFIAPI
ArmDataMemoryBarrier (
  VOID
  );

VOID
EFIAPI
ArmDataSyncronizationBarrier (
  VOID
  );

VOID
EFIAPI
ArmInstructionSynchronizationBarrier (
  VOID
  );

VOID
EFIAPI
ArmWriteVBar (
  IN  UINTN   VectorBase
  );

UINTN
EFIAPI
ArmReadVBar (
  VOID
  );

VOID
EFIAPI
ArmWriteAuxCr (
  IN  UINT32    Bit
  );

UINT32
EFIAPI
ArmReadAuxCr (
  VOID
  );

VOID
EFIAPI
ArmSetAuxCrBit (
  IN  UINT32    Bits
  );

VOID
EFIAPI
ArmUnsetAuxCrBit (
  IN  UINT32    Bits
  );

VOID
EFIAPI
ArmCallSEV (
  VOID
  );

VOID
EFIAPI
ArmCallWFE (
  VOID
  );

VOID
EFIAPI
ArmCallWFI (

  VOID
  );

UINTN
EFIAPI
ArmReadMpidr (
  VOID
  );

UINTN
EFIAPI
ArmReadMidr (
  VOID
  );

UINT32
EFIAPI
ArmReadCpacr (
  VOID
  );

VOID
EFIAPI
ArmWriteCpacr (
  IN  UINT32   Access
  );

VOID
EFIAPI
ArmEnableVFP (
  VOID
  );

/**
  Get the Secure Configuration Register value

  @return   Value read from the Secure Configuration Register

**/
UINT32
EFIAPI
ArmReadScr (
  VOID
  );

/**
  Set the Secure Configuration Register

  @param Value   Value to write to the Secure Configuration Register

**/
VOID
EFIAPI
ArmWriteScr (
  IN  UINT32   Value
  );

UINT32
EFIAPI
ArmReadMVBar (
  VOID
  );

VOID
EFIAPI
ArmWriteMVBar (
  IN  UINT32   VectorMonitorBase
  );

UINT32
EFIAPI
ArmReadSctlr (
  VOID
  );

UINTN
EFIAPI
ArmReadHVBar (
  VOID
  );

VOID
EFIAPI
ArmWriteHVBar (
  IN  UINTN   HypModeVectorBase
  );


//
// Helper functions for accessing CPU ACTLR
//

UINTN
EFIAPI
ArmReadCpuActlr (
  VOID
  );

VOID
EFIAPI
ArmWriteCpuActlr (
  IN  UINTN Val
  );

VOID
EFIAPI
ArmSetCpuActlrBit (
  IN  UINTN    Bits
  );

VOID
EFIAPI
ArmUnsetCpuActlrBit (
  IN  UINTN    Bits
  );

#endif // __ARM_LIB__

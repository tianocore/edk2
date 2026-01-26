/** @file
  Entry point to the Standalone MM Foundation on RiscV platform.

Copyright (c) 2025, Rivos Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __STANDALONEMMCORE_ENTRY_POINT_H__
#define __STANDALONEMMCORE_ENTRY_POINT_H__

#include <Uefi.h>
#include <Library/PeCoffLib.h>
#include <Library/FvLib.h>

typedef struct {
  UINT32    ProcessorId;
  UINT32    Package;
  UINT32    Core;
  UINT32    Flags;
} EFI_RISCV_SMM_CPU_INFO;

typedef struct {
  UINT64                    MmMemBase;
  UINT64                    MmMemLimit;
  UINT64                    MmImageBase;
  UINT64                    MmStackBase;
  UINT64                    MmHeapBase;
  UINT64                    MmNsCommBufBase;
  UINT64                    MmImageSize;
  UINT64                    MmPcpuStackSize;
  UINT64                    MmHeapSize;
  UINT64                    MmNsCommBufSize;
  UINT32                    NumMmMemRegions;
  UINT32                    NumCpus;
  UINT32                    MpxyChannelId;
  EFI_RISCV_SMM_CPU_INFO    CpuInfo;
} EFI_RISCV_SMM_PAYLOAD_INFO;

#define BOOT_INFO_STACK_BASE_OFFSET  24   // Used in assembly
#define CPU_INFO_FLAG_PRIMARY_CPU    0x00000001
STATIC_ASSERT (BOOT_INFO_STACK_BASE_OFFSET == OFFSET_OF (EFI_RISCV_SMM_PAYLOAD_INFO, MmStackBase));

VOID *
EFIAPI
CreateHobListFromBootInfo (
  IN       EFI_RISCV_SMM_PAYLOAD_INFO   *PayloadBootInfo
  );

/**
  The entry point of Standalone MM Foundation.

  @param  [in]  CpuId             The Id assigned to this running CPU
  @param  [in]  PayloadInfoAddress   The address of payload info

**/
VOID
EFIAPI
CModuleEntryPoint (
  IN UINT64  CpuId,
  IN VOID    *PayloadInfoAddress
  );

/**

  @param  HobStart  Pointer to the beginning of the HOB List passed in from the PEI Phase.

**/
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID  *HobStart
  );

#endif

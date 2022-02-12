/** @file
  PlatformInitLib header file.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_INIT_LIB_H_
#define PLATFORM_INIT_LIB_H_

#include <PiPei.h>

#pragma pack(1)
typedef struct {
  EFI_HOB_GUID_TYPE    GuidHeader;
  UINT16               HostBridgePciDevId;

  BOOLEAN              PcdSetNxForStack;
  UINT64               PcdConfidentialComputingGuestAttr;
  UINT64               PcdTdxSharedBitMask;

  UINT64               PcdPciMmio64Base;
  UINT64               PcdPciMmio64Size;
  UINT32               PcdPciMmio32Base;
  UINT32               PcdPciMmio32Size;
  UINT64               PcdPciIoBase;
  UINT64               PcdPciIoSize;

  UINT64               PcdEmuVariableNvStoreReserved;
  UINT32               PcdCpuBootLogicalProcessorNumber;
  UINT32               PcdCpuMaxLogicalProcessorNumber;
} EFI_HOB_PLATFORM_INFO;
#pragma pack()

/**
  Reads 8-bits of CMOS data.

  Reads the 8-bits of CMOS data at the location specified by Index.
  The 8-bit read value is returned.

  @param  Index  The CMOS location to read.

  @return The value read.

**/
UINT8
EFIAPI
PlatformCmosRead8 (
  IN      UINTN  Index
  );

/**
  Writes 8-bits of CMOS data.

  Writes 8-bits of CMOS data to the location specified by Index
  with the value specified by Value and returns Value.

  @param  Index  The CMOS location to write.
  @param  Value  The value to write to CMOS.

  @return The value written to CMOS.

**/
UINT8
EFIAPI
PlatformCmosWrite8 (
  IN      UINTN  Index,
  IN      UINT8  Value
  );

/**
   Dump the CMOS content
 */
VOID
EFIAPI
PlatformDebugDumpCmos (
  VOID
  );

VOID
EFIAPI
PlatformAddIoMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  );

VOID
EFIAPI
PlatformAddIoMemoryRangeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

VOID
EFIAPI
PlatformAddMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  );

VOID
EFIAPI
PlatformAddMemoryRangeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

VOID
EFIAPI
PlatformAddReservedMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize,
  IN BOOLEAN               Cacheable
  );

#endif // PLATFORM_INIT_LIB_H_

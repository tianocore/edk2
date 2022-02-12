/** @file
  PlatformInitLib header file.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_INIT_LIB_H_
#define PLATFORM_INIT_LIB_H_

#include <PiPei.h>
#include <Guid/MemoryTypeInformation.h>

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

/**
 * Return the highest address that DXE could possibly use, plus one.
 *
 * @param Pci64Base   The 64-bit PCI host aperture base address.
 * @param Pci64Size   The 64-bit PCI host aperture size.
 * @param DefaultPciMmio64Size  The default 64-bit PCI host aperture size.
 *
 * @return  The highest address that DXE could possibly use, plus one.
 */
UINT64
EFIAPI
PlatformGetFirstNonAddress (
  OUT UINT64  *Pci64Base,
  OUT UINT64  *Pci64Size,
  IN  UINT64  DefaultPciMmio64Size
  );

/**
 * Initialize the PhysMemAddressWidth variable, based on guest RAM size.
 *
 * @param   FirstNonAddress   The highest address that DXE could possibly use, plus one.
 *
 * @return  The physical memory address width based on the guest RAM size.
 */
UINT8
EFIAPI
PlatformAddressWidthInitialization (
  IN UINT64  FirstNonAddress
  );

/**
 * Get the memory size below 4GB.
 *
 * @return UINT32 The lower memory size.
 */
UINT32
EFIAPI
PlatformGetSystemMemorySizeBelow4gb (
  VOID
  );

/**
 * Initializatoin of Qemu UC32Base.
 *
 * @param HostBridgeDevId   The host bridge Dev Id.
 * @param LowerMemorySize   The lower memory size (under 4G).
 * @return UINT32   The Qemu UC32 base address.
 */
UINT32
EFIAPI
PlatformQemuUc32BaseInitialization (
  IN UINT16  HostBridgeDevId,
  IN UINT32  LowerMemorySize
  );

/**
  Publish system RAM and reserve memory regions.

  @param  Uc32Base
  @param  HostBridgeDevId
  @param  SmmSmramRequire
  @param  BootMode
  @param  S3Supported
  @param  LowerMemorySize
  @param  Q35TsegMbytes
**/
VOID
EFIAPI
PlatformInitializeRamRegions (
  IN UINT32         Uc32Base,
  IN UINT16         HostBridgeDevId,
  IN BOOLEAN        SmmSmramRequire,
  IN EFI_BOOT_MODE  BootMode,
  IN BOOLEAN        S3Supported,
  IN UINT32         LowerMemorySize,
  IN UINT16         Q35TsegMbytes
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

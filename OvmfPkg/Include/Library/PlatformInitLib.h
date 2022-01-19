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
 * Query Host Bridge Dev Id.
 *
 * @return Host Bridge Dev Id.
 */
UINT16
EFIAPI
PlatformQueryHostBridgeDid (
  VOID
  );

/**
  Fetch the boot CPU count and the possible CPU count from QEMU.

  @param  HostBridgeDevId     The Host bridge Dev Id.
  @param  DefaultMaxCpuCount  The default max cpu count.
  @param  MaxCpuCount         The pointer to the returned max cpu count.
  @param  BootCpuCount        The pointer to the returned boot cpu count.
**/
VOID
EFIAPI
PlatformMaxCpuCountInitialization (
  IN  UINT16  HostBridgeDevId,
  IN  UINT32  DefaultMaxCpuCount,
  OUT UINT32  *MaxCpuCount,
  OUT UINT16  *BootCpuCount
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
 * Initialize the Memory Map IO hobs.
 *
 * @param HostBridgeDevId The host bridge Dev Id.
 * @param Uc32Base        The Qemu Uc32Base address.
 * @param PciBase         The pointer to the Pci base address.
 * @param PciSize         The pointer to the Pci base size.
 * @param PciIoBase       The pointer to the Pci Io base address.
 * @param PciIoSize       The pointer to the Pci Io size.
 */
VOID
EFIAPI
PlatformMemMapInitialization (
  IN UINT16   HostBridgeDevId,
  IN UINT32   Uc32Base,
  OUT UINT32  *PciBase,
  OUT UINT32  *PciSize,
  OUT UINT64  *PciIoBase,
  OUT UINT64  *PciIoSize
  );

/**
 * Fetch "opt/ovmf/PcdSetNxForStack" from QEMU
 *
 * @param Setting     The pointer to the setting of "/opt/ovmf/PcdSetNxForStack".
 * @return EFI_SUCCESS  Successfully fetch the settings.
 */
EFI_STATUS
EFIAPI
PlatformNoexecDxeInitialization (
  OUT BOOLEAN  *Setting
  );

/**
 * Misc initialization, such as Disable A20 Mask, Build CPU Hob,
 * PM settings, Set PCI Express Register Range Base Address.
 *
 * @param HostBridgeDevId   The host bridge Dev id.
 * @param PhysMemAddressWidth The physical memory address width.
 */
VOID
EFIAPI
PlatformMiscInitialization (
  IN UINT16  HostBridgeDevId,
  IN UINT8   PhysMemAddressWidth
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
PlatformTdxPublishRamRegions (
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

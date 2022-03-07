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
  UINT16               HostBridgeDevId;

  UINT64               PcdConfidentialComputingGuestAttr;
  BOOLEAN              SevEsIsEnabled;

  UINT32               BootMode;
  BOOLEAN              S3Supported;

  BOOLEAN              SmmSmramRequire;
  BOOLEAN              Q35SmramAtDefaultSmbase;
  UINT16               Q35TsegMbytes;

  UINT64               FirstNonAddress;
  UINT8                PhysMemAddressWidth;
  UINT32               Uc32Base;
  UINT32               Uc32Size;

  BOOLEAN              PcdSetNxForStack;
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
  UINT32               DefaultMaxCpuNumber;
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

/**
 * @brief
 *
 * @param MemoryBase
 * @param MemorySize
 * @return VOID
 */
VOID
EFIAPI
PlatformAddIoMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  );

/**
 * @brief
 *
 * @param MemoryBase
 * @param MemoryLimit
 * @return VOID
 */
VOID
EFIAPI
PlatformAddIoMemoryRangeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

/**
 * @brief
 *
 * @param MemoryBase
 * @param MemorySize
 * @return VOID
 */
VOID
EFIAPI
PlatformAddMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  );

/**
 * @brief
 *
 * @param MemoryBase
 * @param MemoryLimit
 * @return VOID
 */
VOID
EFIAPI
PlatformAddMemoryRangeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

/**
 * @brief
 *
 * @param MemoryBase
 * @param MemorySize
 * @param Cacheable
 * @return VOID
 */
VOID
EFIAPI
PlatformAddReservedMemoryBaseSizeHob (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize,
  IN BOOLEAN               Cacheable
  );

/**
 * @brief
 *
 * @param PlatformInfoHob
 * @return VOID
 */
VOID
EFIAPI
PlatformQemuUc32BaseInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * @brief
 *
 * @param PlatformInfoHob
 * @return UINT32
 */
UINT32
EFIAPI
PlatformGetSystemMemorySizeBelow4gb (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * @brief Initialize the mPhysMemAddressWidth variable, based on guest RAM size.
 *
 * @param PlatformInfoHob
 * @return VOID
 */
VOID
EFIAPI
PlatformAddressWidthInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * @brief Peform Memory Detection for QEMU / KVM
 *
 * @param PlatformInfoHob
 * @return VOID
 */
VOID
EFIAPI
PlatformQemuInitializeRam (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * @brief
 *
 * @param PlatformInfoHob
 * @return VOID
 */
VOID
EFIAPI
PlatformQemuInitializeRamForNotS3Resume (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * @brief
 *
 * @param PlatformInfoHob
 * @return VOID
 */
VOID
EFIAPI
PlatformMemMapInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * @brief Fetch "opt/ovmf/PcdSetNxForStack" from QEMU
 *
 * @param PlatformInfoHob
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
PlatformNoexecDxeInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * @brief
 *
 * @param PlatformInfoHob
 * @return VOID
 */
VOID
EFIAPI
PlatformMiscInitialization (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
 * Fetch the boot CPU count and the possible CPU count from QEMU, and expose
 * them to UefiCpuPkg modules. Set the mMaxCpuCount variable.
 *
 *
 * @param PlatformInfoHob
 * @return VOID
 */
VOID
EFIAPI
PlatformMaxCpuCountInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

#endif // PLATFORM_INIT_LIB_H_

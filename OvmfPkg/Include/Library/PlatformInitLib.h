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

  UINT32               S3AcpiReservedMemoryBase;
  UINT32               S3AcpiReservedMemorySize;
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

VOID
EFIAPI
PlatformQemuUc32BaseInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

UINT32
EFIAPI
PlatformGetSystemMemorySizeBelow4gb (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
  Initialize the PhysMemAddressWidth field in PlatformInfoHob based on guest RAM size.
**/
VOID
EFIAPI
PlatformAddressWidthInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
  Peform Memory Detection for QEMU / KVM

**/
VOID
EFIAPI
PlatformQemuInitializeRam (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

VOID
EFIAPI
PlatformQemuInitializeRamForS3 (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

VOID
EFIAPI
PlatformMemMapInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
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
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

VOID
EFIAPI
PlatformMiscInitialization (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
  Fetch the boot CPU count and the possible CPU count from QEMU, and expose
  them to UefiCpuPkg modules.
**/
VOID
EFIAPI
PlatformMaxCpuCountInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
  In Tdx guest, some information need to be passed from host VMM to guest
  firmware. For example, the memory resource, etc. These information are
  prepared by host VMM and put in HobList which is described in TdxMetadata.

  Information in HobList is treated as external input. From the security
  perspective before it is consumed, it should be validated.

  @retval   EFI_SUCCESS   Successfully process the hoblist
  @retval   Others        Other error as indicated
**/
EFI_STATUS
EFIAPI
ProcessTdxHobList (
  VOID
  );

/**
  In Tdx guest, the system memory is passed in TdHob by host VMM. So
  the major task of PlatformTdxPublishRamRegions is to walk thru the
  TdHob list and transfer the ResourceDescriptorHob and MemoryAllocationHob
  to the hobs in DXE phase.

  MemoryAllocationHob should also be created for Mailbox and Ovmf work area.
**/
VOID
EFIAPI
PlatformTdxPublishRamRegions (
  VOID
  );

#endif // PLATFORM_INIT_LIB_H_

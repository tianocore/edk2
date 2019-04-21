/** @file
  Migrates SEC structures after permanent memory is installed.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SEC_MIGRATION_H__
#define __SEC_MIGRATION_H__

#include <Base.h>

#include <Pi/PiPeiCis.h>
#include <Ppi/RepublishSecPpi.h>
#include <Ppi/SecPerformance.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SecPlatformInformation2.h>
#include <Ppi/TemporaryRamDone.h>
#include <Ppi/TemporaryRamSupport.h>

/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param[in]     PeiServices               Pointer to the PEI Services Table.
  @param[in,out] StructureSize             Pointer to the variable describing size of the input buffer.
  @param[out]    PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_NOT_FOUND         Can't found the HOB created by SecMigrationPei component.
  @retval EFI_BUFFER_TOO_SMALL  The size of buffer pointed by StructureSize is too small and will return
                                the minimal required size in the buffer pointed by StructureSize.
  @retval EFI_INVALID_PARAMETER The StructureSize is NULL or PlatformInformationRecord is NULL.

**/
EFI_STATUS
EFIAPI
SecPlatformInformationPostMemory (
  IN CONST EFI_PEI_SERVICES                     **PeiServices,
  IN OUT   UINT64                               *StructureSize,
     OUT   EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  );

/**
  Re-installs the SEC Platform Information PPIs to implementation in this module to support post-memory.

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The SEC Platform Information PPI could not be re-installed.
  @return Others               An error occurred during PPI re-install.

**/
EFI_STATUS
EFIAPI
SecPlatformInformationPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );

/**
  This interface re-installs PPIs installed in SecCore from a post-memory PEIM.

  This is to allow a platform that may not support relocation of SecCore to update the PPI instance to a post-memory
  copy from a PEIM that has been shadowed to permanent memory.

  @retval EFI_SUCCESS    The SecCore PPIs were re-installed successfully.
  @retval Others         An error occurred re-installing the SecCore PPIs.

**/
EFI_STATUS
EFIAPI
RepublishSecPpis (
  VOID
  );

/**
  Disables the use of Temporary RAM.

  If present, this service is invoked by the PEI Foundation after
  the EFI_PEI_PERMANANT_MEMORY_INSTALLED_PPI is installed.

  @retval EFI_SUCCESS  Dummy function, alway return this value.

**/
EFI_STATUS
EFIAPI
SecTemporaryRamDonePostMemory (
  VOID
  );

/**
  This service of the EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI that migrates temporary RAM into
  permanent memory.

  @param PeiServices            Pointer to the PEI Services Table.
  @param TemporaryMemoryBase    Source Address in temporary memory from which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param PermanentMemoryBase    Destination Address in permanent memory into which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param CopySize               Amount of memory to migrate from temporary to permanent memory.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_INVALID_PARAMETER PermanentMemoryBase + CopySize > TemporaryMemoryBase when
                                TemporaryMemoryBase > PermanentMemoryBase.

**/
EFI_STATUS
EFIAPI
SecTemporaryRamSupportPostMemory (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

/**
  This interface conveys performance information out of the Security (SEC) phase into PEI.

  This service is published by the SEC phase. The SEC phase handoff has an optional
  EFI_PEI_PPI_DESCRIPTOR list as its final argument when control is passed from SEC into the
  PEI Foundation. As such, if the platform supports collecting performance data in SEC,
  this information is encapsulated into the data structure abstracted by this service.
  This information is collected for the boot-strap processor (BSP) on IA-32.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of the PEI_SEC_PERFORMANCE_PPI.
  @param[out] Performance  The pointer to performance data collected in SEC phase.

  @retval EFI_SUCCESS           The performance data was successfully returned.
  @retval EFI_INVALID_PARAMETER The This or Performance is NULL.
  @retval EFI_NOT_FOUND         Can't found the HOB created by the SecMigrationPei component.

**/
EFI_STATUS
EFIAPI
GetPerformancePostMemory (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN       PEI_SEC_PERFORMANCE_PPI   *This,
  OUT      FIRMWARE_SEC_PERFORMANCE  *Performance
  );

typedef struct {
  UINT64                                StructureSize;
  EFI_SEC_PLATFORM_INFORMATION_RECORD   *PlatformInformationRecord;
} SEC_PLATFORM_INFORMATION_CONTEXT;

typedef struct {
  EFI_HOB_GUID_TYPE                     Header;
  UINT8                                 Revision;
  UINT8                                 Reserved[3];
  FIRMWARE_SEC_PERFORMANCE              FirmwareSecPerformance;
  SEC_PLATFORM_INFORMATION_CONTEXT      Context;
} SEC_PLATFORM_INFORMATION_CONTEXT_HOB;

#endif

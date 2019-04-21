/** @file
  Migrates SEC structures after permanent memory is installed.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>

#include "SecMigrationPei.h"

STATIC REPUBLISH_SEC_PPI_PPI  mEdkiiRepublishSecPpiPpi = {
                                RepublishSecPpis
                                };

GLOBAL_REMOVE_IF_UNREFERENCED EFI_SEC_PLATFORM_INFORMATION_PPI  mSecPlatformInformationPostMemoryPpi = {
                                                                  SecPlatformInformationPostMemory
                                                                  };


GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_TEMPORARY_RAM_DONE_PPI mSecTemporaryRamDonePostMemoryPpi = {
                                                               SecTemporaryRamDonePostMemory
                                                               };

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI mSecTemporaryRamSupportPostMemoryPpi = {
                                                                  SecTemporaryRamSupportPostMemory
                                                                  };

GLOBAL_REMOVE_IF_UNREFERENCED PEI_SEC_PERFORMANCE_PPI mSecPerformancePpi = {
                                                        GetPerformancePostMemory
                                                        };

STATIC EFI_PEI_PPI_DESCRIPTOR mEdkiiRepublishSecPpiDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gRepublishSecPpiPpiGuid,
  &mEdkiiRepublishSecPpiPpi
  };

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR mSecPlatformInformationPostMemoryDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiSecPlatformInformationPpiGuid,
  &mSecPlatformInformationPostMemoryPpi
  };

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR mSecTemporaryRamDonePostMemoryDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTemporaryRamDonePpiGuid,
  &mSecTemporaryRamDonePostMemoryPpi
  };

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR mSecTemporaryRamSupportPostMemoryDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTemporaryRamSupportPpiGuid,
  &mSecTemporaryRamSupportPostMemoryPpi
  };

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR mSecPerformancePpiDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiSecPerformancePpiGuid,
  &mSecPerformancePpi
  };

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
  )
{
  //
  // Temporary RAM Done is already done in post-memory
  // install a stub function that is located in permanent memory
  //
  return EFI_SUCCESS;
}

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
  )
{
  //
  // Temporary RAM Support is already done in post-memory
  // install a stub function that is located in permanent memory
  //
  return EFI_SUCCESS;
}

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
  )
{
  SEC_PLATFORM_INFORMATION_CONTEXT_HOB  *SecPlatformInformationContexHob;

  if (This == NULL || Performance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SecPlatformInformationContexHob = GetFirstGuidHob (&gEfiCallerIdGuid);
  if (SecPlatformInformationContexHob == NULL) {
    return EFI_NOT_FOUND;
  }

  Performance->ResetEnd = SecPlatformInformationContexHob->FirmwareSecPerformance.ResetEnd;

  return EFI_SUCCESS;
}

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
  )
{
  SEC_PLATFORM_INFORMATION_CONTEXT_HOB  *SecPlatformInformationContexHob;

  if (StructureSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SecPlatformInformationContexHob = GetFirstGuidHob (&gEfiCallerIdGuid);
  if (SecPlatformInformationContexHob == NULL) {
    return EFI_NOT_FOUND;
  }

  if (*StructureSize < SecPlatformInformationContexHob->Context.StructureSize) {
    *StructureSize = SecPlatformInformationContexHob->Context.StructureSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (PlatformInformationRecord == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *StructureSize = SecPlatformInformationContexHob->Context.StructureSize;
  CopyMem (
    (VOID *) PlatformInformationRecord,
    (VOID *) SecPlatformInformationContexHob->Context.PlatformInformationRecord,
    (UINTN) SecPlatformInformationContexHob->Context.StructureSize
    );

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                            Status;
  EFI_PEI_PPI_DESCRIPTOR                *PeiPpiDescriptor;
  VOID                                  *PeiPpi;
  SEC_PLATFORM_INFORMATION_CONTEXT_HOB  *SecPlatformInformationContextHob;
  EFI_SEC_PLATFORM_INFORMATION_RECORD   *SecPlatformInformationPtr;
  UINT64                                SecStructureSize;

  SecPlatformInformationPtr = NULL;
  SecStructureSize = 0;

  Status = PeiServicesLocatePpi (
             &gEfiTemporaryRamDonePpiGuid,
             0,
             &PeiPpiDescriptor,
             (VOID **) &PeiPpi
             );
  if (!EFI_ERROR (Status)) {
    Status = PeiServicesReInstallPpi (
               PeiPpiDescriptor,
               &mSecTemporaryRamDonePostMemoryDescriptor
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = PeiServicesLocatePpi (
             &gEfiTemporaryRamSupportPpiGuid,
             0,
             &PeiPpiDescriptor,
             (VOID **) &PeiPpi
             );
  if (!EFI_ERROR (Status)) {
    Status = PeiServicesReInstallPpi (
               PeiPpiDescriptor,
               &mSecTemporaryRamSupportPostMemoryDescriptor
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = PeiServicesCreateHob (
             EFI_HOB_TYPE_GUID_EXTENSION,
             sizeof (SEC_PLATFORM_INFORMATION_CONTEXT_HOB),
             (VOID **) &SecPlatformInformationContextHob
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SecPlatformInformation Context HOB could not be created.\n"));
    return Status;
  }

  SecPlatformInformationContextHob->Header.Name = gEfiCallerIdGuid;
  SecPlatformInformationContextHob->Revision    = 1;

  Status = PeiServicesLocatePpi (
             &gPeiSecPerformancePpiGuid,
             0,
             &PeiPpiDescriptor,
             (VOID **) &PeiPpi
             );
  if (!EFI_ERROR (Status)) {
    Status = ((PEI_SEC_PERFORMANCE_PPI *) PeiPpi)->GetPerformance (
                                                     GetPeiServicesTablePointer (),
                                                     (PEI_SEC_PERFORMANCE_PPI *) PeiPpi,
                                                     &SecPlatformInformationContextHob->FirmwareSecPerformance
                                                     );
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      Status = PeiServicesReInstallPpi (
                 PeiPpiDescriptor,
                 &mSecPerformancePpiDescriptor
                 );
      ASSERT_EFI_ERROR (Status);
    }
  }

  Status = PeiServicesLocatePpi (
             &gEfiSecPlatformInformationPpiGuid,
             0,
             &PeiPpiDescriptor,
             (VOID **) &PeiPpi
             );
  if (!EFI_ERROR (Status)) {
    Status = ((EFI_SEC_PLATFORM_INFORMATION_PPI *) PeiPpi)->PlatformInformation (
                                                              GetPeiServicesTablePointer (),
                                                              &SecStructureSize,
                                                              SecPlatformInformationPtr
                                                              );
    ASSERT (Status == EFI_BUFFER_TOO_SMALL);
    if (Status != EFI_BUFFER_TOO_SMALL) {
      return EFI_NOT_FOUND;
    }

    ZeroMem ((VOID *) &(SecPlatformInformationContextHob->Context), sizeof (SEC_PLATFORM_INFORMATION_CONTEXT));
    SecPlatformInformationContextHob->Context.PlatformInformationRecord = AllocatePool ((UINTN) SecStructureSize);
    ASSERT (SecPlatformInformationContextHob->Context.PlatformInformationRecord != NULL);
    if (SecPlatformInformationContextHob->Context.PlatformInformationRecord == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    SecPlatformInformationContextHob->Context.StructureSize = SecStructureSize;

    Status = ((EFI_SEC_PLATFORM_INFORMATION_PPI *) PeiPpi)->PlatformInformation (
                                                              GetPeiServicesTablePointer (),
                                                              &(SecPlatformInformationContextHob->Context.StructureSize),
                                                              SecPlatformInformationContextHob->Context.PlatformInformationRecord
                                                              );
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      Status = PeiServicesReInstallPpi (
                 PeiPpiDescriptor,
                 &mSecPlatformInformationPostMemoryDescriptor
                 );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}

/**
  This function is the entry point which installs an instance of REPUBLISH_SEC_PPI_PPI.

  It install the RepublishSecPpi depent on PcdMigrateTemporaryRamFirmwareVolumes, install
  the PPI when the PcdMigrateTemporaryRamFirmwareVolumes enabled.

  @param[in]  FileHandle   Pointer to image file handle.
  @param[in]  PeiServices  Pointer to PEI Services Table

  @retval EFI_ABORTED  Disable evacuate temporary memory feature by disable
                       PcdMigrateTemporaryRamFirmwareVolumes.
  @retval EFI_SUCCESS  An instance of REPUBLISH_SEC_PPI_PPI was installed successfully.
  @retval Others       An error occurred installing and instance of REPUBLISH_SEC_PPI_PPI.

**/
EFI_STATUS
EFIAPI
SecMigrationPeiInitialize (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = EFI_ABORTED;

  if (PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes)) {
    Status = PeiServicesInstallPpi (&mEdkiiRepublishSecPpiDescriptor);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

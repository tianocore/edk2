/** @file
  Sample to provide FSP wrapper platform sec related function.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SecPerformance.h>

#include <Library/LocalApicLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param[in]     PeiServices               Pointer to the PEI Services Table.
  @param[in,out] StructureSize             Pointer to the variable describing size of the input buffer.
  @param[out]    PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation (
  IN CONST EFI_PEI_SERVICES                  **PeiServices,
  IN OUT   UINT64                            *StructureSize,
  OUT   EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
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

  @retval EFI_SUCCESS  The data was successfully returned.

**/
EFI_STATUS
EFIAPI
SecGetPerformance (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN       PEI_SEC_PERFORMANCE_PPI   *This,
  OUT      FIRMWARE_SEC_PERFORMANCE  *Performance
  );

PEI_SEC_PERFORMANCE_PPI  mSecPerformancePpi = {
  SecGetPerformance
};

EFI_PEI_PPI_DESCRIPTOR  mPeiSecPlatformPpi[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gTopOfTemporaryRamPpiGuid,
    NULL // To be patched later.
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gPeiSecPerformancePpiGuid,
    &mSecPerformancePpi
  },
};

/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI core by SEC
  core. Platform developer may modify the SecCoreData passed to PEI Core.
  It returns a platform specific PPI list that platform wishes to pass to PEI core.
  The Generic SEC core module will merge this list to join the final list passed to
  PEI core.

  @param[in,out] SecCoreData           The same parameter as passing to PEI core. It
                                       could be overridden by this function.

  @return The platform specific PPI list to be passed to PEI core or
          NULL if there is no need of such platform specific PPI list.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF  *SecCoreData
  )
{
  EFI_PEI_PPI_DESCRIPTOR  *PpiList;

  DEBUG ((DEBUG_INFO, "SecPlatformMain\n"));

  DEBUG ((DEBUG_INFO, "BootFirmwareVolumeBase - 0x%x\n", SecCoreData->BootFirmwareVolumeBase));
  DEBUG ((DEBUG_INFO, "BootFirmwareVolumeSize - 0x%x\n", SecCoreData->BootFirmwareVolumeSize));
  DEBUG ((DEBUG_INFO, "TemporaryRamBase       - 0x%x\n", SecCoreData->TemporaryRamBase));
  DEBUG ((DEBUG_INFO, "TemporaryRamSize       - 0x%x\n", SecCoreData->TemporaryRamSize));
  DEBUG ((DEBUG_INFO, "PeiTemporaryRamBase    - 0x%x\n", SecCoreData->PeiTemporaryRamBase));
  DEBUG ((DEBUG_INFO, "PeiTemporaryRamSize    - 0x%x\n", SecCoreData->PeiTemporaryRamSize));
  DEBUG ((DEBUG_INFO, "StackBase              - 0x%x\n", SecCoreData->StackBase));
  DEBUG ((DEBUG_INFO, "StackSize              - 0x%x\n", SecCoreData->StackSize));

  InitializeApicTimer (0, (UINT32)-1, TRUE, 5);

  //
  // Use middle of Heap as temp buffer, it will be copied by caller.
  // Do not use Stack, because it will cause wrong calculation on stack by PeiCore
  //
  PpiList = (VOID *)((UINTN)SecCoreData->PeiTemporaryRamBase + (UINTN)SecCoreData->PeiTemporaryRamSize/2);
  CopyMem (PpiList, mPeiSecPlatformPpi, sizeof (mPeiSecPlatformPpi));

  //
  // Patch TopOfTemporaryRamPpi
  //
  PpiList[0].Ppi = (VOID *)((UINTN)SecCoreData->TemporaryRamBase + SecCoreData->TemporaryRamSize);

  return PpiList;
}

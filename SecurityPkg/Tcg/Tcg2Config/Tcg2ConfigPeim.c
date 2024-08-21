/** @file
  The module entry point for Tcg2 configuration module.

Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/TpmInstance.h>
#include <Guid/Tcg2AcpiCommunicateBuffer.h>
#include <Guid/TpmNvsMm.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/MmUnblockMemoryLib.h>

#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/TpmInitialized.h>
#include <Protocol/Tcg2Protocol.h>

#include "Tcg2ConfigNvData.h"
#include "Tcg2Internal.h"

TPM_INSTANCE_ID  mTpmInstanceId[] = TPM_INSTANCE_ID_LIST;

CONST EFI_PEI_PPI_DESCRIPTOR  gTpmSelectedPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTpmDeviceSelectedGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mTpmInitializationDonePpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

/**
  This routine check both SetupVariable and real TPM device, and return final TpmDevice configuration.

  @param  SetupTpmDevice  TpmDevice configuration in setup driver

  @return TpmDevice configuration
**/
UINT8
DetectTpmDevice (
  IN UINT8  SetupTpmDevice
  );

/**
  Build gEdkiiTcg2AcpiCommunicateBufferHobGuid.

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  Failed to build Tcg2AcpiCommunicateBuffer Hob.

**/
EFI_STATUS
EFIAPI
BuildTcg2AcpiCommunicateBufferHob (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  TCG2_ACPI_COMMUNICATE_BUFFER  *Tcg2AcpiCommunicateBufferHob;
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          Buffer;
  UINTN                         Pages;

  Pages  = EFI_SIZE_TO_PAGES (sizeof (TCG_NVS));
  Status = PeiServicesAllocatePages (
             EfiACPIMemoryNVS,
             Pages,
             &Buffer
             );
  ASSERT_EFI_ERROR (Status);

  Status = MmUnblockMemoryRequest (Buffer, Pages);
  if ((Status != EFI_UNSUPPORTED) && EFI_ERROR (Status)) {
    return Status;
  }

  Tcg2AcpiCommunicateBufferHob = BuildGuidHob (&gEdkiiTcg2AcpiCommunicateBufferHobGuid, sizeof (TCG2_ACPI_COMMUNICATE_BUFFER));
  ASSERT (Tcg2AcpiCommunicateBufferHob != NULL);
  Tcg2AcpiCommunicateBufferHob->Tcg2AcpiCommunicateBuffer = Buffer;
  Tcg2AcpiCommunicateBufferHob->Pages                     = Pages;

  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mPostMemNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  BuildTcg2AcpiCommunicateBufferHob
};

/**
  The entry point for Tcg2 configuration driver.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            Convert variable to PCD successfully.
  @retval Others                 Fail to convert variable to PCD.
**/
EFI_STATUS
EFIAPI
Tcg2ConfigPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINTN                            Size;
  EFI_STATUS                       Status;
  EFI_STATUS                       Status2;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *VariablePpi;
  TCG2_CONFIGURATION               Tcg2Configuration;
  UINTN                            Index;
  UINT8                            TpmDevice;
  VOID                             *Hob;

  Status = PeiServicesLocatePpi (&gEfiPeiReadOnlyVariable2PpiGuid, 0, NULL, (VOID **)&VariablePpi);
  ASSERT_EFI_ERROR (Status);

  Size   = sizeof (Tcg2Configuration);
  Status = VariablePpi->GetVariable (
                          VariablePpi,
                          TCG2_STORAGE_NAME,
                          &gTcg2ConfigFormSetGuid,
                          NULL,
                          &Size,
                          &Tcg2Configuration
                          );
  if (EFI_ERROR (Status)) {
    //
    // Variable not ready, set default value
    //
    Tcg2Configuration.TpmDevice = TPM_DEVICE_DEFAULT;
  }

  //
  // Validation
  //
  if ((Tcg2Configuration.TpmDevice > TPM_DEVICE_MAX) || (Tcg2Configuration.TpmDevice < TPM_DEVICE_MIN)) {
    Tcg2Configuration.TpmDevice = TPM_DEVICE_DEFAULT;
  }

  //
  // Although we have SetupVariable info, we still need detect TPM device manually.
  //
  DEBUG ((DEBUG_INFO, "Tcg2Configuration.TpmDevice from Setup: %x\n", Tcg2Configuration.TpmDevice));

  if (PcdGetBool (PcdTpmAutoDetection)) {
    TpmDevice = DetectTpmDevice (Tcg2Configuration.TpmDevice);
    DEBUG ((DEBUG_INFO, "TpmDevice final: %x\n", TpmDevice));
    if (TpmDevice != TPM_DEVICE_NULL) {
      Tcg2Configuration.TpmDevice = TpmDevice;
    }
  } else {
    TpmDevice = Tcg2Configuration.TpmDevice;
  }

  //
  // Convert variable to PCD.
  // This is work-around because there is no guarantee DynamicHiiPcd can return correct value in DXE phase.
  // Using DynamicPcd instead.
  //
  // NOTE: Tcg2Configuration variable contains the desired TpmDevice type,
  // while PcdTpmInstanceGuid PCD contains the real detected TpmDevice type
  //
  for (Index = 0; Index < sizeof (mTpmInstanceId)/sizeof (mTpmInstanceId[0]); Index++) {
    if (TpmDevice == mTpmInstanceId[Index].TpmDevice) {
      Size   = sizeof (mTpmInstanceId[Index].TpmInstanceGuid);
      Status = PcdSetPtrS (PcdTpmInstanceGuid, &Size, &mTpmInstanceId[Index].TpmInstanceGuid);
      ASSERT_EFI_ERROR (Status);
      DEBUG ((DEBUG_INFO, "TpmDevice PCD: %g\n", &mTpmInstanceId[Index].TpmInstanceGuid));
      break;
    }
  }

  //
  // Build Hob for PcdTpmInstanceGuid
  //
  Hob = BuildGuidDataHob (
          &gEdkiiTpmInstanceHobGuid,
          (VOID *)PcdGetPtr (PcdTpmInstanceGuid),
          sizeof (EFI_GUID)
          );
  ASSERT (Hob != NULL);

  //
  // Build Hob for PcdTcgPhysicalPresenceInterfaceVer
  //
  Hob = BuildGuidDataHob (
          &gEdkiiTcgPhysicalPresenceInterfaceVerHobGuid,
          (VOID *)PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer),
          AsciiStrSize ((CHAR8 *)PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer))
          );
  ASSERT (Hob != NULL);

  PeiServicesNotifyPpi (&mPostMemNotifyList);

  //
  // Selection done
  //
  Status = PeiServicesInstallPpi (&gTpmSelectedPpi);
  ASSERT_EFI_ERROR (Status);

  //
  // Even if no TPM is selected or detected, we still need install TpmInitializationDonePpi.
  // Because TcgPei or Tcg2Pei will not run, but we still need a way to notify other driver.
  // Other driver can know TPM initialization state by TpmInitializedPpi.
  //
  if (CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid)) {
    Status2 = PeiServicesInstallPpi (&mTpmInitializationDonePpiList);
    ASSERT_EFI_ERROR (Status2);
  }

  return Status;
}

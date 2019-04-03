/** @file
  The module entry point for Tcg2 configuration module.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Tcg2ConfigImpl.h"

extern TPM_INSTANCE_ID  mTpmInstanceId[TPM_DEVICE_MAX + 1];

/**
  Update default PCR banks data.

  @param[in]  HiiPackage        HII Package.
  @param[in]  HiiPackageSize    HII Package size.
  @param[in]  PCRBanks          PCR Banks data.

**/
VOID
UpdateDefaultPCRBanks (
  IN VOID                           *HiiPackage,
  IN UINTN                          HiiPackageSize,
  IN UINT32                         PCRBanks
  )
{
  EFI_HII_PACKAGE_HEADER        *HiiPackageHeader;
  EFI_IFR_OP_HEADER             *IfrOpCodeHeader;
  EFI_IFR_CHECKBOX              *IfrCheckBox;
  EFI_IFR_DEFAULT               *IfrDefault;

  HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *)HiiPackage;

  switch (HiiPackageHeader->Type) {
  case EFI_HII_PACKAGE_FORMS:
    IfrOpCodeHeader = (EFI_IFR_OP_HEADER *)(HiiPackageHeader + 1);
    while ((UINTN)IfrOpCodeHeader < (UINTN)HiiPackageHeader + HiiPackageHeader->Length) {
      switch (IfrOpCodeHeader->OpCode) {
      case EFI_IFR_CHECKBOX_OP:
        IfrCheckBox = (EFI_IFR_CHECKBOX *)IfrOpCodeHeader;
        if ((IfrCheckBox->Question.QuestionId >= KEY_TPM2_PCR_BANKS_REQUEST_0) && (IfrCheckBox->Question.QuestionId <= KEY_TPM2_PCR_BANKS_REQUEST_4)) {
          IfrDefault = (EFI_IFR_DEFAULT *)(IfrCheckBox + 1);
          ASSERT (IfrDefault->Header.OpCode == EFI_IFR_DEFAULT_OP);
          ASSERT (IfrDefault->Type == EFI_IFR_TYPE_BOOLEAN);
          IfrDefault->Value.b = (BOOLEAN)((PCRBanks >> (IfrCheckBox->Question.QuestionId - KEY_TPM2_PCR_BANKS_REQUEST_0)) & 0x1);
        }
        break;
      }
      IfrOpCodeHeader = (EFI_IFR_OP_HEADER *)((UINTN)IfrOpCodeHeader + IfrOpCodeHeader->Length);
    }
    break;
  }
  return ;
}

/**
  Initialize TCG2 version information.

  This function will initialize efi varstore configuration data for
  TCG2_VERSION_NAME variable, check the value of related PCD with
  the variable value and set string for the version state content
  according to the PCD value.

  @param[in] PrivateData    Points to TCG2 configuration private data.

**/
VOID
InitializeTcg2VersionInfo (
  IN TCG2_CONFIG_PRIVATE_DATA   *PrivateData
  )
{
  EFI_STATUS                    Status;
  EFI_STRING                    ConfigRequestHdr;
  BOOLEAN                       ActionFlag;
  TCG2_VERSION                  Tcg2Version;
  UINTN                         DataSize;
  UINT64                        PcdTcg2PpiVersion;
  UINT8                         PcdTpm2AcpiTableRev;

  //
  // Get the PCD value before initializing efi varstore configuration data.
  //
  PcdTcg2PpiVersion = 0;
  CopyMem (
    &PcdTcg2PpiVersion,
    PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer),
    AsciiStrSize ((CHAR8 *) PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer))
    );

  PcdTpm2AcpiTableRev = PcdGet8 (PcdTpm2AcpiTableRev);

  //
  // Initialize efi varstore configuration data.
  //
  ZeroMem (&Tcg2Version, sizeof (Tcg2Version));
  ConfigRequestHdr = HiiConstructConfigHdr (
                       &gTcg2ConfigFormSetGuid,
                       TCG2_VERSION_NAME,
                       PrivateData->DriverHandle
                       );
  ASSERT (ConfigRequestHdr != NULL);
  DataSize = sizeof (Tcg2Version);
  Status = gRT->GetVariable (
                  TCG2_VERSION_NAME,
                  &gTcg2ConfigFormSetGuid,
                  NULL,
                  &DataSize,
                  &Tcg2Version
                  );
  if (!EFI_ERROR (Status)) {
    //
    // EFI variable does exist and validate current setting.
    //
    ActionFlag = HiiValidateSettings (ConfigRequestHdr);
    if (!ActionFlag) {
      //
      // Current configuration is invalid, reset to defaults.
      //
      ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
      ASSERT (ActionFlag);
      //
      // Get the default values from variable.
      //
      DataSize = sizeof (Tcg2Version);
      Status = gRT->GetVariable (
                      TCG2_VERSION_NAME,
                      &gTcg2ConfigFormSetGuid,
                      NULL,
                      &DataSize,
                      &Tcg2Version
                      );
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    //
    // EFI variable doesn't exist or variable size is not expected.
    //

    //
    // Store zero data Buffer Storage to EFI variable.
    //
    Status = gRT->SetVariable (
                    TCG2_VERSION_NAME,
                    &gTcg2ConfigFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (Tcg2Version),
                    &Tcg2Version
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tcg2ConfigDriver: Fail to set TCG2_VERSION_NAME\n"));
      return;
    } else {
      //
      // Build this variable based on default values stored in IFR.
      //
      ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
      ASSERT (ActionFlag);
      //
      // Get the default values from variable.
      //
      DataSize = sizeof (Tcg2Version);
      Status = gRT->GetVariable (
                      TCG2_VERSION_NAME,
                      &gTcg2ConfigFormSetGuid,
                      NULL,
                      &DataSize,
                      &Tcg2Version
                      );
      ASSERT_EFI_ERROR (Status);
      if (PcdTcg2PpiVersion != Tcg2Version.PpiVersion) {
        DEBUG ((DEBUG_WARN, "WARNING: PcdTcgPhysicalPresenceInterfaceVer default value is not same with the default value in VFR\n"));
        DEBUG ((DEBUG_WARN, "WARNING: The default value in VFR has be chosen\n"));
      }
      if (PcdTpm2AcpiTableRev != Tcg2Version.Tpm2AcpiTableRev) {
        DEBUG ((DEBUG_WARN, "WARNING: PcdTpm2AcpiTableRev default value is not same with the default value in VFR\n"));
        DEBUG ((DEBUG_WARN, "WARNING: The default value in VFR has be chosen\n"));
      }
    }
  }
  FreePool (ConfigRequestHdr);

  //
  // Get the PCD value again.
  // If the PCD value is not equal to the value in variable,
  // the PCD is not DynamicHii type and does not map to the setup option.
  //
  PcdTcg2PpiVersion = 0;
  CopyMem (
    &PcdTcg2PpiVersion,
    PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer),
    AsciiStrSize ((CHAR8 *) PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer))
    );
  if (PcdTcg2PpiVersion != Tcg2Version.PpiVersion) {
    DEBUG ((DEBUG_WARN, "WARNING: PcdTcgPhysicalPresenceInterfaceVer is not DynamicHii type and does not map to TCG2_VERSION.PpiVersion\n"));
    DEBUG ((DEBUG_WARN, "WARNING: The TCG2 PPI version configuring from setup page will not work\n"));
  }

  switch (PcdTcg2PpiVersion) {
    case TCG2_PPI_VERSION_1_2:
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_PPI_VERSION_STATE_CONTENT), L"1.2", NULL);
      break;
    case TCG2_PPI_VERSION_1_3:
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TCG2_PPI_VERSION_STATE_CONTENT), L"1.3", NULL);
      break;
    default:
      ASSERT (FALSE);
      break;
  }

  //
  // Get the PcdTpm2AcpiTableRev value again.
  // If the PCD value is not equal to the value in variable,
  // the PCD is not DynamicHii type and does not map to TCG2_VERSION Variable.
  //
  PcdTpm2AcpiTableRev = PcdGet8 (PcdTpm2AcpiTableRev);
  if (PcdTpm2AcpiTableRev != Tcg2Version.Tpm2AcpiTableRev) {
    DEBUG ((DEBUG_WARN, "WARNING: PcdTpm2AcpiTableRev is not DynamicHii type and does not map to TCG2_VERSION.Tpm2AcpiTableRev\n"));
    DEBUG ((DEBUG_WARN, "WARNING: The Tpm2 ACPI Revision configuring from setup page will not work\n"));
  }

  switch (PcdTpm2AcpiTableRev) {
    case EFI_TPM2_ACPI_TABLE_REVISION_3:
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TPM2_ACPI_REVISION_STATE_CONTENT), L"Rev 3", NULL);
      break;
    case EFI_TPM2_ACPI_TABLE_REVISION_4:
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TPM2_ACPI_REVISION_STATE_CONTENT), L"Rev 4", NULL);
      break;
    default:
      ASSERT (FALSE);
      break;
  }
}

/**
  The entry point for Tcg2 configuration driver.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_ALREADY_STARTED    The driver already exists in system.
  @retval EFI_OUT_OF_RESOURCES   Fail to execute entry point due to lack of resources.
  @retval EFI_SUCCES             All the related protocols are installed on the driver.
  @retval Others                 Fail to install protocols as indicated.

**/
EFI_STATUS
EFIAPI
Tcg2ConfigDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                    Status;
  TCG2_CONFIG_PRIVATE_DATA      *PrivateData;
  TCG2_CONFIGURATION            Tcg2Configuration;
  TCG2_DEVICE_DETECTION         Tcg2DeviceDetection;
  UINTN                         Index;
  UINTN                         DataSize;
  EDKII_VARIABLE_LOCK_PROTOCOL  *VariableLockProtocol;
  UINT32                        CurrentActivePCRBanks;

  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  NULL,
                  ImageHandle,
                  ImageHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Create a private data structure.
  //
  PrivateData = AllocateCopyPool (sizeof (TCG2_CONFIG_PRIVATE_DATA), &mTcg2ConfigPrivateDateTemplate);
  ASSERT (PrivateData != NULL);
  mTcg2ConfigPrivateDate = PrivateData;
  //
  // Install private GUID.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &PrivateData->Tcg2Protocol);
  ASSERT_EFI_ERROR (Status);

  PrivateData->ProtocolCapability.Size = sizeof(PrivateData->ProtocolCapability);
  Status = PrivateData->Tcg2Protocol->GetCapability (
                                        PrivateData->Tcg2Protocol,
                                        &PrivateData->ProtocolCapability
                                        );
  ASSERT_EFI_ERROR (Status);

  DataSize = sizeof(Tcg2Configuration);
  Status = gRT->GetVariable (
                  TCG2_STORAGE_NAME,
                  &gTcg2ConfigFormSetGuid,
                  NULL,
                  &DataSize,
                  &Tcg2Configuration
                  );
  if (EFI_ERROR (Status)) {
    //
    // Variable not ready, set default value
    //
    Tcg2Configuration.TpmDevice           = TPM_DEVICE_DEFAULT;
  }

  //
  // Validation
  //
  if ((Tcg2Configuration.TpmDevice > TPM_DEVICE_MAX) || (Tcg2Configuration.TpmDevice < TPM_DEVICE_MIN)) {
    Tcg2Configuration.TpmDevice   = TPM_DEVICE_DEFAULT;
  }

  //
  // Set value for Tcg2CurrentActivePCRBanks
  // Search Tcg2ConfigBin[] and update default value there
  //
  Status = PrivateData->Tcg2Protocol->GetActivePcrBanks (PrivateData->Tcg2Protocol, &CurrentActivePCRBanks);
  ASSERT_EFI_ERROR (Status);
  PrivateData->PCRBanksDesired = CurrentActivePCRBanks;
  UpdateDefaultPCRBanks (Tcg2ConfigBin + sizeof(UINT32), ReadUnaligned32((UINT32 *)Tcg2ConfigBin) - sizeof(UINT32), CurrentActivePCRBanks);

  //
  // Sync data from PCD to variable, so that we do not need detect again in S3 phase.
  //
  Tcg2DeviceDetection.TpmDeviceDetected = TPM_DEVICE_NULL;
  for (Index = 0; Index < sizeof(mTpmInstanceId)/sizeof(mTpmInstanceId[0]); Index++) {
    if (CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &mTpmInstanceId[Index].TpmInstanceGuid)) {
      Tcg2DeviceDetection.TpmDeviceDetected = mTpmInstanceId[Index].TpmDevice;
      break;
    }
  }

  PrivateData->TpmDeviceDetected = Tcg2DeviceDetection.TpmDeviceDetected;
  Tcg2Configuration.TpmDevice = Tcg2DeviceDetection.TpmDeviceDetected;

  //
  // Save to variable so platform driver can get it.
  //
  Status = gRT->SetVariable (
                  TCG2_DEVICE_DETECTION_NAME,
                  &gTcg2ConfigFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof(Tcg2DeviceDetection),
                  &Tcg2DeviceDetection
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Tcg2ConfigDriver: Fail to set TCG2_DEVICE_DETECTION_NAME\n"));
    Status = gRT->SetVariable (
                    TCG2_DEVICE_DETECTION_NAME,
                    &gTcg2ConfigFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Save to variable so platform driver can get it.
  //
  Status = gRT->SetVariable (
                  TCG2_STORAGE_NAME,
                  &gTcg2ConfigFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof(Tcg2Configuration),
                  &Tcg2Configuration
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Tcg2ConfigDriver: Fail to set TCG2_STORAGE_NAME\n"));
  }

  //
  // We should lock Tcg2DeviceDetection, because it contains information needed at S3.
  //
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VariableLockProtocol);
  if (!EFI_ERROR (Status)) {
    Status = VariableLockProtocol->RequestToLock (
                                     VariableLockProtocol,
                                     TCG2_DEVICE_DETECTION_NAME,
                                     &gTcg2ConfigFormSetGuid
                                     );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Install Tcg2 configuration form
  //
  Status = InstallTcg2ConfigForm (PrivateData);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  InitializeTcg2VersionInfo (PrivateData);

  return EFI_SUCCESS;

ErrorExit:
  if (PrivateData != NULL) {
    UninstallTcg2ConfigForm (PrivateData);
  }

  return Status;
}

/**
  Unload the Tcg2 configuration form.

  @param[in]  ImageHandle         The driver's image handle.

  @retval     EFI_SUCCESS         The Tcg2 configuration form is unloaded.
  @retval     Others              Failed to unload the form.

**/
EFI_STATUS
EFIAPI
Tcg2ConfigDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                  Status;
  TCG2_CONFIG_PRIVATE_DATA    *PrivateData;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (PrivateData->Signature == TCG2_CONFIG_PRIVATE_DATA_SIGNATURE);

  gBS->UninstallMultipleProtocolInterfaces (
         &ImageHandle,
         &gEfiCallerIdGuid,
         PrivateData,
         NULL
         );

  UninstallTcg2ConfigForm (PrivateData);

  return EFI_SUCCESS;
}

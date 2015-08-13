/** @file
  The module entry point for Tcg2 configuration module.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

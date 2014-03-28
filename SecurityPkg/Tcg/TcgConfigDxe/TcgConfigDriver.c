/** @file
  The module entry point for Tcg configuration module.

Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TcgConfigImpl.h"
#include <Guid/TpmInstance.h>

/**
  The entry point for Tcg configuration driver.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_ALREADY_STARTED    The driver already exists in system.
  @retval EFI_OUT_OF_RESOURCES   Fail to execute entry point due to lack of resources.
  @retval EFI_SUCCES             All the related protocols are installed on the driver.
  @retval Others                 Fail to install protocols as indicated.

**/
EFI_STATUS
EFIAPI
TcgConfigDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                Status;
  TCG_CONFIG_PRIVATE_DATA   *PrivateData;
  EFI_TCG_PROTOCOL          *TcgProtocol;

  if (!CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)){
    DEBUG ((EFI_D_ERROR, "No TPM12 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  Status = TisPcRequestUseTpm ((TIS_TPM_HANDLE) (UINTN) TPM_BASE_ADDRESS);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TPM not detected!\n"));
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **) &TcgProtocol);
  if (EFI_ERROR (Status)) {
    TcgProtocol = NULL;
  }
  
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
  PrivateData = AllocateCopyPool (sizeof (TCG_CONFIG_PRIVATE_DATA), &mTcgConfigPrivateDateTemplate);
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  PrivateData->TcgProtocol = TcgProtocol;
  
  //
  // Install TCG configuration form
  //
  Status = InstallTcgConfigForm (PrivateData);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Install private GUID.
  //    
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  if (PrivateData != NULL) {
    UninstallTcgConfigForm (PrivateData);
  }  
  
  return Status;
}

/**
  Unload the Tcg configuration form.

  @param[in]  ImageHandle         The driver's image handle.

  @retval     EFI_SUCCESS         The Tcg configuration form is unloaded.
  @retval     Others              Failed to unload the form.

**/
EFI_STATUS
EFIAPI
TcgConfigDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                  Status;
  TCG_CONFIG_PRIVATE_DATA   *PrivateData;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData
                  );  
  if (EFI_ERROR (Status)) {
    return Status;  
  }
  
  ASSERT (PrivateData->Signature == TCG_CONFIG_PRIVATE_DATA_SIGNATURE);

  gBS->UninstallMultipleProtocolInterfaces (
         &ImageHandle,
         &gEfiCallerIdGuid,
         PrivateData,
         NULL
         );
  
  UninstallTcgConfigForm (PrivateData);

  return EFI_SUCCESS;
}

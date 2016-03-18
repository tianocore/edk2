/** @file

This is QNC Smm Power Management driver

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "SmmPowerManagement.h"

//
// Global variables
//
EFI_SMM_CPU_PROTOCOL                    *mSmmCpu = NULL;
EFI_GLOBAL_NVS_AREA                     *mGlobalNvsAreaPtr = NULL;
EFI_MP_SERVICES_PROTOCOL                *mMpService = NULL;
EFI_ACPI_SDT_PROTOCOL                   *mAcpiSdt = NULL;
EFI_ACPI_TABLE_PROTOCOL                 *mAcpiTable = NULL;

EFI_GUID    mS3CpuRegisterTableGuid = S3_CPU_REGISTER_TABLE_GUID;

EFI_STATUS
EFIAPI
InitializePowerManagement (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Initializes the SMM Handler Driver

Arguments:

  ImageHandle -

  SystemTable -

Returns:

  None

--*/
{
  EFI_STATUS                                Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL             *SwDispatch;
  EFI_GLOBAL_NVS_AREA_PROTOCOL              *GlobalNvsAreaProtocol;

  //
  // Get SMM CPU protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);

  //
  //  Get the Sw dispatch protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID**)&SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Get Global NVS Area Protocol
  //
  Status = gBS->LocateProtocol (&gEfiGlobalNvsAreaProtocolGuid, NULL, (VOID **)&GlobalNvsAreaProtocol);
  ASSERT_EFI_ERROR (Status);
  mGlobalNvsAreaPtr = GlobalNvsAreaProtocol->Area;

  //
  // Locate and cache PI AcpiSdt Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **) &mAcpiSdt
                  );
  ASSERT_EFI_ERROR (Status);


  //
  // Locate and cache PI AcpiSdt Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **) &mAcpiTable
                  );
  ASSERT_EFI_ERROR (Status);


  //
  // Get MpService protocol
  //
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&mMpService);
  ASSERT_EFI_ERROR (Status);
  //
  // Initialize power management features on processors
  //
  PpmInit();

  return Status;
}

/** @file

  Parts of the SMM/MM implementation that are specific to traditional MM

Copyright (c) 2011 - 2024, Intel Corporation. All rights reserved. <BR>
Copyright (c) 2018, Linaro, Ltd. All rights reserved. <BR>
Copyright (c) 2026, Arm Limited. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmMemLib.h>
#include "Variable.h"

/**
  Whether the TCG or TCG2 protocols are installed in the UEFI protocol database.
  This information is used by the MorLock code to infer whether an existing
  MOR variable is legitimate or not.

  @retval TRUE  Either the TCG or TCG2 protocol is installed in the UEFI
                protocol database. MOR variable is legitimate.
  @retval FALSE Neither the TCG nor the TCG2 protocol is installed in the UEFI
                protocol database. MOR variable is not legitimate.
**/
BOOLEAN
VariableIsMorVariableLegitimate (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;

  Status = gBS->LocateProtocol (
                  &gEfiTcg2ProtocolGuid,
                  NULL,                     // Registration
                  &Interface
                  );
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  Status = gBS->LocateProtocol (
                  &gEfiTcgProtocolGuid,
                  NULL,                     // Registration
                  &Interface
                  );
  return !EFI_ERROR (Status);
}

/**
  Variable Storage FVB MM driver entry point.

  @param[in] ImageHandle    A handle for the image that is initializing this
                            driver
  @param[in] SystemTable    A pointer to the EFI system table

  @retval EFI_SUCCESS       Variable storage successfully initialized.
**/
EFI_STATUS
EFIAPI
VariableStorageFvbSmmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return VariableStorageFvbInitialize (ImageHandle);
}

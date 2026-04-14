/** @file

  Parts of the SMM/MM implementation that are specific to standalone MM

Copyright (c) 2011 - 2024, Intel Corporation. All rights reserved. <BR>
Copyright (c) 2018, Linaro, Ltd. All rights reserved. <BR>
Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/MmServicesTableLib.h>
#include <Library/StandaloneMmMemLib.h>
#include "Variable.h"

/**
  Whether the MOR variable is legitimate or not.

  @retval TRUE  MOR Variable is legitimate.
  @retval FALSE MOR Variable in not legitimate.
**/
BOOLEAN
VariableIsMorVariableLegitimate (
  VOID
  )
{
  return TRUE;
}

/**
  Variable Storage FVB MM driver entry point.

  @param[in] ImageHandle    A handle for the image that is initializing this
                            driver
  @param[in] MmSystemTable  A pointer to the MM system table

  @retval EFI_SUCCESS       Variable storage successfully initialized.
**/
EFI_STATUS
EFIAPI
VariableStorageFvbSmmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return VariableStorageFvbInitialize (ImageHandle);
}

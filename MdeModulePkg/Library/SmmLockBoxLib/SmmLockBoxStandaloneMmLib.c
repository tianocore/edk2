/** @file

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include "SmmLockBoxLibPrivate.h"

/**
  Constructor for SmmLockBox library.
  This is used to set SmmLockBox context, which will be used in PEI phase in S3 boot path later.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
SmmLockBoxStandaloneMmConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return SmmLockBoxMmConstructor ();
}

/**
  Destructor for SmmLockBox library.
  This is used to uninstall SmmLockBoxCommunication configuration table
  if it has been installed in Constructor.

  @param[in] ImageHandle    Image handle of this driver.
  @param[in] SystemTable    A Pointer to the EFI System Table.

  @retval EFI_SUCEESS       The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmLockBoxStandaloneMmDestructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return SmmLockBoxMmDestructor ();
}

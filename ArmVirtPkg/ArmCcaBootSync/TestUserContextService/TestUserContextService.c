/** @file
  Host based unit test User Context service.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>

#include <Base.h>
#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Uefi/UefiBaseType.h>

/**
  Entrypoint for the Test User Context service.

  @param[in] argc         Number of input arguments.
  @param[in] argv         Input arguments.

  @retval    0             Success.
            -1             Failure
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  EFI_STATUS  Status;

  printf ("Test User Context Service.\n");

  Status = ArmCcaBootSyncCryptoInit ();
  if (EFI_ERROR (Status)) {
    printf (
      "Error: Failed to init Crypto interfaces!, Status = 0x%x\n",
      Status
      );
    return -1;
  }

  return 0;
}

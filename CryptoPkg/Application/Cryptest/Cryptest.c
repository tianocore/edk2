/** @file
  Application for Cryptographic Primitives Validation.

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Cryptest.h"

/**
  Entry Point of Cryptographic Validation Utility.

  @param  ImageHandle  The image handle of the UEFI Application.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
CryptestMain (
  IN     EFI_HANDLE                 ImageHandle,
  IN     EFI_SYSTEM_TABLE           *SystemTable
  )
{
  EFI_STATUS  Status;

  Print (L"\nUEFI-OpenSSL Wrapper Cryptosystem Testing: \n");
  Print (L"-------------------------------------------- \n");

  RandomSeed (NULL, 0);

  Status = ValidateCryptDigest ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateCryptHmac ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateCryptBlockCipher ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateCryptRsa ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateCryptRsa2 ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateCryptPkcs7 ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateAuthenticode ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateTSCounterSignature ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateCryptDh ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateCryptPrng ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
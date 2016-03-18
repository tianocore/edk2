/** @file
  Application for Diffie-Hellman Primitives Validation.

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Cryptest.h"

/**
  Validate UEFI-OpenSSL DH Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptDh (
  VOID
  )
{
  VOID    *Dh1;
  VOID    *Dh2;
  UINT8   Prime[64];
  UINT8   PublicKey1[64];
  UINTN   PublicKey1Length;
  UINT8   PublicKey2[64];
  UINTN   PublicKey2Length;
  UINT8   Key1[64];
  UINTN   Key1Length;
  UINT8   Key2[64];
  UINTN   Key2Length;
  BOOLEAN Status;

  Print (L"\nUEFI-OpenSSL DH Engine Testing:\n");

  //
  // Initialize Key Length
  //
  PublicKey1Length = sizeof (PublicKey1);
  PublicKey2Length = sizeof (PublicKey2);
  Key1Length       = sizeof (Key1);
  Key2Length       = sizeof (Key2);

  //
  // Generate & Initialize DH Context
  //
  Print (L"- Context1 ... ");
  Dh1 = DhNew ();
  if (Dh1 == NULL) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Context2 ... ");
  Dh2 = DhNew ();
  if (Dh2 == NULL) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Parameter1 ... ");
  Status = DhGenerateParameter (Dh1, 2, 64, Prime);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Parameter2 ... ");
  Status = DhSetParameter (Dh2, 2, 64, Prime);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Generate key1 ... ");
  Status = DhGenerateKey (Dh1, PublicKey1, &PublicKey1Length);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Generate key2 ... ");
  Status = DhGenerateKey (Dh2, PublicKey2, &PublicKey2Length);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Compute key1 ... ");
  Status = DhComputeKey (Dh1, PublicKey2, PublicKey2Length, Key1, &Key1Length);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Compute key2 ... ");
  Status = DhComputeKey (Dh2, PublicKey1, PublicKey1Length, Key2, &Key2Length);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Compare Keys ... ");
  if (Key1Length != Key2Length) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Key1, Key2, Key1Length) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  return EFI_SUCCESS;
}
/** @file
  Application for PKCS#5 PBKDF2 Function Validation.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Cryptest.h"

//
// PBKDF2 HMAC-SHA1 Test Vector from RFC6070
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *Password = "password";  // Input Password
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        PassLen   = 8;           // Length of Input Password
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *Salt     = "salt";      // Input Salt
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        SaltLen   = 4;           // Length of Input Salt
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN  Count     = 2;           // InterationCount
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN  KeyLen    = 20;          // Length of derived key
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  DerivedKey[]  = {        // Expected output key
  0xea, 0x6c, 0x01, 0x4d, 0xc7, 0x2d, 0x6f, 0x8c, 0xcd, 0x1e, 0xd9, 0x2a, 0xce, 0x1d, 0x41, 0xf0,
  0xd8, 0xde, 0x89, 0x57
  };

/**
  Validate UEFI-OpenSSL PKCS#5 PBKDF2 Interface.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptPkcs5Pbkdf2 (
  VOID
  )
{
  BOOLEAN  Status;
  UINT8    *OutKey;

  Print (L"\nUEFI-OpenSSL PKCS#5 PBKDF2 Testing: ");
  Print (L"\n- PKCS#5 PBKDF2 Verification: ");

  OutKey = AllocatePool (KeyLen);
  if (OutKey == NULL) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  //
  // Verify PKCS#5 PBKDF2 Key Derivation Function
  //
  Print (L"Deriving Key... ");
  Status = Pkcs5HashPassword (
             PassLen,
             Password,
             SaltLen,
             (CONST UINT8 *)Salt,
             Count,
             SHA1_DIGEST_SIZE,
             KeyLen,
             OutKey
             );

  if (!Status) {
    Print (L"[Fail]");
    FreePool (OutKey);
    return EFI_ABORTED;
  }

  //
  // Check the output key with the expected key result
  //
  Print (L"Check Derived Key... ");
  if (CompareMem (OutKey, DerivedKey, KeyLen) != 0) {
    Print (L"[Fail]");
    FreePool (OutKey);
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  //
  // Release Resources
  //
  FreePool (OutKey);

  return EFI_SUCCESS;
}

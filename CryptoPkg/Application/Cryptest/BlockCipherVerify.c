/** @file  
  Application for Block Cipher Primitives Validation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Cryptest.h"

//
// TDES test vectors are extracted from OpenSSL 0.9.8l, crypto\des\destest.c
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 TdesEcbData[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 TdesEcbKey[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 TdesEcbCipher[] = {
  0x8C, 0xA6, 0x4D, 0xE9, 0xC1, 0xB1, 0x23, 0xA7,
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 TdesEcb2Cipher[] = {
  0x92, 0x95, 0xB5, 0x9B, 0xB3, 0x84, 0x73, 0x6E,
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 TdesCbcData[] = {
  0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x20,
  0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74,
  0x68, 0x65, 0x20, 0x74, 0x69, 0x6D, 0x65, 0x20
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 TdesCbcKey[] = {
  0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 
  0xf1, 0xe0, 0xd3, 0xc2, 0xb5, 0xa4, 0x97, 0x86,
  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
  };

GLOBAL_REMOVE_IF_UNREFERENCED UINT8 TdesCbcIvec[] = {
  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 TdesCbc3Cipher[] = {
  0x3F, 0xE3, 0x01, 0xC9, 0x62, 0xAC, 0x01, 0xD0,
  0x22, 0x13, 0x76, 0x3C, 0x1C, 0xBD, 0x4C, 0xDC,
  0x79, 0x96, 0x57, 0xC0, 0x64, 0xEC, 0xF5, 0xD4
  };

//
// AES test vectors are from NIST KAT of AES
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes128EcbData[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes128EcbKey[] = {
  0x10, 0xa5, 0x88, 0x69, 0xd7, 0x4b, 0xe5, 0xa3, 0x74, 0xcf, 0x86, 0x7c, 0xfb, 0x47, 0x38, 0x59
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes128EcbCipher[] = {
  0x6d, 0x25, 0x1e, 0x69, 0x44, 0xb0, 0x51, 0xe0, 0x4e, 0xaa, 0x6f, 0xb4, 0xdb, 0xf7, 0x84, 0x65
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes192EcbData[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes192EcbKey[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes192EcbCipher[] = {
  0xdd, 0x8a, 0x49, 0x35, 0x14, 0x23, 0x1c, 0xbf, 0x56, 0xec, 0xce, 0xe4, 0xc4, 0x08, 0x89, 0xfb
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes256EcbData[] = {
  0x01, 0x47, 0x30, 0xf8, 0x0a, 0xc6, 0x25, 0xfe, 0x84, 0xf0, 0x26, 0xc6, 0x0b, 0xfd, 0x54, 0x7d
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes256EcbKey[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes256EcbCipher[] = {
  0x5c, 0x9d, 0x84, 0x4e, 0xd4, 0x6f, 0x98, 0x85, 0x08, 0x5e, 0x5d, 0x6a, 0x4f, 0x94, 0xc7, 0xd7
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes128CbcData[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes128CbcKey[] = {
  0xc2, 0x86, 0x69, 0x6d, 0x88, 0x7c, 0x9a, 0xa0, 0x61, 0x1b, 0xbb, 0x3e, 0x20, 0x25, 0xa4, 0x5a
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes128CbcIvec[] = {
  0x56, 0x2e, 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Aes128CbcCipher[] = {
  0xd2, 0x96, 0xcd, 0x94, 0xc2, 0xcc, 0xcf, 0x8a, 0x3a, 0x86, 0x30, 0x28, 0xb5, 0xe1, 0xdc, 0x0a,
  0x75, 0x86, 0x60, 0x2d, 0x25, 0x3c, 0xff, 0xf9, 0x1b, 0x82, 0x66, 0xbe, 0xa6, 0xd6, 0x1a, 0xb1
  };

//
// ARC4 Test Vector defined in "Appendix A.1 Test Vectors from [CRYPTLIB]" of
// IETF Draft draft-kaukonen-cipher-arcfour-03 ("A Stream Cipher Encryption Algorithm 'Arcfour'").
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Arc4Data[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Arc4Key[] = {
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
  };

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Arc4Cipher[] = {
  0x74, 0x94, 0xC2, 0xE7, 0x10, 0x4B, 0x08, 0x79
  };

/**
  Validate UEFI-OpenSSL Block Ciphers (Symmetric Crypto) Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptBlockCipher (
  VOID
  )
{
  UINTN    CtxSize;
  VOID     *CipherCtx;
  UINT8    Encrypt[256];
  UINT8    Decrypt[256];
  BOOLEAN  Status;

  Print (L"\nUEFI-OpenSSL Block Cipher Engine Testing: ");

  CtxSize   = TdesGetContextSize ();
  CipherCtx = AllocatePool (CtxSize);

  Print (L"\n- TDES Validation: ");


  Print (L"ECB... ");

  //
  // TDES ECB Validation
  //
  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = TdesInit (CipherCtx, TdesEcbKey, 64);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = TdesEcbEncrypt (CipherCtx, TdesEcbData, 8, Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = TdesEcbDecrypt (CipherCtx, Encrypt, 8, Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, TdesEcbCipher, 8) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Decrypt, TdesEcbData, 8) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"EDE2 ECB... ");

  //
  // TDES EDE2 ECB Validation
  //
  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = TdesInit (CipherCtx, TdesEcbKey, 128);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = TdesEcbEncrypt (CipherCtx, TdesEcbData, 8, Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = TdesEcbDecrypt (CipherCtx, Encrypt, 8, Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, TdesEcb2Cipher, 8) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  } 

  if (CompareMem (Decrypt, TdesEcbData, 8) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"EDE3 CBC... ");

  //
  // TDES EDE3 CBC Validation
  //
  ZeroMem (Encrypt, 256);
  ZeroMem (Decrypt, 256);

  Status = TdesInit (CipherCtx, TdesCbcKey, 192);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = TdesCbcEncrypt (CipherCtx, TdesCbcData, sizeof (TdesCbcData), TdesCbcIvec, Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = TdesCbcDecrypt (CipherCtx, Encrypt, sizeof (TdesCbcData), TdesCbcIvec, Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, TdesCbc3Cipher, sizeof (TdesCbc3Cipher)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Decrypt, TdesCbcData, sizeof (TdesCbcData)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]");

  FreePool (CipherCtx);

  CtxSize   = AesGetContextSize ();
  CipherCtx = AllocatePool (CtxSize);
  
  Print (L"\n- AES Validation:  ");

  Print (L"ECB-128... ");

  //
  // AES-128 ECB Validation
  //
  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = AesInit (CipherCtx, Aes128EcbKey, 128);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesEcbEncrypt (CipherCtx, Aes128EcbData, sizeof (Aes128EcbData), Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesEcbDecrypt (CipherCtx, Encrypt, sizeof (Aes128EcbData), Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, Aes128EcbCipher, sizeof (Aes128EcbCipher)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  } 

  if (CompareMem (Decrypt, Aes128EcbData, sizeof (Aes128EcbData)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"ECB-192... ");

  //
  // AES-192 ECB Validation
  //
  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = AesInit (CipherCtx, Aes192EcbKey, 192);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesEcbEncrypt (CipherCtx, Aes192EcbData, sizeof (Aes192EcbData), Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesEcbDecrypt (CipherCtx, Encrypt, sizeof (Aes192EcbData), Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, Aes192EcbCipher, sizeof (Aes192EcbCipher)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Decrypt, Aes192EcbData, sizeof (Aes192EcbData)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"ECB-256... ");

  //
  // AES-256 ECB Validation
  //
  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = AesInit (CipherCtx, Aes256EcbKey, 256);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesEcbEncrypt (CipherCtx, Aes256EcbData, sizeof (Aes256EcbData), Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesEcbDecrypt (CipherCtx, Encrypt, sizeof (Aes256EcbData), Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, Aes256EcbCipher, sizeof (Aes256EcbCipher)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Decrypt, Aes256EcbData, sizeof (Aes256EcbData)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"CBC-128... ");

  //
  // AES-128 CBC Validation
  //
  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = AesInit (CipherCtx, Aes128CbcKey, 128);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesCbcEncrypt (CipherCtx, Aes128CbcData, sizeof (Aes128CbcData), Aes128CbcIvec, Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = AesCbcDecrypt (CipherCtx, Encrypt, sizeof (Aes128CbcData), Aes128CbcIvec, Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, Aes128CbcCipher, sizeof (Aes128CbcCipher)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Decrypt, Aes128CbcData, sizeof (Aes128CbcData)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]");

  Print (L"\n- ARC4 Validation: ");

  //
  // ARC4 Validation
  //
  CtxSize   = Arc4GetContextSize ();
  CipherCtx = AllocatePool (CtxSize);

  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = Arc4Init (CipherCtx, Arc4Key, sizeof (Arc4Key));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = Arc4Encrypt (CipherCtx, Arc4Data, sizeof (Arc4Data), Encrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = Arc4Reset (CipherCtx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = Arc4Decrypt (CipherCtx, Encrypt, sizeof (Arc4Data), Decrypt);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Encrypt, Arc4Cipher, sizeof (Arc4Cipher)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Decrypt, Arc4Data, sizeof (Arc4Data)) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]");

  Print (L"\n");

  return EFI_SUCCESS;
}

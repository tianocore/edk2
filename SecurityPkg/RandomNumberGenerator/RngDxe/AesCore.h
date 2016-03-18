/** @file
  Function prototype for AES Block Cipher support.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __AES_CORE_H__
#define __AES_CORE_H__

/**
  Encrypts one single block data (128 bits) with AES algorithm.

  @param[in]  Key                AES symmetric key buffer.
  @param[in]  InData             One block of input plaintext to be encrypted.
  @param[out] OutData            Encrypted output ciphertext.

  @retval EFI_SUCCESS            AES Block Encryption succeeded.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
AesEncrypt (
  IN  UINT8        *Key,
  IN  UINT8        *InData,
  OUT UINT8        *OutData
  );

#endif  // __AES_CORE_H__

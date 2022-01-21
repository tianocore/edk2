/** @file
  Null AES Library

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - FIPS 197 November 26, 2001:
     Specification for the ADVANCED ENCRYPTION STANDARD (AES)
**/

#include <Library/AesLib.h>
#include <Library/DebugLib.h>

/** Encrypt an AES block.

  Buffers are little-endian. Overlapping is not checked.

  @param [in]  AesCtx    AES context.
                         AesCtx is initialized with AesInitCtx ().
  @param [in]  InBlock   Input Block. The block to cipher.
  @param [out] OutBlock  Output Block. The ciphered block.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_UNSUPPORTED        Unsupported.
**/
EFI_STATUS
EFIAPI
AesEncrypt (
  IN  AES_CTX      *AesCtx,
  IN  UINT8 CONST  *InBlock,
  OUT UINT8        *OutBlock
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** Decrypt an AES block.

  Buffers are little-endian. Overlapping is not checked.

  @param [in]  AesCtx    AES context.
                         AesCtx is initialized with AesInitCtx ().
  @param [in]  InBlock   Input Block. The block to de-cipher.
  @param [out] OutBlock  Output Block. The de-ciphered block.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_UNSUPPORTED        Unsupported.
**/
EFI_STATUS
EFIAPI
AesDecrypt (
  IN  AES_CTX      *AesCtx,
  IN  UINT8 CONST  *InBlock,
  OUT UINT8        *OutBlock
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** Initialize an AES_CTX structure.

  @param [in]       Key       AES key. Buffer of KeySize bytes.
                              The buffer is little endian.
  @param [in]       KeySize   Size of the key. Must be one of 128|192|256.
  @param [in, out]  AesCtx    AES context to initialize.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_UNSUPPORTED        Unsupported.
**/
EFI_STATUS
EFIAPI
AesInitCtx (
  IN      UINT8    *Key,
  IN      UINT32   KeySize,
  IN OUT  AES_CTX  *AesCtx
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

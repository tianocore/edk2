/** @file
  AES library.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - FIPS 197 November 26, 2001:
     Specification for the ADVANCED ENCRYPTION STANDARD (AES)
**/

#ifndef AES_LIB_H_
#define AES_LIB_H_

/// Key size in bytes.
#define AES_KEY_SIZE_128  16
#define AES_KEY_SIZE_192  24
#define AES_KEY_SIZE_256  32
#define AES_BLOCK_SIZE    16

/*
   The Key Expansion generates a total of Nb (Nr + 1) words with:
    - Nb = 4:
      Number of columns (32-bit words) comprising the State
    - Nr = 10, 12, or 14:
      Number of rounds.
 */
#define AES_MAX_KEYLENGTH_U32  (4 * (14 + 1))

/** A context holding information to for AES encryption/decryption.
 */
typedef struct {
  /// Expanded encryption key.
  UINT32    ExpEncKey[AES_MAX_KEYLENGTH_U32];
  /// Expanded decryption key.
  UINT32    ExpDecKey[AES_MAX_KEYLENGTH_U32];
  /// Key size, in bytes.
  /// Must be one of 16|24|32.
  UINT32    KeySize;
} AES_CTX;

/** Encrypt an AES block.

  Buffers are little-endian. Overlapping is not checked.

  @param [in]  AesCtx    AES context.
                         AesCtx is initialized with AesInitCtx ().
  @param [in]  InBlock   Input Block. The block to cipher.
  @param [out] OutBlock  Output Block. The ciphered block.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval RETURN_UNSUPPORTED        Unsupported.
**/
RETURN_STATUS
EFIAPI
AesEncrypt (
  IN  AES_CTX      *AesCtx,
  IN  UINT8 CONST  *InBlock,
  OUT UINT8        *OutBlock
  );

/** Decrypt an AES block.

  Buffers are little-endian. Overlapping is not checked.

  @param [in]  AesCtx    AES context.
                         AesCtx is initialized with AesInitCtx ().
  @param [in]  InBlock   Input Block. The block to de-cipher.
  @param [out] OutBlock  Output Block. The de-ciphered block.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval RETURN_UNSUPPORTED        Unsupported.
**/
RETURN_STATUS
EFIAPI
AesDecrypt (
  IN  AES_CTX      *AesCtx,
  IN  UINT8 CONST  *InBlock,
  OUT UINT8        *OutBlock
  );

/** Initialize an AES_CTX structure.

  @param [in]       Key       AES key. Buffer of KeySize bytes.
                              The buffer is little endian.
  @param [in]       KeySize   Size of the key. Must be one of 128|192|256.
  @param [in, out]  AesCtx    AES context to initialize.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval RETURN_UNSUPPORTED        Unsupported.
**/
RETURN_STATUS
EFIAPI
AesInitCtx (
  IN      UINT8    *Key,
  IN      UINT32   KeySize,
  IN OUT  AES_CTX  *AesCtx
  );

#endif // AES_LIB_H_

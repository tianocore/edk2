/** @file
  Arm AES Library

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - FIPS 197 November 26, 2001:
     Specification for the ADVANCED ENCRYPTION STANDARD (AES)
**/

#include <Library/BaseLib.h>
#include <Library/AesLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "ArmAesLib.h"

/** The constructor checks that the FEAT_AES extension is available.

  @retval RETURN_SUCCESS   The constructor always returns RETURN_SUCCESS.
**/
RETURN_STATUS
EFIAPI
AesLibConstructor (
  VOID
  )
{
  if (!ArmHasAesExt ()) {
    DEBUG ((
      DEBUG_ERROR,
      "FEAT_AES extension is not available. "
      "This library cannot be used.\n"
      ));
    ASSERT_RETURN_ERROR (RETURN_UNSUPPORTED);
  }

  return RETURN_SUCCESS;
}

/**
  AES key schedule round constants.
*/
STATIC
UINT8 CONST
mRoundConstants[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
};

/** Get the number of Rounds.

  AES needs to perform a different number of rounds depending on the key size:
   * 128-bits: 10
   * 192-bits: 12
   * 256-bits: 14
  So 6 + (n/4) rounds

  @param [in] AesCtx  AES context struct.

  @return Number of rounds.
**/
STATIC
UINT32
GetNumRounds (
  IN  AES_CTX CONST  *AesCtx
  )
{
  return 6 + (AesCtx->KeySize >> 2);
}

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
  )
{
  if ((AesCtx == NULL)    ||
      (InBlock == NULL)   ||
      (OutBlock == NULL)  ||
      (InBlock == OutBlock))
  {
    ASSERT (AesCtx != NULL);
    ASSERT (InBlock != NULL);
    ASSERT (OutBlock != NULL);
    ASSERT (InBlock != OutBlock);
    return RETURN_INVALID_PARAMETER;
  }

  ArmAesEncrypt (
    AesCtx->ExpEncKey,
    GetNumRounds (AesCtx),
    InBlock,
    OutBlock
    );

  return RETURN_SUCCESS;
}

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
  )
{
  if ((AesCtx == NULL)  ||
      (InBlock == NULL) ||
      (OutBlock == NULL)  ||
      (InBlock == OutBlock))
  {
    ASSERT (AesCtx != NULL);
    ASSERT (InBlock != NULL);
    ASSERT (OutBlock != NULL);
    ASSERT (InBlock != OutBlock);
    return RETURN_INVALID_PARAMETER;
  }

  ArmAesDecrypt (
    AesCtx->ExpDecKey,
    GetNumRounds (AesCtx),
    InBlock,
    OutBlock
    );

  return RETURN_SUCCESS;
}

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
  )
{
  UINTN      Index;
  UINTN      RevIndex;
  UINT32     KeyWords;
  UINT32     *KeyIn;
  UINT32     *KeyOut;
  AES_BLOCK  *InBlock;
  AES_BLOCK  *OutBlock;

  if ((Key == NULL)                       ||
      ((KeySize != 8 * AES_KEY_SIZE_128)  &&
       (KeySize != 8 * AES_KEY_SIZE_192)  &&
       (KeySize != 8 * AES_KEY_SIZE_256)) ||
      (AesCtx == NULL))
  {
    ASSERT (Key != NULL);
    ASSERT (
      !((KeySize != 8 * AES_KEY_SIZE_128)   &&
        (KeySize != 8 * AES_KEY_SIZE_192)    &&
        (KeySize != 8 * AES_KEY_SIZE_256))
      );
    ASSERT (AesCtx != NULL);
    return RETURN_INVALID_PARAMETER;
  }

  // Internally, use bytes.
  KeySize         = KeySize >> 3;
  AesCtx->KeySize = KeySize;
  KeyWords        = KeySize >> 2;

  // The first part of the expanded key is the input key.
  for (Index = 0; Index < KeyWords; Index++) {
    AesCtx->ExpEncKey[Index] = ReadUnaligned32 (
                                 (UINT32 *)(Key + (Index * sizeof (UINT32)))
                                 );
  }

  for (Index = 0; Index < sizeof (mRoundConstants); Index++) {
    KeyIn  = AesCtx->ExpEncKey + (Index * KeyWords);
    KeyOut = KeyIn + KeyWords;

    KeyOut[0]  = ArmAesSubWord (RRotU32 (KeyIn[KeyWords - 1], 8));
    KeyOut[0] ^= mRoundConstants[Index] ^ KeyIn[0];
    KeyOut[1]  = KeyOut[0] ^ KeyIn[1];
    KeyOut[2]  = KeyOut[1] ^ KeyIn[2];
    KeyOut[3]  = KeyOut[2] ^ KeyIn[3];

    if (KeySize == AES_KEY_SIZE_192) {
      if (Index >= 7) {
        break;
      }

      KeyOut[4] = KeyOut[3] ^ KeyIn[4];
      KeyOut[5] = KeyOut[4] ^ KeyIn[5];
    } else if (KeySize == AES_KEY_SIZE_256) {
      if (Index >= 6) {
        break;
      }

      KeyOut[4] = ArmAesSubWord (KeyOut[3]) ^ KeyIn[4];
      KeyOut[5] = KeyOut[4] ^ KeyIn[5];
      KeyOut[6] = KeyOut[5] ^ KeyIn[6];
      KeyOut[7] = KeyOut[6] ^ KeyIn[7];
    }
  }

  /*
   * Generate the decryption key for the Equivalent Inverse Cipher.
   * First and last state of the expanded encryption key are copied
   * to the expanded decryption key.
   * The other ones are copied bottom up from the expanded encryption
   * key and undergo an InvMixColumns().
   */
  InBlock  = (AES_BLOCK *)AesCtx->ExpEncKey;
  OutBlock = (AES_BLOCK *)AesCtx->ExpDecKey;
  RevIndex = GetNumRounds (AesCtx);

  CopyMem (&OutBlock[0], &InBlock[RevIndex], sizeof (AES_BLOCK));
  for (Index = 1, RevIndex--; RevIndex > 0; Index++, RevIndex--) {
    ArmAesInvert (OutBlock + Index, InBlock + RevIndex);
  }

  CopyMem (&OutBlock[Index], &InBlock[0], sizeof (AES_BLOCK));

  return RETURN_SUCCESS;
}

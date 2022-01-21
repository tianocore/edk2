/** @file
  Arm AES Library

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - FIPS 197 November 26, 2001:
     Specification for the ADVANCED ENCRYPTION STANDARD (AES)
**/

#ifndef ARM_AES_LIB_H_
#define ARM_AES_LIB_H_

/* An AES block is 128-bits long and can be seen as a matrix of 4 * 4 bytes.
 */
typedef struct AesBlock {
  /// The AES block.
  UINT8    Block[AES_BLOCK_SIZE];
} AES_BLOCK;

/** Encrypt an AES block.

  @param [in]  ExpEncKey  Expanded encryption key. An array of 32-bits words
                          with the number of elements depending on the key
                          size:
                           * 128-bits: 44 words
                           * 192-bits: 52 words
                           * 256-bits: 60 words
  @param [in]  Rounds     Number of rounds (depending on the key size).
  @param [in]  InBlock    Input Block. The block to cipher.
  @param [out] OutBlock   Output Block. The ciphered block.
**/
VOID
ArmAesEncrypt (
  IN  UINT32 CONST  *ExpEncKey,
  IN  UINT32        Rounds,
  IN  UINT8  CONST  *InBlock,
  OUT UINT8         *OutBlock
  );

/** Decrypt an AES 128-bits block.

  @param [in]  ExpDecKey  Expanded decryption key. An array of 32-bits words
                          with the number of elements depending on the key
                          size:
                           * 128-bits: 44 words
                           * 192-bits: 52 words
                           * 256-bits: 60 words
  @param [in]  Rounds     Number of rounds (depending on the key size).
  @param [in]  InBlock    Input Block. The block to de-cipher.
  @param [out] OutBlock   Output Block. The de-ciphered block.
**/
VOID
ArmAesDecrypt (
  IN  UINT32 CONST  *ExpDecKey,
  IN  UINT32        Rounds,
  IN  UINT8  CONST  *InBlock,
  OUT UINT8         *OutBlock
  );

/** Perform a SubWord() operation (applying AES Sbox) on a 32-bits word.

  The Arm AESE instruction performs the AddRoundKey(), ShiftRows() and
  SubBytes() AES steps in this order.

  During key expansion, only SubBytes() should be performed, so:
  - use a key of {0} so AddRoundKey() becomes an identity function;
  - the dup instruction allows to have a matrix with identic rows,
    so ShiftRows() has no effect.

  @param [in]  InWord  The 32-bits word to apply SubWord() on.

  @return SubWord(word).
**/
UINT32
ArmAesSubWord (
  IN  UINT32  InWord
  );

/** Perform a InvMixColumns() operation on an AES block (128-bits) using
    the Arm AESIMC instruction.

  This is usefull to get decryption key for the Equivalent Inverse Cipher.

  @param [in]  InBlock    Input block.
  @param [out] OutBlock   Output blocked.
**/
VOID
ArmAesInvert (
  IN  AES_BLOCK CONST  *InBlock,
  OUT AES_BLOCK        *OutBlock
  );

#endif // ARM_AES_LIB_H_

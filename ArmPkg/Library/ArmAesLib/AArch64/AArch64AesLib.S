/** @file
  AArch64 AES implementation.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AsmMacroIoLibV8.h>

.arch_extension crypto

// Generic notes:
// - In AArch64, the AESE/AESD/AESMC/AESIMC instructions are using registers
//   as <Vx>.16B
// - For some CPUs, the latency of LD1 is 6, thus the unfolding.
// - The latency of the AESE/AESMC pair is 2.
// Cf.
// Arm Cortex-X1 Core Revision: r1p2 Software Optimization Guide
// Arm Cortex-X2 Core Revision: r2p0 Software Optimization Guide

// /** Encrypt an AES block.
//
//   @param [in]  ExpEncKey  Expanded encryption key. An array of 32-bits words
//                           with the number of elements depending on the key
//                           size:
//                            * 128-bits: 44 words
//                            * 192-bits: 52 words
//                            * 256-bits: 60 words
//   @param [in]  Rounds     Number of rounds (depending on the key size).
//   @param [in]  InBlock    Input Block. The block to cipher.
//   @param [out] OutBlock   Output Block. The ciphered block.
// **/
// VOID
// ArmAesEncrypt (
//   IN  UINT32 CONST  *ExpEncKey,
//   IN  UINT32        Rounds,
//   IN  UINT8  CONST  *InBlock,
//   OUT UINT8         *OutBlock
//   );
ASM_FUNC(ArmAesEncrypt)
    ld1      {v0.16b}, [x2]
    cmp      w1, #12
    beq      0f

    // Rounds = 10 or 14. Start loading the expanded key.
    ld1      {v4.4s}, [x0], #16
    ld1      {v1.4s}, [x0], #16
    ld1      {v2.4s}, [x0], #16
    adds     w1, w1, #1
    b        2f

    // Rounds = 12. Start loading the expanded key.
0:  ld1      {v2.4s}, [x0], #16
    ld1      {v3.4s}, [x0], #16
    ld1      {v4.4s}, [x0], #16
    subs     w1, w1, #1
    b        3f

    // Start of the loop (unfolded for 4 rounds).
1:  ld1      {v4.4s}, [x0], #16
    aese     v0.16b, v1.16b
    aesmc    v0.16b, v0.16b
3:  ld1      {v1.4s}, [x0], #16
    aese     v0.16b, v2.16b
    aesmc    v0.16b, v0.16b
    ld1      {v2.4s}, [x0], #16
    aese     v0.16b, v3.16b
    aesmc    v0.16b, v0.16b
2:  subs     w1, w1, #4
    ld1      {v3.4s}, [x0], #16
    aese     v0.16b, v4.16b
    aesmc    v0.16b, v0.16b
    bpl      1b

    // Final round.
    aese     v0.16b, v1.16b
    eor      v0.16b, v0.16b, v2.16b
    st1      {v0.16b}, [x3]
    ret

// /** Decrypt an AES 128-bits block.
//
//   @param [in]  ExpDecKey  Expanded decryption key. An array of 32-bits words
//                           with the number of elements depending on the key
//                           size:
//                            * 128-bits: 44 words
//                            * 192-bits: 52 words
//                            * 256-bits: 60 words
//   @param [in]  Rounds     Number of rounds (depending on the key size).
//   @param [in]  InBlock    Input Block. The block to de-cipher.
//   @param [out] OutBlock   Output Block. The de-ciphered block.
// **/
// VOID
// ArmAesDecrypt (
//   IN  UINT32 CONST  *ExpDecKey,
//   IN  UINT32        Rounds,
//   IN  UINT8  CONST  *InBlock,
//   OUT UINT8         *OutBlock
//   );
ASM_FUNC(ArmAesDecrypt)
    ld1      {v0.16b}, [x2]
    cmp      w1, #12
    beq      0f

    // Rounds = 10 or 14. Start loading the expanded key.
    ld1      {v4.4s}, [x0], #16
    ld1      {v1.4s}, [x0], #16
    ld1      {v2.4s}, [x0], #16
    adds     w1, w1, #1
    b        2f

    // Rounds = 12. Start loading the expanded key.
0:  ld1      {v2.4s}, [x0], #16
    ld1      {v3.4s}, [x0], #16
    ld1      {v4.4s}, [x0], #16
    subs     w1, w1, #1
    b        3f

    // Start of the loop (unfolded for 4 rounds).
1:  ld1      {v4.4s}, [x0], #16
    aesd     v0.16b, v1.16b
    aesimc   v0.16b, v0.16b
3:  ld1      {v1.4s}, [x0], #16
    aesd     v0.16b, v2.16b
    aesimc   v0.16b, v0.16b
    ld1      {v2.4s}, [x0], #16
    aesd     v0.16b, v3.16b
    aesimc   v0.16b, v0.16b
2:  subs     w1, w1, #4
    ld1      {v3.4s}, [x0], #16
    aesd     v0.16b, v4.16b
    aesimc   v0.16b, v0.16b
    bpl      1b

    // Final round.
    aesd     v0.16b, v1.16b
    eor      v0.16b, v0.16b, v2.16b
    st1      {v0.16b}, [x3]
    ret

// /** Perform a SubWord() operation (applying AES Sbox) on a 32-bits word.
//
//   The Arm AESE instruction performs the AddRoundKey(), ShiftRows() and
//   SubBytes() AES steps in this order.
//
//   During key expansion, only SubBytes() should be performed, so:
//   - use a key of {0} so AddRoundKey() becomes an identity function;
//   - the dup instruction allows to have a matrix with identic rows,
//     so ShiftRows() has no effect.
//
//   @param [in]  InWord  The 32-bits word to apply SubWord() on.
//
//   @return SubWord(word).
// **/
// UINT32
// ArmAesSubWord (
//   IN  UINT32  InWord
//   );
ASM_FUNC(ArmAesSubWord)
    dup      v1.4s, w0
    movi     v0.16b, #0
    aese     v0.16b, v1.16b
    umov     w0, v0.s[0]
    ret

// /** Perform a InvMixColumns() operation on an AES block (128-bits) using
//     the Arm AESIMC instruction.
//
//   This is usefull to get decryption key for the Equivalent Inverse Cipher.
//
//   @param [in]  InBlock    Input block.
//   @param [out] OutBlock   Output blocked.
// **/
// VOID
// ArmAesInvert (
//   IN  AES_BLOCK CONST  *InBlock,
//   OUT AES_BLOCK        *OutBlock
//   );
ASM_FUNC(ArmAesInvert)
    ld1      {v0.4s}, [x1]
    aesimc   v1.16b, v0.16b
    st1      {v1.4s}, [x0]
    ret

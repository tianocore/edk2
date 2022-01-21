/** @file
  Arm(32) AES implementation.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AsmMacroIoLibV8.h>

.fpu crypto-neon-fp-armv8

// Generic notes:
// - In Arm32, the AESE/AESD/AESMC/AESIMC instructions are using registers
//   as qX
// - For some CPUs, the latency of VLD1  is 6, thus the unfolding.
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
    vld1.8   {q0}, [r2]
    cmp      r1, #12
    beq      0f

    // Rounds = 10 or 14. Start loading the expanded key.
    vld1.8   {q4}, [r0]!
    vld1.8   {q1}, [r0]!
    vld1.8   {q2}, [r0]!
    adds     r1, r1, #1
    b        2f

    // Rounds = 12. Start loading the expanded key.
0:  vld1.8   {q2}, [r0]!
    vld1.8   {q3}, [r0]!
    vld1.8   {q4}, [r0]!
    subs     r1, r1, #1
    b        3f

    // Start of the loop (unfolded for 4 rounds).
1:  vld1.8   {q4}, [r0]!
    aese.8   q0, q1
    aesmc.8  q0, q0
3:  vld1.8   {q1}, [r0]!
    aese.8   q0, q2
    aesmc.8  q0, q0
    vld1.8   {q2}, [r0]!
    aese.8   q0, q3
    aesmc.8  q0, q0
2:  subs     r1, r1, #4
    vld1.8   {q3}, [r0]!
    aese.8   q0, q4
    aesmc.8  q0, q0
    bpl      1b

    // Final round.
    aese.8   q0, q1
    veor     q0, q0, q2
    vst1.8   {q0}, [r3]
    bx       lr

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
    vld1.8   {q0}, [r2]
    cmp      r1, #12
    beq      0f

    // Rounds = 10 or 14. Start loading the expanded key.
    vld1.8   {q4}, [r0]!
    vld1.8   {q1}, [r0]!
    vld1.8   {q2}, [r0]!
    adds     r1, r1, #1
    b        2f

    // Rounds = 12. Start loading the expanded key.
0:  vld1.8   {q2}, [r0]!
    vld1.8   {q3}, [r0]!
    vld1.8   {q4}, [r0]!
    subs     r1, r1, #1
    b        3f

    // Start of the loop (unfolded for 4 rounds).
1:  vld1.8   {q4}, [r0]!
    aesd.8   q0, q1
    aesimc.8 q0, q0
3:  vld1.8   {q1}, [r0]!
    aesd.8   q0, q2
    aesimc.8 q0, q0
    vld1.8   {q2}, [r0]!
    aesd.8   q0, q3
    aesimc.8 q0, q0
2:  subs     r1, r1, #4
    vld1.8   {q3}, [r0]!
    aesd.8   q0, q4
    aesimc.8 q0, q0
    bpl      1b

    // Final round.
    aesd.8   q0, q1
    veor     q0, q0, q2
    vst1.8   {q0}, [r3]
    bx       lr

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
    vdup.32    q1, r0
    vmov.i64   q0, #0
    aese.8   q0, q1
    vmov.f32  r0, s0
    bx       lr

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
    vld1.8   {q0}, [r1]
    aesimc.8 q1, q0
    vst1.8   {q1}, [r0]
    bx       lr

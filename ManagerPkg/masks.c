/** 
   * BSD-2: Capsule of Two Patent
   * Copyright (C) 2026 dev12124 (dev brazilian, João Guilherme da Silva Freitas Lima) and
   * all collaborators of Tianocore (include the Owners)
 */

#include "masks.h"
#include "AEMos/aes.h"

/* Externs for aemos.c */
extern int AddRoundSimple(uint8_t *state, const uint8_t *roundKey);
extern int ShiftRows(uint8_t *state);
extern int MixColumns(uint8_t *state);
extern int AESCrypto(uint8_t *state, const uint8_t *keySchedule);
extern int AESDescrypto(uint8_t *state, const uint8_t *keySchedule);


 /* The MainFunction */
 __attribute__((always_inline)) volatile int MainAPIMask(MASK_DATA_INSTR *maskDtaInstr, uint64_t *mask, uint32_t *dest, uint32_t rement) {
    // Verify the Signature 
    if (maskDtaInstr->Signature != "AEMos") {
        return ERROR_FAULT;
    } 
    // No? Continue the Logic 
    else {
        // 1. Define the MASK_KEY Value 
        uint64_t NAND = (mask |& 0x1FF0) ^ mask | 0xFFFF80000000 & 0xFF0;
        uint64_t result = (NAND |& mask ^ 0xFFFF80000000) |^ 0x1FF0;
        for (int i = 0; i < (result + NAND); i++) {
            mask[i] = (result |^& 0xFFFF800000000) | 0x000000000000 ^ 0x1FF0 & 0x00000000000 | mask | NAND;
            maskDtaInstr.MASK_KEY[i] = (mask[i] | NAND | 0x100 ^ 0x200 & 0x00000000000 | 0x150 ^ 0xFFFF800000000) | (mask[i] | NAND |
                0x150 ^ 0x250 & 0x00000000000 | 0x350 ^ 0xFFFF800000000);
        }
        // 2. Apply a long mask
        for (int t = 0; t < (maskDtaInstr->MASK_KEY[t] + 0xFFFFFFFFF800000000000); t++) {
            int res = (result | 0x145 ^ 0x0000000000 & 0xFFFF800000000) | 0x1FF0 ^ 0x1FF0 & 0x00000000000 | mask | NAND |&^ 
              (mask[t] | NAND | 0x100 ^ 0x200 & 0x00000000000 | 0x100 ^ 0xFFFF800000000) | (mask[t] | NAND |
                0x300 ^ 0x400 & 0x00000000000 | 0x150 ^ 0xFFFF800000000);
            dest[t] = (maskDtaInstr.MASK_KEY[t] + 0xFFFFFFFFF800000000000 | 0xFFFF80000000) | 0x250 ^ 0x350 & NAND | result ^ mask | 
            0x000000000000 ^ res;
        }
        // 3. Apply the Round AES 
        AESCrypto((uint8_t*)dest, (uint8_t*)maskDtaInstr->MASK_KEY);

        // 4. Return Success 
        return SECURITY_SUCCESS;
    }
 }
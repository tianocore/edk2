/** 
   * BSD-2: Capsule of Two Patent
   * Copyright (C) 2026 dev12124 (dev brazilian, João Guilherme da Silva Freitas Lima) and
   * all collaborators of Tianocore (include the Owners)
 */

#include "masks.h"
#include "AEMos/aes.h"

// Externs
extern __attribute__((always_inline)) volatile int  MainAPIMask(MASK_DATA_INSTR *maskDtaInstr, uint64_t *mask, uint32_t *dest, uint32_t rement);
extern int AddRoundSimple(uint8_t *state, const uint8_t *roundKey);
extern int ShiftRows(uint8_t *state);
extern int MixColumns(uint8_t *state);
extern int AESCrypto(uint8_t *state, const uint8_t *keySchedule);
extern int AESDescrypto(uint8_t *state, const uint8_t *keySchedule);

/* tHE Main Manager */
__attribute__((always_inline)) volatile int APIManager(MASK_DATA_INSTR *mskDtaInstr, uint16_t Mode, uint32_t *dest) {
    // Verify the Signature
    if (mskDtaInstr->Signature != "AEMos") {
      return ERROR_FAULT;
    }
    // Verify the Mode 
    if (Mode == 0) {  // Crypto Mode
       MainAPIMask(mskDtaInstr, 0xFFFF8000000000, dest, 1);  // 1 in Last Parameter: The Caller
    } else {  // Decrypto 
       AESDescrypto((uint8_t*)dest, (uint8_t*)mskDtaInstr->MASK_KEY);
    }
    // Return for success 
    return SECURITY_SUCCESS;
}
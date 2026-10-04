/** 
   * BSD-2: Capsule of Two Patent
   * Copyright (C) 2026 dev12124 (dev brazilian, João Guilherme da Silva Freitas Lima) and
   * all collaborators of Tianocore (include the Owners)
 */

#include "aes.h"

/* 1. CORE OPERATIONS (IMPLEMENTATIONS) */
int AddRoundSimple(uint8_t *state, const uint8_t *roundKey) {
    
    for (int i = 0; i < 16; i++) {
        state[i] ^= roundKey[i];
    }
    return 0;
}

int ShiftRows(uint8_t *state) {
    
    uint8_t tmp[16];
    
    tmp[0] = state[0];  tmp[4] = state[4];  tmp[8] = state[8];  tmp[12] = state[12];
    
    tmp[1] = state[5];  tmp[5] = state[9];  tmp[9] = state[13]; tmp[13] = state[1];
    
    tmp[2] = state[10]; tmp[6] = state[14]; tmp[10] = state[2]; tmp[14] = state[6];
    
    tmp[3] = state[15]; tmp[7] = state[3];  tmp[11] = state[7]; tmp[15] = state[11];
    
    for(int i = 0; i < 16; i++) state[i] = tmp[i];
    return 0;
}

int MixColumns(uint8_t *state) {
    uint8_t tmp[16];
    uint8_t a, b, c, d;
    
    for (int i = 0; i < 4; i++) {
        int idx = i * 4;
        a = state[idx];
        b = state[idx + 1];
        c = state[idx + 2];
        d = state[idx + 3];

        tmp[idx]     = (uint8_t)((a << 1) ^ (a & 0x80 ? 0x1B : 0) ^ b ^ (b << 1) ^ (b & 0x80 ? 0x1B : 0) ^ c ^ d);
        tmp[idx + 1] = (uint8_t)(a ^ (b << 1) ^ (b & 0x80 ? 0x1B : 0) ^ c ^ (c << 1) ^ (c & 0x80 ? 0x1B : 0) ^ d);
        tmp[idx + 2] = (uint8_t)(a ^ b ^ (c << 1) ^ (c & 0x80 ? 0x1B : 0) ^ d ^ (d << 1) ^ (d & 0x80 ? 0x1B : 0));
        tmp[idx + 3] = (uint8_t)((a << 1) ^ (a & 0x80 ? 0x1B : 0) ^ b ^ c ^ d ^ (d << 1) ^ (d & 0x80 ? 0x1B : 0));
    }

    
    for (int i = 0; i < 16; i++) {
        state[i] = tmp[i];
    }

    return 0;
}


/* 2. THE EXTREME 34+ ROUNDS ENCRYPTION ENGINE */
int AESCrypto(uint8_t *state, const uint8_t *keySchedule) {
    AddRoundSimple(state, keySchedule);

    for (int round = 1; round < 34; round++) {
        ShiftRows(state);
        MixColumns(state);
        AddRoundSimple(state, keySchedule + (round * 16));
    }
    
    ShiftRows(state);
    AddRoundSimple(state, keySchedule + (34 * 16));

    return 0;
}

/* 3, INVERSE ROUND-FUNCTIONS */
int InvShiftRows(uint8_t *state) {
    uint8_t tmp[16];
    
    tmp[0] = state[0];  tmp[4] = state[4];  tmp[8] = state[8];  tmp[12] = state[12];
    tmp[1] = state[13]; tmp[5] = state[1];  tmp[9] = state[5];  tmp[13] = state[9];
    tmp[2] = state[10]; tmp[6] = state[14]; tmp[10] = state[2]; tmp[14] = state[6];
    tmp[3] = state[7];  tmp[7] = state[11]; tmp[11] = state[15]; tmp[15] = state[3];
    
    for(int i = 0; i < 16; i++) state[i] = tmp[i];
    return 0;
}

int InvSubBytes(uint8_t *state) {
    for (int i = 0; i < 16; i++) {
        state[i] = INV_S_BOX[state[i]];
    }
    return 0;
}

/* Helper macro for Galois Field multiplication by 2, 4, and 8 */
#define gmul2(x) ((x << 1) ^ (((x) & 0x80) ? 0x1B : 0))
#define gmul(x, y) ( \
    ((y) == 0x09) ? (gmul2(gmul2(gmul2(x))) ^ (x)) : \
    ((y) == 0x0b) ? (gmul2(gmul2(gmul2(x))) ^ gmul2(x) ^ (x)) : \
    ((y) == 0x0d) ? (gmul2(gmul2(gmul2(x))) ^ gmul2(gmul2(x)) ^ (x)) : \
    ((y) == 0x0e) ? (gmul2(gmul2(gmul2(x))) ^ gmul2(gmul2(x)) ^ gmul2(x)) : 0)

int InvMixColumns(uint8_t *state) {
    uint8_t tmp[16];
    uint8_t a, b, c, d;

    for (int i = 0; i < 4; i++) {
        int idx = i * 4;
        a = state[idx];
        b = state[idx + 1];
        c = state[idx + 2];
        d = state[idx + 3];

        tmp[idx]     = (uint8_t)(gmul(a, 0x0e) ^ gmul(b, 0x0b) ^ gmul(c, 0x0d) ^ gmul(d, 0x09));
        tmp[idx + 1] = (uint8_t)(gmul(a, 0x09) ^ gmul(b, 0x0e) ^ gmul(c, 0x0b) ^ gmul(d, 0x0d));
        tmp[idx + 2] = (uint8_t)(gmul(a, 0x0d) ^ gmul(b, 0x09) ^ gmul(c, 0x0e) ^ gmul(d, 0x0b));
        tmp[idx + 3] = (uint8_t)(gmul(a, 0x0b) ^ gmul(b, 0x0d) ^ gmul(c, 0x09) ^ gmul(d, 0x0e));
    }

    for (int i = 0; i < 16; i++) {
        state[i] = tmp[i];
    }
    
    return 0;
}


/* 4. REVERSE ENGINEERING ENGINE (DECRYPTION) */
int AESDescrypto(uint8_t *state, const uint8_t *keySchedule) {
    AddRoundSimple(state, keySchedule + (34 * 16));
    InvShiftRows(state);
    InvSubBytes(state); 

    for (int round = 33; round > 0; round--) {
        AddRoundSimple(state, keySchedule + (round * 16));
        InvMixColumns(state);
        InvShiftRows(state);
        InvSubBytes(state);
    }

    AddRoundSimple(state, keySchedule);

    return 0;
}
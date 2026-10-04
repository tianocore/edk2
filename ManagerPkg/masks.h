
/** 
   * BSD-2: Capsule of Two Patent
   * Copyright (C) 2026 dev12124 (dev brazilian, João Guilherme da Silva Freitas Lima) and
   * all collaborators of Tianocore (include the Owners)
 */


#ifndef MASKS_H 
#define MASKS_H

// In cases of error, return 
#define ERROR_FAULT 100

// In cases of success, return 
#define SECURITY_SUCCESS 0
// A Basic Structure 
 typedef struct {
    uint16_t MASK_KEY[512]; // The Key based in Offsets 
    uint16_t Lenght; // The Returned Size of Key in Bytes 
    uint32_t segm_code; // The Code of Segment, the Ring 0 (the Firmware and Bootloader) no masked 
    uint32_t CODE; // The Code of Next Data (used for imprevisible masks)
    uint32_t DATA; // The Data have 2 Bits: the Bit 0 is the Destinatary (the Masked), and the Bit 1 is the Rementent (the Masker)
    uint8_t Signature[5]; // "AEMos"
 } __attribute__((packed, aligned(4096))) MASK_DATA_INSTR;

 #endif
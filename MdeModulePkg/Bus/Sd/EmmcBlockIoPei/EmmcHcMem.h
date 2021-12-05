/** @file

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EMMC_PEIM_MEM_H_
#define _EMMC_PEIM_MEM_H_

#define EMMC_PEIM_MEM_BIT(a)  ((UINTN)(1 << (a)))

#define EMMC_PEIM_MEM_BIT_IS_SET(Data, Bit)   \
          ((BOOLEAN)(((Data) & EMMC_PEIM_MEM_BIT(Bit)) == EMMC_PEIM_MEM_BIT(Bit)))

typedef struct _EMMC_PEIM_MEM_BLOCK EMMC_PEIM_MEM_BLOCK;

struct _EMMC_PEIM_MEM_BLOCK {
  UINT8                  *Bits;     // Bit array to record which unit is allocated
  UINTN                  BitsLen;
  UINT8                  *Buf;
  UINT8                  *BufHost;
  UINTN                  BufLen;    // Memory size in bytes
  VOID                   *Mapping;
  EMMC_PEIM_MEM_BLOCK    *Next;
};

typedef struct _EMMC_PEIM_MEM_POOL {
  EMMC_PEIM_MEM_BLOCK    *Head;
} EMMC_PEIM_MEM_POOL;

//
// Memory allocation unit, note that the value must meet EMMC spec alignment requirement.
//
#define EMMC_PEIM_MEM_UNIT  128

#define EMMC_PEIM_MEM_UNIT_MASK      (EMMC_PEIM_MEM_UNIT - 1)
#define EMMC_PEIM_MEM_DEFAULT_PAGES  16

#define EMMC_PEIM_MEM_ROUND(Len)  (((Len) + EMMC_PEIM_MEM_UNIT_MASK) & (~EMMC_PEIM_MEM_UNIT_MASK))

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define EMMC_PEIM_NEXT_BIT(Byte, Bit)   \
          do {                \
            (Bit)++;          \
            if ((Bit) > 7) {  \
              (Byte)++;       \
              (Bit) = 0;      \
            }                 \
          } while (0)

#endif

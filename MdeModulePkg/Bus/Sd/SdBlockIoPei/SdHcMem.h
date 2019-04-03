/** @file

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SD_PEIM_MEM_H_
#define _SD_PEIM_MEM_H_

#define SD_PEIM_MEM_BIT(a)          ((UINTN)(1 << (a)))

#define SD_PEIM_MEM_BIT_IS_SET(Data, Bit)   \
          ((BOOLEAN)(((Data) & SD_PEIM_MEM_BIT(Bit)) == SD_PEIM_MEM_BIT(Bit)))

typedef struct _SD_PEIM_MEM_BLOCK SD_PEIM_MEM_BLOCK;

struct _SD_PEIM_MEM_BLOCK {
  UINT8                   *Bits;    // Bit array to record which unit is allocated
  UINTN                   BitsLen;
  UINT8                   *Buf;
  UINT8                   *BufHost;
  UINTN                   BufLen;   // Memory size in bytes
  VOID                    *Mapping;
  SD_PEIM_MEM_BLOCK       *Next;
};

typedef struct _SD_PEIM_MEM_POOL {
  SD_PEIM_MEM_BLOCK       *Head;
} SD_PEIM_MEM_POOL;

//
// Memory allocation unit, note that the value must meet SD spec alignment requirement.
//
#define SD_PEIM_MEM_UNIT           128

#define SD_PEIM_MEM_UNIT_MASK      (SD_PEIM_MEM_UNIT - 1)
#define SD_PEIM_MEM_DEFAULT_PAGES  16

#define SD_PEIM_MEM_ROUND(Len)     (((Len) + SD_PEIM_MEM_UNIT_MASK) & (~SD_PEIM_MEM_UNIT_MASK))

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define SD_PEIM_NEXT_BIT(Byte, Bit)   \
          do {                \
            (Bit)++;          \
            if ((Bit) > 7) {  \
              (Byte)++;       \
              (Bit) = 0;      \
            }                 \
          } while (0)

#endif


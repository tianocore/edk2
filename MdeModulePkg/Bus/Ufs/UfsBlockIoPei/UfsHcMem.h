/** @file

Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UFS_PEIM_MEM_H_
#define _UFS_PEIM_MEM_H_

#define UFS_PEIM_MEM_BIT(a)          ((UINTN)(1 << (a)))

#define UFS_PEIM_MEM_BIT_IS_SET(Data, Bit)   \
          ((BOOLEAN)(((Data) & UFS_PEIM_MEM_BIT(Bit)) == UFS_PEIM_MEM_BIT(Bit)))

typedef struct _UFS_PEIM_MEM_BLOCK UFS_PEIM_MEM_BLOCK;

struct _UFS_PEIM_MEM_BLOCK {
  UINT8                   *Bits;    // Bit array to record which unit is allocated
  UINTN                   BitsLen;
  UINT8                   *Buf;
  UINT8                   *BufHost;
  UINTN                   BufLen;   // Memory size in bytes
  VOID                    *Mapping;
  UFS_PEIM_MEM_BLOCK      *Next;
};

typedef struct _UFS_PEIM_MEM_POOL {
  UFS_PEIM_MEM_BLOCK         *Head;
} UFS_PEIM_MEM_POOL;

//
// Memory allocation unit, note that the value must meet UFS spec alignment requirement.
//
#define UFS_PEIM_MEM_UNIT           128

#define UFS_PEIM_MEM_UNIT_MASK      (UFS_PEIM_MEM_UNIT - 1)
#define UFS_PEIM_MEM_DEFAULT_PAGES  16

#define UFS_PEIM_MEM_ROUND(Len)  (((Len) + UFS_PEIM_MEM_UNIT_MASK) & (~UFS_PEIM_MEM_UNIT_MASK))

//
// Advance the byte and bit to the next bit, adjust byte accordingly.
//
#define UFS_PEIM_NEXT_BIT(Byte, Bit)   \
          do {                \
            (Bit)++;          \
            if ((Bit) > 7) {  \
              (Byte)++;       \
              (Bit) = 0;      \
            }                 \
          } while (0)

#endif


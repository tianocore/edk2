/** @file

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  UINTN                   BufLen;   // Memory size in bytes
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


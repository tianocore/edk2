/** @file
  GCD Operations and data structure used to
  convert from GCD attributes to EFI Memory Map attributes.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _GCD_H_
#define _GCD_H_

//
// GCD Operations
//
#define GCD_MEMORY_SPACE_OPERATION 0x20
#define GCD_IO_SPACE_OPERATION     0x40

#define GCD_ADD_MEMORY_OPERATION             (GCD_MEMORY_SPACE_OPERATION | 0)
#define GCD_ALLOCATE_MEMORY_OPERATION        (GCD_MEMORY_SPACE_OPERATION | 1)
#define GCD_FREE_MEMORY_OPERATION            (GCD_MEMORY_SPACE_OPERATION | 2)
#define GCD_REMOVE_MEMORY_OPERATION          (GCD_MEMORY_SPACE_OPERATION | 3)
#define GCD_SET_ATTRIBUTES_MEMORY_OPERATION  (GCD_MEMORY_SPACE_OPERATION | 4)

#define GCD_ADD_IO_OPERATION                 (GCD_IO_SPACE_OPERATION | 0)
#define GCD_ALLOCATE_IO_OPERATION            (GCD_IO_SPACE_OPERATION | 1)
#define GCD_FREE_IO_OPERATION                (GCD_IO_SPACE_OPERATION | 2)
#define GCD_REMOVE_IO_OPERATION              (GCD_IO_SPACE_OPERATION | 3)

//
// The data structure used to convert from GCD attributes to EFI Memory Map attributes
//
typedef struct {
  UINT64   Attribute;
  UINT64   Capability;
  BOOLEAN  Memory;
} GCD_ATTRIBUTE_CONVERSION_ENTRY;

#endif

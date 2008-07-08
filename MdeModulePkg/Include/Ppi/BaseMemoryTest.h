/** @file
  This file defines Pei memory test PPI used to Perform memory test in PEI phase.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BASE_MEMORY_TEST_H__
#define __BASE_MEMORY_TEST_H__

#define PEI_BASE_MEMORY_TEST_GUID \
  { 0xb6ec423c, 0x21d2, 0x490d, {0x85, 0xc6, 0xdd, 0x58, 0x64, 0xea, 0xa6, 0x74 } }

typedef struct _PEI_BASE_MEMORY_TEST_PPI  PEI_BASE_MEMORY_TEST_PPI;

//
// 4 different test operations
// Ignore op not test memory, Quick and Sparse op test memory quickly, Extensive op test memory detailedly.
//
typedef enum {
  Ignore,
  Quick,
  Sparse,
  Extensive
} PEI_MEMORY_TEST_OP;

/**
  Test a range memory space is ready to read and write.

  @param PeiServices      An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param This             Pointer to Pei memory test PPI instance.
  @param BeginAddress     Beginning of the memory address to be checked.
  @param MemoryLength     Bytes of memory range to be checked.
  @param Operation        Type of memory check operation to be performed.
  @param ErrorAddress     Address which has error when checked.

  @retval EFI_SUCCESS         Memory range pass basic read and write test.
  @retval EFI_DEVICE_ERROR    Memory is not ready to access.
**/
typedef
EFI_STATUS
(EFIAPI *PEI_BASE_MEMORY_TEST) (
  IN  EFI_PEI_SERVICES                   **PeiServices,
  IN PEI_BASE_MEMORY_TEST_PPI            * This,
  IN  EFI_PHYSICAL_ADDRESS               BeginAddress,
  IN  UINT64                             MemoryLength,
  IN  PEI_MEMORY_TEST_OP                 Operation,
  OUT EFI_PHYSICAL_ADDRESS               * ErrorAddress
  );

struct _PEI_BASE_MEMORY_TEST_PPI {
  PEI_BASE_MEMORY_TEST  BaseMemoryTest;
};

extern EFI_GUID gPeiBaseMemoryTestPpiGuid;

#endif

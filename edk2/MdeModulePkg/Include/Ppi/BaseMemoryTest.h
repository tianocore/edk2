/** @file
  Pei memory test PPI used to Perform memory test in PEI phase.

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

typedef enum {
  Ignore,
  Quick,
  Sparse,
  Extensive
} PEI_MEMORY_TEST_OP;

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

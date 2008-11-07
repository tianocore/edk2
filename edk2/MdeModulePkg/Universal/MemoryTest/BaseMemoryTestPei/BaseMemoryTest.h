/** @file
  Internal include file for support of memory test in PEI Phase.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_BASE_MEMORY_TEST_H_
#define _PEI_BASE_MEMORY_TEST_H_

#include <PiPei.h>
#include <Ppi/BaseMemoryTest.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>

#define COVER_SPAN    0x40000
#define TEST_PATTERN  0x5A5A5A5A

/**
  Test base memory.

  @param  PeiServices      An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This             Pointer to this PPI instance.
  @param  BeginAddress     Beginning of the memory address to be checked.
  @param  MemoryLength     Bytes of memory range to be checked.
  @param  Operation        Type of memory check operation to be performed.
  @param  ErrorAddress     Pointer to address of the error memory returned.

  @retval EFI_SUCCESS      Memory test passed.
  @retval EFI_DEVICE_ERROR Memory test failed.

**/    
EFI_STATUS
EFIAPI
BaseMemoryTest (
  IN  EFI_PEI_SERVICES                   **PeiServices,
  IN PEI_BASE_MEMORY_TEST_PPI            *This,
  IN  EFI_PHYSICAL_ADDRESS               BeginAddress,
  IN  UINT64                             MemoryLength,
  IN  PEI_MEMORY_TEST_OP                 Operation,
  OUT EFI_PHYSICAL_ADDRESS               *ErrorAddress
  );

#endif

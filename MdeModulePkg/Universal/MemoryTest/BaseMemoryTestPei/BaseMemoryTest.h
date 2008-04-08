/** @file
  Tiano PEIM to provide a PEI memory test service.

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


//
// Some global define
//
#define COVER_SPAN    0x40000
#define TEST_PATTERN  0x5A5A5A5A

EFI_STATUS
EFIAPI
PeiBaseMemoryTestInit (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  FfsHeader   - TODO: add argument description
  PeiServices - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
BaseMemoryTest (
  IN  EFI_PEI_SERVICES                   **PeiServices,
  IN PEI_BASE_MEMORY_TEST_PPI            *This,
  IN  EFI_PHYSICAL_ADDRESS               BeginAddress,
  IN  UINT64                             MemoryLength,
  IN  PEI_MEMORY_TEST_OP                 Operation,
  OUT EFI_PHYSICAL_ADDRESS               *ErrorAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PeiServices   - TODO: add argument description
  This          - TODO: add argument description
  BeginAddress  - TODO: add argument description
  MemoryLength  - TODO: add argument description
  Operation     - TODO: add argument description
  ErrorAddress  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif

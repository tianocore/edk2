/** @file
  Library class to provide APIs for system memory test.
  
Copyright (c) 2009, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MEMORY_TEST_LIB_H_
#define _MEMORY_TEST_LIB_H_


/**
  Test a system memory range with sparsely sampled memory units.

  This function tests a system memory range, whose memory units
  are sampled sparsely. It leads to quick performance but less
  reliability.

  @param  StartAddress           Start address of the memory range to test.
  @param  Length                 Length of the memory range to test.
  @param  ErrorAddress           Address of the memory where error is encountered.

  @retval RETURN_SUCCESS         The memory range passes test.
  @retval RETURNEFI_DEVICE_ERROR The memory range does not pass test.

**/
RETURN_STATUS
EFIAPI
SparseMemoryTest (
  IN  VOID      *StartAddress,
  IN  UINT64    Length,
  OUT VOID      **ErrorAddress
  );

/**
  Test a system memory range with extensively sampled memory units.

  This function tests a system memory range, whose memory units
  are sampled extensively. Compared with SparseMemoryTest, it achieves
  more reliability and less performance.

  @param  StartAddress           Start address of the memory range to test.
  @param  Length                 Length of the memory range to test.
  @param  ErrorAddress           Address of the memory where error is encountered.

  @retval RETURN_SUCCESS         The memory range passes test.
  @retval RETURNEFI_DEVICE_ERROR The memory range does not pass test.

**/
RETURN_STATUS
EFIAPI
ExtensiveMemoryTest (
  IN  VOID      *StartAddress,
  IN  UINT64    Length,
  OUT VOID      **ErrorAddress
  );

/**
  Test a system memory range with every memory unit checked.

  This function tests a system memory range, whose memory units
  are fully checked. It leads to complete reliability with the
  cost of performance.

  @param  StartAddress           Start address of the memory range to test.
  @param  Length                 Length of the memory range to test.
  @param  ErrorAddress           Address of the memory where error is encountered.

  @retval RETURN_SUCCESS         The memory range passes test.
  @retval RETURNEFI_DEVICE_ERROR The memory range does not pass test.

**/
RETURN_STATUS
EFIAPI
FullMemoryTest (
  IN  VOID      *StartAddress,
  IN  UINT64    Length,
  OUT VOID      **ErrorAddress
  );

#endif

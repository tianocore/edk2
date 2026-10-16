/** @file
  This is a sample to demonstrate the usage of the Unit Test Library that
  supports the PEI, DXE, SMM, UEFI Shell, and host execution environments.
  This test case generates an exception. For some host-based environments, this
  is a fatal condition that terminates the unit tests and no additional test
  cases are executed. On other environments, this condition may be report a unit
  test failure and continue with additional unit tests.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Uefi.h>
#include <Library/UnitTestLib.h>

/**
  Sample unit test the triggers an unexpected exception

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
GenerateUnexpectedException (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  asm volatile ("udf #0");

  return UNIT_TEST_PASSED;
}

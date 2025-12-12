/** @file
  Provides a unit test result report.  This allows new result output formats to
  be easily customized.

  Copyright (c) Microsoft Corporation.<BR>
  Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UNIT_TEST_RESULT_REPORT_LIB_H__
#define __UNIT_TEST_RESULT_REPORT_LIB_H__

#include <UnitTestFrameworkTypes.h>

/**
Method to produce the Unit Test run results

@retval Success
**/
EFI_STATUS
EFIAPI
OutputUnitTestFrameworkReport (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  );

#endif

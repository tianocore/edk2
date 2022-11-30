/** @file
  This header file describes a library that contains functions to save and
  restore unit test internal state, in case the test needs to pause and resume
  (eg. a reboot-based test).

  Copyright (c) Microsoft Corporation.<BR>
  Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UNIT_TEST_PERSISTENCE_LIB_H_
#define _UNIT_TEST_PERSISTENCE_LIB_H_

#include <UnitTestFrameworkTypes.h>

#define UNIT_TEST_PERSISTENCE_LIB_VERSION  1

/**
  Determines whether a persistence cache already exists for
  the given framework.

  @param[in]  FrameworkHandle   A pointer to the framework that is being persisted.

  @retval     TRUE
  @retval     FALSE   Cache doesn't exist or an error occurred.

**/
BOOLEAN
EFIAPI
DoesCacheExist (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  );

/**
  Will save the data associated with an internal Unit Test Framework
  state in a manner that can persist a Unit Test Application quit or
  even a system reboot.

  @param[in]  FrameworkHandle   A pointer to the framework that is being persisted.
  @param[in]  SaveData          A pointer to the buffer containing the serialized
                                framework internal state.

  @retval     EFI_SUCCESS   Data is persisted and the test can be safely quit.
  @retval     Others        Data is not persisted and test cannot be resumed upon exit.

**/
EFI_STATUS
EFIAPI
SaveUnitTestCache (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  IN UNIT_TEST_SAVE_HEADER       *SaveData
  );

/**
  Will retrieve any cached state associated with the given framework.
  Will allocate a buffer to hold the loaded data.

  @param[in]  FrameworkHandle   A pointer to the framework that is being persisted.
  @param[in]  SaveData          A pointer pointer that will be updated with the address
                                of the loaded data buffer.

  @retval     EFI_SUCCESS       Data has been loaded successfully and SaveData is updated
                                with a pointer to the buffer.
  @retval     Others            An error has occurred and no data has been loaded. SaveData
                                is set to NULL.

**/
EFI_STATUS
EFIAPI
LoadUnitTestCache (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  OUT UNIT_TEST_SAVE_HEADER       **SaveData
  );

#endif

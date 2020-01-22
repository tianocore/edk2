/** @file
  This is an instance of the Unit Test Persistence Lib that does nothing.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UnitTestPersistenceLib.h>

/**
  Determines whether a persistence cache already exists for
  the given framework.

  @param[in]  FrameworkHandle  A pointer to the framework that is being persisted.

  @retval  TRUE
  @retval  FALSE  Cache doesn't exist or an error occurred.

**/
BOOLEAN
EFIAPI
DoesCacheExist (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  )
{
  return FALSE;
}

/**
  Will save the data associated with an internal Unit Test Framework
  state in a manner that can persist a Unit Test Application quit or
  even a system reboot.

  @param[in]  FrameworkHandle  A pointer to the framework that is being persisted.
  @param[in]  SaveData         A pointer to the buffer containing the serialized
                               framework internal state.

  @retval  EFI_SUCCESS  Data is persisted and the test can be safely quit.
  @retval  Others       Data is not persisted and test cannot be resumed upon exit.

**/
EFI_STATUS
EFIAPI
SaveUnitTestCache (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  IN UNIT_TEST_SAVE_HEADER       *SaveData
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Will retrieve any cached state associated with the given framework.
  Will allocate a buffer to hold the loaded data.

  @param[in]  FrameworkHandle  A pointer to the framework that is being persisted.
  @param[in]  SaveData         A pointer pointer that will be updated with the address
                               of the loaded data buffer.

  @retval  EFI_SUCCESS  Data has been loaded successfully and SaveData is updated
                        with a pointer to the buffer.
  @retval  Others       An error has occurred and no data has been loaded. SaveData
                        is set to NULL.

**/
EFI_STATUS
EFIAPI
LoadUnitTestCache (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle,
  OUT UNIT_TEST_SAVE_HEADER       **SaveData
  )
{
  return EFI_UNSUPPORTED;
}

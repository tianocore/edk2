/** @file MockSynchronizationLib.cpp
  Google Test mocks for SynchronizationLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockSynchronizationLib.h>

MOCK_INTERFACE_DEFINITION (MockSynchronizationLib);

MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, GetSpinLockProperties, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, InitializeSpinLock, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, AcquireSpinLock, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, AcquireSpinLockOrFail, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, ReleaseSpinLock, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, InterlockedIncrement, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, InterlockedDecrement, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, InterlockedCompareExchange16, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, InterlockedCompareExchange32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, InterlockedCompareExchange64, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSynchronizationLib, InterlockedCompareExchangePointer, 3, EFIAPI);

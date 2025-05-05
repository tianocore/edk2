/** @file MockSynchronizationLib.h
  Google Test mocks for the synchronization Library

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SYNCHRONIZATION_LIB_H_
#define MOCK_SYNCHRONIZATION_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/SynchronizationLib.h>
}

struct MockSynchronizationLib {
  MOCK_INTERFACE_DECLARATION (MockSynchronizationLib);

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    GetSpinLockProperties,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    SPIN_LOCK *,
    InitializeSpinLock,
    (
     OUT SPIN_LOCK  *SpinLock
    )
    );

  MOCK_FUNCTION_DECLARATION (
    SPIN_LOCK *,
    AcquireSpinLock,
    (
     IN OUT SPIN_LOCK  *SpinLock
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    AcquireSpinLockOrFail,
    (
     IN OUT SPIN_LOCK  *SpinLock
    )
    );

  MOCK_FUNCTION_DECLARATION (
    SPIN_LOCK *,
    ReleaseSpinLock,
    (
     IN OUT SPIN_LOCK  *SpinLock
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    InterlockedIncrement,
    (
     IN volatile UINT32  *Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    InterlockedDecrement,
    (
     IN volatile UINT32  *Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    InterlockedCompareExchange16,
    (
     IN OUT volatile UINT16  *Value,
     IN UINT16           CompareValue,
     IN UINT16           ExchangeValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    InterlockedCompareExchange32,
    (
     IN OUT  volatile UINT32  *Value,
     IN      UINT32           CompareValue,
     IN      UINT32           ExchangeValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    InterlockedCompareExchange64,
    (
     IN OUT  volatile UINT64  *Value,
     IN      UINT64           CompareValue,
     IN      UINT64           ExchangeValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    InterlockedCompareExchangePointer,
    (
     IN OUT  VOID            *volatile  *Value,
     IN      VOID            *CompareValue,
     IN      VOID            *ExchangeValue
    )
    );
};

#endif //MOCK_SYNCHRONIZATION_LIB_H_

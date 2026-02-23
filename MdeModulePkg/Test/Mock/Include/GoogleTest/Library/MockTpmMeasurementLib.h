/** @file MockTpmMeasurementLib.h
  Google Test mocks for TpmMeasurementLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_TPM_MEASUREMENT_LIB_H_
#define MOCK_TPM_MEASUREMENT_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Library/TpmMeasurementLib.h>
}

struct MockTpmMeasurementLib {
  MOCK_INTERFACE_DECLARATION (MockTpmMeasurementLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    TpmMeasureAndLogData,
    (
     IN UINT32  PcrIndex,
     IN UINT32  EventType,
     IN VOID    *EventLog,
     IN UINT32  LogLen,
     IN VOID    *HashData,
     IN UINT64  HashDataLen
    )
    );
};

#endif // MOCK_TPM_MEASUREMENT_LIB_H_

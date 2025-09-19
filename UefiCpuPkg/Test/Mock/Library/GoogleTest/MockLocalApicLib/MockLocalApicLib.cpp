/** @file MockLocalApicLib.cpp
  Google Test mocks for LocalApicLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockLocalApicLib.h>

MOCK_INTERFACE_DEFINITION (MockLocalApicLib);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetLocalApicBaseAddress, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SetLocalApicBaseAddress, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicMode, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SetApicMode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetInitialApicId, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicId, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicVersion, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendFixedIpi, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendFixedIpiAllExcludingSelf, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendSmiIpi, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendSmiIpiAllExcludingSelf, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendInitIpi, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendInitIpiAllExcludingSelf, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendStartupIpiAllExcludingSelf, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendInitSipiSipi, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendInitSipiSipiAllExcludingSelf, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, InitializeLocalApicSoftwareEnable, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, ProgramVirtualWireMode, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, DisableLvtInterrupts, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicTimerInitCount, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicTimerCurrentCount, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, InitializeApicTimer, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicTimerState, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, EnableApicTimerInterrupt, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, DisableApicTimerInterrupt, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicTimerInterruptState, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, SendApicEoi, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicMsiAddress, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetApicMsiValue, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetProcessorLocationByApicId, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockLocalApicLib, GetProcessorLocation2ByApicId, 7, EFIAPI);

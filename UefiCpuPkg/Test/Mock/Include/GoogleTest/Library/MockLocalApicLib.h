/** @file MockLocalApicLib.h
  Google Test mocks for LocalApicLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_LOCAL_APIC_LIB_H_
#define MOCK_LOCAL_APIC_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Library/LocalApicLib.h>
}

struct MockLocalApicLib {
  MOCK_INTERFACE_DECLARATION (MockLocalApicLib);

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    GetLocalApicBaseAddress,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SetLocalApicBaseAddress,
    (
     IN UINTN  BaseAddress
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINTN,
    GetApicMode,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SetApicMode,
    (
     IN UINTN  ApicMode
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    GetInitialApicId,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    GetApicId,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    GetApicVersion,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendFixedIpi,
    (
     IN UINT32  ApicId,
     IN UINT8   Vector
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendFixedIpiAllExcludingSelf,
    (
     IN UINT8  Vector
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendSmiIpi,
    (
     IN UINT32  ApicId
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendSmiIpiAllExcludingSelf,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendInitIpi,
    (
     IN UINT32  ApicId
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendInitIpiAllExcludingSelf,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendStartupIpiAllExcludingSelf,
    (
     IN UINT32  StartupRoutine
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendInitSipiSipi,
    (
     IN UINT32  ApicId,
     IN UINT32  StartupRoutine
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendInitSipiSipiAllExcludingSelf,
    (
     IN UINT32  StartupRoutine
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    InitializeLocalApicSoftwareEnable,
    (
     IN BOOLEAN  Enable
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    ProgramVirtualWireMode,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    DisableLvtInterrupts,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    GetApicTimerInitCount,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    GetApicTimerCurrentCount,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    InitializeApicTimer,
    (
     IN UINTN    DivideValue,
     IN UINT32   InitCount,
     IN BOOLEAN  PeriodicMode,
     IN UINT8    Vector
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    GetApicTimerState,
    (
     OUT UINTN    *DivideValue  OPTIONAL,
     OUT BOOLEAN  *PeriodicMode  OPTIONAL,
     OUT UINT8    *Vector  OPTIONAL
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    EnableApicTimerInterrupt,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    DisableApicTimerInterrupt,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    GetApicTimerInterruptState,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    SendApicEoi,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    GetApicMsiAddress,
    (
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT64,
    GetApicMsiValue,
    (
     IN UINT8    Vector,
     IN UINTN    DeliveryMode,
     IN BOOLEAN  LevelTriggered,
     IN BOOLEAN  AssertionLevel
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    GetProcessorLocationByApicId,
    (
     IN  UINT32  InitialApicId,
     OUT UINT32  *Package  OPTIONAL,
     OUT UINT32  *Core    OPTIONAL,
     OUT UINT32  *Thread  OPTIONAL
    )
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    GetProcessorLocation2ByApicId,
    (
     IN  UINT32  InitialApicId,
     OUT UINT32  *Package  OPTIONAL,
     OUT UINT32  *Die      OPTIONAL,
     OUT UINT32  *Tile     OPTIONAL,
     OUT UINT32  *Module   OPTIONAL,
     OUT UINT32  *Core     OPTIONAL,
     OUT UINT32  *Thread   OPTIONAL
    )
    );
};

#endif //MOCK_LOCAL_APIC_LIB_H_

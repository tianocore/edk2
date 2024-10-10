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
    VOID,
    GetProcessorLocationByApicId,
    (
     IN  UINT32  InitialApicId,
     OUT UINT32  *Package  OPTIONAL,
     OUT UINT32  *Core    OPTIONAL,
     OUT UINT32  *Thread  OPTIONAL
    )
    );
};

#endif //MOCK_LOCAL_APIC_LIB_H_

/** @file MockPciLib.h
  Google Test mocks for PciLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PCI_LIB_H_
#define MOCK_PCI_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PciLib.h>
}

struct MockPciLib {
  MOCK_INTERFACE_DECLARATION (MockPciLib);

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciRead8,
    (
     IN      UINTN  Address
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciWrite8,
    (
     IN      UINTN  Address,
     IN      UINT8  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciOr8,
    (
     IN      UINTN  Address,
     IN      UINT8  OrData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciAnd8,
    (
     IN      UINTN  Address,
     IN      UINT8  AndData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciRead16,
    (
     IN      UINTN  Address
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciWrite16,
    (
     IN      UINTN   Address,
     IN      UINT16  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciOr16,
    (
     IN      UINTN   Address,
     IN      UINT16  OrData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciAnd16,
    (
     IN      UINTN   Address,
     IN      UINT16  AndData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciRead32,
    (
     IN      UINTN   Address
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciWrite32,
    (
     IN      UINTN   Address,
     IN      UINT32  Value
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciOr32,
    (
     IN      UINTN   Address,
     IN      UINT32  OrData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciAnd32,
    (
     IN      UINTN   Address,
     IN      UINT32  AndData
    )
    );
};

#endif

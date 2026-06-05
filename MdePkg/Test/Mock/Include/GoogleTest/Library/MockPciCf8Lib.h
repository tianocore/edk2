/** @file MockPciCf8Lib.h
  Google Test mocks for PciCf8Lib

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_IDS_LIB_H_
#define MOCK_IDS_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/PciCf8Lib.h>
}

struct MockPciCf8Lib {
  MOCK_INTERFACE_DECLARATION (MockPciCf8Lib);
  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PciCf8RegisterForRuntimeAccess,
    (
     IN UINTN  Address
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8Read8,
    (
     IN UINTN  Address
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8Write8,
    (
     IN      UINTN  Address,
     IN      UINT8  Value
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8Or8,
    (
     IN      UINTN  Address,
     IN      UINT8  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8And8,
    (
     IN      UINTN  Address,
     IN      UINT8  AndData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8AndThenOr8,
    (
     IN      UINTN  Address,
     IN      UINT8  AndData,
     IN      UINT8  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8BitFieldRead8,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8BitFieldWrite8,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT8  Value
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8BitFieldOr8,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT8  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8BitFieldAnd8,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT8  AndData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciCf8BitFieldAndThenOr8,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT8  AndData,
     IN      UINT8  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8Read16,
    (
     IN UINTN  Address
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8Write16,
    (
     IN      UINTN  Address,
     IN      UINT16  Value
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8Or16,
    (
     IN      UINTN  Address,
     IN      UINT16  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8And16,
    (
     IN      UINTN  Address,
     IN      UINT16  AndData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8AndThenOr16,
    (
     IN      UINTN  Address,
     IN      UINT16  AndData,
     IN      UINT16  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8BitFieldRead16,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8BitFieldWrite16,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT16  Value
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8BitFieldOr16,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT16  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8BitFieldAnd16,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT16  AndData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciCf8BitFieldAndThenOr16,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT16  AndData,
     IN      UINT16  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8Read32,
    (
     IN UINTN  Address
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8Write32,
    (
     IN      UINTN  Address,
     IN      UINT32  Value
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8Or32,
    (
     IN      UINTN  Address,
     IN      UINT32  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8And32,
    (
     IN      UINTN  Address,
     IN      UINT32  AndData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8AndThenOr32,
    (
     IN      UINTN  Address,
     IN      UINT32  AndData,
     IN      UINT32  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8BitFieldRead32,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8BitFieldWrite32,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT32  Value
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8BitFieldOr32,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT32  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8BitFieldAnd32,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT32  AndData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciCf8BitFieldAndThenOr32,
    (
     IN      UINTN  Address,
     IN      UINTN  StartBit,
     IN      UINTN  EndBit,
     IN      UINT32  AndData,
     IN      UINT32  OrData
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINTN,
    PciCf8ReadBuffer,
    (
     IN      UINTN  StartAddress,
     IN      UINTN  Size,
     OUT     VOID   *Buffer
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINTN,
    PciCf8WriteBuffer,
    (
     IN      UINTN  StartAddress,
     IN      UINTN  Size,
     IN      VOID   *Buffer
    )
    );
};

#endif

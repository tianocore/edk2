/** @file MockPciExpressLib.h
  Google Test mocks for PciExpressLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PCI_EXPRESS_LIB_H_
#define MOCK_PCI_EXPRESS_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PciExpressLib.h>
}

struct MockPciExpressLib {
  MOCK_INTERFACE_DECLARATION (MockPciExpressLib);

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PciExpressRegisterForRuntimeAccess,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressRead8,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressWrite8,
    (IN UINTN  Address,
     IN UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressOr8,
    (IN UINTN  Address,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressAnd8,
    (IN UINTN  Address,
     IN UINT8  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressAndThenOr8,
    (IN UINTN  Address,
     IN UINT8  AndData,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressBitFieldRead8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressBitFieldWrite8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressBitFieldOr8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressBitFieldAnd8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    PciExpressBitFieldAndThenOr8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  AndData,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressRead16,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressWrite16,
    (IN UINTN   Address,
     IN UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressOr16,
    (IN UINTN   Address,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressAnd16,
    (IN UINTN   Address,
     IN UINT16  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressAndThenOr16,
    (IN UINTN   Address,
     IN UINT16  AndData,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressBitFieldRead16,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressBitFieldWrite16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressBitFieldOr16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressBitFieldAnd16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    PciExpressBitFieldAndThenOr16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  AndData,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressRead32,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressWrite32,
    (IN UINTN   Address,
     IN UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressOr32,
    (IN UINTN   Address,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressAnd32,
    (IN UINTN   Address,
     IN UINT32  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressAndThenOr32,
    (IN UINTN   Address,
     IN UINT32  AndData,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressBitFieldRead32,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressBitFieldWrite32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressBitFieldOr32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressBitFieldAnd32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    PciExpressBitFieldAndThenOr32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  AndData,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    PciExpressReadBuffer,
    (IN  UINTN  StartAddress,
     IN  UINTN  Size,
     OUT VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    PciExpressWriteBuffer,
    (IN UINTN  StartAddress,
     IN UINTN  Size,
     IN VOID   *Buffer)
    );
};

#endif // MOCK_PCI_EXPRESS_LIB_H_

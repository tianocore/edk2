/** @file MockSerialPortLib.h
  Google Test mocks for SerialPortLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SERIAL_PORT_LIB_H_
#define MOCK_SERIAL_PORT_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/SerialPortLib.h>
}

struct MockSerialPortLib {
  MOCK_INTERFACE_DECLARATION (MockSerialPortLib);

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    SerialPortInitialize,
    (

    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINTN,
    SerialPortWrite,
    (
     IN UINT8  *Buffer,
     IN UINTN  NumberOfBytes
    )
    );
  MOCK_FUNCTION_DECLARATION (
    UINTN,
    SerialPortRead,
    (
     OUT UINT8  *Buffer,
     IN  UINTN  NumberOfBytes
    )
    );
  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    SerialPortPoll,
    (

    )
    );
  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    SerialPortSetControl,
    (
     IN UINT32  Control
    )
    );
  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    SerialPortGetControl,
    (
     OUT UINT32  *Control
    )
    );
  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    SerialPortSetAttributes,
    (
     IN OUT UINT64              *BaudRate,
     IN OUT UINT32              *ReceiveFifoDepth,
     IN OUT UINT32              *Timeout,
     IN OUT EFI_PARITY_TYPE     *Parity,
     IN OUT UINT8               *DataBits,
     IN OUT EFI_STOP_BITS_TYPE  *StopBits
    )
    );
};

#endif

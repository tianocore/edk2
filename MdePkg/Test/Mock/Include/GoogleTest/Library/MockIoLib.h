/** @file MockIoLib.h
  Google Test mocks for IoLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_IO_LIB_H_
#define MOCK_IO_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/IoLib.h>
}

struct MockIoLib {
  MOCK_INTERFACE_DECLARATION (MockIoLib);

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoRead8,
    (IN UINTN  Port)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoWrite8,
    (IN UINTN  Port,
     IN UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    IoReadFifo8,
    (IN  UINTN  Port,
     IN  UINTN  Count,
     OUT VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    IoWriteFifo8,
    (IN UINTN  Port,
     IN UINTN  Count,
     IN VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoOr8,
    (IN UINTN  Port,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoAnd8,
    (IN UINTN  Port,
     IN UINT8  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoAndThenOr8,
    (IN UINTN  Port,
     IN UINT8  AndData,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoBitFieldRead8,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoBitFieldWrite8,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoBitFieldOr8,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoBitFieldAnd8,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    IoBitFieldAndThenOr8,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  AndData,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoRead16,
    (IN UINTN  Port)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoWrite16,
    (IN UINTN   Port,
     IN UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    IoReadFifo16,
    (IN  UINTN  Port,
     IN  UINTN  Count,
     OUT VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    IoWriteFifo16,
    (IN UINTN  Port,
     IN UINTN  Count,
     IN VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoOr16,
    (IN UINTN   Port,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoAnd16,
    (IN UINTN   Port,
     IN UINT16  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoAndThenOr16,
    (IN UINTN   Port,
     IN UINT16  AndData,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoBitFieldRead16,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoBitFieldWrite16,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoBitFieldOr16,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoBitFieldAnd16,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    IoBitFieldAndThenOr16,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  AndData,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoRead32,
    (IN UINTN  Port)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoWrite32,
    (IN UINTN   Port,
     IN UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    IoReadFifo32,
    (IN  UINTN  Port,
     IN  UINTN  Count,
     OUT VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    IoWriteFifo32,
    (IN UINTN  Port,
     IN UINTN  Count,
     IN VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoOr32,
    (IN UINTN   Port,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoAnd32,
    (IN UINTN   Port,
     IN UINT32  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoAndThenOr32,
    (IN UINTN   Port,
     IN UINT32  AndData,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoBitFieldRead32,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoBitFieldWrite32,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoBitFieldOr32,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoBitFieldAnd32,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    IoBitFieldAndThenOr32,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  AndData,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoRead64,
    (IN UINTN  Port)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoWrite64,
    (IN UINTN   Port,
     IN UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoOr64,
    (IN UINTN   Port,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoAnd64,
    (IN UINTN   Port,
     IN UINT64  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoAndThenOr64,
    (IN UINTN   Port,
     IN UINT64  AndData,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoBitFieldRead64,
    (IN UINTN  Port,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoBitFieldWrite64,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoBitFieldOr64,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoBitFieldAnd64,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    IoBitFieldAndThenOr64,
    (IN UINTN   Port,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  AndData,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioRead8,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioWrite8,
    (IN UINTN  Address,
     IN UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioOr8,
    (IN UINTN  Address,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioAnd8,
    (IN UINTN  Address,
     IN UINT8  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioAndThenOr8,
    (IN UINTN  Address,
     IN UINT8  AndData,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioBitFieldRead8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioBitFieldWrite8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioBitFieldOr8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioBitFieldAnd8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    MmioBitFieldAndThenOr8,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit,
     IN UINT8  AndData,
     IN UINT8  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioRead16,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioWrite16,
    (IN UINTN   Address,
     IN UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioOr16,
    (IN UINTN   Address,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioAnd16,
    (IN UINTN   Address,
     IN UINT16  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioAndThenOr16,
    (IN UINTN   Address,
     IN UINT16  AndData,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioBitFieldRead16,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioBitFieldWrite16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioBitFieldOr16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioBitFieldAnd16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    MmioBitFieldAndThenOr16,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT16  AndData,
     IN UINT16  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioRead32,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioWrite32,
    (IN UINTN   Address,
     IN UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioOr32,
    (IN UINTN   Address,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioAnd32,
    (IN UINTN   Address,
     IN UINT32  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioAndThenOr32,
    (IN UINTN   Address,
     IN UINT32  AndData,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioBitFieldRead32,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioBitFieldWrite32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioBitFieldOr32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioBitFieldAnd32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    MmioBitFieldAndThenOr32,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT32  AndData,
     IN UINT32  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioRead64,
    (IN UINTN  Address)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioWrite64,
    (IN UINTN   Address,
     IN UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioOr64,
    (IN UINTN   Address,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioAnd64,
    (IN UINTN   Address,
     IN UINT64  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioAndThenOr64,
    (IN UINTN   Address,
     IN UINT64  AndData,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioBitFieldRead64,
    (IN UINTN  Address,
     IN UINTN  StartBit,
     IN UINTN  EndBit)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioBitFieldWrite64,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioBitFieldOr64,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioBitFieldAnd64,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  AndData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    MmioBitFieldAndThenOr64,
    (IN UINTN   Address,
     IN UINTN   StartBit,
     IN UINTN   EndBit,
     IN UINT64  AndData,
     IN UINT64  OrData)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    MmioReadBuffer8,
    (IN  UINTN  StartAddress,
     IN  UINTN  Length,
     OUT UINT8  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16 *,
    MmioReadBuffer16,
    (IN  UINTN   StartAddress,
     IN  UINTN   Length,
     OUT UINT16  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32 *,
    MmioReadBuffer32,
    (IN  UINTN   StartAddress,
     IN  UINTN   Length,
     OUT UINT32  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64 *,
    MmioReadBuffer64,
    (IN  UINTN   StartAddress,
     IN  UINTN   Length,
     OUT UINT64  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    MmioWriteBuffer8,
    (IN       UINTN  StartAddress,
     IN       UINTN  Length,
     IN CONST UINT8  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16 *,
    MmioWriteBuffer16,
    (IN       UINTN   StartAddress,
     IN       UINTN   Length,
     IN CONST UINT16  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32 *,
    MmioWriteBuffer32,
    (IN       UINTN   StartAddress,
     IN       UINTN   Length,
     IN CONST UINT32  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64 *,
    MmioWriteBuffer64,
    (IN       UINTN   StartAddress,
     IN       UINTN   Length,
     IN CONST UINT64  *Buffer)
    );
};

#endif // MOCK_IO_LIB_H_

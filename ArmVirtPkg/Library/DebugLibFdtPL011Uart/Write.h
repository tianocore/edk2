/** @file
  Declare DebugLibFdtPL011UartWrite(), for abstracting PL011 UART initialization
  differences between flash- vs. RAM-based modules.

  Copyright (C) Red Hat
  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2012 - 2014, ARM Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

/**
  (Copied from SerialPortWrite() in "MdePkg/Include/Library/SerialPortLib.h" at
  commit c4547aefb3d0, with the Buffer non-nullity assertion removed:)

  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.
**/
UINTN
DebugLibFdtPL011UartWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  );

/**
  Retrieve the serial port runtime debug log level from the device tree.

  @param[out] Value  On success, the debug log level bitmask read from the
                     "edk2,debug-level" property of a PL011 UART node.

  @retval EFI_SUCCESS            The debug level was retrieved successfully.
  @retval EFI_INVALID_PARAMETER  Value is NULL.
  @retval EFI_NOT_FOUND          The debug level is not available.
**/
EFI_STATUS
GetSerialDebugLogRuntime (
  UINT32  *Value
  );

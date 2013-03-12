/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2012, ARM Ltd. All rights reserved.
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>


#include <Library/SerialPortLib.h>

/**

  Programmed hardware of Serial port.

  @return    Always return EFI_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes written to serial device.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  return 0;
}


/**
  Read data from serial device and save the data in buffer.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Actual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
  )
{
  return 0;
}



/**
  Poll the serial device to see if there is any data waiting.

  If there is data waiting to be read from the serial port, then return
  TRUE.  If there is no data waiting to be read from the serial port, then 
  return FALSE.

  @retval TRUE             Data is waiting to be read.
  @retval FALSE            There is no data waiting to be read.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  return 0;
}


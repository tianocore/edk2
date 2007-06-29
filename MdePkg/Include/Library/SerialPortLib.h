
/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  SerialPortLib.h

**/

#ifndef __SERIAL_PORT_LIB__
#define __SERIAL_PORT_LIB__

/**

  Programmed hardware of Serial port.

  @return  Status of Serial Port Device initialization.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  );

/**
  Write data to serial device. 
 
  If the buffer is NULL, then return 0; 
  if NumberOfBytes is zero, then return 0. 

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8 	   *Buffer,
  IN UINTN 	   NumberOfBytes
  );


/**
  Read data from serial device and save the datas in buffer.
 
  If the buffer is NULL, then return zero;
  if NumberOfBytes is zero, then return zero.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Aactual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8 	*Buffer,
  IN  UINTN 	NumberOfBytes
  );


#endif

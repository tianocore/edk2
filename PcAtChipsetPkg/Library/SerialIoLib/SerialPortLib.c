/** @file
  UART Serial Port library functions

  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>

//---------------------------------------------
// UART Register Offsets
//---------------------------------------------
#define BAUD_LOW_OFFSET         0x00
#define BAUD_HIGH_OFFSET        0x01
#define IER_OFFSET              0x01
#define LCR_SHADOW_OFFSET       0x01
#define FCR_SHADOW_OFFSET       0x02
#define IR_CONTROL_OFFSET       0x02
#define FCR_OFFSET              0x02
#define EIR_OFFSET              0x02
#define BSR_OFFSET              0x03
#define LCR_OFFSET              0x03
#define MCR_OFFSET              0x04
#define LSR_OFFSET              0x05
#define MSR_OFFSET              0x06

//---------------------------------------------
// UART Register Bit Defines
//---------------------------------------------
#define LSR_TXRDY               0x20
#define LSR_RXDA                0x01
#define DLAB                    0x01

//---------------------------------------------
// UART Settings
//---------------------------------------------
UINT16  gUartBase = 0x3F8;
UINTN   gBps      = 115200;
UINT8   gData     = 8;
UINT8   gStop     = 1;
UINT8   gParity   = 0;
UINT8   gBreakSet = 0;

/**
  Initialize the serial device hardware.
  
  If no initialization is required, then return RETURN_SUCCESS.
  If the serial device was successfuly initialized, then return RETURN_SUCCESS.
  If the serial device could not be initialized, then return RETURN_DEVICE_ERROR.
  
  @retval RETURN_SUCCESS        The serial device was initialized.
  @retval RETURN_DEVICE_ERROR   The serail device could not be initialized.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINTN  Divisor;
  UINT8  OutputData;
  UINT8  Data;

  //
  // Map 5..8 to 0..3
  //
  Data = (UINT8) (gData - (UINT8) 5);

  //
  // Calculate divisor for baud generator
  //
  Divisor = 115200 / gBps;
  
  //
  // Set communications format
  //
  OutputData = (UINT8) ((DLAB << 7) | (gBreakSet << 6) | (gParity << 3) | (gStop << 2) | Data);
  IoWrite8 ((UINTN) (gUartBase + LCR_OFFSET), OutputData);

  //
  // Configure baud rate
  //
  IoWrite8 ((UINTN) (gUartBase + BAUD_HIGH_OFFSET), (UINT8) (Divisor >> 8));
  IoWrite8 ((UINTN) (gUartBase + BAUD_LOW_OFFSET), (UINT8) (Divisor & 0xff));

  //
  // Switch back to bank 0
  //
  OutputData = (UINT8) ((~DLAB << 7) | (gBreakSet << 6) | (gParity << 3) | (gStop << 2) | Data);
  IoWrite8 ((UINTN) (gUartBase + LCR_OFFSET), OutputData);

  return RETURN_SUCCESS;
}

/**
  Write data from buffer to serial device. 
 
  Writes NumberOfBytes data bytes from Buffer to the serial device.  
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.

  If Buffer is NULL, then ASSERT(). 

  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.  
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINTN  Result;
  UINT8  Data;

  if (Buffer == NULL) {
    return 0;
  }

  Result = NumberOfBytes;

  while ((NumberOfBytes--) != 0) {
    //
    // Wait for the serail port to be ready.
    //
    do {
      Data = IoRead8 ((UINT16) gUartBase + LSR_OFFSET);
    } while ((Data & LSR_TXRDY) == 0);
    IoWrite8 ((UINT16) gUartBase, *Buffer++);
  }

  return Result;
}


/**
  Reads data from a serial device into a buffer.

  @param  Buffer           Pointer to the data buffer to store the data read from the serial device.
  @param  NumberOfBytes    Number of bytes to read from the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes read from the serial device.  
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  UINTN  Result;
  UINT8  Data;

  if (NULL == Buffer) {
    return 0;
  }

  Result = NumberOfBytes;

  while ((NumberOfBytes--) != 0) {
    //
    // Wait for the serail port to be ready.
    //
    do {
      Data = IoRead8 ((UINT16) gUartBase + LSR_OFFSET);
    } while ((Data & LSR_RXDA) == 0);

    *Buffer++ = IoRead8 ((UINT16) gUartBase);
  }

  return Result;
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  Polls aserial device to see if there is any data waiting to be read.
  If there is data waiting to be read from the serial device, then TRUE is returned.
  If there is no data waiting to be read from the serial device, then FALSE is returned.

  @retval TRUE             Data is waiting to be read from the serial device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  UINT8  Data;

  //
  // Read the serial port status.
  //
  Data = IoRead8 ((UINT16) gUartBase + LSR_OFFSET);

  return (BOOLEAN) ((Data & LSR_RXDA) != 0);
}


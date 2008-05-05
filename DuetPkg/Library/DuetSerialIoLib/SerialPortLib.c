/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
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

UINT16  gComBase = 0x3f8;
UINTN   gBps = 115200;
UINT8   gData = 8;
UINT8   gStop = 1;
UINT8   gParity = 0;
UINT8   gBreakSet = 0;
/*

  Programmed hardware of Serial port.

  @return    Always return EFI_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINTN           Divisor;
  UINT8           OutputData;
  UINT8           Data;

 //
  // Map 5..8 to 0..3
  //
  Data = (UINT8) (gData - (UINT8)5);

  //
  // Calculate divisor for baud generator
  //
  Divisor = 115200 / gBps; 
  
  //
  // Set communications format
  //
  OutputData = (UINT8)((DLAB << 7) | ((gBreakSet << 6) | ((gParity << 3) | ((gStop << 2) | Data))));
  IoWrite8 (gComBase + LCR_OFFSET, OutputData);

  //
  // Configure baud rate
  //
  IoWrite8 (gComBase + BAUD_HIGH_OFFSET, (UINT8)(Divisor >> 8));
  IoWrite8 (gComBase + BAUD_LOW_OFFSET, (UINT8)(Divisor & 0xff));

  //
  // Switch back to bank 0
  //
  OutputData = (UINT8)((~DLAB<<7)|((gBreakSet<<6)|((gParity<<3)|((gStop<<2)| Data))));
  IoWrite8 (gComBase + LCR_OFFSET, OutputData);

  return RETURN_SUCCESS;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINTN Result;
  UINT8 Data;

  if (NULL == Buffer) {
    return 0;
  }

  Result = NumberOfBytes;

  while (NumberOfBytes--) {
      //
      // Wait for the serail port to be ready.
      //
      do {
        Data = IoRead8 (gComBase + LSR_OFFSET);
      } while ((Data & LSR_TXRDY) == 0);

      IoWrite8 (gComBase, *Buffer++);
  }

  return Result;

}


/**
  Read data from serial device and save the datas in buffer.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Aactual number of bytes read from serial device.

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


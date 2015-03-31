/** @file
  SerialPortLib instance for UART device upon coreboot

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi/UefiBaseType.h>
#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/CbParseLib.h>

//
// 16550 UART register offsets and bitfields
//
#define R_UART_RXBUF          0
#define R_UART_TXBUF          0
#define R_UART_BAUD_LOW       0
#define R_UART_BAUD_HIGH      1
#define R_UART_FCR            2
#define   B_UART_FCR_FIFOE    BIT0
#define   B_UART_FCR_FIFO64   BIT5
#define R_UART_LCR            3
#define   B_UART_LCR_DLAB     BIT7
#define R_UART_MCR            4
#define   B_UART_MCR_RTS      BIT1
#define R_UART_LSR            5
#define   B_UART_LSR_RXRDY    BIT0
#define   B_UART_LSR_TXRDY    BIT5
#define   B_UART_LSR_TEMT     BIT6
#define R_UART_MSR            6
#define   B_UART_MSR_CTS      BIT4
#define   B_UART_MSR_DSR      BIT5

UINT32 mSerialRegBase = 0;
UINT32 mSerialRegAccessType = 0;

/**
  Read an 8-bit 16550 register.  If PcdSerialUseMmio is TRUE, then the value is read from 
  MMIO space.  If PcdSerialUseMmio is FALSE, then the value is read from I/O space.  The
  parameter Offset is added to the base address of the 16550 registers that is specified 
  by PcdSerialRegisterBase. 
  
  @param  Offset  The offset of the 16550 register to read.

  @return The value read from the 16550 register.

**/
UINT8
SerialPortReadRegister (
  UINTN  Offset
  )
{
  if (mSerialRegAccessType == 2) { //MMIO
    return MmioRead8 (mSerialRegBase + Offset);
  } else { //IO
    return IoRead8 ((UINT16)mSerialRegBase + Offset);
  }
}

/**
  Write an 8-bit 16550 register.  If PcdSerialUseMmio is TRUE, then the value is written to
  MMIO space.  If PcdSerialUseMmio is FALSE, then the value is written to I/O space.  The
  parameter Offset is added to the base address of the 16550 registers that is specified 
  by PcdSerialRegisterBase. 
  
  @param  Offset  The offset of the 16550 register to write.
  @param  Value   The value to write to the 16550 register specified by Offset.

  @return The value written to the 16550 register.

**/
UINT8
SerialPortWriteRegister (
  UINTN  Offset,
  UINT8  Value
  )
{
  if (mSerialRegAccessType == 2) { //MMIO
    return MmioWrite8 (mSerialRegBase + Offset, Value);
  } else {// IO
    return IoWrite8 ((UINT16)mSerialRegBase + Offset, Value);
  }
}

/**
  Initialize the serial device hardware.
  
  If no initialization is required, then return RETURN_SUCCESS.
  If the serial device was successfully initialized, then return RETURN_SUCCESS.
  If the serial device could not be initialized, then return RETURN_DEVICE_ERROR.
  
  @retval RETURN_SUCCESS        The serial device was initialized.
  @retval RETURN_DEVICE_ERROR   The serial device could not be initialized.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  RETURN_STATUS  Status;
  UINTN          Divisor;
  BOOLEAN        Initialized;
  
  Status = CbParseSerialInfo (&mSerialRegBase, &mSerialRegAccessType, NULL);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // See if the serial port is already initialized
  //
  Initialized = TRUE;
  if ((SerialPortReadRegister (R_UART_FCR) & (B_UART_FCR_FIFOE | B_UART_FCR_FIFO64)) !=
      (PcdGet8 (PcdSerialFifoControl)      & (B_UART_FCR_FIFOE | B_UART_FCR_FIFO64))     ) {
    Initialized = FALSE;
  }

  if ((SerialPortReadRegister (R_UART_LCR) & 0x3F) != (PcdGet8 (PcdSerialLineControl) & 0x3F)) {
    Initialized = FALSE;
  }
  SerialPortWriteRegister (R_UART_LCR, (UINT8)(SerialPortReadRegister (R_UART_LCR) | B_UART_LCR_DLAB));
  Divisor =  SerialPortReadRegister (R_UART_BAUD_HIGH) << 8;
  Divisor |= SerialPortReadRegister (R_UART_BAUD_LOW);
  SerialPortWriteRegister (R_UART_LCR, (UINT8)(SerialPortReadRegister (R_UART_LCR) & ~B_UART_LCR_DLAB));
  if (Divisor != 115200 / PcdGet32 (PcdSerialBaudRate)) {
    Initialized = FALSE;
  }
  if (Initialized) {
    return RETURN_SUCCESS;
  }
  
  //
  // Configure baud rate
  //
  Divisor = 115200 / PcdGet32 (PcdSerialBaudRate);
  SerialPortWriteRegister (R_UART_LCR, B_UART_LCR_DLAB);
  SerialPortWriteRegister (R_UART_BAUD_HIGH, (UINT8) (Divisor >> 8));
  SerialPortWriteRegister (R_UART_BAUD_LOW, (UINT8) (Divisor & 0xff));

  //
  // Clear DLAB and configure Data Bits, Parity, and Stop Bits.
  // Strip reserved bits from PcdSerialLineControl
  //
  SerialPortWriteRegister (R_UART_LCR, (UINT8)(PcdGet8 (PcdSerialLineControl) & 0x3F));

  //
  // Enable and reset FIFOs
  // Strip reserved bits from PcdSerialFifoControl
  //
  SerialPortWriteRegister (R_UART_FCR, (UINT8)(PcdGet8 (PcdSerialFifoControl) & 0x27));

  //
  // Put Modem Control Register(MCR) into its reset state of 0x00.
  //  
  SerialPortWriteRegister (R_UART_MCR, 0x00);
  
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
  UINTN    Result;
  UINTN    Index;
  UINTN    FifoSize;

  if (Buffer == NULL) {
    return 0;
  }

  if (NumberOfBytes == 0) {
    //
    // Flush the hardware
    //

    //
    // Wait for both the transmit FIFO and shift register empty.
    //
    while ((SerialPortReadRegister (R_UART_LSR) & B_UART_LSR_TEMT) == 0);

    return 0;
  }

  //
  // Compute the maximum size of the Tx FIFO
  //
  FifoSize = 1;
  if ((PcdGet8 (PcdSerialFifoControl) & B_UART_FCR_FIFOE) != 0) {
    if ((PcdGet8 (PcdSerialFifoControl) & B_UART_FCR_FIFO64) == 0) {
      FifoSize = 16;
    } else {
      FifoSize = 64;
    }
  }

  Result = NumberOfBytes;
  while (NumberOfBytes != 0) {
    //
    // Wait for the serial port to be ready, to make sure both the transmit FIFO
    // and shift register empty.
    //
    while ((SerialPortReadRegister (R_UART_LSR) & B_UART_LSR_TEMT) == 0);

    //
    // Fill then entire Tx FIFO
    //
    for (Index = 0; Index < FifoSize && NumberOfBytes != 0; Index++, NumberOfBytes--, Buffer++) {

      //
      // Write byte to the transmit buffer.
      //
      SerialPortWriteRegister (R_UART_TXBUF, *Buffer);
    }
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

  if (NULL == Buffer) {
    return 0;
  }
  
  for (Result = 0; NumberOfBytes-- != 0; Result++, Buffer++) {
    //
    // Wait for the serial port to have some data.
    //
    while ((SerialPortReadRegister (R_UART_LSR) & B_UART_LSR_RXRDY) == 0);
        
    //
    // Read byte from the receive buffer.
    //
    *Buffer = SerialPortReadRegister (R_UART_RXBUF);
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
  //
  // Read the serial port status
  //
  if ((SerialPortReadRegister (R_UART_LSR) & B_UART_LSR_RXRDY) != 0) {
    return TRUE;
  }  
  
  return FALSE;
}

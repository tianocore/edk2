/** @file
  UART Serial Port library functions.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SerialIo.h>
#include <UniversalPayload/SerialPortInfo.h>

//
// 16550 UART register offsets and bitfields
//
#define R_UART_RXBUF         0    // LCR_DLAB = 0
#define R_UART_TXBUF         0    // LCR_DLAB = 0
#define R_UART_BAUD_LOW      0    // LCR_DLAB = 1
#define R_UART_BAUD_HIGH     1    // LCR_DLAB = 1
#define R_UART_IER           1    // LCR_DLAB = 0
#define R_UART_FCR           2
#define   B_UART_FCR_FIFOE   BIT0
#define   B_UART_FCR_FIFO64  BIT5
#define R_UART_LCR           3
#define   B_UART_LCR_DLAB    BIT7
#define R_UART_MCR           4
#define   B_UART_MCR_DTRC    BIT0
#define   B_UART_MCR_RTS     BIT1
#define R_UART_LSR           5
#define   B_UART_LSR_RXRDY   BIT0
#define   B_UART_LSR_TXRDY   BIT5
#define   B_UART_LSR_TEMT    BIT6
#define R_UART_MSR           6
#define   B_UART_MSR_CTS     BIT4
#define   B_UART_MSR_DSR     BIT5
#define   B_UART_MSR_RI      BIT6
#define   B_UART_MSR_DCD     BIT7

#define MAX_SIZE  16

typedef struct {
  UINTN      BaseAddress;
  BOOLEAN    UseMmio;
  UINT32     BaudRate;
  UINT8      RegisterStride;
} UART_INFO;

UART_INFO  mUartInfo[MAX_SIZE];
UINT8      mUartCount                     = 0;
BOOLEAN    mBaseSerialPortLibHobAtRuntime = FALSE;

/**
  Reads an 8-bit register. If UseMmio is TRUE, then the value is read from
  MMIO space. If UseMmio is FALSE, then the value is read from I/O space. The
  parameter Offset is added to the base address of the register.

  @param  Base             The base address register of UART device.
  @param  Offset           The offset of the register to read.
  @param  UseMmio          Check if value has to be read from MMIO space or IO space.
  @param  RegisterStride   Number of bytes between registers in serial device.

  @return The value read from the register.

**/
UINT8
SerialPortReadRegister (
  UINTN    Base,
  UINTN    Offset,
  BOOLEAN  UseMmio,
  UINT8    RegisterStride
  )
{
  if (UseMmio) {
    return MmioRead8 (Base + Offset * RegisterStride);
  } else {
    return IoRead8 (Base + Offset * RegisterStride);
  }
}

/**
  Writes an 8-bit register.. If UseMmio is TRUE, then the value is written to
  MMIO space. If UseMmio is FALSE, then the value is written to I/O space. The
  parameter Offset is added to the base address of the registers.

  @param  Base             The base address register of UART device.
  @param  Offset           The offset of the register to write.
  @param  Value            Value to be written.
  @param  UseMmio          Check if value has to be written to MMIO space or IO space.
  @param  RegisterStride   Number of bytes between registers in serial device.

  @return The value written to the register.

**/
UINT8
SerialPortWriteRegister (
  UINTN    Base,
  UINTN    Offset,
  UINT8    Value,
  BOOLEAN  UseMmio,
  UINT8    RegisterStride
  )
{
  if (UseMmio) {
    return MmioWrite8 (Base + Offset * RegisterStride, Value);
  } else {
    return IoWrite8 (Base + Offset * RegisterStride, Value);
  }
}

/**
  Initialize the serial device hardware.

  If no initialization is required, then return RETURN_SUCCESS.
  If the serial device was successfully initialized, then return RETURN_SUCCESS.

  @retval RETURN_SUCCESS        The serial device was initialized.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *SerialPortInfo;
  EFI_HOB_GUID_TYPE                   *GuidHob;
  UINTN                               SerialRegisterBase;
  UINT8                               RegisterStride;
  UINT32                              Divisor;
  UINT32                              CurrentDivisor;
  UINT32                              BaudRate;
  BOOLEAN                             Initialized;
  BOOLEAN                             MmioEnable;
  UINT8                               Value;

  if (mUartCount > 0) {
    return RETURN_SUCCESS;
  }

  if (GetHobList () == NULL) {
    mUartCount         = 0;
    SerialRegisterBase = PcdGet64 (PcdSerialRegisterBase);
    MmioEnable         = PcdGetBool (PcdSerialUseMmio);
    BaudRate           = PcdGet32 (PcdSerialBaudRate);
    RegisterStride     = (UINT8)PcdGet32 (PcdSerialRegisterStride);

    mUartInfo[mUartCount].BaseAddress    = SerialRegisterBase;
    mUartInfo[mUartCount].UseMmio        = MmioEnable;
    mUartInfo[mUartCount].BaudRate       = BaudRate;
    mUartInfo[mUartCount].RegisterStride = RegisterStride;
    mUartCount++;

    Divisor = PcdGet32 (PcdSerialClockRate) / (BaudRate * 16);
    if ((PcdGet32 (PcdSerialClockRate) % (BaudRate * 16)) >= BaudRate * 8) {
      Divisor++;
    }

    //
    // See if the serial port is already initialized
    //
    Initialized = TRUE;
    if ((SerialPortReadRegister (SerialRegisterBase, R_UART_LCR, MmioEnable, RegisterStride) & 0x3F) != (PcdGet8 (PcdSerialLineControl) & 0x3F)) {
      Initialized = FALSE;
    }

    Value = (UINT8)(SerialPortReadRegister (SerialRegisterBase, R_UART_LCR, MmioEnable, RegisterStride) | B_UART_LCR_DLAB);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, Value, MmioEnable, RegisterStride);
    CurrentDivisor  =  SerialPortReadRegister (SerialRegisterBase, R_UART_BAUD_HIGH, MmioEnable, RegisterStride) << 8;
    CurrentDivisor |= (UINT32)SerialPortReadRegister (SerialRegisterBase, R_UART_BAUD_LOW, MmioEnable, RegisterStride);
    Value           = (UINT8)(SerialPortReadRegister (SerialRegisterBase, R_UART_LCR, MmioEnable, RegisterStride) & ~B_UART_LCR_DLAB);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, Value, MmioEnable, RegisterStride);
    if (CurrentDivisor != Divisor) {
      Initialized = FALSE;
    }

    //
    // Configure baud rate
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, B_UART_LCR_DLAB, MmioEnable, RegisterStride);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_BAUD_HIGH, (UINT8)(Divisor >> 8), MmioEnable, RegisterStride);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_BAUD_LOW, (UINT8)(Divisor & 0xff), MmioEnable, RegisterStride);

    //
    // Clear DLAB and configure Data Bits, Parity, and Stop Bits.
    // Strip reserved bits from PcdSerialLineControl
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, (UINT8)(PcdGet8 (PcdSerialLineControl) & 0x3F), MmioEnable, RegisterStride);

    //
    // Enable and reset FIFOs
    // Strip reserved bits from PcdSerialFifoControl
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_FCR, 0x00, MmioEnable, RegisterStride);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_FCR, (UINT8)(PcdGet8 (PcdSerialFifoControl) & (B_UART_FCR_FIFOE | B_UART_FCR_FIFO64)), MmioEnable, RegisterStride);

    //
    // Set FIFO Polled Mode by clearing IER after setting FCR
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_IER, 0x00, MmioEnable, RegisterStride);

    //
    // Put Modem Control Register(MCR) into its reset state of 0x00.
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_MCR, 0x00, MmioEnable, RegisterStride);

    return RETURN_SUCCESS;
  }

  GuidHob = GetFirstGuidHob (&gUniversalPayloadSerialPortInfoGuid);
  while (GuidHob != NULL) {
    SerialPortInfo     = (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO *)GET_GUID_HOB_DATA (GuidHob);
    SerialRegisterBase = SerialPortInfo->RegisterBase;
    MmioEnable         = SerialPortInfo->UseMmio;
    BaudRate           = SerialPortInfo->BaudRate;
    RegisterStride     = SerialPortInfo->RegisterStride;

    if (SerialRegisterBase == 0) {
      GuidHob = GET_NEXT_HOB (GuidHob);
      GuidHob = GetNextGuidHob (&gUniversalPayloadSerialPortInfoGuid, GuidHob);
      continue;
    }

    mUartInfo[mUartCount].BaseAddress    = SerialRegisterBase;
    mUartInfo[mUartCount].UseMmio        = MmioEnable;
    mUartInfo[mUartCount].BaudRate       = BaudRate;
    mUartInfo[mUartCount].RegisterStride = RegisterStride;
    mUartCount++;

    Divisor = PcdGet32 (PcdSerialClockRate) / (BaudRate * 16);
    if ((PcdGet32 (PcdSerialClockRate) % (BaudRate * 16)) >= BaudRate * 8) {
      Divisor++;
    }

    //
    // See if the serial port is already initialized
    //
    Initialized = TRUE;
    if ((SerialPortReadRegister (SerialRegisterBase, R_UART_LCR, MmioEnable, RegisterStride) & 0x3F) != (PcdGet8 (PcdSerialLineControl) & 0x3F)) {
      Initialized = FALSE;
    }

    Value = (UINT8)(SerialPortReadRegister (SerialRegisterBase, R_UART_LCR, MmioEnable, RegisterStride) | B_UART_LCR_DLAB);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, Value, MmioEnable, RegisterStride);
    CurrentDivisor  =  SerialPortReadRegister (SerialRegisterBase, R_UART_BAUD_HIGH, MmioEnable, RegisterStride) << 8;
    CurrentDivisor |= (UINT32)SerialPortReadRegister (SerialRegisterBase, R_UART_BAUD_LOW, MmioEnable, RegisterStride);
    Value           = (UINT8)(SerialPortReadRegister (SerialRegisterBase, R_UART_LCR, MmioEnable, RegisterStride) & ~B_UART_LCR_DLAB);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, Value, MmioEnable, RegisterStride);
    if (CurrentDivisor != Divisor) {
      Initialized = FALSE;
    }

    if (Initialized) {
      GuidHob = GET_NEXT_HOB (GuidHob);
      GuidHob = GetNextGuidHob (&gUniversalPayloadSerialPortInfoGuid, GuidHob);
      continue;
    }

    //
    // Configure baud rate
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, B_UART_LCR_DLAB, MmioEnable, RegisterStride);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_BAUD_HIGH, (UINT8)(Divisor >> 8), MmioEnable, RegisterStride);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_BAUD_LOW, (UINT8)(Divisor & 0xff), MmioEnable, RegisterStride);

    //
    // Clear DLAB and configure Data Bits, Parity, and Stop Bits.
    // Strip reserved bits from PcdSerialLineControl
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_LCR, (UINT8)(PcdGet8 (PcdSerialLineControl) & 0x3F), MmioEnable, RegisterStride);

    //
    // Enable and reset FIFOs
    // Strip reserved bits from PcdSerialFifoControl
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_FCR, 0x00, MmioEnable, RegisterStride);
    SerialPortWriteRegister (SerialRegisterBase, R_UART_FCR, (UINT8)(PcdGet8 (PcdSerialFifoControl) & (B_UART_FCR_FIFOE | B_UART_FCR_FIFO64)), MmioEnable, RegisterStride);

    //
    // Set FIFO Polled Mode by clearing IER after setting FCR
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_IER, 0x00, MmioEnable, RegisterStride);

    //
    // Put Modem Control Register(MCR) into its reset state of 0x00.
    //
    SerialPortWriteRegister (SerialRegisterBase, R_UART_MCR, 0x00, MmioEnable, RegisterStride);

    GuidHob = GET_NEXT_HOB (GuidHob);
    GuidHob = GetNextGuidHob (&gUniversalPayloadSerialPortInfoGuid, GuidHob);
  }

  return RETURN_SUCCESS;
}

/**
  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.

  If Buffer is NULL, then return 0.

  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  UINTN    BaseAddress;
  BOOLEAN  UseMmio;
  UINTN    BytesLeft;
  UINTN    Index;
  UINTN    FifoSize;
  UINT8    *DataBuffer;
  UINT8    Count;
  UINT8    Stride;

  if ((Buffer == NULL) || (NumberOfBytes == 0) || (mUartCount == 0)) {
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
      FifoSize = PcdGet32 (PcdSerialExtendedTxFifoSize);
    }
  }

  Count = 0;
  while (Count < mUartCount) {
    BaseAddress = mUartInfo[Count].BaseAddress;
    UseMmio     = mUartInfo[Count].UseMmio;
    Stride      = mUartInfo[Count].RegisterStride;

    if (UseMmio && mBaseSerialPortLibHobAtRuntime) {
      Count++;
      continue;
    }

    if (BaseAddress == 0) {
      Count++;
      continue;
    }

    DataBuffer = Buffer;
    BytesLeft  = NumberOfBytes;

    while (BytesLeft != 0) {
      //
      // Wait for the serial port to be ready, to make sure both the transmit FIFO
      // and shift register empty.
      //
      while ((SerialPortReadRegister (BaseAddress, R_UART_LSR, UseMmio, Stride) & B_UART_LSR_TXRDY) == 0) {
      }

      //
      // Fill the entire Tx FIFO
      //
      for (Index = 0; Index < FifoSize && BytesLeft != 0; Index++, BytesLeft--, DataBuffer++) {
        //
        // Write byte to the transmit buffer.
        //
        SerialPortWriteRegister (BaseAddress, R_UART_TXBUF, *DataBuffer, UseMmio, Stride);
      }

      MicroSecondDelay (20);
    }

    Count++;
  }

  return NumberOfBytes;
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
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  UINTN    BaseAddress;
  BOOLEAN  UseMmio;
  BOOLEAN  IsNextPort;
  UINT8    *DataBuffer;
  UINTN    BytesLeft;
  UINTN    Result;
  UINT8    Mcr;
  UINT8    Count;
  UINT8    Stride;

  if (Buffer == NULL) {
    return 0;
  }

  Count = 0;
  while (Count < mUartCount) {
    BaseAddress = mUartInfo[Count].BaseAddress;
    UseMmio     = mUartInfo[Count].UseMmio;
    Stride      = mUartInfo[Count].RegisterStride;

    if (BaseAddress == 0) {
      Count++;
      continue;
    }

    DataBuffer = Buffer;
    BytesLeft  = NumberOfBytes;
    IsNextPort = FALSE;

    Mcr = (UINT8)(SerialPortReadRegister (BaseAddress, R_UART_MCR, UseMmio, Stride) & ~B_UART_MCR_RTS);

    for (Result = 0; BytesLeft-- != 0; Result++, DataBuffer++) {
      //
      // Wait for the serial port to have some data.
      //
      while ((SerialPortReadRegister (BaseAddress, R_UART_LSR, UseMmio, Stride) & B_UART_LSR_RXRDY) == 0) {
        if (PcdGetBool (PcdSerialUseHardwareFlowControl)) {
          //
          // Set RTS to let the peer send some data
          //
          SerialPortWriteRegister (BaseAddress, R_UART_MCR, (UINT8)(Mcr | B_UART_MCR_RTS), UseMmio, Stride);
        }

        IsNextPort = TRUE;
        break;
      }

      if (IsNextPort) {
        break;
      }

      if (PcdGetBool (PcdSerialUseHardwareFlowControl)) {
        //
        // Clear RTS to prevent peer from sending data
        //
        SerialPortWriteRegister (BaseAddress, R_UART_MCR, Mcr, UseMmio, Stride);
      }

      //
      // Read byte from the receive buffer.
      //
      *DataBuffer = SerialPortReadRegister (BaseAddress, R_UART_RXBUF, UseMmio, Stride);
    }

    if ((!IsNextPort) && (*(--DataBuffer) != '\0')) {
      return Result;
    }

    Count++;
  }

  return Result;
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  Polls a serial device to see if there is any data waiting to be read.
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
  UINTN    BaseAddress;
  BOOLEAN  UseMmio;
  UINT8    Stride;
  UINT8    Count;
  BOOLEAN  IsDataReady;

  Count = 0;
  while (Count < mUartCount) {
    BaseAddress = mUartInfo[Count].BaseAddress;
    UseMmio     = mUartInfo[Count].UseMmio;
    Stride      = mUartInfo[Count].RegisterStride;

    if (BaseAddress == 0) {
      Count++;
      continue;
    }

    IsDataReady = FALSE;

    //
    // Read the serial port status
    //
    if ((SerialPortReadRegister (BaseAddress, R_UART_LSR, UseMmio, Stride) & B_UART_LSR_RXRDY) != 0) {
      if (PcdGetBool (PcdSerialUseHardwareFlowControl)) {
        //
        // Clear RTS to prevent peer from sending data
        //
        SerialPortWriteRegister (BaseAddress, R_UART_MCR, (UINT8)(SerialPortReadRegister (BaseAddress, R_UART_MCR, UseMmio, Stride) & ~B_UART_MCR_RTS), UseMmio, Stride);
      }

      IsDataReady = TRUE;
    }

    if (PcdGetBool (PcdSerialUseHardwareFlowControl)) {
      //
      // Set RTS to let the peer send some data
      //
      SerialPortWriteRegister (BaseAddress, R_UART_MCR, (UINT8)(SerialPortReadRegister (BaseAddress, R_UART_MCR, UseMmio, Stride) | B_UART_MCR_RTS), UseMmio, Stride);
    }

    if (IsDataReady) {
      return IsDataReady;
    }

    Count++;
  }

  return IsDataReady;
}

/**
  Sets the control bits on a serial device.

  @param Control                Sets the bits of Control that are settable.

  @retval RETURN_SUCCESS        The new control bits were set on the serial device.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  )
{
  UINTN    BaseAddress;
  BOOLEAN  UseMmio;
  UINT8    Mcr;
  UINT8    Count;
  UINT8    Stride;

  Count = 0;
  while (Count < mUartCount) {
    BaseAddress = mUartInfo[Count].BaseAddress;
    UseMmio     = mUartInfo[Count].UseMmio;
    Stride      = mUartInfo[Count].RegisterStride;

    if (BaseAddress == 0) {
      Count++;
      continue;
    }

    //
    // First determine the parameter is invalid.
    //
    if ((Control & (~(EFI_SERIAL_REQUEST_TO_SEND | EFI_SERIAL_DATA_TERMINAL_READY |
                      EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE))) != 0)
    {
      Count++;
      continue;
    }

    //
    // Read the Modem Control Register.
    //
    Mcr  = SerialPortReadRegister (BaseAddress, R_UART_MCR, UseMmio, Stride);
    Mcr &= (~(B_UART_MCR_DTRC | B_UART_MCR_RTS));

    if ((Control & EFI_SERIAL_DATA_TERMINAL_READY) == EFI_SERIAL_DATA_TERMINAL_READY) {
      Mcr |= B_UART_MCR_DTRC;
    }

    if ((Control & EFI_SERIAL_REQUEST_TO_SEND) == EFI_SERIAL_REQUEST_TO_SEND) {
      Mcr |= B_UART_MCR_RTS;
    }

    //
    // Write the Modem Control Register.
    //
    SerialPortWriteRegister (BaseAddress, R_UART_MCR, Mcr, UseMmio, Stride);
    Count++;
  }

  return RETURN_SUCCESS;
}

/**
  Retrieve the status of the control bits on a serial device.

  @param Control                A pointer to return the current control signals from the serial device.

  @retval RETURN_SUCCESS        The control bits were read from the serial device.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  UINTN    BaseAddress;
  BOOLEAN  UseMmio;
  UINT8    Msr;
  UINT8    Mcr;
  UINT8    Lsr;
  UINT8    Count;
  UINT8    Stride;

  Count = 0;
  while (Count < mUartCount) {
    BaseAddress = mUartInfo[Count].BaseAddress;
    UseMmio     = mUartInfo[Count].UseMmio;
    Stride      = mUartInfo[Count].RegisterStride;

    if (BaseAddress == 0) {
      Count++;
      continue;
    }

    *Control = 0;

    //
    // Read the Modem Status Register.
    //
    Msr = SerialPortReadRegister (BaseAddress, R_UART_MSR, UseMmio, Stride);

    if ((Msr & B_UART_MSR_CTS) == B_UART_MSR_CTS) {
      *Control |= EFI_SERIAL_CLEAR_TO_SEND;
    }

    if ((Msr & B_UART_MSR_DSR) == B_UART_MSR_DSR) {
      *Control |= EFI_SERIAL_DATA_SET_READY;
    }

    if ((Msr & B_UART_MSR_RI) == B_UART_MSR_RI) {
      *Control |= EFI_SERIAL_RING_INDICATE;
    }

    if ((Msr & B_UART_MSR_DCD) == B_UART_MSR_DCD) {
      *Control |= EFI_SERIAL_CARRIER_DETECT;
    }

    //
    // Read the Modem Control Register.
    //
    Mcr = SerialPortReadRegister (BaseAddress, R_UART_MCR, UseMmio, Stride);

    if ((Mcr & B_UART_MCR_DTRC) == B_UART_MCR_DTRC) {
      *Control |= EFI_SERIAL_DATA_TERMINAL_READY;
    }

    if ((Mcr & B_UART_MCR_RTS) == B_UART_MCR_RTS) {
      *Control |= EFI_SERIAL_REQUEST_TO_SEND;
    }

    if (PcdGetBool (PcdSerialUseHardwareFlowControl)) {
      *Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
    }

    //
    // Read the Line Status Register.
    //
    Lsr = SerialPortReadRegister (BaseAddress, R_UART_LSR, UseMmio, Stride);

    if ((Lsr & (B_UART_LSR_TEMT | B_UART_LSR_TXRDY)) == (B_UART_LSR_TEMT | B_UART_LSR_TXRDY)) {
      *Control |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
    }

    if ((Lsr & B_UART_LSR_RXRDY) == 0) {
      *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
    }

    if ((((*Control & EFI_SERIAL_OUTPUT_BUFFER_EMPTY) == EFI_SERIAL_OUTPUT_BUFFER_EMPTY) &&
         ((*Control & EFI_SERIAL_INPUT_BUFFER_EMPTY) != EFI_SERIAL_INPUT_BUFFER_EMPTY)) ||
        ((*Control & (EFI_SERIAL_DATA_SET_READY | EFI_SERIAL_CLEAR_TO_SEND |
                      EFI_SERIAL_CARRIER_DETECT)) == (EFI_SERIAL_DATA_SET_READY | EFI_SERIAL_CLEAR_TO_SEND |
                                                      EFI_SERIAL_CARRIER_DETECT)))
    {
      return RETURN_SUCCESS;
    }

    Count++;
  }

  return RETURN_SUCCESS;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receice time out, parity,
  data bits, and stop bits on a serial device.

  @param BaudRate           The requested baud rate. A BaudRate value of 0 will use the
                            device's default interface speed.
                            On output, the value actually set.
  @param ReveiveFifoDepth   The requested depth of the FIFO on the receive side of the
                            serial interface. A ReceiveFifoDepth value of 0 will use
                            the device's default FIFO depth.
                            On output, the value actually set.
  @param Timeout            The requested time out for a single character in microseconds.
                            This timeout applies to both the transmit and receive side of the
                            interface. A Timeout value of 0 will use the device's default time
                            out value.
                            On output, the value actually set.
  @param Parity             The type of parity to use on this serial device. A Parity value of
                            DefaultParity will use the device's default parity value.
                            On output, the value actually set.
  @param DataBits           The number of data bits to use on the serial device. A DataBits
                            vaule of 0 will use the device's default data bit setting.
                            On output, the value actually set.
  @param StopBits           The number of stop bits to use on this serial device. A StopBits
                            value of DefaultStopBits will use the device's default number of
                            stop bits.
                            On output, the value actually set.

  @retval RETURN_SUCCESS            The new attributes were set on the serial device.

**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT UINT32              *Timeout,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  UINTN    BaseAddress;
  BOOLEAN  UseMmio;
  UINT32   SerialBaudRate;
  UINTN    Divisor;
  UINT8    Lcr;
  UINT8    LcrData;
  UINT8    LcrParity;
  UINT8    LcrStop;
  UINT8    Count;
  UINT8    Stride;

  Count = 0;
  while (Count < mUartCount) {
    BaseAddress = mUartInfo[Count].BaseAddress;
    UseMmio     = mUartInfo[Count].UseMmio;
    Stride      = mUartInfo[Count].RegisterStride;

    if (BaseAddress == 0) {
      Count++;
      continue;
    }

    //
    // Check for default settings and fill in actual values.
    //
    if (*BaudRate == 0) {
      *BaudRate = mUartInfo[Count].BaudRate;
    }

    SerialBaudRate = (UINT32)*BaudRate;

    if (*DataBits == 0) {
      LcrData   = (UINT8)(PcdGet8 (PcdSerialLineControl) & 0x3);
      *DataBits = LcrData + 5;
    } else {
      if ((*DataBits < 5) || (*DataBits > 8)) {
        Count++;
        continue;
      }

      //
      // Map 5..8 to 0..3
      //
      LcrData = (UINT8)(*DataBits - (UINT8)5);
    }

    if (*Parity == DefaultParity) {
      LcrParity = (UINT8)((PcdGet8 (PcdSerialLineControl) >> 3) & 0x7);
      switch (LcrParity) {
        case 0:
          *Parity = NoParity;
          break;

        case 3:
          *Parity = EvenParity;
          break;

        case 1:
          *Parity = OddParity;
          break;

        case 7:
          *Parity = SpaceParity;
          break;

        case 5:
          *Parity = MarkParity;
          break;

        default:
          break;
      }
    } else {
      switch (*Parity) {
        case NoParity:
          LcrParity = 0;
          break;

        case EvenParity:
          LcrParity = 3;
          break;

        case OddParity:
          LcrParity = 1;
          break;

        case SpaceParity:
          LcrParity = 7;
          break;

        case MarkParity:
          LcrParity = 5;
          break;

        default:
          Count++;
          continue;
      }
    }

    if (*StopBits == DefaultStopBits) {
      LcrStop = (UINT8)((PcdGet8 (PcdSerialLineControl) >> 2) & 0x1);
      switch (LcrStop) {
        case 0:
          *StopBits = OneStopBit;
          break;

        case 1:
          if (*DataBits == 5) {
            *StopBits = OneFiveStopBits;
          } else {
            *StopBits = TwoStopBits;
          }

          break;

        default:
          break;
      }
    } else {
      switch (*StopBits) {
        case OneStopBit:
          LcrStop = 0;
          break;

        case OneFiveStopBits:
        case TwoStopBits:
          LcrStop = 1;
          break;

        default:
          Count++;
          continue;
      }
    }

    //
    // Calculate divisor for baud generator
    //    Ref_Clk_Rate / Baud_Rate / 16
    //
    Divisor = PcdGet32 (PcdSerialClockRate) / (SerialBaudRate * 16);
    if ((PcdGet32 (PcdSerialClockRate) % (SerialBaudRate * 16)) >= SerialBaudRate * 8) {
      Divisor++;
    }

    //
    // Configure baud rate
    //
    SerialPortWriteRegister (BaseAddress, R_UART_LCR, B_UART_LCR_DLAB, UseMmio, Stride);
    SerialPortWriteRegister (BaseAddress, R_UART_BAUD_HIGH, (UINT8)(Divisor >> 8), UseMmio, Stride);
    SerialPortWriteRegister (BaseAddress, R_UART_BAUD_LOW, (UINT8)(Divisor & 0xff), UseMmio, Stride);

    //
    // Clear DLAB and configure Data Bits, Parity, and Stop Bits.
    // Strip reserved bits from line control value
    //
    Lcr = (UINT8)((LcrParity << 3) | (LcrStop << 2) | LcrData);
    SerialPortWriteRegister (BaseAddress, R_UART_LCR, (UINT8)(Lcr & 0x3F), UseMmio, Stride);
    Count++;
  }

  return RETURN_SUCCESS;
}

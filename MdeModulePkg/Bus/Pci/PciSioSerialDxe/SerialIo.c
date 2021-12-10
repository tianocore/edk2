/** @file
  SerialIo implementation for PCI or SIO UARTs.

Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Serial.h"

/**
  Skip the optional Controller device path node and return the
  pointer to the next device path node.

  @param DevicePath             Pointer to the device path.
  @param ContainsControllerNode Returns TRUE if the Controller device path exists.
  @param ControllerNumber       Returns the Controller Number if Controller device path exists.

  @return     Pointer to the next device path node.
**/
UART_DEVICE_PATH *
SkipControllerDevicePathNode (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  BOOLEAN                   *ContainsControllerNode,
  UINT32                    *ControllerNumber
  )
{
  if ((DevicePathType (DevicePath) == HARDWARE_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == HW_CONTROLLER_DP)
      )
  {
    if (ContainsControllerNode != NULL) {
      *ContainsControllerNode = TRUE;
    }

    if (ControllerNumber != NULL) {
      *ControllerNumber = ((CONTROLLER_DEVICE_PATH *)DevicePath)->ControllerNumber;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    if (ContainsControllerNode != NULL) {
      *ContainsControllerNode = FALSE;
    }
  }

  return (UART_DEVICE_PATH *)DevicePath;
}

/**
  Checks whether the UART parameters are valid and computes the Divisor.

  @param  ClockRate      The clock rate of the serial device used to verify
                         the BaudRate. Do not verify the BaudRate if it's 0.
  @param  BaudRate       The requested baudrate of the serial device.
  @param  DataBits       Number of databits used in serial device.
  @param  Parity         The type of parity used in serial device.
  @param  StopBits       Number of stopbits used in serial device.
  @param  Divisor        Return the divisor if ClockRate is not 0.
  @param  ActualBaudRate Return the actual supported baudrate without
                         exceeding BaudRate. NULL means baudrate degradation
                         is not allowed.
                         If the requested BaudRate is not supported, the routine
                         returns TRUE and the Actual Baud Rate when ActualBaudRate
                         is not NULL, returns FALSE when ActualBaudRate is NULL.

  @retval TRUE   The UART parameters are valid.
  @retval FALSE  The UART parameters are not valid.
**/
BOOLEAN
VerifyUartParameters (
  IN     UINT32              ClockRate,
  IN     UINT64              BaudRate,
  IN     UINT8               DataBits,
  IN     EFI_PARITY_TYPE     Parity,
  IN     EFI_STOP_BITS_TYPE  StopBits,
  OUT UINT64                 *Divisor,
  OUT UINT64                 *ActualBaudRate
  )
{
  UINT64  Remainder;
  UINT32  ComputedBaudRate;
  UINT64  ComputedDivisor;
  UINT64  Percent;

  if ((DataBits < 5) || (DataBits > 8) ||
      (Parity < NoParity) || (Parity > SpaceParity) ||
      (StopBits < OneStopBit) || (StopBits > TwoStopBits) ||
      ((DataBits == 5) && (StopBits == TwoStopBits)) ||
      ((DataBits >= 6) && (DataBits <= 8) && (StopBits == OneFiveStopBits))
      )
  {
    return FALSE;
  }

  //
  // Do not verify the baud rate if clock rate is unknown (0).
  //
  if (ClockRate == 0) {
    return TRUE;
  }

  //
  // Compute divisor use to program the baud rate using a round determination
  // Divisor = ClockRate / 16 / BaudRate = ClockRate / (16 * BaudRate)
  //         = ClockRate / (BaudRate << 4)
  //
  ComputedDivisor = DivU64x64Remainder (ClockRate, LShiftU64 (BaudRate, 4), &Remainder);
  //
  // Round Divisor up by 1 if the Remainder is more than half (16 * BaudRate)
  // BaudRate * 16 / 2 = BaudRate * 8 = (BaudRate << 3)
  //
  if (Remainder >= LShiftU64 (BaudRate, 3)) {
    ComputedDivisor++;
  }

  //
  // If the computed divisor is larger than the maximum value that can be programmed
  // into the UART, then the requested baud rate can not be supported.
  //
  if (ComputedDivisor > MAX_UINT16) {
    return FALSE;
  }

  //
  // If the computed divisor is 0, then use a computed divisor of 1, which will select
  // the maximum supported baud rate.
  //
  if (ComputedDivisor == 0) {
    ComputedDivisor = 1;
  }

  //
  // Actual baud rate that the serial port will be programmed for
  // should be with in 4% of requested one.
  //
  ComputedBaudRate = ClockRate / ((UINT16)ComputedDivisor << 4);
  if (ComputedBaudRate == 0) {
    return FALSE;
  }

  Percent = DivU64x32 (MultU64x32 (BaudRate, 100), ComputedBaudRate);
  DEBUG ((DEBUG_INFO, "ClockRate = %d\n", ClockRate));
  DEBUG ((DEBUG_INFO, "Divisor   = %ld\n", ComputedDivisor));
  DEBUG ((DEBUG_INFO, "BaudRate/Actual (%ld/%d) = %d%%\n", BaudRate, ComputedBaudRate, Percent));

  //
  // If the requested BaudRate is not supported:
  //  Returns TRUE and the Actual Baud Rate when ActualBaudRate is not NULL;
  //  Returns FALSE when ActualBaudRate is NULL.
  //
  if ((Percent >= 96) && (Percent <= 104)) {
    if (ActualBaudRate != NULL) {
      *ActualBaudRate = BaudRate;
    }

    if (Divisor != NULL) {
      *Divisor = ComputedDivisor;
    }

    return TRUE;
  }

  if (ComputedBaudRate < BaudRate) {
    if (ActualBaudRate != NULL) {
      *ActualBaudRate = ComputedBaudRate;
    }

    if (Divisor != NULL) {
      *Divisor = ComputedDivisor;
    }

    return TRUE;
  }

  //
  // ActualBaudRate is higher than requested baud rate and more than 4%
  // higher than the requested value.  Increment Divisor if it is less
  // than MAX_UINT16 and computed baud rate with new divisor.
  //
  if (ComputedDivisor == MAX_UINT16) {
    return FALSE;
  }

  ComputedDivisor++;
  ComputedBaudRate = ClockRate / ((UINT16)ComputedDivisor << 4);
  if (ComputedBaudRate == 0) {
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "ClockRate = %d\n", ClockRate));
  DEBUG ((DEBUG_INFO, "Divisor   = %ld\n", ComputedDivisor));
  DEBUG ((DEBUG_INFO, "BaudRate/Actual (%ld/%d) = %d%%\n", BaudRate, ComputedBaudRate, Percent));

  if (ActualBaudRate != NULL) {
    *ActualBaudRate = ComputedBaudRate;
  }

  if (Divisor != NULL) {
    *Divisor = ComputedDivisor;
  }

  return TRUE;
}

/**
  Detect whether specific FIFO is full or not.

  @param Fifo    A pointer to the Data Structure SERIAL_DEV_FIFO

  @return whether specific FIFO is full or not
**/
BOOLEAN
SerialFifoFull (
  IN SERIAL_DEV_FIFO  *Fifo
  )
{
  return (BOOLEAN)(((Fifo->Tail + 1) % SERIAL_MAX_FIFO_SIZE) == Fifo->Head);
}

/**
  Detect whether specific FIFO is empty or not.

  @param  Fifo    A pointer to the Data Structure SERIAL_DEV_FIFO

  @return whether specific FIFO is empty or not
**/
BOOLEAN
SerialFifoEmpty (
  IN SERIAL_DEV_FIFO  *Fifo
  )

{
  return (BOOLEAN)(Fifo->Head == Fifo->Tail);
}

/**
  Add data to specific FIFO.

  @param Fifo                  A pointer to the Data Structure SERIAL_DEV_FIFO
  @param Data                  the data added to FIFO

  @retval EFI_SUCCESS           Add data to specific FIFO successfully
  @retval EFI_OUT_OF_RESOURCE   Failed to add data because FIFO is already full
**/
EFI_STATUS
SerialFifoAdd (
  IN OUT SERIAL_DEV_FIFO  *Fifo,
  IN     UINT8            Data
  )
{
  //
  // if FIFO full can not add data
  //
  if (SerialFifoFull (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // FIFO is not full can add data
  //
  Fifo->Data[Fifo->Tail] = Data;
  Fifo->Tail             = (Fifo->Tail + 1) % SERIAL_MAX_FIFO_SIZE;
  return EFI_SUCCESS;
}

/**
  Remove data from specific FIFO.

  @param Fifo                  A pointer to the Data Structure SERIAL_DEV_FIFO
  @param Data                  the data removed from FIFO

  @retval EFI_SUCCESS           Remove data from specific FIFO successfully
  @retval EFI_OUT_OF_RESOURCE   Failed to remove data because FIFO is empty

**/
EFI_STATUS
SerialFifoRemove (
  IN OUT SERIAL_DEV_FIFO  *Fifo,
  OUT    UINT8            *Data
  )
{
  //
  // if FIFO is empty, no data can remove
  //
  if (SerialFifoEmpty (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // FIFO is not empty, can remove data
  //
  *Data      = Fifo->Data[Fifo->Head];
  Fifo->Head = (Fifo->Head + 1) % SERIAL_MAX_FIFO_SIZE;
  return EFI_SUCCESS;
}

/**
  Reads and writes all available data.

  @param SerialDevice           The device to transmit.

  @retval EFI_SUCCESS           Data was read/written successfully.
  @retval EFI_OUT_OF_RESOURCE   Failed because software receive FIFO is full.  Note, when
                                this happens, pending writes are not done.

**/
EFI_STATUS
SerialReceiveTransmit (
  IN SERIAL_DEV  *SerialDevice
  )

{
  SERIAL_PORT_LSR  Lsr;
  UINT8            Data;
  BOOLEAN          ReceiveFifoFull;
  SERIAL_PORT_MSR  Msr;
  SERIAL_PORT_MCR  Mcr;
  UINTN            TimeOut;

  Data = 0;

  //
  // Begin the read or write
  //
  if (SerialDevice->SoftwareLoopbackEnable) {
    do {
      ReceiveFifoFull = SerialFifoFull (&SerialDevice->Receive);
      if (!SerialFifoEmpty (&SerialDevice->Transmit)) {
        SerialFifoRemove (&SerialDevice->Transmit, &Data);
        if (ReceiveFifoFull) {
          return EFI_OUT_OF_RESOURCES;
        }

        SerialFifoAdd (&SerialDevice->Receive, Data);
      }
    } while (!SerialFifoEmpty (&SerialDevice->Transmit));
  } else {
    ReceiveFifoFull = SerialFifoFull (&SerialDevice->Receive);
    //
    // For full handshake flow control, tell the peer to send data
    // if receive buffer is available.
    //
    if (SerialDevice->HardwareFlowControl &&
        !FeaturePcdGet (PcdSerialUseHalfHandshake) &&
        !ReceiveFifoFull
        )
    {
      Mcr.Data     = READ_MCR (SerialDevice);
      Mcr.Bits.Rts = 1;
      WRITE_MCR (SerialDevice, Mcr.Data);
    }

    do {
      Lsr.Data = READ_LSR (SerialDevice);

      //
      // Flush incomming data to prevent a an overrun during a long write
      //
      if ((Lsr.Bits.Dr == 1) && !ReceiveFifoFull) {
        ReceiveFifoFull = SerialFifoFull (&SerialDevice->Receive);
        if (!ReceiveFifoFull) {
          if ((Lsr.Bits.FIFOe == 1) || (Lsr.Bits.Oe == 1) || (Lsr.Bits.Pe == 1) || (Lsr.Bits.Fe == 1) || (Lsr.Bits.Bi == 1)) {
            REPORT_STATUS_CODE_WITH_DEVICE_PATH (
              EFI_ERROR_CODE,
              EFI_P_EC_INPUT_ERROR | EFI_PERIPHERAL_SERIAL_PORT,
              SerialDevice->DevicePath
              );
            if ((Lsr.Bits.FIFOe == 1) || (Lsr.Bits.Pe == 1) || (Lsr.Bits.Fe == 1) || (Lsr.Bits.Bi == 1)) {
              Data = READ_RBR (SerialDevice);
              continue;
            }
          }

          Data = READ_RBR (SerialDevice);

          SerialFifoAdd (&SerialDevice->Receive, Data);

          //
          // For full handshake flow control, if receive buffer full
          // tell the peer to stop sending data.
          //
          if (SerialDevice->HardwareFlowControl &&
              !FeaturePcdGet (PcdSerialUseHalfHandshake)   &&
              SerialFifoFull (&SerialDevice->Receive)
              )
          {
            Mcr.Data     = READ_MCR (SerialDevice);
            Mcr.Bits.Rts = 0;
            WRITE_MCR (SerialDevice, Mcr.Data);
          }

          continue;
        } else {
          REPORT_STATUS_CODE_WITH_DEVICE_PATH (
            EFI_PROGRESS_CODE,
            EFI_P_SERIAL_PORT_PC_CLEAR_BUFFER | EFI_PERIPHERAL_SERIAL_PORT,
            SerialDevice->DevicePath
            );
        }
      }

      //
      // Do the write
      //
      if ((Lsr.Bits.Thre == 1) && !SerialFifoEmpty (&SerialDevice->Transmit)) {
        //
        // Make sure the transmit data will not be missed
        //
        if (SerialDevice->HardwareFlowControl) {
          //
          // For half handshake flow control assert RTS before sending.
          //
          if (FeaturePcdGet (PcdSerialUseHalfHandshake)) {
            Mcr.Data     = READ_MCR (SerialDevice);
            Mcr.Bits.Rts = 0;
            WRITE_MCR (SerialDevice, Mcr.Data);
          }

          //
          // Wait for CTS
          //
          TimeOut  = 0;
          Msr.Data = READ_MSR (SerialDevice);
          while ((Msr.Bits.Dcd == 1) && ((Msr.Bits.Cts == 0) ^ FeaturePcdGet (PcdSerialUseHalfHandshake))) {
            gBS->Stall (TIMEOUT_STALL_INTERVAL);
            TimeOut++;
            if (TimeOut > 5) {
              break;
            }

            Msr.Data = READ_MSR (SerialDevice);
          }

          if ((Msr.Bits.Dcd == 0) || ((Msr.Bits.Cts == 1) ^ FeaturePcdGet (PcdSerialUseHalfHandshake))) {
            SerialFifoRemove (&SerialDevice->Transmit, &Data);
            WRITE_THR (SerialDevice, Data);
          }

          //
          // For half handshake flow control, tell DCE we are done.
          //
          if (FeaturePcdGet (PcdSerialUseHalfHandshake)) {
            Mcr.Data     = READ_MCR (SerialDevice);
            Mcr.Bits.Rts = 1;
            WRITE_MCR (SerialDevice, Mcr.Data);
          }
        } else {
          SerialFifoRemove (&SerialDevice->Transmit, &Data);
          WRITE_THR (SerialDevice, Data);
        }
      }
    } while (Lsr.Bits.Thre == 1 && !SerialFifoEmpty (&SerialDevice->Transmit));
  }

  return EFI_SUCCESS;
}

/**
  Flush the serial hardware transmit FIFO, holding register, and shift register.

  @param SerialDevice  The device to flush.

  @retval  EFI_SUCCESS  The transmit FIFO is completely flushed.
  @retval  EFI_TIMEOUT  A timeout occured waiting for the transmit FIFO to flush.
**/
EFI_STATUS
SerialFlushTransmitFifo (
  SERIAL_DEV  *SerialDevice
  )
{
  SERIAL_PORT_LSR  Lsr;
  UINTN            Timeout;
  UINTN            Elapsed;

  //
  // If this is the first time the UART is being configured, then the current
  // UART settings are not known, so compute a timeout to wait for the Tx FIFO
  // assuming the worst case current settings.
  //
  // Timeout = (Max Bits per Char) * (Max Pending Chars) / (Slowest Baud Rate)
  //   Max Bits per Char = Start bit + 8 data bits + parity + 2 stop bits = 12
  //   Max Pending Chars = Largest Tx FIFO + hold + shift = 64 + 1 + 1 = 66
  //   Slowest Reasonable Baud Rate = 300 baud
  // Timeout = 12 * 66 / 300 = 2.64 seconds = 2,640,000 uS
  //
  Timeout = 2640000;

  //
  // Use the largest of the computed timeout, the default timeout, and the
  // currently set timeout.
  //
  Timeout = MAX (Timeout, SERIAL_PORT_DEFAULT_TIMEOUT);
  Timeout = MAX (Timeout, SerialDevice->SerialMode.Timeout);

  //
  // Wait for the shortest time possible for the serial port to be ready making
  // sure the transmit FIFO, holding register, and shift register are all
  // empty.  The actual wait time is expected to be very small because the
  // number characters currently in the FIFO should be small when a
  // configuration change is requested.
  //
  // NOTE: Do not use any DEBUG() or REPORT_STATUS_CODE() or any other calls
  // in the rest of this function that may send additional characters to this
  // UART device invalidating the flush operation.
  //
  Elapsed  = 0;
  Lsr.Data = READ_LSR (SerialDevice);
  while (Lsr.Bits.Temt == 0 || Lsr.Bits.Thre == 0) {
    if (Elapsed >= Timeout) {
      return EFI_TIMEOUT;
    }

    gBS->Stall (TIMEOUT_STALL_INTERVAL);
    Elapsed += TIMEOUT_STALL_INTERVAL;
    Lsr.Data = READ_LSR (SerialDevice);
  }

  return EFI_SUCCESS;
}

//
// Interface Functions
//

/**
  Reset serial device.

  @param This               Pointer to EFI_SERIAL_IO_PROTOCOL

  @retval EFI_SUCCESS        Reset successfully
  @retval EFI_DEVICE_ERROR   Failed to reset

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  )
{
  EFI_STATUS       Status;
  SERIAL_DEV       *SerialDevice;
  SERIAL_PORT_LCR  Lcr;
  SERIAL_PORT_IER  Ier;
  SERIAL_PORT_MCR  Mcr;
  SERIAL_PORT_FCR  Fcr;
  EFI_TPL          Tpl;
  UINT32           Control;

  SerialDevice = SERIAL_DEV_FROM_THIS (This);

  //
  // Report the status code reset the serial
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_P_PC_RESET | EFI_PERIPHERAL_SERIAL_PORT,
    SerialDevice->DevicePath
    );

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Wait for all data to be transmitted before changing the UART configuration.
  //
  // NOTE: Do not use any DEBUG() or REPORT_STATUS_CODE() or any other calls
  // that may send additional characters to this UART device until the UART
  // configuration change is complete.
  //
  SerialFlushTransmitFifo (SerialDevice);

  //
  // Make sure DLAB is 0.
  //
  Lcr.Data      = READ_LCR (SerialDevice);
  Lcr.Bits.DLab = 0;
  WRITE_LCR (SerialDevice, Lcr.Data);

  //
  // Turn off all interrupts
  //
  Ier.Data       = READ_IER (SerialDevice);
  Ier.Bits.Ravie = 0;
  Ier.Bits.Theie = 0;
  Ier.Bits.Rie   = 0;
  Ier.Bits.Mie   = 0;
  WRITE_IER (SerialDevice, Ier.Data);

  //
  // Reset the FIFO
  //
  Fcr.Data         = 0;
  Fcr.Bits.TrFIFOE = 0;
  WRITE_FCR (SerialDevice, Fcr.Data);

  //
  // Turn off loopback and disable device interrupt.
  //
  Mcr.Data      = READ_MCR (SerialDevice);
  Mcr.Bits.Out1 = 0;
  Mcr.Bits.Out2 = 0;
  Mcr.Bits.Lme  = 0;
  WRITE_MCR (SerialDevice, Mcr.Data);

  //
  // Clear the scratch pad register
  //
  WRITE_SCR (SerialDevice, 0);

  //
  // Enable FIFO
  //
  Fcr.Bits.TrFIFOE = 1;
  if (SerialDevice->ReceiveFifoDepth > 16) {
    Fcr.Bits.TrFIFO64 = 1;
  }

  Fcr.Bits.ResetRF = 1;
  Fcr.Bits.ResetTF = 1;
  WRITE_FCR (SerialDevice, Fcr.Data);

  //
  // Go set the current attributes
  //
  Status = This->SetAttributes (
                   This,
                   This->Mode->BaudRate,
                   This->Mode->ReceiveFifoDepth,
                   This->Mode->Timeout,
                   (EFI_PARITY_TYPE)This->Mode->Parity,
                   (UINT8)This->Mode->DataBits,
                   (EFI_STOP_BITS_TYPE)This->Mode->StopBits
                   );

  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  //
  // Go set the current control bits
  //
  Control = 0;
  if (SerialDevice->HardwareFlowControl) {
    Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
  }

  if (SerialDevice->SoftwareLoopbackEnable) {
    Control |= EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;
  }

  Status = This->SetControl (
                   This,
                   Control
                   );

  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  //
  // Reset the software FIFO
  //
  SerialDevice->Receive.Head  = SerialDevice->Receive.Tail = 0;
  SerialDevice->Transmit.Head = SerialDevice->Transmit.Tail = 0;
  gBS->RestoreTPL (Tpl);

  //
  // Device reset is complete
  //
  return EFI_SUCCESS;
}

/**
  Set new attributes to a serial device.

  @param This                     Pointer to EFI_SERIAL_IO_PROTOCOL
  @param  BaudRate                 The baudrate of the serial device
  @param  ReceiveFifoDepth         The depth of receive FIFO buffer
  @param  Timeout                  The request timeout for a single char
  @param  Parity                   The type of parity used in serial device
  @param  DataBits                 Number of databits used in serial device
  @param  StopBits                 Number of stopbits used in serial device

  @retval  EFI_SUCCESS              The new attributes were set
  @retval  EFI_INVALID_PARAMETERS   One or more attributes have an unsupported value
  @retval  EFI_UNSUPPORTED          Data Bits can not set to 5 or 6
  @retval  EFI_DEVICE_ERROR         The serial device is not functioning correctly (no return)

**/
EFI_STATUS
EFIAPI
SerialSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT64                  BaudRate,
  IN UINT32                  ReceiveFifoDepth,
  IN UINT32                  Timeout,
  IN EFI_PARITY_TYPE         Parity,
  IN UINT8                   DataBits,
  IN EFI_STOP_BITS_TYPE      StopBits
  )
{
  EFI_STATUS        Status;
  SERIAL_DEV        *SerialDevice;
  UINT64            Divisor;
  SERIAL_PORT_LCR   Lcr;
  UART_DEVICE_PATH  *Uart;
  EFI_TPL           Tpl;

  SerialDevice = SERIAL_DEV_FROM_THIS (This);

  //
  // Check for default settings and fill in actual values.
  //
  if (BaudRate == 0) {
    BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  }

  if (ReceiveFifoDepth == 0) {
    ReceiveFifoDepth = SerialDevice->ReceiveFifoDepth;
  }

  if (Timeout == 0) {
    Timeout = SERIAL_PORT_DEFAULT_TIMEOUT;
  }

  if (Parity == DefaultParity) {
    Parity = (EFI_PARITY_TYPE)PcdGet8 (PcdUartDefaultParity);
  }

  if (DataBits == 0) {
    DataBits = PcdGet8 (PcdUartDefaultDataBits);
  }

  if (StopBits == DefaultStopBits) {
    StopBits = (EFI_STOP_BITS_TYPE)PcdGet8 (PcdUartDefaultStopBits);
  }

  if (!VerifyUartParameters (SerialDevice->ClockRate, BaudRate, DataBits, Parity, StopBits, &Divisor, &BaudRate)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((ReceiveFifoDepth == 0) || (ReceiveFifoDepth > SerialDevice->ReceiveFifoDepth)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Timeout < SERIAL_PORT_MIN_TIMEOUT) || (Timeout > SERIAL_PORT_MAX_TIMEOUT)) {
    return EFI_INVALID_PARAMETER;
  }

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Wait for all data to be transmitted before changing the UART configuration.
  //
  // NOTE: Do not use any DEBUG() or REPORT_STATUS_CODE() or any other calls
  // that may send additional characters to this UART device until the UART
  // configuration change is complete.
  //
  SerialFlushTransmitFifo (SerialDevice);

  //
  // Put serial port on Divisor Latch Mode
  //
  Lcr.Data      = READ_LCR (SerialDevice);
  Lcr.Bits.DLab = 1;
  WRITE_LCR (SerialDevice, Lcr.Data);

  //
  // Write the divisor to the serial port
  //
  WRITE_DLL (SerialDevice, (UINT8)Divisor);
  WRITE_DLM (SerialDevice, (UINT8)((UINT16)Divisor >> 8));

  //
  // Put serial port back in normal mode and set remaining attributes.
  //
  Lcr.Bits.DLab = 0;

  switch (Parity) {
    case NoParity:
      Lcr.Bits.ParEn   = 0;
      Lcr.Bits.EvenPar = 0;
      Lcr.Bits.SticPar = 0;
      break;

    case EvenParity:
      Lcr.Bits.ParEn   = 1;
      Lcr.Bits.EvenPar = 1;
      Lcr.Bits.SticPar = 0;
      break;

    case OddParity:
      Lcr.Bits.ParEn   = 1;
      Lcr.Bits.EvenPar = 0;
      Lcr.Bits.SticPar = 0;
      break;

    case SpaceParity:
      Lcr.Bits.ParEn   = 1;
      Lcr.Bits.EvenPar = 1;
      Lcr.Bits.SticPar = 1;
      break;

    case MarkParity:
      Lcr.Bits.ParEn   = 1;
      Lcr.Bits.EvenPar = 0;
      Lcr.Bits.SticPar = 1;
      break;

    default:
      break;
  }

  switch (StopBits) {
    case OneStopBit:
      Lcr.Bits.StopB = 0;
      break;

    case OneFiveStopBits:
    case TwoStopBits:
      Lcr.Bits.StopB = 1;
      break;

    default:
      break;
  }

  //
  // DataBits
  //
  Lcr.Bits.SerialDB = (UINT8)((DataBits - 5) & 0x03);
  WRITE_LCR (SerialDevice, Lcr.Data);

  //
  // Set the Serial I/O mode
  //
  This->Mode->BaudRate         = BaudRate;
  This->Mode->ReceiveFifoDepth = ReceiveFifoDepth;
  This->Mode->Timeout          = Timeout;
  This->Mode->Parity           = Parity;
  This->Mode->DataBits         = DataBits;
  This->Mode->StopBits         = StopBits;

  //
  // See if Device Path Node has actually changed
  //
  if ((SerialDevice->UartDevicePath.BaudRate == BaudRate) &&
      (SerialDevice->UartDevicePath.DataBits == DataBits) &&
      (SerialDevice->UartDevicePath.Parity == Parity) &&
      (SerialDevice->UartDevicePath.StopBits == StopBits)
      )
  {
    gBS->RestoreTPL (Tpl);
    return EFI_SUCCESS;
  }

  //
  // Update the device path
  //
  SerialDevice->UartDevicePath.BaudRate = BaudRate;
  SerialDevice->UartDevicePath.DataBits = DataBits;
  SerialDevice->UartDevicePath.Parity   = (UINT8)Parity;
  SerialDevice->UartDevicePath.StopBits = (UINT8)StopBits;

  Status = EFI_SUCCESS;
  if (SerialDevice->Handle != NULL) {
    //
    // Skip the optional Controller device path node
    //
    Uart = SkipControllerDevicePathNode (
             (EFI_DEVICE_PATH_PROTOCOL *)(
                                          (UINT8 *)SerialDevice->DevicePath + GetDevicePathSize (SerialDevice->ParentDevicePath) - END_DEVICE_PATH_LENGTH
                                          ),
             NULL,
             NULL
             );
    CopyMem (Uart, &SerialDevice->UartDevicePath, sizeof (UART_DEVICE_PATH));
    Status = gBS->ReinstallProtocolInterface (
                    SerialDevice->Handle,
                    &gEfiDevicePathProtocolGuid,
                    SerialDevice->DevicePath,
                    SerialDevice->DevicePath
                    );
  }

  gBS->RestoreTPL (Tpl);

  return Status;
}

/**
  Set Control Bits.

  @param This              Pointer to EFI_SERIAL_IO_PROTOCOL
  @param Control           Control bits that can be settable

  @retval EFI_SUCCESS       New Control bits were set successfully
  @retval EFI_UNSUPPORTED   The Control bits wanted to set are not supported

**/
EFI_STATUS
EFIAPI
SerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  )
{
  SERIAL_DEV                     *SerialDevice;
  SERIAL_PORT_MCR                Mcr;
  EFI_TPL                        Tpl;
  UART_FLOW_CONTROL_DEVICE_PATH  *FlowControl;
  EFI_STATUS                     Status;

  //
  // The control bits that can be set are :
  //     EFI_SERIAL_DATA_TERMINAL_READY: 0x0001  // WO
  //     EFI_SERIAL_REQUEST_TO_SEND: 0x0002  // WO
  //     EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE: 0x1000  // RW
  //     EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE: 0x2000  // RW
  //     EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE: 0x4000 // RW
  //
  SerialDevice = SERIAL_DEV_FROM_THIS (This);

  //
  // first determine the parameter is invalid
  //
  if ((Control & (~(EFI_SERIAL_REQUEST_TO_SEND | EFI_SERIAL_DATA_TERMINAL_READY |
                    EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE | EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE |
                    EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE))) != 0)
  {
    return EFI_UNSUPPORTED;
  }

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Wait for all data to be transmitted before changing the UART configuration.
  //
  // NOTE: Do not use any DEBUG() or REPORT_STATUS_CODE() or any other calls
  // that may send additional characters to this UART device until the UART
  // configuration change is complete.
  //
  SerialFlushTransmitFifo (SerialDevice);

  Mcr.Data                             = READ_MCR (SerialDevice);
  Mcr.Bits.DtrC                        = 0;
  Mcr.Bits.Rts                         = 0;
  Mcr.Bits.Lme                         = 0;
  SerialDevice->SoftwareLoopbackEnable = FALSE;
  SerialDevice->HardwareFlowControl    = FALSE;

  if ((Control & EFI_SERIAL_DATA_TERMINAL_READY) == EFI_SERIAL_DATA_TERMINAL_READY) {
    Mcr.Bits.DtrC = 1;
  }

  if ((Control & EFI_SERIAL_REQUEST_TO_SEND) == EFI_SERIAL_REQUEST_TO_SEND) {
    Mcr.Bits.Rts = 1;
  }

  if ((Control & EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE) == EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE) {
    Mcr.Bits.Lme = 1;
  }

  if ((Control & EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) == EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) {
    SerialDevice->HardwareFlowControl = TRUE;
  }

  WRITE_MCR (SerialDevice, Mcr.Data);

  if ((Control & EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE) == EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE) {
    SerialDevice->SoftwareLoopbackEnable = TRUE;
  }

  Status = EFI_SUCCESS;
  if (SerialDevice->Handle != NULL) {
    FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *)(
                                                    (UINTN)SerialDevice->DevicePath
                                                    + GetDevicePathSize (SerialDevice->ParentDevicePath)
                                                    - END_DEVICE_PATH_LENGTH
                                                    + sizeof (UART_DEVICE_PATH)
                                                    );
    if (IsUartFlowControlDevicePathNode (FlowControl) &&
        ((BOOLEAN)(ReadUnaligned32 (&FlowControl->FlowControlMap) == UART_FLOW_CONTROL_HARDWARE) != SerialDevice->HardwareFlowControl))
    {
      //
      // Flow Control setting is changed, need to reinstall device path protocol
      //
      WriteUnaligned32 (&FlowControl->FlowControlMap, SerialDevice->HardwareFlowControl ? UART_FLOW_CONTROL_HARDWARE : 0);
      Status = gBS->ReinstallProtocolInterface (
                      SerialDevice->Handle,
                      &gEfiDevicePathProtocolGuid,
                      SerialDevice->DevicePath,
                      SerialDevice->DevicePath
                      );
    }
  }

  gBS->RestoreTPL (Tpl);

  return Status;
}

/**
  Get ControlBits.

  @param This          Pointer to EFI_SERIAL_IO_PROTOCOL
  @param Control       Control signals of the serial device

  @retval EFI_SUCCESS   Get Control signals successfully

**/
EFI_STATUS
EFIAPI
SerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  )
{
  SERIAL_DEV       *SerialDevice;
  SERIAL_PORT_MSR  Msr;
  SERIAL_PORT_MCR  Mcr;
  EFI_TPL          Tpl;

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  SerialDevice = SERIAL_DEV_FROM_THIS (This);

  *Control = 0;

  //
  // Read the Modem Status Register
  //
  Msr.Data = READ_MSR (SerialDevice);

  if (Msr.Bits.Cts == 1) {
    *Control |= EFI_SERIAL_CLEAR_TO_SEND;
  }

  if (Msr.Bits.Dsr == 1) {
    *Control |= EFI_SERIAL_DATA_SET_READY;
  }

  if (Msr.Bits.Ri == 1) {
    *Control |= EFI_SERIAL_RING_INDICATE;
  }

  if (Msr.Bits.Dcd == 1) {
    *Control |= EFI_SERIAL_CARRIER_DETECT;
  }

  //
  // Read the Modem Control Register
  //
  Mcr.Data = READ_MCR (SerialDevice);

  if (Mcr.Bits.DtrC == 1) {
    *Control |= EFI_SERIAL_DATA_TERMINAL_READY;
  }

  if (Mcr.Bits.Rts == 1) {
    *Control |= EFI_SERIAL_REQUEST_TO_SEND;
  }

  if (Mcr.Bits.Lme == 1) {
    *Control |= EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
  }

  if (SerialDevice->HardwareFlowControl) {
    *Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
  }

  //
  // Update FIFO status
  //
  SerialReceiveTransmit (SerialDevice);

  //
  // See if the Transmit FIFO is empty
  //
  if (SerialFifoEmpty (&SerialDevice->Transmit)) {
    *Control |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
  }

  //
  // See if the Receive FIFO is empty.
  //
  if (SerialFifoEmpty (&SerialDevice->Receive)) {
    *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  if (SerialDevice->SoftwareLoopbackEnable) {
    *Control |= EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;
  }

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

/**
  Write the specified number of bytes to serial device.

  @param This               Pointer to EFI_SERIAL_IO_PROTOCOL
  @param  BufferSize         On input the size of Buffer, on output the amount of
                       data actually written
  @param  Buffer             The buffer of data to write

  @retval EFI_SUCCESS        The data were written successfully
  @retval EFI_DEVICE_ERROR   The device reported an error
  @retval EFI_TIMEOUT        The write operation was stopped due to timeout

**/
EFI_STATUS
EFIAPI
SerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  )
{
  SERIAL_DEV  *SerialDevice;
  UINT8       *CharBuffer;
  UINT32      Index;
  UINTN       Elapsed;
  UINTN       ActualWrite;
  EFI_TPL     Tpl;
  UINTN       Timeout;
  UINTN       BitsPerCharacter;

  SerialDevice = SERIAL_DEV_FROM_THIS (This);
  Elapsed      = 0;
  ActualWrite  = 0;

  if (*BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if (Buffer == NULL) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE,
      EFI_P_EC_OUTPUT_ERROR | EFI_PERIPHERAL_SERIAL_PORT,
      SerialDevice->DevicePath
      );

    return EFI_DEVICE_ERROR;
  }

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  CharBuffer = (UINT8 *)Buffer;

  //
  // Compute the number of bits in a single character.  This is a start bit,
  // followed by the number of data bits, followed by the number of stop bits.
  // The number of stop bits is specified by an enumeration that includes
  // support for 1.5 stop bits.  Treat 1.5 stop bits as 2 stop bits.
  //
  BitsPerCharacter =
    1 +
    This->Mode->DataBits +
    ((This->Mode->StopBits == TwoStopBits) ? 2 : This->Mode->StopBits);

  //
  // Compute the timeout in microseconds to wait for a single byte to be
  // transmitted.  The Mode structure contans a Timeout field that is the
  // maximum time to transmit or receive a character.  However, many UARTs
  // have a FIFO for transmits, so the time required to add one new character
  // to the transmit FIFO may be the time required to flush a full FIFO.  If
  // the Timeout in the Mode structure is smaller than the time required to
  // flush a full FIFO at the current baud rate, then use a timeout value that
  // is required to flush a full transmit FIFO.
  //
  Timeout = MAX (
              This->Mode->Timeout,
              (UINTN)DivU64x64Remainder (
                       BitsPerCharacter * (SerialDevice->TransmitFifoDepth + 1) * 1000000,
                       This->Mode->BaudRate,
                       NULL
                       )
              );

  for (Index = 0; Index < *BufferSize; Index++) {
    SerialFifoAdd (&SerialDevice->Transmit, CharBuffer[Index]);

    while (SerialReceiveTransmit (SerialDevice) != EFI_SUCCESS || !SerialFifoEmpty (&SerialDevice->Transmit)) {
      //
      //  Unsuccessful write so check if timeout has expired, if not,
      //  stall for a bit, increment time elapsed, and try again
      //
      if (Elapsed >= Timeout) {
        *BufferSize = ActualWrite;
        gBS->RestoreTPL (Tpl);
        return EFI_TIMEOUT;
      }

      gBS->Stall (TIMEOUT_STALL_INTERVAL);

      Elapsed += TIMEOUT_STALL_INTERVAL;
    }

    ActualWrite++;
    //
    //  Successful write so reset timeout
    //
    Elapsed = 0;
  }

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

/**
  Read the specified number of bytes from serial device.

  @param This               Pointer to EFI_SERIAL_IO_PROTOCOL
  @param BufferSize         On input the size of Buffer, on output the amount of
                            data returned in buffer
  @param Buffer             The buffer to return the data into

  @retval EFI_SUCCESS        The data were read successfully
  @retval EFI_DEVICE_ERROR   The device reported an error
  @retval EFI_TIMEOUT        The read operation was stopped due to timeout

**/
EFI_STATUS
EFIAPI
SerialRead (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  )
{
  SERIAL_DEV  *SerialDevice;
  UINT32      Index;
  UINT8       *CharBuffer;
  UINTN       Elapsed;
  EFI_STATUS  Status;
  EFI_TPL     Tpl;

  SerialDevice = SERIAL_DEV_FROM_THIS (This);
  Elapsed      = 0;

  if (*BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if (Buffer == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  Status = SerialReceiveTransmit (SerialDevice);

  if (EFI_ERROR (Status)) {
    *BufferSize = 0;

    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE,
      EFI_P_EC_INPUT_ERROR | EFI_PERIPHERAL_SERIAL_PORT,
      SerialDevice->DevicePath
      );

    gBS->RestoreTPL (Tpl);

    return EFI_DEVICE_ERROR;
  }

  CharBuffer = (UINT8 *)Buffer;
  for (Index = 0; Index < *BufferSize; Index++) {
    while (SerialFifoRemove (&SerialDevice->Receive, &(CharBuffer[Index])) != EFI_SUCCESS) {
      //
      //  Unsuccessful read so check if timeout has expired, if not,
      //  stall for a bit, increment time elapsed, and try again
      //  Need this time out to get conspliter to work.
      //
      if (Elapsed >= This->Mode->Timeout) {
        *BufferSize = Index;
        gBS->RestoreTPL (Tpl);
        return EFI_TIMEOUT;
      }

      gBS->Stall (TIMEOUT_STALL_INTERVAL);
      Elapsed += TIMEOUT_STALL_INTERVAL;

      Status = SerialReceiveTransmit (SerialDevice);
      if (Status == EFI_DEVICE_ERROR) {
        *BufferSize = Index;
        gBS->RestoreTPL (Tpl);
        return EFI_DEVICE_ERROR;
      }
    }

    //
    //  Successful read so reset timeout
    //
    Elapsed = 0;
  }

  SerialReceiveTransmit (SerialDevice);

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

/**
  Use scratchpad register to test if this serial port is present.

  @param SerialDevice   Pointer to serial device structure

  @return if this serial port is present
**/
BOOLEAN
SerialPresent (
  IN SERIAL_DEV  *SerialDevice
  )

{
  UINT8    Temp;
  BOOLEAN  Status;

  Status = TRUE;

  //
  // Save SCR reg
  //
  Temp = READ_SCR (SerialDevice);
  WRITE_SCR (SerialDevice, 0xAA);

  if (READ_SCR (SerialDevice) != 0xAA) {
    Status = FALSE;
  }

  WRITE_SCR (SerialDevice, 0x55);

  if (READ_SCR (SerialDevice) != 0x55) {
    Status = FALSE;
  }

  //
  // Restore SCR
  //
  WRITE_SCR (SerialDevice, Temp);
  return Status;
}

/**
  Read serial port.

  @param SerialDev     Pointer to serial device
  @param Offset        Offset in register group

  @return Data read from serial port

**/
UINT8
SerialReadRegister (
  IN SERIAL_DEV  *SerialDev,
  IN UINT32      Offset
  )
{
  UINT8       Data;
  EFI_STATUS  Status;

  if (SerialDev->PciDeviceInfo == NULL) {
    return IoRead8 ((UINTN)SerialDev->BaseAddress + Offset * SerialDev->RegisterStride);
  } else {
    if (SerialDev->MmioAccess) {
      Status = SerialDev->PciDeviceInfo->PciIo->Mem.Read (
                                                      SerialDev->PciDeviceInfo->PciIo,
                                                      EfiPciIoWidthUint8,
                                                      EFI_PCI_IO_PASS_THROUGH_BAR,
                                                      SerialDev->BaseAddress + Offset * SerialDev->RegisterStride,
                                                      1,
                                                      &Data
                                                      );
    } else {
      Status = SerialDev->PciDeviceInfo->PciIo->Io.Read (
                                                     SerialDev->PciDeviceInfo->PciIo,
                                                     EfiPciIoWidthUint8,
                                                     EFI_PCI_IO_PASS_THROUGH_BAR,
                                                     SerialDev->BaseAddress + Offset * SerialDev->RegisterStride,
                                                     1,
                                                     &Data
                                                     );
    }

    ASSERT_EFI_ERROR (Status);
    return Data;
  }
}

/**
  Write serial port.

  @param  SerialDev     Pointer to serial device
  @param  Offset        Offset in register group
  @param  Data          data which is to be written to some serial port register
**/
VOID
SerialWriteRegister (
  IN SERIAL_DEV  *SerialDev,
  IN UINT32      Offset,
  IN UINT8       Data
  )
{
  EFI_STATUS  Status;

  if (SerialDev->PciDeviceInfo == NULL) {
    IoWrite8 ((UINTN)SerialDev->BaseAddress + Offset * SerialDev->RegisterStride, Data);
  } else {
    if (SerialDev->MmioAccess) {
      Status = SerialDev->PciDeviceInfo->PciIo->Mem.Write (
                                                      SerialDev->PciDeviceInfo->PciIo,
                                                      EfiPciIoWidthUint8,
                                                      EFI_PCI_IO_PASS_THROUGH_BAR,
                                                      SerialDev->BaseAddress + Offset * SerialDev->RegisterStride,
                                                      1,
                                                      &Data
                                                      );
    } else {
      Status = SerialDev->PciDeviceInfo->PciIo->Io.Write (
                                                     SerialDev->PciDeviceInfo->PciIo,
                                                     EfiPciIoWidthUint8,
                                                     EFI_PCI_IO_PASS_THROUGH_BAR,
                                                     SerialDev->BaseAddress + Offset * SerialDev->RegisterStride,
                                                     1,
                                                     &Data
                                                     );
    }

    ASSERT_EFI_ERROR (Status);
  }
}

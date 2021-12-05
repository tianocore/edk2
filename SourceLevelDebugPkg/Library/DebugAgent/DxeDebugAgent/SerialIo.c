/** @file
  Install Serial IO Protocol that layers on top of a Debug Communication Library instance.

  Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeDebugAgentLib.h"

//
// Serial I/O Protocol Interface definitions.
//

/**
  Reset serial device.

  @param[in] This           Pointer to EFI_SERIAL_IO_PROTOCOL.

  @retval EFI_SUCCESS       Reset successfully.

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  );

/**
  Set new attributes to a serial device.

  @param[in]  This                Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in]  BaudRate            The baudrate of the serial device.
  @param[in]  ReceiveFifoDepth    The depth of receive FIFO buffer.
  @param[in]  Timeout             The request timeout for a single char.
  @param[in]  Parity              The type of parity used in serial device.
  @param[in]  DataBits            Number of databits used in serial device.
  @param[in]  StopBits            Number of stopbits used in serial device.

  @retval EFI_SUCCESS             The new attributes were set.
  @retval EFI_INVALID_PARAMETER   One or more attributes have an unsupported value.
  @retval EFI_DEVICE_ERROR        The serial device is not functioning correctly (no return).

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
  );

/**
  Set Control Bits.

  @param[in] This            Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in] Control         Control bits that can be settable.

  @retval EFI_SUCCESS        New Control bits were set successfully.
  @retval EFI_UNSUPPORTED    The Control bits wanted to set are not supported.

**/
EFI_STATUS
EFIAPI
SerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  );

/**
  Get ControlBits.

  @param[in]  This         Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[out] Control      Control signals of the serial device.

  @retval EFI_SUCCESS  Get Control signals successfully.

**/
EFI_STATUS
EFIAPI
SerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  );

/**
  Write the specified number of bytes to serial device.

  @param[in]      This       Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in, out] BufferSize On input the size of Buffer, on output the amount of
                             data actually written.
  @param[in]      Buffer     The buffer of data to write.

  @retval EFI_SUCCESS        The data were written successfully.
  @retval EFI_DEVICE_ERROR   The device reported an error.
  @retval EFI_TIMEOUT        The write operation was stopped due to timeout.

**/
EFI_STATUS
EFIAPI
SerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  );

/**
  Read the specified number of bytes from serial device.

  @param[in] This            Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in, out] BufferSize On input the size of Buffer, on output the amount of
                             data returned in buffer.
  @param[out] Buffer         The buffer to return the data into.

  @retval EFI_SUCCESS        The data were read successfully.
  @retval EFI_DEVICE_ERROR   The device reported an error.
  @retval EFI_TIMEOUT        The read operation was stopped due to timeout.

**/
EFI_STATUS
EFIAPI
SerialRead (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  );

//
// Serial Driver Defaults
//
#define SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH  1
#define SERIAL_PORT_DEFAULT_TIMEOUT             1000000
#define SERIAL_PORT_DEFAULT_CONTROL_MASK        0
#define SERIAL_PORT_LOOPBACK_BUFFER_FULL        BIT8

//
// EFI_SERIAL_IO_MODE instance
//
EFI_SERIAL_IO_MODE  mSerialIoMode = {
  SERIAL_PORT_DEFAULT_CONTROL_MASK,
  SERIAL_PORT_DEFAULT_TIMEOUT,
  0,  // default BaudRate
  SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH,
  0,  // default DataBits
  0,  // default Parity
  0   // default StopBits
};

//
// EFI_SERIAL_IO_PROTOCOL instance
//
EFI_SERIAL_IO_PROTOCOL  mSerialIo = {
  SERIAL_IO_INTERFACE_REVISION,
  SerialReset,
  SerialSetAttributes,
  SerialSetControl,
  SerialGetControl,
  SerialWrite,
  SerialRead,
  &mSerialIoMode
};

//
// Serial IO Device Path definition
//
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  UART_DEVICE_PATH            UartDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevicePath;
} SERIAL_IO_DEVICE_PATH;

//
// Serial IO Device Patch instance
//
SERIAL_IO_DEVICE_PATH  mSerialIoDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_DEBUG_AGENT_GUID,
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,
    0,  // default BaudRate
    0,  // default DataBits
    0,  // default Parity
    0,  // default StopBits
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

#define DEBGU_SERIAL_IO_FIFO_DEPTH  10
//
//  Data buffer for Terminal input character and Debug Symbols.
//  The depth is DEBGU_SERIAL_IO_FIFO_DEPTH.
//  Fields:
//      First   UINT8: The index of the first data in array Data[].
//      Last    UINT8: The index, which you can put a new data into array Data[].
//      Surplus UINT8: Identify how many data you can put into array Data[].
//      Data[]  UINT8: An array, which used to store data.
//
typedef struct {
  UINT8    First;
  UINT8    Last;
  UINT8    Surplus;
  UINT8    Data[DEBGU_SERIAL_IO_FIFO_DEPTH];
} DEBUG_SERIAL_FIFO;

//
// Global Variables
//
EFI_HANDLE         mSerialIoHandle        = NULL;
UINTN              mLoopbackBuffer        = 0;
DEBUG_SERIAL_FIFO  mSerialFifoForTerminal = {
  0, 0, DEBGU_SERIAL_IO_FIFO_DEPTH, { 0 }
};
DEBUG_SERIAL_FIFO  mSerialFifoForDebug = {
  0, 0, DEBGU_SERIAL_IO_FIFO_DEPTH, { 0 }
};

/**
  Detect whether specific FIFO is empty or not.

  @param[in]  Fifo    A pointer to the Data Structure DEBUG_SERIAL_FIFO.

  @return whether specific FIFO is empty or not.

**/
BOOLEAN
IsDebugTermianlFifoEmpty (
  IN DEBUG_SERIAL_FIFO  *Fifo
  )
{
  if (Fifo->Surplus == DEBGU_SERIAL_IO_FIFO_DEPTH) {
    return TRUE;
  }

  return FALSE;
}

/**
  Detect whether specific FIFO is full or not.

  @param[in] Fifo    A pointer to the Data Structure DEBUG_SERIAL_FIFO.

  @return whether specific FIFO is full or not.

**/
BOOLEAN
IsDebugTerminalFifoFull (
  IN DEBUG_SERIAL_FIFO  *Fifo
  )

{
  if (Fifo->Surplus == 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Add data to specific FIFO.

  @param[in] Fifo               A pointer to the Data Structure DEBUG_SERIAL_FIFO.
  @param[in] Data               The data added to FIFO.

  @retval EFI_SUCCESS           Add data to specific FIFO successfully.
  @retval EFI_OUT_OF_RESOURCE   Failed to add data because FIFO is already full.

**/
EFI_STATUS
DebugTerminalFifoAdd (
  IN DEBUG_SERIAL_FIFO  *Fifo,
  IN UINT8              Data
  )

{
  //
  // if FIFO full can not add data
  //
  if (IsDebugTerminalFifoFull (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // FIFO is not full can add data
  //
  Fifo->Data[Fifo->Last] = Data;
  Fifo->Surplus--;
  Fifo->Last++;
  if (Fifo->Last == DEBGU_SERIAL_IO_FIFO_DEPTH) {
    Fifo->Last = 0;
  }

  return EFI_SUCCESS;
}

/**
  Remove data from specific FIFO.

  @param[in]  Fifo              A pointer to the Data Structure DEBUG_SERIAL_FIFO.
  @param[out] Data              The data removed from FIFO.

  @retval EFI_SUCCESS           Remove data from specific FIFO successfully.
  @retval EFI_OUT_OF_RESOURCE   Failed to remove data because FIFO is empty.

**/
EFI_STATUS
DebugTerminalFifoRemove (
  IN  DEBUG_SERIAL_FIFO  *Fifo,
  OUT UINT8              *Data
  )
{
  //
  // if FIFO is empty, no data can remove
  //
  if (IsDebugTermianlFifoEmpty (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // FIFO is not empty, can remove data
  //
  *Data = Fifo->Data[Fifo->First];
  Fifo->Surplus++;
  Fifo->First++;
  if (Fifo->First == DEBGU_SERIAL_IO_FIFO_DEPTH) {
    Fifo->First = 0;
  }

  return EFI_SUCCESS;
}

/**
  Install EFI Serial IO protocol based on Debug Communication Library.

**/
VOID
InstallSerialIo (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSerialIoHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mSerialIoDevicePath,
                  &gEfiSerialIoProtocolGuid,
                  &mSerialIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Debug Agent: Failed to install EFI Serial IO Protocol on Debug Port!\n"));
  }
}

/**
  Reset serial device.

  @param[in] This           Pointer to EFI_SERIAL_IO_PROTOCOL.

  @retval EFI_SUCCESS       Reset successfully.

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  )
{
  mSerialIoMode.ControlMask = SERIAL_PORT_DEFAULT_CONTROL_MASK;
  mLoopbackBuffer           = 0;
  //
  // Not reset serial device hardware indeed.
  //
  return EFI_SUCCESS;
}

/**
  Set new attributes to a serial device.

  @param[in]  This                Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in]  BaudRate            The baudrate of the serial device.
  @param[in]  ReceiveFifoDepth    The depth of receive FIFO buffer.
  @param[in]  Timeout             The request timeout for a single char.
  @param[in]  Parity              The type of parity used in serial device.
  @param[in]  DataBits            Number of databits used in serial device.
  @param[in]  StopBits            Number of stopbits used in serial device.

  @retval EFI_SUCCESS             The new attributes were set.
  @retval EFI_INVALID_PARAMETER   One or more attributes have an unsupported value.
  @retval EFI_DEVICE_ERROR        The serial device is not functioning correctly (no return).

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
  //
  // The Debug Communication Library CAN NOT change communications parameters (if it has)
  // actually. Because it also has no any idea on what parameters are based on, we cannot
  // check the input parameters (like BaudRate, Parity, DataBits and StopBits).
  //

  //
  // Update the Timeout value in the mode structure based on the request.
  // The Debug Communication Library can not support a timeout on writes, but the timeout on
  // reads can be provided by this module.
  //
  if (Timeout == 0) {
    mSerialIoMode.Timeout = SERIAL_PORT_DEFAULT_TIMEOUT;
  } else {
    mSerialIoMode.Timeout = Timeout;
  }

  //
  // Update the ReceiveFifoDepth value in the mode structure based on the request.
  // This module assumes that the Debug Communication Library uses a FIFO depth of
  // SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH.  The Debug Communication Library may actually be
  // using a larger FIFO, but there is no way to tell.
  //
  if ((ReceiveFifoDepth == 0) || (ReceiveFifoDepth >= SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH)) {
    mSerialIoMode.ReceiveFifoDepth = SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Set Control Bits.

  @param[in] This            Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in] Control         Control bits that can be settable.

  @retval EFI_SUCCESS        New Control bits were set successfully.
  @retval EFI_UNSUPPORTED    The Control bits wanted to set are not supported.

**/
EFI_STATUS
EFIAPI
SerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  )
{
  //
  // The only control bit supported by this module is software loopback.
  // If any other bit is set, then return an error
  //
  if ((Control & (~EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE)) != 0) {
    return EFI_UNSUPPORTED;
  }

  mSerialIoMode.ControlMask = Control;
  return EFI_SUCCESS;
}

/**
  Get ControlBits.

  @param[in]  This         Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[out] Control      Control signals of the serial device.

  @retval EFI_SUCCESS  Get Control signals successfully.

**/
EFI_STATUS
EFIAPI
SerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  )
{
  DEBUG_PORT_HANDLE  Handle;
  BOOLEAN            DebugTimerInterruptState;
  EFI_TPL            Tpl;

  //
  // Raise TPL to prevent recursion from EFI timer interrupts
  //
  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Save and disable Debug Timer interrupt to avoid it to access Debug Port
  //
  DebugTimerInterruptState = SaveAndSetDebugTimerInterrupt (FALSE);
  Handle                   = GetDebugPortHandle ();

  //
  // Always assume the output buffer is empty and the Debug Communication Library can process
  // more write requests.
  //
  *Control = mSerialIoMode.ControlMask | EFI_SERIAL_OUTPUT_BUFFER_EMPTY;

  //
  // Check to see if the Terminal FIFO is empty and
  // check to see if the input buffer in the Debug Communication Library is empty
  //
  if (!IsDebugTermianlFifoEmpty (&mSerialFifoForTerminal) || DebugPortPollBuffer (Handle)) {
    *Control &= ~EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  //
  // Restore Debug Timer interrupt
  //
  SaveAndSetDebugTimerInterrupt (DebugTimerInterruptState);

  //
  // Restore to original TPL
  //
  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

/**
  Write the specified number of bytes to serial device.

  @param[in]      This       Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in, out] BufferSize On input the size of Buffer, on output the amount of
                             data actually written.
  @param[in]      Buffer     The buffer of data to write.

  @retval EFI_SUCCESS        The data were written successfully.
  @retval EFI_DEVICE_ERROR   The device reported an error.
  @retval EFI_TIMEOUT        The write operation was stopped due to timeout.

**/
EFI_STATUS
EFIAPI
SerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  )
{
  DEBUG_PORT_HANDLE  Handle;
  BOOLEAN            DebugTimerInterruptState;
  EFI_TPL            Tpl;

  //
  // Raise TPL to prevent recursion from EFI timer interrupts
  //
  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Save and disable Debug Timer interrupt to avoid it to access Debug Port
  //
  DebugTimerInterruptState = SaveAndSetDebugTimerInterrupt (FALSE);
  Handle                   = GetDebugPortHandle ();

  if ((mSerialIoMode.ControlMask & EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE) != 0) {
    if (*BufferSize == 0) {
      return EFI_SUCCESS;
    }

    if ((mLoopbackBuffer & SERIAL_PORT_LOOPBACK_BUFFER_FULL) != 0) {
      *BufferSize = 0;
      return EFI_TIMEOUT;
    }

    mLoopbackBuffer = SERIAL_PORT_LOOPBACK_BUFFER_FULL | *(UINT8 *)Buffer;
    *BufferSize     = 1;
  } else {
    *BufferSize = DebugPortWriteBuffer (Handle, Buffer, *BufferSize);
  }

  //
  // Restore Debug Timer interrupt
  //
  SaveAndSetDebugTimerInterrupt (DebugTimerInterruptState);

  //
  // Restore to original TPL
  //
  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

/**
  Read the specified number of bytes from serial device.

  @param[in] This            Pointer to EFI_SERIAL_IO_PROTOCOL.
  @param[in, out] BufferSize On input the size of Buffer, on output the amount of
                             data returned in buffer.
  @param[out] Buffer         The buffer to return the data into.

  @retval EFI_SUCCESS        The data were read successfully.
  @retval EFI_DEVICE_ERROR   The device reported an error.
  @retval EFI_TIMEOUT        The read operation was stopped due to timeout.

**/
EFI_STATUS
EFIAPI
SerialRead (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_STATUS           Status;
  UINTN                Index;
  UINT8                *Uint8Buffer;
  BOOLEAN              DebugTimerInterruptState;
  EFI_TPL              Tpl;
  DEBUG_PORT_HANDLE    Handle;
  DEBUG_PACKET_HEADER  DebugHeader;
  UINT8                *Data8;

  //
  // Raise TPL to prevent recursion from EFI timer interrupts
  //
  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Save and disable Debug Timer interrupt to avoid it to access Debug Port
  //
  DebugTimerInterruptState = SaveAndSetDebugTimerInterrupt (FALSE);
  Handle                   = GetDebugPortHandle ();

  Data8       = (UINT8 *)&DebugHeader;
  Uint8Buffer = (UINT8 *)Buffer;
  if ((mSerialIoMode.ControlMask & EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE) != 0) {
    if ((mLoopbackBuffer & SERIAL_PORT_LOOPBACK_BUFFER_FULL) == 0) {
      return EFI_TIMEOUT;
    }

    *Uint8Buffer    = (UINT8)(mLoopbackBuffer & 0xff);
    mLoopbackBuffer = 0;
    *BufferSize     = 1;
  } else {
    for (Index = 0; Index < *BufferSize; Index++) {
      //
      // Read input character from terminal FIFO firstly
      //
      Status = DebugTerminalFifoRemove (&mSerialFifoForTerminal, Data8);
      if (Status == EFI_SUCCESS) {
        *Uint8Buffer = *Data8;
        Uint8Buffer++;
        continue;
      }

      //
      // Read the input character from Debug Port
      //
      if (!DebugPortPollBuffer (Handle)) {
        break;
      }

      DebugAgentReadBuffer (Handle, Data8, 1, 0);

      if (*Data8 == DEBUG_STARTING_SYMBOL_ATTACH) {
        //
        // Add the debug symbol into Debug FIFO
        //
        DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Terminal Timer attach symbol received %x", *Data8);
        DebugTerminalFifoAdd (&mSerialFifoForDebug, *Data8);
      } else if (*Data8 == DEBUG_STARTING_SYMBOL_NORMAL) {
        Status = ReadRemainingBreakPacket (Handle, &DebugHeader);
        if (Status == EFI_SUCCESS) {
          DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Terminal Timer break symbol received %x", DebugHeader.Command);
          DebugTerminalFifoAdd (&mSerialFifoForDebug, DebugHeader.Command);
        }

        if (Status == EFI_TIMEOUT) {
          continue;
        }
      } else {
        *Uint8Buffer = *Data8;
        Uint8Buffer++;
      }
    }

    *BufferSize = (UINTN)Uint8Buffer - (UINTN)Buffer;
  }

  //
  // Restore Debug Timer interrupt
  //
  SaveAndSetDebugTimerInterrupt (DebugTimerInterruptState);

  //
  // Restore to original TPL
  //
  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

/**
  Read the Attach/Break-in symbols from the debug port.

  @param[in]  Handle         Pointer to Debug Port handle.
  @param[out] BreakSymbol    Returned break symbol.

  @retval EFI_SUCCESS        Read the symbol in BreakSymbol.
  @retval EFI_NOT_FOUND      No read the break symbol.

**/
EFI_STATUS
DebugReadBreakFromDebugPort (
  IN  DEBUG_PORT_HANDLE  Handle,
  OUT UINT8              *BreakSymbol
  )
{
  EFI_STATUS           Status;
  DEBUG_PACKET_HEADER  DebugHeader;
  UINT8                *Data8;

  *BreakSymbol = 0;
  //
  // If Debug Port buffer has data, read it till it was break symbol or Debug Port buffer empty.
  //
  Data8 = (UINT8 *)&DebugHeader;
  while (TRUE) {
    //
    // If start symbol is not received
    //
    if (!DebugPortPollBuffer (Handle)) {
      //
      // If no data in Debug Port, exit
      //
      break;
    }

    //
    // Try to read the start symbol
    //
    DebugAgentReadBuffer (Handle, Data8, 1, 0);
    if (*Data8 == DEBUG_STARTING_SYMBOL_ATTACH) {
      DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Debug Timer attach symbol received %x", *Data8);
      *BreakSymbol = *Data8;
      return EFI_SUCCESS;
    }

    if (*Data8 == DEBUG_STARTING_SYMBOL_NORMAL) {
      Status = ReadRemainingBreakPacket (Handle, &DebugHeader);
      if (Status == EFI_SUCCESS) {
        DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Debug Timer break symbol received %x", DebugHeader.Command);
        *BreakSymbol = DebugHeader.Command;
        return EFI_SUCCESS;
      }

      if (Status == EFI_TIMEOUT) {
        break;
      }
    } else {
      //
      // Add to Terminal FIFO
      //
      DebugTerminalFifoAdd (&mSerialFifoForTerminal, *Data8);
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Read the Attach/Break-in symbols.

  @param[in]  Handle         Pointer to Debug Port handle.
  @param[out] BreakSymbol    Returned break symbol.

  @retval EFI_SUCCESS        Read the symbol in BreakSymbol.
  @retval EFI_NOT_FOUND      No read the break symbol.

**/
EFI_STATUS
DebugReadBreakSymbol (
  IN  DEBUG_PORT_HANDLE  Handle,
  OUT UINT8              *BreakSymbol
  )
{
  EFI_STATUS  Status;
  UINT8       Data8;

  //
  // Read break symbol from debug FIFO firstly
  //
  Status = DebugTerminalFifoRemove (&mSerialFifoForDebug, &Data8);
  if (Status == EFI_SUCCESS) {
    *BreakSymbol = Data8;
    return EFI_SUCCESS;
  } else {
    //
    // Read Break symbol from debug port
    //
    return DebugReadBreakFromDebugPort (Handle, BreakSymbol);
  }
}

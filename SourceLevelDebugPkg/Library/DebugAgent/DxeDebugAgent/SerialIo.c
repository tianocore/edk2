/** @file
  Install Serial IO Protocol that layers on top of a Debug Communication Library instance.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeDebugAgentLib.h"

//
// Serial I/O Protocol Interface defintions.
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
  0,  // BaudRate
  SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH,
  0,  // DataBits
  0,  // Parity
  0   // StopBits
};

//
// EFI_SERIAL_IO_PROTOCOL instance
//
EFI_SERIAL_IO_PROTOCOL mSerialIo = {
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
  VENDOR_DEVICE_PATH        VendorDevicePath;
  UART_DEVICE_PATH          UartDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} SERIAL_IO_DEVICE_PATH;

//
// Serial IO Device Patch instance
//
SERIAL_IO_DEVICE_PATH mSerialIoDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
      (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
    },
    EFI_DEBUG_AGENT_GUID,
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      (UINT8) (sizeof (UART_DEVICE_PATH)),
      (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8)
    },
    0,
    0,  // BaudRate
    0,  // DataBits
    0,  // Parity
    0,  // StopBits
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

#define DEBGU_SERIAL_IO_FIFO_DEPTH      10
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
  UINT8  First;
  UINT8  Last;
  UINT8  Surplus;
  UINT8  Data[DEBGU_SERIAL_IO_FIFO_DEPTH];
} DEBUG_SERIAL_FIFO;

//
// Global Varibles
//
EFI_HANDLE                   mSerialIoHandle        = NULL;
UINTN                        mLoopbackBuffer        = 0;
DEBUG_SERIAL_FIFO            mSerialFifoForTerminal = {0, 0, DEBGU_SERIAL_IO_FIFO_DEPTH, { 0 }};
DEBUG_SERIAL_FIFO            mSerialFifoForDebug    = {0, 0, DEBGU_SERIAL_IO_FIFO_DEPTH, { 0 }};

/**
  Detect whether specific FIFO is empty or not.
 
  @param[in]  Fifo    A pointer to the Data Structure DEBUG_SERIAL_FIFO.

  @return whether specific FIFO is empty or not.

**/
BOOLEAN
IsDebugTermianlFifoEmpty (
  IN DEBUG_SERIAL_FIFO    *Fifo
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
  IN DEBUG_SERIAL_FIFO    *Fifo
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
  IN DEBUG_SERIAL_FIFO   *Fifo,
  IN UINT8               Data
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
  IN  DEBUG_SERIAL_FIFO   *Fifo,
  OUT UINT8               *Data
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
  Notification function on EFI PCD protocol to install EFI Serial IO protocol based
  on Debug Communication Library. 

  @param[in]  Event    The event of notify protocol.
  @param[in]  Context  Notify event context.

**/
VOID
EFIAPI
InstallSerialIoNotification (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  EFI_STATUS  Status;

  //
  // Get Debug Port parameters from PCDs
  //
  mSerialIoDevicePath.UartDevicePath.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  mSerialIoDevicePath.UartDevicePath.DataBits = PcdGet8 (PcdUartDefaultDataBits);
  mSerialIoDevicePath.UartDevicePath.Parity   = PcdGet8 (PcdUartDefaultParity);
  mSerialIoDevicePath.UartDevicePath.StopBits = PcdGet8 (PcdUartDefaultStopBits);

  mSerialIoMode.BaudRate = mSerialIoDevicePath.UartDevicePath.BaudRate;
  mSerialIoMode.DataBits = mSerialIoDevicePath.UartDevicePath.DataBits;
  mSerialIoMode.Parity   = mSerialIoDevicePath.UartDevicePath.Parity;
  mSerialIoMode.StopBits = mSerialIoDevicePath.UartDevicePath.StopBits;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSerialIoHandle,
                  &gEfiDevicePathProtocolGuid, &mSerialIoDevicePath,
                  &gEfiSerialIoProtocolGuid,   &mSerialIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Debug Agent: Failed to install EFI Serial IO Protocol on Debug Port!\n"));
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
  mLoopbackBuffer = 0;
  //
  // Not reset serial devcie hardware indeed.
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
  // The Debug Communication Library does not support changing communications parameters, so unless
  // the request is to use the default value or the value the Debug Communication Library is already
  // using, then return EFI_INVALID_PARAMETER.
  //
  if (BaudRate != 0 && BaudRate != PcdGet64 (PcdUartDefaultBaudRate)) {
    return EFI_INVALID_PARAMETER;
  }
  if (Parity != DefaultParity && Parity != PcdGet8 (PcdUartDefaultParity)) {
    return EFI_INVALID_PARAMETER;
  }
  if (DataBits != 0 && DataBits != PcdGet8 (PcdUartDefaultDataBits)) {
    return EFI_INVALID_PARAMETER;
  }
  if (StopBits != DefaultStopBits && StopBits != PcdGet8 (PcdUartDefaultStopBits)) {
    return EFI_INVALID_PARAMETER;
  }
  
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
  if (ReceiveFifoDepth == 0 || ReceiveFifoDepth >= SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH) {
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
  DEBUG_PORT_HANDLE                Handle;

  Handle = GetDebugPortHandle ();
  
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
  DEBUG_PORT_HANDLE                Handle;

  Handle = GetDebugPortHandle ();
  
  if ((mSerialIoMode.ControlMask & EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE) != 0)  {
    if (*BufferSize == 0) {
      return EFI_SUCCESS;
    }
    if ((mLoopbackBuffer & SERIAL_PORT_LOOPBACK_BUFFER_FULL) != 0) {
      *BufferSize = 0;
      return EFI_TIMEOUT;
    }
    mLoopbackBuffer = SERIAL_PORT_LOOPBACK_BUFFER_FULL | *(UINT8 *)Buffer;
    *BufferSize = 1;
  } else {
    *BufferSize = DebugPortWriteBuffer (Handle, Buffer, *BufferSize);
  }
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
  EFI_STATUS                  Status;
  UINTN                       Index;
  UINT8                       *Uint8Buffer;
  BOOLEAN                     OldInterruptState;
  DEBUG_PORT_HANDLE           Handle;
  UINT8                       Data;

  Handle = GetDebugPortHandle ();

  //
  // Save and disable Debug Timer interrupt to avoid it to access Debug Port
  //
  OldInterruptState = SaveAndSetDebugTimerInterrupt (FALSE);
  
  Uint8Buffer = (UINT8 *)Buffer;
  if ((mSerialIoMode.ControlMask & EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE) != 0)  {
    if ((mLoopbackBuffer & SERIAL_PORT_LOOPBACK_BUFFER_FULL) == 0) {
      return EFI_TIMEOUT;
    }
    *Uint8Buffer = (UINT8)(mLoopbackBuffer & 0xff);
    mLoopbackBuffer = 0;
    *BufferSize = 1;
  } else {
    for (Index = 0; Index < *BufferSize; Index++) {
      //
      // Read input character from terminal FIFO firstly
      //
      Status = DebugTerminalFifoRemove (&mSerialFifoForTerminal, &Data);
      if (Status == EFI_SUCCESS) {
        *Uint8Buffer = Data;
        Uint8Buffer ++;
        continue;
      }
      //
      // Read the input character from Debug Port 
      //
      if (!DebugPortPollBuffer (Handle)) {
        break;
      }
      DebugPortReadBuffer (Handle, &Data, 1, 0);

      if (Data== DEBUG_STARTING_SYMBOL_ATTACH ||
          Data == DEBUG_STARTING_SYMBOL_BREAK) {
        //
        // Add the debug symbol into Debug FIFO
        //
        DebugTerminalFifoAdd (&mSerialFifoForDebug, Data);
      } else {
        *Uint8Buffer = Data;
        Uint8Buffer ++;
      }
    }
    *BufferSize = (UINTN)Uint8Buffer - (UINTN)Buffer;
  }

  //
  // Restore Debug Timer interrupt
  //  
  SaveAndSetDebugTimerInterrupt (OldInterruptState);
  
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
DebugReadBreakSymbol (
  IN  DEBUG_PORT_HANDLE      Handle,
  OUT UINT8                  *BreakSymbol
  )
{
  EFI_STATUS               Status;
  UINT8                    Data;

  Status = DebugTerminalFifoRemove (&mSerialFifoForDebug, &Data);
  if (Status != EFI_SUCCESS) {
    if (!DebugPortPollBuffer (Handle)) {
      //
      // No data in Debug Port buffer.
      //
      return EFI_NOT_FOUND;
    } else {
      //
      // Read one character from Debug Port.
      //
      DebugPortReadBuffer (Handle, &Data, 1, 0);
      if ((Data != DEBUG_STARTING_SYMBOL_ATTACH) && (Data != DEBUG_STARTING_SYMBOL_BREAK)) {
        //
        // If the data is not Break symbol, add it into Terminal FIFO
        //
        DebugTerminalFifoAdd (&mSerialFifoForTerminal, Data);
        return EFI_NOT_FOUND;
      }
    }
  }
  
  *BreakSymbol = Data;
  return EFI_SUCCESS;
}

/** @file
  Debug Port Library implementation based on serial port.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/DebugCommunicationLib.h>
#include <Library/SerialPortLib.h>
#include <Library/DebugLib.h>

/**
  Initialize the debug port.

  This function will initialize debug port to get it ready for data transmission. If
  certain Debug Communication Library instance has to save some private data in the
  stack, this function must work on the mode that doesn't return to the caller, then
  the caller needs to wrap up all rest of logic after DebugPortInitialize() into one
  function and pass it into DebugPortInitialize(). DebugPortInitialize() is
  responsible to invoke the passing-in function at the end of DebugPortInitialize().

  If the parameter Function is not NULL, Debug Communication Library instance will
  invoke it by passing in the Context to be the first parameter. Debug Communication
  Library instance could create one debug port handle to be the second parameter
  passing into the Function. Debug Communication Library instance also could pass
  NULL to be the second parameter if it doesn't create the debug port handle.

  If the parameter Function is NULL, and Context is not NULL. At this time, Context
  is the debug port handle created by the previous Debug Communication Library
  instance.
  a) If the instance can understand and continue use the private data of the previous
     instance, it could return the same handle as passed in (as Context parameter).
  b) If the instance does not understand, or does not want to continue use the
     private data of the previous instance, it could ignore the input Context parameter
     and create the new handle to be returned.

  If Function() is NULL and Context is NULL, Debug Communication Library could create a
  new handle and return it. NULL is also a valid handle to be returned.

  @param[in] Context      Context needed by callback function; it was optional.
  @param[in] Function     Continue function called by Debug Communication library;
                          it was optional.

  @return  The debug port handle created by Debug Communication Library if Function
           is not NULL.

**/
DEBUG_PORT_HANDLE
EFIAPI
DebugPortInitialize (
  IN VOID                 *Context,
  IN DEBUG_PORT_CONTINUE  Function
  )
{
  RETURN_STATUS  Status;

  Status = SerialPortInitialize ();
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Debug Serial Port: Initialization failed!\n"));
  }

  if (Function != NULL) {
    Function (Context, NULL);
  }

  return NULL;
}

/**
  Read data from debug device and save the datas in buffer.

  Reads NumberOfBytes data bytes from a debug device into the buffer
  specified by Buffer. The number of bytes actually read is returned.
  If the return value is less than NumberOfBytes, then the rest operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Handle           Debug port handle.
  @param  Buffer           Pointer to the data buffer to store the data read from the debug device.
  @param  NumberOfBytes    Number of bytes which will be read.
  @param  Timeout          Timeout value for reading from debug device. It unit is Microsecond.

  @retval 0                Read data failed, no data is to be read.
  @retval >0               Actual number of bytes read from debug device.

**/
UINTN
EFIAPI
DebugPortReadBuffer (
  IN DEBUG_PORT_HANDLE  Handle,
  IN UINT8              *Buffer,
  IN UINTN              NumberOfBytes,
  IN UINTN              Timeout
  )
{
  if ((NumberOfBytes != 1) || (Buffer == NULL) || (Timeout != 0)) {
    return 0;
  }

  return SerialPortRead (Buffer, 1);
}

/**
  Write data from buffer to debug device.

  Writes NumberOfBytes data bytes from Buffer to the debug device.
  The number of bytes actually written to the debug device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Handle           Debug port handle.
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the debug device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the debug device.
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
DebugPortWriteBuffer (
  IN DEBUG_PORT_HANDLE  Handle,
  IN UINT8              *Buffer,
  IN UINTN              NumberOfBytes
  )
{
  return SerialPortWrite (Buffer, NumberOfBytes);
}

/**
  Polls a debug device to see if there is any data waiting to be read.

  Polls a debug device to see if there is any data waiting to be read.
  If there is data waiting to be read from the debug device, then TRUE is returned.
  If there is no data waiting to be read from the debug device, then FALSE is returned.

  @param  Handle           Debug port handle.

  @retval TRUE             Data is waiting to be read from the debug device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
DebugPortPollBuffer (
  IN DEBUG_PORT_HANDLE  Handle
  )
{
  return SerialPortPoll ();
}

/** @file
  Debug Port Library implementation based on serial port.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/DebugCommunicationLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#pragma pack(1)
//
// The internal data structure of DEBUG_PORT_HANDLE, which stores some
// important datum which are used across various phases.
//
typedef struct _SERIAL_DEBUG_PORT_HANDLE{
  //
  // Timter settings
  //
  UINT64       TimerFrequency;
  UINT64       TimerCycle;
  BOOLEAN      TimerCountDown;
} SERIAL_DEBUG_PORT_HANDLE;
#pragma pack()

//
// The global variable which can be used after memory is ready.
//
SERIAL_DEBUG_PORT_HANDLE     mSerialDebugPortHandle;

/**
  Check if the timer is timeout.
  
  @param[in] SerialDebugPortHandle  Pointer to Serial Debug port handle
  @param[in] Timer                  The start timer from the begin.
  @param[in] TimeoutTicker          Ticker number need time out.

  @return TRUE  Timer time out occurs.
  @retval FALSE Timer does not time out.

**/
BOOLEAN
IsTimerTimeout (
  IN SERIAL_DEBUG_PORT_HANDLE   *SerialDebugPortHandle,
  IN UINT64                     Timer,
  IN UINT64                     TimeoutTicker
  )
{
  UINT64  CurrentTimer;
  UINT64  Delta;

  CurrentTimer = GetPerformanceCounter ();

  if (SerialDebugPortHandle->TimerCountDown) {
    //
    // The timer counter counts down.  Check for roll over condition.
    //
    if (CurrentTimer < Timer) {
      Delta = Timer - CurrentTimer;
    } else {
      //
      // Handle one roll-over. 
      //
      Delta = SerialDebugPortHandle->TimerCycle - (CurrentTimer - Timer);
    }
  } else {
    //
    // The timer counter counts up.  Check for roll over condition.
    //
    if (CurrentTimer > Timer) {
      Delta = CurrentTimer - Timer;
    } else {
      //
      // Handle one roll-over. 
      //
      Delta = SerialDebugPortHandle->TimerCycle - (Timer - CurrentTimer);
    }
  }
 
  return (BOOLEAN) (Delta >= TimeoutTicker);
}

/**
  Initialize the debug port.

  This function will initialize debug port to get it ready for data transmition. If
  certain Debug Communication Library instance has to save some private data in the
  stack, this function must work on the mode that doesn't return to the caller, then
  the caller needs to wrap up all rest of logic after DebugPortInitialize() into one
  function and pass it into DebugPortInitialize(). DebugPortInitialize() is
  responsible to invoke the passing-in funciton at the end of DebugPortInitialize().

  If the paramter Function is not NULL, Debug Communication Libary instance will
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
     and create the new hanlde to be returned.

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
  RETURN_STATUS              Status;
  SERIAL_DEBUG_PORT_HANDLE   Handle;
  SERIAL_DEBUG_PORT_HANDLE   *SerialDebugPortHandle;
  UINT64                     TimerStartValue;
  UINT64                     TimerEndValue;

  //
  // Validate the PCD PcdDebugPortHandleBufferSize value 
  //
  ASSERT (PcdGet16 (PcdDebugPortHandleBufferSize) == sizeof (SERIAL_DEBUG_PORT_HANDLE));

  if (Context != NULL && Function == NULL) {
    SerialDebugPortHandle = (SERIAL_DEBUG_PORT_HANDLE *)Context;
  } else {
    ZeroMem (&Handle, sizeof (SERIAL_DEBUG_PORT_HANDLE));
    SerialDebugPortHandle = &Handle;
  }
  SerialDebugPortHandle->TimerFrequency = GetPerformanceCounterProperties (
                                            &TimerStartValue,
                                            &TimerEndValue
                                            );
  DEBUG ((EFI_D_INFO, "Serial Debug Port: TimerFrequency  = 0x%lx\n", SerialDebugPortHandle->TimerFrequency)); 
  DEBUG ((EFI_D_INFO, "Serial Debug Port: TimerStartValue = 0x%lx\n", TimerStartValue)); 
  DEBUG ((EFI_D_INFO, "Serial Debug Port: TimerEndValue   = 0x%lx\n", TimerEndValue)); 

  if (TimerEndValue < TimerStartValue) {
    SerialDebugPortHandle->TimerCountDown = TRUE;
    SerialDebugPortHandle->TimerCycle     = TimerStartValue - TimerEndValue;
  } else {
    SerialDebugPortHandle->TimerCountDown = FALSE;
    SerialDebugPortHandle->TimerCycle     = TimerEndValue - TimerStartValue;
  }  

  if (Function == NULL && Context != NULL) {
    return (DEBUG_PORT_HANDLE *) Context;
  }

  Status = SerialPortInitialize ();
  if (RETURN_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "Debug Serial Port: Initialization failed!\n")); 
  }

  if (Function != NULL) {
    Function (Context, SerialDebugPortHandle);
  } else {
    CopyMem(&mSerialDebugPortHandle, SerialDebugPortHandle, sizeof (SERIAL_DEBUG_PORT_HANDLE));
  }

  return (DEBUG_PORT_HANDLE)(UINTN)&mSerialDebugPortHandle;
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
  IN DEBUG_PORT_HANDLE     Handle,
  IN UINT8                 *Buffer,
  IN UINTN                 NumberOfBytes,
  IN UINTN                 Timeout
  )
{
  SERIAL_DEBUG_PORT_HANDLE *SerialDebugPortHandle;
  UINTN                    Index;
  UINT64                   Begin;
  UINT64                   TimeoutTicker;
  UINT64                   TimerRound;
  
  //
  // If Handle is NULL, it means memory is ready for use.
  // Use global variable to store handle value.
  //
  if (Handle == NULL) {
    SerialDebugPortHandle = &mSerialDebugPortHandle;
  } else {
    SerialDebugPortHandle = (SERIAL_DEBUG_PORT_HANDLE *)Handle;
  }

  Begin         = 0;
  TimeoutTicker = 0;  
  TimerRound    = 0;
  if (Timeout != 0) {
    Begin = GetPerformanceCounter ();
    TimeoutTicker = DivU64x32 (
                      MultU64x64 (
                        SerialDebugPortHandle->TimerFrequency,
                        Timeout
                        ),
                      1000000u
                      );
    TimerRound = DivU64x64Remainder (
                   TimeoutTicker,
                   DivU64x32 (SerialDebugPortHandle->TimerCycle, 2),
                   &TimeoutTicker
                   );
  }
  Index = 0;
  while (Index < NumberOfBytes) {
    if (SerialPortPoll () || Timeout == 0) {
      SerialPortRead (Buffer + Index, 1);
      Index ++; 
      continue;
    }
    if (TimerRound == 0) {
      if (IsTimerTimeout (SerialDebugPortHandle, Begin, TimeoutTicker)) {
        //
        // If time out occurs.
        //
        return 0;
      }
    } else {
      if (IsTimerTimeout (SerialDebugPortHandle, Begin, DivU64x32 (SerialDebugPortHandle->TimerCycle, 2))) {
        TimerRound --;
      }
    }
  }

  return Index;
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
  IN DEBUG_PORT_HANDLE     Handle,
  IN UINT8                 *Buffer,
  IN UINTN                 NumberOfBytes
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
  IN DEBUG_PORT_HANDLE     Handle
  )
{
  return SerialPortPoll ();
}


/** @file
  Xen console SerialPortLib instance

  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>

#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/XenHypercallLib.h>

#include <IndustryStandard/Xen/io/console.h>
#include <IndustryStandard/Xen/hvm/params.h>
#include <IndustryStandard/Xen/event_channel.h>

//
// We can't use DebugLib due to a constructor dependency cycle between DebugLib
// and ourselves.
//
#define ASSERT(Expression)      \
  do {                          \
    if (!(Expression)) {        \
      CpuDeadLoop ();           \
    }                           \
  } while (FALSE)

//
// The code below expects these global variables to be mutable, even in the case
// that we have been incorporated into SEC or PEIM phase modules (which is
// allowed by our INF description). While this is a dangerous assumption to make
// in general, it is actually fine for the Xen domU (guest) environment that
// this module is intended for, as UEFI always executes from DRAM in that case.
//
STATIC evtchn_send_t              mXenConsoleEventChain;
STATIC struct xencons_interface   *mXenConsoleInterface;

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
  if (! XenHypercallIsAvailable ()) {
    return RETURN_DEVICE_ERROR;
  }

  if (!mXenConsoleInterface) {
    mXenConsoleEventChain.port = (UINT32)XenHypercallHvmGetParam (HVM_PARAM_CONSOLE_EVTCHN);
    mXenConsoleInterface = (struct xencons_interface *)(UINTN)
      (XenHypercallHvmGetParam (HVM_PARAM_CONSOLE_PFN) << EFI_PAGE_SHIFT);

    //
    // No point in ASSERT'ing here as we won't be seeing the output
    //
  }
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
                           If this value is less than NumberOfBytes, then the write operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  XENCONS_RING_IDX  Consumer, Producer;
  UINTN             Sent;

  ASSERT (Buffer != NULL);

  if (NumberOfBytes == 0) {
    return 0;
  }

  if (!mXenConsoleInterface) {
    return 0;
  }

  Sent = 0;
  do {
    Consumer = mXenConsoleInterface->out_cons;
    Producer = mXenConsoleInterface->out_prod;

    MemoryFence ();

    while (Sent < NumberOfBytes && ((Producer - Consumer) < sizeof (mXenConsoleInterface->out)))
      mXenConsoleInterface->out[MASK_XENCONS_IDX(Producer++, mXenConsoleInterface->out)] = Buffer[Sent++];

    MemoryFence ();

    mXenConsoleInterface->out_prod = Producer;

    XenHypercallEventChannelOp (EVTCHNOP_send, &mXenConsoleEventChain);

  } while (Sent < NumberOfBytes);

  return Sent;
}

/**
  Read data from serial device and save the datas in buffer.

  Reads NumberOfBytes data bytes from a serial device into the buffer
  specified by Buffer. The number of bytes actually read is returned.
  If Buffer is NULL, then ASSERT().
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to store the data read from the serial device.
  @param  NumberOfBytes    Number of bytes which will be read.

  @retval 0                Read data failed, no data is to be read.
  @retval >0               Actual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  XENCONS_RING_IDX  Consumer, Producer;
  UINTN             Received;

  ASSERT (Buffer != NULL);

  if (NumberOfBytes == 0) {
    return 0;
  }

  if (!mXenConsoleInterface) {
    return 0;
  }

  Consumer = mXenConsoleInterface->in_cons;
  Producer = mXenConsoleInterface->in_prod;

  MemoryFence ();

  Received = 0;
  while (Received < NumberOfBytes && Consumer < Producer)
     Buffer[Received++] = mXenConsoleInterface->in[MASK_XENCONS_IDX(Consumer++, mXenConsoleInterface->in)];

  MemoryFence ();

  mXenConsoleInterface->in_cons = Consumer;

  XenHypercallEventChannelOp (EVTCHNOP_send, &mXenConsoleEventChain);

  return Received;
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  @retval TRUE             Data is waiting to be read from the serial device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  return mXenConsoleInterface &&
    mXenConsoleInterface->in_cons != mXenConsoleInterface->in_prod;
}

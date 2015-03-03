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
// The code below expects these global variables to be mutable, even in the case
// that we have been incorporated into SEC or PEIM phase modules (which is
// allowed by our INF description). While this is a dangerous assumption to make
// in general, it is actually fine for the Xen domU (guest) environment that
// this module is intended for, as UEFI always executes from DRAM in that case.
//
STATIC evtchn_send_t              mXenConsoleEventChain;
STATIC struct xencons_interface   *mXenConsoleInterface;

RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  if (! XenHypercallIsAvailable ()) {
    return RETURN_NOT_FOUND;
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
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes written to serial device.

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

  if (!mXenConsoleInterface) {
    return 0;
  }

  Consumer = mXenConsoleInterface->out_cons;
  Producer = mXenConsoleInterface->out_prod;

  MemoryFence ();

  Sent = 0;
  while (Sent < NumberOfBytes && ((Producer - Consumer) < sizeof (mXenConsoleInterface->out)))
    mXenConsoleInterface->out[MASK_XENCONS_IDX(Producer++, mXenConsoleInterface->out)] = Buffer[Sent++];

  MemoryFence ();

  mXenConsoleInterface->out_prod = Producer;

  if (Sent > 0) {
    XenHypercallEventChannelOp (EVTCHNOP_send, &mXenConsoleEventChain);
  }

  return Sent;
}

/**
  Read data from serial device and save the data in buffer.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Size of Buffer[].

  @retval 0                Read data failed.
  @retval !0               Actual number of bytes read from serial device.

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
  Check to see if any data is available to be read from the debug device.

  @retval TRUE       At least one byte of data is available to be read
  @retval FALSE      No data is available to be read

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

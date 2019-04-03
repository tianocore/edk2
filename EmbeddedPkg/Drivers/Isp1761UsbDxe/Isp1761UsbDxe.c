/** @file

  Copyright (c) 2013-2015, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>

#include <IndustryStandard/Usb.h>

#include <Protocol/UsbDevice.h>

#include "Isp1761UsbDxe.h"

/*
  Driver for using the NXP ISP1761 as a USB Peripheral controller.
  Doesn't use USB OTG - just sets it in Pure Peripheral mode.

  The ISP1582 datasheet has a little more info on the Peripheral controller
  registers than the ISP1761 datasheet

  We don't do string descriptors. They're optional.
  We currently assume the device has one configuration, one interface, one IN
  endpoint, and one OUT endpoint (plus the default control endpoint).

  In fact, this driver is the minimum required to implement fastboot.
*/

// TODO Make sure the controller isn't sending empty packets when it shouldn't
// (check behaviour in cases when Buffer Length isn't explcitly set)

// ISP1582 Datasheet:
// "Data transfers preceding the status stage must first be fully
// completed before the STATUS bit can be set."
// This variable stores whether some control data has been pended in the EP0TX
// Tx buffer, so that when an EP0TX interrupt is received we can set the STATUS
// bit to go to the Status stage of the control transfer.
STATIC BOOLEAN mControlTxPending = FALSE;

STATIC USB_DEVICE_DESCRIPTOR    *mDeviceDescriptor;

// The config descriptor, interface descriptor, and endpoint descriptors in a
// buffer (in that order)
STATIC VOID                     *mDescriptors;
// Convenience pointers to those descriptors inside the buffer:
STATIC USB_INTERFACE_DESCRIPTOR *mInterfaceDescriptor;
STATIC USB_CONFIG_DESCRIPTOR    *mConfigDescriptor;
STATIC USB_ENDPOINT_DESCRIPTOR  *mEndpointDescriptors;

STATIC USB_DEVICE_RX_CALLBACK   mDataReceivedCallback;
STATIC USB_DEVICE_TX_CALLBACK   mDataSentCallback;

// The time between interrupt polls, in units of 100 nanoseconds
// 10 Microseconds
#define ISP1761_INTERRUPT_POLL_PERIOD 10000

STATIC
VOID
SelectEndpoint (
  IN UINT8 Endpoint
  )
{
  // The DMA Endpoint Index must not point to the same as the
  // Endpoint Index Register.
  WRITE_REG32 (ISP1761_DMA_ENDPOINT_INDEX, ((Endpoint + 2) % ISP1761_NUM_ENDPOINTS));
  WRITE_REG32 (ISP1761_ENDPOINT_INDEX, Endpoint);
}

// Enable going to the Data stage of a control transfer
STATIC
VOID
DataStageEnable (
  IN UINT8 Endpoint
  )
{
  SelectEndpoint (Endpoint);
  WRITE_REG32 (ISP1761_CTRL_FUNCTION, ISP1761_CTRL_FUNCTION_DSEN);
}

// Go to the Status stage of a successful control transfer
STATIC
VOID
StatusAcknowledge (
  IN UINT8 Endpoint
)
{
  SelectEndpoint (Endpoint);
  WRITE_REG32 (ISP1761_CTRL_FUNCTION, ISP1761_CTRL_FUNCTION_STATUS);
}

// Read the FIFO for the endpoint indexed by Endpoint, into the buffer pointed
// at by Buffer, whose size is *Size bytes.
//
// If *Size is less than the number of bytes in the FIFO, return EFI_BUFFER_TOO_SMALL
//
// Update *Size with the number of bytes of data in the FIFO.
STATIC
EFI_STATUS
ReadEndpointBuffer (
  IN      UINT8   Endpoint,
  IN OUT  UINTN  *Size,
  IN OUT  VOID   *Buffer
  )
{
  UINT16  NumBytesAvailable;
  UINT32  Val32;
  UINTN   Index;
  UINTN   NumBytesRead;

  SelectEndpoint (Endpoint);

  NumBytesAvailable = READ_REG16 (ISP1761_BUFFER_LENGTH);

  if (NumBytesAvailable > *Size) {
    *Size = NumBytesAvailable;
    return EFI_BUFFER_TOO_SMALL;
  }
  *Size = NumBytesAvailable;

  /* -- NB! --
    The datasheet says the Data Port is 16 bits but it actually appears to
    be 32 bits.
   */

  // Read 32-bit chunks
  for (Index = 0; Index < NumBytesAvailable / 4; Index++) {
    ((UINT32 *) Buffer)[Index] = READ_REG32 (ISP1761_DATA_PORT);
  }

  // Read remaining bytes

  // Round NumBytesAvailable down to nearest power of 4
  NumBytesRead = NumBytesAvailable & (~0x3);
  if (NumBytesRead != NumBytesAvailable) {
    Val32 = READ_REG32 (ISP1761_DATA_PORT);
    // Copy each required byte of 32-bit word into buffer
    for (Index = 0; Index < NumBytesAvailable % 4; Index++) {
      ((UINT8 *) Buffer)[NumBytesRead + Index] = Val32 >> (Index * 8);
    }
  }
  return EFI_SUCCESS;
}

/*
  Write an endpoint buffer. Parameters:
  Endpoint        Endpoint index (see Endpoint Index Register in datasheet)
  MaxPacketSize   The MaxPacketSize this endpoint is configured for
  Size            The size of the Buffer
  Buffer          The data

  Assumes MaxPacketSize is a multiple of 4.
  (It seems that all valid values for MaxPacketSize _are_ multiples of 4)
*/
STATIC
EFI_STATUS
WriteEndpointBuffer (
  IN       UINT8   Endpoint,
  IN       UINTN   MaxPacketSize,
  IN       UINTN   Size,
  IN CONST VOID   *Buffer
  )
{
  UINTN    Index;
  UINT32  *DwordBuffer;

  DwordBuffer = (UINT32 *) Buffer;
  SelectEndpoint (Endpoint);

  /* -- NB! --
    The datasheet says the Data Port is 16 bits but it actually appears to
    be 32 bits.
   */

  // Write packets of size MaxPacketSize
  while (Size > MaxPacketSize) {
    for (Index = 0; Index < MaxPacketSize / 4; Index++) {
      WRITE_REG32 (ISP1761_DATA_PORT, DwordBuffer[Index]);
    }
    Size -= MaxPacketSize;
    DwordBuffer += (MaxPacketSize / sizeof (UINT32));
  }

  // Write remaining data

  if (Size > 0) {
    WRITE_REG32 (ISP1761_BUFFER_LENGTH, Size);

    while (Size > 4) {
      WRITE_REG32 (ISP1761_DATA_PORT, DwordBuffer[0]);
      Size -= 4;
      DwordBuffer++;
    }

    if (Size > 0) {
      WRITE_REG32 (ISP1761_DATA_PORT, DwordBuffer[0]);
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
HandleGetDescriptor (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  EFI_STATUS  Status;
  UINT8       DescriptorType;
  UINTN       ResponseSize;
  VOID       *ResponseData;

  ResponseSize = 0;
  ResponseData = NULL;
  Status = EFI_SUCCESS;

  // Pretty confused if bmRequestType is anything but this:
  ASSERT (Request->RequestType == USB_DEV_GET_DESCRIPTOR_REQ_TYPE);

  // Choose the response
  DescriptorType = Request->Value >> 8;
  switch (DescriptorType) {
  case USB_DESC_TYPE_DEVICE:
    DEBUG ((EFI_D_INFO, "USB: Got a request for device descriptor\n"));
    ResponseSize = sizeof (USB_DEVICE_DESCRIPTOR);
    ResponseData = mDeviceDescriptor;
    break;
  case USB_DESC_TYPE_CONFIG:
    DEBUG ((EFI_D_INFO, "USB: Got a request for config descriptor\n"));
    ResponseSize = mConfigDescriptor->TotalLength;
    ResponseData = mDescriptors;
    break;
  case USB_DESC_TYPE_STRING:
    DEBUG ((EFI_D_INFO, "USB: Got a request for String descriptor %d\n", Request->Value & 0xFF));
    break;
  default:
    DEBUG ((EFI_D_INFO, "USB: Didn't understand request for descriptor 0x%04x\n", Request->Value));
    Status = EFI_NOT_FOUND;
    break;
  }

  // Send the response
  if (ResponseData) {
    ASSERT (ResponseSize != 0);

    if (Request->Length < ResponseSize) {
      // Truncate response
      ResponseSize = Request->Length;
    } else if (Request->Length > ResponseSize) {
      DEBUG ((EFI_D_INFO, "USB: Info: ResponseSize < wLength\n"));
    }

    DataStageEnable (ISP1761_EP0TX);
    Status = WriteEndpointBuffer (
              ISP1761_EP0TX,
              MAX_PACKET_SIZE_CONTROL,
              ResponseSize,
              ResponseData
              );
    if (!EFI_ERROR (Status)) {
      // Setting this value should cause us to go to the Status stage on the
      // next EP0TX interrupt
      mControlTxPending = TRUE;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
HandleSetAddress (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  // Pretty confused if bmRequestType is anything but this:
  ASSERT (Request->RequestType == USB_DEV_SET_ADDRESS_REQ_TYPE);
  // USB Spec: "The USB device does not change its device address until after
  // the Status stage of this request is completed successfully."
  // ISP1582 datasheet: "The new device address is activated when the
  // device receives an acknowledgment from the host for the empty packet
  // token". (StatusAcknowledge causes an empty packet to be sent).
  // So, we write the Address register _before_ acking the SET_ADDRESS.
  DEBUG ((EFI_D_INFO, "USB: Setting address to %d\n", Request->Value));
  WRITE_REG32 (ISP1761_ADDRESS, Request->Value | ISP1761_ADDRESS_DEVEN);
  StatusAcknowledge (ISP1761_EP0TX);

  return EFI_SUCCESS;
}

// Move the device to the Configured state.
// (This code only supports one configuration for a device, so the configuration
//  index is ignored)
STATIC
EFI_STATUS
HandleSetConfiguration (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  USB_ENDPOINT_DESCRIPTOR  *EPDesc;
  UINTN                     Index;
  UINT8                     EndpointIndex;

  ASSERT (Request->RequestType == USB_DEV_SET_CONFIGURATION_REQ_TYPE);
  DEBUG ((EFI_D_INFO, "USB: Setting configuration.\n"));

  // Configure endpoints
  for (Index = 0; Index < mInterfaceDescriptor->NumEndpoints; Index++) {
    EPDesc = &mEndpointDescriptors[Index];

    // To simplify for now, assume endpoints aren't "sparse", and are in order.
    ASSERT ((EPDesc->EndpointAddress & 0xF) == ((Index / 2) + 1));

    // Convert from USB endpoint index to ISP1761 endpoint Index
    // USB:     Endpoint number is bits [3:0], IN/OUT is bit [7]
    // ISP1761: Endpoint number is bits [4:1], IN/OUT is bit [0]
    EndpointIndex = ((EPDesc->EndpointAddress & 0xF) << 1) |
                    ((EPDesc->EndpointAddress & BIT7) >> 7);
    SelectEndpoint (EndpointIndex);
    // Set endpoint type (Bulk/Isochronous/Interrupt)
    WRITE_REG32 (ISP1761_ENDPOINT_MAX_PACKET_SIZE, EPDesc->MaxPacketSize);
    // Hardware foible (bug?): Although the datasheet seems to suggest it should
    // automatically be set to MaxPacketSize, the Buffer Length register appears
    // to be reset to 0, which causes an empty packet to be sent in response to
    // the first IN token of the session. The NOEMPKT field of the Endpoint Type
    // register sounds like it might fix this problem, but it doesn't
    // (it's "applicable only in the DMA mode").
    WRITE_REG32 (ISP1761_BUFFER_LENGTH, EPDesc->MaxPacketSize);
    WRITE_REG32 (ISP1761_ENDPOINT_TYPE, (EPDesc->Attributes & 0x3) |
                                        ISP1761_ENDPOINT_TYPE_ENABLE);
  }

  StatusAcknowledge (ISP1761_EP0TX);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
HandleDeviceRequest (
  IN USB_DEVICE_REQUEST  *Request
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  switch (Request->Request) {
  case USB_DEV_GET_DESCRIPTOR:
    Status = HandleGetDescriptor (Request);
    break;
  case USB_DEV_SET_ADDRESS:
    Status = HandleSetAddress (Request);
    break;
  case USB_DEV_SET_CONFIGURATION:
    Status = HandleSetConfiguration (Request);
    break;
  default:
    DEBUG ((EFI_D_ERROR,
      "Didn't understand RequestType 0x%x Request 0x%x\n",
      Request->RequestType, Request->Request));
      Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}

// Instead of actually registering interrupt handlers, we poll the controller's
//  interrupt source register in this function.
STATIC
VOID
CheckInterrupts (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
  UINT32      DcInterrupts;
  UINTN       NumBytes;
  UINTN       MoreBytes;
  UINT8       Packet[512];
  VOID       *DataPacket;
  UINT32      HandledInterrupts;
  UINT32      UnhandledInterrupts;
  EFI_STATUS  Status;

  // Set bits in HandledInterrupts to mark the interrupt source handled.
  HandledInterrupts = 0;

  WRITE_REG32 (ISP1761_DEVICE_UNLOCK, ISP1761_DEVICE_UNLOCK_MAGIC);

  DcInterrupts = READ_REG32 (ISP1761_DC_INTERRUPT);
  if (DcInterrupts & ISP1761_DC_INTERRUPT_SUSP) {
    DEBUG ((EFI_D_INFO, "USB: Suspend\n"));
    HandledInterrupts |= ISP1761_DC_INTERRUPT_SUSP;
  }
  if (DcInterrupts & ISP1761_DC_INTERRUPT_RESUME) {
    DEBUG ((EFI_D_INFO, "USB: Resume\n"));
    HandledInterrupts |= ISP1761_DC_INTERRUPT_RESUME;
  }
  if (DcInterrupts & ISP1761_DC_INTERRUPT_EP0SETUP) {
    NumBytes = 512;
    ReadEndpointBuffer (0x20, &NumBytes, &Packet);
    ASSERT (NumBytes == 8);
    HandleDeviceRequest ((USB_DEVICE_REQUEST *) Packet);
    HandledInterrupts |= ISP1761_DC_INTERRUPT_EP0SETUP;
  }
  if (DcInterrupts & ISP1761_DC_INTERRUPT_EP0RX) {
    HandledInterrupts |= ISP1761_DC_INTERRUPT_EP0RX;
  }
  if (DcInterrupts & ISP1761_DC_INTERRUPT_EP0TX) {
    if (mControlTxPending) {
      // We previously put some data in the Control Endpoint's IN (Tx) FIFO.
      // We assume that that data has now been sent in response to the IN token
      // that triggered this interrupt. We can therefore go to the Status stage
      // of the control transfer.
      StatusAcknowledge (ISP1761_EP0TX);
      mControlTxPending = FALSE;
    }
    HandledInterrupts |= ISP1761_DC_INTERRUPT_EP0TX;
  }
  if (DcInterrupts & ISP1761_DC_INTERRUPT_EP1RX) {
    NumBytes = 512;
    DataPacket = AllocatePool (NumBytes);
    Status = ReadEndpointBuffer (ISP1761_EP1RX, &NumBytes, DataPacket);
    if (EFI_ERROR (Status) || NumBytes == 0) {
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Couldn't read EP1RX data: %r\n", Status));
      }
      FreePool (DataPacket);
    } else {
      // Signal this event again so we poll again ASAP
      gBS->SignalEvent (Event);
      mDataReceivedCallback (NumBytes, DataPacket);
    }
    HandledInterrupts |= ISP1761_DC_INTERRUPT_EP1RX;
  }
  if (DcInterrupts & ISP1761_DC_INTERRUPT_EP1TX) {
    mDataSentCallback (1);
    HandledInterrupts |= ISP1761_DC_INTERRUPT_EP1TX;
  }
  if (DcInterrupts & (ISP1761_DC_INTERRUPT_SOF | ISP1761_DC_INTERRUPT_PSOF)) {
    // Don't care about SOFs or pseudo-SOFs
    HandledInterrupts |= (ISP1761_DC_INTERRUPT_SOF | ISP1761_DC_INTERRUPT_PSOF);
  }
  if (ISP1761_DC_INTERRUPT_BRESET) {
    HandledInterrupts |= ISP1761_DC_INTERRUPT_BRESET;
  }
  if (ISP1761_DC_INTERRUPT_HS_STAT) {
    HandledInterrupts |= ISP1761_DC_INTERRUPT_HS_STAT;
  }
  if (ISP1761_DC_INTERRUPT_VBUS) {
    HandledInterrupts |= ISP1761_DC_INTERRUPT_VBUS;
  }

  UnhandledInterrupts = DcInterrupts & (~HandledInterrupts) & ISP1761_DC_INTERRUPT_MASK;
  if (UnhandledInterrupts) {
    DEBUG ((EFI_D_ERROR, "USB: Unhandled DC Interrupts: 0x%08x\n",
      UnhandledInterrupts));
  }

  // Check if we received any more data while we were handling the interrupt.
  SelectEndpoint (ISP1761_EP1RX);
  MoreBytes = READ_REG16 (ISP1761_BUFFER_LENGTH);
  if (MoreBytes) {
    HandledInterrupts &= ~ISP1761_DC_INTERRUPT_EP1RX;
  }

  WRITE_REG32 (ISP1761_DC_INTERRUPT, HandledInterrupts);
}

EFI_STATUS
Isp1761PeriphSend (
  IN        UINT8  EndpointIndex,
  IN        UINTN  Size,
  IN  CONST VOID  *Buffer
  )
{
  return WriteEndpointBuffer (
          (EndpointIndex << 1) | 0x1, //Convert to ISP1761 endpoint index, Tx
          MAX_PACKET_SIZE_BULK,
          Size,
          Buffer
          );
}

EFI_STATUS
EFIAPI
Isp1761PeriphStart (
  IN USB_DEVICE_DESCRIPTOR   *DeviceDescriptor,
  IN VOID                   **Descriptors,
  IN USB_DEVICE_RX_CALLBACK   RxCallback,
  IN USB_DEVICE_TX_CALLBACK   TxCallback
  )
{
  UINT16                    OtgStatus;
  UINT8                    *Ptr;
  EFI_STATUS                Status;
  EFI_EVENT                 TimerEvent;

  ASSERT (DeviceDescriptor != NULL);
  ASSERT (Descriptors[0] != NULL);
  ASSERT (RxCallback != NULL);
  ASSERT (TxCallback != NULL);

  WRITE_REG32 (ISP1761_DEVICE_UNLOCK, ISP1761_DEVICE_UNLOCK_MAGIC);

  WRITE_REG32 (ISP1761_SW_RESET_REG, ISP1761_SW_RESET_ALL);
  while (READ_REG32 (ISP1761_SW_RESET_REG) & ISP1761_SW_RESET_ALL) {
    //busy wait
  }
  WRITE_REG32 (ISP1761_MODE, ISP1761_MODE_SFRESET);
  while (READ_REG32 (ISP1761_MODE) & ISP1761_MODE_SFRESET) {
    //busy wait
  }
  DEBUG ((EFI_D_INFO, "USB: Software reset done\n"));

  WRITE_REG32 (ISP1761_DC_INTERRUPT_ENABLE, 0x03FFFFFF);
  WRITE_REG32 (ISP1761_OTG_INTERRUPT_ENABLE_RISE, 0x07FF);

  WRITE_REG8 (ISP1761_ADDRESS, ISP1761_ADDRESS_DEVEN);
  WRITE_REG8 (ISP1761_MODE, ISP1761_MODE_WKUPCS | ISP1761_MODE_CLKAON);

  // Use port 1 as peripheral controller (magic - disagrees with datasheet)
  WRITE_REG32 (ISP1761_OTG_CTRL_SET, 0xffff0000);
  WRITE_REG32 (ISP1761_OTG_CTRL_SET, 0x000014d1);

  OtgStatus = READ_REG16 (ISP1761_OTG_STATUS);
  if ((OtgStatus & ISP1761_OTG_STATUS_B_SESS_END) != 0) {
    DEBUG ((EFI_D_ERROR, "USB: Vbus not powered.\n"));
  }
  if ((OtgStatus & ISP1761_OTG_STATUS_A_B_SESS_VLD) == 0) {
    DEBUG ((EFI_D_ERROR, "USB: Session not valid.\n"));
  }

  // Configure Control endpoints
  SelectEndpoint (0x20);
  WRITE_REG32 (ISP1761_ENDPOINT_MAX_PACKET_SIZE, MAX_PACKET_SIZE_CONTROL);
  WRITE_REG32 (ISP1761_ENDPOINT_TYPE, ISP1761_ENDPOINT_TYPE_ENABLE);
  SelectEndpoint (0x0);
  WRITE_REG32 (ISP1761_ENDPOINT_MAX_PACKET_SIZE, MAX_PACKET_SIZE_CONTROL);
  WRITE_REG32 (ISP1761_ENDPOINT_TYPE, ISP1761_ENDPOINT_TYPE_ENABLE);
  SelectEndpoint (0x1);
  WRITE_REG32 (ISP1761_ENDPOINT_MAX_PACKET_SIZE, MAX_PACKET_SIZE_CONTROL);
  WRITE_REG32 (ISP1761_ENDPOINT_TYPE, ISP1761_ENDPOINT_TYPE_ENABLE);

  // Interrupt on all ACK and NAK
  WRITE_REG32 (ISP1761_INTERRUPT_CONFIG, ISP1761_INTERRUPT_CONFIG_ACK_ONLY);

  mDeviceDescriptor = DeviceDescriptor;
  mDescriptors = Descriptors[0];

  // Right now we just support one configuration
  ASSERT (mDeviceDescriptor->NumConfigurations == 1);
  // ... and one interface
  mConfigDescriptor = (USB_CONFIG_DESCRIPTOR *)mDescriptors;
  ASSERT (mConfigDescriptor->NumInterfaces == 1);

  Ptr = ((UINT8 *) mDescriptors) + sizeof (USB_CONFIG_DESCRIPTOR);
  mInterfaceDescriptor = (USB_INTERFACE_DESCRIPTOR *) Ptr;
  Ptr += sizeof (USB_INTERFACE_DESCRIPTOR);

  mEndpointDescriptors = (USB_ENDPOINT_DESCRIPTOR *) Ptr;

  mDataReceivedCallback = RxCallback;
  mDataSentCallback = TxCallback;

  // Register a timer event so CheckInterupts gets called periodically
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckInterrupts,
                  NULL,
                  &TimerEvent
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  TimerEvent,
                  TimerPeriodic,
                  ISP1761_INTERRUPT_POLL_PERIOD
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

USB_DEVICE_PROTOCOL mUsbDevice = {
  Isp1761PeriphStart,
  Isp1761PeriphSend
};


EFI_STATUS
EFIAPI
Isp1761PeriphEntryPoint (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  UINT32      DeviceId;
  EFI_HANDLE  Handle;

  DeviceId = READ_REG32 (ISP1761_DEVICE_ID);

  if (DeviceId != ISP1761_DEVICE_ID_VAL) {
    DEBUG ((EFI_D_ERROR,
      "ERROR: Read incorrect device ID for ISP1761: 0x%08x, expected 0x%08x\n",
      DeviceId , ISP1761_DEVICE_ID_VAL
      ));
    return EFI_DEVICE_ERROR;
  }

  Handle = NULL;
  return gBS->InstallProtocolInterface (
    &Handle,
    &gUsbDeviceProtocolGuid,
    EFI_NATIVE_INTERFACE,
    &mUsbDevice
    );
}

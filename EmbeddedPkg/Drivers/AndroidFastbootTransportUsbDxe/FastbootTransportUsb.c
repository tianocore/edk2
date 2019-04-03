/** @file

  Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
 * Implementation of the FASTBOOT_TRANSPORT_PROTOCOL using the USB_DEVICE_PROTOCOL
 */

#include <Protocol/UsbDevice.h>
#include <Protocol/AndroidFastbootTransport.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

STATIC USB_DEVICE_PROTOCOL *mUsbDevice;

// Configuration attributes:
// bit 7 reserved and must be 1, bit 6 means self-powered.
#define CONFIG_DESC_ATTRIBUTES      (BIT7 | BIT6)

#define MAX_PACKET_SIZE_BULK        512

STATIC USB_DEVICE_PROTOCOL  *mUsbDevice;
STATIC EFI_EVENT             mReceiveEvent = NULL;
STATIC LIST_ENTRY            mPacketList;

// List type for queued received packets
typedef struct _FASTBOOT_USB_PACKET_LIST {
  LIST_ENTRY  Link;
  VOID       *Buffer;
  UINTN       BufferSize;
} FASTBOOT_USB_PACKET_LIST;


/*
  No string descriptors - all string descriptor members are set to 0
*/

STATIC USB_DEVICE_DESCRIPTOR mDeviceDescriptor = {
  sizeof (USB_DEVICE_DESCRIPTOR),                  //Length
  USB_DESC_TYPE_DEVICE,                            //DescriptorType
  0x0200,                                          //BcdUSB
  0xFF,                                            //DeviceClass
  0,                                               //DeviceSubClass
  0,                                               //DeviceProtocol
  64,                                              //MaxPacketSize0
  FixedPcdGet32 (PcdAndroidFastbootUsbVendorId),   //IdVendor
  FixedPcdGet32 (PcdAndroidFastbootUsbProductId),  //IdProduct
  0,                                               //BcdDevice
  0,                                               //StrManufacturer
  0,                                               //StrProduct
  0,                                               //StrSerialNumber
  1                                                //NumConfigurations
};

/*
  We have one configuration, one interface, and two endpoints (one IN, one OUT)
*/

// Lazy (compile-time) way to concatenate descriptors to pass to the USB device
// protocol

#pragma pack(1)
typedef struct {
  USB_CONFIG_DESCRIPTOR     ConfigDescriptor;
  USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor1;
  USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor2;
} GET_CONFIG_DESCRIPTOR_RESPONSE;
#pragma pack()

STATIC GET_CONFIG_DESCRIPTOR_RESPONSE mGetConfigDescriptorResponse = {
  { // USB_CONFIG_DESCRIPTOR
    sizeof (USB_CONFIG_DESCRIPTOR),                   //Length;
    USB_DESC_TYPE_CONFIG,                             //DescriptorType;
    sizeof (GET_CONFIG_DESCRIPTOR_RESPONSE),          //TotalLength;
    1,                                                //NumInterfaces;
    1,                                                //ConfigurationValue;
    0,                                                //Configuration;
    CONFIG_DESC_ATTRIBUTES,                           //Attributes;
    0                                                 //MaxPower;
  },
  { // USB_INTERFACE_DESCRIPTOR
    sizeof (USB_INTERFACE_DESCRIPTOR), //Length;
    USB_DESC_TYPE_INTERFACE, //DescriptorType;
    0,                                                //InterfaceNumber;
    0,                                                //AlternateSetting;
    2,                                                //NumEndpoints;
    0xFF,                                             //InterfaceClass;
    // Vendor specific interface subclass and protocol codes.
    // I found these values in the Fastboot code
    // (in match_fastboot_with_serial in fastboot.c).
    0x42,                                             //InterfaceSubClass;
    0x03,                                             //InterfaceProtocol;
    0                                                 //Interface;
  },
  { // USB_ENDPOINT_DESCRIPTOR (In Endpoint)
    sizeof (USB_ENDPOINT_DESCRIPTOR),                 //Length;
    USB_DESC_TYPE_ENDPOINT,                           //DescriptorType;
    1 | BIT7,                                         //EndpointAddress;
    0x2,                                              //Attributes;
    MAX_PACKET_SIZE_BULK,                             //MaxPacketSize;
    16                                                //Interval;
  },
  { // STATIC USB_ENDPOINT_DESCRIPTOR (Out Endpoint)
    sizeof (USB_ENDPOINT_DESCRIPTOR),                 //Length;
    USB_DESC_TYPE_ENDPOINT,                           //DescriptorType;
    1,                                                //EndpointAddress;
    0x2,                                              //Attributes;
    MAX_PACKET_SIZE_BULK,                             //MaxPacketSize;
    16                                                //Interval;
  }
};

STATIC
VOID
DataReceived (
  IN UINTN    Size,
  IN VOID    *Buffer
  )
{
  FASTBOOT_USB_PACKET_LIST *NewEntry;

  NewEntry = AllocatePool (sizeof (*NewEntry));
  ASSERT (NewEntry != NULL);

  NewEntry->Buffer = Buffer;
  NewEntry->BufferSize = Size;

  InsertTailList (&mPacketList, &NewEntry->Link);

  if (mReceiveEvent) {
    gBS->SignalEvent (mReceiveEvent);
  }
}

STATIC
VOID
DataSent (
  IN UINT8 EndpointIndex
  )
{
  // Don't care.
}

/*
  Set up the transport system for use by Fastboot.
  e.g. For USB this probably means making the device enumerable.
*/
EFI_STATUS
FastbootTransportUsbStart (
  EFI_EVENT ReceiveEvent
  )
{
  GET_CONFIG_DESCRIPTOR_RESPONSE  *Responses;

  mReceiveEvent = ReceiveEvent;

  mGetConfigDescriptorResponse.ConfigDescriptor.TotalLength = sizeof (GET_CONFIG_DESCRIPTOR_RESPONSE);
  Responses = &mGetConfigDescriptorResponse;

  InitializeListHead (&mPacketList);

  return mUsbDevice->Start (&mDeviceDescriptor, (VOID **) &Responses, DataReceived, DataSent);
}

/*
  Function to be called when all Fastboot transactions are finished, to
  de-initialise the transport system.
  e.g. A USB OTG system might want to get out of peripheral mode so it can be
       a USB host.
*/
EFI_STATUS
FastbootTransportUsbStop (
  VOID
  )
{
  // not yet implemented in USB
  return EFI_SUCCESS;
}

/*
  Send data. This function can be used both for command responses like "OKAY"
  and for the data phase (the protocol doesn't describe any situation when the
   latter might be necessary, but does allow it)
 */
EFI_STATUS
FastbootTransportUsbSend (
  IN        UINTN      BufferSize,
  IN  CONST VOID      *Buffer,
  IN        EFI_EVENT *FatalErrorEvent
  )
{
  // Current USB protocol is blocking, so ignore FatalErrorEvent
  return mUsbDevice->Send(1, BufferSize, Buffer);
}

/*
  When the event has been Signalled to say data is available from the host,
  this function is used to get data. In order to handle the case where several
  packets are received before ReceiveEvent's notify function is called, packets
  received are queued, and each call to this function returns the next packet in
  the queue. It should therefore be called in a loop, the exit condition being a
  return of EFI_NOT_READY.

  Parameters:
    Buffer      - The buffer in which to place data
    BufferSize  - The size of Buffer in bytes

  Return EFI_NOT_READY if there is no data available
*/
EFI_STATUS
FastbootTransportUsbReceive (
  OUT UINTN  *BufferSize,
  OUT VOID  **Buffer
  )
{
  FASTBOOT_USB_PACKET_LIST *Entry;

  if (IsListEmpty (&mPacketList)) {
    return EFI_NOT_READY;
  }

  Entry = (FASTBOOT_USB_PACKET_LIST *) GetFirstNode (&mPacketList);

  *BufferSize = Entry->BufferSize;
  *Buffer = Entry->Buffer;

  RemoveEntryList (&Entry->Link);
  FreePool (Entry);

  return EFI_SUCCESS;
}

STATIC FASTBOOT_TRANSPORT_PROTOCOL mTransportProtocol = {
  FastbootTransportUsbStart,
  FastbootTransportUsbStop,
  FastbootTransportUsbSend,
  FastbootTransportUsbReceive
};

EFI_STATUS
FastbootTransportUsbEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  // Assume there's only one USB peripheral controller.
  Status = gBS->LocateProtocol (&gUsbDeviceProtocolGuid, NULL, (VOID **) &mUsbDevice);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gAndroidFastbootTransportProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTransportProtocol
                  );
  return Status;
}

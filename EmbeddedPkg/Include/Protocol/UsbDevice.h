/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __USB_DEVICE_PROTOCOL_H__
#define __USB_DEVICE_PROTOCOL_H__

#include <IndustryStandard/Usb.h>

extern EFI_GUID gUsbDeviceProtocolGuid;

/*
 * Note: This Protocol is just  the bare minimum for Android Fastboot. It
 * only makes sense for devices that only do Bulk Transfers and only have one
 * endpoint.
 */

/*
  Callback to be called when data is received.
  Buffer is callee-allocated and it's the caller's responsibility to free it with
  FreePool.

  @param[in] Size        Size in bytes of data.
  @param[in] Buffer      Pointer to data.
*/
typedef
VOID
(*USB_DEVICE_RX_CALLBACK) (
  IN UINTN    Size,
  IN VOID    *Buffer
  );

/*
  Callback to be called when the host asks for data by sending an IN token
  (excluding during the data stage of a control transfer).
  When this function is called, data previously buffered by calling Send() has
  been sent.

  @param[in]Endpoint    Endpoint index, as specified in endpoint descriptors, of
                        the endpoint the IN token was sent to.
*/
typedef
VOID
(*USB_DEVICE_TX_CALLBACK) (
  IN UINT8    EndpointIndex
  );

/*
  Put data in the Tx buffer to be sent on the next IN token.
  Don't call this function again until the TxCallback has been called.

  @param[in]Endpoint    Endpoint index, as specified in endpoint descriptors, of
                        the endpoint to send the data from.
  @param[in]Size        Size in bytes of data.
  @param[in]Buffer      Pointer to data.

  @retval EFI_SUCCESS           The data was queued successfully.
  @retval EFI_INVALID_PARAMETER There was an error sending the data.
*/
typedef
EFI_STATUS
(*USB_DEVICE_SEND) (
  IN       UINT8    EndpointIndex,
  IN       UINTN    Size,
  IN CONST VOID    *Buffer
  );

/*
  Restart the USB peripheral controller and respond to enumeration.

  @param[in] DeviceDescriptor   pointer to device descriptor
  @param[in] Descriptors        Array of pointers to buffers, where
                                Descriptors[n] contains the response to a
                                GET_DESCRIPTOR request for configuration n. From
                                USB Spec section 9.4.3:
                                "The first interface descriptor follows the
                                configuration descriptor. The endpoint
                                descriptors for the first interface follow the
                                first interface descriptor. If there are
                                additional interfaces, their interface
                                descriptor and endpoint descriptors follow the
                                first interface’s endpoint descriptors".

                                The size of each buffer is the TotalLength
                                member of the Configuration Descriptor.

                                The size of the array is
                                DeviceDescriptor->NumConfigurations.
  @param[in]RxCallback          See USB_DEVICE_RX_CALLBACK
  @param[in]TxCallback          See USB_DEVICE_TX_CALLBACK
*/
typedef
EFI_STATUS
(*USB_DEVICE_START) (
  IN USB_DEVICE_DESCRIPTOR     *DeviceDescriptor,
  IN VOID                     **Descriptors,
  IN USB_DEVICE_RX_CALLBACK     RxCallback,
  IN USB_DEVICE_TX_CALLBACK     TxCallback
  );

struct _USB_DEVICE_PROTOCOL {
  USB_DEVICE_START Start;
  USB_DEVICE_SEND  Send;
};

typedef struct _USB_DEVICE_PROTOCOL USB_DEVICE_PROTOCOL;

#endif //ifndef __USB_DEVICE_PROTOCOL_H__

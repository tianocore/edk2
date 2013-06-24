/** @file
  Defines the PEI_USB_IO_PPI that the USB-related PEIM can use for I/O operations 
  on the USB BUS.  This interface enables recovery from a 
  USB-class storage device, such as USB CD/DVD, USB hard drive, or USB FLASH 
  drive.  These interfaces are modeled on the UEFI 2.3 specification EFI_USB_IO_PROTOCOL.
  Refer to section 16.2.4 of the UEFI 2.3 Specification for more information on 
  these interfaces.

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_USB_IO_PPI_H_
#define _PEI_USB_IO_PPI_H_

#include <Protocol/Usb2HostController.h>

///
/// Global ID for the PEI_USB_IO_PPI.
///
#define PEI_USB_IO_PPI_GUID \
  { \
    0x7c29785c, 0x66b9, 0x49fc, { 0xb7, 0x97, 0x1c, 0xa5, 0x55, 0xe, 0xf2, 0x83} \
  }

///
/// Forward declaration for the PEI_USB_IO_PPI.
///
typedef struct _PEI_USB_IO_PPI  PEI_USB_IO_PPI;

/**
  Submits control transfer to a target USB device.

  @param[in]     PeiServices   The pointer to the PEI Services Table.
  @param[in]     This          The pointer to this instance of the PEI_USB_IO_PPI.
  @param[in]     Request       A pointer to the USB device request that will be 
                               sent to the USB device.
  @param[in]     Direction     Specifies the data direction for the transfer. There 
                               are three values available: 
                               EfiUsbDataIn, EfiUsbDataOut and EfiUsbNoData.
  @param[in]     Timeout       Indicates the maximum time, in milliseconds, that 
                               the transfer is allowed to complete.
                               If Timeout is 0, then the caller must wait for the
                               function to be completed until EFI_SUCCESS or
                               EFI_DEVICE_ERROR is returned.
  @param[in,out] Data          A pointer to the buffer of data that will be 
                               transmitted to or received from the USB device.
  @param[in]     DataLength    On input, indicates the size, in bytes, of the data 
                               buffer specified by Data.
                               
  @retval EFI_SUCCESS             The control transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER   Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    The control transfer could not be completed due 
                                  to a lack of resources.
  @retval EFI_TIMEOUT             The control transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR        The control transfer failed due to host controller 
                                  or device error.
                                  Caller should check TransferResult for detailed 
                                  error information.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_CONTROL_TRANSFER)(
  IN     EFI_PEI_SERVICES        **PeiServices,
  IN     PEI_USB_IO_PPI          *This,
  IN     EFI_USB_DEVICE_REQUEST  *Request,
  IN     EFI_USB_DATA_DIRECTION  Direction,
  IN     UINT32                  Timeout,
  IN OUT VOID                    *Data OPTIONAL,
  IN     UINTN                   DataLength  OPTIONAL
  );

/**
  Submits bulk transfer to a target USB device.

  @param[in] PeiServices       The pointer to the PEI Services Table.
  @param[in] This              The pointer to this instance of the PEI_USB_IO_PPI.
  @param[in] DeviceEndpoint    The endpoint address.
  @param[in] Data              The data buffer to be transfered.
  @param[in] DataLength        The length of data buffer.
  @param[in] Timeout           The timeout for the transfer, in milliseconds.
                               If Timeout is 0, then the caller must wait for the
                               function to be completed until EFI_SUCCESS or
                               EFI_DEVICE_ERROR is returned.

  @retval EFI_SUCCESS             The bulk transfer completed successfully.
  @retval EFI_INVALID_PARAMETER   Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    The bulk transfer could not be completed due to 
                                  a lack of resources.
  @retval EFI_TIMEOUT             The bulk transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR        The bulk transfer failed due to host controller 
                                  or device error.
                                  Caller should check TransferResult for detailed 
                                  error information.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_BULK_TRANSFER)(
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *This,
  IN UINT8             DeviceEndpoint,
  IN OUT VOID          *Data,
  IN OUT UINTN         *DataLength,
  IN UINTN             Timeout
  );

/**
  Get interface descriptor from a USB device.

  @param[in] PeiServices           The pointer to the PEI Services Table.
  @param[in] This                  The pointer to this instance of the PEI_USB_IO_PPI.
  @param[in] InterfaceDescriptor   The interface descriptor.

  @retval EFI_SUCCESS             The interface descriptor was returned.
  @retval EFI_INVALID_PARAMETER   Some parameters are invalid.
  @retval EFI_DEVICE_ERROR        A device error occurred, the function failed to 
                                  get the interface descriptor.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_GET_INTERFACE_DESCRIPTOR)(
  IN EFI_PEI_SERVICES              **PeiServices,
  IN PEI_USB_IO_PPI                *This,
  IN EFI_USB_INTERFACE_DESCRIPTOR  **InterfaceDescriptor
  );

/**
  Get endpoint descriptor from a USB device.

  @param[in] PeiServices          The pointer to the PEI Services Table.
  @param[in] This                 The pointer to this instance of the PEI_USB_IO_PPI.
  @param[in] EndPointIndex        The index of the end point.
  @param[in] EndpointDescriptor   The endpoint descriptor.

  @retval EFI_SUCCESS             The endpoint descriptor was returned.
  @retval EFI_INVALID_PARAMETER   Some parameters are invalid.
  @retval EFI_DEVICE_ERROR        A device error occurred, the function failed to 
                                  get the endpoint descriptor.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_GET_ENDPOINT_DESCRIPTOR)(
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_IO_PPI                 *This,
  IN UINT8                          EndpointIndex,
  IN EFI_USB_ENDPOINT_DESCRIPTOR    **EndpointDescriptor
  );

/**
  Issue a port reset to the device.

  @param[in] PeiServices   The pointer to the PEI Services Table.
  @param[in] This          The pointer to this instance of the PEI_USB_IO_PPI.

  @retval EFI_SUCCESS             The port reset was issued successfully.
  @retval EFI_INVALID_PARAMETER   Some parameters are invalid.
  @retval EFI_DEVICE_ERROR        Device error occurred.

**/
typedef
EFI_STATUS
(EFIAPI *PEI_USB_PORT_RESET)(
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *This
  );

///
/// This PPI contains a set of services to interact with the USB host controller.
/// These interfaces are modeled on the UEFI 2.3 specification EFI_USB_IO_PROTOCOL.
/// Refer to section 16.2.4 of the UEFI 2.3 Specification for more information on 
/// these interfaces.
///
struct _PEI_USB_IO_PPI {
  PEI_USB_CONTROL_TRANSFER          UsbControlTransfer;
  PEI_USB_BULK_TRANSFER             UsbBulkTransfer;
  PEI_USB_GET_INTERFACE_DESCRIPTOR  UsbGetInterfaceDescriptor;
  PEI_USB_GET_ENDPOINT_DESCRIPTOR   UsbGetEndpointDescriptor;
  PEI_USB_PORT_RESET                UsbPortReset;
};

extern EFI_GUID gPeiUsbIoPpiGuid;

#endif

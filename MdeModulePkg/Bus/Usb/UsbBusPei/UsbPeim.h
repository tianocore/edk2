/** @file
Usb Peim definition.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved. <BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PEI_USB_PEIM_H_
#define _PEI_USB_PEIM_H_


#include <PiPei.h>

#include <Ppi/UsbHostController.h>
#include <Ppi/Usb2HostController.h>
#include <Ppi/UsbIo.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Usb.h>

//
// A common header for usb standard descriptor.
// Each stand descriptor has a length and type.
//
#pragma pack(1)
typedef struct {
  UINT8                   Len;
  UINT8                   Type;
} USB_DESC_HEAD;
#pragma pack()

#define MAX_INTERFACE             8
#define MAX_ENDPOINT              16

#define PEI_USB_DEVICE_SIGNATURE  SIGNATURE_32 ('U', 's', 'b', 'D')
typedef struct {
  UINTN                         Signature;
  PEI_USB_IO_PPI                UsbIoPpi;
  EFI_PEI_PPI_DESCRIPTOR        UsbIoPpiList;
  UINT16                        MaxPacketSize0;
  UINT16                        DataToggle;
  UINT8                         DeviceAddress;
  UINT8                         DeviceSpeed;
  UINT8                         IsHub;
  UINT8                         DownStreamPortNo;
  UINTN                         AllocateAddress;
  PEI_USB_HOST_CONTROLLER_PPI   *UsbHcPpi;
  PEI_USB2_HOST_CONTROLLER_PPI  *Usb2HcPpi;
  UINT8                         ConfigurationData[1024];
  EFI_USB_CONFIG_DESCRIPTOR     *ConfigDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDescList[MAX_INTERFACE];
  EFI_USB_ENDPOINT_DESCRIPTOR   *EndpointDesc[MAX_ENDPOINT];
  EFI_USB_ENDPOINT_DESCRIPTOR   *EndpointDescList[MAX_INTERFACE][MAX_ENDPOINT];
  EFI_USB2_HC_TRANSACTION_TRANSLATOR Translator;
  UINT8                          Tier;
} PEI_USB_DEVICE;

#define PEI_USB_DEVICE_FROM_THIS(a) CR (a, PEI_USB_DEVICE, UsbIoPpi, PEI_USB_DEVICE_SIGNATURE)

#define USB_BIT_IS_SET(Data, Bit)   ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define USB_BUS_1_MILLISECOND       1000

//
// Wait for port reset, refers to specification
// [USB20-7.1.7.5, it says 10ms for hub and 50ms for
// root hub]
//
// According to USB2.0, Chapter 11.5.1.5 Resetting,
// the worst case for TDRST is 20ms
//
#define USB_SET_PORT_RESET_STALL        (20 * USB_BUS_1_MILLISECOND)
#define USB_SET_ROOT_PORT_RESET_STALL   (50 * USB_BUS_1_MILLISECOND)

//
// Wait for clear roothub port reset, set by experience
//
#define USB_CLR_ROOT_PORT_RESET_STALL   (20 * USB_BUS_1_MILLISECOND)

//
// Wait for port statue reg change, set by experience
//
#define USB_WAIT_PORT_STS_CHANGE_STALL  (100)

//
// Host software return timeout if port status doesn't change
// after 500ms(LOOP * STALL = 5000 * 0.1ms), set by experience
//
#define USB_WAIT_PORT_STS_CHANGE_LOOP   5000

//
// Wait for hub port power-on, refers to specification
// [USB20-11.23.2]
//
#define USB_SET_PORT_POWER_STALL        (2 * USB_BUS_1_MILLISECOND)

//
// Wait for set device address, refers to specification
// [USB20-9.2.6.3, it says 2ms]
//
#define USB_SET_DEVICE_ADDRESS_STALL    (2 * USB_BUS_1_MILLISECOND)

//
// Wait for get configuration descriptor, set by experience
//
#define USB_GET_CONFIG_DESCRIPTOR_STALL (1 * USB_BUS_1_MILLISECOND)

/**
  Submits control transfer to a target USB device.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_IO_PPI.
  @param  Request                USB device request to send.
  @param  Direction              Specifies the data direction for the data stage.
  @param  Timeout                Indicates the maximum timeout, in millisecond. If Timeout
                                 is 0, then the caller must wait for the function to be
                                 completed until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
PeiUsbControlTransfer (
  IN     EFI_PEI_SERVICES          **PeiServices,
  IN     PEI_USB_IO_PPI            *This,
  IN     EFI_USB_DEVICE_REQUEST    *Request,
  IN     EFI_USB_DATA_DIRECTION    Direction,
  IN     UINT32                    Timeout,
  IN OUT VOID                      *Data,      OPTIONAL
  IN     UINTN                     DataLength  OPTIONAL
  );

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_IO_PPI.
  @param  DeviceEndpoint        Endpoint number and its direction in bit 7.
  @param  Data                  A pointer to the buffer of data to transmit
                                from or receive into.
  @param  DataLength            The length of the data buffer.
  @param  Timeout               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete. If Timeout is 0, then
                                the caller must wait for the function to be completed
                                until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
PeiUsbBulkTransfer (
  IN     EFI_PEI_SERVICES    **PeiServices,
  IN     PEI_USB_IO_PPI      *This,
  IN     UINT8               DeviceEndpoint,
  IN OUT VOID                *Data,
  IN OUT UINTN               *DataLength,
  IN     UINTN               Timeout
  );

/**
  Get the usb interface descriptor.

  @param  PeiServices          General-purpose services that are available to every PEIM.
  @param  This                 Indicates the PEI_USB_IO_PPI instance.
  @param  InterfaceDescriptor  Request interface descriptor.


  @retval EFI_SUCCESS          Usb interface descriptor is obtained successfully.

**/
EFI_STATUS
EFIAPI
PeiUsbGetInterfaceDescriptor (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  PEI_USB_IO_PPI                  *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR    **InterfaceDescriptor
  );

/**
  Get the usb endpoint descriptor.

  @param  PeiServices          General-purpose services that are available to every PEIM.
  @param  This                 Indicates the PEI_USB_IO_PPI instance.
  @param  EndpointIndex        The valid index of the specified endpoint.
  @param  EndpointDescriptor   Request endpoint descriptor.

  @retval EFI_SUCCESS       Usb endpoint descriptor is obtained successfully.
  @retval EFI_NOT_FOUND     Usb endpoint descriptor is NOT found.

**/
EFI_STATUS
EFIAPI
PeiUsbGetEndpointDescriptor (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  PEI_USB_IO_PPI                 *This,
  IN  UINT8                          EndpointIndex,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR    **EndpointDescriptor
  );

/**
  Reset the port and re-configure the usb device.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  This           Indicates the PEI_USB_IO_PPI instance.

  @retval EFI_SUCCESS    Usb device is reset and configured successfully.
  @retval Others         Other failure occurs.

**/
EFI_STATUS
EFIAPI
PeiUsbPortReset (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN PEI_USB_IO_PPI      *This
  );

/**
  Send reset signal over the given root hub port.

  @param  PeiServices       Describes the list of possible PEI Services.
  @param  UsbHcPpi          The pointer of PEI_USB_HOST_CONTROLLER_PPI instance.
  @param  Usb2HcPpi         The pointer of PEI_USB2_HOST_CONTROLLER_PPI instance.
  @param  PortNum           The port to be reset.
  @param  RetryIndex        The retry times.

**/
VOID
ResetRootPort (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    *UsbHcPpi,
  IN PEI_USB2_HOST_CONTROLLER_PPI   *Usb2HcPpi,
  IN UINT8                          PortNum,
  IN UINT8                          RetryIndex
  );

#endif

/** @file MockUsbIo.cpp
  Google Test mock for Service Binding Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockUsbIo.h>

MOCK_INTERFACE_DEFINITION (MockUsbIoProtocol);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbControlTransfer, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbBulkTransfer, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbAsyncInterruptTransfer, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbSyncInterruptTransfer, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbIsochronousTransfer, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbAsyncIsochronousTransfer, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbGetDeviceDescriptor, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbGetConfigDescriptor, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbGetInterfaceDescriptor, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbGetEndpointDescriptor, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbGetStringDescriptor, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbGetSupportedLanguages, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUsbIoProtocol, UsbPortReset, 1, EFIAPI);

EFI_USB_IO_PROTOCOL  USB_IO_PROTOCOL_MOCK = {
  UsbControlTransfer,             // EFI_USB_IO_CONTROL_TRANSFER              UsbControlTransfer;
  UsbBulkTransfer,                // EFI_USB_IO_BULK_TRANSFER                 UsbBulkTransfer;
  UsbAsyncInterruptTransfer,      // EFI_USB_IO_ASYNC_INTERRUPT_TRANSFER      UsbAsyncInterruptTransfer;
  UsbSyncInterruptTransfer,       // EFI_USB_IO_SYNC_INTERRUPT_TRANSFER       UsbSyncInterruptTransfer;
  UsbIsochronousTransfer,         // EFI_USB_IO_ISOCHRONOUS_TRANSFER          UsbIsochronousTransfer;
  UsbAsyncIsochronousTransfer,    // EFI_USB_IO_ASYNC_ISOCHRONOUS_TRANSFER    UsbAsyncIsochronousTransfer;
  UsbGetDeviceDescriptor,         // EFI_USB_IO_GET_DEVICE_DESCRIPTOR         UsbGetDeviceDescriptor;
  UsbGetConfigDescriptor,         // EFI_USB_IO_GET_CONFIG_DESCRIPTOR         UsbGetConfigDescriptor;
  UsbGetInterfaceDescriptor,      // EFI_USB_IO_GET_INTERFACE_DESCRIPTOR      UsbGetInterfaceDescriptor;
  UsbGetEndpointDescriptor,       // EFI_USB_IO_GET_ENDPOINT_DESCRIPTOR       UsbGetEndpointDescriptor;
  UsbGetStringDescriptor,         // EFI_USB_IO_GET_STRING_DESCRIPTOR         UsbGetStringDescriptor;
  UsbGetSupportedLanguages,       // EFI_USB_IO_GET_SUPPORTED_LANGUAGE        UsbGetSupportedLanguages;
  UsbPortReset                    // EFI_USB_IO_PORT_RESET                    UsbPortReset;
};

extern "C" {
  EFI_USB_IO_PROTOCOL  *gUsbIoProtocol = &USB_IO_PROTOCOL_MOCK;
}

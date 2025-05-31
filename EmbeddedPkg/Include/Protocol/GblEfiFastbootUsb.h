/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: Apache-2

**/

/*
  GBL EFI Fastboot USB Protocol.
  Lets firmware expose a device-mode USB interface for Android Fastboot.
*/

#ifndef GBL_EFI_FASTBOOT_USB_PROTOCOL_H_
#define GBL_EFI_FASTBOOT_USB_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {6281a893-ac23-4ca7-b281-340ef8168955}
//
#define GBL_EFI_FASTBOOT_USB_PROTOCOL_GUID \
  { 0x6281a893, 0xac23, 0x4ca7, { 0xb2, 0x81, 0x34, 0x0e, 0xf8, 0x16, 0x89, 0x55 } }

#define GBL_EFI_FASTBOOT_USB_PROTOCOL_REVISION  0x00000000

typedef struct _GBL_EFI_FASTBOOT_USB_PROTOCOL  GBL_EFI_FASTBOOT_USB_PROTOCOL;

/// Start Fastboot USB interface and report max packet size.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_INTERFACE_START)(
  IN  GBL_EFI_FASTBOOT_USB_PROTOCOL  *This,
  OUT UINTN                          *MaxPacketSize
  );

/// Stop Fastboot USB interface.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_INTERFACE_STOP)(
  IN GBL_EFI_FASTBOOT_USB_PROTOCOL  *This
  );

/// Receive next USB packet if available.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_RECEIVE)(
  IN     GBL_EFI_FASTBOOT_USB_PROTOCOL  *This,
  IN OUT UINTN                          *BufferSize,
  OUT    VOID                           *Buffer
  );

/// Send a USB packet (non-blocking).
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_SEND)(
  IN     GBL_EFI_FASTBOOT_USB_PROTOCOL  *This,
  IN OUT UINTN                          *BufferSize,
  IN     CONST VOID                     *Buffer
  );

/*
  Firmware-published protocol instance.
*/
struct _GBL_EFI_FASTBOOT_USB_PROTOCOL {
  UINT64                                             Revision;
  GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_INTERFACE_START  FastbootUsbInterfaceStart;
  GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_INTERFACE_STOP   FastbootUsbInterfaceStop;
  GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_RECEIVE          FastbootUsbReceive;
  GBL_EFI_FASTBOOT_USB_FASTBOOT_USB_SEND             FastbootUsbSend;
  EFI_EVENT                                          WaitForSendCompletion;
};

#endif // GBL_EFI_FASTBOOT_USB_PROTOCOL_H_

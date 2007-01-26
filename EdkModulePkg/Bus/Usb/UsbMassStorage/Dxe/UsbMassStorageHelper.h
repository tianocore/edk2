/*++
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    UsbMassStorageHelper.h

Abstract:

    Function prototype for USB Mass Storage Driver

Revision History
--*/
#ifndef _USB_FLPHLP_H
#define _USB_FLPHLP_H

#include "UsbMassStorage.h"

EFI_STATUS
USBFloppyIdentify (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

EFI_STATUS
USBFloppyPacketCommand (
  USB_FLOPPY_DEV            *UsbFloppyDevice,
  VOID                      *Command,
  UINT8                     CommandSize,
  VOID                      *DataBuffer,
  UINT32                    BufferLength,
  EFI_USB_DATA_DIRECTION    Direction,
  UINT16                    TimeOutInMilliSeconds
  );

EFI_STATUS
USBFloppyInquiry (
  IN    USB_FLOPPY_DEV    *UsbFloppyDevice,
  OUT   USB_INQUIRY_DATA  **Idata
  );

EFI_STATUS
USBFloppyRead10 (
  IN    USB_FLOPPY_DEV    *UsbFloppyDevice,
  IN    VOID              *Buffer,
  IN    EFI_LBA           Lba,
  IN    UINTN             NumberOfBlocks
  );

EFI_STATUS
USBFloppyReadFormatCapacity (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

EFI_STATUS
UsbFloppyRequestSense (
  IN  USB_FLOPPY_DEV  *UsbFloppyDevice,
  OUT UINTN           *SenseCounts
  );

EFI_STATUS
UsbFloppyTestUnitReady (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

EFI_STATUS
USBFloppyWrite10 (
  IN    USB_FLOPPY_DEV    *UsbFloppyDevice,
  IN    VOID              *Buffer,
  IN    EFI_LBA           Lba,
  IN    UINTN             NumberOfBlocks
  );

EFI_STATUS
UsbFloppyDetectMedia (
  IN  USB_FLOPPY_DEV  *UsbFloppyDevice,
  OUT BOOLEAN         *MediaChange
  );

EFI_STATUS
UsbFloppyModeSense5APage5 (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

EFI_STATUS
UsbFloppyModeSense5APage1C (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

EFI_STATUS
UsbFloppyModeSense5APage3F (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

EFI_STATUS
UsbSCSIModeSense1APage3F (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

EFI_STATUS
UsbMassStorageModeSense (
  IN  USB_FLOPPY_DEV    *UsbFloppyDevice
  );

#endif

/**@file
   Head file for BDS Platform specific code

Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _PLATFORM_BOOT_MANAGER_H
#define _PLATFORM_BOOT_MANAGER_H

#include <PiDxe.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/BootLogoLib.h>
#include <Protocol/SmmAccess2.h>

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} PLATFORM_CONSOLE_CONNECT_ENTRY;

extern PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE,\
    END_ENTIRE_DEVICE_PATH_SUBTYPE,\
    { END_DEVICE_PATH_LENGTH, 0 },\
  }

#define CONSOLE_OUT BIT0
#define CONSOLE_IN  BIT1
#define STD_ERROR   BIT2

#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH  VendorDevicePath;
  UINT32              Instance;
} WIN_NT_VENDOR_DEVICE_PATH_NODE;

//
// Below is the platform console device path
//
typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  SerialDevice;
  UART_DEVICE_PATH                Uart;
  VENDOR_DEVICE_PATH              TerminalType;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_ISA_SERIAL_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  NtGopDevice;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_PLATFORM_GOP_DEVICE_PATH;

extern USB_CLASS_FORMAT_DEVICE_PATH              gUsbClassKeyboardDevicePath;

/**
  Use SystemTable Conout to stop video based Simple Text Out consoles from going
  to the video device. Put up LogoFile on every video device that is a console.

  @param[in]  LogoFile   File name of logo to display on the center of the screen.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found

**/
EFI_STATUS
PlatformBootManagerEnableQuietBoot (
  IN  EFI_GUID  *LogoFile
);

/**
  Use SystemTable Conout to turn on video based Simple Text Out consoles. The
  Simple Text Out screens will now be synced up with all non video output devices

  @retval EFI_SUCCESS     UGA devices are back in text mode and synced up.

**/
EFI_STATUS
PlatformBootManagerDisableQuietBoot (
  VOID
);

/**
  Show progress bar with title above it. It only works in Graphics mode.

  @param TitleForeground Foreground color for Title.
  @param TitleBackground Background color for Title.
  @param Title           Title above progress bar.
  @param ProgressColor   Progress bar color.
  @param Progress        Progress (0-100)
  @param PreviousValue   The previous value of the progress.

  @retval  EFI_STATUS       Success update the progress bar

**/
EFI_STATUS
PlatformBootManagerShowProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  IN CHAR16                        *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  IN UINTN                         Progress,
  IN UINTN                         PreviousValue
);

#endif // _PLATFORM_BOOT_MANAGER_H

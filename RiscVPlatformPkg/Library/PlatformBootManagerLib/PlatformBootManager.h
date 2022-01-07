/** @file
   Head file for BDS Platform specific code

Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_BOOT_MANAGER_H_
#define PLATFORM_BOOT_MANAGER_H_

#include <PiDxe.h>
#include <IndustryStandard/Bmp.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/BootLogo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  UINTN                       ConnectType;
} PLATFORM_CONSOLE_CONNECT_ENTRY;

extern PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];

#define CONSOLE_OUT  BIT0
#define CONSOLE_IN   BIT1
#define STD_ERROR    BIT2

// D3987D4B-971A-435F-8CAF-4967EB627241
#define EFI_SERIAL_DXE_GUID \
  { 0xD3987D4B, 0x971A, 0x435F, { 0x8C, 0xAF, 0x49, 0x67, 0xEB, 0x62, 0x72, 0x41 } }

typedef struct {
  VENDOR_DEVICE_PATH          Guid;
  UART_DEVICE_PATH            Uart;
  VENDOR_DEVICE_PATH          TerminalType;
  EFI_DEVICE_PATH_PROTOCOL    End;
} SERIAL_CONSOLE_DEVICE_PATH;

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
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
PlatformBootManagerMemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL  Level
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
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleBackground,
  IN CHAR16                         *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  ProgressColor,
  IN UINTN                          Progress,
  IN UINTN                          PreviousValue
  );

#endif // _PLATFORM_BOOT_MANAGER_H

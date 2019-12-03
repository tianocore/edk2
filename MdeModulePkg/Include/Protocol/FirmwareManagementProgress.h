/** @file
  EDK II Firmware Management Progress Protocol.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL_H__
#define __EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL_H__

#include <Protocol/GraphicsOutput.h>

///
/// EDK II Firmware Management Progress Protocol GUID value
///
#define EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL_GUID \
  { \
    0x1849bda2, 0x6952, 0x4e86, { 0xa1, 0xdb, 0x55, 0x9a, 0x3c, 0x47, 0x9d, 0xf1 } \
  }

///
/// EDK II Firmware Management Progress Protocol structure
///
typedef struct {
  ///
  /// The version of this structure.  Initial version value is 0x00000001.
  ///
  UINT32                               Version;
  ///
  /// The foreground color of a progress bar that is used by the Progress()
  /// function that is passed into the Firmware Management Protocol SetImage()
  /// service is called.
  ///
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  ProgressBarForegroundColor;
  ///
  /// The time in seconds to arm the watchdog timer each time the Progress()
  /// function passed into the  Firmware Management Protocol SetImage() service
  /// is called.
  ///
  UINTN                                WatchdogSeconds;
} EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL;

///
/// EDK II Firmware Management Progress Protocol GUID variable.
///
extern EFI_GUID gEdkiiFirmwareManagementProgressProtocolGuid;

#endif

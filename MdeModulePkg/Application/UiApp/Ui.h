/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _BDS_MODULE_H_
#define _BDS_MODULE_H_

#include <PiDxe.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/SmBios.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/GlobalVariable.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/FileInfo.h>
#include <Guid/HiiBootMaintenanceFormset.h>

#include <Protocol/LoadFile.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/Smbios.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/BlockIo.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/SerialIo.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/DevicePathToText.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PerformanceLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/CapsuleLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiHiiServicesLib.h>

#include <Library/UefiBootManagerLib.h>

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()


//
//The interface functions related to the Setup Browser Reset Reminder feature
//
/**
  Enable the setup browser reset reminder feature.
  This routine is used in a platform tip. If the platform policy needs the feature, use the routine to enable it.

**/
VOID
EFIAPI
EnableResetReminderFeature (
  VOID
  );

/**
  Disable the setup browser reset reminder feature.
  This routine is used in a platform tip. If the platform policy does not want the feature, use the routine to disable it.

**/
VOID
EFIAPI
DisableResetReminderFeature (
  VOID
  );

/**
  Record the info that a reset is required.
  A module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
EnableResetRequired (
  VOID
  );


/**
  Record the info that no reset is required.
  A module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
DisableResetRequired (
  VOID
  );

/**
  Check whether platform policy enables the reset reminder feature. The default is enabled.

**/
BOOLEAN
EFIAPI
IsResetReminderFeatureEnable (
  VOID
  );

/**
  Check if the user changed any option setting that needs a system reset to be effective.

**/
BOOLEAN
EFIAPI
IsResetRequired (
  VOID
  );

/**
  Check whether a reset is needed, and finish the reset reminder feature.
  If a reset is needed, pop up a menu to notice user, and finish the feature
  according to the user selection.

**/
VOID
EFIAPI
SetupResetReminder (
  VOID
  );

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
UiDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );
#endif

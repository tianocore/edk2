/** @file
  UEFI variable support functions for Firmware Management Protocol based
  firmware updates.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VARIABLE_SUPPORT_H__
#define __VARIABLE_SUPPORT_H__

#define DEFAULT_VERSION                 0x1
#define DEFAULT_LOWESTSUPPORTEDVERSION  0x0
#define DEFAULT_LASTATTEMPT             0x0

#define VARNAME_VERSION                 L"FmpVersion"
#define VARNAME_LSV                     L"FmpLsv"

#define VARNAME_LASTATTEMPTSTATUS       L"LastAttemptStatus"
#define VARNAME_LASTATTEMPTVERSION      L"LastAttemptVersion"

/**
  Returns the value used to fill in the Version field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default version value
  is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpVersion"

  @return  The version of the firmware image in the firmware device.

**/
UINT32
GetVersionFromVariable (
  VOID
  );

/**
  Returns the value used to fill in the LowestSupportedVersion field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default lowest
  supported version value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpLsv"

  @return  The lowest supported version of the firmware image in the firmware
           device.

**/
UINT32
GetLowestSupportedVersionFromVariable (
  VOID
  );

/**
  Returns the value used to fill in the LastAttemptStatus field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default last attempt
  status value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"LastAttemptStatus"

  @return  The last attempt status value for the most recent capsule update.

**/
UINT32
GetLastAttemptStatusFromVariable (
  VOID
  );

/**
  Returns the value used to fill in the LastAttemptVersion field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default last attempt
  version value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"LastAttemptVersion"

  @return  The last attempt version value for the most recent capsule update.

**/
UINT32
GetLastAttemptVersionFromVariable (
  VOID
  );

/**
  Saves the version current of the firmware image in the firmware device to a
  UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpVersion"

  @param[in] Version  The version of the firmware image in the firmware device.

**/
VOID
SetVersionInVariable (
   UINT32  Version
  );

/**
  Saves the lowest supported version current of the firmware image in the
  firmware device to a UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpLsv"

  @param[in] LowestSupportedVersion The lowest supported version of the firmware image
                                    in the firmware device.

**/
VOID
SetLowestSupportedVersionInVariable (
   UINT32  LowestSupportedVersion
  );

/**
  Saves the last attempt status value of the most recent FMP capsule update to a
  UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"LastAttemptStatus"

  @param[in] LastAttemptStatus  The last attempt status of the most recent FMP
                                capsule update.

**/
VOID
SetLastAttemptStatusInVariable (
   UINT32  LastAttemptStatus
  );

/**
  Saves the last attempt version value of the most recent FMP capsule update to
  a UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"LastAttemptVersion"

  @param[in] LastAttemptVersion  The last attempt version value of the most
                                 recent FMP capsule update.

**/
VOID
SetLastAttemptVersionInVariable (
   UINT32  LastAttemptVersion
  );

/**
  Locks all the UEFI Variables that use gEfiCallerIdGuid of the currently
  executing module.

**/
EFI_STATUS
LockAllFmpVariables (
  VOID
  );

#endif

/** @file
  UEFI variable support functions for Firmware Management Protocol based
  firmware updates.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VARIABLE_SUPPORT_H__
#define __VARIABLE_SUPPORT_H__

///
/// Default values for FMP Controller State information
///
#define DEFAULT_VERSION                 0x1
#define DEFAULT_LOWESTSUPPORTEDVERSION  0x0
#define DEFAULT_LASTATTEMPTSTATUS       0x0
#define DEFAULT_LASTATTEMPTVERSION      0x0

///
/// Base UEFI Variable names for FMP Controller State information stored in
/// separate variables.
///
#define VARNAME_VERSION                 L"FmpVersion"
#define VARNAME_LSV                     L"FmpLsv"
#define VARNAME_LASTATTEMPTSTATUS       L"LastAttemptStatus"
#define VARNAME_LASTATTEMPTVERSION      L"LastAttemptVersion"

///
/// Base UEFI Variable name for FMP Controller State information stored in a
/// merged UEFI Variable.  If the separate UEFI Variables above are detected,
/// then they are merged into a single variable and the separate variables are
/// deleted.
///
#define VARNAME_FMPSTATE                L"FmpState"

///
/// FMP Controller State structure that is used to store the state of
/// a controller in one combined UEFI Variable.
///
typedef struct {
  BOOLEAN  VersionValid;
  BOOLEAN  LsvValid;
  BOOLEAN  LastAttemptStatusValid;
  BOOLEAN  LastAttemptVersionValid;
  UINT32   Version;
  UINT32   Lsv;
  UINT32   LastAttemptStatus;
  UINT32   LastAttemptVersion;
} FMP_CONTROLLER_STATE;

/**
  Generate the names of the UEFI Variables used to store state information for
  a managed controller.  The UEFI Variables names are a combination of a base
  name and an optional hardware instance value as a 16 character hex value.  If
  the hardware instance value is 0, then the 16 character hex value is not
  included.  These storage for the UEFI Variable names are allocated using the
  UEFI Boot Service AllocatePool() and the pointers are stored in the Private.
  The following are examples of variable names produces for hardware instance
  value 0 and value 0x1234567812345678.

    FmpVersion
    FmpLsv
    LastAttemptStatus
    LastAttemptVersion
    FmpDxe

    FmpVersion1234567812345678
    FmpLsv1234567812345678
    LastAttemptStatus1234567812345678
    LastAttemptVersion1234567812345678
    FmpDxe1234567812345678

  @param[in,out] Private  Private context structure for the managed controller.
**/
VOID
GenerateFmpVariableNames (
  IN OUT FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  );

/**
  Returns the value used to fill in the Version field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default version value
  is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private  Private context structure for the managed controller.

  @return  The version of the firmware image in the firmware device.
**/
UINT32
GetVersionFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  );

/**
  Returns the value used to fill in the LowestSupportedVersion field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default lowest
  supported version value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private  Private context structure for the managed controller.

  @return  The lowest supported version of the firmware image in the firmware
           device.
**/
UINT32
GetLowestSupportedVersionFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  );

/**
  Returns the value used to fill in the LastAttemptStatus field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default last attempt
  status value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private  Private context structure for the managed controller.

  @return  The last attempt status value for the most recent capsule update.
**/
UINT32
GetLastAttemptStatusFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  );

/**
  Returns the value used to fill in the LastAttemptVersion field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default last attempt
  version value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private  Private context structure for the managed controller.

  @return  The last attempt version value for the most recent capsule update.
**/
UINT32
GetLastAttemptVersionFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  );

/**
  Saves the version current of the firmware image in the firmware device to a
  UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private  Private context structure for the managed controller.
  @param[in] Version  The version of the firmware image in the firmware device.
**/
VOID
SetVersionInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            Version
  );

/**
  Saves the lowest supported version current of the firmware image in the
  firmware device to a UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private                 Private context structure for the managed
                                     controller.
  @param[in] LowestSupportedVersion  The lowest supported version of the
                                     firmware image in the firmware device.
**/
VOID
SetLowestSupportedVersionInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            LowestSupportedVersion
  );

/**
  Saves the last attempt status value of the most recent FMP capsule update to a
  UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private            Private context structure for the managed
                                controller.
  @param[in] LastAttemptStatus  The last attempt status of the most recent FMP
                                capsule update.
**/
VOID
SetLastAttemptStatusInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            LastAttemptStatus
  );

/**
  Saves the last attempt version value of the most recent FMP capsule update to
  a UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpDxe"

  @param[in] Private             Private context structure for the managed
                                 controller.
  @param[in] LastAttemptVersion  The last attempt version value of the most
                                 recent FMP capsule update.
**/
VOID
SetLastAttemptVersionInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            LastAttemptVersion
  );

/**
  Locks all the UEFI Variables that use gEfiCallerIdGuid of the currently
  executing module.

  @param[in] Private  Private context structure for the managed controller.

  @retval  EFI_SUCCESS      All UEFI variables are locked.
  @retval  EFI_UNSUPPORTED  Variable Lock Protocol not found.
  @retval  Other            One of the UEFI variables could not be locked.
**/
EFI_STATUS
LockAllFmpVariables (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  );

#endif

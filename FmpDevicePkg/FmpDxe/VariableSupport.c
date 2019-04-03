/** @file
  UEFI variable support functions for Firmware Management Protocol based
  firmware updates.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/VariableLock.h>
#include "VariableSupport.h"

///
/// Array of UEFI variable names that are locked in LockAllFmpVariables().
///
const CHAR16  *mFmpVariableLockList[] = {
  VARNAME_VERSION,
  VARNAME_LSV,
  VARNAME_LASTATTEMPTSTATUS,
  VARNAME_LASTATTEMPTVERSION
};

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
  )
{
  EFI_STATUS  Status;
  UINT32      *Value;
  UINTN       Size;
  UINT32      Version;

  Value = NULL;
  Size = 0;
  Version = DEFAULT_VERSION;

  Status = GetVariable2 (VARNAME_VERSION, &gEfiCallerIdGuid, (VOID **)&Value, &Size);
  if (EFI_ERROR (Status) || (Value == NULL)) {
    DEBUG ((DEBUG_ERROR, "Failed to get the Version from variable.  Status = %r\n", Status));
    return Version;
  }

  //
  // No error from call
  //
  if (Size == sizeof (*Value)) {
    //
    // Successful read
    //
    Version = *Value;
  } else {
    //
    // Return default since size was unknown
    //
    DEBUG ((DEBUG_ERROR, "Getting version Variable returned a size different than expected. Size = 0x%x\n", Size));
  }

  FreePool (Value);

  return Version;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      *Value;
  UINTN       Size;
  UINT32      Version;

  Value   = NULL;
  Size    = 0;
  Version = DEFAULT_LOWESTSUPPORTEDVERSION;

  Status = GetVariable2 (VARNAME_LSV, &gEfiCallerIdGuid, (VOID **)&Value, &Size);
  if (EFI_ERROR (Status) || (Value == NULL)) {
    DEBUG ((DEBUG_WARN, "Warning: Failed to get the Lowest Supported Version from variable.  Status = %r\n", Status));
    return Version;
  }

  //
  // No error from call
  //
  if (Size == sizeof (*Value)) {
    //
    // Successful read
    //
    Version = *Value;
  } else {
    //
    // Return default since size was unknown
    //
    DEBUG ((DEBUG_ERROR, "Getting LSV Variable returned a size different than expected. Size = 0x%x\n", Size));
  }

  FreePool (Value);

  return Version;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      *Value;
  UINTN       Size;
  UINT32      LastAttemptStatus;

  Value = NULL;
  Size  = 0;
  LastAttemptStatus     = DEFAULT_LASTATTEMPT;

  Status = GetVariable2 (VARNAME_LASTATTEMPTSTATUS, &gEfiCallerIdGuid, (VOID **)&Value, &Size);
  if (EFI_ERROR (Status) || (Value == NULL)) {
    DEBUG ((DEBUG_WARN, "Warning: Failed to get the Last Attempt Status from variable.  Status = %r\n", Status));
    return LastAttemptStatus;
  }

  //
  // No error from call
  //
  if (Size == sizeof (*Value)) {
    //
    // Successful read
    //
    LastAttemptStatus = *Value;
  } else {
    //
    // Return default since size was unknown
    //
    DEBUG (
      (DEBUG_ERROR,
      "Getting Last Attempt Status Variable returned a size different than expected. Size = 0x%x\n",
      Size)
      );
  }

  FreePool (Value);

  return LastAttemptStatus;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      *Value;
  UINTN       Size;
  UINT32      Version;

  Value   = NULL;
  Size    = 0;
  Version = DEFAULT_LASTATTEMPT;

  Status = GetVariable2 (VARNAME_LASTATTEMPTVERSION, &gEfiCallerIdGuid, (VOID **)&Value, &Size);
  if (EFI_ERROR (Status) || (Value == NULL)) {
    DEBUG ((DEBUG_WARN, "Warning: Failed to get the Last Attempt Version from variable.  Status = %r\n", Status));
    return Version;
  }

  //
  // No error from call
  //
  if (Size == sizeof (*Value)) {
    //
    // Successful read
    //
    Version = *Value;
  } else {
    //
    // Return default since size was unknown
    //
    DEBUG (
      (DEBUG_ERROR,
      "Getting Last Attempt Version variable returned a size different than expected. Size = 0x%x\n",
      Size)
      );
  }

  FreePool (Value);

  return Version;
}


/**
  Saves the version current of the firmware image in the firmware device to a
  UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpVersion"

  @param[in] Version  The version of the firmware image in the firmware device.

**/
VOID
SetVersionInVariable (
   UINT32  Version
  )
{
  EFI_STATUS  Status;
  UINT32      Current;

  Status = EFI_SUCCESS;

  Current = GetVersionFromVariable();
  if (Current != Version) {
    Status = gRT->SetVariable (
                    VARNAME_VERSION,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (Version),
                    &Version
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to set the Version into a variable.  Status = %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Version variable doesn't need to update.  Same value as before.\n"));
  }
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      Current;

  Status = EFI_SUCCESS;

  Current = GetLowestSupportedVersionFromVariable();
  if (LowestSupportedVersion > Current) {
    Status = gRT->SetVariable (
                    VARNAME_LSV,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (LowestSupportedVersion), &LowestSupportedVersion
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to set the LSV into a variable.  Status = %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "LSV variable doesn't need to update.  Same value as before.\n"));
  }
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      Current;

  Status = EFI_SUCCESS;

  Current = GetLastAttemptStatusFromVariable();
  if (Current != LastAttemptStatus) {
    Status = gRT->SetVariable (
                    VARNAME_LASTATTEMPTSTATUS,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (LastAttemptStatus),
                    &LastAttemptStatus
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to set the LastAttemptStatus into a variable.  Status = %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "LastAttemptStatus variable doesn't need to update.  Same value as before.\n"));
  }
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      Current;

  Status = EFI_SUCCESS;

  Current = GetLastAttemptVersionFromVariable();
  if (Current != LastAttemptVersion) {
    Status = gRT->SetVariable (
                    VARNAME_LASTATTEMPTVERSION,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (LastAttemptVersion),
                    &LastAttemptVersion
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to set the LastAttemptVersion into a variable.  Status = %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "LastAttemptVersion variable doesn't need to update.  Same value as before.\n"));
  }
}

/**
  Locks all the UEFI Variables used by this module.

  @retval  EFI_SUCCESS      All UEFI variables are locked.
  @retval  EFI_UNSUPPORTED  Variable Lock Protocol not found.
  @retval  Other            One of the UEFI variables could not be locked.

**/
EFI_STATUS
LockAllFmpVariables (
  VOID
  )
{
  EFI_STATUS                    Status;
  EDKII_VARIABLE_LOCK_PROTOCOL  *VariableLock;
  EFI_STATUS                    ReturnStatus;
  UINTN                         Index;

  VariableLock = NULL;
  Status = gBS->LocateProtocol (
                  &gEdkiiVariableLockProtocolGuid,
                  NULL,
                  (VOID **)&VariableLock
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: Failed to locate Variable Lock Protocol (%r).\n", Status));
    return EFI_UNSUPPORTED;
  }

  ReturnStatus = EFI_SUCCESS;
  for (Index = 0; Index < ARRAY_SIZE (mFmpVariableLockList); Index++) {
    Status = VariableLock->RequestToLock (
                             VariableLock,
                             (CHAR16 *)mFmpVariableLockList[Index],
                             &gEfiCallerIdGuid
                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe: Failed to lock variable %g %s.  Status = %r\n",
        &gEfiCallerIdGuid,
        mFmpVariableLockList[Index],
        Status
        ));
      if (!EFI_ERROR (ReturnStatus)) {
        ReturnStatus = Status;
      }
    }
  }

  return ReturnStatus;
}

/** @file
  UEFI variable support functions for Firmware Management Protocol based
  firmware updates.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FmpDxe.h"
#include "VariableSupport.h"

/**
  Retrieve the value of a 32-bit UEFI Variable specified by VariableName and
  a GUID of gEfiCallerIdGuid.

  @param[in]  VariableName  Pointer to the UEFI Variable name to retrieve.
  @param[out] Valid         Set to TRUE if UEFI Variable is present and the size
                            of the UEFI Variable value is 32-bits.  Otherwise
                            FALSE.
  @param[out] Value         If Valid is set to TRUE, then the 32-bit value of
                            the UEFI Variable.  Otherwise 0.
**/
static
VOID
GetFmpVariable (
  IN  CHAR16   *VariableName,
  OUT BOOLEAN  *Valid,
  OUT UINT32   *Value
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  UINT32      *Buffer;

  *Valid = FALSE;
  *Value = 0;
  Size   = 0;
  Buffer = NULL;
  Status = GetVariable2 (
             VariableName,
             &gEfiCallerIdGuid,
             (VOID **)&Buffer,
             &Size
             );
  if (!EFI_ERROR (Status) && (Size == sizeof (*Value)) && (Buffer != NULL)) {
    *Valid = TRUE;
    *Value = *Buffer;
  }

  if (Buffer != NULL) {
    FreePool (Buffer);
  }
}

/**
  Delete the UEFI Variable with name specified by VariableName and GUID of
  gEfiCallerIdGuid.  If the variable can not be deleted, then print a
  DEBUG_ERROR message.

  @param[in] VariableName  Pointer to the UEFI Variable name to delete.
**/
static
VOID
DeleteFmpVariable (
  IN CHAR16  *VariableName
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Valid;
  UINT32      Value;

  GetFmpVariable (VariableName, &Valid, &Value);
  if (Valid) {
    Status = gRT->SetVariable (VariableName, &gEfiCallerIdGuid, 0, 0, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to delete variable %s.  Status = %r\n", mImageIdName, VariableName, Status));
    } else {
      DEBUG ((DEBUG_INFO, "FmpDxe(%s): Deleted variable %s\n", mImageIdName, VariableName));
    }
  }
}

/**
  Retrieve the FMP Controller State UEFI Variable value.  Return NULL if
  the variable does not exist or if the size of the UEFI Variable is not the
  size of FMP_CONTROLLER_STATE.  The buffer for the UEFI Variable value
  if allocated using the UEFI Boot Service AllocatePool().

  @param[in] Private  Private context structure for the managed controller.

  @return  Pointer to the allocated FMP Controller State.  Returns NULL
           if the variable does not exist or is a different size than expected.
**/
static
FMP_CONTROLLER_STATE *
GetFmpControllerState (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS            Status;
  FMP_CONTROLLER_STATE  *FmpControllerState;
  UINTN                 Size;

  FmpControllerState = NULL;
  Size               = 0;
  Status             = GetVariable2 (
                         Private->FmpStateVariableName,
                         &gEfiCallerIdGuid,
                         (VOID **)&FmpControllerState,
                         &Size
                         );
  if (EFI_ERROR (Status) || (FmpControllerState == NULL)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to get the controller state.  Status = %r\n", mImageIdName, Status));
  } else {
    if (Size == sizeof (*FmpControllerState)) {
      return FmpControllerState;
    }

    DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Getting controller state returned a size different than expected. Size = 0x%x\n", mImageIdName, Size));
  }

  if (FmpControllerState != NULL) {
    FreePool (FmpControllerState);
  }

  return NULL;
}

/**
  Generates a Null-terminated Unicode string UEFI Variable name from a base name
  and a hardware instance.  If the hardware instance value is 0, then the base
  name is returned.  If the hardware instance value is non-zero, then the 64-bit
  hardware instance value is converted to a 16 character hex string and appended
  to base name.  The UEFI Variable name returned is allocated using the UEFI
  Boot Service AllocatePool().

  @param[in] HardwareInstance  64-bit hardware instance value.
  @param[in] BaseVariableName  Null-terminated Unicode string that is the base
                               name of the UEFI Variable.

  @return  Pointer to the allocated UEFI Variable name.  Returns NULL if the
           UEFI Variable can not be allocated.
**/
static
CHAR16 *
GenerateFmpVariableName (
  IN  UINT64  HardwareInstance,
  IN  CHAR16  *BaseVariableName
  )
{
  UINTN   Size;
  CHAR16  *VariableName;

  //
  // Allocate Unicode string with room for BaseVariableName and a 16 digit
  // hexadecimal value for the HardwareInstance value.
  //
  Size         = StrSize (BaseVariableName) + 16 * sizeof (CHAR16);
  VariableName = AllocateCopyPool (Size, BaseVariableName);
  if (VariableName == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to generate variable name %s.\n", mImageIdName, BaseVariableName));
    return VariableName;
  }

  if (HardwareInstance == 0) {
    return VariableName;
  }

  UnicodeValueToStringS (
    &VariableName[StrLen (BaseVariableName)],
    Size,
    PREFIX_ZERO | RADIX_HEX,
    HardwareInstance,
    16
    );
  return VariableName;
}

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
    FmpState

    FmpVersion1234567812345678
    FmpLsv1234567812345678
    LastAttemptStatus1234567812345678
    LastAttemptVersion1234567812345678
    FmpState1234567812345678

  @param[in,out] Private  Private context structure for the managed controller.
**/
VOID
GenerateFmpVariableNames (
  IN OUT FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS            Status;
  VOID                  *Buffer;
  FMP_CONTROLLER_STATE  FmpControllerState;

  if (Private->VersionVariableName != NULL) {
    FreePool (Private->VersionVariableName);
  }

  if (Private->LsvVariableName != NULL) {
    FreePool (Private->LsvVariableName);
  }

  if (Private->LastAttemptStatusVariableName != NULL) {
    FreePool (Private->LastAttemptStatusVariableName);
  }

  if (Private->LastAttemptVersionVariableName != NULL) {
    FreePool (Private->LastAttemptVersionVariableName);
  }

  if (Private->FmpStateVariableName != NULL) {
    FreePool (Private->FmpStateVariableName);
  }

  Private->VersionVariableName = GenerateFmpVariableName (
                                   Private->Descriptor.HardwareInstance,
                                   VARNAME_VERSION
                                   );
  Private->LsvVariableName = GenerateFmpVariableName (
                               Private->Descriptor.HardwareInstance,
                               VARNAME_LSV
                               );
  Private->LastAttemptStatusVariableName = GenerateFmpVariableName (
                                             Private->Descriptor.HardwareInstance,
                                             VARNAME_LASTATTEMPTSTATUS
                                             );
  Private->LastAttemptVersionVariableName = GenerateFmpVariableName (
                                              Private->Descriptor.HardwareInstance,
                                              VARNAME_LASTATTEMPTVERSION
                                              );
  Private->FmpStateVariableName = GenerateFmpVariableName (
                                    Private->Descriptor.HardwareInstance,
                                    VARNAME_FMPSTATE
                                    );

  DEBUG ((DEBUG_INFO, "FmpDxe(%s): Variable %g %s\n", mImageIdName, &gEfiCallerIdGuid, Private->VersionVariableName));
  DEBUG ((DEBUG_INFO, "FmpDxe(%s): Variable %g %s\n", mImageIdName, &gEfiCallerIdGuid, Private->LsvVariableName));
  DEBUG ((DEBUG_INFO, "FmpDxe(%s): Variable %g %s\n", mImageIdName, &gEfiCallerIdGuid, Private->LastAttemptStatusVariableName));
  DEBUG ((DEBUG_INFO, "FmpDxe(%s): Variable %g %s\n", mImageIdName, &gEfiCallerIdGuid, Private->LastAttemptVersionVariableName));
  DEBUG ((DEBUG_INFO, "FmpDxe(%s): Variable %g %s\n", mImageIdName, &gEfiCallerIdGuid, Private->FmpStateVariableName));

  Buffer = GetFmpControllerState (Private);
  if (Buffer != NULL) {
    //
    // FMP Controller State was found with correct size.
    // Delete old variables if they exist.
    //
    FreePool (Buffer);
    DeleteFmpVariable (Private->VersionVariableName);
    DeleteFmpVariable (Private->LsvVariableName);
    DeleteFmpVariable (Private->LastAttemptStatusVariableName);
    DeleteFmpVariable (Private->LastAttemptVersionVariableName);
    return;
  }

  //
  // FMP Controller State was either not found or is wrong size.
  // Create a new FMP Controller State variable with the correct size.
  //
  DEBUG ((DEBUG_INFO, "FmpDxe(%s): Create controller state\n", mImageIdName));
  GetFmpVariable (
    Private->VersionVariableName,
    &FmpControllerState.VersionValid,
    &FmpControllerState.Version
    );
  GetFmpVariable (
    Private->LsvVariableName,
    &FmpControllerState.LsvValid,
    &FmpControllerState.Lsv
    );
  GetFmpVariable (
    Private->LastAttemptStatusVariableName,
    &FmpControllerState.LastAttemptStatusValid,
    &FmpControllerState.LastAttemptStatus
    );
  GetFmpVariable (
    Private->LastAttemptVersionVariableName,
    &FmpControllerState.LastAttemptVersionValid,
    &FmpControllerState.LastAttemptVersion
    );
  Status = gRT->SetVariable (
                  Private->FmpStateVariableName,
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (FmpControllerState),
                  &FmpControllerState
                  );
  if (EFI_ERROR (Status)) {
    //
    // Failed to create FMP Controller State.  In this case, do not
    // delete the individual variables.  They can be used again on next boot
    // to create the FMP Controller State.
    //
    DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to create controller state.  Status = %r\n", mImageIdName, Status));
  } else {
    DeleteFmpVariable (Private->VersionVariableName);
    DeleteFmpVariable (Private->LsvVariableName);
    DeleteFmpVariable (Private->LastAttemptStatusVariableName);
    DeleteFmpVariable (Private->LastAttemptVersionVariableName);
  }
}

/**
  Returns the value used to fill in the Version field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default version value
  is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private  Private context structure for the managed controller.

  @return  The version of the firmware image in the firmware device.
**/
UINT32
GetVersionFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  )
{
  FMP_CONTROLLER_STATE  *FmpControllerState;
  UINT32                Value;

  Value              = DEFAULT_VERSION;
  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState != NULL) {
    if (FmpControllerState->VersionValid) {
      Value = FmpControllerState->Version;
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Get variable %g %s Version %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        Value
        ));
    }

    FreePool (FmpControllerState);
  }

  return Value;
}

/**
  Returns the value used to fill in the LowestSupportedVersion field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default lowest
  supported version value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private  Private context structure for the managed controller.

  @return  The lowest supported version of the firmware image in the firmware
           device.
**/
UINT32
GetLowestSupportedVersionFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  )
{
  FMP_CONTROLLER_STATE  *FmpControllerState;
  UINT32                Value;

  Value              = DEFAULT_LOWESTSUPPORTEDVERSION;
  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState != NULL) {
    if (FmpControllerState->LsvValid) {
      Value = FmpControllerState->Lsv;
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Get variable %g %s LowestSupportedVersion %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        Value
        ));
    }

    FreePool (FmpControllerState);
  }

  return Value;
}

/**
  Returns the value used to fill in the LastAttemptStatus field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default last attempt
  status value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private  Private context structure for the managed controller.

  @return  The last attempt status value for the most recent capsule update.
**/
UINT32
GetLastAttemptStatusFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  )
{
  FMP_CONTROLLER_STATE  *FmpControllerState;
  UINT32                Value;

  Value              = DEFAULT_LASTATTEMPTSTATUS;
  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState != NULL) {
    if (FmpControllerState->LastAttemptStatusValid) {
      Value = FmpControllerState->LastAttemptStatus;
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Get variable %g %s LastAttemptStatus %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        Value
        ));
    }

    FreePool (FmpControllerState);
  }

  return Value;
}

/**
  Returns the value used to fill in the LastAttemptVersion field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  The value is read from a UEFI
  variable.  If the UEFI variables does not exist, then a default last attempt
  version value is returned.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private  Private context structure for the managed controller.

  @return  The last attempt version value for the most recent capsule update.
**/
UINT32
GetLastAttemptVersionFromVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  )
{
  FMP_CONTROLLER_STATE  *FmpControllerState;
  UINT32                Value;

  Value              = DEFAULT_LASTATTEMPTVERSION;
  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState != NULL) {
    if (FmpControllerState->LastAttemptVersionValid) {
      Value = FmpControllerState->LastAttemptVersion;
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Get variable %g %s LastAttemptVersion %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        Value
        ));
    }

    FreePool (FmpControllerState);
  }

  return Value;
}

/**
  Saves the version current of the firmware image in the firmware device to a
  UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private  Private context structure for the managed controller.
  @param[in] Version  The version of the firmware image in the firmware device.
**/
VOID
SetVersionInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            Version
  )
{
  EFI_STATUS            Status;
  FMP_CONTROLLER_STATE  *FmpControllerState;
  BOOLEAN               Update;

  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState == NULL) {
    //
    // Can not update value if FMP Controller State does not exist.
    // This variable is guaranteed to be created by GenerateFmpVariableNames().
    //
    return;
  }

  Update = FALSE;
  if (!FmpControllerState->VersionValid) {
    Update = TRUE;
  }

  if (FmpControllerState->Version != Version) {
    Update = TRUE;
  }

  if (!Update) {
    DEBUG ((DEBUG_INFO, "FmpDxe(%s): No need to update controller state.  Same value as before.\n", mImageIdName));
  } else {
    FmpControllerState->VersionValid = TRUE;
    FmpControllerState->Version      = Version;
    Status                           = gRT->SetVariable (
                                              Private->FmpStateVariableName,
                                              &gEfiCallerIdGuid,
                                              EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                                              sizeof (*FmpControllerState),
                                              FmpControllerState
                                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to update controller state.  Status = %r\n", mImageIdName, Status));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Set variable %g %s Version %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        Version
        ));
    }
  }

  FreePool (FmpControllerState);
}

/**
  Saves the lowest supported version current of the firmware image in the
  firmware device to a UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private                 Private context structure for the managed
                                     controller.
  @param[in] LowestSupportedVersion  The lowest supported version of the
                                     firmware image in the firmware device.
**/
VOID
SetLowestSupportedVersionInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            LowestSupportedVersion
  )
{
  EFI_STATUS            Status;
  FMP_CONTROLLER_STATE  *FmpControllerState;
  BOOLEAN               Update;

  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState == NULL) {
    //
    // Can not update value if FMP Controller State does not exist.
    // This variable is guaranteed to be created by GenerateFmpVariableNames().
    //
    return;
  }

  Update = FALSE;
  if (!FmpControllerState->LsvValid) {
    Update = TRUE;
  }

  if (FmpControllerState->Lsv < LowestSupportedVersion) {
    Update = TRUE;
  }

  if (!Update) {
    DEBUG ((DEBUG_INFO, "FmpDxe(%s): No need to update controller state.  Same value as before.\n", mImageIdName));
  } else {
    FmpControllerState->LsvValid = TRUE;
    FmpControllerState->Lsv      = LowestSupportedVersion;
    Status                       = gRT->SetVariable (
                                          Private->FmpStateVariableName,
                                          &gEfiCallerIdGuid,
                                          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                                          sizeof (*FmpControllerState),
                                          FmpControllerState
                                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to update controller state.  Status = %r\n", mImageIdName, Status));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Set variable %g %s LowestSupportedVersion %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        LowestSupportedVersion
        ));
    }
  }

  FreePool (FmpControllerState);
}

/**
  Saves the last attempt status value of the most recent FMP capsule update to a
  UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private            Private context structure for the managed
                                controller.
  @param[in] LastAttemptStatus  The last attempt status of the most recent FMP
                                capsule update.
**/
VOID
SetLastAttemptStatusInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            LastAttemptStatus
  )
{
  EFI_STATUS            Status;
  FMP_CONTROLLER_STATE  *FmpControllerState;
  BOOLEAN               Update;

  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState == NULL) {
    //
    // Can not update value if FMP Controller State does not exist.
    // This variable is guaranteed to be created by GenerateFmpVariableNames().
    //
    return;
  }

  Update = FALSE;
  if (!FmpControllerState->LastAttemptStatusValid) {
    Update = TRUE;
  }

  if (FmpControllerState->LastAttemptStatus != LastAttemptStatus) {
    Update = TRUE;
  }

  if (!Update) {
    DEBUG ((DEBUG_INFO, "FmpDxe(%s): No need to update controller state.  Same value as before.\n", mImageIdName));
  } else {
    FmpControllerState->LastAttemptStatusValid = TRUE;
    FmpControllerState->LastAttemptStatus      = LastAttemptStatus;
    Status                                     = gRT->SetVariable (
                                                        Private->FmpStateVariableName,
                                                        &gEfiCallerIdGuid,
                                                        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                                                        sizeof (*FmpControllerState),
                                                        FmpControllerState
                                                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to update controller state.  Status = %r\n", mImageIdName, Status));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Set variable %g %s LastAttemptStatus %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        LastAttemptStatus
        ));
    }
  }

  FreePool (FmpControllerState);
}

/**
  Saves the last attempt version value of the most recent FMP capsule update to
  a UEFI variable.

  UEFI Variable accessed: GUID = gEfiCallerIdGuid, Name = L"FmpState"

  @param[in] Private             Private context structure for the managed
                                 controller.
  @param[in] LastAttemptVersion  The last attempt version value of the most
                                 recent FMP capsule update.
**/
VOID
SetLastAttemptVersionInVariable (
  IN FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private,
  IN UINT32                            LastAttemptVersion
  )
{
  EFI_STATUS            Status;
  FMP_CONTROLLER_STATE  *FmpControllerState;
  BOOLEAN               Update;

  FmpControllerState = GetFmpControllerState (Private);
  if (FmpControllerState == NULL) {
    //
    // Can not update value if FMP Controller State does not exist.
    // This variable is guaranteed to be created by GenerateFmpVariableNames().
    //
    return;
  }

  Update = FALSE;
  if (!FmpControllerState->LastAttemptVersionValid) {
    Update = TRUE;
  }

  if (FmpControllerState->LastAttemptVersion != LastAttemptVersion) {
    Update = TRUE;
  }

  if (!Update) {
    DEBUG ((DEBUG_INFO, "FmpDxe(%s): No need to update controller state.  Same value as before.\n", mImageIdName));
  } else {
    FmpControllerState->LastAttemptVersionValid = TRUE;
    FmpControllerState->LastAttemptVersion      = LastAttemptVersion;
    Status                                      = gRT->SetVariable (
                                                         Private->FmpStateVariableName,
                                                         &gEfiCallerIdGuid,
                                                         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                                                         sizeof (*FmpControllerState),
                                                         FmpControllerState
                                                         );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe(%s): Failed to update controller state.  Status = %r\n", mImageIdName, Status));
    } else {
      DEBUG ((
        DEBUG_INFO,
        "FmpDxe(%s): Set variable %g %s LastAttemptVersion %08x\n",
        mImageIdName,
        &gEfiCallerIdGuid,
        Private->FmpStateVariableName,
        LastAttemptVersion
        ));
    }
  }

  FreePool (FmpControllerState);
}

/**
  Attempts to lock a single UEFI Variable propagating the error state of the
  first lock attempt that fails.  Uses gEfiCallerIdGuid as the variable GUID.

  @param[in] PreviousStatus  The previous UEFI Variable lock attempt status.
  @param[in] VariableLock    The EDK II Variable Lock Protocol instance.
  @param[in] VariableName    The name of the UEFI Variable to lock.

  @retval  EFI_SUCCESS  The UEFI Variable was locked and the previous variable
                        lock attempt also succeeded.
  @retval  Other        The UEFI Variable could not be locked or the previous
                        variable lock attempt failed.
**/
static
EFI_STATUS
LockFmpVariable (
  IN EFI_STATUS                      PreviousStatus,
  IN EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy,
  IN CHAR16                          *VariableName
  )
{
  EFI_STATUS  Status;

  // If success, go ahead and set the policies to protect the target variables.
  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &gEfiCallerIdGuid,
             VariableName,
             VARIABLE_POLICY_NO_MIN_SIZE,
             VARIABLE_POLICY_NO_MAX_SIZE,
             VARIABLE_POLICY_NO_MUST_ATTR,
             VARIABLE_POLICY_NO_CANT_ATTR,
             VARIABLE_POLICY_TYPE_LOCK_NOW
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "FmpDxe(%s): Failed to lock variable %g %s. Status = %r\n",
      mImageIdName,
      &gEfiCallerIdGuid,
      VariableName,
      Status
      ));
  }

  if (EFI_ERROR (PreviousStatus)) {
    return PreviousStatus;
  }

  return Status;
}

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
  FIRMWARE_MANAGEMENT_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                      Status;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;

  // Locate the VariablePolicy protocol.
  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe %a - Could not locate VariablePolicy protocol! %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = EFI_SUCCESS;
  Status = LockFmpVariable (Status, VariablePolicy, Private->VersionVariableName);
  Status = LockFmpVariable (Status, VariablePolicy, Private->LsvVariableName);
  Status = LockFmpVariable (Status, VariablePolicy, Private->LastAttemptStatusVariableName);
  Status = LockFmpVariable (Status, VariablePolicy, Private->LastAttemptVersionVariableName);
  Status = LockFmpVariable (Status, VariablePolicy, Private->FmpStateVariableName);

  return Status;
}

/** @file
  TCG MOR (Memory Overwrite Request) Lock Control support (SMM version).

  This module initilizes MemoryOverwriteRequestControlLock variable.
  This module adds Variable Hook and check MemoryOverwriteRequestControlLock.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/MemoryOverwriteControl.h>
#include <IndustryStandard/MemoryOverwriteRequestControlLock.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include "Variable.h"

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyHelperLib.h>
#include <Library/VariablePolicyLib.h>

typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
} VARIABLE_TYPE;

VARIABLE_TYPE  mMorVariableType[] = {
  { MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,     &gEfiMemoryOverwriteControlDataGuid        },
  { MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME, &gEfiMemoryOverwriteRequestControlLockGuid },
};

BOOLEAN  mMorPassThru = FALSE;

#define MOR_LOCK_DATA_UNLOCKED            0x0
#define MOR_LOCK_DATA_LOCKED_WITHOUT_KEY  0x1
#define MOR_LOCK_DATA_LOCKED_WITH_KEY     0x2

#define MOR_LOCK_V1_SIZE      1
#define MOR_LOCK_V2_KEY_SIZE  8

typedef enum {
  MorLockStateUnlocked = 0,
  MorLockStateLocked   = 1,
} MOR_LOCK_STATE;

BOOLEAN         mMorLockInitializationRequired = FALSE;
UINT8           mMorLockKey[MOR_LOCK_V2_KEY_SIZE];
BOOLEAN         mMorLockKeyEmpty = TRUE;
BOOLEAN         mMorLockPassThru = FALSE;
MOR_LOCK_STATE  mMorLockState    = MorLockStateUnlocked;

/**
  Returns if this is MOR related variable.

  @param  VariableName the name of the vendor's variable, it's a Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.

  @retval  TRUE            The variable is MOR related.
  @retval  FALSE           The variable is NOT MOR related.
**/
BOOLEAN
IsAnyMorVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mMorVariableType)/sizeof (mMorVariableType[0]); Index++) {
    if ((StrCmp (VariableName, mMorVariableType[Index].VariableName) == 0) &&
        (CompareGuid (VendorGuid, mMorVariableType[Index].VendorGuid)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Returns if this is MOR lock variable.

  @param  VariableName the name of the vendor's variable, it's a Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.

  @retval  TRUE            The variable is MOR lock variable.
  @retval  FALSE           The variable is NOT MOR lock variable.
**/
BOOLEAN
IsMorLockVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  if ((StrCmp (VariableName, MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME) == 0) &&
      (CompareGuid (VendorGuid, &gEfiMemoryOverwriteRequestControlLockGuid)))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Set MOR lock variable.

  @param  Data         MOR Lock variable data.

  @retval  EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                  defined by the Attributes.
  @retval  EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the
                                  DataSize exceeds the maximum allowed.
  @retval  EFI_INVALID_PARAMETER  VariableName is an empty Unicode string.
  @retval  EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval  EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval  EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval  EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
                                  set but the AuthInfo does NOT pass the validation check carried
                                  out by the firmware.
  @retval  EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
SetMorLockVariable (
  IN UINT8  Data
  )
{
  EFI_STATUS  Status;

  mMorLockPassThru = TRUE;
  Status           = VariableServiceSetVariable (
                       MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                       &gEfiMemoryOverwriteRequestControlLockGuid,
                       EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                       sizeof (Data),
                       &Data
                       );
  mMorLockPassThru = FALSE;
  return Status;
}

/**
  This service is an MorLock checker handler for the SetVariable().

  @param  VariableName the name of the vendor's variable, as a
                       Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.
  @param  Attributes   Point to memory location to return the attributes of variable. If the point
                       is NULL, the parameter would be ignored.
  @param  DataSize     The size in bytes of Data-Buffer.
  @param  Data         Point to the content of the variable.

  @retval  EFI_SUCCESS            The MorLock check pass, and Variable driver can store the variable data.
  @retval  EFI_INVALID_PARAMETER  The MorLock data or data size or attributes is not allowed.
  @retval  EFI_ACCESS_DENIED      The MorLock is locked.
  @retval  EFI_WRITE_PROTECTED    The MorLock deletion is not allowed.
  @retval  EFI_ALREADY_STARTED    The MorLock variable is handled inside this function.
                                  Variable driver can just return EFI_SUCCESS.
**/
EFI_STATUS
SetVariableCheckHandlerMorLock (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  EFI_STATUS  Status;

  //
  // Basic Check
  //
  if ((Attributes == 0) || (DataSize == 0) || (Data == NULL)) {
    //
    // Permit deletion for passthru request, deny it otherwise.
    //
    return mMorLockPassThru ? EFI_SUCCESS : EFI_WRITE_PROTECTED;
  }

  if ((Attributes != (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)) ||
      ((DataSize != MOR_LOCK_V1_SIZE) && (DataSize != MOR_LOCK_V2_KEY_SIZE)))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do not check if the request is passthru.
  //
  if (mMorLockPassThru) {
    return EFI_SUCCESS;
  }

  if (mMorLockState == MorLockStateUnlocked) {
    //
    // In Unlocked State
    //
    if (DataSize == MOR_LOCK_V1_SIZE) {
      //
      // V1 - lock permanently
      //
      if (*(UINT8 *)Data == MOR_LOCK_DATA_UNLOCKED) {
        //
        // Unlock
        //
        Status = SetMorLockVariable (MOR_LOCK_DATA_UNLOCKED);
        if (!EFI_ERROR (Status)) {
          //
          // return EFI_ALREADY_STARTED to skip variable set.
          //
          return EFI_ALREADY_STARTED;
        } else {
          //
          // SetVar fail
          //
          return Status;
        }
      } else if (*(UINT8 *)Data == MOR_LOCK_DATA_LOCKED_WITHOUT_KEY) {
        //
        // Lock without key
        //
        Status = SetMorLockVariable (MOR_LOCK_DATA_LOCKED_WITHOUT_KEY);
        if (!EFI_ERROR (Status)) {
          //
          // Lock success
          //
          mMorLockState = MorLockStateLocked;
          //
          // return EFI_ALREADY_STARTED to skip variable set.
          //
          return EFI_ALREADY_STARTED;
        } else {
          //
          // SetVar fail
          //
          return Status;
        }
      } else {
        return EFI_INVALID_PARAMETER;
      }
    } else if (DataSize == MOR_LOCK_V2_KEY_SIZE) {
      //
      // V2 lock and provision the key
      //

      //
      // Need set here because the data value on flash is different
      //
      Status = SetMorLockVariable (MOR_LOCK_DATA_LOCKED_WITH_KEY);
      if (EFI_ERROR (Status)) {
        //
        // SetVar fail, do not provision the key
        //
        return Status;
      } else {
        //
        // Lock success, provision the key
        //
        mMorLockKeyEmpty = FALSE;
        CopyMem (mMorLockKey, Data, MOR_LOCK_V2_KEY_SIZE);
        mMorLockState = MorLockStateLocked;
        //
        // return EFI_ALREADY_STARTED to skip variable set.
        //
        return EFI_ALREADY_STARTED;
      }
    } else {
      ASSERT (FALSE);
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // In Locked State
    //
    if (mMorLockKeyEmpty || (DataSize != MOR_LOCK_V2_KEY_SIZE)) {
      return EFI_ACCESS_DENIED;
    }

    if ((CompareMem (Data, mMorLockKey, MOR_LOCK_V2_KEY_SIZE) == 0)) {
      //
      // Key match - unlock
      //

      //
      // Need set here because the data value on flash is different
      //
      Status = SetMorLockVariable (MOR_LOCK_DATA_UNLOCKED);
      if (EFI_ERROR (Status)) {
        //
        // SetVar fail
        //
        return Status;
      } else {
        //
        // Unlock Success
        //
        mMorLockState    = MorLockStateUnlocked;
        mMorLockKeyEmpty = TRUE;
        ZeroMem (mMorLockKey, sizeof (mMorLockKey));
        //
        // return EFI_ALREADY_STARTED to skip variable set.
        //
        return EFI_ALREADY_STARTED;
      }
    } else {
      //
      // Key mismatch - Prevent Dictionary Attack
      //
      mMorLockState    = MorLockStateLocked;
      mMorLockKeyEmpty = TRUE;
      ZeroMem (mMorLockKey, sizeof (mMorLockKey));
      return EFI_ACCESS_DENIED;
    }
  }
}

/**
  This service is an MOR/MorLock checker handler for the SetVariable().

  @param[in]  VariableName the name of the vendor's variable, as a
                           Null-Terminated Unicode String
  @param[in]  VendorGuid   Unify identifier for vendor.
  @param[in]  Attributes   Attributes bitmask to set for the variable.
  @param[in]  DataSize     The size in bytes of Data-Buffer.
  @param[in]  Data         Point to the content of the variable.

  @retval  EFI_SUCCESS            The MOR/MorLock check pass, and Variable
                                  driver can store the variable data.
  @retval  EFI_INVALID_PARAMETER  The MOR/MorLock data or data size or
                                  attributes is not allowed for MOR variable.
  @retval  EFI_ACCESS_DENIED      The MOR/MorLock is locked.
  @retval  EFI_ALREADY_STARTED    The MorLock variable is handled inside this
                                  function. Variable driver can just return
                                  EFI_SUCCESS.
**/
EFI_STATUS
SetVariableCheckHandlerMor (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  //
  // do not handle non-MOR variable
  //
  if (!IsAnyMorVariable (VariableName, VendorGuid)) {
    return EFI_SUCCESS;
  }

  // Permit deletion when policy is disabled.
  if (!IsVariablePolicyEnabled () && ((Attributes == 0) || (DataSize == 0))) {
    return EFI_SUCCESS;
  }

  //
  // MorLock variable
  //
  if (IsMorLockVariable (VariableName, VendorGuid)) {
    return SetVariableCheckHandlerMorLock (
             VariableName,
             VendorGuid,
             Attributes,
             DataSize,
             Data
             );
  }

  //
  // Mor Variable
  //

  //
  // Permit deletion for passthru request.
  //
  if (((Attributes == 0) || (DataSize == 0)) && mMorPassThru) {
    return EFI_SUCCESS;
  }

  //
  // Basic Check
  //
  if ((Attributes != (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)) ||
      (DataSize != sizeof (UINT8)) ||
      (Data == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (mMorLockState == MorLockStateLocked) {
    //
    // If lock, deny access
    //
    return EFI_ACCESS_DENIED;
  }

  //
  // grant access
  //
  return EFI_SUCCESS;
}

/**
  Initialization for MOR Control Lock.

  @retval EFI_SUCCESS     MorLock initialization success.
  @return Others          Some error occurs.
**/
EFI_STATUS
MorLockInit (
  VOID
  )
{
  mMorLockInitializationRequired = TRUE;
  return EFI_SUCCESS;
}

/**
  Delayed initialization for MOR Control Lock at EndOfDxe.

  This function performs any operations queued by MorLockInit().
**/
VOID
MorLockInitAtEndOfDxe (
  VOID
  )
{
  UINTN                  MorSize;
  EFI_STATUS             MorStatus;
  EFI_STATUS             Status;
  VARIABLE_POLICY_ENTRY  *NewPolicy;

  if (!mMorLockInitializationRequired) {
    //
    // The EFI_SMM_FAULT_TOLERANT_WRITE_PROTOCOL has never been installed, thus
    // the variable write service is unavailable. This should never happen.
    //
    ASSERT (FALSE);
    return;
  }

  //
  // Check if the MOR variable exists.
  //
  MorSize   = 0;
  MorStatus = VariableServiceGetVariable (
                MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                &gEfiMemoryOverwriteControlDataGuid,
                NULL,                                   // Attributes
                &MorSize,
                NULL                                    // Data
                );
  //
  // We provided a zero-sized buffer, so the above call can never succeed.
  //
  ASSERT (EFI_ERROR (MorStatus));

  if (MorStatus == EFI_BUFFER_TOO_SMALL) {
    //
    // The MOR variable exists.
    //
    // Some OSes don't follow the TCG's Platform Reset Attack Mitigation spec
    // in that the OS should never create the MOR variable, only read and write
    // it -- these OSes (unintentionally) create MOR if the platform firmware
    // does not produce it. Whether this is the case (from the last OS boot)
    // can be deduced from the absence of the TCG / TCG2 protocols, as edk2's
    // MOR implementation depends on (one of) those protocols.
    //
    if (VariableHaveTcgProtocols ()) {
      //
      // The MOR variable originates from the platform firmware; set the MOR
      // Control Lock variable to report the locking capability to the OS.
      //
      SetMorLockVariable (0);
      return;
    }

    //
    // The MOR variable's origin is inexplicable; delete it.
    //
    DEBUG ((
      DEBUG_WARN,
      "%a: deleting unexpected / unsupported variable %g:%s\n",
      __FUNCTION__,
      &gEfiMemoryOverwriteControlDataGuid,
      MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME
      ));

    mMorPassThru = TRUE;
    VariableServiceSetVariable (
      MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
      &gEfiMemoryOverwriteControlDataGuid,
      0,                                      // Attributes
      0,                                      // DataSize
      NULL                                    // Data
      );
    mMorPassThru = FALSE;
  }

  //
  // The MOR variable is absent; the platform firmware does not support it.
  // Lock the variable so that no other module may create it.
  //
  NewPolicy = NULL;
  Status    = CreateBasicVariablePolicy (
                &gEfiMemoryOverwriteControlDataGuid,
                MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                VARIABLE_POLICY_NO_MIN_SIZE,
                VARIABLE_POLICY_NO_MAX_SIZE,
                VARIABLE_POLICY_NO_MUST_ATTR,
                VARIABLE_POLICY_NO_CANT_ATTR,
                VARIABLE_POLICY_TYPE_LOCK_NOW,
                &NewPolicy
                );
  if (!EFI_ERROR (Status)) {
    Status = RegisterVariablePolicy (NewPolicy);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to lock variable %s! %r\n", __FUNCTION__, MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME, Status));
    ASSERT_EFI_ERROR (Status);
  }

  if (NewPolicy != NULL) {
    FreePool (NewPolicy);
  }

  //
  // Delete the MOR Control Lock variable too (should it exists for some
  // reason) and prevent other modules from creating it.
  //
  mMorLockPassThru = TRUE;
  VariableServiceSetVariable (
    MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
    &gEfiMemoryOverwriteRequestControlLockGuid,
    0,                                          // Attributes
    0,                                          // DataSize
    NULL                                        // Data
    );
  mMorLockPassThru = FALSE;

  NewPolicy = NULL;
  Status    = CreateBasicVariablePolicy (
                &gEfiMemoryOverwriteRequestControlLockGuid,
                MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
                VARIABLE_POLICY_NO_MIN_SIZE,
                VARIABLE_POLICY_NO_MAX_SIZE,
                VARIABLE_POLICY_NO_MUST_ATTR,
                VARIABLE_POLICY_NO_CANT_ATTR,
                VARIABLE_POLICY_TYPE_LOCK_NOW,
                &NewPolicy
                );
  if (!EFI_ERROR (Status)) {
    Status = RegisterVariablePolicy (NewPolicy);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to lock variable %s! %r\n", __FUNCTION__, MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME, Status));
    ASSERT_EFI_ERROR (Status);
  }

  if (NewPolicy != NULL) {
    FreePool (NewPolicy);
  }
}

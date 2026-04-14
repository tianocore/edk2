/** @file
  Base Variable Storage Lib for Smm.


Copyright (c) 2010 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>
Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Protocol/SmmFaultTolerantWrite.h>
#include <Protocol/MmEndOfDxe.h>
#include <Protocol/SmmVarCheck.h>
#include <Protocol/VariableStorage.h>

#include <Library/MmServicesTableLib.h>
#include <Library/VariablePolicyLib.h>

#include "Variable.h"
#include "VariableParsing.h"
#include "VariableRuntimeCache.h"

STATIC BOOLEAN                mAtRuntime                = FALSE;
STATIC BOOLEAN                mVariablePolicyLocked     = FALSE;
STATIC WRITE_READY_NOTIFY_FN  mVariableWriteReadyNotify = NULL;

STATIC EDKII_SMM_VAR_CHECK_PROTOCOL  mSmmVarCheck = {
  VarCheckRegisterSetVariableCheckHandler,
  VarCheckVariablePropertySet,
  VarCheckVariablePropertyGet
};

/**
  Return TRUE if ExitBootServices () has been called.

  @retval TRUE If ExitBootServices () has been called.
**/
BOOLEAN
AtRuntime (
  VOID
  )
{
  return mAtRuntime;
}

/**
  SecureBoot Hook for SetVariable.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.

**/
VOID
EFIAPI
SecureBootHook (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  return;
}

/**
  Initializes variable write service for SMM.

**/
STATIC
VOID
EFIAPI
VariableWriteServiceInitializeSmm (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = VariableWriteServiceInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Variable write service initialization failed. Status = %r\n", Status));
  }

  //
  // Notify the variable wrapper driver the variable write service is ready
  //
  if (mVariableWriteReadyNotify != NULL) {
    mVariableWriteReadyNotify ();
  }
}

/**
  Lock Variable Policy and Reclaim Variable.

  @param[in] EndOfDxe        This function is called at EndOfDxe phase.


**/
STATIC
EFI_STATUS
EFIAPI
VariableLockPolicyAndReclaimVariable (
  BOOLEAN  EndOfDxe
  )
{
  EFI_STATUS  Status;

  if (!mVariablePolicyLocked) {
    MorLockInitAtEndOfDxe ();
    Status = LockVariablePolicy ();
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    mVariablePolicyLocked = TRUE;
    VarCheckLibInitializeAtEndOfDxe (NULL);
    //
    // The initialization for variable quota.
    //
    InitializeVariableQuota ();
  }

  if (!EndOfDxe || PcdGetBool (PcdReclaimVariableSpaceAtEndOfDxe)) {
    ReclaimForOS ();
  }

  return EFI_SUCCESS;
}

/**
  Notification handler after gEfiSmmVariableProtocolGuid is installed.

  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS
  @retval Others     Errors

 **/
STATIC
EFI_STATUS
EFIAPI
VariableProtocolInstallNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  return gMmst->MmInstallProtocolInterface (
                  &Handle,
                  &gEdkiiSmmVarCheckProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mSmmVarCheck
                  );
}

/**
  Retrieve the SMM FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of SMM FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of SMM FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the SMM FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the SMM FVB protocol interface on the handle
  //
  return gMmst->MmHandleProtocol (
                  FvBlockHandle,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  (VOID **)FvBlock
                  );
}

/**
  Retrieve the SMM Fault Tolerant Write protocol interface.

  @param[out] FtwProtocol       The interface of SMM Ftw protocol

  @retval EFI_SUCCESS           The SMM FTW protocol instance was found and returned in FtwProtocol.
  @retval EFI_NOT_FOUND         The SMM FTW protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID  **FtwProtocol
  )
{
  EFI_STATUS  Status;

  //
  // Locate Smm Fault Tolerant Write protocol
  //
  Status = gMmst->MmLocateProtocol (
                    &gEfiSmmFaultTolerantWriteProtocolGuid,
                    NULL,
                    FtwProtocol
                    );
  return Status;
}

/**
  SMM Fault Tolerant Write protocol notification event handler.

  Non-Volatile variable write may needs FTW protocol to reclaim when
  writing variable.

  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   SmmEventCallback runs successfully
  @retval EFI_NOT_FOUND The Fvb protocol for variable is not found.

 **/
EFI_STATUS
EFIAPI
SmmFtwNotificationEvent (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                              Status;
  EFI_PHYSICAL_ADDRESS                    VariableStoreBase;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvbProtocol;
  EFI_SMM_FAULT_TOLERANT_WRITE_PROTOCOL   *FtwProtocol;
  EFI_PHYSICAL_ADDRESS                    NvStorageVariableBase;
  UINTN                                   FtwMaxBlockSize;
  UINT32                                  NvStorageVariableSize;
  UINT64                                  NvStorageVariableSize64;

  if (mVariableModuleGlobal->FvbInstance != NULL) {
    return EFI_SUCCESS;
  }

  //
  // Ensure SMM FTW protocol is installed.
  //
  Status = GetFtwProtocol ((VOID **)&FtwProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetVariableFlashNvStorageInfo (&NvStorageVariableBase, &NvStorageVariableSize64);
  ASSERT_EFI_ERROR (Status);

  Status = SafeUint64ToUint32 (NvStorageVariableSize64, &NvStorageVariableSize);
  // This driver currently assumes the size will be UINT32 so assert the value is safe for now.
  ASSERT_EFI_ERROR (Status);

  ASSERT (NvStorageVariableBase != 0);
  VariableStoreBase = NvStorageVariableBase + mNvFvHeaderCache->HeaderLength;

  Status = FtwProtocol->GetMaxBlockSize (FtwProtocol, &FtwMaxBlockSize);
  if (!EFI_ERROR (Status)) {
    ASSERT (NvStorageVariableSize <= FtwMaxBlockSize);
  }

  //
  // Let NonVolatileVariableBase point to flash variable store base directly after FTW ready.
  //
  mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase = VariableStoreBase;

  //
  // Find the proper FVB protocol for variable.
  //
  Status = GetFvbInfoByAddress (NvStorageVariableBase, NULL, &FvbProtocol);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  mVariableModuleGlobal->FvbInstance = FvbProtocol;

  //
  // Initializes variable write service after FTW was ready.
  //
  VariableWriteServiceInitializeSmm ();

  return EFI_SUCCESS;
}

/**
  Initializes a basic mutual exclusion lock.

  This function initializes a basic mutual exclusion lock to the released state
  and returns the lock.  Each lock provides mutual exclusion access at its task
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.
  If Lock is NULL, then ASSERT().
  If Priority is not a valid TPL value, then ASSERT().

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL is associated with the lock.

  @return The lock.

**/
EFI_LOCK *
InitializeLock (
  IN OUT EFI_LOCK  *Lock,
  IN EFI_TPL       Priority
  )
{
  return Lock;
}

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temporary function that will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtime driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
}

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temporary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtime driver in RT phase.
  It calls EfiReleaseLock() at boot time and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
}

/**
  Function returns an array of handles that support the SMM FVB protocol
  in a buffer allocated from pool.

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support SMM FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No SMM FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN       *NumberHandles,
  OUT EFI_HANDLE  **Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  if ((NumberHandles == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize     = 0;
  *NumberHandles = 0;
  *Buffer        = NULL;
  Status         = gMmst->MmLocateHandle (
                            ByProtocol,
                            &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                            NULL,
                            &BufferSize,
                            *Buffer
                            );
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return EFI_NOT_FOUND;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gMmst->MmLocateHandle (
                    ByProtocol,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    NULL,
                    &BufferSize,
                    *Buffer
                    );

  *NumberHandles = BufferSize / sizeof (EFI_HANDLE);
  if (EFI_ERROR (Status)) {
    *NumberHandles = 0;
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return Status;
}

/**
  Check whether Variable storage use Auth Format.

  @return TRUE    Use Auth format.
  @return FALSE   Not use Auth format.

**/
BOOLEAN
EFIAPI
VariableStorageFvbCheckAuthFormat (
  VOID
  )
{
  return mVariableModuleGlobal->VariableGlobal.AuthFormat;
}

/**
  Get maximum variable size, covering both non-volatile and volatile variables.

  @return Maximum variable size.

**/
UINTN
EFIAPI
VariableStorageFvbGetMaxVariableSize (
  VOID
  )
{
  return GetMaxVariableSize ();
}

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       The buffer to return the contents of the variable. May be NULL
                                    with a zero DataSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     DataSize is NULL.
  @retval EFI_INVALID_PARAMETER     The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR          The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION    The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED           After ExitBootServices() has been called, this return code may be returned
                                    if no variable storage is supported. The platform should describe this
                                    runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                    configuration table.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  )
{
  return VariableServiceGetVariable (
           VariableName,
           VendorGuid,
           Attributes,
           DataSize,
           Data
           );
}

/**

  This code Finds the Next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param VariableNameSize           The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param VariableName               Pointer to variable name.
  @param VendorGuid                 Variable Vendor Guid.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The VariableNameSize is too small for the result.
                                    VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER     VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     The input values of VariableName and VendorGuid are not a name and
                                    GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER     Null-terminator is not found in the first VariableNameSize bytes of
                                    the input VariableName buffer.
  @retval EFI_DEVICE_ERROR          The variable could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED           After ExitBootServices() has been called, this return code may be returned
                                    if no variable storage is supported. The platform should describe this
                                    runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                    configuration table.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbGetNextVariableName (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  return VariableServiceGetNextVariableName (
           VariableNameSize,
           VariableName,
           VendorGuid
           );
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.
  @param FromTrusted                      Whether request comes from trusted.

  @retval EFI_SUCCESS                     The function completed successfully.
  @retval EFI_NOT_FOUND                   The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL            The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER           VariableName is NULL.
  @retval EFI_INVALID_PARAMETER           VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER           DataSize is NULL.
  @retval EFI_INVALID_PARAMETER           The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR                The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION          The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED                 After ExitBootServices() has been called, this return code may be returned
                                          if no variable storage is supported. The platform should describe this
                                          runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                          configuration table.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data,
  IN BOOLEAN   FromTrusted
  )
{
  EFI_STATUS  Status;

  //
  // Disable write protection when the calling SetVariable() through EFI_SMM_VARIABLE_PROTOCOL.
  //
  if (FromTrusted) {
    mRequestSource = VarCheckFromTrusted;
  }

  Status = VariableServiceSetVariable (
             VariableName,
             VendorGuid,
             Attributes,
             DataSize,
             Data
             );

  if (FromTrusted) {
    mRequestSource = VarCheckFromUntrusted;
  }

  return Status;
}

/**
  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbQueryVariableInfo (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  return VariableServiceQueryVariableInfo (
           Attributes,
           MaximumVariableStorageSize,
           RemainingVariableStorageSize,
           MaximumVariableSize
           );
}

/**
  Callback function at End of DXE phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbEndOfDxe (
  IN VOID
  )
{
  mEndOfDxe = TRUE;
  return VariableLockPolicyAndReclaimVariable (TRUE);
}

/**
  Callback function at READY_TO_BOOT phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbReadyToBoot (
  IN VOID
  )
{
  if (mAtRuntime) {
    return EFI_UNSUPPORTED;
  }

  if (!mEndOfDxe) {
    mEndOfDxe = TRUE;
  }

  return VariableLockPolicyAndReclaimVariable (FALSE);
}

/**
  Callback function for EXIT_BOOT phase notification.

  @return EFI_SUCCESS
  @return Others                   ERROR.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbExitBootService (
  IN VOID
  )
{
  mAtRuntime = TRUE;
  return EFI_SUCCESS;
}

/**
  Get the variable statistics information from the information buffer pointed by gVariableInfo.

  Caution: This function may be invoked at SMM runtime.
  InfoEntry and InfoSize are external input. Care must be taken to make sure not security issue at runtime.

  @param[in, out]  InfoEntry    A pointer to the buffer of variable information entry.
                                On input, point to the variable information returned last time. if
                                InfoEntry->VendorGuid is zero, return the first information.
                                On output, point to the next variable information.
  @param[in, out]  InfoSize     On input, the size of the variable information buffer.
                                On output, the returned variable information size.

  @retval EFI_SUCCESS           The variable information is found and returned successfully.
  @retval EFI_UNSUPPORTED       No variable information exists in variable driver.
  @retval EFI_BUFFER_TOO_SMALL  The buffer is too small to hold the next variable information.
  @retval EFI_INVALID_PARAMETER Input parameter is invalid.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbGetStatics (
  IN OUT VARIABLE_INFO_ENTRY  *InfoEntry,
  IN OUT UINTN                *InfoSize
  )
{
  VARIABLE_INFO_ENTRY  *VariableInfo;
  UINTN                NameSize;
  UINTN                StatisticsInfoSize;
  CHAR16               *InfoName;
  UINTN                InfoNameMaxSize;
  EFI_GUID             VendorGuid;

  if (InfoEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableInfo = gVariableInfo;
  if (VariableInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  StatisticsInfoSize = sizeof (VARIABLE_INFO_ENTRY);
  if (*InfoSize < StatisticsInfoSize) {
    *InfoSize = StatisticsInfoSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  InfoName        = (CHAR16 *)(InfoEntry + 1);
  InfoNameMaxSize = (*InfoSize - sizeof (VARIABLE_INFO_ENTRY));

  CopyGuid (&VendorGuid, &InfoEntry->VendorGuid);

  if (IsZeroGuid (&VendorGuid)) {
    //
    // Return the first variable info
    //
    NameSize           = StrSize (VariableInfo->Name);
    StatisticsInfoSize = sizeof (VARIABLE_INFO_ENTRY) + NameSize;
    if (*InfoSize < StatisticsInfoSize) {
      *InfoSize = StatisticsInfoSize;
      return EFI_BUFFER_TOO_SMALL;
    }

    CopyMem (InfoEntry, VariableInfo, sizeof (VARIABLE_INFO_ENTRY));
    CopyMem (InfoName, VariableInfo->Name, NameSize);
    *InfoSize = StatisticsInfoSize;
    return EFI_SUCCESS;
  }

  //
  // Get the next variable info
  //
  while (VariableInfo != NULL) {
    if (CompareGuid (&VariableInfo->VendorGuid, &VendorGuid)) {
      NameSize = StrSize (VariableInfo->Name);
      if (NameSize <= InfoNameMaxSize) {
        if (CompareMem (VariableInfo->Name, InfoName, NameSize) == 0) {
          //
          // Find the match one
          //
          VariableInfo = VariableInfo->Next;
          break;
        }
      }
    }

    VariableInfo = VariableInfo->Next;
  }

  if (VariableInfo == NULL) {
    *InfoSize = 0;
    return EFI_SUCCESS;
  }

  //
  // Output the new variable info
  //
  NameSize           = StrSize (VariableInfo->Name);
  StatisticsInfoSize = sizeof (VARIABLE_INFO_ENTRY) + NameSize;
  if (*InfoSize < StatisticsInfoSize) {
    *InfoSize = StatisticsInfoSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (InfoEntry, VariableInfo, sizeof (VARIABLE_INFO_ENTRY));
  CopyMem (InfoName, VariableInfo->Name, NameSize);
  *InfoSize = StatisticsInfoSize;

  return EFI_SUCCESS;
}

/**
  Mark a variable that will become read-only after leaving the DXE phase of execution.

  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableStorageFvbRequestToLock (
  IN       CHAR16    *VariableName,
  IN       EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS  Status;

  if (mEndOfDxe) {
    Status = EFI_ACCESS_DENIED;
  } else {
    Status = VariableLockRequestToLock (
               NULL,
               VariableName,
               VendorGuid
               );
  }

  return Status;
}

/**
  Variable property set.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string,
                                or the fields of VariableProperty are not valid.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbPropertySet (
  IN CHAR16                       *Name,
  IN EFI_GUID                     *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  )
{
  return VarCheckVariablePropertySet (
           Name,
           Guid,
           VariableProperty
           );
}

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbPropertyGet (
  IN CHAR16                        *Name,
  IN EFI_GUID                      *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  )
{
  return VarCheckVariablePropertyGet (
           Name,
           Guid,
           VariableProperty
           );
}

/**
  Initialise Variable Runtime Cache.

  @param[in]   HobCache             Address of cache for variables in HobList.
  @param[in]   VolatileCache        Address of cache for variables in  volatile memory.
  @param[in]   NvCache              Address of cache for variables in  in non-volatile memory.
  @param[in]   PendingUpdate        Address to bool to check pending update.
  @param[in]   ReadLock             Address to bool to check read operation is locked.
  @param[in]   HobFlushComplete     Address to bool to check flush hob is completed.

  @return      EFI_SUCCESS
  @return      EFI_UNSUPPORTED      Runtime variable cache isn't supported
  @return      Others               Errors

**/
EFI_STATUS
EFIAPI
VariableStorageFvbInitCache (
  IN  VARIABLE_STORE_HEADER  *HobCache,
  IN  VARIABLE_STORE_HEADER  *VolatileCache,
  IN  VARIABLE_STORE_HEADER  *NvCache,
  IN  BOOLEAN                *PendingUpdate,
  IN  BOOLEAN                *ReadLock,
  IN  BOOLEAN                *HobFlushComplete
  )
{
  VARIABLE_STORE_HEADER           *VariableCache;
  VARIABLE_RUNTIME_CACHE_CONTEXT  *VariableCacheContext;

  VariableCacheContext = &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext;

  VariableCacheContext->VariableRuntimeHobCache.Store      = HobCache;
  VariableCacheContext->VariableRuntimeVolatileCache.Store = VolatileCache;
  VariableCacheContext->VariableRuntimeNvCache.Store       = NvCache;
  VariableCacheContext->PendingUpdate                      = PendingUpdate;
  VariableCacheContext->ReadLock                           = ReadLock;
  VariableCacheContext->HobFlushComplete                   = HobFlushComplete;

  // Set up the initial pending request since the RT cache needs to be in sync with SMM cache
  VariableCacheContext->VariableRuntimeHobCache.PendingUpdateOffset = 0;
  VariableCacheContext->VariableRuntimeHobCache.PendingUpdateLength = 0;

  if ((mVariableModuleGlobal->VariableGlobal.HobVariableBase > 0) &&
      (VariableCacheContext->VariableRuntimeHobCache.Store != NULL))
  {
    VariableCache                                                     = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
    VariableCacheContext->VariableRuntimeHobCache.PendingUpdateLength = (UINT32)((UINTN)GetEndPointer (VariableCache) - (UINTN)VariableCache);
    CopyGuid (&(VariableCacheContext->VariableRuntimeHobCache.Store->Signature), &(VariableCache->Signature));
  }

  VariableCache                                                          = (VARIABLE_STORE_HEADER  *)(UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  VariableCacheContext->VariableRuntimeVolatileCache.PendingUpdateOffset = 0;
  VariableCacheContext->VariableRuntimeVolatileCache.PendingUpdateLength = (UINT32)((UINTN)GetEndPointer (VariableCache) - (UINTN)VariableCache);
  CopyGuid (&(VariableCacheContext->VariableRuntimeVolatileCache.Store->Signature), &(VariableCache->Signature));

  VariableCache                                                    = (VARIABLE_STORE_HEADER  *)(UINTN)mNvVariableCache;
  VariableCacheContext->VariableRuntimeNvCache.PendingUpdateOffset = 0;
  VariableCacheContext->VariableRuntimeNvCache.PendingUpdateLength = (UINT32)((UINTN)GetEndPointer (VariableCache) - (UINTN)VariableCache);
  CopyGuid (&(VariableCacheContext->VariableRuntimeNvCache.Store->Signature), &(VariableCache->Signature));

  *(VariableCacheContext->PendingUpdate)    = TRUE;
  *(VariableCacheContext->ReadLock)         = FALSE;
  *(VariableCacheContext->HobFlushComplete) = FALSE;

  return EFI_SUCCESS;
}

/**
  Copies any pending updates to runtime variable caches.

  @retval EFI_SUCCESS             The cache store was updated successfully.
  @retval EFI_UNSUPPORTED         The cache store to be updated is not initialized properly.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbSyncCache (
  VOID
  )
{
  return FlushPendingRuntimeVariableCacheUpdates ();
}

/**
  Get runtime variable caches info.

  @param[out]    HobCacheSize                  Cache size of variables in Hob list.
  @param[out]    VolatileCacheSize             Cache size of variables in volatile memory.
  @param[out]    NvCacheSize                   Cache size of variables in non-volatile memory.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
VariableStorageFvbGetCacheInfo (
  OUT UINTN  *HobCacheSize,
  OUT UINTN  *VolatileCacheSize,
  OUT UINTN  *NvCacheSize
  )
{
  VARIABLE_STORE_HEADER  *VariableCache;

  if (mVariableModuleGlobal->VariableGlobal.HobVariableBase > 0) {
    VariableCache = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
    *HobCacheSize = VariableCache->Size;
  } else {
    *HobCacheSize = 0;
  }

  VariableCache      = (VARIABLE_STORE_HEADER  *)(UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  *VolatileCacheSize = VariableCache->Size;

  VariableCache = (VARIABLE_STORE_HEADER  *)(UINTN)mNvVariableCache;
  *NvCacheSize  = (UINTN)VariableCache->Size;

  return EFI_SUCCESS;
}

/**
  Initialise Variable Write Service.
  This function should install gSmmVariableWriteGuid or register notifier to install

  @param[in] WriteReadyNotify   Notifier to be called when variables are
                                writable.

  @return EFI_SUCCESS
  @return Others                Failed to initialise write service.

**/
EFI_STATUS
EFIAPI
VariableStorageFvbInitWriteService (
  IN WRITE_READY_NOTIFY_FN  WriteReadyNotify
  )
{
  EFI_STATUS  Status;
  VOID        *SmmFtwRegistration;

  mVariableWriteReadyNotify = WriteReadyNotify;

  if (!PcdGetBool (PcdEmuVariableNvModeEnable)) {
    //
    // Register FtwNotificationEvent () notify function.
    //
    Status = gMmst->MmRegisterProtocolNotify (
                      &gEfiSmmFaultTolerantWriteProtocolGuid,
                      SmmFtwNotificationEvent,
                      &SmmFtwRegistration
                      );
    ASSERT_EFI_ERROR (Status);

    SmmFtwNotificationEvent (NULL, NULL, NULL);
  } else {
    //
    // Emulated non-volatile variable mode does not depend on FVB and FTW.
    //
    VariableWriteServiceInitializeSmm ();
    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC EDKII_VARIABLE_STORAGE_PROTOCOL  mSmmVariableStorageFvb = {
  VariableStorageFvbCheckAuthFormat,
  VariableStorageFvbGetMaxVariableSize,
  VariableStorageFvbGetVariable,
  VariableStorageFvbGetNextVariableName,
  VariableStorageFvbSetVariable,
  VariableStorageFvbQueryVariableInfo,
  VariableStorageFvbEndOfDxe,
  VariableStorageFvbReadyToBoot,
  VariableStorageFvbExitBootService,
  VariableStorageFvbGetStatics,
  VariableStorageFvbRequestToLock,
  VariableStorageFvbPropertySet,
  VariableStorageFvbPropertyGet,
  VariableStorageFvbInitCache,
  VariableStorageFvbSyncCache,
  VariableStorageFvbGetCacheInfo,
  VariableStorageFvbInitWriteService,
};

/**
  Initailize Variable Stoarge.

  @param  [in]  ImageHandle      The firmware allocated handle for the EFI image

  @retval EFI_SUCCESS            Success
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
VariableStorageFvbInitialize (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  //
  // Variable initialize.
  //
  Status = VariableCommonInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // Install Notification for gEfiSmmVariableProtocolGuid,
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiSmmVariableProtocolGuid,
                    VariableProtocolInstallNotify,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Install Variable Storage Protocol.
  //
  Status = gMmst->MmInstallProtocolInterface (
                    &ImageHandle,
                    &gEdkiiVariableStorageProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSmmVariableStorageFvb
                    );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

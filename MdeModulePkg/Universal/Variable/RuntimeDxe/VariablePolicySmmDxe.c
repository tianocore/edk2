/** @file -- VariablePolicySmmDxe.c
This protocol allows communication with Variable Policy Engine.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/SafeIntLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariablePolicy.h>
#include <Protocol/MmCommunication2.h>

#include <Guid/VarCheckPolicyMmi.h>

#include "Variable.h"

EDKII_VARIABLE_POLICY_PROTOCOL  mVariablePolicyProtocol;
EFI_MM_COMMUNICATION2_PROTOCOL  *mMmCommunication;

VOID      *mMmCommunicationBuffer;
UINTN     mMmCommunicationBufferSize;
EFI_LOCK  mMmCommunicationLock;

/**
  Internal helper function to consolidate communication method.

  @param[in,out]  CommBuffer
  @param[in,out]  CommSize    Size of the CommBuffer.

  @retval   EFI_STATUS    Result from communication method.

**/
STATIC
EFI_STATUS
InternalMmCommunicate (
  IN OUT VOID   *CommBuffer,
  IN OUT UINTN  *CommSize
  )
{
  EFI_STATUS  Status;

  if ((CommBuffer == NULL) || (CommSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = mMmCommunication->Communicate (mMmCommunication, CommBuffer, CommBuffer, CommSize);
  return Status;
}

/**
  This API function disables the variable policy enforcement. If it's
  already been called once, will return EFI_ALREADY_STARTED.

  @retval     EFI_SUCCESS
  @retval     EFI_ALREADY_STARTED   Has already been called once this boot.
  @retval     EFI_WRITE_PROTECTED   Interface has been locked until reboot.
  @retval     EFI_WRITE_PROTECTED   Interface option is disabled by platform PCD.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolDisableVariablePolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER     *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER  *PolicyHeader;
  UINTN                         BufferSize;

  // Check the PCD for convenience.
  // This would also be rejected by the lib, but why go to MM if we don't have to?
  if (!PcdGetBool (PcdAllowVariablePolicyEnforcementDisable)) {
    return EFI_WRITE_PROTECTED;
  }

  AcquireLockOnlyAtBootTime (&mMmCommunicationLock);

  // Set up the MM communication.
  BufferSize   = mMmCommunicationBufferSize;
  CommHeader   = mMmCommunicationBuffer;
  PolicyHeader = (VAR_CHECK_POLICY_COMM_HEADER *)&CommHeader->Data;
  CopyGuid (&CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid);
  CommHeader->MessageLength = BufferSize - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_DISABLE;

  Status = InternalMmCommunicate (CommHeader, &BufferSize);
  DEBUG ((DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status));

  ReleaseLockOnlyAtBootTime (&mMmCommunicationLock);

  return (EFI_ERROR (Status)) ? Status : PolicyHeader->Result;
}

/**
  This API function returns whether or not the policy engine is
  currently being enforced.

  @param[out]   State       Pointer to a return value for whether the policy enforcement
                            is currently enabled.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolIsVariablePolicyEnabled (
  OUT BOOLEAN  *State
  )
{
  EFI_STATUS                               Status;
  EFI_MM_COMMUNICATE_HEADER                *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER             *PolicyHeader;
  VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS  *CommandParams;
  UINTN                                    BufferSize;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mMmCommunicationLock);

  // Set up the MM communication.
  BufferSize    = mMmCommunicationBufferSize;
  CommHeader    = mMmCommunicationBuffer;
  PolicyHeader  = (VAR_CHECK_POLICY_COMM_HEADER *)&CommHeader->Data;
  CommandParams = (VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS *)(PolicyHeader + 1);
  CopyGuid (&CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid);
  CommHeader->MessageLength = BufferSize - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_IS_ENABLED;

  Status = InternalMmCommunicate (CommHeader, &BufferSize);
  DEBUG ((DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status));

  if (!EFI_ERROR (Status)) {
    Status = PolicyHeader->Result;
    *State = CommandParams->State;
  }

  ReleaseLockOnlyAtBootTime (&mMmCommunicationLock);

  return Status;
}

/**
  This API function validates and registers a new policy with
  the policy enforcement engine.

  @param[in]  NewPolicy     Pointer to the incoming policy structure.

  @retval     EFI_SUCCESS
  @retval     EFI_INVALID_PARAMETER   NewPolicy is NULL or is internally inconsistent.
  @retval     EFI_ALREADY_STARTED     An identical matching policy already exists.
  @retval     EFI_WRITE_PROTECTED     The interface has been locked until the next reboot.
  @retval     EFI_UNSUPPORTED         Policy enforcement has been disabled. No reason to add more policies.
  @retval     EFI_ABORTED             A calculation error has prevented this function from completing.
  @retval     EFI_OUT_OF_RESOURCES    Cannot grow the table to hold any more policies.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolRegisterVariablePolicy (
  IN CONST VARIABLE_POLICY_ENTRY  *NewPolicy
  )
{
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER     *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER  *PolicyHeader;
  VOID                          *PolicyBuffer;
  UINTN                         BufferSize;
  UINTN                         RequiredSize;

  if (NewPolicy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // First, make sure that the required size does not exceed the capabilities
  // of the MmCommunication buffer.
  RequiredSize = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + sizeof (VAR_CHECK_POLICY_COMM_HEADER);
  Status       = SafeUintnAdd (RequiredSize, NewPolicy->Size, &RequiredSize);
  if (EFI_ERROR (Status) || (RequiredSize > mMmCommunicationBufferSize)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - Policy too large for buffer! %r, %d > %d \n",
      __FUNCTION__,
      Status,
      RequiredSize,
      mMmCommunicationBufferSize
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  AcquireLockOnlyAtBootTime (&mMmCommunicationLock);

  // Set up the MM communication.
  BufferSize   = mMmCommunicationBufferSize;
  CommHeader   = mMmCommunicationBuffer;
  PolicyHeader = (VAR_CHECK_POLICY_COMM_HEADER *)&CommHeader->Data;
  PolicyBuffer = (VOID *)(PolicyHeader + 1);
  CopyGuid (&CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid);
  CommHeader->MessageLength = BufferSize - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_REGISTER;

  // Copy the policy into place. This copy is safe because we've already tested above.
  CopyMem (PolicyBuffer, NewPolicy, NewPolicy->Size);

  Status = InternalMmCommunicate (CommHeader, &BufferSize);
  DEBUG ((DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status));

  ReleaseLockOnlyAtBootTime (&mMmCommunicationLock);

  return (EFI_ERROR (Status)) ? Status : PolicyHeader->Result;
}

/**
  This helper function takes care of the overhead of formatting, sending, and interpreting
  the results for a single DumpVariablePolicy request.

  @param[in]      PageRequested   The page of the paginated results from MM. 0 for metadata.
  @param[out]     TotalSize       The total size of the entire buffer. Returned as part of metadata.
  @param[out]     PageSize        The size of the current page being returned. Not valid as part of metadata.
  @param[out]     HasMore         A flag indicating whether there are more pages after this one.
  @param[out]     Buffer          The start of the current page from MM.

  @retval     EFI_SUCCESS             Output params have been updated (either metadata or dump page).
  @retval     EFI_INVALID_PARAMETER   One of the output params is NULL.
  @retval     Others                  Response from MM handler.

**/
STATIC
EFI_STATUS
DumpVariablePolicyHelper (
  IN  UINT32   PageRequested,
  OUT UINT32   *TotalSize,
  OUT UINT32   *PageSize,
  OUT BOOLEAN  *HasMore,
  OUT UINT8    **Buffer
  )
{
  EFI_STATUS                         Status;
  EFI_MM_COMMUNICATE_HEADER          *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER       *PolicyHeader;
  VAR_CHECK_POLICY_COMM_DUMP_PARAMS  *CommandParams;
  UINTN                              BufferSize;

  if ((TotalSize == NULL) || (PageSize == NULL) || (HasMore == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Set up the MM communication.
  BufferSize    = mMmCommunicationBufferSize;
  CommHeader    = mMmCommunicationBuffer;
  PolicyHeader  = (VAR_CHECK_POLICY_COMM_HEADER *)&CommHeader->Data;
  CommandParams = (VAR_CHECK_POLICY_COMM_DUMP_PARAMS *)(PolicyHeader + 1);
  CopyGuid (&CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid);
  CommHeader->MessageLength = BufferSize - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_DUMP;

  CommandParams->PageRequested = PageRequested;

  Status = InternalMmCommunicate (CommHeader, &BufferSize);
  DEBUG ((DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status));

  if (!EFI_ERROR (Status)) {
    Status     = PolicyHeader->Result;
    *TotalSize = CommandParams->TotalSize;
    *PageSize  = CommandParams->PageSize;
    *HasMore   = CommandParams->HasMore;
    *Buffer    = (UINT8 *)(CommandParams + 1);
  }

  return Status;
}

/**
  This API function will dump the entire contents of the variable policy table.

  Similar to GetVariable, the first call can be made with a 0 size and it will return
  the size of the buffer required to hold the entire table.

  @param[out]     Policy  Pointer to the policy buffer. Can be NULL if Size is 0.
  @param[in,out]  Size    On input, the size of the output buffer. On output, the size
                          of the data returned.

  @retval     EFI_SUCCESS             Policy data is in the output buffer and Size has been updated.
  @retval     EFI_INVALID_PARAMETER   Size is NULL, or Size is non-zero and Policy is NULL.
  @retval     EFI_BUFFER_TOO_SMALL    Size is insufficient to hold policy. Size updated with required size.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolDumpVariablePolicy (
  OUT UINT8      *Policy OPTIONAL,
  IN OUT UINT32  *Size
  )
{
  EFI_STATUS  Status;
  UINT8       *Source;
  UINT8       *Destination;
  UINT32      PolicySize;
  UINT32      PageSize;
  BOOLEAN     HasMore;
  UINT32      PageIndex;

  if ((Size == NULL) || ((*Size > 0) && (Policy == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mMmCommunicationLock);

  // Repeat this whole process until we either have a failure case or get the entire buffer.
  do {
    // First, we must check the zero page to determine the buffer size and
    // reset the internal state.
    PolicySize = 0;
    PageSize   = 0;
    HasMore    = FALSE;
    Status     = DumpVariablePolicyHelper (0, &PolicySize, &PageSize, &HasMore, &Source);
    if (EFI_ERROR (Status)) {
      break;
    }

    // If we're good, we can at least check the required size now.
    if (*Size < PolicySize) {
      *Size  = PolicySize;
      Status = EFI_BUFFER_TOO_SMALL;
      break;
    }

    // On further thought, let's update the size either way.
    *Size = PolicySize;
    // And get ready to ROCK.
    Destination = Policy;

    // Keep looping and copying until we're either done or freak out.
    for (PageIndex = 1; !EFI_ERROR (Status) && HasMore && PageIndex < MAX_UINT32; PageIndex++) {
      Status = DumpVariablePolicyHelper (PageIndex, &PolicySize, &PageSize, &HasMore, &Source);
      if (!EFI_ERROR (Status)) {
        CopyMem (Destination, Source, PageSize);
        Destination += PageSize;
      }
    }

    // Next, we check to see whether
  } while (Status == EFI_TIMEOUT);

  ReleaseLockOnlyAtBootTime (&mMmCommunicationLock);

  // There's currently no use for this, but it shouldn't be hard to implement.
  return Status;
}

/**
  This API function locks the interface so that no more policy updates
  can be performed or changes made to the enforcement until the next boot.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
STATIC
EFI_STATUS
EFIAPI
ProtocolLockVariablePolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER     *CommHeader;
  VAR_CHECK_POLICY_COMM_HEADER  *PolicyHeader;
  UINTN                         BufferSize;

  AcquireLockOnlyAtBootTime (&mMmCommunicationLock);

  // Set up the MM communication.
  BufferSize   = mMmCommunicationBufferSize;
  CommHeader   = mMmCommunicationBuffer;
  PolicyHeader = (VAR_CHECK_POLICY_COMM_HEADER *)&CommHeader->Data;
  CopyGuid (&CommHeader->HeaderGuid, &gVarCheckPolicyLibMmiHandlerGuid);
  CommHeader->MessageLength = BufferSize - OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
  PolicyHeader->Signature   = VAR_CHECK_POLICY_COMM_SIG;
  PolicyHeader->Revision    = VAR_CHECK_POLICY_COMM_REVISION;
  PolicyHeader->Command     = VAR_CHECK_POLICY_COMMAND_LOCK;

  Status = InternalMmCommunicate (CommHeader, &BufferSize);
  DEBUG ((DEBUG_VERBOSE, "%a - MmCommunication returned %r.\n", __FUNCTION__, Status));

  ReleaseLockOnlyAtBootTime (&mMmCommunicationLock);

  return (EFI_ERROR (Status)) ? Status : PolicyHeader->Result;
}

/**
  This helper function locates the shared comm buffer and assigns it to input pointers.

  @param[in,out]  BufferSize      On input, the minimum buffer size required INCLUDING the MM communicate header.
                                  On output, the size of the matching buffer found.
  @param[out]     LocatedBuffer   A pointer to the matching buffer.

  @retval     EFI_SUCCESS
  @retval     EFI_INVALID_PARAMETER   One of the output pointers was NULL.
  @retval     EFI_OUT_OF_RESOURCES    Not enough memory to allocate a comm buffer.

**/
STATIC
EFI_STATUS
InitMmCommonCommBuffer (
  IN OUT  UINTN  *BufferSize,
  OUT     VOID   **LocatedBuffer
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  // Make sure that we're working with good pointers.
  if ((BufferSize == NULL) || (LocatedBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Allocate the runtime memory for the comm buffer.
  *LocatedBuffer = AllocateRuntimePool (*BufferSize);
  if (*LocatedBuffer == NULL) {
    Status      = EFI_OUT_OF_RESOURCES;
    *BufferSize = 0;
  }

  EfiInitializeLock (&mMmCommunicationLock, TPL_NOTIFY);

  return Status;
}

/**
  Convert internal pointer addresses to virtual addresses.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
STATIC
VOID
EFIAPI
VariablePolicyVirtualAddressCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EfiConvertPointer (0, (VOID **)&mMmCommunication);
  EfiConvertPointer (0, (VOID **)&mMmCommunicationBuffer);
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
VariablePolicySmmDxeMain (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  BOOLEAN     ProtocolInstalled;
  BOOLEAN     VirtualAddressChangeRegistered;
  EFI_EVENT   VirtualAddressChangeEvent;

  Status                         = EFI_SUCCESS;
  ProtocolInstalled              = FALSE;
  VirtualAddressChangeRegistered = FALSE;

  // Update the minimum buffer size.
  mMmCommunicationBufferSize = VAR_CHECK_POLICY_MM_COMM_BUFFER_SIZE;
  // Locate the shared comm buffer to use for sending MM commands.
  Status = InitMmCommonCommBuffer (&mMmCommunicationBufferSize, &mMmCommunicationBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to locate a viable MM comm buffer! %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Locate the MmCommunication protocol.
  Status = gBS->LocateProtocol (&gEfiMmCommunication2ProtocolGuid, NULL, (VOID **)&mMmCommunication);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to locate MmCommunication protocol! %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Configure the VariablePolicy protocol structure.
  mVariablePolicyProtocol.Revision                = EDKII_VARIABLE_POLICY_PROTOCOL_REVISION;
  mVariablePolicyProtocol.DisableVariablePolicy   = ProtocolDisableVariablePolicy;
  mVariablePolicyProtocol.IsVariablePolicyEnabled = ProtocolIsVariablePolicyEnabled;
  mVariablePolicyProtocol.RegisterVariablePolicy  = ProtocolRegisterVariablePolicy;
  mVariablePolicyProtocol.DumpVariablePolicy      = ProtocolDumpVariablePolicy;
  mVariablePolicyProtocol.LockVariablePolicy      = ProtocolLockVariablePolicy;

  // Register all the protocols and return the status.
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEdkiiVariablePolicyProtocolGuid,
                  &mVariablePolicyProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to install protocol! %r\n", __FUNCTION__, Status));
    goto Exit;
  } else {
    ProtocolInstalled = TRUE;
  }

  // Normally, we might want to register a callback
  // to lock the interface, but this is integrated
  // into the existing callbacks in VaraiableSmm.c
  // and VariableDxe.c.

  //
  // Register a VirtualAddressChange callback for the MmComm protocol and Comm buffer.
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VariablePolicyVirtualAddressCallback,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &VirtualAddressChangeEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to create VirtualAddressChange event! %r\n", __FUNCTION__, Status));
    goto Exit;
  } else {
    VirtualAddressChangeRegistered = TRUE;
  }

Exit:
  //
  // If we're about to return a failed status (and unload this driver), we must first undo anything that
  // has been successfully done.
  if (EFI_ERROR (Status)) {
    if (ProtocolInstalled) {
      gBS->UninstallProtocolInterface (&ImageHandle, &gEdkiiVariablePolicyProtocolGuid, &mVariablePolicyProtocol);
    }

    if (VirtualAddressChangeRegistered) {
      gBS->CloseEvent (VirtualAddressChangeEvent);
    }
  }

  return Status;
}

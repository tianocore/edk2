/** @file

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LockBoxLib.h>
#include <Library/DebugLib.h>
#include <Guid/SmmLockBox.h>
#include <Guid/EndOfS3Resume.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmEndOfDxe.h>
#include <Protocol/SmmSxDispatch2.h>

#include "SmmLockBoxLibPrivate.h"

/**
  We need handle this library carefully. Only one library instance will construct the environment.
  Below 2 global variable can only be used in constructor. They should NOT be used in any other library functions.
**/
SMM_LOCK_BOX_CONTEXT mSmmLockBoxContext;
LIST_ENTRY           mLockBoxQueue = INITIALIZE_LIST_HEAD_VARIABLE (mLockBoxQueue);

BOOLEAN              mSmmConfigurationTableInstalled = FALSE;
VOID                 *mSmmLockBoxRegistrationSmmEndOfDxe = NULL;
VOID                 *mSmmLockBoxRegistrationSmmReadyToLock = NULL;
VOID                 *mSmmLockBoxRegistrationEndOfS3Resume = NULL;
BOOLEAN              mSmmLockBoxSmmReadyToLock = FALSE;
BOOLEAN              mSmmLockBoxDuringS3Resume = FALSE;

/**
  This function return SmmLockBox context from SMST.

  @return SmmLockBox context from SMST.
**/
SMM_LOCK_BOX_CONTEXT *
InternalGetSmmLockBoxContext (
  VOID
  )
{
  UINTN   Index;

  //
  // Check if gEfiSmmLockBoxCommunicationGuid is installed by someone
  //
  for (Index = 0; Index < gSmst->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gSmst->SmmConfigurationTable[Index].VendorGuid, &gEfiSmmLockBoxCommunicationGuid)) {
      //
      // Found. That means some other library instance is already run.
      // No need to install again, just return.
      //
      return (SMM_LOCK_BOX_CONTEXT *)gSmst->SmmConfigurationTable[Index].VendorTable;
    }
  }

  //
  // Not found.
  //
  return NULL;
}

/**
  Notification for SMM ReadyToLock protocol.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification runs successfully.
**/
EFI_STATUS
EFIAPI
SmmLockBoxSmmReadyToLockNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  mSmmLockBoxSmmReadyToLock = TRUE;
  return EFI_SUCCESS;
}

/**
  Main entry point for an SMM handler dispatch or communicate-based callback.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
EFI_STATUS
EFIAPI
SmmLockBoxS3EntryCallBack (
  IN           EFI_HANDLE           DispatchHandle,
  IN     CONST VOID                 *Context         OPTIONAL,
  IN OUT       VOID                 *CommBuffer      OPTIONAL,
  IN OUT       UINTN                *CommBufferSize  OPTIONAL
  )
{
  mSmmLockBoxDuringS3Resume = TRUE;
  return EFI_SUCCESS;
}

/**
  Notification for SMM EndOfDxe protocol.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification runs successfully.
**/
EFI_STATUS
EFIAPI
SmmLockBoxSmmEndOfDxeNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_SX_DISPATCH2_PROTOCOL     *SxDispatch;
  EFI_SMM_SX_REGISTER_CONTEXT       EntryRegisterContext;
  EFI_HANDLE                        S3EntryHandle;

  //
  // Locate SmmSxDispatch2 protocol.
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSxDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&SxDispatch
                    );
  if (!EFI_ERROR (Status) && (SxDispatch != NULL)) {
    //
    // Register a S3 entry callback function to
    // determine if it will be during S3 resume.
    //
    EntryRegisterContext.Type  = SxS3;
    EntryRegisterContext.Phase = SxEntry;
    Status = SxDispatch->Register (
                           SxDispatch,
                           SmmLockBoxS3EntryCallBack,
                           &EntryRegisterContext,
                           &S3EntryHandle
                           );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Notification for SMM EndOfS3Resume protocol.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification runs successfully.
**/
EFI_STATUS
EFIAPI
SmmLockBoxEndOfS3ResumeNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  mSmmLockBoxDuringS3Resume = FALSE;
  return EFI_SUCCESS;
}

/**
  Constructor for SmmLockBox library.
  This is used to set SmmLockBox context, which will be used in PEI phase in S3 boot path later.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS     
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
SmmLockBoxSmmConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  SMM_LOCK_BOX_CONTEXT *SmmLockBoxContext;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SmmLockBoxSmmConstructor - Enter\n"));

  //
  // Register SmmReadyToLock notification.
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmReadyToLockProtocolGuid,
                    SmmLockBoxSmmReadyToLockNotify,
                    &mSmmLockBoxRegistrationSmmReadyToLock
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register SmmEndOfDxe notification.
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmEndOfDxeProtocolGuid,
                    SmmLockBoxSmmEndOfDxeNotify,
                    &mSmmLockBoxRegistrationSmmEndOfDxe
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register EndOfS3Resume notification.
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEdkiiEndOfS3ResumeGuid,
                    SmmLockBoxEndOfS3ResumeNotify,
                    &mSmmLockBoxRegistrationEndOfS3Resume
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Check if gEfiSmmLockBoxCommunicationGuid is installed by someone
  //
  SmmLockBoxContext = InternalGetSmmLockBoxContext ();
  if (SmmLockBoxContext != NULL) {
    //
    // Find it. That means some other library instance is already run.
    // No need to install again, just return.
    //
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SmmLockBoxContext - already installed\n"));
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SmmLockBoxSmmConstructor - Exit\n"));
    return EFI_SUCCESS;
  }

  //
  // If no one install this, it means this is first instance. Install it.
  //
  if (sizeof(UINTN) == sizeof(UINT64)) {
    mSmmLockBoxContext.Signature = SMM_LOCK_BOX_SIGNATURE_64;
  } else {
    mSmmLockBoxContext.Signature = SMM_LOCK_BOX_SIGNATURE_32;
  }
  mSmmLockBoxContext.LockBoxDataAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)&mLockBoxQueue;

  Status = gSmst->SmmInstallConfigurationTable (
                    gSmst,
                    &gEfiSmmLockBoxCommunicationGuid,
                    &mSmmLockBoxContext,
                    sizeof(mSmmLockBoxContext)
                    );
  ASSERT_EFI_ERROR (Status);
  mSmmConfigurationTableInstalled = TRUE;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SmmLockBoxContext - %x\n", (UINTN)&mSmmLockBoxContext));
  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib LockBoxDataAddress - %x\n", (UINTN)&mLockBoxQueue));
  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SmmLockBoxSmmConstructor - Exit\n"));

  return Status;
}

/**
  Destructor for SmmLockBox library.
  This is used to uninstall SmmLockBoxCommunication configuration table
  if it has been installed in Constructor.

  @param[in] ImageHandle    Image handle of this driver.
  @param[in] SystemTable    A Pointer to the EFI System Table.

  @retval EFI_SUCEESS       The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmLockBoxSmmDestructor (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SmmLockBoxSmmDestructor in %a module\n", gEfiCallerBaseName));

  if (mSmmConfigurationTableInstalled) {
    Status = gSmst->SmmInstallConfigurationTable (
                      gSmst,
                      &gEfiSmmLockBoxCommunicationGuid,
                      NULL,
                      0
                      );
    ASSERT_EFI_ERROR (Status);
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib uninstall SmmLockBoxCommunication configuration table\n"));
  }

  if (mSmmLockBoxRegistrationSmmReadyToLock != NULL) {
    //
    // Unregister SmmReadyToLock notification.
    //
    Status = gSmst->SmmRegisterProtocolNotify (
                      &gEfiSmmReadyToLockProtocolGuid,
                      NULL,
                      &mSmmLockBoxRegistrationSmmReadyToLock
                      );
    ASSERT_EFI_ERROR (Status);
  }
  if (mSmmLockBoxRegistrationSmmEndOfDxe != NULL) {
    //
    // Unregister SmmEndOfDxe notification.
    //
    Status = gSmst->SmmRegisterProtocolNotify (
                      &gEfiSmmEndOfDxeProtocolGuid,
                      NULL,
                      &mSmmLockBoxRegistrationSmmEndOfDxe
                      );
    ASSERT_EFI_ERROR (Status);
  }
  if (mSmmLockBoxRegistrationEndOfS3Resume != NULL) {
    //
    // Unregister EndOfS3Resume notification.
    //
    Status = gSmst->SmmRegisterProtocolNotify (
                      &gEdkiiEndOfS3ResumeGuid,
                      NULL,
                      &mSmmLockBoxRegistrationEndOfS3Resume
                      );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  This function return SmmLockBox queue address.

  @return SmmLockBox queue address.
**/
LIST_ENTRY *
InternalGetLockBoxQueue (
  VOID
  )
{
  SMM_LOCK_BOX_CONTEXT        *SmmLockBoxContext;

  SmmLockBoxContext = InternalGetSmmLockBoxContext ();
  ASSERT (SmmLockBoxContext != NULL);
  if (SmmLockBoxContext == NULL) {
    return NULL;
  }
  return (LIST_ENTRY *)(UINTN)SmmLockBoxContext->LockBoxDataAddress;
}

/**
  This function find LockBox by GUID.

  @param Guid The guid to indentify the LockBox

  @return LockBoxData
**/
SMM_LOCK_BOX_DATA *
InternalFindLockBoxByGuid (
  IN EFI_GUID   *Guid
  )
{
  LIST_ENTRY                    *Link;
  SMM_LOCK_BOX_DATA             *LockBox;
  LIST_ENTRY                    *LockBoxQueue;

  LockBoxQueue = InternalGetLockBoxQueue ();
  ASSERT (LockBoxQueue != NULL);

  for (Link = LockBoxQueue->ForwardLink;
       Link != LockBoxQueue;
       Link = Link->ForwardLink) {
    LockBox = BASE_CR (
                Link,
                SMM_LOCK_BOX_DATA,
                Link
                );
    if (CompareGuid (&LockBox->Guid, Guid)) {
      return LockBox;
    }
  }
  return NULL;
}

/**
  This function will save confidential information to lockbox.

  @param Guid       the guid to identify the confidential information
  @param Buffer     the address of the confidential information
  @param Length     the length of the confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or Length is 0
  @retval RETURN_ALREADY_STARTED    the requested GUID already exist.
  @retval RETURN_OUT_OF_RESOURCES   no enough resource to save the information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
SaveLockBox (
  IN  GUID                        *Guid,
  IN  VOID                        *Buffer,
  IN  UINTN                       Length
  )
{
  SMM_LOCK_BOX_DATA           *LockBox;
  EFI_PHYSICAL_ADDRESS        SmramBuffer;
  EFI_STATUS                  Status;
  LIST_ENTRY                  *LockBoxQueue;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SaveLockBox - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) || (Buffer == NULL) || (Length == 0)) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SaveLockBox - Exit (%r)\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find LockBox
  //
  LockBox = InternalFindLockBoxByGuid (Guid);
  if (LockBox != NULL) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SaveLockBox - Exit (%r)\n", EFI_ALREADY_STARTED));
    return EFI_ALREADY_STARTED;
  }

  //
  // Allocate SMRAM buffer
  //
  Status = gSmst->SmmAllocatePages (
                    AllocateAnyPages,
                    EfiRuntimeServicesData,
                    EFI_SIZE_TO_PAGES (Length),
                    &SmramBuffer
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SaveLockBox - Exit (%r)\n", EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Allocate LockBox
  //
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof(*LockBox),
                    (VOID **)&LockBox
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    gSmst->SmmFreePages (SmramBuffer, EFI_SIZE_TO_PAGES (Length));
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SaveLockBox - Exit (%r)\n", EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Save data
  //
  CopyMem ((VOID *)(UINTN)SmramBuffer, (VOID *)(UINTN)Buffer, Length);

  //
  // Insert LockBox to queue
  //
  LockBox->Signature   = SMM_LOCK_BOX_DATA_SIGNATURE;
  CopyMem (&LockBox->Guid, Guid, sizeof(EFI_GUID));
  LockBox->Buffer      = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
  LockBox->Length      = (UINT64)Length;
  LockBox->Attributes  = 0;
  LockBox->SmramBuffer = SmramBuffer;

  DEBUG ((
    EFI_D_INFO,
    "LockBoxGuid - %g, SmramBuffer - 0x%lx, Length - 0x%lx\n",
    &LockBox->Guid,
    LockBox->SmramBuffer,
    LockBox->Length
    ));

  LockBoxQueue = InternalGetLockBoxQueue ();
  ASSERT (LockBoxQueue != NULL);
  InsertTailList (LockBoxQueue, &LockBox->Link);

  //
  // Done
  //
  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SaveLockBox - Exit (%r)\n", EFI_SUCCESS));
  return EFI_SUCCESS;
}

/**
  This function will set lockbox attributes.

  @param Guid       the guid to identify the confidential information
  @param Attributes the attributes of the lockbox

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  attributes is invalid.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
SetLockBoxAttributes (
  IN  GUID                        *Guid,
  IN  UINT64                      Attributes
  )
{
  SMM_LOCK_BOX_DATA              *LockBox;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SetLockBoxAttributes - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) ||
      ((Attributes & ~(LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE | LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY)) != 0)) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SetLockBoxAttributes - Exit (%r)\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  if (((Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) != 0) &&
      ((Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY) != 0)) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SetLockBoxAttributes - Exit (%r)\n", EFI_INVALID_PARAMETER));
    DEBUG ((EFI_D_INFO, "  LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE and LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY\n\n"));
    DEBUG ((EFI_D_INFO, "  can not be set together\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find LockBox
  //
  LockBox = InternalFindLockBoxByGuid (Guid);
  if (LockBox == NULL) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SetLockBoxAttributes - Exit (%r)\n", EFI_NOT_FOUND));
    return EFI_NOT_FOUND;
  }

  if ((((Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) != 0) &&
      ((LockBox->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY) != 0)) ||
      (((LockBox->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) != 0) &&
      ((Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY) != 0))) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SetLockBoxAttributes 0x%lx 0x%lx - Exit (%r)\n", LockBox->Attributes, Attributes, EFI_INVALID_PARAMETER));
    DEBUG ((EFI_D_INFO, "  LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE and LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY\n\n"));
    DEBUG ((EFI_D_INFO, "  can not be set together\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update data
  //
  LockBox->Attributes = Attributes;

  //
  // Done
  //
  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib SetLockBoxAttributes - Exit (%r)\n", EFI_SUCCESS));
  return EFI_SUCCESS;
}

/**
  This function will update confidential information to lockbox.

  @param Guid   the guid to identify the original confidential information
  @param Offset the offset of the original confidential information
  @param Buffer the address of the updated confidential information
  @param Length the length of the updated confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or Length is 0.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_BUFFER_TOO_SMALL   the original buffer to too small to hold new information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
UpdateLockBox (
  IN  GUID                        *Guid,
  IN  UINTN                       Offset,
  IN  VOID                        *Buffer,
  IN  UINTN                       Length
  )
{
  SMM_LOCK_BOX_DATA             *LockBox;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib UpdateLockBox - Enter\n"));

  //
  // Basic check
  //
  if ((Guid == NULL) || (Buffer == NULL) || (Length == 0)) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib UpdateLockBox - Exit (%r)\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find LockBox
  //
  LockBox = InternalFindLockBoxByGuid (Guid);
  if (LockBox == NULL) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib UpdateLockBox - Exit (%r)\n", EFI_NOT_FOUND));
    return EFI_NOT_FOUND;
  }

  //
  // Update data
  //
  if (LockBox->Length < Offset + Length) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib UpdateLockBox - Exit (%r)\n", EFI_BUFFER_TOO_SMALL));
    return EFI_BUFFER_TOO_SMALL;
  }
  ASSERT ((UINTN)LockBox->SmramBuffer <= (MAX_ADDRESS - Offset));
  CopyMem ((VOID *)((UINTN)LockBox->SmramBuffer + Offset), Buffer, Length);

  //
  // Done
  //
  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib UpdateLockBox - Exit (%r)\n", EFI_SUCCESS));
  return EFI_SUCCESS;
}

/**
  This function will restore confidential information from lockbox.

  @param Guid   the guid to identify the confidential information
  @param Buffer the address of the restored confidential information
                NULL means restored to original address, Length MUST be NULL at same time.
  @param Length the length of the restored confidential information

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or one of Buffer and Length is NULL.
  @retval RETURN_WRITE_PROTECTED    Buffer and Length are NULL, but the LockBox has no 
                                    LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE attribute.
  @retval RETURN_BUFFER_TOO_SMALL   the Length is too small to hold the confidential information.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_ACCESS_DENIED      not allow to restore to the address
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreLockBox (
  IN  GUID                        *Guid,
  IN  VOID                        *Buffer, OPTIONAL
  IN  OUT UINTN                   *Length  OPTIONAL
  )
{
  SMM_LOCK_BOX_DATA              *LockBox;
  VOID                           *RestoreBuffer;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreLockBox - Enter\n"));

  //
  // Restore this, Buffer and Length MUST be both NULL or both non-NULL
  //
  if ((Guid == NULL) ||
      ((Buffer == NULL) && (Length != NULL)) ||
      ((Buffer != NULL) && (Length == NULL))) {
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreLockBox - Exit (%r)\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find LockBox
  //
  LockBox = InternalFindLockBoxByGuid (Guid);
  if (LockBox == NULL) {
    //
    // Not found
    //
    DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreLockBox - Exit (%r)\n", EFI_NOT_FOUND));
    return EFI_NOT_FOUND;
  }

  if (((LockBox->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY) != 0) &&
      mSmmLockBoxSmmReadyToLock &&
      !mSmmLockBoxDuringS3Resume) {
    //
    // With LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY,
    // this LockBox can be restored in S3 resume only.
    //
    return EFI_ACCESS_DENIED;
  }

  //
  // Set RestoreBuffer
  //
  if (Buffer != NULL) {
    //
    // restore to new buffer
    //
    RestoreBuffer = Buffer;
  } else {
    //
    // restore to original buffer
    //
    if ((LockBox->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) == 0) {
      DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreLockBox - Exit (%r)\n", EFI_WRITE_PROTECTED));
      return EFI_WRITE_PROTECTED;
    }
    RestoreBuffer = (VOID *)(UINTN)LockBox->Buffer;
  }

  //
  // Set RestoreLength
  //
  if (Length != NULL) {
    if (*Length < (UINTN)LockBox->Length) {
      //
      // Input buffer is too small to hold all data.
      //
      *Length = (UINTN)LockBox->Length;
      DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreLockBox - Exit (%r)\n", EFI_BUFFER_TOO_SMALL));
      return EFI_BUFFER_TOO_SMALL;
    }
    *Length = (UINTN)LockBox->Length;
  }

  //
  // Restore data
  //
  CopyMem (RestoreBuffer, (VOID *)(UINTN)LockBox->SmramBuffer, (UINTN)LockBox->Length);

  //
  // Done
  //
  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreLockBox - Exit (%r)\n", EFI_SUCCESS));
  return EFI_SUCCESS;
}

/**
  This function will restore confidential information from all lockbox which have RestoreInPlace attribute.

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreAllLockBoxInPlace (
  VOID
  )
{
  SMM_LOCK_BOX_DATA             *LockBox;
  LIST_ENTRY                    *Link;
  LIST_ENTRY                    *LockBoxQueue;

  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreAllLockBoxInPlace - Enter\n"));

  LockBoxQueue = InternalGetLockBoxQueue ();
  ASSERT (LockBoxQueue != NULL);

  //
  // Restore all, Buffer and Length MUST be NULL
  //
  for (Link = LockBoxQueue->ForwardLink;
       Link != LockBoxQueue;
       Link = Link->ForwardLink) {
    LockBox = BASE_CR (
                Link,
                SMM_LOCK_BOX_DATA,
                Link
                );
    if ((LockBox->Attributes & LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE) != 0) {
      //
      // Restore data
      //
      CopyMem ((VOID *)(UINTN)LockBox->Buffer, (VOID *)(UINTN)LockBox->SmramBuffer, (UINTN)LockBox->Length);
    }
  }
  //
  // Done
  //
  DEBUG ((EFI_D_INFO, "SmmLockBoxSmmLib RestoreAllLockBoxInPlace - Exit (%r)\n", EFI_SUCCESS));
  return EFI_SUCCESS;
}


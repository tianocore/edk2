/** @file
  MmCommunicationDxe driver produces MmCommunication protocol and
  create the notifications of some protocols and event.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MmCommunicationDxe.h"

//
// PI 1.7 MM Communication Protocol 2 instance
//
EFI_MM_COMMUNICATION2_PROTOCOL  mMmCommunication2 = {
  MmCommunicate2
};

//
// PI 1.7 MM Communication Protocol instance
//
EFI_MM_COMMUNICATION_PROTOCOL  mMmCommunication = {
  MmCommunicate
};

MM_COMM_BUFFER             mMmCommonBuffer;
EFI_SMM_CONTROL2_PROTOCOL  *mSmmControl2;
EFI_SMM_ACCESS2_PROTOCOL   *mSmmAccess;
BOOLEAN                    mSmmLocked = FALSE;
BOOLEAN                    mEndOfDxe  = FALSE;

//
// Table of Protocol notification and GUIDed Event notifications that the Standalone Mm requires
//
MM_EVENT_NOTIFICATION  mMmEvents[] = {
  //
  // Declare protocol notification on DxeMmReadyToLock protocols.  When this notification is established,
  // the associated event is immediately signalled, so the notification function will be executed and the
  // DXE Mm Ready To Lock Protocol will be found if it is already in the handle database.
  //
  { ProtocolNotify, TRUE,  &gEfiDxeMmReadyToLockProtocolGuid,  MmReadyToLockEventNotify,    &gEfiDxeMmReadyToLockProtocolGuid, NULL },
  //
  // Declare event notification on Ready To Boot Event Group.  This is an extra event notification that is
  // used to make sure SMRAM is locked before any boot options are processed.
  //
  { EventNotify,    TRUE,  &gEfiEventReadyToBootGuid,          MmReadyToLockEventNotify,    &gEfiEventReadyToBootGuid,         NULL },
  //
  // Declare event notification on Ready To Boot Event Group.  This is used to inform the MM Core
  // to notify MM driver that system enter ready to boot.
  //
  { EventNotify,    FALSE, &gEfiEventReadyToBootGuid,          MmGuidedEventNotify,         &gEfiEventReadyToBootGuid,         NULL },
  //
  // Declare event notification on EndOfDxe event.  When this notification is established,
  // the associated event is immediately signalled, so the notification function will be executed and the
  // End Of Dxe Protocol will be found if it is already in the handle database.
  //
  { EventNotify,    TRUE,  &gEfiEndOfDxeEventGroupGuid,        MmGuidedEventNotify,         &gEfiEndOfDxeEventGroupGuid,       NULL },
  //
  // Declare event notification on EndOfDxe event.  This is used to set EndOfDxe event signaled flag.
  //
  { EventNotify,    TRUE,  &gEfiEndOfDxeEventGroupGuid,        MmEndOfDxeEventNotify,       &gEfiEndOfDxeEventGroupGuid,       NULL },
  //
  // Declare event notification on Exit Boot Services Event Group.  This is used to inform the MM Core
  // to notify MM driver that system enter exit boot services.
  //
  { EventNotify,    FALSE, &gEfiEventExitBootServicesGuid,     MmGuidedEventNotify,         &gEfiEventExitBootServicesGuid,    NULL },
  //
  // Declare event notification on SetVirtualAddressMap() Event Group.  This is used to convert fixed MM communication buffer
  // and MM_COMM_BUFFER_STATUS in mMmCommonBuffer, mSmmControl2 from physical addresses to virtual addresses.
  //
  { EventNotify,    FALSE, &gEfiEventVirtualAddressChangeGuid, MmVirtualAddressChangeEvent, NULL,                              NULL },
  //
  // Terminate the table of event notifications
  //
  { EndNotify,      FALSE, NULL,                               NULL,                        NULL,                              NULL }
};

/**
  Event notification that is fired when GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
MmGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN                      Size;
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;

  CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)mMmCommonBuffer.PhysicalStart;

  //
  // Use Guid to initialize EFI_MM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&CommunicateHeader->HeaderGuid, (EFI_GUID *)Context);
  CommunicateHeader->MessageLength = 1;
  CommunicateHeader->Data[0]       = 0;

  //
  // Generate the Software SMI and return the result
  //
  Size = sizeof (EFI_MM_COMMUNICATE_HEADER);
  MmCommunicate2 (&mMmCommunication2, CommunicateHeader, CommunicateHeader, &Size);
}

/**
  Event notification that is fired every time a DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
MmReadyToLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;
  UINTN       Index;

  //
  // See if we are already locked
  //
  if (mSmmLocked) {
    return;
  }

  //
  // Make sure this notification is for this handler
  //
  if (CompareGuid ((EFI_GUID *)Context, &gEfiDxeMmReadyToLockProtocolGuid)) {
    Status = gBS->LocateProtocol (&gEfiDxeMmReadyToLockProtocolGuid, NULL, &Interface);
    if (EFI_ERROR (Status)) {
      return;
    }
  } else {
    //
    // If MM is not locked yet and we got here from gEfiEventReadyToBootGuid being
    // signaled, then gEfiDxeMmReadyToLockProtocolGuid was not installed as expected.
    // Print a warning on debug builds.
    //
    DEBUG ((DEBUG_WARN, "DXE Mm Ready To Lock Protocol not installed before Ready To Boot signal\n"));
  }

  if (!mEndOfDxe) {
    DEBUG ((DEBUG_ERROR, "EndOfDxe Event must be signaled before DxeSmmReadyToLock Protocol installation!\n"));
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED,
      (EFI_SOFTWARE_SMM_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE)
      );
    ASSERT (FALSE);
  }

  //
  // Lock the SMRAM (Note: Locking SMRAM may not be supported on all platforms)
  //
  mSmmAccess->Lock (mSmmAccess);

  //
  // Close protocol and event notification events that do not apply after the
  // DXE MM Ready To Lock Protocol has been installed or the Ready To Boot
  // event has been signalled.
  //
  for (Index = 0; mMmEvents[Index].NotifyFunction != NULL; Index++) {
    if (mMmEvents[Index].CloseOnLock) {
      gBS->CloseEvent (mMmEvents[Index].Event);
    }
  }

  //
  // Inform MM Core that the DxeSmmReadyToLock protocol was installed
  //
  MmGuidedEventNotify (Event, (VOID *)&gEfiDxeMmReadyToLockProtocolGuid);

  //
  // Print debug message that the SMRAM window is now locked.
  //
  DEBUG ((DEBUG_INFO, "MmCommunicationDxe locked SMRAM window\n"));

  //
  // Set flag so this operation will not be performed again
  //
  mSmmLocked = TRUE;
}

/**
  Event notification that is fired when EndOfDxe Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
MmEndOfDxeEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mEndOfDxe = TRUE;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
MmVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mMmCommonBuffer.Status);
  EfiConvertPointer (0x0, (VOID **)&mMmCommonBuffer.PhysicalStart);
  EfiConvertPointer (0x0, (VOID **)&mSmmControl2);
}

/**
  Processes the communication buffer for Mm communication protocols.

  This function encapsulates the common logic for handling communication buffers
  used by MmCommunicate2 and MmCommunicate functions.

  @param[in, out] CommBuffer          Pointer to the MM communication buffer
  @param[in, out] CommSize            The size of the data buffer being passed in. On exit, the size of data
                                      being returned. Zero if the handler does not wish to reply with any data.
                                      This parameter is optional and may be NULL.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation.
                                      If this error is returned, the MessageLength field
                                      in the CommBuffer header or the integer pointed by
                                      CommSize, are updated to reflect the maximum payload
                                      size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter or CommSize parameter,
                                      if not omitted, are in address range that cannot be
                                      accessed by the MM environment.
**/
EFI_STATUS
EFIAPI
ProcessCommunicationBuffer (
  IN OUT VOID   *CommBuffer,
  IN OUT UINTN  *CommSize OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  MM_COMM_BUFFER_STATUS      *CommonBufferStatus;
  UINTN                      BufferSize;

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)CommBuffer;
  BufferSize        = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + CommunicateHeader->MessageLength;

  if (CommSize != NULL) {
    ASSERT (*CommSize == BufferSize);
  }

  CommonBufferStatus = (MM_COMM_BUFFER_STATUS *)(UINTN)mMmCommonBuffer.Status;

  //
  // Copy the content at input CommBuffer to fixed MM communication buffer
  // if CommBuffer is not equal to fixed MM communication buffer.
  //
  if ((UINTN)CommBuffer != mMmCommonBuffer.PhysicalStart) {
    CopyMem ((VOID *)(UINTN)mMmCommonBuffer.PhysicalStart, CommBuffer, BufferSize);
  }

  CommonBufferStatus->IsCommBufferValid = TRUE;

  //
  // Generate Software SMI
  //
  Status = mSmmControl2->Trigger (mSmmControl2, NULL, NULL, FALSE, 0);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Copy the returned data to the non-mmram buffer (CommBuffer)
  //
  if ((UINTN)CommBuffer != mMmCommonBuffer.PhysicalStart) {
    CopyMem (CommBuffer, (VOID *)(UINTN)mMmCommonBuffer.PhysicalStart, CommonBufferStatus->ReturnBufferSize);
  }

  //
  // Retrieve BufferSize and return status from CommonBufferStatus
  //
  if (CommSize != NULL) {
    *CommSize = CommonBufferStatus->ReturnBufferSize;
  }

  CommonBufferStatus->IsCommBufferValid = FALSE;

  return CommonBufferStatus->ReturnStatus;
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                     The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBufferPhysical  Physical address of the MM communication buffer.
  @param[in, out] CommBufferVirtual   Virtual address of the MM communication buffer.
  @param[in, out] CommSize            The size of the data buffer being passed in. On exit, the size of data
                                      being returned. Zero if the handler does not wish to reply with any data.
                                      This parameter is optional and may be NULL.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation.
                                      If this error is returned, the MessageLength field
                                      in the CommBuffer header or the integer pointed by
                                      CommSize, are updated to reflect the maximum payload
                                      size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter or CommSize parameter,
                                      if not omitted, are in address range that cannot be
                                      accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
MmCommunicate2 (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL  *This,
  IN OUT VOID                              *CommBufferPhysical,
  IN OUT VOID                              *CommBufferVirtual,
  IN OUT UINTN                             *CommSize OPTIONAL
  )
{
  return ProcessCommunicationBuffer (CommBufferVirtual, CommSize);
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                     The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer          Pointer to the MM communication buffer
  @param[in, out] CommSize            The size of the data buffer being passed in. On exit, the size of data
                                      being returned. Zero if the handler does not wish to reply with any data.
                                      This parameter is optional and may be NULL.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation.
                                      If this error is returned, the MessageLength field
                                      in the CommBuffer header or the integer pointed by
                                      CommSize, are updated to reflect the maximum payload
                                      size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter or CommSize parameter,
                                      if not omitted, are in address range that cannot be
                                      accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
MmCommunicate (
  IN CONST EFI_MM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                             *CommBuffer,
  IN OUT UINTN                            *CommSize OPTIONAL
  )
{
  return ProcessCommunicationBuffer (CommBuffer, CommSize);
}

/**
  The Entry Point for MmCommunicateDxe driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MmCommunicationEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  EFI_HANDLE         Handle;
  EFI_HOB_GUID_TYPE  *GuidHob;
  MM_COMM_BUFFER     *MmCommonBuffer;
  UINTN              Index;
  VOID               *Registration;

  //
  // Locate gMmCommBufferHobGuid and cache the content
  //
  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  ASSERT (GuidHob != NULL);
  MmCommonBuffer = GET_GUID_HOB_DATA (GuidHob);
  CopyMem (&mMmCommonBuffer, MmCommonBuffer, sizeof (MM_COMM_BUFFER));

  //
  // Get SMM Control2 Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmControl2ProtocolGuid, NULL, (VOID **)&mSmmControl2);
  ASSERT_EFI_ERROR (Status);

  //
  // Get SMM Access Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&mSmmAccess);
  ASSERT_EFI_ERROR (Status);

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiMmCommunication2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication2
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiMmCommunicationProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create the set of protocol and event notifications that the Standalone Mm requires
  //
  for (Index = 0; mMmEvents[Index].NotificationType != EndNotify; Index++) {
    if (mMmEvents[Index].NotificationType == ProtocolNotify) {
      mMmEvents[Index].Event = EfiCreateProtocolNotifyEvent (
                                 mMmEvents[Index].Guid,
                                 TPL_CALLBACK,
                                 mMmEvents[Index].NotifyFunction,
                                 mMmEvents[Index].NotifyContext,
                                 &Registration
                                 );
    } else {
      Status = gBS->CreateEventEx (
                      EVT_NOTIFY_SIGNAL,
                      TPL_CALLBACK,
                      mMmEvents[Index].NotifyFunction,
                      mMmEvents[Index].NotifyContext,
                      mMmEvents[Index].Guid,
                      &mMmEvents[Index].Event
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}

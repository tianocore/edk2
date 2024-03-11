/** @file
MmCommunicationDxe driverproduce MmCommunication protocol and signal events via
and communicate UEFI memory map to PiSmmCpuStandaloneMm driver in EndOfDxe callback.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MmCommunicationDxe.h"

//
// PI 1.7 MM Communication Protocol 2 instance
//
EFI_MM_COMMUNICATION2_PROTOCOL  mMmCommunication2 = {
  SmmCommunicationMmCommunicate2
};

//
// Handle to install the SMM Base2 Protocol and the SMM Communication Protocol
//
EFI_HANDLE  mSmmIplHandle = NULL;

//
// SMM IPL global variables
//
EFI_SMM_CONTROL2_PROTOCOL  *mSmmControl2;
EFI_SMM_ACCESS2_PROTOCOL   *mSmmAccess;
BOOLEAN                    mSmmLocked = FALSE;
BOOLEAN                    mEndOfDxe  = FALSE;
EFI_PHYSICAL_ADDRESS       mSmramCacheBase;
UINT64                     mSmramCacheSize;
EFI_MM_COMMUNICATE_HEADER  mCommunicateHeader;

VOID
EFIAPI
SmmReadyToLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
SmmGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
SmmEndOfDxeEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// Data structure used to declare a table of protocol notifications and event
// notifications required by the SMM IPL
//
typedef struct {
  BOOLEAN             Protocol;
  BOOLEAN             CloseOnLock;
  EFI_GUID            *Guid;
  EFI_EVENT_NOTIFY    NotifyFunction;
  VOID                *NotifyContext;
  EFI_TPL             NotifyTpl;
  EFI_EVENT           Event;
} SMM_EVENT_NOTIFICATION;

//
// Table of Protocol notification and GUIDed Event notifications that the SMM IPL requires
//
SMM_EVENT_NOTIFICATION  mSmmEvents[] = {
  //
  // Declare protocol notification on DxeSmmReadyToLock protocols.  When this notification is established,
  // the associated event is immediately signalled, so the notification function will be executed and the
  // DXE SMM Ready To Lock Protocol will be found if it is already in the handle database.
  //
  { TRUE,  TRUE,  &gEfiDxeSmmReadyToLockProtocolGuid, SmmReadyToLockEventNotify, &gEfiDxeSmmReadyToLockProtocolGuid, TPL_CALLBACK, NULL },
  //
  // Declare event notification on EndOfDxe event.  When this notification is established,
  // the associated event is immediately signalled, so the notification function will be executed and the
  // SMM End Of Dxe Protocol will be found if it is already in the handle database.
  //
  { FALSE, TRUE,  &gEfiEndOfDxeEventGroupGuid,        SmmGuidedEventNotify,      &gEfiEndOfDxeEventGroupGuid,        TPL_CALLBACK, NULL },
  //
  // Declare event notification on EndOfDxe event.  This is used to set EndOfDxe event signaled flag.
  //
  { FALSE, TRUE,  &gEfiEndOfDxeEventGroupGuid,        SmmEndOfDxeEventNotify,    &gEfiEndOfDxeEventGroupGuid,        TPL_CALLBACK, NULL },
  //
  // Declare event notification on Ready To Boot Event Group.  This is an extra event notification that is
  // used to make sure SMRAM is locked before any boot options are processed.
  //
  { FALSE, TRUE,  &gEfiEventReadyToBootGuid,          SmmReadyToLockEventNotify, &gEfiEventReadyToBootGuid,          TPL_CALLBACK, NULL },
  //
  // Declare event notification on Ready To Boot Event Group.  This is used to inform the SMM Core
  // to notify SMM driver that system enter ready to boot.
  //
  { FALSE, FALSE, &gEfiEventReadyToBootGuid,          SmmGuidedEventNotify,      &gEfiEventReadyToBootGuid,          TPL_CALLBACK, NULL },
  //
  // Declare event notification on Exit Boot Services Event Group.  This is used to inform the SMM Core
  // to notify SMM driver that system enter exit boot services.
  //
  { FALSE, FALSE, &gEfiEventExitBootServicesGuid,     SmmGuidedEventNotify,      &gEfiEventExitBootServicesGuid,     TPL_CALLBACK, NULL },
  //
  // Terminate the table of event notifications
  //
  { FALSE, FALSE, NULL,                               NULL,                      NULL,                               TPL_CALLBACK, NULL }
};

/**
  Event notification that is fired when GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Size;

  //
  // Use Guid to initialize EFI_MM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&mCommunicateHeader.HeaderGuid, (EFI_GUID *)Context);
  mCommunicateHeader.MessageLength = 1;
  mCommunicateHeader.Data[0]       = 0;

  //
  // Generate the Software SMI and return the result
  //
  Size = sizeof (mCommunicateHeader);
  SmmCommunicationMmCommunicate2 (&mMmCommunication2, &mCommunicateHeader, NULL, &Size);
}

/**
  Event notification that is fired every time a DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmReadyToLockEventNotify (
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
  if (CompareGuid ((EFI_GUID *)Context, &gEfiDxeSmmReadyToLockProtocolGuid)) {
    Status = gBS->LocateProtocol (&gEfiDxeSmmReadyToLockProtocolGuid, NULL, &Interface);
    if (EFI_ERROR (Status)) {
      return;
    }
  } else {
    //
    // If SMM is not locked yet and we got here from gEfiEventReadyToBootGuid being
    // signaled, then gEfiDxeSmmReadyToLockProtocolGuid was not installed as expected.
    // Print a warning on debug builds.
    //
    DEBUG ((DEBUG_WARN, "SMM IPL!  DXE SMM Ready To Lock Protocol not installed before Ready To Boot signal\n"));
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
  // DXE SMM Ready To Lock Protocol has been installed or the Ready To Boot
  // event has been signalled.
  //
  for (Index = 0; mSmmEvents[Index].NotifyFunction != NULL; Index++) {
    if (mSmmEvents[Index].CloseOnLock) {
      gBS->CloseEvent (mSmmEvents[Index].Event);
    }
  }

  //
  // Inform SMM Core that the DxeSmmReadyToLock protocol was installed
  //
  SmmGuidedEventNotify (Event, (VOID *)&gEfiDxeSmmReadyToLockProtocolGuid);

  //
  // Print debug message that the SMRAM window is now locked.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL locked SMRAM window\n"));

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
SmmEndOfDxeEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mEndOfDxe = TRUE;
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in] CommBufferPhysical  Physical address of the MM communication buffer
  @param[in] CommBufferVirtual   Virtual address of the MM communication buffer
  @param[in] CommSize            The size of the data buffer being passed in. On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE    The buffer is too large for the MM implementation.
                                 If this error is returned, the MessageLength field
                                 in the CommBuffer header or the integer pointed by
                                 CommSize, are updated to reflect the maximum payload
                                 size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED      The CommunicateBuffer parameter or CommSize parameter,
                                 if not omitted, are in address range that cannot be
                                 accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
SmmCommunicationMmCommunicate2 (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL  *This,
  IN OUT VOID                              *CommBufferPhysical,
  IN OUT VOID                              *CommBufferVirtual,
  IN OUT UINTN                             *CommSize OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  EFI_HOB_GUID_TYPE          *GuidHob;
  MM_COMM_BUFFER_DATA        *MmCommonBufferData;
  COMMUNICATION_IN_OUT       *CommunicationInOutBuffer;
  UINTN                      BufferSize;

  //
  // Check parameters
  //
  if (CommBufferPhysical == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)CommBufferPhysical;
  BufferSize        = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + CommunicateHeader->MessageLength;

  if (CommSize != NULL) {
    ASSERT (*CommSize == BufferSize);
  }

  //
  // Locate gEdkiiCommunicationBufferGuid
  //
  GuidHob = GetFirstGuidHob (&gEdkiiCommunicationBufferGuid);
  ASSERT (GuidHob != NULL);
  MmCommonBufferData       = GET_GUID_HOB_DATA (GuidHob);
  CommunicationInOutBuffer = (COMMUNICATION_IN_OUT *)(UINTN)MmCommonBufferData->CommunicationInOut;

  //
  // Copy the content at input CommBufferPhysical to FixedCommBuffer
  // if CommBufferPhysical is not equal to FixedCommBuffer.
  //
  if ((UINTN)CommBufferPhysical != MmCommonBufferData->FixedCommBuffer) {
    CopyMem ((VOID *)(MmCommonBufferData->FixedCommBuffer), CommBufferPhysical, BufferSize);
  }

  //
  // Generate Software SMI
  //
  Status = mSmmControl2->Trigger (mSmmControl2, NULL, NULL, FALSE, 0);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Retrive BufferSize and return status from gMmCoreDataHobGuid
  //
  if (CommSize != NULL) {
    *CommSize = CommunicationInOutBuffer->ReturnBufferSize;
  }

  return CommunicationInOutBuffer->ReturnStatus;
}

/**
  The Entry Point for SMM IPL

  Load SMM Core into SMRAM, register SMM Core entry point for SMIs, install
  SMM Base 2 Protocol and SMM Communication Protocol, and register for the
  critical events required to coordinate between DXE and SMM environments.

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
  EFI_STATUS  Status;
  UINTN       Index;
  VOID        *Registration;
  EFI_HANDLE  Handle;

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

  /*
    //
    // Install SMM Communication Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gEfiMmCommunication2ProtocolGuid,
                    &mMmCommunication2,
                    NULL
                    );
                  */
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiMmCommunication2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication2
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create the set of protocol and event notifications that the SMM IPL requires
  //
  for (Index = 0; mSmmEvents[Index].NotifyFunction != NULL; Index++) {
    if (mSmmEvents[Index].Protocol) {
      mSmmEvents[Index].Event = EfiCreateProtocolNotifyEvent (
                                  mSmmEvents[Index].Guid,
                                  mSmmEvents[Index].NotifyTpl,
                                  mSmmEvents[Index].NotifyFunction,
                                  mSmmEvents[Index].NotifyContext,
                                  &Registration
                                  );
    } else {
      Status = gBS->CreateEventEx (
                      EVT_NOTIFY_SIGNAL,
                      mSmmEvents[Index].NotifyTpl,
                      mSmmEvents[Index].NotifyFunction,
                      mSmmEvents[Index].NotifyContext,
                      mSmmEvents[Index].Guid,
                      &mSmmEvents[Index].Event
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}

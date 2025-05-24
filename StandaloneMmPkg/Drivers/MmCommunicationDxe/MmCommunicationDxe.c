/** @file
  MmCommunicationDxe driver produces MmCommunication protocol and
  create the notifications of some protocols and event.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MmCommunicationDxe.h"

//
// PI 1.9 MM Communication Protocol 3 instance
//
EFI_MM_COMMUNICATION3_PROTOCOL  mMmCommunication3 = {
  MmCommunicate3
};

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
EFI_EVENT                  mVirtualAddressChangeEvent = NULL;

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
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER     *CommunicateHeader;
  EFI_MM_COMMUNICATE_HEADER_V3  *CommunicateHeaderV3;
  MM_COMM_BUFFER_STATUS         *CommonBufferStatus;
  UINTN                         BufferSize;

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)CommBuffer;
  if (CompareGuid (&CommunicateHeader->HeaderGuid, &gEfiMmCommunicateHeaderV3Guid)) {
    CommunicateHeaderV3 = (EFI_MM_COMMUNICATE_HEADER_V3 *)CommBuffer;
    if (CommunicateHeaderV3->BufferSize < sizeof (EFI_MM_COMMUNICATE_HEADER_V3) + CommunicateHeaderV3->MessageSize) {
      return EFI_INVALID_PARAMETER;
    }

    BufferSize = ((EFI_MM_COMMUNICATE_HEADER_V3 *)CommBuffer)->BufferSize;
  } else {
    BufferSize = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + CommunicateHeader->MessageLength;
  }

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

  @param[in] This                     The EFI_MM_COMMUNICATION3_PROTOCOL instance.
  @param[in, out] CommBufferPhysical  Physical address of the MM communication buffer, of which content must
                                      start with EFI_MM_COMMUNICATE_HEADER_V3.
  @param[in, out] CommBufferVirtual   Virtual address of the MM communication buffer, of which content must
                                      start with EFI_MM_COMMUNICATE_HEADER_V3.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       CommBufferPhysical was NULL or CommBufferVirtual was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation.
                                      If this error is returned, the MessageSize field
                                      in the CommBuffer header, are updated to reflect
                                      the maximum payload size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter are in address range
                                      that cannot be accessed by the MM environment.

**/
EFI_STATUS
EFIAPI
MmCommunicate3 (
  IN CONST EFI_MM_COMMUNICATION3_PROTOCOL  *This,
  IN OUT VOID                              *CommBufferPhysical,
  IN OUT VOID                              *CommBufferVirtual
  )
{
  return ProcessCommunicationBuffer (CommBufferVirtual, NULL);
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
                  &gEfiMmCommunication3ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication3
                  );
  ASSERT_EFI_ERROR (Status);

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
  // Register the event to convert the pointer for runtime.
  //
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         MmVirtualAddressChangeEvent,
         NULL,
         &gEfiEventVirtualAddressChangeGuid,
         &mVirtualAddressChangeEvent
         );

  return EFI_SUCCESS;
}

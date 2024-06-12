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

MM_COMM_BUFFER             mMmCommonBuffer;
EFI_SMM_CONTROL2_PROTOCOL  *mSmmControl2;

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
MmVariableAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mMmCommonBuffer.CommunicationInOut);
  EfiConvertPointer (0x0, (VOID **)&mMmCommonBuffer.FixedCommBuffer);
  EfiConvertPointer (0x0, (VOID **)&mSmmControl2);
}

/**
  Processes the communication buffer for Mm communication protocols.

  This function encapsulates the common logic for handling communication buffers
  used by MmCommunicate2 functions.

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
  COMMUNICATION_IN_OUT       *CommunicationInOutBuffer;
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

  CommunicationInOutBuffer = (COMMUNICATION_IN_OUT *)(UINTN)mMmCommonBuffer.CommunicationInOut;

  //
  // Copy the content at input CommBuffer to FixedCommBuffer
  // if CommBuffer is not equal to FixedCommBuffer.
  //
  if ((UINTN)CommBuffer != mMmCommonBuffer.FixedCommBuffer) {
    CopyMem ((VOID *)(UINTN)mMmCommonBuffer.FixedCommBuffer, CommBuffer, BufferSize);
  }

  CommunicationInOutBuffer->IsCommBufferValid = TRUE;

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
  if ((UINTN)CommBuffer != mMmCommonBuffer.FixedCommBuffer) {
    CopyMem (CommBuffer, (VOID *)(UINTN)mMmCommonBuffer.FixedCommBuffer, CommunicationInOutBuffer->ReturnBufferSize);
  }

  //
  // Retrieve BufferSize and return status from CommunicationInOutBuffer
  //
  if (CommSize != NULL) {
    *CommSize = CommunicationInOutBuffer->ReturnBufferSize;
  }

  CommunicationInOutBuffer->IsCommBufferValid = FALSE;

  return CommunicationInOutBuffer->ReturnStatus;
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
  // Locate gEdkiiCommunicationBufferGuid and cache the content
  //
  GuidHob = GetFirstGuidHob (&gEdkiiCommunicationBufferGuid);
  ASSERT (GuidHob != NULL);
  MmCommonBuffer = GET_GUID_HOB_DATA (GuidHob);
  CopyMem (&mMmCommonBuffer, MmCommonBuffer, sizeof (MM_COMM_BUFFER));

  //
  // Get SMM Control2 Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmControl2ProtocolGuid, NULL, (VOID **)&mSmmControl2);
  ASSERT_EFI_ERROR (Status);

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiMmCommunication2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication2
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register the event to convert the pointer for runtime.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  MmVariableAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

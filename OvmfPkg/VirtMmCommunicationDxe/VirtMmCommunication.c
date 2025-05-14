/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/MmCommunication2.h>

#include "VirtMmCommunication.h"

VOID                  *mCommunicateBuffer;
EFI_PHYSICAL_ADDRESS  mCommunicateBufferPhys;
BOOLEAN               mHaveSvsmProtocol = FALSE;
BOOLEAN               mUsePioTransfer   = FALSE;

// Notification event when virtual address map is set.
STATIC EFI_EVENT  mSetVirtualAddressMapEvent;

// Handle to install the MM Communication Protocol
STATIC EFI_HANDLE  mMmCommunicateHandle;

// Handle to install the EfiSmmVariableProtocol
STATIC EFI_HANDLE  mSmmVariableHandle;

// Handle to install the SmmVariableWrite
STATIC EFI_HANDLE  mSmmVariableWriteHandle;

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                     The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBufferPhysical  Physical address of the MM communication buffer
  @param[in, out] CommBufferVirtual   Virtual address of the MM communication buffer
  @param[in, out] CommSize            The size of the data buffer being passed in. On input,
                                      when not omitted, the buffer should cover EFI_MM_COMMUNICATE_HEADER
                                      and the value of MessageLength field. On exit, the size
                                      of data being returned. Zero if the handler does not
                                      wish to reply with any data. This parameter is optional
                                      and may be NULL.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  CommBufferPhysical or CommBufferVirtual was NULL, or
                                 integer value pointed by CommSize does not cover
                                 EFI_MM_COMMUNICATE_HEADER and the value of MessageLength
                                 field.
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
VirtMmCommunication2Communicate (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL  *This,
  IN OUT VOID                              *CommBufferPhysical,
  IN OUT VOID                              *CommBufferVirtual,
  IN OUT UINTN                             *CommSize OPTIONAL
  )
{
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  EFI_STATUS                 Status;
  UINTN                      BufferSize;

  //
  // Check parameters
  //
  if ((CommBufferVirtual == NULL) || (CommBufferPhysical == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status            = EFI_SUCCESS;
  CommunicateHeader = CommBufferVirtual;
  // CommBuffer is a mandatory parameter. Hence, Rely on
  // MessageLength + Header to ascertain the
  // total size of the communication payload rather than
  // rely on optional CommSize parameter
  BufferSize = CommunicateHeader->MessageLength +
               sizeof (CommunicateHeader->HeaderGuid) +
               sizeof (CommunicateHeader->MessageLength);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: %g msglen %ld, BufferSize %ld, CommSize %ld\n",
    __func__,
    &CommunicateHeader->HeaderGuid,
    CommunicateHeader->MessageLength,
    BufferSize,
    *CommSize
    ));

  // If CommSize is not omitted, perform size inspection before proceeding.
  if (CommSize != NULL) {
    // This case can be used by the consumer of this driver to find out the
    // max size that can be used for allocating CommBuffer.
    if ((*CommSize == 0) ||
        (*CommSize > MAX_BUFFER_SIZE))
    {
      *CommSize = MAX_BUFFER_SIZE;
      Status    = EFI_BAD_BUFFER_SIZE;
    }

    //
    // CommSize should cover at least MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER);
    //
    if (*CommSize < BufferSize) {
      Status = EFI_INVALID_PARAMETER;
    }
  }

  //
  // If the message length is 0 or greater than what can be tolerated by the MM
  // environment then return the expected size.
  //
  if ((CommunicateHeader->MessageLength == 0) ||
      (BufferSize > MAX_BUFFER_SIZE))
  {
    CommunicateHeader->MessageLength = MAX_BUFFER_SIZE -
                                       sizeof (CommunicateHeader->HeaderGuid) -
                                       sizeof (CommunicateHeader->MessageLength);
    Status = EFI_BAD_BUFFER_SIZE;
  }

  // MessageLength or CommSize check has failed, return here.
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: check error: %r\n", __func__, Status));
    return Status;
  }

  if (mHaveSvsmProtocol) {
    CopyMem (mCommunicateBuffer, CommBufferVirtual, BufferSize);

    Status = VirtMmSvsmComm ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: svsm comm error: %r\n", __func__, Status));
      return Status;
    }

    CopyMem (CommBufferVirtual, mCommunicateBuffer, BufferSize);
  } else if (mUsePioTransfer) {
    Status = VirtMmHwPioTransfer (CommBufferVirtual, BufferSize, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: pio write error: %r\n", __func__, Status));
      return Status;
    }

    Status = VirtMmHwComm ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: pio comm error: %r\n", __func__, Status));
      return Status;
    }

    Status = VirtMmHwPioTransfer (CommBufferVirtual, BufferSize, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: pio read error: %r\n", __func__, Status));
      return Status;
    }
  } else {
    CopyMem (mCommunicateBuffer, CommBufferVirtual, BufferSize);

    Status = VirtMmHwComm ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: dma comm error: %r\n", __func__, Status));
      return Status;
    }

    CopyMem (CommBufferVirtual, mCommunicateBuffer, BufferSize);
  }

  DEBUG ((DEBUG_VERBOSE, "%a: success (%d)\n", __func__, BufferSize));
  return EFI_SUCCESS;
}

//
// MM Communication Protocol instance
//
STATIC EFI_MM_COMMUNICATION2_PROTOCOL  mMmCommunication2 = {
  VirtMmCommunication2Communicate
};

/**
  Notification callback on SetVirtualAddressMap event.

  This function notifies the MM communication protocol interface on
  SetVirtualAddressMap event and converts pointers used in this driver
  from physical to virtual address.

  @param  Event          SetVirtualAddressMap event.
  @param  Context        A context when the SetVirtualAddressMap triggered.

  @retval EFI_SUCCESS    The function executed successfully.
  @retval Other          Some error occurred when executing this function.

**/
STATIC
VOID
EFIAPI
VirtMmNotifySetVirtualAddressMap (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_VERBOSE, "%a: << %p\n", __func__, mCommunicateBuffer));
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, &mCommunicateBuffer);
  DEBUG ((DEBUG_VERBOSE, "%a: >> %p\n", __func__, mCommunicateBuffer));

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Unable to convert MM runtime pointer. Status: %r\n",
      __func__,
      Status
      ));
  }

  if (!mHaveSvsmProtocol) {
    Status = VirtMmHwVirtMap ();
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: VirtMm%aVirtMap failed. Status: %r\n",
      __func__,
      mHaveSvsmProtocol ? "Svsm" : "Hw",
      Status
      ));
  }
}

STATIC EFI_GUID *CONST  mGuidedEventGuid[] = {
  &gEfiEndOfDxeEventGroupGuid,
  &gEfiEventExitBootServicesGuid,
  &gEfiEventReadyToBootGuid,
};

STATIC EFI_EVENT  mGuidedEvent[ARRAY_SIZE (mGuidedEventGuid)];

/**
  Event notification that is fired when GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
STATIC
VOID
EFIAPI
VirtMmGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_MM_COMMUNICATE_HEADER  Header;
  UINTN                      Size;

  //
  // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&Header.HeaderGuid, Context);
  Header.MessageLength = 1;
  Header.Data[0]       = 0;

  Size = sizeof (Header);
  VirtMmCommunication2Communicate (&mMmCommunication2, &Header, &Header, &Size);
}

/**
  The Entry Point for MM Communication

  This function installs the MM communication protocol interface and finds out
  what type of buffer management will be required prior to invoking the
  communication SMC.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
VirtMmCommunication2Initialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if (FeaturePcdGet (PcdEnableVariableRuntimeCache)) {
    ASSERT (!"Variable driver runtime cache is not supported.\n");
    CpuDeadLoop ();
  }

  if (FeaturePcdGet (PcdVariableCollectStatistics)) {
    ASSERT (!"Variable driver statistics are not supported.\n");
    CpuDeadLoop ();
  }

  mCommunicateBuffer = AllocateRuntimePages (EFI_SIZE_TO_PAGES (MAX_BUFFER_SIZE));
  if (!mCommunicateBuffer) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ReturnErrorStatus;
  }

  mCommunicateBufferPhys = (EFI_PHYSICAL_ADDRESS)(UINTN)(mCommunicateBuffer);

  if (VirtMmSvsmProbe ()) {
    mHaveSvsmProtocol = TRUE;
    Status            = EFI_SUCCESS;
  } else {
    Status = VirtMmHwInit ();
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to init %a: %r\n",
      __func__,
      mHaveSvsmProtocol ? "SVSM" : "HW",
      Status
      ));
    goto FreeBufferPages;
  }

  // Install the communication protocol
  Status = gBS->InstallProtocolInterface (
                  &mMmCommunicateHandle,
                  &gEfiMmCommunication2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication2
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install MM communication protocol\n",
      __func__
      ));
    goto FreeBufferPages;
  }

  Status = gBS->InstallProtocolInterface (
                  &mSmmVariableHandle,
                  &gEfiSmmVariableProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface (
                  &mSmmVariableWriteHandle,
                  &gSmmVariableWriteGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  // Register notification callback when virtual address is associated
  // with the physical address.
  // Create a Set Virtual Address Map event.
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  VirtMmNotifySetVirtualAddressMap,
                  NULL,
                  &mSetVirtualAddressMapEvent
                  );
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < ARRAY_SIZE (mGuidedEventGuid); Index++) {
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    VirtMmGuidedEventNotify,
                    mGuidedEventGuid[Index],
                    mGuidedEventGuid[Index],
                    &mGuidedEvent[Index]
                    );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      while (Index-- > 0) {
        gBS->CloseEvent (mGuidedEvent[Index]);
      }

      goto UninstallProtocol;
    }
  }

  return EFI_SUCCESS;

UninstallProtocol:
  gBS->UninstallProtocolInterface (
         mMmCommunicateHandle,
         &gEfiMmCommunication2ProtocolGuid,
         &mMmCommunication2
         );

FreeBufferPages:
  FreePages (mCommunicateBuffer, EFI_SIZE_TO_PAGES (MAX_BUFFER_SIZE));

ReturnErrorStatus:
  if (FeaturePcdGet (PcdQemuVarsRequire)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: qemu uefi variable service is not available, use \n"
 #if defined (MDE_CPU_X64)
      "  'qemu-system-x86_64 -device uefi-vars-x64'\n"
 #elif defined (MDE_CPU_AARCH64)
      "  'qemu-system-aarch64 -device uefi-vars-sysbus'\n"
 #elif defined (MDE_CPU_RISCV64)
      "  'qemu-system-riscv64 -device uefi-vars-sysbus'\n"
 #else
      #error unsupported architecture
 #endif
      ));
    CpuDeadLoop ();
  }

  return EFI_INVALID_PARAMETER;
}

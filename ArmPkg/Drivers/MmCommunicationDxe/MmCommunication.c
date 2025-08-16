/** @file

  Copyright (c) 2016-2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/ArmLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/MmCommunication2.h>
#include <Protocol/MmCommunication3.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/MmCommunicate.h>

//
// Partition ID if FF-A support is enabled
//
STATIC UINT16  mPartId;
STATIC UINT16  mStMmPartId;

//
// Address, Length of the pre-allocated buffer for communication with the secure
// world.
//
STATIC ARM_MEMORY_REGION_DESCRIPTOR  mNsCommBuffMemRegion;

// Notification event when virtual address map is set.
STATIC EFI_EVENT  mSetVirtualAddressMapEvent;

//
// Handle to install the MM Communication Protocol
//
STATIC EFI_HANDLE  mMmCommunicateHandle;

/**
  Send mm communicate request via FF-A.

  @retval EFI_SUCCESS
  @retval Others                   Error.

**/
STATIC
EFI_STATUS
EFIAPI
SendFfaMmCommunicate (
  IN VOID
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  CommunicateArgs;

  ZeroMem (&CommunicateArgs, sizeof (DIRECT_MSG_ARGS));

  CommunicateArgs.Arg0 = (UINTN)mNsCommBuffMemRegion.PhysicalBase;

  Status = ArmFfaLibMsgSendDirectReq (
             mStMmPartId,
             0,
             &CommunicateArgs
             );

  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the StMM SP since it is UP.
    Status = ArmFfaLibRun (mStMmPartId, 0x00);
  }

  return Status;
}

/**
  Convert SmcMmRet value to EFI_STATUS.

  @param[in] SmcMmRet              Mm return code

  @retval EFI_SUCCESS
  @retval Others                   Error status correspond to SmcMmRet

**/
STATIC
EFI_STATUS
SmcMmRetToEfiStatus (
  IN UINTN  SmcMmRet
  )
{
  switch ((UINT32)SmcMmRet) {
    case ARM_SMC_MM_RET_SUCCESS:
      return EFI_SUCCESS;
    case ARM_SMC_MM_RET_INVALID_PARAMS:
      return EFI_INVALID_PARAMETER;
    case ARM_SMC_MM_RET_DENIED:
      return EFI_ACCESS_DENIED;
    case ARM_SMC_MM_RET_NO_MEMORY:
      return EFI_OUT_OF_RESOURCES;
    default:
      return EFI_ACCESS_DENIED;
  }
}

/**
  Send mm communicate request via SPM_MM.

  @retval EFI_SUCCESS
  @retval Others                   Error.

**/
STATIC
EFI_STATUS
EFIAPI
SendSpmMmCommunicate (
  IN VOID
  )
{
  ARM_SMC_ARGS  CommunicateSmcArgs;

  ZeroMem (&CommunicateSmcArgs, sizeof (ARM_SMC_ARGS));

  // SMC Function ID
  CommunicateSmcArgs.Arg0 = ARM_SMC_ID_MM_COMMUNICATE_AARCH64;

  // Cookie
  CommunicateSmcArgs.Arg1 = 0;

  // comm_buffer_address (64-bit physical address)
  CommunicateSmcArgs.Arg2 = (UINTN)mNsCommBuffMemRegion.PhysicalBase;

  // comm_size_address (not used, indicated by setting to zero)
  CommunicateSmcArgs.Arg3 = 0;

  // Call the Standalone MM environment.
  ArmCallSmc (&CommunicateSmcArgs);

  return SmcMmRetToEfiStatus (CommunicateSmcArgs.Arg0);
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

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
STATIC
EFI_STATUS
EFIAPI
MmCommunicationCommon (
  IN OUT VOID   *CommBufferPhysical,
  IN OUT VOID   *CommBufferVirtual,
  IN OUT UINTN  *CommSize OPTIONAL
  )
{
  EFI_MM_COMMUNICATE_HEADER     *CommunicateHeader;
  EFI_MM_COMMUNICATE_HEADER_V3  *CommunicateHeaderV3;
  UINTN                         BufferSize;
  UINTN                         *MessageSize;
  UINTN                         HeaderSize;
  EFI_STATUS                    Status;

  Status     = EFI_ACCESS_DENIED;
  BufferSize = 0;

  //
  // Check parameters
  //
  if ((CommBufferVirtual == NULL) || (CommBufferPhysical == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  CommunicateHeader   = CommBufferVirtual;
  CommunicateHeaderV3 = NULL;
  // CommBuffer is a mandatory parameter. Hence, Rely on
  // MessageLength + Header to ascertain the
  // total size of the communication payload rather than
  // rely on optional CommSize parameter
  if (CompareGuid (
        &CommunicateHeader->HeaderGuid,
        &gEfiMmCommunicateHeaderV3Guid
        ))
  {
    CommunicateHeaderV3 = (EFI_MM_COMMUNICATE_HEADER_V3 *)CommBufferVirtual;
    BufferSize          = CommunicateHeaderV3->BufferSize;
    MessageSize         = &CommunicateHeaderV3->MessageSize;
    HeaderSize          = sizeof (EFI_MM_COMMUNICATE_HEADER_V3);
  } else {
    BufferSize = CommunicateHeader->MessageLength +
                 sizeof (CommunicateHeader->HeaderGuid) +
                 sizeof (CommunicateHeader->MessageLength);
    MessageSize = &CommunicateHeader->MessageLength;
    HeaderSize  = sizeof (CommunicateHeader->HeaderGuid) +
                  sizeof (CommunicateHeader->MessageLength);
  }

  // If CommSize is not omitted, perform size inspection before proceeding.
  if (CommSize != NULL) {
    // This case can be used by the consumer of this driver to find out the
    // max size that can be used for allocating CommBuffer.
    if ((*CommSize == 0) ||
        (*CommSize > mNsCommBuffMemRegion.Length))
    {
      *CommSize = mNsCommBuffMemRegion.Length;
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
  if ((*MessageSize == 0) ||
      (BufferSize > mNsCommBuffMemRegion.Length))
  {
    *MessageSize = mNsCommBuffMemRegion.Length - HeaderSize;
    Status       = EFI_BAD_BUFFER_SIZE;
  }

  // MessageLength or CommSize check has failed, return here.
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Copy Communication Payload
  CopyMem ((VOID *)mNsCommBuffMemRegion.VirtualBase, CommBufferVirtual, BufferSize);

  if (IsFfaSupported ()) {
    Status = SendFfaMmCommunicate ();
  } else {
    Status = SendSpmMmCommunicate ();
  }

  if (!EFI_ERROR (Status)) {
    ZeroMem (CommBufferVirtual, BufferSize);
    // On successful return, the size of data being returned is inferred from
    // MessageLength + Header.
    CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)mNsCommBuffMemRegion.VirtualBase;
    if ((CommunicateHeaderV3 != NULL) && !CompareGuid (
                                            &CommunicateHeader->HeaderGuid,
                                            &gEfiMmCommunicateHeaderV3Guid
                                            ))
    {
      // Sanity check to make sure we are still using v3
      DEBUG ((
        DEBUG_ERROR,
        "%a Expected v3 header, but got v1 header! %g\n",
        __func__,
        &CommunicateHeader->HeaderGuid
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (CommunicateHeaderV3 != NULL) {
      CommunicateHeaderV3 = (EFI_MM_COMMUNICATE_HEADER_V3 *)CommunicateHeader;
      BufferSize          = CommunicateHeaderV3->BufferSize;
    } else {
      BufferSize = CommunicateHeader->MessageLength +
                   sizeof (CommunicateHeader->HeaderGuid) +
                   sizeof (CommunicateHeader->MessageLength);
    }

    if (BufferSize > mNsCommBuffMemRegion.Length) {
      // Something bad has happened, we should have landed in ARM_SMC_MM_RET_NO_MEMORY
      Status = EFI_BAD_BUFFER_SIZE;
      DEBUG ((
        DEBUG_ERROR,
        "%a Returned buffer exceeds communication buffer limit. Has: 0x%llx vs. max: 0x%llx!\n",
        __func__,
        BufferSize,
        (UINTN)mNsCommBuffMemRegion.Length
        ));
    } else {
      CopyMem (
        CommBufferVirtual,
        (VOID *)mNsCommBuffMemRegion.VirtualBase,
        BufferSize
        );
    }
  }

  return Status;
}

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
MmCommunication2Communicate (
  IN CONST EFI_MM_COMMUNICATION2_PROTOCOL  *This,
  IN OUT VOID                              *CommBufferPhysical,
  IN OUT VOID                              *CommBufferVirtual,
  IN OUT UINTN                             *CommSize OPTIONAL
  )
{
  return MmCommunicationCommon (
           CommBufferPhysical,
           CommBufferVirtual,
           CommSize
           );
}

EFI_STATUS
EFIAPI
MmCommunication3Communicate (
  IN CONST EFI_MM_COMMUNICATION3_PROTOCOL  *This,
  IN OUT VOID                              *CommBufferPhysical,
  IN OUT VOID                              *CommBufferVirtual
  )
{
  return MmCommunicationCommon (
           CommBufferPhysical,
           CommBufferVirtual,
           NULL
           );
}

//
// MM Communication Protocol instance
//
STATIC EFI_MM_COMMUNICATION2_PROTOCOL  mMmCommunication2 = {
  MmCommunication2Communicate
};

//
// MM Communication Protocol instance
//
STATIC EFI_MM_COMMUNICATION3_PROTOCOL  mMmCommunication3 = {
  MmCommunication3Communicate
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
NotifySetVirtualAddressMap (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gRT->ConvertPointer (
                  EFI_OPTIONAL_PTR,
                  (VOID **)&mNsCommBuffMemRegion.VirtualBase
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "NotifySetVirtualAddressMap():"
      " Unable to convert MM runtime pointer. Status:0x%r\n",
      Status
      ));
  }
}

/**
  Check mm communication compatibility when use SPM_MM.

**/
STATIC
EFI_STATUS
EFIAPI
GetMmCompatibility (
  VOID
  )
{
  EFI_STATUS    Status;
  UINT32        MmVersion;
  ARM_SMC_ARGS  MmVersionArgs;

  // MM_VERSION uses SMC32 calling conventions
  MmVersionArgs.Arg0 = ARM_SMC_ID_MM_VERSION_AARCH32;

  ArmCallSmc (&MmVersionArgs);

  if (MmVersionArgs.Arg0 == ARM_SMC_MM_RET_NOT_SUPPORTED) {
    return EFI_UNSUPPORTED;
  }

  MmVersion = MmVersionArgs.Arg0;

  if ((MM_MAJOR_VER (MmVersion) == MM_CALLER_MAJOR_VER) &&
      (MM_MINOR_VER (MmVersion) >= MM_CALLER_MINOR_VER))
  {
    DEBUG ((
      DEBUG_INFO,
      "MM Version: Major=0x%x, Minor=0x%x\n",
      MM_MAJOR_VER (MmVersion),
      MM_MINOR_VER (MmVersion)
      ));
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "Incompatible MM Versions.\n Current Version: Major=0x%x, Minor=0x%x.\n Expected: Major=0x%x, Minor>=0x%x.\n",
      MM_MAJOR_VER (MmVersion),
      MM_MINOR_VER (MmVersion),
      MM_CALLER_MAJOR_VER,
      MM_CALLER_MINOR_VER
      ));
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  Check mm communication compatibility when use FF-A.

**/
STATIC
EFI_STATUS
EFIAPI
GetFfaCompatibility (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      CurrentMajorVersion;
  UINT16      CurrentMinorVersion;

  Status = ArmFfaLibGetVersion (
             ARM_FFA_MAJOR_VERSION,
             ARM_FFA_MINOR_VERSION,
             &CurrentMajorVersion,
             &CurrentMinorVersion
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get FF-A version. Status: %r\n", Status));
    return EFI_UNSUPPORTED;
  }

  if ((ARM_FFA_MAJOR_VERSION != CurrentMajorVersion) ||
      (ARM_FFA_MINOR_VERSION > CurrentMinorVersion))
  {
    DEBUG ((
      DEBUG_ERROR,
      "Incompatible FF-A Versions for MM_COMM.\n" \
      "Request Version: Major=0x%x, Minor=0x%x.\n" \
      "Current Version: Major=0x%x, Minor>=0x%x.\n",
      ARM_FFA_MAJOR_VERSION,
      ARM_FFA_MINOR_VERSION,
      CurrentMajorVersion,
      CurrentMinorVersion
      ));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_INFO,
    "FF-A Version for MM_COMM: Major=0x%x, Minor=0x%x\n",
    CurrentMajorVersion,
    CurrentMinorVersion
    ));

  return EFI_SUCCESS;
}

/**
  Initialize communication via FF-A.

**/
STATIC
EFI_STATUS
EFIAPI
InitializeFfaCommunication (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_FFA_PART_INFO_DESC  StmmPartInfo;

  Status = ArmFfaLibPartitionIdGet (&mPartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get partition id. Status: %r\n",
      Status
      ));
    return Status;
  }

  Status = ArmFfaLibGetPartitionInfo (&gEfiMmCommunication2ProtocolGuid, &StmmPartInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get partition Info(%g). Status: %r\n",
      &gEfiMmCommunication2ProtocolGuid,
      Status
      ));
    return Status;
  }

  if ((StmmPartInfo.PartitionProps & FFA_PART_PROP_RECV_DIRECT_REQ) == 0x00) {
    Status = EFI_UNSUPPORTED;
    DEBUG ((DEBUG_ERROR, "StandaloneMm doesn't receive DIRECT_MSG_REQ...\n"));
    return Status;
  }

  mStMmPartId = StmmPartInfo.PartitionId;

  return EFI_SUCCESS;
}

/**
 Initialize mm communication.

**/
STATIC
EFI_STATUS
EFIAPI
InitializeCommunication (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;

  if (IsFfaSupported ()) {
    Status = GetFfaCompatibility ();
    if (!EFI_ERROR (Status)) {
      Status = InitializeFfaCommunication ();
    }
  } else {
    Status = GetMmCompatibility ();
    // No further initialisation required for SpmMM
  }

  return Status;
}

STATIC EFI_GUID *CONST  mGuidedEventGuid[] = {
  &gEfiEndOfDxeEventGroupGuid,
  &gEfiEventExitBootServicesGuid,
  &gEfiEventReadyToBootGuid,
};

STATIC EFI_EVENT  mGuidedEvent[ARRAY_SIZE (mGuidedEventGuid)];

/**
  Event notification that is fired when ReadyToLockProtocol is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
MMReadyToLockProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_MM_COMMUNICATE_HEADER  Header;
  UINTN                      Size;

  //
  // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure
  //
  CopyGuid (&Header.HeaderGuid, &gEfiDxeMmReadyToLockProtocolGuid);
  Header.MessageLength = 1;
  Header.Data[0]       = 0;

  Size = sizeof (Header);
  MmCommunication2Communicate (&mMmCommunication2, &Header, &Header, &Size);
}

/**
  Event notification that is fired when GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
STATIC
VOID
EFIAPI
MmGuidedEventNotify (
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
  MmCommunication2Communicate (&mMmCommunication2, &Header, &Header, &Size);
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
MmCommunication2Initialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  // Initialize to make mm communication
  Status = InitializeCommunication ();
  if (EFI_ERROR (Status)) {
    goto ReturnErrorStatus;
  }

  mNsCommBuffMemRegion.PhysicalBase = PcdGet64 (PcdMmBufferBase);
  // During boot , Virtual and Physical are same
  mNsCommBuffMemRegion.VirtualBase = mNsCommBuffMemRegion.PhysicalBase;
  mNsCommBuffMemRegion.Length      = PcdGet64 (PcdMmBufferSize);

  ASSERT (mNsCommBuffMemRegion.PhysicalBase != 0);

  ASSERT (mNsCommBuffMemRegion.Length != 0);

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeReserved,
                  mNsCommBuffMemRegion.PhysicalBase,
                  mNsCommBuffMemRegion.Length,
                  EFI_MEMORY_WB |
                  EFI_MEMORY_XP |
                  EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MmCommunicateInitialize: "
      "Failed to add MM-NS Buffer Memory Space\n"
      ));
    goto ReturnErrorStatus;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  mNsCommBuffMemRegion.PhysicalBase,
                  mNsCommBuffMemRegion.Length,
                  EFI_MEMORY_WB | EFI_MEMORY_XP | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MmCommunicateInitialize: "
      "Failed to set MM-NS Buffer Memory attributes\n"
      ));
    goto CleanAddedMemorySpace;
  }

  // Install the communication protocol
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mMmCommunicateHandle,
                  &gEfiMmCommunication2ProtocolGuid,
                  &mMmCommunication2,
                  &gEfiMmCommunication3ProtocolGuid,
                  &mMmCommunication3,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MmCommunicationInitialize: "
      "Failed to install MM communication protocol\n"
      ));
    goto CleanAddedMemorySpace;
  }

  // Register notification callback when virtual address is associated
  // with the physical address.
  // Create a Set Virtual Address Map event.
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  NotifySetVirtualAddressMap,
                  NULL,
                  &mSetVirtualAddressMapEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = EfiNamedEventListen (
             &gEfiDxeMmReadyToLockProtocolGuid,
             TPL_NOTIFY,
             MMReadyToLockProtocolNotify,
             NULL,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < ARRAY_SIZE (mGuidedEventGuid); Index++) {
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    MmGuidedEventNotify,
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

CleanAddedMemorySpace:
  gDS->RemoveMemorySpace (
         mNsCommBuffMemRegion.PhysicalBase,
         mNsCommBuffMemRegion.Length
         );

ReturnErrorStatus:
  return EFI_INVALID_PARAMETER;
}

/** @file

  Copyright (c) 2016-2021, Arm Limited.
  Copyright (c) 2023, Intel Corporation, All rights reserved.<BR>
  Copyright (c) 2024, Rivos Inc, All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include "Library/BaseRiscVSbiLib.h"
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeRiscvMpxy.h>
#include <Uefi/UefiBaseType.h>

#include <Protocol/MmCommunication2.h>
#include <Protocol/FdtClient.h>
#include <Guid/MmCommBuffer.h>

#include "MmCommunicate.h"

//
// Address, Length of the pre-allocated buffer for communication with the secure
// world.
//
STATIC RISCV_SMM_MEM_REGION_DESCRIPTOR  mNsCommBuffMemRegion;

/**
      +------------------------+
      | MM_COMM_BUFFER_STATUS  |
      |                        <----+
      +------------------------+    |
      |                        +----+
Page 0|    CommBuffer Struct   |
      +------------------------+
      |                        |
      |                        |
      |    NS Comm Buffer      |
Page 1|..N                     |
      +------------------------+

  Steal one page from allocated MMCommBuffer and use it
  for MM_COMM_BUFFER
*/
//
// Handle to install the MM Communication Protocol
//
STATIC EFI_HANDLE             mMmCommunicateHandle;
STATIC MM_COMM_BUFFER         *MmCommonBuffer;
STATIC MM_COMM_BUFFER_STATUS  *mCommunicationStatus;

struct GuidMapping {
  char      NodeName[30];
  GUID      *ServiceGuid;
  UINT32    ChannelId;
};

#define MM_STR  "riscv,sbi-mpxy-mm"

STATIC struct GuidMapping  GuidChidArray[] = {
  {
    MM_STR, &gMmHestGetErrorSourceInfoGuid, 0
  },
  {
    MM_STR, &gEfiSmmVariableProtocolGuid, 0
  },
  {
    MM_STR, &gVarCheckPolicyLibMmiHandlerGuid, 0
  }
};

/**
  Retrieve the channel ID for a given GUID.

  This function searches for a mapping between the provided GUID and a channel ID
  in the `GuidChidArray`.

  @param[in]  GuidStr     Pointer to the GUID for which the channel ID is required.
  @param[out] ChannelId   Pointer to a variable that will receive the channel ID.

  @retval EFI_SUCCESS     The channel ID was found and returned successfully.
  @retval EFI_NO_MAPPING  No mapping for the provided GUID was found.
**/
STATIC
EFI_STATUS
EFIAPI
GetChannelForGuid (
  GUID        *GuidStr,
  OUT UINT32  *ChannelId
  )
{
  for (UINT8 Index = 0; Index < sizeof (GuidChidArray)/ sizeof (struct GuidMapping); Index++) {
    if (CompareGuid (GuidChidArray[Index].ServiceGuid, GuidStr)) {
      *ChannelId = GuidChidArray[Index].ChannelId;
      return EFI_SUCCESS;
    }
  }

  return EFI_NO_MAPPING;
}

/**
  Retrieve the channel ID from the device tree (DT) for a given node name.

  This function searches the device tree for a node compatible with the provided
  string and retrieves the associated channel ID.

  @param[in]  FdtClient   Pointer to the FDT client protocol.
  @param[in]  MatchStr    String to match a compatible node in the device tree.
  @param[out] ChannelId   Pointer to a variable that will receive the channel ID.

  @retval EFI_SUCCESS     The channel ID was found and returned successfully.
  @retval EFI_NOT_FOUND   No matching node or property was found in the device tree.
**/
STATIC
EFI_STATUS
EFIAPI
GetDTChannelForGuid (
  FDT_CLIENT_PROTOCOL  *FdtClient,
  IN CHAR8             *MatchStr,
  OUT UINT32           *ChannelId
  )
{
  EFI_STATUS    Status;
  INT32         Node;
  CONST UINT64  *Reg;
  UINT32        RegSize;

  Status = FdtClient->FindCompatibleNode (FdtClient, MatchStr, &Node);
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_WARN,
       "%a: No compatible DT node found\n",
       __func__
      )
      );
    return EFI_NOT_FOUND;
  }

  Status = FdtClient->GetNodeProperty (
                        FdtClient,
                        Node,
                        "riscv,sbi-mpxy-channel-id",
                        (CONST VOID **)&Reg,
                        &RegSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_WARN,
       "%a: No 'riscv,sbi-mpxy-channel-id' compatible DT node found\n",
       __func__
      )
      );
    return EFI_NOT_FOUND;
  } else {
    ASSERT (RegSize == 4);
    *ChannelId = SwapBytes32 (Reg[0]);
    return EFI_SUCCESS;
  }
}

/**
  Populate channel ID information from the device tree.

  This function iterates through `GuidChidArray` and attempts to retrieve channel
  IDs from the device tree for each GUID in the array. If a mapping is found, the
  channel ID is updated in the array.

  @retval EFI_SUCCESS     All channel IDs were processed successfully.
  @retval Other           One or more errors occurred during processing.
**/
STATIC
EFI_STATUS
EFIAPI
FillMmMpxyChannelIdInfo (
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  UINT32               ChannelId;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  ChannelId = 0;
  for (UINT8 Index = 0; Index < sizeof (GuidChidArray)/ sizeof (struct GuidMapping); Index++) {
    Status = GetDTChannelForGuid (FdtClient, GuidChidArray[Index].NodeName, &ChannelId);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Found Channel %d for Guid Idx %d", ChannelId, Index));
      GuidChidArray[Index].ChannelId = ChannelId;
      ChannelId                      = 0; // Get ready for next node
    } else {
      DEBUG ((DEBUG_INFO, "No Channel Mapping Found Guid Idx %d", Index));
    }
  }

  return EFI_SUCCESS;
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
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  RISCV_SMM_MSG_COMM_ARGS    CommunicateArgs;
  RISCV_SMM_MSG_COMM_RESP    CommunicateResp;
  EFI_STATUS                 Status;
  UINTN                      BufferSize;
  UINTN                      MmRespLen;
  UINT32                     ChannelId;

  ChannelId  = 0;
  Status     = EFI_ACCESS_DENIED;
  BufferSize = 0;

  // For now i/p and o/p offsets are same at offset 0.
  ZeroMem (&CommunicateArgs, sizeof (RISCV_SMM_MSG_COMM_ARGS));

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
  if ((CommunicateHeader->MessageLength == 0) ||
      (BufferSize > mNsCommBuffMemRegion.Length))
  {
    CommunicateHeader->MessageLength = mNsCommBuffMemRegion.Length -
                                       sizeof (CommunicateHeader->HeaderGuid) -
                                       sizeof (CommunicateHeader->MessageLength);
    Status = EFI_BAD_BUFFER_SIZE;
  }

  // MessageLength or CommSize check has failed, return here.
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Copy Communication Payload
  CopyMem ((VOID *)mNsCommBuffMemRegion.VirtualBase, CommBufferVirtual, BufferSize);

  Status = GetChannelForGuid (&CommunicateHeader->HeaderGuid, &ChannelId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "No Channel Mapping Found For Requested Service"));
    return Status;
  }

  mCommunicationStatus->IsCommBufferValid = 1;
  // Call the Standalone MM environment.
  Status = SbiMpxySendMessage (
             ChannelId,
             RISCV_MSG_ID_SMM_COMMUNICATE,
             (VOID *)&CommunicateArgs,
             sizeof (RISCV_SMM_MSG_COMM_ARGS),
             (VOID *)&CommunicateResp,
             &MmRespLen
             );

  mCommunicationStatus->IsCommBufferValid = 0;
  if (EFI_ERROR (Status) || (0 == MmRespLen)) {
    return Status;
  }

  switch (mCommunicationStatus->ReturnStatus) {
    case RISCV_SMM_RET_SUCCESS:
      ZeroMem (CommBufferVirtual, BufferSize);
      // On successful return, the size of data being returned is inferred from
      // MessageLength + Header.
      CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)mNsCommBuffMemRegion.VirtualBase;
      BufferSize        = CommunicateHeader->MessageLength +
                          sizeof (CommunicateHeader->HeaderGuid) +
                          sizeof (CommunicateHeader->MessageLength);

      CopyMem (
        CommBufferVirtual,
        (VOID *)mNsCommBuffMemRegion.VirtualBase,
        BufferSize
        );
      Status = EFI_SUCCESS;
      break;

    case RISCV_SMM_RET_INVALID_PARAMS:
      Status = EFI_INVALID_PARAMETER;
      break;

    case RISCV_SMM_RET_DENIED:
      Status = EFI_ACCESS_DENIED;
      break;

    case RISCV_SMM_RET_NO_MEMORY:
      // Unexpected error since the CommSize was checked for zero length
      // prior to issuing the SMC
      Status = EFI_OUT_OF_RESOURCES;
      ASSERT (0);
      break;

    default:
      Status = EFI_ACCESS_DENIED;
      ASSERT (0);
  }

  return Status;
}

//
// MM Communication Protocol instance
//
STATIC EFI_MM_COMMUNICATION2_PROTOCOL  mMmCommunication2 = {
  MmCommunication2Communicate
};

/**
  Check MM compatibility with the given channel ID.

  This function retrieves the MM version and checks if it is compatible with the caller.
  It also initializes the memory region for the MM communication buffer.

  @param[in]  ChannelId    The channel ID used to communicate with the MM environment.

  @retval EFI_SUCCESS      The MM environment is compatible, and the memory region is initialized.
  @retval EFI_UNSUPPORTED  The MM version is not compatible.
  @retval Other            An error occurred during compatibility checking or initialization.
**/
STATIC
EFI_STATUS
GetMmCompatibility (
  UINT8  ChannelId
  )
{
  EFI_STATUS         Status;
  UINT32             MmVersion;
  MM_GET_ATTRIBUTES  MmVersionArgs;
  UINTN              MmRespLen;
  UINT64             MmShmemAddr;

  Status = SbiMpxySendMessage (
             ChannelId,
             RISCV_MSG_ID_SMM_GET_ATTRIBUTES,
             (VOID *)&MmVersionArgs,
             sizeof (MM_GET_ATTRIBUTES),
             (VOID *)&MmVersionArgs,
             &MmRespLen
             );

  if (EFI_ERROR (Status) || (0 == MmRespLen)) {
    return Status;
  }

  MmVersion = MmVersionArgs.mmVersion;

  if ((MM_MAJOR_VER (MmVersion) == MM_CALLER_MAJOR_VER) &&
      (MM_MINOR_VER (MmVersion) >= MM_CALLER_MINOR_VER))
  {
    DEBUG (
      (
       DEBUG_INFO,
       "MM Version: Major=0x%x, Minor=0x%x\n",
       MM_MAJOR_VER (MmVersion),
       MM_MINOR_VER (MmVersion)
      )
      );
  } else {
    DEBUG (
      (
       DEBUG_ERROR,
       "Incompatible MM Versions.\n Current Version: Major=0x%x, Minor=0x%x.\n Expected: Major=0x%x, Minor>=0x%x.\n",
       MM_MAJOR_VER (MmVersion),
       MM_MINOR_VER (MmVersion),
       MM_CALLER_MAJOR_VER,
       MM_CALLER_MINOR_VER
      )
      );
    return EFI_UNSUPPORTED;
  }

  // There are 2 different types of MM related Buffers. 1. Actual MM Shared buffer
  // 2. MmBuffer Status. Status buffer contains IsCommBufferValid flag which is set by
  // the caller and MM side receiver checks this flag befor consuming this buffer. We
  // carve out status buffer from overall.
  MmShmemAddr = ((UINT64)MmVersionArgs.mmShmemaddrHigh) << 32 | (UINT64)MmVersionArgs.mmShmemAddrLow;

  mNsCommBuffMemRegion.PhysicalBase = MmShmemAddr + EFI_PAGE_SIZE;

  // During boot , Virtual and Physical are same
  mNsCommBuffMemRegion.VirtualBase = mNsCommBuffMemRegion.PhysicalBase;
  mNsCommBuffMemRegion.Length      = MmVersionArgs.mmShmemSize - EFI_PAGE_SIZE;

  MmCommonBuffer = (MM_COMM_BUFFER *)MmShmemAddr;

  MmCommonBuffer->NumberOfPages           = EFI_SIZE_TO_PAGES (mNsCommBuffMemRegion.PhysicalBase);
  MmCommonBuffer->PhysicalStart           = mNsCommBuffMemRegion.VirtualBase;
  MmCommonBuffer->Status                  = MmShmemAddr + sizeof (MM_COMM_BUFFER);
  mCommunicationStatus                    = (MM_COMM_BUFFER_STATUS *)MmCommonBuffer->Status;
  mCommunicationStatus->IsCommBufferValid = 0;

  return EFI_SUCCESS;
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
  This function installs the MM communication protocol interface

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

  Status = FillMmMpxyChannelIdInfo ();

  for (UINT8 Index = 0; Index < sizeof (GuidChidArray)/ sizeof (struct GuidMapping); Index++) {
    if (GuidChidArray[Index].ChannelId == 0) {
      // TODO: Channel Id 0 is invalid. Spec??
      continue;
    }

    Status = SbiMpxyInit ();
    if (EFI_ERROR (Status)) {
      DEBUG (
        (
         DEBUG_ERROR,
         "InitRiscVSmmArgs: "
         "Failed to init MPXY\n"
        )
        );
      ASSERT (0);
    }

    // Check if we can make the MM call
    Status = GetMmCompatibility (GuidChidArray[Index].ChannelId);
    if (EFI_ERROR (Status)) {
      goto ReturnErrorStatus;
    }
  }

  // Install the communication protocol
  Status = gBS->InstallProtocolInterface (
                  &mMmCommunicateHandle,
                  &gEfiMmCommunication2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication2
                  );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "MmCommunicationInitialize: "
       "Failed to install MM communication protocol\n"
      )
      );
    goto CleanAddedMemorySpace;
  }

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

  return Status;

ReturnErrorStatus:
  return EFI_INVALID_PARAMETER;
}

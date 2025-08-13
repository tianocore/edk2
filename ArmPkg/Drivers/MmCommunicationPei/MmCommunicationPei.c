/** @file -- MmCommunicationPei.c
  Provides an interface to send MM request in PEI

  Copyright (c) 2016-2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/MmCommunicate.h>

#include <Protocol/MmCommunication.h>
#include <Ppi/MmCommunication.h>
#include <Ppi/MmCommunication3.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>

//
// Partition ID if FF-A support is enabled
//
STATIC UINT16  mPartId;
STATIC UINT16  mStMmPartId;

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
    return EFI_UNSUPPORTED;
  }

  if ((ARM_FFA_MAJOR_VERSION != CurrentMajorVersion) ||
      (ARM_FFA_MINOR_VERSION > CurrentMinorVersion))
  {
    DEBUG ((
      DEBUG_INFO,
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
  VOID                    *TxBuffer;
  UINT64                  TxBufferSize;
  VOID                    *RxBuffer;
  UINT64                  RxBufferSize;
  EFI_FFA_PART_INFO_DESC  *StmmPartInfo;
  UINT32                  Count;
  UINT32                  Size;

  Status = ArmFfaLibPartitionIdGet (&mPartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get partition id. Status: %r\n",
      Status
      ));
    return Status;
  }

  Status = ArmFfaLibGetRxTxBuffers (
             &TxBuffer,
             &TxBufferSize,
             &RxBuffer,
             &RxBufferSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get Rx/Tx Buffer. Status: %r\n",
      Status
      ));
    return Status;
  }

  Status = ArmFfaLibPartitionInfoGet (
             &gEfiMmCommunication2ProtocolGuid,
             FFA_PART_INFO_FLAG_TYPE_DESC,
             &Count,
             &Size
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get Stmm(%g) partition Info. Status: %r\n",
      &gEfiMmCommunication2ProtocolGuid,
      Status
      ));
    return Status;
  }

  if ((Count != 1) || (Size < sizeof (EFI_FFA_PART_INFO_DESC))) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "Invalid partition Info(%g). Count: %d, Size: %d\n",
      &gEfiMmCommunication2ProtocolGuid,
      Count,
      Size
      ));
    goto ErrorHandler;
  }

  StmmPartInfo = (EFI_FFA_PART_INFO_DESC *)RxBuffer;

  if ((StmmPartInfo->PartitionProps & FFA_PART_PROP_RECV_DIRECT_REQ) == 0x00) {
    Status = EFI_UNSUPPORTED;
    DEBUG ((DEBUG_ERROR, "StandaloneMm doesn't receive DIRECT_MSG_REQ...\n"));
    goto ErrorHandler;
  }

  mStMmPartId = StmmPartInfo->PartitionId;

ErrorHandler:
  ArmFfaLibRxRelease (mPartId);
  return Status;
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

/**
  Send mm communicate request via FF-A.

  @retval EFI_SUCCESS
  @retval Others                   Error.

**/
STATIC
EFI_STATUS
EFIAPI
SendFfaMmCommunicate (
  VOID
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  CommunicateArgs;

  ZeroMem (&CommunicateArgs, sizeof (DIRECT_MSG_ARGS));

  CommunicateArgs.Arg0 = (UINTN)PcdGet64 (PcdMmBufferBase);

  Status = ArmFfaLibMsgSendDirectReq (
             mStMmPartId,
             0,
             &CommunicateArgs
             );

  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the StMM SP since it is UP.
    Status = ArmFfaLibRun (mStMmPartId, 0x00, NULL);
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
  VOID
  )
{
  ARM_SMC_ARGS  CommunicateSmcArgs;

  ZeroMem (&CommunicateSmcArgs, sizeof (ARM_SMC_ARGS));

  // SMC Function ID
  CommunicateSmcArgs.Arg0 = ARM_SMC_ID_MM_COMMUNICATE_AARCH64;

  // Cookie
  CommunicateSmcArgs.Arg1 = 0;

  // comm_buffer_address (64-bit physical address)
  CommunicateSmcArgs.Arg2 = (UINTN)PcdGet64 (PcdMmBufferBase);

  // comm_size_address (not used, indicated by setting to zero)
  CommunicateSmcArgs.Arg3 = 0;

  // Call the Standalone MM environment.
  ArmCallSmc (&CommunicateSmcArgs);

  return SmcMmRetToEfiStatus (CommunicateSmcArgs.Arg0);
}

/**
  MmCommunicationPeim
  Communicates with a registered handler.
  This function provides a service to send and receive messages from a registered UEFI service during PEI.

  @param[in]      This            The EFI_PEI_MM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer      Pointer to the data buffer
  @param[in, out] CommSize        The size of the data buffer being passed in. On exit, the
                                  size of data being returned. Zero if the handler does not
                                  wish to reply with any data.

  @retval EFI_SUCCESS             The message was successfully posted.
  @retval EFI_INVALID_PARAMETER   CommBuffer or CommSize was NULL, or *CommSize does not
                                  match MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER).
  @retval EFI_BAD_BUFFER_SIZE     The buffer is too large for the MM implementation.
                                  If this error is returned, the MessageLength field
                                  in the CommBuffer header or the integer pointed by
                                  CommSize, are updated to reflect the maximum payload
                                  size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED       The CommunicateBuffer parameter or CommSize parameter,
                                  if not omitted, are in address range that cannot be
                                  accessed by the MM environment.
**/
STATIC
EFI_STATUS
EFIAPI
MmCommunicationPeimCommon (
  IN OUT VOID   *CommBuffer,
  IN OUT UINTN  *CommSize
  )
{
  EFI_MM_COMMUNICATE_HEADER     *CommunicateHeader;
  EFI_MM_COMMUNICATE_HEADER_V3  *CommunicateHeaderV3;
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  UINTN                         HeaderSize;

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    ASSERT (CommBuffer != NULL);
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader   = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)CommBuffer;
  CommunicateHeaderV3 = NULL;
  if (CompareGuid (
        &CommunicateHeader->HeaderGuid,
        &gEfiMmCommunicateHeaderV3Guid
        ))
  {
    // This is a v3 header
    CommunicateHeaderV3 = (EFI_MM_COMMUNICATE_HEADER_V3 *)(UINTN)CommBuffer;
    HeaderSize          = sizeof (EFI_MM_COMMUNICATE_HEADER_V3);
    BufferSize          = CommunicateHeaderV3->BufferSize;
  } else {
    // This is a v1 header, do some checks
    if (CommSize == NULL) {
      // If CommSize is not NULL, then it must be the size of the header
      // plus the size of the message.
      DEBUG ((
        DEBUG_ERROR,
        "%a CommSize is NULL!\n",
        __func__
        ));
      return EFI_INVALID_PARAMETER;
    }

    // If the length of the CommBuffer is 0 then return the expected length.
    // This case can be used by the consumer of this driver to find out the
    // max size that can be used for allocating CommBuffer.
    if ((*CommSize == 0) || (*CommSize > (UINTN)PcdGet64 (PcdMmBufferSize))) {
      DEBUG ((
        DEBUG_ERROR,
        "%a Invalid CommSize value 0x%llx!\n",
        __func__,
        *CommSize
        ));
      *CommSize = (UINTN)PcdGet64 (PcdMmBufferSize);
      return EFI_BAD_BUFFER_SIZE;
    }

    HeaderSize =  sizeof (CommunicateHeader->HeaderGuid) +
                 sizeof (CommunicateHeader->MessageLength);

    // CommBuffer is a mandatory parameter. Hence, Rely on
    // MessageLength + Header to ascertain the
    // total size of the communication payload rather than
    // rely on optional CommSize parameter
    BufferSize = CommunicateHeader->MessageLength +
                 sizeof (CommunicateHeader->HeaderGuid) +
                 sizeof (CommunicateHeader->MessageLength);
    //
    // If CommSize is supplied it must match MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER);
    //
    if (*CommSize != BufferSize) {
      DEBUG ((
        DEBUG_ERROR,
        "%a Unexpected CommSize value, has: 0x%llx vs. expected: 0x%llx!\n",
        __func__,
        *CommSize,
        BufferSize
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  // Now we know that the size is something we can handle, copy it over to the designated comm buffer.
  CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)(PcdGet64 (PcdMmBufferBase));

  CopyMem (CommunicateHeader, CommBuffer, BufferSize);
  if (IsFfaSupported ()) {
    Status = SendFfaMmCommunicate ();
  } else {
    Status = SendSpmMmCommunicate ();
  }

  if (!EFI_ERROR (Status)) {
    // On successful return, the size of data being returned is inferred from
    // MessageLength + Header.
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

    if (BufferSize > (UINTN)PcdGet64 (PcdMmBufferSize)) {
      // Something bad has happened, we should have landed in ARM_SMC_MM_RET_NO_MEMORY
      Status = EFI_BAD_BUFFER_SIZE;
      DEBUG ((
        DEBUG_ERROR,
        "%a Returned buffer exceeds communication buffer limit. Has: 0x%llx vs. max: 0x%llx!\n",
        __func__,
        BufferSize,
        (UINTN)PcdGet64 (PcdMmBufferSize)
        ));
    } else {
      CopyMem (CommBuffer, CommunicateHeader, BufferSize);
      if (CommSize != NULL) {
        // If CommSize is not NULL, then it must be the size of the header
        // plus the size of the message.
        *CommSize = BufferSize;
      }
    }
  }

  return Status;
}

/**
  MmCommunicationPeim
  Communicates with a registered handler.
  This function provides a service to send and receive messages from a registered UEFI service during PEI.

  @param[in]      This            The EFI_PEI_MM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer      Pointer to the data buffer
  @param[in, out] CommSize        The size of the data buffer being passed in. On exit, the
                                  size of data being returned. Zero if the handler does not
                                  wish to reply with any data.

  @retval EFI_SUCCESS             The message was successfully posted.
  @retval EFI_INVALID_PARAMETER   CommBuffer or CommSize was NULL, or *CommSize does not
                                  match MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER).
  @retval EFI_BAD_BUFFER_SIZE     The buffer is too large for the MM implementation.
                                  If this error is returned, the MessageLength field
                                  in the CommBuffer header or the integer pointed by
                                  CommSize, are updated to reflect the maximum payload
                                  size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED       The CommunicateBuffer parameter or CommSize parameter,
                                  if not omitted, are in address range that cannot be
                                  accessed by the MM environment.
**/
STATIC
EFI_STATUS
EFIAPI
MmCommunicationPeim (
  IN CONST EFI_PEI_MM_COMMUNICATION_PPI  *This,
  IN OUT VOID                            *CommBuffer,
  IN OUT UINTN                           *CommSize
  )
{
  return MmCommunicationPeimCommon (
           CommBuffer,
           CommSize
           );
}

/**
  Communicates with a registered handler through MM communicate v3.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATE3 instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
**/
STATIC
EFI_STATUS
EFIAPI
MmCommunication3Peim (
  IN CONST EFI_PEI_MM_COMMUNICATION3_PPI  *This,
  IN OUT VOID                             *CommBuffer
  )
{
  return MmCommunicationPeimCommon (
           CommBuffer,
           NULL
           );
}

//
// Module globals for the MM Communication PPI
//
STATIC CONST EFI_PEI_MM_COMMUNICATION_PPI  mPeiMmCommunication = {
  MmCommunicationPeim
};

STATIC CONST EFI_PEI_MM_COMMUNICATION3_PPI  mPeiMmCommunication3 = {
  MmCommunication3Peim
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mPeiMmCommunicationPpis[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI),
    &gEfiPeiMmCommunication3PpiGuid,
    (VOID *)&mPeiMmCommunication3
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMmCommunicationPpiGuid,
    (VOID *)&mPeiMmCommunication
  }
};

/**
  Entry point of PEI MM Communication driver

  @param  FileHandle   Handle of the file being invoked.
                       Type EFI_PEI_FILE_HANDLE is defined in FfsFindNextFile().
  @param  PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS  If the interface could be successfully installed
  @retval Others       Returned from PeiServicesInstallPpi()
**/
EFI_STATUS
EFIAPI
MmCommunicationPeiInitialize (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  // Check that our static buffer is looking good.
  // We are using PcdMmBufferBase to transfer variable data.
  // We are not using the full size of the buffer since there is a cost
  // of copying data between Normal and Secure World.
  if ((PcdGet64 (PcdMmBufferBase) == 0) || (PcdGet64 (PcdMmBufferSize) == 0)) {
    ASSERT (PcdGet64 (PcdMmBufferSize) > 0);
    ASSERT (PcdGet64 (PcdMmBufferBase) != 0);
    return EFI_UNSUPPORTED;
  }

  // Check if we can make the MM call
  Status = InitializeCommunication ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return PeiServicesInstallPpi (mPeiMmCommunicationPpis);
}

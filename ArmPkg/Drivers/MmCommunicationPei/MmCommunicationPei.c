/** @file -- MmCommunicationPei.c
  Provides an interface to send MM request in PEI

  Copyright (c) 2016-2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MmCommunicationPei.h"

//
// Module globals
//
EFI_PEI_MM_COMMUNICATION_PPI  mPeiMmCommunication = {
  MmCommunicationPeim
};

EFI_PEI_PPI_DESCRIPTOR  mPeiMmCommunicationPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMmCommunicationPpiGuid,
  &mPeiMmCommunication
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
  return PeiServicesInstallPpi (&mPeiMmCommunicationPpi);
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
  @retval EFI_INVALID_PARAMETER   CommBuffer was NULL or *CommSize does not match
                                  MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER).
  @retval EFI_BAD_BUFFER_SIZE     The buffer is too large for the MM implementation.
                                  If this error is returned, the MessageLength field
                                  in the CommBuffer header or the integer pointed by
                                  CommSize, are updated to reflect the maximum payload
                                  size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED       The CommunicateBuffer parameter or CommSize parameter,
                                  if not omitted, are in address range that cannot be
                                  accessed by the MM environment.
**/
EFI_STATUS
EFIAPI
MmCommunicationPeim (
  IN CONST EFI_PEI_MM_COMMUNICATION_PPI  *This,
  IN OUT VOID                            *CommBuffer,
  IN OUT UINTN                           *CommSize
  )
{
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  ARM_SMC_ARGS               CommunicateSmcArgs;
  EFI_STATUS                 Status;
  UINTN                      BufferSize;

  Status     = EFI_ACCESS_DENIED;
  BufferSize = 0;

  ZeroMem (&CommunicateSmcArgs, sizeof (ARM_SMC_ARGS));

  // Check that our static buffer is looking good.
  // We are using PcdMmBufferBase to transfer variable data.
  // We are not using the full size of the buffer since there is a cost
  // of copying data between Normal and Secure World.
  ASSERT (PcdGet64 (PcdMmBufferSize) > 0 && PcdGet64 (PcdMmBufferBase) != 0);

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // If the length of the CommBuffer is 0 then return the expected length.
  // This case can be used by the consumer of this driver to find out the
  // max size that can be used for allocating CommBuffer.
  if ((CommSize != NULL) && \
      ((*CommSize == 0) || (*CommSize > (UINTN)PcdGet64 (PcdMmBufferSize))))
  {
    *CommSize = (UINTN)PcdGet64 (PcdMmBufferSize);
    return EFI_BAD_BUFFER_SIZE;
  }

  CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)(PcdGet64 (PcdMmBufferBase));

  CopyMem ((VOID *)CommunicateHeader, CommBuffer, *CommSize);

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
  if ((CommSize != NULL) && (*CommSize != BufferSize)) {
    return EFI_INVALID_PARAMETER;
  }

  // SMC Function ID
  CommunicateSmcArgs.Arg0 = ARM_SMC_ID_MM_COMMUNICATE_AARCH64;

  // Cookie
  CommunicateSmcArgs.Arg1 = 0;

  // comm_buffer_address (64-bit physical address)
  CommunicateSmcArgs.Arg2 = (UINTN)CommunicateHeader;

  // comm_size_address (not used, indicated by setting to zero)
  CommunicateSmcArgs.Arg3 = 0;

  // Call the Standalone MM environment.
  ArmCallSmc (&CommunicateSmcArgs);

  switch (CommunicateSmcArgs.Arg0) {
    case ARM_SMC_MM_RET_SUCCESS:
      // On successful return, the size of data being returned is inferred from
      // MessageLength + Header.
      BufferSize = CommunicateHeader->MessageLength +
                   sizeof (CommunicateHeader->HeaderGuid) +
                   sizeof (CommunicateHeader->MessageLength);
      CopyMem (CommBuffer, (VOID *)CommunicateHeader, BufferSize);
      if (CommSize != NULL) {
        *CommSize = BufferSize;
      }

      Status = EFI_SUCCESS;
      break;

    case ARM_SMC_MM_RET_INVALID_PARAMS:
      Status = EFI_INVALID_PARAMETER;
      break;

    case ARM_SMC_MM_RET_DENIED:
      Status = EFI_ACCESS_DENIED;
      break;

    case ARM_SMC_MM_RET_NO_MEMORY:
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

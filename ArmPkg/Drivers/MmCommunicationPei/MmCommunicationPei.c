/** @file -- MmCommunicationPei.c
  Provides an interface to send MM request in PEI

  Copyright (c) 2016-2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <IndustryStandard/ArmStdSmc.h>

#include <Protocol/MmCommunication.h>
#include <Ppi/MmCommunication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>

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
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  EFI_MM_COMMUNICATE_HEADER  *TempCommHeader;
  ARM_SMC_ARGS               CommunicateSmcArgs;
  EFI_STATUS                 Status;
  UINTN                      BufferSize;

  ZeroMem (&CommunicateSmcArgs, sizeof (ARM_SMC_ARGS));

  // Check that our static buffer is looking good.
  // We are using PcdMmBufferBase to transfer variable data.
  // We are not using the full size of the buffer since there is a cost
  // of copying data between Normal and Secure World.
  if ((PcdGet64 (PcdMmBufferBase) == 0) || (PcdGet64 (PcdMmBufferSize) == 0)) {
    ASSERT (PcdGet64 (PcdMmBufferSize) > 0);
    ASSERT (PcdGet64 (PcdMmBufferBase) != 0);
    return EFI_UNSUPPORTED;
  }

  //
  // Check parameters
  //
  if ((CommBuffer == NULL) || (CommSize == NULL)) {
    ASSERT (CommBuffer != NULL);
    ASSERT (CommSize != NULL);
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

  // Given CommBuffer is not NULL here, we use it to test the legitimacy of CommSize.
  TempCommHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)CommBuffer;

  // CommBuffer is a mandatory parameter. Hence, Rely on
  // MessageLength + Header to ascertain the
  // total size of the communication payload rather than
  // rely on optional CommSize parameter
  BufferSize = TempCommHeader->MessageLength +
               sizeof (TempCommHeader->HeaderGuid) +
               sizeof (TempCommHeader->MessageLength);

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

  // Now we know that the size is something we can handle, copy it over to the designated comm buffer.
  CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)(PcdGet64 (PcdMmBufferBase));

  CopyMem (CommunicateHeader, CommBuffer, *CommSize);

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
      if (BufferSize > (UINTN)PcdGet64 (PcdMmBufferSize)) {
        // Something bad has happened, we should have landed in ARM_SMC_MM_RET_NO_MEMORY
        DEBUG ((
          DEBUG_ERROR,
          "%a Returned buffer exceeds communication buffer limit. Has: 0x%llx vs. max: 0x%llx!\n",
          __func__,
          BufferSize,
          (UINTN)PcdGet64 (PcdMmBufferSize)
          ));
        Status = EFI_BAD_BUFFER_SIZE;
        break;
      }

      CopyMem (CommBuffer, CommunicateHeader, BufferSize);
      *CommSize = BufferSize;
      Status    = EFI_SUCCESS;
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
      break;
  }

  return Status;
}

//
// Module globals for the MM Communication PPI
//
STATIC CONST EFI_PEI_MM_COMMUNICATION_PPI  mPeiMmCommunication = {
  MmCommunicationPeim
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mPeiMmCommunicationPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMmCommunicationPpiGuid,
  (VOID *)&mPeiMmCommunication
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

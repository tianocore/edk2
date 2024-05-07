/** @file
  MM IPL that load the MM Core into MMRAM at PEI stage

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmIplPei.h"

/**
  Build communication buffer HOB.

  @return  MM_COMM_BUFFER     Pointer of MM communication buffer

**/
MM_COMM_BUFFER *
MmIplBuildCommBufferHob (
  VOID
  )
{
  EFI_STATUS      Status;
  MM_COMM_BUFFER  *MmCommBuffer;
  UINT64          FixedCommBufferPages;

  FixedCommBufferPages = PcdGet32 (PcdFixedCommBufferPages);

  MmCommBuffer = BuildGuidHob (&gEdkiiCommunicationBufferGuid, sizeof (MM_COMM_BUFFER));
  ASSERT (MmCommBuffer != NULL);

  //
  // Set fixed communicate buffer size
  //
  MmCommBuffer->FixedCommBufferSize = FixedCommBufferPages * EFI_PAGE_SIZE;

  //
  // Allocate runtime memory for fixed communicate buffer
  //
  MmCommBuffer->FixedCommBuffer = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (FixedCommBufferPages);
  if (MmCommBuffer->FixedCommBuffer == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate fixed communication buffer\n"));
    ASSERT (MmCommBuffer->FixedCommBuffer != 0);
  }

  //
  // Build MM unblock memory region HOB for communication buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->FixedCommBuffer, FixedCommBufferPages);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate runtime memory for communication in and out parameters :
  // ReturnStatus, ReturnBufferSize, IsCommBufferValid
  //
  MmCommBuffer->CommunicationInOut = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (EFI_SIZE_TO_PAGES (sizeof (COMMUNICATION_IN_OUT)));
  if (MmCommBuffer->CommunicationInOut == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate communication in/out buffer\n"));
    ASSERT (MmCommBuffer->CommunicationInOut != 0);
  }

  //
  // Build MM unblock memory region HOB for communication in/out buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->CommunicationInOut, EFI_SIZE_TO_PAGES (sizeof (COMMUNICATION_IN_OUT)));
  ASSERT_EFI_ERROR (Status);

  return MmCommBuffer;
}

/**
  The Entry Point for MM IPL at PEI stage.

  Load MM Core into MMRAM.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
StandaloneMmIplPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  MM_COMM_BUFFER  *MmCommBuffer;

  //
  // Build communication buffer HOB.
  //
  MmCommBuffer = MmIplBuildCommBufferHob ();
  ASSERT (MmCommBuffer != NULL);

  return EFI_SUCCESS;
}

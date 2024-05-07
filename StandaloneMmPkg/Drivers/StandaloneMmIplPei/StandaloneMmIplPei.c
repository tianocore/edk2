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
  UINT64          MmCommBufferPages;

  MmCommBufferPages = PcdGet32 (PcdMmCommBufferPages);

  MmCommBuffer = BuildGuidHob (&gMmCommBufferHobGuid, sizeof (MM_COMM_BUFFER));
  ASSERT (MmCommBuffer != NULL);

  //
  // Set MM communicate buffer size
  //
  MmCommBuffer->NumberOfPages = MmCommBufferPages;

  //
  // Allocate runtime memory for MM communicate buffer
  //
  MmCommBuffer->PhysicalStart = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (MmCommBufferPages);
  if (MmCommBuffer->PhysicalStart == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate MM communication buffer\n"));
    ASSERT (MmCommBuffer->PhysicalStart != 0);
  }

  //
  // Build MM unblock memory region HOB for MM communication buffer
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->PhysicalStart, MmCommBufferPages);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate runtime memory for MM communication status parameters :
  // ReturnStatus, ReturnBufferSize, IsCommBufferValid
  //
  MmCommBuffer->Status = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages (EFI_SIZE_TO_PAGES (sizeof (MM_COMM_BUFFER_STATUS)));
  if (MmCommBuffer->Status == 0) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for MM communication status\n"));
    ASSERT (MmCommBuffer->Status != 0);
  }

  //
  // Build MM unblock memory region HOB for MM communication status
  //
  Status = MmUnblockMemoryRequest (MmCommBuffer->Status, EFI_SIZE_TO_PAGES (sizeof (MM_COMM_BUFFER_STATUS)));
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

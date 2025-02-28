/** @file
  Comm Buffer related functions.

  Copyright (c) 2009 - 2025, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "StandaloneMmCore.h"

extern MM_COMM_BUFFER  *mMmCommunicationBuffer;
extern VOID            *mInternalCommBufferCopy;

MM_COMM_BUFFER  TranslationCommBuffer;

/**
  Prepare communication buffer for MMI.
**/
VOID
MmCorePrepareCommunicationBuffer (
  VOID
  )
{
  EFI_STATUS             Status;
  EFI_HOB_GUID_TYPE      *GuidHob;
  EFI_PHYSICAL_ADDRESS   Buffer;
  EFI_MMRAM_DESCRIPTOR   *NonSecureBuffer;
  MM_COMM_BUFFER_STATUS  *CommBufferStatus;

  NonSecureBuffer         = NULL;
  CommBufferStatus        = NULL;
  mMmCommunicationBuffer  = NULL;
  mInternalCommBufferCopy = NULL;

  GuidHob = GetFirstGuidHob (&gEfiStandaloneMmNonSecureBufferGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return;
  }

  //
  // TFA supplies the HOB gEfiStandaloneMmNonSecureBufferGuid, whose data
  // is a EFI_MMRAM_DESCRIPTOR. This communicates the Memory that is
  // made available for the communication buffer to MM.
  //
  NonSecureBuffer = (EFI_MMRAM_DESCRIPTOR *)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((
    DEBUG_INFO,
    "MM Communication Buffer is at %x, number of pages is %x\n",
    NonSecureBuffer->PhysicalStart,
    EFI_SIZE_TO_PAGES (NonSecureBuffer->PhysicalSize)
    ));
  ASSERT (NonSecureBuffer->PhysicalStart != 0 && NonSecureBuffer->PhysicalSize != 0);

  if (!MmIsBufferOutsideMmValid (
         NonSecureBuffer->PhysicalStart,
         EFI_PAGES_TO_SIZE (NonSecureBuffer->PhysicalSize)
         ))
  {
    mMmCommunicationBuffer = NULL;
    DEBUG ((DEBUG_ERROR, "MM Communication Buffer is invalid!\n"));
    ASSERT (FALSE);
    return;
  }

  //
  // From the range of memory that TFA has provided, take the last page of
  //  the buffer to use as the MM_COMM_BUFFER_STATUS structure.
  //
  CommBufferStatus = (MM_COMM_BUFFER_STATUS *)(UINTN)(NonSecureBuffer->PhysicalStart - EFI_PAGE_SIZE + NonSecureBuffer->PhysicalSize);
  ZeroMem (CommBufferStatus, sizeof (MM_COMM_BUFFER_STATUS));
  CommBufferStatus->IsCommBufferValid = TRUE;

  //
  // Translate the information from the TFA's HOB into a MM_COMM_BUFFER
  //  structure.
  //
  ZeroMem (&TranslationCommBuffer, sizeof (MM_COMM_BUFFER));
  TranslationCommBuffer.PhysicalStart = NonSecureBuffer->PhysicalStart;
  TranslationCommBuffer.NumberOfPages = MIN (
                                          PcdGet32 (PcdMmCommBufferPages),
                                          EFI_SIZE_TO_PAGES (NonSecureBuffer->PhysicalSize) - 1
                                          );

  TranslationCommBuffer.Status = (EFI_PHYSICAL_ADDRESS)(UINTN)CommBufferStatus;

  //
  // Update the global variables that is required by StandaloneMmCore to point to
  //  the translated MM_COMM_BUFFER structure.
  //
  mMmCommunicationBuffer = &TranslationCommBuffer;

  //
  // Attempt to allocate the internal copy of the communication buffer.
  //
  Status = MmAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesData,
             mMmCommunicationBuffer->NumberOfPages,
             &Buffer
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate internal copy of the MM communication buffer\n"));
    ASSERT (EFI_ERROR (Status));
    return;
  }

  mInternalCommBufferCopy = (VOID *)(UINTN)Buffer;
  DEBUG ((DEBUG_INFO, "Internal Communication Buffer Copy is at %p\n", mInternalCommBufferCopy));
}

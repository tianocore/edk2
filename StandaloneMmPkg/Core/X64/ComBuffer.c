/** @file
  Comm Buffer related functions.

  Copyright (c) 2009 - 2025, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "StandaloneMmCore.h"

extern MM_COMM_BUFFER  *mMmCommunicationBuffer;
extern VOID            *mInternalCommBufferCopy;

/**
  Prepare communication buffer for MMI.
**/
VOID
MmCorePrepareCommunicationBuffer (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_HOB_GUID_TYPE     *GuidHob;
  EFI_PHYSICAL_ADDRESS  Buffer;

  mMmCommunicationBuffer  = NULL;
  mInternalCommBufferCopy = NULL;

  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return;
  }

  mMmCommunicationBuffer = (MM_COMM_BUFFER *)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((
    DEBUG_INFO,
    "MM Communication Buffer is at %x, number of pages is %x\n",
    mMmCommunicationBuffer->PhysicalStart,
    mMmCommunicationBuffer->NumberOfPages
    ));
  ASSERT (mMmCommunicationBuffer->PhysicalStart != 0 && mMmCommunicationBuffer->NumberOfPages != 0);

  if (!MmIsBufferOutsideMmValid (
         mMmCommunicationBuffer->PhysicalStart,
         EFI_PAGES_TO_SIZE (mMmCommunicationBuffer->NumberOfPages)
         ))
  {
    mMmCommunicationBuffer = NULL;
    DEBUG ((DEBUG_ERROR, "MM Communication Buffer is invalid!\n"));
    ASSERT (FALSE);
    return;
  }

  Status = MmAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesData,
             mMmCommunicationBuffer->NumberOfPages,
             &Buffer
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return;
  }

  mInternalCommBufferCopy = (VOID *)(UINTN)Buffer;
  DEBUG ((DEBUG_INFO, "Internal Communication Buffer Copy is at %p\n", mInternalCommBufferCopy));
}

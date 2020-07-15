/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by FTW module.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_FTW_DXE_H__
#define __SMM_FTW_DXE_H__

#include <PiDxe.h>

#include <Protocol/MmCommunication2.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/EventGroup.h>

#include "FaultTolerantWriteSmmCommon.h"

/**
  Get the size of the largest block that can be updated in a fault-tolerant manner.

  @param[in]  This             Indicates a pointer to the calling context.
  @param[out] BlockSize        A pointer to a caller-allocated UINTN that is
                               updated to indicate the size of the largest block
                               that can be updated.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.

**/
EFI_STATUS
EFIAPI
FtwGetMaxBlockSize (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  OUT UINTN                                 *BlockSize
  );


/**
  Allocates space for the protocol to maintain information about writes.
  Since writes must be completed in a fault-tolerant manner and multiple
  writes require more resources to be successful, this function
  enables the protocol to ensure that enough space exists to track
  information about upcoming writes.

  @param[in]  This             A pointer to the calling context.
  @param[in]  CallerId         The GUID identifying the write.
  @param[in]  PrivateDataSize  The size of the caller's private data  that must be
                               recorded for each write.
  @param[in]  NumberOfWrites   The number of fault tolerant block writes that will
                               need to occur.

  @retval EFI_SUCCESS          The function completed successfully
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_ACCESS_DENIED    Not all allocated writes have been completed.  All
                               writes must be completed or aborted before another
                               fault tolerant write can occur.

**/
EFI_STATUS
EFIAPI
FtwAllocate (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  IN EFI_GUID                               *CallerId,
  IN UINTN                                  PrivateDataSize,
  IN UINTN                                  NumberOfWrites
  );


/**
  Starts a target block update. This records information about the write
  in fault tolerant storage, and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param[in]  This             The calling context.
  @param[in]  Lba              The logical block address of the target block.
  @param[in]  Offset           The offset within the target block to place the
                               data.
  @param[in]  Length           The number of bytes to write to the target block.
  @param[in]  PrivateData      A pointer to private data that the caller requires
                               to complete any pending writes in the event of a
                               fault.
  @param[in]  FvBlockHandle    The handle of FVB protocol that provides services
                               for reading, writing, and erasing the target block.
  @param[in]  Buffer           The data to write.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_BAD_BUFFER_SIZE  The write would span a block boundary, which is not
                               a valid action.
  @retval EFI_ACCESS_DENIED    No writes have been allocated.
  @retval EFI_NOT_READY        The last write has not been completed. Restart()
                               must be called to complete it.

**/
EFI_STATUS
EFIAPI
FtwWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  IN EFI_LBA                                Lba,
  IN UINTN                                  Offset,
  IN UINTN                                  Length,
  IN VOID                                   *PrivateData,
  IN EFI_HANDLE                             FvBlockHandle,
  IN VOID                                   *Buffer
  );


/**
  Restarts a previously interrupted write. The caller must provide the
  block protocol needed to complete the interrupted write.

  @param[in]  This             The calling context.
  @param[in]  FvBlockHandle    The handle of FVB protocol that provides services.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_ACCESS_DENIED    No pending writes exist.

**/
EFI_STATUS
EFIAPI
FtwRestart (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  IN EFI_HANDLE                             FvBlockHandle
  );


/**
  Aborts all previously allocated writes.

  @param  This                 The calling context.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_NOT_FOUND        No allocated writes exist.

**/
EFI_STATUS
EFIAPI
FtwAbort (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This
  );


/**
  Starts a target block update. This function records information about the write
  in fault-tolerant storage and completes the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[out]     CallerId        The GUID identifying the last write.
  @param[out]     Lba             The logical block address of the last write.
  @param[out]     Offset          The offset within the block of the last write.
  @param[out]     Length          The length of the last write.
  @param[in, out] PrivateDataSize On input, the size of the PrivateData buffer. On
                                  output, the size of the private data stored for
                                  this write.
  @param[out]     PrivateData     A pointer to a buffer. The function will copy
                                  PrivateDataSize bytes from the private data stored
                                  for this write.
  @param[out]     Complete        A Boolean value with TRUE indicating that the write
                                  was completed.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             The function could not complete successfully.
  @retval EFI_NOT_FOUND           No allocated writes exist.

**/
EFI_STATUS
EFIAPI
FtwGetLastWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL      *This,
  OUT EFI_GUID                              *CallerId,
  OUT EFI_LBA                               *Lba,
  OUT UINTN                                 *Offset,
  OUT UINTN                                 *Length,
  IN OUT UINTN                              *PrivateDataSize,
  OUT VOID                                  *PrivateData,
  OUT BOOLEAN                               *Complete
  );

#endif

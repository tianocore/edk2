/** @file
  Fault Tolerant Write protocol provides boot-time service for fault tolerant
  write capability for block devices.  The protocol provides for non-volatile
  storage of the intermediate data and private information a caller would need to
  recover from a critical fault, such as a power failure.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FW_FAULT_TOLERANT_WRITE_PROTOCOL_H_
#define _FW_FAULT_TOLERANT_WRITE_PROTOCOL_H_

#define EFI_FAULT_TOLERANT_WRITE_PROTOCOL_GUID \
  { \
    0x3ebd9e82, 0x2c78, 0x4de6, {0x97, 0x86, 0x8d, 0x4b, 0xfc, 0xb7, 0xc8, 0x81 } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_FAULT_TOLERANT_WRITE_PROTOCOL EFI_FAULT_TOLERANT_WRITE_PROTOCOL;

/**
  Get the size of the largest block that can be updated in a fault-tolerant manner.

  @param  This                 Indicates a pointer to the calling context.
  @param  BlockSize            A pointer to a caller-allocated UINTN that is
                               updated to indicate the size of the largest block
                               that can be updated.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FAULT_TOLERANT_WRITE_GET_MAX_BLOCK_SIZE)(
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL    *This,
  OUT UINTN                               *BlockSize
  );

/**
  Allocates space for the protocol to maintain information about writes.
  Since writes must be completed in a fault-tolerant manner and multiple
  writes require more resources to be successful, this function
  enables the protocol to ensure that enough space exists to track
  information about upcoming writes.

  @param  This                 A pointer to the calling context.
  @param  CallerId             The GUID identifying the write.
  @param  PrivateDataSize      The size of the caller's private data  that must be
                               recorded for each write.
  @param  NumberOfWrites       The number of fault tolerant block writes that will
                               need to occur.

  @retval EFI_SUCCESS          The function completed successfully
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_ACCESS_DENIED    Not all allocated writes have been completed.  All
                               writes must be completed or aborted before another
                               fault tolerant write can occur.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FAULT_TOLERANT_WRITE_ALLOCATE)(
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL    *This,
  IN EFI_GUID                             *CallerId,
  IN UINTN                                PrivateDataSize,
  IN UINTN                                NumberOfWrites
  );

/**
  Starts a target block update. This records information about the write
  in fault tolerant storage, and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param  This                 The calling context.
  @param  Lba                  The logical block address of the target block.
  @param  Offset               The offset within the target block to place the
                               data.
  @param  Length               The number of bytes to write to the target block.
  @param  PrivateData          A pointer to private data that the caller requires
                               to complete any pending writes in the event of a
                               fault.
  @param  FvBlockHandle        The handle of FVB protocol that provides services
                               for reading, writing, and erasing the target block.
  @param  Buffer               The data to write.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_BAD_BUFFER_SIZE  The write would span a block boundary, which is not
                               a valid action.
  @retval EFI_ACCESS_DENIED    No writes have been allocated.
  @retval EFI_NOT_READY        The last write has not been completed. Restart()
                               must be called to complete it.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FAULT_TOLERANT_WRITE_WRITE)(
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  IN EFI_LBA                               Lba,
  IN UINTN                                 Offset,
  IN UINTN                                 Length,
  IN VOID                                  *PrivateData,
  IN EFI_HANDLE                            FvbHandle,
  IN VOID                                  *Buffer
  );

/**
  Restarts a previously interrupted write. The caller must provide the
  block protocol needed to complete the interrupted write.

  @param  This                 The calling context.
  @param  FvBlockProtocol      The handle of FVB protocol that provides services.
                               for reading, writing, and erasing the target block.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_ACCESS_DENIED    No pending writes exist.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FAULT_TOLERANT_WRITE_RESTART)(
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  IN EFI_HANDLE                            FvbHandle
  );

/**
  Aborts all previously allocated writes.

  @param  This                 The calling context.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_NOT_FOUND        No allocated writes exist.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FAULT_TOLERANT_WRITE_ABORT)(
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This
  );

/**
  Starts a target block update. This function records information about the write
  in fault-tolerant storage and completes the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param  This                 Indicates a pointer to the calling context.
  @param  CallerId             The GUID identifying the last write.
  @param  Lba                  The logical block address of the last write.
  @param  Offset               The offset within the block of the last write.
  @param  Length               The length of the last write.
  @param  PrivateDataSize      On input, the size of the PrivateData buffer. On
                               output, the size of the private data stored for
                               this write.
  @param  PrivateData          A pointer to a buffer. The function will copy
                               PrivateDataSize bytes from the private data stored
                               for this write.
  @param  Complete             A Boolean value with TRUE indicating that the write
                               was completed.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_NOT_FOUND        No allocated writes exist.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FAULT_TOLERANT_WRITE_GET_LAST_WRITE)(
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  OUT EFI_GUID                             *CallerId,
  OUT EFI_LBA                              *Lba,
  OUT UINTN                                *Offset,
  OUT UINTN                                *Length,
  IN OUT UINTN                             *PrivateDataSize,
  OUT VOID                                 *PrivateData,
  OUT BOOLEAN                              *Complete
  );

//
// Protocol declaration
//
struct _EFI_FAULT_TOLERANT_WRITE_PROTOCOL {
  EFI_FAULT_TOLERANT_WRITE_GET_MAX_BLOCK_SIZE    GetMaxBlockSize;
  EFI_FAULT_TOLERANT_WRITE_ALLOCATE              Allocate;
  EFI_FAULT_TOLERANT_WRITE_WRITE                 Write;
  EFI_FAULT_TOLERANT_WRITE_RESTART               Restart;
  EFI_FAULT_TOLERANT_WRITE_ABORT                 Abort;
  EFI_FAULT_TOLERANT_WRITE_GET_LAST_WRITE        GetLastWrite;
};

extern EFI_GUID  gEfiFaultTolerantWriteProtocolGuid;

#endif

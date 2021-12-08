/** @file
  DiskIo driver that lays on every BlockIo protocol in the system.
  DiskIo converts a block oriented device to a byte oriented device.

  Disk access may have to handle unaligned request about sector boundaries.
  There are three cases:
    UnderRun - The first byte is not on a sector boundary or the read request is
               less than a sector in length.
    Aligned  - A read of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DiskIo.h"

//
// Driver binding protocol implementation for DiskIo driver.
//
EFI_DRIVER_BINDING_PROTOCOL  gDiskIoDriverBinding = {
  DiskIoDriverBindingSupported,
  DiskIoDriverBindingStart,
  DiskIoDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Template for DiskIo private data structure.
// The pointer to BlockIo protocol interface is assigned dynamically.
//
DISK_IO_PRIVATE_DATA  gDiskIoPrivateDataTemplate = {
  DISK_IO_PRIVATE_DATA_SIGNATURE,
  {
    EFI_DISK_IO_PROTOCOL_REVISION,
    DiskIoReadDisk,
    DiskIoWriteDisk
  },
  {
    EFI_DISK_IO2_PROTOCOL_REVISION,
    DiskIo2Cancel,
    DiskIo2ReadDiskEx,
    DiskIo2WriteDiskEx,
    DiskIo2FlushDiskEx
  }
};

/**
  Test to see if this driver supports ControllerHandle.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test.
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiBlockIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return EFI_SUCCESS;
}

/**
  Start this driver on ControllerHandle by opening a Block IO protocol and
  installing a Disk IO protocol on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS            Status;
  DISK_IO_PRIVATE_DATA  *Instance;
  EFI_TPL               OldTpl;

  Instance = NULL;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Connect to the Block IO and Block IO2 interface on ControllerHandle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&gDiskIoPrivateDataTemplate.BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit1;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIo2ProtocolGuid,
                  (VOID **)&gDiskIoPrivateDataTemplate.BlockIo2,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    gDiskIoPrivateDataTemplate.BlockIo2 = NULL;
  }

  //
  // Initialize the Disk IO device instance.
  //
  Instance = AllocateCopyPool (sizeof (DISK_IO_PRIVATE_DATA), &gDiskIoPrivateDataTemplate);
  if (Instance == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  //
  // The BlockSize and IoAlign of BlockIo and BlockIo2 should equal.
  //
  ASSERT (
    (Instance->BlockIo2 == NULL) ||
    ((Instance->BlockIo->Media->IoAlign == Instance->BlockIo2->Media->IoAlign) &&
     (Instance->BlockIo->Media->BlockSize == Instance->BlockIo2->Media->BlockSize)
    )
    );

  InitializeListHead (&Instance->TaskQueue);
  EfiInitializeLock (&Instance->TaskQueueLock, TPL_NOTIFY);
  Instance->SharedWorkingBuffer = AllocateAlignedPages (
                                    EFI_SIZE_TO_PAGES (PcdGet32 (PcdDiskIoDataBufferBlockNum) * Instance->BlockIo->Media->BlockSize),
                                    Instance->BlockIo->Media->IoAlign
                                    );
  if (Instance->SharedWorkingBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  //
  // Install protocol interfaces for the Disk IO device.
  //
  if (Instance->BlockIo2 != NULL) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gEfiDiskIoProtocolGuid,
                    &Instance->DiskIo,
                    &gEfiDiskIo2ProtocolGuid,
                    &Instance->DiskIo2,
                    NULL
                    );
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gEfiDiskIoProtocolGuid,
                    &Instance->DiskIo,
                    NULL
                    );
  }

ErrorExit:
  if (EFI_ERROR (Status)) {
    if ((Instance != NULL) && (Instance->SharedWorkingBuffer != NULL)) {
      FreeAlignedPages (
        Instance->SharedWorkingBuffer,
        EFI_SIZE_TO_PAGES (PcdGet32 (PcdDiskIoDataBufferBlockNum) * Instance->BlockIo->Media->BlockSize)
        );
    }

    if (Instance != NULL) {
      FreePool (Instance);
    }

    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiBlockIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
  }

ErrorExit1:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Stop this driver on ControllerHandle by removing Disk IO protocol and closing
  the Block IO protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS             Status;
  EFI_DISK_IO_PROTOCOL   *DiskIo;
  EFI_DISK_IO2_PROTOCOL  *DiskIo2;
  DISK_IO_PRIVATE_DATA   *Instance;
  BOOLEAN                AllTaskDone;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIo2ProtocolGuid,
                  (VOID **)&DiskIo2,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DiskIo2 = NULL;
  }

  Instance = DISK_IO_PRIVATE_DATA_FROM_DISK_IO (DiskIo);

  if (DiskIo2 != NULL) {
    //
    // Call BlockIo2::Reset() to terminate any in-flight non-blocking I/O requests
    //
    ASSERT (Instance->BlockIo2 != NULL);
    Status = Instance->BlockIo2->Reset (Instance->BlockIo2, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ControllerHandle,
                    &gEfiDiskIoProtocolGuid,
                    &Instance->DiskIo,
                    &gEfiDiskIo2ProtocolGuid,
                    &Instance->DiskIo2,
                    NULL
                    );
  } else {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ControllerHandle,
                    &gEfiDiskIoProtocolGuid,
                    &Instance->DiskIo,
                    NULL
                    );
  }

  if (!EFI_ERROR (Status)) {
    do {
      EfiAcquireLock (&Instance->TaskQueueLock);
      AllTaskDone = IsListEmpty (&Instance->TaskQueue);
      EfiReleaseLock (&Instance->TaskQueueLock);
    } while (!AllTaskDone);

    FreeAlignedPages (
      Instance->SharedWorkingBuffer,
      EFI_SIZE_TO_PAGES (PcdGet32 (PcdDiskIoDataBufferBlockNum) * Instance->BlockIo->Media->BlockSize)
      );

    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiBlockIoProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );
    ASSERT_EFI_ERROR (Status);
    if (DiskIo2 != NULL) {
      Status = gBS->CloseProtocol (
                      ControllerHandle,
                      &gEfiBlockIo2ProtocolGuid,
                      This->DriverBindingHandle,
                      ControllerHandle
                      );
      ASSERT_EFI_ERROR (Status);
    }

    FreePool (Instance);
  }

  return Status;
}

/**
  Destroy the sub task.

  @param Instance     Pointer to the DISK_IO_PRIVATE_DATA.
  @param Subtask      Subtask.

  @return LIST_ENTRY *  Pointer to the next link of subtask.
**/
LIST_ENTRY *
DiskIoDestroySubtask (
  IN DISK_IO_PRIVATE_DATA  *Instance,
  IN DISK_IO_SUBTASK       *Subtask
  )
{
  LIST_ENTRY  *Link;

  if (Subtask->Task != NULL) {
    EfiAcquireLock (&Subtask->Task->SubtasksLock);
  }

  Link = RemoveEntryList (&Subtask->Link);
  if (Subtask->Task != NULL) {
    EfiReleaseLock (&Subtask->Task->SubtasksLock);
  }

  if (!Subtask->Blocking) {
    if (Subtask->WorkingBuffer != NULL) {
      FreeAlignedPages (
        Subtask->WorkingBuffer,
        Subtask->Length < Instance->BlockIo->Media->BlockSize
        ? EFI_SIZE_TO_PAGES (Instance->BlockIo->Media->BlockSize)
        : EFI_SIZE_TO_PAGES (Subtask->Length)
        );
    }

    if (Subtask->BlockIo2Token.Event != NULL) {
      gBS->CloseEvent (Subtask->BlockIo2Token.Event);
    }
  }

  FreePool (Subtask);

  return Link;
}

/**
  The callback for the BlockIo2 ReadBlocksEx/WriteBlocksEx.
  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which points to the DISK_IO_SUBTASK instance.
**/
VOID
EFIAPI
DiskIo2OnReadWriteComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DISK_IO_SUBTASK       *Subtask;
  DISK_IO2_TASK         *Task;
  EFI_STATUS            TransactionStatus;
  DISK_IO_PRIVATE_DATA  *Instance;

  Subtask           = (DISK_IO_SUBTASK *)Context;
  TransactionStatus = Subtask->BlockIo2Token.TransactionStatus;
  Task              = Subtask->Task;
  Instance          = Task->Instance;

  ASSERT (Subtask->Signature  == DISK_IO_SUBTASK_SIGNATURE);
  ASSERT (Instance->Signature == DISK_IO_PRIVATE_DATA_SIGNATURE);
  ASSERT (Task->Signature     == DISK_IO2_TASK_SIGNATURE);

  if ((Subtask->WorkingBuffer != NULL) && !EFI_ERROR (TransactionStatus) &&
      (Task->Token != NULL) && !Subtask->Write
      )
  {
    CopyMem (Subtask->Buffer, Subtask->WorkingBuffer + Subtask->Offset, Subtask->Length);
  }

  DiskIoDestroySubtask (Instance, Subtask);

  if (EFI_ERROR (TransactionStatus) || IsListEmpty (&Task->Subtasks)) {
    if (Task->Token != NULL) {
      //
      // Signal error status once the subtask is failed.
      // Or signal the last status once the last subtask is finished.
      //
      Task->Token->TransactionStatus = TransactionStatus;
      gBS->SignalEvent (Task->Token->Event);

      //
      // Mark token to NULL indicating the Task is a dead task.
      //
      Task->Token = NULL;
    }
  }
}

/**
  Create the subtask.

  @param Write         TRUE: Write request; FALSE: Read request.
  @param Lba           The starting logical block address to read from on the device.
  @param Offset        The starting byte offset to read from the LBA.
  @param Length        The number of bytes to read from the device.
  @param WorkingBuffer The aligned buffer to hold the data for reading or writing.
  @param Buffer        The buffer to hold the data for reading or writing.
  @param Blocking      TRUE: Blocking request; FALSE: Non-blocking request.

  @return A pointer to the created subtask.
**/
DISK_IO_SUBTASK *
DiskIoCreateSubtask (
  IN BOOLEAN  Write,
  IN UINT64   Lba,
  IN UINT32   Offset,
  IN UINTN    Length,
  IN VOID     *WorkingBuffer   OPTIONAL,
  IN VOID     *Buffer,
  IN BOOLEAN  Blocking
  )
{
  DISK_IO_SUBTASK  *Subtask;
  EFI_STATUS       Status;

  Subtask = AllocateZeroPool (sizeof (DISK_IO_SUBTASK));
  if (Subtask == NULL) {
    return NULL;
  }

  Subtask->Signature     = DISK_IO_SUBTASK_SIGNATURE;
  Subtask->Write         = Write;
  Subtask->Lba           = Lba;
  Subtask->Offset        = Offset;
  Subtask->Length        = Length;
  Subtask->WorkingBuffer = WorkingBuffer;
  Subtask->Buffer        = Buffer;
  Subtask->Blocking      = Blocking;
  if (!Blocking) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    DiskIo2OnReadWriteComplete,
                    Subtask,
                    &Subtask->BlockIo2Token.Event
                    );
    if (EFI_ERROR (Status)) {
      FreePool (Subtask);
      return NULL;
    }
  }

  DEBUG ((
    DEBUG_BLKIO,
    "  %c:Lba/Offset/Length/WorkingBuffer/Buffer = %016lx/%08x/%08x/%08x/%08x\n",
    Write ? 'W' : 'R',
    Lba,
    Offset,
    Length,
    WorkingBuffer,
    Buffer
    ));

  return Subtask;
}

/**
  Create the subtask list.

  @param Instance            Pointer to the DISK_IO_PRIVATE_DATA.
  @param Write               TRUE: Write request; FALSE: Read request.
  @param Offset              The starting byte offset to read from the device.
  @param BufferSize          The size in bytes of Buffer. The number of bytes to read from the device.
  @param Buffer              A pointer to the buffer for the data.
  @param Blocking            TRUE: Blocking request; FALSE: Non-blocking request.
  @param SharedWorkingBuffer The aligned buffer to hold the data for reading or writing.
  @param Subtasks            The subtask list header.

  @retval TRUE  The subtask list is created successfully.
  @retval FALSE The subtask list is not created.
**/
BOOLEAN
DiskIoCreateSubtaskList (
  IN DISK_IO_PRIVATE_DATA  *Instance,
  IN BOOLEAN               Write,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer,
  IN BOOLEAN               Blocking,
  IN VOID                  *SharedWorkingBuffer,
  IN OUT LIST_ENTRY        *Subtasks
  )
{
  UINT32           BlockSize;
  UINT32           IoAlign;
  UINT64           Lba;
  UINT64           OverRunLba;
  UINT32           UnderRun;
  UINT32           OverRun;
  UINT8            *BufferPtr;
  UINTN            Length;
  UINTN            DataBufferSize;
  DISK_IO_SUBTASK  *Subtask;
  VOID             *WorkingBuffer;
  LIST_ENTRY       *Link;

  DEBUG ((DEBUG_BLKIO, "DiskIo: Create subtasks for task: Offset/BufferSize/Buffer = %016lx/%08x/%08x\n", Offset, BufferSize, Buffer));

  BlockSize = Instance->BlockIo->Media->BlockSize;
  IoAlign   = Instance->BlockIo->Media->IoAlign;
  if (IoAlign == 0) {
    IoAlign = 1;
  }

  Lba       = DivU64x32Remainder (Offset, BlockSize, &UnderRun);
  BufferPtr = (UINT8 *)Buffer;

  //
  // Special handling for zero BufferSize
  //
  if (BufferSize == 0) {
    Subtask = DiskIoCreateSubtask (Write, Lba, UnderRun, 0, NULL, BufferPtr, Blocking);
    if (Subtask == NULL) {
      goto Done;
    }

    InsertTailList (Subtasks, &Subtask->Link);
    return TRUE;
  }

  if (UnderRun != 0) {
    Length = MIN (BlockSize - UnderRun, BufferSize);
    if (Blocking) {
      WorkingBuffer = SharedWorkingBuffer;
    } else {
      WorkingBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES (BlockSize), IoAlign);
      if (WorkingBuffer == NULL) {
        goto Done;
      }
    }

    if (Write) {
      //
      // A half write operation can be splitted to a blocking block-read and half write operation
      // This can simplify the sub task processing logic
      //
      Subtask = DiskIoCreateSubtask (FALSE, Lba, 0, BlockSize, NULL, WorkingBuffer, TRUE);
      if (Subtask == NULL) {
        goto Done;
      }

      InsertTailList (Subtasks, &Subtask->Link);
    }

    Subtask = DiskIoCreateSubtask (Write, Lba, UnderRun, Length, WorkingBuffer, BufferPtr, Blocking);
    if (Subtask == NULL) {
      goto Done;
    }

    InsertTailList (Subtasks, &Subtask->Link);

    BufferPtr  += Length;
    Offset     += Length;
    BufferSize -= Length;
    Lba++;
  }

  OverRunLba  = Lba + DivU64x32Remainder (BufferSize, BlockSize, &OverRun);
  BufferSize -= OverRun;

  if (OverRun != 0) {
    if (Blocking) {
      WorkingBuffer = SharedWorkingBuffer;
    } else {
      WorkingBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES (BlockSize), IoAlign);
      if (WorkingBuffer == NULL) {
        goto Done;
      }
    }

    if (Write) {
      //
      // A half write operation can be splitted to a blocking block-read and half write operation
      // This can simplify the sub task processing logic
      //
      Subtask = DiskIoCreateSubtask (FALSE, OverRunLba, 0, BlockSize, NULL, WorkingBuffer, TRUE);
      if (Subtask == NULL) {
        goto Done;
      }

      InsertTailList (Subtasks, &Subtask->Link);
    }

    Subtask = DiskIoCreateSubtask (Write, OverRunLba, 0, OverRun, WorkingBuffer, BufferPtr + BufferSize, Blocking);
    if (Subtask == NULL) {
      goto Done;
    }

    InsertTailList (Subtasks, &Subtask->Link);
  }

  if (OverRunLba > Lba) {
    //
    // If the DiskIo maps directly to a BlockIo device do the read.
    //
    if (ALIGN_POINTER (BufferPtr, IoAlign) == BufferPtr) {
      Subtask = DiskIoCreateSubtask (Write, Lba, 0, BufferSize, NULL, BufferPtr, Blocking);
      if (Subtask == NULL) {
        goto Done;
      }

      InsertTailList (Subtasks, &Subtask->Link);

      BufferPtr  += BufferSize;
      Offset     += BufferSize;
      BufferSize -= BufferSize;
    } else {
      if (Blocking) {
        //
        // Use the allocated buffer instead of the original buffer
        // to avoid alignment issue.
        //
        for ( ; Lba < OverRunLba; Lba += PcdGet32 (PcdDiskIoDataBufferBlockNum)) {
          DataBufferSize = MIN (BufferSize, PcdGet32 (PcdDiskIoDataBufferBlockNum) * BlockSize);

          Subtask = DiskIoCreateSubtask (Write, Lba, 0, DataBufferSize, SharedWorkingBuffer, BufferPtr, Blocking);
          if (Subtask == NULL) {
            goto Done;
          }

          InsertTailList (Subtasks, &Subtask->Link);

          BufferPtr  += DataBufferSize;
          Offset     += DataBufferSize;
          BufferSize -= DataBufferSize;
        }
      } else {
        WorkingBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), IoAlign);
        if (WorkingBuffer == NULL) {
          //
          // If there is not enough memory, downgrade to blocking access
          //
          DEBUG ((DEBUG_VERBOSE, "DiskIo: No enough memory so downgrade to blocking access\n"));
          if (!DiskIoCreateSubtaskList (Instance, Write, Offset, BufferSize, BufferPtr, TRUE, SharedWorkingBuffer, Subtasks)) {
            goto Done;
          }
        } else {
          Subtask = DiskIoCreateSubtask (Write, Lba, 0, BufferSize, WorkingBuffer, BufferPtr, Blocking);
          if (Subtask == NULL) {
            goto Done;
          }

          InsertTailList (Subtasks, &Subtask->Link);
        }

        BufferPtr  += BufferSize;
        Offset     += BufferSize;
        BufferSize -= BufferSize;
      }
    }
  }

  ASSERT (BufferSize == 0);

  return TRUE;

Done:
  //
  // Remove all the subtasks.
  //
  for (Link = GetFirstNode (Subtasks); !IsNull (Subtasks, Link); ) {
    Subtask = CR (Link, DISK_IO_SUBTASK, Link, DISK_IO_SUBTASK_SIGNATURE);
    Link    = DiskIoDestroySubtask (Instance, Subtask);
  }

  return FALSE;
}

/**
  Terminate outstanding asynchronous requests to a device.

  @param This                   Indicates a pointer to the calling context.

  @retval EFI_SUCCESS           All outstanding requests were successfully terminated.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the cancel
                                operation.
**/
EFI_STATUS
EFIAPI
DiskIo2Cancel (
  IN EFI_DISK_IO2_PROTOCOL  *This
  )
{
  DISK_IO_PRIVATE_DATA  *Instance;
  DISK_IO2_TASK         *Task;
  LIST_ENTRY            *Link;

  Instance = DISK_IO_PRIVATE_DATA_FROM_DISK_IO2 (This);

  EfiAcquireLock (&Instance->TaskQueueLock);

  for (Link = GetFirstNode (&Instance->TaskQueue)
       ; !IsNull (&Instance->TaskQueue, Link)
       ; Link = GetNextNode (&Instance->TaskQueue, Link)
       )
  {
    Task = CR (Link, DISK_IO2_TASK, Link, DISK_IO2_TASK_SIGNATURE);

    if (Task->Token != NULL) {
      Task->Token->TransactionStatus = EFI_ABORTED;
      gBS->SignalEvent (Task->Token->Event);
      //
      // Set Token to NULL so that the further BlockIo2 responses will be ignored
      //
      Task->Token = NULL;
    }
  }

  EfiReleaseLock (&Instance->TaskQueueLock);

  return EFI_SUCCESS;
}

/**
  Remove the completed tasks from Instance->TaskQueue. Completed tasks are those who don't have any subtasks.

  @param Instance    Pointer to the DISK_IO_PRIVATE_DATA.

  @retval TRUE       The Instance->TaskQueue is empty after the completed tasks are removed.
  @retval FALSE      The Instance->TaskQueue is not empty after the completed tasks are removed.
**/
BOOLEAN
DiskIo2RemoveCompletedTask (
  IN DISK_IO_PRIVATE_DATA  *Instance
  )
{
  BOOLEAN        QueueEmpty;
  LIST_ENTRY     *Link;
  DISK_IO2_TASK  *Task;

  QueueEmpty = TRUE;

  EfiAcquireLock (&Instance->TaskQueueLock);
  for (Link = GetFirstNode (&Instance->TaskQueue); !IsNull (&Instance->TaskQueue, Link); ) {
    Task = CR (Link, DISK_IO2_TASK, Link, DISK_IO2_TASK_SIGNATURE);
    if (IsListEmpty (&Task->Subtasks)) {
      Link = RemoveEntryList (&Task->Link);
      ASSERT (Task->Token == NULL);
      FreePool (Task);
    } else {
      Link       = GetNextNode (&Instance->TaskQueue, Link);
      QueueEmpty = FALSE;
    }
  }

  EfiReleaseLock (&Instance->TaskQueueLock);

  return QueueEmpty;
}

/**
  Common routine to access the disk.

  @param Instance    Pointer to the DISK_IO_PRIVATE_DATA.
  @param Write       TRUE: Write operation; FALSE: Read operation.
  @param MediaId     ID of the medium to access.
  @param Offset      The starting byte offset on the logical block I/O device to access.
  @param Token       A pointer to the token associated with the transaction.
                     If this field is NULL, synchronous/blocking IO is performed.
  @param  BufferSize            The size in bytes of Buffer. The number of bytes to read from the device.
  @param  Buffer                A pointer to the destination buffer for the data.
                                The caller is responsible either having implicit or explicit ownership of the buffer.
**/
EFI_STATUS
DiskIo2ReadWriteDisk (
  IN DISK_IO_PRIVATE_DATA  *Instance,
  IN BOOLEAN               Write,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN EFI_DISK_IO2_TOKEN    *Token,
  IN UINTN                 BufferSize,
  IN UINT8                 *Buffer
  )
{
  EFI_STATUS              Status;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL  *BlockIo2;
  EFI_BLOCK_IO_MEDIA      *Media;
  LIST_ENTRY              *Link;
  LIST_ENTRY              *NextLink;
  LIST_ENTRY              Subtasks;
  DISK_IO_SUBTASK         *Subtask;
  DISK_IO2_TASK           *Task;
  EFI_TPL                 OldTpl;
  BOOLEAN                 Blocking;
  BOOLEAN                 SubtaskBlocking;
  LIST_ENTRY              *SubtasksPtr;

  Task     = NULL;
  BlockIo  = Instance->BlockIo;
  BlockIo2 = Instance->BlockIo2;
  Media    = BlockIo->Media;
  Status   = EFI_SUCCESS;
  Blocking = (BOOLEAN)((Token == NULL) || (Token->Event == NULL));

  if (Blocking) {
    //
    // Wait till pending async task is completed.
    //
    while (!DiskIo2RemoveCompletedTask (Instance)) {
    }

    SubtasksPtr = &Subtasks;
  } else {
    DiskIo2RemoveCompletedTask (Instance);
    Task = AllocatePool (sizeof (DISK_IO2_TASK));
    if (Task == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    EfiAcquireLock (&Instance->TaskQueueLock);
    InsertTailList (&Instance->TaskQueue, &Task->Link);
    EfiReleaseLock (&Instance->TaskQueueLock);

    Task->Signature = DISK_IO2_TASK_SIGNATURE;
    Task->Instance  = Instance;
    Task->Token     = Token;
    EfiInitializeLock (&Task->SubtasksLock, TPL_NOTIFY);

    SubtasksPtr = &Task->Subtasks;
  }

  InitializeListHead (SubtasksPtr);
  if (!DiskIoCreateSubtaskList (Instance, Write, Offset, BufferSize, Buffer, Blocking, Instance->SharedWorkingBuffer, SubtasksPtr)) {
    if (Task != NULL) {
      FreePool (Task);
    }

    return EFI_OUT_OF_RESOURCES;
  }

  ASSERT (!IsListEmpty (SubtasksPtr));

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  for ( Link = GetFirstNode (SubtasksPtr), NextLink = GetNextNode (SubtasksPtr, Link)
        ; !IsNull (SubtasksPtr, Link)
        ; Link = NextLink, NextLink = GetNextNode (SubtasksPtr, NextLink)
        )
  {
    Subtask         = CR (Link, DISK_IO_SUBTASK, Link, DISK_IO_SUBTASK_SIGNATURE);
    Subtask->Task   = Task;
    SubtaskBlocking = Subtask->Blocking;

    ASSERT ((Subtask->Length % Media->BlockSize == 0) || (Subtask->Length < Media->BlockSize));

    if (Subtask->Write) {
      //
      // Write
      //
      if (Subtask->WorkingBuffer != NULL) {
        //
        // A sub task before this one should be a block read operation, causing the WorkingBuffer filled with the entire one block data.
        //
        CopyMem (Subtask->WorkingBuffer + Subtask->Offset, Subtask->Buffer, Subtask->Length);
      }

      if (SubtaskBlocking) {
        Status = BlockIo->WriteBlocks (
                            BlockIo,
                            MediaId,
                            Subtask->Lba,
                            (Subtask->Length % Media->BlockSize == 0) ? Subtask->Length : Media->BlockSize,
                            (Subtask->WorkingBuffer != NULL) ? Subtask->WorkingBuffer : Subtask->Buffer
                            );
      } else {
        Status = BlockIo2->WriteBlocksEx (
                             BlockIo2,
                             MediaId,
                             Subtask->Lba,
                             &Subtask->BlockIo2Token,
                             (Subtask->Length % Media->BlockSize == 0) ? Subtask->Length : Media->BlockSize,
                             (Subtask->WorkingBuffer != NULL) ? Subtask->WorkingBuffer : Subtask->Buffer
                             );
      }
    } else {
      //
      // Read
      //
      if (SubtaskBlocking) {
        Status = BlockIo->ReadBlocks (
                            BlockIo,
                            MediaId,
                            Subtask->Lba,
                            (Subtask->Length % Media->BlockSize == 0) ? Subtask->Length : Media->BlockSize,
                            (Subtask->WorkingBuffer != NULL) ? Subtask->WorkingBuffer : Subtask->Buffer
                            );
        if (!EFI_ERROR (Status) && (Subtask->WorkingBuffer != NULL)) {
          CopyMem (Subtask->Buffer, Subtask->WorkingBuffer + Subtask->Offset, Subtask->Length);
        }
      } else {
        Status = BlockIo2->ReadBlocksEx (
                             BlockIo2,
                             MediaId,
                             Subtask->Lba,
                             &Subtask->BlockIo2Token,
                             (Subtask->Length % Media->BlockSize == 0) ? Subtask->Length : Media->BlockSize,
                             (Subtask->WorkingBuffer != NULL) ? Subtask->WorkingBuffer : Subtask->Buffer
                             );
      }
    }

    if (SubtaskBlocking || EFI_ERROR (Status)) {
      //
      // Make sure the subtask list only contains non-blocking subtasks.
      // Remove failed non-blocking subtasks as well because the callback won't be called.
      //
      DiskIoDestroySubtask (Instance, Subtask);
    }

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Remove all the remaining subtasks when failure.
  // We shouldn't remove all the tasks because the non-blocking requests have been submitted and cannot be canceled.
  //
  if (EFI_ERROR (Status)) {
    while (!IsNull (SubtasksPtr, NextLink)) {
      Subtask  = CR (NextLink, DISK_IO_SUBTASK, Link, DISK_IO_SUBTASK_SIGNATURE);
      NextLink = DiskIoDestroySubtask (Instance, Subtask);
    }
  }

  //
  // It's possible that the non-blocking subtasks finish before raising TPL to NOTIFY,
  // so the subtasks list might be empty at this point.
  //
  if (!Blocking && IsListEmpty (SubtasksPtr)) {
    EfiAcquireLock (&Instance->TaskQueueLock);
    RemoveEntryList (&Task->Link);
    EfiReleaseLock (&Instance->TaskQueueLock);

    if (!EFI_ERROR (Status) && (Task->Token != NULL)) {
      //
      // Task->Token should be set to NULL by the DiskIo2OnReadWriteComplete
      // It it's not, that means the non-blocking request was downgraded to blocking request.
      //
      DEBUG ((DEBUG_VERBOSE, "DiskIo: Non-blocking request was downgraded to blocking request, signal event directly.\n"));
      Task->Token->TransactionStatus = Status;
      gBS->SignalEvent (Task->Token->Event);
    }

    FreePool (Task);
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Reads a specified number of bytes from a device.

  @param This                   Indicates a pointer to the calling context.
  @param MediaId                ID of the medium to be read.
  @param Offset                 The starting byte offset on the logical block I/O device to read from.
  @param Token                  A pointer to the token associated with the transaction.
                                If this field is NULL, synchronous/blocking IO is performed.
  @param  BufferSize            The size in bytes of Buffer. The number of bytes to read from the device.
  @param  Buffer                A pointer to the destination buffer for the data.
                                The caller is responsible either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           If Event is NULL (blocking I/O): The data was read correctly from the device.
                                If Event is not NULL (asynchronous I/O): The request was successfully queued for processing.
                                                                         Event will be signaled upon completion.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no medium in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId is not for the current medium.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not valid for the device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
DiskIo2ReadDiskEx (
  IN EFI_DISK_IO2_PROTOCOL   *This,
  IN UINT32                  MediaId,
  IN UINT64                  Offset,
  IN OUT EFI_DISK_IO2_TOKEN  *Token,
  IN UINTN                   BufferSize,
  OUT VOID                   *Buffer
  )
{
  return DiskIo2ReadWriteDisk (
           DISK_IO_PRIVATE_DATA_FROM_DISK_IO2 (This),
           FALSE,
           MediaId,
           Offset,
           Token,
           BufferSize,
           (UINT8 *)Buffer
           );
}

/**
  Writes a specified number of bytes to a device.

  @param This        Indicates a pointer to the calling context.
  @param MediaId     ID of the medium to be written.
  @param Offset      The starting byte offset on the logical block I/O device to write to.
  @param Token       A pointer to the token associated with the transaction.
                     If this field is NULL, synchronous/blocking IO is performed.
  @param BufferSize  The size in bytes of Buffer. The number of bytes to write to the device.
  @param Buffer      A pointer to the buffer containing the data to be written.

  @retval EFI_SUCCESS           If Event is NULL (blocking I/O): The data was written correctly to the device.
                                If Event is not NULL (asynchronous I/O): The request was successfully queued for processing.
                                                                         Event will be signaled upon completion.
  @retval EFI_WRITE_PROTECTED   The device cannot be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write operation.
  @retval EFI_NO_MEDIA          There is no medium in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId is not for the current medium.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not valid for the device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
DiskIo2WriteDiskEx (
  IN EFI_DISK_IO2_PROTOCOL   *This,
  IN UINT32                  MediaId,
  IN UINT64                  Offset,
  IN OUT EFI_DISK_IO2_TOKEN  *Token,
  IN UINTN                   BufferSize,
  IN VOID                    *Buffer
  )
{
  return DiskIo2ReadWriteDisk (
           DISK_IO_PRIVATE_DATA_FROM_DISK_IO2 (This),
           TRUE,
           MediaId,
           Offset,
           Token,
           BufferSize,
           (UINT8 *)Buffer
           );
}

/**
  The callback for the BlockIo2 FlushBlocksEx.
  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which points to the DISK_IO2_FLUSH_TASK instance.
**/
VOID
EFIAPI
DiskIo2OnFlushComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DISK_IO2_FLUSH_TASK  *Task;

  gBS->CloseEvent (Event);

  Task = (DISK_IO2_FLUSH_TASK *)Context;
  ASSERT (Task->Signature == DISK_IO2_FLUSH_TASK_SIGNATURE);
  Task->Token->TransactionStatus = Task->BlockIo2Token.TransactionStatus;
  gBS->SignalEvent (Task->Token->Event);

  FreePool (Task);
}

/**
  Flushes all modified data to the physical device.

  @param This        Indicates a pointer to the calling context.
  @param Token       A pointer to the token associated with the transaction.
                     If this field is NULL, synchronous/blocking IO is performed.

  @retval EFI_SUCCESS           If Event is NULL (blocking I/O): The data was flushed successfully to the device.
                                If Event is not NULL (asynchronous I/O): The request was successfully queued for processing.
                                                                         Event will be signaled upon completion.
  @retval EFI_WRITE_PROTECTED   The device cannot be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write operation.
  @retval EFI_NO_MEDIA          There is no medium in the device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
DiskIo2FlushDiskEx (
  IN EFI_DISK_IO2_PROTOCOL   *This,
  IN OUT EFI_DISK_IO2_TOKEN  *Token
  )
{
  EFI_STATUS            Status;
  DISK_IO2_FLUSH_TASK   *Task;
  DISK_IO_PRIVATE_DATA  *Private;

  Private = DISK_IO_PRIVATE_DATA_FROM_DISK_IO2 (This);

  if ((Token != NULL) && (Token->Event != NULL)) {
    Task = AllocatePool (sizeof (DISK_IO2_FLUSH_TASK));
    if (Task == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    DiskIo2OnFlushComplete,
                    Task,
                    &Task->BlockIo2Token.Event
                    );
    if (EFI_ERROR (Status)) {
      FreePool (Task);
      return Status;
    }

    Task->Signature = DISK_IO2_FLUSH_TASK_SIGNATURE;
    Task->Token     = Token;
    Status          = Private->BlockIo2->FlushBlocksEx (Private->BlockIo2, &Task->BlockIo2Token);
    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (Task->BlockIo2Token.Event);
      FreePool (Task);
    }
  } else {
    Status = Private->BlockIo2->FlushBlocksEx (Private->BlockIo2, NULL);
  }

  return Status;
}

/**
  Read BufferSize bytes from Offset into Buffer.
  Reads may support reads that are not aligned on
  sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the read request is
               less than a sector in length.
    Aligned  - A read of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary.

  @param  This                  Protocol instance pointer.
  @param  MediaId               Id of the media, changes every time the media is replaced.
  @param  Offset                The starting byte offset to read from
  @param  BufferSize            Size of Buffer
  @param  Buffer                Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  OUT VOID                 *Buffer
  )
{
  return DiskIo2ReadWriteDisk (
           DISK_IO_PRIVATE_DATA_FROM_DISK_IO (This),
           FALSE,
           MediaId,
           Offset,
           NULL,
           BufferSize,
           (UINT8 *)Buffer
           );
}

/**
  Writes BufferSize bytes from Buffer into Offset.
  Writes may require a read modify write to support writes that are not
  aligned on sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the write request
               is less than a sector in length. Read modify write is required.
    Aligned  - A write of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary. Read modified write
               required.

  @param  This       Protocol instance pointer.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Offset     The starting byte offset to read from
  @param  BufferSize Size of Buffer
  @param  Buffer     Buffer containing read data

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not
                                 valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  )
{
  return DiskIo2ReadWriteDisk (
           DISK_IO_PRIVATE_DATA_FROM_DISK_IO (This),
           TRUE,
           MediaId,
           Offset,
           NULL,
           BufferSize,
           (UINT8 *)Buffer
           );
}

/**
  The user Entry Point for module DiskIo. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeDiskIo (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gDiskIoDriverBinding,
             ImageHandle,
             &gDiskIoComponentName,
             &gDiskIoComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

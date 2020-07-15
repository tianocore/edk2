/** @file
  Miscellaneous functions.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Fat.h"
UINT8  mMonthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/**

  Create the task

  @param  IFile                 - The instance of the open file.
  @param  Token                 - A pointer to the token associated with the transaction.

  @return FAT_TASK *            - Return the task instance.

**/
FAT_TASK *
FatCreateTask (
  FAT_IFILE           *IFile,
  EFI_FILE_IO_TOKEN   *Token
  )
{
  FAT_TASK            *Task;

  Task = AllocateZeroPool (sizeof (*Task));
  if (Task != NULL) {
    Task->Signature   = FAT_TASK_SIGNATURE;
    Task->IFile       = IFile;
    Task->FileIoToken = Token;
    InitializeListHead (&Task->Subtasks);
    InitializeListHead (&Task->Link);
  }
  return Task;
}

/**

  Destroy the task.

  @param  Task                  - The task to be destroyed.

**/
VOID
FatDestroyTask (
  FAT_TASK            *Task
  )
{
  LIST_ENTRY          *Link;
  FAT_SUBTASK         *Subtask;

  Link = GetFirstNode (&Task->Subtasks);
  while (!IsNull (&Task->Subtasks, Link)) {
    Subtask = CR (Link, FAT_SUBTASK, Link, FAT_SUBTASK_SIGNATURE);
    Link = FatDestroySubtask (Subtask);
  }
  FreePool (Task);
}

/**

  Wait all non-blocking requests complete.

  @param  IFile                 - The instance of the open file.

**/
VOID
FatWaitNonblockingTask (
  FAT_IFILE           *IFile
  )
{
  BOOLEAN             TaskQueueEmpty;

  do {
    EfiAcquireLock (&FatTaskLock);
    TaskQueueEmpty = IsListEmpty (&IFile->Tasks);
    EfiReleaseLock (&FatTaskLock);
  } while (!TaskQueueEmpty);
}

/**

  Remove the subtask from subtask list.

  @param  Subtask               - The subtask to be removed.

  @return LIST_ENTRY *          - The next node in the list.

**/
LIST_ENTRY *
FatDestroySubtask (
  FAT_SUBTASK         *Subtask
  )
{
  LIST_ENTRY          *Link;

  gBS->CloseEvent (Subtask->DiskIo2Token.Event);

  Link = RemoveEntryList (&Subtask->Link);
  FreePool (Subtask);

  return Link;
}

/**

  Execute the task.

  @param  IFile                 - The instance of the open file.
  @param  Task                  - The task to be executed.

  @retval EFI_SUCCESS           - The task was executed successfully.
  @return other                 - An error occurred when executing the task.

**/
EFI_STATUS
FatQueueTask (
  IN FAT_IFILE        *IFile,
  IN FAT_TASK         *Task
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Link;
  LIST_ENTRY          *NextLink;
  FAT_SUBTASK         *Subtask;

  //
  // Sometimes the Task doesn't contain any subtasks, signal the event directly.
  //
  if (IsListEmpty (&Task->Subtasks)) {
    Task->FileIoToken->Status = EFI_SUCCESS;
    gBS->SignalEvent (Task->FileIoToken->Event);
    FreePool (Task);
    return EFI_SUCCESS;
  }

  EfiAcquireLock (&FatTaskLock);
  InsertTailList (&IFile->Tasks, &Task->Link);
  EfiReleaseLock (&FatTaskLock);

  Status = EFI_SUCCESS;
  //
  // Use NextLink to store the next link of the list, because Link might be remove from the
  // doubly-linked list and get freed in the end of current loop.
  //
  // Also, list operation APIs like IsNull() and GetNextNode() are avoided during the loop, since
  // they may check the validity of doubly-linked lists by traversing them. These APIs cannot
  // handle list elements being removed during the traverse.
  //
  for ( Link = GetFirstNode (&Task->Subtasks), NextLink = GetNextNode (&Task->Subtasks, Link)
      ; Link != &Task->Subtasks
      ; Link = NextLink, NextLink = Link->ForwardLink
      ) {
    Subtask = CR (Link, FAT_SUBTASK, Link, FAT_SUBTASK_SIGNATURE);
    if (Subtask->Write) {

      Status = IFile->OFile->Volume->DiskIo2->WriteDiskEx (
                                                IFile->OFile->Volume->DiskIo2,
                                                IFile->OFile->Volume->MediaId,
                                                Subtask->Offset,
                                                &Subtask->DiskIo2Token,
                                                Subtask->BufferSize,
                                                Subtask->Buffer
                                                );
    } else {
      Status = IFile->OFile->Volume->DiskIo2->ReadDiskEx (
                                                IFile->OFile->Volume->DiskIo2,
                                                IFile->OFile->Volume->MediaId,
                                                Subtask->Offset,
                                                &Subtask->DiskIo2Token,
                                                Subtask->BufferSize,
                                                Subtask->Buffer
                                                );
    }
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    EfiAcquireLock (&FatTaskLock);
    //
    // Remove all the remaining subtasks when failure.
    // We shouldn't remove all the tasks because the non-blocking requests have
    // been submitted and cannot be canceled.
    //
    while (!IsNull (&Task->Subtasks, Link)) {
      Subtask = CR (Link, FAT_SUBTASK, Link, FAT_SUBTASK_SIGNATURE);
      Link = FatDestroySubtask (Subtask);
    }

    if (IsListEmpty (&Task->Subtasks)) {
      RemoveEntryList (&Task->Link);
      FreePool (Task);
    } else {
      //
      // If one or more subtasks have been already submitted, set FileIoToken
      // to NULL so that the callback won't signal the event.
      //
      Task->FileIoToken = NULL;
    }

    EfiReleaseLock (&FatTaskLock);
  }

  return Status;
}

/**

  Set the volume as dirty or not.

  @param  Volume                - FAT file system volume.
  @param  IoMode                - The access mode.
  @param  DirtyValue            - Set the volume as dirty or not.

  @retval EFI_SUCCESS           - Set the new FAT entry value successfully.
  @return other                 - An error occurred when operation the FAT entries.

**/
EFI_STATUS
FatAccessVolumeDirty (
  IN FAT_VOLUME       *Volume,
  IN IO_MODE          IoMode,
  IN VOID             *DirtyValue
  )
{
  UINTN WriteCount;

  WriteCount = Volume->FatEntrySize;
  return FatDiskIo (Volume, IoMode, Volume->FatPos + WriteCount, WriteCount, DirtyValue, NULL);
}

/**
  Invoke a notification event.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
FatOnAccessComplete (
  IN  EFI_EVENT                Event,
  IN  VOID                     *Context
  )
{
  EFI_STATUS             Status;
  FAT_SUBTASK            *Subtask;
  FAT_TASK               *Task;

  //
  // Avoid someone in future breaks the below assumption.
  //
  ASSERT (EfiGetCurrentTpl () == FatTaskLock.Tpl);

  Subtask = (FAT_SUBTASK *) Context;
  Task    = Subtask->Task;
  Status  = Subtask->DiskIo2Token.TransactionStatus;

  ASSERT (Task->Signature    == FAT_TASK_SIGNATURE);
  ASSERT (Subtask->Signature == FAT_SUBTASK_SIGNATURE);

  //
  // Remove the task unconditionally
  //
  FatDestroySubtask (Subtask);

  //
  // Task->FileIoToken is NULL which means the task will be ignored (just recycle the subtask and task memory).
  //
  if (Task->FileIoToken != NULL) {
    if (IsListEmpty (&Task->Subtasks) || EFI_ERROR (Status)) {
      Task->FileIoToken->Status = Status;
      gBS->SignalEvent (Task->FileIoToken->Event);
      //
      // Mark Task->FileIoToken to NULL so that the subtasks belonging to the task will be ignored.
      //
      Task->FileIoToken = NULL;
    }
  }

  if (IsListEmpty (&Task->Subtasks)) {
    RemoveEntryList (&Task->Link);
    FreePool (Task);
  }
}

/**

  General disk access function.

  @param  Volume                - FAT file system volume.
  @param  IoMode                - The access mode (disk read/write or cache access).
  @param  Offset                - The starting byte offset to read from.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing read data.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - The operation is performed successfully.
  @retval EFI_VOLUME_CORRUPTED  - The access is
  @return Others                - The status of read/write the disk

**/
EFI_STATUS
FatDiskIo (
  IN     FAT_VOLUME       *Volume,
  IN     IO_MODE          IoMode,
  IN     UINT64           Offset,
  IN     UINTN            BufferSize,
  IN OUT VOID             *Buffer,
  IN     FAT_TASK         *Task
  )
{
  EFI_STATUS            Status;
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  EFI_DISK_READ         IoFunction;
  FAT_SUBTASK           *Subtask;

  //
  // Verify the IO is in devices range
  //
  Status = EFI_VOLUME_CORRUPTED;
  if (Offset + BufferSize <= Volume->VolumeSize) {
    if (CACHE_ENABLED (IoMode)) {
      //
      // Access cache
      //
      Status = FatAccessCache (Volume, CACHE_TYPE (IoMode), RAW_ACCESS (IoMode), Offset, BufferSize, Buffer, Task);
    } else {
      //
      // Access disk directly
      //
      if (Task == NULL) {
        //
        // Blocking access
        //
        DiskIo      = Volume->DiskIo;
        IoFunction  = (IoMode == ReadDisk) ? DiskIo->ReadDisk : DiskIo->WriteDisk;
        Status      = IoFunction (DiskIo, Volume->MediaId, Offset, BufferSize, Buffer);
      } else {
        //
        // Non-blocking access
        //
        Subtask = AllocateZeroPool (sizeof (*Subtask));
        if (Subtask == NULL) {
          Status        = EFI_OUT_OF_RESOURCES;
        } else {
          Subtask->Signature  = FAT_SUBTASK_SIGNATURE;
          Subtask->Task       = Task;
          Subtask->Write      = (BOOLEAN) (IoMode == WriteDisk);
          Subtask->Offset     = Offset;
          Subtask->Buffer     = Buffer;
          Subtask->BufferSize = BufferSize;
          Status = gBS->CreateEvent (
                          EVT_NOTIFY_SIGNAL,
                          TPL_NOTIFY,
                          FatOnAccessComplete,
                          Subtask,
                          &Subtask->DiskIo2Token.Event
                          );
          if (!EFI_ERROR (Status)) {
            InsertTailList (&Task->Subtasks, &Subtask->Link);
          } else {
            FreePool (Subtask);
          }
        }
      }
    }
  }

  if (EFI_ERROR (Status)) {
    Volume->DiskError = TRUE;
    DEBUG ((EFI_D_ERROR, "FatDiskIo: error %r\n", Status));
  }

  return Status;
}

/**

  Lock the volume.

**/
VOID
FatAcquireLock (
  VOID
  )
{
  EfiAcquireLock (&FatFsLock);
}

/**

  Lock the volume.
  If the lock is already in the acquired state, then EFI_ACCESS_DENIED is returned.
  Otherwise, EFI_SUCCESS is returned.

  @retval EFI_SUCCESS           - The volume is locked.
  @retval EFI_ACCESS_DENIED     - The volume could not be locked because it is already locked.

**/
EFI_STATUS
FatAcquireLockOrFail (
  VOID
  )
{
  return EfiAcquireLockOrFail (&FatFsLock);
}

/**

  Unlock the volume.

**/
VOID
FatReleaseLock (
  VOID
  )
{
  EfiReleaseLock (&FatFsLock);
}

/**

  Free directory entry.

  @param  DirEnt                - The directory entry to be freed.

**/
VOID
FatFreeDirEnt (
  IN FAT_DIRENT       *DirEnt
  )
{
  if (DirEnt->FileString != NULL) {
    FreePool (DirEnt->FileString);
  }

  FreePool (DirEnt);
}

/**

  Free volume structure (including the contents of directory cache and disk cache).

  @param  Volume                - The volume structure to be freed.

**/
VOID
FatFreeVolume (
  IN FAT_VOLUME       *Volume
  )
{
  //
  // Free disk cache
  //
  if (Volume->CacheBuffer != NULL) {
    FreePool (Volume->CacheBuffer);
  }
  //
  // Free directory cache
  //
  FatCleanupODirCache (Volume);
  FreePool (Volume);
}

/**

  Translate EFI time to FAT time.

  @param  ETime                 - The time of EFI_TIME.
  @param  FTime                 - The time of FAT_DATE_TIME.

**/
VOID
FatEfiTimeToFatTime (
  IN  EFI_TIME        *ETime,
  OUT FAT_DATE_TIME   *FTime
  )
{
  //
  // ignores timezone info in source ETime
  //
  if (ETime->Year > 1980) {
    FTime->Date.Year = (UINT16) (ETime->Year - 1980);
  }

  if (ETime->Year >= 1980 + FAT_MAX_YEAR_FROM_1980) {
    FTime->Date.Year = FAT_MAX_YEAR_FROM_1980;
  }

  FTime->Date.Month         = ETime->Month;
  FTime->Date.Day           = ETime->Day;
  FTime->Time.Hour          = ETime->Hour;
  FTime->Time.Minute        = ETime->Minute;
  FTime->Time.DoubleSecond  = (UINT16) (ETime->Second / 2);
}

/**

  Translate Fat time to EFI time.

  @param  FTime                 - The time of FAT_DATE_TIME.
  @param  ETime                 - The time of EFI_TIME..

**/
VOID
FatFatTimeToEfiTime (
  IN  FAT_DATE_TIME     *FTime,
  OUT EFI_TIME          *ETime
  )
{
  ETime->Year       = (UINT16) (FTime->Date.Year + 1980);
  ETime->Month      = (UINT8) FTime->Date.Month;
  ETime->Day        = (UINT8) FTime->Date.Day;
  ETime->Hour       = (UINT8) FTime->Time.Hour;
  ETime->Minute     = (UINT8) FTime->Time.Minute;
  ETime->Second     = (UINT8) (FTime->Time.DoubleSecond * 2);
  ETime->Nanosecond = 0;
  ETime->TimeZone   = EFI_UNSPECIFIED_TIMEZONE;
  ETime->Daylight   = 0;
}

/**

  Get Current FAT time.

  @param  FatNow                - Current FAT time.

**/
VOID
FatGetCurrentFatTime (
  OUT FAT_DATE_TIME   *FatNow
  )
{
  EFI_STATUS Status;
  EFI_TIME   Now;

  Status = gRT->GetTime (&Now, NULL);
  if (!EFI_ERROR (Status)) {
    FatEfiTimeToFatTime (&Now, FatNow);
  } else {
    ZeroMem (&Now, sizeof (EFI_TIME));
    Now.Year = 1980;
    Now.Month = 1;
    Now.Day = 1;
    FatEfiTimeToFatTime (&Now, FatNow);
  }
}

/**

  Check whether a time is valid.

  @param  Time                  - The time of EFI_TIME.

  @retval TRUE                  - The time is valid.
  @retval FALSE                 - The time is not valid.

**/
BOOLEAN
FatIsValidTime (
  IN EFI_TIME         *Time
  )
{
  UINTN         Day;
  BOOLEAN       ValidTime;

  ValidTime = TRUE;

  //
  // Check the fields for range problems
  // Fat can only support from 1980
  //
  if (Time->Year < 1980 ||
      Time->Month < 1 ||
      Time->Month > 12 ||
      Time->Day < 1 ||
      Time->Day > 31 ||
      Time->Hour > 23 ||
      Time->Minute > 59 ||
      Time->Second > 59 ||
      Time->Nanosecond > 999999999
      ) {

    ValidTime = FALSE;

  } else {
    //
    // Perform a more specific check of the day of the month
    //
    Day = mMonthDays[Time->Month - 1];
    if (Time->Month == 2 && IS_LEAP_YEAR (Time->Year)) {
      Day += 1;
      //
      // 1 extra day this month
      //
    }
    if (Time->Day > Day) {
      ValidTime = FALSE;
    }
  }

  return ValidTime;
}

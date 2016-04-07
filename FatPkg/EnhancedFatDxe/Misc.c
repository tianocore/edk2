/*++

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Misc.c

Abstract:

  Miscellaneous functions

Revision History

--*/

#include "Fat.h"

FAT_TASK *
FatCreateTask (
  FAT_IFILE           *IFile,
  EFI_FILE_IO_TOKEN   *Token
  )
/*++

Routine Description:

  Create the task

Arguments:

  IFile                 - The instance of the open file.
  Token                 - A pointer to the token associated with the transaction.

Return:
  FAT_TASK *            - Return the task instance.
**/
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

VOID
FatDestroyTask (
  FAT_TASK            *Task
  )
/*++

Routine Description:

  Destroy the task

Arguments:

  Task                  - The task to be destroyed.
**/
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

VOID
FatWaitNonblockingTask (
  FAT_IFILE           *IFile
  )
/*++

Routine Description:

  Wait all non-blocking requests complete.

Arguments:

  IFile                 - The instance of the open file.
**/
{
  BOOLEAN             TaskQueueEmpty;

  do {
    EfiAcquireLock (&FatTaskLock);
    TaskQueueEmpty = IsListEmpty (&IFile->Tasks);
    EfiReleaseLock (&FatTaskLock);
  } while (!TaskQueueEmpty);
}

LIST_ENTRY *
FatDestroySubtask (
  FAT_SUBTASK         *Subtask
  )
/*++

Routine Description:

  Remove the subtask from subtask list.

Arguments:

  Subtask               - The subtask to be removed.

Returns:

  LIST_ENTRY *          - The next node in the list.

--*/
{
  LIST_ENTRY          *Link;

  gBS->CloseEvent (Subtask->DiskIo2Token.Event);

  Link = RemoveEntryList (&Subtask->Link);
  FreePool (Subtask);

  return Link;
}

EFI_STATUS
FatQueueTask (
  IN FAT_IFILE        *IFile,
  IN FAT_TASK         *Task
  )
/*++

Routine Description:

  Execute the task

Arguments:

  IFile                 - The instance of the open file.
  Task                  - The task to be executed.

Returns:

  EFI_SUCCESS           - The task was executed sucessfully.
  other                 - An error occurred when executing the task.

--*/
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Link;
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
  for ( Link = GetFirstNode (&Task->Subtasks)
      ; !IsNull (&Task->Subtasks, Link)
      ; Link = GetNextNode (&Task->Subtasks, Link)
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

EFI_STATUS
FatAccessVolumeDirty (
  IN FAT_VOLUME       *Volume,
  IN IO_MODE          IoMode,
  IN VOID             *DirtyValue
  )
/*++

Routine Description:

  Set the volume as dirty or not

Arguments:

  Volume                - FAT file system volume.
  IoMode                - The access mode.
  DirtyValue            - Set the volume as dirty or not.

Returns:

  EFI_SUCCESS           - Set the new FAT entry value sucessfully.
  other                 - An error occurred when operation the FAT entries.

--*/
{
  UINTN WriteCount;

  WriteCount = Volume->FatEntrySize;
  return FatDiskIo (Volume, IoMode, Volume->FatPos + WriteCount, WriteCount, DirtyValue, NULL);
}

/**
  Invoke a notification event

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
/*++

Routine Description:

  Invoke a notification event
  case #1. some subtasks are not completed when the FatOpenEx checks the Task->Subtasks
           - sets Task->SubtaskCollected so callback to signal the event and free the task.
  case #2. all subtasks are completed when the FatOpenEx checks the Task->Subtasks
           - FatOpenEx signal the event and free the task.
Arguments:

  Event                 - Event whose notification function is being invoked.
  Context               - The pointer to the notification function's context,
                          which is implementation-dependent.

--*/
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

EFI_STATUS
FatDiskIo (
  IN     FAT_VOLUME       *Volume,
  IN     IO_MODE          IoMode,
  IN     UINT64           Offset,
  IN     UINTN            BufferSize,
  IN OUT VOID             *Buffer,
  IN     FAT_TASK         *Task
  )
/*++

Routine Description:

  General disk access function

Arguments:

  Volume                - FAT file system volume.
  IoMode                - The access mode (disk read/write or cache access).
  Offset                - The starting byte offset to read from.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing read data.

Returns:

  EFI_SUCCESS           - The operation is performed successfully.
  EFI_VOLUME_CORRUPTED  - The accesss is
  Others                - The status of read/write the disk

--*/
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
        IoFunction  = (IoMode == READ_DISK) ? DiskIo->ReadDisk : DiskIo->WriteDisk;
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
          Subtask->Write      = (BOOLEAN) (IoMode == WRITE_DISK);
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

VOID
FatAcquireLock (
  VOID
  )
/*++

Routine Description:

  Lock the volume.

Arguments:

  None.

Returns:

  None.

--*/
{
  EfiAcquireLock (&FatFsLock);
}

EFI_STATUS
FatAcquireLockOrFail (
  VOID
  )
/*++

Routine Description:

  Lock the volume.
  If the lock is already in the acquired state, then EFI_ACCESS_DENIED is returned.
  Otherwise, EFI_SUCCESS is returned.

Arguments:

  None.

Returns:

  EFI_SUCCESS           - The volume is locked.
  EFI_ACCESS_DENIED     - The volume could not be locked because it is already locked.

--*/
{
  return EfiAcquireLockOrFail (&FatFsLock);
}

VOID
FatReleaseLock (
  VOID
  )
/*++

Routine Description:

  Unlock the volume.

Arguments:

  Null.

Returns:

  None.

--*/
{
  EfiReleaseLock (&FatFsLock);
}

VOID
FatFreeDirEnt (
  IN FAT_DIRENT       *DirEnt
  )
/*++

Routine Description:

  Free directory entry.

Arguments:

  DirEnt                - The directory entry to be freed.

Returns:

  None.

--*/
{
  if (DirEnt->FileString != NULL) {
    FreePool (DirEnt->FileString);
  }

  FreePool (DirEnt);
}

VOID
FatFreeVolume (
  IN FAT_VOLUME       *Volume
  )
/*++

Routine Description:

  Free volume structure (including the contents of directory cache and disk cache).

Arguments:

  Volume                - The volume structure to be freed.

Returns:

  None.

--*/
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

VOID
FatEfiTimeToFatTime (
  IN  EFI_TIME        *ETime,
  OUT FAT_DATE_TIME   *FTime
  )
/*++

Routine Description:

  Translate EFI time to FAT time.

Arguments:

  ETime                 - The time of EFI_TIME.
  FTime                 - The time of FAT_DATE_TIME.

Returns:

  None.

--*/
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

VOID
FatFatTimeToEfiTime (
  IN  FAT_DATE_TIME     *FTime,
  OUT EFI_TIME          *ETime
  )
/*++

Routine Description:

  Translate Fat time to EFI time.

Arguments:

  FTime                 - The time of FAT_DATE_TIME.
  ETime                 - The time of EFI_TIME.

Returns:

  None.

--*/
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

VOID
FatGetCurrentFatTime (
  OUT FAT_DATE_TIME   *FatNow
  )
/*++

Routine Description:

  Get Current FAT time.

Arguments:

  FatNow                - Current FAT time.

Returns:

  None.

--*/
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

BOOLEAN
FatIsValidTime (
  IN EFI_TIME         *Time
  )
/*++

Routine Description:

  Check whether a time is valid.

Arguments:

  Time                  - The time of EFI_TIME.

Returns:

  TRUE                  - The time is valid.
  FALSE                 - The time is not valid.

--*/
{
  static UINT8  MonthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
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
    Day = MonthDays[Time->Month - 1];
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

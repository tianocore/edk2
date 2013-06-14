/*++

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
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
  return FatDiskIo (Volume, IoMode, Volume->FatPos + WriteCount, WriteCount, DirtyValue);
}

EFI_STATUS
FatDiskIo (
  IN     FAT_VOLUME       *Volume,
  IN     IO_MODE          IoMode,
  IN     UINT64           Offset,
  IN     UINTN            BufferSize,
  IN OUT VOID             *Buffer
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

  //
  // Verify the IO is in devices range
  //
  Status = EFI_VOLUME_CORRUPTED;
  if (Offset + BufferSize <= Volume->VolumeSize) {
    if (CACHE_ENABLED (IoMode)) {
      //
      // Access cache
      //
      Status = FatAccessCache (Volume, CACHE_TYPE (IoMode), RAW_ACCESS (IoMode), Offset, BufferSize, Buffer);
    } else {
      //
      // Access disk directly
      //
      DiskIo      = Volume->DiskIo;
      IoFunction  = (IoMode == READ_DISK) ? DiskIo->ReadDisk : DiskIo->WriteDisk;
      Status      = IoFunction (DiskIo, Volume->MediaId, Offset, BufferSize, Buffer);
    }
  }

  if (EFI_ERROR (Status)) {
    Volume->DiskError = TRUE;
    DEBUG ((EFI_D_INFO, "FatDiskIo: error %r\n", Status));
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

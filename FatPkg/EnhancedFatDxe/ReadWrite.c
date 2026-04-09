/** @file
  Functions that perform file read/write.

Copyright (c) 2005 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Fat.h"

/**

  Get the file's position of the file.


  @param  FHand                 - The handle of file.
  @param  Position              - The file's position of the file.

  @retval EFI_SUCCESS           - Get the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_UNSUPPORTED       - The open file is not a file.

**/
EFI_STATUS
EFIAPI
FatGetPosition (
  IN  EFI_FILE_PROTOCOL  *FHand,
  OUT UINT64             *Position
  )
{
  FAT_IFILE  *IFile;
  FAT_OFILE  *OFile;

  IFile = IFILE_FROM_FHAND (FHand);
  OFile = IFile->OFile;

  if (OFile->Error == EFI_NOT_FOUND) {
    return EFI_DEVICE_ERROR;
  }

  if (OFile->ODir != NULL) {
    return EFI_UNSUPPORTED;
  }

  *Position = IFile->Position;
  return EFI_SUCCESS;
}

/**

  Set the file's position of the file.

  @param  FHand                 - The handle of file.
  @param  Position              - The file's position of the file.

  @retval EFI_SUCCESS           - Set the info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_UNSUPPORTED       - Set a directory with a not-zero position.

**/
EFI_STATUS
EFIAPI
FatSetPosition (
  IN EFI_FILE_PROTOCOL  *FHand,
  IN UINT64             Position
  )
{
  FAT_IFILE  *IFile;
  FAT_OFILE  *OFile;

  IFile = IFILE_FROM_FHAND (FHand);
  OFile = IFile->OFile;

  if (OFile->Error == EFI_NOT_FOUND) {
    return EFI_DEVICE_ERROR;
  }

  FatWaitNonblockingTask (IFile);

  //
  // If this is a directory, we can only set back to position 0
  //
  if (OFile->ODir != NULL) {
    if (Position != 0) {
      //
      // Reset current directory cursor;
      //
      return EFI_UNSUPPORTED;
    }

    FatResetODirCursor (OFile);
  }

  //
  // Set the position
  //
  if (Position == (UINT64)-1) {
    Position = OFile->FileSize;
  }

  //
  // Set the position
  //
  IFile->Position = Position;
  return EFI_SUCCESS;
}

/**

  Get the file info from the open file of the IFile into Buffer.

  @param  IFile                 - The instance of the open file.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing read data.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval other                 - An error occurred when operation the disk.

**/
EFI_STATUS
FatIFileReadDir (
  IN     FAT_IFILE  *IFile,
  IN OUT UINTN      *BufferSize,
  OUT VOID          *Buffer
  )
{
  EFI_STATUS  Status;
  FAT_OFILE   *OFile;
  FAT_ODIR    *ODir;
  FAT_DIRENT  *DirEnt;
  UINT32      CurrentPos;

  OFile      = IFile->OFile;
  ODir       = OFile->ODir;
  CurrentPos = ((UINT32)IFile->Position) / sizeof (FAT_DIRECTORY_ENTRY);

  //
  // We need to relocate the directory
  //
  if (CurrentPos < ODir->CurrentPos) {
    //
    // The directory cursor has been modified by another IFile, we reset the cursor
    //
    FatResetODirCursor (OFile);
  }

  //
  // We seek the next directory entry's position
  //
  do {
    Status = FatGetNextDirEnt (OFile, &DirEnt);
    if (EFI_ERROR (Status) || (DirEnt == NULL)) {
      //
      // Something error occurred or reach the end of directory,
      // return 0 buffersize
      //
      *BufferSize = 0;
      goto Done;
    }
  } while (ODir->CurrentPos <= CurrentPos);

  Status = FatGetDirEntInfo (OFile->Volume, DirEnt, BufferSize, Buffer);

Done:
  //
  // Update IFile's Position
  //
  if (!EFI_ERROR (Status)) {
    //
    // Update IFile->Position, if everything is all right
    //
    CurrentPos      = ODir->CurrentPos;
    IFile->Position = CurrentPos * sizeof (FAT_DIRECTORY_ENTRY);
  }

  return Status;
}

/**

  Get the file info from the open file of the IFile into Buffer.

  @param FHand                 - The file handle to access.
  @param  IoMode                - Indicate whether the access mode is reading or writing.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing read data.
  @param  Token                 - A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @retval EFI_WRITE_PROTECTED   - The disk is write protect.
  @retval EFI_ACCESS_DENIED     - The file is read-only.
  @return other                 - An error occurred when operating on the disk.

**/
EFI_STATUS
FatIFileAccess (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN     IO_MODE            IoMode,
  IN OUT UINTN              *BufferSize,
  IN OUT VOID               *Buffer,
  IN     EFI_FILE_IO_TOKEN  *Token
  )
{
  EFI_STATUS  Status;
  FAT_IFILE   *IFile;
  FAT_OFILE   *OFile;
  FAT_VOLUME  *Volume;
  UINT64      EndPosition;
  FAT_TASK    *Task;

  IFile  = IFILE_FROM_FHAND (FHand);
  OFile  = IFile->OFile;
  Volume = OFile->Volume;
  Task   = NULL;

  //
  // Write to a directory is unsupported
  //
  if ((OFile->ODir != NULL) && (IoMode == WriteData)) {
    return EFI_UNSUPPORTED;
  }

  if (OFile->Error == EFI_NOT_FOUND) {
    return EFI_DEVICE_ERROR;
  }

  if (IoMode == ReadData) {
    //
    // If position is at EOF, then return device error
    //
    if (IFile->Position > OFile->FileSize) {
      return EFI_DEVICE_ERROR;
    }
  } else {
    //
    // Check if the we can write data
    //
    if (Volume->ReadOnly) {
      return EFI_WRITE_PROTECTED;
    }

    if (IFile->ReadOnly) {
      return EFI_ACCESS_DENIED;
    }
  }

  if (Token == NULL) {
    FatWaitNonblockingTask (IFile);
  } else {
    //
    // Caller shouldn't call the non-blocking interfaces if the low layer doesn't support DiskIo2.
    // But if it calls, the below check can avoid crash.
    //
    if (FHand->Revision < EFI_FILE_PROTOCOL_REVISION2) {
      return EFI_UNSUPPORTED;
    }

    Task = FatCreateTask (IFile, Token);
    if (Task == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  FatAcquireLock ();

  Status = OFile->Error;
  if (!EFI_ERROR (Status)) {
    if (OFile->ODir != NULL) {
      //
      // Read a directory is supported
      //
      ASSERT (IoMode == ReadData);
      Status = FatIFileReadDir (IFile, BufferSize, Buffer);
      OFile  = NULL;
    } else {
      //
      // Access a file
      //
      EndPosition = IFile->Position + *BufferSize;
      if (EndPosition > OFile->FileSize) {
        //
        // The position goes beyond the end of file
        //
        if (IoMode == ReadData) {
          //
          // Adjust the actual size read
          //
          *BufferSize -= (UINTN)EndPosition - OFile->FileSize;
        } else {
          //
          // We expand the file size of OFile
          //
          Status = FatGrowEof (OFile, EndPosition);
          if (EFI_ERROR (Status)) {
            //
            // Must update the file's info into the file's Directory Entry
            // and then flush the dirty cache info into disk.
            //
            *BufferSize = 0;
            FatOFileFlush (OFile);
            OFile = NULL;
            goto Done;
          }

          FatUpdateDirEntClusterSizeInfo (OFile);
        }
      }

      Status           = FatAccessOFile (OFile, IoMode, (UINTN)IFile->Position, BufferSize, Buffer, Task);
      IFile->Position += *BufferSize;
    }
  }

  if (Token != NULL) {
    if (!EFI_ERROR (Status)) {
      Status = FatQueueTask (IFile, Task);
    } else {
      FatDestroyTask (Task);
    }
  }

Done:
  //
  // On EFI_SUCCESS case, not calling FatCleanupVolume():
  // 1) The Cache flush operation is avoided to enhance
  // performance. Caller is responsible to call Flush() when necessary.
  // 2) The volume dirty bit is probably set already, and is expected to be
  // cleaned in subsequent Flush() or other operations.
  // 3) Write operation doesn't affect OFile/IFile structure, so
  // Reference checking is not necessary.
  //
  if (EFI_ERROR (Status)) {
    Status = FatCleanupVolume (Volume, OFile, Status, NULL);
  }

  FatReleaseLock ();
  return Status;
}

/**

  Get the file info.

  @param  FHand                 - The handle of the file.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing read data.


  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatRead (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  )
{
  return FatIFileAccess (FHand, ReadData, BufferSize, Buffer, NULL);
}

/**

  Get the file info.

  @param  FHand                 - The handle of the file.
  @param  Token                 - A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatReadEx (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  return FatIFileAccess (FHand, ReadData, &Token->BufferSize, Token->Buffer, Token);
}

/**

  Write the content of buffer into files.

  @param  FHand                 - The handle of the file.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing write data.

  @retval EFI_SUCCESS           - Set the file info successfully.
  @retval EFI_WRITE_PROTECTED   - The disk is write protect.
  @retval EFI_ACCESS_DENIED     - The file is read-only.
  @retval EFI_DEVICE_ERROR      - The OFile is not valid.
  @retval EFI_UNSUPPORTED       - The open file is not a file.
                        - The writing file size is larger than 4GB.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatWrite (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  )
{
  return FatIFileAccess (FHand, WriteData, BufferSize, Buffer, NULL);
}

/**

  Get the file info.

  @param  FHand                 - The handle of the file.
  @param  Token                 - A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_DEVICE_ERROR      - Can not find the OFile for the file.
  @retval EFI_VOLUME_CORRUPTED  - The file type of open file is error.
  @return other                 - An error occurred when operation the disk.

**/
EFI_STATUS
EFIAPI
FatWriteEx (
  IN     EFI_FILE_PROTOCOL  *FHand,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  return FatIFileAccess (FHand, WriteData, &Token->BufferSize, Token->Buffer, Token);
}

/**

  This function reads data from a file or writes data to a file.
  It uses OFile->PosRem to determine how much data can be accessed in one time.

  @param  OFile                 - The open file.
  @param  IoMode                - Indicate whether the access mode is reading or writing.
  @param  Position              - The position where data will be accessed.
  @param  DataBufferSize        - Size of Buffer.
  @param  UserBuffer            - Buffer containing data.
  @param  Task                    point to task instance.

  @retval EFI_SUCCESS           - Access the data successfully.
  @return other                 - An error occurred when operating on the disk.

**/
EFI_STATUS
FatAccessOFile (
  IN     FAT_OFILE  *OFile,
  IN     IO_MODE    IoMode,
  IN     UINTN      Position,
  IN OUT UINTN      *DataBufferSize,
  IN OUT UINT8      *UserBuffer,
  IN FAT_TASK       *Task
  )
{
  FAT_VOLUME  *Volume;
  UINTN       Len;
  EFI_STATUS  Status;
  UINTN       BufferSize;

  BufferSize = *DataBufferSize;
  Volume     = OFile->Volume;
  ASSERT_VOLUME_LOCKED (Volume);

  Status = EFI_SUCCESS;
  while (BufferSize > 0) {
    //
    // Seek the OFile to the file position
    //
    Status = FatOFilePosition (OFile, Position, BufferSize);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Clip length to block run
    //
    Len = BufferSize > OFile->PosRem ? OFile->PosRem : BufferSize;

    //
    // Write the data
    //
    Status = FatDiskIo (Volume, IoMode, OFile->PosDisk, Len, UserBuffer, Task);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Data was successfully accessed
    //
    Position   += Len;
    UserBuffer += Len;
    BufferSize -= Len;
    if (IoMode == WriteData) {
      OFile->Dirty   = TRUE;
      OFile->Archive = TRUE;
    }

    //
    // Make sure no outbound occurred
    //
    ASSERT (Position <= OFile->FileSize);
  }

  //
  // Update the number of bytes accessed
  //
  *DataBufferSize -= BufferSize;
  return Status;
}

/**

  Expand OFile by appending zero bytes at the end of OFile.

  @param  OFile                 - The open file.
  @param  ExpandedSize          - The number of zero bytes appended at the end of the file.

  @retval EFI_SUCCESS           - The file is expanded successfully.
  @return other                 - An error occurred when expanding file.

**/
EFI_STATUS
FatExpandOFile (
  IN FAT_OFILE  *OFile,
  IN UINT64     ExpandedSize
  )
{
  EFI_STATUS  Status;
  UINTN       WritePos;

  WritePos = OFile->FileSize;
  Status   = FatGrowEof (OFile, ExpandedSize);
  if (!EFI_ERROR (Status)) {
    Status = FatWriteZeroPool (OFile, WritePos);
  }

  return Status;
}

/**

  Write zero pool from the WritePos to the end of OFile.

  @param  OFile                 - The open file to write zero pool.
  @param  WritePos              - The number of zero bytes written.

  @retval EFI_SUCCESS           - Write the zero pool successfully.
  @retval EFI_OUT_OF_RESOURCES  - Not enough memory to perform the operation.
  @return other                 - An error occurred when writing disk.

**/
EFI_STATUS
FatWriteZeroPool (
  IN FAT_OFILE  *OFile,
  IN UINTN      WritePos
  )
{
  EFI_STATUS  Status;
  VOID        *ZeroBuffer;
  UINTN       AppendedSize;
  UINTN       BufferSize;
  UINTN       WriteSize;

  AppendedSize = OFile->FileSize - WritePos;
  BufferSize   = AppendedSize;
  if (AppendedSize > FAT_MAX_ALLOCATE_SIZE) {
    //
    // If the appended size is larger, maybe we can not allocate the whole
    // memory once. So if the growed size is larger than 10M, we just
    // allocate 10M memory (one healthy system should have 10M available
    // memory), and then write the zerobuffer to the file several times.
    //
    BufferSize = FAT_MAX_ALLOCATE_SIZE;
  }

  ZeroBuffer = AllocateZeroPool (BufferSize);
  if (ZeroBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  do {
    WriteSize     = AppendedSize > BufferSize ? BufferSize : (UINTN)AppendedSize;
    AppendedSize -= WriteSize;
    Status        = FatAccessOFile (OFile, WriteData, WritePos, &WriteSize, ZeroBuffer, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }

    WritePos += WriteSize;
  } while (AppendedSize > 0);

  FreePool (ZeroBuffer);
  return Status;
}

/**

  Truncate the OFile to smaller file size.

  @param  OFile                 - The open file.
  @param  TruncatedSize         - The new file size.

  @retval EFI_SUCCESS           - The file is truncated successfully.
  @return other                 - An error occurred when truncating file.

**/
EFI_STATUS
FatTruncateOFile (
  IN FAT_OFILE  *OFile,
  IN UINTN      TruncatedSize
  )
{
  OFile->FileSize = TruncatedSize;
  return FatShrinkEof (OFile);
}

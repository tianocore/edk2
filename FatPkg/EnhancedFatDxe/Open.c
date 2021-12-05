/** @file
  Routines dealing with file open.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

/**

  Create an Open instance for the existing OFile.
  The IFile of the newly opened file is passed out.

  @param  OFile                 - The file that serves as a starting reference point.
  @param  PtrIFile              - The newly generated IFile instance.

  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for the IFile
  @retval EFI_SUCCESS           - Create the new IFile for the OFile successfully

**/
EFI_STATUS
FatAllocateIFile (
  IN FAT_OFILE   *OFile,
  OUT FAT_IFILE  **PtrIFile
  )
{
  FAT_IFILE  *IFile;

  ASSERT_VOLUME_LOCKED (OFile->Volume);

  //
  // Allocate a new open instance
  //
  IFile = AllocateZeroPool (sizeof (FAT_IFILE));
  if (IFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IFile->Signature = FAT_IFILE_SIGNATURE;

  CopyMem (&(IFile->Handle), &FatFileInterface, sizeof (EFI_FILE_PROTOCOL));

  //
  // Report the correct revision number based on the DiskIo2 availability
  //
  if (OFile->Volume->DiskIo2 != NULL) {
    IFile->Handle.Revision = EFI_FILE_PROTOCOL_REVISION2;
  } else {
    IFile->Handle.Revision = EFI_FILE_PROTOCOL_REVISION;
  }

  IFile->OFile = OFile;
  InsertTailList (&OFile->Opens, &IFile->Link);
  InitializeListHead (&IFile->Tasks);

  *PtrIFile = IFile;
  return EFI_SUCCESS;
}

/**

  Open a file for a file name relative to an existing OFile.
  The IFile of the newly opened file is passed out.

  @param  OFile                 - The file that serves as a starting reference point.
  @param  NewIFile              - The newly generated IFile instance.
  @param  FileName              - The file name relative to the OFile.
  @param  OpenMode              - Open mode.
  @param  Attributes            - Attributes to set if the file is created.


  @retval EFI_SUCCESS           - Open the file successfully.
  @retval EFI_INVALID_PARAMETER - The open mode is conflict with the attributes
                          or the file name is not valid.
  @retval EFI_NOT_FOUND         - Conflicts between dir intention and attribute.
  @retval EFI_WRITE_PROTECTED   - Can't open for write if the volume is read only.
  @retval EFI_ACCESS_DENIED     - If the file's attribute is read only, and the
                          open is for read-write fail it.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.

**/
EFI_STATUS
FatOFileOpen (
  IN  FAT_OFILE  *OFile,
  OUT FAT_IFILE  **NewIFile,
  IN  CHAR16     *FileName,
  IN  UINT64     OpenMode,
  IN  UINT8      Attributes
  )
{
  FAT_VOLUME  *Volume;
  EFI_STATUS  Status;
  CHAR16      NewFileName[EFI_PATH_STRING_LENGTH];
  FAT_DIRENT  *DirEnt;
  UINT8       FileAttributes;
  BOOLEAN     WriteMode;

  DirEnt = NULL;
  Volume = OFile->Volume;
  ASSERT_VOLUME_LOCKED (Volume);
  WriteMode = (BOOLEAN)(OpenMode & EFI_FILE_MODE_WRITE);
  if (Volume->ReadOnly && WriteMode) {
    return EFI_WRITE_PROTECTED;
  }

  //
  // Verify the source file handle isn't in an error state
  //
  Status = OFile->Error;
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get new OFile for the file
  //
  Status = FatLocateOFile (&OFile, FileName, Attributes, NewFileName);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*NewFileName != 0) {
    //
    // If there's a remaining part of the name, then we had
    // better be creating the file in the directory
    //
    if ((OpenMode & EFI_FILE_MODE_CREATE) == 0) {
      return EFI_NOT_FOUND;
    }

    Status = FatCreateDirEnt (OFile, NewFileName, Attributes, &DirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ASSERT (DirEnt != NULL);
    Status = FatOpenDirEnt (OFile, DirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    OFile = DirEnt->OFile;
    if (OFile->ODir != NULL) {
      //
      // If we just created a directory, we need to create "." and ".."
      //
      Status = FatCreateDotDirEnts (OFile);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // If the file's attribute is read only, and the open is for
  // read-write, then the access is denied.
  //
  FileAttributes = OFile->DirEnt->Entry.Attributes;
  if (((FileAttributes & EFI_FILE_READ_ONLY) != 0) && ((FileAttributes & FAT_ATTRIBUTE_DIRECTORY) == 0) && WriteMode) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Create an open instance of the OFile
  //
  Status = FatAllocateIFile (OFile, NewIFile);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  (*NewIFile)->ReadOnly = (BOOLEAN) !WriteMode;

  DEBUG ((DEBUG_INFO, "FSOpen: Open '%S' %r\n", FileName, Status));
  return FatOFileFlush (OFile);
}

/**

  Implements OpenEx() of Simple File System Protocol.

  @param  FHand                 - File handle of the file serves as a starting reference point.
  @param  NewHandle             - Handle of the file that is newly opened.
  @param  FileName              - File name relative to FHand.
  @param  OpenMode              - Open mode.
  @param  Attributes            - Attributes to set if the file is created.
  @param  Token                 - A pointer to the token associated with the transaction.:

  @retval EFI_INVALID_PARAMETER - The FileName is NULL or the file string is empty.
                          The OpenMode is not supported.
                          The Attributes is not the valid attributes.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for file string.
  @retval EFI_SUCCESS           - Open the file successfully.
  @return Others                - The status of open file.

**/
EFI_STATUS
EFIAPI
FatOpenEx (
  IN  EFI_FILE_PROTOCOL     *FHand,
  OUT EFI_FILE_PROTOCOL     **NewHandle,
  IN  CHAR16                *FileName,
  IN  UINT64                OpenMode,
  IN  UINT64                Attributes,
  IN OUT EFI_FILE_IO_TOKEN  *Token
  )
{
  FAT_IFILE   *IFile;
  FAT_IFILE   *NewIFile;
  FAT_OFILE   *OFile;
  EFI_STATUS  Status;
  FAT_TASK    *Task;

  //
  // Perform some parameter checking
  //
  if (FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for a valid mode
  //
  switch (OpenMode) {
    case EFI_FILE_MODE_READ:
    case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
    case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE:
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // Check for valid Attributes for file creation case.
  //
  if (((OpenMode & EFI_FILE_MODE_CREATE) != 0) && ((Attributes & (EFI_FILE_READ_ONLY | (~EFI_FILE_VALID_ATTR))) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  IFile = IFILE_FROM_FHAND (FHand);
  OFile = IFile->OFile;
  Task  = NULL;

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

  //
  // Lock
  //
  FatAcquireLock ();

  //
  // Open the file
  //
  Status = FatOFileOpen (OFile, &NewIFile, FileName, OpenMode, (UINT8)Attributes);

  //
  // If the file was opened, return the handle to the caller
  //
  if (!EFI_ERROR (Status)) {
    *NewHandle = &NewIFile->Handle;
  }

  //
  // Unlock
  //
  Status = FatCleanupVolume (OFile->Volume, NULL, Status, Task);
  FatReleaseLock ();

  if (Token != NULL) {
    if (!EFI_ERROR (Status)) {
      Status = FatQueueTask (IFile, Task);
    } else {
      FatDestroyTask (Task);
    }
  }

  return Status;
}

/**

  Implements Open() of Simple File System Protocol.


  @param   FHand                 - File handle of the file serves as a starting reference point.
  @param   NewHandle             - Handle of the file that is newly opened.
  @param   FileName              - File name relative to FHand.
  @param   OpenMode              - Open mode.
  @param   Attributes            - Attributes to set if the file is created.

  @retval EFI_INVALID_PARAMETER - The FileName is NULL or the file string is empty.
                          The OpenMode is not supported.
                          The Attributes is not the valid attributes.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory for file string.
  @retval EFI_SUCCESS           - Open the file successfully.
  @return Others                - The status of open file.

**/
EFI_STATUS
EFIAPI
FatOpen (
  IN  EFI_FILE_PROTOCOL  *FHand,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN  CHAR16             *FileName,
  IN  UINT64             OpenMode,
  IN  UINT64             Attributes
  )
{
  return FatOpenEx (FHand, NewHandle, FileName, OpenMode, Attributes, NULL);
}

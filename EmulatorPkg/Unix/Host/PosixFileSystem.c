/*++ @file
 POSIX Pthreads to emulate APs and implement threads

Copyright (c) 2011, Apple Inc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Host.h"


#define EMU_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE SIGNATURE_32 ('E', 'P', 'f', 's')

typedef struct {
  UINTN                           Signature;
  EMU_IO_THUNK_PROTOCOL           *Thunk;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL SimpleFileSystem;
  CHAR8                           *FilePath;
  CHAR16                          *VolumeLabel;
  BOOLEAN                         FileHandlesOpen;
} EMU_SIMPLE_FILE_SYSTEM_PRIVATE;

#define EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      EMU_SIMPLE_FILE_SYSTEM_PRIVATE, \
      SimpleFileSystem, \
      EMU_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE \
      )


#define EMU_EFI_FILE_PRIVATE_SIGNATURE SIGNATURE_32 ('E', 'P', 'f', 'i')

typedef struct {
  UINTN                           Signature;
  EMU_IO_THUNK_PROTOCOL           *Thunk;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
  EFI_FILE_PROTOCOL               EfiFile;
  int                             fd;
  DIR                             *Dir;
  BOOLEAN                         IsRootDirectory;
  BOOLEAN                         IsDirectoryPath;
  BOOLEAN                         IsOpenedByRead;
  char                            *FileName;
  struct dirent                   *Dirent;
} EMU_EFI_FILE_PRIVATE;

#define EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      EMU_EFI_FILE_PRIVATE, \
      EfiFile, \
      EMU_EFI_FILE_PRIVATE_SIGNATURE \
      )

EFI_STATUS
PosixFileGetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  );

EFI_STATUS
PosixFileSetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  );


EFI_FILE_PROTOCOL gPosixFileProtocol = {
  EFI_FILE_REVISION,
  GasketPosixFileOpen,
  GasketPosixFileCLose,
  GasketPosixFileDelete,
  GasketPosixFileRead,
  GasketPosixFileWrite,
  GasketPosixFileGetPossition,
  GasketPosixFileSetPossition,
  GasketPosixFileGetInfo,
  GasketPosixFileSetInfo,
  GasketPosixFileFlush
};

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL gPosixFileSystemProtocol = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  GasketPosixOpenVolume,
};


/**
  Open the root directory on a volume.

  @param  This Protocol instance pointer.
  @param  Root Returns an Open file handle for the root directory

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_UNSUPPORTED      This volume does not support the file system.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources.

**/
EFI_STATUS
PosixOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *This,
  OUT EFI_FILE_PROTOCOL                 **Root
  )
{
  EFI_STATUS                        Status;
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE    *Private;
  EMU_EFI_FILE_PRIVATE              *PrivateFile;

  Private     = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (This);

  Status = EFI_OUT_OF_RESOURCES;
  PrivateFile = malloc (sizeof (EMU_EFI_FILE_PRIVATE));
  if (PrivateFile == NULL) {
    goto Done;
  }

  PrivateFile->FileName = malloc (AsciiStrSize (Private->FilePath));
  if (PrivateFile->FileName == NULL) {
    goto Done;
  }
  AsciiStrCpy (PrivateFile->FileName, Private->FilePath);

  PrivateFile->Signature            = EMU_EFI_FILE_PRIVATE_SIGNATURE;
  PrivateFile->Thunk                = Private->Thunk;
  PrivateFile->SimpleFileSystem     = This;
  PrivateFile->IsRootDirectory      = TRUE;
  PrivateFile->IsDirectoryPath      = TRUE;
  PrivateFile->IsOpenedByRead       = TRUE;

  CopyMem (&PrivateFile->EfiFile, &gPosixFileProtocol, sizeof (EFI_FILE_PROTOCOL));

  PrivateFile->fd                   = -1;
  PrivateFile->Dir                  = NULL;
  PrivateFile->Dirent               = NULL;

  *Root = &PrivateFile->EfiFile;

  PrivateFile->Dir = opendir (PrivateFile->FileName);
  if (PrivateFile->Dir == NULL) {
    Status = EFI_ACCESS_DENIED;
  } else {
    Status = EFI_SUCCESS;
  }

Done:
  if (EFI_ERROR (Status)) {
    if (PrivateFile != NULL) {
      if (PrivateFile->FileName != NULL) {
        free (PrivateFile->FileName);
      }

      free (PrivateFile);
    }

    *Root = NULL;
  }

  return Status;
}


EFI_STATUS
ErrnoToEfiStatus ()
{
  switch (errno) {
  case EACCES:
    return EFI_ACCESS_DENIED;

  case EDQUOT:
  case ENOSPC:
    return EFI_VOLUME_FULL;

  default:
    return EFI_DEVICE_ERROR;
  }
}

VOID
CutPrefix (
  IN  CHAR8  *Str,
  IN  UINTN   Count
  )
{
  CHAR8  *Pointer;

  if (AsciiStrLen (Str) < Count) {
    ASSERT (0);
  }

  for (Pointer = Str; *(Pointer + Count); Pointer++) {
    *Pointer = *(Pointer + Count);
  }

  *Pointer = *(Pointer + Count);
}


VOID
PosixSystemTimeToEfiTime (
  IN  time_t                SystemTime,
  OUT EFI_TIME              *Time
  )
{
  struct tm *tm;

  tm           = gmtime (&SystemTime);
  Time->Year   = tm->tm_year;
  Time->Month  = tm->tm_mon + 1;
  Time->Day    = tm->tm_mday;
  Time->Hour   = tm->tm_hour;
  Time->Minute = tm->tm_min;
  Time->Second = tm->tm_sec;
  Time->Nanosecond = 0;

  Time->TimeZone = timezone;
  Time->Daylight = (daylight ? EFI_TIME_ADJUST_DAYLIGHT : 0) | (tm->tm_isdst > 0 ? EFI_TIME_IN_DAYLIGHT : 0);
}


EFI_STATUS
UnixSimpleFileSystemFileInfo (
  EMU_EFI_FILE_PRIVATE            *PrivateFile,
  IN     CHAR8                    *FileName,
  IN OUT UINTN                    *BufferSize,
  OUT    VOID                     *Buffer
  )
{
  EFI_STATUS                  Status;
  UINTN                       Size;
  UINTN                       NameSize;
  UINTN                       ResultSize;
  EFI_FILE_INFO               *Info;
  CHAR8                       *RealFileName;
  CHAR8                       *TempPointer;
  CHAR16                      *BufferFileName;
  struct stat                 buf;

  if (FileName != NULL) {
    RealFileName = FileName;
  } else if (PrivateFile->IsRootDirectory) {
    RealFileName = "";
  } else {
    RealFileName  = PrivateFile->FileName;
  }

  TempPointer = RealFileName;
  while (*TempPointer) {
    if (*TempPointer == '/') {
      RealFileName = TempPointer + 1;
    }

    TempPointer++;
  }

  Size        = SIZE_OF_EFI_FILE_INFO;
  NameSize    = AsciiStrSize (RealFileName) * 2;
  ResultSize  = Size + NameSize;

  if (*BufferSize < ResultSize) {
    *BufferSize = ResultSize;
    return EFI_BUFFER_TOO_SMALL;
  }
  if (stat (FileName == NULL ? PrivateFile->FileName : FileName, &buf) < 0) {
    return EFI_DEVICE_ERROR;
  }

  Status  = EFI_SUCCESS;

  Info    = Buffer;
  ZeroMem (Info, ResultSize);

  Info->Size          = ResultSize;
  Info->FileSize      = buf.st_size;
  Info->PhysicalSize  = MultU64x32 (buf.st_blocks, buf.st_blksize);

  PosixSystemTimeToEfiTime (buf.st_ctime, &Info->CreateTime);
  PosixSystemTimeToEfiTime (buf.st_atime, &Info->LastAccessTime);
  PosixSystemTimeToEfiTime (buf.st_mtime, &Info->ModificationTime);

  if (!(buf.st_mode & S_IWUSR)) {
    Info->Attribute |= EFI_FILE_READ_ONLY;
  }

  if (S_ISDIR(buf.st_mode)) {
    Info->Attribute |= EFI_FILE_DIRECTORY;
  }


  BufferFileName = (CHAR16 *)((CHAR8 *) Buffer + Size);
  while (*RealFileName) {
    *BufferFileName++ = *RealFileName++;
  }
  *BufferFileName = 0;

  *BufferSize = ResultSize;
  return Status;
}

BOOLEAN
IsZero (
  IN VOID   *Buffer,
  IN UINTN  Length
  )
{
  if (Buffer == NULL || Length == 0) {
    return FALSE;
  }

  if (*(UINT8 *) Buffer != 0) {
    return FALSE;
  }

  if (Length > 1) {
    if (!CompareMem (Buffer, (UINT8 *) Buffer + 1, Length - 1)) {
      return FALSE;
    }
  }

  return TRUE;
}



/**
  Opens a new file relative to the source file's location.

  @param  This       The protocol instance pointer.
  @param  NewHandle  Returns File Handle for FileName.
  @param  FileName   Null terminated string. "\", ".", and ".." are supported.
  @param  OpenMode   Open mode for file.
  @param  Attributes Only used for EFI_FILE_MODE_CREATE.

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_NOT_FOUND        The specified file could not be found on the device.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_MEDIA_CHANGED    The media has changed.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
PosixFileOpen (
  IN EFI_FILE_PROTOCOL        *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN CHAR16                   *FileName,
  IN UINT64                   OpenMode,
  IN UINT64                   Attributes
  )
{
  EFI_FILE_PROTOCOL                 *Root;
  EMU_EFI_FILE_PRIVATE              *PrivateFile;
  EMU_EFI_FILE_PRIVATE              *NewPrivateFile;
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE    *PrivateRoot;
  EFI_STATUS                        Status;
  CHAR16                            *Src;
  char                              *Dst;
  CHAR8                             *RealFileName;
  char                              *ParseFileName;
  char                              *GuardPointer;
  CHAR8                             TempChar;
  UINTN                             Count;
  BOOLEAN                           TrailingDash;
  BOOLEAN                           LoopFinish;
  UINTN                             InfoSize;
  EFI_FILE_INFO                     *Info;
  struct stat                       finfo;
  int                               res;


  PrivateFile     = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot     = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);
  NewPrivateFile  = NULL;
  Status          = EFI_OUT_OF_RESOURCES;

  //
  // BUGBUG: assume an open of root
  // if current location, return current data
  //
  TrailingDash = FALSE;
  if ((StrCmp (FileName, L"\\") == 0) ||
      (StrCmp (FileName, L".") == 0 && PrivateFile->IsRootDirectory)) {
OpenRoot:
    Status          = PosixOpenVolume (PrivateFile->SimpleFileSystem, &Root);
    NewPrivateFile  = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (Root);
    goto Done;
  }

  if (FileName[StrLen (FileName) - 1] == L'\\') {
    TrailingDash = TRUE;
    FileName[StrLen (FileName) - 1]  = 0;
  }

  //
  // Attempt to open the file
  //
  NewPrivateFile = malloc (sizeof (EMU_EFI_FILE_PRIVATE));
  if (NewPrivateFile == NULL) {
    goto Done;
  }

  CopyMem (NewPrivateFile, PrivateFile, sizeof (EMU_EFI_FILE_PRIVATE));

  NewPrivateFile->FileName = malloc (AsciiStrSize (PrivateFile->FileName) + 1 + StrLen (FileName) + 1);
  if (NewPrivateFile->FileName == NULL) {
    goto Done;
  }

  if (*FileName == L'\\') {
    AsciiStrCpy (NewPrivateFile->FileName, PrivateRoot->FilePath);
    // Skip first '\'.
    Src = FileName + 1;
  } else {
    AsciiStrCpy (NewPrivateFile->FileName, PrivateFile->FileName);
    Src = FileName;
  }
  Dst = NewPrivateFile->FileName + AsciiStrLen (NewPrivateFile->FileName);
  GuardPointer = NewPrivateFile->FileName + AsciiStrLen (PrivateRoot->FilePath);
  *Dst++ = '/';
  // Convert unicode to ascii and '\' to '/'
  while (*Src) {
    if (*Src == '\\') {
      *Dst++ = '/';
    } else {
      *Dst++ = *Src;
    }
    Src++;
  }
  *Dst = 0;


  //
  // Get rid of . and .., except leading . or ..
  //

  //
  // GuardPointer protect simplefilesystem root path not be destroyed
  //

  LoopFinish    = FALSE;
  while (!LoopFinish) {
    LoopFinish = TRUE;

    for (ParseFileName = GuardPointer; *ParseFileName; ParseFileName++) {
      if (*ParseFileName == '.' &&
          (*(ParseFileName + 1) == 0 || *(ParseFileName + 1) == '/') &&
          *(ParseFileName - 1) == '/'
          ) {

        //
        // cut /.
        //
        CutPrefix (ParseFileName - 1, 2);
        LoopFinish = FALSE;
        break;
      }

      if (*ParseFileName == '.' &&
          *(ParseFileName + 1) == '.' &&
          (*(ParseFileName + 2) == 0 || *(ParseFileName + 2) == '/') &&
          *(ParseFileName - 1) == '/'
          ) {

        ParseFileName--;
        Count = 3;

        while (ParseFileName != GuardPointer) {
          ParseFileName--;
          Count++;
          if (*ParseFileName == '/') {
            break;
          }
        }

        //
        // cut /.. and its left directory
        //
        CutPrefix (ParseFileName, Count);
        LoopFinish = FALSE;
        break;
      }
    }
  }

  if (AsciiStrCmp (NewPrivateFile->FileName, PrivateRoot->FilePath) == 0) {
    NewPrivateFile->IsRootDirectory = TRUE;
    free (NewPrivateFile->FileName);
    free (NewPrivateFile);
    goto OpenRoot;
  }

  RealFileName = NewPrivateFile->FileName + AsciiStrLen(NewPrivateFile->FileName) - 1;
  while (RealFileName > NewPrivateFile->FileName && *RealFileName != '/') {
    RealFileName--;
  }

  TempChar            = *(RealFileName - 1);
  *(RealFileName - 1) = 0;
  *(RealFileName - 1) = TempChar;


  //
  // Test whether file or directory
  //
  NewPrivateFile->IsRootDirectory = FALSE;
  NewPrivateFile->fd = -1;
  NewPrivateFile->Dir = NULL;
  if (OpenMode & EFI_FILE_MODE_CREATE) {
    if (Attributes & EFI_FILE_DIRECTORY) {
      NewPrivateFile->IsDirectoryPath = TRUE;
    } else {
      NewPrivateFile->IsDirectoryPath = FALSE;
    }
  } else {
    res = stat (NewPrivateFile->FileName, &finfo);
    if (res == 0 && S_ISDIR(finfo.st_mode)) {
      NewPrivateFile->IsDirectoryPath = TRUE;
    } else {
      NewPrivateFile->IsDirectoryPath = FALSE;
    }
  }

  if (OpenMode & EFI_FILE_MODE_WRITE) {
    NewPrivateFile->IsOpenedByRead = FALSE;
  } else {
    NewPrivateFile->IsOpenedByRead = TRUE;
  }

  Status = EFI_SUCCESS;

  //
  // deal with directory
  //
  if (NewPrivateFile->IsDirectoryPath) {
    if ((OpenMode & EFI_FILE_MODE_CREATE)) {
      //
      // Create a directory
      //
      if (mkdir (NewPrivateFile->FileName, 0777) != 0) {
        if (errno != EEXIST) {
          //free (TempFileName);
          Status = EFI_ACCESS_DENIED;
          goto Done;
        }
      }
    }

    NewPrivateFile->Dir = opendir (NewPrivateFile->FileName);
    if (NewPrivateFile->Dir == NULL) {
      if (errno == EACCES) {
        Status = EFI_ACCESS_DENIED;
      } else {
        Status = EFI_NOT_FOUND;
      }

      goto Done;
    }

  } else {
    //
    // deal with file
    //
    NewPrivateFile->fd = open (
                          NewPrivateFile->FileName,
                          ((OpenMode & EFI_FILE_MODE_CREATE) ? O_CREAT : 0) | (NewPrivateFile->IsOpenedByRead ? O_RDONLY : O_RDWR),
                          0666
                          );
    if (NewPrivateFile->fd < 0) {
      if (errno == ENOENT) {
        Status = EFI_NOT_FOUND;
      } else {
        Status = EFI_ACCESS_DENIED;
      }
    }
  }

  if ((OpenMode & EFI_FILE_MODE_CREATE) && Status == EFI_SUCCESS) {
    //
    // Set the attribute
    //
    InfoSize  = 0;
    Info      = NULL;
    Status    = PosixFileGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);
    if (Status != EFI_BUFFER_TOO_SMALL) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    Info = malloc (InfoSize);
    if (Info == NULL) {
      goto Done;
    }

    Status = PosixFileGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Info->Attribute = Attributes;
    PosixFileSetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, InfoSize, Info);

    free (Info);
  }

Done: ;
  if (TrailingDash) {
    FileName[StrLen (FileName) + 1]  = 0;
    FileName[StrLen (FileName)]      = L'\\';
  }

  if (EFI_ERROR (Status)) {
    if (NewPrivateFile) {
      if (NewPrivateFile->FileName) {
        free (NewPrivateFile->FileName);
      }

      free (NewPrivateFile);
    }
  } else {
    *NewHandle = &NewPrivateFile->EfiFile;
  }

  return Status;
}



/**
  Close the file handle

  @param  This          Protocol instance pointer.

  @retval EFI_SUCCESS   The device was opened.

**/
EFI_STATUS
PosixFileCLose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EMU_EFI_FILE_PRIVATE *PrivateFile;

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->fd >= 0) {
    close (PrivateFile->fd);
  }
  if (PrivateFile->Dir != NULL) {
    closedir (PrivateFile->Dir);
  }

  PrivateFile->fd = -1;
  PrivateFile->Dir = NULL;

  if (PrivateFile->FileName) {
    free (PrivateFile->FileName);
  }

  free (PrivateFile);

  return EFI_SUCCESS;
}


/**
  Close and delete the file handle.

  @param  This                     Protocol instance pointer.

  @retval EFI_SUCCESS              The device was opened.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed but the file was not deleted.

**/
EFI_STATUS
PosixFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  EMU_EFI_FILE_PRIVATE   *PrivateFile;

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  Status      = EFI_WARN_DELETE_FAILURE;

  if (PrivateFile->IsDirectoryPath) {
    if (PrivateFile->Dir != NULL) {
      closedir (PrivateFile->Dir);
      PrivateFile->Dir = NULL;
    }

    if (rmdir (PrivateFile->FileName) == 0) {
      Status = EFI_SUCCESS;
    }
  } else {
    close (PrivateFile->fd);
    PrivateFile->fd = -1;

    if (!PrivateFile->IsOpenedByRead) {
      if (!unlink (PrivateFile->FileName)) {
        Status = EFI_SUCCESS;
      }
    }
  }

  free (PrivateFile->FileName);
  free (PrivateFile);

  return Status;
}


/**
  Read data from the file.

  @param  This       Protocol instance pointer.
  @param  BufferSize On input size of buffer, on output amount of data in buffer.
  @param  Buffer     The buffer in which data is read.

  @retval EFI_SUCCESS          Data was read.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL  BufferSize is too small. BufferSize contains required size.

**/
EFI_STATUS
PosixFileRead (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  )
{
  EMU_EFI_FILE_PRIVATE    *PrivateFile;
  EFI_STATUS              Status;
  int                     Res;
  UINTN                   Size;
  UINTN                   NameSize;
  UINTN                   ResultSize;
  CHAR8                   *FullFileName;


  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (!PrivateFile->IsDirectoryPath) {
    if (PrivateFile->fd < 0) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    Res = read (PrivateFile->fd, Buffer, *BufferSize);
    if (Res < 0) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }
    *BufferSize = Res;
    Status = EFI_SUCCESS;
    goto Done;
  }

  //
  // Read on a directory.
  //
  if (PrivateFile->Dir == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (PrivateFile->Dirent == NULL) {
    PrivateFile->Dirent = readdir (PrivateFile->Dir);
    if (PrivateFile->Dirent == NULL) {
      *BufferSize = 0;
      Status = EFI_SUCCESS;
      goto Done;
    }
  }

  Size        = SIZE_OF_EFI_FILE_INFO;
  NameSize    = AsciiStrLen (PrivateFile->Dirent->d_name) + 1;
  ResultSize  = Size + 2 * NameSize;

  if (*BufferSize < ResultSize) {
    *BufferSize = ResultSize;
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }
  Status  = EFI_SUCCESS;

  *BufferSize = ResultSize;

  FullFileName = malloc (AsciiStrLen(PrivateFile->FileName) + 1 + NameSize);
  if (FullFileName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  AsciiStrCpy (FullFileName, PrivateFile->FileName);
  AsciiStrCat (FullFileName, "/");
  AsciiStrCat (FullFileName, PrivateFile->Dirent->d_name);
  Status = UnixSimpleFileSystemFileInfo (
            PrivateFile,
            FullFileName,
            BufferSize,
            Buffer
            );
  free (FullFileName);

  PrivateFile->Dirent = NULL;

Done:
  return Status;
}



/**
  Write data to a file.

  @param  This       Protocol instance pointer.
  @param  BufferSize On input size of buffer, on output amount of data in buffer.
  @param  Buffer     The buffer in which data to write.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORTED      Writes to Open directory are not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_DEVICE_ERROR     An attempt was made to write to a deleted file.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
PosixFileWrite (
  IN EFI_FILE_PROTOCOL        *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
{
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  int                   Res;


  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->fd < 0) {
    return EFI_DEVICE_ERROR;
  }

  if (PrivateFile->IsDirectoryPath) {
    return EFI_UNSUPPORTED;
  }

  if (PrivateFile->IsOpenedByRead) {
    return EFI_ACCESS_DENIED;
  }

  Res = write (PrivateFile->fd, Buffer, *BufferSize);
  if (Res == (UINTN)-1) {
    return ErrnoToEfiStatus ();
  }

  *BufferSize = Res;
  return EFI_SUCCESS;
}



/**
  Set a files current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open.

**/
EFI_STATUS
PosixFileSetPossition (
  IN EFI_FILE_PROTOCOL        *This,
  IN UINT64                   Position
  )
{
  EMU_EFI_FILE_PRIVATE    *PrivateFile;
  off_t                   Pos;

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->IsDirectoryPath) {
    if (Position != 0) {
      return EFI_UNSUPPORTED;
    }

    if (PrivateFile->Dir == NULL) {
      return EFI_DEVICE_ERROR;
    }
    rewinddir (PrivateFile->Dir);
    return EFI_SUCCESS;
  } else {
    if (Position == (UINT64) -1) {
      Pos = lseek (PrivateFile->fd, 0, SEEK_END);
    } else {
      Pos = lseek (PrivateFile->fd, Position, SEEK_SET);
    }
    if (Pos == (off_t)-1) {
      return ErrnoToEfiStatus ();
    }
    return EFI_SUCCESS;
  }
}



/**
  Get a file's current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open..

**/
EFI_STATUS
PosixFileGetPossition (
  IN EFI_FILE_PROTOCOL        *This,
  OUT UINT64                  *Position
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;

  PrivateFile   = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->IsDirectoryPath) {
    Status = EFI_UNSUPPORTED;
  } else {
    *Position = (UINT64)lseek (PrivateFile->fd, 0, SEEK_CUR);
    Status = (*Position == (UINT64) -1) ? ErrnoToEfiStatus () : EFI_SUCCESS;
  }

  return Status;
}


/**
  Get information about a file.

  @param  This            Protocol instance pointer.
  @param  InformationType Type of information to return in Buffer.
  @param  BufferSize      On input size of buffer, on output amount of data in buffer.
  @param  Buffer          The buffer to return data.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORTED      InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_BUFFER_TOO_SMALL Buffer was too small; required size returned in BufferSize.

**/
EFI_STATUS
PosixFileGetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN OUT UINTN                *BufferSize,
  OUT VOID                    *Buffer
  )
{
  EFI_STATUS                        Status;
  EMU_EFI_FILE_PRIVATE              *PrivateFile;
  EFI_FILE_SYSTEM_INFO              *FileSystemInfoBuffer;
  int                               UnixStatus;
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE    *PrivateRoot;
  struct statfs                     buf;

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);

  Status = EFI_SUCCESS;
  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = UnixSimpleFileSystemFileInfo (PrivateFile, NULL, BufferSize, Buffer);
  } else if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    if (*BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel)) {
      *BufferSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
      return EFI_BUFFER_TOO_SMALL;
    }

    UnixStatus = statfs (PrivateFile->FileName, &buf);
    if (UnixStatus < 0) {
      return EFI_DEVICE_ERROR;
    }

    FileSystemInfoBuffer            = (EFI_FILE_SYSTEM_INFO *) Buffer;
    FileSystemInfoBuffer->Size      = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
    FileSystemInfoBuffer->ReadOnly  = FALSE;

    //
    // Succeeded
    //
    FileSystemInfoBuffer->VolumeSize  = MultU64x32 (buf.f_blocks, buf.f_bsize);
    FileSystemInfoBuffer->FreeSpace   = MultU64x32 (buf.f_bavail, buf.f_bsize);
    FileSystemInfoBuffer->BlockSize   = buf.f_bsize;


    StrCpy ((CHAR16 *) FileSystemInfoBuffer->VolumeLabel, PrivateRoot->VolumeLabel);
    *BufferSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);

  } else if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    if (*BufferSize < StrSize (PrivateRoot->VolumeLabel)) {
      *BufferSize = StrSize (PrivateRoot->VolumeLabel);
      return EFI_BUFFER_TOO_SMALL;
    }

    StrCpy ((CHAR16 *) Buffer, PrivateRoot->VolumeLabel);
    *BufferSize = StrSize (PrivateRoot->VolumeLabel);

  }

  return Status;
}


/**
  Set information about a file

  @param  File            Protocol instance pointer.
  @param  InformationType Type of information in Buffer.
  @param  BufferSize      Size of buffer.
  @param  Buffer          The data to write.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORTED      InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.

**/
EFI_STATUS
PosixFileSetInfo (
  IN EFI_FILE_PROTOCOL        *This,
  IN EFI_GUID                 *InformationType,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE    *PrivateRoot;
  EMU_EFI_FILE_PRIVATE              *PrivateFile;
  EFI_FILE_INFO                     *OldFileInfo;
  EFI_FILE_INFO                     *NewFileInfo;
  EFI_STATUS                        Status;
  UINTN                             OldInfoSize;
  mode_t                            NewAttr;
  struct stat                       OldAttr;
  CHAR8                             *OldFileName;
  CHAR8                             *NewFileName;
  CHAR8                             *CharPointer;
  BOOLEAN                           AttrChangeFlag;
  BOOLEAN                           NameChangeFlag;
  BOOLEAN                           SizeChangeFlag;
  BOOLEAN                           TimeChangeFlag;
  struct tm                         NewLastAccessSystemTime;
  struct tm                         NewLastWriteSystemTime;
  EFI_FILE_SYSTEM_INFO              *NewFileSystemInfo;
  CHAR8                             *AsciiFilePtr;
  CHAR16                            *UnicodeFilePtr;
  int                               UnixStatus;
  struct utimbuf                    Utime;


  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);
  errno       = 0;
  Status      = EFI_UNSUPPORTED;
  OldFileInfo = NewFileInfo = NULL;
  OldFileName = NewFileName = NULL;
  AttrChangeFlag = NameChangeFlag = SizeChangeFlag = TimeChangeFlag = FALSE;

  //
  // Set file system information.
  //
  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    if (BufferSize < (SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel))) {
      Status = EFI_BAD_BUFFER_SIZE;
      goto Done;
    }

    NewFileSystemInfo = (EFI_FILE_SYSTEM_INFO *) Buffer;

    free (PrivateRoot->VolumeLabel);

    PrivateRoot->VolumeLabel = malloc (StrSize (NewFileSystemInfo->VolumeLabel));
    if (PrivateRoot->VolumeLabel == NULL) {
      goto Done;
    }

    StrCpy (PrivateRoot->VolumeLabel, NewFileSystemInfo->VolumeLabel);

    Status = EFI_SUCCESS;
    goto Done;
  }

  //
  // Set volume label information.
  //
  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    if (BufferSize < StrSize (PrivateRoot->VolumeLabel)) {
      Status = EFI_BAD_BUFFER_SIZE;
      goto Done;
    }

    StrCpy (PrivateRoot->VolumeLabel, (CHAR16 *) Buffer);

    Status = EFI_SUCCESS;
    goto Done;
  }

  if (!CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (BufferSize < SIZE_OF_EFI_FILE_INFO) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  //
  // Set file/directory information.
  //

  //
  // Check for invalid set file information parameters.
  //
  NewFileInfo = (EFI_FILE_INFO *) Buffer;
  if (NewFileInfo->Size <= sizeof (EFI_FILE_INFO) ||
      (NewFileInfo->Attribute &~(EFI_FILE_VALID_ATTR)) ||
      (sizeof (UINTN) == 4 && NewFileInfo->Size > 0xFFFFFFFF)
      ) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // Get current file information so we can determine what kind
  // of change request this is.
  //
  OldInfoSize = 0;
  Status = UnixSimpleFileSystemFileInfo (PrivateFile, NULL, &OldInfoSize, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  OldFileInfo = malloc (OldInfoSize);
  if (OldFileInfo == NULL) {
    goto Done;
  }

  Status = UnixSimpleFileSystemFileInfo (PrivateFile, NULL, &OldInfoSize, OldFileInfo);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  OldFileName = malloc (AsciiStrSize (PrivateFile->FileName));
  if (OldFileInfo == NULL) {
    goto Done;
  }

  AsciiStrCpy (OldFileName, PrivateFile->FileName);

  //
  // Make full pathname from new filename and rootpath.
  //
  if (NewFileInfo->FileName[0] == '\\') {
    NewFileName = malloc (AsciiStrLen (PrivateRoot->FilePath) + 1 + StrLen (NewFileInfo->FileName) + 1);
    if (NewFileName == NULL) {
      goto Done;
    }

    AsciiStrCpy (NewFileName, PrivateRoot->FilePath);
    AsciiFilePtr = NewFileName + AsciiStrLen(NewFileName);
    UnicodeFilePtr = NewFileInfo->FileName + 1;
    *AsciiFilePtr++ ='/';
  } else {
    NewFileName = malloc (AsciiStrLen (PrivateFile->FileName) + 2 + StrLen (NewFileInfo->FileName) + 1);
    if (NewFileName == NULL) {
      goto Done;
    }

    AsciiStrCpy (NewFileName, PrivateRoot->FilePath);
    AsciiFilePtr = NewFileName + AsciiStrLen(NewFileName);
    if ((AsciiFilePtr[-1] != '/') && (NewFileInfo->FileName[0] != '/')) {
      // make sure there is a / between Root FilePath and NewFileInfo Filename
      AsciiFilePtr[0] = '/';
      AsciiFilePtr[1] = '\0';
      AsciiFilePtr++;
    }
    UnicodeFilePtr = NewFileInfo->FileName;
  }
  // Convert to ascii.
  while (*UnicodeFilePtr) {
    *AsciiFilePtr++ = *UnicodeFilePtr++;
  }
  *AsciiFilePtr = 0;

  //
  // Is there an attribute change request?
  //
  if (NewFileInfo->Attribute != OldFileInfo->Attribute) {
    if ((NewFileInfo->Attribute & EFI_FILE_DIRECTORY) != (OldFileInfo->Attribute & EFI_FILE_DIRECTORY)) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    AttrChangeFlag = TRUE;
  }

  //
  // Is there a name change request?
  // bugbug: - Should really use EFI_UNICODE_COLLATION_PROTOCOL
  //
  if (StrCmp (NewFileInfo->FileName, OldFileInfo->FileName)) {
    NameChangeFlag = TRUE;
  }

  //
  // Is there a size change request?
  //
  if (NewFileInfo->FileSize != OldFileInfo->FileSize) {
    SizeChangeFlag = TRUE;
  }

  //
  // Is there a time stamp change request?
  //
  if (!IsZero (&NewFileInfo->CreateTime, sizeof (EFI_TIME)) &&
      CompareMem (&NewFileInfo->CreateTime, &OldFileInfo->CreateTime, sizeof (EFI_TIME))
      ) {
    TimeChangeFlag = TRUE;
  } else if (!IsZero (&NewFileInfo->LastAccessTime, sizeof (EFI_TIME)) &&
             CompareMem (&NewFileInfo->LastAccessTime, &OldFileInfo->LastAccessTime, sizeof (EFI_TIME))
             ) {
    TimeChangeFlag = TRUE;
  } else if (!IsZero (&NewFileInfo->ModificationTime, sizeof (EFI_TIME)) &&
             CompareMem (&NewFileInfo->ModificationTime, &OldFileInfo->ModificationTime, sizeof (EFI_TIME))
             ) {
    TimeChangeFlag = TRUE;
  }

  //
  // All done if there are no change requests being made.
  //
  if (!(AttrChangeFlag || NameChangeFlag || SizeChangeFlag || TimeChangeFlag)) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  //
  // Set file or directory information.
  //
  if (stat (OldFileName, &OldAttr) != 0) {
    Status = ErrnoToEfiStatus ();
    goto Done;
  }

  //
  // Name change.
  //
  if (NameChangeFlag) {
    //
    // Close the handles first
    //
    if (PrivateFile->IsOpenedByRead) {
      Status = EFI_ACCESS_DENIED;
      goto Done;
    }

    for (CharPointer = NewFileName; *CharPointer != 0 && *CharPointer != L'/'; CharPointer++) {
    }

    if (*CharPointer != 0) {
      Status = EFI_ACCESS_DENIED;
      goto Done;
    }

    UnixStatus = rename (OldFileName, NewFileName);
    if (UnixStatus == 0) {
      //
      // modify file name
      //
      free (PrivateFile->FileName);

      PrivateFile->FileName = malloc (AsciiStrSize (NewFileName));
      if (PrivateFile->FileName == NULL) {
        goto Done;
      }

      AsciiStrCpy (PrivateFile->FileName, NewFileName);
    } else {
      Status    = EFI_DEVICE_ERROR;
      goto Done;
    }
  }

  //
  //  Size change
  //
  if (SizeChangeFlag) {
    if (PrivateFile->IsDirectoryPath) {
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    if (PrivateFile->IsOpenedByRead || OldFileInfo->Attribute & EFI_FILE_READ_ONLY) {
      Status = EFI_ACCESS_DENIED;
      goto Done;
    }

    if (ftruncate (PrivateFile->fd, NewFileInfo->FileSize) != 0) {
      Status = ErrnoToEfiStatus ();
      goto Done;
    }

  }

  //
  // Time change
  //
  if (TimeChangeFlag) {
    NewLastAccessSystemTime.tm_year    = NewFileInfo->LastAccessTime.Year;
    NewLastAccessSystemTime.tm_mon     = NewFileInfo->LastAccessTime.Month;
    NewLastAccessSystemTime.tm_mday    = NewFileInfo->LastAccessTime.Day;
    NewLastAccessSystemTime.tm_hour    = NewFileInfo->LastAccessTime.Hour;
    NewLastAccessSystemTime.tm_min     = NewFileInfo->LastAccessTime.Minute;
    NewLastAccessSystemTime.tm_sec     = NewFileInfo->LastAccessTime.Second;
    NewLastAccessSystemTime.tm_isdst   = 0;

    Utime.actime = mktime (&NewLastAccessSystemTime);

    NewLastWriteSystemTime.tm_year    = NewFileInfo->ModificationTime.Year;
    NewLastWriteSystemTime.tm_mon     = NewFileInfo->ModificationTime.Month;
    NewLastWriteSystemTime.tm_mday    = NewFileInfo->ModificationTime.Day;
    NewLastWriteSystemTime.tm_hour    = NewFileInfo->ModificationTime.Hour;
    NewLastWriteSystemTime.tm_min     = NewFileInfo->ModificationTime.Minute;
    NewLastWriteSystemTime.tm_sec     = NewFileInfo->ModificationTime.Second;
    NewLastWriteSystemTime.tm_isdst   = 0;

    Utime.modtime = mktime (&NewLastWriteSystemTime);

    if (Utime.actime == (time_t)-1 || Utime.modtime == (time_t)-1) {
      goto Done;
    }

    if (utime (PrivateFile->FileName, &Utime) == -1) {
      Status = ErrnoToEfiStatus ();
      goto Done;
    }
  }

  //
  // No matter about AttrChangeFlag, Attribute must be set.
  // Because operation before may cause attribute change.
  //
  NewAttr = OldAttr.st_mode;

  if (NewFileInfo->Attribute & EFI_FILE_READ_ONLY) {
    NewAttr &= ~(S_IRUSR | S_IRGRP | S_IROTH);
  } else {
    NewAttr |= S_IRUSR;
  }

  if (chmod (NewFileName, NewAttr) != 0) {
    Status = ErrnoToEfiStatus ();
  }

Done:
  if (OldFileInfo != NULL) {
    free (OldFileInfo);
  }

  if (OldFileName != NULL) {
    free (OldFileName);
  }

  if (NewFileName != NULL) {
    free (NewFileName);
  }

  return Status;
}


/**
  Flush data back for the file handle.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORTED      Writes to Open directory are not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
PosixFileFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EMU_EFI_FILE_PRIVATE     *PrivateFile;


  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->IsDirectoryPath) {
    return EFI_UNSUPPORTED;
  }

  if (PrivateFile->IsOpenedByRead) {
    return EFI_ACCESS_DENIED;
  }

  if (PrivateFile->fd < 0) {
    return EFI_DEVICE_ERROR;
  }

  if (fsync (PrivateFile->fd) != 0) {
    return ErrnoToEfiStatus ();
  }

  return EFI_SUCCESS;
}



EFI_STATUS
PosixFileSystmeThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE  *Private;
  UINTN                           i;

  if (This->Private != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (!CompareGuid (This->Protocol, &gEfiSimpleFileSystemProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = malloc (sizeof (EMU_SIMPLE_FILE_SYSTEM_PRIVATE));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->FilePath = malloc (StrLen (This->ConfigString) + 1);
  if (Private->FilePath == NULL) {
    free (Private);
    return EFI_OUT_OF_RESOURCES;
  }

  // Convert Unicode to Ascii
  for (i = 0; This->ConfigString[i] != 0; i++) {
    Private->FilePath[i] = This->ConfigString[i];
  }
  Private->FilePath[i] = 0;


  Private->VolumeLabel = malloc (StrSize (L"EFI_EMULATED"));
  if (Private->VolumeLabel == NULL) {
    free (Private->FilePath);
    free (Private);
    return EFI_OUT_OF_RESOURCES;
  }
  StrCpy (Private->VolumeLabel, L"EFI_EMULATED");

  Private->Signature = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE;
  Private->Thunk     = This;
  CopyMem (&Private->SimpleFileSystem, &gPosixFileSystemProtocol, sizeof (Private->SimpleFileSystem));
  Private->FileHandlesOpen = FALSE;

  This->Interface = &Private->SimpleFileSystem;
  This->Private   = Private;
  return EFI_SUCCESS;
}


EFI_STATUS
PosixFileSystmeThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE  *Private;

  if (!CompareGuid (This->Protocol, &gEfiSimpleFileSystemProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = This->Private;

  if (Private->FileHandlesOpen) {
    //
    // Close only supported if all the EFI_FILE_HANDLEs have been closed.
    //
    return EFI_NOT_READY;
  }

  if (This->Private != NULL) {
    if (Private->VolumeLabel != NULL) {
      free (Private->VolumeLabel);
    }
    free (This->Private);
    This->Private = NULL;
  }

  return EFI_SUCCESS;
}


EMU_IO_THUNK_PROTOCOL gPosixFileSystemThunkIo = {
  &gEfiSimpleFileSystemProtocolGuid,
  NULL,
  NULL,
  0,
  GasketPosixFileSystmeThunkOpen,
  GasketPosixFileSystmeThunkClose,
  NULL
};



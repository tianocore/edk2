/** @file
  This driver uses the EFI_FIRMWARE_VOLUME2_PROTOCOL to expose files in firmware
  volumes via the the EFI_SIMPLE_FILESYSTEM_PROTOCOL and EFI_FILE_PROTOCOL.

  It will expose a single directory, containing one file for each file in the firmware
  volume. If a file has a UI section, its contents will be used as a filename.
  Otherwise, a string representation of the GUID will be used.
  Files of an executable type (That is PEIM, DRIVER, COMBINED_PEIM_DRIVER and APPLICATION)
  will have ".efi" added to their filename.

  Its primary intended use is to be able to start EFI applications embedded in FVs
  from the UEFI shell. It is entirely read-only.

Copyright (c) 2014, ARM Limited. All rights reserved.
Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FvSimpleFileSystemInternal.h"

//
// Template for EFI_FILE_SYSTEM_INFO data structure.
//
EFI_FILE_SYSTEM_INFO  mFsInfoTemplate = {
  0,    // Populate at runtime
  TRUE, // Read-only
  0,    // Don't know volume size
  0,    // No free space
  0,    // Don't know block size
  L""   // Populate at runtime
};

//
// Template for EFI_FILE_PROTOCOL data structure.
//
EFI_FILE_PROTOCOL  mFileSystemTemplate = {
  EFI_FILE_PROTOCOL_REVISION,
  FvSimpleFileSystemOpen,
  FvSimpleFileSystemClose,
  FvSimpleFileSystemDelete,
  FvSimpleFileSystemRead,
  FvSimpleFileSystemWrite,
  FvSimpleFileSystemGetPosition,
  FvSimpleFileSystemSetPosition,
  FvSimpleFileSystemGetInfo,
  FvSimpleFileSystemSetInfo,
  FvSimpleFileSystemFlush
};

/**
  Find and call ReadSection on the first section found of an executable type.

  @param  FvProtocol                  A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL instance.
  @param  FvFileInfo                  A pointer to the FV_FILESYSTEM_FILE_INFO instance that is a struct
                                      representing a file's info.
  @param  BufferSize                  Pointer to a caller-allocated UINTN. It indicates the size of
                                      the memory represented by *Buffer.
  @param  Buffer                      Pointer to a pointer to a data buffer to contain file content.

  @retval EFI_SUCCESS                 The call completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL   The buffer is too small to contain the requested output.
  @retval EFI_ACCESS_DENIED           The firmware volume is configured to disallow reads.
  @retval EFI_NOT_FOUND               The requested file was not found in the firmware volume.
  @retval EFI_DEVICE_ERROR            A hardware error occurred when attempting toaccess the firmware volume.

**/
EFI_STATUS
FvFsFindExecutableSection (
  IN     EFI_FIRMWARE_VOLUME2_PROTOCOL  *FvProtocol,
  IN     FV_FILESYSTEM_FILE_INFO        *FvFileInfo,
  IN OUT UINTN                          *BufferSize,
  IN OUT VOID                           **Buffer
  )
{
  EFI_SECTION_TYPE  SectionType;
  UINT32            AuthenticationStatus;
  EFI_STATUS        Status;

  for (SectionType = EFI_SECTION_PE32; SectionType <= EFI_SECTION_TE; SectionType++) {
    Status = FvProtocol->ReadSection (
                           FvProtocol,
                           &FvFileInfo->NameGuid,
                           SectionType,
                           0,
                           Buffer,
                           BufferSize,
                           &AuthenticationStatus
                           );
    if (Status != EFI_NOT_FOUND) {
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get the size of the buffer that will be returned by FvFsReadFile.

  @param  FvProtocol                  A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL instance.
  @param  FvFileInfo                  A pointer to the FV_FILESYSTEM_FILE_INFO instance that is a struct
                                      representing a file's info.

  @retval EFI_SUCCESS                 The file size was gotten correctly.
  @retval Others                      The file size wasn't gotten correctly.

**/
EFI_STATUS
FvFsGetFileSize (
  IN     EFI_FIRMWARE_VOLUME2_PROTOCOL  *FvProtocol,
  IN OUT FV_FILESYSTEM_FILE_INFO        *FvFileInfo
  )
{
  UINT32                  AuthenticationStatus;
  EFI_FV_FILETYPE         FoundType;
  EFI_FV_FILE_ATTRIBUTES  Attributes;
  EFI_STATUS              Status;
  UINT8                   IgnoredByte;
  VOID                    *IgnoredPtr;

  //
  // To get the size of a section, we pass 0 for BufferSize. But we can't pass
  // NULL for Buffer, as that will cause a return of INVALID_PARAMETER, and we
  // can't pass NULL for *Buffer, as that will cause the callee to allocate
  // a buffer of the sections size.
  //
  IgnoredPtr                    = &IgnoredByte;
  FvFileInfo->FileInfo.FileSize = 0;

  if (FV_FILETYPE_IS_EXECUTABLE (FvFileInfo->Type)) {
    //
    // Get the size of the first executable section out of the file.
    //
    Status = FvFsFindExecutableSection (FvProtocol, FvFileInfo, (UINTN *)&FvFileInfo->FileInfo.FileSize, &IgnoredPtr);
    if (Status == EFI_WARN_BUFFER_TOO_SMALL) {
      return EFI_SUCCESS;
    }
  } else if (FvFileInfo->Type == EFI_FV_FILETYPE_FREEFORM) {
    //
    // Try to get the size of a raw section out of the file
    //
    Status = FvProtocol->ReadSection (
                           FvProtocol,
                           &FvFileInfo->NameGuid,
                           EFI_SECTION_RAW,
                           0,
                           &IgnoredPtr,
                           (UINTN *)&FvFileInfo->FileInfo.FileSize,
                           &AuthenticationStatus
                           );
    if (Status == EFI_WARN_BUFFER_TOO_SMALL) {
      return EFI_SUCCESS;
    }

    if (EFI_ERROR (Status)) {
      //
      // Didn't find a raw section, just return the whole file's size.
      //
      return FvProtocol->ReadFile (
                           FvProtocol,
                           &FvFileInfo->NameGuid,
                           NULL,
                           (UINTN *)&FvFileInfo->FileInfo.FileSize,
                           &FoundType,
                           &Attributes,
                           &AuthenticationStatus
                           );
    }
  } else {
    //
    // Get the size of the entire file
    //
    return FvProtocol->ReadFile (
                         FvProtocol,
                         &FvFileInfo->NameGuid,
                         NULL,
                         (UINTN *)&FvFileInfo->FileInfo.FileSize,
                         &FoundType,
                         &Attributes,
                         &AuthenticationStatus
                         );
  }

  return Status;
}

/**
  Helper function to read a file.

  The data returned depends on the type of the underlying FV file:
  - For executable types, the first section found that contains executable code is returned.
  - For files of type FREEFORM, the driver attempts to return the first section of type RAW.
    If none is found, the entire contents of the FV file are returned.
  - On all other files the entire contents of the FV file is returned, as by
    EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadFile.

  @param  FvProtocol                  A pointer to the EFI_FIRMWARE_VOLUME2_PROTOCOL instance.
  @param  FvFileInfo                  A pointer to the FV_FILESYSTEM_FILE_INFO instance that is a struct
                                      representing a file's info.
  @param  BufferSize                  Pointer to a caller-allocated UINTN. It indicates the size of
                                      the memory represented by *Buffer.
  @param  Buffer                      Pointer to a pointer to a data buffer to contain file content.

  @retval EFI_SUCCESS                 The call completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL   The buffer is too small to contain the requested output.
  @retval EFI_ACCESS_DENIED           The firmware volume is configured to disallow reads.
  @retval EFI_NOT_FOUND               The requested file was not found in the firmware volume.
  @retval EFI_DEVICE_ERROR            A hardware error occurred when attempting toaccess the firmware volume.

**/
EFI_STATUS
FvFsReadFile (
  IN     EFI_FIRMWARE_VOLUME2_PROTOCOL  *FvProtocol,
  IN     FV_FILESYSTEM_FILE_INFO        *FvFileInfo,
  IN OUT UINTN                          *BufferSize,
  IN OUT VOID                           **Buffer
  )
{
  UINT32                  AuthenticationStatus;
  EFI_FV_FILETYPE         FoundType;
  EFI_FV_FILE_ATTRIBUTES  Attributes;
  EFI_STATUS              Status;

  if (FV_FILETYPE_IS_EXECUTABLE (FvFileInfo->Type)) {
    //
    // Read the first executable section out of the file.
    //
    Status = FvFsFindExecutableSection (FvProtocol, FvFileInfo, BufferSize, Buffer);
  } else if (FvFileInfo->Type == EFI_FV_FILETYPE_FREEFORM) {
    //
    // Try to read a raw section out of the file
    //
    Status = FvProtocol->ReadSection (
                           FvProtocol,
                           &FvFileInfo->NameGuid,
                           EFI_SECTION_RAW,
                           0,
                           Buffer,
                           BufferSize,
                           &AuthenticationStatus
                           );
    if (EFI_ERROR (Status)) {
      //
      // Didn't find a raw section, just return the whole file.
      //
      Status = FvProtocol->ReadFile (
                             FvProtocol,
                             &FvFileInfo->NameGuid,
                             Buffer,
                             BufferSize,
                             &FoundType,
                             &Attributes,
                             &AuthenticationStatus
                             );
    }
  } else {
    //
    // Read the entire file
    //
    Status = FvProtocol->ReadFile (
                           FvProtocol,
                           &FvFileInfo->NameGuid,
                           Buffer,
                           BufferSize,
                           &FoundType,
                           &Attributes,
                           &AuthenticationStatus
                           );
  }

  return Status;
}

/**
  Helper function for populating an EFI_FILE_INFO for a file.

  Note the CreateTime, LastAccessTime and ModificationTime fields in EFI_FILE_INFO
  are full zero as FV2 protocol has no corresponding info to fill.

  @param  FvFileInfo                  A pointer to the FV_FILESYSTEM_FILE_INFO instance that is a struct
                                      representing a file's info.
  @param  BufferSize                  Pointer to a caller-allocated UINTN. It indicates the size of
                                      the memory represented by FileInfo.
  @param  FileInfo                    A pointer to EFI_FILE_INFO to contain the returned file info.

  @retval EFI_SUCCESS                 The call completed successfully.
  @retval EFI_BUFFER_TOO_SMALL        The buffer is too small to contain the requested output.

**/
EFI_STATUS
FvFsGetFileInfo (
  IN     FV_FILESYSTEM_FILE_INFO  *FvFileInfo,
  IN OUT UINTN                    *BufferSize,
  OUT EFI_FILE_INFO               *FileInfo
  )
{
  UINTN  InfoSize;

  InfoSize = (UINTN)FvFileInfo->FileInfo.Size;
  if (*BufferSize < InfoSize) {
    *BufferSize = InfoSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Initialize FileInfo
  //
  CopyMem (FileInfo, &FvFileInfo->FileInfo, InfoSize);

  *BufferSize = InfoSize;
  return EFI_SUCCESS;
}

/**
  Removes the last directory or file entry in a path by changing the last
  L'\' to a CHAR_NULL.

  @param  Path      The pointer to the path to modify.

  @retval FALSE     Nothing was found to remove.
  @retval TRUE      A directory or file was removed.

**/
BOOLEAN
EFIAPI
RemoveLastItemFromPath (
  IN OUT CHAR16  *Path
  )
{
  CHAR16  *Walker;
  CHAR16  *LastSlash;

  //
  // get directory name from path... ('chop' off extra)
  //
  for ( Walker = Path, LastSlash = NULL
        ; Walker != NULL && *Walker != CHAR_NULL
        ; Walker++
        )
  {
    if ((*Walker == L'\\') && (*(Walker + 1) != CHAR_NULL)) {
      LastSlash = Walker + 1;
    }
  }

  if (LastSlash != NULL) {
    *LastSlash = CHAR_NULL;
    return (TRUE);
  }

  return (FALSE);
}

/**
  Function to clean up paths.

  - Single periods in the path are removed.
  - Double periods in the path are removed along with a single parent directory.
  - Forward slashes L'/' are converted to backward slashes L'\'.

  This will be done inline and the existing buffer may be larger than required
  upon completion.

  @param  Path          The pointer to the string containing the path.

  @retval NULL          An error occurred.
  @return Path in all other instances.

**/
CHAR16 *
EFIAPI
TrimFilePathToAbsolutePath (
  IN CHAR16  *Path
  )
{
  CHAR16  *TempString;
  UINTN   TempSize;

  if (Path == NULL) {
    return NULL;
  }

  //
  // Fix up the '/' vs '\'
  //
  for (TempString = Path; (TempString != NULL) && (*TempString != CHAR_NULL); TempString++) {
    if (*TempString == L'/') {
      *TempString = L'\\';
    }
  }

  //
  // Fix up the ..
  //
  while ((TempString = StrStr (Path, L"\\..\\")) != NULL) {
    *TempString = CHAR_NULL;
    TempString += 4;
    RemoveLastItemFromPath (Path);
    TempSize = StrSize (TempString);
    CopyMem (Path + StrLen (Path), TempString, TempSize);
  }

  if (((TempString = StrStr (Path, L"\\..")) != NULL) && (*(TempString + 3) == CHAR_NULL)) {
    *TempString = CHAR_NULL;
    RemoveLastItemFromPath (Path);
  }

  //
  // Fix up the .
  //
  while ((TempString = StrStr (Path, L"\\.\\")) != NULL) {
    *TempString = CHAR_NULL;
    TempString += 2;
    TempSize    = StrSize (TempString);
    CopyMem (Path + StrLen (Path), TempString, TempSize);
  }

  if (((TempString = StrStr (Path, L"\\.")) != NULL) && (*(TempString + 2) == CHAR_NULL)) {
    *(TempString + 1) = CHAR_NULL;
  }

  while ((TempString = StrStr (Path, L"\\\\")) != NULL) {
    *TempString = CHAR_NULL;
    TempString += 1;
    TempSize    = StrSize (TempString);
    CopyMem (Path + StrLen (Path), TempString, TempSize);
  }

  if (((TempString = StrStr (Path, L"\\\\")) != NULL) && (*(TempString + 1) == CHAR_NULL)) {
    *(TempString) = CHAR_NULL;
  }

  return Path;
}

/**
  Opens a new file relative to the source file's location.

  @param  This       A pointer to the EFI_FILE_PROTOCOL instance that is the file
                     handle to the source location. This would typically be an open
                     handle to a directory.
  @param  NewHandle  A pointer to the location to return the opened handle for the new
                     file.
  @param  FileName   The Null-terminated string of the name of the file to be opened.
                     The file name may contain the following path modifiers: "\", ".",
                     and "..".
  @param  OpenMode   The mode to open the file. The only valid combinations that the
                     file may be opened with are: Read, Read/Write, or Create/Read/Write.
  @param  Attributes Only valid for EFI_FILE_MODE_CREATE, in which case these are the
                     attribute bits for the newly created file.

  @retval EFI_SUCCESS          The file was opened.
  @retval EFI_NOT_FOUND        The specified file could not be found on the device.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_MEDIA_CHANGED    The device has a different medium in it or the medium is no
                               longer supported.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  An attempt was made to create a file, or open a file for write
                               when the media is write-protected.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES Not enough resources were available to open the file.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemOpen (
  IN     EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL     **NewHandle,
  IN     CHAR16             *FileName,
  IN     UINT64             OpenMode,
  IN     UINT64             Attributes
  )
{
  FV_FILESYSTEM_INSTANCE   *Instance;
  FV_FILESYSTEM_FILE       *File;
  FV_FILESYSTEM_FILE       *NewFile;
  FV_FILESYSTEM_FILE_INFO  *FvFileInfo;
  LIST_ENTRY               *FvFileInfoLink;
  EFI_STATUS               Status;
  UINTN                    FileNameLength;
  UINTN                    NewFileNameLength;
  CHAR16                   *FileNameWithExtension;

  //
  // Check for a valid mode
  //
  switch (OpenMode) {
    case EFI_FILE_MODE_READ:
      break;

    default:
      return EFI_WRITE_PROTECTED;
  }

  File     = FVFS_FILE_FROM_FILE_THIS (This);
  Instance = File->Instance;

  FileName = TrimFilePathToAbsolutePath (FileName);
  if (FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (FileName[0] == L'\\') {
    FileName++;
  }

  //
  // Check for opening root
  //
  if ((StrCmp (FileName, L".") == 0) || (StrCmp (FileName, L"") == 0)) {
    NewFile = AllocateZeroPool (sizeof (FV_FILESYSTEM_FILE));
    if (NewFile == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewFile->Signature  = FVFS_FILE_SIGNATURE;
    NewFile->Instance   = Instance;
    NewFile->FvFileInfo = File->FvFileInfo;
    CopyMem (&NewFile->FileProtocol, &mFileSystemTemplate, sizeof (mFileSystemTemplate));
    InitializeListHead (&NewFile->Link);
    InsertHeadList (&Instance->FileHead, &NewFile->Link);

    NewFile->DirReadNext = NULL;
    if (!IsListEmpty (&Instance->FileInfoHead)) {
      NewFile->DirReadNext = FVFS_GET_FIRST_FILE_INFO (Instance);
    }

    *NewHandle = &NewFile->FileProtocol;
    return EFI_SUCCESS;
  }

  //
  // Do a linear search for a file in the FV with a matching filename
  //
  Status     = EFI_NOT_FOUND;
  FvFileInfo = NULL;
  for (FvFileInfoLink = GetFirstNode (&Instance->FileInfoHead);
       !IsNull (&Instance->FileInfoHead, FvFileInfoLink);
       FvFileInfoLink = GetNextNode (&Instance->FileInfoHead, FvFileInfoLink))
  {
    FvFileInfo = FVFS_FILE_INFO_FROM_LINK (FvFileInfoLink);
    if (mUnicodeCollation->StriColl (mUnicodeCollation, &FvFileInfo->FileInfo.FileName[0], FileName) == 0) {
      Status = EFI_SUCCESS;
      break;
    }
  }

  // If the file has not been found check if the filename exists with an extension
  // in case there was no extension present.
  // FvFileSystem adds a 'virtual' extension '.EFI' to EFI applications and drivers
  // present in the Firmware Volume
  if (Status == EFI_NOT_FOUND) {
    FileNameLength = StrLen (FileName);

    // Does the filename already contain the '.EFI' extension?
    if (mUnicodeCollation->StriColl (mUnicodeCollation, FileName + FileNameLength - 4, L".efi") != 0) {
      // No, there was no extension. So add one and search again for the file
      // NewFileNameLength = FileNameLength + 1 + 4 = (Number of non-null character) + (file extension) + (a null character)
      NewFileNameLength     = FileNameLength + 1 + 4;
      FileNameWithExtension = AllocatePool (NewFileNameLength * 2);
      StrCpyS (FileNameWithExtension, NewFileNameLength, FileName);
      StrCatS (FileNameWithExtension, NewFileNameLength, L".EFI");

      for (FvFileInfoLink = GetFirstNode (&Instance->FileInfoHead);
           !IsNull (&Instance->FileInfoHead, FvFileInfoLink);
           FvFileInfoLink = GetNextNode (&Instance->FileInfoHead, FvFileInfoLink))
      {
        FvFileInfo = FVFS_FILE_INFO_FROM_LINK (FvFileInfoLink);
        if (mUnicodeCollation->StriColl (mUnicodeCollation, &FvFileInfo->FileInfo.FileName[0], FileNameWithExtension) == 0) {
          Status = EFI_SUCCESS;
          break;
        }
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    NewFile = AllocateZeroPool (sizeof (FV_FILESYSTEM_FILE));
    if (NewFile == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewFile->Signature  = FVFS_FILE_SIGNATURE;
    NewFile->Instance   = Instance;
    NewFile->FvFileInfo = FvFileInfo;
    CopyMem (&NewFile->FileProtocol, &mFileSystemTemplate, sizeof (mFileSystemTemplate));
    InitializeListHead (&NewFile->Link);
    InsertHeadList (&Instance->FileHead, &NewFile->Link);

    *NewHandle = &NewFile->FileProtocol;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Closes a specified file handle.

  @param  This          A pointer to the EFI_FILE_PROTOCOL instance that is the file
                        handle to close.

  @retval EFI_SUCCESS   The file was closed.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  FV_FILESYSTEM_INSTANCE  *Instance;
  FV_FILESYSTEM_FILE      *File;

  File     = FVFS_FILE_FROM_FILE_THIS (This);
  Instance = File->Instance;

  if (File != Instance->Root) {
    RemoveEntryList (&File->Link);
    FreePool (File);
  }

  return EFI_SUCCESS;
}

/**
  Reads data from a file.

  @param  This       A pointer to the EFI_FILE_PROTOCOL instance that is the file
                     handle to read data from.
  @param  BufferSize On input, the size of the Buffer. On output, the amount of data
                     returned in Buffer. In both cases, the size is measured in bytes.
  @param  Buffer     The buffer into which the data is read.

  @retval EFI_SUCCESS          Data was read.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_DEVICE_ERROR     An attempt was made to read from a deleted file.
  @retval EFI_DEVICE_ERROR     On entry, the current file position is beyond the end of the file.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL The BufferSize is too small to read the current directory
                               entry. BufferSize has been updated with the size
                               needed to complete the request.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemRead (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  )
{
  FV_FILESYSTEM_INSTANCE  *Instance;
  FV_FILESYSTEM_FILE      *File;
  EFI_STATUS              Status;
  LIST_ENTRY              *FvFileInfoLink;
  VOID                    *FileBuffer;
  UINTN                   FileSize;

  File     = FVFS_FILE_FROM_FILE_THIS (This);
  Instance = File->Instance;

  if (File->FvFileInfo == Instance->Root->FvFileInfo) {
    if (File->DirReadNext) {
      //
      // Directory read: populate Buffer with an EFI_FILE_INFO
      //
      Status = FvFsGetFileInfo (File->DirReadNext, BufferSize, Buffer);
      if (!EFI_ERROR (Status)) {
        //
        // Successfully read a directory entry, now update the pointer to the
        // next file, which will be read on the next call to this function
        //
        FvFileInfoLink = GetNextNode (&Instance->FileInfoHead, &File->DirReadNext->Link);
        if (IsNull (&Instance->FileInfoHead, FvFileInfoLink)) {
          //
          // No more files left
          //
          File->DirReadNext = NULL;
        } else {
          File->DirReadNext = FVFS_FILE_INFO_FROM_LINK (FvFileInfoLink);
        }
      }

      return Status;
    } else {
      //
      // Directory read. All entries have been read, so return a zero-size
      // buffer.
      //
      *BufferSize = 0;
      return EFI_SUCCESS;
    }
  } else {
    FileSize = (UINTN)File->FvFileInfo->FileInfo.FileSize;

    FileBuffer = AllocateZeroPool (FileSize);
    if (FileBuffer == NULL) {
      return EFI_DEVICE_ERROR;
    }

    Status = FvFsReadFile (File->Instance->FvProtocol, File->FvFileInfo, &FileSize, &FileBuffer);
    if (EFI_ERROR (Status)) {
      FreePool (FileBuffer);
      return EFI_DEVICE_ERROR;
    }

    if (*BufferSize + File->Position > FileSize) {
      *BufferSize = (UINTN)(FileSize - File->Position);
    }

    CopyMem (Buffer, (UINT8 *)FileBuffer + File->Position, *BufferSize);
    File->Position += *BufferSize;

    FreePool (FileBuffer);

    return EFI_SUCCESS;
  }
}

/**
  Writes data to a file.

  @param  This       A pointer to the EFI_FILE_PROTOCOL instance that is the file
                     handle to write data to.
  @param  BufferSize On input, the size of the Buffer. On output, the amount of data
                     actually written. In both cases, the size is measured in bytes.
  @param  Buffer     The buffer of data to write.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORTED      Writes to open directory files are not supported.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_DEVICE_ERROR     An attempt was made to write to a deleted file.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED    The file was opened read only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemWrite (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  )
{
  FV_FILESYSTEM_INSTANCE  *Instance;
  FV_FILESYSTEM_FILE      *File;

  File     = FVFS_FILE_FROM_FILE_THIS (This);
  Instance = File->Instance;

  if (File->FvFileInfo == Instance->Root->FvFileInfo) {
    return EFI_UNSUPPORTED;
  } else {
    return EFI_WRITE_PROTECTED;
  }
}

/**
  Returns a file's current position.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the file
                          handle to get the current position on.
  @param  Position        The address to return the file's current position value.

  @retval EFI_SUCCESS      The position was returned.
  @retval EFI_UNSUPPORTED  The request is not valid on open directories.
  @retval EFI_DEVICE_ERROR An attempt was made to get the position from a deleted file.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemGetPosition (
  IN     EFI_FILE_PROTOCOL  *This,
  OUT UINT64                *Position
  )
{
  FV_FILESYSTEM_INSTANCE  *Instance;
  FV_FILESYSTEM_FILE      *File;

  File     = FVFS_FILE_FROM_FILE_THIS (This);
  Instance = File->Instance;

  if (File->FvFileInfo == Instance->Root->FvFileInfo) {
    return EFI_UNSUPPORTED;
  } else {
    *Position = File->Position;
    return EFI_SUCCESS;
  }
}

/**
  Sets a file's current position.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the
                          file handle to set the requested position on.
  @param  Position        The byte position from the start of the file to set.

  @retval EFI_SUCCESS      The position was set.
  @retval EFI_UNSUPPORTED  The seek request for nonzero is not valid on open
                           directories.
  @retval EFI_DEVICE_ERROR An attempt was made to set the position of a deleted file.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  FV_FILESYSTEM_INSTANCE  *Instance;
  FV_FILESYSTEM_FILE      *File;

  File     = FVFS_FILE_FROM_FILE_THIS (This);
  Instance = File->Instance;

  if (File->FvFileInfo == Instance->Root->FvFileInfo) {
    if (Position != 0) {
      return EFI_UNSUPPORTED;
    }

    //
    // Reset directory position to first entry
    //
    if (File->DirReadNext) {
      File->DirReadNext = FVFS_GET_FIRST_FILE_INFO (Instance);
    }
  } else if (Position == 0xFFFFFFFFFFFFFFFFull) {
    File->Position = File->FvFileInfo->FileInfo.FileSize;
  } else {
    File->Position = Position;
  }

  return EFI_SUCCESS;
}

/**
  Flushes all modified data associated with a file to a device.

  @param  This A pointer to the EFI_FILE_PROTOCOL instance that is the file
               handle to flush.

  @retval EFI_SUCCESS          The data was flushed.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The file or medium is write-protected.
  @retval EFI_ACCESS_DENIED    The file was opened read-only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  return EFI_WRITE_PROTECTED;
}

/**
  Close and delete the file handle.

  @param  This                     A pointer to the EFI_FILE_PROTOCOL instance that is the
                                   handle to the file to delete.

  @retval EFI_SUCCESS              The file was closed and deleted, and the handle was closed.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed, but the file was not deleted.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  Status = FvSimpleFileSystemClose (This);
  ASSERT_EFI_ERROR (Status);

  return EFI_WARN_DELETE_FAILURE;
}

/**
  Returns information about a file.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the file
                          handle the requested information is for.
  @param  InformationType The type identifier for the information being requested.
  @param  BufferSize      On input, the size of Buffer. On output, the amount of data
                          returned in Buffer. In both cases, the size is measured in bytes.
  @param  Buffer          A pointer to the data buffer to return. The buffer's type is
                          indicated by InformationType.

  @retval EFI_SUCCESS          The information was returned.
  @retval EFI_UNSUPPORTED      The InformationType is not known.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL The BufferSize is too small to read the current directory entry.
                               BufferSize has been updated with the size needed to complete
                               the request.
**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemGetInfo (
  IN     EFI_FILE_PROTOCOL  *This,
  IN     EFI_GUID           *InformationType,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  )
{
  FV_FILESYSTEM_FILE            *File;
  EFI_FILE_SYSTEM_INFO          *FsInfoOut;
  EFI_FILE_SYSTEM_VOLUME_LABEL  *FsVolumeLabel;
  FV_FILESYSTEM_INSTANCE        *Instance;
  UINTN                         Size;
  EFI_STATUS                    Status;

  File = FVFS_FILE_FROM_FILE_THIS (This);

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    //
    // Return filesystem info
    //
    Instance = File->Instance;

    Size = sizeof (EFI_FILE_SYSTEM_INFO) + StrSize (Instance->VolumeLabel) - sizeof (CHAR16);

    if (*BufferSize < Size) {
      *BufferSize = Size;
      return EFI_BUFFER_TOO_SMALL;
    }

    //
    // Cast output buffer for convenience
    //
    FsInfoOut = (EFI_FILE_SYSTEM_INFO *)Buffer;

    CopyMem (FsInfoOut, &mFsInfoTemplate, sizeof (EFI_FILE_SYSTEM_INFO));
    Status = StrnCpyS (
               FsInfoOut->VolumeLabel,
               (*BufferSize - OFFSET_OF (EFI_FILE_SYSTEM_INFO, VolumeLabel)) / sizeof (CHAR16),
               Instance->VolumeLabel,
               StrLen (Instance->VolumeLabel)
               );
    ASSERT_EFI_ERROR (Status);
    FsInfoOut->Size = Size;
    return Status;
  } else if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    //
    // Return file info
    //
    return FvFsGetFileInfo (File->FvFileInfo, BufferSize, (EFI_FILE_INFO *)Buffer);
  } else if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    //
    // Return Volume Label
    //
    Instance = File->Instance;
    Size     = sizeof (EFI_FILE_SYSTEM_VOLUME_LABEL) + StrSize (Instance->VolumeLabel) - sizeof (CHAR16);
    if (*BufferSize < Size) {
      *BufferSize = Size;
      return EFI_BUFFER_TOO_SMALL;
    }

    FsVolumeLabel = (EFI_FILE_SYSTEM_VOLUME_LABEL *)Buffer;
    Status        = StrnCpyS (
                      FsVolumeLabel->VolumeLabel,
                      (*BufferSize - OFFSET_OF (EFI_FILE_SYSTEM_VOLUME_LABEL, VolumeLabel)) / sizeof (CHAR16),
                      Instance->VolumeLabel,
                      StrLen (Instance->VolumeLabel)
                      );
    ASSERT_EFI_ERROR (Status);
    return Status;
  } else {
    return EFI_UNSUPPORTED;
  }
}

/**
  Sets information about a file.

  @param  This            A pointer to the EFI_FILE_PROTOCOL instance that is the file
                          handle the information is for.
  @param  InformationType The type identifier for the information being set.
  @param  BufferSize      The size, in bytes, of Buffer.
  @param  Buffer          A pointer to the data buffer to write. The buffer's type is
                          indicated by InformationType.

  @retval EFI_SUCCESS          The information was set.
  @retval EFI_UNSUPPORTED      The InformationType is not known.
  @retval EFI_NO_MEDIA         The device has no medium.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  InformationType is EFI_FILE_INFO_ID and the media is
                               read-only.
  @retval EFI_WRITE_PROTECTED  InformationType is EFI_FILE_PROTOCOL_SYSTEM_INFO_ID
                               and the media is read only.
  @retval EFI_WRITE_PROTECTED  InformationType is EFI_FILE_SYSTEM_VOLUME_LABEL_ID
                               and the media is read-only.
  @retval EFI_ACCESS_DENIED    An attempt is made to change the name of a file to a
                               file that is already present.
  @retval EFI_ACCESS_DENIED    An attempt is being made to change the EFI_FILE_DIRECTORY
                               Attribute.
  @retval EFI_ACCESS_DENIED    An attempt is being made to change the size of a directory.
  @retval EFI_ACCESS_DENIED    InformationType is EFI_FILE_INFO_ID and the file was opened
                               read-only and an attempt is being made to modify a field
                               other than Attribute.
  @retval EFI_VOLUME_FULL      The volume is full.
  @retval EFI_BAD_BUFFER_SIZE  BufferSize is smaller than the size of the type indicated
                               by InformationType.

**/
EFI_STATUS
EFIAPI
FvSimpleFileSystemSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid) ||
      CompareGuid (InformationType, &gEfiFileInfoGuid) ||
      CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid))
  {
    return EFI_WRITE_PROTECTED;
  }

  return EFI_UNSUPPORTED;
}

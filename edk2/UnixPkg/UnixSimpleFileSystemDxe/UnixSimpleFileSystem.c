/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixSimpleFileSystem.c

Abstract:

  Produce Simple File System abstractions for directories on your PC using Posix APIs.
  The configuration of what devices to mount or emulate comes from UNIX 
  environment variables. The variables must be visible to the Microsoft* 
  Developer Studio for them to work.

  * Other names and brands may be claimed as the property of others.

--*/

#include "UnixSimpleFileSystem.h"

EFI_DRIVER_BINDING_PROTOCOL gUnixSimpleFileSystemDriverBinding = {
  UnixSimpleFileSystemDriverBindingSupported,
  UnixSimpleFileSystemDriverBindingStart,
  UnixSimpleFileSystemDriverBindingStop,
  0xa,
  NULL,
  NULL
};


CHAR16 *
EfiStrChr (
  IN CHAR16   *Str,
  IN CHAR16   Chr
  )
/*++

Routine Description:

  Locate the first occurance of a character in a string.

Arguments:

  Str - Pointer to NULL terminated unicode string.
  Chr - Character to locate.

Returns:

  If Str is NULL, then NULL is returned.
  If Chr is not contained in Str, then NULL is returned.
  If Chr is contained in Str, then a pointer to the first occurance of Chr in Str is returned.

--*/
{
  if (Str == NULL) {
    return Str;
  }

  while (*Str != '\0' && *Str != Chr) {
    ++Str;
  }

  return (*Str == Chr) ? Str : NULL;
}

BOOLEAN
IsZero (
  IN VOID   *Buffer,
  IN UINTN  Length
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Buffer  - TODO: add argument description
  Length  - TODO: add argument description

Returns:

  TODO: add return values

--*/
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

VOID
CutPrefix (
  IN  CHAR8  *Str,
  IN  UINTN   Count
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Str   - TODO: add argument description
  Count - TODO: add argument description

Returns:

  TODO: add return values

--*/
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



EFI_STATUS
EFIAPI
UnixSimpleFileSystemDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  Check to see if the driver supports a given controller.

Arguments:

  This                - A pointer to an instance of the EFI_DRIVER_BINDING_PROTOCOL.

  ControllerHandle    - EFI handle of the controller to test.

  RemainingDevicePath - Pointer to remaining portion of a device path.

Returns:

  EFI_SUCCESS         - The device specified by ControllerHandle and RemainingDevicePath is supported by the driver
                        specified by This.

  EFI_ALREADY_STARTED - The device specified by ControllerHandle and RemainingDevicePath is already being managed by
                        the driver specified by This.

  EFI_ACCESS_DENIED   - The device specified by ControllerHandle and RemainingDevicePath is already being managed by
                        a different driver or an application that requires exclusive access.

  EFI_UNSUPPORTED     - The device specified by ControllerHandle and RemainingDevicePath is not supported by the
                        driver specified by This.

--*/
{
  EFI_STATUS              Status;
  EFI_UNIX_IO_PROTOCOL  *UnixIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID **)&UnixIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure GUID is for a File System handle.
  //
  Status = EFI_UNSUPPORTED;
  if (CompareGuid (UnixIo->TypeGuid, &gEfiUnixFileSystemGuid)) {
    Status = EFI_SUCCESS;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiUnixIoProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
/*++

Routine Description:

  Starts a device controller or a bus controller.

Arguments:

  This                - A pointer to an instance of the EFI_DRIVER_BINDING_PROTOCOL.

  ControllerHandle    - EFI handle of the controller to start.

  RemainingDevicePath - Pointer to remaining portion of a device path.

Returns:

  EFI_SUCCESS           - The device or bus controller has been started.

  EFI_DEVICE_ERROR      - The device could not be started due to a device failure.

  EFI_OUT_OF_RESOURCES  - The request could not be completed due to lack of resources.

--*/
{
  EFI_STATUS                        Status;
  EFI_UNIX_IO_PROTOCOL            *UnixIo;
  UNIX_SIMPLE_FILE_SYSTEM_PRIVATE *Private;
  INTN i;

  Private = NULL;

  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID **)&UnixIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Validate GUID
  //
  if (!CompareGuid (UnixIo->TypeGuid, &gEfiUnixFileSystemGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (UNIX_SIMPLE_FILE_SYSTEM_PRIVATE),
                  (VOID **)&Private
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Private->Signature  = UNIX_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE;
  Private->UnixThunk = UnixIo->UnixThunk;
  Private->FilePath   = NULL;
  Private->VolumeLabel = NULL;

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  StrLen (UnixIo->EnvString) + 1,
                  (VOID **)&Private->FilePath
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  for (i = 0; UnixIo->EnvString[i] != 0; i++)
    Private->FilePath[i] = UnixIo->EnvString[i];
  Private->FilePath[i] = 0;

  Private->VolumeLabel      = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  StrSize (L"EFI_EMULATED"),
                  (VOID **)&Private->VolumeLabel
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  StrCpy (Private->VolumeLabel, L"EFI_EMULATED");

  Private->SimpleFileSystem.Revision    = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Private->SimpleFileSystem.OpenVolume  = UnixSimpleFileSystemOpenVolume;

  Private->ControllerNameTable = NULL;

  AddUnicodeString (
    "eng",
    gUnixSimpleFileSystemComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    UnixIo->EnvString
    );

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Private->SimpleFileSystem,
                  NULL
                  );

Done:
  if (EFI_ERROR (Status)) {

    if (Private != NULL) {

      if (Private->VolumeLabel != NULL)
	gBS->FreePool (Private->VolumeLabel);
      if (Private->FilePath != NULL)
	gBS->FreePool (Private->FilePath);
      FreeUnicodeStringTable (Private->ControllerNameTable);

      gBS->FreePool (Private);
    }

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiUnixIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - A pointer to an instance of the EFI_DRIVER_BINDING_PROTOCOL.

  ControllerHandle  - A handle to the device to be stopped.

  NumberOfChildren  - The number of child device handles in ChildHandleBuffer.

  ChildHandleBuffer - An array of child device handles to be freed.

Returns:

  EFI_SUCCESS       - The device has been stopped.

  EFI_DEVICE_ERROR  - The device could not be stopped due to a device failure.

--*/
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS                        Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *SimpleFileSystem;
  UNIX_SIMPLE_FILE_SYSTEM_PRIVATE *Private;

  //
  // Get our context back
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&SimpleFileSystem,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = UNIX_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (SimpleFileSystem);

  //
  // Uninstall the Simple File System Protocol from ControllerHandle
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Private->SimpleFileSystem,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiUnixIoProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );
  }

  if (!EFI_ERROR (Status)) {
    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    gBS->FreePool (Private);
  }

  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE                        **Root
  )
/*++

Routine Description:

  Open the root directory on a volume.

Arguments:

  This  - A pointer to the volume to open.

  Root  - A pointer to storage for the returned opened file handle of the root directory.

Returns:

  EFI_SUCCESS           - The volume was opened.

  EFI_UNSUPPORTED       - The volume does not support the requested file system type.

  EFI_NO_MEDIA          - The device has no media.

  EFI_DEVICE_ERROR      - The device reported an error.

  EFI_VOLUME_CORRUPTED  - The file system structures are corrupted.

  EFI_ACCESS_DENIED     - The service denied access to the file.

  EFI_OUT_OF_RESOURCES  - The file volume could not be opened due to lack of resources.

  EFI_MEDIA_CHANGED     - The device has new media or the media is no longer supported.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS                        Status;
  UNIX_SIMPLE_FILE_SYSTEM_PRIVATE *Private;
  UNIX_EFI_FILE_PRIVATE           *PrivateFile;
  EFI_TPL                           OldTpl;

  if (This == NULL || Root == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private     = UNIX_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (This);

  PrivateFile = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (UNIX_EFI_FILE_PRIVATE),
                  (VOID **)&PrivateFile
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  PrivateFile->FileName = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  AsciiStrSize (Private->FilePath),
                  (VOID **)&PrivateFile->FileName
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  AsciiStrCpy (PrivateFile->FileName, Private->FilePath);
  PrivateFile->Signature            = UNIX_EFI_FILE_PRIVATE_SIGNATURE;
  PrivateFile->UnixThunk           = Private->UnixThunk;
  PrivateFile->SimpleFileSystem     = This;
  PrivateFile->IsRootDirectory      = TRUE;
  PrivateFile->IsDirectoryPath      = TRUE;
  PrivateFile->IsOpenedByRead       = TRUE;
  PrivateFile->EfiFile.Revision     = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  PrivateFile->EfiFile.Open         = UnixSimpleFileSystemOpen;
  PrivateFile->EfiFile.Close        = UnixSimpleFileSystemClose;
  PrivateFile->EfiFile.Delete       = UnixSimpleFileSystemDelete;
  PrivateFile->EfiFile.Read         = UnixSimpleFileSystemRead;
  PrivateFile->EfiFile.Write        = UnixSimpleFileSystemWrite;
  PrivateFile->EfiFile.GetPosition  = UnixSimpleFileSystemGetPosition;
  PrivateFile->EfiFile.SetPosition  = UnixSimpleFileSystemSetPosition;
  PrivateFile->EfiFile.GetInfo      = UnixSimpleFileSystemGetInfo;
  PrivateFile->EfiFile.SetInfo      = UnixSimpleFileSystemSetInfo;
  PrivateFile->EfiFile.Flush        = UnixSimpleFileSystemFlush;
  PrivateFile->fd                   = -1;
  PrivateFile->Dir                  = NULL;
  PrivateFile->Dirent               = NULL;
  
  *Root = &PrivateFile->EfiFile;

  PrivateFile->Dir = PrivateFile->UnixThunk->OpenDir(PrivateFile->FileName);

  if (PrivateFile->Dir == NULL) {
    Status = EFI_ACCESS_DENIED;
  }
  else {
    Status = EFI_SUCCESS;
  }

Done:
  if (EFI_ERROR (Status)) {
    if (PrivateFile) {
      if (PrivateFile->FileName) {
        gBS->FreePool (PrivateFile->FileName);
      }

      gBS->FreePool (PrivateFile);
    }
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemOpen (
  IN  EFI_FILE  *This,
  OUT EFI_FILE  **NewHandle,
  IN  CHAR16    *FileName,
  IN  UINT64    OpenMode,
  IN  UINT64    Attributes
  )
/*++

Routine Description:

  Open a file relative to the source file location.

Arguments:

  This        - A pointer to the source file location.

  NewHandle   - Pointer to storage for the new file handle.

  FileName    - Pointer to the file name to be opened.

  OpenMode    - File open mode information.

  Attributes  - File creation attributes.

Returns:

  EFI_SUCCESS           - The file was opened.

  EFI_NOT_FOUND         - The file could not be found in the volume.

  EFI_NO_MEDIA          - The device has no media.

  EFI_MEDIA_CHANGED     - The device has new media or the media is no longer supported.

  EFI_DEVICE_ERROR      - The device reported an error.

  EFI_VOLUME_CORRUPTED  - The file system structures are corrupted.

  EFI_WRITE_PROTECTED   - The volume or file is write protected.

  EFI_ACCESS_DENIED     - The service denied access to the file.

  EFI_OUT_OF_RESOURCES  - Not enough resources were available to open the file.

  EFI_VOLUME_FULL       - There is not enough space left to create the new file.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_FILE                          *Root;
  UNIX_EFI_FILE_PRIVATE           *PrivateFile;
  UNIX_EFI_FILE_PRIVATE           *NewPrivateFile;
  UNIX_SIMPLE_FILE_SYSTEM_PRIVATE *PrivateRoot;
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

  TrailingDash = FALSE;

  //
  // Check for obvious invalid parameters.
  //
  if (This == NULL || NewHandle == NULL || FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (OpenMode) {
  case EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
    if (Attributes &~EFI_FILE_VALID_ATTR) {
      return EFI_INVALID_PARAMETER;
    }

    if (Attributes & EFI_FILE_READ_ONLY) {
      return EFI_INVALID_PARAMETER;
    }

  //
  // fall through
  //
  case EFI_FILE_MODE_READ:
  case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }


  PrivateFile     = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot     = UNIX_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);
  NewPrivateFile  = NULL;

  //
  // BUGBUG: assume an open of root
  // if current location, return current data
  //
  if (StrCmp (FileName, L"\\") == 0
      || (StrCmp (FileName, L".") == 0 && PrivateFile->IsRootDirectory)) {
    //
    // BUGBUG: assume an open root
    //
OpenRoot:
    Status          = UnixSimpleFileSystemOpenVolume (PrivateFile->SimpleFileSystem, &Root);
    NewPrivateFile  = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (Root);
    goto Done;
  }

  if (FileName[StrLen (FileName) - 1] == L'\\') {
    TrailingDash                        = TRUE;
    FileName[StrLen (FileName) - 1]  = 0;
  }

  //
  // Attempt to open the file
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (UNIX_EFI_FILE_PRIVATE),
                  (VOID **)&NewPrivateFile
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  CopyMem (NewPrivateFile, PrivateFile, sizeof (UNIX_EFI_FILE_PRIVATE));

  NewPrivateFile->FileName = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  AsciiStrSize (PrivateFile->FileName) + 1 + StrLen (FileName) + 1,
                  (VOID **)&NewPrivateFile->FileName
                  );

  if (EFI_ERROR (Status)) {
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
  Dst = NewPrivateFile->FileName + AsciiStrLen(NewPrivateFile->FileName);
  GuardPointer = NewPrivateFile->FileName + AsciiStrLen(PrivateRoot->FilePath);
  *Dst++ = '/';
  // Convert unicode to ascii and '\' to '/'
  while (*Src) {
    if (*Src == '\\')
      *Dst++ = '/';
    else
      *Dst++ = *Src;
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
    gBS->FreePool (NewPrivateFile->FileName);
    gBS->FreePool (NewPrivateFile);
    goto OpenRoot;
  }

  RealFileName = NewPrivateFile->FileName + AsciiStrLen(NewPrivateFile->FileName) - 1;
  while (RealFileName > NewPrivateFile->FileName && *RealFileName != '/')
    RealFileName--;

  TempChar            = *(RealFileName - 1);
  *(RealFileName - 1) = 0;

  *(RealFileName - 1)             = TempChar;



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
    struct stat finfo;
    int res = NewPrivateFile->UnixThunk->Stat (NewPrivateFile->FileName, &finfo);
    if (res == 0 && S_ISDIR(finfo.st_mode))
      NewPrivateFile->IsDirectoryPath = TRUE;
    else
      NewPrivateFile->IsDirectoryPath = FALSE;
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
      if (NewPrivateFile->UnixThunk->MkDir (NewPrivateFile->FileName, 0777) != 0) {
	INTN LastError;

        LastError = PrivateFile->UnixThunk->GetErrno ();
        if (LastError != EEXIST) {
          //gBS->FreePool (TempFileName);
          Status = EFI_ACCESS_DENIED;
          goto Done;
        }
      }
    }

    NewPrivateFile->Dir = NewPrivateFile->UnixThunk->OpenDir
      (NewPrivateFile->FileName);

    if (NewPrivateFile->Dir == NULL) {
      if (PrivateFile->UnixThunk->GetErrno () == EACCES) {
        Status                    = EFI_ACCESS_DENIED;
      } else {
        Status = EFI_NOT_FOUND;
      }

      goto Done;
    }

  } else {
    //
    // deal with file
    //
    NewPrivateFile->fd = NewPrivateFile->UnixThunk->Open
      (NewPrivateFile->FileName,
       ((OpenMode & EFI_FILE_MODE_CREATE) ? O_CREAT : 0)
       | (NewPrivateFile->IsOpenedByRead ? O_RDONLY : O_RDWR),
       0666);
    if (NewPrivateFile->fd < 0) {
      if (PrivateFile->UnixThunk->GetErrno () == ENOENT) {
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

    Status    = UnixSimpleFileSystemGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);

    if (Status != EFI_BUFFER_TOO_SMALL) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    InfoSize,
                    (VOID **)&Info
                    );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Status = UnixSimpleFileSystemGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Info->Attribute = Attributes;

    UnixSimpleFileSystemSetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, InfoSize, Info);
  }

Done: ;
  if (TrailingDash) {
    FileName[StrLen (FileName) + 1]  = 0;
    FileName[StrLen (FileName)]      = L'\\';
  }

  if (EFI_ERROR (Status)) {
    if (NewPrivateFile) {
      if (NewPrivateFile->FileName) {
        gBS->FreePool (NewPrivateFile->FileName);
      }

      gBS->FreePool (NewPrivateFile);
    }
  } else {
    *NewHandle = &NewPrivateFile->EfiFile;
  }

  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemClose (
  IN EFI_FILE  *This
  )
/*++

Routine Description:

  Close the specified file handle.

Arguments:

  This  - Pointer to a returned opened file handle.

Returns:

  EFI_SUCCESS - The file handle has been closed.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  UNIX_EFI_FILE_PRIVATE *PrivateFile;
  EFI_TPL                OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateFile = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (PrivateFile->fd >= 0) {
    PrivateFile->UnixThunk->Close (PrivateFile->fd);
  }
  if (PrivateFile->Dir != NULL) {
    PrivateFile->UnixThunk->CloseDir (PrivateFile->Dir);
  }

  PrivateFile->fd = -1;
  PrivateFile->Dir = NULL;

  if (PrivateFile->FileName) {
    gBS->FreePool (PrivateFile->FileName);
  }

  gBS->FreePool (PrivateFile);

  gBS->RestoreTPL (OldTpl);
  
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemDelete (
  IN EFI_FILE  *This
  )
/*++

Routine Description:

  Close and delete a file.

Arguments:

  This  - Pointer to a returned opened file handle.

Returns:

  EFI_SUCCESS             - The file handle was closed and deleted.

  EFI_WARN_DELETE_FAILURE - The handle was closed but could not be deleted.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS              Status;
  UNIX_EFI_FILE_PRIVATE   *PrivateFile;
  EFI_TPL                 OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  
  PrivateFile = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status      = EFI_WARN_DELETE_FAILURE;

  if (PrivateFile->IsDirectoryPath) {
    if (PrivateFile->Dir != NULL) {
      PrivateFile->UnixThunk->CloseDir (PrivateFile->Dir);
      PrivateFile->Dir = NULL;
    }

    if (PrivateFile->UnixThunk->RmDir (PrivateFile->FileName) == 0) {
      Status = EFI_SUCCESS;
    }
  } else {
    PrivateFile->UnixThunk->Close (PrivateFile->fd);
    PrivateFile->fd = -1;

    if (!PrivateFile->IsOpenedByRead) {
      if (!PrivateFile->UnixThunk->UnLink (PrivateFile->FileName)) {
        Status = EFI_SUCCESS;
      }
    }
  }

  gBS->FreePool (PrivateFile->FileName);
  gBS->FreePool (PrivateFile);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

VOID
UnixSystemTimeToEfiTime (
  EFI_UNIX_THUNK_PROTOCOL        *UnixThunk,
  IN time_t                 SystemTime,
  OUT EFI_TIME              *Time
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SystemTime  - TODO: add argument description
  TimeZone    - TODO: add argument description
  Time        - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  struct tm *tm;
  tm = UnixThunk->GmTime (&SystemTime);
  Time->Year   = tm->tm_year;
  Time->Month  = tm->tm_mon;
  Time->Day    = tm->tm_mday;
  Time->Hour   = tm->tm_hour;
  Time->Minute = tm->tm_min;
  Time->Second = tm->tm_sec;
  Time->Nanosecond  = 0;

  Time->TimeZone    = UnixThunk->GetTimeZone ();

  if (UnixThunk->GetDayLight ()) {
    Time->Daylight = EFI_TIME_ADJUST_DAYLIGHT;
  }
}

EFI_STATUS
UnixSimpleFileSystemFileInfo (
  UNIX_EFI_FILE_PRIVATE          *PrivateFile,
  IN     CHAR8                    *FileName,
  IN OUT UINTN                    *BufferSize,
  OUT    VOID                     *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PrivateFile - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  EFI_STATUS                  Status;
  UINTN                       Size;
  UINTN                       NameSize;
  UINTN                       ResultSize;
  EFI_FILE_INFO               *Info;
  CHAR8                      *RealFileName;
  CHAR8                      *TempPointer;
  CHAR16                      *BufferFileName;
  struct stat                 buf;

  if (FileName != NULL) {
    RealFileName = FileName;
  }
  else if (PrivateFile->IsRootDirectory) {
    RealFileName = "";
  } else {
    RealFileName  = PrivateFile->FileName;
  }

  TempPointer   = RealFileName;
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
  if (PrivateFile->UnixThunk->Stat (
          FileName == NULL ? PrivateFile->FileName : FileName,
	  &buf) < 0)
    return EFI_DEVICE_ERROR;

  Status  = EFI_SUCCESS;

  Info    = Buffer;
  ZeroMem (Info, ResultSize);

  Info->Size = ResultSize;
  Info->FileSize      = buf.st_size;
  Info->PhysicalSize  = MultU64x32 (buf.st_blocks, buf.st_blksize);

  UnixSystemTimeToEfiTime (PrivateFile->UnixThunk, buf.st_ctime, &Info->CreateTime);
  UnixSystemTimeToEfiTime (PrivateFile->UnixThunk, buf.st_atime, &Info->LastAccessTime);
  UnixSystemTimeToEfiTime (PrivateFile->UnixThunk, buf.st_mtime, &Info->ModificationTime);

  if (!(buf.st_mode & S_IWUSR)) {
    Info->Attribute |= EFI_FILE_READ_ONLY;
  }

  if (S_ISDIR(buf.st_mode)) {
    Info->Attribute |= EFI_FILE_DIRECTORY;
  }


  BufferFileName = (CHAR16 *)((CHAR8 *) Buffer + Size);
  while (*RealFileName)
    *BufferFileName++ = *RealFileName++;
  *BufferFileName = 0;

  *BufferSize = ResultSize;
  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemRead (
  IN     EFI_FILE  *This,
  IN OUT UINTN     *BufferSize,
  OUT    VOID      *Buffer
  )
/*++

Routine Description:

  Read data from a file.

Arguments:

  This        - Pointer to a returned open file handle.

  BufferSize  - On input, the size of the Buffer.  On output, the number of bytes stored in the Buffer.

  Buffer      - Pointer to the first byte of the read Buffer.

Returns:

  EFI_SUCCESS           - The data was read.

  EFI_NO_MEDIA          - The device has no media.

  EFI_DEVICE_ERROR      - The device reported an error.

  EFI_VOLUME_CORRUPTED  - The file system structures are corrupted.

  EFI_BUFFER_TOO_SMALL  - The supplied buffer size was too small to store the current directory entry.
                          *BufferSize has been updated with the size needed to complete the request.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  UNIX_EFI_FILE_PRIVATE *PrivateFile;
  EFI_STATUS              Status;
  INTN                    Res;
  UINTN                   Size;
  UINTN                   NameSize;
  UINTN                   ResultSize;
  CHAR8                   *FullFileName;
  EFI_TPL                 OldTpl;

  if (This == NULL || BufferSize == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  
  PrivateFile = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (!PrivateFile->IsDirectoryPath) {

    if (PrivateFile->fd < 0) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    Res = PrivateFile->UnixThunk->Read (
					 PrivateFile->fd,
					 Buffer,
					 *BufferSize);
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
    PrivateFile->Dirent = PrivateFile->UnixThunk->ReadDir (PrivateFile->Dir);
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

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  AsciiStrLen(PrivateFile->FileName) + 1 + NameSize,
                  (VOID **)&FullFileName
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }
		  
  AsciiStrCpy(FullFileName, PrivateFile->FileName);
  AsciiStrCat(FullFileName, "/");
  AsciiStrCat(FullFileName, PrivateFile->Dirent->d_name);
  Status = UnixSimpleFileSystemFileInfo (PrivateFile,
					  FullFileName,
					  BufferSize,
					  Buffer);
  gBS->FreePool (FullFileName);

  PrivateFile->Dirent = NULL;

Done:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemWrite (
  IN     EFI_FILE  *This,
  IN OUT UINTN     *BufferSize,
  IN     VOID      *Buffer
  )
/*++

Routine Description:

  Write data to a file.

Arguments:

  This        - Pointer to an opened file handle.

  BufferSize  - On input, the number of bytes in the Buffer to write to the file.  On output, the number of bytes
                of data written to the file.

  Buffer      - Pointer to the first by of data in the buffer to write to the file.

Returns:

  EFI_SUCCESS           - The data was written to the file.

  EFI_UNSUPPORTED       - Writes to an open directory are not supported.

  EFI_NO_MEDIA          - The device has no media.

  EFI_DEVICE_ERROR      - The device reported an error.

  EFI_VOLUME_CORRUPTED  - The file system structures are corrupt.

  EFI_WRITE_PROTECTED   - The file, directory, volume, or device is write protected.

  EFI_ACCESS_DENIED     - The file was opened read-only.

  EFI_VOLUME_FULL       - The volume is full.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  UNIX_EFI_FILE_PRIVATE *PrivateFile;
  UINTN                 Res;
  EFI_STATUS            Status;
  EFI_TPL               OldTpl;

  if (This == NULL || BufferSize == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->fd < 0) {
    return EFI_DEVICE_ERROR;
  }

  if (PrivateFile->IsDirectoryPath) {
    return EFI_UNSUPPORTED;
  }

  if (PrivateFile->IsOpenedByRead) {
    return EFI_ACCESS_DENIED;
  }

  Res = PrivateFile->UnixThunk->Write (
					PrivateFile->fd,
					Buffer,
					*BufferSize);
  if (Res == (UINTN)-1) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  *BufferSize = Res;
  Status = EFI_SUCCESS;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;

  //
  // bugbug: need to access unix error reporting
  //
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemSetPosition (
  IN EFI_FILE  *This,
  IN UINT64    Position
  )
/*++

Routine Description:

  Set a file's current position.

Arguments:

  This      - Pointer to an opened file handle.

  Position  - The byte position from the start of the file to set.

Returns:

  EFI_SUCCESS     - The file position has been changed.

  EFI_UNSUPPORTED - The seek request for non-zero is not supported for directories.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS              Status;
  UNIX_EFI_FILE_PRIVATE *PrivateFile;
  UINT64                  Pos;
  EFI_TPL                 OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  
  PrivateFile = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->IsDirectoryPath) {
    if (Position != 0) {
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    if (PrivateFile->Dir == NULL) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }
    PrivateFile->UnixThunk->RewindDir (PrivateFile->Dir);
    Status = EFI_SUCCESS;
    goto Done;
  } else {
    if (Position == (UINT64) -1) {
      Pos = PrivateFile->UnixThunk->Lseek (PrivateFile->fd, 0, SEEK_END);
    } else {
      Pos = PrivateFile->UnixThunk->Lseek (PrivateFile->fd, Position, SEEK_SET);
    }
    Status = (Pos == (UINT64) -1) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  }

Done:
    gBS->RestoreTPL (OldTpl);
    return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemGetPosition (
  IN  EFI_FILE  *This,
  OUT UINT64    *Position
  )
/*++

Routine Description:

  Get a file's current position.

Arguments:

  This      - Pointer to an opened file handle.

  Position  - Pointer to storage for the current position.

Returns:

  EFI_SUCCESS     - The file position has been reported.

  EFI_UNSUPPORTED - Not valid for directories.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS            Status;
  UNIX_EFI_FILE_PRIVATE *PrivateFile;
  EFI_TPL               OldTpl;

  if (This == NULL || Position == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  
  PrivateFile   = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->IsDirectoryPath) {
    Status = EFI_UNSUPPORTED;
  } else {
    *Position = PrivateFile->UnixThunk->Lseek (PrivateFile->fd, 0, SEEK_CUR);
    Status = (*Position == (UINT64) -1) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemGetInfo (
  IN     EFI_FILE  *This,
  IN     EFI_GUID  *InformationType,
  IN OUT UINTN     *BufferSize,
  OUT    VOID      *Buffer
  )
/*++

Routine Description:

  Return information about a file or volume.

Arguments:

  This            - Pointer to an opened file handle.

  InformationType - GUID describing the type of information to be returned.

  BufferSize      - On input, the size of the information buffer.  On output, the number of bytes written to the
                    information buffer.

  Buffer          - Pointer to the first byte of the information buffer.

Returns:

  EFI_SUCCESS           - The requested information has been written into the buffer.

  EFI_UNSUPPORTED       - The InformationType is not known.

  EFI_NO_MEDIA          - The device has no media.

  EFI_DEVICE_ERROR      - The device reported an error.

  EFI_VOLUME_CORRUPTED  - The file system structures are corrupt.

  EFI_BUFFER_TOO_SMALL  - The buffer size was too small to contain the requested information.  The buffer size has
                          been updated with the size needed to complete the requested operation.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS                        Status;
  UNIX_EFI_FILE_PRIVATE           *PrivateFile;
  EFI_FILE_SYSTEM_INFO              *FileSystemInfoBuffer;
  INTN                              UnixStatus;
  UNIX_SIMPLE_FILE_SYSTEM_PRIVATE *PrivateRoot;
  struct statfs                     buf;
  EFI_TPL                           OldTpl;

  if (This == NULL || InformationType == NULL || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
    
  PrivateFile = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot = UNIX_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);

  Status      = EFI_UNSUPPORTED;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = UnixSimpleFileSystemFileInfo (PrivateFile, NULL, BufferSize, Buffer);
  } else if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    if (*BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel)) {
      *BufferSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
      Status = EFI_BUFFER_TOO_SMALL;
      goto Done;
    }

    UnixStatus = PrivateFile->UnixThunk->StatFs (PrivateFile->FileName, &buf);
    if (UnixStatus < 0) {
        Status = EFI_DEVICE_ERROR;
        goto Done;
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
    Status      = EFI_SUCCESS;
  } else if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    if (*BufferSize < StrSize (PrivateRoot->VolumeLabel)) {
      *BufferSize = StrSize (PrivateRoot->VolumeLabel);
      Status = EFI_BUFFER_TOO_SMALL;
      goto Done;
    }

    StrCpy ((CHAR16 *) Buffer, PrivateRoot->VolumeLabel);
    *BufferSize = StrSize (PrivateRoot->VolumeLabel);
    Status      = EFI_SUCCESS;
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemSetInfo (
  IN EFI_FILE         *This,
  IN EFI_GUID         *InformationType,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  )
/*++

Routine Description:

  Set information about a file or volume.

Arguments:

  This            - Pointer to an opened file handle.

  InformationType - GUID identifying the type of information to set.

  BufferSize      - Number of bytes of data in the information buffer.

  Buffer          - Pointer to the first byte of data in the information buffer.

Returns:

  EFI_SUCCESS           - The file or volume information has been updated.

  EFI_UNSUPPORTED       - The information identifier is not recognised.

  EFI_NO_MEDIA          - The device has no media.

  EFI_DEVICE_ERROR      - The device reported an error.

  EFI_VOLUME_CORRUPTED  - The file system structures are corrupt.

  EFI_WRITE_PROTECTED   - The file, directory, volume, or device is write protected.

  EFI_ACCESS_DENIED     - The file was opened read-only.

  EFI_VOLUME_FULL       - The volume is full.

  EFI_BAD_BUFFER_SIZE   - The buffer size is smaller than the type indicated by InformationType.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  UNIX_SIMPLE_FILE_SYSTEM_PRIVATE *PrivateRoot;
  UNIX_EFI_FILE_PRIVATE           *PrivateFile;
  EFI_FILE_INFO                     *OldFileInfo;
  EFI_FILE_INFO                     *NewFileInfo;
  EFI_STATUS                        Status;
  UINTN                             OldInfoSize;
  EFI_TPL                           OldTpl;
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
  INTN                              UnixStatus;

  //
  // Check for invalid parameters.
  //
  if (This == NULL || InformationType == NULL || BufferSize == 0 || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  
  //
  // Initialise locals.
  //
  PrivateFile               = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot               = UNIX_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);

  Status                    = EFI_UNSUPPORTED;
  OldFileInfo               = NewFileInfo = NULL;
  OldFileName               = NewFileName = NULL;
  AttrChangeFlag = NameChangeFlag = SizeChangeFlag = TimeChangeFlag = FALSE;

  //
  // Set file system information.
  //
  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    if (BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel)) {
      Status = EFI_BAD_BUFFER_SIZE;
      goto Done;
    }

    NewFileSystemInfo = (EFI_FILE_SYSTEM_INFO *) Buffer;

    gBS->FreePool (PrivateRoot->VolumeLabel);

    PrivateRoot->VolumeLabel = NULL;
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    StrSize (NewFileSystemInfo->VolumeLabel),
                    (VOID **)&PrivateRoot->VolumeLabel
                    );

    if (EFI_ERROR (Status)) {
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
  // bugbug: - This is not safe.  We need something like EfiStrMaxSize()
  // that would have an additional parameter that would be the size
  // of the string array just in case there are no NULL characters in
  // the string array.
  //
  //
  // Get current file information so we can determine what kind
  // of change request this is.
  //
  OldInfoSize = 0;
  Status      = UnixSimpleFileSystemFileInfo (PrivateFile, NULL, &OldInfoSize, NULL);

  if (Status != EFI_BUFFER_TOO_SMALL) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  Status = gBS->AllocatePool (EfiBootServicesData, OldInfoSize,
			      (VOID **)&OldFileInfo);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = UnixSimpleFileSystemFileInfo (PrivateFile, NULL, &OldInfoSize, OldFileInfo);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  AsciiStrSize (PrivateFile->FileName),
                  (VOID **)&OldFileName
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  AsciiStrCpy (OldFileName, PrivateFile->FileName);

  //
  // Make full pathname from new filename and rootpath.
  //
  if (NewFileInfo->FileName[0] == '\\') {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    AsciiStrLen (PrivateRoot->FilePath) + 1 + StrLen (NewFileInfo->FileName) + 1,
                    (VOID **)&NewFileName
                    );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    AsciiStrCpy (NewFileName, PrivateRoot->FilePath);
    AsciiFilePtr = NewFileName + AsciiStrLen(NewFileName);
    UnicodeFilePtr = NewFileInfo->FileName + 1;
    *AsciiFilePtr++ ='/';
  } else {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    AsciiStrLen (PrivateFile->FileName) + 1 + StrLen (NewFileInfo->FileName) + 1,
                    (VOID **)&NewFileName
                    );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    AsciiStrCpy (NewFileName, PrivateRoot->FilePath);
    AsciiFilePtr = NewFileName + AsciiStrLen(NewFileName);
    while (AsciiFilePtr > NewFileName && AsciiFilePtr[-1] != '/') {
      AsciiFilePtr--;
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
  // bugbug: - Need EfiStrCaseCmp()
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
  if (PrivateFile->UnixThunk->Stat (OldFileName, &OldAttr) != 0) {
    Status = EFI_DEVICE_ERROR;
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

    UnixStatus = PrivateFile->UnixThunk->Rename (OldFileName, NewFileName);

    if (UnixStatus == 0) {
      //
      // modify file name
      //
      gBS->FreePool (PrivateFile->FileName);

      Status = gBS->AllocatePool (
                      EfiBootServicesData,
                      AsciiStrSize (NewFileName),
                      (VOID **)&PrivateFile->FileName
                      );

      if (EFI_ERROR (Status)) {
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

    if (PrivateFile->UnixThunk->FTruncate (PrivateFile->fd, NewFileInfo->FileSize) != 0) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

  }

  //
  // Time change
  //
  if (TimeChangeFlag) {
    struct utimbuf utime;

    NewLastAccessSystemTime.tm_year    = NewFileInfo->LastAccessTime.Year;
    NewLastAccessSystemTime.tm_mon     = NewFileInfo->LastAccessTime.Month;
    NewLastAccessSystemTime.tm_mday    = NewFileInfo->LastAccessTime.Day;
    NewLastAccessSystemTime.tm_hour    = NewFileInfo->LastAccessTime.Hour;
    NewLastAccessSystemTime.tm_min     = NewFileInfo->LastAccessTime.Minute;
    NewLastAccessSystemTime.tm_sec     = NewFileInfo->LastAccessTime.Second;
    NewLastAccessSystemTime.tm_isdst   = 0;

    utime.actime = PrivateFile->UnixThunk->MkTime (&NewLastAccessSystemTime);

    NewLastWriteSystemTime.tm_year    = NewFileInfo->ModificationTime.Year;
    NewLastWriteSystemTime.tm_mon     = NewFileInfo->ModificationTime.Month;
    NewLastWriteSystemTime.tm_mday    = NewFileInfo->ModificationTime.Day;
    NewLastWriteSystemTime.tm_hour    = NewFileInfo->ModificationTime.Hour;
    NewLastWriteSystemTime.tm_min     = NewFileInfo->ModificationTime.Minute;
    NewLastWriteSystemTime.tm_sec     = NewFileInfo->ModificationTime.Second;
    NewLastWriteSystemTime.tm_isdst   = 0;

    utime.modtime = PrivateFile->UnixThunk->MkTime (&NewLastWriteSystemTime);

    if (utime.actime == (time_t)-1 || utime.modtime == (time_t)-1) {
      goto Done;
    }

    if (PrivateFile->UnixThunk->UTime (PrivateFile->FileName, &utime) == -1) {
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

  UnixStatus = PrivateFile->UnixThunk->Chmod (NewFileName, NewAttr);

  if (UnixStatus != 0) {
    Status    = EFI_DEVICE_ERROR;
  }

Done:
  if (OldFileInfo != NULL) {
    gBS->FreePool (OldFileInfo);
  }

  if (OldFileName != NULL) {
    gBS->FreePool (OldFileName);
  }

  if (NewFileName != NULL) {
    gBS->FreePool (NewFileName);
  }

  gBS->RestoreTPL (OldTpl);
  
  return Status;
}

EFI_STATUS
EFIAPI
UnixSimpleFileSystemFlush (
  IN EFI_FILE  *This
  )
/*++

Routine Description:

  Flush all modified data to the media.

Arguments:

  This  - Pointer to an opened file handle.

Returns:

  EFI_SUCCESS           - The data has been flushed.

  EFI_NO_MEDIA          - The device has no media.

  EFI_DEVICE_ERROR      - The device reported an error.

  EFI_VOLUME_CORRUPTED  - The file system structures have been corrupted.

  EFI_WRITE_PROTECTED   - The file, directory, volume, or device is write protected.

  EFI_ACCESS_DENIED     - The file was opened read-only.

  EFI_VOLUME_FULL       - The volume is full.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  UNIX_EFI_FILE_PRIVATE     *PrivateFile;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  
  PrivateFile = UNIX_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);


  if (PrivateFile->IsDirectoryPath) {
    goto Done;
  }

  if (PrivateFile->IsOpenedByRead) {
    Status = EFI_ACCESS_DENIED;
    goto Done;
  }

  if (PrivateFile->fd < 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  PrivateFile->UnixThunk->FSync (PrivateFile->fd) == 0 ? EFI_SUCCESS : EFI_DEVICE_ERROR;

Done:
  gBS->RestoreTPL (OldTpl);

  return Status;

  //
  // bugbug: - Use Unix error reporting.
  //
}



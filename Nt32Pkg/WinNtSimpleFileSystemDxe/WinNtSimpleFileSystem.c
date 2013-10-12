/**@file

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtSimpleFileSystem.c

Abstract:

  Produce Simple File System abstractions for directories on your PC using Win32 APIs.
  The configuration of what devices to mount or emulate comes from NT
  environment variables. The variables must be visible to the Microsoft*
  Developer Studio for them to work.

  * Other names and brands may be claimed as the property of others.

**/

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <WinNtDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/ComponentName.h>
#include <Guid/FileInfo.h>
#include <Protocol/DriverBinding.h>
#include <Guid/FileSystemInfo.h>
#include <Protocol/SimpleFileSystem.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "WinNtSimpleFileSystem.h"

EFI_DRIVER_BINDING_PROTOCOL gWinNtSimpleFileSystemDriverBinding = {
  WinNtSimpleFileSystemDriverBindingSupported,
  WinNtSimpleFileSystemDriverBindingStart,
  WinNtSimpleFileSystemDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The user Entry Point for module WinNtSimpleFileSystem. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeWinNtSimpleFileSystem(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gWinNtSimpleFileSystemDriverBinding,
             ImageHandle,
             &gWinNtSimpleFileSystemComponentName,
             &gWinNtSimpleFileSystemComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

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
  IN  CHAR16  *Str,
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
  CHAR16  *Pointer;

  if (StrLen (Str) < Count) {
    ASSERT (0);
  }

  if (Count != 0) {
    for (Pointer = Str; *(Pointer + Count); Pointer++) {
      *Pointer = *(Pointer + Count);
    }
    *Pointer = *(Pointer + Count);
  }
}



EFI_STATUS
EFIAPI
WinNtSimpleFileSystemDriverBindingSupported (
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
  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
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
  if (CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtFileSystemGuid)) {
    Status = EFI_SUCCESS;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiWinNtIoProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemDriverBindingStart (
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
  EFI_WIN_NT_IO_PROTOCOL            *WinNtIo;
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE *Private;

  Private = NULL;

  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
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
  if (!CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtFileSystemGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Private = AllocatePool (sizeof (WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;

    goto Done;
  }

  Private->Signature  = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE;
  Private->WinNtThunk = WinNtIo->WinNtThunk;

  Private->FilePath = WinNtIo->EnvString;

  Private->VolumeLabel = AllocatePool (StrSize (L"EFI_EMULATED"));
  if (Private->VolumeLabel == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  StrCpy (Private->VolumeLabel, L"EFI_EMULATED");

  Private->SimpleFileSystem.Revision    = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Private->SimpleFileSystem.OpenVolume  = WinNtSimpleFileSystemOpenVolume;

  Private->WinNtThunk->SetErrorMode (SEM_FAILCRITICALERRORS);

  Private->ControllerNameTable = NULL;

  AddUnicodeString2 (
    "eng",
    gWinNtSimpleFileSystemComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    WinNtIo->EnvString,
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gWinNtSimpleFileSystemComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    WinNtIo->EnvString,
    FALSE
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

      FreeUnicodeStringTable (Private->ControllerNameTable);

      FreePool (Private);
    }

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiWinNtIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemDriverBindingStop (
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
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE *Private;

  //
  // Get our context back
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &SimpleFileSystem,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (SimpleFileSystem);

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
                    &gEfiWinNtIoProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );
  }

  if (!EFI_ERROR (Status)) {
    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    FreePool (Private);
  }

  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL               **Root
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
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE *Private;
  WIN_NT_EFI_FILE_PRIVATE           *PrivateFile;
  EFI_TPL                           OldTpl;
  CHAR16                            *TempFileName;
  UINTN                             Size;

  if (This == NULL || Root == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private     = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (This);

  PrivateFile = AllocatePool (sizeof (WIN_NT_EFI_FILE_PRIVATE));
  if (PrivateFile == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  PrivateFile->FileName = AllocatePool (StrSize (Private->FilePath));
  if (PrivateFile->FileName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  PrivateFile->FilePath = AllocatePool (StrSize (Private->FilePath));
  if (PrivateFile->FilePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  StrCpy (PrivateFile->FilePath, Private->FilePath);
  StrCpy (PrivateFile->FileName, PrivateFile->FilePath);
  PrivateFile->Signature            = WIN_NT_EFI_FILE_PRIVATE_SIGNATURE;
  PrivateFile->WinNtThunk           = Private->WinNtThunk;
  PrivateFile->SimpleFileSystem     = This;
  PrivateFile->IsRootDirectory      = TRUE;
  PrivateFile->IsDirectoryPath      = TRUE;
  PrivateFile->IsOpenedByRead       = TRUE;
  PrivateFile->EfiFile.Revision     = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  PrivateFile->EfiFile.Open         = WinNtSimpleFileSystemOpen;
  PrivateFile->EfiFile.Close        = WinNtSimpleFileSystemClose;
  PrivateFile->EfiFile.Delete       = WinNtSimpleFileSystemDelete;
  PrivateFile->EfiFile.Read         = WinNtSimpleFileSystemRead;
  PrivateFile->EfiFile.Write        = WinNtSimpleFileSystemWrite;
  PrivateFile->EfiFile.GetPosition  = WinNtSimpleFileSystemGetPosition;
  PrivateFile->EfiFile.SetPosition  = WinNtSimpleFileSystemSetPosition;
  PrivateFile->EfiFile.GetInfo      = WinNtSimpleFileSystemGetInfo;
  PrivateFile->EfiFile.SetInfo      = WinNtSimpleFileSystemSetInfo;
  PrivateFile->EfiFile.Flush        = WinNtSimpleFileSystemFlush;
  PrivateFile->IsValidFindBuf       = FALSE;

  //
  // Set DirHandle
  //
  PrivateFile->DirHandle = PrivateFile->WinNtThunk->CreateFile (
                                                      PrivateFile->FilePath,
                                                      GENERIC_READ,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                      NULL,
                                                      OPEN_EXISTING,
                                                      FILE_FLAG_BACKUP_SEMANTICS,
                                                      NULL
                                                      );

  if (PrivateFile->DirHandle == INVALID_HANDLE_VALUE) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Find the first file under it
  //
  Size  = StrSize (PrivateFile->FilePath);
  Size += StrSize (L"\\*");
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  Size,
                  (VOID **)&TempFileName
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  StrCpy (TempFileName, PrivateFile->FilePath);
  StrCat (TempFileName, L"\\*");

  PrivateFile->LHandle = PrivateFile->WinNtThunk->FindFirstFile (TempFileName, &PrivateFile->FindBuf);

  if (PrivateFile->LHandle == INVALID_HANDLE_VALUE) {
    PrivateFile->IsValidFindBuf = FALSE;
  } else {
    PrivateFile->IsValidFindBuf = TRUE;
  }
  *Root = &PrivateFile->EfiFile;

  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    if (PrivateFile) {
      if (PrivateFile->FileName) {
        FreePool (PrivateFile->FileName);
      }

      if (PrivateFile->FilePath) {
        FreePool (PrivateFile->FilePath);
      }

      FreePool (PrivateFile);
    }
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Count the number of Leading Dot in FileNameToken.

  @param FileNameToken  A string representing a token in the path name.

  @return  UINTN             The number of leading dot in the name.

**/
UINTN
CountLeadingDots (
  IN CONST CHAR16 * FileNameToken
  )
{
  UINTN          Num;

  Num = 0;
  while (*FileNameToken == L'.') {
    Num++;
    FileNameToken++;
  }
  
  return Num;
}

BOOLEAN 
IsFileNameTokenValid (
  IN CONST CHAR16 * FileNameToken
  )
{
  UINTN Num;
  if (StrStr (FileNameToken, L"/") != NULL) {
    //
    // No L'/' in file name.
    //
    return FALSE;
  } else {
    //
    // If Token has all dot, the number should not exceed 2
    //
    Num = CountLeadingDots (FileNameToken);

    if (Num == StrLen (FileNameToken)) {
      //
      // If the FileNameToken only contains a number of L'.'.
      //
      if (Num > 2) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**
  Return the first string token found in the indirect pointer a String named by FileName.

  On input, FileName is a indirect pointer pointing to a String.
  On output, FileName is a updated to point to the next character after the first
  found L"\" or NULL if there is no L"\" found.

  @param FileName  A indirect pointer pointing to a FileName.

  @return  Token      The first string token found before a L"\".

**/
CHAR16 *
GetNextFileNameToken (
  IN OUT CONST CHAR16 ** FileName 
  )
{
  CHAR16 *SlashPos;
  CHAR16 *Token;
  UINTN  Offset;
  ASSERT (**FileName != L'\\');
  ASSERT (**FileName != L'\0');

  SlashPos = StrStr (*FileName, L"\\");
  if (SlashPos == NULL) {
    Token = AllocateCopyPool (StrSize(*FileName), *FileName);
    *FileName = NULL;
  } else {
    Offset = SlashPos - *FileName;
    Token = AllocateZeroPool ((Offset + 1) * sizeof (CHAR16));
    StrnCpy (Token, *FileName, Offset);
    //
    // Point *FileName to the next character after L'\'.
    //
    *FileName = *FileName + Offset + 1;
  }

  return Token;
}

/**
  Check if a FileName contains only Valid Characters.

  If FileName contains only a single L'\', return TRUE.
  If FileName contains two adjacent L'\', return FALSE.
  If FileName conatins L'/' , return FALSE.
  If FielName contains more than two dots seperated with other FileName characters
  by L'\', return FALSE. For example, L'.\...\filename.txt' is invalid path name. But L'..TwoDots\filename.txt' is valid path name.

  @param FileName  The File Name String to check.

  @return  TRUE        FileName only contains valid characters.
  @return  FALSE       FileName contains at least one invalid character.

**/

BOOLEAN
IsFileNameValid (
  IN CONST CHAR16 *FileName 
  )
{
  CHAR16       *Token;
  BOOLEAN      Valid;

  //
  // If FileName is just L'\', then it is a valid pathname. 
  //
  if (StrCmp (FileName, L"\\") == 0) {
    return TRUE;
  }
  //
  // We don't support two or more adjacent L'\'.
  //
  if (StrStr (FileName, L"\\\\") != NULL) {
    return FALSE;
  }

  //
  // Is FileName has a leading L"\", skip to next character.
  //
  if (FileName [0] == L'\\') {
    FileName++;
  }

  do {
    Token = GetNextFileNameToken (&FileName);
    Valid = IsFileNameTokenValid (Token);
    FreePool (Token);
    
    if (!Valid)
      return FALSE;
  } while (FileName != NULL);

  return TRUE;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemOpen (
  IN  EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN  CHAR16             *FileName,
  IN  UINT64             OpenMode,
  IN  UINT64             Attributes
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
  WIN_NT_EFI_FILE_PRIVATE           *PrivateFile;
  WIN_NT_EFI_FILE_PRIVATE           *NewPrivateFile;
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE *PrivateRoot;
  EFI_STATUS                        Status;
  CHAR16                            *RealFileName;
  CHAR16                            *TempFileName;
  CHAR16                            *ParseFileName;
  CHAR16                            *GuardPointer;
  CHAR16                            TempChar;
  DWORD                             LastError;
  UINTN                             Count;
  BOOLEAN                           LoopFinish;
  UINTN                             InfoSize;
  EFI_FILE_INFO                     *Info;
  UINTN                             Size;

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

  //
  // Init local variables
  //
  PrivateFile     = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot     = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);
  NewPrivateFile  = NULL;

  //
  // Allocate buffer for FileName as the passed in FileName may be read only
  //
  TempFileName = AllocatePool (StrSize (FileName));
  if (TempFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  StrCpy (TempFileName, FileName);
  FileName = TempFileName;

  if (FileName[StrLen (FileName) - 1] == L'\\') {
    FileName[StrLen (FileName) - 1]  = 0;
  }

  //
  // If file name does not equal to "." or ".." and not trailed with "\..",
  // then we trim the leading/trailing blanks and trailing dots
  //
  if (StrCmp (FileName, L".") != 0 && StrCmp (FileName, L"..") != 0 && 
    ((StrLen (FileName) >= 3) ? (StrCmp (&FileName[StrLen (FileName) - 3], L"\\..") != 0) : TRUE)) {
    //
    // Trim leading blanks
    //
    Count = 0;
    for (TempFileName = FileName;
      *TempFileName != 0 && *TempFileName == L' ';
      TempFileName++) {
      Count++;
    }
    CutPrefix (FileName, Count);
    //
    // Trim trailing blanks
    //
    for (TempFileName = FileName + StrLen (FileName) - 1;
      TempFileName >= FileName && (*TempFileName == L' ');
      TempFileName--) {
      ;
    }
    *(TempFileName + 1) = 0;
  }

  //
  // Attempt to open the file
  //
  NewPrivateFile = AllocatePool (sizeof (WIN_NT_EFI_FILE_PRIVATE));
  if (NewPrivateFile == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyMem (NewPrivateFile, PrivateFile, sizeof (WIN_NT_EFI_FILE_PRIVATE));

  NewPrivateFile->FilePath = AllocatePool (StrSize (PrivateFile->FileName));
  if (NewPrivateFile->FilePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  if (PrivateFile->IsDirectoryPath) {
    StrCpy (NewPrivateFile->FilePath, PrivateFile->FileName);
  } else {
    StrCpy (NewPrivateFile->FilePath, PrivateFile->FilePath);
  }

  Size = StrSize (NewPrivateFile->FilePath);
  Size += StrSize (L"\\");
  Size += StrSize (FileName);
  NewPrivateFile->FileName = AllocatePool (Size);
  if (NewPrivateFile->FileName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  if (*FileName == L'\\') {
    StrCpy (NewPrivateFile->FileName, PrivateRoot->FilePath);
    StrCat (NewPrivateFile->FileName, L"\\");
    StrCat (NewPrivateFile->FileName, FileName + 1);
  } else {
    StrCpy (NewPrivateFile->FileName, NewPrivateFile->FilePath);
    if (StrCmp (FileName, L"") != 0) {
      //
      // In case the filename becomes empty, especially after trimming dots and blanks
      //
      StrCat (NewPrivateFile->FileName, L"\\");
      StrCat (NewPrivateFile->FileName, FileName);
    }
  }

  if (!IsFileNameValid (NewPrivateFile->FileName)) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Get rid of . and .., except leading . or ..
  //

  //
  // GuardPointer protect simplefilesystem root path not be destroyed
  //
  GuardPointer  = NewPrivateFile->FileName + StrLen (PrivateRoot->FilePath);

  LoopFinish    = FALSE;

  while (!LoopFinish) {

    LoopFinish = TRUE;

    for (ParseFileName = GuardPointer; *ParseFileName; ParseFileName++) {
      if (*ParseFileName == L'.' &&
          (*(ParseFileName + 1) == 0 || *(ParseFileName + 1) == L'\\') &&
          *(ParseFileName - 1) == L'\\'
          ) {

        //
        // cut \.
        //
        CutPrefix (ParseFileName - 1, 2);
        LoopFinish = FALSE;
        break;
      }

      if (*ParseFileName == L'.' &&
          *(ParseFileName + 1) == L'.' &&
          (*(ParseFileName + 2) == 0 || *(ParseFileName + 2) == L'\\') &&
          *(ParseFileName - 1) == L'\\'
          ) {

        ParseFileName--;
        Count = 3;

        while (ParseFileName != GuardPointer) {
          ParseFileName--;
          Count++;
          if (*ParseFileName == L'\\') {
            break;
          }
        }

        //
        // cut \.. and its left directory
        //
        CutPrefix (ParseFileName, Count);
        LoopFinish = FALSE;
        break;
      }
    }
  }

  RealFileName = NewPrivateFile->FileName;
  while (EfiStrChr (RealFileName, L'\\') != NULL) {
    RealFileName = EfiStrChr (RealFileName, L'\\') + 1;
  }

  TempChar = 0;
  if (RealFileName != NewPrivateFile->FileName) {
    TempChar            = *(RealFileName - 1);
    *(RealFileName - 1) = 0;
    }
  
  FreePool (NewPrivateFile->FilePath);
  NewPrivateFile->FilePath = NULL;
  NewPrivateFile->FilePath = AllocatePool (StrSize (NewPrivateFile->FileName));
  if (NewPrivateFile->FilePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  StrCpy (NewPrivateFile->FilePath, NewPrivateFile->FileName);
  if (TempChar != 0) {
    *(RealFileName - 1)             = TempChar;
  }

  NewPrivateFile->IsRootDirectory = FALSE;

  //
  // Test whether file or directory
  //
  if (OpenMode & EFI_FILE_MODE_CREATE) {
    if (Attributes & EFI_FILE_DIRECTORY) {
      NewPrivateFile->IsDirectoryPath = TRUE;
    } else {
      NewPrivateFile->IsDirectoryPath = FALSE;
    }
  } else {
    NewPrivateFile->LHandle = INVALID_HANDLE_VALUE;
    NewPrivateFile->LHandle = NewPrivateFile->WinNtThunk->CreateFile (
                                                            NewPrivateFile->FileName,
                                                            GENERIC_READ,
                                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                            NULL,
                                                            OPEN_EXISTING,
                                                            0,
                                                            NULL
                                                            );

    if (NewPrivateFile->LHandle != INVALID_HANDLE_VALUE) {
      NewPrivateFile->IsDirectoryPath = FALSE;
      NewPrivateFile->WinNtThunk->CloseHandle (NewPrivateFile->LHandle);
    } else {
      NewPrivateFile->IsDirectoryPath = TRUE;
    }

    NewPrivateFile->LHandle = INVALID_HANDLE_VALUE;
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

    Size  = StrSize (NewPrivateFile->FileName);
    Size += StrSize (L"\\*");
    TempFileName = AllocatePool (Size);
    if (TempFileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpy (TempFileName, NewPrivateFile->FileName);

    if ((OpenMode & EFI_FILE_MODE_CREATE)) {
      //
      // Create a directory
      //
      if (!NewPrivateFile->WinNtThunk->CreateDirectory (TempFileName, NULL)) {

        LastError = PrivateFile->WinNtThunk->GetLastError ();
        if (LastError != ERROR_ALREADY_EXISTS) {
          FreePool (TempFileName);
          Status = EFI_ACCESS_DENIED;
          goto Done;
        }
      }
    }

    NewPrivateFile->DirHandle = NewPrivateFile->WinNtThunk->CreateFile (
                                                              TempFileName,
                                                              NewPrivateFile->IsOpenedByRead ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
                                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                              NULL,
                                                              OPEN_EXISTING,
                                                              FILE_FLAG_BACKUP_SEMANTICS,
                                                              NULL
                                                              );

    if (NewPrivateFile->DirHandle == INVALID_HANDLE_VALUE) {

      NewPrivateFile->DirHandle = NewPrivateFile->WinNtThunk->CreateFile (
                                                                TempFileName,
                                                                GENERIC_READ,
                                                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                                NULL,
                                                                OPEN_EXISTING,
                                                                FILE_FLAG_BACKUP_SEMANTICS,
                                                                NULL
                                                                );

      if (NewPrivateFile->DirHandle != INVALID_HANDLE_VALUE) {
        NewPrivateFile->WinNtThunk->CloseHandle (NewPrivateFile->DirHandle);
        NewPrivateFile->DirHandle = INVALID_HANDLE_VALUE;
        Status                    = EFI_ACCESS_DENIED;
      } else {
        Status = EFI_NOT_FOUND;
      }

      goto Done;
    }

    //
    // Find the first file under it
    //
    StrCat (TempFileName, L"\\*");
    NewPrivateFile->LHandle = NewPrivateFile->WinNtThunk->FindFirstFile (TempFileName, &NewPrivateFile->FindBuf);

    if (NewPrivateFile->LHandle == INVALID_HANDLE_VALUE) {
      NewPrivateFile->IsValidFindBuf = FALSE;
    } else {
      NewPrivateFile->IsValidFindBuf = TRUE;
    }
  } else {
    //
    // deal with file
    //
    if (!NewPrivateFile->IsOpenedByRead) {
      NewPrivateFile->LHandle = NewPrivateFile->WinNtThunk->CreateFile (
                                                              NewPrivateFile->FileName,
                                                              GENERIC_READ | GENERIC_WRITE,
                                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                              NULL,
                                                              (OpenMode & EFI_FILE_MODE_CREATE) ? OPEN_ALWAYS : OPEN_EXISTING,
                                                              0,
                                                              NULL
                                                              );

      if (NewPrivateFile->LHandle == INVALID_HANDLE_VALUE) {
        NewPrivateFile->LHandle = NewPrivateFile->WinNtThunk->CreateFile (
                                                                NewPrivateFile->FileName,
                                                                GENERIC_READ,
                                                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                                NULL,
                                                                OPEN_EXISTING,
                                                                0,
                                                                NULL
                                                                );

        if (NewPrivateFile->LHandle == INVALID_HANDLE_VALUE) {
          Status = EFI_NOT_FOUND;
        } else {
          Status = EFI_ACCESS_DENIED;
          NewPrivateFile->WinNtThunk->CloseHandle (NewPrivateFile->LHandle);
          NewPrivateFile->LHandle = INVALID_HANDLE_VALUE;
        }
      }
    } else {
      NewPrivateFile->LHandle = NewPrivateFile->WinNtThunk->CreateFile (
                                                              NewPrivateFile->FileName,
                                                              GENERIC_READ,
                                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                              NULL,
                                                              OPEN_EXISTING,
                                                              0,
                                                              NULL
                                                              );

      if (NewPrivateFile->LHandle == INVALID_HANDLE_VALUE) {
        Status = EFI_NOT_FOUND;
      }
    }
  }

  if ((OpenMode & EFI_FILE_MODE_CREATE) && Status == EFI_SUCCESS) {
    //
    // Set the attribute
    //
    InfoSize  = 0;
    Info      = NULL;

    Status    = WinNtSimpleFileSystemGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);

    if (Status != EFI_BUFFER_TOO_SMALL) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    Info = AllocatePool (InfoSize);
    if (Info == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    Status = WinNtSimpleFileSystemGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Info->Attribute = Attributes;

    WinNtSimpleFileSystemSetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, InfoSize, Info);
  }

Done:
  FreePool (FileName);

  if (EFI_ERROR (Status)) {
    if (NewPrivateFile) {
      if (NewPrivateFile->FileName) {
        FreePool (NewPrivateFile->FileName);
      }

      if (NewPrivateFile->FilePath) {
        FreePool (NewPrivateFile->FilePath);
      }

      FreePool (NewPrivateFile);
    }
  } else {
    *NewHandle = &NewPrivateFile->EfiFile;
    if (StrCmp (NewPrivateFile->FileName, PrivateRoot->FilePath) == 0) {
     NewPrivateFile->IsRootDirectory = TRUE;
    }   
  }

  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemClose (
  IN EFI_FILE_PROTOCOL  *This
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
  WIN_NT_EFI_FILE_PRIVATE *PrivateFile;
  EFI_TPL                 OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->LHandle != INVALID_HANDLE_VALUE) {
    if (PrivateFile->IsDirectoryPath) {
      PrivateFile->WinNtThunk->FindClose (PrivateFile->LHandle);
    } else {
      PrivateFile->WinNtThunk->CloseHandle (PrivateFile->LHandle);
    }

    PrivateFile->LHandle = INVALID_HANDLE_VALUE;
  }

  if (PrivateFile->IsDirectoryPath && PrivateFile->DirHandle != INVALID_HANDLE_VALUE) {
    PrivateFile->WinNtThunk->CloseHandle (PrivateFile->DirHandle);
    PrivateFile->DirHandle = INVALID_HANDLE_VALUE;
  }

  if (PrivateFile->FileName) {
    FreePool (PrivateFile->FileName);
  }

  FreePool (PrivateFile);

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemDelete (
  IN EFI_FILE_PROTOCOL  *This
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
  WIN_NT_EFI_FILE_PRIVATE *PrivateFile;
  EFI_TPL                 OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status      = EFI_WARN_DELETE_FAILURE;

  if (PrivateFile->IsDirectoryPath) {
    if (PrivateFile->LHandle != INVALID_HANDLE_VALUE) {
      PrivateFile->WinNtThunk->FindClose (PrivateFile->LHandle);
    }

    if (PrivateFile->DirHandle != INVALID_HANDLE_VALUE) {
      PrivateFile->WinNtThunk->CloseHandle (PrivateFile->DirHandle);
      PrivateFile->DirHandle = INVALID_HANDLE_VALUE;
    }

    if (PrivateFile->WinNtThunk->RemoveDirectory (PrivateFile->FileName)) {
      Status = EFI_SUCCESS;
    }
  } else {
    PrivateFile->WinNtThunk->CloseHandle (PrivateFile->LHandle);
    PrivateFile->LHandle = INVALID_HANDLE_VALUE;

    if (!PrivateFile->IsOpenedByRead) {
      if (PrivateFile->WinNtThunk->DeleteFile (PrivateFile->FileName)) {
        Status = EFI_SUCCESS;
      }
    }
  }

  FreePool (PrivateFile->FileName);
  FreePool (PrivateFile);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

VOID
WinNtSystemTimeToEfiTime (
  IN SYSTEMTIME             *SystemTime,
  IN TIME_ZONE_INFORMATION  *TimeZone,
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
  Time->Year        = (UINT16) SystemTime->wYear;
  Time->Month       = (UINT8) SystemTime->wMonth;
  Time->Day         = (UINT8) SystemTime->wDay;
  Time->Hour        = (UINT8) SystemTime->wHour;
  Time->Minute      = (UINT8) SystemTime->wMinute;
  Time->Second      = (UINT8) SystemTime->wSecond;
  Time->Nanosecond  = (UINT32) SystemTime->wMilliseconds * 1000000;
  Time->TimeZone    = (INT16) TimeZone->Bias;

  if (TimeZone->StandardDate.wMonth) {
    Time->Daylight = EFI_TIME_ADJUST_DAYLIGHT;
  }
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemRead (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  OUT    VOID               *Buffer
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
  WIN_NT_EFI_FILE_PRIVATE *PrivateFile;
  EFI_STATUS              Status;
  UINTN                   Size;
  UINTN                   NameSize;
  UINTN                   ResultSize;
  UINTN                   Index;
  SYSTEMTIME              SystemTime;
  EFI_FILE_INFO           *Info;
  WCHAR                   *pw;
  TIME_ZONE_INFORMATION   TimeZone;
  EFI_FILE_INFO           *FileInfo;
  UINT64                  Pos;
  UINT64                  FileSize;
  UINTN                   FileInfoSize;
  EFI_TPL                 OldTpl;

  if (This == NULL || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->LHandle == INVALID_HANDLE_VALUE) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (!PrivateFile->IsDirectoryPath) {

    if (This->GetPosition (This, &Pos) != EFI_SUCCESS) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    FileInfoSize = SIZE_OF_EFI_FILE_SYSTEM_INFO;
    FileInfo = AllocatePool (FileInfoSize);

    Status = This->GetInfo (
                    This,
                    &gEfiFileInfoGuid,
                    &FileInfoSize,
                    FileInfo
                    );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (FileInfo);
      FileInfo = AllocatePool (FileInfoSize);
      Status = This->GetInfo (
                      This,
                      &gEfiFileInfoGuid,
                      &FileInfoSize,
                      FileInfo
                      );
    }

    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    FileSize = FileInfo->FileSize;

    FreePool (FileInfo);

    if (Pos >= FileSize) {
      *BufferSize = 0;
      if (Pos == FileSize) {
        Status = EFI_SUCCESS;
        goto Done;
      } else {
        Status = EFI_DEVICE_ERROR;
        goto Done;
      }
    }

    Status = PrivateFile->WinNtThunk->ReadFile (
                                        PrivateFile->LHandle,
                                        Buffer,
                                        (DWORD)*BufferSize,
                                        (LPDWORD)BufferSize,
                                        NULL
                                        ) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // Read on a directory.  Perform a find next
  //
  if (!PrivateFile->IsValidFindBuf) {
    *BufferSize = 0;
    Status = EFI_SUCCESS;
    goto Done;
  }

  Size        = SIZE_OF_EFI_FILE_INFO;

  NameSize    = StrSize (PrivateFile->FindBuf.cFileName);

  ResultSize  = Size + NameSize;

  Status      = EFI_BUFFER_TOO_SMALL;

  if (*BufferSize >= ResultSize) {
    Status  = EFI_SUCCESS;

    Info    = Buffer;
    ZeroMem (Info, ResultSize);

    Info->Size = ResultSize;

    PrivateFile->WinNtThunk->GetTimeZoneInformation (&TimeZone);

    PrivateFile->WinNtThunk->FileTimeToLocalFileTime (
                              &PrivateFile->FindBuf.ftCreationTime,
                              &PrivateFile->FindBuf.ftCreationTime
                              );

    PrivateFile->WinNtThunk->FileTimeToSystemTime (&PrivateFile->FindBuf.ftCreationTime, &SystemTime);

    WinNtSystemTimeToEfiTime (&SystemTime, &TimeZone, &Info->CreateTime);

    PrivateFile->WinNtThunk->FileTimeToLocalFileTime (
                              &PrivateFile->FindBuf.ftLastWriteTime,
                              &PrivateFile->FindBuf.ftLastWriteTime
                              );

    PrivateFile->WinNtThunk->FileTimeToSystemTime (&PrivateFile->FindBuf.ftLastWriteTime, &SystemTime);

    WinNtSystemTimeToEfiTime (&SystemTime, &TimeZone, &Info->ModificationTime);

    Info->FileSize      = PrivateFile->FindBuf.nFileSizeLow;

    Info->PhysicalSize  = PrivateFile->FindBuf.nFileSizeLow;

    if (PrivateFile->FindBuf.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
      Info->Attribute |= EFI_FILE_ARCHIVE;
    }

    if (PrivateFile->FindBuf.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
      Info->Attribute |= EFI_FILE_HIDDEN;
    }

    if (PrivateFile->FindBuf.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
      Info->Attribute |= EFI_FILE_SYSTEM;
    }

    if (PrivateFile->FindBuf.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
      Info->Attribute |= EFI_FILE_READ_ONLY;
    }

    if (PrivateFile->FindBuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      Info->Attribute |= EFI_FILE_DIRECTORY;
    }

    NameSize  = NameSize / sizeof (WCHAR);

    pw        = (WCHAR *) (((CHAR8 *) Buffer) + Size);

    for (Index = 0; Index < NameSize; Index++) {
      pw[Index] = PrivateFile->FindBuf.cFileName[Index];
    }

    if (PrivateFile->WinNtThunk->FindNextFile (PrivateFile->LHandle, &PrivateFile->FindBuf)) {
      PrivateFile->IsValidFindBuf = TRUE;
    } else {
      PrivateFile->IsValidFindBuf = FALSE;
    }
  }

  *BufferSize = ResultSize;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemWrite (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
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
  WIN_NT_EFI_FILE_PRIVATE *PrivateFile;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;

  if (This == NULL || BufferSize == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->LHandle == INVALID_HANDLE_VALUE) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (PrivateFile->IsDirectoryPath) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (PrivateFile->IsOpenedByRead) {
    Status = EFI_ACCESS_DENIED;
    goto Done;
  }

  Status = PrivateFile->WinNtThunk->WriteFile (
                                      PrivateFile->LHandle,
                                      Buffer,
                                      (DWORD)*BufferSize,
                                      (LPDWORD)BufferSize,
                                      NULL
                                      ) ? EFI_SUCCESS : EFI_DEVICE_ERROR;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;

  //
  // bugbug: need to access windows error reporting
  //
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
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
  WIN_NT_EFI_FILE_PRIVATE *PrivateFile;
  UINT32                  PosLow;
  UINT32                  PosHigh;
  CHAR16                  *FileName;
  EFI_TPL                 OldTpl;
  UINTN                   Size;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->IsDirectoryPath) {
    if (Position != 0) {
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    Size  = StrSize (PrivateFile->FileName);
    Size += StrSize (L"\\*");
    FileName = AllocatePool (Size);
    if (FileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpy (FileName, PrivateFile->FileName);
    StrCat (FileName, L"\\*");

    if (PrivateFile->LHandle != INVALID_HANDLE_VALUE) {
      PrivateFile->WinNtThunk->FindClose (PrivateFile->LHandle);
    }

    PrivateFile->LHandle = PrivateFile->WinNtThunk->FindFirstFile (FileName, &PrivateFile->FindBuf);

    if (PrivateFile->LHandle == INVALID_HANDLE_VALUE) {
      PrivateFile->IsValidFindBuf = FALSE;
    } else {
      PrivateFile->IsValidFindBuf = TRUE;
    }

    FreePool (FileName);

    Status = (PrivateFile->LHandle == INVALID_HANDLE_VALUE) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  } else {
    if (Position == (UINT64) -1) {
      PosLow = PrivateFile->WinNtThunk->SetFilePointer (PrivateFile->LHandle, (ULONG) 0, NULL, FILE_END);
    } else {
      PosHigh = (UINT32) RShiftU64 (Position, 32);

      PosLow  = PrivateFile->WinNtThunk->SetFilePointer (PrivateFile->LHandle, (ULONG) Position, (PLONG)&PosHigh, FILE_BEGIN);
    }

    Status = (PosLow == 0xFFFFFFFF) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemGetPosition (
  IN  EFI_FILE_PROTOCOL   *This,
  OUT UINT64              *Position
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
  EFI_STATUS              Status;
  WIN_NT_EFI_FILE_PRIVATE *PrivateFile;
  INT32                   PositionHigh;
  UINT64                  PosHigh64;
  EFI_TPL                 OldTpl;

  if (This == NULL || Position == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  PrivateFile   = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  PositionHigh  = 0;
  PosHigh64     = 0;

  if (PrivateFile->IsDirectoryPath) {

    Status = EFI_UNSUPPORTED;
    goto Done;

  } else {

    PositionHigh = 0;
    *Position = PrivateFile->WinNtThunk->SetFilePointer (
                                           PrivateFile->LHandle,
                                           0,
                                           (PLONG)&PositionHigh,
                                           FILE_CURRENT
                                           );

    Status = *Position == 0xffffffff ? EFI_DEVICE_ERROR : EFI_SUCCESS;
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    PosHigh64 = PositionHigh;
    *Position += LShiftU64 (PosHigh64, 32);
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
WinNtSimpleFileSystemFileInfo (
  IN     WIN_NT_EFI_FILE_PRIVATE  *PrivateFile,
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
  BY_HANDLE_FILE_INFORMATION  FileInfo;
  SYSTEMTIME                  SystemTime;
  CHAR16                      *RealFileName;
  CHAR16                      *TempPointer;

  Size        = SIZE_OF_EFI_FILE_INFO;
  NameSize    = StrSize (PrivateFile->FileName);
  ResultSize  = Size + NameSize;

  Status      = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status  = EFI_SUCCESS;

    Info    = Buffer;
    ZeroMem (Info, ResultSize);

    Info->Size = ResultSize;
    PrivateFile->WinNtThunk->GetFileInformationByHandle (
                              PrivateFile->IsDirectoryPath ? PrivateFile->DirHandle : PrivateFile->LHandle,
                              &FileInfo
                              );
    Info->FileSize      = FileInfo.nFileSizeLow;
    Info->PhysicalSize  = Info->FileSize;

    PrivateFile->WinNtThunk->FileTimeToLocalFileTime(&FileInfo.ftCreationTime, &FileInfo.ftCreationTime);
    PrivateFile->WinNtThunk->FileTimeToSystemTime (&FileInfo.ftCreationTime, &SystemTime);
    Info->CreateTime.Year   = SystemTime.wYear;
    Info->CreateTime.Month  = (UINT8) SystemTime.wMonth;
    Info->CreateTime.Day    = (UINT8) SystemTime.wDay;
    Info->CreateTime.Hour   = (UINT8) SystemTime.wHour;
    Info->CreateTime.Minute = (UINT8) SystemTime.wMinute;
    Info->CreateTime.Second = (UINT8) SystemTime.wSecond;

    PrivateFile->WinNtThunk->FileTimeToLocalFileTime(&FileInfo.ftLastAccessTime, &FileInfo.ftLastAccessTime);
    PrivateFile->WinNtThunk->FileTimeToSystemTime (&FileInfo.ftLastAccessTime, &SystemTime);
    Info->LastAccessTime.Year   = SystemTime.wYear;
    Info->LastAccessTime.Month  = (UINT8) SystemTime.wMonth;
    Info->LastAccessTime.Day    = (UINT8) SystemTime.wDay;
    Info->LastAccessTime.Hour   = (UINT8) SystemTime.wHour;
    Info->LastAccessTime.Minute = (UINT8) SystemTime.wMinute;
    Info->LastAccessTime.Second = (UINT8) SystemTime.wSecond;

    PrivateFile->WinNtThunk->FileTimeToLocalFileTime(&FileInfo.ftLastWriteTime, &FileInfo.ftLastWriteTime);
    PrivateFile->WinNtThunk->FileTimeToSystemTime (&FileInfo.ftLastWriteTime, &SystemTime);
    Info->ModificationTime.Year   = SystemTime.wYear;
    Info->ModificationTime.Month  = (UINT8) SystemTime.wMonth;
    Info->ModificationTime.Day    = (UINT8) SystemTime.wDay;
    Info->ModificationTime.Hour   = (UINT8) SystemTime.wHour;
    Info->ModificationTime.Minute = (UINT8) SystemTime.wMinute;
    Info->ModificationTime.Second = (UINT8) SystemTime.wSecond;

    if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
      Info->Attribute |= EFI_FILE_ARCHIVE;
    }

    if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
      Info->Attribute |= EFI_FILE_HIDDEN;
    }

    if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
      Info->Attribute |= EFI_FILE_READ_ONLY;
    }

    if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
      Info->Attribute |= EFI_FILE_SYSTEM;
    }

    if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      Info->Attribute |= EFI_FILE_DIRECTORY;
    }

    if (PrivateFile->IsDirectoryPath) {
      Info->Attribute |= EFI_FILE_DIRECTORY;
    }

    RealFileName  = PrivateFile->FileName;
    TempPointer   = RealFileName;

    while (*TempPointer) {
      if (*TempPointer == '\\') {
        RealFileName = TempPointer + 1;
      }

      TempPointer++;
    }

    if (PrivateFile->IsRootDirectory) {
      *((CHAR8 *) Buffer + Size) = 0;
    } else {
      CopyMem ((CHAR8 *) Buffer + Size, RealFileName, NameSize);
    }
  }

  *BufferSize = ResultSize;
  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemGetInfo (
  IN     EFI_FILE_PROTOCOL  *This,
  IN     EFI_GUID           *InformationType,
  IN OUT UINTN              *BufferSize,
  OUT    VOID               *Buffer
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
  WIN_NT_EFI_FILE_PRIVATE           *PrivateFile;
  EFI_FILE_SYSTEM_INFO              *FileSystemInfoBuffer;
  UINT32                            SectorsPerCluster;
  UINT32                            BytesPerSector;
  UINT32                            FreeClusters;
  UINT32                            TotalClusters;
  UINT32                            BytesPerCluster;
  CHAR16                            *DriveName;
  BOOLEAN                           DriveNameFound;
  BOOL                              NtStatus;
  UINTN                             Index;
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE *PrivateRoot;
  EFI_TPL                           OldTpl;

  if (This == NULL || InformationType == NULL || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);

  Status      = EFI_UNSUPPORTED;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = WinNtSimpleFileSystemFileInfo (PrivateFile, BufferSize, Buffer);
  }

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    if (*BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel)) {
      *BufferSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
      Status = EFI_BUFFER_TOO_SMALL;
      goto Done;
    }

    FileSystemInfoBuffer            = (EFI_FILE_SYSTEM_INFO *) Buffer;
    FileSystemInfoBuffer->Size      = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
    FileSystemInfoBuffer->ReadOnly  = FALSE;

    //
    // Try to get the drive name
    //
    DriveNameFound  = FALSE;
    DriveName = AllocatePool (StrSize (PrivateFile->FilePath) + 1);
    if (DriveName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpy (DriveName, PrivateFile->FilePath);
    for (Index = 0; DriveName[Index] != 0 && DriveName[Index] != ':'; Index++) {
      ;
    }

    if (DriveName[Index] == ':') {
      DriveName[Index + 1]  = '\\';
      DriveName[Index + 2]  = 0;
      DriveNameFound        = TRUE;
    } else if (DriveName[0] == '\\' && DriveName[1] == '\\') {
      for (Index = 2; DriveName[Index] != 0 && DriveName[Index] != '\\'; Index++) {
        ;
      }

      if (DriveName[Index] == '\\') {
        DriveNameFound = TRUE;
        for (Index++; DriveName[Index] != 0 && DriveName[Index] != '\\'; Index++) {
          ;
        }

        DriveName[Index]      = '\\';
        DriveName[Index + 1]  = 0;
      }
    }

    //
    // Try GetDiskFreeSpace first
    //
    NtStatus = PrivateFile->WinNtThunk->GetDiskFreeSpace (
                                          DriveNameFound ? DriveName : NULL,
                                          (LPDWORD)&SectorsPerCluster,
                                          (LPDWORD)&BytesPerSector,
                                          (LPDWORD)&FreeClusters,
                                          (LPDWORD)&TotalClusters
                                          );
    if (DriveName) {
      FreePool (DriveName);
    }

    if (NtStatus) {
      //
      // Succeeded
      //
      BytesPerCluster                   = BytesPerSector * SectorsPerCluster;
      FileSystemInfoBuffer->VolumeSize  = MultU64x32 (TotalClusters, BytesPerCluster);
      FileSystemInfoBuffer->FreeSpace   = MultU64x32 (FreeClusters, BytesPerCluster);
      FileSystemInfoBuffer->BlockSize   = BytesPerCluster;

    } else {
      //
      // try GetDiskFreeSpaceEx then
      //
      FileSystemInfoBuffer->BlockSize = 0;
      NtStatus = PrivateFile->WinNtThunk->GetDiskFreeSpaceEx (
                                            PrivateFile->FilePath,
                                            (PULARGE_INTEGER) (&FileSystemInfoBuffer->FreeSpace),
                                            (PULARGE_INTEGER) (&FileSystemInfoBuffer->VolumeSize),
                                            NULL
                                            );
      if (!NtStatus) {
        Status = EFI_DEVICE_ERROR;
        goto Done;
      }
    }

    StrCpy ((CHAR16 *) FileSystemInfoBuffer->VolumeLabel, PrivateRoot->VolumeLabel);
    *BufferSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
    Status      = EFI_SUCCESS;
  }

  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
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
WinNtSimpleFileSystemSetInfo (
  IN EFI_FILE_PROTOCOL*This,
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
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE *PrivateRoot;
  WIN_NT_EFI_FILE_PRIVATE           *PrivateFile;
  EFI_FILE_INFO                     *OldFileInfo;
  EFI_FILE_INFO                     *NewFileInfo;
  EFI_STATUS                        Status;
  UINTN                             OldInfoSize;
  INTN                              NtStatus;
  UINT32                            NewAttr;
  UINT32                            OldAttr;
  CHAR16                            *OldFileName;
  CHAR16                            *NewFileName;
  CHAR16                            *TempFileName;
  CHAR16                            *CharPointer;
  BOOLEAN                           AttrChangeFlag;
  BOOLEAN                           NameChangeFlag;
  BOOLEAN                           SizeChangeFlag;
  BOOLEAN                           TimeChangeFlag;
  UINT64                            CurPos;
  SYSTEMTIME                        NewCreationSystemTime;
  SYSTEMTIME                        NewLastAccessSystemTime;
  SYSTEMTIME                        NewLastWriteSystemTime;
  FILETIME                          NewCreationFileTime;
  FILETIME                          NewLastAccessFileTime;
  FILETIME                          NewLastWriteFileTime;
  WIN32_FIND_DATA                   FindBuf;
  EFI_FILE_SYSTEM_INFO              *NewFileSystemInfo;
  EFI_TPL                           OldTpl;
  UINTN                             Size;

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
  PrivateFile               = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot               = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);

  Status                    = EFI_UNSUPPORTED;
  OldFileInfo               = NewFileInfo = NULL;
  OldFileName               = NewFileName = NULL;
  AttrChangeFlag = NameChangeFlag = SizeChangeFlag = TimeChangeFlag = FALSE;

  //
  // Set file system information.
  //
  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    NewFileSystemInfo = (EFI_FILE_SYSTEM_INFO *) Buffer;
    if (BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (NewFileSystemInfo->VolumeLabel)) {
      Status = EFI_BAD_BUFFER_SIZE;
      goto Done;
    }


    FreePool (PrivateRoot->VolumeLabel);
    PrivateRoot->VolumeLabel = AllocatePool (StrSize (NewFileSystemInfo->VolumeLabel));
    if (PrivateRoot->VolumeLabel == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
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

  if ((NewFileInfo->Size <= SIZE_OF_EFI_FILE_INFO) ||
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
  Status      = WinNtSimpleFileSystemFileInfo (PrivateFile, &OldInfoSize, NULL);

  if (Status != EFI_BUFFER_TOO_SMALL) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  OldFileInfo = AllocatePool (OldInfoSize);
  if (OldFileInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = WinNtSimpleFileSystemFileInfo (PrivateFile, &OldInfoSize, OldFileInfo);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  OldFileName = AllocatePool (StrSize (PrivateFile->FileName));
  if (OldFileName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  StrCpy (OldFileName, PrivateFile->FileName);

  //
  // Make full pathname from new filename and rootpath.
  //
  if (NewFileInfo->FileName[0] == '\\') {
    Size  = StrSize (PrivateRoot->FilePath);
    Size += StrSize (L"\\");
    Size += StrSize (NewFileInfo->FileName);
    NewFileName = AllocatePool (Size);
    if (NewFileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpy (NewFileName, PrivateRoot->FilePath);
    StrCat (NewFileName, L"\\");
    StrCat (NewFileName, NewFileInfo->FileName + 1);
  } else {
    Size  = StrSize (PrivateFile->FilePath);
    Size += StrSize (L"\\");
    Size += StrSize (NewFileInfo->FileName);
    NewFileName = AllocatePool (Size);
    if (NewFileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpy (NewFileName, PrivateFile->FilePath);
    StrCat (NewFileName, L"\\");
    StrCat (NewFileName, NewFileInfo->FileName);
  }

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
  OldAttr = PrivateFile->WinNtThunk->GetFileAttributes (OldFileName);

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

    if (PrivateFile->LHandle != INVALID_HANDLE_VALUE) {
      if (PrivateFile->IsDirectoryPath) {
        PrivateFile->WinNtThunk->FindClose (PrivateFile->LHandle);
      } else {
        PrivateFile->WinNtThunk->CloseHandle (PrivateFile->LHandle);
        PrivateFile->LHandle = INVALID_HANDLE_VALUE;
      }
    }

    if (PrivateFile->IsDirectoryPath && PrivateFile->DirHandle != INVALID_HANDLE_VALUE) {
      PrivateFile->WinNtThunk->CloseHandle (PrivateFile->DirHandle);
      PrivateFile->DirHandle = INVALID_HANDLE_VALUE;
    }

    NtStatus = PrivateFile->WinNtThunk->MoveFile (OldFileName, NewFileName);

    if (NtStatus) {
      //
      // modify file name
      //
      FreePool (PrivateFile->FileName);

      PrivateFile->FileName = AllocatePool (StrSize (NewFileName));
      if (PrivateFile->FileName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      StrCpy (PrivateFile->FileName, NewFileName);

      Size  =  StrSize (NewFileName);
      Size += StrSize (L"\\*");
      TempFileName = AllocatePool (Size);

      StrCpy (TempFileName, NewFileName);

      if (!PrivateFile->IsDirectoryPath) {
       PrivateFile->LHandle = PrivateFile->WinNtThunk->CreateFile (
                                                          TempFileName,
                                                          PrivateFile->IsOpenedByRead ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                          NULL,
                                                          OPEN_EXISTING,
                                                          0,
                                                          NULL
                                                          );

        FreePool (TempFileName);

        //
        //  Flush buffers just in case
        //
        if (PrivateFile->WinNtThunk->FlushFileBuffers (PrivateFile->LHandle) == 0) {
          Status = EFI_DEVICE_ERROR;
          goto Done;
        }
      } else {
        PrivateFile->DirHandle = PrivateFile->WinNtThunk->CreateFile (
                                                            TempFileName,
                                                            PrivateFile->IsOpenedByRead ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                            NULL,
                                                            OPEN_EXISTING,
                                                            FILE_FLAG_BACKUP_SEMANTICS,
                                                            NULL
                                                            );

        StrCat (TempFileName, L"\\*");
        PrivateFile->LHandle = PrivateFile->WinNtThunk->FindFirstFile (TempFileName, &FindBuf);

        FreePool (TempFileName);
      }
    } else {
      Status    = EFI_ACCESS_DENIED;
Reopen: ;

      NtStatus  = PrivateFile->WinNtThunk->SetFileAttributes (OldFileName, OldAttr);

      if (!NtStatus) {
        goto Done;
      }

      Size =  StrSize (OldFileName);
      Size += StrSize (L"\\*");
      TempFileName = AllocatePool (Size);

      StrCpy (TempFileName, OldFileName);

      if (!PrivateFile->IsDirectoryPath) {
        PrivateFile->LHandle = PrivateFile->WinNtThunk->CreateFile (
                                                          TempFileName,
                                                          PrivateFile->IsOpenedByRead ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                          NULL,
                                                          OPEN_EXISTING,
                                                          0,
                                                          NULL
                                                          );
      } else {
        PrivateFile->DirHandle = PrivateFile->WinNtThunk->CreateFile (
                                                            TempFileName,
                                                            PrivateFile->IsOpenedByRead ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                            NULL,
                                                            OPEN_EXISTING,
                                                            FILE_FLAG_BACKUP_SEMANTICS,
                                                            NULL
                                                            );

        StrCat (TempFileName, L"\\*");
        PrivateFile->LHandle = PrivateFile->WinNtThunk->FindFirstFile (TempFileName, &FindBuf);
      }

      FreePool (TempFileName);

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

    Status = This->GetPosition (This, &CurPos);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Status = This->SetPosition (This, NewFileInfo->FileSize);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (PrivateFile->WinNtThunk->SetEndOfFile (PrivateFile->LHandle) == 0) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    Status = This->SetPosition (This, CurPos);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  //
  // Time change
  //
  if (TimeChangeFlag) {

    NewCreationSystemTime.wYear         = NewFileInfo->CreateTime.Year;
    NewCreationSystemTime.wMonth        = NewFileInfo->CreateTime.Month;
    NewCreationSystemTime.wDay          = NewFileInfo->CreateTime.Day;
    NewCreationSystemTime.wHour         = NewFileInfo->CreateTime.Hour;
    NewCreationSystemTime.wMinute       = NewFileInfo->CreateTime.Minute;
    NewCreationSystemTime.wSecond       = NewFileInfo->CreateTime.Second;
    NewCreationSystemTime.wMilliseconds = 0;

    if (!PrivateFile->WinNtThunk->SystemTimeToFileTime (
                                    &NewCreationSystemTime,
                                    &NewCreationFileTime
                                    )) {
      goto Done;
    }

    if (!PrivateFile->WinNtThunk->LocalFileTimeToFileTime (
                                    &NewCreationFileTime,
                                    &NewCreationFileTime
                                    )) {
      goto Done;
    }

    NewLastAccessSystemTime.wYear         = NewFileInfo->LastAccessTime.Year;
    NewLastAccessSystemTime.wMonth        = NewFileInfo->LastAccessTime.Month;
    NewLastAccessSystemTime.wDay          = NewFileInfo->LastAccessTime.Day;
    NewLastAccessSystemTime.wHour         = NewFileInfo->LastAccessTime.Hour;
    NewLastAccessSystemTime.wMinute       = NewFileInfo->LastAccessTime.Minute;
    NewLastAccessSystemTime.wSecond       = NewFileInfo->LastAccessTime.Second;
    NewLastAccessSystemTime.wMilliseconds = 0;

    if (!PrivateFile->WinNtThunk->SystemTimeToFileTime (
                                    &NewLastAccessSystemTime,
                                    &NewLastAccessFileTime
                                    )) {
      goto Done;
    }

    if (!PrivateFile->WinNtThunk->LocalFileTimeToFileTime (
                                    &NewLastAccessFileTime,
                                    &NewLastAccessFileTime
                                    )) {
      goto Done;
    }

    NewLastWriteSystemTime.wYear          = NewFileInfo->ModificationTime.Year;
    NewLastWriteSystemTime.wMonth         = NewFileInfo->ModificationTime.Month;
    NewLastWriteSystemTime.wDay           = NewFileInfo->ModificationTime.Day;
    NewLastWriteSystemTime.wHour          = NewFileInfo->ModificationTime.Hour;
    NewLastWriteSystemTime.wMinute        = NewFileInfo->ModificationTime.Minute;
    NewLastWriteSystemTime.wSecond        = NewFileInfo->ModificationTime.Second;
    NewLastWriteSystemTime.wMilliseconds  = 0;

    if (!PrivateFile->WinNtThunk->SystemTimeToFileTime (
                                    &NewLastWriteSystemTime,
                                    &NewLastWriteFileTime
                                    )) {
      goto Done;
    }

    if (!PrivateFile->WinNtThunk->LocalFileTimeToFileTime (
                                    &NewLastWriteFileTime,
                                    &NewLastWriteFileTime
                                    )) {
      goto Done;
    }

    if (!PrivateFile->WinNtThunk->SetFileTime (
                                    PrivateFile->IsDirectoryPath ? PrivateFile->DirHandle : PrivateFile->LHandle,
                                    &NewCreationFileTime,
                                    &NewLastAccessFileTime,
                                    &NewLastWriteFileTime
                                    )) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

  }

  //
  // No matter about AttrChangeFlag, Attribute must be set.
  // Because operation before may cause attribute change.
  //
  NewAttr = OldAttr;

  if (NewFileInfo->Attribute & EFI_FILE_ARCHIVE) {
    NewAttr |= FILE_ATTRIBUTE_ARCHIVE;
  } else {
    NewAttr &= ~FILE_ATTRIBUTE_ARCHIVE;
  }

  if (NewFileInfo->Attribute & EFI_FILE_HIDDEN) {
    NewAttr |= FILE_ATTRIBUTE_HIDDEN;
  } else {
    NewAttr &= ~FILE_ATTRIBUTE_HIDDEN;
  }

  if (NewFileInfo->Attribute & EFI_FILE_SYSTEM) {
    NewAttr |= FILE_ATTRIBUTE_SYSTEM;
  } else {
    NewAttr &= ~FILE_ATTRIBUTE_SYSTEM;
  }

  if (NewFileInfo->Attribute & EFI_FILE_READ_ONLY) {
    NewAttr |= FILE_ATTRIBUTE_READONLY;
  } else {
    NewAttr &= ~FILE_ATTRIBUTE_READONLY;
  }

  NtStatus = PrivateFile->WinNtThunk->SetFileAttributes (NewFileName, NewAttr);

  if (!NtStatus) {
    Status    = EFI_DEVICE_ERROR;
    goto Reopen;
  }

Done:
  if (OldFileInfo != NULL) {
    FreePool (OldFileInfo);
  }

  if (OldFileName != NULL) {
    FreePool (OldFileName);
  }

  if (NewFileName != NULL) {
    FreePool (NewFileName);
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
WinNtSimpleFileSystemFlush (
  IN EFI_FILE_PROTOCOL  *This
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
  BY_HANDLE_FILE_INFORMATION  FileInfo;
  WIN_NT_EFI_FILE_PRIVATE     *PrivateFile;
  EFI_STATUS                  Status;
  EFI_TPL                     OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->LHandle == INVALID_HANDLE_VALUE) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (PrivateFile->IsDirectoryPath) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  if (PrivateFile->IsOpenedByRead) {
    Status = EFI_ACCESS_DENIED;
    goto Done;
  }

  PrivateFile->WinNtThunk->GetFileInformationByHandle (PrivateFile->LHandle, &FileInfo);

  if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
    Status = EFI_ACCESS_DENIED;
    goto Done;
  }

  Status = PrivateFile->WinNtThunk->FlushFileBuffers (PrivateFile->LHandle) ? EFI_SUCCESS : EFI_DEVICE_ERROR;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
  //
  // bugbug: - Use Windows error reporting.
  //
}



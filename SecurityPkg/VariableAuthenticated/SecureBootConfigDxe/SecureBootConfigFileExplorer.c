/** @file
  Internal file explorer functions for SecureBoot configuration module.

Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecureBootConfigImpl.h"

///
/// File system selection menu
///
SECUREBOOT_MENU_OPTION      FsOptionMenu = {
  SECUREBOOT_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Files and sub-directories in current directory menu
///
SECUREBOOT_MENU_OPTION      DirectoryMenu = {
  SECUREBOOT_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

VOID                  *mStartOpCodeHandle = NULL;
VOID                  *mEndOpCodeHandle = NULL;
EFI_IFR_GUID_LABEL    *mStartLabel = NULL;
EFI_IFR_GUID_LABEL    *mEndLabel = NULL;

/**
  Duplicate a string.

  @param[in]    Src             The source string.

  @return      A new string which is duplicated copy of the source,
               or NULL if there is not enough memory.

**/
CHAR16 *
StrDuplicate (
  IN CHAR16   *Src
  )
{
  CHAR16  *Dest;
  UINTN   Size;

  Size  = StrSize (Src);
  Dest  = AllocateZeroPool (Size);
  ASSERT (Dest != NULL);
  if (Dest != NULL) {
    CopyMem (Dest, Src, Size);
  }

  return Dest;
}

/**
  Helper function called as part of the code needed to allocate
  the proper sized buffer for various EFI interfaces.

  @param[in, out]   Status          Current status
  @param[in, out]   Buffer          Current allocated buffer, or NULL
  @param[in]        BufferSize      Current buffer size needed

  @retval  TRUE         If the buffer was reallocated and the caller
                        should try the API again.
  @retval  FALSE        The caller should not call this function again.

**/
BOOLEAN
GrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  )
{
  BOOLEAN TryAgain;

  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if ((*Buffer == NULL) && (BufferSize != 0)) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }
  //
  // If the status code is "buffer too small", resize the buffer
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {

    if (*Buffer != NULL) {
      FreePool (*Buffer);
    }

    *Buffer = AllocateZeroPool (BufferSize);

    if (*Buffer != NULL) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR (*Status) && (*Buffer != NULL)) {
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}

/**
  Append file name to existing file name, and allocate a new buffer
  to hold the appended result.

  @param[in]  Str1  The existing file name
  @param[in]  Str2  The file name to be appended

  @return      A new string with appended result.

**/
CHAR16 *
AppendFileName (
  IN  CHAR16  *Str1,
  IN  CHAR16  *Str2
  )
{
  UINTN   Size1;
  UINTN   Size2;
  CHAR16  *Str;
  CHAR16  *TmpStr;
  CHAR16  *Ptr;
  CHAR16  *LastSlash;

  Size1 = StrSize (Str1);
  Size2 = StrSize (Str2);
  Str   = AllocateZeroPool (Size1 + Size2 + sizeof (CHAR16));
  ASSERT (Str != NULL);

  TmpStr = AllocateZeroPool (Size1 + Size2 + sizeof (CHAR16));
  ASSERT (TmpStr != NULL);

  StrCat (Str, Str1);
  if (!((*Str == '\\') && (*(Str + 1) == 0))) {
    StrCat (Str, L"\\");
  }

  StrCat (Str, Str2);

  Ptr       = Str;
  LastSlash = Str;
  while (*Ptr != 0) {
    if (*Ptr == '\\' && *(Ptr + 1) == '.' && *(Ptr + 2) == '.' && *(Ptr + 3) == L'\\') {
      //
      // Convert "\Name\..\" to "\"
      // DO NOT convert the .. if it is at the end of the string. This will
      // break the .. behavior in changing directories.
      //

      //
      // Use TmpStr as a backup, as StrCpy in BaseLib does not handle copy of two strings
      // that overlap.
      //
      StrCpy (TmpStr, Ptr + 3);
      StrCpy (LastSlash, TmpStr);
      Ptr = LastSlash;
    } else if (*Ptr == '\\' && *(Ptr + 1) == '.' && *(Ptr + 2) == '\\') {
      //
      // Convert a "\.\" to a "\"
      //

      //
      // Use TmpStr as a backup, as StrCpy in BaseLib does not handle copy of two strings
      // that overlap.
      //
      StrCpy (TmpStr, Ptr + 2);
      StrCpy (Ptr, TmpStr);
      Ptr = LastSlash;
    } else if (*Ptr == '\\') {
      LastSlash = Ptr;
    }

    Ptr++;
  }

  FreePool (TmpStr);

  return Str;
}

/**
  Create a SECUREBOOT_MENU_ENTRY, and stores it in a buffer allocated from the pool.

  @return           The new menu entry or NULL of error happens.

**/
SECUREBOOT_MENU_ENTRY *
CreateMenuEntry (
  VOID
  )
{
  SECUREBOOT_MENU_ENTRY *MenuEntry;
  UINTN                 ContextSize;

  //
  // Create new menu entry
  //
  MenuEntry = AllocateZeroPool (sizeof (SECUREBOOT_MENU_ENTRY));
  if (MenuEntry == NULL) {
    return NULL;
  }

  ContextSize = sizeof (SECUREBOOT_FILE_CONTEXT);
  MenuEntry->FileContext = AllocateZeroPool (ContextSize);
  if (MenuEntry->FileContext == NULL) {
    FreePool (MenuEntry);
    return NULL;
  }

  MenuEntry->Signature = SECUREBOOT_MENU_ENTRY_SIGNATURE;

  return MenuEntry;
}

/**
  Get Menu Entry from the Menu Entry List by MenuNumber.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param[in]  MenuOption      The Menu Entry List to read the menu entry.
  @param[in]  MenuNumber      The index of Menu Entry.

  @return     The Menu Entry.

**/
SECUREBOOT_MENU_ENTRY *
GetMenuEntry (
  IN  SECUREBOOT_MENU_OPTION      *MenuOption,
  IN  UINTN                       MenuNumber
  )
{
  SECUREBOOT_MENU_ENTRY       *NewMenuEntry;
  UINTN                       Index;
  LIST_ENTRY                  *List;

  ASSERT (MenuNumber < MenuOption->MenuNumber);

  List = MenuOption->Head.ForwardLink;
  for (Index = 0; Index < MenuNumber; Index++) {
    List = List->ForwardLink;
  }

  NewMenuEntry = CR (List, SECUREBOOT_MENU_ENTRY, Link, SECUREBOOT_MENU_ENTRY_SIGNATURE);

  return NewMenuEntry;
}

/**
  Create string tokens for a menu from its help strings and display strings.

  @param[in] HiiHandle          Hii Handle of the package to be updated.
  @param[in] MenuOption         The Menu whose string tokens need to be created.

**/
VOID
CreateMenuStringToken (
  IN EFI_HII_HANDLE                          HiiHandle,
  IN SECUREBOOT_MENU_OPTION                  *MenuOption
  )
{
  SECUREBOOT_MENU_ENTRY *NewMenuEntry;
  UINTN         Index;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry = GetMenuEntry (MenuOption, Index);

    NewMenuEntry->DisplayStringToken = HiiSetString (
                                         HiiHandle,
                                         0,
                                         NewMenuEntry->DisplayString,
                                         NULL
                                         );

    if (NewMenuEntry->HelpString == NULL) {
      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;
    } else {
      NewMenuEntry->HelpStringToken = HiiSetString (
                                        HiiHandle,
                                        0,
                                        NewMenuEntry->HelpString,
                                        NULL
                                        );
    }
  }
}

/**
  Free up all resources allocated for a SECUREBOOT_MENU_ENTRY.

  @param[in, out]  MenuEntry   A pointer to SECUREBOOT_MENU_ENTRY.

**/
VOID
DestroyMenuEntry (
  IN OUT SECUREBOOT_MENU_ENTRY         *MenuEntry
  )
{
  SECUREBOOT_FILE_CONTEXT           *FileContext;


  FileContext = (SECUREBOOT_FILE_CONTEXT *) MenuEntry->FileContext;

  if (!FileContext->IsRoot && FileContext->DevicePath != NULL) {
    FreePool (FileContext->DevicePath);
  } else {
    if (FileContext->FHandle != NULL) {
      FileContext->FHandle->Close (FileContext->FHandle);
    }
  }

  if (FileContext->FileName != NULL) {
    FreePool (FileContext->FileName);
  }
  if (FileContext->Info != NULL) {
    FreePool (FileContext->Info);
  }

  FreePool (FileContext);

  if (MenuEntry->DisplayString != NULL) {
    FreePool (MenuEntry->DisplayString);
  }
  if (MenuEntry->HelpString != NULL) {
    FreePool (MenuEntry->HelpString);
  }

  FreePool (MenuEntry);
}

/**
  Free resources allocated in Allocate Rountine.

  @param[in, out]  MenuOption        Menu to be freed

**/
VOID
FreeMenu (
  IN OUT SECUREBOOT_MENU_OPTION        *MenuOption
  )
{
  SECUREBOOT_MENU_ENTRY      *MenuEntry;
  while (!IsListEmpty (&MenuOption->Head)) {
    MenuEntry = CR (
                  MenuOption->Head.ForwardLink,
                  SECUREBOOT_MENU_ENTRY,
                  Link,
                  SECUREBOOT_MENU_ENTRY_SIGNATURE
                  );
    RemoveEntryList (&MenuEntry->Link);
    DestroyMenuEntry (MenuEntry);
  }
  MenuOption->MenuNumber = 0;
}

/**
  This function gets the file information from an open file descriptor, and stores it
  in a buffer allocated from pool.

  @param[in]  FHand           File Handle.

  @return    A pointer to a buffer with file information or NULL is returned

**/
EFI_FILE_INFO *
FileInfo (
  IN EFI_FILE_HANDLE      FHand
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *Buffer;
  UINTN         BufferSize;

  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = SIZE_OF_EFI_FILE_INFO + 200;

  //
  // Call the real function
  //
  while (GrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
    Status = FHand->GetInfo (
                      FHand,
                      &gEfiFileInfoGuid,
                      &BufferSize,
                      Buffer
                      );
  }

  return Buffer;
}

/**
  This function gets the file system information from an open file descriptor,
  and stores it in a buffer allocated from pool.

  @param[in] FHand       The file handle.

  @return                A pointer to a buffer with file information.
  @retval                NULL is returned if failed to get Vaolume Label Info.

**/
EFI_FILE_SYSTEM_VOLUME_LABEL *
FileSystemVolumeLabelInfo (
  IN EFI_FILE_HANDLE      FHand
  )
{
  EFI_STATUS                        Status;
  EFI_FILE_SYSTEM_VOLUME_LABEL      *Buffer;
  UINTN                             BufferSize;
  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL + 200;

  //
  // Call the real function
  //
  while (GrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
    Status = FHand->GetInfo (
                      FHand,
                      &gEfiFileSystemVolumeLabelInfoIdGuid,
                      &BufferSize,
                      Buffer
                      );
  }

  return Buffer;
}

/**
  This function will open a file or directory referenced by DevicePath.

  This function opens a file with the open mode according to the file path. The
  Attributes is valid only for EFI_FILE_MODE_CREATE.

  @param[in, out]  FilePath        On input, the device path to the file.
                                   On output, the remaining device path.
  @param[out]      FileHandle      Pointer to the file handle.
  @param[in]       OpenMode        The mode to open the file with.
  @param[in]       Attributes      The file's file attributes.

  @retval EFI_SUCCESS              The information was set.
  @retval EFI_INVALID_PARAMETER    One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED          Could not open the file path.
  @retval EFI_NOT_FOUND            The specified file could not be found on the
                                   device or the file system could not be found on
                                   the device.
  @retval EFI_NO_MEDIA             The device has no medium.
  @retval EFI_MEDIA_CHANGED        The device has a different medium in it or the
                                   medium is no longer supported.
  @retval EFI_DEVICE_ERROR         The device reported an error.
  @retval EFI_VOLUME_CORRUPTED     The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED      The file or medium is write protected.
  @retval EFI_ACCESS_DENIED        The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES     Not enough resources were available to open the
                                   file.
  @retval EFI_VOLUME_FULL          The volume is full.
**/
EFI_STATUS
EFIAPI
OpenFileByDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **FilePath,
  OUT EFI_FILE_HANDLE                 *FileHandle,
  IN UINT64                           OpenMode,
  IN UINT64                           Attributes
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *EfiSimpleFileSystemProtocol;
  EFI_FILE_PROTOCOL               *Handle1;
  EFI_FILE_PROTOCOL               *Handle2;
  EFI_HANDLE                      DeviceHandle;

  if ((FilePath == NULL || FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  FilePath,
                  &DeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol(
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID**)&EfiSimpleFileSystemProtocol,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiSimpleFileSystemProtocol->OpenVolume(EfiSimpleFileSystemProtocol, &Handle1);
  if (EFI_ERROR (Status)) {
    FileHandle = NULL;
    return Status;
  }

  //
  // go down directories one node at a time.
  //
  while (!IsDevicePathEnd (*FilePath)) {
    //
    // For file system access each node should be a file path component
    //
    if (DevicePathType    (*FilePath) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (*FilePath) != MEDIA_FILEPATH_DP
       ) {
      FileHandle = NULL;
      return (EFI_INVALID_PARAMETER);
    }
    //
    // Open this file path node
    //
    Handle2  = Handle1;
    Handle1 = NULL;

    //
    // Try to test opening an existing file
    //
    Status = Handle2->Open (
                          Handle2,
                          &Handle1,
                          ((FILEPATH_DEVICE_PATH*)*FilePath)->PathName,
                          OpenMode &~EFI_FILE_MODE_CREATE,
                          0
                         );

    //
    // see if the error was that it needs to be created
    //
    if ((EFI_ERROR (Status)) && (OpenMode != (OpenMode &~EFI_FILE_MODE_CREATE))) {
      Status = Handle2->Open (
                            Handle2,
                            &Handle1,
                            ((FILEPATH_DEVICE_PATH*)*FilePath)->PathName,
                            OpenMode,
                            Attributes
                           );
    }
    //
    // Close the last node
    //
    Handle2->Close (Handle2);

    if (EFI_ERROR(Status)) {
      return (Status);
    }

    //
    // Get the next node
    //
    *FilePath = NextDevicePathNode (*FilePath);
  }

  //
  // This is a weak spot since if the undefined SHELL_FILE_HANDLE format changes this must change also!
  //
  *FileHandle = (VOID*)Handle1;
  return EFI_SUCCESS;
}

/**
  Function opens and returns a file handle to the root directory of a volume.

  @param[in]   DeviceHandle    A handle for a device

  @return      A valid file handle or NULL if error happens.

**/
EFI_FILE_HANDLE
OpenRoot (
  IN EFI_HANDLE                   DeviceHandle
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  EFI_FILE_HANDLE                 File;

  File = NULL;

  //
  // File the file system interface to the device
  //
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID *) &Volume
                  );

  //
  // Open the root directory of the volume
  //
  if (!EFI_ERROR (Status)) {
    Status = Volume->OpenVolume (
                      Volume,
                      &File
                      );
  }
  //
  // Done
  //
  return EFI_ERROR (Status) ? NULL : File;
}

/**
  This function builds the FsOptionMenu list which records all
  available file system in the system. They include all instances
  of EFI_SIMPLE_FILE_SYSTEM_PROTOCOL, all instances of EFI_LOAD_FILE_SYSTEM
  and all type of legacy boot device.

  @retval  EFI_SUCCESS             Success find the file system
  @retval  EFI_OUT_OF_RESOURCES    Can not create menu entry

**/
EFI_STATUS
FindFileSystem (
  VOID
  )
{
  UINTN                     NoBlkIoHandles;
  UINTN                     NoSimpleFsHandles;
  EFI_HANDLE                *BlkIoHandle;
  EFI_HANDLE                *SimpleFsHandle;
  UINT16                    *VolumeLabel;
  EFI_BLOCK_IO_PROTOCOL     *BlkIo;
  UINTN                     Index;
  EFI_STATUS                Status;
  SECUREBOOT_MENU_ENTRY     *MenuEntry;
  SECUREBOOT_FILE_CONTEXT   *FileContext;
  UINT16                    *TempStr;
  UINTN                     OptionNumber;
  VOID                      *Buffer;

  BOOLEAN                   RemovableMedia;


  NoSimpleFsHandles = 0;
  OptionNumber      = 0;
  InitializeListHead (&FsOptionMenu.Head);

  //
  // Locate Handles that support BlockIo protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &NoBlkIoHandles,
                  &BlkIoHandle
                  );
  if (!EFI_ERROR (Status)) {

    for (Index = 0; Index < NoBlkIoHandles; Index++) {
      Status = gBS->HandleProtocol (
                      BlkIoHandle[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **) &BlkIo
                      );

      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Issue a dummy read to trigger reinstall of BlockIo protocol for removable media
      //
      if (BlkIo->Media->RemovableMedia) {
        Buffer = AllocateZeroPool (BlkIo->Media->BlockSize);
        if (NULL == Buffer) {
          FreePool (BlkIoHandle);
          return EFI_OUT_OF_RESOURCES;
        }

        BlkIo->ReadBlocks (
                BlkIo,
                BlkIo->Media->MediaId,
                0,
                BlkIo->Media->BlockSize,
                Buffer
                );
        FreePool (Buffer);
      }
    }
    FreePool (BlkIoHandle);
  }

  //
  // Locate Handles that support Simple File System protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NoSimpleFsHandles,
                  &SimpleFsHandle
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Find all the instances of the File System prototocol
    //
    for (Index = 0; Index < NoSimpleFsHandles; Index++) {
      Status = gBS->HandleProtocol (
                      SimpleFsHandle[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **) &BlkIo
                      );
      if (EFI_ERROR (Status)) {
        //
        // If no block IO exists assume it's NOT a removable media
        //
        RemovableMedia = FALSE;
      } else {
        //
        // If block IO exists check to see if it's remobable media
        //
        RemovableMedia = BlkIo->Media->RemovableMedia;
      }

      //
      // Allocate pool for this instance.
      //
      MenuEntry = CreateMenuEntry ();
      if (NULL == MenuEntry) {
        FreePool (SimpleFsHandle);
        return EFI_OUT_OF_RESOURCES;
      }

      FileContext = (SECUREBOOT_FILE_CONTEXT *) MenuEntry->FileContext;

      FileContext->Handle     = SimpleFsHandle[Index];
      MenuEntry->OptionNumber = Index;
      FileContext->FHandle    = OpenRoot (FileContext->Handle);
      if (FileContext->FHandle == NULL) {
        DestroyMenuEntry (MenuEntry);
        continue;
      }

      MenuEntry->HelpString = DevicePathToStr (DevicePathFromHandle (FileContext->Handle));
      FileContext->Info = FileSystemVolumeLabelInfo (FileContext->FHandle);
      FileContext->FileName = StrDuplicate (L"\\");
      FileContext->DevicePath = FileDevicePath (
                                  FileContext->Handle,
                                  FileContext->FileName
                                  );
      FileContext->IsDir            = TRUE;
      FileContext->IsRoot           = TRUE;
      FileContext->IsRemovableMedia = RemovableMedia;
      FileContext->IsLoadFile       = FALSE;

      //
      // Get current file system's Volume Label
      //
      if (FileContext->Info == NULL) {
        VolumeLabel = L"NO FILE SYSTEM INFO";
      } else {
        if (FileContext->Info->VolumeLabel == NULL) {
          VolumeLabel = L"NULL VOLUME LABEL";
        } else {
          VolumeLabel = FileContext->Info->VolumeLabel;
          if (*VolumeLabel == 0x0000) {
            VolumeLabel = L"NO VOLUME LABEL";
          }
        }
      }

      TempStr                   = MenuEntry->HelpString;
      MenuEntry->DisplayString  = AllocateZeroPool (MAX_CHAR);
      ASSERT (MenuEntry->DisplayString != NULL);
      UnicodeSPrint (
        MenuEntry->DisplayString,
        MAX_CHAR,
        L"%s, [%s]",
        VolumeLabel,
        TempStr
        );
      OptionNumber++;
      InsertTailList (&FsOptionMenu.Head, &MenuEntry->Link);
    }
  }

  if (NoSimpleFsHandles != 0) {
    FreePool (SimpleFsHandle);
  }

  //
  // Remember how many file system options are here
  //
  FsOptionMenu.MenuNumber = OptionNumber;
  return EFI_SUCCESS;
}


/**
  Find files under the current directory. All files and sub-directories
  in current directory will be stored in DirectoryMenu for future use.

  @param[in] MenuEntry     The Menu Entry.

  @retval EFI_SUCCESS      Get files from current dir successfully.
  @return Other            Can't get files from current dir.

**/
EFI_STATUS
FindFiles (
  IN SECUREBOOT_MENU_ENTRY              *MenuEntry
  )
{
  EFI_FILE_HANDLE           NewDir;
  EFI_FILE_HANDLE           Dir;
  EFI_FILE_INFO             *DirInfo;
  UINTN                     BufferSize;
  UINTN                     DirBufferSize;
  SECUREBOOT_MENU_ENTRY     *NewMenuEntry;
  SECUREBOOT_FILE_CONTEXT   *FileContext;
  SECUREBOOT_FILE_CONTEXT   *NewFileContext;
  UINTN                     Pass;
  EFI_STATUS                Status;
  UINTN                     OptionNumber;

  FileContext   = (SECUREBOOT_FILE_CONTEXT *) MenuEntry->FileContext;
  Dir           = FileContext->FHandle;
  OptionNumber  = 0;
  //
  // Open current directory to get files from it
  //
  Status = Dir->Open (
                  Dir,
                  &NewDir,
                  FileContext->FileName,
                  EFI_FILE_READ_ONLY,
                  0
                  );
  if (!FileContext->IsRoot) {
    Dir->Close (Dir);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DirInfo = FileInfo (NewDir);
  if (DirInfo == NULL) {
    return EFI_NOT_FOUND;
  }

  if ((DirInfo->Attribute & EFI_FILE_DIRECTORY) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  FileContext->DevicePath = FileDevicePath (
                              FileContext->Handle,
                              FileContext->FileName
                              );

  DirBufferSize = sizeof (EFI_FILE_INFO) + 1024;
  DirInfo       = AllocateZeroPool (DirBufferSize);
  if (DirInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get all files in current directory
  // Pass 1 to get Directories
  // Pass 2 to get files that are EFI images
  //
  for (Pass = 1; Pass <= 2; Pass++) {
    NewDir->SetPosition (NewDir, 0);
    for (;;) {
      BufferSize  = DirBufferSize;
      Status      = NewDir->Read (NewDir, &BufferSize, DirInfo);
      if (EFI_ERROR (Status) || BufferSize == 0) {
        break;
      }

      if (((DirInfo->Attribute & EFI_FILE_DIRECTORY) != 0 && Pass == 2) ||
          ((DirInfo->Attribute & EFI_FILE_DIRECTORY) == 0 && Pass == 1)
          ) {
        //
        // Pass 1 is for Directories
        // Pass 2 is for file names
        //
        continue;
      }

      NewMenuEntry = CreateMenuEntry ();
      if (NULL == NewMenuEntry) {
        return EFI_OUT_OF_RESOURCES;
      }

      NewFileContext          = (SECUREBOOT_FILE_CONTEXT *) NewMenuEntry->FileContext;
      NewFileContext->Handle  = FileContext->Handle;
      NewFileContext->FileName = AppendFileName (
                                  FileContext->FileName,
                                  DirInfo->FileName
                                  );
      NewFileContext->FHandle = NewDir;
      NewFileContext->DevicePath = FileDevicePath (
                                    NewFileContext->Handle,
                                    NewFileContext->FileName
                                    );
      NewMenuEntry->HelpString = NULL;

      NewFileContext->IsDir = (BOOLEAN) ((DirInfo->Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY);
      if (NewFileContext->IsDir) {
        BufferSize = StrLen (DirInfo->FileName) * 2 + 6;
        NewMenuEntry->DisplayString = AllocateZeroPool (BufferSize);

        UnicodeSPrint (
          NewMenuEntry->DisplayString,
          BufferSize,
          L"<%s>",
          DirInfo->FileName
          );

      } else {
        NewMenuEntry->DisplayString = StrDuplicate (DirInfo->FileName);
      }

      NewFileContext->IsRoot            = FALSE;
      NewFileContext->IsLoadFile        = FALSE;
      NewFileContext->IsRemovableMedia  = FALSE;

      NewMenuEntry->OptionNumber        = OptionNumber;
      OptionNumber++;
      InsertTailList (&DirectoryMenu.Head, &NewMenuEntry->Link);
    }
  }

  DirectoryMenu.MenuNumber = OptionNumber;
  FreePool (DirInfo);
  return EFI_SUCCESS;
}

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                         mStartOpCodeHandle,
                                         &gEfiIfrTianoGuid,
                                         NULL,
                                         sizeof (EFI_IFR_GUID_LABEL)
                                         );
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
}

/**
  Update the File Explore page.

  @param[in] HiiHandle          Hii Handle of the package to be updated.
  @param[in] MenuOption         The Menu whose string tokens need to be updated.
  @param[in] FeCurrentState     Current file explorer state.

**/
VOID
UpdateFileExplorePage (
  IN EFI_HII_HANDLE               HiiHandle,
  IN SECUREBOOT_MENU_OPTION       *MenuOption,
  IN FILE_EXPLORER_STATE          FeCurrentState
  )
{
  UINTN                   Index;
  SECUREBOOT_MENU_ENTRY   *NewMenuEntry;
  SECUREBOOT_FILE_CONTEXT *NewFileContext;
  EFI_FORM_ID             FormId;
  EFI_FORM_ID             FileFormId;

  if (FeCurrentState == FileExplorerStateEnrollPkFile) {
    FormId     = SECUREBOOT_ADD_PK_FILE_FORM_ID;
    FileFormId = FORM_FILE_EXPLORER_ID_PK;
  } else if (FeCurrentState == FileExplorerStateEnrollKekFile) {
    FormId     = FORMID_ENROLL_KEK_FORM;
    FileFormId = FORM_FILE_EXPLORER_ID_KEK;
  } else if (FeCurrentState == FileExplorerStateEnrollSignatureFileToDb) {
    FormId     = SECUREBOOT_ENROLL_SIGNATURE_TO_DB;
    FileFormId = FORM_FILE_EXPLORER_ID_DB;
  } else if (FeCurrentState == FileExplorerStateEnrollSignatureFileToDbx) {
    FormId     = SECUREBOOT_ENROLL_SIGNATURE_TO_DBX;
    FileFormId = FORM_FILE_EXPLORER_ID_DBX;
  } else if (FeCurrentState == FileExplorerStateEnrollSignatureFileToDbt) {
    FormId     = SECUREBOOT_ENROLL_SIGNATURE_TO_DBT;
    FileFormId = FORM_FILE_EXPLORER_ID_DBT;
  } else {
    return;
  }

  NewMenuEntry    = NULL;
  NewFileContext  = NULL;

  RefreshUpdateData ();
  mStartLabel->Number = FORM_FILE_EXPLORER_ID;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry    = GetMenuEntry (MenuOption, Index);
    NewFileContext  = (SECUREBOOT_FILE_CONTEXT *) NewMenuEntry->FileContext;

    if (NewFileContext->IsDir) {
      //
      // Create Text opcode for directory.
      //
      HiiCreateActionOpCode (
        mStartOpCodeHandle,
        (UINT16) (FILE_OPTION_OFFSET + Index),
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL),
        EFI_IFR_FLAG_CALLBACK,
        0
        );
    } else {

      //
      // Create Goto opcode for file.
      //
      HiiCreateGotoOpCode (
        mStartOpCodeHandle,
        FormId,
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL),
        EFI_IFR_FLAG_CALLBACK,
        (UINT16) (FILE_OPTION_GOTO_OFFSET + Index)
        );
    }
  }

  HiiUpdateForm (
    HiiHandle,
    &gSecureBootConfigFormSetGuid,
    FileFormId,
    mStartOpCodeHandle, // Label FORM_FILE_EXPLORER_ID
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Update the file explorer page with the refreshed file system.

  @param[in] PrivateData     Module private data.
  @param[in] KeyValue        Key value to identify the type of data to expect.

  @retval  TRUE           Inform the caller to create a callback packet to exit file explorer.
  @retval  FALSE          Indicate that there is no need to exit file explorer.

**/
BOOLEAN
UpdateFileExplorer (
  IN SECUREBOOT_CONFIG_PRIVATE_DATA   *PrivateData,
  IN UINT16                           KeyValue
  )
{
  UINT16                              FileOptionMask;
  SECUREBOOT_MENU_ENTRY               *NewMenuEntry;
  SECUREBOOT_FILE_CONTEXT             *NewFileContext;
  EFI_FORM_ID                         FormId;
  BOOLEAN                             ExitFileExplorer;
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *TmpDevicePath;

  NewMenuEntry      = NULL;
  NewFileContext    = NULL;
  ExitFileExplorer  = FALSE;
  FileOptionMask    = (UINT16) (FILE_OPTION_MASK & KeyValue);

  if (PrivateData->FeDisplayContext == FileExplorerDisplayUnknown) {
    //
    // First in, display file system.
    //
    FreeMenu (&FsOptionMenu);
    FindFileSystem ();

    CreateMenuStringToken (PrivateData->HiiHandle, &FsOptionMenu);
    UpdateFileExplorePage (PrivateData->HiiHandle, &FsOptionMenu, PrivateData->FeCurrentState);

    PrivateData->FeDisplayContext = FileExplorerDisplayFileSystem;
  } else {
    if (PrivateData->FeDisplayContext == FileExplorerDisplayFileSystem) {
      NewMenuEntry = GetMenuEntry (&FsOptionMenu, FileOptionMask);
    } else if (PrivateData->FeDisplayContext == FileExplorerDisplayDirectory) {
      NewMenuEntry = GetMenuEntry (&DirectoryMenu, FileOptionMask);
    }

    NewFileContext = (SECUREBOOT_FILE_CONTEXT *) NewMenuEntry->FileContext;

    if (NewFileContext->IsDir ) {
      PrivateData->FeDisplayContext = FileExplorerDisplayDirectory;

      RemoveEntryList (&NewMenuEntry->Link);
      FreeMenu (&DirectoryMenu);
      Status = FindFiles (NewMenuEntry);
       if (EFI_ERROR (Status)) {
         ExitFileExplorer = TRUE;
         goto OnExit;
       }
      CreateMenuStringToken (PrivateData->HiiHandle, &DirectoryMenu);
      DestroyMenuEntry (NewMenuEntry);

      UpdateFileExplorePage (PrivateData->HiiHandle, &DirectoryMenu, PrivateData->FeCurrentState);

    } else {
      if (PrivateData->FeCurrentState == FileExplorerStateEnrollPkFile) {
        FormId = SECUREBOOT_ADD_PK_FILE_FORM_ID;
      } else if (PrivateData->FeCurrentState == FileExplorerStateEnrollKekFile) {
        FormId = FORMID_ENROLL_KEK_FORM;
      } else if (PrivateData->FeCurrentState == FileExplorerStateEnrollSignatureFileToDb) {
        FormId = SECUREBOOT_ENROLL_SIGNATURE_TO_DB;
      } else if (PrivateData->FeCurrentState == FileExplorerStateEnrollSignatureFileToDbx) {
        FormId = SECUREBOOT_ENROLL_SIGNATURE_TO_DBX;
      } else if (PrivateData->FeCurrentState == FileExplorerStateEnrollSignatureFileToDbt) {
        FormId = SECUREBOOT_ENROLL_SIGNATURE_TO_DBT;
      } else {
        return FALSE;
      }

      PrivateData->MenuEntry = NewMenuEntry;
      PrivateData->FileContext->FileName = NewFileContext->FileName;

      TmpDevicePath = NewFileContext->DevicePath;
      OpenFileByDevicePath (
        &TmpDevicePath,
        &PrivateData->FileContext->FHandle,
        EFI_FILE_MODE_READ,
        0
        );

      //
      // Create Subtitle op-code for the display string of the option.
      //
      RefreshUpdateData ();
      mStartLabel->Number = FormId;

      HiiCreateSubTitleOpCode (
        mStartOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        0,
        0,
        0
        );

      HiiUpdateForm (
        PrivateData->HiiHandle,
        &gSecureBootConfigFormSetGuid,
        FormId,
        mStartOpCodeHandle, // Label FormId
        mEndOpCodeHandle    // LABEL_END
        );
    }
  }

OnExit:
  return ExitFileExplorer;
}

/**
  Clean up the dynamic opcode at label and form specified by both LabelId.

  @param[in] LabelId         It is both the Form ID and Label ID for opcode deletion.
  @param[in] PrivateData     Module private data.

**/
VOID
CleanUpPage (
  IN UINT16                           LabelId,
  IN SECUREBOOT_CONFIG_PRIVATE_DATA   *PrivateData
  )
{
  RefreshUpdateData ();

  //
  // Remove all op-codes from dynamic page
  //
  mStartLabel->Number = LabelId;
  HiiUpdateForm (
    PrivateData->HiiHandle,
    &gSecureBootConfigFormSetGuid,
    LabelId,
    mStartOpCodeHandle, // Label LabelId
    mEndOpCodeHandle    // LABEL_END
    );
}

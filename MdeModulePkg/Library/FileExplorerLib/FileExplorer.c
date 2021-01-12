/** @file
File explorer related functions.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "FileExplorer.h"

EFI_GUID FileExplorerGuid       = EFI_FILE_EXPLORE_FORMSET_GUID;

///
/// File system selection menu
///
MENU_OPTION      mFsOptionMenu = {
  MENU_OPTION_SIGNATURE,
  {NULL},
  0,
  FALSE
};

FILE_EXPLORER_CALLBACK_DATA  gFileExplorerPrivate = {
  FILE_EXPLORER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    LibExtractConfig,
    LibRouteConfig,
    LibCallback
  },
  NULL,
  &mFsOptionMenu,
  0
};

HII_VENDOR_DEVICE_PATH  *gHiiVendorDevicePath;

HII_VENDOR_DEVICE_PATH  FeHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // Will be replace with gEfiCallerIdGuid in code.
    //
    { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

VOID                *mLibStartOpCodeHandle = NULL;
VOID                *mLibEndOpCodeHandle = NULL;
EFI_IFR_GUID_LABEL  *mLibStartLabel = NULL;
EFI_IFR_GUID_LABEL  *mLibEndLabel = NULL;
UINT16              mQuestionIdUpdate;
CHAR16  mNewFileName[MAX_FILE_NAME_LEN];
CHAR16  mNewFolderName[MAX_FOLDER_NAME_LEN];
UINTN  mNewFileQuestionId    = NEW_FILE_QUESTION_ID_BASE;
UINTN  mNewFolderQuestionId  = NEW_FOLDER_QUESTION_ID_BASE;

/**
  Create a new file or folder in current directory.

  @param FileName              Point to the fileNmae or folder.
  @param CreateFile            CreateFile== TRUE  means create a new file.
                               CreateFile== FALSE means create a new Folder.

**/
EFI_STATUS
LibCreateNewFile (
  IN CHAR16     *FileName,
  IN BOOLEAN    CreateFile
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
LibExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
LibRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.
  When user select a interactive opcode, this callback will be triggered.
  Based on the Question(QuestionId) that triggers the callback, the corresponding
  actions is performed. It handles:

  1) Process the axtra action or exit file explorer when user select one file .
  2) update of file content if a dir is selected.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  other error           Error occur when parse one directory.
**/
EFI_STATUS
EFIAPI
LibCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS    Status;
  BOOLEAN       NeedExit;
  CHAR16        *NewFileName;
  CHAR16        *NewFolderName;

  NeedExit = TRUE;
  NewFileName   = NULL;
  NewFolderName = NULL;

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if (QuestionId == KEY_VALUE_CREATE_FILE_AND_EXIT) {
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      if (!IsZeroBuffer (mNewFileName, sizeof (mNewFileName))) {
        Status = LibCreateNewFile (mNewFileName,TRUE);
        ZeroMem (mNewFileName,sizeof (mNewFileName));
      }
    }

    if (QuestionId == KEY_VALUE_NO_CREATE_FILE_AND_EXIT) {
      ZeroMem (mNewFileName,sizeof (mNewFileName));
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    }

    if (QuestionId == KEY_VALUE_CREATE_FOLDER_AND_EXIT) {
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      if (!IsZeroBuffer (mNewFolderName, sizeof (mNewFolderName))) {
        Status = LibCreateNewFile (mNewFolderName, FALSE);
        ZeroMem (mNewFolderName,sizeof (mNewFolderName));
      }
    }

    if (QuestionId == KEY_VALUE_NO_CREATE_FOLDER_AND_EXIT) {
      ZeroMem (mNewFolderName,sizeof (mNewFolderName));
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    }

    if (QuestionId == NEW_FILE_NAME_ID) {
      NewFileName = HiiGetString (gFileExplorerPrivate.FeHiiHandle, Value->string, NULL);
      if (NewFileName != NULL) {
        StrCpyS (mNewFileName, MAX_FILE_NAME_LEN, NewFileName);
        FreePool (NewFileName);
        NewFileName = NULL;
      } else {
        return EFI_INVALID_PARAMETER;
      }
    }

    if (QuestionId == NEW_FOLDER_NAME_ID) {
      NewFolderName = HiiGetString (gFileExplorerPrivate.FeHiiHandle, Value->string, NULL);
      if (NewFolderName != NULL) {
        StrCpyS (mNewFolderName, MAX_FOLDER_NAME_LEN, NewFolderName);
        FreePool (NewFolderName);
        NewFolderName = NULL;
      } else {
        return EFI_INVALID_PARAMETER;
      }
    }

    if (QuestionId >= FILE_OPTION_OFFSET) {
      LibGetDevicePath(QuestionId);

      //
      // Process the extra action.
      //
      if (gFileExplorerPrivate.ChooseHandler != NULL) {
        NeedExit = gFileExplorerPrivate.ChooseHandler (gFileExplorerPrivate.RetDevicePath);
      }

      if (NeedExit) {
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      }
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    if (QuestionId >= FILE_OPTION_OFFSET) {
      LibGetDevicePath(QuestionId);
      Status = LibUpdateFileExplorer (QuestionId);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Create a menu entry by given menu type.

  @retval NULL           If failed to create the menu.
  @return the new menu entry.

**/
MENU_ENTRY *
LibCreateMenuEntry (
  VOID
  )
{
  MENU_ENTRY *MenuEntry;

  //
  // Create new menu entry
  //
  MenuEntry = AllocateZeroPool (sizeof (MENU_ENTRY));
  if (MenuEntry == NULL) {
    return NULL;
  }

  MenuEntry->VariableContext = AllocateZeroPool (sizeof (FILE_CONTEXT));
  if (MenuEntry->VariableContext == NULL) {
    FreePool (MenuEntry);
    return NULL;
  }

  MenuEntry->Signature        = MENU_ENTRY_SIGNATURE;
  return MenuEntry;
}


/**
  Get the Menu Entry from the list in Menu Entry List.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param MenuOption      The Menu Entry List to read the menu entry.
  @param MenuNumber      The index of Menu Entry.

  @return The Menu Entry.

**/
MENU_ENTRY *
LibGetMenuEntry (
  MENU_OPTION         *MenuOption,
  UINTN               MenuNumber
  )
{
  MENU_ENTRY      *NewMenuEntry;
  UINTN           Index;
  LIST_ENTRY      *List;

  ASSERT (MenuNumber < MenuOption->MenuNumber);

  List = MenuOption->Head.ForwardLink;
  for (Index = 0; Index < MenuNumber; Index++) {
    List = List->ForwardLink;
  }

  NewMenuEntry = CR (List, MENU_ENTRY, Link, MENU_ENTRY_SIGNATURE);

  return NewMenuEntry;
}

/**
  Free up all resource allocated for a BM_MENU_ENTRY.

  @param MenuEntry   A pointer to BM_MENU_ENTRY.

**/
VOID
LibDestroyMenuEntry (
  MENU_ENTRY         *MenuEntry
  )
{
  FILE_CONTEXT           *FileContext;

  FileContext = (FILE_CONTEXT *) MenuEntry->VariableContext;

  if (!FileContext->IsRoot) {
    if (FileContext->DevicePath != NULL) {
      FreePool (FileContext->DevicePath);
    }
  } else {
    if (FileContext->FileHandle != NULL) {
      FileContext->FileHandle->Close (FileContext->FileHandle);
    }
  }

  if (FileContext->FileName != NULL) {
    FreePool (FileContext->FileName);
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

  @param FreeMenu        Menu to be freed
**/
VOID
LibFreeMenu (
  MENU_OPTION        *FreeMenu
  )
{
  MENU_ENTRY *MenuEntry;
  while (!IsListEmpty (&FreeMenu->Head)) {
    MenuEntry = CR (
                  FreeMenu->Head.ForwardLink,
                  MENU_ENTRY,
                  Link,
                  MENU_ENTRY_SIGNATURE
                  );
    RemoveEntryList (&MenuEntry->Link);
    LibDestroyMenuEntry (MenuEntry);
  }
  FreeMenu->MenuNumber = 0;
}

/**

  Function opens and returns a file handle to the root directory of a volume.

  @param DeviceHandle    A handle for a device

  @return A valid file handle or NULL is returned

**/
EFI_FILE_HANDLE
LibOpenRoot (
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
  This function converts an input device structure to a Unicode string.

  @param DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
LibDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *ToText;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevPathToText;

  if (DevPath == NULL) {
    return NULL;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevPathToText
                  );
  ASSERT_EFI_ERROR (Status);
  ToText = DevPathToText->ConvertDevicePathToText (
                            DevPath,
                            FALSE,
                            TRUE
                            );
  ASSERT (ToText != NULL);

  return ToText;
}

/**
  Duplicate a string.

  @param Src             The source.

  @return A new string which is duplicated copy of the source.
  @retval NULL If there is not enough memory.

**/
CHAR16 *
LibStrDuplicate (
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

  Function gets the file information from an open file descriptor, and stores it
  in a buffer allocated from pool.

  @param FHand           File Handle.
  @param InfoType        Info type need to get.

  @retval                A pointer to a buffer with file information or NULL is returned

**/
VOID *
LibFileInfo (
  IN EFI_FILE_HANDLE      FHand,
  IN EFI_GUID             *InfoType
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *Buffer;
  UINTN         BufferSize;

  Buffer      = NULL;
  BufferSize  = 0;

  Status = FHand->GetInfo (
                    FHand,
                    InfoType,
                    &BufferSize,
                    Buffer
                    );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocatePool (BufferSize);
    ASSERT (Buffer != NULL);
  }

  Status = FHand->GetInfo (
                    FHand,
                    InfoType,
                    &BufferSize,
                    Buffer
                    );

  return Buffer;
}

/**

  Get file type base on the file name.
  Just cut the file name, from the ".". eg ".efi"

  @param FileName  File need to be checked.

  @retval the file type string.

**/
CHAR16*
LibGetTypeFromName (
  IN CHAR16   *FileName
  )
{
  UINTN    Index;

  Index = StrLen (FileName) - 1;
  while ((FileName[Index] != L'.') && (Index != 0)) {
    Index--;
  }

  return Index == 0 ? NULL : &FileName[Index];
}

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param ConfigString  String to be converted

**/
VOID
LibToLowerString (
  IN CHAR16  *String
  )
{
  CHAR16      *TmpStr;

  for (TmpStr = String; *TmpStr != L'\0'; TmpStr++) {
    if (*TmpStr >= L'A' && *TmpStr <= L'Z') {
      *TmpStr = (CHAR16) (*TmpStr - L'A' + L'a');
    }
  }
}

/**

  Check whether current FileName point to a valid
  Efi Image File.

  @param FileName  File need to be checked.

  @retval TRUE  Is Efi Image
  @retval FALSE Not a valid Efi Image

**/
BOOLEAN
LibIsSupportedFileType (
  IN UINT16  *FileName
  )
{
  CHAR16     *InputFileType;
  CHAR16     *TmpStr;
  BOOLEAN    IsSupported;

  if (gFileExplorerPrivate.FileType == NULL) {
    return TRUE;
  }

  InputFileType = LibGetTypeFromName (FileName);
  //
  // If the file not has *.* style, always return TRUE.
  //
  if (InputFileType == NULL) {
    return TRUE;
  }

  TmpStr = AllocateCopyPool (StrSize (InputFileType), InputFileType);
  ASSERT(TmpStr != NULL);
  LibToLowerString(TmpStr);

  IsSupported = (StrStr (gFileExplorerPrivate.FileType, TmpStr) == NULL ? FALSE : TRUE);

  FreePool (TmpStr);
  return IsSupported;
}

/**

  Append file name to existing file name.

  @param Str1  The existing file name
  @param Str2  The file name to be appended

  @return Allocate a new string to hold the appended result.
          Caller is responsible to free the returned string.

**/
CHAR16 *
LibAppendFileName (
  IN  CHAR16  *Str1,
  IN  CHAR16  *Str2
  )
{
  UINTN   Size1;
  UINTN   Size2;
  UINTN   MaxLen;
  CHAR16  *Str;
  CHAR16  *TmpStr;
  CHAR16  *Ptr;
  CHAR16  *LastSlash;

  Size1 = StrSize (Str1);
  Size2 = StrSize (Str2);

  //
  // Check overflow
  //
  if (((MAX_UINTN - Size1) < Size2) || ((MAX_UINTN - Size1 - Size2) < sizeof(CHAR16))) {
    return NULL;
  }

  MaxLen = (Size1 + Size2 + sizeof (CHAR16))/ sizeof (CHAR16);
  Str   = AllocateZeroPool (Size1 + Size2 + sizeof (CHAR16));
  ASSERT (Str != NULL);

  TmpStr = AllocateZeroPool (Size1 + Size2 + sizeof (CHAR16));
  ASSERT (TmpStr != NULL);

  StrCpyS (Str, MaxLen, Str1);
  if (!((*Str == '\\') && (*(Str + 1) == 0))) {
    StrCatS (Str, MaxLen, L"\\");
  }

  StrCatS (Str, MaxLen, Str2);

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
      // Use TmpStr as a backup, as StrCpyS in BaseLib does not handle copy of two strings
      // that overlap.
      //
      StrCpyS (TmpStr, MaxLen, Ptr + 3);
      StrCpyS (LastSlash, MaxLen - ((UINTN) LastSlash - (UINTN) Str) / sizeof (CHAR16), TmpStr);
      Ptr = LastSlash;
    } else if (*Ptr == '\\' && *(Ptr + 1) == '.' && *(Ptr + 2) == '\\') {
      //
      // Convert a "\.\" to a "\"
      //

      //
      // Use TmpStr as a backup, as StrCpyS in BaseLib does not handle copy of two strings
      // that overlap.
      //
      StrCpyS (TmpStr, MaxLen, Ptr + 2);
      StrCpyS (Ptr, MaxLen - ((UINTN) Ptr - (UINTN) Str) / sizeof (CHAR16), TmpStr);
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
  This function build the FsOptionMenu list which records all
  available file system in the system. They includes all instances
  of EFI_SIMPLE_FILE_SYSTEM_PROTOCOL, all instances of EFI_LOAD_FILE_SYSTEM.


  @retval  EFI_SUCCESS             Success find the file system
  @retval  EFI_OUT_OF_RESOURCES    Can not create menu entry

**/
EFI_STATUS
LibFindFileSystem (
  VOID
  )
{
  UINTN                        NoSimpleFsHandles;
  EFI_HANDLE                   *SimpleFsHandle;
  UINT16                       *VolumeLabel;
  UINTN                        Index;
  EFI_STATUS                   Status;
  MENU_ENTRY                   *MenuEntry;
  FILE_CONTEXT                 *FileContext;
  UINTN                        OptionNumber;
  EFI_FILE_SYSTEM_VOLUME_LABEL *Info;

  NoSimpleFsHandles = 0;
  OptionNumber      = 0;

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
      //
      // Allocate pool for this load option
      //
      MenuEntry = LibCreateMenuEntry ();
      if (NULL == MenuEntry) {
        FreePool (SimpleFsHandle);
        return EFI_OUT_OF_RESOURCES;
      }

      FileContext = (FILE_CONTEXT *) MenuEntry->VariableContext;
      FileContext->DeviceHandle = SimpleFsHandle[Index];
      FileContext->FileHandle = LibOpenRoot (FileContext->DeviceHandle);
      if (FileContext->FileHandle == NULL) {
        LibDestroyMenuEntry (MenuEntry);
        continue;
      }

      MenuEntry->HelpString = LibDevicePathToStr (DevicePathFromHandle (FileContext->DeviceHandle));
      FileContext->FileName = LibStrDuplicate (L"\\");
      FileContext->DevicePath = FileDevicePath (FileContext->DeviceHandle, FileContext->FileName);
      FileContext->IsDir = TRUE;
      FileContext->IsRoot = TRUE;

      //
      // Get current file system's Volume Label
      //
      Info = (EFI_FILE_SYSTEM_VOLUME_LABEL *) LibFileInfo (FileContext->FileHandle, &gEfiFileSystemVolumeLabelInfoIdGuid);
      if (Info == NULL) {
        VolumeLabel = L"NO FILE SYSTEM INFO";
      } else {
        VolumeLabel = Info->VolumeLabel;
        if (*VolumeLabel == 0x0000) {
          VolumeLabel = L"NO VOLUME LABEL";
        }
      }
      MenuEntry->DisplayString  = AllocateZeroPool (MAX_CHAR);
      ASSERT (MenuEntry->DisplayString != NULL);
      UnicodeSPrint (
        MenuEntry->DisplayString,
        MAX_CHAR,
        L"%s, [%s]",
        VolumeLabel,
        MenuEntry->HelpString
        );
      MenuEntry->DisplayStringToken = HiiSetString (
                                             gFileExplorerPrivate.FeHiiHandle,
                                             0,
                                             MenuEntry->DisplayString,
                                             NULL
                                             );

      if (Info != NULL)
        FreePool (Info);

      OptionNumber++;
      InsertTailList (&gFileExplorerPrivate.FsOptionMenu->Head, &MenuEntry->Link);
    }
  }

  if (NoSimpleFsHandles != 0) {
    FreePool (SimpleFsHandle);
  }

  gFileExplorerPrivate.FsOptionMenu->MenuNumber = OptionNumber;

  return EFI_SUCCESS;
}

/**
  Find the file handle from the input menu info.

  @param  MenuEntry        Input Menu info.
  @param  RetFileHandle    Return the file handle for the input device path.

  @retval EFI_SUCESS       Find the file handle success.
  @retval Other            Find the file handle failure.
**/
EFI_STATUS
LibGetFileHandleFromMenu (
  IN  MENU_ENTRY                *MenuEntry,
  OUT EFI_FILE_HANDLE           *RetFileHandle
  )
{
  EFI_FILE_HANDLE Dir;
  EFI_FILE_HANDLE NewDir;
  FILE_CONTEXT    *FileContext;
  EFI_STATUS      Status;

  FileContext   = (FILE_CONTEXT *) MenuEntry->VariableContext;
  Dir           = FileContext->FileHandle;

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
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!FileContext->IsRoot) {
    Dir->Close (Dir);
  }

  *RetFileHandle = NewDir;

  return EFI_SUCCESS;
}

/**
  Find the file handle from the input device path info.

  @param  RootDirectory    Device path info.
  @param  RetFileHandle    Return the file handle for the input device path.
  @param  ParentFileName   Parent file name.
  @param  DeviceHandle     Driver handle for this partition.

  @retval EFI_SUCESS       Find the file handle success.
  @retval Other            Find the file handle failure.
**/
EFI_STATUS
LibGetFileHandleFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDirectory,
  OUT EFI_FILE_HANDLE           *RetFileHandle,
  OUT UINT16                    **ParentFileName,
  OUT EFI_HANDLE                *DeviceHandle
  )
{
  EFI_DEVICE_PATH_PROTOCOL          *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL          *TempDevicePathNode;
  EFI_STATUS                        Status;
  EFI_HANDLE                        Handle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Volume;
  EFI_FILE_HANDLE                   FileHandle;
  EFI_FILE_HANDLE                   LastHandle;
  CHAR16                            *TempPath;

  *ParentFileName = NULL;

  //
  // Attempt to access the file via a file system interface
  //
  DevicePathNode = RootDirectory;
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &DevicePathNode, &Handle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Volume);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the Volume to get the File System handle
  //
  Status = Volume->OpenVolume (Volume, &FileHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *DeviceHandle = Handle;

  if (IsDevicePathEnd(DevicePathNode)) {
    *ParentFileName = AllocateCopyPool (StrSize (L"\\"), L"\\");
    *RetFileHandle = FileHandle;
    return EFI_SUCCESS;
  }

  //
  // Duplicate the device path to avoid the access to unaligned device path node.
  // Because the device path consists of one or more FILE PATH MEDIA DEVICE PATH
  // nodes, It assures the fields in device path nodes are 2 byte aligned.
  //
  TempDevicePathNode = DuplicateDevicePath (DevicePathNode);
  if (TempDevicePathNode == NULL) {

    //
    // Setting Status to an EFI_ERROR value will cause the rest of
    // the file system support below to be skipped.
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Parse each MEDIA_FILEPATH_DP node. There may be more than one, since the
  // directory information and filename can be seperate. The goal is to inch
  // our way down each device path node and close the previous node
  //
  DevicePathNode = TempDevicePathNode;
  while (!EFI_ERROR (Status) && !IsDevicePathEnd (DevicePathNode)) {
    if (DevicePathType (DevicePathNode) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (DevicePathNode) != MEDIA_FILEPATH_DP) {
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    LastHandle = FileHandle;
    FileHandle = NULL;

    Status = LastHandle->Open (
                          LastHandle,
                          &FileHandle,
                          ((FILEPATH_DEVICE_PATH *) DevicePathNode)->PathName,
                          EFI_FILE_MODE_READ,
                          0
                          );
    if (*ParentFileName == NULL) {
      *ParentFileName = AllocateCopyPool (StrSize (((FILEPATH_DEVICE_PATH *) DevicePathNode)->PathName), ((FILEPATH_DEVICE_PATH *) DevicePathNode)->PathName);
    } else {
      TempPath = LibAppendFileName (*ParentFileName, ((FILEPATH_DEVICE_PATH *) DevicePathNode)->PathName);
      if (TempPath == NULL) {
        LastHandle->Close (LastHandle);
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      FreePool (*ParentFileName);
      *ParentFileName = TempPath;
    }

    //
    // Close the previous node
    //
    LastHandle->Close (LastHandle);

    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  *RetFileHandle = FileHandle;

  Status = EFI_SUCCESS;

Done:
  if (TempDevicePathNode != NULL) {
    FreePool (TempDevicePathNode);
  }

  if ((FileHandle != NULL) && (EFI_ERROR (Status))) {
    FileHandle->Close (FileHandle);
  }

  return Status;
}

/**
  Create a new file or folder in current directory.

  @param FileName              Point to the fileNmae or folder name.
  @param CreateFile            CreateFile== TRUE  means create a new file.
                               CreateFile== FALSE means create a new Folder.

**/
EFI_STATUS
LibCreateNewFile (
  IN CHAR16     *FileName,
  IN BOOLEAN    CreateFile
  )
{
  EFI_FILE_HANDLE      FileHandle;
  EFI_FILE_HANDLE      NewHandle;
  EFI_HANDLE           DeviceHandle;
  EFI_STATUS           Status;
  CHAR16               *ParentName;
  CHAR16               *FullFileName;

  NewHandle = NULL;
  FullFileName = NULL;

  LibGetFileHandleFromDevicePath(gFileExplorerPrivate.RetDevicePath, &FileHandle, &ParentName, &DeviceHandle);
  FullFileName = LibAppendFileName (ParentName, FileName);
  if (FullFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  if (CreateFile) {
    Status = FileHandle->Open(
                          FileHandle,
                          &NewHandle,
                          FullFileName,
                          EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE| EFI_FILE_MODE_CREATE,
                          0
                          );
    if (EFI_ERROR (Status)) {
      FileHandle->Close (FileHandle);
      return Status;
    }
  } else {
    Status = FileHandle->Open(
                          FileHandle,
                          &NewHandle,
                          FullFileName,
                          EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE| EFI_FILE_MODE_CREATE,
                          EFI_FILE_DIRECTORY
                          );
    if (EFI_ERROR (Status)) {
      FileHandle->Close (FileHandle);
      return Status;
    }
  }

  FileHandle->Close (FileHandle);

  //
  // Return the DevicePath of the new created file or folder.
  //
  gFileExplorerPrivate.RetDevicePath = FileDevicePath (DeviceHandle, FullFileName);

  return EFI_SUCCESS;

}

/**
  Find files under current directory.

  All files and sub-directories in current directory
  will be stored in DirectoryMenu for future use.

  @param FileHandle    Parent file handle.
  @param FileName      Parent file name.
  @param DeviceHandle  Driver handle for this partition.

  @retval EFI_SUCCESS         Get files from current dir successfully.
  @return Other value if can't get files from current dir.

**/
EFI_STATUS
LibFindFiles (
  IN EFI_FILE_HANDLE           FileHandle,
  IN UINT16                    *FileName,
  IN EFI_HANDLE                DeviceHandle
  )
{
  EFI_FILE_INFO   *DirInfo;
  UINTN           BufferSize;
  UINTN           DirBufferSize;
  MENU_ENTRY      *NewMenuEntry;
  FILE_CONTEXT    *NewFileContext;
  UINTN           Pass;
  EFI_STATUS      Status;
  UINTN           OptionNumber;

  OptionNumber = 0;

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
  Status = EFI_SUCCESS;
  for (Pass = 1; Pass <= 2; Pass++) {
    FileHandle->SetPosition (FileHandle, 0);
    for (;;) {
      BufferSize  = DirBufferSize;
      Status      = FileHandle->Read (FileHandle, &BufferSize, DirInfo);
      if (EFI_ERROR (Status) || BufferSize == 0) {
        Status = EFI_SUCCESS;
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

      if (!((DirInfo->Attribute & EFI_FILE_DIRECTORY) != 0 || LibIsSupportedFileType (DirInfo->FileName))) {
        //
        // Slip file unless it is a directory entry or a .EFI file
        //
        continue;
      }

      NewMenuEntry = LibCreateMenuEntry ();
      if (NULL == NewMenuEntry) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      NewFileContext = (FILE_CONTEXT *) NewMenuEntry->VariableContext;
      NewFileContext->DeviceHandle = DeviceHandle;
      NewFileContext->FileName = LibAppendFileName (FileName, DirInfo->FileName);
      if  (NewFileContext->FileName == NULL) {
        LibDestroyMenuEntry (NewMenuEntry);
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      NewFileContext->FileHandle = FileHandle;
      NewFileContext->DevicePath = FileDevicePath (NewFileContext->DeviceHandle, NewFileContext->FileName);
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
        NewMenuEntry->DisplayString = LibStrDuplicate (DirInfo->FileName);
      }

      NewMenuEntry->DisplayStringToken = HiiSetString (
                                           gFileExplorerPrivate.FeHiiHandle,
                                           0,
                                           NewMenuEntry->DisplayString,
                                           NULL
                                           );

      NewFileContext->IsRoot            = FALSE;

      OptionNumber++;
      InsertTailList (&gFileExplorerPrivate.FsOptionMenu->Head, &NewMenuEntry->Link);
    }
  }

  gFileExplorerPrivate.FsOptionMenu->MenuNumber = OptionNumber;

Done:

  FreePool (DirInfo);

  return Status;
}

/**
  Refresh the global UpdateData structure.

**/
VOID
LibRefreshUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //
  if (mLibStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mLibStartOpCodeHandle);
  }
  if (mLibEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mLibEndOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mLibStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  mLibEndOpCodeHandle = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mLibStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                         mLibStartOpCodeHandle,
                                         &gEfiIfrTianoGuid,
                                         NULL,
                                         sizeof (EFI_IFR_GUID_LABEL)
                                         );
  mLibStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  mLibStartLabel->Number = FORM_FILE_EXPLORER_ID;

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mLibEndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                         mLibEndOpCodeHandle,
                                         &gEfiIfrTianoGuid,
                                         NULL,
                                         sizeof (EFI_IFR_GUID_LABEL)
                                         );
  mLibEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  mLibEndLabel->Number = LABEL_END;
}

/**

  Update the File Explore page.

**/
VOID
LibUpdateFileExplorePage (
  VOID
  )
{
  UINTN           Index;
  MENU_ENTRY      *NewMenuEntry;
  FILE_CONTEXT    *NewFileContext;
  MENU_OPTION     *MenuOption;
  BOOLEAN         CreateNewFile;

  NewMenuEntry    = NULL;
  NewFileContext  = NULL;
  CreateNewFile   = FALSE;

  LibRefreshUpdateData ();
  MenuOption = gFileExplorerPrivate.FsOptionMenu;

  mQuestionIdUpdate += QUESTION_ID_UPDATE_STEP;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry    = LibGetMenuEntry (MenuOption, Index);
    NewFileContext  = (FILE_CONTEXT *) NewMenuEntry->VariableContext;

    if (!NewFileContext->IsRoot && !CreateNewFile) {
      HiiCreateGotoOpCode (
        mLibStartOpCodeHandle,
        FORM_ADD_NEW_FILE_ID,
        STRING_TOKEN (STR_NEW_FILE),
        STRING_TOKEN (STR_NEW_FILE_HELP),
        EFI_IFR_FLAG_CALLBACK,
        (UINT16) (mNewFileQuestionId++)
        );
      HiiCreateGotoOpCode (
        mLibStartOpCodeHandle,
        FORM_ADD_NEW_FOLDER_ID,
        STRING_TOKEN (STR_NEW_FOLDER),
        STRING_TOKEN (STR_NEW_FOLDER_HELP),
        EFI_IFR_FLAG_CALLBACK,
        (UINT16) (mNewFolderQuestionId++)
        );
      HiiCreateTextOpCode(
        mLibStartOpCodeHandle,
        STRING_TOKEN (STR_NULL_STRING),
        STRING_TOKEN (STR_NULL_STRING),
        0
        );
      CreateNewFile = TRUE;
    }

    if (!NewFileContext->IsDir) {
      //
      // Create Text opcode for directory, also create Text opcode for file in FileExplorerStateBootFromFile.
      //
      HiiCreateActionOpCode (
        mLibStartOpCodeHandle,
        (UINT16) (FILE_OPTION_OFFSET + Index + mQuestionIdUpdate),
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL_STRING),
        EFI_IFR_FLAG_CALLBACK,
        0
        );
    } else {
      //
      // Create Goto opcode for file in FileExplorerStateAddBootOption or FileExplorerStateAddDriverOptionState.
      //
      HiiCreateGotoOpCode (
        mLibStartOpCodeHandle,
        FORM_FILE_EXPLORER_ID,
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL_STRING),
        EFI_IFR_FLAG_CALLBACK,
        (UINT16) (FILE_OPTION_OFFSET + Index + mQuestionIdUpdate)
        );
    }
  }

  HiiUpdateForm (
    gFileExplorerPrivate.FeHiiHandle,
    &FileExplorerGuid,
    FORM_FILE_EXPLORER_ID,
    mLibStartOpCodeHandle, // Label FORM_FILE_EXPLORER_ID
    mLibEndOpCodeHandle    // LABEL_END
    );
}

/**
  Update the file explower page with the refershed file system.

  @param KeyValue        Key value to identify the type of data to expect.

  @retval  EFI_SUCCESS   Update the file explorer form success.
  @retval  other errors  Error occur when parse one directory.

**/
EFI_STATUS
LibUpdateFileExplorer (
  IN UINT16                       KeyValue
  )
{
  UINT16          FileOptionMask;
  MENU_ENTRY      *NewMenuEntry;
  FILE_CONTEXT    *NewFileContext;
  EFI_STATUS      Status;
  EFI_FILE_HANDLE FileHandle;

  Status = EFI_SUCCESS;
  FileOptionMask = (UINT16) (FILE_OPTION_MASK & KeyValue) - mQuestionIdUpdate;
  NewMenuEntry   = LibGetMenuEntry (gFileExplorerPrivate.FsOptionMenu, FileOptionMask);
  NewFileContext = (FILE_CONTEXT *) NewMenuEntry->VariableContext;

  if (NewFileContext->IsDir) {
    RemoveEntryList (&NewMenuEntry->Link);
    LibFreeMenu (gFileExplorerPrivate.FsOptionMenu);
    Status = LibGetFileHandleFromMenu (NewMenuEntry, &FileHandle);
    if (!EFI_ERROR (Status)) {
      Status = LibFindFiles (FileHandle, NewFileContext->FileName, NewFileContext->DeviceHandle);
      if (!EFI_ERROR (Status)) {
        LibUpdateFileExplorePage ();
      } else {
        LibFreeMenu (gFileExplorerPrivate.FsOptionMenu);
      }
    }
    LibDestroyMenuEntry (NewMenuEntry);
  }

  return Status;
}

/**
  Get the device path info saved in the menu structure.

  @param KeyValue        Key value to identify the type of data to expect.

**/
VOID
LibGetDevicePath (
  IN UINT16                       KeyValue
  )
{
  UINT16          FileOptionMask;
  MENU_ENTRY      *NewMenuEntry;
  FILE_CONTEXT    *NewFileContext;

  FileOptionMask    = (UINT16) (FILE_OPTION_MASK & KeyValue) - mQuestionIdUpdate;

  NewMenuEntry = LibGetMenuEntry (gFileExplorerPrivate.FsOptionMenu, FileOptionMask);

  NewFileContext = (FILE_CONTEXT *) NewMenuEntry->VariableContext;

  if (gFileExplorerPrivate.RetDevicePath != NULL) {
    FreePool (gFileExplorerPrivate.RetDevicePath);
  }
  gFileExplorerPrivate.RetDevicePath = DuplicateDevicePath (NewFileContext->DevicePath);
}

/**
  Choose a file in the specified directory.

  If user input NULL for the RootDirectory, will choose file in the system.

  If user input *File != NULL, function will return the allocate device path
  info for the choosed file, caller has to free the memory after use it.

  @param  RootDirectory    Pointer to the root directory.
  @param  FileType         The file type need to choose.
  @param  ChooseHandler    Function pointer to the extra task need to do
                           after choose one file.
  @param  File             Return the device path for the last time chosed file.

  @retval EFI_SUCESS             Choose file success.
  @retval EFI_INVALID_PARAMETER  Both ChooseHandler and return device path are NULL
                                 One of them must not NULL.
  @retval Other errors           Choose file failed.
**/
EFI_STATUS
EFIAPI
ChooseFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDirectory,
  IN  CHAR16                    *FileType,  OPTIONAL
  IN  CHOOSE_HANDLER            ChooseHandler,  OPTIONAL
  OUT EFI_DEVICE_PATH_PROTOCOL  **File  OPTIONAL
  )
{
  EFI_FILE_HANDLE                   FileHandle;
  EFI_STATUS                        Status;
  UINT16                            *FileName;
  EFI_HANDLE                        DeviceHandle;

  if ((ChooseHandler == NULL) && (File == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  mQuestionIdUpdate = 0;
  FileName = NULL;

  gFileExplorerPrivate.RetDevicePath = NULL;
  gFileExplorerPrivate.ChooseHandler = ChooseHandler;
  if (FileType != NULL) {
    gFileExplorerPrivate.FileType = AllocateCopyPool (StrSize (FileType), FileType);
    ASSERT(gFileExplorerPrivate.FileType != NULL);
    LibToLowerString(gFileExplorerPrivate.FileType);
  } else {
    gFileExplorerPrivate.FileType = NULL;
  }

  if (RootDirectory == NULL) {
    Status = LibFindFileSystem();
  } else {
    Status = LibGetFileHandleFromDevicePath(RootDirectory, &FileHandle, &FileName, &DeviceHandle);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Status = LibFindFiles (FileHandle, FileName, DeviceHandle);
  }
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  LibUpdateFileExplorePage();

  gFileExplorerPrivate.FormBrowser2->SendForm (
                         gFileExplorerPrivate.FormBrowser2,
                         &gFileExplorerPrivate.FeHiiHandle,
                         1,
                         &FileExplorerGuid,
                         0,
                         NULL,
                         NULL
                         );

Done:
  if ((Status == EFI_SUCCESS) && (File != NULL)) {
    *File  = gFileExplorerPrivate.RetDevicePath;
  } else if (gFileExplorerPrivate.RetDevicePath != NULL) {
    FreePool (gFileExplorerPrivate.RetDevicePath);
  }

  if (gFileExplorerPrivate.FileType != NULL) {
    FreePool (gFileExplorerPrivate.FileType);
  }

  LibFreeMenu (gFileExplorerPrivate.FsOptionMenu);

  if (FileName != NULL) {
    FreePool (FileName);
  }

  return Status;
}

/**

  Install Boot Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install File explorer library success.

**/
EFI_STATUS
EFIAPI
FileExplorerLibConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS                     Status;

  gHiiVendorDevicePath = (HII_VENDOR_DEVICE_PATH*) DuplicateDevicePath ((EFI_DEVICE_PATH_PROTOCOL*)&FeHiiVendorDevicePath);
  ASSERT (gHiiVendorDevicePath != NULL);
  CopyGuid (&gHiiVendorDevicePath->VendorDevicePath.Guid, &gEfiCallerIdGuid);

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gFileExplorerPrivate.FeDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  gHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gFileExplorerPrivate.FeConfigAccess,
                  NULL
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Post our File Explorer VFR binary to the HII database.
  //
  gFileExplorerPrivate.FeHiiHandle = HiiAddPackages (
                                   &FileExplorerGuid,
                                   gFileExplorerPrivate.FeDriverHandle,
                                   FileExplorerVfrBin,
                                   FileExplorerLibStrings,
                                   NULL
                                   );
  ASSERT (gFileExplorerPrivate.FeHiiHandle != NULL);

  //
  // Locate Formbrowser2 protocol
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &gFileExplorerPrivate.FormBrowser2);
  ASSERT_EFI_ERROR (Status);

  InitializeListHead (&gFileExplorerPrivate.FsOptionMenu->Head);

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.
  @param[in]  SystemTable       The system table.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
FileExplorerLibDestructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS    Status;

  ASSERT (gHiiVendorDevicePath != NULL);

  if (gFileExplorerPrivate.FeDriverHandle != NULL) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    gFileExplorerPrivate.FeDriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    gHiiVendorDevicePath,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &gFileExplorerPrivate.FeConfigAccess,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    HiiRemovePackages (gFileExplorerPrivate.FeHiiHandle);
    gFileExplorerPrivate.FeDriverHandle = NULL;
  }

  FreePool (gHiiVendorDevicePath);

  return EFI_SUCCESS;
}


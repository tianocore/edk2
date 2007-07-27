/*++
Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BootOption.c

Abstract:

  Provide boot option support for Application "BootMaint"

  Include file system navigation, system handle selection

  Boot option manipulation

Revision History

--*/

#include "BootMaint.h"
#include "BBSsupport.h"

BM_MENU_ENTRY *
BOpt_CreateMenuEntry (
  UINTN           MenuType
  )
/*++

Routine Description
  Create Menu Entry for future use, make all types together
  in order to reduce code size

Arguments:
  MenuType            Use this parameter to identify current
                      Menu type

Returns:
  NULL                Cannot allocate memory for current menu
                      entry
  Others              A valid pointer pointing to the allocated
                      memory pool for current menu entry

--*/
{
  BM_MENU_ENTRY *MenuEntry;
  UINTN         ContextSize;

  switch (MenuType) {
  case BM_LOAD_CONTEXT_SELECT:
    ContextSize = sizeof (BM_LOAD_CONTEXT);
    break;

  case BM_FILE_CONTEXT_SELECT:
    ContextSize = sizeof (BM_FILE_CONTEXT);
    break;

  case BM_CONSOLE_CONTEXT_SELECT:
    ContextSize = sizeof (BM_CONSOLE_CONTEXT);
    break;

  case BM_TERMINAL_CONTEXT_SELECT:
    ContextSize = sizeof (BM_TERMINAL_CONTEXT);
    break;

  case BM_HANDLE_CONTEXT_SELECT:
    ContextSize = sizeof (BM_HANDLE_CONTEXT);
    break;

  case BM_LEGACY_DEV_CONTEXT_SELECT:
    ContextSize = sizeof (BM_LEGACY_DEVICE_CONTEXT);
    break;

  default:
    ContextSize = 0;
    break;

  }

  if (0 == ContextSize) {
    return NULL;
  }

  MenuEntry = AllocateZeroPool (sizeof (BM_MENU_ENTRY));
  if (NULL == MenuEntry) {
    return MenuEntry;
  }

  MenuEntry->VariableContext = AllocateZeroPool (ContextSize);
  if (NULL == MenuEntry->VariableContext) {
    SafeFreePool (MenuEntry);
    MenuEntry = NULL;
    return MenuEntry;
  }

  MenuEntry->Signature        = BM_MENU_ENTRY_SIGNATURE;
  MenuEntry->ContextSelection = MenuType;
  return MenuEntry;
}

VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY         *MenuEntry
  )
/*++
  Routine Description :
    Destroy the menu entry passed in

  Arguments :
    The menu entry need to be destroyed

  Returns :
    None

--*/
{
  BM_LOAD_CONTEXT           *LoadContext;
  BM_FILE_CONTEXT           *FileContext;
  BM_CONSOLE_CONTEXT        *ConsoleContext;
  BM_TERMINAL_CONTEXT       *TerminalContext;
  BM_HANDLE_CONTEXT         *HandleContext;
  BM_LEGACY_DEVICE_CONTEXT  *LegacyDevContext;

  //
  //  Select by the type in Menu entry for current context type
  //
  switch (MenuEntry->ContextSelection) {
  case BM_LOAD_CONTEXT_SELECT:
    LoadContext = (BM_LOAD_CONTEXT *) MenuEntry->VariableContext;
    SafeFreePool (LoadContext->FilePathList);
    SafeFreePool (LoadContext->LoadOption);
    SafeFreePool (LoadContext->OptionalData);
    SafeFreePool (LoadContext);
    break;

  case BM_FILE_CONTEXT_SELECT:
    FileContext = (BM_FILE_CONTEXT *) MenuEntry->VariableContext;

    if (!FileContext->IsRoot) {
      SafeFreePool (FileContext->DevicePath);
    } else {
      if (FileContext->FHandle != NULL) {
        FileContext->FHandle->Close (FileContext->FHandle);
      }
    }

    SafeFreePool (FileContext->FileName);
    SafeFreePool (FileContext->Info);
    SafeFreePool (FileContext);
    break;

  case BM_CONSOLE_CONTEXT_SELECT:
    ConsoleContext = (BM_CONSOLE_CONTEXT *) MenuEntry->VariableContext;
    SafeFreePool (ConsoleContext->DevicePath);
    SafeFreePool (ConsoleContext);
    break;

  case BM_TERMINAL_CONTEXT_SELECT:
    TerminalContext = (BM_TERMINAL_CONTEXT *) MenuEntry->VariableContext;
    SafeFreePool (TerminalContext->DevicePath);
    SafeFreePool (TerminalContext);
    break;

  case BM_HANDLE_CONTEXT_SELECT:
    HandleContext = (BM_HANDLE_CONTEXT *) MenuEntry->VariableContext;
    SafeFreePool (HandleContext);
    break;

  case BM_LEGACY_DEV_CONTEXT_SELECT:
    LegacyDevContext = (BM_LEGACY_DEVICE_CONTEXT *) MenuEntry->VariableContext;
    SafeFreePool (LegacyDevContext);

  default:
    break;
  }

  SafeFreePool (MenuEntry->DisplayString);
  if (NULL != MenuEntry->HelpString) {
    SafeFreePool (MenuEntry->HelpString);
  }

  SafeFreePool (MenuEntry);
}

BM_MENU_ENTRY *
BOpt_GetMenuEntry (
  BM_MENU_OPTION      *MenuOption,
  UINTN               MenuNumber
  )
/*++
  Rountine Description :
    Use this routine to get one particular menu entry in specified
    menu

  Arguments :
    MenuOption        The menu that we will search

    MenuNumber        The menunubmer that we want

  Returns :
    The desired menu entry

--*/
{
  BM_MENU_ENTRY   *NewMenuEntry;
  UINTN           Index;
  LIST_ENTRY      *List;

  if (MenuNumber >= MenuOption->MenuNumber) {
    return NULL;
  }

  List = MenuOption->Head.ForwardLink;
  for (Index = 0; Index < MenuNumber; Index++) {
    List = List->ForwardLink;
  }

  NewMenuEntry = CR (List, BM_MENU_ENTRY, Link, BM_MENU_ENTRY_SIGNATURE);

  return NewMenuEntry;
}

EFI_STATUS
BOpt_FindFileSystem (
  IN BMM_CALLBACK_DATA          *CallbackData
  )
/*++

Routine Description
  Find file systems for current Extensible Firmware
  Including Handles that support Simple File System
  protocol, Load File protocol.

  Building up the FileSystem Menu for user selection
  All file system will be stored in FsOptionMenu
  for future use.

Arguments:
  CallbackData           -   BMM context data

Returns:
  EFI_SUCCESS            -   Success find the file system
  EFI_OUT_OF_RESOURCES   -   Can not create menu entry

--*/
{
  UINTN                     NoSimpleFsHandles;
  UINTN                     NoLoadFileHandles;
  EFI_HANDLE                *SimpleFsHandle;
  EFI_HANDLE                *LoadFileHandle;
  UINT16                    *VolumeLabel;
  EFI_BLOCK_IO_PROTOCOL     *BlkIo;
  UINTN                     Index;
  EFI_STATUS                Status;
  BM_MENU_ENTRY             *MenuEntry;
  BM_FILE_CONTEXT           *FileContext;
  UINT16                    *TempStr;
  UINTN                     OptionNumber;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINT16                    DeviceType;
  BBS_BBS_DEVICE_PATH       BbsDevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   RemovableMedia;


  NoSimpleFsHandles = 0;
  NoLoadFileHandles = 0;
  OptionNumber      = 0;
  InitializeListHead (&FsOptionMenu.Head);

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
                      &BlkIo
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
      // Allocate pool for this load option
      //
      MenuEntry = BOpt_CreateMenuEntry (BM_FILE_CONTEXT_SELECT);
      if (NULL == MenuEntry) {
        SafeFreePool (SimpleFsHandle);
        return EFI_OUT_OF_RESOURCES;
      }

      FileContext = (BM_FILE_CONTEXT *) MenuEntry->VariableContext;

      FileContext->Handle     = SimpleFsHandle[Index];
      MenuEntry->OptionNumber = Index;
      FileContext->FHandle    = EfiLibOpenRoot (FileContext->Handle);
      if (!FileContext->FHandle) {
        BOpt_DestroyMenuEntry (MenuEntry);
        continue;
      }

      MenuEntry->HelpString = DevicePathToStr (DevicePathFromHandle (FileContext->Handle));
      FileContext->Info = EfiLibFileSystemVolumeLabelInfo (FileContext->FHandle);
      FileContext->FileName = EfiStrDuplicate (L"\\");
      FileContext->DevicePath = FileDevicePath (
                                  FileContext->Handle,
                                  FileContext->FileName
                                  );
      FileContext->IsDir            = TRUE;
      FileContext->IsRoot           = TRUE;
      FileContext->IsRemovableMedia = FALSE;
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
    SafeFreePool (SimpleFsHandle);
  }
  //
  // Searching for handles that support Load File protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadFileProtocolGuid,
                  NULL,
                  &NoLoadFileHandles,
                  &LoadFileHandle
                  );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NoLoadFileHandles; Index++) {
      MenuEntry = BOpt_CreateMenuEntry (BM_FILE_CONTEXT_SELECT);
      if (NULL == MenuEntry) {
        SafeFreePool (LoadFileHandle);
        return EFI_OUT_OF_RESOURCES;
      }

      FileContext                   = (BM_FILE_CONTEXT *) MenuEntry->VariableContext;
      FileContext->IsRemovableMedia = FALSE;
      FileContext->IsLoadFile       = TRUE;
      FileContext->Handle           = LoadFileHandle[Index];
      FileContext->IsRoot           = TRUE;

      FileContext->DevicePath = DevicePathFromHandle (FileContext->Handle);

      MenuEntry->HelpString     = DevicePathToStr (FileContext->DevicePath);

      TempStr                   = MenuEntry->HelpString;
      MenuEntry->DisplayString  = AllocateZeroPool (MAX_CHAR);
      ASSERT (MenuEntry->DisplayString != NULL);
      UnicodeSPrint (
        MenuEntry->DisplayString,
        MAX_CHAR,
        L"Load File [%s]",
        TempStr
        );

      MenuEntry->OptionNumber = OptionNumber;
      OptionNumber++;
      InsertTailList (&FsOptionMenu.Head, &MenuEntry->Link);
    }
  }

  if (NoLoadFileHandles != 0) {
    SafeFreePool (LoadFileHandle);
  }

  //
  // Add Legacy Boot Option Support Here
  //
  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  if (!EFI_ERROR (Status)) {

    for (Index = BBS_TYPE_FLOPPY; Index <= BBS_TYPE_EMBEDDED_NETWORK; Index++) {
      MenuEntry = BOpt_CreateMenuEntry (BM_FILE_CONTEXT_SELECT);
      if (NULL == MenuEntry) {
        return EFI_OUT_OF_RESOURCES;
      }

      FileContext                       = (BM_FILE_CONTEXT *) MenuEntry->VariableContext;

      FileContext->IsRemovableMedia     = FALSE;
      FileContext->IsLoadFile           = TRUE;
      FileContext->IsBootLegacy         = TRUE;
      DeviceType                        = (UINT16) Index;
      BbsDevicePathNode.Header.Type     = BBS_DEVICE_PATH;
      BbsDevicePathNode.Header.SubType  = BBS_BBS_DP;
      SetDevicePathNodeLength (
        &BbsDevicePathNode.Header,
        sizeof (BBS_BBS_DEVICE_PATH)
        );
      BbsDevicePathNode.DeviceType  = DeviceType;
      BbsDevicePathNode.StatusFlag  = 0;
      BbsDevicePathNode.String[0]   = 0;
      DevicePath = AppendDevicePathNode (
                    EndDevicePath,
                    (EFI_DEVICE_PATH_PROTOCOL *) &BbsDevicePathNode
                    );

      FileContext->DevicePath   = DevicePath;
      MenuEntry->HelpString     = DevicePathToStr (FileContext->DevicePath);

      TempStr                   = MenuEntry->HelpString;
      MenuEntry->DisplayString  = AllocateZeroPool (MAX_CHAR);
      ASSERT (MenuEntry->DisplayString != NULL);
      UnicodeSPrint (
        MenuEntry->DisplayString,
        MAX_CHAR,
        L"Boot Legacy [%s]",
        TempStr
        );
      MenuEntry->OptionNumber = OptionNumber;
      OptionNumber++;
      InsertTailList (&FsOptionMenu.Head, &MenuEntry->Link);
    }
  }
  //
  // Remember how many file system options are here
  //
  FsOptionMenu.MenuNumber = OptionNumber;
  return EFI_SUCCESS;
}

VOID
BOpt_FreeMenu (
  BM_MENU_OPTION        *FreeMenu
  )
/*++

Routine Description
  Free resources allocated in Allocate Rountine

Arguments:
  FreeMenu        Menu to be freed

Returns:
  VOID

--*/
{
  BM_MENU_ENTRY *MenuEntry;
  while (!IsListEmpty (&FreeMenu->Head)) {
    MenuEntry = CR (
                  FreeMenu->Head.ForwardLink,
                  BM_MENU_ENTRY,
                  Link,
                  BM_MENU_ENTRY_SIGNATURE
                  );
    RemoveEntryList (&MenuEntry->Link);
    BOpt_DestroyMenuEntry (MenuEntry);
  }
}

EFI_STATUS
BOpt_FindFiles (
  IN BMM_CALLBACK_DATA          *CallbackData,
  IN BM_MENU_ENTRY              *MenuEntry
  )
/*++

Routine Description
  Find files under current directory
  All files and sub-directories in current directory
  will be stored in DirectoryMenu for future use.

Arguments:
  FileOption   -- Pointer for Dir to explore

Returns:
  TRUE         -- Get files from current dir successfully
  FALSE        -- Can't get files from current dir

--*/
{
  EFI_FILE_HANDLE NewDir;
  EFI_FILE_HANDLE Dir;
  EFI_FILE_INFO   *DirInfo;
  UINTN           BufferSize;
  UINTN           DirBufferSize;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_FILE_CONTEXT *FileContext;
  BM_FILE_CONTEXT *NewFileContext;
  UINTN           Pass;
  EFI_STATUS      Status;
  UINTN           OptionNumber;

  FileContext   = (BM_FILE_CONTEXT *) MenuEntry->VariableContext;
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

  DirInfo = EfiLibFileInfo (NewDir);
  if (!DirInfo) {
    return EFI_NOT_FOUND;
  }

  if (!(DirInfo->Attribute & EFI_FILE_DIRECTORY)) {
    return EFI_INVALID_PARAMETER;
  }

  FileContext->DevicePath = FileDevicePath (
                              FileContext->Handle,
                              FileContext->FileName
                              );

  DirBufferSize = sizeof (EFI_FILE_INFO) + 1024;
  DirInfo       = AllocateZeroPool (DirBufferSize);
  if (!DirInfo) {
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

      if ((DirInfo->Attribute & EFI_FILE_DIRECTORY && Pass == 2) ||
          (!(DirInfo->Attribute & EFI_FILE_DIRECTORY) && Pass == 1)
          ) {
        //
        // Pass 1 is for Directories
        // Pass 2 is for file names
        //
        continue;
      }

      if (!(BOpt_IsEfiImageName (DirInfo->FileName) || DirInfo->Attribute & EFI_FILE_DIRECTORY)) {
        //
        // Slip file unless it is a directory entry or a .EFI file
        //
        continue;
      }

      NewMenuEntry = BOpt_CreateMenuEntry (BM_FILE_CONTEXT_SELECT);
      if (NULL == NewMenuEntry) {
        return EFI_OUT_OF_RESOURCES;
      }

      NewFileContext          = (BM_FILE_CONTEXT *) NewMenuEntry->VariableContext;
      NewFileContext->Handle  = FileContext->Handle;
      NewFileContext->FileName = BOpt_AppendFileName (
                                  FileContext->FileName,
                                  DirInfo->FileName
                                  );
      NewFileContext->FHandle = NewDir;
      NewFileContext->DevicePath = FileDevicePath (
                                    NewFileContext->Handle,
                                    NewFileContext->FileName
                                    );
      NewMenuEntry->HelpString = NULL;

      MenuEntry->DisplayStringToken = GetStringTokenFromDepository (
                                        CallbackData,
                                        FileOptionStrDepository
                                        );

      NewFileContext->IsDir = (BOOLEAN) ((DirInfo->Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY);

      if (NewFileContext->IsDir) {
        BufferSize                  = StrLen (DirInfo->FileName) * 2 + 6;
        NewMenuEntry->DisplayString = AllocateZeroPool (BufferSize);

        UnicodeSPrint (
          NewMenuEntry->DisplayString,
          BufferSize,
          L"<%s>",
          DirInfo->FileName
          );

      } else {
        NewMenuEntry->DisplayString = EfiStrDuplicate (DirInfo->FileName);
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
  SafeFreePool (DirInfo);
  return TRUE;
}

EFI_STATUS
BOpt_GetLegacyOptions (
  VOID
  )
/*++
Routine Description:

  Build the LegacyFDMenu LegacyHDMenu LegacyCDMenu according to LegacyBios.GetBbsInfo().

Arguments:
  None

Returns:
  The device info of legacy device.

--*/
{
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_LEGACY_DEVICE_CONTEXT  *NewLegacyDevContext;
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINT16                    HddCount;
  HDD_INFO                  *HddInfo;
  UINT16                    BbsCount;
  BBS_TABLE                 *BbsTable;
  UINTN                     Index;
  CHAR16                    DescString[100];
  UINTN                     FDNum;
  UINTN                     HDNum;
  UINTN                     CDNum;
  UINTN                     NETNum;
  UINTN                     BEVNum;

  NewMenuEntry  = NULL;
  HddInfo       = NULL;
  BbsTable      = NULL;
  BbsCount      = 0;

  //
  // Initialize Bbs Table Context from BBS info data
  //
  InitializeListHead (&LegacyFDMenu.Head);
  InitializeListHead (&LegacyHDMenu.Head);
  InitializeListHead (&LegacyCDMenu.Head);
  InitializeListHead (&LegacyNETMenu.Head);
  InitializeListHead (&LegacyBEVMenu.Head);

  Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
  if (!EFI_ERROR (Status)) {
    Status = LegacyBios->GetBbsInfo (
                          LegacyBios,
                          &HddCount,
                          &HddInfo,
                          &BbsCount,
                          &BbsTable
                          );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  FDNum   = 0;
  HDNum   = 0;
  CDNum   = 0;
  NETNum  = 0;
  BEVNum  = 0;

  for (Index = 0; Index < BbsCount; Index++) {
    if ((BBS_IGNORE_ENTRY == BbsTable[Index].BootPriority) ||
        (BBS_DO_NOT_BOOT_FROM == BbsTable[Index].BootPriority) ||
        (BBS_LOWEST_PRIORITY == BbsTable[Index].BootPriority)
        ) {
      continue;
    }

    NewMenuEntry = BOpt_CreateMenuEntry (BM_LEGACY_DEV_CONTEXT_SELECT);
    if (NULL == NewMenuEntry) {
      break;
    }

    NewLegacyDevContext           = (BM_LEGACY_DEVICE_CONTEXT *) NewMenuEntry->VariableContext;
    NewLegacyDevContext->BbsTable = &BbsTable[Index];
    NewLegacyDevContext->Index    = Index;
    NewLegacyDevContext->BbsCount = BbsCount;
    BdsBuildLegacyDevNameString (
      &BbsTable[Index],
      Index,
      sizeof (DescString),
      DescString
      );
    NewLegacyDevContext->Description = AllocateZeroPool (StrSize (DescString));
    if (NULL == NewLegacyDevContext->Description) {
      break;
    }

    CopyMem (NewLegacyDevContext->Description, DescString, StrSize (DescString));
    NewMenuEntry->DisplayString = NewLegacyDevContext->Description;
    NewMenuEntry->HelpString    = NULL;

    switch (BbsTable[Index].DeviceType) {
    case BBS_FLOPPY:
      InsertTailList (&LegacyFDMenu.Head, &NewMenuEntry->Link);
      FDNum++;
      break;

    case BBS_HARDDISK:
      InsertTailList (&LegacyHDMenu.Head, &NewMenuEntry->Link);
      HDNum++;
      break;

    case BBS_CDROM:
      InsertTailList (&LegacyCDMenu.Head, &NewMenuEntry->Link);
      CDNum++;
      break;

    case BBS_EMBED_NETWORK:
      InsertTailList (&LegacyNETMenu.Head, &NewMenuEntry->Link);
      NETNum++;
      break;

    case BBS_BEV_DEVICE:
      InsertTailList (&LegacyBEVMenu.Head, &NewMenuEntry->Link);
      BEVNum++;
      break;
    }
  }

  if (Index != BbsCount) {
    BOpt_FreeLegacyOptions ();
    return EFI_OUT_OF_RESOURCES;
  }

  LegacyFDMenu.MenuNumber   = FDNum;
  LegacyHDMenu.MenuNumber   = HDNum;
  LegacyCDMenu.MenuNumber   = CDNum;
  LegacyNETMenu.MenuNumber  = NETNum;
  LegacyBEVMenu.MenuNumber  = BEVNum;
  return EFI_SUCCESS;
}

VOID
BOpt_FreeLegacyOptions (
  VOID
  )
{
  BOpt_FreeMenu (&LegacyFDMenu);
  BOpt_FreeMenu (&LegacyHDMenu);
  BOpt_FreeMenu (&LegacyCDMenu);
  BOpt_FreeMenu (&LegacyNETMenu);
  BOpt_FreeMenu (&LegacyBEVMenu);
}

EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  )
/*++

Routine Description:

  Build the BootOptionMenu according to BootOrder Variable.
  This Routine will access the Boot#### to get EFI_LOAD_OPTION

Arguments:
  None

Returns:
  The number of the Var Boot####

--*/
{
  UINTN                     Index;
  UINT16                    BootString[10];
  UINT8                     *LoadOptionFromVar;
  UINT8                     *LoadOption;
  UINTN                     BootOptionSize;
  BOOLEAN                   BootNextFlag;
  UINT16                    *BootOrderList;
  UINTN                     BootOrderListSize;
  UINT16                    *BootNext;
  UINTN                     BootNextSize;
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_LOAD_CONTEXT           *NewLoadContext;
  UINT8                     *LoadOptionPtr;
  UINTN                     StringSize;
  UINTN                     OptionalDataSize;
  UINT8                     *LoadOptionEnd;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     MenuCount;
  UINT8                     *Ptr;

  MenuCount         = 0;
  BootOrderListSize = 0;
  BootNextSize      = 0;
  BootOrderList     = NULL;
  BootNext          = NULL;
  LoadOptionFromVar = NULL;
  BOpt_FreeMenu (&BootOptionMenu);
  InitializeListHead (&BootOptionMenu.Head);

  //
  // Get the BootOrder from the Var
  //
  BootOrderList = BdsLibGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );

  //
  // Get the BootNext from the Var
  //
  BootNext = BdsLibGetVariableAndSize (
              L"BootNext",
              &gEfiGlobalVariableGuid,
              &BootNextSize
              );

  if (BootNext) {
    if (BootNextSize != sizeof (UINT16)) {
      SafeFreePool (BootNext);
      BootNext = NULL;
    }
  }

  for (Index = 0; Index < BootOrderListSize / sizeof (UINT16); Index++) {
    UnicodeSPrint (BootString, sizeof (BootString), L"Boot%04x", BootOrderList[Index]);
    //
    //  Get all loadoptions from the VAR
    //
    LoadOptionFromVar = BdsLibGetVariableAndSize (
                          BootString,
                          &gEfiGlobalVariableGuid,
                          &BootOptionSize
                          );
    if (!LoadOptionFromVar) {
      continue;
    }

    LoadOption = AllocateZeroPool (BootOptionSize);
    if (!LoadOption) {
      continue;
    }

    CopyMem (LoadOption, LoadOptionFromVar, BootOptionSize);
    SafeFreePool (LoadOptionFromVar);

    if (BootNext) {
      BootNextFlag = (BOOLEAN) (*BootNext == BootOrderList[Index]);
    } else {
      BootNextFlag = FALSE;
    }

    if (0 == (*((UINT32 *) LoadOption) & LOAD_OPTION_ACTIVE)) {
      SafeFreePool (LoadOption);
      continue;
    }
    //
    // BUGBUG: could not return EFI_OUT_OF_RESOURCES here directly.
    // the buffer allocated already should be freed before returning.
    //
    NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
    if (NULL == NewMenuEntry) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewLoadContext                      = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;

    LoadOptionPtr                       = LoadOption;
    LoadOptionEnd                       = LoadOption + BootOptionSize;

    NewMenuEntry->OptionNumber          = BootOrderList[Index];
    NewLoadContext->LoadOptionModified  = FALSE;
    NewLoadContext->Deleted             = FALSE;
    NewLoadContext->IsBootNext          = BootNextFlag;

    //
    // Is a Legacy Device?
    //
    Ptr = (UINT8 *) LoadOption;

    //
    // Attribute = *(UINT32 *)Ptr;
    //
    Ptr += sizeof (UINT32);

    //
    // FilePathSize = *(UINT16 *)Ptr;
    //
    Ptr += sizeof (UINT16);

    //
    // Description = (CHAR16 *)Ptr;
    //
    Ptr += StrSize ((CHAR16 *) Ptr);

    //
    // Now Ptr point to Device Path
    //
    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
    if ((BBS_DEVICE_PATH == DevicePath->Type) && (BBS_BBS_DP == DevicePath->SubType)) {
      NewLoadContext->IsLegacy = TRUE;
    } else {
      NewLoadContext->IsLegacy = FALSE;
    }
    //
    // LoadOption is a pointer type of UINT8
    // for easy use with following LOAD_OPTION
    // embedded in this struct
    //
    NewLoadContext->LoadOption      = LoadOption;
    NewLoadContext->LoadOptionSize  = BootOptionSize;

    NewLoadContext->Attributes      = *(UINT32 *) LoadOptionPtr;
    NewLoadContext->IsActive        = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_ACTIVE);

    NewLoadContext->ForceReconnect  = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_FORCE_RECONNECT);

    LoadOptionPtr += sizeof (UINT32);

    NewLoadContext->FilePathListLength = *(UINT16 *) LoadOptionPtr;
    LoadOptionPtr += sizeof (UINT16);

    StringSize                  = StrSize ((UINT16 *) LoadOptionPtr);
    NewLoadContext->Description = AllocateZeroPool (StringSize);
    ASSERT (NewLoadContext->Description != NULL);
    CopyMem (
      NewLoadContext->Description,
      (UINT16 *) LoadOptionPtr,
      StringSize
      );
    NewMenuEntry->DisplayString = NewLoadContext->Description;

    LoadOptionPtr += StringSize;

    NewLoadContext->FilePathList = AllocateZeroPool (NewLoadContext->FilePathListLength);
    ASSERT (NewLoadContext->FilePathList != NULL);
    CopyMem (
      NewLoadContext->FilePathList,
      (EFI_DEVICE_PATH_PROTOCOL *) LoadOptionPtr,
      NewLoadContext->FilePathListLength
      );

    NewMenuEntry->HelpString = DevicePathToStr (NewLoadContext->FilePathList);
    NewMenuEntry->DisplayStringToken = GetStringTokenFromDepository (
                                        CallbackData,
                                        BootOptionStrDepository
                                        );
    NewMenuEntry->HelpStringToken = GetStringTokenFromDepository (
                                      CallbackData,
                                      BootOptionHelpStrDepository
                                      );
    LoadOptionPtr += NewLoadContext->FilePathListLength;

    if (LoadOptionPtr < LoadOptionEnd) {
      OptionalDataSize = BootOptionSize -
        sizeof (UINT32) -
        sizeof (UINT16) -
        StringSize -
        NewLoadContext->FilePathListLength;

      NewLoadContext->OptionalData = AllocateZeroPool (OptionalDataSize);
      ASSERT (NewLoadContext->OptionalData != NULL);
      CopyMem (
        NewLoadContext->OptionalData,
        LoadOptionPtr,
        OptionalDataSize
        );

      NewLoadContext->OptionalDataSize = OptionalDataSize;
    }

    InsertTailList (&BootOptionMenu.Head, &NewMenuEntry->Link);
    MenuCount++;
  }

  SafeFreePool (BootNext);
  SafeFreePool (BootOrderList);
  BootOptionMenu.MenuNumber = MenuCount;
  return MenuCount;
}

CHAR16 *
BdsStrCpy (
  OUT     CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  )
{
  CHAR16                            *ReturnValue;

  //
  // Destination cannot be NULL
  //
  ASSERT (Destination != NULL);

  ReturnValue = Destination;
  while (*Source) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;
  return ReturnValue;
}

CHAR16 *
BOpt_AppendFileName (
  IN  CHAR16  *Str1,
  IN  CHAR16  *Str2
  )
/*++

Routine Description
  Append file name to existing file name.

Arguments:
  Str1  -   existing file name
  Str2  -   file name to be appended

Returns:
  Allocate a new string to hold the appended result.
  Caller is responsible to free the returned string.

--*/
{
  UINTN   Size1;
  UINTN   Size2;
  CHAR16  *Str;
  CHAR16  *Ptr;
  CHAR16  *LastSlash;

  Size1 = StrSize (Str1);
  Size2 = StrSize (Str2);
  Str   = AllocateZeroPool (Size1 + Size2 + sizeof (CHAR16));
  ASSERT (Str != NULL);

  StrCat (Str, Str1);
  if (!((*Str == '\\') && (*(Str + 1) == 0))) {
    StrCat (Str, L"\\");
  }

  StrCat (Str, Str2);

  Ptr       = Str;
  LastSlash = Str;
  while (*Ptr != 0) {
    if (*Ptr == '\\' && *(Ptr + 1) == '.' && *(Ptr + 2) == '.' && *(Ptr + 3) != 0) {
      //
      // Convert \Name\..\ to \
      // DO NOT convert the .. if it is at the end of the string. This will
      // break the .. behavior in changing directories.
      //
      BdsStrCpy (LastSlash, Ptr + 3);
      Ptr = LastSlash;
    } else if (*Ptr == '\\' && *(Ptr + 1) == '.' && *(Ptr + 2) == '\\') {
      //
      // Convert a \.\ to a \
      //
      BdsStrCpy (Ptr, Ptr + 2);
      Ptr = LastSlash;
    } else if (*Ptr == '\\') {
      LastSlash = Ptr;
    }

    Ptr++;
  }

  return Str;
}

BOOLEAN
BOpt_IsEfiImageName (
  IN UINT16  *FileName
  )
/*++

Routine Description
  Check whether current FileName point to a valid
  Efi Image File.

Arguments:
  FileName  -   File need to be checked.

Returns:
  TRUE  -   Is Efi Image
  FALSE -   Not a valid Efi Image

--*/
{
  //
  // Search for ".efi" extension
  //
  while (*FileName) {
    if (FileName[0] == '.') {
      if (FileName[1] == 'e' || FileName[1] == 'E') {
        if (FileName[2] == 'f' || FileName[2] == 'F') {
          if (FileName[3] == 'i' || FileName[3] == 'I') {
            return TRUE;
          } else if (FileName[3] == 0x0000) {
            return FALSE;
          }
        } else if (FileName[2] == 0x0000) {
          return FALSE;
        }
      } else if (FileName[1] == 0x0000) {
        return FALSE;
      }
    }

    FileName += 1;
  }

  return FALSE;
}


RETURN_STATUS
EFIAPI
IsEfiAppReadFromFile (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
{
  EFI_STATUS        Status;
  EFI_FILE_HANDLE   File;

  File = (EFI_FILE_HANDLE)FileHandle;
  Status = File->SetPosition (File, FileOffset);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return File->Read (File, ReadSize, Buffer);
}



BOOLEAN
BOpt_IsEfiApp (
  IN EFI_FILE_HANDLE Dir,
  IN UINT16          *FileName
  )
/*++

Routine Description:
  Check whether current FileName point to a valid Efi Application

Arguments:
  Dir       -   Pointer to current Directory
  FileName  -   Pointer to current File name.

Returns:
  TRUE      -   Is a valid Efi Application
  FALSE     -   not a valid Efi Application

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  EFI_FILE_HANDLE                       File;

  Status = Dir->Open (Dir, &File, FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle    = (VOID *)File;
  ImageContext.ImageRead = IsEfiAppReadFromFile;

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  File->Close (File);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (ImageContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
    return TRUE;
  } else {
    return FALSE;
  }
 }


EFI_STATUS
BOpt_FindDrivers (
  VOID
  )
/*++

Routine Description
  Find drivers that will be added as Driver#### variables from handles
  in current system environment
  All valid handles in the system except those consume SimpleFs, LoadFile
  are stored in DriverMenu for future use.

Arguments:
  None

Returns:
  EFI_SUCCESS
  Others

--*/
{
  UINTN                           NoDevicePathHandles;
  EFI_HANDLE                      *DevicePathHandle;
  UINTN                           Index;
  EFI_STATUS                      Status;
  BM_MENU_ENTRY                   *NewMenuEntry;
  BM_HANDLE_CONTEXT               *NewHandleContext;
  EFI_HANDLE                      CurHandle;
  UINTN                           OptionNumber;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFs;
  EFI_LOAD_FILE_PROTOCOL          *LoadFile;

  SimpleFs  = NULL;
  LoadFile  = NULL;

  InitializeListHead (&DriverMenu.Head);

  //
  // At first, get all handles that support Device Path
  // protocol which is the basic requirement for
  // Driver####
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  &NoDevicePathHandles,
                  &DevicePathHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OptionNumber = 0;
  for (Index = 0; Index < NoDevicePathHandles; Index++) {
    CurHandle = DevicePathHandle[Index];

    //
    //  Check whether this handle support
    //  driver binding
    //
    Status = gBS->HandleProtocol (
                    CurHandle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    &SimpleFs
                    );
    if (Status == EFI_SUCCESS) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    CurHandle,
                    &gEfiLoadFileProtocolGuid,
                    &LoadFile
                    );
    if (Status == EFI_SUCCESS) {
      continue;
    }

    NewMenuEntry = BOpt_CreateMenuEntry (BM_HANDLE_CONTEXT_SELECT);
    if (NULL == NewMenuEntry) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewHandleContext              = (BM_HANDLE_CONTEXT *) NewMenuEntry->VariableContext;
    NewHandleContext->Handle      = CurHandle;
    NewHandleContext->DevicePath  = DevicePathFromHandle (CurHandle);
    NewMenuEntry->DisplayString = DevicePathToStr (NewHandleContext->DevicePath);
    NewMenuEntry->HelpString    = NULL;
    NewMenuEntry->OptionNumber  = OptionNumber;
    OptionNumber++;
    InsertTailList (&DriverMenu.Head, &NewMenuEntry->Link);

  }

  DriverMenu.MenuNumber = OptionNumber;
  return EFI_SUCCESS;
}

UINT16
BOpt_GetBootOptionNumber (
  VOID
  )
/*++

Routine Description:
  Get the Option Number that does not used

Arguments:

Returns:
  The Option Number

--*/
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        *BootOrderList;
  UINTN         BootOrderListSize;
  UINT16        Number;
  UINTN         Index;
  UINTN         Index2;
  BOOLEAN       Found;
  CHAR16        StrTemp[100];
  UINT16        *OptionBuffer;
  UINTN         OptionSize;

  BootOrderListSize = 0;
  BootOrderList     = NULL;

  BootOrderList = BdsLibGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );
  if (BootOrderList) {
    //
    // already have Boot####
    //
    // AlreadyBootNumbers = BootOrderListSize / sizeof(UINT16);
    //
    for (Index = 0; Index < BootOrderListSize / sizeof (UINT16); Index++) {
      Found = TRUE;
      for (Index2 = 0; Index2 < BootOptionMenu.MenuNumber; Index2++) {
        NewMenuEntry = BOpt_GetMenuEntry (&BootOptionMenu, Index2);
        if (Index == NewMenuEntry->OptionNumber) {
          Found = FALSE;
          break;
        }
      }

      if (Found) {
  	   UnicodeSPrint (StrTemp, 100, L"Boot%04x", Index);
  	   DEBUG((EFI_D_ERROR,"INdex= %s\n", StrTemp));
       OptionBuffer = BdsLibGetVariableAndSize (
                StrTemp,
                &gEfiGlobalVariableGuid,
                &OptionSize
                );
      if (NULL == OptionBuffer)
        break;
      }
    }
    //
    // end for Index
    //
    Number = (UINT16) Index;
  } else {
    //
    // No Boot####
    //
    Number = 0;
  }

  return Number;
}

UINT16
BOpt_GetDriverOptionNumber (
  VOID
  )
/*++

Routine Description:
  Get the Option Number that does not used

Arguments:

Returns:
  The Option Number

--*/
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        *DriverOrderList;
  UINTN         DriverOrderListSize;
  UINT16        Number;
  UINTN         Index;
  UINTN         Index2;
  BOOLEAN       Found;

  DriverOrderListSize = 0;
  DriverOrderList     = NULL;

  DriverOrderList = BdsLibGetVariableAndSize (
                      L"DriverOrder",
                      &gEfiGlobalVariableGuid,
                      &DriverOrderListSize
                      );
  if (DriverOrderList) {
    //
    // already have Driver####
    //
    // AlreadyDriverNumbers = DriverOrderListSize / sizeof(UINT16);
    //
    for (Index = 0; Index < DriverOrderListSize / sizeof (UINT16); Index++) {
      Found = TRUE;
      for (Index2 = 0; Index2 < DriverOptionMenu.MenuNumber; Index2++) {
        NewMenuEntry = BOpt_GetMenuEntry (&DriverOptionMenu, Index2);
        if (Index == NewMenuEntry->OptionNumber) {
          Found = FALSE;
          break;
        }
      }

      if (Found) {
        break;
      }
    }
    //
    // end for Index
    //
    Number = (UINT16) Index;
  } else {
    //
    // No Driver####
    //
    Number = 0;
  }

  return Number;
}

EFI_STATUS
BOpt_GetDriverOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  )
/*++

Routine Description:
  Build up all DriverOptionMenu

Arguments:

Returns:
  The Option Number

--*/
{
  UINTN           Index;
  UINT16          DriverString[12];
  UINT8           *LoadOptionFromVar;
  UINT8           *LoadOption;
  UINTN           DriverOptionSize;

  UINT16          *DriverOrderList;
  UINTN           DriverOrderListSize;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT8           *LoadOptionPtr;
  UINTN           StringSize;
  UINTN           OptionalDataSize;
  UINT8           *LoadOptionEnd;

  DriverOrderListSize = 0;
  DriverOrderList     = NULL;
  DriverOptionSize    = 0;
  LoadOptionFromVar   = NULL;
  BOpt_FreeMenu (&DriverOptionMenu);
  InitializeListHead (&DriverOptionMenu.Head);
  //
  // Get the DriverOrder from the Var
  //
  DriverOrderList = BdsLibGetVariableAndSize (
                      L"DriverOrder",
                      &gEfiGlobalVariableGuid,
                      &DriverOrderListSize
                      );

  for (Index = 0; Index < DriverOrderListSize / sizeof (UINT16); Index++) {
    UnicodeSPrint (
      DriverString,
      sizeof (DriverString),
      L"Driver%04x",
      DriverOrderList[Index]
      );
    //
    //  Get all loadoptions from the VAR
    //
    LoadOptionFromVar = BdsLibGetVariableAndSize (
                          DriverString,
                          &gEfiGlobalVariableGuid,
                          &DriverOptionSize
                          );
    if (!LoadOptionFromVar) {
      continue;
    }

    LoadOption = AllocateZeroPool (DriverOptionSize);
    if (!LoadOption) {
      continue;
    }

    CopyMem (LoadOption, LoadOptionFromVar, DriverOptionSize);
    SafeFreePool (LoadOptionFromVar);

    NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
    if (NULL == NewMenuEntry) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewLoadContext                      = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    LoadOptionPtr                       = LoadOption;
    LoadOptionEnd                       = LoadOption + DriverOptionSize;
    NewMenuEntry->OptionNumber          = DriverOrderList[Index];
    NewLoadContext->LoadOptionModified  = FALSE;
    NewLoadContext->Deleted             = FALSE;
    NewLoadContext->IsLegacy            = FALSE;

    //
    // LoadOption is a pointer type of UINT8
    // for easy use with following LOAD_OPTION
    // embedded in this struct
    //
    NewLoadContext->LoadOption      = LoadOption;
    NewLoadContext->LoadOptionSize  = DriverOptionSize;

    NewLoadContext->Attributes      = *(UINT32 *) LoadOptionPtr;
    NewLoadContext->IsActive        = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_ACTIVE);

    NewLoadContext->ForceReconnect  = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_FORCE_RECONNECT);

    LoadOptionPtr += sizeof (UINT32);

    NewLoadContext->FilePathListLength = *(UINT16 *) LoadOptionPtr;
    LoadOptionPtr += sizeof (UINT16);

    StringSize                  = StrSize ((UINT16 *) LoadOptionPtr);
    NewLoadContext->Description = AllocateZeroPool (StringSize);
    ASSERT (NewLoadContext->Description != NULL);
    CopyMem (
      NewLoadContext->Description,
      (UINT16 *) LoadOptionPtr,
      StringSize
      );
    NewMenuEntry->DisplayString = NewLoadContext->Description;

    LoadOptionPtr += StringSize;

    NewLoadContext->FilePathList = AllocateZeroPool (NewLoadContext->FilePathListLength);
    ASSERT (NewLoadContext->FilePathList != NULL);
    CopyMem (
      NewLoadContext->FilePathList,
      (EFI_DEVICE_PATH_PROTOCOL *) LoadOptionPtr,
      NewLoadContext->FilePathListLength
      );

    NewMenuEntry->HelpString = DevicePathToStr (NewLoadContext->FilePathList);
    NewMenuEntry->DisplayStringToken = GetStringTokenFromDepository (
                                        CallbackData,
                                        DriverOptionStrDepository
                                        );
    NewMenuEntry->HelpStringToken = GetStringTokenFromDepository (
                                      CallbackData,
                                      DriverOptionHelpStrDepository
                                      );
    LoadOptionPtr += NewLoadContext->FilePathListLength;

    if (LoadOptionPtr < LoadOptionEnd) {
      OptionalDataSize = DriverOptionSize -
        sizeof (UINT32) -
        sizeof (UINT16) -
        StringSize -
        NewLoadContext->FilePathListLength;

      NewLoadContext->OptionalData = AllocateZeroPool (OptionalDataSize);
      ASSERT (NewLoadContext->OptionalData != NULL);
      CopyMem (
        NewLoadContext->OptionalData,
        LoadOptionPtr,
        OptionalDataSize
        );

      NewLoadContext->OptionalDataSize = OptionalDataSize;
    }

    InsertTailList (&DriverOptionMenu.Head, &NewMenuEntry->Link);

  }

  SafeFreePool (DriverOrderList);
  DriverOptionMenu.MenuNumber = Index;
  return EFI_SUCCESS;

}

VOID
SafeFreePool (
  IN VOID    *Buffer
  )
/*++

Routine Description:
  Wrap original FreePool gBS call
  in order to decrease code length

Arguments:

Returns:

--*/
{
  if (Buffer != NULL) {
    FreePool (Buffer);
    Buffer = NULL;
  }
}

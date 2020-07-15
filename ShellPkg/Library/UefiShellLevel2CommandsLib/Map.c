/** @file
  Main file for map shell level 2 command.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Library/DevicePathLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/SortLib.h>

/**
  Determine if a string has only numbers and letters.

  This is useful for such things as Map names which can only be letters and numbers.

  @param[in] String       pointer to the string to analyze,
  @param[in] Len          Number of characters to analyze.

  @retval TRUE            String has only numbers and letters
  @retval FALSE           String has at least one other character.
**/
BOOLEAN
IsNumberLetterOnly(
  IN CONST CHAR16 *String,
  IN CONST UINTN  Len
  )
{
  UINTN Count;
  for (Count = 0 ; Count < Len && String != NULL && *String != CHAR_NULL ; String++,Count++) {
    if (! ((*String >= L'a' && *String <= L'z') ||
           (*String >= L'A' && *String <= L'Z') ||
           (*String >= L'0' && *String <= L'9'))
        ){
      return (FALSE);
    }
  }
  return (TRUE);
}

/**
  Do a search in the Target delimited list.

  @param[in] List         The list to seatch in.
  @param[in] MetaTarget   The item to search for. MetaMatching supported.
  @param[out] FullName    Optional pointer to an allocated buffer containing
                          the match.
  @param[in] Meta         TRUE to use MetaMatching.
  @param[in] SkipTrailingNumbers  TRUE to allow for numbers after the MetaTarget.
  @param[in] Target       The single character that delimits list
                          items (";" normally).
**/
BOOLEAN
SearchList(
  IN CONST CHAR16   *List,
  IN CONST CHAR16   *MetaTarget,
  OUT CHAR16        **FullName OPTIONAL,
  IN CONST BOOLEAN  Meta,
  IN CONST BOOLEAN  SkipTrailingNumbers,
  IN CONST CHAR16   *Target

  )
{
  CHAR16        *TempList;
  CONST CHAR16  *ListWalker;
  BOOLEAN       Result;
  CHAR16        *TempSpot;

  for (ListWalker = List , TempList = NULL
     ; ListWalker != NULL && *ListWalker != CHAR_NULL
     ;
   ) {
    TempList = StrnCatGrow(&TempList, NULL, ListWalker, 0);
    ASSERT(TempList != NULL);
    TempSpot = StrStr(TempList, Target);
    if (TempSpot != NULL) {
      *TempSpot = CHAR_NULL;
    }

    while (SkipTrailingNumbers && (ShellIsDecimalDigitCharacter(TempList[StrLen(TempList)-1]) || TempList[StrLen(TempList)-1] == L':')) {
      TempList[StrLen(TempList)-1] = CHAR_NULL;
    }

    ListWalker = StrStr(ListWalker, Target);
    while(ListWalker != NULL && *ListWalker == *Target) {
      ListWalker++;
    }
    if (Meta) {
      Result = gUnicodeCollation->MetaiMatch(gUnicodeCollation, (CHAR16*)TempList, (CHAR16*)MetaTarget);
    } else {
      Result = (BOOLEAN)(StrCmp(TempList, MetaTarget)==0);
    }
    if (Result) {
      if (FullName != NULL) {
        *FullName = TempList;
      } else {
        FreePool(TempList);
      }
      return (TRUE);
    }
    FreePool(TempList);
    TempList = NULL;
  }

  return (FALSE);
}

/**
  Determine what type of device is represented and return it's string.  The
  string is in allocated memory and must be callee freed.  The HII is is listed below.
  The actual string cannot be determined.

  @param[in] DevicePath     The device to analyze.

  @retval STR_MAP_MEDIA_UNKNOWN   The media type is unknown.
  @retval STR_MAP_MEDIA_HARDDISK  The media is a hard drive.
  @retval STR_MAP_MEDIA_CDROM     The media is a CD ROM.
  @retval STR_MAP_MEDIA_FLOPPY    The media is a floppy drive.
**/
CHAR16*
GetDeviceMediaType (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  ACPI_HID_DEVICE_PATH  *Acpi;

  //
  //  Parse the device path:
  //  Devicepath sub type                 mediatype
  //    MEDIA_HANRDDRIVE_DP      ->       Hard Disk
  //    MEDIA_CDROM_DP           ->       CD Rom
  //    Acpi.HID = 0X0604        ->       Floppy
  //
  if (NULL == DevicePath) {
    return HiiGetString(gShellLevel2HiiHandle, STRING_TOKEN(STR_MAP_MEDIA_UNKNOWN), NULL);
  }

  for (;!IsDevicePathEndType (DevicePath) ;DevicePath = NextDevicePathNode (DevicePath)) {
    if (DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) {
      switch (DevicePathSubType (DevicePath)) {
      case MEDIA_HARDDRIVE_DP:
        return HiiGetString(gShellLevel2HiiHandle, STRING_TOKEN(STR_MAP_MEDIA_HARDDISK), NULL);
      case MEDIA_CDROM_DP:
        return HiiGetString(gShellLevel2HiiHandle, STRING_TOKEN(STR_MAP_MEDIA_CDROM), NULL);
      }
    } else if (DevicePathType (DevicePath) == ACPI_DEVICE_PATH) {
      Acpi = (ACPI_HID_DEVICE_PATH *) DevicePath;
      if (EISA_ID_TO_NUM (Acpi->HID) == 0x0604) {
        return HiiGetString(gShellLevel2HiiHandle, STRING_TOKEN(STR_MAP_MEDIA_FLOPPY), NULL);
      }
    }
  }

  return HiiGetString(gShellLevel2HiiHandle, STRING_TOKEN(STR_MAP_MEDIA_UNKNOWN), NULL);
}

/**
  Function to detemine if a handle has removable storage.

  @param[in] DevicePath             DevicePath to test.

  @retval TRUE                      The handle has removable storage.
  @retval FALSE                     The handle does not have removable storage.
**/
BOOLEAN
IsRemoveableDevice (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  if (NULL == DevicePath) {
    return FALSE;
  }

  while (!IsDevicePathEndType (DevicePath)) {
    if (DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) {
      switch (DevicePathSubType (DevicePath)) {
      case MSG_USB_DP:
      case MSG_SCSI_DP:
        return TRUE;
      default:
        return FALSE;
      }
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  return FALSE;
}

/**
  Function to detemine if a something on the map list matches.

  @param[in] MapList          The pointer to the list to test.
  @param[in] Specific         The pointer to a specific name to test for.
  @param[in] TypeString       The pointer to the list of types.
  @param[in] Normal           Always show normal mappings.
  @param[in] Consist          Always show consistent mappings.

  @retval TRUE                The map should be displayed.
  @retval FALSE               The map should not be displayed.
**/
BOOLEAN
MappingListHasType(
  IN CONST CHAR16     *MapList,
  IN CONST CHAR16     *Specific,
  IN CONST CHAR16     *TypeString,
  IN CONST BOOLEAN    Normal,
  IN CONST BOOLEAN    Consist
  )
{
  CHAR16              *NewSpecific;
  RETURN_STATUS       Status;
  UINTN               Length;

  //
  // specific has priority
  //
  if (Specific != NULL) {
    Length      = StrLen (Specific);
    //
    // Allocate enough buffer for Specific and potential ":"
    //
    NewSpecific = AllocatePool ((Length + 2) * sizeof(CHAR16));
    if (NewSpecific == NULL){
      return FALSE;
    }
    StrCpyS (NewSpecific, Length + 2, Specific);
    if (Specific[Length - 1] != L':') {
      Status = StrnCatS(NewSpecific, Length + 2, L":", StrLen(L":"));
      if (EFI_ERROR (Status)) {
        FreePool(NewSpecific);
        return FALSE;
      }
    }

    if (SearchList(MapList, NewSpecific, NULL, TRUE, FALSE, L";")) {
      FreePool(NewSpecific);
      return (TRUE);
    }
    FreePool(NewSpecific);
  }
  if (  Consist
    && Specific == NULL
    && (SearchList(MapList, L"HD*",  NULL, TRUE, TRUE, L";")
      ||SearchList(MapList, L"CD*",  NULL, TRUE, TRUE, L";")
      ||SearchList(MapList, L"F*",   NULL, TRUE, TRUE, L";")
      ||SearchList(MapList, L"FP*",  NULL, TRUE, TRUE, L";"))){
    return (TRUE);
  }

  if (  Normal
    && Specific == NULL
    && (SearchList(MapList, L"FS",  NULL, FALSE, TRUE, L";")
      ||SearchList(MapList, L"BLK", NULL, FALSE, TRUE, L";"))){
    return (TRUE);
  }

  if (TypeString != NULL && SearchList(MapList, TypeString,  NULL, TRUE, TRUE, L";")) {
    return (TRUE);
  }
  return (FALSE);
}


/**
  Display a single map line for device Handle if conditions are met.

  @param[in] Verbose                TRUE to display (extra) verbose information.
  @param[in] Consist                TRUE to display consistent mappings.
  @param[in] Normal                 TRUE to display normal (not consist) mappings.
  @param[in] TypeString             pointer to string of filter types.
  @param[in] SFO                    TRUE to display output in Standard Output Format.
  @param[in] Specific               pointer to string for specific map to display.
  @param[in] Handle                 The handle to display from.

  @retval EFI_SUCCESS               The mapping was displayed.
**/
EFI_STATUS
PerformSingleMappingDisplay(
  IN CONST BOOLEAN    Verbose,
  IN CONST BOOLEAN    Consist,
  IN CONST BOOLEAN    Normal,
  IN CONST CHAR16     *TypeString,
  IN CONST BOOLEAN    SFO,
  IN CONST CHAR16     *Specific OPTIONAL,
  IN CONST EFI_HANDLE Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathCopy;
  CONST CHAR16              *MapList;
  CHAR16                    *CurrentName;
  CHAR16                    *MediaType;
  CHAR16                    *DevPathString;
  CHAR16                    *TempSpot;
  CHAR16                    *Alias;
  UINTN                     TempLen;
  BOOLEAN                   Removable;
  CONST CHAR16              *TempSpot2;

  Alias       = NULL;
  TempSpot2   = NULL;
  CurrentName = NULL;
  DevPath = DevicePathFromHandle(Handle);
  DevPathCopy = DevPath;
  MapList = gEfiShellProtocol->GetMapFromDevicePath(&DevPathCopy);
  if (MapList == NULL) {
    return EFI_NOT_FOUND;
  }

  if (!MappingListHasType(MapList, Specific, TypeString, Normal, Consist)){
    return EFI_NOT_FOUND;
  }

  if (Normal || !Consist) {
    //
    // need the Normal here since people can use both on command line.  otherwise unused.
    //

    //
    // Allocate a name
    //
    CurrentName = NULL;
    CurrentName = StrnCatGrow(&CurrentName, 0, MapList, 0);
    if (CurrentName == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }

    //
    // Chop off the other names that become "Alias(s)"
    // leaving just the normal name
    //
    TempSpot = StrStr(CurrentName, L";");
    if (TempSpot != NULL) {
      *TempSpot = CHAR_NULL;
    }
  } else {
    CurrentName = NULL;

    //
    // Skip the first name.  This is the standard name.
    //
    TempSpot = StrStr(MapList, L";");
    if (TempSpot != NULL) {
      TempSpot++;
    }
    SearchList(TempSpot, L"HD*", &CurrentName, TRUE, FALSE, L";");
    if (CurrentName == NULL) {
      SearchList(TempSpot, L"CD*", &CurrentName, TRUE, FALSE, L";");
    }
    if (CurrentName == NULL) {
      SearchList(TempSpot, L"FP*", &CurrentName, TRUE, FALSE, L";");
    }
    if (CurrentName == NULL) {
      SearchList(TempSpot, L"F*",  &CurrentName, TRUE, FALSE, L";");
    }
    if (CurrentName == NULL) {
      //
      // We didnt find anything, so just the first one in the list...
      //
      CurrentName = StrnCatGrow(&CurrentName, 0, MapList, 0);
      if (CurrentName == NULL) {
        return (EFI_OUT_OF_RESOURCES);
      }
      TempSpot = StrStr(CurrentName, L";");
      if (TempSpot != NULL) {
        *TempSpot = CHAR_NULL;
      }
    } else {
      Alias = StrnCatGrow(&Alias, 0, MapList, 0);
      if (Alias == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      TempSpot = StrStr(Alias, CurrentName);
      if (TempSpot != NULL) {
        TempSpot2 = StrStr(TempSpot, L";");
        if (TempSpot2 != NULL) {
          TempSpot2++; // Move past ";" from CurrentName
          CopyMem(TempSpot, TempSpot2, StrSize(TempSpot2));
        } else {
          *TempSpot = CHAR_NULL;
        }
      }
      if (Alias[StrLen(Alias)-1] == L';') {
        Alias[StrLen(Alias)-1] = CHAR_NULL;
      }
    }
  }
  DevPathString = ConvertDevicePathToText(DevPath, TRUE, FALSE);
  TempLen = StrLen(CurrentName);
  if (!SFO) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_MAP_ENTRY),
      gShellLevel2HiiHandle,
      CurrentName,
      Alias!=NULL?Alias:(TempLen < StrLen(MapList)?MapList + TempLen+1:L""),
      DevPathString
     );
    if (Verbose) {
      //
      // also print handle, media type, removable (y/n), and current directory
      //
      MediaType = GetDeviceMediaType(DevPath);
      if ((TypeString != NULL &&MediaType != NULL && StrStr(TypeString, MediaType) != NULL) || TypeString == NULL) {
        Removable = IsRemoveableDevice(DevPath);
        TempSpot2 = ShellGetCurrentDir(CurrentName);
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_MAP_ENTRY_VERBOSE),
          gShellLevel2HiiHandle,
          ConvertHandleToHandleIndex(Handle),
          MediaType,
          Removable?L"Yes":L"No",
          TempSpot2
         );
      }
      SHELL_FREE_NON_NULL(MediaType);
    }
  } else {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_MAP_SFO_MAPPINGS),
      gShellLevel2HiiHandle,
      CurrentName,
      DevPathString,
      Consist?L"":(TempLen < StrLen(MapList)?MapList + TempLen+1:L"")
     );
  }
  SHELL_FREE_NON_NULL(DevPathString);
  SHELL_FREE_NON_NULL(CurrentName);
  SHELL_FREE_NON_NULL(Alias);
  return EFI_SUCCESS;
}

/**
  Delete Specific from the list of maps for device Handle.

  @param[in] Specific   The name to delete.
  @param[in] Handle     The device to look on.

  @retval EFI_SUCCESS     The delete was successful.
  @retval EFI_NOT_FOUND   Name was not a map on Handle.
**/
EFI_STATUS
PerformSingleMappingDelete(
  IN CONST CHAR16     *Specific,
  IN CONST EFI_HANDLE Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathCopy;
  CONST CHAR16              *MapList;
  CHAR16                    *CurrentName;

  DevPath     = DevicePathFromHandle(Handle);
  DevPathCopy = DevPath;
  MapList     = gEfiShellProtocol->GetMapFromDevicePath(&DevPathCopy);
  CurrentName = NULL;

  if (MapList == NULL) {
    return (EFI_NOT_FOUND);
  }
  //
  // if there is a specific and its not on the list...
  //
  if (!SearchList(MapList, Specific, &CurrentName, TRUE, FALSE, L";")) {
    return (EFI_NOT_FOUND);
  }
  return (gEfiShellProtocol->SetMap(NULL, CurrentName));
}

CONST CHAR16 Cd[] = L"cd*";
CONST CHAR16 Hd[] = L"hd*";
CONST CHAR16 Fp[] = L"fp*";
CONST CHAR16 AnyF[] = L"F*";
/**
  Function to display mapping information to the user.

  If Specific is specified then Consist and Normal will be ignored since information will
  be printed for the specific item only.

  @param[in] Verbose                TRUE to display (extra) verbose information.
  @param[in] Consist                TRUE to display consistent mappings.
  @param[in] Normal                 TRUE to display normal (not consist) mappings.
  @param[in] TypeString             Pointer to string of filter types.
  @param[in] SFO                    TRUE to display output in Standard Output Format.
  @param[in] Specific               Pointer to string for specific map to display.
  @param[in] Header                 TRUE to print the header block.

  @retval SHELL_SUCCESS               The display was printed.
  @retval SHELL_INVALID_PARAMETER     One of Consist or Normal must be TRUE if no Specific.

**/
SHELL_STATUS
PerformMappingDisplay(
  IN CONST BOOLEAN Verbose,
  IN CONST BOOLEAN Consist,
  IN CONST BOOLEAN Normal,
  IN CONST CHAR16  *TypeString,
  IN CONST BOOLEAN SFO,
  IN CONST CHAR16  *Specific OPTIONAL,
  IN CONST BOOLEAN Header
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     BufferSize;
  UINTN                     LoopVar;
  CHAR16                    *Test;
  BOOLEAN                   Found;

  if (!Consist && !Normal && Specific == NULL && TypeString == NULL) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"map");
    return (SHELL_INVALID_PARAMETER);
  }

  if (TypeString != NULL) {
    Test = (CHAR16*)Cd;
    if (StrnCmp(TypeString, Test, StrLen(Test)-1) != 0) {
      Test = (CHAR16*)Hd;
      if (StrnCmp(TypeString, Test, StrLen(Test)-1) != 0) {
        Test = (CHAR16*)Fp;
        if (StrnCmp(TypeString, Test, StrLen(Test)-1) != 0) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle, L"map", TypeString);
          return (SHELL_INVALID_PARAMETER);
        }
      } else if (Test == NULL) {
        Test = (CHAR16*)AnyF;
      }
    }
  } else {
    Test = NULL;
  }

  if (Header) {
    //
    // Print the header
    //
    if (!SFO) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MAP_HEADER), gShellLevel2HiiHandle);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_SFO_HEADER), gShellLevel2HiiHandle, L"map");
    }
  }

  BufferSize    = 0;
  HandleBuffer  = NULL;

  //
  // Look up all SimpleFileSystems in the platform
  //
  Status = gBS->LocateHandle(
    ByProtocol,
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    &BufferSize,
    HandleBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HandleBuffer = AllocateZeroPool(BufferSize);
    if (HandleBuffer == NULL) {
      return (SHELL_OUT_OF_RESOURCES);
    }
    Status = gBS->LocateHandle(
      ByProtocol,
      &gEfiSimpleFileSystemProtocolGuid,
      NULL,
      &BufferSize,
      HandleBuffer);
  }

  //
  // Get the map name(s) for each one.
  //
  for ( LoopVar = 0, Found = FALSE
      ; LoopVar < (BufferSize / sizeof(EFI_HANDLE)) && HandleBuffer != NULL
      ; LoopVar ++
     ){
    Status = PerformSingleMappingDisplay(
      Verbose,
      Consist,
      Normal,
      Test,
      SFO,
      Specific,
      HandleBuffer[LoopVar]);
    if (!EFI_ERROR(Status)) {
      Found = TRUE;
    }
  }

  //
  // Look up all BlockIo in the platform
  //
  Status = gBS->LocateHandle(
    ByProtocol,
    &gEfiBlockIoProtocolGuid,
    NULL,
    &BufferSize,
    HandleBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    SHELL_FREE_NON_NULL(HandleBuffer);
    HandleBuffer = AllocateZeroPool(BufferSize);
    if (HandleBuffer == NULL) {
      return (SHELL_OUT_OF_RESOURCES);
    }
    Status = gBS->LocateHandle(
      ByProtocol,
      &gEfiBlockIoProtocolGuid,
      NULL,
      &BufferSize,
      HandleBuffer);
  }
  if (!EFI_ERROR(Status) && HandleBuffer != NULL) {
    //
    // Get the map name(s) for each one.
    //
    for ( LoopVar = 0
        ; LoopVar < BufferSize / sizeof(EFI_HANDLE)
        ; LoopVar ++
       ){
      //
      // Skip any that were already done...
      //
      if (gBS->OpenProtocol(
        HandleBuffer[LoopVar],
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        gImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_TEST_PROTOCOL) == EFI_SUCCESS) {
          continue;
      }
      Status = PerformSingleMappingDisplay(
        Verbose,
        Consist,
        Normal,
        Test,
        SFO,
        Specific,
        HandleBuffer[LoopVar]);
      if (!EFI_ERROR(Status)) {
        Found = TRUE;
      }
    }
    FreePool(HandleBuffer);
  }
  if (!Found) {
    if (Specific != NULL) {
      ShellPrintHiiEx(gST->ConOut->Mode->CursorColumn, gST->ConOut->Mode->CursorRow-1, NULL, STRING_TOKEN (STR_MAP_NF), gShellLevel2HiiHandle, L"map", Specific);
    } else {
      ShellPrintHiiEx(gST->ConOut->Mode->CursorColumn, gST->ConOut->Mode->CursorRow-1, NULL, STRING_TOKEN (STR_CD_NF), gShellLevel2HiiHandle, L"map");
    }
  }
  return (SHELL_SUCCESS);
}

/**
  Perform a mapping display and parse for multiple types in the TypeString.

  @param[in] Verbose      TRUE to use verbose output.
  @param[in] Consist      TRUE to display consistent names.
  @param[in] Normal       TRUE to display normal names.
  @param[in] TypeString   An optional comma-delimited list of types.
  @param[in] SFO          TRUE to display in SFO format.  See Spec.
  @param[in] Specific     An optional specific map name to display alone.

  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
  @retval SHELL_SUCCESS             The display was successful.
  @sa PerformMappingDisplay
**/
SHELL_STATUS
PerformMappingDisplay2(
  IN CONST BOOLEAN Verbose,
  IN CONST BOOLEAN Consist,
  IN CONST BOOLEAN Normal,
  IN CONST CHAR16  *TypeString,
  IN CONST BOOLEAN SFO,
  IN CONST CHAR16  *Specific OPTIONAL
  )
{
  CONST CHAR16  *TypeWalker;
  SHELL_STATUS  ShellStatus;
  CHAR16        *Comma;


  if (TypeString == NULL) {
    return (PerformMappingDisplay(Verbose, Consist, Normal, NULL, SFO, Specific, TRUE));
  }
  ShellStatus = SHELL_SUCCESS;
  for (TypeWalker = TypeString ; TypeWalker != NULL && *TypeWalker != CHAR_NULL ;) {
    Comma = StrStr(TypeWalker, L",");
    if (Comma == NULL) {
      if (ShellStatus == SHELL_SUCCESS) {
        ShellStatus = PerformMappingDisplay(Verbose, Consist, Normal, TypeWalker, SFO, Specific, (BOOLEAN)(TypeWalker == TypeString));
      } else {
        PerformMappingDisplay(Verbose, Consist, Normal, TypeWalker, SFO, Specific, (BOOLEAN)(TypeWalker == TypeString));
      }
      break;
    } else {
      *Comma = CHAR_NULL;
      if (ShellStatus == SHELL_SUCCESS) {
        ShellStatus = PerformMappingDisplay(Verbose, Consist, Normal, TypeWalker, SFO, Specific, (BOOLEAN)(TypeWalker == TypeString));
      } else {
        PerformMappingDisplay(Verbose, Consist, Normal, TypeWalker, SFO, Specific, (BOOLEAN)(TypeWalker == TypeString));
      }
      *Comma = L',';
      TypeWalker = Comma + 1;
    }
  }

  return (ShellStatus);
}

/**
  Delete a specific map.

  @param[in] Specific  The pointer to the name of the map to delete.

  @retval EFI_INVALID_PARAMETER     Specific was NULL.
  @retval EFI_SUCCESS               The operation was successful.
  @retval EFI_NOT_FOUND             Specific could not be found.
**/
EFI_STATUS
PerformMappingDelete(
  IN CONST CHAR16  *Specific
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     BufferSize;
  UINTN                     LoopVar;
  BOOLEAN                   Deleted;

  if (Specific == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  BufferSize    = 0;
  HandleBuffer  = NULL;
  Deleted       = FALSE;

  //
  // Look up all SimpleFileSystems in the platform
  //
  Status = gBS->LocateHandle(
    ByProtocol,
    &gEfiDevicePathProtocolGuid,
    NULL,
    &BufferSize,
    HandleBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HandleBuffer = AllocateZeroPool(BufferSize);
    if (HandleBuffer == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    Status = gBS->LocateHandle(
      ByProtocol,
      &gEfiDevicePathProtocolGuid,
      NULL,
      &BufferSize,
      HandleBuffer);
  }
  if (EFI_ERROR(Status)) {
    SHELL_FREE_NON_NULL(HandleBuffer);
    return (Status);
  }

  if (HandleBuffer != NULL) {
    //
    // Get the map name(s) for each one.
    //
    for ( LoopVar = 0
        ; LoopVar < BufferSize / sizeof(EFI_HANDLE)
        ; LoopVar ++
       ){
      if (PerformSingleMappingDelete(Specific,HandleBuffer[LoopVar]) == SHELL_SUCCESS) {
          Deleted = TRUE;
      }
    }
  }
  //
  // Look up all BlockIo in the platform
  //
  Status = gBS->LocateHandle(
    ByProtocol,
    &gEfiBlockIoProtocolGuid,
    NULL,
    &BufferSize,
    HandleBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool(HandleBuffer);
    HandleBuffer = AllocateZeroPool(BufferSize);
    if (HandleBuffer == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    Status = gBS->LocateHandle(
      ByProtocol,
      &gEfiBlockIoProtocolGuid,
      NULL,
      &BufferSize,
      HandleBuffer);
  }
  if (EFI_ERROR(Status)) {
    SHELL_FREE_NON_NULL(HandleBuffer);
    return (Status);
  }

  if (HandleBuffer != NULL) {
    //
    // Get the map name(s) for each one.
    //
    for ( LoopVar = 0
        ; LoopVar < BufferSize / sizeof(EFI_HANDLE)
        ; LoopVar ++
       ){
      //
      // Skip any that were already done...
      //
      if (gBS->OpenProtocol(
        HandleBuffer[LoopVar],
        &gEfiDevicePathProtocolGuid,
        NULL,
        gImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_TEST_PROTOCOL) == EFI_SUCCESS) {
          continue;
      }
      if (PerformSingleMappingDelete(Specific,HandleBuffer[LoopVar]) == SHELL_SUCCESS) {
          Deleted = TRUE;
      }
    }
  }
  SHELL_FREE_NON_NULL(HandleBuffer);
  if (!Deleted) {
    return (EFI_NOT_FOUND);
  }
  return (EFI_SUCCESS);
}

/**
  function to add a mapping from mapping.

  This function will get the device path associated with the mapping and call SetMap.

  @param[in] Map         The Map to add a mapping for
  @param[in] SName       The name of the new mapping

  @retval SHELL_SUCCESS             the mapping was added
  @retval SHELL_INVALID_PARAMETER   the device path for Map could not be retrieved.
  @return                           Shell version of a return value from EfiShellProtocol->SetMap

**/
SHELL_STATUS
AddMappingFromMapping(
  IN CONST CHAR16     *Map,
  IN CONST CHAR16     *SName
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_STATUS                      Status;
  CHAR16                          *NewSName;
  RETURN_STATUS                   StrRetStatus;

  NewSName = AllocateCopyPool(StrSize(SName) + sizeof(CHAR16), SName);
  if (NewSName == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }
  if (NewSName[StrLen(NewSName)-1] != L':') {
    StrRetStatus = StrnCatS(NewSName, (StrSize(SName) + sizeof(CHAR16))/sizeof(CHAR16), L":", StrLen(L":"));
    if (EFI_ERROR(StrRetStatus)) {
      FreePool(NewSName);
      return ((SHELL_STATUS) (StrRetStatus & (~MAX_BIT)));
    }
  }

  if (!IsNumberLetterOnly(NewSName, StrLen(NewSName)-1)) {
    FreePool(NewSName);
    return (SHELL_INVALID_PARAMETER);
  }

  DevPath = gEfiShellProtocol->GetDevicePathFromMap(Map);
  if (DevPath == NULL) {
    FreePool(NewSName);
    return (SHELL_INVALID_PARAMETER);
  }

  Status = gEfiShellProtocol->SetMap(DevPath, NewSName);
  FreePool(NewSName);
  if (EFI_ERROR(Status)) {
    return (SHELL_DEVICE_ERROR);
  }
  return (SHELL_SUCCESS);
}

/**
  function to add a mapping from an EFI_HANDLE.

  This function will get the device path associated with the Handle and call SetMap.

  @param[in] Handle       The handle to add a mapping for
  @param[in] SName        The name of the new mapping

  @retval SHELL_SUCCESS           the mapping was added
  @retval SHELL_INVALID_PARAMETER SName was not valid for a map name.
  @return                         Shell version of a return value from either
                                  gBS->OpenProtocol or EfiShellProtocol->SetMap

**/
SHELL_STATUS
AddMappingFromHandle(
  IN CONST EFI_HANDLE Handle,
  IN CONST CHAR16     *SName
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_STATUS                Status;
  CHAR16                    *NewSName;
  RETURN_STATUS             StrRetStatus;

  NewSName = AllocateCopyPool(StrSize(SName) + sizeof(CHAR16), SName);
  if (NewSName == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }
  if (NewSName[StrLen(NewSName)-1] != L':') {
    StrRetStatus = StrnCatS(NewSName, (StrSize(SName) + sizeof(CHAR16))/sizeof(CHAR16), L":", StrLen(L":"));
    if (EFI_ERROR(StrRetStatus)) {
      FreePool(NewSName);
      return ((SHELL_STATUS) (StrRetStatus & (~MAX_BIT)));
    }
  }

  if (!IsNumberLetterOnly(NewSName, StrLen(NewSName)-1)) {
    FreePool(NewSName);
    return (SHELL_INVALID_PARAMETER);
  }

  Status = gBS->OpenProtocol(
    Handle,
    &gEfiDevicePathProtocolGuid,
    (VOID**)&DevPath,
    gImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
   );
  if (EFI_ERROR(Status)) {
    FreePool(NewSName);
    return (SHELL_DEVICE_ERROR);
  }
  Status = gEfiShellProtocol->SetMap(DevPath, NewSName);
  FreePool(NewSName);
  if (EFI_ERROR(Status)) {
    return (SHELL_DEVICE_ERROR);
  }
  return (SHELL_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM MapParamList[] = {
  {L"-d", TypeValue},
  {L"-r", TypeFlag},
  {L"-v", TypeFlag},
  {L"-c", TypeFlag},
  {L"-f", TypeFlag},
  {L"-u", TypeFlag},
  {L"-t", TypeValue},
  {L"-sfo", TypeValue},
  {NULL, TypeMax}
  };

/**
  The routine issues dummy read for every physical block device to cause
  the BlockIo re-installed if media change happened.
**/
VOID
ProbeForMediaChange (
  VOID
  )
{
  EFI_STATUS                            Status;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *Handles;
  EFI_BLOCK_IO_PROTOCOL                 *BlockIo;
  UINTN                                 Index;

  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiBlockIoProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  //
  // Probe for media change for every physical block io
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo
                    );
    if (!EFI_ERROR (Status)) {
      if (!BlockIo->Media->LogicalPartition) {
        //
        // Per spec:
        //   The function (ReadBlocks) must return EFI_NO_MEDIA or
        //   EFI_MEDIA_CHANGED even if LBA, BufferSize, or Buffer are invalid so the caller can probe
        //   for changes in media state.
        //
        BlockIo->ReadBlocks (
                   BlockIo,
                   BlockIo->Media->MediaId,
                   0,
                   0,
                   NULL
                   );
      }
    }
  }
}

/**
  Function for 'map' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMap (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  CONST CHAR16  *SName;
  CONST CHAR16  *Mapping;
  EFI_HANDLE    MapAsHandle;
  SHELL_STATUS  ShellStatus;
  BOOLEAN       SfoMode;
  BOOLEAN       ConstMode;
  BOOLEAN       NormlMode;
  CONST CHAR16  *Param1;
  CONST CHAR16  *TypeString;
  UINTN         TempStringLength;

  ProblemParam  = NULL;
  Mapping       = NULL;
  SName         = NULL;
  ShellStatus   = SHELL_SUCCESS;
  MapAsHandle = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (MapParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"map", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    SfoMode   = ShellCommandLineGetFlag(Package, L"-sfo");
    ConstMode = ShellCommandLineGetFlag(Package, L"-c");
    NormlMode = ShellCommandLineGetFlag(Package, L"-f");
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    } else if (ShellCommandLineGetRawValue(Package, 3) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"map");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // Deleting a map name...
      //
      if (ShellCommandLineGetFlag(Package, L"-d")) {
        if ( ShellCommandLineGetFlag(Package, L"-r")
          || ShellCommandLineGetFlag(Package, L"-v")
          || ConstMode
          || NormlMode
          || ShellCommandLineGetFlag(Package, L"-u")
          || ShellCommandLineGetFlag(Package, L"-t")
         ){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CON), gShellLevel2HiiHandle, L"map");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          SName = ShellCommandLineGetValue(Package, L"-d");
          if (SName != NULL) {
            Status = PerformMappingDelete(SName);
            if (EFI_ERROR(Status)) {
              if (Status == EFI_ACCESS_DENIED) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellLevel2HiiHandle, L"map");
                ShellStatus = SHELL_ACCESS_DENIED;
              } else if (Status == EFI_NOT_FOUND) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MAP_NF), gShellLevel2HiiHandle, L"map", SName);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel2HiiHandle, L"map", Status);
                ShellStatus = SHELL_UNSUPPORTED;
              }
            }
          } else {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"map");
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        }
      } else if ( ShellCommandLineGetFlag(Package, L"-r")
//               || ShellCommandLineGetFlag(Package, L"-v")
               || ConstMode
               || NormlMode
               || ShellCommandLineGetFlag(Package, L"-u")
               || ShellCommandLineGetFlag(Package, L"-t")
              ){
        ProbeForMediaChange ();
        if ( ShellCommandLineGetFlag(Package, L"-r")) {
          //
          // Do the reset
          //
          Status = ShellCommandCreateInitialMappingsAndPaths();
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel2HiiHandle, L"map", Status);
            ShellStatus = SHELL_UNSUPPORTED;
          }
        }
        if ( ShellStatus == SHELL_SUCCESS && ShellCommandLineGetFlag(Package, L"-u")) {
          //
          // Do the Update
          //
          Status = ShellCommandUpdateMapping ();
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel2HiiHandle, L"map", Status);
            ShellStatus = SHELL_UNSUPPORTED;
          }
        }
        if (ShellStatus == SHELL_SUCCESS) {
          Param1 = ShellCommandLineGetRawValue(Package, 1);
          TypeString = ShellCommandLineGetValue(Package, L"-t");
          if (!ConstMode
            &&!NormlMode
            &&TypeString  == NULL
            ) {
            //
            // now do the display...
            //
            ShellStatus = PerformMappingDisplay(
              ShellCommandLineGetFlag(Package, L"-v"),
              TRUE,
              TRUE,
              NULL,
              SfoMode,
              Param1,
              TRUE
             );
          } else {
            //
            // now do the display...
            //
            ShellStatus = PerformMappingDisplay2(
              ShellCommandLineGetFlag(Package, L"-v"),
              ConstMode,
              NormlMode,
              TypeString,
              SfoMode,
              Param1
             );
          }
        }
      } else {
        //
        // adding or displaying (there were no flags)
        //
        SName = ShellCommandLineGetRawValue(Package, 1);
        Mapping = ShellCommandLineGetRawValue(Package, 2);
        if ( SName == NULL
          && Mapping == NULL
         ){
          //
          // display only since no flags
          //
          ShellStatus = PerformMappingDisplay(
            ShellCommandLineGetFlag(Package, L"-v"),
            TRUE,
            TRUE,
            NULL,
            SfoMode,
            NULL,
            TRUE
           );
        } else if ( SName == NULL
          || Mapping == NULL
         ){
            //
            // Display only the one specified
            //
          ShellStatus = PerformMappingDisplay(
            FALSE,
            FALSE,
            FALSE,
            NULL,
            SfoMode,
            SName, // note the variable here...
            TRUE
           );
        } else {
          if (ShellIsHexOrDecimalNumber(Mapping, TRUE, FALSE)) {
            MapAsHandle = ConvertHandleIndexToHandle(ShellStrToUintn(Mapping));
          } else {
            MapAsHandle = NULL;
          }
          if (MapAsHandle == NULL && Mapping[StrLen(Mapping)-1] != L':') {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle, L"map", Mapping);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            TempStringLength = StrLen(SName);
            if (!IsNumberLetterOnly(SName, TempStringLength-(SName[TempStringLength-1]==L':'?1:0))) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle, L"map", SName);
              ShellStatus = SHELL_INVALID_PARAMETER;
            }

            if (ShellStatus == SHELL_SUCCESS) {
              if (MapAsHandle != NULL) {
                ShellStatus = AddMappingFromHandle(MapAsHandle, SName);
              } else {
                ShellStatus = AddMappingFromMapping(Mapping, SName);
              }

              if (ShellStatus != SHELL_SUCCESS) {
                switch (ShellStatus) {
                  case SHELL_ACCESS_DENIED:
                    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellLevel2HiiHandle, L"map");
                    break;
                  case SHELL_INVALID_PARAMETER:
                    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle, L"map", Mapping);
                    break;
                  case SHELL_DEVICE_ERROR:
                    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MAP_NOF), gShellLevel2HiiHandle, L"map", Mapping);
                    break;
                  default:
                    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel2HiiHandle, L"map", ShellStatus|MAX_BIT);
                }
              } else {
                //
                // now do the display...
                //
                ShellStatus = PerformMappingDisplay(
                  FALSE,
                  FALSE,
                  FALSE,
                  NULL,
                  SfoMode,
                  SName,
                  TRUE
                 );
              } // we were sucessful so do an output
            }
          } // got a valid map target
        } // got 2 variables
      } // we are adding a mapping
    } // got valid parameters
  }

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}


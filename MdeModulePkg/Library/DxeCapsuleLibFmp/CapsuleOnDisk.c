/** @file
  The implementation supports Capusle on Disk.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleOnDisk.h"

/**
  Return if this capsule is a capsule name capsule, based upon CapsuleHeader.

  @param[in] CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval TRUE  It is a capsule name capsule.
  @retval FALSE It is not a capsule name capsule.
**/
BOOLEAN
IsCapsuleNameCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  );

/**
  Check the integrity of the capsule name capsule.
  If the capsule is vaild, return the physical address of each capsule name string.

  This routine assumes the capsule has been validated by IsValidCapsuleHeader(), so
  capsule memory overflow is not going to happen in this routine.

  @param[in]  CapsuleHeader   Pointer to the capsule header of a capsule name capsule.
  @param[out] CapsuleNameNum  Number of capsule name.

  @retval NULL                Capsule name capsule is not valid.
  @retval CapsuleNameBuf      Array of capsule name physical address.

**/
EFI_PHYSICAL_ADDRESS *
ValidateCapsuleNameCapsuleIntegrity (
  IN  EFI_CAPSULE_HEADER  *CapsuleHeader,
  OUT UINTN               *CapsuleNameNum
  )
{
  UINT8                 *CapsuleNamePtr;
  UINT8                 *CapsuleNameBufStart;
  UINT8                 *CapsuleNameBufEnd;
  UINTN                 Index;
  UINTN                 StringSize;
  EFI_PHYSICAL_ADDRESS  *CapsuleNameBuf;

  if (!IsCapsuleNameCapsule (CapsuleHeader)) {
    return NULL;
  }

  //
  // Total string size must be even.
  //
  if (((CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize) & BIT0) != 0) {
    return NULL;
  }

  *CapsuleNameNum     = 0;
  Index               = 0;
  CapsuleNameBufStart = (UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize;

  //
  // If strings are not aligned on a 16-bit boundary, reallocate memory for it.
  //
  if (((UINTN)CapsuleNameBufStart & BIT0) != 0) {
    CapsuleNameBufStart = AllocateCopyPool (CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize, CapsuleNameBufStart);
    if (CapsuleNameBufStart == NULL) {
      return NULL;
    }
  }

  CapsuleNameBufEnd = CapsuleNameBufStart + CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize;

  CapsuleNamePtr = CapsuleNameBufStart;
  while (CapsuleNamePtr < CapsuleNameBufEnd) {
    StringSize      = StrnSizeS ((CHAR16 *)CapsuleNamePtr, (CapsuleNameBufEnd - CapsuleNamePtr)/sizeof (CHAR16));
    CapsuleNamePtr += StringSize;
    (*CapsuleNameNum)++;
  }

  //
  // Integrity check.
  //
  if (CapsuleNamePtr != CapsuleNameBufEnd) {
    if (CapsuleNameBufStart != (UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize) {
      FreePool (CapsuleNameBufStart);
    }

    return NULL;
  }

  CapsuleNameBuf = AllocatePool (*CapsuleNameNum * sizeof (EFI_PHYSICAL_ADDRESS));
  if (CapsuleNameBuf == NULL) {
    if (CapsuleNameBufStart != (UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize) {
      FreePool (CapsuleNameBufStart);
    }

    return NULL;
  }

  CapsuleNamePtr = CapsuleNameBufStart;
  while (CapsuleNamePtr < CapsuleNameBufEnd) {
    StringSize            = StrnSizeS ((CHAR16 *)CapsuleNamePtr, (CapsuleNameBufEnd - CapsuleNamePtr)/sizeof (CHAR16));
    CapsuleNameBuf[Index] = (EFI_PHYSICAL_ADDRESS)(UINTN)CapsuleNamePtr;
    CapsuleNamePtr       += StringSize;
    Index++;
  }

  return CapsuleNameBuf;
}

/**
  This routine is called to upper case given unicode string.

  @param[in]   Str              String to upper case

  @retval upper cased string after process

**/
static
CHAR16 *
UpperCaseString (
  IN CHAR16  *Str
  )
{
  CHAR16  *Cptr;

  for (Cptr = Str; *Cptr != L'\0'; Cptr++) {
    if ((L'a' <= *Cptr) && (*Cptr <= L'z')) {
      *Cptr = *Cptr - L'a' + L'A';
    }
  }

  return Str;
}

/**
  This routine is used to return substring before period '.' or '\0'
  Caller should respsonsible of substr space allocation & free

  @param[in]   Str              String to check
  @param[out]  SubStr           First part of string before period or '\0'
  @param[out]  SubStrLen        Length of first part of string

**/
static
VOID
GetSubStringBeforePeriod (
  IN  CHAR16  *Str,
  OUT CHAR16  *SubStr,
  OUT UINTN   *SubStrLen
  )
{
  UINTN  Index;

  for (Index = 0; Str[Index] != L'.' && Str[Index] != L'\0'; Index++) {
    SubStr[Index] = Str[Index];
  }

  SubStr[Index] = L'\0';
  *SubStrLen    = Index;
}

/**
  This routine pad the string in tail with input character.

  @param[in]   StrBuf            Str buffer to be padded, should be enough room for
  @param[in]   PadLen            Expected padding length
  @param[in]   Character         Character used to pad

**/
static
VOID
PadStrInTail (
  IN CHAR16  *StrBuf,
  IN UINTN   PadLen,
  IN CHAR16  Character
  )
{
  UINTN  Index;

  for (Index = 0; StrBuf[Index] != L'\0'; Index++) {
  }

  while (PadLen != 0) {
    StrBuf[Index] = Character;
    Index++;
    PadLen--;
  }

  StrBuf[Index] = L'\0';
}

/**
  This routine find the offset of the last period '.' of string. If No period exists
  function FileNameExtension is set to L'\0'

  @param[in]  FileName           File name to split between last period
  @param[out] FileNameFirst      First FileName before last period
  @param[out] FileNameExtension  FileName after last period

**/
static
VOID
SplitFileNameExtension (
  IN CHAR16   *FileName,
  OUT CHAR16  *FileNameFirst,
  OUT CHAR16  *FileNameExtension
  )
{
  UINTN  Index;
  UINTN  StringLen;

  StringLen = StrnLenS (FileName, MAX_FILE_NAME_SIZE);
  for (Index = StringLen; Index > 0 && FileName[Index] != L'.'; Index--) {
  }

  //
  // No period exists. No FileName Extension
  //
  if ((Index == 0) && (FileName[Index] != L'.')) {
    FileNameExtension[0] = L'\0';
    Index                = StringLen;
  } else {
    StrCpyS (FileNameExtension, MAX_FILE_NAME_SIZE, &FileName[Index+1]);
  }

  //
  // Copy First file name
  //
  StrnCpyS (FileNameFirst, MAX_FILE_NAME_SIZE, FileName, Index);
  FileNameFirst[Index] = L'\0';
}

/**
  This routine is called to get all boot options in the order determnined by:
    1. "OptionBuf"
    2. "BootOrder"

  @param[out] OptionBuf           BootList buffer to all boot options returned
  @param[out] OptionCount         BootList count of all boot options returned

  @retval EFI_SUCCESS             There is no error when processing capsule

**/
EFI_STATUS
GetBootOptionInOrder (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION  **OptionBuf,
  OUT UINTN                         *OptionCount
  )
{
  EFI_STATUS                    Status;
  UINTN                         DataSize;
  UINT16                        BootNext;
  CHAR16                        BootOptionName[20];
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOrderOptionBuf;
  UINTN                         BootOrderCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootNextOptionEntry;
  UINTN                         BootNextCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *TempBuf;

  BootOrderOptionBuf = NULL;
  TempBuf            = NULL;
  BootNextCount      = 0;
  BootOrderCount     = 0;
  *OptionBuf         = NULL;
  *OptionCount       = 0;

  //
  // First Get BootOption from "BootNext"
  //
  DataSize = sizeof (BootNext);
  Status   = gRT->GetVariable (
                    EFI_BOOT_NEXT_VARIABLE_NAME,
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &DataSize,
                    (VOID *)&BootNext
                    );
  //
  // BootNext variable is a single UINT16
  //
  if (!EFI_ERROR (Status) && (DataSize == sizeof (UINT16))) {
    //
    // Add the boot next boot option
    //
    UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04x", BootNext);
    ZeroMem (&BootNextOptionEntry, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
    Status = EfiBootManagerVariableToLoadOption (BootOptionName, &BootNextOptionEntry);

    if (!EFI_ERROR (Status)) {
      BootNextCount = 1;
    }
  }

  //
  // Second get BootOption from "BootOrder"
  //
  BootOrderOptionBuf = EfiBootManagerGetLoadOptions (&BootOrderCount, LoadOptionTypeBoot);
  if ((BootNextCount == 0) && (BootOrderCount == 0)) {
    return EFI_NOT_FOUND;
  }

  //
  // At least one BootOption is found
  //
  TempBuf = AllocatePool (sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (BootNextCount + BootOrderCount));
  if (TempBuf != NULL) {
    if (BootNextCount == 1) {
      CopyMem (TempBuf, &BootNextOptionEntry, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
    }

    if (BootOrderCount > 0) {
      CopyMem (TempBuf + BootNextCount, BootOrderOptionBuf, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * BootOrderCount);
    }

    *OptionBuf   = TempBuf;
    *OptionCount = BootNextCount + BootOrderCount;
    Status       = EFI_SUCCESS;
  } else {
    Status = EFI_OUT_OF_RESOURCES;
  }

  FreePool (BootOrderOptionBuf);

  return Status;
}

/**
  This routine is called to get boot option by OptionNumber.

  @param[in] Number               The OptionNumber of boot option
  @param[out] OptionBuf           BootList buffer to all boot options returned

  @retval EFI_SUCCESS             There is no error when getting boot option

**/
EFI_STATUS
GetBootOptionByNumber (
  IN  UINT16                        Number,
  OUT EFI_BOOT_MANAGER_LOAD_OPTION  **OptionBuf
  )
{
  EFI_STATUS                    Status;
  CHAR16                        BootOptionName[20];
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;

  UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04x", Number);
  ZeroMem (&BootOption, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
  Status = EfiBootManagerVariableToLoadOption (BootOptionName, &BootOption);

  if (!EFI_ERROR (Status)) {
    *OptionBuf = AllocatePool (sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
    if (*OptionBuf != NULL) {
      CopyMem (*OptionBuf, &BootOption, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  return Status;
}

/**
  Get Active EFI System Partition within GPT based on device path.

  @param[in] DevicePath    Device path to find a active EFI System Partition
  @param[out] FsHandle     BootList points to all boot options returned

  @retval EFI_SUCCESS      Active EFI System Partition is succesfully found
  @retval EFI_NOT_FOUND    No Active EFI System Partition is found

**/
EFI_STATUS
GetEfiSysPartitionFromDevPath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT EFI_HANDLE               *FsHandle
  )
{
  EFI_STATUS                       Status;
  EFI_DEVICE_PATH_PROTOCOL         *TempDevicePath;
  HARDDRIVE_DEVICE_PATH            *Hd;
  EFI_HANDLE                       Handle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;

  //
  // Check if the device path contains GPT node
  //
  TempDevicePath = DevicePath;
  while (!IsDevicePathEnd (TempDevicePath)) {
    if ((DevicePathType (TempDevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (TempDevicePath) == MEDIA_HARDDRIVE_DP))
    {
      Hd = (HARDDRIVE_DEVICE_PATH *)TempDevicePath;
      if (Hd->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER) {
        break;
      }
    }

    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }

  if (!IsDevicePathEnd (TempDevicePath)) {
    //
    // Search for EFI system partition protocol on full device path in Boot Option
    //
    Status = gBS->LocateDevicePath (&gEfiPartTypeSystemPartGuid, &DevicePath, &Handle);

    //
    // Search for simple file system on this handler
    //
    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
      if (!EFI_ERROR (Status)) {
        *FsHandle = Handle;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This routine is called to get Simple File System protocol on the first EFI system partition found in
  active boot option. The boot option list is detemined in order by
    1. "BootNext"
    2. "BootOrder"

  @param[in]       MaxRetry           Max Connection Retry. Stall 100ms between each connection try to ensure
                                      device like USB can get enumerated.
  @param[in, out]  LoadOptionNumber   On input, specify the boot option to get EFI system partition.
                                      On output, return the OptionNumber of the boot option where EFI
                                      system partition is got from.
  @param[out]      FsFsHandle         Simple File System Protocol found on first active EFI system partition

  @retval EFI_SUCCESS     Simple File System protocol found for EFI system partition
  @retval EFI_NOT_FOUND   No Simple File System protocol found for EFI system partition

**/
EFI_STATUS
GetEfiSysPartitionFromActiveBootOption (
  IN UINTN        MaxRetry,
  IN OUT UINT16   **LoadOptionNumber,
  OUT EFI_HANDLE  *FsHandle
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptionBuf;
  UINTN                         BootOptionNum;
  UINTN                         Index;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *CurFullPath;
  EFI_DEVICE_PATH_PROTOCOL      *PreFullPath;

  *FsHandle   = NULL;
  CurFullPath = NULL;

  if (*LoadOptionNumber != NULL) {
    BootOptionNum = 1;
    Status        = GetBootOptionByNumber (**LoadOptionNumber, &BootOptionBuf);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "GetBootOptionByIndex Failed %x! No BootOption available for connection\n", Status));
      return Status;
    }
  } else {
    Status = GetBootOptionInOrder (&BootOptionBuf, &BootOptionNum);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "GetBootOptionInOrder Failed %x! No BootOption available for connection\n", Status));
      return Status;
    }
  }

  //
  // Search BootOptionList to check if it is an active boot option with EFI system partition
  //  1. Connect device path
  //  2. expend short/plug in devicepath
  //  3. LoadImage
  //
  for (Index = 0; Index < BootOptionNum; Index++) {
    //
    // Get the boot option from the link list
    //
    DevicePath = BootOptionBuf[Index].FilePath;

    //
    // Skip inactive or legacy boot options
    //
    if (((BootOptionBuf[Index].Attributes & LOAD_OPTION_ACTIVE) == 0) ||
        (DevicePathType (DevicePath) == BBS_DEVICE_PATH))
    {
      continue;
    }

    DEBUG_CODE_BEGIN ();
    CHAR16  *DevicePathStr;

    DevicePathStr = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
    if (DevicePathStr != NULL) {
      DEBUG ((DEBUG_INFO, "Try BootOption %s\n", DevicePathStr));
      FreePool (DevicePathStr);
    } else {
      DEBUG ((DEBUG_INFO, "DevicePathToStr failed\n"));
    }

    DEBUG_CODE_END ();

    CurFullPath = NULL;
    //
    // Try every full device Path generated from bootoption
    //
    do {
      PreFullPath = CurFullPath;
      CurFullPath = EfiBootManagerGetNextLoadOptionDevicePath (DevicePath, CurFullPath);

      if (PreFullPath != NULL) {
        FreePool (PreFullPath);
      }

      if (CurFullPath == NULL) {
        //
        // No Active EFI system partition is found in BootOption device path
        //
        Status = EFI_NOT_FOUND;
        break;
      }

      DEBUG_CODE_BEGIN ();
      CHAR16  *DevicePathStr1;

      DevicePathStr1 = ConvertDevicePathToText (CurFullPath, TRUE, TRUE);
      if (DevicePathStr1 != NULL) {
        DEBUG ((DEBUG_INFO, "Full device path %s\n", DevicePathStr1));
        FreePool (DevicePathStr1);
      }

      DEBUG_CODE_END ();

      //
      // Make sure the boot option device path connected.
      // Only handle first device in boot option. Other optional device paths are described as OSV specific
      // FullDevice could contain extra directory & file info. So don't check connection status here.
      //
      EfiBootManagerConnectDevicePath (CurFullPath, NULL);
      Status = GetEfiSysPartitionFromDevPath (CurFullPath, FsHandle);

      //
      // Some relocation device like USB need more time to get enumerated
      //
      while (EFI_ERROR (Status) && MaxRetry > 0) {
        EfiBootManagerConnectDevicePath (CurFullPath, NULL);

        //
        // Search for EFI system partition protocol on full device path in Boot Option
        //
        Status = GetEfiSysPartitionFromDevPath (CurFullPath, FsHandle);
        if (!EFI_ERROR (Status)) {
          break;
        }

        DEBUG ((DEBUG_ERROR, "GetEfiSysPartitionFromDevPath Loop %x\n", Status));
        //
        // Stall 100ms if connection failed to ensure USB stack is ready
        //
        gBS->Stall (100000);
        MaxRetry--;
      }
    } while (EFI_ERROR (Status));

    //
    // Find a qualified Simple File System
    //
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  //
  // Return the OptionNumber of the boot option where EFI system partition is got from
  //
  if (*LoadOptionNumber == NULL) {
    *LoadOptionNumber = AllocateCopyPool (sizeof (UINT16), (UINT16 *)&BootOptionBuf[Index].OptionNumber);
    if (*LoadOptionNumber == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // No qualified EFI system partition found
  //
  if (*FsHandle == NULL) {
    Status = EFI_NOT_FOUND;
  }

  DEBUG_CODE_BEGIN ();
  CHAR16  *DevicePathStr2;

  if (*FsHandle != NULL) {
    DevicePathStr2 = ConvertDevicePathToText (CurFullPath, TRUE, TRUE);
    if (DevicePathStr2 != NULL) {
      DEBUG ((DEBUG_INFO, "Found Active EFI System Partion on %s\n", DevicePathStr2));
      FreePool (DevicePathStr2);
    }
  } else {
    DEBUG ((DEBUG_INFO, "Failed to found Active EFI System Partion\n"));
  }

  DEBUG_CODE_END ();

  if (CurFullPath != NULL) {
    FreePool (CurFullPath);
  }

  //
  // Free BootOption Buffer
  //
  for (Index = 0; Index < BootOptionNum; Index++) {
    if (BootOptionBuf[Index].Description != NULL) {
      FreePool (BootOptionBuf[Index].Description);
    }

    if (BootOptionBuf[Index].FilePath != NULL) {
      FreePool (BootOptionBuf[Index].FilePath);
    }

    if (BootOptionBuf[Index].OptionalData != NULL) {
      FreePool (BootOptionBuf[Index].OptionalData);
    }
  }

  FreePool (BootOptionBuf);

  return Status;
}

/**
  This routine is called to get all file infos with in a given dir & with given file attribute, the file info is listed in
  alphabetical order described in UEFI spec.

  @param[in]  Dir                 Directory file handler
  @param[in]  FileAttr            Attribute of file to be red from directory
  @param[out] FileInfoList        File images info list red from directory
  @param[out] FileNum             File images number red from directory

  @retval EFI_SUCCESS             File FileInfo list in the given

**/
EFI_STATUS
GetFileInfoListInAlphabetFromDir (
  IN EFI_FILE_HANDLE  Dir,
  IN UINT64           FileAttr,
  OUT LIST_ENTRY      *FileInfoList,
  OUT UINTN           *FileNum
  )
{
  EFI_STATUS       Status;
  FILE_INFO_ENTRY  *NewFileInfoEntry;
  FILE_INFO_ENTRY  *TempFileInfoEntry;
  EFI_FILE_INFO    *FileInfo;
  CHAR16           *NewFileName;
  CHAR16           *ListedFileName;
  CHAR16           *NewFileNameExtension;
  CHAR16           *ListedFileNameExtension;
  CHAR16           *TempNewSubStr;
  CHAR16           *TempListedSubStr;
  LIST_ENTRY       *Link;
  BOOLEAN          NoFile;
  UINTN            FileCount;
  UINTN            IndexNew;
  UINTN            IndexListed;
  UINTN            NewSubStrLen;
  UINTN            ListedSubStrLen;
  INTN             SubStrCmpResult;

  Status                  = EFI_SUCCESS;
  NewFileName             = NULL;
  ListedFileName          = NULL;
  NewFileNameExtension    = NULL;
  ListedFileNameExtension = NULL;
  TempNewSubStr           = NULL;
  TempListedSubStr        = NULL;
  FileInfo                = NULL;
  NoFile                  = FALSE;
  FileCount               = 0;

  InitializeListHead (FileInfoList);

  TempNewSubStr    = (CHAR16 *)AllocateZeroPool (MAX_FILE_NAME_SIZE);
  TempListedSubStr = (CHAR16 *)AllocateZeroPool (MAX_FILE_NAME_SIZE);

  if ((TempNewSubStr == NULL) || (TempListedSubStr == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  for ( Status = FileHandleFindFirstFile (Dir, &FileInfo)
        ; !EFI_ERROR (Status) && !NoFile
        ; Status = FileHandleFindNextFile (Dir, FileInfo, &NoFile)
        )
  {
    if (FileInfo == NULL) {
      goto EXIT;
    }

    //
    // Skip file with mismatching File attribute
    //
    if ((FileInfo->Attribute & (FileAttr)) == 0) {
      continue;
    }

    NewFileInfoEntry = NULL;
    NewFileInfoEntry = (FILE_INFO_ENTRY *)AllocateZeroPool (sizeof (FILE_INFO_ENTRY));
    if (NewFileInfoEntry == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    NewFileInfoEntry->Signature = FILE_INFO_SIGNATURE;
    NewFileInfoEntry->FileInfo  = AllocateCopyPool ((UINTN)FileInfo->Size, FileInfo);
    if (NewFileInfoEntry->FileInfo == NULL) {
      FreePool (NewFileInfoEntry);
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    NewFileInfoEntry->FileNameFirstPart = (CHAR16 *)AllocateZeroPool (MAX_FILE_NAME_SIZE);
    if (NewFileInfoEntry->FileNameFirstPart == NULL) {
      FreePool (NewFileInfoEntry->FileInfo);
      FreePool (NewFileInfoEntry);
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    NewFileInfoEntry->FileNameSecondPart = (CHAR16 *)AllocateZeroPool (MAX_FILE_NAME_SIZE);
    if (NewFileInfoEntry->FileNameSecondPart == NULL) {
      FreePool (NewFileInfoEntry->FileInfo);
      FreePool (NewFileInfoEntry->FileNameFirstPart);
      FreePool (NewFileInfoEntry);
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    //
    // Splitter the whole New file name into 2 parts between the last period L'.' into NewFileName NewFileExtension
    // If no period in the whole file name. NewFileExtension is set to L'\0'
    //
    NewFileName          = NewFileInfoEntry->FileNameFirstPart;
    NewFileNameExtension = NewFileInfoEntry->FileNameSecondPart;
    SplitFileNameExtension (FileInfo->FileName, NewFileName, NewFileNameExtension);
    UpperCaseString (NewFileName);
    UpperCaseString (NewFileNameExtension);

    //
    // Insert capsule file in alphabetical ordered list
    //
    for (Link = FileInfoList->ForwardLink; Link != FileInfoList; Link = Link->ForwardLink) {
      //
      // Get the FileInfo from the link list
      //
      TempFileInfoEntry       = CR (Link, FILE_INFO_ENTRY, Link, FILE_INFO_SIGNATURE);
      ListedFileName          = TempFileInfoEntry->FileNameFirstPart;
      ListedFileNameExtension = TempFileInfoEntry->FileNameSecondPart;

      //
      // Follow rule in UEFI spec 8.5.5 to compare file name
      //
      IndexListed = 0;
      IndexNew    = 0;
      while (TRUE) {
        //
        // First compare each substrings in NewFileName & ListedFileName between periods
        //
        GetSubStringBeforePeriod (&NewFileName[IndexNew], TempNewSubStr, &NewSubStrLen);
        GetSubStringBeforePeriod (&ListedFileName[IndexListed], TempListedSubStr, &ListedSubStrLen);
        if (NewSubStrLen > ListedSubStrLen) {
          //
          // Substr in NewFileName is longer. Pad tail with SPACE
          //
          PadStrInTail (TempListedSubStr, NewSubStrLen - ListedSubStrLen, L' ');
        } else if (NewSubStrLen < ListedSubStrLen) {
          //
          // Substr in ListedFileName is longer. Pad tail with SPACE
          //
          PadStrInTail (TempNewSubStr, ListedSubStrLen - NewSubStrLen, L' ');
        }

        SubStrCmpResult = StrnCmp (TempNewSubStr, TempListedSubStr, MAX_FILE_NAME_LEN);
        if (SubStrCmpResult != 0) {
          break;
        }

        //
        // Move to skip this substring
        //
        IndexNew    += NewSubStrLen;
        IndexListed += ListedSubStrLen;
        //
        // Reach File First Name end
        //
        if ((NewFileName[IndexNew] == L'\0') || (ListedFileName[IndexListed] == L'\0')) {
          break;
        }

        //
        // Skip the period L'.'
        //
        IndexNew++;
        IndexListed++;
      }

      if (SubStrCmpResult < 0) {
        //
        // NewFileName is smaller. Find the right place to insert New file
        //
        break;
      } else if (SubStrCmpResult == 0) {
        //
        // 2 cases whole NewFileName is smaller than ListedFileName
        //   1. if NewFileName == ListedFileName. Continue to compare FileNameExtension
        //   2. if NewFileName is shorter than ListedFileName
        //
        if (NewFileName[IndexNew] == L'\0') {
          if ((ListedFileName[IndexListed] != L'\0') || (StrnCmp (NewFileNameExtension, ListedFileNameExtension, MAX_FILE_NAME_LEN) < 0)) {
            break;
          }
        }
      }

      //
      // Other case, ListedFileName is smaller. Continue to compare the next file in the list
      //
    }

    //
    // If Find an entry in the list whose name is bigger than new FileInfo in alphabet order
    //    Insert it before this entry
    // else
    //    Insert at the tail of this list (Link = FileInfoList)
    //
    InsertTailList (Link, &NewFileInfoEntry->Link);

    FileCount++;
  }

  *FileNum = FileCount;

EXIT:

  if (TempNewSubStr != NULL) {
    FreePool (TempNewSubStr);
  }

  if (TempListedSubStr != NULL) {
    FreePool (TempListedSubStr);
  }

  if (EFI_ERROR (Status)) {
    while (!IsListEmpty (FileInfoList)) {
      Link = FileInfoList->ForwardLink;
      RemoveEntryList (Link);

      TempFileInfoEntry = CR (Link, FILE_INFO_ENTRY, Link, FILE_INFO_SIGNATURE);

      FreePool (TempFileInfoEntry->FileInfo);
      FreePool (TempFileInfoEntry->FileNameFirstPart);
      FreePool (TempFileInfoEntry->FileNameSecondPart);
      FreePool (TempFileInfoEntry);
    }

    *FileNum = 0;
  }

  return Status;
}

/**
  This routine is called to get all qualified image from file from an given directory
  in alphabetic order. All the file image is copied to allocated boottime memory.
  Caller should free these memory

  @param[in]  Dir            Directory file handler
  @param[in]  FileAttr       Attribute of file to be red from directory
  @param[out] FilePtr        File images Info buffer red from directory
  @param[out] FileNum        File images number red from directory

  @retval EFI_SUCCESS  Succeed to get all capsules in alphabetic order.

**/
EFI_STATUS
GetFileImageInAlphabetFromDir (
  IN EFI_FILE_HANDLE  Dir,
  IN UINT64           FileAttr,
  OUT IMAGE_INFO      **FilePtr,
  OUT UINTN           *FileNum
  )
{
  EFI_STATUS       Status;
  LIST_ENTRY       *Link;
  EFI_FILE_HANDLE  FileHandle;
  FILE_INFO_ENTRY  *FileInfoEntry;
  EFI_FILE_INFO    *FileInfo;
  UINTN            FileCount;
  IMAGE_INFO       *TempFilePtrBuf;
  UINTN            Size;
  LIST_ENTRY       FileInfoList;

  FileHandle     = NULL;
  FileCount      = 0;
  TempFilePtrBuf = NULL;
  *FilePtr       = NULL;

  //
  // Get file list in Dir in alphabetical order
  //
  Status = GetFileInfoListInAlphabetFromDir (
             Dir,
             FileAttr,
             &FileInfoList,
             &FileCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetFileInfoListInAlphabetFromDir Failed!\n"));
    goto EXIT;
  }

  if (FileCount == 0) {
    DEBUG ((DEBUG_ERROR, "No file found in Dir!\n"));
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  TempFilePtrBuf = (IMAGE_INFO *)AllocateZeroPool (sizeof (IMAGE_INFO) * FileCount);
  if (TempFilePtrBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Read all files from FileInfoList to BS memory
  //
  FileCount = 0;
  for (Link = FileInfoList.ForwardLink; Link != &FileInfoList; Link = Link->ForwardLink) {
    //
    // Get FileInfo from the link list
    //
    FileInfoEntry = CR (Link, FILE_INFO_ENTRY, Link, FILE_INFO_SIGNATURE);
    FileInfo      = FileInfoEntry->FileInfo;

    Status = Dir->Open (
                    Dir,
                    &FileHandle,
                    FileInfo->FileName,
                    EFI_FILE_MODE_READ,
                    0
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Size                                   = (UINTN)FileInfo->FileSize;
    TempFilePtrBuf[FileCount].ImageAddress = AllocateZeroPool (Size);
    if (TempFilePtrBuf[FileCount].ImageAddress == NULL) {
      DEBUG ((DEBUG_ERROR, "Fail to allocate memory for capsule. Stop processing the rest.\n"));
      break;
    }

    Status = FileHandle->Read (
                           FileHandle,
                           &Size,
                           TempFilePtrBuf[FileCount].ImageAddress
                           );

    FileHandle->Close (FileHandle);

    //
    // Skip read error file
    //
    if (EFI_ERROR (Status) || (Size != (UINTN)FileInfo->FileSize)) {
      //
      // Remove this error file info accordingly
      // & move Link to BackLink
      //
      Link = RemoveEntryList (Link);
      Link = Link->BackLink;

      FreePool (FileInfoEntry->FileInfo);
      FreePool (FileInfoEntry->FileNameFirstPart);
      FreePool (FileInfoEntry->FileNameSecondPart);
      FreePool (FileInfoEntry);

      FreePool (TempFilePtrBuf[FileCount].ImageAddress);
      TempFilePtrBuf[FileCount].ImageAddress = NULL;
      TempFilePtrBuf[FileCount].FileInfo     = NULL;

      continue;
    }

    TempFilePtrBuf[FileCount].FileInfo = FileInfo;
    FileCount++;
  }

  DEBUG_CODE_BEGIN ();
  for (Link = FileInfoList.ForwardLink; Link != &FileInfoList; Link = Link->ForwardLink) {
    FileInfoEntry = CR (Link, FILE_INFO_ENTRY, Link, FILE_INFO_SIGNATURE);
    FileInfo      = FileInfoEntry->FileInfo;
    DEBUG ((DEBUG_INFO, "Successfully read capsule file %s from disk.\n", FileInfo->FileName));
  }

  DEBUG_CODE_END ();

EXIT:

  *FilePtr = TempFilePtrBuf;
  *FileNum = FileCount;

  //
  // FileInfo will be freed by Calller
  //
  while (!IsListEmpty (&FileInfoList)) {
    Link = FileInfoList.ForwardLink;
    RemoveEntryList (Link);

    FileInfoEntry = CR (Link, FILE_INFO_ENTRY, Link, FILE_INFO_SIGNATURE);

    FreePool (FileInfoEntry->FileNameFirstPart);
    FreePool (FileInfoEntry->FileNameSecondPart);
    FreePool (FileInfoEntry);
  }

  return Status;
}

/**
  This routine is called to remove all qualified image from file from an given directory.

  @param[in] Dir                  Directory file handler
  @param[in] FileAttr             Attribute of files to be deleted

  @retval EFI_SUCCESS  Succeed to remove all files from an given directory.

**/
EFI_STATUS
RemoveFileFromDir (
  IN EFI_FILE_HANDLE  Dir,
  IN UINT64           FileAttr
  )
{
  EFI_STATUS       Status;
  LIST_ENTRY       *Link;
  LIST_ENTRY       FileInfoList;
  EFI_FILE_HANDLE  FileHandle;
  FILE_INFO_ENTRY  *FileInfoEntry;
  EFI_FILE_INFO    *FileInfo;
  UINTN            FileCount;

  FileHandle = NULL;

  //
  // Get file list in Dir in alphabetical order
  //
  Status = GetFileInfoListInAlphabetFromDir (
             Dir,
             FileAttr,
             &FileInfoList,
             &FileCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetFileInfoListInAlphabetFromDir Failed!\n"));
    goto EXIT;
  }

  if (FileCount == 0) {
    DEBUG ((DEBUG_ERROR, "No file found in Dir!\n"));
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  //
  // Delete all files with given attribute in Dir
  //
  for (Link = FileInfoList.ForwardLink; Link != &(FileInfoList); Link = Link->ForwardLink) {
    //
    // Get FileInfo from the link list
    //
    FileInfoEntry = CR (Link, FILE_INFO_ENTRY, Link, FILE_INFO_SIGNATURE);
    FileInfo      = FileInfoEntry->FileInfo;

    Status = Dir->Open (
                    Dir,
                    &FileHandle,
                    FileInfo->FileName,
                    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                    0
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = FileHandle->Delete (FileHandle);
  }

EXIT:

  while (!IsListEmpty (&FileInfoList)) {
    Link = FileInfoList.ForwardLink;
    RemoveEntryList (Link);

    FileInfoEntry = CR (Link, FILE_INFO_ENTRY, Link, FILE_INFO_SIGNATURE);

    FreePool (FileInfoEntry->FileInfo);
    FreePool (FileInfoEntry);
  }

  return Status;
}

/**
  This routine is called to get all caspules from file. The capsule file image is
  copied to BS memory. Caller is responsible to free them.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated.
  @param[out]   CapsulePtr           Copied Capsule file Image Info buffer
  @param[out]   CapsuleNum           CapsuleNumber
  @param[out]   FsHandle             File system handle
  @param[out]   LoadOptionNumber     OptionNumber of boot option

  @retval EFI_SUCCESS  Succeed to get all capsules.

**/
EFI_STATUS
GetAllCapsuleOnDisk (
  IN  UINTN       MaxRetry,
  OUT IMAGE_INFO  **CapsulePtr,
  OUT UINTN       *CapsuleNum,
  OUT EFI_HANDLE  *FsHandle,
  OUT UINT16      *LoadOptionNumber
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_FILE_HANDLE                  RootDir;
  EFI_FILE_HANDLE                  FileDir;
  UINT16                           *TempOptionNumber;

  TempOptionNumber = NULL;
  *CapsuleNum      = 0;

  Status = GetEfiSysPartitionFromActiveBootOption (MaxRetry, &TempOptionNumber, FsHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (*FsHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Fs->OpenVolume (Fs, &RootDir);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = RootDir->Open (
                      RootDir,
                      &FileDir,
                      EFI_CAPSULE_FILE_DIRECTORY,
                      EFI_FILE_MODE_READ,
                      0
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CodLibGetAllCapsuleOnDisk fail to open RootDir!\n"));
    RootDir->Close (RootDir);
    return Status;
  }

  RootDir->Close (RootDir);

  //
  // Only Load files with EFI_FILE_SYSTEM or EFI_FILE_ARCHIVE attribute
  // ignore EFI_FILE_READ_ONLY, EFI_FILE_HIDDEN, EFI_FILE_RESERVED, EFI_FILE_DIRECTORY
  //
  Status = GetFileImageInAlphabetFromDir (
             FileDir,
             EFI_FILE_SYSTEM | EFI_FILE_ARCHIVE,
             CapsulePtr,
             CapsuleNum
             );
  DEBUG ((DEBUG_INFO, "GetFileImageInAlphabetFromDir status %x\n", Status));

  //
  // Always remove file to avoid deadloop in capsule process
  //
  Status = RemoveFileFromDir (FileDir, EFI_FILE_SYSTEM | EFI_FILE_ARCHIVE);
  DEBUG ((DEBUG_INFO, "RemoveFileFromDir status %x\n", Status));

  FileDir->Close (FileDir);

  if (LoadOptionNumber != NULL) {
    *LoadOptionNumber = *TempOptionNumber;
  }

  return Status;
}

/**
  Build Gather list for a list of capsule images.

  @param[in]  CapsuleBuffer    An array of pointer to capsule images
  @param[in]  CapsuleSize      An array of UINTN to capsule images size
  @param[in]  CapsuleNum       The count of capsule images
  @param[out] BlockDescriptors The block descriptors for the capsule images

  @retval EFI_SUCCESS The block descriptors for the capsule images are constructed.

**/
EFI_STATUS
BuildGatherList (
  IN VOID                           **CapsuleBuffer,
  IN UINTN                          *CapsuleSize,
  IN UINTN                          CapsuleNum,
  OUT EFI_CAPSULE_BLOCK_DESCRIPTOR  **BlockDescriptors
  )
{
  EFI_STATUS                    Status;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptors1;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptorPre;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptorsHeader;
  UINTN                         Index;

  BlockDescriptors1      = NULL;
  BlockDescriptorPre     = NULL;
  BlockDescriptorsHeader = NULL;

  for (Index = 0; Index < CapsuleNum; Index++) {
    //
    // Allocate memory for the descriptors.
    //
    BlockDescriptors1 = AllocateZeroPool (2 * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR));
    if (BlockDescriptors1 == NULL) {
      DEBUG ((DEBUG_ERROR, "BuildGatherList: failed to allocate memory for descriptors\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto ERREXIT;
    } else {
      DEBUG ((DEBUG_INFO, "BuildGatherList: creating capsule descriptors at 0x%X\n", (UINTN)BlockDescriptors1));
    }

    //
    // Record descirptor header
    //
    if (Index == 0) {
      BlockDescriptorsHeader = BlockDescriptors1;
    }

    if (BlockDescriptorPre != NULL) {
      BlockDescriptorPre->Union.ContinuationPointer = (UINTN)BlockDescriptors1;
      BlockDescriptorPre->Length                    = 0;
    }

    BlockDescriptors1->Union.DataBlock = (UINTN)CapsuleBuffer[Index];
    BlockDescriptors1->Length          = CapsuleSize[Index];

    BlockDescriptorPre = BlockDescriptors1 + 1;
    BlockDescriptors1  = NULL;
  }

  //
  // Null-terminate.
  //
  if (BlockDescriptorPre != NULL) {
    BlockDescriptorPre->Union.ContinuationPointer = (UINTN)NULL;
    BlockDescriptorPre->Length                    = 0;
    *BlockDescriptors                             = BlockDescriptorsHeader;
  }

  return EFI_SUCCESS;

ERREXIT:
  if (BlockDescriptors1 != NULL) {
    FreePool (BlockDescriptors1);
  }

  return Status;
}

/**
  This routine is called to check if CapsuleOnDisk flag in OsIndications Variable
  is enabled.

  @retval TRUE     Flag is enabled
  @retval FALSE    Flag is not enabled

**/
BOOLEAN
EFIAPI
CoDCheckCapsuleOnDiskFlag (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      OsIndication;
  UINTN       DataSize;

  //
  // Check File Capsule Delivery Supported Flag in OsIndication variable
  //
  OsIndication = 0;
  DataSize     = sizeof (UINT64);
  Status       = gRT->GetVariable (
                        EFI_OS_INDICATIONS_VARIABLE_NAME,
                        &gEfiGlobalVariableGuid,
                        NULL,
                        &DataSize,
                        &OsIndication
                        );
  if (!EFI_ERROR (Status) &&
      ((OsIndication & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  This routine is called to clear CapsuleOnDisk flags including OsIndications and BootNext variable.

  @retval EFI_SUCCESS   All Capsule On Disk flags are cleared

**/
EFI_STATUS
EFIAPI
CoDClearCapsuleOnDiskFlag (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      OsIndication;
  UINTN       DataSize;

  //
  // Reset File Capsule Delivery Supported Flag in OsIndication variable
  //
  OsIndication = 0;
  DataSize     = sizeof (UINT64);
  Status       = gRT->GetVariable (
                        EFI_OS_INDICATIONS_VARIABLE_NAME,
                        &gEfiGlobalVariableGuid,
                        NULL,
                        &DataSize,
                        &OsIndication
                        );
  if (EFI_ERROR (Status) ||
      ((OsIndication & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) == 0))
  {
    return Status;
  }

  OsIndication &= ~((UINT64)EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED);
  Status        = gRT->SetVariable (
                         EFI_OS_INDICATIONS_VARIABLE_NAME,
                         &gEfiGlobalVariableGuid,
                         EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                         sizeof (UINT64),
                         &OsIndication
                         );
  ASSERT (!EFI_ERROR (Status));

  //
  // Delete BootNext variable. Capsule Process may reset system, so can't rely on Bds to clear this variable
  //
  Status = gRT->SetVariable (
                  EFI_BOOT_NEXT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  0,
                  0,
                  NULL
                  );
  ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);

  return EFI_SUCCESS;
}

/**
  This routine is called to clear CapsuleOnDisk Relocation Info variable.
  Total Capsule On Disk length is recorded in this variable

  @retval EFI_SUCCESS   Capsule On Disk flags are cleared

**/
EFI_STATUS
CoDClearCapsuleRelocationInfo (
  VOID
  )
{
  return gRT->SetVariable (
                COD_RELOCATION_INFO_VAR_NAME,
                &gEfiCapsuleVendorGuid,
                0,
                0,
                NULL
                );
}

/**
  Relocate Capsule on Disk from EFI system partition to a platform-specific NV storage device
  with BlockIo protocol. Relocation device path, identified by PcdCodRelocationDevPath, must
  be a full device path.
  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  Side Effects:
    Content corruption. Block IO write directly touches low level write. Orignal partitions, file systems
    of the relocation device will be corrupted.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated.

  @retval EFI_SUCCESS   Capsule on Disk images are sucessfully relocated to the platform-specific device.

**/
EFI_STATUS
RelocateCapsuleToDisk (
  UINTN  MaxRetry
  )
{
  EFI_STATUS                       Status;
  UINTN                            CapsuleOnDiskNum;
  UINTN                            Index;
  UINTN                            DataSize;
  UINT64                           TotalImageSize;
  UINT64                           TotalImageNameSize;
  IMAGE_INFO                       *CapsuleOnDiskBuf;
  EFI_HANDLE                       Handle;
  EFI_HANDLE                       TempHandle;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            NumberOfHandles;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  UINT8                            *CapsuleDataBuf;
  UINT8                            *CapsulePtr;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_FILE_HANDLE                  RootDir;
  EFI_FILE_HANDLE                  TempCodFile;
  UINT64                           TempCodFileSize;
  EFI_DEVICE_PATH                  *TempDevicePath;
  BOOLEAN                          RelocationInfo;
  UINT16                           LoadOptionNumber;
  EFI_CAPSULE_HEADER               FileNameCapsuleHeader;

  RootDir          = NULL;
  TempCodFile      = NULL;
  HandleBuffer     = NULL;
  CapsuleDataBuf   = NULL;
  CapsuleOnDiskBuf = NULL;
  NumberOfHandles  = 0;

  DEBUG ((DEBUG_INFO, "CapsuleOnDisk RelocateCapsule Enter\n"));

  //
  // 1. Load all Capsule On Disks in to memory
  //
  Status = GetAllCapsuleOnDisk (MaxRetry, &CapsuleOnDiskBuf, &CapsuleOnDiskNum, &Handle, &LoadOptionNumber);
  if (EFI_ERROR (Status) || (CapsuleOnDiskNum == 0) || (CapsuleOnDiskBuf == NULL)) {
    DEBUG ((DEBUG_INFO, "RelocateCapsule: GetAllCapsuleOnDisk Status - 0x%x\n", Status));
    return EFI_NOT_FOUND;
  }

  //
  // 2. Connect platform special device path as relocation device.
  // If no platform special device path specified or the device path is invalid, use the EFI system partition where
  // stores the capsules as relocation device.
  //
  if (IsDevicePathValid ((EFI_DEVICE_PATH *)PcdGetPtr (PcdCodRelocationDevPath), PcdGetSize (PcdCodRelocationDevPath))) {
    Status = EfiBootManagerConnectDevicePath ((EFI_DEVICE_PATH *)PcdGetPtr (PcdCodRelocationDevPath), &TempHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "RelocateCapsule: EfiBootManagerConnectDevicePath Status - 0x%x\n", Status));
      goto EXIT;
    }

    //
    // Connect all the child handle. Partition & FAT drivers are allowed in this case
    //
    gBS->ConnectController (TempHandle, NULL, NULL, TRUE);
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiSimpleFileSystemProtocolGuid,
                    NULL,
                    &NumberOfHandles,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "RelocateCapsule: LocateHandleBuffer Status - 0x%x\n", Status));
      goto EXIT;
    }

    //
    // Find first Simple File System Handle which can match PcdCodRelocationDevPath
    //
    for (Index = 0; Index < NumberOfHandles; Index++) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&TempDevicePath);
      if (EFI_ERROR (Status)) {
        continue;
      }

      DataSize = GetDevicePathSize ((EFI_DEVICE_PATH *)PcdGetPtr (PcdCodRelocationDevPath)) - sizeof (EFI_DEVICE_PATH);
      if (0 == CompareMem ((EFI_DEVICE_PATH *)PcdGetPtr (PcdCodRelocationDevPath), TempDevicePath, DataSize)) {
        Handle = HandleBuffer[Index];
        break;
      }
    }

    FreePool (HandleBuffer);

    if (Index == NumberOfHandles) {
      DEBUG ((DEBUG_ERROR, "RelocateCapsule: No simple file system protocol found.\n"));
      Status = EFI_NOT_FOUND;
    }
  }

  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
  if (EFI_ERROR (Status) || BlockIo->Media->ReadOnly) {
    DEBUG ((DEBUG_ERROR, "Fail to find Capsule on Disk relocation BlockIo device or device is ReadOnly!\n"));
    goto EXIT;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Check if device used to relocate Capsule On Disk is big enough
  //
  TotalImageSize     = 0;
  TotalImageNameSize = 0;
  for (Index = 0; Index < CapsuleOnDiskNum; Index++) {
    //
    // Overflow check
    //
    if (MAX_ADDRESS - (UINTN)TotalImageSize <= CapsuleOnDiskBuf[Index].FileInfo->FileSize) {
      Status = EFI_INVALID_PARAMETER;
      goto EXIT;
    }

    if (MAX_ADDRESS - (UINTN)TotalImageNameSize <= StrSize (CapsuleOnDiskBuf[Index].FileInfo->FileName)) {
      Status = EFI_INVALID_PARAMETER;
      goto EXIT;
    }

    TotalImageSize     += CapsuleOnDiskBuf[Index].FileInfo->FileSize;
    TotalImageNameSize += StrSize (CapsuleOnDiskBuf[Index].FileInfo->FileName);
    DEBUG ((DEBUG_INFO, "RelocateCapsule: %x Size %x\n", CapsuleOnDiskBuf[Index].FileInfo->FileName, CapsuleOnDiskBuf[Index].FileInfo->FileSize));
  }

  DEBUG ((DEBUG_INFO, "RelocateCapsule: TotalImageSize %x\n", TotalImageSize));
  DEBUG ((DEBUG_INFO, "RelocateCapsule: TotalImageNameSize %x\n", TotalImageNameSize));

  if ((MAX_ADDRESS - (UINTN)TotalImageNameSize <= sizeof (UINT64) * 2) ||
      (MAX_ADDRESS - (UINTN)TotalImageSize <= (UINTN)TotalImageNameSize + sizeof (UINT64) * 2))
  {
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  TempCodFileSize = sizeof (UINT64) + TotalImageSize + sizeof (EFI_CAPSULE_HEADER) + TotalImageNameSize;

  //
  // Check if CapsuleTotalSize. There could be reminder, so use LastBlock number directly
  //
  if (DivU64x32 (TempCodFileSize, BlockIo->Media->BlockSize) > BlockIo->Media->LastBlock) {
    DEBUG ((DEBUG_ERROR, "RelocateCapsule: Relocation device isn't big enough to hold all Capsule on Disk!\n"));
    DEBUG ((DEBUG_ERROR, "TotalImageSize = %x\n", TotalImageSize));
    DEBUG ((DEBUG_ERROR, "TotalImageNameSize = %x\n", TotalImageNameSize));
    DEBUG ((DEBUG_ERROR, "RelocationDev BlockSize = %x LastBlock = %x\n", BlockIo->Media->BlockSize, BlockIo->Media->LastBlock));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  CapsuleDataBuf = AllocatePool ((UINTN)TempCodFileSize);
  if (CapsuleDataBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // First UINT64 reserved for total image size, including capsule name capsule.
  //
  *(UINT64 *)CapsuleDataBuf = TotalImageSize + sizeof (EFI_CAPSULE_HEADER) + TotalImageNameSize;

  //
  // Line up all the Capsule on Disk and write to relocation disk at one time. It could save some time in disk write
  //
  for (Index = 0, CapsulePtr = CapsuleDataBuf + sizeof (UINT64); Index < CapsuleOnDiskNum; Index++) {
    CopyMem (CapsulePtr, CapsuleOnDiskBuf[Index].ImageAddress, (UINTN)CapsuleOnDiskBuf[Index].FileInfo->FileSize);
    CapsulePtr += CapsuleOnDiskBuf[Index].FileInfo->FileSize;
  }

  //
  // Line the capsule header for capsule name capsule.
  //
  CopyGuid (&FileNameCapsuleHeader.CapsuleGuid, &gEdkiiCapsuleOnDiskNameGuid);
  FileNameCapsuleHeader.CapsuleImageSize = (UINT32)TotalImageNameSize + sizeof (EFI_CAPSULE_HEADER);
  FileNameCapsuleHeader.Flags            = CAPSULE_FLAGS_PERSIST_ACROSS_RESET;
  FileNameCapsuleHeader.HeaderSize       = sizeof (EFI_CAPSULE_HEADER);
  CopyMem (CapsulePtr, &FileNameCapsuleHeader, FileNameCapsuleHeader.HeaderSize);
  CapsulePtr += FileNameCapsuleHeader.HeaderSize;

  //
  // Line up all the Capsule file names.
  //
  for (Index = 0; Index < CapsuleOnDiskNum; Index++) {
    CopyMem (CapsulePtr, CapsuleOnDiskBuf[Index].FileInfo->FileName, StrSize (CapsuleOnDiskBuf[Index].FileInfo->FileName));
    CapsulePtr += StrSize (CapsuleOnDiskBuf[Index].FileInfo->FileName);
  }

  //
  // 5. Flash all Capsules on Disk to TempCoD.tmp under RootDir
  //
  Status = Fs->OpenVolume (Fs, &RootDir);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RelocateCapsule: OpenVolume error. %x\n", Status));
    goto EXIT;
  }

  Status = RootDir->Open (
                      RootDir,
                      &TempCodFile,
                      (CHAR16 *)PcdGetPtr (PcdCoDRelocationFileName),
                      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                      0
                      );
  if (!EFI_ERROR (Status)) {
    //
    // Error handling code to prevent malicious code to hold this file to block capsule on disk
    //
    TempCodFile->Delete (TempCodFile);
  }

  Status = RootDir->Open (
                      RootDir,
                      &TempCodFile,
                      (CHAR16 *)PcdGetPtr (PcdCoDRelocationFileName),
                      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                      0
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RelocateCapsule: Open TemCoD.tmp error. %x\n", Status));
    goto EXIT;
  }

  //
  // Always write at the begining of TempCap file
  //
  DataSize = (UINTN)TempCodFileSize;
  Status   = TempCodFile->Write (
                            TempCodFile,
                            &DataSize,
                            CapsuleDataBuf
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RelocateCapsule: Write TemCoD.tmp error. %x\n", Status));
    goto EXIT;
  }

  if (DataSize != TempCodFileSize) {
    Status = EFI_DEVICE_ERROR;
    goto EXIT;
  }

  //
  // Save Capsule On Disk relocation info to "CodRelocationInfo" Var
  // It is used in next reboot by TCB
  //
  RelocationInfo = TRUE;
  Status         = gRT->SetVariable (
                          COD_RELOCATION_INFO_VAR_NAME,
                          &gEfiCapsuleVendorGuid,
                          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                          sizeof (BOOLEAN),
                          &RelocationInfo
                          );
  //
  // Save the LoadOptionNumber of the boot option, where the capsule is relocated,
  // into "CodRelocationLoadOption" var. It is used in next reboot after capsule is
  // updated out of TCB to remove the TempCoDFile.
  //
  Status = gRT->SetVariable (
                  COD_RELOCATION_LOAD_OPTION_VAR_NAME,
                  &gEfiCapsuleVendorGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (UINT16),
                  &LoadOptionNumber
                  );

EXIT:

  if (CapsuleDataBuf != NULL) {
    FreePool (CapsuleDataBuf);
  }

  if (CapsuleOnDiskBuf != NULL) {
    //
    // Free resources allocated by CodLibGetAllCapsuleOnDisk
    //
    for (Index = 0; Index < CapsuleOnDiskNum; Index++ ) {
      FreePool (CapsuleOnDiskBuf[Index].ImageAddress);
      FreePool (CapsuleOnDiskBuf[Index].FileInfo);
    }

    FreePool (CapsuleOnDiskBuf);
  }

  if (TempCodFile != NULL) {
    if (EFI_ERROR (Status)) {
      TempCodFile->Delete (TempCodFile);
    } else {
      TempCodFile->Close (TempCodFile);
    }
  }

  if (RootDir != NULL) {
    RootDir->Close (RootDir);
  }

  return Status;
}

/**
  For the platforms that support Capsule In Ram, reuse the Capsule In Ram to deliver capsule.
  Relocate Capsule On Disk to memory and call UpdateCapsule().
  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated.

  @retval EFI_SUCCESS   Deliver capsule through Capsule In Ram successfully.

**/
EFI_STATUS
RelocateCapsuleToRam (
  UINTN  MaxRetry
  )
{
  EFI_STATUS                    Status;
  UINTN                         CapsuleOnDiskNum;
  IMAGE_INFO                    *CapsuleOnDiskBuf;
  EFI_HANDLE                    Handle;
  EFI_CAPSULE_BLOCK_DESCRIPTOR  *BlockDescriptors;
  VOID                          **CapsuleBuffer;
  UINTN                         *CapsuleSize;
  EFI_CAPSULE_HEADER            *FileNameCapsule;
  UINTN                         Index;
  UINT8                         *StringBuf;
  UINTN                         StringSize;
  UINTN                         TotalStringSize;
  UINTN                         CapsulesToProcess;

  CapsuleOnDiskBuf = NULL;
  BlockDescriptors = NULL;
  CapsuleBuffer    = NULL;
  CapsuleSize      = NULL;
  FileNameCapsule  = NULL;
  TotalStringSize  = 0;

  //
  // 1. Load all Capsule On Disks into memory
  //
  Status = GetAllCapsuleOnDisk (MaxRetry, &CapsuleOnDiskBuf, &CapsuleOnDiskNum, &Handle, NULL);
  if (EFI_ERROR (Status) || (CapsuleOnDiskNum == 0) || (CapsuleOnDiskBuf == NULL)) {
    DEBUG ((DEBUG_ERROR, "GetAllCapsuleOnDisk Status - 0x%x\n", Status));
    return EFI_NOT_FOUND;
  }

  //
  // 2. Add a capsule for Capsule file name strings
  //
  CapsuleBuffer = AllocateZeroPool ((CapsuleOnDiskNum + 1) * sizeof (VOID *));
  if (CapsuleBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for capsules.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  CapsuleSize = AllocateZeroPool ((CapsuleOnDiskNum + 1) * sizeof (UINTN));
  if (CapsuleSize == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for capsules.\n"));
    FreePool (CapsuleBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < CapsuleOnDiskNum; Index++) {
    CapsuleBuffer[Index] = (VOID *)(UINTN)CapsuleOnDiskBuf[Index].ImageAddress;
    CapsuleSize[Index]   = (UINTN)CapsuleOnDiskBuf[Index].FileInfo->FileSize;
    TotalStringSize     += StrSize (CapsuleOnDiskBuf[Index].FileInfo->FileName);
  }

  // If Persist Across Reset isn't supported, skip the file name strings capsule
  if (!FeaturePcdGet (PcdSupportUpdateCapsuleReset)) {
    CapsulesToProcess = CapsuleOnDiskNum;
    goto BuildGather;
  }

  CapsulesToProcess = CapsuleOnDiskNum + 1;

  FileNameCapsule = AllocateZeroPool (sizeof (EFI_CAPSULE_HEADER) + TotalStringSize);
  if (FileNameCapsule == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for name capsule.\n"));
    FreePool (CapsuleBuffer);
    FreePool (CapsuleSize);
    return EFI_OUT_OF_RESOURCES;
  }

  FileNameCapsule->CapsuleImageSize = (UINT32)(sizeof (EFI_CAPSULE_HEADER) + TotalStringSize);
  FileNameCapsule->Flags            = CAPSULE_FLAGS_PERSIST_ACROSS_RESET;
  FileNameCapsule->HeaderSize       = sizeof (EFI_CAPSULE_HEADER);
  CopyGuid (&(FileNameCapsule->CapsuleGuid), &gEdkiiCapsuleOnDiskNameGuid);

  StringBuf = (UINT8 *)FileNameCapsule + FileNameCapsule->HeaderSize;
  for (Index = 0; Index < CapsuleOnDiskNum; Index++) {
    StringSize = StrSize (CapsuleOnDiskBuf[Index].FileInfo->FileName);
    CopyMem (StringBuf, CapsuleOnDiskBuf[Index].FileInfo->FileName, StringSize);
    StringBuf += StringSize;
  }

  CapsuleBuffer[CapsuleOnDiskNum] = FileNameCapsule;
  CapsuleSize[CapsuleOnDiskNum]   = TotalStringSize + sizeof (EFI_CAPSULE_HEADER);

  //
  // 3. Build Gather list for the capsules
  //
BuildGather:
  Status = BuildGatherList (CapsuleBuffer, CapsuleSize, CapsulesToProcess, &BlockDescriptors);
  if (EFI_ERROR (Status) || (BlockDescriptors == NULL)) {
    FreePool (CapsuleBuffer);
    FreePool (CapsuleSize);
    if (FileNameCapsule != NULL) {
      FreePool (FileNameCapsule);
    }

    return EFI_OUT_OF_RESOURCES;
  }

  //
  // 4. Call UpdateCapsule() service
  //
  Status = gRT->UpdateCapsule (
                  (EFI_CAPSULE_HEADER **)CapsuleBuffer,
                  CapsulesToProcess,
                  (UINTN)BlockDescriptors
                  );

  return Status;
}

/**
  Relocate Capsule on Disk from EFI system partition.

  Two solution to deliver Capsule On Disk:
  Solution A: If PcdCapsuleInRamSupport is enabled, relocate Capsule On Disk to memory and call UpdateCapsule().
  Solution B: If PcdCapsuleInRamSupport is disabled, relocate Capsule On Disk to a platform-specific NV storage
  device with BlockIo protocol.

  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  Side Effects:
    Capsule Delivery Supported Flag in OsIndication variable and BootNext variable will be cleared.
    Solution B: Content corruption. Block IO write directly touches low level write. Orignal partitions, file
  systems of the relocation device will be corrupted.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated. Input 0 means no retry.

  @retval EFI_SUCCESS   Capsule on Disk images are successfully relocated.

**/
EFI_STATUS
EFIAPI
CoDRelocateCapsule (
  UINTN  MaxRetry
  )
{
  if (!PcdGetBool (PcdCapsuleOnDiskSupport)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Clear CapsuleOnDisk Flag firstly.
  //
  CoDClearCapsuleOnDiskFlag ();

  //
  // If Capsule In Ram is supported, delivery capsules through memory
  //
  if (PcdGetBool (PcdCapsuleInRamSupport)) {
    DEBUG ((DEBUG_INFO, "Capsule In Ram is supported, call gRT->UpdateCapsule().\n"));
    return RelocateCapsuleToRam (MaxRetry);
  } else {
    DEBUG ((DEBUG_INFO, "Reallcoate all Capsule on Disks to %s in RootDir.\n", (CHAR16 *)PcdGetPtr (PcdCoDRelocationFileName)));
    return RelocateCapsuleToDisk (MaxRetry);
  }
}

/**
  Remove the temp file from the root of EFI System Partition.
  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated. Input 0 means no retry.

  @retval EFI_SUCCESS   Remove the temp file successfully.

**/
EFI_STATUS
EFIAPI
CoDRemoveTempFile (
  UINTN  MaxRetry
  )
{
  EFI_STATUS                       Status;
  UINTN                            DataSize;
  UINT16                           *LoadOptionNumber;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_HANDLE                       FsHandle;
  EFI_FILE_HANDLE                  RootDir;
  EFI_FILE_HANDLE                  TempCodFile;

  RootDir     = NULL;
  TempCodFile = NULL;
  DataSize    = sizeof (UINT16);

  LoadOptionNumber = AllocatePool (sizeof (UINT16));
  if (LoadOptionNumber == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check if capsule files are relocated
  //
  Status = gRT->GetVariable (
                  COD_RELOCATION_LOAD_OPTION_VAR_NAME,
                  &gEfiCapsuleVendorGuid,
                  NULL,
                  &DataSize,
                  (VOID *)LoadOptionNumber
                  );
  if (EFI_ERROR (Status) || (DataSize != sizeof (UINT16))) {
    goto EXIT;
  }

  //
  // Get the EFI file system from the boot option where the capsules are relocated
  //
  Status = GetEfiSysPartitionFromActiveBootOption (MaxRetry, &LoadOptionNumber, &FsHandle);
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  Status = gBS->HandleProtocol (FsHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  Status = Fs->OpenVolume (Fs, &RootDir);
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Delete the TempCoDFile
  //
  Status = RootDir->Open (
                      RootDir,
                      &TempCodFile,
                      (CHAR16 *)PcdGetPtr (PcdCoDRelocationFileName),
                      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                      0
                      );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  TempCodFile->Delete (TempCodFile);

  //
  // Clear "CoDRelocationLoadOption" variable
  //
  Status = gRT->SetVariable (
                  COD_RELOCATION_LOAD_OPTION_VAR_NAME,
                  &gEfiCapsuleVendorGuid,
                  0,
                  0,
                  NULL
                  );

EXIT:
  if (LoadOptionNumber != NULL) {
    FreePool (LoadOptionNumber);
  }

  if (RootDir != NULL) {
    RootDir->Close (RootDir);
  }

  return Status;
}

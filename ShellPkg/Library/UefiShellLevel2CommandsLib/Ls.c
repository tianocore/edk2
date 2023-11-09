/** @file
  Main file for ls shell level 2 function.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"
#include <Guid/FileSystemInfo.h>

UINTN  mDayOfMonth[] = { 31, 28, 31, 30, 31, 30, 31, 30, 31, 30, 31, 30 };

/**
  print out the standard format output volume entry.

  @param[in] TheList           a list of files from the volume.
**/
EFI_STATUS
PrintSfoVolumeInfoTableEntry (
  IN CONST EFI_SHELL_FILE_INFO  *TheList
  )
{
  EFI_STATUS            Status;
  EFI_SHELL_FILE_INFO   *Node;
  CHAR16                *DirectoryName;
  EFI_FILE_SYSTEM_INFO  *SysInfo;
  UINTN                 SysInfoSize;
  SHELL_FILE_HANDLE     ShellFileHandle;
  EFI_FILE_PROTOCOL     *EfiFpHandle;

  //
  // Get the first valid handle (directories)
  //
  for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode (&TheList->Link)
        ; !IsNull (&TheList->Link, &Node->Link) && Node->Handle == NULL
        ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode (&TheList->Link, &Node->Link)
        )
  {
  }

  if (Node->Handle == NULL) {
    DirectoryName = GetFullyQualifiedPath (((EFI_SHELL_FILE_INFO *)GetFirstNode (&TheList->Link))->FullName);

    //
    // We need to open something up to get system information
    //
    Status = gEfiShellProtocol->OpenFileByName (
                                  DirectoryName,
                                  &ShellFileHandle,
                                  EFI_FILE_MODE_READ
                                  );

    ASSERT_EFI_ERROR (Status);
    FreePool (DirectoryName);

    //
    // Get the Volume Info from ShellFileHandle
    //
    SysInfo     = NULL;
    SysInfoSize = 0;
    EfiFpHandle = ConvertShellHandleToEfiFileProtocol (ShellFileHandle);
    Status      = EfiFpHandle->GetInfo (
                                 EfiFpHandle,
                                 &gEfiFileSystemInfoGuid,
                                 &SysInfoSize,
                                 SysInfo
                                 );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      SysInfo = AllocateZeroPool (SysInfoSize);
      Status  = EfiFpHandle->GetInfo (
                               EfiFpHandle,
                               &gEfiFileSystemInfoGuid,
                               &SysInfoSize,
                               SysInfo
                               );
    }

    ASSERT_EFI_ERROR (Status);

    gEfiShellProtocol->CloseFile (ShellFileHandle);
  } else {
    //
    // Get the Volume Info from Node->Handle
    //
    SysInfo     = NULL;
    SysInfoSize = 0;
    EfiFpHandle = ConvertShellHandleToEfiFileProtocol (Node->Handle);
    Status      = EfiFpHandle->GetInfo (
                                 EfiFpHandle,
                                 &gEfiFileSystemInfoGuid,
                                 &SysInfoSize,
                                 SysInfo
                                 );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      SysInfo = AllocateZeroPool (SysInfoSize);
      Status  = EfiFpHandle->GetInfo (
                               EfiFpHandle,
                               &gEfiFileSystemInfoGuid,
                               &SysInfoSize,
                               SysInfo
                               );
    }

    ASSERT_EFI_ERROR (Status);
  }

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_GEN_SFO_HEADER),
    gShellLevel2HiiHandle,
    L"ls"
    );
  //
  // print VolumeInfo table
  //
  ASSERT (SysInfo != NULL);
  ShellPrintHiiEx (
    0,
    gST->ConOut->Mode->CursorRow,
    NULL,
    STRING_TOKEN (STR_LS_SFO_VOLINFO),
    gShellLevel2HiiHandle,
    SysInfo->VolumeLabel,
    SysInfo->VolumeSize,
    SysInfo->ReadOnly ? L"TRUE" : L"FALSE",
    SysInfo->FreeSpace,
    SysInfo->BlockSize
    );

  SHELL_FREE_NON_NULL (SysInfo);

  return (Status);
}

/**
  print out the info on a single file.

  @param[in] Sfo      TRUE if in SFO, false otherwise.
  @param[in] TheNode  the EFI_SHELL_FILE_INFO node to print out information on.
  @param[in] Files    incremented if a file is printed.
  @param[in] Size     incremented by file size.
  @param[in] Dirs     incremented if a directory is printed.

**/
VOID
PrintFileInformation (
  IN CONST BOOLEAN              Sfo,
  IN CONST EFI_SHELL_FILE_INFO  *TheNode,
  IN UINT64                     *Files,
  IN UINT64                     *Size,
  IN UINT64                     *Dirs
  )
{
  ASSERT (Files    != NULL);
  ASSERT (Size     != NULL);
  ASSERT (Dirs     != NULL);
  ASSERT (TheNode  != NULL);

  if (Sfo) {
    //
    // Print the FileInfo Table
    //
    ShellPrintHiiEx (
      0,
      gST->ConOut->Mode->CursorRow,
      NULL,
      STRING_TOKEN (STR_LS_SFO_FILEINFO),
      gShellLevel2HiiHandle,
      TheNode->FullName,
      TheNode->Info->FileSize,
      TheNode->Info->PhysicalSize,
      (TheNode->Info->Attribute & EFI_FILE_ARCHIVE)   != 0 ? L"a" : L"",
      (TheNode->Info->Attribute & EFI_FILE_DIRECTORY) != 0 ? L"d" : L"",
      (TheNode->Info->Attribute & EFI_FILE_HIDDEN)    != 0 ? L"h" : L"",
      (TheNode->Info->Attribute & EFI_FILE_READ_ONLY) != 0 ? L"r" : L"",
      (TheNode->Info->Attribute & EFI_FILE_SYSTEM)    != 0 ? L"s" : L"",
      TheNode->Info->CreateTime.Hour,
      TheNode->Info->CreateTime.Minute,
      TheNode->Info->CreateTime.Second,
      TheNode->Info->CreateTime.Day,
      TheNode->Info->CreateTime.Month,
      TheNode->Info->CreateTime.Year,
      TheNode->Info->LastAccessTime.Hour,
      TheNode->Info->LastAccessTime.Minute,
      TheNode->Info->LastAccessTime.Second,
      TheNode->Info->LastAccessTime.Day,
      TheNode->Info->LastAccessTime.Month,
      TheNode->Info->LastAccessTime.Year,
      TheNode->Info->ModificationTime.Hour,
      TheNode->Info->ModificationTime.Minute,
      TheNode->Info->ModificationTime.Second,
      TheNode->Info->ModificationTime.Day,
      TheNode->Info->ModificationTime.Month,
      TheNode->Info->ModificationTime.Year
      );
  } else {
    //
    // print this one out...
    // first print the universal start, next print the type specific name format, last print the CRLF
    //
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_LS_LINE_START_ALL),
      gShellLevel2HiiHandle,
      &TheNode->Info->ModificationTime,
      (TheNode->Info->Attribute & EFI_FILE_DIRECTORY) != 0 ? L"<DIR>" : L"",
      (TheNode->Info->Attribute & EFI_FILE_READ_ONLY) != 0 ? L'r' : L' ',
      TheNode->Info->FileSize
      );
    if (TheNode->Info->Attribute & EFI_FILE_DIRECTORY) {
      (*Dirs)++;
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_LS_LINE_END_DIR),
        gShellLevel2HiiHandle,
        TheNode->FileName
        );
    } else {
      (*Files)++;
      (*Size) += TheNode->Info->FileSize;
      if (  (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16 *)L".nsh", (CHAR16 *)&(TheNode->FileName[StrLen (TheNode->FileName) - 4])) == 0)
         || (gUnicodeCollation->StriColl (gUnicodeCollation, (CHAR16 *)L".efi", (CHAR16 *)&(TheNode->FileName[StrLen (TheNode->FileName) - 4])) == 0)
            )
      {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_LS_LINE_END_EXE),
          gShellLevel2HiiHandle,
          TheNode->FileName
          );
      } else {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_LS_LINE_END_FILE),
          gShellLevel2HiiHandle,
          TheNode->FileName
          );
      }
    }
  }
}

/**
  print out the header when not using standard format output.

  @param[in] Path           String with starting path.
**/
VOID
PrintNonSfoHeader (
  IN CONST CHAR16  *Path
  )
{
  CHAR16  *DirectoryName;

  //
  // get directory name from path...
  //
  DirectoryName = GetFullyQualifiedPath (Path);

  if (DirectoryName != NULL) {
    //
    // print header
    //
    ShellPrintHiiEx (
      0,
      gST->ConOut->Mode->CursorRow,
      NULL,
      STRING_TOKEN (STR_LS_HEADER_LINE1),
      gShellLevel2HiiHandle,
      DirectoryName
      );

    SHELL_FREE_NON_NULL (DirectoryName);
  }
}

/**
  print out the footer when not using standard format output.

  @param[in] Files            The number of files.
  @param[in] Size             The size of files in bytes.
  @param[in] Dirs             The number of directories.
**/
VOID
PrintNonSfoFooter (
  IN UINT64  Files,
  IN UINT64  Size,
  IN UINT64  Dirs
  )
{
  //
  // print footer
  //
  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_LS_FOOTER_LINE),
    gShellLevel2HiiHandle,
    Files,
    Size,
    Dirs
    );
}

/**
  Change the file time to local time based on the timezone.

  @param[in] Time               The file time.
  @param[in] LocalTimeZone      Local time zone.
**/
VOID
FileTimeToLocalTime (
  IN EFI_TIME  *Time,
  IN INT16     LocalTimeZone
  )
{
  INTN  MinuteDiff;
  INTN  TempMinute;
  INTN  HourNumberOfTempMinute;
  INTN  TempHour;
  INTN  DayNumberOfTempHour;
  INTN  TempDay;
  INTN  MonthNumberOfTempDay;
  INTN  TempMonth;
  INTN  YearNumberOfTempMonth;
  INTN  MonthRecord;

  ASSERT ((Time->TimeZone >= -1440) && (Time->TimeZone <= 1440));
  ASSERT ((LocalTimeZone >= -1440) && (LocalTimeZone <= 1440));
  ASSERT ((Time->Month >= 1) && (Time->Month <= 12));

  if (Time->TimeZone == LocalTimeZone) {
    //
    // if the file timezone is equal to the local timezone, there is no need to adjust the file time.
    //
    return;
  }

  if (((Time->Year % 4 == 0) && (Time->Year / 100 != 0)) || (Time->Year % 400 == 0)) {
    //
    // Day in February of leap year is 29.
    //
    mDayOfMonth[1] = 29;
  }

  MinuteDiff = Time->TimeZone - LocalTimeZone;
  TempMinute = Time->Minute + MinuteDiff;

  //
  // Calculate Time->Minute
  // TempHour will be used to calculate Time->Hour
  //
  HourNumberOfTempMinute = TempMinute / 60;
  if (TempMinute < 0) {
    HourNumberOfTempMinute--;
  }

  TempHour     = Time->Hour + HourNumberOfTempMinute;
  Time->Minute = (UINT8)(TempMinute - 60 * HourNumberOfTempMinute);

  //
  // Calculate Time->Hour
  // TempDay will be used to calculate Time->Day
  //
  DayNumberOfTempHour = TempHour / 24;
  if (TempHour < 0) {
    DayNumberOfTempHour--;
  }

  TempDay    = Time->Day + DayNumberOfTempHour;
  Time->Hour = (UINT8)(TempHour - 24 * DayNumberOfTempHour);

  //
  // Calculate Time->Day
  // TempMonth will be used to calculate Time->Month
  //
  MonthNumberOfTempDay = (TempDay - 1) / (INTN)mDayOfMonth[Time->Month - 1];
  MonthRecord          = (INTN)(Time->Month);
  if (TempDay - 1 < 0) {
    MonthNumberOfTempDay--;
    MonthRecord--;
  }

  TempMonth = Time->Month + MonthNumberOfTempDay;
  Time->Day = (UINT8)(TempDay - (INTN)mDayOfMonth[(MonthRecord - 1 + 12) % 12] * MonthNumberOfTempDay);

  //
  // Calculate Time->Month, Time->Year
  //
  YearNumberOfTempMonth = (TempMonth - 1) / 12;
  if (TempMonth - 1 < 0) {
    YearNumberOfTempMonth--;
  }

  Time->Month = (UINT8)(TempMonth - 12 * (YearNumberOfTempMonth));
  Time->Year  = (UINT16)(Time->Year + YearNumberOfTempMonth);
}

/**
  print out the list of files and directories from the LS command

  @param[in] Rec            TRUE to automatically recurse into each found directory
                            FALSE to only list the specified directory.
  @param[in] Attribs        List of required Attribute for display.
                            If 0 then all non-system and non-hidden files will be printed.
  @param[in] Sfo            TRUE to use Standard Format Output, FALSE otherwise
  @param[in] RootPath       String with starting path to search in.
  @param[in] SearchString   String with search string.
  @param[in] Found          Set to TRUE, if anyone were found.
  @param[in] Count          The count of bits enabled in Attribs.
  @param[in] TimeZone       The current time zone offset.
  @param[in] ListUnfiltered TRUE to request listing the directory contents
                            unfiltered.

  @retval SHELL_SUCCESS     the printing was successful.
**/
SHELL_STATUS
PrintLsOutput (
  IN CONST BOOLEAN  Rec,
  IN CONST UINT64   Attribs,
  IN CONST BOOLEAN  Sfo,
  IN CONST CHAR16   *RootPath,
  IN CONST CHAR16   *SearchString,
  IN       BOOLEAN  *Found,
  IN CONST UINTN    Count,
  IN CONST INT16    TimeZone,
  IN CONST BOOLEAN  ListUnfiltered
  )
{
  EFI_STATUS           Status;
  EFI_SHELL_FILE_INFO  *ListHead;
  EFI_SHELL_FILE_INFO  *Node;
  SHELL_STATUS         ShellStatus;
  UINT64               FileCount;
  UINT64               DirCount;
  UINT64               FileSize;
  UINTN                LongestPath;
  CHAR16               *CorrectedPath;
  BOOLEAN              FoundOne;
  BOOLEAN              HeaderPrinted;
  EFI_TIME             LocalTime;

  HeaderPrinted = FALSE;
  FileCount     = 0;
  DirCount      = 0;
  FileSize      = 0;
  ListHead      = NULL;
  ShellStatus   = SHELL_SUCCESS;
  LongestPath   = 0;
  CorrectedPath = NULL;

  if (Found != NULL) {
    FoundOne = *Found;
  } else {
    FoundOne = FALSE;
  }

  CorrectedPath = StrnCatGrow (&CorrectedPath, &LongestPath, RootPath, 0);
  if (CorrectedPath == NULL) {
    return SHELL_OUT_OF_RESOURCES;
  }

  if (  (CorrectedPath[StrLen (CorrectedPath)-1] != L'\\')
     && (CorrectedPath[StrLen (CorrectedPath)-1] != L'/'))
  {
    CorrectedPath = StrnCatGrow (&CorrectedPath, &LongestPath, L"\\", 0);
  }

  CorrectedPath = StrnCatGrow (&CorrectedPath, &LongestPath, SearchString, 0);
  if (CorrectedPath == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }

  PathCleanUpDirectories (CorrectedPath);

  Status = ShellOpenFileMetaArg ((CHAR16 *)CorrectedPath, EFI_FILE_MODE_READ, &ListHead);
  if (!EFI_ERROR (Status)) {
    if ((ListHead == NULL) || IsListEmpty (&ListHead->Link)) {
      SHELL_FREE_NON_NULL (CorrectedPath);
      return (SHELL_SUCCESS);
    }

    if (Sfo && (Found == NULL)) {
      PrintSfoVolumeInfoTableEntry (ListHead);
    }

    if (!Sfo) {
      //
      // Sort the file list by FileName, stably.
      //
      // If the call below fails, then the EFI_SHELL_FILE_INFO list anchored to
      // ListHead will not be changed in any way.
      //
      ShellSortFileList (
        &ListHead,
        NULL,                       // Duplicates
        ShellSortFileListByFileName
        );
    }

    for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode (&ListHead->Link), LongestPath = 0
          ; !IsNull (&ListHead->Link, &Node->Link)
          ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode (&ListHead->Link, &Node->Link)
          )
    {
      if (ShellGetExecutionBreakFlag ()) {
        ShellStatus = SHELL_ABORTED;
        break;
      }

      ASSERT (Node != NULL);

      //
      // Change the file time to local time.
      //
      Status = gRT->GetTime (&LocalTime, NULL);
      if (!EFI_ERROR (Status) && (LocalTime.TimeZone != EFI_UNSPECIFIED_TIMEZONE)) {
        if ((Node->Info->CreateTime.TimeZone != EFI_UNSPECIFIED_TIMEZONE) &&
            ((Node->Info->CreateTime.Month >= 1) && (Node->Info->CreateTime.Month <= 12)))
        {
          //
          // FileTimeToLocalTime () requires Month is in a valid range, other buffer out-of-band access happens.
          //
          FileTimeToLocalTime (&Node->Info->CreateTime, LocalTime.TimeZone);
        }

        if ((Node->Info->LastAccessTime.TimeZone != EFI_UNSPECIFIED_TIMEZONE) &&
            ((Node->Info->LastAccessTime.Month >= 1) && (Node->Info->LastAccessTime.Month <= 12)))
        {
          FileTimeToLocalTime (&Node->Info->LastAccessTime, LocalTime.TimeZone);
        }

        if ((Node->Info->ModificationTime.TimeZone != EFI_UNSPECIFIED_TIMEZONE) &&
            ((Node->Info->ModificationTime.Month >= 1) && (Node->Info->ModificationTime.Month <= 12)))
        {
          FileTimeToLocalTime (&Node->Info->ModificationTime, LocalTime.TimeZone);
        }
      }

      if (LongestPath < StrSize (Node->FullName)) {
        LongestPath = StrSize (Node->FullName);
      }

      ASSERT (Node->Info != NULL);
      ASSERT ((Node->Info->Attribute & EFI_FILE_VALID_ATTR) == Node->Info->Attribute);
      if (Attribs == 0) {
        //
        // NOT system & NOT hidden
        //
        if (  (Node->Info->Attribute & EFI_FILE_SYSTEM)
           || (Node->Info->Attribute & EFI_FILE_HIDDEN)
              )
        {
          continue;
        }
      } else if ((Attribs != EFI_FILE_VALID_ATTR) ||
                 (Count == 5))
      {
        //
        // Only matches the bits which "Attribs" contains, not
        // all files/directories with any of the bits.
        // Count == 5 is used to tell the difference between a user
        // specifying all bits (EX: -arhsda) and just specifying
        // -a (means display all files with any attribute).
        //
        if ((Node->Info->Attribute & Attribs) != Attribs) {
          continue;
        }
      }

      if (!Sfo && !HeaderPrinted) {
        PathRemoveLastItem (CorrectedPath);
        PrintNonSfoHeader (CorrectedPath);
      }

      PrintFileInformation (Sfo, Node, &FileCount, &FileSize, &DirCount);
      FoundOne      = TRUE;
      HeaderPrinted = TRUE;
    }

    if (!Sfo && (ShellStatus != SHELL_ABORTED) && HeaderPrinted) {
      PrintNonSfoFooter (FileCount, FileSize, DirCount);
    }
  }

  if (Rec && (ShellStatus != SHELL_ABORTED)) {
    //
    // Re-Open all the files under the starting path for directories that didnt necessarily match our file filter
    //
    ShellCloseFileMetaArg (&ListHead);
    CorrectedPath[0] = CHAR_NULL;
    CorrectedPath    = StrnCatGrow (&CorrectedPath, &LongestPath, RootPath, 0);
    if (CorrectedPath == NULL) {
      return SHELL_OUT_OF_RESOURCES;
    }

    if (  (CorrectedPath[StrLen (CorrectedPath)-1] != L'\\')
       && (CorrectedPath[StrLen (CorrectedPath)-1] != L'/'))
    {
      CorrectedPath = StrnCatGrow (&CorrectedPath, &LongestPath, L"\\", 0);
    }

    CorrectedPath = StrnCatGrow (&CorrectedPath, &LongestPath, L"*", 0);
    Status        = ShellOpenFileMetaArg ((CHAR16 *)CorrectedPath, EFI_FILE_MODE_READ, &ListHead);

    if (!EFI_ERROR (Status)) {
      for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode (&ListHead->Link)
            ; !IsNull (&ListHead->Link, &Node->Link) && ShellStatus == SHELL_SUCCESS
            ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode (&ListHead->Link, &Node->Link)
            )
      {
        if (ShellGetExecutionBreakFlag ()) {
          ShellStatus = SHELL_ABORTED;
          break;
        }

        //
        // recurse on any directory except the traversing ones...
        //
        if (  ((Node->Info->Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY)
           && (StrCmp (Node->FileName, L".") != 0)
           && (StrCmp (Node->FileName, L"..") != 0)
              )
        {
          ShellStatus = PrintLsOutput (
                          Rec,
                          Attribs,
                          Sfo,
                          Node->FullName,
                          SearchString,
                          &FoundOne,
                          Count,
                          TimeZone,
                          FALSE
                          );

          //
          // Since it's running recursively, we have to break immediately when returned SHELL_ABORTED
          //
          if (ShellStatus == SHELL_ABORTED) {
            break;
          }
        }
      }
    }
  }

  SHELL_FREE_NON_NULL (CorrectedPath);
  ShellCloseFileMetaArg (&ListHead);

  if ((Found == NULL) && !FoundOne) {
    if (ListUnfiltered) {
      //
      // When running "ls" without any filtering request, avoid outputing
      // "File not found" when the directory is entirely empty, but print
      // header and footer stating "0 File(s), 0 Dir(s)".
      //
      if (!Sfo) {
        PrintNonSfoHeader (RootPath);
        if (ShellStatus != SHELL_ABORTED) {
          PrintNonSfoFooter (FileCount, FileSize, DirCount);
        }
      }
    } else {
      return (SHELL_NOT_FOUND);
    }
  }

  if (Found != NULL) {
    *Found = FoundOne;
  }

  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM  LsParamList[] = {
  { L"-r",   TypeFlag  },
  { L"-a",   TypeStart },
  { L"-sfo", TypeFlag  },
  { NULL,    TypeMax   }
};

/**
  Function for 'ls' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunLs (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  CONST CHAR16  *Attribs;
  SHELL_STATUS  ShellStatus;
  UINT64        RequiredAttributes;
  CONST CHAR16  *PathName;
  CONST CHAR16  *CurDir;
  UINTN         Count;
  CHAR16        *FullPath;
  UINTN         Size;
  EFI_TIME      TheTime;
  CHAR16        *SearchString;
  BOOLEAN       ListUnfiltered;

  Size               = 0;
  FullPath           = NULL;
  ProblemParam       = NULL;
  Attribs            = NULL;
  ShellStatus        = SHELL_SUCCESS;
  RequiredAttributes = 0;
  PathName           = NULL;
  SearchString       = NULL;
  CurDir             = NULL;
  Count              = 0;
  ListUnfiltered     = FALSE;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // Fix local copies of the protocol pointers
  //
  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (LsParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"ls", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag (Package, L"-?")) {
      ASSERT (FALSE);
    }

    if (ShellCommandLineGetCount (Package) > 2) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"ls");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // check for -a
      //
      if (ShellCommandLineGetFlag (Package, L"-a")) {
        for ( Attribs = ShellCommandLineGetValue (Package, L"-a")
              ; Attribs != NULL && *Attribs != CHAR_NULL && ShellStatus == SHELL_SUCCESS
              ; Attribs++
              )
        {
          switch (*Attribs) {
            case L'a':
            case L'A':
              RequiredAttributes |= EFI_FILE_ARCHIVE;
              Count++;
              continue;
            case L's':
            case L'S':
              RequiredAttributes |= EFI_FILE_SYSTEM;
              Count++;
              continue;
            case L'h':
            case L'H':
              RequiredAttributes |= EFI_FILE_HIDDEN;
              Count++;
              continue;
            case L'r':
            case L'R':
              RequiredAttributes |= EFI_FILE_READ_ONLY;
              Count++;
              continue;
            case L'd':
            case L'D':
              RequiredAttributes |= EFI_FILE_DIRECTORY;
              Count++;
              continue;
            default:
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ATTRIBUTE), gShellLevel2HiiHandle, L"ls", ShellCommandLineGetValue (Package, L"-a"));
              ShellStatus = SHELL_INVALID_PARAMETER;
              break;
          } // switch
        } // for loop

        //
        // if nothing is specified all are specified
        //
        if (RequiredAttributes == 0) {
          RequiredAttributes = EFI_FILE_VALID_ATTR;
        }
      } // if -a present

      if (ShellStatus == SHELL_SUCCESS) {
        PathName = ShellCommandLineGetRawValue (Package, 1);
        if (PathName == NULL) {
          //
          // Nothing specified... must start from current directory
          //
          CurDir = gEfiShellProtocol->GetCurDir (NULL);
          if (CurDir == NULL) {
            ShellStatus = SHELL_NOT_FOUND;
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle, L"ls");
          }

          ListUnfiltered = TRUE;
          //
          // Copy to the 2 strings for starting path and file search string
          //
          ASSERT (SearchString == NULL);
          ASSERT (FullPath == NULL);
          StrnCatGrow (&SearchString, NULL, L"*", 0);
          StrnCatGrow (&FullPath, NULL, CurDir, 0);
          Size = FullPath != NULL ? StrSize (FullPath) : 0;
          StrnCatGrow (&FullPath, &Size, L"\\", 0);
        } else {
          if ((StrStr (PathName, L":") == NULL) && (gEfiShellProtocol->GetCurDir (NULL) == NULL)) {
            //
            // If we got something and it doesnt have a fully qualified path, then we needed to have a CWD.
            //
            ShellStatus = SHELL_NOT_FOUND;
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle, L"ls");
          } else {
            //
            // We got a valid fully qualified path or we have a CWD
            //
            ASSERT ((FullPath == NULL && Size == 0) || (FullPath != NULL));
            if (StrStr (PathName, L":") == NULL) {
              StrnCatGrow (&FullPath, &Size, gEfiShellProtocol->GetCurDir (NULL), 0);
              if (FullPath == NULL) {
                ShellCommandLineFreeVarList (Package);
                return SHELL_OUT_OF_RESOURCES;
              }

              Size = FullPath != NULL ? StrSize (FullPath) : 0;
              StrnCatGrow (&FullPath, &Size, L"\\", 0);
            }

            StrnCatGrow (&FullPath, &Size, PathName, 0);
            if (FullPath == NULL) {
              ShellCommandLineFreeVarList (Package);
              return SHELL_OUT_OF_RESOURCES;
            }

            if (ShellIsDirectory (PathName) == EFI_SUCCESS) {
              //
              // is listing ends with a directory, then we list all files in that directory
              //
              ListUnfiltered = TRUE;
              StrnCatGrow (&SearchString, NULL, L"*", 0);
            } else {
              //
              // must split off the search part that applies to files from the end of the directory part
              //
              StrnCatGrow (&SearchString, NULL, FullPath, 0);
              if (SearchString == NULL) {
                FreePool (FullPath);
                ShellCommandLineFreeVarList (Package);
                return SHELL_OUT_OF_RESOURCES;
              }

              PathRemoveLastItem (FullPath);
              CopyMem (SearchString, SearchString + StrLen (FullPath), StrSize (SearchString + StrLen (FullPath)));
            }
          }
        }

        Status = gRT->GetTime (&TheTime, NULL);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_UEFI_FUNC_WARN), gShellLevel2HiiHandle, L"ls", L"gRT->GetTime", Status);
          TheTime.TimeZone = EFI_UNSPECIFIED_TIMEZONE;
        }

        if (ShellStatus == SHELL_SUCCESS) {
          ShellStatus = PrintLsOutput (
                          ShellCommandLineGetFlag (Package, L"-r"),
                          RequiredAttributes,
                          ShellCommandLineGetFlag (Package, L"-sfo"),
                          FullPath,
                          SearchString,
                          NULL,
                          Count,
                          TheTime.TimeZone,
                          ListUnfiltered
                          );
          if (ShellStatus == SHELL_NOT_FOUND) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_LS_FILE_NOT_FOUND), gShellLevel2HiiHandle, L"ls", FullPath);
          } else if (ShellStatus == SHELL_INVALID_PARAMETER) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle, L"ls", FullPath);
          } else if (ShellStatus == SHELL_ABORTED) {
            //
            // Ignore aborting.
            //
          } else if (ShellStatus != SHELL_SUCCESS) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle, L"ls", FullPath);
          }
        }
      }
    }
  }

  //
  // Free memory allocated
  //
  SHELL_FREE_NON_NULL (SearchString);
  SHELL_FREE_NON_NULL (FullPath);
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}

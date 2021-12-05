/*++ @file
  Support OS native directory access.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "WinHost.h"

#define WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE  SIGNATURE_32 ('N', 'T', 'f', 's')

typedef struct {
  UINTN                              Signature;
  EMU_IO_THUNK_PROTOCOL              *Thunk;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    SimpleFileSystem;
  CHAR16                             *FilePath;
  CHAR16                             *VolumeLabel;
} WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE;

#define WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE, \
      SimpleFileSystem, \
      WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE \
      )

#define WIN_NT_EFI_FILE_PRIVATE_SIGNATURE  SIGNATURE_32 ('l', 'o', 'f', 's')

typedef struct {
  UINTN                              Signature;
  EMU_IO_THUNK_PROTOCOL              *Thunk;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *SimpleFileSystem;
  EFI_FILE_PROTOCOL                  EfiFile;
  HANDLE                             LHandle;
  HANDLE                             DirHandle;
  BOOLEAN                            IsRootDirectory;
  BOOLEAN                            IsDirectoryPath;
  BOOLEAN                            IsOpenedByRead;
  CHAR16                             *FilePath;
  WCHAR                              *FileName;
  BOOLEAN                            IsValidFindBuf;
  WIN32_FIND_DATA                    FindBuf;
} WIN_NT_EFI_FILE_PRIVATE;

#define WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      WIN_NT_EFI_FILE_PRIVATE, \
      EfiFile, \
      WIN_NT_EFI_FILE_PRIVATE_SIGNATURE \
      )

extern EFI_FILE_PROTOCOL                gWinNtFileProtocol;
extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  gWinNtFileSystemProtocol;

EFI_STATUS
WinNtFileGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  );

EFI_STATUS
WinNtFileSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  );

CHAR16 *
EfiStrChr (
  IN CHAR16  *Str,
  IN CHAR16  Chr
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
{
  if ((Buffer == NULL) || (Length == 0)) {
    return FALSE;
  }

  if (*(UINT8 *)Buffer != 0) {
    return FALSE;
  }

  if (Length > 1) {
    if (!CompareMem (Buffer, (UINT8 *)Buffer + 1, Length - 1)) {
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
WinNtOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL               **Root
  )
{
  EFI_STATUS                         Status;
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE  *Private;
  WIN_NT_EFI_FILE_PRIVATE            *PrivateFile;
  CHAR16                             *TempFileName;
  UINTN                              Size;

  if ((This == NULL) || (Root == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (This);

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

  StrCpyS (
    PrivateFile->FilePath,
    StrSize (Private->FilePath) / sizeof (CHAR16),
    Private->FilePath
    );
  StrCpyS (
    PrivateFile->FileName,
    StrSize (Private->FilePath) / sizeof (CHAR16),
    PrivateFile->FilePath
    );
  PrivateFile->Signature        = WIN_NT_EFI_FILE_PRIVATE_SIGNATURE;
  PrivateFile->Thunk            = Private->Thunk;
  PrivateFile->SimpleFileSystem = This;
  PrivateFile->IsRootDirectory  = TRUE;
  PrivateFile->IsDirectoryPath  = TRUE;
  PrivateFile->IsOpenedByRead   = TRUE;
  CopyMem (&PrivateFile->EfiFile, &gWinNtFileProtocol, sizeof (gWinNtFileProtocol));
  PrivateFile->IsValidFindBuf = FALSE;

  //
  // Set DirHandle
  //
  PrivateFile->DirHandle = CreateFile (
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
  Size         = StrSize (PrivateFile->FilePath);
  Size        += StrSize (L"\\*");
  TempFileName = AllocatePool (Size);
  if (TempFileName == NULL) {
    goto Done;
  }

  StrCpyS (TempFileName, Size / sizeof (CHAR16), PrivateFile->FilePath);
  StrCatS (TempFileName, Size / sizeof (CHAR16), L"\\*");

  PrivateFile->LHandle = FindFirstFile (TempFileName, &PrivateFile->FindBuf);
  FreePool (TempFileName);

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

  return Status;
}

/**
  Count the number of Leading Dot in FileNameToken.

  @param FileNameToken  A string representing a token in the path name.

  @return  UINTN             The number of leading dot in the name.

**/
UINTN
CountLeadingDots (
  IN CONST CHAR16  *FileNameToken
  )
{
  UINTN  Num;

  Num = 0;
  while (*FileNameToken == L'.') {
    Num++;
    FileNameToken++;
  }

  return Num;
}

BOOLEAN
IsFileNameTokenValid (
  IN CONST CHAR16  *FileNameToken
  )
{
  UINTN  Num;

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
  IN OUT CONST CHAR16  **FileName
  )
{
  CHAR16  *SlashPos;
  CHAR16  *Token;
  UINTN   Offset;

  ASSERT (**FileName != L'\\');
  ASSERT (**FileName != L'\0');

  SlashPos = StrStr (*FileName, L"\\");
  if (SlashPos == NULL) {
    Token     = AllocateCopyPool (StrSize (*FileName), *FileName);
    *FileName = NULL;
  } else {
    Offset = SlashPos - *FileName;
    Token  = AllocateZeroPool ((Offset + 1) * sizeof (CHAR16));
    StrnCpyS (Token, Offset + 1, *FileName, Offset);
    //
    // Point *FileName to the next character after L'\'.
    //
    *FileName = *FileName + Offset + 1;
    //
    // If *FileName is an empty string, then set *FileName to NULL
    //
    if (**FileName == L'\0') {
      *FileName = NULL;
    }
  }

  return Token;
}

/**
  Check if a FileName contains only Valid Characters.

  If FileName contains only a single L'\', return TRUE.
  If FileName contains two adjacent L'\', return FALSE.
  If FileName conatins L'/' , return FALSE.
  If FileName contains more than two dots separated with other FileName characters
  by L'\', return FALSE. For example, L'.\...\filename.txt' is invalid path name. But L'..TwoDots\filename.txt' is valid path name.

  @param FileName  The File Name String to check.

  @return  TRUE        FileName only contains valid characters.
  @return  FALSE       FileName contains at least one invalid character.

**/
BOOLEAN
IsFileNameValid (
  IN CONST CHAR16  *FileName
  )
{
  CHAR16   *Token;
  BOOLEAN  Valid;

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
  if (FileName[0] == L'\\') {
    FileName++;
  }

  do {
    Token = GetNextFileNameToken (&FileName);
    Valid = IsFileNameTokenValid (Token);
    FreePool (Token);

    if (!Valid) {
      return FALSE;
    }
  } while (FileName != NULL);

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
WinNtFileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  )
{
  WIN_NT_EFI_FILE_PRIVATE            *PrivateFile;
  WIN_NT_EFI_FILE_PRIVATE            *NewPrivateFile;
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE  *PrivateRoot;
  EFI_STATUS                         Status;
  CHAR16                             *RealFileName;
  CHAR16                             *TempFileName;
  CHAR16                             *ParseFileName;
  CHAR16                             *GuardPointer;
  CHAR16                             TempChar;
  DWORD                              LastError;
  UINTN                              Count;
  BOOLEAN                            LoopFinish;
  UINTN                              InfoSize;
  EFI_FILE_INFO                      *Info;
  UINTN                              Size;

  //
  // Init local variables
  //
  PrivateFile    = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot    = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);
  NewPrivateFile = NULL;

  //
  // Allocate buffer for FileName as the passed in FileName may be read only
  //
  TempFileName = AllocatePool (StrSize (FileName));
  if (TempFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrCpyS (TempFileName, StrSize (FileName) / sizeof (CHAR16), FileName);
  FileName = TempFileName;

  if (FileName[StrLen (FileName) - 1] == L'\\') {
    FileName[StrLen (FileName) - 1] = 0;
  }

  //
  // If file name does not equal to "." or ".." and not trailed with "\..",
  // then we trim the leading/trailing blanks and trailing dots
  //
  if ((StrCmp (FileName, L".") != 0) && (StrCmp (FileName, L"..") != 0) &&
      ((StrLen (FileName) >= 3) ? (StrCmp (&FileName[StrLen (FileName) - 3], L"\\..") != 0) : TRUE))
  {
    //
    // Trim leading blanks
    //
    Count = 0;
    for (TempFileName = FileName;
         *TempFileName != 0 && *TempFileName == L' ';
         TempFileName++)
    {
      Count++;
    }

    CutPrefix (FileName, Count);
    //
    // Trim trailing blanks
    //
    for (TempFileName = FileName + StrLen (FileName) - 1;
         TempFileName >= FileName && (*TempFileName == L' ');
         TempFileName--)
    {
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
    StrCpyS (
      NewPrivateFile->FilePath,
      StrSize (PrivateFile->FileName) / sizeof (CHAR16),
      PrivateFile->FileName
      );
  } else {
    StrCpyS (
      NewPrivateFile->FilePath,
      StrSize (PrivateFile->FileName) / sizeof (CHAR16),
      PrivateFile->FilePath
      );
  }

  Size                     = StrSize (NewPrivateFile->FilePath);
  Size                    += StrSize (L"\\");
  Size                    += StrSize (FileName);
  NewPrivateFile->FileName = AllocatePool (Size);
  if (NewPrivateFile->FileName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  if (*FileName == L'\\') {
    StrCpyS (NewPrivateFile->FileName, Size / sizeof (CHAR16), PrivateRoot->FilePath);
    StrCatS (NewPrivateFile->FileName, Size / sizeof (CHAR16), L"\\");
    StrCatS (NewPrivateFile->FileName, Size / sizeof (CHAR16), FileName + 1);
  } else {
    StrCpyS (NewPrivateFile->FileName, Size / sizeof (CHAR16), NewPrivateFile->FilePath);
    if (StrCmp (FileName, L"") != 0) {
      //
      // In case the filename becomes empty, especially after trimming dots and blanks
      //
      StrCatS (NewPrivateFile->FileName, Size / sizeof (CHAR16), L"\\");
      StrCatS (NewPrivateFile->FileName, Size / sizeof (CHAR16), FileName);
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
  GuardPointer = NewPrivateFile->FileName + StrLen (PrivateRoot->FilePath);

  LoopFinish = FALSE;

  while (!LoopFinish) {
    LoopFinish = TRUE;

    for (ParseFileName = GuardPointer; *ParseFileName; ParseFileName++) {
      if ((*ParseFileName == L'.') &&
          ((*(ParseFileName + 1) == 0) || (*(ParseFileName + 1) == L'\\')) &&
          (*(ParseFileName - 1) == L'\\')
          )
      {
        //
        // cut \.
        //
        CutPrefix (ParseFileName - 1, 2);
        LoopFinish = FALSE;
        break;
      }

      if ((*ParseFileName == L'.') &&
          (*(ParseFileName + 1) == L'.') &&
          ((*(ParseFileName + 2) == 0) || (*(ParseFileName + 2) == L'\\')) &&
          (*(ParseFileName - 1) == L'\\')
          )
      {
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

  StrCpyS (
    NewPrivateFile->FilePath,
    StrSize (NewPrivateFile->FileName) / sizeof (CHAR16),
    NewPrivateFile->FileName
    );
  if (TempChar != 0) {
    *(RealFileName - 1) = TempChar;
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
    NewPrivateFile->LHandle = CreateFile (
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
      CloseHandle (NewPrivateFile->LHandle);
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
    Size         = StrSize (NewPrivateFile->FileName);
    Size        += StrSize (L"\\*");
    TempFileName = AllocatePool (Size);
    if (TempFileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpyS (TempFileName, Size / sizeof (CHAR16), NewPrivateFile->FileName);

    if ((OpenMode & EFI_FILE_MODE_CREATE)) {
      //
      // Create a directory
      //
      if (!CreateDirectory (TempFileName, NULL)) {
        LastError = GetLastError ();
        if (LastError != ERROR_ALREADY_EXISTS) {
          FreePool (TempFileName);
          Status = EFI_ACCESS_DENIED;
          goto Done;
        }
      }
    }

    NewPrivateFile->DirHandle = CreateFile (
                                  TempFileName,
                                  NewPrivateFile->IsOpenedByRead ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_BACKUP_SEMANTICS,
                                  NULL
                                  );

    if (NewPrivateFile->DirHandle == INVALID_HANDLE_VALUE) {
      NewPrivateFile->DirHandle = CreateFile (
                                    TempFileName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL
                                    );

      if (NewPrivateFile->DirHandle != INVALID_HANDLE_VALUE) {
        CloseHandle (NewPrivateFile->DirHandle);
        NewPrivateFile->DirHandle = INVALID_HANDLE_VALUE;
        Status                    = EFI_ACCESS_DENIED;
      } else {
        Status = EFI_NOT_FOUND;
      }

      FreePool (TempFileName);
      goto Done;
    }

    //
    // Find the first file under it
    //
    StrCatS (TempFileName, Size / sizeof (CHAR16), L"\\*");
    NewPrivateFile->LHandle = FindFirstFile (TempFileName, &NewPrivateFile->FindBuf);
    FreePool (TempFileName);

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
      NewPrivateFile->LHandle = CreateFile (
                                  NewPrivateFile->FileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  (OpenMode & EFI_FILE_MODE_CREATE) ? OPEN_ALWAYS : OPEN_EXISTING,
                                  0,
                                  NULL
                                  );

      if (NewPrivateFile->LHandle == INVALID_HANDLE_VALUE) {
        NewPrivateFile->LHandle = CreateFile (
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
          CloseHandle (NewPrivateFile->LHandle);
          NewPrivateFile->LHandle = INVALID_HANDLE_VALUE;
        }
      }
    } else {
      NewPrivateFile->LHandle = CreateFile (
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

  if ((OpenMode & EFI_FILE_MODE_CREATE) && (Status == EFI_SUCCESS)) {
    //
    // Set the attribute
    //
    InfoSize = 0;
    Info     = NULL;

    Status = WinNtFileGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);

    if (Status != EFI_BUFFER_TOO_SMALL) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    Info = AllocatePool (InfoSize);
    if (Info == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    Status = WinNtFileGetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, &InfoSize, Info);

    if (EFI_ERROR (Status)) {
      FreePool (Info);
      goto Done;
    }

    Info->Attribute = Attributes;

    WinNtFileSetInfo (&NewPrivateFile->EfiFile, &gEfiFileInfoGuid, InfoSize, Info);
    FreePool (Info);
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

/**
  Close the file handle

  @param  This          Protocol instance pointer.

  @retval EFI_SUCCESS   The device was opened.

**/
EFI_STATUS
WinNtFileClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  WIN_NT_EFI_FILE_PRIVATE  *PrivateFile;

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->LHandle != INVALID_HANDLE_VALUE) {
    if (PrivateFile->IsDirectoryPath) {
      FindClose (PrivateFile->LHandle);
    } else {
      CloseHandle (PrivateFile->LHandle);
    }

    PrivateFile->LHandle = INVALID_HANDLE_VALUE;
  }

  if (PrivateFile->IsDirectoryPath && (PrivateFile->DirHandle != INVALID_HANDLE_VALUE)) {
    CloseHandle (PrivateFile->DirHandle);
    PrivateFile->DirHandle = INVALID_HANDLE_VALUE;
  }

  if (PrivateFile->FileName) {
    FreePool (PrivateFile->FileName);
  }

  if (PrivateFile->FilePath) {
    FreePool (PrivateFile->FilePath);
  }

  FreePool (PrivateFile);

  return EFI_SUCCESS;
}

/**
  Close and delete the file handle.

  @param  This                     Protocol instance pointer.

  @retval EFI_SUCCESS              The device was opened.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed but the file was not deleted.

**/
EFI_STATUS
WinNtFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS               Status;
  WIN_NT_EFI_FILE_PRIVATE  *PrivateFile;

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = EFI_WARN_DELETE_FAILURE;

  if (PrivateFile->IsDirectoryPath) {
    if (PrivateFile->LHandle != INVALID_HANDLE_VALUE) {
      FindClose (PrivateFile->LHandle);
    }

    if (PrivateFile->DirHandle != INVALID_HANDLE_VALUE) {
      CloseHandle (PrivateFile->DirHandle);
      PrivateFile->DirHandle = INVALID_HANDLE_VALUE;
    }

    if (RemoveDirectory (PrivateFile->FileName)) {
      Status = EFI_SUCCESS;
    }
  } else {
    CloseHandle (PrivateFile->LHandle);
    PrivateFile->LHandle = INVALID_HANDLE_VALUE;

    if (!PrivateFile->IsOpenedByRead) {
      if (DeleteFile (PrivateFile->FileName)) {
        Status = EFI_SUCCESS;
      }
    }
  }

  FreePool (PrivateFile->FileName);
  FreePool (PrivateFile->FilePath);
  FreePool (PrivateFile);

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
  Time->Year       = (UINT16)SystemTime->wYear;
  Time->Month      = (UINT8)SystemTime->wMonth;
  Time->Day        = (UINT8)SystemTime->wDay;
  Time->Hour       = (UINT8)SystemTime->wHour;
  Time->Minute     = (UINT8)SystemTime->wMinute;
  Time->Second     = (UINT8)SystemTime->wSecond;
  Time->Nanosecond = (UINT32)SystemTime->wMilliseconds * 1000000;
  Time->TimeZone   = (INT16)TimeZone->Bias;

  if (TimeZone->StandardDate.wMonth) {
    Time->Daylight = EFI_TIME_ADJUST_DAYLIGHT;
  }
}

/**
  Convert the FileTime to EfiTime.

  @param PrivateFile  Pointer to WIN_NT_EFI_FILE_PRIVATE.
  @param TimeZone     Pointer to the current time zone.
  @param FileTime     Pointer to file time.
  @param EfiTime      Pointer to EFI time.
**/
VOID
WinNtFileTimeToEfiTime (
  IN CONST WIN_NT_EFI_FILE_PRIVATE  *PrivateFile,
  IN       TIME_ZONE_INFORMATION    *TimeZone,
  IN CONST FILETIME                 *FileTime,
  OUT      EFI_TIME                 *EfiTime
  )
{
  FILETIME    TempFileTime;
  SYSTEMTIME  SystemTime;

  FileTimeToLocalFileTime (FileTime, &TempFileTime);
  FileTimeToSystemTime (&TempFileTime, &SystemTime);
  WinNtSystemTimeToEfiTime (&SystemTime, TimeZone, EfiTime);
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
WinNtFileRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  WIN_NT_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_STATUS               Status;
  UINTN                    Size;
  UINTN                    NameSize;
  UINTN                    ResultSize;
  UINTN                    Index;
  EFI_FILE_INFO            *Info;
  WCHAR                    *pw;
  TIME_ZONE_INFORMATION    TimeZone;
  EFI_FILE_INFO            *FileInfo;
  UINT64                   Pos;
  UINT64                   FileSize;
  UINTN                    FileInfoSize;

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
    FileInfo     = AllocatePool (FileInfoSize);

    Status = This->GetInfo (
                     This,
                     &gEfiFileInfoGuid,
                     &FileInfoSize,
                     FileInfo
                     );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (FileInfo);
      FileInfo = AllocatePool (FileInfoSize);
      Status   = This->GetInfo (
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

    Status = ReadFile (
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
    Status      = EFI_SUCCESS;
    goto Done;
  }

  Size = SIZE_OF_EFI_FILE_INFO;

  NameSize = StrSize (PrivateFile->FindBuf.cFileName);

  ResultSize = Size + NameSize;

  Status = EFI_BUFFER_TOO_SMALL;

  if (*BufferSize >= ResultSize) {
    Status = EFI_SUCCESS;

    Info = Buffer;
    ZeroMem (Info, ResultSize);

    Info->Size = ResultSize;

    GetTimeZoneInformation (&TimeZone);
    WinNtFileTimeToEfiTime (PrivateFile, &TimeZone, &PrivateFile->FindBuf.ftCreationTime, &Info->CreateTime);
    WinNtFileTimeToEfiTime (PrivateFile, &TimeZone, &PrivateFile->FindBuf.ftLastAccessTime, &Info->LastAccessTime);
    WinNtFileTimeToEfiTime (PrivateFile, &TimeZone, &PrivateFile->FindBuf.ftLastWriteTime, &Info->ModificationTime);

    Info->FileSize = PrivateFile->FindBuf.nFileSizeLow;

    Info->PhysicalSize = PrivateFile->FindBuf.nFileSizeLow;

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

    NameSize = NameSize / sizeof (WCHAR);

    pw = (WCHAR *)(((CHAR8 *)Buffer) + Size);

    for (Index = 0; Index < NameSize; Index++) {
      pw[Index] = PrivateFile->FindBuf.cFileName[Index];
    }

    if (FindNextFile (PrivateFile->LHandle, &PrivateFile->FindBuf)) {
      PrivateFile->IsValidFindBuf = TRUE;
    } else {
      PrivateFile->IsValidFindBuf = FALSE;
    }
  }

  *BufferSize = ResultSize;

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
WinNtFileWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  WIN_NT_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_STATUS               Status;

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

  Status = WriteFile (
             PrivateFile->LHandle,
             Buffer,
             (DWORD)*BufferSize,
             (LPDWORD)BufferSize,
             NULL
             ) ? EFI_SUCCESS : EFI_DEVICE_ERROR;

Done:
  return Status;

  //
  // bugbug: need to access windows error reporting
  //
}

/**
  Set a files current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open.

**/
EFI_STATUS
WinNtFileSetPossition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  EFI_STATUS               Status;
  WIN_NT_EFI_FILE_PRIVATE  *PrivateFile;
  UINT32                   PosLow;
  UINT32                   PosHigh;
  CHAR16                   *FileName;
  UINTN                    Size;

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  if (PrivateFile->IsDirectoryPath) {
    if (Position != 0) {
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    Size     = StrSize (PrivateFile->FileName);
    Size    += StrSize (L"\\*");
    FileName = AllocatePool (Size);
    if (FileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpyS (FileName, Size / sizeof (CHAR16), PrivateFile->FileName);
    StrCatS (FileName, Size / sizeof (CHAR16), L"\\*");

    if (PrivateFile->LHandle != INVALID_HANDLE_VALUE) {
      FindClose (PrivateFile->LHandle);
    }

    PrivateFile->LHandle = FindFirstFile (FileName, &PrivateFile->FindBuf);

    if (PrivateFile->LHandle == INVALID_HANDLE_VALUE) {
      PrivateFile->IsValidFindBuf = FALSE;
    } else {
      PrivateFile->IsValidFindBuf = TRUE;
    }

    FreePool (FileName);

    Status = (PrivateFile->LHandle == INVALID_HANDLE_VALUE) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  } else {
    if (Position == (UINT64)-1) {
      PosLow = SetFilePointer (PrivateFile->LHandle, (ULONG)0, NULL, FILE_END);
    } else {
      PosHigh = (UINT32)RShiftU64 (Position, 32);

      PosLow = SetFilePointer (PrivateFile->LHandle, (ULONG)Position, (PLONG)&PosHigh, FILE_BEGIN);
    }

    Status = (PosLow == 0xFFFFFFFF) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  }

Done:
  return Status;
}

/**
  Get a file's current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open..

**/
EFI_STATUS
WinNtFileGetPossition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  )
{
  EFI_STATUS               Status;
  WIN_NT_EFI_FILE_PRIVATE  *PrivateFile;
  INT32                    PositionHigh;
  UINT64                   PosHigh64;

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  PositionHigh = 0;
  PosHigh64    = 0;

  if (PrivateFile->IsDirectoryPath) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  } else {
    PositionHigh = 0;
    *Position    = SetFilePointer (
                     PrivateFile->LHandle,
                     0,
                     (PLONG)&PositionHigh,
                     FILE_CURRENT
                     );

    Status = *Position == 0xffffffff ? EFI_DEVICE_ERROR : EFI_SUCCESS;
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    PosHigh64  = PositionHigh;
    *Position += LShiftU64 (PosHigh64, 32);
  }

Done:
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
  CHAR16                      *RealFileName;
  CHAR16                      *TempPointer;
  TIME_ZONE_INFORMATION       TimeZone;

  Size = SIZE_OF_EFI_FILE_INFO;

  RealFileName = PrivateFile->FileName;
  TempPointer  = RealFileName;
  while (*TempPointer) {
    if (*TempPointer == '\\') {
      RealFileName = TempPointer + 1;
    }

    TempPointer++;
  }

  NameSize = StrSize (RealFileName);

  ResultSize = Size + NameSize;

  Status = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status = EFI_SUCCESS;

    Info = Buffer;
    ZeroMem (Info, ResultSize);

    Info->Size = ResultSize;
    GetFileInformationByHandle (
      PrivateFile->IsDirectoryPath ? PrivateFile->DirHandle : PrivateFile->LHandle,
      &FileInfo
      );
    Info->FileSize     = FileInfo.nFileSizeLow;
    Info->PhysicalSize = Info->FileSize;

    GetTimeZoneInformation (&TimeZone);
    WinNtFileTimeToEfiTime (PrivateFile, &TimeZone, &FileInfo.ftCreationTime, &Info->CreateTime);
    WinNtFileTimeToEfiTime (PrivateFile, &TimeZone, &FileInfo.ftLastAccessTime, &Info->LastAccessTime);
    WinNtFileTimeToEfiTime (PrivateFile, &TimeZone, &FileInfo.ftLastWriteTime, &Info->ModificationTime);

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

    if (PrivateFile->IsRootDirectory) {
      *((CHAR8 *)Buffer + Size) = 0;
    } else {
      CopyMem ((CHAR8 *)Buffer + Size, RealFileName, NameSize);
    }
  }

  *BufferSize = ResultSize;
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
WinNtFileGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  EFI_STATUS                         Status;
  WIN_NT_EFI_FILE_PRIVATE            *PrivateFile;
  EFI_FILE_SYSTEM_INFO               *FileSystemInfoBuffer;
  UINT32                             SectorsPerCluster;
  UINT32                             BytesPerSector;
  UINT32                             FreeClusters;
  UINT32                             TotalClusters;
  UINT32                             BytesPerCluster;
  CHAR16                             *DriveName;
  BOOLEAN                            DriveNameFound;
  BOOL                               NtStatus;
  UINTN                              Index;
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE  *PrivateRoot;

  if ((This == NULL) || (InformationType == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);

  Status = EFI_UNSUPPORTED;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = WinNtSimpleFileSystemFileInfo (PrivateFile, BufferSize, Buffer);
  }

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    if (*BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel)) {
      *BufferSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
      Status      = EFI_BUFFER_TOO_SMALL;
      goto Done;
    }

    FileSystemInfoBuffer           = (EFI_FILE_SYSTEM_INFO *)Buffer;
    FileSystemInfoBuffer->Size     = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
    FileSystemInfoBuffer->ReadOnly = FALSE;

    //
    // Try to get the drive name
    //
    DriveNameFound = FALSE;
    DriveName      = AllocatePool (StrSize (PrivateFile->FilePath) + 1);
    if (DriveName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpyS (
      DriveName,
      (StrSize (PrivateFile->FilePath) + 1) / sizeof (CHAR16),
      PrivateFile->FilePath
      );
    for (Index = 0; DriveName[Index] != 0 && DriveName[Index] != ':'; Index++) {
    }

    if (DriveName[Index] == ':') {
      DriveName[Index + 1] = '\\';
      DriveName[Index + 2] = 0;
      DriveNameFound       = TRUE;
    } else if ((DriveName[0] == '\\') && (DriveName[1] == '\\')) {
      for (Index = 2; DriveName[Index] != 0 && DriveName[Index] != '\\'; Index++) {
      }

      if (DriveName[Index] == '\\') {
        DriveNameFound = TRUE;
        for (Index++; DriveName[Index] != 0 && DriveName[Index] != '\\'; Index++) {
        }

        DriveName[Index]     = '\\';
        DriveName[Index + 1] = 0;
      }
    }

    //
    // Try GetDiskFreeSpace first
    //
    NtStatus = GetDiskFreeSpace (
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
      BytesPerCluster                  = BytesPerSector * SectorsPerCluster;
      FileSystemInfoBuffer->VolumeSize = MultU64x32 (TotalClusters, BytesPerCluster);
      FileSystemInfoBuffer->FreeSpace  = MultU64x32 (FreeClusters, BytesPerCluster);
      FileSystemInfoBuffer->BlockSize  = BytesPerCluster;
    } else {
      //
      // try GetDiskFreeSpaceEx then
      //
      FileSystemInfoBuffer->BlockSize = 0;
      NtStatus                        = GetDiskFreeSpaceEx (
                                          PrivateFile->FilePath,
                                          (PULARGE_INTEGER)(&FileSystemInfoBuffer->FreeSpace),
                                          (PULARGE_INTEGER)(&FileSystemInfoBuffer->VolumeSize),
                                          NULL
                                          );
      if (!NtStatus) {
        Status = EFI_DEVICE_ERROR;
        goto Done;
      }
    }

    StrCpyS (
      (CHAR16 *)FileSystemInfoBuffer->VolumeLabel,
      (*BufferSize - SIZE_OF_EFI_FILE_SYSTEM_INFO) / sizeof (CHAR16),
      PrivateRoot->VolumeLabel
      );
    *BufferSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (PrivateRoot->VolumeLabel);
    Status      = EFI_SUCCESS;
  }

  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    if (*BufferSize < StrSize (PrivateRoot->VolumeLabel)) {
      *BufferSize = StrSize (PrivateRoot->VolumeLabel);
      Status      = EFI_BUFFER_TOO_SMALL;
      goto Done;
    }

    StrCpyS (
      (CHAR16 *)Buffer,
      *BufferSize / sizeof (CHAR16),
      PrivateRoot->VolumeLabel
      );
    *BufferSize = StrSize (PrivateRoot->VolumeLabel);
    Status      = EFI_SUCCESS;
  }

Done:
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
WinNtFileSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE  *PrivateRoot;
  WIN_NT_EFI_FILE_PRIVATE            *PrivateFile;
  EFI_FILE_INFO                      *OldFileInfo;
  EFI_FILE_INFO                      *NewFileInfo;
  EFI_STATUS                         Status;
  UINTN                              OldInfoSize;
  INTN                               NtStatus;
  UINT32                             NewAttr;
  UINT32                             OldAttr;
  CHAR16                             *OldFileName;
  CHAR16                             *NewFileName;
  CHAR16                             *TempFileName;
  CHAR16                             *CharPointer;
  BOOLEAN                            AttrChangeFlag;
  BOOLEAN                            NameChangeFlag;
  BOOLEAN                            SizeChangeFlag;
  BOOLEAN                            TimeChangeFlag;
  UINT64                             CurPos;
  SYSTEMTIME                         NewCreationSystemTime;
  SYSTEMTIME                         NewLastAccessSystemTime;
  SYSTEMTIME                         NewLastWriteSystemTime;
  FILETIME                           NewCreationFileTime;
  FILETIME                           NewLastAccessFileTime;
  FILETIME                           NewLastWriteFileTime;
  WIN32_FIND_DATA                    FindBuf;
  EFI_FILE_SYSTEM_INFO               *NewFileSystemInfo;
  UINTN                              Size;

  //
  // Initialise locals.
  //
  PrivateFile = WIN_NT_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);
  PrivateRoot = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (PrivateFile->SimpleFileSystem);

  Status         = EFI_UNSUPPORTED;
  OldFileInfo    = NewFileInfo = NULL;
  OldFileName    = NewFileName = NULL;
  AttrChangeFlag = NameChangeFlag = SizeChangeFlag = TimeChangeFlag = FALSE;

  //
  // Set file system information.
  //
  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    NewFileSystemInfo = (EFI_FILE_SYSTEM_INFO *)Buffer;
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

    StrCpyS (
      PrivateRoot->VolumeLabel,
      StrSize (NewFileSystemInfo->VolumeLabel) / sizeof (CHAR16),
      NewFileSystemInfo->VolumeLabel
      );

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

    StrCpyS (
      PrivateRoot->VolumeLabel,
      StrSize (PrivateRoot->VolumeLabel) / sizeof (CHAR16),
      (CHAR16 *)Buffer
      );

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
  NewFileInfo = (EFI_FILE_INFO *)Buffer;

  if ((NewFileInfo->Size <= SIZE_OF_EFI_FILE_INFO) ||
      (NewFileInfo->Attribute &~(EFI_FILE_VALID_ATTR)) ||
      ((sizeof (UINTN) == 4) && (NewFileInfo->Size > 0xFFFFFFFF))
      )
  {
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

  StrCpyS (
    OldFileName,
    StrSize (PrivateFile->FileName) / sizeof (CHAR16),
    PrivateFile->FileName
    );

  //
  // Make full pathname from new filename and rootpath.
  //
  if (NewFileInfo->FileName[0] == '\\') {
    Size        = StrSize (PrivateRoot->FilePath);
    Size       += StrSize (L"\\");
    Size       += StrSize (NewFileInfo->FileName);
    NewFileName = AllocatePool (Size);
    if (NewFileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpyS (NewFileName, Size / sizeof (CHAR16), PrivateRoot->FilePath);
    StrCatS (NewFileName, Size / sizeof (CHAR16), L"\\");
    StrCatS (NewFileName, Size / sizeof (CHAR16), NewFileInfo->FileName + 1);
  } else {
    Size        = StrSize (PrivateFile->FilePath);
    Size       += StrSize (L"\\");
    Size       += StrSize (NewFileInfo->FileName);
    NewFileName = AllocatePool (Size);
    if (NewFileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpyS (NewFileName, Size / sizeof (CHAR16), PrivateFile->FilePath);
    StrCatS (NewFileName, Size / sizeof (CHAR16), L"\\");
    StrCatS (NewFileName, Size / sizeof (CHAR16), NewFileInfo->FileName);
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
      )
  {
    TimeChangeFlag = TRUE;
  } else if (!IsZero (&NewFileInfo->LastAccessTime, sizeof (EFI_TIME)) &&
             CompareMem (&NewFileInfo->LastAccessTime, &OldFileInfo->LastAccessTime, sizeof (EFI_TIME))
             )
  {
    TimeChangeFlag = TRUE;
  } else if (!IsZero (&NewFileInfo->ModificationTime, sizeof (EFI_TIME)) &&
             CompareMem (&NewFileInfo->ModificationTime, &OldFileInfo->ModificationTime, sizeof (EFI_TIME))
             )
  {
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
  OldAttr = GetFileAttributes (OldFileName);

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
        FindClose (PrivateFile->LHandle);
      } else {
        CloseHandle (PrivateFile->LHandle);
        PrivateFile->LHandle = INVALID_HANDLE_VALUE;
      }
    }

    if (PrivateFile->IsDirectoryPath && (PrivateFile->DirHandle != INVALID_HANDLE_VALUE)) {
      CloseHandle (PrivateFile->DirHandle);
      PrivateFile->DirHandle = INVALID_HANDLE_VALUE;
    }

    NtStatus = MoveFile (OldFileName, NewFileName);

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

      StrCpyS (PrivateFile->FileName, StrSize (NewFileName) / sizeof (CHAR16), NewFileName);

      Size         = StrSize (NewFileName);
      Size        += StrSize (L"\\*");
      TempFileName = AllocatePool (Size);

      StrCpyS (TempFileName, Size / sizeof (CHAR16), NewFileName);

      if (!PrivateFile->IsDirectoryPath) {
        PrivateFile->LHandle = CreateFile (
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
        if (FlushFileBuffers (PrivateFile->LHandle) == 0) {
          Status = EFI_DEVICE_ERROR;
          goto Done;
        }
      } else {
        PrivateFile->DirHandle = CreateFile (
                                   TempFileName,
                                   PrivateFile->IsOpenedByRead ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS,
                                   NULL
                                   );

        StrCatS (TempFileName, Size / sizeof (CHAR16), L"\\*");
        PrivateFile->LHandle = FindFirstFile (TempFileName, &FindBuf);

        FreePool (TempFileName);
      }
    } else {
      Status = EFI_ACCESS_DENIED;
Reopen:;

      NtStatus = SetFileAttributes (OldFileName, OldAttr);

      if (!NtStatus) {
        goto Done;
      }

      Size         = StrSize (OldFileName);
      Size        += StrSize (L"\\*");
      TempFileName = AllocatePool (Size);

      StrCpyS (TempFileName, Size / sizeof (CHAR16), OldFileName);

      if (!PrivateFile->IsDirectoryPath) {
        PrivateFile->LHandle = CreateFile (
                                 TempFileName,
                                 PrivateFile->IsOpenedByRead ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL
                                 );
      } else {
        PrivateFile->DirHandle = CreateFile (
                                   TempFileName,
                                   PrivateFile->IsOpenedByRead ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS,
                                   NULL
                                   );

        StrCatS (TempFileName, Size / sizeof (CHAR16), L"\\*");
        PrivateFile->LHandle = FindFirstFile (TempFileName, &FindBuf);
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

    if (SetEndOfFile (PrivateFile->LHandle) == 0) {
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

    if (!SystemTimeToFileTime (
           &NewCreationSystemTime,
           &NewCreationFileTime
           ))
    {
      goto Done;
    }

    if (!LocalFileTimeToFileTime (
           &NewCreationFileTime,
           &NewCreationFileTime
           ))
    {
      goto Done;
    }

    NewLastAccessSystemTime.wYear         = NewFileInfo->LastAccessTime.Year;
    NewLastAccessSystemTime.wMonth        = NewFileInfo->LastAccessTime.Month;
    NewLastAccessSystemTime.wDay          = NewFileInfo->LastAccessTime.Day;
    NewLastAccessSystemTime.wHour         = NewFileInfo->LastAccessTime.Hour;
    NewLastAccessSystemTime.wMinute       = NewFileInfo->LastAccessTime.Minute;
    NewLastAccessSystemTime.wSecond       = NewFileInfo->LastAccessTime.Second;
    NewLastAccessSystemTime.wMilliseconds = 0;

    if (!SystemTimeToFileTime (
           &NewLastAccessSystemTime,
           &NewLastAccessFileTime
           ))
    {
      goto Done;
    }

    if (!LocalFileTimeToFileTime (
           &NewLastAccessFileTime,
           &NewLastAccessFileTime
           ))
    {
      goto Done;
    }

    NewLastWriteSystemTime.wYear         = NewFileInfo->ModificationTime.Year;
    NewLastWriteSystemTime.wMonth        = NewFileInfo->ModificationTime.Month;
    NewLastWriteSystemTime.wDay          = NewFileInfo->ModificationTime.Day;
    NewLastWriteSystemTime.wHour         = NewFileInfo->ModificationTime.Hour;
    NewLastWriteSystemTime.wMinute       = NewFileInfo->ModificationTime.Minute;
    NewLastWriteSystemTime.wSecond       = NewFileInfo->ModificationTime.Second;
    NewLastWriteSystemTime.wMilliseconds = 0;

    if (!SystemTimeToFileTime (
           &NewLastWriteSystemTime,
           &NewLastWriteFileTime
           ))
    {
      goto Done;
    }

    if (!LocalFileTimeToFileTime (
           &NewLastWriteFileTime,
           &NewLastWriteFileTime
           ))
    {
      goto Done;
    }

    if (!SetFileTime (
           PrivateFile->IsDirectoryPath ? PrivateFile->DirHandle : PrivateFile->LHandle,
           &NewCreationFileTime,
           &NewLastAccessFileTime,
           &NewLastWriteFileTime
           ))
    {
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

  NtStatus = SetFileAttributes (NewFileName, NewAttr);

  if (!NtStatus) {
    Status = EFI_DEVICE_ERROR;
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
WinNtFileFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  BY_HANDLE_FILE_INFORMATION  FileInfo;
  WIN_NT_EFI_FILE_PRIVATE     *PrivateFile;
  EFI_STATUS                  Status;

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

  GetFileInformationByHandle (PrivateFile->LHandle, &FileInfo);

  if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
    Status = EFI_ACCESS_DENIED;
    goto Done;
  }

  Status = FlushFileBuffers (PrivateFile->LHandle) ? EFI_SUCCESS : EFI_DEVICE_ERROR;

Done:
  return Status;
  //
  // bugbug: - Use Windows error reporting.
  //
}

EFI_STATUS
WinNtFileSystmeThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE  *Private;

  Private = AllocateZeroPool (sizeof (*Private));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->FilePath = AllocateCopyPool (StrSize (This->ConfigString), This->ConfigString);
  if (Private->FilePath == NULL) {
    FreePool (Private);
    return EFI_OUT_OF_RESOURCES;
  }

  Private->VolumeLabel = AllocateCopyPool (StrSize (L"EFI_EMULATED"), L"EFI_EMULATED");
  if (Private->VolumeLabel == NULL) {
    FreePool (Private->FilePath);
    FreePool (Private);
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE;
  Private->Thunk     = This;
  CopyMem (&Private->SimpleFileSystem, &gWinNtFileSystemProtocol, sizeof (Private->SimpleFileSystem));

  This->Interface = &Private->SimpleFileSystem;
  This->Private   = Private;
  return EFI_SUCCESS;
}

EFI_STATUS
WinNtFileSystmeThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  WIN_NT_SIMPLE_FILE_SYSTEM_PRIVATE  *Private;

  Private = This->Private;
  ASSERT (Private != NULL);

  if (Private->VolumeLabel != NULL) {
    FreePool (Private->VolumeLabel);
  }

  if (Private->FilePath != NULL) {
    FreePool (Private->FilePath);
  }

  FreePool (Private);
  return EFI_SUCCESS;
}

EFI_FILE_PROTOCOL  gWinNtFileProtocol = {
  EFI_FILE_REVISION,
  WinNtFileOpen,
  WinNtFileClose,
  WinNtFileDelete,
  WinNtFileRead,
  WinNtFileWrite,
  WinNtFileGetPossition,
  WinNtFileSetPossition,
  WinNtFileGetInfo,
  WinNtFileSetInfo,
  WinNtFileFlush
};

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  gWinNtFileSystemProtocol = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  WinNtOpenVolume
};

EMU_IO_THUNK_PROTOCOL  mWinNtFileSystemThunkIo = {
  &gEfiSimpleFileSystemProtocolGuid,
  NULL,
  NULL,
  0,
  WinNtFileSystmeThunkOpen,
  WinNtFileSystmeThunkClose,
  NULL
};

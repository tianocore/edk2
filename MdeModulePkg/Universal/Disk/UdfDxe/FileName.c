/** @file
  Helper functions for mangling file names in UDF/ECMA-167 file systems.

  Copyright (C) 2014-2017 Paulo Alcantara <pcacjr@zytor.com>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "Udf.h"

CHAR16 *
TrimString (
  IN CHAR16    *String
  )
{
  CHAR16       *TempString;

  for ( ; *String != L'\0' && *String == L' '; String++) {
    ;
  }

  TempString = String + StrLen (String) - 1;
  while ((TempString >= String) && (*TempString == L' ')) {
    TempString--;
  }

  *(TempString + 1) = L'\0';

  return String;
}

VOID
ReplaceLeft (
  IN CHAR16         *Destination,
  IN CONST CHAR16   *Source
  )
{
  CONST CHAR16      *EndString;

  EndString = Source + StrLen (Source);
  while (Source <= EndString) {
    *Destination++ = *Source++;
  }
}

CHAR16 *
ExcludeTrailingBackslashes (
  IN CHAR16                    *String
  )
{
  CHAR16                       *TempString;

  switch (*(String + 1)) {
  case L'\\':
    break;
  case L'\0':
  default:
    String++;
    goto Exit;
  }

  TempString = String;
  while (*TempString != L'\0' && *TempString == L'\\') {
    TempString++;
  }

  if (TempString - 1 > String) {
    ReplaceLeft (String + 1, TempString);
  }

  String++;

Exit:
  return String;
}

/**
  Mangle a filename by cutting off trailing whitespaces, "\\", "." and "..".

  @param[in] FileName Filename.

  @retval @p FileName Filename mangled.

**/
CHAR16 *
MangleFileName (
  IN CHAR16        *FileName
  )
{
  CHAR16           *FileNameSavedPointer;
  CHAR16           *TempFileName;
  UINTN            BackslashesNo;

  if (FileName == NULL || *FileName == L'\0') {
    FileName = NULL;
    goto Exit;
  }

  FileName = TrimString (FileName);
  if (*FileName == L'\0') {
    goto Exit;
  }

  if ((StrLen (FileName) > 1) && (FileName[StrLen (FileName) - 1] == L'\\')) {
    FileName[StrLen (FileName) - 1] = L'\0';
  }

  FileNameSavedPointer = FileName;

  if (FileName[0] == L'.') {
    if (FileName[1] == L'.') {
      if (FileName[2] == L'\0') {
        goto Exit;
      } else {
        FileName += 2;
      }
    } else if (FileName[1] == L'\0') {
      goto Exit;
    }
  }

  while (*FileName != L'\0') {
    if (*FileName == L'\\') {
      FileName = ExcludeTrailingBackslashes (FileName);
    } else if (*FileName == L'.') {
      switch (*(FileName + 1)) {
      case L'\0':
        *FileName = L'\0';
        break;
      case L'\\':
        TempFileName = FileName + 1;
        TempFileName = ExcludeTrailingBackslashes (TempFileName);
        ReplaceLeft (FileName, TempFileName);
        break;
      case '.':
        if ((*(FileName - 1) != L'\\') && ((*(FileName + 2) != L'\\') ||
                                           (*(FileName + 2) != L'\0'))) {
          FileName++;
          continue;
        }

        BackslashesNo = 0;
        TempFileName = FileName - 1;
        while (TempFileName >= FileNameSavedPointer) {
          if (*TempFileName == L'\\') {
            if (++BackslashesNo == 2) {
              break;
            }
          }

          TempFileName--;
        }

        TempFileName++;

        if ((*TempFileName == L'.') && (*(TempFileName + 1) == L'.')) {
          FileName += 2;
        } else {
          if (*(FileName + 2) != L'\0') {
            ReplaceLeft (TempFileName, FileName + 3);
            if (*(TempFileName - 1) == L'\\') {
              FileName = TempFileName;
              ExcludeTrailingBackslashes (TempFileName - 1);
              TempFileName = FileName;
            }
          } else {
            *TempFileName = L'\0';
          }

          FileName = TempFileName;
        }

        break;
      default:
        FileName++;
      }
    } else {
      FileName++;
    }
  }

  FileName = FileNameSavedPointer;
  if ((StrLen (FileName) > 1) && (FileName [StrLen (FileName) - 1] == L'\\')) {
    FileName [StrLen (FileName) - 1] = L'\0';
  }

Exit:
  return FileName;
}

/** @file
  Implement the opendir, closedir, and readdir functions.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "internal.h"
#include <Library/ShellLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>

typedef struct {
  UINT32            Signature;
  SHELL_FILE_HANDLE DirHandle;
  EFI_FILE_INFO     *FileInfo;
  struct dirent     *DirentStructure;
} DIR_STRUCTURE;
  
DIR *                  opendir(const char * AsciiFileName)
{
  EFI_STATUS        Status;
  DIR_STRUCTURE     *DirStruct;
  CHAR16            *FileName;

  DirStruct = (DIR_STRUCTURE*)AllocateZeroPool(sizeof(DIR_STRUCTURE));
  if (DirStruct == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  FileName = (CHAR16*)AllocateZeroPool((1+AsciiStrLen(AsciiFileName))*sizeof(CHAR16));
  if (FileName == NULL) {
    FreePool(DirStruct);
    errno = ENOMEM;
    return NULL;
  }
  AsciiStrToUnicodeStr(AsciiFileName, FileName);

  Status = ShellOpenFileByName(FileName, &DirStruct->DirHandle, EFI_FILE_MODE_READ, 0);
  FreePool(FileName);
  if (EFI_ERROR(Status)) {
    errno = ENOENT;
    FreePool(DirStruct);
    return NULL;
  }
  DirStruct->Signature = 0x08675309;
  return ((DIR*)DirStruct);
}

int                    closedir(DIR * DirPointer)
{
  DIR_STRUCTURE     *DirStruct;

  if (DirPointer == NULL) {
    return 0;
  }

  DirStruct = (DIR_STRUCTURE*)DirPointer;
  if (DirStruct->Signature != 0x08675309) {
    return 0;
  }

  ShellCloseFile(DirStruct->DirHandle);
  SHELL_FREE_NON_NULL(DirStruct->FileInfo);
  SHELL_FREE_NON_NULL(DirStruct->DirentStructure);
  SHELL_FREE_NON_NULL(DirStruct);
  
  return 0;
}

struct dirent *        readdir(DIR * DirPointer)
{
  DIR_STRUCTURE     *DirStruct;
  EFI_STATUS        Status;
  BOOLEAN           NoFile;

  NoFile = FALSE;

  if (DirPointer == NULL) {
    errno = EBADF;
    return NULL;
  }

  DirStruct = (DIR_STRUCTURE*)DirPointer;
  if (DirStruct->Signature != 0x08675309) {
    errno = EBADF;
    return NULL;
  }

  if (DirStruct->FileInfo == NULL) {
    Status = ShellFindFirstFile(DirStruct->DirHandle, &(DirStruct->FileInfo));
  } else {
    Status = ShellFindNextFile(DirStruct->DirHandle, DirStruct->FileInfo, &NoFile);
  }

  if (EFI_ERROR(Status)) {
    errno = ENOENT;
    return NULL;
  }

  if (NoFile) {
    return (NULL);
  }

  SHELL_FREE_NON_NULL(DirStruct->DirentStructure);

  DirStruct->DirentStructure = AllocateZeroPool(sizeof(DIR_STRUCTURE)+(StrSize(DirStruct->FileInfo->FileName)));
  if (DirStruct->DirentStructure == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  StrCpy(DirStruct->FileInfo->FileName, DirStruct->DirentStructure->FileName);

  DirStruct->DirentStructure->FileSize                 = DirStruct->FileInfo->FileSize;
  DirStruct->DirentStructure->PhysicalSize             = DirStruct->FileInfo->PhysicalSize;
  DirStruct->DirentStructure->Attribute                = DirStruct->FileInfo->Attribute;
  DirStruct->DirentStructure->CreateTime.tv_sec        = Efi2Time(&DirStruct->FileInfo->CreateTime);
  DirStruct->DirentStructure->CreateTime.tv_nsec       = DirStruct->FileInfo->CreateTime.Nanosecond;
  DirStruct->DirentStructure->LastAccessTime.tv_nsec   = Efi2Time(&DirStruct->FileInfo->LastAccessTime);
  DirStruct->DirentStructure->LastAccessTime.tv_sec    = DirStruct->FileInfo->LastAccessTime.Nanosecond;
  DirStruct->DirentStructure->ModificationTime.tv_sec  = Efi2Time(&DirStruct->FileInfo->ModificationTime);
  DirStruct->DirentStructure->ModificationTime.tv_nsec = DirStruct->FileInfo->ModificationTime.Nanosecond;
  DirStruct->DirentStructure->Size                     = StrSize(DirStruct->DirentStructure->FileName) + sizeof(DIR_STRUCTURE);

  return (DirStruct->DirentStructure);
}

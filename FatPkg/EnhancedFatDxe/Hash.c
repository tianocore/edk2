/*++

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Hash.c

Abstract:

  Hash table operations

Revision History

--*/

#include "Fat.h"

STATIC
UINT32
FatHashLongName (
  IN CHAR16   *LongNameString
  )
/*++

Routine Description:

  Get hash value for long name.

Arguments:

  LongNameString        - The long name string to be hashed.

Returns:

  HashValue.

--*/
{
  UINT32  HashValue;
  CHAR16  UpCasedLongFileName[EFI_PATH_STRING_LENGTH];
  StrnCpyS (
    UpCasedLongFileName,
    ARRAY_SIZE (UpCasedLongFileName),
    LongNameString,
    ARRAY_SIZE (UpCasedLongFileName) - 1
    );
  FatStrUpr (UpCasedLongFileName);
  gBS->CalculateCrc32 (UpCasedLongFileName, StrSize (UpCasedLongFileName), &HashValue);
  return (HashValue & HASH_TABLE_MASK);
}

STATIC
UINT32
FatHashShortName (
  IN CHAR8   *ShortNameString
  )
/*++

Routine Description:

  Get hash value for short name.

Arguments:

  ShortNameString       - The short name string to be hashed.

Returns:

  HashValue

--*/
{
  UINT32  HashValue;
  gBS->CalculateCrc32 (ShortNameString, FAT_NAME_LEN, &HashValue);
  return (HashValue & HASH_TABLE_MASK);
}

FAT_DIRENT **
FatLongNameHashSearch (
  IN FAT_ODIR       *ODir,
  IN CHAR16         *LongNameString
  )
/*++

Routine Description:

  Search the long name hash table for the directory entry.

Arguments:

  ODir                  - The directory to be searched.
  LongNameString        - The long name string to search.

Returns:

  The previous long name hash node of the directory entry.

--*/
{
  FAT_DIRENT  **PreviousHashNode;
  for (PreviousHashNode   = &ODir->LongNameHashTable[FatHashLongName (LongNameString)];
       *PreviousHashNode != NULL;
       PreviousHashNode   = &(*PreviousHashNode)->LongNameForwardLink
      ) {
    if (FatStriCmp (LongNameString, (*PreviousHashNode)->FileString) == 0) {
      break;
    }
  }

  return PreviousHashNode;
}

FAT_DIRENT **
FatShortNameHashSearch (
  IN FAT_ODIR      *ODir,
  IN CHAR8         *ShortNameString
  )
/*++

Routine Description:

  Search the short name hash table for the directory entry.

Arguments:

  ODir                  - The directory to be searched.
  ShortNameString       - The short name string to search.

Returns:

  The previous short name hash node of the directory entry.

--*/
{
  FAT_DIRENT  **PreviousHashNode;
  for (PreviousHashNode   = &ODir->ShortNameHashTable[FatHashShortName (ShortNameString)];
       *PreviousHashNode != NULL;
       PreviousHashNode   = &(*PreviousHashNode)->ShortNameForwardLink
      ) {
    if (CompareMem (ShortNameString, (*PreviousHashNode)->Entry.FileName, FAT_NAME_LEN) == 0) {
      break;
    }
  }

  return PreviousHashNode;
}

VOID
FatInsertToHashTable (
  IN FAT_ODIR     *ODir,
  IN FAT_DIRENT   *DirEnt
  )
/*++

Routine Description:

  Insert directory entry to hash table.

Arguments:

  ODir                  - The parent directory.
  DirEnt                - The directory entry node.

Returns:

  None.

--*/
{
  FAT_DIRENT  **HashTable;
  UINT32      HashTableIndex;

  //
  // Insert hash table index for short name
  //
  HashTableIndex                = FatHashShortName (DirEnt->Entry.FileName);
  HashTable                     = ODir->ShortNameHashTable;
  DirEnt->ShortNameForwardLink  = HashTable[HashTableIndex];
  HashTable[HashTableIndex]     = DirEnt;
  //
  // Insert hash table index for long name
  //
  HashTableIndex                = FatHashLongName (DirEnt->FileString);
  HashTable                     = ODir->LongNameHashTable;
  DirEnt->LongNameForwardLink   = HashTable[HashTableIndex];
  HashTable[HashTableIndex]     = DirEnt;
}

VOID
FatDeleteFromHashTable (
  IN FAT_ODIR     *ODir,
  IN FAT_DIRENT   *DirEnt
  )
/*++

Routine Description:

  Delete directory entry from hash table.

Arguments:

  ODir                  - The parent directory.
  DirEnt                - The directory entry node.

Returns:

  None.

--*/
{
  *FatShortNameHashSearch (ODir, DirEnt->Entry.FileName) = DirEnt->ShortNameForwardLink;
  *FatLongNameHashSearch (ODir, DirEnt->FileString)      = DirEnt->LongNameForwardLink;
}

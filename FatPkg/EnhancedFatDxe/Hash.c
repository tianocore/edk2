/** @file
  Hash table operations.

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

/**

  Get hash value for long name.

  @param  LongNameString        - The long name string to be hashed.

  @return HashValue.

**/
STATIC
UINT32
FatHashLongName (
  IN CHAR16  *LongNameString
  )
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

/**

  Get hash value for short name.

  @param  ShortNameString       - The short name string to be hashed.

  @return HashValue

**/
STATIC
UINT32
FatHashShortName (
  IN CHAR8  *ShortNameString
  )
{
  UINT32  HashValue;

  gBS->CalculateCrc32 (ShortNameString, FAT_NAME_LEN, &HashValue);
  return (HashValue & HASH_TABLE_MASK);
}

/**

  Search the long name hash table for the directory entry.

  @param  ODir                  - The directory to be searched.
  @param  LongNameString        - The long name string to search.

  @return The previous long name hash node of the directory entry.

**/
FAT_DIRENT **
FatLongNameHashSearch (
  IN FAT_ODIR  *ODir,
  IN CHAR16    *LongNameString
  )
{
  FAT_DIRENT  **PreviousHashNode;

  for (PreviousHashNode   = &ODir->LongNameHashTable[FatHashLongName (LongNameString)];
       *PreviousHashNode != NULL;
       PreviousHashNode   = &(*PreviousHashNode)->LongNameForwardLink
       )
  {
    if (FatStriCmp (LongNameString, (*PreviousHashNode)->FileString) == 0) {
      break;
    }
  }

  return PreviousHashNode;
}

/**

  Search the short name hash table for the directory entry.

  @param  ODir                  - The directory to be searched.
  @param  ShortNameString       - The short name string to search.

  @return The previous short name hash node of the directory entry.

**/
FAT_DIRENT **
FatShortNameHashSearch (
  IN FAT_ODIR  *ODir,
  IN CHAR8     *ShortNameString
  )
{
  FAT_DIRENT  **PreviousHashNode;

  for (PreviousHashNode   = &ODir->ShortNameHashTable[FatHashShortName (ShortNameString)];
       *PreviousHashNode != NULL;
       PreviousHashNode   = &(*PreviousHashNode)->ShortNameForwardLink
       )
  {
    if (CompareMem (ShortNameString, (*PreviousHashNode)->Entry.FileName, FAT_NAME_LEN) == 0) {
      break;
    }
  }

  return PreviousHashNode;
}

/**

  Insert directory entry to hash table.

  @param  ODir                  - The parent directory.
  @param  DirEnt                - The directory entry node.

**/
VOID
FatInsertToHashTable (
  IN FAT_ODIR    *ODir,
  IN FAT_DIRENT  *DirEnt
  )
{
  FAT_DIRENT  **HashTable;
  UINT32      HashTableIndex;

  //
  // Insert hash table index for short name
  //
  HashTableIndex               = FatHashShortName (DirEnt->Entry.FileName);
  HashTable                    = ODir->ShortNameHashTable;
  DirEnt->ShortNameForwardLink = HashTable[HashTableIndex];
  HashTable[HashTableIndex]    = DirEnt;
  //
  // Insert hash table index for long name
  //
  HashTableIndex              = FatHashLongName (DirEnt->FileString);
  HashTable                   = ODir->LongNameHashTable;
  DirEnt->LongNameForwardLink = HashTable[HashTableIndex];
  HashTable[HashTableIndex]   = DirEnt;
}

/**

  Delete directory entry from hash table.

  @param  ODir                  - The parent directory.
  @param  DirEnt                - The directory entry node.

**/
VOID
FatDeleteFromHashTable (
  IN FAT_ODIR    *ODir,
  IN FAT_DIRENT  *DirEnt
  )
{
  *FatShortNameHashSearch (ODir, DirEnt->Entry.FileName) = DirEnt->ShortNameForwardLink;
  *FatLongNameHashSearch (ODir, DirEnt->FileString)      = DirEnt->LongNameForwardLink;
}

/** @file
  Functions for directory cache operation.

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Fat.h"

/**

  Free the directory structure and release the memory.

  @param  ODir                  - The directory to be freed.

**/
STATIC
VOID
FatFreeODir (
  IN FAT_ODIR  *ODir
  )
{
  FAT_DIRENT  *DirEnt;

  //
  // Release Directory Entry Nodes
  //
  while (!IsListEmpty (&ODir->ChildList)) {
    DirEnt = DIRENT_FROM_LINK (ODir->ChildList.ForwardLink);
    RemoveEntryList (&DirEnt->Link);
    //
    // Make sure the OFile has been closed
    //
    ASSERT (DirEnt->OFile == NULL);
    FatFreeDirEnt (DirEnt);
  }

  FreePool (ODir);
}

/**

  Allocate the directory structure.

  @param  OFile                   - The corresponding OFile.

**/
STATIC
FAT_ODIR *
FatAllocateODir (
  IN FAT_OFILE  *OFile
  )
{
  FAT_ODIR  *ODir;

  ODir = AllocateZeroPool (sizeof (FAT_ODIR));
  if (ODir != NULL) {
    //
    // Initialize the directory entry list
    //
    ODir->Signature = FAT_ODIR_SIGNATURE;
    InitializeListHead (&ODir->ChildList);
    ODir->CurrentCursor = &ODir->ChildList;
  }

  return ODir;
}

/**

  Discard the directory structure when an OFile will be freed.
  Volume will cache this directory if the OFile does not represent a deleted file.

  @param  OFile                 - The OFile whose directory structure is to be discarded.

**/
VOID
FatDiscardODir (
  IN FAT_OFILE  *OFile
  )
{
  FAT_ODIR    *ODir;
  FAT_VOLUME  *Volume;

  Volume = OFile->Volume;
  ODir   = OFile->ODir;
  if (!OFile->DirEnt->Invalid) {
    //
    // If OFile does not represent a deleted file, then we will cache the directory
    // We use OFile's first cluster as the directory's tag
    //
    ODir->DirCacheTag = OFile->FileCluster;
    InsertHeadList (&Volume->DirCacheList, &ODir->DirCacheLink);
    if (Volume->DirCacheCount == FAT_MAX_DIR_CACHE_COUNT) {
      //
      // Replace the least recent used directory
      //
      ODir = ODIR_FROM_DIRCACHELINK (Volume->DirCacheList.BackLink);
      RemoveEntryList (&ODir->DirCacheLink);
    } else {
      //
      // No need to find a replace
      //
      Volume->DirCacheCount++;
      ODir = NULL;
    }
  }

  //
  // Release ODir Structure
  //
  if (ODir != NULL) {
    FatFreeODir (ODir);
  }
}

/**


  Request the directory structure when an OFile is newly generated.
  If the directory structure is cached by volume, then just return this directory;
  Otherwise, allocate a new one for OFile.

  @param  OFile                 - The OFile which requests directory structure.

**/
VOID
FatRequestODir (
  IN FAT_OFILE  *OFile
  )
{
  UINTN       DirCacheTag;
  FAT_VOLUME  *Volume;
  FAT_ODIR    *ODir;
  FAT_ODIR    *CurrentODir;
  LIST_ENTRY  *CurrentODirLink;

  Volume      = OFile->Volume;
  ODir        = NULL;
  DirCacheTag = OFile->FileCluster;
  for (CurrentODirLink  = Volume->DirCacheList.ForwardLink;
       CurrentODirLink != &Volume->DirCacheList;
       CurrentODirLink  = CurrentODirLink->ForwardLink
       )
  {
    CurrentODir = ODIR_FROM_DIRCACHELINK (CurrentODirLink);
    if (CurrentODir->DirCacheTag == DirCacheTag) {
      RemoveEntryList (&CurrentODir->DirCacheLink);
      Volume->DirCacheCount--;
      ODir = CurrentODir;
      break;
    }
  }

  if (ODir == NULL) {
    //
    // This directory is not cached, then allocate a new one
    //
    ODir = FatAllocateODir (OFile);
  }

  OFile->ODir = ODir;
}

/**

  Clean up all the cached directory structures when the volume is going to be abandoned.

  @param  Volume                - FAT file system volume.

**/
VOID
FatCleanupODirCache (
  IN FAT_VOLUME  *Volume
  )
{
  FAT_ODIR  *ODir;

  while (Volume->DirCacheCount > 0) {
    ODir = ODIR_FROM_DIRCACHELINK (Volume->DirCacheList.BackLink);
    RemoveEntryList (&ODir->DirCacheLink);
    FatFreeODir (ODir);
    Volume->DirCacheCount--;
  }
}

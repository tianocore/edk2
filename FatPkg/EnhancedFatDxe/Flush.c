/*++

Copyright (c) 2005 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  flush.c

Abstract:

  Routines that check references and flush OFiles

Revision History

--*/

#include "Fat.h"

EFI_STATUS
EFIAPI
FatFlush (
  IN EFI_FILE_PROTOCOL  *FHand
  )
/*++

Routine Description:

  Flushes all data associated with the file handle.

Arguments:

  FHand                 - Handle to file to flush.

Returns:

  EFI_SUCCESS           - Flushed the file successfully.
  EFI_WRITE_PROTECTED   - The volume is read only.
  EFI_ACCESS_DENIED     - The file is read only.
  Others                - Flushing of the file failed.

--*/
{
  FAT_IFILE   *IFile;
  FAT_OFILE   *OFile;
  FAT_VOLUME  *Volume;
  EFI_STATUS  Status;

  IFile   = IFILE_FROM_FHAND (FHand);
  OFile   = IFile->OFile;
  Volume  = OFile->Volume;

  //
  // If the file has a permanent error, return it
  //
  if (EFI_ERROR (OFile->Error)) {
    return OFile->Error;
  }

  if (Volume->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }
  //
  // If read only, return error
  //
  if (IFile->ReadOnly) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Flush the OFile
  //
  FatAcquireLock ();
  Status  = FatOFileFlush (OFile);
  Status  = FatCleanupVolume (OFile->Volume, OFile, Status);
  FatReleaseLock ();
  return Status;
}

EFI_STATUS
EFIAPI
FatClose (
  IN EFI_FILE_PROTOCOL  *FHand
  )
/*++

Routine Description:

  Flushes & Closes the file handle.

Arguments:

  FHand                 - Handle to the file to delete.

Returns:

  EFI_SUCCESS           - Closed the file successfully.

--*/
{
  FAT_IFILE   *IFile;
  FAT_OFILE   *OFile;
  FAT_VOLUME  *Volume;

  IFile   = IFILE_FROM_FHAND (FHand);
  OFile   = IFile->OFile;
  Volume  = OFile->Volume;

  //
  // Lock the volume
  //
  FatAcquireLock ();

  //
  // Close the file instance handle
  //
  FatIFileClose (IFile);

  //
  // Done. Unlock the volume
  //
  FatCleanupVolume (Volume, OFile, EFI_SUCCESS);
  FatReleaseLock ();

  //
  // Close always succeed
  //
  return EFI_SUCCESS;
}

EFI_STATUS
FatIFileClose (
  FAT_IFILE           *IFile
  )
/*++

Routine Description:

  Close the open file instance.

Arguments:

  IFile                 - Open file instance.

Returns:

  EFI_SUCCESS           - Closed the file successfully.

--*/
{
  FAT_OFILE   *OFile;
  FAT_VOLUME  *Volume;

  OFile   = IFile->OFile;
  Volume  = OFile->Volume;

  ASSERT_VOLUME_LOCKED (Volume);

  //
  // Remove the IFile struct
  //
  RemoveEntryList (&IFile->Link);

  //
  // Add the OFile to the check reference list
  //
  if (OFile->CheckLink.ForwardLink == NULL) {
    InsertHeadList (&Volume->CheckRef, &OFile->CheckLink);
  }
  //
  // Done. Free the open instance structure
  //
  FreePool (IFile);
  return EFI_SUCCESS;
}

EFI_STATUS
FatOFileFlush (
  IN FAT_OFILE    *OFile
  )
/*++

Routine Description:

  Flush the data associated with an open file.
  In this implementation, only last Mod/Access time is updated.

Arguments:

  OFile                 - The open file.

Returns:

  EFI_SUCCESS           - The OFile is flushed successfully.
  Others                - An error occurred when flushing this OFile.

--*/
{
  EFI_STATUS    Status;
  FAT_OFILE     *Parent;
  FAT_DIRENT    *DirEnt;
  FAT_DATE_TIME FatNow;

  //
  // Flush each entry up the tree while dirty
  //
  do {
    //
    // If the file has a permanant error, then don't write any
    // of its data to the device (may be from different media)
    //
    if (EFI_ERROR (OFile->Error)) {
      return OFile->Error;
    }

    Parent  = OFile->Parent;
    DirEnt  = OFile->DirEnt;
    if (OFile->Dirty) {
      //
      // Update the last modification time
      //
      FatGetCurrentFatTime (&FatNow);
      CopyMem (&DirEnt->Entry.FileLastAccess, &FatNow.Date, sizeof (FAT_DATE));
      if (!OFile->PreserveLastModification) {
        FatGetCurrentFatTime (&DirEnt->Entry.FileModificationTime);
      }

      OFile->PreserveLastModification = FALSE;
      if (OFile->Archive) {
        DirEnt->Entry.Attributes |= FAT_ATTRIBUTE_ARCHIVE;
        OFile->Archive = FALSE;
      }
      //
      // Write the directory entry
      //
      if (Parent != NULL && !DirEnt->Invalid) {
        //
        // Write the OFile's directory entry
        //
        Status = FatStoreDirEnt (Parent, DirEnt);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      OFile->Dirty = FALSE;
    }
    //
    // Check the parent
    //
    OFile = Parent;
  } while (OFile != NULL);
  return EFI_SUCCESS;
}

BOOLEAN
FatCheckOFileRef (
  IN FAT_OFILE   *OFile
  )
/*++

Routine Description:

  Check the references of the OFile.
  If the OFile (that is checked) is no longer
  referenced, then it is freed.

Arguments:

  OFile                 - The OFile to be checked.

Returns:

  TRUE                  - The OFile is not referenced and freed.
  FALSE                 - The OFile is kept.

--*/
{
  //
  // If the OFile is on the check ref list, remove it
  //
  if (OFile->CheckLink.ForwardLink != NULL) {
    RemoveEntryList (&OFile->CheckLink);
    OFile->CheckLink.ForwardLink = NULL;
  }

  FatOFileFlush (OFile);
  //
  // Are there any references to this OFile?
  //
  if (!IsListEmpty (&OFile->Opens) || !IsListEmpty (&OFile->ChildHead)) {
    //
    // The OFile cannot be freed
    //
    return FALSE;
  }
  //
  // Free the Ofile
  //
  FatCloseDirEnt (OFile->DirEnt);
  return TRUE;
}

STATIC
VOID
FatCheckVolumeRef (
  IN FAT_VOLUME   *Volume
  )
/*++

Routine Description:

  Check the references of all open files on the volume.
  Any open file (that is checked) that is no longer
  referenced, is freed - and it's parent open file
  is then referenced checked.

Arguments:

  Volume                - The volume to check the pending open file list.

Returns:

  None

--*/
{
  FAT_OFILE *OFile;
  FAT_OFILE *Parent;

  //
  // Check all files on the pending check list
  //
  while (!IsListEmpty (&Volume->CheckRef)) {
    //
    // Start with the first file listed
    //
    Parent = OFILE_FROM_CHECKLINK (Volume->CheckRef.ForwardLink);
    //
    // Go up the tree cleaning up any un-referenced OFiles
    //
    while (Parent != NULL) {
      OFile   = Parent;
      Parent  = OFile->Parent;
      if (!FatCheckOFileRef (OFile)) {
        break;
      }
    }
  }
}

EFI_STATUS
FatCleanupVolume (
  IN FAT_VOLUME       *Volume,
  IN FAT_OFILE        *OFile,
  IN EFI_STATUS       EfiStatus
  )
/*++

Routine Description:

  Set error status for a specific OFile, reference checking the volume.
  If volume is already marked as invalid, and all resources are freed
  after reference checking, the file system protocol is uninstalled and
  the volume structure is freed.

Arguments:

  Volume                - the Volume that is to be reference checked and unlocked.
  OFile                 - the OFile whose permanent error code is to be set.
  EfiStatus             - error code to be set.

Returns:

  EFI_SUCCESS           - Clean up the volume successfully.
  Others                - Cleaning up of the volume is failed.

--*/
{
  EFI_STATUS  Status;
  //
  // Flag the OFile
  //
  if (OFile != NULL) {
    FatSetVolumeError (OFile, EfiStatus);
  }
  //
  // Clean up any dangling OFiles that don't have IFiles
  // we don't check return status here because we want the
  // volume be cleaned up even the volume is invalid.
  //
  FatCheckVolumeRef (Volume);
  if (Volume->Valid) {
    //
    // Update the free hint info. Volume->FreeInfoPos != 0
    // indicates this a FAT32 volume
    //
    if (Volume->FreeInfoValid && Volume->FatDirty && Volume->FreeInfoPos) {
      Status = FatDiskIo (Volume, WRITE_DISK, Volume->FreeInfoPos, sizeof (FAT_INFO_SECTOR), &Volume->FatInfoSector);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
    //
    // Update that the volume is not dirty
    //
    if (Volume->FatDirty && Volume->FatType != FAT12) {
      Volume->FatDirty  = FALSE;
      Status            = FatAccessVolumeDirty (Volume, WRITE_FAT, &Volume->NotDirtyValue);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
    //
    // Flush all dirty cache entries to disk
    //
    Status = FatVolumeFlushCache (Volume);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // If the volume is cleared , remove it.
  // The only time volume be invalidated is in DriverBindingStop.
  //
  if (Volume->Root == NULL && !Volume->Valid) {
    //
    // Free the volume structure
    //
    FatFreeVolume (Volume);
  }

  return EfiStatus;
}

VOID
FatSetVolumeError (
  IN FAT_OFILE            *OFile,
  IN EFI_STATUS           Status
  )
/*++

Routine Description:

  Set the OFile and its child OFile with the error Status

Arguments:

  OFile                 - The OFile whose permanent error code is to be set.
  Status                - Error code to be set.

Returns:

  None

--*/
{
  LIST_ENTRY      *Link;
  FAT_OFILE       *ChildOFile;

  //
  // If this OFile doesn't already have an error, set one
  //
  if (!EFI_ERROR (OFile->Error)) {
    OFile->Error = Status;
  }
  //
  // Set the error on each child OFile
  //
  for (Link = OFile->ChildHead.ForwardLink; Link != &OFile->ChildHead; Link = Link->ForwardLink) {
    ChildOFile = OFILE_FROM_CHILDLINK (Link);
    FatSetVolumeError (ChildOFile, Status);
  }
}

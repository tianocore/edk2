/** @file
  OpenVolume() function of Simple File System Protocol.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Fat.h"

/**

  Implements Simple File System Protocol interface function OpenVolume().

  @param  This                  - Calling context.
  @param  File                  - the Root Directory of the volume.

  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
  @retval EFI_VOLUME_CORRUPTED  - The FAT type is error.
  @retval EFI_SUCCESS           - Open the volume successfully.

**/
EFI_STATUS
EFIAPI
FatOpenVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL                **File
  )
{
  EFI_STATUS  Status;
  FAT_VOLUME  *Volume;
  FAT_IFILE   *IFile;

  Volume = VOLUME_FROM_VOL_INTERFACE (This);
  FatAcquireLock ();

  //
  // Open Root file
  //
  Status = FatOpenDirEnt (NULL, &Volume->RootDirEnt);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Open a new instance to the root
  //
  Status = FatAllocateIFile (Volume->Root, &IFile);
  if (!EFI_ERROR (Status)) {
    *File = &IFile->Handle;
  }

Done:

  Status = FatCleanupVolume (Volume, Volume->Root, Status, NULL);
  FatReleaseLock ();

  return Status;
}

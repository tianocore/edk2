/** @file
*
*  Copyright (c) 2014, ARM Ltd. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution. The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
*  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <Guid/BootMonFsFileInfo.h>
#include <Protocol/SimpleFileSystem.h> // EFI_FILE_HANDLE

#include "ArmShellCmdRunAxf.h"
#include "BootMonFsLoader.h"

/**
  Check that loading the file is supported.

  Not all information is checked, only the properties that matters to us in
  our simplified loader.

  BootMonFS file properties is not in a file header but in the file-system
  metadata, so we need to pass a handle to the file to allow access to the
  information.

  @param[in] FileHandle  Handle of the file to check.

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER if the header is invalid.
  @retval EFI_UNSUPPORTED if the file type/platform is not supported.
**/
EFI_STATUS
BootMonFsCheckFile (
  IN  CONST EFI_FILE_HANDLE  FileHandle
  )
{
  EFI_STATUS           Status;
  BOOTMON_FS_FILE_INFO Info;
  UINTN                InfoSize;
  UINTN                Index;

  ASSERT (FileHandle != NULL);

  // Try to load the file information as BootMonFS executable.
  InfoSize = sizeof (Info);
  // Get BootMon File info and see if it gives us what we need to load the file.
  Status = FileHandle->GetInfo (FileHandle, &gArmBootMonFsFileInfoGuid,
                               &InfoSize, &Info);

  if (!EFI_ERROR (Status)) {
    // Check the values return to see if they look reasonable.
    // Do we have a good entrypoint and at least one good load region?
    // We assume here that we cannot load to address 0x0.
    if ((Info.Size == 0) || (Info.EntryPoint == 0) || (Info.RegionCount == 0) ||
        (Info.RegionCount > BOOTMONFS_IMAGE_DESCRIPTION_REGION_MAX)) {
      // The file does not seem to be of the right type.
      Status = EFI_UNSUPPORTED;
    } else {
      // Check load regions. We just check for valid numbers, we dont do the
      // checksums. Info.Offset can be zero if it loads from the start of the
      // file.
      for (Index = 0; Index < Info.RegionCount; Index++) {
        if ((Info.Region[Index].LoadAddress == 0) || (Info.Region[Index].Size == 0)) {
          Status = EFI_UNSUPPORTED;
          break;
        }
      }
    }
  }

  return Status;
}

/**
  Load a binary file from BootMonFS.

  @param[in]  FileHandle    Handle of the file to load.

  @param[in]  FileData      Address  of the file data in memory.

  @param[out] EntryPoint    Will be filled with the ELF entry point address.

  @param[out] ImageSize     Will be filled with the file size in memory. This
                            will effectively be equal to the sum of the load
                            region sizes.

  This function assumes the file is valid and supported as checked with
  BootMonFsCheckFile().

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER if the file is invalid.
**/
EFI_STATUS
BootMonFsLoadFile (
  IN  CONST EFI_FILE_HANDLE   FileHandle,
  IN  CONST VOID             *FileData,
  OUT VOID                  **EntryPoint,
  OUT LIST_ENTRY             *LoadList
  )
{
  EFI_STATUS            Status;
  BOOTMON_FS_FILE_INFO  Info;
  UINTN                 InfoSize;
  UINTN                 Index;
  UINTN                 ImageSize;
  RUNAXF_LOAD_LIST     *LoadNode;

  ASSERT (FileHandle != NULL);
  ASSERT (FileData   != NULL);
  ASSERT (EntryPoint != NULL);
  ASSERT (LoadList   != NULL);

  ImageSize = 0;

  InfoSize = sizeof (Info);
  Status = FileHandle->GetInfo (FileHandle, &gArmBootMonFsFileInfoGuid,
                               &InfoSize, &Info);

  if (!EFI_ERROR (Status)) {
    *EntryPoint = (VOID*)((UINTN)Info.EntryPoint);
    // Load all the regions to run-time memory
    for (Index = 0; Index < Info.RegionCount; Index++) {
      LoadNode = AllocateRuntimeZeroPool (sizeof (RUNAXF_LOAD_LIST));
      if (LoadNode == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      LoadNode->MemOffset  = (UINTN)Info.Region[Index].LoadAddress;
      LoadNode->FileOffset = (UINTN)FileData + Info.Region[Index].Offset;
      LoadNode->Length     = (UINTN)Info.Region[Index].Size;
      InsertTailList (LoadList, &LoadNode->Link);

      ImageSize += LoadNode->Length;
    }
  }

  if ((!EFI_ERROR (Status)) && (ImageSize == 0)) {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

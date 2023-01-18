/** @file
  Responsibility of this file is to load the DXE Core from a Firmware Volume.

Copyright (c) 2016 HP Development Company, L.P.
Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeilessStartupInternal.h"
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/MemoryAllocationHob.h>
#include <Guid/PcdDataBaseSignatureGuid.h>
#include <Register/Intel/Cpuid.h>
#include <Library/PrePiLib.h>
#include "X64/PageTables.h"
#include <Library/ReportStatusCodeLib.h>

#define STACK_SIZE  0x20000
extern EFI_GUID  gEfiNonCcFvGuid;

/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore

   @param DxeCoreEntryPoint         The entry point of DxeCore.

**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint
  )
{
  VOID   *BaseOfStack;
  VOID   *TopOfStack;
  UINTN  PageTables;

  //
  // Clear page 0 and mark it as allocated if NULL pointer detection is enabled.
  //
  if (IsNullDetectionEnabled ()) {
    ClearFirst4KPage (GetHobList ());
    BuildMemoryAllocationHob (0, EFI_PAGES_TO_SIZE (1), EfiBootServicesData);
  }

  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *)((UINTN)BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  DEBUG ((DEBUG_INFO, "BaseOfStack=0x%x, TopOfStack=0x%x\n", BaseOfStack, TopOfStack));

  //
  // Create page table and save PageMapLevel4 to CR3
  //
  PageTables = CreateIdentityMappingPageTables (
                 (EFI_PHYSICAL_ADDRESS)(UINTN)BaseOfStack,
                 STACK_SIZE
                 );
  if (PageTables == 0) {
    DEBUG ((DEBUG_ERROR, "Failed to create idnetity mapping page tables.\n"));
    CpuDeadLoop ();
  }

  AsmWriteCr3 (PageTables);

  //
  // Update the contents of BSP stack HOB to reflect the real stack info passed to DxeCore.
  //
  UpdateStackHob ((EFI_PHYSICAL_ADDRESS)(UINTN)BaseOfStack, STACK_SIZE);

  DEBUG ((DEBUG_INFO, "SwitchStack then Jump to DxeCore\n"));
  //
  // Transfer the control to the entry point of DxeCore.
  //
  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeCoreEntryPoint,
    GetHobList (),
    NULL,
    TopOfStack
    );
}

/**
   Searches DxeCore in all firmware Volumes and loads the first
   instance that contains DxeCore.

   @return FileHandle of DxeCore to load DxeCore.

**/
EFI_STATUS
FindDxeCore (
  IN INTN                         FvInstance,
  IN OUT     EFI_PEI_FILE_HANDLE  *FileHandle
  )
{
  EFI_STATUS         Status;
  EFI_PEI_FV_HANDLE  VolumeHandle;

  if (FileHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *FileHandle = NULL;

  //
  // Caller passed in a specific FV to try, so only try that one
  //
  Status = FfsFindNextVolume (FvInstance, &VolumeHandle);
  if (!EFI_ERROR (Status)) {
    Status = FfsFindNextFile (EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, VolumeHandle, FileHandle);
    if (*FileHandle) {
      // Assume the FV that contains multiple compressed FVs.
      // So decompress the compressed FVs
      Status = FfsProcessFvFile (*FileHandle);
      ASSERT_EFI_ERROR (Status);
      Status = FfsAnyFvFindFirstFile (EFI_FV_FILETYPE_DXE_CORE, &VolumeHandle, FileHandle);
    }
  }

  return Status;
}

/**
 * This is a FFS_CHECK_SECTION_HOOK which is defined by caller to check
 * if the section is an EFI_SECTION_FIRMWARE_VOLUME_IMAGE and if it is
 * a NonCc FV.
 *
 * @param Section       The section in which we're checking for the NonCc FV.
 * @return EFI_STATUS   The section is the NonCc FV.
 */
EFI_STATUS
EFIAPI
CheckSectionHookForDxeNonCc (
  IN EFI_COMMON_SECTION_HEADER  *Section
  )
{
  VOID         *Buffer;
  EFI_STATUS   Status;
  EFI_FV_INFO  FvImageInfo;

  if (Section->Type != EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
    return EFI_INVALID_PARAMETER;
  }

  if (IS_SECTION2 (Section)) {
    Buffer = (VOID *)((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER2));
  } else {
    Buffer = (VOID *)((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER));
  }

  ZeroMem (&FvImageInfo, sizeof (FvImageInfo));
  Status = FfsGetVolumeInfo ((EFI_PEI_FV_HANDLE)(UINTN)Buffer, &FvImageInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Cannot get volume info! %r\n", Status));
    return Status;
  }

  return CompareGuid (&FvImageInfo.FvName, &gEfiNonCcFvGuid) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
 * Find the NonCc FV.
 *
 * @param FvInstance    The FvInstance number.
 * @return EFI_STATUS   Successfuly find the NonCc FV.
 */
EFI_STATUS
EFIAPI
FindDxeNonCc (
  IN INTN  FvInstance
  )
{
  EFI_STATUS           Status;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;
  EFI_PEI_FV_HANDLE    FvImageHandle;
  EFI_FV_INFO          FvImageInfo;
  UINT32               FvAlignment;
  VOID                 *FvBuffer;

  FileHandle = NULL;

  //
  // Caller passed in a specific FV to try, so only try that one
  //
  Status = FfsFindNextVolume (FvInstance, &VolumeHandle);
  ASSERT (Status == EFI_SUCCESS);

  Status = FfsFindNextFile (EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, VolumeHandle, &FileHandle);
  ASSERT (FileHandle != NULL);

  //
  // Find FvImage in FvFile
  //
  Status = FfsFindSectionDataWithHook (EFI_SECTION_FIRMWARE_VOLUME_IMAGE, CheckSectionHookForDxeNonCc, FileHandle, (VOID **)&FvImageHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Collect FvImage Info.
  //
  ZeroMem (&FvImageInfo, sizeof (FvImageInfo));
  Status = FfsGetVolumeInfo (FvImageHandle, &FvImageInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // FvAlignment must be more than 8 bytes required by FvHeader structure.
  //
  FvAlignment = 1 << ((FvImageInfo.FvAttributes & EFI_FVB2_ALIGNMENT) >> 16);
  if (FvAlignment < 8) {
    FvAlignment = 8;
  }

  //
  // Check FvImage
  //
  if ((UINTN)FvImageInfo.FvStart % FvAlignment != 0) {
    FvBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES ((UINT32)FvImageInfo.FvSize), FvAlignment);
    if (FvBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (FvBuffer, FvImageInfo.FvStart, (UINTN)FvImageInfo.FvSize);
    //
    // Update FvImageInfo after reload FvImage to new aligned memory
    //
    FfsGetVolumeInfo ((EFI_PEI_FV_HANDLE)FvBuffer, &FvImageInfo);
  }

  //
  // Inform HOB consumer phase, i.e. DXE core, the existence of this FV
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)FvImageInfo.FvStart, FvImageInfo.FvSize);

  //
  // Makes the encapsulated volume show up in DXE phase to skip processing of
  // encapsulated file again.
  //
  BuildFv2Hob (
    (EFI_PHYSICAL_ADDRESS)(UINTN)FvImageInfo.FvStart,
    FvImageInfo.FvSize,
    &FvImageInfo.FvName,
    &(((EFI_FFS_FILE_HEADER *)FileHandle)->Name)
    );

  return Status;
}

/**
   This function finds DXE Core in the firmware volume and transfer the control to
   DXE core.

   @return EFI_SUCCESS              DXE core was successfully loaded.
   @return EFI_OUT_OF_RESOURCES     There are not enough resources to load DXE core.

**/
EFI_STATUS
EFIAPI
DxeLoadCore (
  IN INTN  FvInstance
  )
{
  EFI_STATUS            Status;
  EFI_FV_FILE_INFO      DxeCoreFileInfo;
  EFI_PHYSICAL_ADDRESS  DxeCoreAddress;
  UINT64                DxeCoreSize;
  EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint;
  EFI_PEI_FILE_HANDLE   FileHandle;
  VOID                  *PeCoffImage;

  //
  // Look in all the FVs present and find the DXE Core FileHandle
  //
  Status = FindDxeCore (FvInstance, &FileHandle);

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  if (!TdIsEnabled ()) {
    FindDxeNonCc (FvInstance);
  }

  //
  // Load the DXE Core from a Firmware Volume.
  //
  Status = FfsFindSectionDataWithHook (EFI_SECTION_PE32, NULL, FileHandle, &PeCoffImage);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LoadPeCoffImage (PeCoffImage, &DxeCoreAddress, &DxeCoreSize, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Extract the DxeCore GUID file name.
  //
  Status = FfsGetFileInfo (FileHandle, &DxeCoreFileInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // Add HOB for the DXE Core
  //
  BuildModuleHob (
    &DxeCoreFileInfo.FileName,
    DxeCoreAddress,
    ALIGN_VALUE (DxeCoreSize, EFI_PAGE_SIZE),
    DxeCoreEntryPoint
    );

  DEBUG ((
    DEBUG_INFO | DEBUG_LOAD,
    "Loading DXE CORE at 0x%11p EntryPoint=0x%11p\n",
    (VOID *)(UINTN)DxeCoreAddress,
    FUNCTION_ENTRY_POINT (DxeCoreEntryPoint)
    ));

  // Transfer control to the DXE Core
  // The hand off state is simply a pointer to the HOB list
  //
  HandOffToDxeCore (DxeCoreEntryPoint);

  //
  // If we get here, then the DXE Core returned.  This is an error
  // DxeCore should not return.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();

  return EFI_OUT_OF_RESOURCES;
}

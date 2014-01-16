/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/IoLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SimpleFileSystem.h>

#include "BootMonFsInternal.h"

UINT32
BootMonFsChecksum (
  IN VOID   *Data,
  IN UINT32 Size
  )
{
  UINT32  *Ptr;
  UINT32  Word;
  UINT32  Checksum;

  ASSERT (Size % 4 == 0);

  Checksum = 0;
  Ptr = (UINT32*)Data;

  while (Size > 0) {
    Word = *Ptr++;
    Size -= 4;

    if (Word > ~Checksum) {
      Checksum++;
    }

    Checksum += Word;
  }

  return ~Checksum;
}

EFI_STATUS
BootMonFsComputeFooterChecksum (
  IN OUT HW_IMAGE_DESCRIPTION *Footer
  )
{
  HW_IMAGE_DESCRIPTION *Description;
  UINT32                Index;

  Footer->Attributes = 1;

  Description = AllocateZeroPool (sizeof (HW_IMAGE_DESCRIPTION));
  if (Description == NULL) {
    DEBUG ((DEBUG_ERROR, "BootMonFsComputeFooterChecksum: Unable to allocate memory.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Copy over to temporary shim
  CopyMem (Description, Footer, sizeof (HW_IMAGE_DESCRIPTION));

  // BootMon doesn't checksum the previous checksum
  Description->FooterChecksum = 0;

  // Blank out regions which aren't being used.
  for (Index = Footer->RegionCount; Index < HW_IMAGE_DESCRIPTION_REGION_MAX; Index++) {
    Description->Region[Index].Checksum = 0;
    Description->Region[Index].LoadAddress = 0;
    Description->Region[Index].Offset = 0;
    Description->Region[Index].Size = 0;
  }

  // Compute the checksum
  Footer->FooterChecksum = BootMonFsChecksum (Description, sizeof (HW_IMAGE_DESCRIPTION));

  FreePool (Description);

  return EFI_SUCCESS;
}

BOOLEAN
BootMonFsImageInThisBlock (
  IN  VOID                  *Buf,
  IN  UINTN                  Size,
  IN  UINT32                 Block,
  OUT HW_IMAGE_DESCRIPTION  *Image
  )
{
  EFI_STATUS            Status;
  HW_IMAGE_FOOTER      *Ptr;
  HW_IMAGE_DESCRIPTION *Footer;
  UINT32                Checksum;

  // The footer is stored as the last thing in the block
  Ptr = (HW_IMAGE_FOOTER *)((UINT8 *)Buf + Size - sizeof (HW_IMAGE_FOOTER));

  // Check that the verification bytes are present
  if ((Ptr->FooterSignature1 != HW_IMAGE_FOOTER_SIGNATURE_1) || (Ptr->FooterSignature2 != HW_IMAGE_FOOTER_SIGNATURE_2)) {
    return FALSE;
  }

  if (Ptr->Version != HW_IMAGE_FOOTER_VERSION) {
    return FALSE;
  }

  if (Ptr->Offset != HW_IMAGE_FOOTER_OFFSET) {
    return FALSE;
  }

  Footer = (HW_IMAGE_DESCRIPTION *)(((UINT8 *)Buf + Size - sizeof (HW_IMAGE_DESCRIPTION)));
  Checksum = Footer->FooterChecksum;
  Status = BootMonFsComputeFooterChecksum (Footer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Warning: failed to compute checksum for image '%a'\n", Footer->Footer.Filename));
  }

  if (Footer->FooterChecksum != Checksum) {
    DEBUG ((DEBUG_ERROR, "Warning: image '%a' checksum mismatch.\n", Footer->Footer.Filename));
  }

  if ((Footer->BlockEnd != Block) || (Footer->BlockStart > Footer->BlockEnd)) {
    return FALSE;
  }

  // Copy the image out
  CopyMem (Image, Footer, sizeof (HW_IMAGE_DESCRIPTION));

  return TRUE;
}

EFI_STATUS
BootMonFsDiscoverNextImage (
  IN BOOTMON_FS_INSTANCE      *Instance,
  IN EFI_LBA                  *LbaStart,
  OUT HW_IMAGE_DESCRIPTION    *Image
  )
{
  EFI_BLOCK_IO_PROTOCOL *Blocks;
  EFI_LBA                CurrentLba;
  VOID                  *Out;

  Blocks = Instance->BlockIo;

  // Allocate an output buffer
  Out = AllocatePool (Instance->Media->BlockSize);
  if (Out == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Blocks->Reset (Blocks, FALSE);
  CurrentLba = *LbaStart;

  // Look for images in the rest of this block
  while (CurrentLba <= Instance->Media->LastBlock) {
    // Read in the next block
    Blocks->ReadBlocks (Blocks, Instance->Media->MediaId, CurrentLba, Instance->Media->BlockSize, Out);
    // Check for an image in the current block
    if (BootMonFsImageInThisBlock (Out, Instance->Media->BlockSize, (CurrentLba - Instance->Media->LowestAlignedLba), Image)) {
      DEBUG ((EFI_D_ERROR, "Found image: %a in block %d.\n", &(Image->Footer.Filename), (UINTN)(CurrentLba - Instance->Media->LowestAlignedLba)));
      FreePool (Out);
      *LbaStart = Image->BlockEnd + 1;
      return EFI_SUCCESS;
    } else {
      CurrentLba++;
    }
  }

  *LbaStart = CurrentLba;
  FreePool (Out);
  return EFI_NOT_FOUND;
}

EFI_STATUS
BootMonFsInitialize (
  IN BOOTMON_FS_INSTANCE *Instance
  )
{
  EFI_STATUS               Status;
  EFI_LBA                  Lba;
  UINT32                   ImageCount;
  BOOTMON_FS_FILE          *NewFile;

  ImageCount = 0;
  Lba = 0;

  while (1) {
    Status = BootMonFsCreateFile (Instance, &NewFile);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = BootMonFsDiscoverNextImage (Instance, &Lba, &(NewFile->HwDescription));
    if (EFI_ERROR (Status)) {
      // Free NewFile allocated by BootMonFsCreateFile ()
      FreePool (NewFile);
      break;
    }
    InsertTailList (&Instance->RootFile->Link, &NewFile->Link);
    ImageCount++;
  }

  Instance->Initialized = TRUE;
  return EFI_SUCCESS;
}

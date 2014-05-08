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
BootMonFsIsImageValid (
  IN HW_IMAGE_DESCRIPTION  *Desc,
  IN EFI_LBA                Lba
  )
{
  EFI_STATUS            Status;
  HW_IMAGE_FOOTER      *Footer;
  UINT32                Checksum;

  Footer = &Desc->Footer;

  // Check that the verification bytes are present
  if ((Footer->FooterSignature1 != HW_IMAGE_FOOTER_SIGNATURE_1) ||
      (Footer->FooterSignature2 != HW_IMAGE_FOOTER_SIGNATURE_2)) {
    return FALSE;
  }

  if (Footer->Version == HW_IMAGE_FOOTER_VERSION) {
    if (Footer->Offset != HW_IMAGE_FOOTER_OFFSET) {
      return FALSE;
    }
  } else if (Footer->Version == HW_IMAGE_FOOTER_VERSION2) {
    if (Footer->Offset != HW_IMAGE_FOOTER_OFFSET2) {
      return FALSE;
    }
  } else {
    return FALSE;
  }

  Checksum = Desc->FooterChecksum;
  Status = BootMonFsComputeFooterChecksum (Desc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Warning: failed to compute checksum for image '%a'\n", Desc->Footer.Filename));
  }

  if (Desc->FooterChecksum != Checksum) {
    DEBUG ((DEBUG_ERROR, "Warning: image '%a' checksum mismatch.\n", Desc->Footer.Filename));
  }

  if ((Desc->BlockEnd != Lba) || (Desc->BlockStart > Desc->BlockEnd)) {
    return FALSE;
  }

  return TRUE;
}

STATIC
EFI_STATUS
BootMonFsDiscoverNextImage (
  IN     BOOTMON_FS_INSTANCE      *Instance,
  IN OUT EFI_LBA                  *LbaStart,
  IN OUT BOOTMON_FS_FILE          *File
  )
{
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  EFI_LBA                CurrentLba;
  UINT64                 DescOffset;
  EFI_STATUS             Status;

  DiskIo = Instance->DiskIo;

  CurrentLba = *LbaStart;

  // Look for images in the rest of this block
  while (CurrentLba <= Instance->Media->LastBlock) {
    // Work out the byte offset into media of the image description in this block
    // If present, the image description is at the very end of the block.
    DescOffset = ((CurrentLba + 1) * Instance->Media->BlockSize) - sizeof (HW_IMAGE_DESCRIPTION);

    // Read the image description from media
    Status = DiskIo->ReadDisk (DiskIo,
                       Instance->Media->MediaId,
                       DescOffset,
                       sizeof (HW_IMAGE_DESCRIPTION),
                       &File->HwDescription
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // If we found a valid image description...
    if (BootMonFsIsImageValid (&File->HwDescription, (CurrentLba - Instance->Media->LowestAlignedLba))) {
      DEBUG ((EFI_D_ERROR, "Found image: %a in block %d.\n",
        &(File->HwDescription.Footer.Filename),
        (UINTN)(CurrentLba - Instance->Media->LowestAlignedLba)
        ));
      File->HwDescAddress = DescOffset;

      *LbaStart = CurrentLba + 1;
      return EFI_SUCCESS;
    } else {
      CurrentLba++;
    }
  }

  *LbaStart = CurrentLba;
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

    Status = BootMonFsDiscoverNextImage (Instance, &Lba, NewFile);
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

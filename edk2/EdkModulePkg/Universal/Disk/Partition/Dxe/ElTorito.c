/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  ElTorito.c

Abstract:

  Decode an El Torito formatted CD-ROM

Revision History

--*/

#include "Partition.h"

EFI_STATUS
PartitionInstallElToritoChildHandles (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DISK_IO_PROTOCOL         *DiskIo,
  IN  EFI_BLOCK_IO_PROTOCOL        *BlockIo,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
/*++

Routine Description:
  Install child handles if the Handle supports El Torito format.

Arguments:
  This       - Calling context.
  Handle     - Parent Handle
  DiskIo     - Parent DiskIo interface
  BlockIo    - Parent BlockIo interface
  DevicePath - Parent Device Path

Returns:
  EFI_SUCCESS       - some child handle(s) was added
  EFI_MEDIA_CHANGED - Media changed Detected
  !EFI_SUCCESS      - no child handle was added

--*/
{
  EFI_STATUS              Status;
  UINT32                  VolDescriptorLba;
  UINT32                  Lba;
  EFI_BLOCK_IO_MEDIA      *Media;
  CDROM_VOLUME_DESCRIPTOR *VolDescriptor;
  ELTORITO_CATALOG        *Catalog;
  UINTN                   Check;
  UINTN                   Index;
  UINTN                   BootEntry;
  UINTN                   MaxIndex;
  UINT16                  *CheckBuffer;
  CDROM_DEVICE_PATH       CdDev;
  UINT32                  SubBlockSize;
  UINT32                  SectorCount;
  EFI_STATUS              Found;
  UINT32                  VolSpaceSize;

  Found         = EFI_NOT_FOUND;
  Media         = BlockIo->Media;
  VolSpaceSize  = 0;

  //
  // CD_ROM has the fixed block size as 2048 bytes
  //
  if (Media->BlockSize != 2048) {
    return EFI_NOT_FOUND;
  }

  VolDescriptor = AllocatePool ((UINTN) Media->BlockSize);

  if (VolDescriptor == NULL) {
    return EFI_NOT_FOUND;
  }

  Catalog = (ELTORITO_CATALOG *) VolDescriptor;

  //
  // the ISO-9660 volume descriptor starts at 32k on the media
  // and CD_ROM has the fixed block size as 2048 bytes, so...
  //
  //
  // ((16*2048) / Media->BlockSize) - 1;
  //
  VolDescriptorLba = 15;
  //
  // Loop: handle one volume descriptor per time
  //
  while (TRUE) {

    VolDescriptorLba += 1;
    if (VolDescriptorLba > Media->LastBlock) {
      //
      // We are pointing past the end of the device so exit
      //
      break;
    }

    Status = BlockIo->ReadBlocks (
                        BlockIo,
                        Media->MediaId,
                        VolDescriptorLba,
                        Media->BlockSize,
                        VolDescriptor
                        );
    if (EFI_ERROR (Status)) {
      Found = Status;
      break;
    }
    //
    // Check for valid volume descriptor signature
    //
    if (VolDescriptor->Type == CDVOL_TYPE_END ||
        CompareMem (VolDescriptor->Id, CDVOL_ID, sizeof (VolDescriptor->Id)) != 0
        ) {
      //
      // end of Volume descriptor list
      //
      break;
    }
    //
    // Read the Volume Space Size from Primary Volume Descriptor 81-88 byte,
    // the 32-bit numerical values is stored in Both-byte orders
    //
    if (VolDescriptor->Type == CDVOL_TYPE_CODED) {
      VolSpaceSize = VolDescriptor->VolSpaceSize[0];
    }
    //
    // Is it an El Torito volume descriptor?
    //
    if (CompareMem (VolDescriptor->SystemId, CDVOL_ELTORITO_ID, sizeof (CDVOL_ELTORITO_ID) - 1) != 0) {
      continue;
    }
    //
    // Read in the boot El Torito boot catalog
    //
    Lba = UNPACK_INT32 (VolDescriptor->EltCatalog);
    if (Lba > Media->LastBlock) {
      continue;
    }

    Status = BlockIo->ReadBlocks (
                        BlockIo,
                        Media->MediaId,
                        Lba,
                        Media->BlockSize,
                        Catalog
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "EltCheckDevice: error reading catalog %r\n", Status));
      continue;
    }
    //
    // We don't care too much about the Catalog header's contents, but we do want
    // to make sure it looks like a Catalog header
    //
    if (Catalog->Catalog.Indicator != ELTORITO_ID_CATALOG || Catalog->Catalog.Id55AA != 0xAA55) {
      DEBUG ((EFI_D_ERROR, "EltCheckBootCatalog: El Torito boot catalog header IDs not correct\n"));
      continue;
    }

    Check       = 0;
    CheckBuffer = (UINT16 *) Catalog;
    for (Index = 0; Index < sizeof (ELTORITO_CATALOG) / sizeof (UINT16); Index += 1) {
      Check += CheckBuffer[Index];
    }

    if (Check & 0xFFFF) {
      DEBUG ((EFI_D_ERROR, "EltCheckBootCatalog: El Torito boot catalog header checksum failed\n"));
      continue;
    }

    MaxIndex = Media->BlockSize / sizeof (ELTORITO_CATALOG);
    for (Index = 1, BootEntry = 1; Index < MaxIndex; Index += 1) {
      //
      // Next entry
      //
      Catalog += 1;

      //
      // Check this entry
      //
      if (Catalog->Boot.Indicator != ELTORITO_ID_SECTION_BOOTABLE || Catalog->Boot.Lba == 0) {
        continue;
      }

      SubBlockSize  = 512;
      SectorCount   = Catalog->Boot.SectorCount;

      switch (Catalog->Boot.MediaType) {

      case ELTORITO_NO_EMULATION:
        SubBlockSize = Media->BlockSize;
        break;

      case ELTORITO_HARD_DISK:
        break;

      case ELTORITO_12_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x0F;
        break;

      case ELTORITO_14_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x12;
        break;

      case ELTORITO_28_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x24;
        break;

      default:
        DEBUG ((EFI_D_INIT, "EltCheckDevice: unsupported El Torito boot media type %x\n", Catalog->Boot.MediaType));
        SectorCount   = 0;
        SubBlockSize  = Media->BlockSize;
        break;
      }
      //
      // Create child device handle
      //
      CdDev.Header.Type     = MEDIA_DEVICE_PATH;
      CdDev.Header.SubType  = MEDIA_CDROM_DP;
      SetDevicePathNodeLength (&CdDev.Header, sizeof (CdDev));

      if (Index == 1) {
        //
        // This is the initial/default entry
        //
        BootEntry = 0;
      }

      CdDev.BootEntry = (UINT32) BootEntry;
      BootEntry++;
      CdDev.PartitionStart = Catalog->Boot.Lba;
      if (SectorCount < 2) {
        //
        // When the SectorCount < 2, set the Partition as the whole CD.
        //
        if (VolSpaceSize > (Media->LastBlock + 1)) {
          CdDev.PartitionSize = (UINT32)(Media->LastBlock - Catalog->Boot.Lba + 1);
        } else {
          CdDev.PartitionSize = (UINT32)(VolSpaceSize - Catalog->Boot.Lba);
        }
      } else {
        CdDev.PartitionSize = DivU64x32 (
                                MultU64x32 (
                                  SectorCount,
                                  SubBlockSize
                                  ) + Media->BlockSize - 1,
                                Media->BlockSize
                                );
      }

      Status = PartitionInstallChildHandle (
                This,
                Handle,
                DiskIo,
                BlockIo,
                DevicePath,
                (EFI_DEVICE_PATH_PROTOCOL *) &CdDev,
                Catalog->Boot.Lba,
                Catalog->Boot.Lba + CdDev.PartitionSize - 1,
                SubBlockSize,
                FALSE
                );
      if (!EFI_ERROR (Status)) {
        Found = EFI_SUCCESS;
      }
    }
  }

  FreePool (VolDescriptor);

  return Found;
}

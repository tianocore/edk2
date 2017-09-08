/** @file
  Scan for an UDF file system on a formatted media.

  Copyright (C) 2014-2017 Paulo Alcantara <pcacjr@zytor.com>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "Partition.h"

//
// C5BD4D42-1A76-4996-8956-73CDA326CD0A
//
#define EFI_UDF_DEVICE_PATH_GUID                        \
  { 0xC5BD4D42, 0x1A76, 0x4996,                         \
    { 0x89, 0x56, 0x73, 0xCD, 0xA3, 0x26, 0xCD, 0x0A }  \
  }

typedef struct {
  VENDOR_DEVICE_PATH        DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  End;
} UDF_DEVICE_PATH;

//
// Vendor-Defined Media Device Path for UDF file system
//
UDF_DEVICE_PATH gUdfDevicePath = {
  { { MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH), 0 } },
    EFI_UDF_DEVICE_PATH_GUID
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

EFI_STATUS
FindAnchorVolumeDescriptorPointer (
  IN   EFI_BLOCK_IO_PROTOCOL                 *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL                  *DiskIo,
  OUT  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoint
  )
{
  EFI_STATUS  Status;
  UINT32      BlockSize = BlockIo->Media->BlockSize;
  EFI_LBA     EndLBA = BlockIo->Media->LastBlock;
  EFI_LBA     DescriptorLBAs[] = { 256, EndLBA - 256, EndLBA, 512 };
  UINTN       Index;

  for (Index = 0; Index < ARRAY_SIZE (DescriptorLBAs); Index++) {
    Status = DiskIo->ReadDisk (
      DiskIo,
      BlockIo->Media->MediaId,
      MultU64x32 (DescriptorLBAs[Index], BlockSize),
      sizeof (UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER),
      (VOID *)AnchorPoint
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Check if read LBA has a valid AVDP descriptor.
    //
    if (IS_AVDP (AnchorPoint)) {
      return EFI_SUCCESS;
    }
  }
  //
  // No AVDP found.
  //
  return EFI_VOLUME_CORRUPTED;
}

/**
  Check if block device supports a valid UDF file system as specified by OSTA
  Universal Disk Format Specification 2.60.

  @param[in]   BlockIo  BlockIo interface.
  @param[in]   DiskIo   DiskIo interface.

  @retval EFI_SUCCESS          UDF file system found.
  @retval EFI_UNSUPPORTED      UDF file system not found.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The scan was not successful due to lack of
                               resources.

**/
EFI_STATUS
SupportUdfFileSystem (
  IN EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN EFI_DISK_IO_PROTOCOL   *DiskIo
  )
{
  EFI_STATUS                            Status;
  UINT64                                Offset;
  UINT64                                EndDiskOffset;
  CDROM_VOLUME_DESCRIPTOR               VolDescriptor;
  CDROM_VOLUME_DESCRIPTOR               TerminatingVolDescriptor;
  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  AnchorPoint;

  ZeroMem ((VOID *)&TerminatingVolDescriptor, sizeof (CDROM_VOLUME_DESCRIPTOR));

  //
  // Start Volume Recognition Sequence
  //
  EndDiskOffset = MultU64x32 (BlockIo->Media->LastBlock,
                              BlockIo->Media->BlockSize);

  for (Offset = UDF_VRS_START_OFFSET; Offset < EndDiskOffset;
       Offset += UDF_LOGICAL_SECTOR_SIZE) {
    //
    // Check if block device has a Volume Structure Descriptor and an Extended
    // Area.
    //
    Status = DiskIo->ReadDisk (
      DiskIo,
      BlockIo->Media->MediaId,
      Offset,
      sizeof (CDROM_VOLUME_DESCRIPTOR),
      (VOID *)&VolDescriptor
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                    (VOID *)UDF_BEA_IDENTIFIER,
                    sizeof (VolDescriptor.Unknown.Id)) == 0) {
      break;
    }

    if ((CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                     (VOID *)CDVOL_ID,
                     sizeof (VolDescriptor.Unknown.Id)) != 0) ||
        (CompareMem ((VOID *)&VolDescriptor,
                     (VOID *)&TerminatingVolDescriptor,
                     sizeof (CDROM_VOLUME_DESCRIPTOR)) == 0)) {
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Look for "NSR0{2,3}" identifiers in the Extended Area.
  //
  Offset += UDF_LOGICAL_SECTOR_SIZE;
  if (Offset >= EndDiskOffset) {
    return EFI_UNSUPPORTED;
  }

  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    Offset,
    sizeof (CDROM_VOLUME_DESCRIPTOR),
    (VOID *)&VolDescriptor
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                   (VOID *)UDF_NSR2_IDENTIFIER,
                   sizeof (VolDescriptor.Unknown.Id)) != 0) &&
      (CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                   (VOID *)UDF_NSR3_IDENTIFIER,
                   sizeof (VolDescriptor.Unknown.Id)) != 0)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Look for "TEA01" identifier in the Extended Area
  //
  Offset += UDF_LOGICAL_SECTOR_SIZE;
  if (Offset >= EndDiskOffset) {
    return EFI_UNSUPPORTED;
  }

  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    Offset,
    sizeof (CDROM_VOLUME_DESCRIPTOR),
    (VOID *)&VolDescriptor
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (CompareMem ((VOID *)VolDescriptor.Unknown.Id,
                  (VOID *)UDF_TEA_IDENTIFIER,
                  sizeof (VolDescriptor.Unknown.Id)) != 0) {
    return EFI_UNSUPPORTED;
  }

  Status = FindAnchorVolumeDescriptorPointer (BlockIo, DiskIo, &AnchorPoint);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Install child handles if the Handle supports UDF/ECMA-167 volume format.

  @param[in]  This        Calling context.
  @param[in]  Handle      Parent Handle.
  @param[in]  DiskIo      Parent DiskIo interface.
  @param[in]  DiskIo2     Parent DiskIo2 interface.
  @param[in]  BlockIo     Parent BlockIo interface.
  @param[in]  BlockIo2    Parent BlockIo2 interface.
  @param[in]  DevicePath  Parent Device Path


  @retval EFI_SUCCESS         Child handle(s) was added.
  @retval EFI_MEDIA_CHANGED   Media changed Detected.
  @retval other               no child handle was added.

**/
EFI_STATUS
PartitionInstallUdfChildHandles (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DISK_IO_PROTOCOL         *DiskIo,
  IN  EFI_DISK_IO2_PROTOCOL        *DiskIo2,
  IN  EFI_BLOCK_IO_PROTOCOL        *BlockIo,
  IN  EFI_BLOCK_IO2_PROTOCOL       *BlockIo2,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_BLOCK_IO_MEDIA           *Media;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePathNode;
  EFI_GUID                     *VendorDefinedGuid;
  EFI_GUID                     UdfDevPathGuid = EFI_UDF_DEVICE_PATH_GUID;
  EFI_PARTITION_INFO_PROTOCOL  PartitionInfo;

  Media = BlockIo->Media;

  //
  // Check if UDF logical block size is multiple of underlying device block size
  //
  if ((UDF_LOGICAL_SECTOR_SIZE % Media->BlockSize) != 0 ||
      Media->BlockSize > UDF_LOGICAL_SECTOR_SIZE) {
    return EFI_NOT_FOUND;
  }

  DevicePathNode = DevicePath;
  while (!IsDevicePathEnd (DevicePathNode)) {
    //
    // Do not allow checking for UDF file systems in CDROM "El Torito"
    // partitions, and skip duplicate installation of UDF file system child
    // nodes.
    //
    if (DevicePathType (DevicePathNode) == MEDIA_DEVICE_PATH) {
      if (DevicePathSubType (DevicePathNode) == MEDIA_CDROM_DP) {
        return EFI_NOT_FOUND;
      }
      if (DevicePathSubType (DevicePathNode) == MEDIA_VENDOR_DP) {
        VendorDefinedGuid = (EFI_GUID *)((UINTN)DevicePathNode +
                                         OFFSET_OF (VENDOR_DEVICE_PATH, Guid));
        if (CompareGuid (VendorDefinedGuid, &UdfDevPathGuid)) {
          return EFI_NOT_FOUND;
        }
      }
    }
    //
    // Try next device path node
    //
    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }

  //
  // Check if block device supports an UDF file system
  //
  Status = SupportUdfFileSystem (BlockIo, DiskIo);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Create Partition Info protocol for UDF file system
  //
  ZeroMem (&PartitionInfo, sizeof (EFI_PARTITION_INFO_PROTOCOL));
  PartitionInfo.Revision = EFI_PARTITION_INFO_PROTOCOL_REVISION;
  PartitionInfo.Type = PARTITION_TYPE_OTHER;

  //
  // Install partition child handle for UDF file system
  //
  Status = PartitionInstallChildHandle (
    This,
    Handle,
    DiskIo,
    DiskIo2,
    BlockIo,
    BlockIo2,
    DevicePath,
    (EFI_DEVICE_PATH_PROTOCOL *)&gUdfDevicePath,
    &PartitionInfo,
    0,
    Media->LastBlock,
    Media->BlockSize
    );
  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/** @file
  FAT file system access routines for FAT recovery PEIM

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FatLitePeim.h"

/**
  Check if there is a valid FAT in the corresponding Block device
  of the volume and if yes, fill in the relevant fields for the
  volume structure. Note there should be a valid Block device number
  already set.

  @param  PrivateData            Global memory map for accessing global
                                 variables.
  @param  Volume                 On input, the BlockDeviceNumber field of the
                                 Volume  should be a valid value. On successful
                                 output, all  fields except the VolumeNumber
                                 field is initialized.

  @retval EFI_SUCCESS            A FAT is found and the volume structure is
                                 initialized.
  @retval EFI_NOT_FOUND          There is no FAT on the corresponding device.
  @retval EFI_DEVICE_ERROR       There is something error while accessing device.

**/
EFI_STATUS
FatGetBpbInfo (
  IN      PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN OUT  PEI_FAT_VOLUME        *Volume
  )
{
  EFI_STATUS              Status;
  PEI_FAT_BOOT_SECTOR     Bpb;
  PEI_FAT_BOOT_SECTOR_EX  BpbEx;
  UINT32                  Sectors;
  UINT32                  SectorsPerFat;
  UINT32                  RootDirSectors;
  UINT64                  FatLba;
  UINT64                  RootLba;
  UINT64                  FirstClusterLba;

  //
  // Read in the BPB
  //
  Status = FatReadDisk (
             PrivateData,
             Volume->BlockDeviceNo,
             0,
             sizeof (PEI_FAT_BOOT_SECTOR_EX),
             &BpbEx
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (
    (UINT8 *)(&Bpb),
    (UINT8 *)(&BpbEx),
    sizeof (PEI_FAT_BOOT_SECTOR)
    );

  Volume->FatType = FatUnknown;

  Sectors = Bpb.Sectors;
  if (Sectors == 0) {
    Sectors = Bpb.LargeSectors;
  }

  SectorsPerFat = Bpb.SectorsPerFat;
  if (SectorsPerFat == 0) {
    SectorsPerFat   = BpbEx.LargeSectorsPerFat;
    Volume->FatType = Fat32;
  }

  //
  // Filter out those not a FAT
  //
  if ((Bpb.Ia32Jump[0] != 0xe9) && (Bpb.Ia32Jump[0] != 0xeb) && (Bpb.Ia32Jump[0] != 0x49)) {
    return EFI_NOT_FOUND;
  }

  if ((Bpb.ReservedSectors == 0) || (Bpb.NoFats == 0) || (Sectors == 0)) {
    return EFI_NOT_FOUND;
  }

  if ((Bpb.SectorsPerCluster != 1) &&
      (Bpb.SectorsPerCluster != 2) &&
      (Bpb.SectorsPerCluster != 4) &&
      (Bpb.SectorsPerCluster != 8) &&
      (Bpb.SectorsPerCluster != 16) &&
      (Bpb.SectorsPerCluster != 32) &&
      (Bpb.SectorsPerCluster != 64) &&
      (Bpb.SectorsPerCluster != 128)
      )
  {
    return EFI_NOT_FOUND;
  }

  if ((Volume->FatType == Fat32) && ((SectorsPerFat == 0) || (BpbEx.FsVersion != 0))) {
    return EFI_NOT_FOUND;
  }

  if ((Bpb.Media != 0xf0) &&
      (Bpb.Media != 0xf8) &&
      (Bpb.Media != 0xf9) &&
      (Bpb.Media != 0xfb) &&
      (Bpb.Media != 0xfc) &&
      (Bpb.Media != 0xfd) &&
      (Bpb.Media != 0xfe) &&
      (Bpb.Media != 0xff) &&
      //
      // FujitsuFMR
      //
      (Bpb.Media != 0x00) &&
      (Bpb.Media != 0x01) &&
      (Bpb.Media != 0xfa)
      )
  {
    return EFI_NOT_FOUND;
  }

  if ((Volume->FatType != Fat32) && (Bpb.RootEntries == 0)) {
    return EFI_NOT_FOUND;
  }

  //
  // If this is fat32, refuse to mount mirror-disabled volumes
  //
  if ((Volume->FatType == Fat32) && ((BpbEx.ExtendedFlags & 0x80) != 0)) {
    return EFI_NOT_FOUND;
  }

  //
  // Fill in the volume structure fields
  // (Sectors & SectorsPerFat is computed earlier already)
  //
  Volume->ClusterSize = Bpb.SectorSize * Bpb.SectorsPerCluster;
  Volume->RootEntries = Bpb.RootEntries;
  Volume->SectorSize  = Bpb.SectorSize;

  RootDirSectors = ((Volume->RootEntries * sizeof (FAT_DIRECTORY_ENTRY)) + (Volume->SectorSize - 1)) / Volume->SectorSize;

  FatLba          = Bpb.ReservedSectors;
  RootLba         = Bpb.NoFats * SectorsPerFat + FatLba;
  FirstClusterLba = RootLba + RootDirSectors;

  Volume->VolumeSize      = MultU64x32 (Sectors, Volume->SectorSize);
  Volume->FatPos          = MultU64x32 (FatLba, Volume->SectorSize);
  Volume->RootDirPos      = MultU64x32 (RootLba, Volume->SectorSize);
  Volume->FirstClusterPos = MultU64x32 (FirstClusterLba, Volume->SectorSize);
  Volume->MaxCluster      = (UINT32)(Sectors - FirstClusterLba) / Bpb.SectorsPerCluster;
  Volume->RootDirCluster  = BpbEx.RootDirFirstCluster;

  //
  // If this is not a fat32, determine if it's a fat16 or fat12
  //
  if (Volume->FatType != Fat32) {
    if (Volume->MaxCluster >= 65525) {
      return EFI_NOT_FOUND;
    }

    Volume->FatType = Volume->MaxCluster < 4085 ? Fat12 : Fat16;
  }

  return EFI_SUCCESS;
}

/**
  Gets the next cluster in the cluster chain

  @param  PrivateData            Global memory map for accessing global variables
  @param  Volume                 The volume
  @param  Cluster                The cluster
  @param  NextCluster            The cluster number of the next cluster

  @retval EFI_SUCCESS            The address is got
  @retval EFI_INVALID_PARAMETER  ClusterNo exceeds the MaxCluster of the volume.
  @retval EFI_DEVICE_ERROR       Read disk error

**/
EFI_STATUS
FatGetNextCluster (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_VOLUME        *Volume,
  IN  UINT32                Cluster,
  OUT UINT32                *NextCluster
  )
{
  EFI_STATUS  Status;
  UINT64      FatEntryPos;
  UINT32      Dummy;

  *NextCluster = 0;

  if (Volume->FatType == Fat32) {
    FatEntryPos = Volume->FatPos + MultU64x32 (4, Cluster);

    Status        = FatReadDisk (PrivateData, Volume->BlockDeviceNo, FatEntryPos, 4, NextCluster);
    *NextCluster &= 0x0fffffff;

    //
    // Pad high bits for our FAT_CLUSTER_... macro definitions to work
    //
    if ((*NextCluster) >= 0x0ffffff7) {
      *NextCluster |= (-1 &~0xf);
    }
  } else if (Volume->FatType == Fat16) {
    FatEntryPos = Volume->FatPos + MultU64x32 (2, Cluster);

    Status = FatReadDisk (PrivateData, Volume->BlockDeviceNo, FatEntryPos, 2, NextCluster);

    //
    // Pad high bits for our FAT_CLUSTER_... macro definitions to work
    //
    if ((*NextCluster) >= 0xfff7) {
      *NextCluster |= (-1 &~0xf);
    }
  } else {
    FatEntryPos = Volume->FatPos + DivU64x32Remainder (MultU64x32 (3, Cluster), 2, &Dummy);

    Status = FatReadDisk (PrivateData, Volume->BlockDeviceNo, FatEntryPos, 2, NextCluster);

    if ((Cluster & 0x01) != 0) {
      *NextCluster = (*NextCluster) >> 4;
    } else {
      *NextCluster = (*NextCluster) & 0x0fff;
    }

    //
    // Pad high bits for our FAT_CLUSTER_... macro definitions to work
    //
    if ((*NextCluster) >= 0x0ff7) {
      *NextCluster |= (-1 &~0xf);
    }
  }

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Set a file's CurrentPos and CurrentCluster, then compute StraightReadAmount.

  @param  PrivateData            the global memory map
  @param  File                   the file
  @param  Pos                    the Position which is offset from the file's
                                 CurrentPos

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Pos is beyond file's size.
  @retval EFI_DEVICE_ERROR       Something error while accessing media.

**/
EFI_STATUS
FatSetFilePos (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_FILE          *File,
  IN  UINT32                Pos
  )
{
  EFI_STATUS  Status;
  UINT32      AlignedPos;
  UINT32      Offset;
  UINT32      Cluster;
  UINT32      PrevCluster;

  if (File->IsFixedRootDir) {
    if (Pos >= MultU64x32 (File->Volume->RootEntries, 32) - File->CurrentPos) {
      return EFI_INVALID_PARAMETER;
    }

    File->CurrentPos        += Pos;
    File->StraightReadAmount = (UINT32)(MultU64x32 (File->Volume->RootEntries, 32) - File->CurrentPos);
  } else {
    DivU64x32Remainder (File->CurrentPos, File->Volume->ClusterSize, &Offset);
    AlignedPos = (UINT32)File->CurrentPos - (UINT32)Offset;

    while
    (
     !FAT_CLUSTER_FUNCTIONAL (File->CurrentCluster) &&
     AlignedPos + File->Volume->ClusterSize <= File->CurrentPos + Pos
    )
    {
      AlignedPos += File->Volume->ClusterSize;
      Status      = FatGetNextCluster (
                      PrivateData,
                      File->Volume,
                      File->CurrentCluster,
                      &File->CurrentCluster
                      );
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    if (FAT_CLUSTER_FUNCTIONAL (File->CurrentCluster)) {
      return EFI_INVALID_PARAMETER;
    }

    File->CurrentPos += Pos;
    //
    // Calculate the amount of consecutive cluster occupied by the file.
    // FatReadFile() will use it to read these blocks once.
    //
    File->StraightReadAmount = 0;
    Cluster                  = File->CurrentCluster;
    while (!FAT_CLUSTER_FUNCTIONAL (Cluster)) {
      File->StraightReadAmount += File->Volume->ClusterSize;
      PrevCluster               = Cluster;
      Status                    = FatGetNextCluster (PrivateData, File->Volume, Cluster, &Cluster);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      if (Cluster != PrevCluster + 1) {
        break;
      }
    }

    DivU64x32Remainder (File->CurrentPos, File->Volume->ClusterSize, &Offset);
    File->StraightReadAmount -= (UINT32)Offset;
  }

  return EFI_SUCCESS;
}

/**
  Reads file data. Updates the file's CurrentPos.

  @param  PrivateData            Global memory map for accessing global variables
  @param  File                   The file.
  @param  Size                   The amount of data to read.
  @param  Buffer                 The buffer storing the data.

  @retval EFI_SUCCESS            The data is read.
  @retval EFI_INVALID_PARAMETER  File is invalid.
  @retval EFI_DEVICE_ERROR       Something error while accessing media.

**/
EFI_STATUS
FatReadFile (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_FILE          *File,
  IN  UINTN                 Size,
  OUT VOID                  *Buffer
  )
{
  EFI_STATUS  Status;
  CHAR8       *BufferPtr;
  UINT32      Offset;
  UINT64      PhysicalAddr;
  UINTN       Amount;

  BufferPtr = Buffer;

  if (File->IsFixedRootDir) {
    //
    // This is the fixed root dir in FAT12 and FAT16
    //
    if (File->CurrentPos + Size > File->Volume->RootEntries * sizeof (FAT_DIRECTORY_ENTRY)) {
      return EFI_INVALID_PARAMETER;
    }

    Status = FatReadDisk (
               PrivateData,
               File->Volume->BlockDeviceNo,
               File->Volume->RootDirPos + File->CurrentPos,
               Size,
               Buffer
               );
    File->CurrentPos += (UINT32)Size;
    return Status;
  } else {
    if ((File->Attributes & FAT_ATTR_DIRECTORY) == 0) {
      Size = Size < (File->FileSize - File->CurrentPos) ? Size : (File->FileSize - File->CurrentPos);
    }

    //
    // This is a normal cluster based file
    //
    while (Size != 0) {
      DivU64x32Remainder (File->CurrentPos, File->Volume->ClusterSize, &Offset);
      PhysicalAddr = File->Volume->FirstClusterPos + MultU64x32 (File->Volume->ClusterSize, File->CurrentCluster - 2);

      Amount = File->StraightReadAmount;
      Amount = Size > Amount ? Amount : Size;
      Status = FatReadDisk (
                 PrivateData,
                 File->Volume->BlockDeviceNo,
                 PhysicalAddr + Offset,
                 Amount,
                 BufferPtr
                 );
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      //
      // Advance the file's current pos and current cluster
      //
      FatSetFilePos (PrivateData, File, (UINT32)Amount);

      BufferPtr += Amount;
      Size      -= Amount;
    }

    return EFI_SUCCESS;
  }
}

/**
  This function reads the next item in the parent directory and
  initializes the output parameter SubFile (CurrentPos is initialized to 0).
  The function updates the CurrentPos of the parent dir to after the item read.
  If no more items were found, the function returns EFI_NOT_FOUND.

  @param  PrivateData            Global memory map for accessing global variables
  @param  ParentDir              The parent directory.
  @param  SubFile                The File structure containing the sub file that
                                 is caught.

  @retval EFI_SUCCESS            The next sub file is obtained.
  @retval EFI_INVALID_PARAMETER  The ParentDir is not a directory.
  @retval EFI_NOT_FOUND          No more sub file exists.
  @retval EFI_DEVICE_ERROR       Something error while accessing media.

**/
EFI_STATUS
FatReadNextDirectoryEntry (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_FILE          *ParentDir,
  OUT PEI_FAT_FILE          *SubFile
  )
{
  EFI_STATUS           Status;
  FAT_DIRECTORY_ENTRY  DirEntry;
  CHAR16               *Pos;
  CHAR16               BaseName[9];
  CHAR16               Ext[4];

  ZeroMem ((UINT8 *)SubFile, sizeof (PEI_FAT_FILE));

  //
  // Pick a valid directory entry
  //
  while (1) {
    //
    // Read one entry
    //
    Status = FatReadFile (PrivateData, ParentDir, 32, &DirEntry);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // We only search for *FILE* in root directory
    // Long file name entry is *NOT* supported
    //
    if (((DirEntry.Attributes & FAT_ATTR_DIRECTORY) == FAT_ATTR_DIRECTORY) || (DirEntry.Attributes == FAT_ATTR_LFN)) {
      continue;
    }

    //
    // if this is a terminator dir entry, just return EFI_NOT_FOUND
    //
    if (DirEntry.FileName[0] == EMPTY_ENTRY_MARK) {
      return EFI_NOT_FOUND;
    }

    //
    // If this not an invalid entry neither an empty entry, this is what we want.
    // otherwise we will start a new loop to continue to find something meaningful
    //
    if ((UINT8)DirEntry.FileName[0] != DELETE_ENTRY_MARK) {
      break;
    }
  }

  //
  // fill in the output parameter
  //
  EngFatToStr (8, DirEntry.FileName, BaseName);
  EngFatToStr (3, DirEntry.FileName + 8, Ext);

  Pos = (UINT16 *)SubFile->FileName;
  SetMem ((UINT8 *)Pos, FAT_MAX_FILE_NAME_LENGTH, 0);
  CopyMem ((UINT8 *)Pos, (UINT8 *)BaseName, 2 * (StrLen (BaseName) + 1));

  if (Ext[0] != 0) {
    Pos += StrLen (BaseName);
    *Pos = '.';
    Pos++;
    CopyMem ((UINT8 *)Pos, (UINT8 *)Ext, 2 * (StrLen (Ext) + 1));
  }

  SubFile->Attributes     = DirEntry.Attributes;
  SubFile->CurrentCluster = DirEntry.FileCluster;
  if (ParentDir->Volume->FatType == Fat32) {
    SubFile->CurrentCluster |= DirEntry.FileClusterHigh << 16;
  }

  SubFile->CurrentPos      = 0;
  SubFile->FileSize        = DirEntry.FileSize;
  SubFile->StartingCluster = SubFile->CurrentCluster;
  SubFile->Volume          = ParentDir->Volume;

  //
  // in Pei phase, time parameters do not need to be filled for minimum use.
  //
  return Status;
}

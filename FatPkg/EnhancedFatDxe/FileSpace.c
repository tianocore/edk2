/** @file
  Routines dealing with disk spaces and FAT table entries.

Copyright (c) 2005 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent



**/

#include "Fat.h"

/**

  Get the FAT entry of the volume, which is identified with the Index.

  @param  Volume                - FAT file system volume.
  @param  Index                 - The index of the FAT entry of the volume.

  @return The buffer of the FAT entry

**/
STATIC
VOID *
FatLoadFatEntry (
  IN FAT_VOLUME  *Volume,
  IN UINTN       Index
  )
{
  UINTN       Pos;
  EFI_STATUS  Status;

  if (Index > (Volume->MaxCluster + 1)) {
    Volume->FatEntryBuffer = (UINT32)-1;
    return &Volume->FatEntryBuffer;
  }

  //
  // Compute buffer position needed
  //
  switch (Volume->FatType) {
    case Fat12:
      Pos = FAT_POS_FAT12 (Index);
      break;

    case Fat16:
      Pos = FAT_POS_FAT16 (Index);
      break;

    default:
      Pos = FAT_POS_FAT32 (Index);
  }

  //
  // Set the position and read the buffer
  //
  Volume->FatEntryPos = Volume->FatPos + Pos;
  Status              = FatDiskIo (
                          Volume,
                          ReadFat,
                          Volume->FatEntryPos,
                          Volume->FatEntrySize,
                          &Volume->FatEntryBuffer,
                          NULL
                          );
  if (EFI_ERROR (Status)) {
    Volume->FatEntryBuffer = (UINT32)-1;
  }

  return &Volume->FatEntryBuffer;
}

/**

  Get the FAT entry value of the volume, which is identified with the Index.

  @param  Volume                - FAT file system volume.
  @param  Index                 - The index of the FAT entry of the volume.

  @return  The value of the FAT entry.

**/
STATIC
UINTN
FatGetFatEntry (
  IN FAT_VOLUME  *Volume,
  IN UINTN       Index
  )
{
  VOID    *Pos;
  UINT8   *En12;
  UINT16  *En16;
  UINT32  *En32;
  UINTN   Accum;

  Pos = FatLoadFatEntry (Volume, Index);

  if (Index > (Volume->MaxCluster + 1)) {
    return (UINTN)-1;
  }

  switch (Volume->FatType) {
    case Fat12:
      En12  = Pos;
      Accum = En12[0] | (En12[1] << 8);
      Accum = FAT_ODD_CLUSTER_FAT12 (Index) ? (Accum >> 4) : (Accum & FAT_CLUSTER_MASK_FAT12);
      Accum = Accum | ((Accum >= FAT_CLUSTER_SPECIAL_FAT12) ? FAT_CLUSTER_SPECIAL_EXT : 0);
      break;

    case Fat16:
      En16  = Pos;
      Accum = *En16;
      Accum = Accum | ((Accum >= FAT_CLUSTER_SPECIAL_FAT16) ? FAT_CLUSTER_SPECIAL_EXT : 0);
      break;

    default:
      En32  = Pos;
      Accum = *En32 & FAT_CLUSTER_MASK_FAT32;
      Accum = Accum | ((Accum >= FAT_CLUSTER_SPECIAL_FAT32) ? FAT_CLUSTER_SPECIAL_EXT : 0);
  }

  return Accum;
}

/**

  Set the FAT entry value of the volume, which is identified with the Index.

  @param  Volume                - FAT file system volume.
  @param  Index                 - The index of the FAT entry of the volume.
  @param  Value                 - The new value of the FAT entry.

  @retval EFI_SUCCESS           - Set the new FAT entry value successfully.
  @retval EFI_VOLUME_CORRUPTED  - The FAT type of the volume is error.
  @return other                 - An error occurred when operation the FAT entries.

**/
STATIC
EFI_STATUS
FatSetFatEntry (
  IN FAT_VOLUME  *Volume,
  IN UINTN       Index,
  IN UINTN       Value
  )
{
  VOID        *Pos;
  UINT8       *En12;
  UINT16      *En16;
  UINT32      *En32;
  UINTN       Accum;
  EFI_STATUS  Status;
  UINTN       OriginalVal;

  if (Index < FAT_MIN_CLUSTER) {
    return EFI_VOLUME_CORRUPTED;
  }

  OriginalVal = FatGetFatEntry (Volume, Index);
  if ((Value == FAT_CLUSTER_FREE) && (OriginalVal != FAT_CLUSTER_FREE)) {
    Volume->FatInfoSector.FreeInfo.ClusterCount += 1;
    if (Index < Volume->FatInfoSector.FreeInfo.NextCluster) {
      Volume->FatInfoSector.FreeInfo.NextCluster = (UINT32)Index;
    }
  } else if ((Value != FAT_CLUSTER_FREE) && (OriginalVal == FAT_CLUSTER_FREE)) {
    if (Volume->FatInfoSector.FreeInfo.ClusterCount != 0) {
      Volume->FatInfoSector.FreeInfo.ClusterCount -= 1;
    }
  }

  //
  // Make sure the entry is in memory
  //
  Pos = FatLoadFatEntry (Volume, Index);

  //
  // Update the value
  //
  switch (Volume->FatType) {
    case Fat12:
      En12  = Pos;
      Accum = En12[0] | (En12[1] << 8);
      Value = Value & FAT_CLUSTER_MASK_FAT12;

      if (FAT_ODD_CLUSTER_FAT12 (Index)) {
        Accum = (Value << 4) | (Accum & 0xF);
      } else {
        Accum = Value | (Accum & FAT_CLUSTER_UNMASK_FAT12);
      }

      En12[0] = (UINT8)(Accum & 0xFF);
      En12[1] = (UINT8)(Accum >> 8);
      break;

    case Fat16:
      En16  = Pos;
      *En16 = (UINT16)Value;
      break;

    default:
      En32  = Pos;
      *En32 = (*En32 & FAT_CLUSTER_UNMASK_FAT32) | (UINT32)(Value & FAT_CLUSTER_MASK_FAT32);
  }

  //
  // If the volume's dirty bit is not set, set it now
  //
  if (!Volume->FatDirty && (Volume->FatType != Fat12)) {
    Volume->FatDirty = TRUE;
    FatAccessVolumeDirty (Volume, WriteFat, &Volume->DirtyValue);
  }

  //
  // Write the updated fat entry value to the volume
  // The fat is the first fat, and other fat will be in sync
  // when the FAT cache flush back.
  //
  Status = FatDiskIo (
             Volume,
             WriteFat,
             Volume->FatEntryPos,
             Volume->FatEntrySize,
             &Volume->FatEntryBuffer,
             NULL
             );
  return Status;
}

/**

  Free the cluster chain.

  @param  Volume                - FAT file system volume.
  @param  Cluster               - The first cluster of cluster chain.

  @retval EFI_SUCCESS           - The cluster chain is freed successfully.
  @retval EFI_VOLUME_CORRUPTED  - There are errors in the file's clusters.

**/
STATIC
EFI_STATUS
FatFreeClusters (
  IN FAT_VOLUME  *Volume,
  IN UINTN       Cluster
  )
{
  UINTN  LastCluster;

  while (!FAT_END_OF_FAT_CHAIN (Cluster)) {
    if ((Cluster == FAT_CLUSTER_FREE) || (Cluster >= FAT_CLUSTER_SPECIAL)) {
      DEBUG ((DEBUG_INIT | DEBUG_ERROR, "FatShrinkEof: cluster chain corrupt\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    LastCluster = Cluster;
    Cluster     = FatGetFatEntry (Volume, Cluster);
    FatSetFatEntry (Volume, LastCluster, FAT_CLUSTER_FREE);
  }

  return EFI_SUCCESS;
}

/**

  Allocate a free cluster and return the cluster index.

  @param  Volume                - FAT file system volume.

  @return The index of the free cluster

**/
STATIC
UINTN
FatAllocateCluster (
  IN FAT_VOLUME  *Volume
  )
{
  UINTN  Cluster;

  //
  // Start looking at FatFreePos for the next unallocated cluster
  //
  if (Volume->DiskError) {
    return (UINTN)FAT_CLUSTER_LAST;
  }

  for ( ; ;) {
    //
    // If the end of the list, return no available cluster
    //
    if (Volume->FatInfoSector.FreeInfo.NextCluster > (Volume->MaxCluster + 1)) {
      if (Volume->FreeInfoValid && (0 < (INT32)(Volume->FatInfoSector.FreeInfo.ClusterCount))) {
        Volume->FreeInfoValid = FALSE;
      }

      FatComputeFreeInfo (Volume);
      if (Volume->FatInfoSector.FreeInfo.NextCluster > (Volume->MaxCluster + 1)) {
        return (UINTN)FAT_CLUSTER_LAST;
      }
    }

    Cluster = FatGetFatEntry (Volume, Volume->FatInfoSector.FreeInfo.NextCluster);
    if (Cluster == FAT_CLUSTER_FREE) {
      break;
    }

    //
    // Try the next cluster
    //
    Volume->FatInfoSector.FreeInfo.NextCluster += 1;
  }

  Cluster                                     = Volume->FatInfoSector.FreeInfo.NextCluster;
  Volume->FatInfoSector.FreeInfo.NextCluster += 1;
  return Cluster;
}

/**

  Count the number of clusters given a size.

  @param  Volume                - The file system volume.
  @param  Size                  - The size in bytes.

  @return The number of the clusters.

**/
STATIC
UINTN
FatSizeToClusters (
  IN FAT_VOLUME  *Volume,
  IN UINTN       Size
  )
{
  UINTN  Clusters;

  Clusters = Size >> Volume->ClusterAlignment;
  if ((Size & (Volume->ClusterSize - 1)) > 0) {
    Clusters += 1;
  }

  return Clusters;
}

/**

  Shrink the end of the open file base on the file size.

  @param  OFile                 - The open file.

  @retval EFI_SUCCESS           - Shrinked successfully.
  @retval EFI_VOLUME_CORRUPTED  - There are errors in the file's clusters.

**/
EFI_STATUS
FatShrinkEof (
  IN FAT_OFILE  *OFile
  )
{
  FAT_VOLUME  *Volume;
  UINTN       NewSize;
  UINTN       CurSize;
  UINTN       Cluster;
  UINTN       LastCluster;

  Volume = OFile->Volume;
  ASSERT_VOLUME_LOCKED (Volume);

  NewSize = FatSizeToClusters (Volume, OFile->FileSize);

  //
  // Find the address of the last cluster
  //
  Cluster     = OFile->FileCluster;
  LastCluster = FAT_CLUSTER_FREE;

  if (NewSize != 0) {
    for (CurSize = 0; CurSize < NewSize; CurSize++) {
      if ((Cluster == FAT_CLUSTER_FREE) || (Cluster >= FAT_CLUSTER_SPECIAL)) {
        DEBUG ((DEBUG_INIT | DEBUG_ERROR, "FatShrinkEof: cluster chain corrupt\n"));
        return EFI_VOLUME_CORRUPTED;
      }

      LastCluster = Cluster;
      Cluster     = FatGetFatEntry (Volume, Cluster);
    }

    FatSetFatEntry (Volume, LastCluster, (UINTN)FAT_CLUSTER_LAST);
  } else {
    //
    // Check to see if the file is already completely truncated
    //
    if (Cluster == FAT_CLUSTER_FREE) {
      return EFI_SUCCESS;
    }

    //
    // The file is being completely truncated.
    //
    OFile->FileCluster = FAT_CLUSTER_FREE;
  }

  //
  // Set CurrentCluster == FileCluster
  // to force a recalculation of Position related stuffs
  //
  OFile->FileCurrentCluster = OFile->FileCluster;
  OFile->FileLastCluster    = LastCluster;
  OFile->Dirty              = TRUE;
  //
  // Free the remaining cluster chain
  //
  return FatFreeClusters (Volume, Cluster);
}

/**

  Grow the end of the open file base on the NewSizeInBytes.

  @param  OFile                 - The open file.
  @param  NewSizeInBytes        - The new size in bytes of the open file.

  @retval EFI_SUCCESS           - The file is grown successfully.
  @retval EFI_UNSUPPORTED       - The file size is larger than 4GB.
  @retval EFI_VOLUME_CORRUPTED  - There are errors in the files' clusters.
  @retval EFI_VOLUME_FULL       - The volume is full and can not grow the file.

**/
EFI_STATUS
FatGrowEof (
  IN FAT_OFILE  *OFile,
  IN UINT64     NewSizeInBytes
  )
{
  FAT_VOLUME  *Volume;
  EFI_STATUS  Status;
  UINTN       Cluster;
  UINTN       CurSize;
  UINTN       NewSize;
  UINTN       LastCluster;
  UINTN       NewCluster;
  UINTN       ClusterCount;

  //
  // For FAT file system, the max file is 4GB.
  //
  if (NewSizeInBytes > 0x0FFFFFFFFL) {
    return EFI_UNSUPPORTED;
  }

  Volume = OFile->Volume;
  ASSERT_VOLUME_LOCKED (Volume);
  //
  // If the file is already large enough, do nothing
  //
  CurSize = FatSizeToClusters (Volume, OFile->FileSize);
  NewSize = FatSizeToClusters (Volume, (UINTN)NewSizeInBytes);

  if (CurSize < NewSize) {
    //
    // If we haven't found the files last cluster do it now
    //
    if ((OFile->FileCluster != 0) && (OFile->FileLastCluster == 0)) {
      Cluster      = OFile->FileCluster;
      ClusterCount = 0;

      while (!FAT_END_OF_FAT_CHAIN (Cluster)) {
        if ((Cluster < FAT_MIN_CLUSTER) || (Cluster > Volume->MaxCluster + 1)) {
          DEBUG (
            (DEBUG_INIT | DEBUG_ERROR,
             "FatGrowEof: cluster chain corrupt\n")
            );
          Status = EFI_VOLUME_CORRUPTED;
          goto Done;
        }

        ClusterCount++;
        OFile->FileLastCluster = Cluster;
        Cluster                = FatGetFatEntry (Volume, Cluster);
      }

      if (ClusterCount != CurSize) {
        DEBUG (
          (DEBUG_INIT | DEBUG_ERROR,
           "FatGrowEof: cluster chain size does not match file size\n")
          );
        Status = EFI_VOLUME_CORRUPTED;
        goto Done;
      }
    }

    //
    // Loop until we've allocated enough space
    //
    LastCluster = OFile->FileLastCluster;

    while (CurSize < NewSize) {
      NewCluster = FatAllocateCluster (Volume);
      if (FAT_END_OF_FAT_CHAIN (NewCluster)) {
        if (LastCluster != FAT_CLUSTER_FREE) {
          FatSetFatEntry (Volume, LastCluster, (UINTN)FAT_CLUSTER_LAST);
          OFile->FileLastCluster = LastCluster;
        }

        Status = EFI_VOLUME_FULL;
        goto Done;
      }

      if ((NewCluster < FAT_MIN_CLUSTER) || (NewCluster > Volume->MaxCluster + 1)) {
        Status = EFI_VOLUME_CORRUPTED;
        goto Done;
      }

      if (LastCluster != 0) {
        FatSetFatEntry (Volume, LastCluster, NewCluster);
      } else {
        OFile->FileCluster        = NewCluster;
        OFile->FileCurrentCluster = NewCluster;
      }

      LastCluster = NewCluster;
      CurSize    += 1;

      //
      // Terminate the cluster list
      //
      // Note that we must do this EVERY time we allocate a cluster, because
      // FatAllocateCluster scans the FAT looking for a free cluster and
      // "LastCluster" is no longer free!  Usually, FatAllocateCluster will
      // start looking with the cluster after "LastCluster"; however, when
      // there is only one free cluster left, it will find "LastCluster"
      // a second time.  There are other, less predictable scenarios
      // where this could happen, as well.
      //
      FatSetFatEntry (Volume, LastCluster, (UINTN)FAT_CLUSTER_LAST);
      OFile->FileLastCluster = LastCluster;
    }
  }

  OFile->FileSize = (UINTN)NewSizeInBytes;
  OFile->Dirty    = TRUE;
  return EFI_SUCCESS;

Done:
  FatShrinkEof (OFile);
  return Status;
}

/**

  Seek OFile to requested position, and calculate the number of
  consecutive clusters from the position in the file

  @param  OFile                 - The open file.
  @param  Position              - The file's position which will be accessed.
  @param  PosLimit              - The maximum length current reading/writing may access

  @retval EFI_SUCCESS           - Set the info successfully.
  @retval EFI_VOLUME_CORRUPTED  - Cluster chain corrupt.

**/
EFI_STATUS
FatOFilePosition (
  IN FAT_OFILE  *OFile,
  IN UINTN      Position,
  IN UINTN      PosLimit
  )
{
  FAT_VOLUME  *Volume;
  UINTN       ClusterSize;
  UINTN       Cluster;
  UINTN       StartPos;
  UINTN       Run;

  Volume      = OFile->Volume;
  ClusterSize = Volume->ClusterSize;

  ASSERT_VOLUME_LOCKED (Volume);

  //
  // If this is the fixed root dir, then compute its position
  // from its fixed info in the fat bpb
  //
  if (OFile->IsFixedRootDir) {
    OFile->PosDisk = Volume->RootPos + Position;
    Run            = OFile->FileSize - Position;
  } else {
    //
    // Run the file's cluster chain to find the current position
    // If possible, run from the current cluster rather than
    // start from beginning
    // Assumption: OFile->Position is always consistent with
    // OFile->FileCurrentCluster.
    // OFile->Position is not modified outside this function;
    // OFile->FileCurrentCluster is modified outside this function
    // to be the same as OFile->FileCluster
    // when OFile->FileCluster is updated, so make a check of this
    // and invalidate the original OFile->Position in this case
    //
    Cluster  = OFile->FileCurrentCluster;
    StartPos = OFile->Position;
    if ((Position < StartPos) || (OFile->FileCluster == Cluster)) {
      StartPos = 0;
      Cluster  = OFile->FileCluster;
    }

    while (StartPos + ClusterSize <= Position) {
      StartPos += ClusterSize;
      if ((Cluster == FAT_CLUSTER_FREE) || (Cluster >= FAT_CLUSTER_SPECIAL)) {
        DEBUG ((DEBUG_INIT | DEBUG_ERROR, "FatOFilePosition:" " cluster chain corrupt\n"));
        return EFI_VOLUME_CORRUPTED;
      }

      Cluster = FatGetFatEntry (Volume, Cluster);
    }

    if ((Cluster < FAT_MIN_CLUSTER) || (Cluster > Volume->MaxCluster + 1)) {
      return EFI_VOLUME_CORRUPTED;
    }

    OFile->PosDisk = Volume->FirstClusterPos +
                     LShiftU64 (Cluster - FAT_MIN_CLUSTER, Volume->ClusterAlignment) +
                     Position - StartPos;
    OFile->FileCurrentCluster = Cluster;
    OFile->Position           = StartPos;

    //
    // Compute the number of consecutive clusters in the file
    //
    Run = StartPos + ClusterSize - Position;
    if (!FAT_END_OF_FAT_CHAIN (Cluster)) {
      while ((FatGetFatEntry (Volume, Cluster) == Cluster + 1) && Run < PosLimit) {
        Run     += ClusterSize;
        Cluster += 1;
      }
    }
  }

  OFile->PosRem = Run;
  return EFI_SUCCESS;
}

/**

  Get the size of directory of the open file.

  @param  Volume                - The File System Volume.
  @param  Cluster               - The Starting cluster.

  @return The physical size of the file starting at the input cluster, if there is error in the
  cluster chain, the return value is 0.

**/
UINTN
FatPhysicalDirSize (
  IN FAT_VOLUME  *Volume,
  IN UINTN       Cluster
  )
{
  UINTN  Size;

  ASSERT_VOLUME_LOCKED (Volume);
  //
  // Run the cluster chain for the OFile
  //
  Size = 0;
  //
  // N.B. ".." directories on some media do not contain a starting
  // cluster.  In the case of "." or ".." we don't need the size anyway.
  //
  if (Cluster != 0) {
    while (!FAT_END_OF_FAT_CHAIN (Cluster)) {
      if ((Cluster == FAT_CLUSTER_FREE) || (Cluster >= FAT_CLUSTER_SPECIAL)) {
        DEBUG (
          (DEBUG_INIT | DEBUG_ERROR,
           "FATDirSize: cluster chain corrupt\n")
          );
        return 0;
      }

      Size   += Volume->ClusterSize;
      Cluster = FatGetFatEntry (Volume, Cluster);
    }
  }

  return Size;
}

/**

  Get the physical size of a file on the disk.

  @param  Volume                - The file system volume.
  @param  RealSize              - The real size of a file.

  @return The physical size of a file on the disk.

**/
UINT64
FatPhysicalFileSize (
  IN FAT_VOLUME  *Volume,
  IN UINTN       RealSize
  )
{
  UINTN   ClusterSizeMask;
  UINT64  PhysicalSize;

  ClusterSizeMask = Volume->ClusterSize - 1;
  PhysicalSize    = (RealSize + ClusterSizeMask) & (~((UINT64)ClusterSizeMask));
  return PhysicalSize;
}

/**

  Update the free cluster info of FatInfoSector of the volume.

  @param  Volume                - FAT file system volume.

**/
VOID
FatComputeFreeInfo (
  IN FAT_VOLUME  *Volume
  )
{
  UINTN  Index;

  //
  // If we don't have valid info, compute it now
  //
  if (!Volume->FreeInfoValid) {
    Volume->FreeInfoValid                       = TRUE;
    Volume->FatInfoSector.FreeInfo.ClusterCount = 0;
    for (Index = Volume->MaxCluster + 1; Index >= FAT_MIN_CLUSTER; Index--) {
      if (Volume->DiskError) {
        break;
      }

      if (FatGetFatEntry (Volume, Index) == FAT_CLUSTER_FREE) {
        Volume->FatInfoSector.FreeInfo.ClusterCount += 1;
        Volume->FatInfoSector.FreeInfo.NextCluster   = (UINT32)Index;
      }
    }

    Volume->FatInfoSector.Signature          = FAT_INFO_SIGNATURE;
    Volume->FatInfoSector.InfoBeginSignature = FAT_INFO_BEGIN_SIGNATURE;
    Volume->FatInfoSector.InfoEndSignature   = FAT_INFO_END_SIGNATURE;
  }
}

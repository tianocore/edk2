/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  FileSpace.c

Abstract:

  Routines dealing with disk spaces and FAT table entries

Revision History

--*/

#include "Fat.h"


STATIC
VOID *
FatLoadFatEntry (
  IN FAT_VOLUME       *Volume,
  IN UINTN            Index
  )
/*++

Routine Description:

  Get the FAT entry of the volume, which is identified with the Index.

Arguments:

  Volume                - FAT file system volume.
  Index                 - The index of the FAT entry of the volume.

Returns:

  The buffer of the FAT entry

--*/
{
  UINTN       Pos;
  EFI_STATUS  Status;

  if (Index > (Volume->MaxCluster + 1)) {
    Volume->FatEntryBuffer = (UINT32) -1;
    return &Volume->FatEntryBuffer;
  }
  //
  // Compute buffer position needed
  //
  switch (Volume->FatType) {
  case FAT12:
    Pos = FAT_POS_FAT12 (Index);
    break;

  case FAT16:
    Pos = FAT_POS_FAT16 (Index);
    break;

  default:
    Pos = FAT_POS_FAT32 (Index);
  }
  //
  // Set the position and read the buffer
  //
  Volume->FatEntryPos = Volume->FatPos + Pos;
  Status = FatDiskIo (
             Volume,
             READ_FAT,
             Volume->FatEntryPos,
             Volume->FatEntrySize,
             &Volume->FatEntryBuffer
             );
  if (EFI_ERROR (Status)) {
    Volume->FatEntryBuffer = (UINT32) -1;
  }

  return &Volume->FatEntryBuffer;
}

STATIC
UINTN
FatGetFatEntry (
  IN FAT_VOLUME       *Volume,
  IN UINTN            Index
  )
/*++

Routine Description:

  Get the FAT entry value of the volume, which is identified with the Index.

Arguments:

  Volume                - FAT file system volume.
  Index                 - The index of the FAT entry of the volume.

Returns:

  The value of the FAT entry.

--*/
{
  VOID    *Pos;
  UINT8   *E12;
  UINT16  *E16;
  UINT32  *E32;
  UINTN   Accum;

  Pos = FatLoadFatEntry (Volume, Index);

  if (Index > (Volume->MaxCluster + 1)) {
    return (UINTN) -1;
  }

  switch (Volume->FatType) {
  case FAT12:
    E12   = Pos;
    Accum = E12[0] | (E12[1] << 8);
    Accum = FAT_ODD_CLUSTER_FAT12 (Index) ? (Accum >> 4) : (Accum & FAT_CLUSTER_MASK_FAT12);
    Accum = Accum | ((Accum >= FAT_CLUSTER_SPECIAL_FAT12) ? FAT_CLUSTER_SPECIAL_EXT : 0);
    break;

  case FAT16:
    E16   = Pos;
    Accum = *E16;
    Accum = Accum | ((Accum >= FAT_CLUSTER_SPECIAL_FAT16) ? FAT_CLUSTER_SPECIAL_EXT : 0);
    break;

  default:
    E32   = Pos;
    Accum = *E32 & FAT_CLUSTER_MASK_FAT32;
    Accum = Accum | ((Accum >= FAT_CLUSTER_SPECIAL_FAT32) ? FAT_CLUSTER_SPECIAL_EXT : 0);
  }

  return Accum;
}

STATIC
EFI_STATUS
FatSetFatEntry (
  IN FAT_VOLUME       *Volume,
  IN UINTN            Index,
  IN UINTN            Value
  )
/*++

Routine Description:

  Set the FAT entry value of the volume, which is identified with the Index.

Arguments:

  Volume                - FAT file system volume.
  Index                 - The index of the FAT entry of the volume.
  Value                 - The new value of the FAT entry.

Returns:

  EFI_SUCCESS           - Set the new FAT entry value sucessfully.
  EFI_VOLUME_CORRUPTED  - The FAT type of the volume is error.
  other                 - An error occurred when operation the FAT entries.

--*/
{
  VOID        *Pos;
  UINT8       *E12;
  UINT16      *E16;
  UINT32      *E32;
  UINTN       Accum;
  EFI_STATUS  Status;
  UINTN       OriginalVal;

  if (Index < FAT_MIN_CLUSTER) {
    return EFI_VOLUME_CORRUPTED;
  }

  OriginalVal = FatGetFatEntry (Volume, Index);
  if (Value == FAT_CLUSTER_FREE && OriginalVal != FAT_CLUSTER_FREE) {
    Volume->FatInfoSector.FreeInfo.ClusterCount += 1;
    if (Index < Volume->FatInfoSector.FreeInfo.NextCluster) {
      Volume->FatInfoSector.FreeInfo.NextCluster = (UINT32) Index;
    }
  } else if (Value != FAT_CLUSTER_FREE && OriginalVal == FAT_CLUSTER_FREE) {
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
  case FAT12:
    E12   = Pos;
    Accum = E12[0] | (E12[1] << 8);
    Value = Value & FAT_CLUSTER_MASK_FAT12;

    if (FAT_ODD_CLUSTER_FAT12 (Index)) {
      Accum = (Value << 4) | (Accum & 0xF);
    } else {
      Accum = Value | (Accum & FAT_CLUSTER_UNMASK_FAT12);
    }

    E12[0]  = (UINT8) (Accum & 0xFF);
    E12[1]  = (UINT8) (Accum >> 8);
    break;

  case FAT16:
    E16   = Pos;
    *E16  = (UINT16) Value;
    break;

  default:
    E32   = Pos;
    *E32  = (*E32 & FAT_CLUSTER_UNMASK_FAT32) | (UINT32) (Value & FAT_CLUSTER_MASK_FAT32);
  }
  //
  // If the volume's dirty bit is not set, set it now
  //
  if (!Volume->FatDirty && Volume->FatType != FAT12) {
    Volume->FatDirty = TRUE;
    FatAccessVolumeDirty (Volume, WRITE_FAT, &Volume->DirtyValue);
  }
  //
  // Write the updated fat entry value to the volume
  // The fat is the first fat, and other fat will be in sync
  // when the FAT cache flush back.
  //
  Status = FatDiskIo (
             Volume,
             WRITE_FAT,
             Volume->FatEntryPos,
             Volume->FatEntrySize,
             &Volume->FatEntryBuffer
             );
  return Status;
}

STATIC
EFI_STATUS
FatFreeClusters (
  IN FAT_VOLUME           *Volume,
  IN UINTN                Cluster
  )
/*++

Routine Description:

  Free the cluster clain.

Arguments:

  Volume                - FAT file system volume.
  Cluster               - The first cluster of cluster chain.

Returns:

  EFI_SUCCESS           - The cluster chain is freed successfully.
  EFI_VOLUME_CORRUPTED  - There are errors in the file's clusters.

--*/
{
  UINTN LastCluster;

  while (!FAT_END_OF_FAT_CHAIN (Cluster)) {
    if (Cluster == FAT_CLUSTER_FREE || Cluster >= FAT_CLUSTER_SPECIAL) {

      DEBUG ((EFI_D_INIT | EFI_D_ERROR, "FatShrinkEof: cluster chain corrupt\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    LastCluster = Cluster;
    Cluster     = FatGetFatEntry (Volume, Cluster);
    FatSetFatEntry (Volume, LastCluster, FAT_CLUSTER_FREE);
  }

  return EFI_SUCCESS;
}

STATIC
UINTN
FatAllocateCluster (
  IN FAT_VOLUME   *Volume
  )
/*++

Routine Description:

  Allocate a free cluster and return the cluster index.

Arguments:

  Volume                - FAT file system volume.

Returns:

  The index of the free cluster

--*/
{
  UINTN Cluster;

  //
  // Start looking at FatFreePos for the next unallocated cluster
  //
  if (Volume->DiskError) {
    return (UINTN) FAT_CLUSTER_LAST;
  }

  for (;;) {
    //
    // If the end of the list, return no available cluster
    //
    if (Volume->FatInfoSector.FreeInfo.NextCluster > (Volume->MaxCluster + 1)) {
      if (Volume->FreeInfoValid && 0 < (INT32) (Volume->FatInfoSector.FreeInfo.ClusterCount)) {
        Volume->FreeInfoValid = FALSE;
      }

      FatComputeFreeInfo (Volume);
      if (Volume->FatInfoSector.FreeInfo.NextCluster > (Volume->MaxCluster + 1)) {
        return (UINTN) FAT_CLUSTER_LAST;
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

  Cluster = Volume->FatInfoSector.FreeInfo.NextCluster;
  Volume->FatInfoSector.FreeInfo.NextCluster += 1;
  return Cluster;
}

STATIC
UINTN
FatSizeToClusters (
  IN FAT_VOLUME       *Volume,
  IN UINTN            Size
  )
/*++

Routine Description:

  Count the number of clusters given a size

Arguments:

  Volume                - The file system volume.
  Size                  - The size in bytes.

Returns:

  The number of the clusters.

--*/
{
  UINTN Clusters;

  Clusters = Size >> Volume->ClusterAlignment;
  if ((Size & (Volume->ClusterSize - 1)) > 0) {
    Clusters += 1;
  }

  return Clusters;
}

EFI_STATUS
FatShrinkEof (
  IN FAT_OFILE            *OFile
  )
/*++

Routine Description:

  Shrink the end of the open file base on the file size.

Arguments:

  OFile                 - The open file.

Returns:

  EFI_SUCCESS           - Shrinked sucessfully.
  EFI_VOLUME_CORRUPTED  - There are errors in the file's clusters.

--*/
{
  FAT_VOLUME  *Volume;
  UINTN       NewSize;
  UINTN       CurSize;
  UINTN       Cluster;
  UINTN       LastCluster;

  Volume  = OFile->Volume;
  ASSERT_VOLUME_LOCKED (Volume);

  NewSize = FatSizeToClusters (Volume, OFile->FileSize);

  //
  // Find the address of the last cluster
  //
  Cluster     = OFile->FileCluster;
  LastCluster = FAT_CLUSTER_FREE;

  if (NewSize != 0) {

    for (CurSize = 0; CurSize < NewSize; CurSize++) {
      if (Cluster == FAT_CLUSTER_FREE || Cluster >= FAT_CLUSTER_SPECIAL) {

        DEBUG ((EFI_D_INIT | EFI_D_ERROR, "FatShrinkEof: cluster chain corrupt\n"));
        return EFI_VOLUME_CORRUPTED;
      }

      LastCluster = Cluster;
      Cluster     = FatGetFatEntry (Volume, Cluster);
    }

    FatSetFatEntry (Volume, LastCluster, (UINTN) FAT_CLUSTER_LAST);

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
    OFile->FileCluster      = FAT_CLUSTER_FREE;
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

EFI_STATUS
FatGrowEof (
  IN FAT_OFILE            *OFile,
  IN UINT64               NewSizeInBytes
  )
/*++

Routine Description:

  Grow the end of the open file base on the NewSizeInBytes.

Arguments:

  OFile                 - The open file.
  NewSizeInBytes        - The new size in bytes of the open file.

Returns:

  EFI_SUCCESS           - The file is grown sucessfully.
  EFI_UNSUPPORTED       - The file size is larger than 4GB.
  EFI_VOLUME_CORRUPTED  - There are errors in the files' clusters.
  EFI_VOLUME_FULL       - The volume is full and can not grow the file.

--*/
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
  NewSize = FatSizeToClusters (Volume, (UINTN) NewSizeInBytes);

  if (CurSize < NewSize) {
    //
    // If we haven't found the files last cluster do it now
    //
    if ((OFile->FileCluster != 0) && (OFile->FileLastCluster == 0)) {
      Cluster       = OFile->FileCluster;
      ClusterCount  = 0;

      while (!FAT_END_OF_FAT_CHAIN (Cluster)) {
        if (Cluster == FAT_CLUSTER_FREE || Cluster >= FAT_CLUSTER_SPECIAL) {

          DEBUG (
            (EFI_D_INIT | EFI_D_ERROR,
            "FatGrowEof: cluster chain corrupt\n")
            );
          Status = EFI_VOLUME_CORRUPTED;
          goto Done;
        }

        ClusterCount++;
        OFile->FileLastCluster  = Cluster;
        Cluster                 = FatGetFatEntry (Volume, Cluster);
      }

      if (ClusterCount != CurSize) {
        DEBUG (
          (EFI_D_INIT | EFI_D_ERROR,
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
          FatSetFatEntry (Volume, LastCluster, (UINTN) FAT_CLUSTER_LAST);
          OFile->FileLastCluster = LastCluster;
        }

        Status = EFI_VOLUME_FULL;
        goto Done;
      }

      if (LastCluster != 0) {
        FatSetFatEntry (Volume, LastCluster, NewCluster);
      } else {
        OFile->FileCluster        = NewCluster;
        OFile->FileCurrentCluster = NewCluster;
      }

      LastCluster = NewCluster;
      CurSize += 1;
    }
    //
    // Terminate the cluster list
    //
    FatSetFatEntry (Volume, LastCluster, (UINTN) FAT_CLUSTER_LAST);
    OFile->FileLastCluster = LastCluster;
  }

  OFile->FileSize = (UINTN) NewSizeInBytes;
  OFile->Dirty    = TRUE;
  return EFI_SUCCESS;

Done:
  FatShrinkEof (OFile);
  return Status;
}

EFI_STATUS
FatOFilePosition (
  IN FAT_OFILE            *OFile,
  IN UINTN                Position,
  IN UINTN                PosLimit
  )
/*++

Routine Description:

  Seek OFile to requested position, and calculate the number of
  consecutive clusters from the position in the file

Arguments:

  OFile                 - The open file.
  Position              - The file's position which will be accessed.
  PosLimit              - The maximum length current reading/writing may access

Returns:

  EFI_SUCCESS           - Set the info successfully.
  EFI_VOLUME_CORRUPTED  - Cluster chain corrupt.

--*/
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
  // If this is the fixed root dir, then compute it's position
  // from it's fixed info in the fat bpb
  //
  if (OFile->IsFixedRootDir) {
    OFile->PosDisk  = Volume->RootPos + Position;
    Run             = OFile->FileSize - Position;
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
    Cluster     = OFile->FileCurrentCluster;
    StartPos    = OFile->Position;
    if (Position < StartPos || OFile->FileCluster == Cluster) {
      StartPos  = 0;
      Cluster   = OFile->FileCluster;
    }

    while (StartPos + ClusterSize <= Position) {
      StartPos += ClusterSize;
      if (Cluster == FAT_CLUSTER_FREE || (Cluster >= FAT_CLUSTER_SPECIAL)) {
        DEBUG ((EFI_D_INIT | EFI_D_ERROR, "FatOFilePosition:"" cluster chain corrupt\n"));
        return EFI_VOLUME_CORRUPTED;
      }

      Cluster = FatGetFatEntry (Volume, Cluster);
    }

    if (Cluster < FAT_MIN_CLUSTER) {
      return EFI_VOLUME_CORRUPTED;
    }

    OFile->PosDisk            = Volume->FirstClusterPos +
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

UINTN
FatPhysicalDirSize (
  IN FAT_VOLUME            *Volume,
  IN UINTN                 Cluster
  )
/*++

Routine Description:

 Get the size of directory of the open file

Arguments:

  Volume                - The File System Volume.
  Cluster               - The Starting cluster.

Returns:

  The physical size of the file starting at the input cluster, if there is error in the
  cluster chain, the return value is 0.

--*/
{
  UINTN Size;
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
      if (Cluster == FAT_CLUSTER_FREE || Cluster >= FAT_CLUSTER_SPECIAL) {
        DEBUG (
          (EFI_D_INIT | EFI_D_ERROR,
          "FATDirSize: cluster chain corrupt\n")
          );
        return 0;
      }

      Size += Volume->ClusterSize;
      Cluster = FatGetFatEntry (Volume, Cluster);
    }
  }

  return Size;
}

UINT64
FatPhysicalFileSize (
  IN FAT_VOLUME            *Volume,
  IN UINTN                 RealSize
  )
/*++

Routine Description:

 Get the physical size of a file on the disk.

Arguments:

  Volume                - The file system volume.
  RealSize              - The real size of a file.

Returns:

  The physical size of a file on the disk.

--*/
{
  UINTN   ClusterSizeMask;
  UINT64  PhysicalSize;
  ClusterSizeMask = Volume->ClusterSize - 1;
  PhysicalSize    = (RealSize + ClusterSizeMask) & (~((UINT64) ClusterSizeMask));
  return PhysicalSize;
}

VOID
FatComputeFreeInfo (
  IN FAT_VOLUME *Volume
  )
/*++

Routine Description:

  Update the free cluster info of FatInfoSector of the volume.

Arguments:

  Volume                - FAT file system volume.

Returns:

  None.

--*/
{
  UINTN Index;

  //
  // If we don't have valid info, compute it now
  //
  if (!Volume->FreeInfoValid) {

    Volume->FreeInfoValid                        = TRUE;
    Volume->FatInfoSector.FreeInfo.ClusterCount  = 0;
    for (Index = Volume->MaxCluster + 1; Index >= FAT_MIN_CLUSTER; Index--) {
      if (Volume->DiskError) {
        break;
      }

      if (FatGetFatEntry (Volume, Index) == FAT_CLUSTER_FREE) {
        Volume->FatInfoSector.FreeInfo.ClusterCount += 1;
        Volume->FatInfoSector.FreeInfo.NextCluster = (UINT32) Index;
      }
    }

    Volume->FatInfoSector.Signature          = FAT_INFO_SIGNATURE;
    Volume->FatInfoSector.InfoBeginSignature = FAT_INFO_BEGIN_SIGNATURE;
    Volume->FatInfoSector.InfoEndSignature   = FAT_INFO_END_SIGNATURE;
  }
}

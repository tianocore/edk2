#include "fat.h"
#include <stdio.h>

INTN
GetDrvNumOffset (
  IN VOID *BootSector
  )
{
  FAT_BPB_STRUCT  *FatBpb;
  UINTN           RootDirSectors;
  UINTN           FATSz;
  UINTN           TotSec;
  UINTN           DataSec;
  UINTN           CountOfClusters;

  FatBpb  = (FAT_BPB_STRUCT *) BootSector;

  //
  // Check FAT type algorithm from FAT spec
  //
  RootDirSectors = ((FatBpb->Fat12_16.BPB_RootEntCnt * sizeof(FAT_DIRECTORY_ENTRY)) +
                    (FatBpb->Fat12_16.BPB_BytsPerSec - 1)) / FatBpb->Fat12_16.BPB_BytsPerSec;

  if (FatBpb->Fat12_16.BPB_FATSz16 != 0) {
    FATSz = FatBpb->Fat12_16.BPB_FATSz16;
  } else {
    FATSz = FatBpb->Fat32.BPB_FATSz32;
  }
  if (FATSz == 0) {
    fprintf (stderr, "ERROR: FAT: BPB_FATSz16, BPB_FATSz32 - 0, expected - Non-Zero\n");
    return -1;
  }

  if (FatBpb->Fat12_16.BPB_TotSec16 != 0) {
    TotSec = FatBpb->Fat12_16.BPB_TotSec16;
  } else {
    TotSec = FatBpb->Fat12_16.BPB_TotSec32;
  }
  if (TotSec == 0) {
    fprintf (stderr, "ERROR: FAT: BPB_TotSec16, BPB_TotSec32 - 0, expected - Non-Zero\n");
    return -1;
  }

  DataSec = TotSec - (
                      FatBpb->Fat12_16.BPB_RsvdSecCnt +
                      FatBpb->Fat12_16.BPB_NumFATs * FATSz +
                      RootDirSectors
                     );

  CountOfClusters = DataSec / FatBpb->Fat12_16.BPB_SecPerClus;

  if (CountOfClusters < FAT_MAX_FAT16_CLUSTER) {
    return (INTN) ((UINTN) &FatBpb->Fat12_16.BS_DrvNum - (UINTN) FatBpb);
  } else {
    return (INTN) ((UINTN) &FatBpb->Fat32.BS_DrvNum - (UINTN) FatBpb);
  }
}


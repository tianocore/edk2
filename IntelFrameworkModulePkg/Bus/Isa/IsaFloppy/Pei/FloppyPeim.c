/*++

Copyright (c) 2006, Intel Corporation. All rights reserved. <BR> 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

    FloppyPeim.c
    
Abstract: 
    

Revision History
--*/


#include "FloppyPeim.h"
#include "IndustryStandard/Pcat.h"
//
// #include "sio.h"
//

#define PageSize                4096
#define ISA_MAX_MEMORY_ADDRESS  0x1000000 // 16 MB Memory Range
UINT16                    FdcBaseAddress = 0x3f0;

static DISKET_PARA_TABLE  DiskPara[9] = {
  {
    0x09,
    0x50,
    0xff,
    0x2,
    0x27,
    0x4,
    0x25,
    0x14,
    0x80
  },
  {
    0x09,
    0x2a,
    0xff,
    0x2,
    0x27,
    0x4,
    0x25,
    0x0f,
    0x40
  },
  {
    0x0f,
    0x54,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x0
  },
  {
    0x09,
    0x50,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x80
  },
  {
    0x09,
    0x2a,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x80
  },
  {
    0x12,
    0x1b,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x0
  },
  {
    0x09,
    0x2a,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x80
  },
  {
    0x12,
    0x1b,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x0
  },
  {
    0x24,
    0x1b,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0xc0
  }
};

static UINTN              BytePerSector[6] = { 0, 256, 512, 1024, 2048, 4096 };

//
// PEIM Entry Ppint
//

EFI_STATUS
FdcPeimEntry (
  IN  EFI_FFS_FILE_HEADER   *FfsHeader,
  IN  EFI_PEI_SERVICES      **PeiServices
  )
/*++

Routine Description:

  Initializes the Fdc Block Io PPI

Arguments:

  PeiServices           - General purpose services available to every PEIM.
  FfsHeader             - Ffs header pointer
  
Returns:

  EFI_UNSUPPORTED       - Can't find neccessary Ppi.
  EFI_OUT_OF_RESOURCES  - Have no enough memory to create instance or descriptors.
  EFI_SUCCESS           - Success.    

--*/
{
  UINTN                 MemPages;
  EFI_STATUS            Status;
  FDC_BLK_IO_DEV        *FdcBlkIoDev;
  EFI_PHYSICAL_ADDRESS  TempPtr;

  //
  // Initializing PEI floppy driver.
  //
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_PERIPHERAL_REMOVABLE_MEDIA + EFI_P_PC_INIT);

  //
  // Data
  //
  // Allocate PEI instance data.
  //
  MemPages = sizeof (FDC_BLK_IO_DEV) / PageSize + 1;
  Status = PeiServicesAllocatePages (
             EfiConventionalMemory,
             MemPages,
             &TempPtr
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize PEI instance data.
  //
  FdcBlkIoDev               = (FDC_BLK_IO_DEV *) ((UINTN) TempPtr);
  FdcBlkIoDev->Signature    = FDC_BLK_IO_DEV_SIGNATURE;
 
  //
  // InitSio ();
  //
  FdcEnumeration (FdcBlkIoDev);

  FdcBlkIoDev->FdcBlkIo.GetNumberOfBlockDevices = FdcGetNumberOfBlockDevices;
  FdcBlkIoDev->FdcBlkIo.GetBlockDeviceMediaInfo = FdcGetBlockDeviceMediaInfo;
  FdcBlkIoDev->FdcBlkIo.ReadBlocks              = FdcReadBlocks;

  FdcBlkIoDev->PpiDescriptor.Flags              = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  FdcBlkIoDev->PpiDescriptor.Guid               = &gEfiPei144FloppyBlockIoPpiGuid;
  FdcBlkIoDev->PpiDescriptor.Ppi                = &FdcBlkIoDev->FdcBlkIo;

  if (FdcBlkIoDev->DeviceCount != 0) {
    Status = PeiServicesInstallPpi (&FdcBlkIoDev->PpiDescriptor);
    if (EFI_ERROR (Status)) {
      //
      // PeiServicesFreePages (TempPtr, MemPages);
      //
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // PeiServicesFreePages (TempPtr, MemPages);
    //
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FdcGetNumberOfBlockDevices (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  OUT  UINTN                             *NumberBlockDevices
  )
/*++

Routine Description:

Arguments:
    
Returns:

--*/
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    NumberBlockDevices - add argument and description to function comment
// GC_TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  FDC_BLK_IO_DEV  *FdcBlkIoDev;

  FdcBlkIoDev = NULL;
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FdcBlkIoDev         = PEI_RECOVERY_FDC_FROM_BLKIO_THIS (This);

  *NumberBlockDevices = FdcBlkIoDev->DeviceCount;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FdcGetBlockDeviceMediaInfo (
  IN   EFI_PEI_SERVICES                     **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI        *This,
  IN   UINTN                                DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO_MEDIA               *MediaInfo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This        - GC_TODO: add argument description
  DeviceIndex - GC_TODO: add argument description
  MediaInfo   - GC_TODO: add argument description

Returns:

  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINTN           DeviceCount;
  FDC_BLK_IO_DEV  *FdcBlkIoDev;
  BOOLEAN         bStatus;

  FdcBlkIoDev = NULL;

  if (This == NULL || MediaInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FdcBlkIoDev = PEI_RECOVERY_FDC_FROM_BLKIO_THIS (This);

  DeviceCount = FdcBlkIoDev->DeviceCount;

  //
  // DeviceIndex is zero-based value.
  //
  if (DeviceIndex > DeviceCount - 1) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // probe media and retrieve latest media information
  //
  bStatus = DiscoverFdcDevice (
              FdcBlkIoDev,
              &FdcBlkIoDev->DeviceInfo[DeviceIndex],
              MediaInfo
              );

  if (!bStatus) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (
    &(FdcBlkIoDev->DeviceInfo[DeviceIndex].MediaInfo),
    MediaInfo,
    sizeof (EFI_PEI_BLOCK_IO_MEDIA)
    );
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FdcReadBlocks (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  IN   UINTN                             DeviceIndex,
  IN   EFI_PEI_LBA                       StartLba,
  IN   UINTN                             BufferSize,
  OUT  VOID                              *Buffer
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This        - GC_TODO: add argument description
  DeviceIndex - GC_TODO: add argument description
  StartLba    - GC_TODO: add argument description
  BufferSize  - GC_TODO: add argument description
  Buffer      - GC_TODO: add argument description

Returns:

  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_NO_MEDIA - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value

--*/
{

  EFI_PEI_BLOCK_IO_MEDIA MediaInfo;
  EFI_STATUS            Status;
  UINTN                 i;
  UINTN                 NumberOfBlocks;
  UINTN                 BlockSize;
  FDC_BLK_IO_DEV        *FdcBlkIoDev;
  EFI_PHYSICAL_ADDRESS  MemPage;

  FdcBlkIoDev = NULL;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FdcBlkIoDev = PEI_RECOVERY_FDC_FROM_BLKIO_THIS (This);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FdcGetBlockDeviceMediaInfo (PeiServices, This, DeviceIndex, &MediaInfo);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  BlockSize = MediaInfo.BlockSize;

  if (BufferSize % BlockSize != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (!MediaInfo.MediaPresent) {
    return EFI_NO_MEDIA;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  //
  // allocate 40 blocks: 5*4k=20k=20*1024=40blocks
  //
  MemPage = ISA_MAX_MEMORY_ADDRESS - 1;
  Status = PeiServicesAllocatePages (
             EfiConventionalMemory,
             ((BufferSize % EFI_PAGE_SIZE) ? (BufferSize / EFI_PAGE_SIZE + 1) : (BufferSize / EFI_PAGE_SIZE)),
             &MemPage
             );
  if (EFI_ERROR (Status) || (MemPage >= ISA_MAX_MEMORY_ADDRESS)) {
    //
    // If failed, designate the address space for DMA
    //
    MemPage = 0x0f00000;
    //
    // return EFI_OUT_OF_RESOURCES;
    //
  }
  //
  // MemPage = (EFI_PHYSICAL_ADDRESS)(UINTN)Temp;
  //
  Status = MotorOn (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DeviceIndex]));
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Status = Setup (FdcBlkIoDev, FdcBlkIoDev->DeviceInfo[DeviceIndex].DevPos);
  if (Status != EFI_SUCCESS) {
    MotorOff (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DeviceIndex]));
    return EFI_DEVICE_ERROR;
  }
  //
  // read blocks in the same cylinder.
  // in a cylinder , there are 18 * 2 = 36 blocks
  //
  while ((i = GetTransferBlockCount (
                &(FdcBlkIoDev->DeviceInfo[DeviceIndex]),
                StartLba,
                NumberOfBlocks
                )) != 0 && Status == EFI_SUCCESS) {
    Status = ReadWriteDataSector (
              FdcBlkIoDev,
              &(FdcBlkIoDev->DeviceInfo[DeviceIndex]),
              (UINT8 *) (UINTN) MemPage,
              StartLba,
              i,
              READ
              );
    CopyMem ((UINT8 *) Buffer, (UINT8 *) (UINTN) MemPage, BlockSize * i);
    StartLba += i;
    NumberOfBlocks -= i;
    Buffer = (VOID *) ((UINTN) Buffer + i * BlockSize);
  }
  //
  // PeiServicesFreePages (MemPage, 5);
  //
  MotorOff (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DeviceIndex]));

  switch (Status) {
  case EFI_SUCCESS:
    return EFI_SUCCESS;

  default:
    FdcReset (FdcBlkIoDev, FdcBlkIoDev->DeviceInfo[DeviceIndex].DevPos);
    return EFI_DEVICE_ERROR;
  }
  //
  // return Status;
  //
}
//
// Internal function Implementation
//
UINT8
FdcEnumeration (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev
  )
/*++

Routine Description:

  Enumerate floppy device

Arguments:

  FdcBlkIoDev - Instance of floppy device controller

Returns:

  DevNo       - Device No.

--*/
{
  UINT8                   DevPos;
  UINT8                   DevNo;
  EFI_PEI_BLOCK_IO_MEDIA  MediaInfo;
  EFI_STATUS              Status;

  DevNo = 0;

  //
  // DevPos=0 means A: 1 means B:
  //
  for (DevPos = 0; DevPos < 2; DevPos++) {
    //
    // Detecting device presence
    //
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_PERIPHERAL_REMOVABLE_MEDIA + EFI_P_PC_PRESENCE_DETECT);

    //
    // Data
    //
    // Reset FDC
    //
    Status = FdcReset (FdcBlkIoDev, DevPos);

    if (EFI_ERROR (Status)) {
      continue;
    }

    FdcBlkIoDev->DeviceInfo[DevPos].DevPos          = DevPos;
    FdcBlkIoDev->DeviceInfo[DevPos].Pcn             = 0;
    FdcBlkIoDev->DeviceInfo[DevPos].MotorOn         = FALSE;
    FdcBlkIoDev->DeviceInfo[DevPos].NeedRecalibrate = TRUE;
    FdcBlkIoDev->DeviceInfo[DevPos].Type            = _1440K_1440K;

    //
    // Discover FDC device
    //
    if (DiscoverFdcDevice (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DevPos]), &MediaInfo)) {
      FdcBlkIoDev->DeviceInfo[DevNo].DevPos           = DevPos;

      FdcBlkIoDev->DeviceInfo[DevNo].Pcn              = FdcBlkIoDev->DeviceInfo[DevPos].Pcn;
      FdcBlkIoDev->DeviceInfo[DevNo].MotorOn          = FdcBlkIoDev->DeviceInfo[DevPos].MotorOn;
      FdcBlkIoDev->DeviceInfo[DevNo].NeedRecalibrate  = FdcBlkIoDev->DeviceInfo[DevPos].NeedRecalibrate;
      FdcBlkIoDev->DeviceInfo[DevNo].Type             = FdcBlkIoDev->DeviceInfo[DevPos].Type;

      CopyMem (
        &(FdcBlkIoDev->DeviceInfo[DevNo].MediaInfo),
        &MediaInfo,
        sizeof (EFI_PEI_BLOCK_IO_MEDIA)
        );

      DevNo++;
    } else {
      //
      // Assume controller error
      //
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        EFI_PERIPHERAL_REMOVABLE_MEDIA + EFI_P_EC_CONTROLLER_ERROR
        );

      //
      // Data
      //
    }
  }

  FdcBlkIoDev->DeviceCount = DevNo;
  return DevNo;
}

BOOLEAN
DiscoverFdcDevice (
  IN  FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN  OUT PEI_FLOPPY_DEVICE_INFO *Info,
  OUT EFI_PEI_BLOCK_IO_MEDIA     *MediaInfo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  Info        - GC_TODO: add argument description
  MediaInfo   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  EFI_STATUS        Status;
  DISKET_PARA_TABLE *Para;

  Status = MotorOn (FdcBlkIoDev, Info);
  if (Status != EFI_SUCCESS) {
    return FALSE;
  }

  Status = Recalibrate (FdcBlkIoDev, Info);

  if (Status != EFI_SUCCESS) {
    MotorOff (FdcBlkIoDev, Info);
    return FALSE;
  }
  //
  // Set Media Parameter
  //
  MediaInfo->DeviceType   = LegacyFloppy;
  MediaInfo->MediaPresent = TRUE;

  //
  // Check Media
  //
  Status = DisketChanged (FdcBlkIoDev, Info);
  switch (Status) {
  case EFI_NO_MEDIA:
    MediaInfo->MediaPresent = FALSE;
    break;

  case EFI_MEDIA_CHANGED:
  case EFI_SUCCESS:
    break;

  default:
    //
    // EFI_DEVICE_ERROR
    //
    MotorOff (FdcBlkIoDev, Info);
    return FALSE;
  }

  MotorOff (FdcBlkIoDev, Info);

  Para                  = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);
  MediaInfo->BlockSize  = BytePerSector[Para->Number];
  MediaInfo->LastBlock  = Para->EOT * 2 * (Para->MaxTrackNum + 1) - 1;

  return TRUE;
}

EFI_STATUS
FdcReset (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            DevPos
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  DevPos      - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINT8 data;
  UINT8 sts0;
  UINT8 pcn;
  UINTN i;

  //
  // Reset specified Floppy Logic Drive according to Fdd -> Disk
  // Set Digital Output Register(DOR) to do reset work
  //    bit0 & bit1 of DOR : Drive Select
  //    bit2 : Reset bit
  //    bit3 : DMA and Int bit
  // Reset : A "0" written to bit2 resets the FDC, this reset will remain active until
  //       a "1" is written to this bit.
  // Reset step 1:
  //    use bit0 & bit1 to  select the logic drive
  //    write "0" to bit2
  //
  data = 0x0;
  data = (UINT8) (data | (SELECT_DRV & DevPos));
  IoWrite8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DOR), data);

  //
  // wait some time,at least 120us
  //
  MicroSecondDelay (500);
  //
  // Reset step 2:
  //    write "1" to bit2
  //    write "1" to bit3 : enable DMA
  //
  data |= 0x0C;
  IoWrite8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DOR), data);

  MicroSecondDelay (2000);
 
  //
  // wait specified floppy logic drive is not busy
  //
  if (FdcWaitForBSYClear (FdcBlkIoDev, DevPos, 1) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Set the Transfer Data Rate
  //
  IoWrite8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_CCR), 0x0);

  MicroSecondDelay (100);

  //
  // Issue Sense interrupt command for each drive (total 4 drives)
  //
  for (i = 0; i < 4; i++) {
    if (SenseIntStatus (FdcBlkIoDev, &sts0, &pcn) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // issue Specify command
  //
  if (Specify (FdcBlkIoDev) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
FdcWaitForBSYClear (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            DevPos,
  IN UINTN            TimeoutInSeconds
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev       - GC_TODO: add argument description
  DevPos            - GC_TODO: add argument description
  TimeoutInSeconds  - GC_TODO: add argument description

Returns:

  EFI_TIMEOUT - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINTN   Delay;
  UINT8   StatusRegister;
  UINT8   Mask;

  //
  // How to determine drive and command are busy or not: by the bits of Main Status Register
  // bit0: Drive 0 busy (drive A)
  // bit1: Drive 1 busy (drive B)
  // bit4: Command busy
  //
  // set mask: for drive A set bit0 & bit4; for drive B set bit1 & bit4
  //
  Mask  = (UINT8) ((DevPos == 0 ? MSR_DAB : MSR_DBB) | MSR_CB);

  Delay = ((TimeoutInSeconds * STALL_1_MSECOND) / 50) + 1;

  do {
    StatusRegister = IoRead8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_MSR));

    if ((StatusRegister & Mask) == 0x00) {
      break;
      //
      // not busy
      //
    }

    MicroSecondDelay (50);
  } while (--Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SenseIntStatus (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN OUT UINT8        *sts0,
  IN OUT UINT8        *pcn
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  sts0        - GC_TODO: add argument description
  pcn         - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINT8 command;

  command = SENSE_INT_STATUS_CMD;
  if (DataOutByte (FdcBlkIoDev, &command) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (DataInByte (FdcBlkIoDev, sts0) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (DataInByte (FdcBlkIoDev, pcn) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
Specify (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  FDC_SPECIFY_CMD Command;
  UINTN           i;
  UINT8           *pt;

  ZeroMem (&Command, sizeof (FDC_SPECIFY_CMD));
  Command.CommandCode = SPECIFY_CMD;
  //
  // set SRT, HUT
  //
  Command.SrtHut = 0xdf;
  //
  // 0xdf;
  // set HLT and DMA
  //
  Command.HltNd = 0x02;

  pt            = (UINT8 *) (&Command);
  for (i = 0; i < sizeof (FDC_SPECIFY_CMD); i++) {
    if (DataOutByte (FdcBlkIoDev, pt++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DataInByte (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN OUT UINT8        *pt
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  pt          - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINT8 data;

  //
  // wait for 1ms and detect the FDC is ready to be read
  //
  if (FdcDRQReady (FdcBlkIoDev, DATA_IN, 1) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
    //
    // is not ready
    //
  }

  data = IoRead8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DTR));
  MicroSecondDelay (50);
  *pt = data;
  return EFI_SUCCESS;
}

EFI_STATUS
DataOutByte (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            *pt
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  pt          - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINT8 data;

  //
  // wait for 1ms and detect the FDC is ready to be written
  //
  if (FdcDRQReady (FdcBlkIoDev, DATA_OUT, 1) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
    //
    // is not ready
    //
  }

  data = *pt;
  IoWrite8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DTR), data);
  MicroSecondDelay (50);
  return EFI_SUCCESS;
}

EFI_STATUS
FdcDRQReady (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN BOOLEAN          Dio,
  IN UINTN            TimeoutInSeconds
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev       - GC_TODO: add argument description
  Dio               - GC_TODO: add argument description
  TimeoutInSeconds  - GC_TODO: add argument description

Returns:

  EFI_NOT_READY - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINTN   Delay;
  UINT8   StatusRegister;
  UINT8   DataInOut;

  //
  // Before writing to FDC or reading from FDC, the Host must examine
  // the bit7(RQM) and bit6(DIO) of the Main Status Register.
  // That is to say:
  //  command bytes can not be written to Data Register unless RQM is 1 and DIO is 0
  //  result bytes can not be read from Data Register unless RQM is 1 and DIO is 1
  //
  DataInOut = (UINT8) (Dio << 6);
  //
  // in order to compare bit6
  //
  Delay = ((TimeoutInSeconds * STALL_1_MSECOND) / 50) + 1;
  do {
    StatusRegister = IoRead8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_MSR));
    if ((StatusRegister & MSR_RQM) == MSR_RQM && (StatusRegister & MSR_DIO) == DataInOut) {
      break;
      //
      // FDC is ready
      //
    }

    MicroSecondDelay (50);
  } while (--Delay);

  if (Delay == 0) {
    return EFI_NOT_READY;
    //
    // FDC is not ready within the specified time period
    //
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MotorOn (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  Info        - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  //
  // EFI_STATUS  Status;
  //
  UINT8 data;
  UINT8 DevPos;

  //
  // Control of the floppy drive motors is a big pain. If motor is off, you have to turn it
  // on first. But you can not leave the motor on all the time, since that would wear out the
  // disk. On the other hand, if you turn the motor off after each operation, the system performance
  // will be awful. The compromise used in this driver is to leave the motor on for 2 seconds after
  // each operation. If a new operation is started in that interval(2s), the motor need not be
  // turned on again. If no new operation is started, a timer goes off and the motor is turned off
  //
  DevPos = Info->DevPos;

  if (Info->MotorOn) {
    return EFI_SUCCESS;
  }
  //
  // The drive's motor is off, so need turn it on
  // first look at command and drive are busy or not
  //
  if (FdcWaitForBSYClear (FdcBlkIoDev, DevPos, 1) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // for drive A: 1CH, drive B: 2DH
  //
  data = 0x0C;
  data = (UINT8) (data | (SELECT_DRV & DevPos));
  if (DevPos == 0) {
    data |= DRVA_MOTOR_ON;
    //
    // FdcTimer[1].MotorOn = FALSE;
    // Info->MotorOn = FALSE;
    //
  } else {
    data |= DRVB_MOTOR_ON;
    //
    // FdcTimer[0].MotorOn = FALSE;
    // Info->MotorOn = FALSE;
    //
  }

  Info->MotorOn = FALSE;

  IoWrite8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DOR), data);

  MicroSecondDelay (4000);
  //
  // FdcTimer[DevPos].MotorOn = TRUE;
  //
  Info->MotorOn = TRUE;
  return EFI_SUCCESS;
}

EFI_STATUS
MotorOff (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  Info        - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINT8 data;
  UINT8 DevPos;

  DevPos = Info->DevPos;

  if (!Info->MotorOn) {
    return EFI_SUCCESS;
  }
  //
  // the motor is on, so need motor off
  //
  data = 0x0C;
  data = (UINT8) (data | (SELECT_DRV & DevPos));

  IoWrite8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DOR), data);
  MicroSecondDelay (50);
  //
  // FdcTimer[DevPos].MotorOn = FALSE;
  //
  Info->MotorOn = FALSE;

  return EFI_SUCCESS;
}

EFI_STATUS
DisketChanged (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  Info        - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_NO_MEDIA - GC_TODO: Add description for return value
  EFI_MEDIA_CHANGED - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  EFI_STATUS  Status;
  UINT8       data;

  //
  // Check change line
  //
  data = IoRead8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DIR));

  MicroSecondDelay (50);

  if ((data & DIR_DCL) == 0x80) {
    if (Info->Pcn != 0) {
      Status = Recalibrate (FdcBlkIoDev, Info);
    } else {
      Status = Seek (FdcBlkIoDev, Info, 0x30);
    }

    if (Status != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
      //
      // Fail to do the seek or recalibrate operation
      //
    }

    data = IoRead8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_DIR));

    MicroSecondDelay (50);

    if ((data & DIR_DCL) == 0x80) {
      return EFI_NO_MEDIA;
    }

    return EFI_MEDIA_CHANGED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
Recalibrate (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  Info        - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  FDC_COMMAND_PACKET2 Command;
  UINTN               i;
  UINT8               sts0;
  UINT8               pcn;
  UINT8               *pt;
  UINT8               Count;
  UINT8               DevPos;

  Count   = 2;
  DevPos  = Info->DevPos;

  while (Count > 0) {
    ZeroMem (&Command, sizeof (FDC_COMMAND_PACKET2));
    Command.CommandCode = RECALIBRATE_CMD;
    //
    // drive select
    //
    if (DevPos == 0) {
      Command.DiskHeadSel = 0;
      //
      // 0
      //
    } else {
      Command.DiskHeadSel = 1;
      //
      // 1
      //
    }

    pt = (UINT8 *) (&Command);
    for (i = 0; i < sizeof (FDC_COMMAND_PACKET2); i++) {
      if (DataOutByte (FdcBlkIoDev, pt++) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }
    }

    MicroSecondDelay (250000);

    if (SenseIntStatus (FdcBlkIoDev, &sts0, &pcn) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }

    if ((sts0 & 0xf0) == 0x20 && pcn == 0) {
      //
      // FdcTimer[DevPos].Pcn = 0;
      //
      Info->Pcn = 0;
      //
      // FdcTimer[DevPos].NeedRecalibrate = FALSE;
      //
      Info->NeedRecalibrate = FALSE;
      return EFI_SUCCESS;
    } else {
      Count--;
      if (Count == 0) {
        return EFI_DEVICE_ERROR;
      }
    }
  }
  //
  // end while
  //
  return EFI_SUCCESS;
}

EFI_STATUS
Seek (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info,
  IN     EFI_PEI_LBA            Lba
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  Info        - GC_TODO: add argument description
  Lba         - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value

--*/
{
  FDC_SEEK_CMD      Command;
  DISKET_PARA_TABLE *Para;
  UINT8             EndOfTrack;
  UINT8             Head;
  UINT8             Cylinder;
  UINT8             sts0;
  UINT8             *pt;
  UINT8             pcn;
  UINTN             i;
  UINT8             x;
  UINT8             DevPos;

  DevPos = Info->DevPos;
  if (Info->NeedRecalibrate) {
    if (Recalibrate (FdcBlkIoDev, Info) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    //
    // Recalibrate Success
    //
    Info->NeedRecalibrate = FALSE;
  }

  Para        = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);
  EndOfTrack  = Para->EOT;
  //
  // Calculate cylinder based on Lba and EOT
  //
  Cylinder = (UINT8) ((UINTN) Lba / EndOfTrack / 2);

  //
  // if the dest cylinder is the present cylinder, unnecessary to do the seek operation
  //
  if (Info->Pcn == Cylinder) {
    return EFI_SUCCESS;
  }
  //
  // Calculate the head : 0 or 1
  //
  Head = (UINT8) ((UINTN) Lba / EndOfTrack % 2);

  ZeroMem (&Command, sizeof (FDC_SEEK_CMD));
  Command.CommandCode = SEEK_CMD;
  if (DevPos == 0) {
    Command.DiskHeadSel = 0;
    //
    // 0
    //
  } else {
    Command.DiskHeadSel = 1;
    //
    // 1
    //
  }

  Command.DiskHeadSel = (UINT8) (Command.DiskHeadSel | (Head << 2));
  Command.NewCylinder = Cylinder;

  pt                  = (UINT8 *) (&Command);
  for (i = 0; i < sizeof (FDC_SEEK_CMD); i++) {
    if (DataOutByte (FdcBlkIoDev, pt++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  MicroSecondDelay (50);

  //
  // Calculate waiting time
  //
  if (Info->Pcn > Cylinder) {
    x = (UINT8) (Info->Pcn - Cylinder);
  } else {
    x = (UINT8) (Cylinder - Info->Pcn);
  }

  MicroSecondDelay ((x + 1) * 4000);

  if (SenseIntStatus (FdcBlkIoDev, &sts0, &pcn) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((sts0 & 0xf0) == 0x20) {
    Info->Pcn             = Command.NewCylinder;
    Info->NeedRecalibrate = FALSE;
    return EFI_SUCCESS;
  } else {
    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
}

UINTN
GetTransferBlockCount (
  IN  PEI_FLOPPY_DEVICE_INFO *Info,
  IN  EFI_PEI_LBA            LBA,
  IN  UINTN                  NumberOfBlocks
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Info            - GC_TODO: add argument description
  LBA             - GC_TODO: add argument description
  NumberOfBlocks  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  DISKET_PARA_TABLE *Para;
  UINT8             EndOfTrack;
  UINT8             Head;
  UINT8             SectorsInTrack;

  Para            = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);
  EndOfTrack      = Para->EOT;
  Head            = (UINT8) ((UINTN) LBA / EndOfTrack % 2);

  SectorsInTrack  = (UINT8) (EndOfTrack * (2 - Head) - (UINT8) ((UINTN) LBA % EndOfTrack));
  if (SectorsInTrack < NumberOfBlocks) {
    return SectorsInTrack;
  } else {
    return NumberOfBlocks;
  }
}

EFI_STATUS
ReadWriteDataSector (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info,
  IN     VOID                   *Buffer,
  IN     EFI_PEI_LBA            Lba,
  IN     UINTN                  NumberOfBlocks,
  IN     BOOLEAN                Read
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev     - GC_TODO: add argument description
  Info            - GC_TODO: add argument description
  Buffer          - GC_TODO: add argument description
  Lba             - GC_TODO: add argument description
  NumberOfBlocks  - GC_TODO: add argument description
  Read            - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_TIMEOUT - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value

--*/
{
  EFI_STATUS          Status;
  FDC_COMMAND_PACKET1 Command;
  FDC_RESULT_PACKET   Result;
  UINTN               i;
  UINTN               Times;
  UINT8               *pt;
  //
  // UINT8                   Temp;
  //
  Status = Seek (FdcBlkIoDev, Info, Lba);
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Set up DMA
  //
  SetDMA (FdcBlkIoDev, Buffer, NumberOfBlocks, Read);

  //
  // Allocate Read or Write command packet
  //
  ZeroMem (&Command, sizeof (FDC_COMMAND_PACKET1));
  if (Read == READ) {
    Command.CommandCode = READ_DATA_CMD | CMD_MT | CMD_MFM | CMD_SK;
  }
  //
  // else
  // Command.CommandCode = WRITE_DATA_CMD | CMD_MT | CMD_MFM;
  //
  FillPara (Info, Lba, &Command);

  //
  // Write command bytes to FDC
  //
  pt = (UINT8 *) (&Command);
  for (i = 0; i < sizeof (FDC_COMMAND_PACKET1); i++) {
    if (DataOutByte (FdcBlkIoDev, pt++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // wait for some time
  //
  Times = (STALL_1_SECOND / 50) + 1;
  do {
    if ((IoRead8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_MSR)) & 0xc0) == 0xc0) {
      break;
    }

    MicroSecondDelay (50);
  } while (--Times);

  if (Times == 0) {
    return EFI_TIMEOUT;
  }
  //
  // Read result bytes from FDC
  //
  pt = (UINT8 *) (&Result);
  for (i = 0; i < sizeof (FDC_RESULT_PACKET); i++) {
    if (DataInByte (FdcBlkIoDev, pt++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  return CheckResult (&Result, Info);
}

EFI_STATUS
CheckResult (
  IN  FDC_RESULT_PACKET         *Result,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Result  - GC_TODO: add argument description
  Info    - GC_TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  if ((Result->Status0 & STS0_IC) != IC_NT) {
    if ((Result->Status0 & STS0_SE) == 0x20) {
      //
      // seek error
      //
      Info->NeedRecalibrate = TRUE;
    }

    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
  //
  // Check Status Register1
  //
  if (Result->Status1 & (STS1_EN | STS1_DE | STS1_OR | STS1_ND | STS1_NW | STS1_MA)) {
    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
  //
  // Check Status Register2
  //
  if (Result->Status2 & (STS2_CM | STS2_DD | STS2_WC | STS2_BC | STS2_MD)) {
    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

VOID
FillPara (
  IN  PEI_FLOPPY_DEVICE_INFO *Info,
  IN  EFI_PEI_LBA            Lba,
  IN  FDC_COMMAND_PACKET1    *Command
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Info    - GC_TODO: add argument description
  Lba     - GC_TODO: add argument description
  Command - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  DISKET_PARA_TABLE *Para;
  UINT8             EndOfTrack;
  UINT8             DevPos;

  DevPos      = Info->DevPos;
  Para        = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);
  EndOfTrack  = Para->EOT;

  if (DevPos == 0) {
    Command->DiskHeadSel = 0;
  } else {
    Command->DiskHeadSel = 1;
  }

  Command->Cylinder = (UINT8) ((UINTN) Lba / EndOfTrack / 2);
  Command->Head     = (UINT8) ((UINTN) Lba / EndOfTrack % 2);
  Command->Sector   = (UINT8) ((UINT8) ((UINTN) Lba % EndOfTrack) + 1);
  Command->DiskHeadSel = (UINT8) (Command->DiskHeadSel | (Command->Head << 2));
  Command->Number     = Para->Number;
  Command->EndOfTrack = Para->EOT;
  Command->GapLength  = Para->GPL;
  Command->DataLength = Para->DTL;
}

EFI_STATUS
Setup (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN  UINT8           DevPos
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev - GC_TODO: add argument description
  DevPos      - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  IoWrite8 ((UINT16) (FdcBaseAddress + FDC_REGISTER_CCR), 0x0);

  MicroSecondDelay (100);

  Specify (FdcBlkIoDev);
  return EFI_SUCCESS;
}

EFI_STATUS
SetDMA (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN VOID             *Buffer,
  IN UINTN            NumberOfBlocks,
  IN BOOLEAN          Read
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcBlkIoDev     - GC_TODO: add argument description
  Buffer          - GC_TODO: add argument description
  NumberOfBlocks  - GC_TODO: add argument description
  Read            - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  UINT8 data;
  UINTN count;

  //
  // mask DMA channel 2;
  //
  IoWrite8 (R_8237_DMA_WRSMSK_CH0_3, B_8237_DMA_WRSMSK_CMS | 2);

  //
  // clear first/last flip flop
  //
  IoWrite8 (R_8237_DMA_CBPR_CH0_3, B_8237_DMA_WRSMSK_CMS | 2);

  //
  // set mode
  //
  if (Read == READ) {
    IoWrite8 (R_8237_DMA_CHMODE_CH0_3, V_8237_DMA_CHMODE_SINGLE | V_8237_DMA_CHMODE_IO2MEM | 2);
  } else {
    IoWrite8 (R_8237_DMA_CHMODE_CH0_3, V_8237_DMA_CHMODE_SINGLE | V_8237_DMA_CHMODE_MEM2IO | 2);
  }
  //
  // set base address and page register
  //
  data = (UINT8) (UINTN) Buffer;
  IoWrite8 (R_8237_DMA_BASE_CA_CH2, data);
  data = (UINT8) ((UINTN) Buffer >> 8);
  IoWrite8 (R_8237_DMA_BASE_CA_CH2, data);

  data = (UINT8) ((UINTN) Buffer >> 16);
  IoWrite8 (R_8237_DMA_MEM_LP_CH2, data);

  //
  // set count register
  //
  count = 512 * NumberOfBlocks - 1;
  data  = (UINT8) (count & 0xff);
  IoWrite8 (R_8237_DMA_BASE_CC_CH2, data);
  data = (UINT8) (count >> 8);
  IoWrite8 (R_8237_DMA_BASE_CC_CH2, data);

  //
  // clear channel 2 mask
  //
  IoWrite8 (R_8237_DMA_WRSMSK_CH0_3, 0x02);

  return EFI_SUCCESS;
}


/*++

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved. 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

    FloppyPeim.h
    
Abstract: 
    

Revision History
--*/

#ifndef _RECOVERY_FLOPPY_H
#define _RECOVERY_FLOPPY_H

//
// The package level header files this module uses
//
#include <PiPei.h>
#include <FrameworkPei.h>

#include <Ppi/BlockIo.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>

#include "Fdc.h"
//
// define some macro
//
#define STALL_1_SECOND  1000000
#define STALL_1_MSECOND 1000

#define DATA_IN         1
#define DATA_OUT        0
#define READ            0
#define WRITE           1

typedef enum {
  _360K_360K  = 0,
  _360K_1200K,
  _1200K_1200K,
  _720K_720K,
  _720K_1440K,
  _1440K_1440K,
  _720K_2880K,
  _1440K_2880K,
  _2880K_2880K
} FDC_DISKET_TYPE;

typedef struct {
  UINT8                      DevPos;
  UINT8                      Pcn;
  BOOLEAN                    MotorOn;
  BOOLEAN                    NeedRecalibrate;
  FDC_DISKET_TYPE            Type;
  EFI_PEI_BLOCK_IO_MEDIA     MediaInfo;
} PEI_FLOPPY_DEVICE_INFO;

#define FDC_BLK_IO_DEV_SIGNATURE  EFI_SIGNATURE_32 ('F', 'b', 'i', 'o')

typedef struct {
  UINTN                           Signature;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI   FdcBlkIo;
  EFI_PEI_PPI_DESCRIPTOR          PpiDescriptor;
  UINTN                           DeviceCount;
  PEI_FLOPPY_DEVICE_INFO          DeviceInfo[2];
} FDC_BLK_IO_DEV;

#define PEI_RECOVERY_FDC_FROM_BLKIO_THIS(a) CR (a, FDC_BLK_IO_DEV, FdcBlkIo, FDC_BLK_IO_DEV_SIGNATURE)

//
// PEI Recovery Block I/O PPI
//
EFI_STATUS
EFIAPI
FdcGetNumberOfBlockDevices (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  OUT  UINTN                             *NumberBlockDevices
  );

EFI_STATUS
EFIAPI
FdcGetBlockDeviceMediaInfo (
  IN   EFI_PEI_SERVICES                     **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI        *This,
  IN   UINTN                                DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO_MEDIA               *MediaInfo
  );

EFI_STATUS
EFIAPI
FdcReadBlocks (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  IN   UINTN                             DeviceIndex,
  IN   EFI_PEI_LBA                       StartLba,
  IN   UINTN                             BufferSize,
  OUT  VOID                              *Buffer
  );

//
// Internal function declare
//
UINT8
FdcEnumeration (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev
  );

EFI_STATUS
FdcReset (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            DevPos
  );

BOOLEAN
DiscoverFdcDevice (
  IN  FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN  OUT PEI_FLOPPY_DEVICE_INFO *Info,
  OUT EFI_PEI_BLOCK_IO_MEDIA     *MediaInfo
  );

EFI_STATUS
Recalibrate (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  );

EFI_STATUS
Seek (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info,
  IN EFI_PEI_LBA                Lba
  );

EFI_STATUS
MotorOn (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  );

EFI_STATUS
MotorOff (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  );

EFI_STATUS
FdcWaitForBSYClear (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN  UINT8           DevPos,
  IN  UINTN           TimeoutInSeconds
  );

EFI_STATUS
SenseIntStatus (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN OUT UINT8        *sts0,
  IN OUT UINT8        *pcn
  );

EFI_STATUS
Specify (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev
  );

EFI_STATUS
DisketChanged (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  );

EFI_STATUS
DataInByte (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN OUT UINT8        *pt
  );

EFI_STATUS
DataOutByte (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            *pt
  );

EFI_STATUS
FdcDRQReady (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN BOOLEAN          Dio,
  IN UINTN            TimeoutInSeconds
  );

UINTN
GetTransferBlockCount (
  IN PEI_FLOPPY_DEVICE_INFO *Info,
  IN EFI_PEI_LBA            LBA,
  IN UINTN                  NumberOfBlocks
  );

EFI_STATUS
ReadWriteDataSector (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info,
  IN VOID                       *Buffer,
  IN EFI_PEI_LBA                Lba,
  IN UINTN                      NumberOfBlocks,
  IN BOOLEAN                    Read
  );

EFI_STATUS
SetDMA (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN VOID             *Buffer,
  IN UINTN            NumberOfBlocks,
  IN BOOLEAN          Read
  );

VOID
FillPara (
  IN  PEI_FLOPPY_DEVICE_INFO *Info,
  IN  EFI_PEI_LBA            Lba,
  IN  FDC_COMMAND_PACKET1    *Command
  );

EFI_STATUS
Setup (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            DevPos
  );

EFI_STATUS
CheckResult (
  IN FDC_RESULT_PACKET          *Result,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  );

#endif

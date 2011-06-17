/** @file
Private include file for IsaFloppyPei PEIM.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _RECOVERY_FLOPPY_H_
#define _RECOVERY_FLOPPY_H_

#include <Ppi/BlockIo.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "Fdc.h"


//
// Some PC AT Compatible Device definitions
//
//
// 8237 DMA registers
//
#define R_8237_DMA_BASE_CA_CH0                    0x00
#define R_8237_DMA_BASE_CA_CH1                    0x02
#define R_8237_DMA_BASE_CA_CH2                    0x04
#define R_8237_DMA_BASE_CA_CH3                    0xd6
#define R_8237_DMA_BASE_CA_CH5                    0xc4
#define R_8237_DMA_BASE_CA_CH6                    0xc8
#define R_8237_DMA_BASE_CA_CH7                    0xcc

#define R_8237_DMA_BASE_CC_CH0                    0x01
#define R_8237_DMA_BASE_CC_CH1                    0x03
#define R_8237_DMA_BASE_CC_CH2                    0x05
#define R_8237_DMA_BASE_CC_CH3                    0xd7
#define R_8237_DMA_BASE_CC_CH5                    0xc6
#define R_8237_DMA_BASE_CC_CH6                    0xca
#define R_8237_DMA_BASE_CC_CH7                    0xce

#define R_8237_DMA_MEM_LP_CH0                     0x87
#define R_8237_DMA_MEM_LP_CH1                     0x83
#define R_8237_DMA_MEM_LP_CH2                     0x81
#define R_8237_DMA_MEM_LP_CH3                     0x82
#define R_8237_DMA_MEM_LP_CH5                     0x8B
#define R_8237_DMA_MEM_LP_CH6                     0x89
#define R_8237_DMA_MEM_LP_CH7                     0x8A


#define R_8237_DMA_COMMAND_CH0_3                  0x08
#define R_8237_DMA_COMMAND_CH4_7                  0xd0
#define   B_8237_DMA_COMMAND_GAP                  0x10
#define   B_8237_DMA_COMMAND_CGE                  0x04


#define R_8237_DMA_STA_CH0_3                      0x09
#define R_8237_DMA_STA_CH4_7                      0xd2

#define R_8237_DMA_WRSMSK_CH0_3                   0x0a
#define R_8237_DMA_WRSMSK_CH4_7                   0xd4
#define   B_8237_DMA_WRSMSK_CMS                   0x04


#define R_8237_DMA_CHMODE_CH0_3                   0x0b
#define R_8237_DMA_CHMODE_CH4_7                   0xd6
#define   V_8237_DMA_CHMODE_DEMAND                0x00
#define   V_8237_DMA_CHMODE_SINGLE                0x40
#define   V_8237_DMA_CHMODE_CASCADE               0xc0
#define   B_8237_DMA_CHMODE_DECREMENT             0x20
#define   B_8237_DMA_CHMODE_INCREMENT             0x00
#define   B_8237_DMA_CHMODE_AE                    0x10
#define   V_8237_DMA_CHMODE_VERIFY                0
#define   V_8237_DMA_CHMODE_IO2MEM                0x04
#define   V_8237_DMA_CHMODE_MEM2IO                0x08

#define R_8237_DMA_CBPR_CH0_3                     0x0c
#define R_8237_DMA_CBPR_CH4_7                     0xd8

#define R_8237_DMA_MCR_CH0_3                      0x0d
#define R_8237_DMA_MCR_CH4_7                      0xda

#define R_8237_DMA_CLMSK_CH0_3                    0x0e
#define R_8237_DMA_CLMSK_CH4_7                    0xdc

#define R_8237_DMA_WRMSK_CH0_3                    0x0f
#define R_8237_DMA_WRMSK_CH4_7                    0xde

///
/// ISA memory range
///
#define ISA_MAX_MEMORY_ADDRESS  0x1000000 

//
// Macro for time delay & interval
//
#define STALL_1_SECOND           1000000
#define STALL_1_MSECOND          1000
#define FDC_CHECK_INTERVAL       50

#define FDC_SHORT_DELAY          50
#define FDC_MEDIUM_DELAY         100
#define FDC_LONG_DELAY           4000
#define FDC_RESET_DELAY          2000
#define FDC_RECALIBRATE_DELAY    250000

typedef enum {
  FdcType360K360K  = 0,
  FdcType360K1200K,
  FdcType1200K1200K,
  FdcType720K720K,
  FdcType720K1440K,
  FdcType1440K1440K,
  FdcType720K2880K,
  FdcType1440K2880K,
  FdcType2880K2880K
} FDC_DISKET_TYPE;

typedef struct {
  UINT8 Register;
  UINT8 Value;
} PEI_DMA_TABLE;

typedef struct {
  UINT8                      DevPos;
  UINT8                      Pcn;
  BOOLEAN                    MotorOn;
  BOOLEAN                    NeedRecalibrate;
  FDC_DISKET_TYPE            Type;
  EFI_PEI_BLOCK_IO_MEDIA     MediaInfo;
} PEI_FLOPPY_DEVICE_INFO;

#define FDC_BLK_IO_DEV_SIGNATURE  SIGNATURE_32 ('F', 'b', 'i', 'o')

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

/**
  Get the number of FDC devices.

  This function implements EFI_PEI_RECOVERY_BLOCK_IO_PPI.GetNumberOfBlockDevices.
  It get the number of FDC devices in the system.

  @param  PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This                  Pointer to this PPI instance.
  @param  NumberBlockDevices    Pointer to the the number of FDC devices for output.

  @retval EFI_SUCCESS           Number of FDC devices is retrieved successfully.
  @retval EFI_INVALID_PARAMETER Parameter This is NULL.

**/
EFI_STATUS
EFIAPI
FdcGetNumberOfBlockDevices (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  OUT  UINTN                             *NumberBlockDevices
  );

/**
  Get the specified media information.

  This function implements EFI_PEI_RECOVERY_BLOCK_IO_PPI.GetBlockDeviceMediaInfo.
  It gets the specified media information.

  @param  PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This                  Pointer to this PPI instance.
  @param  DeviceIndex           Index of FDC device to get information.
  @param  MediaInfo             Pointer to the media info buffer for output.

  @retval EFI_SUCCESS           Number of FDC devices is retrieved successfully.
  @retval EFI_INVALID_PARAMETER Parameter This is NULL.
  @retval EFI_INVALID_PARAMETER Parameter MediaInfo is NULL.
  @retval EFI_INVALID_PARAMETER DeviceIndex is not valid.
  @retval EFI_DEVICE_ERROR      FDC device does not exist or has errors.

**/
EFI_STATUS
EFIAPI
FdcGetBlockDeviceMediaInfo (
  IN   EFI_PEI_SERVICES                     **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI        *This,
  IN   UINTN                                DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO_MEDIA               *MediaInfo
  );

/**
  Get the requested number of blocks from the specified FDC device.

  This function implements EFI_PEI_RECOVERY_BLOCK_IO_PPI.ReadBlocks.
  It reads the requested number of blocks from the specified FDC device.

  @param  PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This                  Pointer to this PPI instance.
  @param  DeviceIndex           Index of FDC device to get information.
  @param  StartLba              The start LBA to read from.
  @param  BufferSize            The size of range to read.
  @param  Buffer                Buffer to hold the data read from FDC.

  @retval EFI_SUCCESS           Number of FDC devices is retrieved successfully.
  @retval EFI_INVALID_PARAMETER Parameter This is NULL.
  @retval EFI_INVALID_PARAMETER Parameter Buffer is NULL.
  @retval EFI_INVALID_PARAMETER Parameter BufferSize cannot be divided by block size of FDC device.
  @retval EFI_NO_MEDIA          No media present.
  @retval EFI_DEVICE_ERROR      FDC device has error.
  @retval Others                Fail to read blocks.

**/
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

#endif

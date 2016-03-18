/** @file
  Include file for ISA Floppy Driver
  
Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISA_FLOPPY_H_
#define _ISA_FLOPPY_H_

#include <Uefi.h>

#include <Protocol/BlockIo.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>
#include <Guid/StatusCodeDataTypeId.h>

#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>

extern EFI_DRIVER_BINDING_PROTOCOL  gFdcControllerDriver;

#define STALL_1_SECOND  1000000
#define STALL_1_MSECOND 1000

#define DATA_IN         1
#define DATA_OUT        0
#define READ            0
#define WRITE           1

//
// Internal Data Structures
//
#define FDC_BLK_IO_DEV_SIGNATURE            SIGNATURE_32 ('F', 'B', 'I', 'O')
#define FLOPPY_CONTROLLER_CONTEXT_SIGNATURE SIGNATURE_32 ('F', 'D', 'C', 'C')

typedef enum {
  FdcDisk0   = 0,
  FdcDisk1   = 1,
  FdcMaxDisk = 2
} EFI_FDC_DISK;

typedef struct {
  UINT32          Signature;
  LIST_ENTRY      Link;
  BOOLEAN         FddResetPerformed;
  EFI_STATUS      FddResetStatus;
  BOOLEAN         NeedRecalibrate;
  UINT8           NumberOfDrive;
  UINT16          BaseAddress;
} FLOPPY_CONTROLLER_CONTEXT;

typedef struct {
  UINTN                                     Signature;
  EFI_HANDLE                                Handle;
  EFI_BLOCK_IO_PROTOCOL                     BlkIo;
  EFI_BLOCK_IO_MEDIA                        BlkMedia;

  EFI_ISA_IO_PROTOCOL                       *IsaIo;

  UINT16                                    BaseAddress;

  EFI_FDC_DISK                              Disk;
  UINT8                                     PresentCylinderNumber;
  UINT8                                     *Cache;

  EFI_EVENT                                 Event;
  EFI_UNICODE_STRING_TABLE                  *ControllerNameTable;
  FLOPPY_CONTROLLER_CONTEXT                 *ControllerState;

  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath;
} FDC_BLK_IO_DEV;

#include "ComponentName.h"

#define FDD_BLK_IO_FROM_THIS(a) CR (a, FDC_BLK_IO_DEV, BlkIo, FDC_BLK_IO_DEV_SIGNATURE)
#define FLOPPY_CONTROLLER_FROM_LIST_ENTRY(a) \
          CR (a, \
              FLOPPY_CONTROLLER_CONTEXT, \
              Link, \
              FLOPPY_CONTROLLER_CONTEXT_SIGNATURE \
              )

#define DISK_1440K_EOT            0x12
#define DISK_1440K_GPL            0x1b
#define DISK_1440K_DTL            0xff
#define DISK_1440K_NUMBER         0x02
#define DISK_1440K_MAXTRACKNUM    0x4f
#define DISK_1440K_BYTEPERSECTOR  512

typedef struct {
  UINT8 CommandCode;
  UINT8 DiskHeadSel;
  UINT8 Cylinder;
  UINT8 Head;
  UINT8 Sector;
  UINT8 Number;
  UINT8 EndOfTrack;
  UINT8 GapLength;
  UINT8 DataLength;
} FDD_COMMAND_PACKET1;

typedef struct {
  UINT8 CommandCode;
  UINT8 DiskHeadSel;
} FDD_COMMAND_PACKET2;

typedef struct {
  UINT8 CommandCode;
  UINT8 SrtHut;
  UINT8 HltNd;
} FDD_SPECIFY_CMD;

typedef struct {
  UINT8 CommandCode;
  UINT8 DiskHeadSel;
  UINT8 NewCylinder;
} FDD_SEEK_CMD;

typedef struct {
  UINT8 CommandCode;
  UINT8 DiskHeadSel;
  UINT8 Cylinder;
  UINT8 Head;
  UINT8 Sector;
  UINT8 EndOfTrack;
  UINT8 GapLength;
  UINT8 ScanTestPause;
} FDD_SCAN_CMD;

typedef struct {
  UINT8 Status0;
  UINT8 Status1;
  UINT8 Status2;
  UINT8 Cylinder;
  UINT8 Head;
  UINT8 Sector;
  UINT8 Number;
} FDD_RESULT_PACKET;

//
// FDC Registers
//
//
// Digital Output Register address offset
//
#define FDC_REGISTER_DOR  2

//
// Main Status Register address offset
//
#define FDC_REGISTER_MSR  4

//
// Data Register address offset
//
#define FDC_REGISTER_DTR  5

//
// Configuration Control Register(data rate select) address offset
//
#define FDC_REGISTER_CCR  7

//
// Digital Input Register(diskchange) address offset
//
#define FDC_REGISTER_DIR  7


//
// FDC Register Bit Definitions
//
//
// Digital Out Register(WO)
//
//
// Select Drive: 0=A 1=B
//
#define SELECT_DRV  BIT0

//
// Reset FDC
//
#define RESET_FDC BIT2

//
// Enable Int & DMA
//
#define INT_DMA_ENABLE  BIT3

//
// Turn On Drive A Motor
//
#define DRVA_MOTOR_ON BIT4

//
// Turn On Drive B Motor
//
#define DRVB_MOTOR_ON BIT5

//
// Main Status Register(RO)
//
//
// Drive A Busy
//
#define MSR_DAB BIT0

//
// Drive B Busy
//
#define MSR_DBB BIT1

//
// FDC Busy
//
#define MSR_CB  BIT4

//
// Non-DMA Mode
//
#define MSR_NDM BIT5

//
// Data Input/Output
//
#define MSR_DIO BIT6

//
// Request For Master
//
#define MSR_RQM BIT7

//
// Configuration Control Register(WO)
//
//
// Data Rate select
//
#define CCR_DRC (BIT0 | BIT1)

//
// Digital Input Register(RO)
//
//
// Disk change line
//
#define DIR_DCL BIT7
//
// #define CCR_DCL         BIT7      // Diskette change
//
// 500K
//
#define DRC_500KBS  0x0

//
// 300K
//
#define DRC_300KBS  0x01

//
// 250K
//
#define DRC_250KBS  0x02

//
// FDC Command Code
//
#define READ_DATA_CMD         0x06
#define WRITE_DATA_CMD        0x05
#define WRITE_DEL_DATA_CMD    0x09
#define READ_DEL_DATA_CMD     0x0C
#define READ_TRACK_CMD        0x02
#define READ_ID_CMD           0x0A
#define FORMAT_TRACK_CMD      0x0D
#define SCAN_EQU_CMD          0x11
#define SCAN_LOW_EQU_CMD      0x19
#define SCAN_HIGH_EQU_CMD     0x1D
#define SEEK_CMD              0x0F
#define RECALIBRATE_CMD       0x07
#define SENSE_INT_STATUS_CMD  0x08
#define SPECIFY_CMD           0x03
#define SENSE_DRV_STATUS_CMD  0x04

//
// CMD_MT: Multi_Track Selector
// when set , this flag selects the multi-track operating mode.
// In this mode, the FDC treats a complete cylinder under head0 and 1
// as a single track
//
#define CMD_MT  BIT7

//
// CMD_MFM: MFM/FM Mode Selector
// A one selects the double density(MFM) mode
// A zero selects single density (FM) mode
//
#define CMD_MFM BIT6

//
// CMD_SK: Skip Flag
// When set to 1, sectors containing a deleted data address mark will
// automatically be skipped during the execution of Read Data.
// When set to 0, the sector is read or written the same as the read and
// write commands.
//
#define CMD_SK  BIT5

//
// FDC Status Register Bit Definitions
//
//
// Status Register 0
//
//
// Interrupt Code
//
#define STS0_IC (BIT7 | BIT6)

//
// Seek End: the FDC completed a seek or recalibrate command
//
#define STS0_SE BIT5

//
// Equipment Check
//
#define STS0_EC BIT4

//
// Not Ready(unused), this bit is always 0
//
#define STS0_NR BIT3

//
// Head Address: the current head address
//
#define STS0_HA BIT2

//
// STS0_US1 & STS0_US0: Drive Select(the current selected drive)
//
//
// Unit Select1
//
#define STS0_US1  BIT1

//
// Unit Select0
//
#define STS0_US0  BIT0

//
// Status Register 1
//
//
// End of Cylinder
//
#define STS1_EN BIT7

//
// BIT6 is unused
//
//
// Data Error: The FDC detected a CRC error in either the ID field or
// data field of a sector
//
#define STS1_DE BIT5

//
// Overrun/Underrun: Becomes set if FDC does not receive CPU or DMA service
// within the required time interval
//
#define STS1_OR BIT4

//
// BIT3 is unused
//
//
// No data
//
#define STS1_ND BIT2

//
// Not Writable
//
#define STS1_NW BIT1

//
// Missing Address Mark
//
#define STS1_MA BIT0

//
// Control Mark
//
#define STS2_CM BIT6

//
// Data Error in Data Field: The FDC detected a CRC error in the data field
//
#define STS2_DD BIT5

//
// Wrong Cylinder: The track address from sector ID field is different from
// the track address maintained inside FDC
//
#define STS2_WC BIT4

//
// Bad Cylinder
//
#define STS2_BC BIT1

//
// Missing Address Mark in Data Field
//
#define STS2_MD BIT0

//
// Write Protected
//
#define STS3_WP BIT6

//
// Track 0
//
#define STS3_T0 BIT4

//
// Head Address
//
#define STS3_HD BIT2

//
// STS3_US1 & STS3_US0 : Drive Select
//
#define STS3_US1  BIT1
#define STS3_US0  BIT0

//
// Status Register 0 Interrupt Code Description
//
//
// Normal Termination of Command
//
#define IC_NT 0x0

//
// Abnormal Termination of Command
//
#define IC_AT 0x40

//
// Invalid Command
//
#define IC_IC 0x80

//
// Abnormal Termination caused by Polling
//
#define IC_ATRC 0xC0

//
// EFI Driver Binding Protocol Functions
//

/**
  Test controller is a floppy disk drive device
  
  @param[in] This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.  
  @param[in] Controller           The handle of the controller to test.
  @param[in] RemainingDevicePath  A pointer to the remaining portion of a device path.
  
  @retval EFI_SUCCESS             The device is supported by this driver.
  @retval EFI_ALREADY_STARTED     The device is already being managed by this driver.
  @retval EFI_ACCESS_DENIED       The device is already being managed by a different driver 
                                  or an application that requires exclusive access.
**/
EFI_STATUS
EFIAPI
FdcControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Start this driver on Controller.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.
**/
EFI_STATUS
EFIAPI
FdcControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must 
                                support a bus specific I/O protocol for the driver 
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.
**/
EFI_STATUS
EFIAPI
FdcControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

//
// EFI Block I/O Protocol Functions
//

/**
  Reset the Floppy Logic Drive, call the FddReset function.   
  
  @param This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @param ExtendedVerification BOOLEAN: Indicate that the driver may perform a more 
                    exhaustive verification operation of the device during 
                    reset, now this par is ignored in this driver          
  @retval  EFI_SUCCESS:      The Floppy Logic Drive is reset
  @retval  EFI_DEVICE_ERROR: The Floppy Logic Drive is not functioning correctly 
                      and can not be reset

**/
EFI_STATUS
EFIAPI
FdcReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  );

/**
  Flush block via fdd controller.
  
  @param  This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @return EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FddFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  );

/**
  Read the requested number of blocks from the device.   
  
  @param This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @param MediaId UINT32:    The media id that the read request is for    
  @param  Lba EFI_LBA:     The starting logic block address to read from on the device
  @param  BufferSize UINTN:  The size of the Buffer in bytes
  @param  Buffer VOID *:     A pointer to the destination buffer for the data
  
  @retval  EFI_SUCCESS:     The data was read correctly from the device
  @retval  EFI_DEVICE_ERROR:The device reported an error while attempting to perform
                     the read operation
  @retval  EFI_NO_MEDIA:    There is no media in the device
  @retval  EFI_MEDIA_CHANGED:   The MediaId is not for the current media
  @retval  EFI_BAD_BUFFER_SIZE: The BufferSize parameter is not a multiple of the 
                         intrinsic block size of the device
  @retval  EFI_INVALID_PARAMETER:The read request contains LBAs that are not valid, 
                          or the buffer is not on proper alignment 

**/
EFI_STATUS
EFIAPI
FddReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Write a specified number of blocks to the device.   
  
  @param  This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @param  MediaId UINT32:    The media id that the write request is for   
  @param  Lba EFI_LBA:     The starting logic block address to be written
  @param  BufferSize UINTN:  The size in bytes in Buffer
  @param  Buffer VOID *:     A pointer to the source buffer for the data
  
  @retval  EFI_SUCCESS:     The data were written correctly to the device
  @retval  EFI_WRITE_PROTECTED: The device can not be written to 
  @retval  EFI_NO_MEDIA:    There is no media in the device
  @retval  EFI_MEDIA_CHANGED:   The MediaId is not for the current media
  @retval  EFI_DEVICE_ERROR:  The device reported an error while attempting to perform 
                       the write operation 
  @retval  EFI_BAD_BUFFER_SIZE: The BufferSize parameter is not a multiple of the 
                         intrinsic block size of the device
  @retval  EFI_INVALID_PARAMETER:The write request contains LBAs that are not valid, 
                          or the buffer is not on proper alignment 
**/
EFI_STATUS
EFIAPI
FddWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  );

//
// Prototypes of internal functions
//
/**

  Detect the floppy drive is presented or not.
 
  @param  FdcDev FDC_BLK_IO_DEV * : A pointer to the Data Structure FDC_BLK_IO_DEV
  @retval EFI_SUCCESS    Drive is presented
  @retval EFI_NOT_FOUND  Drive is not presented

**/
EFI_STATUS
DiscoverFddDevice (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**

  Do recalibrate  and see the drive is presented or not.
  Set the media parameters.
  
  @param FdcDev FDC_BLK_IO_DEV * : A pointer to the Data Structure FDC_BLK_IO_DEV
  @return the drive is presented or not

**/
EFI_STATUS
FddIdentify (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**

  Reset the Floppy Logic Drive.
  
  @param  FdcDev FDC_BLK_IO_DEV * : A pointer to the Data Structure FDC_BLK_IO_DEV
  
  @retval EFI_SUCCESS:    The Floppy Logic Drive is reset
  @retval EFI_DEVICE_ERROR: The Floppy Logic Drive is not functioning correctly and
                      can not be reset

**/
EFI_STATUS
FddReset (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**

  Turn the drive's motor on.
  The drive's motor must be on before any command can be executed.
  
  @param  FdcDev FDC_BLK_IO_DEV * : A pointer to the Data Structure FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS:       Turn the drive's motor on successfully
  @retval  EFI_DEVICE_ERROR:    The drive is busy, so can not turn motor on
  @retval  EFI_INVALID_PARAMETER: Fail to Set timer(Cancel timer)

**/
EFI_STATUS
MotorOn (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**

  Set a Timer and when Timer goes off, turn the motor off.
  
  
  @param  FdcDev FDC_BLK_IO_DEV * : A pointer to the Data Structure FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS:       Set the Timer successfully
  @retval  EFI_INVALID_PARAMETER: Fail to Set the timer

**/
EFI_STATUS
MotorOff (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**
  Detect the disk in the drive is changed or not.
  
  
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS:    No disk media change
  @retval  EFI_DEVICE_ERROR: Fail to do the recalibrate or seek operation
  @retval  EFI_NO_MEDIA:   No disk in the drive
  @retval  EFI_MEDIA_CHANGED:  There is a new disk in the drive
**/
EFI_STATUS
DisketChanged (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**
  Do the Specify command, this command sets DMA operation
  and the initial values for each of the three internal
  times: HUT, SRT and HLT.
  
  @param FdcDev    Pointer to instance of FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS:    Execute the Specify command successfully
  @retval  EFI_DEVICE_ERROR: Fail to execute the command

**/
EFI_STATUS
Specify (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**
  Set the head of floppy drive to track 0.
 
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  @retval EFI_SUCCESS:    Execute the Recalibrate operation successfully
  @retval EFI_DEVICE_ERROR: Fail to execute the Recalibrate operation

**/
EFI_STATUS
Recalibrate (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**
  Set the head of floppy drive to the new cylinder.
  
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  @param  Lba EFI_LBA     : The logic block address want to seek
  
  @retval  EFI_SUCCESS:    Execute the Seek operation successfully
  @retval  EFI_DEVICE_ERROR: Fail to execute the Seek operation

**/
EFI_STATUS
Seek (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN EFI_LBA         Lba
  );

/**
  Do the Sense Interrupt Status command, this command resets the interrupt signal.
  
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  @param  StatusRegister0 UINT8 *: Be used to save Status Register 0 read from FDC
  @param  PresentCylinderNumber  UINT8 *: Be used to save present cylinder number
                                    read from FDC
  
  @retval  EFI_SUCCESS:    Execute the Sense Interrupt Status command successfully
  @retval  EFI_DEVICE_ERROR: Fail to execute the command

**/
EFI_STATUS
SenseIntStatus (
  IN     FDC_BLK_IO_DEV  *FdcDev,
  IN OUT UINT8           *StatusRegister0,
  IN OUT UINT8           *PresentCylinderNumber
  );

/**
  Do the Sense Drive Status command.
  
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  @param  Lba EFI_LBA     : Logic block address
  
  @retval  EFI_SUCCESS:    Execute the Sense Drive Status command successfully
  @retval  EFI_DEVICE_ERROR: Fail to execute the command
  @retval  EFI_WRITE_PROTECTED:The disk is write protected

**/
EFI_STATUS
SenseDrvStatus (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN EFI_LBA         Lba
  );

/**
  Update the disk media properties and if necessary reinstall Block I/O interface.
 
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS:    Do the operation successfully
  @retval  EFI_DEVICE_ERROR: Fail to the operation

**/
EFI_STATUS
DetectMedia (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**
  Set the data rate and so on.
 
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV

  @retval EFI_SUCCESS success to set the data rate
**/
EFI_STATUS
Setup (
  IN FDC_BLK_IO_DEV  *FdcDev
  );

/**
  Read or Write a number of blocks in the same cylinder.
 
  @param  FdcDev      A pointer to Data Structure FDC_BLK_IO_DEV
  @param  HostAddress device address 
  @param  Lba         The starting logic block address to read from on the device
  @param  NumberOfBlocks The number of block wanted to be read or write
  @param  Read        Operation type: read or write
  
  @retval EFI_SUCCESS Success operate

**/
EFI_STATUS
ReadWriteDataSector (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN VOID            *HostAddress,
  IN EFI_LBA         Lba,
  IN UINTN           NumberOfBlocks,
  IN BOOLEAN         Read
  );

/**
  Fill in FDD command's parameter.
  
  @param FdcDev   Pointer to instance of FDC_BLK_IO_DEV
  @param Lba      The starting logic block address to read from on the device
  @param Command  FDD command

**/
VOID
FillPara (
  IN FDC_BLK_IO_DEV       *FdcDev,
  IN EFI_LBA              Lba,
  IN FDD_COMMAND_PACKET1  *Command
  );

/**
  Read result byte from Data Register of FDC.
  
  @param FdcDev   Pointer to instance of FDC_BLK_IO_DEV
  @param Pointer  Buffer to store the byte read from FDC
  
  @retval EFI_SUCCESS       Read result byte from FDC successfully
  @retval EFI_DEVICE_ERROR  The FDC is not ready to be read

**/
EFI_STATUS
DataInByte (
  IN  FDC_BLK_IO_DEV  *FdcDev,
  OUT UINT8           *Pointer
  );

/**
  Write command byte to Data Register of FDC.
  
  @param FdcDev  Pointer to instance of FDC_BLK_IO_DEV
  @param Pointer Be used to save command byte written to FDC
  
  @retval  EFI_SUCCESS:    Write command byte to FDC successfully
  @retval  EFI_DEVICE_ERROR: The FDC is not ready to be written

**/
EFI_STATUS
DataOutByte (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINT8           *Pointer
  );

/**
  Detect the specified floppy logic drive is busy or not within a period of time.
  
  @param FdcDev           Indicate it is drive A or drive B
  @param Timeout          The time period for waiting
  
  @retval EFI_SUCCESS:  The drive and command are not busy
  @retval EFI_TIMEOUT:  The drive or command is still busy after a period time that
                        set by Timeout

**/
EFI_STATUS
FddWaitForBSYClear (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINTN           Timeout
  );

/**
  Determine whether FDC is ready to write or read.
  
  @param  FdcDev Pointer to instance of FDC_BLK_IO_DEV
  @param  Dio BOOLEAN:      Indicate the FDC is waiting to write or read
  @param  Timeout           The time period for waiting
  
  @retval EFI_SUCCESS:  FDC is ready to write or read
  @retval EFI_NOT_READY:  FDC is not ready within the specified time period

**/
EFI_STATUS
FddDRQReady (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN BOOLEAN         Dio,
  IN UINTN           Timeout
  );

/**
  Set FDC control structure's attribute according to result.

  @param Result  Point to result structure
  @param FdcDev  FDC control structure

  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value

**/
EFI_STATUS
CheckResult (
  IN     FDD_RESULT_PACKET  *Result,
  IN OUT FDC_BLK_IO_DEV     *FdcDev
  );

/**
  Check the drive status information.
  
  @param StatusRegister3  the value of Status Register 3
  
  @retval EFI_SUCCESS           The disk is not write protected
  @retval EFI_WRITE_PROTECTED:  The disk is write protected

**/
EFI_STATUS
CheckStatus3 (
  IN UINT8 StatusRegister3
  );

/**
  Calculate the number of block in the same cylinder according to Lba.
  
  @param FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  @param Lba EFI_LBA:      The starting logic block address
  @param NumberOfBlocks UINTN: The number of blocks
  
  @return The number of blocks in the same cylinder which the starting
        logic block address is Lba

**/
UINTN
GetTransferBlockCount (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN EFI_LBA         Lba,
  IN UINTN           NumberOfBlocks
  );

/**
  When the Timer(2s) off, turn the drive's motor off.
  
  @param Event EFI_EVENT: Event(the timer) whose notification function is being
                     invoked
  @param Context VOID *:  Pointer to the notification function's context

**/
VOID
EFIAPI
FddTimerProc (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Read I/O port for FDC.
 
  @param FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  @param Offset The offset address of port

**/
UINT8
FdcReadPort (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINT32          Offset
  );

/**
  Write I/O port for FDC.
 
  @param FdcDev FDC_BLK_IO_DEV *: A pointer to Data Structure FDC_BLK_IO_DEV
  @param Offset The offset address of port
  @param Data   Value written to port
  
**/
VOID
FdcWritePort (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINT32          Offset,
  IN UINT8           Data
  );

/**
  Read or Write a number of blocks to floppy device.

  @param This     Pointer to instance of EFI_BLOCK_IO_PROTOCOL
  @param MediaId  The media id of read/write request
  @param Lba      The starting logic block address to read from on the device
  @param BufferSize The size of the Buffer in bytes
  @param Operation   - GC_TODO: add argument description
  @param Buffer      - GC_TODO: add argument description

  @retval EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_NO_MEDIA - GC_TODO: Add description for return value
  @retval EFI_MEDIA_CHANGED - GC_TODO: Add description for return value
  @retval EFI_WRITE_PROTECTED - GC_TODO: Add description for return value
  @retval EFI_BAD_BUFFER_SIZE - GC_TODO: Add description for return value
  @retval EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  @retval EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value

**/
EFI_STATUS
FddReadWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  IN  BOOLEAN                Operation,
  OUT VOID                   *Buffer
  );

/**
  Common interface for free cache. 
  
  @param FdcDev  Pointer of FDC_BLK_IO_DEV instance
  
**/
VOID
FdcFreeCache (
  IN    FDC_BLK_IO_DEV  *FdcDev
  );

#endif


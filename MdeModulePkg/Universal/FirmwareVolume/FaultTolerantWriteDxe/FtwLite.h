/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by FtwLite module.

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/

#ifndef _EFI_FAULT_TOLERANT_WRITE_LITE_H_
#define _EFI_FAULT_TOLERANT_WRITE_LITE_H_


#include <PiDxe.h>

#include <Protocol/PciRootBridgeIo.h>
#include <Guid/SystemNvDataGuid.h>
#include <Protocol/FaultTolerantWriteLite.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <WorkingBlockHeader.h>

#define EFI_D_FTW_LITE  EFI_D_ERROR
#define EFI_D_FTW_INFO  EFI_D_INFO

//
// Flash erase polarity is 1
//
#define FTW_ERASE_POLARITY  1

#define FTW_VALID_STATE     0
#define FTW_INVALID_STATE   1

#define FTW_ERASED_BYTE     ((UINT8) (255))
#define FTW_POLARITY_REVERT ((UINT8) (255))

typedef struct {
  UINT8         WriteAllocated : 1;
  UINT8         SpareCompleted : 1;
  UINT8         WriteCompleted : 1;
  UINT8         Reserved : 5;
#define WRITE_ALLOCATED 0x1
#define SPARE_COMPLETED 0x2
#define WRITE_COMPLETED 0x4

  EFI_DEV_PATH  DevPath;
  EFI_LBA       Lba;
  UINTN         Offset;
  UINTN         NumBytes;
  //
  // UINTN           SpareAreaOffset;
  //
} EFI_FTW_LITE_RECORD;

#define FTW_LITE_DEVICE_SIGNATURE SIGNATURE_32 ('F', 'T', 'W', 'L')

//
// MACRO for Block size.
// Flash Erasing will do in block granularity.
//
#ifdef FV_BLOCK_SIZE
#define FTW_BLOCK_SIZE  FV_BLOCK_SIZE
#else
#define FV_BLOCK_SIZE   0x10000
#define FTW_BLOCK_SIZE  FV_BLOCK_SIZE
#endif
//
// MACRO for FTW WORK SPACE Base & Size
//
#ifdef EFI_FTW_WORKING_OFFSET
#define FTW_WORK_SPACE_BASE EFI_FTW_WORKING_OFFSET
#else
#define FTW_WORK_SPACE_BASE 0x00E000
#endif

#ifdef EFI_FTW_WORKING_LENGTH
#define FTW_WORK_SPACE_SIZE EFI_FTW_WORKING_LENGTH
#else
#define FTW_WORK_SPACE_SIZE 0x002000
#endif
//
// MACRO for FTW header and record
//
#define FTW_WORKING_QUEUE_SIZE  (FTW_WORK_SPACE_SIZE - sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER))
#define FTW_LITE_RECORD_SIZE    (sizeof (EFI_FTW_LITE_RECORD))
#define WRITE_TOTAL_SIZE        FTW_LITE_RECORD_SIZE

//
// EFI Fault tolerant protocol private data structure
//
typedef struct {
  UINTN                                   Signature;
  EFI_HANDLE                              Handle;
  EFI_FTW_LITE_PROTOCOL                   FtwLiteInstance;
  EFI_PHYSICAL_ADDRESS                    WorkSpaceAddress;
  UINTN                                   WorkSpaceLength;
  EFI_PHYSICAL_ADDRESS                    SpareAreaAddress;
  UINTN                                   SpareAreaLength;
  UINTN                                   NumberOfSpareBlock; // Number of the blocks in spare block
  UINTN                                   SizeOfSpareBlock;   // Block size in bytes of the blocks in spare block
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *FtwWorkSpaceHeader;
  EFI_FTW_LITE_RECORD                     *FtwLastRecord;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *FtwFvBlock;        // FVB of working block
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *FtwBackupFvb;      // FVB of spare block
  EFI_LBA                                 FtwSpareLba;
  EFI_LBA                                 FtwWorkBlockLba;    // Start LBA of working block
  EFI_LBA                                 FtwWorkSpaceLba;    // Start LBA of working space
  UINTN                                   FtwWorkSpaceBase;   // Offset from LBA start addr
  UINTN                                   FtwWorkSpaceSize;
  UINT8                                   *FtwWorkSpace;
  //
  // Following a buffer of FtwWorkSpace[FTW_WORK_SPACE_SIZE],
  // Allocated with EFI_FTW_LITE_DEVICE.
  //
} EFI_FTW_LITE_DEVICE;

#define FTW_LITE_CONTEXT_FROM_THIS(a) CR (a, EFI_FTW_LITE_DEVICE, FtwLiteInstance, FTW_LITE_DEVICE_SIGNATURE)

//
// Driver entry point
//
/**
  This function is the entry point of the Fault Tolerant Write driver.


  @param ImageHandle     EFI_HANDLE: A handle for the image that is initializing
                         this driver
  @param SystemTable     EFI_SYSTEM_TABLE: A pointer to the EFI system table

  @retval  EFI_SUCCESS            FTW has finished the initialization
  @retval  EFI_ABORTED            FTW initialization error

**/
EFI_STATUS
EFIAPI
InitializeFtwLite (
  IN EFI_HANDLE                 ImageHandle,
  IN EFI_SYSTEM_TABLE           *SystemTable
  );

//
// Fault Tolerant Write Protocol API
//
/**
  Starts a target block update. This function will record data about write
  in fault tolerant storage and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.


  @param This            Calling context
  @param FvbHandle       The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param Lba             The logical block address of the target block.
  @param Offset          The offset within the target block to place the data.
  @param NumBytes        The number of bytes to write to the target block.
  @param Buffer          The data to write.

  @retval  EFI_SUCCESS           The function completed successfully
  @retval  EFI_BAD_BUFFER_SIZE   The write would span a target block, which is not
                                 a valid action.
  @retval  EFI_ACCESS_DENIED     No writes have been allocated.
  @retval  EFI_NOT_FOUND         Cannot find FVB by handle.
  @retval  EFI_OUT_OF_RESOURCES  Cannot allocate memory.
  @retval  EFI_ABORTED           The function could not complete successfully.

**/
EFI_STATUS
EFIAPI
FtwLiteWrite (
  IN EFI_FTW_LITE_PROTOCOL                      *This,
  IN EFI_HANDLE                                 FvbHandle,
  IN EFI_LBA                                    Lba,
  IN UINTN                                      Offset,
  IN OUT UINTN                                 *NumBytes,
  IN VOID                                       *Buffer
  );

//
// Internal functions
//
/**
  Restarts a previously interrupted write. The caller must provide the
  block protocol needed to complete the interrupted write.


  @param FtwLiteDevice   The private data of FTW_LITE driver
                         FvbHandle           - The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ACCESS_DENIED    No pending writes exist
  @retval  EFI_NOT_FOUND        FVB protocol not found by the handle
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwRestart (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice
  );

/**
  Aborts all previous allocated writes.


  @param FtwLiteDevice   The private data of FTW_LITE driver

  @retval  EFI_SUCCESS       The function completed successfully
  @retval  EFI_ABORTED       The function could not complete successfully.
  @retval  EFI_NOT_FOUND     No allocated writes exist.

**/
EFI_STATUS
FtwAbort (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice
  );


/**
  Write a record with fault tolerant mannaer.
  Since the content has already backuped in spare block, the write is
  guaranteed to be completed with fault tolerant manner.


  @param FtwLiteDevice   The private data of FTW_LITE driver
  @param Fvb             The FVB protocol that provides services for
                         reading, writing, and erasing the target block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwWriteRecord (
  IN EFI_FTW_LITE_DEVICE                   *FtwLiteDevice,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb
  );

/**
  To Erase one block. The size is FTW_BLOCK_SIZE


  @param FtwLiteDevice   Calling context
  @param FvBlock         FVB Protocol interface
  @param Lba             Lba of the firmware block

  @retval  EFI_SUCCESS    Block LBA is Erased successfully
  @retval  Others         Error occurs

**/
EFI_STATUS
FtwEraseBlock (
  IN EFI_FTW_LITE_DEVICE              *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  );

/**

  Erase spare block.


  @param FtwLiteDevice   Calling context

  @retval EFI_SUCCESS The erase request was successfully
                      completed.
  
  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              WriteDisabled state.
  @retval EFI_DEVICE_ERROR  The block device is not functioning
                            correctly and could not be written.
                            The firmware device may have been
                            partially erased.
  @retval EFI_INVALID_PARAMETER One or more of the LBAs listed
                                in the variable argument list do
                                not exist in the firmware volume.  

**/
EFI_STATUS
FtwEraseSpareBlock (
  IN EFI_FTW_LITE_DEVICE   *FtwLiteDevice
  );

/**
  Retrive the proper FVB protocol interface by HANDLE.


  @param FvBlockHandle   The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param FvBlock         The interface of FVB protocol

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
FtwGetFvbByHandle (
  IN EFI_HANDLE                           FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  );

/**

  Get firmware block by address.


  @param Address         Address specified the block
  @param FvBlock         The block caller wanted

  @retval  EFI_SUCCESS    The protocol instance if found.
  @retval  EFI_NOT_FOUND  Block not found

**/
EFI_STATUS
GetFvbByAddress (
  IN  EFI_PHYSICAL_ADDRESS               Address,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL **FvBlock
  );

/**

  Is it in working block?


  @param FtwLiteDevice   Calling context
  @param FvBlock         Fvb protocol instance
  @param Lba             The block specified

  @return A BOOLEAN value indicating in working block or not.

**/
BOOLEAN
IsInWorkingBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  );

/**

  Check whether the block is a boot block.


  @param FtwLiteDevice   Calling context
  @param FvBlock         Fvb protocol instance
  @param Lba             Lba value

  @retval FALSE           This is a boot block.
  @retval TRUE            This is not a boot block.

**/
BOOLEAN
IsBootBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  );

/**
  Copy the content of spare block to a target block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is
  FtwLiteDevice->FtwSpareLba.
  Target block is accessed by FvBlock protocol interface. LBA is Lba.


  @param FtwLiteDevice   The private data of FTW_LITE driver
  @param FvBlock         FVB Protocol interface to access target block
  @param Lba             Lba of the target block

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_INVALID_PARAMETER     Input parameter error
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToTargetBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  );

/**
  Copy the content of spare block to working block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is
  FtwLiteDevice->FtwSpareLba.
  Working block is accessed by FTW working FVB protocol interface. LBA is
  FtwLiteDevice->FtwWorkBlockLba.


  @param FtwLiteDevice   The private data of FTW_LITE driver

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully
                                     Notes:
                                     Since the working block header is important when FTW initializes, the
                                     state of the operation should be handled carefully. The Crc value is
                                     calculated without STATE element.

**/
EFI_STATUS
FlushSpareBlockToWorkingBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice
  );

/**
  Copy the content of spare block to a boot block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is
  FtwLiteDevice->FtwSpareLba.
  Boot block is accessed by BootFvb protocol interface. LBA is 0.


  @param FtwLiteDevice   The private data of FTW_LITE driver

  @retval  EFI_SUCCESS               Spare block content is copied to boot block
  @retval  EFI_INVALID_PARAMETER     Input parameter error
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully
                                     Notes:

**/
EFI_STATUS
FlushSpareBlockToBootBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice
  );

/**
  Update a bit of state on a block device. The location of the bit is
  calculated by the (Lba, Offset, bit). Here bit is determined by the
  the name of a certain bit.


  @param FvBlock         FVB Protocol interface to access SrcBlock and DestBlock
  @param Lba             Lba of a block
  @param Offset          Offset on the Lba
  @param NewBit          New value that will override the old value if it can be change

  @retval  EFI_SUCCESS    A state bit has been updated successfully
  @retval  Others         Access block device error.
                          Notes:
                          Assume all bits of State are inside the same BYTE.
  @retval  EFI_ABORTED    Read block fail

**/
EFI_STATUS
FtwUpdateFvState (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  IN EFI_LBA                             Lba,
  IN UINTN                               Offset,
  IN UINT8                               NewBit
  );

/**
  Get the last Write record pointer.
  The last record is the record whose 'complete' state hasn't been set.
  After all, this header may be a EMPTY header entry for next Allocate.


  @param FtwLiteDevice   Private data of this driver
  @param FtwLastRecord   Pointer to retrieve the last write record

  @retval  EFI_SUCCESS      Get the last write record successfully
  @retval  EFI_ABORTED      The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastRecord (
  IN  EFI_FTW_LITE_DEVICE  *FtwLiteDevice,
  OUT EFI_FTW_LITE_RECORD  **FtwLastRecord
  );

/**

  Check whether a flash buffer is erased.


  @param Polarity        All 1 or all 0
  @param Buffer          Buffer to check
  @param BufferSize      Size of the buffer

  @return A BOOLEAN value indicating erased or not.

**/
BOOLEAN
IsErasedFlashBuffer (
  IN BOOLEAN         Polarity,
  IN UINT8           *Buffer,
  IN UINTN           BufferSize
  );

/**
  Initialize a work space when there is no work space.


  @param WorkingHeader   Pointer of working block header

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
InitWorkSpaceHeader (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingHeader
  );

/**
  Read from working block to refresh the work space in memory.


  @param FtwLiteDevice   Point to private data of FTW driver

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
WorkSpaceRefresh (
  IN EFI_FTW_LITE_DEVICE  *FtwLiteDevice
  );

/**
  Check to see if it is a valid work space.


  @param WorkingHeader   Pointer of working block header

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
BOOLEAN
IsValidWorkSpace (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingHeader
  );

/**
  Reclaim the work space on the working block.


  @param FtwLiteDevice   Point to private data of FTW driver
  @param PreserveRecord  Whether to preserve the working record is needed

  @retval  EFI_SUCCESS            The function completed successfully
  @retval  EFI_OUT_OF_RESOURCES   Allocate memory error
  @retval  EFI_ABORTED            The function could not complete successfully

**/
EFI_STATUS
FtwReclaimWorkSpace (
  IN EFI_FTW_LITE_DEVICE  *FtwLiteDevice,
  IN BOOLEAN              PreserveRecord
  );

#endif

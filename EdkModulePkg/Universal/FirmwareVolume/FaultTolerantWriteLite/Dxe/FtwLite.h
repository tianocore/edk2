/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 


Module Name:

  FtwLite.h

Abstract:

  This is a simple fault tolerant write driver, based on PlatformFd library.
  And it only supports write BufferSize <= SpareAreaLength.

  This boot service only protocol provides fault tolerant write capability for 
  block devices.  The protocol has internal non-volatile intermediate storage 
  of the data and private information. It should be able to recover 
  automatically from a critical fault, such as power failure. 

--*/

#ifndef _EFI_FAULT_TOLERANT_WRITE_LITE_H_
#define _EFI_FAULT_TOLERANT_WRITE_LITE_H_

#include <Common/FlashMap.h>
#include <Common/WorkingBlockHeader.h>

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

#define FTW_LITE_DEVICE_SIGNATURE EFI_SIGNATURE_32 ('F', 'T', 'W', 'L')

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
EFI_STATUS
EFIAPI
InitializeFtwLite (
  IN EFI_HANDLE                 ImageHandle,
  IN EFI_SYSTEM_TABLE           *SystemTable
  )
/*++

Routine Description:
    This function is the entry point of the Fault Tolerant Write driver.

Arguments:
    ImageHandle   - EFI_HANDLE: A handle for the image that is initializing 
                    this driver
    SystemTable   - EFI_SYSTEM_TABLE: A pointer to the EFI system table

Returns:
    EFI_SUCCESS           - FTW has finished the initialization
    EFI_ABORTED           - FTW initialization error

--*/
;

//
// Fault Tolerant Write Protocol API
//
EFI_STATUS
EFIAPI
FtwLiteWrite (
  IN EFI_FTW_LITE_PROTOCOL                 *This,
  IN EFI_HANDLE                            FvbHandle,
  IN EFI_LBA                               Lba,
  IN UINTN                                 Offset,
  IN UINTN                                 *NumBytes,
  IN VOID                                  *Buffer
  )
/*++

Routine Description:
    Starts a target block update. This function will record data about write 
    in fault tolerant storage and will complete the write in a recoverable 
    manner, ensuring at all times that either the original contents or 
    the modified contents are available.

Arguments:
    This             - Calling context
    FvbHandle        - The handle of FVB protocol that provides services for 
                       reading, writing, and erasing the target block.
    Lba              - The logical block address of the target block.  
    Offset           - The offset within the target block to place the data.
    NumBytes         - The number of bytes to write to the target block.
    Buffer           - The data to write.

Returns:
    EFI_SUCCESS          - The function completed successfully
    EFI_BAD_BUFFER_SIZE  - The write would span a target block, which is not 
                           a valid action.
    EFI_ACCESS_DENIED    - No writes have been allocated.
    EFI_NOT_FOUND        - Cannot find FVB by handle.
    EFI_OUT_OF_RESOURCES - Cannot allocate memory.
    EFI_ABORTED          - The function could not complete successfully.

--*/
;

//
// Internal functions
//
EFI_STATUS
FtwRestart (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice
  )
/*++

Routine Description:
    Restarts a previously interrupted write. The caller must provide the 
    block protocol needed to complete the interrupted write.

Arguments:
    FtwLiteDevice       - The private data of FTW_LITE driver
    FvbHandle           - The handle of FVB protocol that provides services for 
                          reading, writing, and erasing the target block.

Returns:
    EFI_SUCCESS         - The function completed successfully
    EFI_ACCESS_DENIED   - No pending writes exist
    EFI_NOT_FOUND       - FVB protocol not found by the handle
    EFI_ABORTED         - The function could not complete successfully

--*/
;

EFI_STATUS
FtwAbort (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice
  )
/*++

Routine Description:
    Aborts all previous allocated writes.

Arguments:
    FtwLiteDevice    - The private data of FTW_LITE driver

Returns:
    EFI_SUCCESS      - The function completed successfully
    EFI_ABORTED      - The function could not complete successfully.
    EFI_NOT_FOUND    - No allocated writes exist.

--*/
;


EFI_STATUS
FtwWriteRecord (
  IN EFI_FTW_LITE_DEVICE                   *FtwLiteDevice,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb
  )
/*++

Routine Description:
    Write a record with fault tolerant mannaer.
    Since the content has already backuped in spare block, the write is 
    guaranteed to be completed with fault tolerant manner.

Arguments:
    FtwLiteDevice       - The private data of FTW_LITE driver
    Fvb                 - The FVB protocol that provides services for 
                          reading, writing, and erasing the target block.

Returns:
    EFI_SUCCESS         - The function completed successfully
    EFI_ABORTED         - The function could not complete successfully

--*/
;

EFI_STATUS
FtwEraseBlock (
  IN EFI_FTW_LITE_DEVICE              *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
/*++

Routine Description:
    To Erase one block. The size is FTW_BLOCK_SIZE

Arguments:
    FtwLiteDevice - Calling context
    FvBlock       - FVB Protocol interface
    Lba           - Lba of the firmware block

Returns:
    EFI_SUCCESS   - Block LBA is Erased successfully
    Others        - Error occurs

--*/
;

EFI_STATUS
FtwEraseSpareBlock (
  IN EFI_FTW_LITE_DEVICE   *FtwLiteDevice
  )
/*++

Routine Description:

  Erase spare block.

Arguments:

  FtwLiteDevice - Calling context

Returns:

  Status code

--*/
;

EFI_STATUS
FtwGetFvbByHandle (
  IN EFI_HANDLE                           FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
/*++

Routine Description:
    Retrive the proper FVB protocol interface by HANDLE.

Arguments:
    FvBlockHandle       - The handle of FVB protocol that provides services for 
                          reading, writing, and erasing the target block.
    FvBlock             - The interface of FVB protocol

Returns:
    EFI_SUCCESS         - The function completed successfully
    EFI_ABORTED         - The function could not complete successfully
--*/
;

EFI_STATUS
GetFvbByAddress (
  IN  EFI_PHYSICAL_ADDRESS               Address,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL **FvBlock
  )
/*++

Routine Description:

  Get firmware block by address.

Arguments:

  Address - Address specified the block
  FvBlock - The block caller wanted

Returns:

  Status code

  EFI_NOT_FOUND - Block not found

--*/
;

BOOLEAN
IsInWorkingBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
/*++

Routine Description:

  Is it in working block?

Arguments:

  FtwLiteDevice - Calling context
  FvBlock       - Fvb protocol instance
  Lba           - The block specified

Returns:

  In working block or not

--*/
;

BOOLEAN
IsBootBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
/*++

Routine Description:

  Check whether the block is a boot block.

Arguments:

  FtwLiteDevice - Calling context
  FvBlock       - Fvb protocol instance
  Lba           - Lba value

Returns:

  Is a boot block or not

--*/
;

EFI_STATUS
FlushSpareBlockToTargetBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
/*++

Routine Description:
    Copy the content of spare block to a target block. Size is FTW_BLOCK_SIZE.
    Spare block is accessed by FTW backup FVB protocol interface. LBA is 
    FtwLiteDevice->FtwSpareLba.
    Target block is accessed by FvBlock protocol interface. LBA is Lba.

Arguments:
    FtwLiteDevice  - The private data of FTW_LITE driver
    FvBlock        - FVB Protocol interface to access target block
    Lba            - Lba of the target block

Returns:
    EFI_SUCCESS              - Spare block content is copied to target block
    EFI_INVALID_PARAMETER    - Input parameter error
    EFI_OUT_OF_RESOURCES     - Allocate memory error
    EFI_ABORTED              - The function could not complete successfully

--*/
;

EFI_STATUS
FlushSpareBlockToWorkingBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice
  )
/*++

Routine Description:
    Copy the content of spare block to working block. Size is FTW_BLOCK_SIZE.
    Spare block is accessed by FTW backup FVB protocol interface. LBA is 
    FtwLiteDevice->FtwSpareLba.
    Working block is accessed by FTW working FVB protocol interface. LBA is 
    FtwLiteDevice->FtwWorkBlockLba.

Arguments:
    FtwLiteDevice  - The private data of FTW_LITE driver

Returns:
    EFI_SUCCESS              - Spare block content is copied to target block
    EFI_OUT_OF_RESOURCES     - Allocate memory error
    EFI_ABORTED              - The function could not complete successfully

Notes:
    Since the working block header is important when FTW initializes, the 
    state of the operation should be handled carefully. The Crc value is 
    calculated without STATE element. 

--*/
;

EFI_STATUS
FlushSpareBlockToBootBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice
  )
/*++

Routine Description:
    Copy the content of spare block to a boot block. Size is FTW_BLOCK_SIZE.
    Spare block is accessed by FTW backup FVB protocol interface. LBA is 
    FtwLiteDevice->FtwSpareLba.
    Boot block is accessed by BootFvb protocol interface. LBA is 0.

Arguments:
    FtwLiteDevice  - The private data of FTW_LITE driver

Returns:
    EFI_SUCCESS              - Spare block content is copied to boot block
    EFI_INVALID_PARAMETER    - Input parameter error
    EFI_OUT_OF_RESOURCES     - Allocate memory error
    EFI_ABORTED              - The function could not complete successfully

Notes:

--*/
;

EFI_STATUS
FtwUpdateFvState (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  IN EFI_LBA                             Lba,
  IN UINTN                               Offset,
  IN UINT8                               NewBit
  )
/*++

Routine Description:
    Update a bit of state on a block device. The location of the bit is 
    calculated by the (Lba, Offset, bit). Here bit is determined by the 
    the name of a certain bit.

Arguments:
    FvBlock    - FVB Protocol interface to access SrcBlock and DestBlock
    Lba        - Lba of a block
    Offset     - Offset on the Lba
    NewBit     - New value that will override the old value if it can be change

Returns:
    EFI_SUCCESS   - A state bit has been updated successfully
    Others        - Access block device error.

Notes:
    Assume all bits of State are inside the same BYTE. 

    EFI_ABORTED   - Read block fail
--*/
;

EFI_STATUS
FtwGetLastRecord (
  IN  EFI_FTW_LITE_DEVICE  *FtwLiteDevice,
  OUT EFI_FTW_LITE_RECORD  **FtwLastRecord
  )
/*++

Routine Description:
    Get the last Write record pointer. 
    The last record is the record whose 'complete' state hasn't been set.
    After all, this header may be a EMPTY header entry for next Allocate. 

Arguments:
    FtwLiteDevice   - Private data of this driver
    FtwLastRecord   - Pointer to retrieve the last write record

Returns:
    EFI_SUCCESS     - Get the last write record successfully
    EFI_ABORTED     - The FTW work space is damaged

--*/
;

BOOLEAN
IsErasedFlashBuffer (
  IN BOOLEAN         Polarity,
  IN UINT8           *Buffer,
  IN UINTN           BufferSize
  )
/*++

Routine Description:

  Check whether a flash buffer is erased.

Arguments:

  Polarity    - All 1 or all 0
  Buffer      - Buffer to check
  BufferSize  - Size of the buffer

Returns:

  Erased or not.

--*/
;

EFI_STATUS
InitWorkSpaceHeader (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingHeader
  )
/*++

Routine Description:
    Initialize a work space when there is no work space.

Arguments:
    WorkingHeader - Pointer of working block header 

Returns:
    EFI_SUCCESS   - The function completed successfully
    EFI_ABORTED   - The function could not complete successfully.

--*/
;

EFI_STATUS
WorkSpaceRefresh (
  IN EFI_FTW_LITE_DEVICE  *FtwLiteDevice
  )
/*++

Routine Description:
    Read from working block to refresh the work space in memory.

Arguments:
    FtwLiteDevice     - Point to private data of FTW driver

Returns:
    EFI_SUCCESS   - The function completed successfully
    EFI_ABORTED   - The function could not complete successfully.

--*/
;

BOOLEAN
IsValidWorkSpace (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *WorkingHeader
  )
/*++

Routine Description:
    Check to see if it is a valid work space.

Arguments:
    WorkingHeader - Pointer of working block header 

Returns:
    EFI_SUCCESS   - The function completed successfully
    EFI_ABORTED   - The function could not complete successfully.

--*/
;

EFI_STATUS
CleanupWorkSpace (
  IN EFI_FTW_LITE_DEVICE  *FtwLiteDevice,
  IN OUT UINT8            *BlockBuffer,
  IN UINTN                BufferSize
  )
/*++

Routine Description:
    Reclaim the work space. Get rid of all the completed write records
    and write records in the Fault Tolerant work space.

Arguments:
    FtwLiteDevice   - Point to private data of FTW driver
    FtwSpaceBuffer  - Buffer to contain the reclaimed clean data
    BufferSize      - Size of the FtwSpaceBuffer

Returns:
    EFI_SUCCESS           - The function completed successfully
    EFI_BUFFER_TOO_SMALL  - The FtwSpaceBuffer is too small
    EFI_ABORTED           - The function could not complete successfully.

--*/
;

EFI_STATUS
FtwReclaimWorkSpace (
  IN EFI_FTW_LITE_DEVICE  *FtwLiteDevice
  )
/*++

Routine Description:
    Reclaim the work space on the working block.

Arguments:
    FtwLiteDevice     - Point to private data of FTW driver

Returns:
    EFI_SUCCESS           - The function completed successfully
    EFI_OUT_OF_RESOURCES  - Allocate memory error
    EFI_ABORTED           - The function could not complete successfully

--*/
;

#endif

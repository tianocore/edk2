/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by Ftw module.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/

#ifndef _EFI_FAULT_TOLERANT_WRITE_H_
#define _EFI_FAULT_TOLERANT_WRITE_H_

#include <PiDxe.h>

#include <Guid/SystemNvDataGuid.h>
#include <Guid/ZeroGuid.h>
#include <Protocol/FaultTolerantWrite.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/SwapAddressRange.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>

//
// Flash erase polarity is 1
//
#define FTW_ERASE_POLARITY  1

#define FTW_ERASED_BYTE     ((UINT8) (255))
#define FTW_POLARITY_REVERT ((UINT8) (255))

#define HEADER_ALLOCATED  0x1
#define WRITES_ALLOCATED  0x2
#define WRITES_COMPLETED  0x4

#define BOOT_BLOCK_UPDATE 0x1
#define SPARE_COMPLETED   0x2
#define DEST_COMPLETED    0x4

#define FTW_BLOCKS(Length, BlockSize) ((UINTN) ((Length) / (BlockSize) + (((Length) & ((BlockSize) - 1)) ? 1 : 0)))

#define FTW_DEVICE_SIGNATURE  SIGNATURE_32 ('F', 'T', 'W', 'D')

//
// EFI Fault tolerant protocol private data structure
//
typedef struct {
  UINTN                                   Signature;
  EFI_HANDLE                              Handle;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL       FtwInstance;
  EFI_PHYSICAL_ADDRESS                    WorkSpaceAddress;   // Base address of working space range in flash.
  EFI_PHYSICAL_ADDRESS                    SpareAreaAddress;   // Base address of spare range in flash.
  UINTN                                   WorkSpaceLength;    // Size of working space range in flash.
  UINTN                                   NumberOfWorkSpaceBlock; // Number of the blocks in work block for work space.
  UINTN                                   WorkBlockSize;      // Block size in bytes of the work blocks in flash
  UINTN                                   SpareAreaLength;    // Size of spare range in flash.
  UINTN                                   NumberOfSpareBlock; // Number of the blocks in spare block.
  UINTN                                   SpareBlockSize;     // Block size in bytes of the spare blocks in flash
  EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER *FtwWorkSpaceHeader;// Pointer to Working Space Header in memory buffer
  EFI_FAULT_TOLERANT_WRITE_HEADER         *FtwLastWriteHeader;// Pointer to last record header in memory buffer
  EFI_FAULT_TOLERANT_WRITE_RECORD         *FtwLastWriteRecord;// Pointer to last record in memory buffer
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *FtwFvBlock;        // FVB of working block
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *FtwBackupFvb;      // FVB of spare block
  EFI_LBA                                 FtwSpareLba;        // Start LBA of spare block
  EFI_LBA                                 FtwWorkBlockLba;    // Start LBA of working block that contains working space in its last block.
  UINTN                                   NumberOfWorkBlock;  // Number of the blocks in work block.
  EFI_LBA                                 FtwWorkSpaceLba;    // Start LBA of working space
  UINTN                                   FtwWorkSpaceBase;   // Offset into the FtwWorkSpaceLba block.
  UINTN                                   FtwWorkSpaceSize;   // Size of working space range that stores write record.
  EFI_LBA                                 FtwWorkSpaceLbaInSpare; // Start LBA of working space in spare block.
  UINTN                                   FtwWorkSpaceBaseInSpare;// Offset into the FtwWorkSpaceLbaInSpare block.
  UINT8                                   *FtwWorkSpace;      // Point to Work Space in memory buffer 
  //
  // Following a buffer of FtwWorkSpace[FTW_WORK_SPACE_SIZE],
  // Allocated with EFI_FTW_DEVICE.
  //
} EFI_FTW_DEVICE;

#define FTW_CONTEXT_FROM_THIS(a)  CR (a, EFI_FTW_DEVICE, FtwInstance, FTW_DEVICE_SIGNATURE)

//
// Driver entry point
//
/**
  This function is the entry point of the Fault Tolerant Write driver.

  @param ImageHandle     A handle for the image that is initializing this driver
  @param SystemTable     A pointer to the EFI system table

  @return EFI_SUCCESS           FTW has finished the initialization
  @retval EFI_NOT_FOUND         Locate FVB protocol error
  @retval EFI_OUT_OF_RESOURCES  Allocate memory error
  @retval EFI_VOLUME_CORRUPTED  Firmware volume is error
  @retval EFI_ABORTED           FTW initialization error

**/
EFI_STATUS
EFIAPI
InitializeFaultTolerantWrite (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

//
// Fault Tolerant Write Protocol API
//

/**
  Query the largest block that may be updated in a fault tolerant manner.


  @param This            Indicates a pointer to the calling context.
  @param BlockSize       A pointer to a caller allocated UINTN that is updated to
                         indicate the size of the largest block that can be updated.

  @return EFI_SUCCESS   The function completed successfully

**/
EFI_STATUS
EFIAPI
FtwGetMaxBlockSize (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL    *This,
  OUT UINTN                               *BlockSize
  );

/**
  Allocates space for the protocol to maintain information about writes.
  Since writes must be completed in a fault tolerant manner and multiple
  updates will require more resources to be successful, this function
  enables the protocol to ensure that enough space exists to track
  information about the upcoming writes.

  All writes must be completed or aborted before another fault tolerant write can occur.

  @param This            Indicates a pointer to the calling context.
  @param CallerId        The GUID identifying the write.
  @param PrivateDataSize The size of the caller's private data
                         that must be recorded for each write.
  @param NumberOfWrites  The number of fault tolerant block writes
                         that will need to occur.

  @return EFI_SUCCESS        The function completed successfully
  @retval EFI_ABORTED        The function could not complete successfully.
  @retval EFI_ACCESS_DENIED  All allocated writes have not been completed.

**/
EFI_STATUS
EFIAPI
FtwAllocate (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL    *This,
  IN EFI_GUID                             *CallerId,
  IN UINTN                                PrivateDataSize,
  IN UINTN                                NumberOfWrites
  );

/**
  Starts a target block update. This function will record data about write
  in fault tolerant storage and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.


  @param This            Calling context
  @param Lba             The logical block address of the target block.
  @param Offset          The offset within the target block to place the data.
  @param Length          The number of bytes to write to the target block.
  @param PrivateData     A pointer to private data that the caller requires to
                         complete any pending writes in the event of a fault.
  @param FvBlockHandle   The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param Buffer          The data to write.

  @retval EFI_SUCCESS          The function completed successfully 
  @retval EFI_ABORTED          The function could not complete successfully. 
  @retval EFI_BAD_BUFFER_SIZE  The input data can't fit within the spare block. 
                               Offset + *NumBytes > SpareAreaLength.
  @retval EFI_ACCESS_DENIED    No writes have been allocated. 
  @retval EFI_OUT_OF_RESOURCES Cannot allocate enough memory resource.
  @retval EFI_NOT_FOUND        Cannot find FVB protocol by handle.

**/
EFI_STATUS
EFIAPI
FtwWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  IN EFI_LBA                               Lba,
  IN UINTN                                 Offset,
  IN UINTN                                 Length,
  IN VOID                                  *PrivateData,
  IN EFI_HANDLE                            FvBlockHandle,
  IN VOID                                  *Buffer
  );

/**
  Restarts a previously interrupted write. The caller must provide the
  block protocol needed to complete the interrupted write.

  @param This            Calling context.
  @param FvBlockHandle   The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.

  @retval  EFI_SUCCESS          The function completed successfully
  @retval  EFI_ACCESS_DENIED    No pending writes exist
  @retval  EFI_NOT_FOUND        FVB protocol not found by the handle
  @retval  EFI_ABORTED          The function could not complete successfully

**/
EFI_STATUS
EFIAPI
FtwRestart (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  IN EFI_HANDLE                            FvBlockHandle
  );

/**
  Aborts all previous allocated writes.

  @param  This                 Calling context

  @retval EFI_SUCCESS          The function completed successfully
  @retval EFI_ABORTED          The function could not complete successfully.
  @retval EFI_NOT_FOUND        No allocated writes exist.

**/
EFI_STATUS
EFIAPI
FtwAbort (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This
  );

/**
  Starts a target block update. This records information about the write
  in fault tolerant storage and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param This            Indicates a pointer to the calling context.
  @param CallerId        The GUID identifying the last write.
  @param Lba             The logical block address of the last write.
  @param Offset          The offset within the block of the last write.
  @param Length          The length of the last write.
  @param PrivateDataSize bytes from the private data
                         stored for this write.
  @param PrivateData     A pointer to a buffer. The function will copy
  @param Complete        A Boolean value with TRUE indicating
                         that the write was completed.

  @retval EFI_SUCCESS           The function completed successfully
  @retval EFI_ABORTED           The function could not complete successfully
  @retval EFI_NOT_FOUND         No allocated writes exist
  @retval EFI_BUFFER_TOO_SMALL  Input buffer is not larget enough

**/
EFI_STATUS
EFIAPI
FtwGetLastWrite (
  IN EFI_FAULT_TOLERANT_WRITE_PROTOCOL     *This,
  OUT EFI_GUID                             *CallerId,
  OUT EFI_LBA                              *Lba,
  OUT UINTN                                *Offset,
  OUT UINTN                                *Length,
  IN OUT UINTN                             *PrivateDataSize,
  OUT VOID                                 *PrivateData,
  OUT BOOLEAN                              *Complete
  );

/**
  Erase spare block.

  @param FtwDevice        The private data of FTW driver

  @retval EFI_SUCCESS           The erase request was successfully completed.
  @retval EFI_ACCESS_DENIED     The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR      The block device is not functioning
                                correctly and could not be written.
                                The firmware device may have been
                                partially erased.
  @retval EFI_INVALID_PARAMETER One or more of the LBAs listed
                                in the variable argument list do
                                not exist in the firmware volume.  


**/
EFI_STATUS
FtwEraseSpareBlock (
  IN EFI_FTW_DEVICE   *FtwDevice
  );

/**
  Retrive the proper FVB protocol interface by HANDLE.


  @param FvBlockHandle   The handle of FVB protocol that provides services for
                         reading, writing, and erasing the target block.
  @param FvBlock         The interface of FVB protocol

  @retval  EFI_SUCCESS   The function completed successfully
  @retval  EFI_ABORTED   The function could not complete successfully

**/
EFI_STATUS
FtwGetFvbByHandle (
  IN EFI_HANDLE                           FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  );

/**

  Is it in working block?

  @param FtwDevice       The private data of FTW driver
  @param FvBlock         Fvb protocol instance
  @param Lba             The block specified

  @return A BOOLEAN value indicating in working block or not.

**/
BOOLEAN
IsWorkingBlock (
  EFI_FTW_DEVICE                      *FtwDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  );

/**

  Is it in boot block?

  @param FtwDevice       The private data of FTW driver
  @param FvBlock         Fvb protocol instance

  @return A BOOLEAN value indicating in boot block or not.

**/
BOOLEAN
IsBootBlock (
  EFI_FTW_DEVICE                      *FtwDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock
  );

/**
  Copy the content of spare block to a target block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface.
  Target block is accessed by FvBlock protocol interface.


  @param FtwDevice       The private data of FTW driver
  @param FvBlock         FVB Protocol interface to access target block
  @param Lba             Lba of the target block
  @param BlockSize       The size of the block
  @param NumberOfBlocks  The number of consecutive blocks starting with Lba

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_INVALID_PARAMETER     Input parameter error
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToTargetBlock (
  EFI_FTW_DEVICE                      *FtwDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba,
  UINTN                               BlockSize,
  UINTN                               NumberOfBlocks
  );

/**
  Copy the content of spare block to working block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is
  FtwDevice->FtwSpareLba.
  Working block is accessed by FTW working FVB protocol interface. LBA is
  FtwDevice->FtwWorkBlockLba.

  Since the working block header is important when FTW initializes, the
  state of the operation should be handled carefully. The Crc value is
  calculated without STATE element.

  @param FtwDevice       The private data of FTW driver

  @retval  EFI_SUCCESS               Spare block content is copied to target block
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToWorkingBlock (
  EFI_FTW_DEVICE                      *FtwDevice
  );

/**
  Copy the content of spare block to a boot block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW working FVB protocol interface.
  Target block is accessed by FvBlock protocol interface.

  FTW will do extra work on boot block update.
  FTW should depend on a protocol of EFI_ADDRESS_RANGE_SWAP_PROTOCOL,
  which is produced by a chipset driver.
  FTW updating boot block steps may be:
  1. GetRangeLocation(), if the Range is inside the boot block, FTW know
  that boot block will be update. It shall add a FLAG in the working block.
  2. When spare block is ready,
  3. SetSwapState(SWAPPED)
  4. erasing boot block,
  5. programming boot block until the boot block is ok.
  6. SetSwapState(UNSWAPPED)
  FTW shall not allow to update boot block when battery state is error.

  @param FtwDevice       The private data of FTW driver

  @retval EFI_SUCCESS             Spare block content is copied to boot block
  @retval EFI_INVALID_PARAMETER   Input parameter error
  @retval EFI_OUT_OF_RESOURCES    Allocate memory error
  @retval EFI_ABORTED             The function could not complete successfully

**/
EFI_STATUS
FlushSpareBlockToBootBlock (
  EFI_FTW_DEVICE                      *FtwDevice
  );

/**
  Update a bit of state on a block device. The location of the bit is
  calculated by the (Lba, Offset, bit). Here bit is determined by the
  the name of a certain bit.


  @param FvBlock         FVB Protocol interface to access SrcBlock and DestBlock
  @param BlockSize       The size of the block
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
  IN UINTN                               BlockSize,
  IN EFI_LBA                             Lba,
  IN UINTN                               Offset,
  IN UINT8                               NewBit
  );

/**
  Get the last Write Header pointer.
  The last write header is the header whose 'complete' state hasn't been set.
  After all, this header may be a EMPTY header entry for next Allocate.


  @param FtwWorkSpaceHeader Pointer of the working block header
  @param FtwWorkSpaceSize   Size of the work space
  @param FtwWriteHeader     Pointer to retrieve the last write header

  @retval  EFI_SUCCESS      Get the last write record successfully
  @retval  EFI_ABORTED      The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastWriteHeader (
  IN EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER  *FtwWorkSpaceHeader,
  IN UINTN                                    FtwWorkSpaceSize,
  OUT EFI_FAULT_TOLERANT_WRITE_HEADER         **FtwWriteHeader
  );

/**
  Get the last Write Record pointer. The last write Record is the Record
  whose DestinationCompleted state hasn't been set. After all, this Record
  may be a EMPTY record entry for next write.


  @param FtwWriteHeader  Pointer to the write record header
  @param FtwWriteRecord  Pointer to retrieve the last write record

  @retval EFI_SUCCESS        Get the last write record successfully
  @retval EFI_ABORTED        The FTW work space is damaged

**/
EFI_STATUS
FtwGetLastWriteRecord (
  IN EFI_FAULT_TOLERANT_WRITE_HEADER          *FtwWriteHeader,
  OUT EFI_FAULT_TOLERANT_WRITE_RECORD         **FtwWriteRecord
  );

/**
  To check if FtwRecord is the first record of FtwHeader.

  @param FtwHeader  Pointer to the write record header
  @param FtwRecord  Pointer to the write record

  @retval TRUE      FtwRecord is the first Record of the FtwHeader
  @retval FALSE     FtwRecord is not the first Record of the FtwHeader

**/
BOOLEAN
IsFirstRecordOfWrites (
  IN EFI_FAULT_TOLERANT_WRITE_HEADER    *FtwHeader,
  IN EFI_FAULT_TOLERANT_WRITE_RECORD    *FtwRecord
  );

/**
  To check if FtwRecord is the last record of FtwHeader. Because the
  FtwHeader has NumberOfWrites & PrivateDataSize, the FtwRecord can be
  determined if it is the last record of FtwHeader.

  @param FtwHeader  Pointer to the write record header
  @param FtwRecord  Pointer to the write record

  @retval TRUE      FtwRecord is the last Record of the FtwHeader
  @retval FALSE     FtwRecord is not the last Record of the FtwHeader

**/
BOOLEAN
IsLastRecordOfWrites (
  IN EFI_FAULT_TOLERANT_WRITE_HEADER    *FtwHeader,
  IN EFI_FAULT_TOLERANT_WRITE_RECORD    *FtwRecord
  );

/**
  To check if FtwRecord is the first record of FtwHeader.

  @param FtwHeader  Pointer to the write record header
  @param FtwRecord  Pointer to retrieve the previous write record

  @retval EFI_ACCESS_DENIED  Input record is the first record, no previous record is return.
  @retval EFI_SUCCESS        The previous write record is found.

**/
EFI_STATUS
GetPreviousRecordOfWrites (
  IN     EFI_FAULT_TOLERANT_WRITE_HEADER    *FtwHeader,
  IN OUT EFI_FAULT_TOLERANT_WRITE_RECORD    **FtwRecord
  );

/**

  Check whether a flash buffer is erased.

  @param Buffer          Buffer to check
  @param BufferSize      Size of the buffer

  @return A BOOLEAN value indicating erased or not.

**/
BOOLEAN
IsErasedFlashBuffer (
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

  @param FtwDevice   Point to private data of FTW driver

  @retval  EFI_SUCCESS    The function completed successfully
  @retval  EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
WorkSpaceRefresh (
  IN EFI_FTW_DEVICE  *FtwDevice
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

  @param FtwDevice       Point to private data of FTW driver
  @param PreserveRecord  Whether to preserve the working record is needed

  @retval EFI_SUCCESS            The function completed successfully
  @retval EFI_OUT_OF_RESOURCES   Allocate memory error
  @retval EFI_ABORTED            The function could not complete successfully

**/
EFI_STATUS
FtwReclaimWorkSpace (
  IN EFI_FTW_DEVICE  *FtwDevice,
  IN BOOLEAN         PreserveRecord
  );

/**

  Get firmware volume block by address.


  @param Address         Address specified the block
  @param FvBlock         The block caller wanted

  @retval  EFI_SUCCESS    The protocol instance if found.
  @retval  EFI_NOT_FOUND  Block not found

**/
EFI_HANDLE
GetFvbByAddress (
  IN  EFI_PHYSICAL_ADDRESS               Address,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL **FvBlock
  );

/**
  Retrive the proper Swap Address Range protocol interface.

  @param[out] SarProtocol       The interface of SAR protocol

  @retval EFI_SUCCESS           The SAR protocol instance was found and returned in SarProtocol.
  @retval EFI_NOT_FOUND         The SAR protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
FtwGetSarProtocol (
  OUT VOID                                **SarProtocol
  );
  
/**
  Function returns an array of handles that support the FVB protocol
  in a buffer allocated from pool. 

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN                               *NumberHandles,
  OUT EFI_HANDLE                          **Buffer
  );


/**
  Allocate private data for FTW driver and initialize it.

  @param[out] FtwData           Pointer to the FTW device structure

  @retval EFI_SUCCESS           Initialize the FTW device successfully.
  @retval EFI_OUT_OF_RESOURCES  Allocate memory error
  @retval EFI_INVALID_PARAMETER Workspace or Spare block does not exist

**/
EFI_STATUS
InitFtwDevice (
  OUT EFI_FTW_DEVICE               **FtwData 
  );


/**
  Initialization for Fault Tolerant Write is done in this handler.

  @param[in, out] FtwDevice     Pointer to the FTW device structure

  @retval EFI_SUCCESS           Initialize the FTW protocol successfully.
  @retval EFI_NOT_FOUND         No proper FVB protocol was found.
  
**/
EFI_STATUS
InitFtwProtocol (
  IN OUT EFI_FTW_DEVICE               *FtwDevice
  );

/**
  Initialize a local work space header.

  Since Signature and WriteQueueSize have been known, Crc can be calculated out,
  then the work space header will be fixed.
**/
VOID
InitializeLocalWorkSpaceHeader (
  VOID
  );

/**
  Read work space data from work block or spare block.

  @param FvBlock        FVB Protocol interface to access the block.
  @param BlockSize      The size of the block.
  @param Lba            Lba of the block.
  @param Offset         The offset within the block.
  @param Length         The number of bytes to read from the block.
  @param Buffer         The data is read.

  @retval EFI_SUCCESS   The function completed successfully.
  @retval EFI_ABORTED   The function could not complete successfully.

**/
EFI_STATUS
ReadWorkSpaceData (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvBlock,
  IN UINTN                              BlockSize,
  IN EFI_LBA                            Lba,
  IN UINTN                              Offset,
  IN UINTN                              Length,
  OUT UINT8                             *Buffer
  );

/**
  Write data to work block.

  @param FvBlock        FVB Protocol interface to access the block.
  @param BlockSize      The size of the block.
  @param Lba            Lba of the block.
  @param Offset         The offset within the block to place the data.
  @param Length         The number of bytes to write to the block.
  @param Buffer         The data to write.

  @retval EFI_SUCCESS   The function completed successfully.
  @retval EFI_ABORTED   The function could not complete successfully.

**/
EFI_STATUS
WriteWorkSpaceData (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvBlock,
  IN UINTN                              BlockSize,
  IN EFI_LBA                            Lba,
  IN UINTN                              Offset,
  IN UINTN                              Length,
  IN UINT8                              *Buffer
  );

#endif

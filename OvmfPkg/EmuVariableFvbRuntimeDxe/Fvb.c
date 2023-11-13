/** @file
  Firmware Block Services to support emulating non-volatile variables
  by pretending that a memory buffer is storage for the NV variables.

  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiDxe.h"
#include <Guid/EventGroup.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/VariableFormat.h>

#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformFvbLib.h>
#include "Fvb.h"

#define EFI_AUTHENTICATED_VARIABLE_GUID \
{ 0xaaf32c78, 0x947b, 0x439a, { 0xa1, 0x80, 0x2e, 0x14, 0x4e, 0xc3, 0x77, 0x92 } }

//
// Virtual Address Change Event
//
// This is needed for runtime variable access.
//
EFI_EVENT  mEmuVarsFvbAddrChangeEvent = NULL;

//
// This is the single instance supported by this driver.  It
// supports the FVB and Device Path protocols.
//
EFI_FW_VOL_BLOCK_DEVICE  mEmuVarsFvb = {
  FVB_DEVICE_SIGNATURE,
  {     // DevicePath
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_MEMMAP_DP,
        {
          sizeof (MEMMAP_DEVICE_PATH),
          0
        }
      },
      EfiMemoryMappedIO,
      0,
      0,
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        sizeof (EFI_DEVICE_PATH_PROTOCOL),
        0
      }
    }
  },
  NULL,               // BufferPtr
  EMU_FVB_BLOCK_SIZE, // BlockSize
  EMU_FVB_SIZE,       // Size
  {     // FwVolBlockInstance
    FvbProtocolGetAttributes,
    FvbProtocolSetAttributes,
    FvbProtocolGetPhysicalAddress,
    FvbProtocolGetBlockSize,
    FvbProtocolRead,
    FvbProtocolWrite,
    FvbProtocolEraseBlocks,
    NULL
  },
};

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It converts pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
FvbVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, &mEmuVarsFvb.BufferPtr);
}

//
// FVB protocol APIs
//

/**
  The GetPhysicalAddress() function retrieves the base address of
  a memory-mapped firmware volume. This function should be called
  only for memory-mapped firmware volumes.

  @param This     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Address  Pointer to a caller-allocated
                  EFI_PHYSICAL_ADDRESS that, on successful
                  return from GetPhysicalAddress(), contains the
                  base address of the firmware volume.

  @retval EFI_SUCCESS       The firmware volume base address is returned.

  @retval EFI_NOT_SUPPORTED The firmware volume is not memory mapped.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  *Address = (EFI_PHYSICAL_ADDRESS)(UINTN)FvbDevice->BufferPtr;

  return EFI_SUCCESS;
}

/**
  The GetBlockSize() function retrieves the size of the requested
  block. It also returns the number of additional blocks with
  the identical size. The GetBlockSize() function is used to
  retrieve the block map (see EFI_FIRMWARE_VOLUME_HEADER).


  @param This           Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Lba            Indicates the block for which to return the size.

  @param BlockSize      Pointer to a caller-allocated UINTN in which
                        the size of the block is returned.

  @param NumberOfBlocks Pointer to a caller-allocated UINTN in
                        which the number of consecutive blocks,
                        starting with Lba, is returned. All
                        blocks in this range have a size of
                        BlockSize.


  @retval EFI_SUCCESS             The firmware volume base address is returned.

  @retval EFI_INVALID_PARAMETER   The requested LBA is out of range.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;

  if (Lba >= EMU_FVB_NUM_TOTAL_BLOCKS) {
    return EFI_INVALID_PARAMETER;
  }

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  *BlockSize      = FvbDevice->BlockSize;
  *NumberOfBlocks = (UINTN)(EMU_FVB_NUM_TOTAL_BLOCKS - Lba);

  return EFI_SUCCESS;
}

/**
  The GetAttributes() function retrieves the attributes and
  current settings of the block. Status Codes Returned

  @param This       Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Attributes Pointer to EFI_FVB_ATTRIBUTES_2 in which the
                    attributes and current settings are
                    returned. Type EFI_FVB_ATTRIBUTES_2 is defined
                    in EFI_FIRMWARE_VOLUME_HEADER.

  @retval EFI_SUCCESS The firmware volume attributes were
                      returned.

**/
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  *Attributes =
    (EFI_FVB_ATTRIBUTES_2)(
                           EFI_FVB2_READ_ENABLED_CAP |
                           EFI_FVB2_READ_STATUS |
                           EFI_FVB2_WRITE_ENABLED_CAP |
                           EFI_FVB2_WRITE_STATUS |
                           EFI_FVB2_ERASE_POLARITY
                           );

  return EFI_SUCCESS;
}

/**
  The SetAttributes() function sets configurable firmware volume
  attributes and returns the new settings of the firmware volume.

  @param This         Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Attributes   On input, Attributes is a pointer to
                      EFI_FVB_ATTRIBUTES_2 that contains the
                      desired firmware volume settings. On
                      successful return, it contains the new
                      settings of the firmware volume. Type
                      EFI_FVB_ATTRIBUTES_2 is defined in
                      EFI_FIRMWARE_VOLUME_HEADER.

  @retval EFI_SUCCESS           The firmware volume attributes were returned.

  @retval EFI_INVALID_PARAMETER The attributes requested are in
                                conflict with the capabilities
                                as declared in the firmware
                                volume header.

**/
EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  return EFI_ACCESS_DENIED;
}

/**
  Erases and initializes a firmware volume block.

  The EraseBlocks() function erases one or more blocks as denoted
  by the variable argument list. The entire parameter list of
  blocks must be verified before erasing any blocks. If a block is
  requested that does not exist within the associated firmware
  volume (it has a larger index than the last block of the
  firmware volume), the EraseBlocks() function must return the
  status code EFI_INVALID_PARAMETER without modifying the contents
  of the firmware volume. Implementations should be mindful that
  the firmware volume might be in the WriteDisabled state. If it
  is in this state, the EraseBlocks() function must return the
  status code EFI_ACCESS_DENIED without modifying the contents of
  the firmware volume. All calls to EraseBlocks() must be fully
  flushed to the hardware before the EraseBlocks() service
  returns.

  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL
                instance.

  @param ...    The variable argument list is a list of tuples.
                Each tuple describes a range of LBAs to erase
                and consists of the following:
                - An EFI_LBA that indicates the starting LBA
                - A UINTN that indicates the number of blocks to
                  erase

                The list is terminated with an
                EFI_LBA_LIST_TERMINATOR. For example, the
                following indicates that two ranges of blocks
                (5-7 and 10-11) are to be erased: EraseBlocks
                (This, 5, 3, 10, 2, EFI_LBA_LIST_TERMINATOR);

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
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  ...
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  VA_LIST                  Args;
  EFI_LBA                  StartingLba;
  UINTN                    NumOfLba;
  UINT8                    *ErasePtr;
  UINTN                    EraseSize;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  //
  // Check input parameters
  //
  VA_START (Args, This);
  do {
    StartingLba = VA_ARG (Args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (Args, UINTN);

    if ((StartingLba > EMU_FVB_NUM_TOTAL_BLOCKS) ||
        (NumOfLba > EMU_FVB_NUM_TOTAL_BLOCKS - StartingLba))
    {
      VA_END (Args);
      return EFI_INVALID_PARAMETER;
    }
  } while (1);

  VA_END (Args);

  //
  // Erase blocks
  //
  VA_START (Args, This);
  do {
    StartingLba = VA_ARG (Args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (Args, UINTN);

    ErasePtr  = FvbDevice->BufferPtr;
    ErasePtr += (UINTN)StartingLba * FvbDevice->BlockSize;
    EraseSize = NumOfLba * FvbDevice->BlockSize;

    SetMem (ErasePtr, EraseSize, ERASED_UINT8);
  } while (1);

  VA_END (Args);

  //
  // Call platform hook
  //
  VA_START (Args, This);
  PlatformFvbBlocksErased (This, Args);
  VA_END (Args);

  return EFI_SUCCESS;
}

/**
  Writes the specified number of bytes from the input buffer to the block.

  The Write() function writes the specified number of bytes from
  the provided buffer to the specified block and offset. If the
  firmware volume is sticky write, the caller must ensure that
  all the bits of the specified range to write are in the
  EFI_FVB_ERASE_POLARITY state before calling the Write()
  function, or else the result will be unpredictable. This
  unpredictability arises because, for a sticky-write firmware
  volume, a write may negate a bit in the EFI_FVB_ERASE_POLARITY
  state but cannot flip it back again. In general, before
  calling the Write() function, the caller should call the
  EraseBlocks() function first to erase the specified block to
  write. A block erase cycle will transition bits from the
  (NOT)EFI_FVB_ERASE_POLARITY state back to the
  EFI_FVB_ERASE_POLARITY state. Implementations should be
  mindful that the firmware volume might be in the WriteDisabled
  state. If it is in this state, the Write() function must
  return the status code EFI_ACCESS_DENIED without modifying the
  contents of the firmware volume. The Write() function must
  also prevent spanning block boundaries. If a write is
  requested that spans a block boundary, the write must store up
  to the boundary but not beyond. The output parameter NumBytes
  must be set to correctly indicate the number of bytes actually
  written. The caller must be aware that a write may be
  partially completed. All writes, partial or otherwise, must be
  fully flushed to the hardware before the Write() service
  returns.

  @param This     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Lba      The starting logical block index to write to.

  @param Offset   Offset into the block at which to begin writing.

  @param NumBytes Pointer to a UINTN. At entry, *NumBytes
                  contains the total size of the buffer. At
                  exit, *NumBytes contains the total number of
                  bytes actually written.

  @param Buffer   Pointer to a caller-allocated buffer that
                  contains the source for the write.

  @retval EFI_SUCCESS         The firmware volume was written successfully.

  @retval EFI_BAD_BUFFER_SIZE The write was attempted across an
                              LBA boundary. On output, NumBytes
                              contains the total number of bytes
                              actually written.

  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              WriteDisabled state.

  @retval EFI_DEVICE_ERROR    The block device is malfunctioning
                              and could not be written.


**/
EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  UINT8                    *FvbDataPtr;
  EFI_STATUS               Status;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  if ((Lba >= EMU_FVB_NUM_TOTAL_BLOCKS) ||
      (Offset > FvbDevice->BlockSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if (*NumBytes > FvbDevice->BlockSize - Offset) {
    *NumBytes = FvbDevice->BlockSize - Offset;
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  FvbDataPtr  = FvbDevice->BufferPtr;
  FvbDataPtr += (UINTN)Lba * FvbDevice->BlockSize;
  FvbDataPtr += Offset;

  CopyMem (FvbDataPtr, Buffer, *NumBytes);
  PlatformFvbDataWritten (This, Lba, Offset, *NumBytes, Buffer);
  return Status;
}

/**
  Reads the specified number of bytes into a buffer from the specified block.

  The Read() function reads the requested number of bytes from the
  requested block and stores them in the provided buffer.
  Implementations should be mindful that the firmware volume
  might be in the ReadDisabled state. If it is in this state,
  the Read() function must return the status code
  EFI_ACCESS_DENIED without modifying the contents of the
  buffer. The Read() function must also prevent spanning block
  boundaries. If a read is requested that would span a block
  boundary, the read must read up to the boundary but not
  beyond. The output parameter NumBytes must be set to correctly
  indicate the number of bytes actually read. The caller must be
  aware that a read may be partially completed.

  @param This     Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param Lba      The starting logical block index
                  from which to read.

  @param Offset   Offset into the block at which to begin reading.

  @param NumBytes Pointer to a UINTN. At entry, *NumBytes
                  contains the total size of the buffer. At
                  exit, *NumBytes contains the total number of
                  bytes read.

  @param Buffer   Pointer to a caller-allocated buffer that will
                  be used to hold the data that is read.

  @retval EFI_SUCCESS         The firmware volume was read successfully
                              and contents are in Buffer.

  @retval EFI_BAD_BUFFER_SIZE Read attempted across an LBA
                              boundary. On output, NumBytes
                              contains the total number of bytes
                              returned in Buffer.

  @retval EFI_ACCESS_DENIED   The firmware volume is in the
                              ReadDisabled state.

  @retval EFI_DEVICE_ERROR    The block device is not
                              functioning correctly and could
                              not be read.

**/
EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN OUT    UINT8                                *Buffer
  )
{
  EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice;
  UINT8                    *FvbDataPtr;
  EFI_STATUS               Status;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  if ((Lba >= EMU_FVB_NUM_TOTAL_BLOCKS) ||
      (Offset > FvbDevice->BlockSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if (*NumBytes > FvbDevice->BlockSize - Offset) {
    *NumBytes = FvbDevice->BlockSize - Offset;
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  FvbDataPtr  = FvbDevice->BufferPtr;
  FvbDataPtr += (UINTN)Lba * FvbDevice->BlockSize;
  FvbDataPtr += Offset;

  CopyMem (Buffer, FvbDataPtr, *NumBytes);
  PlatformFvbDataRead (This, Lba, Offset, *NumBytes, Buffer);
  return Status;
}

/**
  Check the integrity of firmware volume header.

  @param[in] FwVolHeader - A pointer to a firmware volume header

  @retval  EFI_SUCCESS   - The firmware volume is consistent
  @retval  EFI_NOT_FOUND - The firmware volume has been corrupted.

**/
EFI_STATUS
ValidateFvHeader (
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  )
{
  UINT16  Checksum;

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if ((FwVolHeader->Revision != EFI_FVH_REVISION) ||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength != EMU_FVB_SIZE) ||
      (FwVolHeader->HeaderLength != EMU_FV_HEADER_LENGTH)
      )
  {
    DEBUG ((DEBUG_INFO, "EMU Variable FVB: Basic FV headers were invalid\n"));
    return EFI_NOT_FOUND;
  }

  //
  // Verify the header checksum
  //
  Checksum = CalculateSum16 ((VOID *)FwVolHeader, FwVolHeader->HeaderLength);

  if (Checksum != 0) {
    DEBUG ((DEBUG_INFO, "EMU Variable FVB: FV checksum was invalid\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Initializes the FV Header and Variable Store Header
  to support variable operations.

  @param[in]  Ptr - Location to initialize the headers

**/
VOID
InitializeFvAndVariableStoreHeaders (
  IN  VOID  *Ptr
  )
{
  //
  // Templates for authenticated variable FV header
  //
  STATIC FVB_FV_HDR_AND_VARS_TEMPLATE  FvAndAuthenticatedVarTemplate = {
    { // EFI_FIRMWARE_VOLUME_HEADER FvHdr;
      // UINT8                     ZeroVector[16];
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

      // EFI_GUID                  FileSystemGuid;
      EFI_SYSTEM_NV_DATA_FV_GUID,

      // UINT64                    FvLength;
      EMU_FVB_SIZE,

      // UINT32                    Signature;
      EFI_FVH_SIGNATURE,

      // EFI_FVB_ATTRIBUTES_2      Attributes;
      0x4feff,

      // UINT16                    HeaderLength;
      EMU_FV_HEADER_LENGTH,

      // UINT16                    Checksum;
      0,

      // UINT16                    ExtHeaderOffset;
      0,

      // UINT8                     Reserved[1];
      { 0 },

      // UINT8                     Revision;
      EFI_FVH_REVISION,

      // EFI_FV_BLOCK_MAP_ENTRY    BlockMap[1];
      {
        {
          EMU_FVB_NUM_TOTAL_BLOCKS, // UINT32 NumBlocks;
          EMU_FVB_BLOCK_SIZE        // UINT32 Length;
        }
      }
    },
    // EFI_FV_BLOCK_MAP_ENTRY     EndBlockMap;
    { 0, 0 }, // End of block map
    { // VARIABLE_STORE_HEADER      VarHdr;
      // EFI_GUID  Signature;     // need authenticated variables for secure boot
      EFI_AUTHENTICATED_VARIABLE_GUID,

      // UINT32  Size;
      (
        FixedPcdGet32 (PcdFlashNvStorageVariableSize) -
        OFFSET_OF (FVB_FV_HDR_AND_VARS_TEMPLATE, VarHdr)
      ),

      // UINT8   Format;
      VARIABLE_STORE_FORMATTED,

      // UINT8   State;
      VARIABLE_STORE_HEALTHY,

      // UINT16  Reserved;
      0,

      // UINT32  Reserved1;
      0
    }
  };

  EFI_FIRMWARE_VOLUME_HEADER  *Fv;

  //
  // Copy the template structure into the location
  //
  CopyMem (
    Ptr,
    &FvAndAuthenticatedVarTemplate,
    sizeof FvAndAuthenticatedVarTemplate
    );

  //
  // Update the checksum for the FV header
  //
  Fv           = (EFI_FIRMWARE_VOLUME_HEADER *)Ptr;
  Fv->Checksum = CalculateCheckSum16 (Ptr, Fv->HeaderLength);
}

/**
  Main entry point.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Successfully initialized.

**/
EFI_STATUS
EFIAPI
FvbInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  VOID                  *Ptr;
  VOID                  *SubPtr;
  BOOLEAN               Initialize;
  EFI_HANDLE            Handle;
  EFI_PHYSICAL_ADDRESS  Address;
  RETURN_STATUS         PcdStatus;

  DEBUG ((DEBUG_INFO, "EMU Variable FVB Started\n"));

  //
  // Verify that the PCD's are set correctly.
  //
  ASSERT (
    FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize) %
    EMU_FVB_BLOCK_SIZE == 0
    );
  if (
      (PcdGet32 (PcdFlashNvStorageVariableSize) +
       PcdGet32 (PcdFlashNvStorageFtwWorkingSize)
      ) >
      EMU_FVB_NUM_SPARE_BLOCKS * EMU_FVB_BLOCK_SIZE
      )
  {
    DEBUG ((DEBUG_ERROR, "EMU Variable invalid PCD sizes\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (PcdGet64 (PcdFlashNvStorageVariableBase64) != 0) {
    DEBUG ((
      DEBUG_INFO,
      "Disabling EMU Variable FVB since "
      "flash variables appear to be supported.\n"
      ));
    return EFI_ABORTED;
  }

  //
  // By default we will initialize the FV contents.  But, if
  // PcdEmuVariableNvStoreReserved is non-zero, then we will
  // use this location for our buffer.
  //
  // If this location does not have a proper FV header, then
  // we will initialize it.
  //
  Initialize = TRUE;
  if (PcdGet64 (PcdEmuVariableNvStoreReserved) != 0) {
    Ptr = (VOID *)(UINTN)PcdGet64 (PcdEmuVariableNvStoreReserved);
    DEBUG ((
      DEBUG_INFO,
      "EMU Variable FVB: Using pre-reserved block at %p\n",
      Ptr
      ));
    Status = ValidateFvHeader (Ptr);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "EMU Variable FVB: Found valid pre-existing FV\n"));
      Initialize = FALSE;
    }
  } else {
    Ptr = AllocateRuntimePages (EFI_SIZE_TO_PAGES (EMU_FVB_SIZE));
  }

  mEmuVarsFvb.BufferPtr = Ptr;

  //
  // Initialize the main FV header and variable store header
  //
  if (Initialize) {
    SetMem (Ptr, EMU_FVB_SIZE, ERASED_UINT8);
    InitializeFvAndVariableStoreHeaders (Ptr);
  }

  PcdStatus = PcdSet64S (PcdFlashNvStorageVariableBase64, (UINTN)Ptr);
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // Initialize the Fault Tolerant Write data area
  //
  SubPtr    = (VOID *)((UINT8 *)Ptr + PcdGet32 (PcdFlashNvStorageVariableSize));
  PcdStatus = PcdSet64S (
                PcdFlashNvStorageFtwWorkingBase64,
                (UINTN)SubPtr
                );
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // Initialize the Fault Tolerant Write spare block
  //
  SubPtr = (VOID *)((UINT8 *)Ptr +
                    EMU_FVB_NUM_SPARE_BLOCKS * EMU_FVB_BLOCK_SIZE);
  PcdStatus = PcdSet64S (
                PcdFlashNvStorageFtwSpareBase64,
                (UINTN)SubPtr
                );
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // Setup FVB device path
  //
  Address                                              = (EFI_PHYSICAL_ADDRESS)(UINTN)Ptr;
  mEmuVarsFvb.DevicePath.MemMapDevPath.StartingAddress = Address;
  mEmuVarsFvb.DevicePath.MemMapDevPath.EndingAddress   = Address + EMU_FVB_SIZE - 1;

  //
  // Install the protocols
  //
  DEBUG ((DEBUG_INFO, "Installing FVB for EMU Variable support\n"));
  Handle = 0;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiFirmwareVolumeBlock2ProtocolGuid,
                  &mEmuVarsFvb.FwVolBlockInstance,
                  &gEfiDevicePathProtocolGuid,
                  &mEmuVarsFvb.DevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FvbVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mEmuVarsFvbAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

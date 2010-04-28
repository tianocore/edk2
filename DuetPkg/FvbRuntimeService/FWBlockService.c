/**@file
Copyright (c) 2007 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FWBlockService.c
    
Abstract:

Revision History

**/
#include "FWBlockService.h"
#include "EfiFlashMap.h"
#include "FileIo.h"
#include "FlashLayout.h"

ESAL_FWB_GLOBAL       *mFvbModuleGlobal;
VOID                  *mSFSRegistration;
#define TRY_ASSIGN(var, value)  if(var != NULL) {*var = value;}

EFI_FW_VOL_BLOCK_DEVICE mFvbDeviceTemplate = {
  FVB_DEVICE_SIGNATURE,
  {
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
  0,
  {
    FvbProtocolGetAttributes,
    FvbProtocolSetAttributes,
    FvbProtocolGetPhysicalAddress,
    FvbProtocolGetBlockSize,
    FvbProtocolRead,
    FvbProtocolWrite,
    FvbProtocolEraseBlocks,
    NULL
  }
};


EFI_STATUS
FlashFdWrite (
  IN     UINTN                            Address,
  IN     EFI_FW_VOL_INSTANCE              *FwhInstance,
  IN OUT UINTN                            *NumBytes,
  IN     UINT8                            *Buffer
  )
/*++

Routine Description:
  Writes specified number of bytes from the input buffer to the address

Arguments:

Returns: 

--*/
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *File;
  UINTN               FileOffset;
  UINTN               BufferForFile;
  UINTN               Length;

  Status = EFI_SUCCESS;
  CopyMem ((VOID *) Address, Buffer, *NumBytes);

  if (!EfiAtRuntime () && (FwhInstance->Device != NULL)) {
    Status = FileOpen (FwhInstance->Device, FwhInstance->MappedFile, &File, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE);
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      if (Address - FwhInstance->FvBase[FVB_PHYSICAL] < FwhInstance->Offset) {
        FileOffset = 0;
        BufferForFile = FwhInstance->FvBase[FVB_PHYSICAL] + FwhInstance->Offset;
        Length = *NumBytes - (FwhInstance->Offset - (Address - FwhInstance->FvBase[FVB_PHYSICAL]));
      } else {
        FileOffset = Address - FwhInstance->FvBase[FVB_PHYSICAL] - FwhInstance->Offset;
        BufferForFile = Address;
        Length = *NumBytes;
      }
      
      Status = FileWrite (File, FileOffset, BufferForFile, Length);
      ASSERT_EFI_ERROR (Status);
      FileClose (File);
    }
  }
  return Status;
}

EFI_STATUS
FlashFdErase (
  IN UINTN                                Address,
  IN EFI_FW_VOL_INSTANCE                  *FwhInstance,
  IN UINTN                                LbaLength
  )
/*++

Routine Description:
  Erase a certain block from address LbaWriteAddress

Arguments:

Returns: 

--*/
{
  EFI_STATUS           Status;
  EFI_FILE_PROTOCOL    *File;
  UINTN                FileOffset;
  UINTN                BufferForFile;
  UINTN                Length;

  Status = EFI_SUCCESS;

  SetMem ((VOID *)Address, LbaLength, 0xff);

  if (!EfiAtRuntime () && (FwhInstance->Device != NULL)) {
    Status = FileOpen (FwhInstance->Device, FwhInstance->MappedFile, &File, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE);
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      if (Address - FwhInstance->FvBase[FVB_PHYSICAL] < FwhInstance->Offset) {
        FileOffset = 0;
        BufferForFile = FwhInstance->FvBase[FVB_PHYSICAL] + FwhInstance->Offset;
        Length = LbaLength - (FwhInstance->Offset - (Address - FwhInstance->FvBase[FVB_PHYSICAL]));
      } else {
        FileOffset = Address - FwhInstance->FvBase[FVB_PHYSICAL] - FwhInstance->Offset;
        BufferForFile = Address;
        Length = LbaLength;
      }
      
      Status = FileWrite (File, FileOffset, BufferForFile, Length);
      ASSERT_EFI_ERROR (Status);
      FileClose (File);
    }
  }
  return Status;
}

VOID
EFIAPI
FvbVirtualddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Fixup internal data so that EFI and SAL can be call in virtual mode.
  Call the passed in Child Notify event and convert the mFvbModuleGlobal
  date items to there virtual address.

  mFvbModuleGlobal->FvInstance[FVB_PHYSICAL]  - Physical copy of instance data
  mFvbModuleGlobal->FvInstance[FVB_VIRTUAL]   - Virtual pointer to common 
                                                instance data.

Arguments:

  (Standard EFI notify event - EFI_EVENT_NOTIFY)

Returns: 

  None

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  UINTN               Index;

  EfiConvertPointer (0, (VOID **) &mFvbModuleGlobal->FvInstance[FVB_VIRTUAL]);

  //
  // Convert the base address of all the instances
  //
  Index       = 0;
  FwhInstance = mFvbModuleGlobal->FvInstance[FVB_PHYSICAL];
  while (Index < mFvbModuleGlobal->NumFv) {
    EfiConvertPointer (0, (VOID **) &FwhInstance->FvBase[FVB_VIRTUAL]);
    FwhInstance = (EFI_FW_VOL_INSTANCE *) ((UINTN)((UINT8 *)FwhInstance) + FwhInstance->VolumeHeader.HeaderLength
                    + (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER)));
    Index++;
  }

  EfiConvertPointer (0, (VOID **) &mFvbModuleGlobal->FvbScratchSpace[FVB_VIRTUAL]);
  EfiConvertPointer (0, (VOID **) &mFvbModuleGlobal);
}

EFI_STATUS
GetFvbInstance (
  IN  UINTN                               Instance,
  IN  ESAL_FWB_GLOBAL                     *Global,
  OUT EFI_FW_VOL_INSTANCE                 **FwhInstance,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV

Arguments:
  Instance              - The FV instance whose base address is going to be
                          returned
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  FwhInstance           - The EFI_FW_VOL_INSTANCE fimrware instance structure
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  EFI_FW_VOL_INSTANCE *FwhRecord;

  if (Instance >= Global->NumFv) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find the right instance of the FVB private data
  //
  FwhRecord = Global->FvInstance[Virtual];
  while (Instance > 0) {
    FwhRecord = (EFI_FW_VOL_INSTANCE *) ((UINTN)((UINT8 *)FwhRecord) + FwhRecord->VolumeHeader.HeaderLength 
                     + (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER)));
    Instance--;
  }

  *FwhInstance = FwhRecord;

  return EFI_SUCCESS;
}

EFI_STATUS
FvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *Address,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV

Arguments:
  Instance              - The FV instance whose base address is going to be
                          returned
  Address               - Pointer to a caller allocated EFI_PHYSICAL_ADDRESS 
                          that on successful return, contains the base address
                          of the firmware volume. 
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  EFI_STATUS          Status;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);
  *Address = FwhInstance->FvBase[Virtual];

  return EFI_SUCCESS;
}

EFI_STATUS
FvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES_2                *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          returned
  Attributes            - Output buffer which contains attributes
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  EFI_STATUS          Status;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);
  *Attributes = FwhInstance->VolumeHeader.Attributes;

  return EFI_SUCCESS;
}

EFI_STATUS
FvbGetLbaAddress (
  IN  UINTN                               Instance,
  IN  EFI_LBA                             Lba,
  OUT UINTN                               *LbaAddress  OPTIONAL,
  OUT UINTN                               *LbaLength   OPTIONAL,
  OUT UINTN                               *NumOfBlocks OPTIONAL,
  IN  ESAL_FWB_GLOBAL                     *Global,
  IN  BOOLEAN                             Virtual
  )
/*++

Routine Description:
  Retrieves the starting address of an LBA in an FV

Arguments:
  Instance              - The FV instance which the Lba belongs to
  Lba                   - The logical block address
  LbaAddress            - On output, contains the physical starting address
                          of the Lba for writing
  LbaLength             - On output, contains the length of the block
  NumOfBlocks           - A pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - Successfully returns
  EFI_INVALID_PARAMETER - Instance not found

--*/
{
  UINT32                  NumBlocks;
  UINT32                  BlockLength;
  UINTN                   Offset;
  EFI_LBA                 StartLba;
  EFI_LBA                 NextLba;
  EFI_FW_VOL_INSTANCE     *FwhInstance;
  EFI_FV_BLOCK_MAP_ENTRY  *BlockMap;
  EFI_STATUS              Status;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  StartLba  = 0;
  Offset    = 0;
  BlockMap  = &(FwhInstance->VolumeHeader.BlockMap[0]);

  //
  // Parse the blockmap of the FV to find which map entry the Lba belongs to
  //
  while (TRUE) {
    NumBlocks   = BlockMap->NumBlocks;
    BlockLength = BlockMap->Length;

    if (NumBlocks == 0 || BlockLength == 0) {
      return EFI_INVALID_PARAMETER;
    }

    NextLba = StartLba + NumBlocks;

    //
    // The map entry found
    //
    if (Lba >= StartLba && Lba < NextLba) {
      Offset = Offset + (UINTN) MultU64x32 ((Lba - StartLba), BlockLength);

      if (LbaAddress) {
        *LbaAddress = FwhInstance->FvBase[Virtual] + Offset;
      }

      if (LbaLength) {
        *LbaLength = BlockLength;
      }

      if (NumOfBlocks) {
        *NumOfBlocks = (UINTN) (NextLba - Lba);
      }

      return EFI_SUCCESS;
    }

    StartLba  = NextLba;
    Offset    = Offset + NumBlocks * BlockLength;
    BlockMap++;
  }
}

EFI_STATUS
FvbReadBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Reads specified number of bytes into a buffer from the specified block

Arguments:
  Instance              - The FV instance to be read from
  Lba                   - The logical block address to be read from
  BlockOffset           - Offset into the block at which to begin reading
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes read
  Buffer                - Pointer to a caller allocated buffer that will be
                          used to hold the data read
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - The firmware volume was read successfully and 
                          contents are in Buffer
  EFI_BAD_BUFFER_SIZE   - Read attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes returned
                          in Buffer
  EFI_ACCESS_DENIED     - The firmware volume is in the ReadDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and 
                          could not be read
  EFI_INVALID_PARAMETER - Instance not found, or NumBytes, Buffer are NULL

--*/
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;

  //
  // Check for invalid conditions
  //
  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check if the FV is read enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_READ_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Perform boundary checks and adjust NumBytes
  //
  if (BlockOffset > LbaLength) {
    return EFI_INVALID_PARAMETER;
  }

  if (LbaLength < (*NumBytes + BlockOffset)) {
    *NumBytes = (UINT32) (LbaLength - BlockOffset);
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  CopyMem (Buffer, (VOID *) (LbaAddress + BlockOffset), (UINTN) *NumBytes);

  return Status;
}
EFI_STATUS
FvbWriteBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Writes specified number of bytes from the input buffer to the block

Arguments:
  Instance              - The FV instance to be written to
  Lba                   - The starting logical block index to write to
  BlockOffset           - Offset into the block at which to begin writing
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes actually written
  Buffer                - Pointer to a caller allocated buffer that contains
                          the source for the write
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - The firmware volume was written successfully
  EFI_BAD_BUFFER_SIZE   - Write attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes
                          actually written
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and 
                          could not be written
  EFI_INVALID_PARAMETER - Instance not found, or NumBytes, Buffer are NULL

--*/
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_FW_VOL_INSTANCE   *FwhInstance;
  EFI_STATUS            Status;
  EFI_STATUS            ReturnStatus;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  //
  // Writes are enabled in the init routine itself
  //
  if (!FwhInstance->WriteEnabled) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Check for invalid conditions
  //
  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*NumBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check if the FV is write enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Perform boundary checks and adjust NumBytes
  //
  if (BlockOffset > LbaLength) {
    return EFI_INVALID_PARAMETER;
  }

  if (LbaLength < (*NumBytes + BlockOffset)) {
    *NumBytes = (UINT32) (LbaLength - BlockOffset);
    Status    = EFI_BAD_BUFFER_SIZE;
  }

  ReturnStatus = FlashFdWrite (
                  LbaAddress + BlockOffset,
                  FwhInstance,
                  NumBytes,
                  Buffer
                  );
  if (EFI_ERROR (ReturnStatus)) {
    return ReturnStatus;
  }

  return Status;
}

EFI_STATUS
FvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Erases and initializes a firmware volume block

Arguments:
  Instance              - The FV instance to be erased
  Lba                   - The logical block index to be erased
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - The erase request was successfully completed
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and 
                          could not be written. Firmware device may have been
                          partially erased
  EFI_INVALID_PARAMETER - Instance not found

--*/
{

  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  EFI_FW_VOL_INSTANCE   *FwhInstance;
  UINTN                 LbaLength;
  EFI_STATUS            Status;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  //
  // Writes are enabled in the init routine itself
  //
  if (!FwhInstance->WriteEnabled) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Check if the FV is write enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Get the starting address of the block for erase. For debug reasons,
  // LbaWriteAddress may not be the same as LbaAddress.
  //
  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL, Global, Virtual);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return FlashFdErase (
          LbaAddress,
          FwhInstance,
          LbaLength
          );
}

EFI_STATUS
FvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2             *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
/*++

Routine Description:
  Modifies the current settings of the firmware volume according to the 
  input parameter, and returns the new setting of the volume

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          modified
  Attributes            - On input, it is a pointer to EFI_FVB_ATTRIBUTES_2 
                          containing the desired firmware volume settings.
                          On successful return, it contains the new settings
                          of the firmware volume
  Global                - Pointer to ESAL_FWB_GLOBAL that contains all
                          instance data
  Virtual               - Whether CPU is in virtual or physical mode

Returns: 
  EFI_SUCCESS           - Successfully returns
  EFI_ACCESS_DENIED     - The volume setting is locked and cannot be modified
  EFI_INVALID_PARAMETER - Instance not found, or The attributes requested are
                          in conflict with the capabilities as declared in the
                          firmware volume header

--*/
{
  EFI_FW_VOL_INSTANCE   *FwhInstance;
  EFI_FVB_ATTRIBUTES_2  OldAttributes;
  EFI_FVB_ATTRIBUTES_2  *AttribPtr;
  UINT32                Capabilities;
  UINT32                OldStatus;
  UINT32                NewStatus;
  EFI_STATUS            Status;

  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  AttribPtr     = (EFI_FVB_ATTRIBUTES_2 *) &(FwhInstance->VolumeHeader.Attributes);
  OldAttributes = *AttribPtr;
  Capabilities  = OldAttributes & EFI_FVB2_CAPABILITIES;
  OldStatus     = OldAttributes & EFI_FVB2_STATUS;
  NewStatus     = *Attributes & EFI_FVB2_STATUS;

  //
  // If firmware volume is locked, no status bit can be updated
  //
  if (OldAttributes & EFI_FVB2_LOCK_STATUS) {
    if (OldStatus ^ NewStatus) {
      return EFI_ACCESS_DENIED;
    }
  }
  //
  // Test read disable
  //
  if ((Capabilities & EFI_FVB2_READ_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_READ_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test read enable
  //
  if ((Capabilities & EFI_FVB2_READ_ENABLED_CAP) == 0) {
    if (NewStatus & EFI_FVB2_READ_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test write disable
  //
  if ((Capabilities & EFI_FVB2_WRITE_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_WRITE_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test write enable
  //
  if ((Capabilities & EFI_FVB2_WRITE_ENABLED_CAP) == 0) {
    if (NewStatus & EFI_FVB2_WRITE_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Test lock
  //
  if ((Capabilities & EFI_FVB2_LOCK_CAP) == 0) {
    if (NewStatus & EFI_FVB2_LOCK_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  *AttribPtr  = (*AttribPtr) & (0xFFFFFFFF & (~EFI_FVB2_STATUS));
  *AttribPtr  = (*AttribPtr) | NewStatus;
  *Attributes = *AttribPtr;

  return EFI_SUCCESS;
}
//
// FVB protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN  CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL          *This,
  OUT       EFI_PHYSICAL_ADDRESS                        *Address
  )
/*++

Routine Description:

  Retrieves the physical address of the device.

Arguments:

  This                  - Calling context
  Address               - Output buffer containing the address.

Returns:

Returns: 
  EFI_SUCCESS           - Successfully returns

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetPhysicalAddress (FvbDevice->Instance, Address, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN  EFI_LBA                                     Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  )
/*++

Routine Description:
  Retrieve the size of a logical block

Arguments:
  This                  - Calling context
  Lba                   - Indicates which block to return the size for.
  BlockSize             - A pointer to a caller allocated UINTN in which
                          the size of the block is returned
  NumOfBlocks           - a pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize

Returns: 
  EFI_SUCCESS           - The firmware volume was read successfully and 
                          contents are in Buffer

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetLbaAddress (
          FvbDevice->Instance,
          Lba,
          NULL,
          BlockSize,
          NumOfBlocks,
          mFvbModuleGlobal,
          EfiGoneVirtual ()
          );
}

EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL          *This,
  OUT      EFI_FVB_ATTRIBUTES_2                        *Attributes
  )
/*++

Routine Description:
    Retrieves Volume attributes.  No polarity translations are done.

Arguments:
    This                - Calling context
    Attributes          - output buffer which contains attributes

Returns: 
  EFI_SUCCESS           - Successfully returns

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  IN OUT   EFI_FVB_ATTRIBUTES_2                     *Attributes
  )
/*++

Routine Description:
  Sets Volume attributes. No polarity translations are done.

Arguments:
  This                  - Calling context
  Attributes            - output buffer which contains attributes

Returns: 
  EFI_SUCCESS           - Successfully returns

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbSetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...  
  )
/*++

Routine Description:

  The EraseBlock() function erases one or more blocks as denoted by the 
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist 
  within the associated firmware volume (it has a larger index than the last 
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

Arguments:
  This                  - Calling context
  ...                   - Starting LBA followed by Number of Lba to erase. 
                          a -1 to terminate the list.

Returns: 
  EFI_SUCCESS           - The erase request was successfully completed
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and 
                          could not be written. Firmware device may have been
                          partially erased

--*/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;
  EFI_FW_VOL_INSTANCE     *FwhInstance;
  UINTN                   NumOfBlocks;
  VA_LIST                 args;
  EFI_LBA                 StartingLba;
  UINTN                   NumOfLba;
  EFI_STATUS              Status;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  Status    = GetFvbInstance (FvbDevice->Instance, mFvbModuleGlobal, &FwhInstance, EfiGoneVirtual ());
  ASSERT_EFI_ERROR (Status);

  NumOfBlocks = FwhInstance->NumOfBlocks;

  VA_START (args, This);

  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (args, UINT32);

    //
    // Check input parameters
    //
    if ((NumOfLba == 0) || ((StartingLba + NumOfLba) > NumOfBlocks)) {
      VA_END (args);
      return EFI_INVALID_PARAMETER;
    }
  } while (1);

  VA_END (args);

  VA_START (args, This);
  do {
    StartingLba = VA_ARG (args, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    NumOfLba = VA_ARG (args, UINT32);

    while (NumOfLba > 0) {
      Status = FvbEraseBlock (FvbDevice->Instance, StartingLba, mFvbModuleGlobal, EfiGoneVirtual ());
      if (EFI_ERROR (Status)) {
        VA_END (args);
        return Status;
      }

      StartingLba++;
      NumOfLba--;
    }

  } while (1);

  VA_END (args);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:

  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

Arguments:
  This                  - Calling context
  Lba                   - Block in which to begin write
  Offset                - Offset in the block at which to begin write
  NumBytes              - On input, indicates the requested write size. On
                          output, indicates the actual number of bytes written
  Buffer                - Buffer containing source data for the write.

Returns: 
  EFI_SUCCESS           - The firmware volume was written successfully
  EFI_BAD_BUFFER_SIZE   - Write attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes
                          actually written
  EFI_ACCESS_DENIED     - The firmware volume is in the WriteDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and 
                          could not be written
  EFI_INVALID_PARAMETER - NumBytes or Buffer are NULL

--*/
{

  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbWriteBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:

  Reads data beginning at Lba:Offset from FV. The Read terminates either
  when *NumBytes of data have been read, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

Arguments:
  This                  - Calling context
  Lba                   - Block in which to begin Read
  Offset                - Offset in the block at which to begin Read
  NumBytes              - On input, indicates the requested write size. On
                          output, indicates the actual number of bytes Read
  Buffer                - Buffer containing source data for the Read.

Returns: 
  EFI_SUCCESS           - The firmware volume was read successfully and 
                          contents are in Buffer
  EFI_BAD_BUFFER_SIZE   - Read attempted across a LBA boundary. On output,
                          NumBytes contains the total number of bytes returned
                          in Buffer
  EFI_ACCESS_DENIED     - The firmware volume is in the ReadDisabled state
  EFI_DEVICE_ERROR      - The block device is not functioning correctly and 
                          could not be read
  EFI_INVALID_PARAMETER - NumBytes or Buffer are NULL

--*/
{

  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbReadBlock (FvbDevice->Instance, Lba, Offset, NumBytes, Buffer, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
ValidateFvHeader (
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader
  )
/*++

Routine Description:
  Check the integrity of firmware volume header

Arguments:
  FwVolHeader           - A pointer to a firmware volume header

Returns: 
  EFI_SUCCESS           - The firmware volume is consistent
  EFI_NOT_FOUND         - The firmware volume has corrupted. So it is not an FV

--*/
{
  UINT16  *Ptr;
  UINT16  HeaderLength;
  UINT16  Checksum;

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if ((FwVolHeader->Revision != EFI_FVH_REVISION) ||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength == ((UINTN) -1)) ||
      ((FwVolHeader->HeaderLength & 0x01) != 0)
      ) {
    return EFI_NOT_FOUND;
  }
  //
  // Verify the header checksum
  //
  HeaderLength  = (UINT16) (FwVolHeader->HeaderLength / 2);
  Ptr           = (UINT16 *) FwVolHeader;
  Checksum      = 0;
  while (HeaderLength > 0) {
    Checksum = Checksum + (*Ptr);
    HeaderLength--;
    Ptr++;
  }

  if (Checksum != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
GetFvbHeader (
  IN OUT EFI_PEI_HOB_POINTERS           *HobList,
  OUT    EFI_FIRMWARE_VOLUME_HEADER     **FwVolHeader,
  OUT    EFI_PHYSICAL_ADDRESS           *BaseAddress     OPTIONAL,
  OUT    UINT32                         *VolumeId        OPTIONAL,
  OUT    CHAR16                         **MappedFile     OPTIONAL,
  OUT    UINT32                         *ActuralSize     OPTIONAL,
  OUT    UINT32                         *Offset          OPTIONAL,
  OUT    BOOLEAN                        *WriteBack       OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_FLASH_MAP_FS_ENTRY_DATA *FlashMapEntry;
  EFI_FLASH_SUBAREA_ENTRY     *FlashMapSubEntry;

  Status        = EFI_SUCCESS;
  *FwVolHeader  = NULL;
  TRY_ASSIGN (WriteBack, FALSE);

  DEBUG ((EFI_D_INFO, "Hob start is 0x%x\n", (UINTN)(*HobList).Raw));
  (*HobList).Raw = GetNextGuidHob (&gEfiFlashMapHobGuid, (*HobList).Raw);
  if ((*HobList).Raw == NULL) {
    return EFI_NOT_FOUND;
  }

  FlashMapEntry     = (EFI_FLASH_MAP_FS_ENTRY_DATA *) GET_GUID_HOB_DATA ((*HobList).Guid);
  FlashMapSubEntry  = &FlashMapEntry->Entries[0];
  
  //
  // Check if it is a "FVB" area
  //
  if (!CompareGuid (&FlashMapSubEntry->FileSystem, &gEfiFirmwareVolumeBlockProtocolGuid)) {
    return Status;
  }
  //
  // Check if it is a "real" flash
  //
  if (FlashMapSubEntry->Attributes != (EFI_FLASH_AREA_FV | EFI_FLASH_AREA_MEMMAPPED_FV)) {
    return Status;
  }

  TRY_ASSIGN (BaseAddress, FlashMapSubEntry->Base);

  //
  // Cast buffer to FLASH_AREA_INFO to get extra information related to the special FVB driver
  //
  TRY_ASSIGN (VolumeId,    FlashMapEntry->VolumeId);
  TRY_ASSIGN (ActuralSize, FlashMapEntry->ActuralSize);
  TRY_ASSIGN (MappedFile, ((CHAR16 *) FlashMapEntry->FilePath));
  TRY_ASSIGN (Offset,      FlashMapEntry->Offset);

  DEBUG ((
    EFI_D_INFO, 
    "FlashMap HOB: BaseAddress = 0x%x, Length = 0x%x, ActuralLength = 0x%x, Offset = 0x%x\n", 
    (UINTN) FlashMapSubEntry->Base, (UINTN) FlashMapSubEntry->Length, 
    (UINTN) FlashMapEntry->ActuralSize, (UINTN) FlashMapEntry->Offset
  ));
  DEBUG ((
    EFI_D_INFO,
    "FlashMap HOB: VolumeId = 0x%lx, MappedFile = %s\n",
    (UINTN) FlashMapEntry->VolumeId, (UINTN) FlashMapEntry->FilePath
  ));
  *FwVolHeader  = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (FlashMapSubEntry->Base);
  Status        = ValidateFvHeader (*FwVolHeader);
  if (EFI_ERROR (Status)) {
    //
    // Get FvbInfo
    //
    TRY_ASSIGN (WriteBack, TRUE);
    Status = GetFvbInfo (FlashMapSubEntry->Length, FwVolHeader);
    DEBUG ((EFI_D_ERROR, "Fvb: FV header invalid, GetFvbInfo - %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
OnSimpleFileSystemInstall (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleSize;
  EFI_HANDLE                Handle;
  UINTN                     Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Device;
  EFI_FILE_PROTOCOL         *File;
  EFI_FW_VOL_INSTANCE       *FwhInstance;
  while (TRUE) {
    HandleSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mSFSRegistration,
                    &HandleSize,
                    &Handle
                    );
    if (Status == EFI_NOT_FOUND) {
      break;
    }
    DEBUG ((EFI_D_ERROR, "Fwh: New FileSystem Installed!\n"));
    ASSERT_EFI_ERROR (Status);
    //
    // Check if this is the storage we care about, and store it in FwhInstance
    //
    for (Instance = 0; Instance < mFvbModuleGlobal->NumFv; ++Instance) {
      Status = GetFvbInstance (Instance, mFvbModuleGlobal, &FwhInstance, FALSE);
      ASSERT_EFI_ERROR (Status);

      if (FwhInstance->MappedFile[0] == L'\0') {
        //
        // The instance of FVB isn't mapped to file.
        //
        continue;
      }

      if ((FwhInstance->Device != NULL) && 
          !EFI_ERROR (CheckStoreExists (FwhInstance->Device))
          ) {
        //
        // The instance of FVB has already associated to a device
        //  and the device is not removed from system.
        //
        DEBUG ((
              EFI_D_ERROR, "Fwh: MappedFile FVB (0x%x:0x%x) - Already mapped, Skip!\n", 
              (UINTN) FwhInstance->FvBase[FVB_PHYSICAL],
              (UINTN) FwhInstance->Offset
              ));
        continue;
      }

      Status = CheckStore (Handle, FwhInstance->VolumeId, &Device);
      if (!EFI_ERROR (Status)) {
        //
        // Write back memory content to file
        //
        Status = FileOpen (Device, FwhInstance->MappedFile, &File, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE);
        ASSERT_EFI_ERROR (Status); 
        if (!EFI_ERROR (Status)) {
          DEBUG ((
                EFI_D_ERROR, "Fwh: MappedFile FVB (0x%x:0x%x) - Write back to mapped file!\n", 
                (UINTN) FwhInstance->FvBase[FVB_PHYSICAL],
                (UINTN) FwhInstance->Offset
                ));
          Status = FileWrite (
                     File, 
                     0, 
                     FwhInstance->FvBase[FVB_PHYSICAL] + FwhInstance->Offset, 
                     FwhInstance->ActuralSize - FwhInstance->Offset
                     );
          ASSERT_EFI_ERROR (Status); 
          if (!EFI_ERROR (Status)) {
            if (FwhInstance->Device != NULL) {
              gBS->FreePool (FwhInstance->Device);
            }
            FwhInstance->Device = Device;
            DEBUG ((
                  EFI_D_ERROR, "Fwh: MappedFile FVB (0x%x:0x%x) - Mapped!\n",
                  (UINTN) FwhInstance->FvBase[FVB_PHYSICAL],
                  (UINTN) FwhInstance->Offset
                  ));
          }
          FileClose (File);
        }
      }
    }
  }
}

VOID
FvbInstallSfsNotify (
  VOID
)
{
  EFI_STATUS Status;
  EFI_EVENT  Event;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnSimpleFileSystemInstall,
                  NULL,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->RegisterProtocolNotify (
                  &gEfiSimpleFileSystemProtocolGuid,
                  Event,
                  &mSFSRegistration
                  );
  ASSERT_EFI_ERROR (Status);
}


EFI_STATUS
EFIAPI
FvbInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  This function does common initialization for FVB services

Arguments:

Returns:

--*/
{
  EFI_STATUS                          Status;
  EFI_FW_VOL_INSTANCE                 *FwhInstance;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_PEI_HOB_POINTERS                FirmwareVolumeHobList;
  UINT32                              BufferSize;
  EFI_FV_BLOCK_MAP_ENTRY              *PtrBlockMapEntry;
  UINTN                               LbaAddress;
  EFI_HANDLE                          FwbHandle;
  EFI_FW_VOL_BLOCK_DEVICE             *FvbDevice;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *OldFwbInterface;
  EFI_DEVICE_PATH_PROTOCOL            *TempFwbDevicePath;
  FV_DEVICE_PATH                      TempFvbDevicePathData;
  UINT32                              MaxLbaSize;
  EFI_PHYSICAL_ADDRESS                BaseAddress;
  UINT32                              VolumeId;
  CHAR16                              *MappedFile;
  UINT32                              ActuralSize;
  UINT32                              Offset;
  BOOLEAN                             WriteBack;
  UINTN                               NumOfBlocks;
  UINTN                               HeaderLength;
  BOOLEAN                             InstallSfsNotify;

  HeaderLength     = 0;
  InstallSfsNotify = FALSE;

  //
  // Allocate runtime services data for global variable, which contains
  // the private data of all firmware volume block instances
  //
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  sizeof (ESAL_FWB_GLOBAL),
                  &mFvbModuleGlobal
                  );
  ASSERT_EFI_ERROR (Status);
  //
  // Calculate the total size for all firmware volume block instances
  //
  BufferSize            = 0;
  FirmwareVolumeHobList.Raw = GetHobList();
  do {
    Status = GetFvbHeader (&FirmwareVolumeHobList, &FwVolHeader, NULL, NULL, NULL, NULL, NULL, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }
    FirmwareVolumeHobList.Raw = GET_NEXT_HOB (FirmwareVolumeHobList);

    if (FwVolHeader) {
      BufferSize += (FwVolHeader->HeaderLength + sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER));
    }
  } while (TRUE);

  //
  // Only need to allocate once. There is only one copy of physical memory for
  // the private data of each FV instance. But in virtual mode or in physical
  // mode, the address of the the physical memory may be different.
  //
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  BufferSize,
                  &mFvbModuleGlobal->FvInstance[FVB_PHYSICAL]
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Make a virtual copy of the FvInstance pointer.
  //
  FwhInstance = mFvbModuleGlobal->FvInstance[FVB_PHYSICAL];
  mFvbModuleGlobal->FvInstance[FVB_VIRTUAL] = FwhInstance;

  mFvbModuleGlobal->NumFv     = 0;
  FirmwareVolumeHobList.Raw   = GetHobList();
  MaxLbaSize                  = 0;

  //
  // Fill in the private data of each firmware volume block instance
  //
  do {
    Status = GetFvbHeader (
               &FirmwareVolumeHobList, &FwVolHeader, 
               &BaseAddress, &VolumeId, &MappedFile, &ActuralSize, &Offset,
               &WriteBack
             );
    if (EFI_ERROR (Status)) {
      break;
    }
    FirmwareVolumeHobList.Raw = GET_NEXT_HOB (FirmwareVolumeHobList);

    if (!FwVolHeader) {
      continue;
    }
    
    CopyMem ((UINTN *) &(FwhInstance->VolumeHeader), (UINTN *) FwVolHeader, FwVolHeader->HeaderLength);
    FwVolHeader                       = &(FwhInstance->VolumeHeader);

    FwhInstance->FvBase[FVB_PHYSICAL] = (UINTN) BaseAddress;
    FwhInstance->FvBase[FVB_VIRTUAL]  = (UINTN) BaseAddress;
    FwhInstance->Device               = NULL;
    FwhInstance->Offset               = Offset;

    if (*MappedFile != '\0') {
      FwhInstance->VolumeId             = VolumeId;
      FwhInstance->ActuralSize          = ActuralSize;
      StrCpy (FwhInstance->MappedFile, MappedFile);

      InstallSfsNotify = TRUE;
    } else {
      FwhInstance->VolumeId             = (UINT32) -1;
      FwhInstance->ActuralSize          = (UINT32) -1;
      FwhInstance->MappedFile[0]        = L'\0';
    }
    
    DEBUG ((EFI_D_INFO, "FirmVolume Found! BaseAddress=0x%lx, VolumeId=0x%x, MappedFile=%s, Size=0x%x\n",
           (UINTN) BaseAddress, VolumeId, MappedFile, ActuralSize));
    //
    // We may expose readonly FVB in future.
    //
    FwhInstance->WriteEnabled         = TRUE; // Ken: Why enable write?
    EfiInitializeLock (&(FwhInstance->FvbDevLock), TPL_HIGH_LEVEL);

    LbaAddress  = (UINTN) FwhInstance->FvBase[0];
    NumOfBlocks = 0;

    if (FwhInstance->WriteEnabled) {
      for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {

        LbaAddress += PtrBlockMapEntry->NumBlocks * PtrBlockMapEntry->Length;
        //
        // Get the maximum size of a block. The size will be used to allocate
        // buffer for Scratch space, the intermediate buffer for FVB extension
        // protocol
        //
        if (MaxLbaSize < PtrBlockMapEntry->Length) {
          MaxLbaSize = PtrBlockMapEntry->Length;
        }

        NumOfBlocks += PtrBlockMapEntry->NumBlocks;
      }
      //
      //  Write back a healthy FV header
      //
      if (WriteBack) {
        Status = FlashFdErase (
                  (UINTN) FwhInstance->FvBase[0],
                  FwhInstance,
                  FwVolHeader->BlockMap->Length
                  );

        HeaderLength = (UINTN) FwVolHeader->HeaderLength;

        Status = FlashFdWrite (
                  (UINTN) FwhInstance->FvBase[0],
                  FwhInstance,
                  (UINTN *) &HeaderLength,
                  (UINT8 *) FwVolHeader
                  );

        FwVolHeader->HeaderLength = (UINT16) HeaderLength;

        DEBUG ((EFI_D_ERROR, "Fvb (0x%x): FV header invalid, write back - %r\n", (UINTN) FwhInstance->FvBase[0], Status));
      }
    }
    //
    // The total number of blocks in the FV.
    //
    FwhInstance->NumOfBlocks = NumOfBlocks;

    //
    // Add a FVB Protocol Instance
    //
    Status = gBS->AllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (EFI_FW_VOL_BLOCK_DEVICE),
                    &FvbDevice
                    );
    ASSERT_EFI_ERROR (Status);

    CopyMem (FvbDevice, &mFvbDeviceTemplate, sizeof (EFI_FW_VOL_BLOCK_DEVICE));

    FvbDevice->Instance = mFvbModuleGlobal->NumFv;
    mFvbModuleGlobal->NumFv++;

    //
    // Set up the devicepath
    //
    FvbDevice->DevicePath.MemMapDevPath.StartingAddress = BaseAddress;
    FvbDevice->DevicePath.MemMapDevPath.EndingAddress   = BaseAddress + (FwVolHeader->FvLength - 1);

    //
    // Find a handle with a matching device path that has supports FW Block protocol
    //
    TempFwbDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) &TempFvbDevicePathData;
    CopyMem (TempFwbDevicePath, &FvbDevice->DevicePath, sizeof (FV_DEVICE_PATH));
    Status = gBS->LocateDevicePath (&gEfiFirmwareVolumeBlockProtocolGuid, &TempFwbDevicePath, &FwbHandle);
    if (EFI_ERROR (Status)) {
      //
      // LocateDevicePath fails so install a new interface and device path
      //
      FwbHandle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &FwbHandle,
                      &gEfiFirmwareVolumeBlockProtocolGuid,
                      &FvbDevice->FwVolBlockInstance,
                      &gEfiDevicePathProtocolGuid,
                      &FvbDevice->DevicePath,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);
    } else if (IsDevicePathEnd (TempFwbDevicePath)) {
      //
      // Device allready exists, so reinstall the FVB protocol
      //
      Status = gBS->HandleProtocol (
                      FwbHandle,
                      &gEfiFirmwareVolumeBlockProtocolGuid,
                      &OldFwbInterface
                      );
      ASSERT_EFI_ERROR (Status);

      Status = gBS->ReinstallProtocolInterface (
                      FwbHandle,
                      &gEfiFirmwareVolumeBlockProtocolGuid,
                      OldFwbInterface,
                      &FvbDevice->FwVolBlockInstance
                      );
      ASSERT_EFI_ERROR (Status);

    } else {
      //
      // There was a FVB protocol on an End Device Path node
      //
      ASSERT (FALSE);
    }

    FwhInstance = (EFI_FW_VOL_INSTANCE *)
      (
        (UINTN) ((UINT8 *) FwhInstance) + FwVolHeader->HeaderLength +
          (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
      );
  } while (TRUE);

  //
  // Allocate for scratch space, an intermediate buffer for FVB extention
  //
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  MaxLbaSize,
                  &mFvbModuleGlobal->FvbScratchSpace[FVB_PHYSICAL]
                  );
  ASSERT_EFI_ERROR (Status);

  mFvbModuleGlobal->FvbScratchSpace[FVB_VIRTUAL] = mFvbModuleGlobal->FvbScratchSpace[FVB_PHYSICAL];

  if (InstallSfsNotify) {
    FvbInstallSfsNotify ();
  }
  return EFI_SUCCESS;
}

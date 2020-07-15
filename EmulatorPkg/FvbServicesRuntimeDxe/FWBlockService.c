/*++ @file

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiDxe.h"
#include <Guid/EventGroup.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include "FwBlockService.h"

ESAL_FWB_GLOBAL         *mFvbModuleGlobal;

#define EFI_FVB2_STATUS (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)

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

**/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  UINTN               Index;

  EfiConvertPointer (0x0, (VOID **) &mFvbModuleGlobal->FvInstance[FVB_VIRTUAL]);

  //
  // Convert the base address of all the instances
  //
  Index       = 0;
  FwhInstance = mFvbModuleGlobal->FvInstance[FVB_PHYSICAL];
  while (Index < mFvbModuleGlobal->NumFv) {
    EfiConvertPointer (0x0, (VOID **) &FwhInstance->FvBase[FVB_VIRTUAL]);
    FwhInstance = (EFI_FW_VOL_INSTANCE *)
      (
        (UINTN) ((UINT8 *) FwhInstance) + FwhInstance->VolumeHeader.HeaderLength +
          (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
      );
    Index++;
  }

  EfiConvertPointer (0x0, (VOID **) &mFvbModuleGlobal);
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

**/
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
    FwhRecord = (EFI_FW_VOL_INSTANCE *)
      (
        (UINTN) ((UINT8 *) FwhRecord) + FwhRecord->VolumeHeader.HeaderLength +
          (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
      );
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

**/
{
  EFI_FW_VOL_INSTANCE *FwhInstance = NULL;
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

**/
{
  EFI_FW_VOL_INSTANCE *FwhInstance = NULL;
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
  OUT UINTN                               *LbaAddress,
  OUT UINTN                               *LbaLength,
  OUT UINTN                               *NumOfBlocks,
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
                          of the Lba
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

**/
{
  UINT32                  NumBlocks;
  UINT32                  BlockLength;
  UINTN                   Offset;
  EFI_LBA                 StartLba;
  EFI_LBA                 NextLba;
  EFI_FW_VOL_INSTANCE     *FwhInstance = NULL;
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
      if (LbaAddress != NULL) {
        *LbaAddress = FwhInstance->FvBase[Virtual] + Offset;
      }

      if (LbaLength != NULL) {
        *LbaLength = BlockLength;
      }

      if (NumOfBlocks != NULL) {
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

**/
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN               LbaAddress;
  UINTN               LbaLength;
  EFI_STATUS          Status;

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

  CopyMem (Buffer, (UINT8 *) (LbaAddress + BlockOffset), (UINTN) (*NumBytes));

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

**/
{
  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN               LbaAddress;
  UINTN               LbaLength;
  EFI_STATUS          Status;

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
  //
  // Write data
  //
  CopyMem ((UINT8 *) (LbaAddress + BlockOffset), Buffer, (UINTN) (*NumBytes));

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

**/
{

  EFI_FVB_ATTRIBUTES_2  Attributes;
  UINTN                 LbaAddress;
  UINTN                 LbaLength;
  EFI_STATUS            Status;
  UINT8                 Data;

  //
  // Check if the FV is write enabled
  //
  FvbGetVolumeAttributes (Instance, &Attributes, Global, Virtual);

  if ((Attributes & EFI_FVB2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Get the starting address of the block for erase.
  //
  Status = FvbGetLbaAddress (Instance, Lba, &LbaAddress, &LbaLength, NULL, Global, Virtual);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Attributes & EFI_FVB2_ERASE_POLARITY) != 0) {
    Data = 0xFF;
  } else {
    Data = 0x0;
  }

  SetMem ((UINT8 *) LbaAddress, LbaLength, Data);

  return EFI_SUCCESS;
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

**/
{
  EFI_FW_VOL_INSTANCE   *FwhInstance = NULL;
  EFI_FVB_ATTRIBUTES_2  OldAttributes;
  EFI_FVB_ATTRIBUTES_2  *AttribPtr;
  UINT32                Capabilities;
  UINT32                OldStatus;
  UINT32                NewStatus;
  EFI_STATUS            Status;
  EFI_FVB_ATTRIBUTES_2  UnchangedAttributes;


  //
  // Find the right instance of the FVB private data
  //
  Status = GetFvbInstance (Instance, Global, &FwhInstance, Virtual);
  ASSERT_EFI_ERROR (Status);

  AttribPtr     = (EFI_FVB_ATTRIBUTES_2 *) &(FwhInstance->VolumeHeader.Attributes);
  OldAttributes = *AttribPtr;
  Capabilities  = OldAttributes & (EFI_FVB2_READ_DISABLED_CAP | \
                                   EFI_FVB2_READ_ENABLED_CAP | \
                                   EFI_FVB2_WRITE_DISABLED_CAP | \
                                   EFI_FVB2_WRITE_ENABLED_CAP | \
                                   EFI_FVB2_LOCK_CAP \
                                   );

  OldStatus     = OldAttributes & EFI_FVB2_STATUS;
  NewStatus     = *Attributes & EFI_FVB2_STATUS;
  UnchangedAttributes = EFI_FVB2_READ_DISABLED_CAP  | \
                        EFI_FVB2_READ_ENABLED_CAP   | \
                        EFI_FVB2_WRITE_DISABLED_CAP | \
                        EFI_FVB2_WRITE_ENABLED_CAP  | \
                        EFI_FVB2_LOCK_CAP           | \
                        EFI_FVB2_STICKY_WRITE       | \
                        EFI_FVB2_MEMORY_MAPPED      | \
                        EFI_FVB2_ERASE_POLARITY     | \
                        EFI_FVB2_READ_LOCK_CAP      | \
                        EFI_FVB2_WRITE_LOCK_CAP     | \
                        EFI_FVB2_ALIGNMENT;

  //
  // Some attributes of FV is read only can *not* be set
  //
  if ((OldAttributes & UnchangedAttributes) ^ (*Attributes & UnchangedAttributes)) {
    return EFI_INVALID_PARAMETER;
  }

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
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  OUT EFI_PHYSICAL_ADDRESS                        *Address
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

**/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetPhysicalAddress (FvbDevice->Instance, Address, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
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

**/
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
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  OUT EFI_FVB_ATTRIBUTES_2                        *Attributes
  )
/*++

Routine Description:
    Retrieves Volume attributes.  No polarity translations are done.

Arguments:
    This                - Calling context
    Attributes          - output buffer which contains attributes

Returns:
  EFI_SUCCESS           - Successfully returns

**/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);

  return FvbGetVolumeAttributes (FvbDevice->Instance, Attributes, mFvbModuleGlobal, EfiGoneVirtual ());
}

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                     *Attributes
  )
/*++

Routine Description:
  Sets Volume attributes. No polarity translations are done.

Arguments:
  This                  - Calling context
  Attributes            - output buffer which contains attributes

Returns:
  EFI_SUCCESS           - Successfully returns

**/
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

**/
{
  EFI_FW_VOL_BLOCK_DEVICE *FvbDevice;
  EFI_FW_VOL_INSTANCE     *FwhInstance = NULL;
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

    NumOfLba = VA_ARG (args, UINTN);

    //
    // Check input parameters
    //
    if (NumOfLba == 0 || (StartingLba + NumOfLba) > NumOfBlocks) {
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

    NumOfLba = VA_ARG (args, UINTN);

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

**/
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

**/
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

**/
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

**/
{
  EFI_STATUS                          Status;
  EFI_FW_VOL_INSTANCE                 *FwhInstance = NULL;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_DXE_SERVICES                    *DxeServices;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     Descriptor;
  UINT32                              BufferSize;
  EFI_FV_BLOCK_MAP_ENTRY              *PtrBlockMapEntry;
  EFI_HANDLE                          FwbHandle;
  EFI_FW_VOL_BLOCK_DEVICE             *FvbDevice;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *OldFwbInterface;
  EFI_DEVICE_PATH_PROTOCOL            *TempFwbDevicePath;
  FV_DEVICE_PATH                      TempFvbDevicePathData;
  UINT32                              MaxLbaSize;
  EFI_PHYSICAL_ADDRESS                BaseAddress;
  UINT64                              Length;
  UINTN                               NumOfBlocks;
  EFI_PEI_HOB_POINTERS                FvHob;

   //
  // Get the DXE services table
  //
  DxeServices = gDS;

  //
  // Allocate runtime services data for global variable, which contains
  // the private data of all firmware volume block instances
  //
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  sizeof (ESAL_FWB_GLOBAL),
                  (VOID**) &mFvbModuleGlobal
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Calculate the total size for all firmware volume block instances
  //
  BufferSize            = 0;

  FvHob.Raw = GetHobList ();
  while ((FvHob.Raw = GetNextHob (EFI_HOB_TYPE_FV, FvHob.Raw)) != NULL) {
    BaseAddress = FvHob.FirmwareVolume->BaseAddress;
    Length      = FvHob.FirmwareVolume->Length;
    //
    // Check if it is a "real" flash
    //
    Status = DxeServices->GetMemorySpaceDescriptor (
                            BaseAddress,
                            &Descriptor
                            );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) {
      FvHob.Raw = GET_NEXT_HOB (FvHob);
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) BaseAddress;
    Status      = ValidateFvHeader (FwVolHeader);
    if (EFI_ERROR (Status)) {
      //
      // Get FvbInfo
      //
      Status = GetFvbInfo (Length, &FwVolHeader);
      if (EFI_ERROR (Status)) {
        FvHob.Raw = GET_NEXT_HOB (FvHob);
        continue;
      }
    }

    BufferSize += (sizeof (EFI_FW_VOL_INSTANCE) + FwVolHeader->HeaderLength - sizeof (EFI_FIRMWARE_VOLUME_HEADER));
    FvHob.Raw = GET_NEXT_HOB (FvHob);
  }

  //
  // Only need to allocate once. There is only one copy of physical memory for
  // the private data of each FV instance. But in virtual mode or in physical
  // mode, the address of the the physical memory may be different.
  //
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  BufferSize,
                  (VOID**) &mFvbModuleGlobal->FvInstance[FVB_PHYSICAL]
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Make a virtual copy of the FvInstance pointer.
  //
  FwhInstance = mFvbModuleGlobal->FvInstance[FVB_PHYSICAL];
  mFvbModuleGlobal->FvInstance[FVB_VIRTUAL] = FwhInstance;

  mFvbModuleGlobal->NumFv                   = 0;
  MaxLbaSize = 0;

  FvHob.Raw = GetHobList ();
  while (NULL != (FvHob.Raw = GetNextHob (EFI_HOB_TYPE_FV, FvHob.Raw))) {
    BaseAddress = FvHob.FirmwareVolume->BaseAddress;
    Length      = FvHob.FirmwareVolume->Length;
    //
    // Check if it is a "real" flash
    //
    Status = DxeServices->GetMemorySpaceDescriptor (
                            BaseAddress,
                            &Descriptor
                            );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) {
      FvHob.Raw = GET_NEXT_HOB (FvHob);
      continue;
    }

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) BaseAddress;
    Status      = ValidateFvHeader (FwVolHeader);
    if (EFI_ERROR (Status)) {
      //
      // Get FvbInfo to provide in FwhInstance.
      //
      Status = GetFvbInfo (Length, &FwVolHeader);
      if (EFI_ERROR (Status)) {
        FvHob.Raw = GET_NEXT_HOB (FvHob);
        continue;
      }
      //
      //  Write healthy FV header back.
      //
      CopyMem (
        (VOID *) (UINTN) BaseAddress,
        (VOID *) FwVolHeader,
        FwVolHeader->HeaderLength
        );
    }

    FwhInstance->FvBase[FVB_PHYSICAL] = (UINTN) BaseAddress;
    FwhInstance->FvBase[FVB_VIRTUAL]  = (UINTN) BaseAddress;

    CopyMem ((UINTN *) &(FwhInstance->VolumeHeader), (UINTN *) FwVolHeader, FwVolHeader->HeaderLength);
    FwVolHeader = &(FwhInstance->VolumeHeader);
    EfiInitializeLock (&(FwhInstance->FvbDevLock), TPL_HIGH_LEVEL);

    NumOfBlocks = 0;

    for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
      //
      // Get the maximum size of a block. The size will be used to allocate
      // buffer for Scratch space, the intermediate buffer for FVB extension
      // protocol
      //
      if (MaxLbaSize < PtrBlockMapEntry->Length) {
        MaxLbaSize = PtrBlockMapEntry->Length;
      }

      NumOfBlocks = NumOfBlocks + PtrBlockMapEntry->NumBlocks;
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
                    (VOID**) &FvbDevice
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
                      (VOID**)&OldFwbInterface
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

    FvHob.Raw = GET_NEXT_HOB (FvHob);
  }

  return EFI_SUCCESS;
}

/** @file

  Implement the Firmware Volume Block (FVB) services based on SMM FVB
  module and install FVB protocol.

Copyright (c) 2010  - 2014, Intel Corporation. All rights reserved. <BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include "FvbSmmDxe.h"

EFI_HANDLE                       mHandle           = NULL;
EFI_SMM_COMMUNICATION_PROTOCOL  *mSmmCommunication = NULL;

//
// Template structure used when installing FVB protocol.
//
EFI_FVB_DEVICE    mFvbDeviceTemplate = {
  FVB_DEVICE_SIGNATURE,
  NULL,
  {
    FvbGetAttributes,
    FvbSetAttributes,
    FvbGetPhysicalAddress,
    FvbGetBlockSize,
    FvbRead,
    FvbWrite,
    FvbEraseBlocks,
    NULL
  },
  NULL
};

FV_MEMMAP_DEVICE_PATH mFvMemmapDevicePathTemplate = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      {
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8)
      }
    },
    EfiMemoryMappedIO,
    (EFI_PHYSICAL_ADDRESS) 0,
    (EFI_PHYSICAL_ADDRESS) 0,
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

FV_PIWG_DEVICE_PATH mFvPIWGDevicePathTemplate = {
  {
    {
      MEDIA_DEVICE_PATH,
      MEDIA_PIWG_FW_VOL_DP,
      {
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH)),
        (UINT8)(sizeof (MEDIA_FW_VOL_DEVICE_PATH) >> 8)
      }
    },
    { 0 }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

/**
  Initialize the communicate buffer using DataSize and Function.

  The communicate size is: SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE +
  DataSize.

  @param[out]      CommunicateBuffer The communicate buffer. Caller should free it after use.
  @param[out]      DataPtr           Points to the data in the communicate buffer. Caller should not free it.
  @param[in]       DataSize          The payload size.
  @param[in]       Function          The function number used to initialize the communicate header.

  @retval EFI_INVALID_PARAMETER      The data size is too big.
  @retval EFI_SUCCESS                Find the specified variable.

**/
EFI_STATUS
InitCommunicateBuffer (
  OUT     VOID                              **CommunicateBuffer,
  OUT     VOID                              **DataPtr,
  IN      UINTN                             DataSize,
  IN      UINTN                             Function
  )
{
  EFI_SMM_COMMUNICATE_HEADER                *SmmCommunicateHeader;
  SMM_FVB_COMMUNICATE_FUNCTION_HEADER       *SmmFvbFunctionHeader;

  //
  // The whole buffer size: SMM_COMMUNICATE_HEADER_SIZE + SMM_FVB_COMMUNICATE_HEADER_SIZE + DataSize.
  //
  SmmCommunicateHeader = AllocatePool (DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_FVB_COMMUNICATE_HEADER_SIZE);
  ASSERT (SmmCommunicateHeader != NULL);

  //
  // Prepare data buffer.
  //
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmFirmwareVolumeBlockProtocolGuid);
  SmmCommunicateHeader->MessageLength = DataSize + SMM_FVB_COMMUNICATE_HEADER_SIZE;

  SmmFvbFunctionHeader = (SMM_FVB_COMMUNICATE_FUNCTION_HEADER *) SmmCommunicateHeader->Data;
  SmmFvbFunctionHeader->Function = Function;

  *CommunicateBuffer = SmmCommunicateHeader;
  *DataPtr = SmmFvbFunctionHeader->Data;

  return EFI_SUCCESS;
}


/**
  Send the data in communicate buffer to SMM.

  @param[out]      SmmCommunicateHeader    The communicate buffer.
  @param[in]       DataSize                The payload size.

**/
EFI_STATUS
SendCommunicateBuffer (
  IN      EFI_SMM_COMMUNICATE_HEADER        *SmmCommunicateHeader,
  IN      UINTN                             DataSize
  )
{
  EFI_STATUS                                Status;
  UINTN                                     CommSize;
  SMM_FVB_COMMUNICATE_FUNCTION_HEADER       *SmmFvbFunctionHeader;

  CommSize = DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_FVB_COMMUNICATE_HEADER_SIZE;
  Status = mSmmCommunication->Communicate (
                                mSmmCommunication,
                                SmmCommunicateHeader,
                                &CommSize
                                );
  ASSERT_EFI_ERROR (Status);

  SmmFvbFunctionHeader = (SMM_FVB_COMMUNICATE_FUNCTION_HEADER *) SmmCommunicateHeader->Data;
  return  SmmFvbFunctionHeader->ReturnStatus;
}

/**
  This function retrieves the attributes and current settings of the block.

  @param[in]  This       Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param[out] Attributes Pointer to EFI_FVB_ATTRIBUTES_2 in which the attributes
                         and current settings are returned. Type EFI_FVB_ATTRIBUTES_2
                         is defined in EFI_FIRMWARE_VOLUME_HEADER.

  @retval EFI_SUCCESS              The firmware volume attributes were returned.
  @retval EFI_INVALID_PARAMETER    Attributes is NULL.
**/
EFI_STATUS
EFIAPI
FvbGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
     OUT   EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER                    *SmmCommunicateHeader;
  SMM_FVB_ATTRIBUTES_HEADER                     *SmmFvbAttributesHeader;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *SmmFvb;
  EFI_FVB_DEVICE                                *FvbDevice;

  if (Attributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  SmmFvb    = FvbDevice->SmmFvbInstance;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FVB_ATTRIBUTES_HEADER);
  Status = InitCommunicateBuffer (
             (VOID **)&SmmCommunicateHeader,
             (VOID **)&SmmFvbAttributesHeader,
             PayloadSize,
             EFI_FUNCTION_GET_ATTRIBUTES
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFvbAttributesHeader->SmmFvb     = SmmFvb;
  SmmFvbAttributesHeader->Attributes = 0;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM.
  //
  *Attributes = SmmFvbAttributesHeader->Attributes;
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  Sets Volume attributes. No polarity translations are done.

  @param[in]  This        Calling context.
  @param[out] Attributes  Output buffer which contains attributes.

  @retval     EFI_SUCCESS              Set the Attributes successfully.
  @retval     EFI_INVALID_PARAMETER    Attributes is NULL.

**/
EFI_STATUS
EFIAPI
FvbSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN OUT   EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER                    *SmmCommunicateHeader;
  SMM_FVB_ATTRIBUTES_HEADER                     *SmmFvbAttributesHeader;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *SmmFvb;
  EFI_FVB_DEVICE                                *FvbDevice;

  if (Attributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  SmmFvb    = FvbDevice->SmmFvbInstance;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FVB_ATTRIBUTES_HEADER);
  Status = InitCommunicateBuffer (
             (VOID **)&SmmCommunicateHeader,
             (VOID **)&SmmFvbAttributesHeader,
             PayloadSize,
             EFI_FUNCTION_SET_ATTRIBUTES
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFvbAttributesHeader->SmmFvb     = SmmFvb;
  SmmFvbAttributesHeader->Attributes = *Attributes;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM.
  //
  *Attributes = SmmFvbAttributesHeader->Attributes;
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  Retrieves the physical address of the FVB instance.

  @param[in]  SmmFvb         A pointer to EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL.
  @param[out] Address        Output buffer containing the address.

  @retval     EFI_SUCCESS    Get the address successfully.
  @retval     Others         Failed to get address.

**/
EFI_STATUS
GetPhysicalAddress (
  IN   EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *SmmFvb,
  OUT  EFI_PHYSICAL_ADDRESS                    *Address
  )
{
  EFI_STATUS                                   Status;
  UINTN                                        PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER                   *SmmCommunicateHeader;
  SMM_FVB_PHYSICAL_ADDRESS_HEADER              *SmmFvbPhysicalAddressHeader;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FVB_PHYSICAL_ADDRESS_HEADER);
  Status = InitCommunicateBuffer (
             (VOID **)&SmmCommunicateHeader,
             (VOID **)&SmmFvbPhysicalAddressHeader,
             PayloadSize,
             EFI_FUNCTION_GET_PHYSICAL_ADDRESS
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFvbPhysicalAddressHeader->SmmFvb  = SmmFvb;
  SmmFvbPhysicalAddressHeader->Address = 0;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM.
  //
  *Address = SmmFvbPhysicalAddressHeader->Address;
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  Retrieves the physical address of the FVB instance.

  @param[in]  This                     A pointer to EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL.
  @param[out] Address                  Output buffer containing the address.

  @retval     EFI_SUCCESS              Get the address successfully.
  @retval     Others                   Failed to get the address.

**/
EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
     OUT   EFI_PHYSICAL_ADDRESS                *Address
  )
{
  EFI_STATUS                                   Status;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *SmmFvb;
  EFI_FVB_DEVICE                               *FvbDevice;

  if (Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  SmmFvb    = FvbDevice->SmmFvbInstance;

  Status = GetPhysicalAddress (SmmFvb, Address);

  return Status;
}


/**
  Retrieve the size of a logical block.

  @param[in]  This        Calling context.
  @param[in]  Lba         Indicates which block to return the size for.
  @param[out] BlockSize   A pointer to a caller allocated UINTN in which
                          the size of the block is returned.
  @param[out] NumOfBlocks A pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize.

  @retval     EFI_SUCCESS              Get BlockSize and NumOfBlocks successfully.
  @retval     EFI_INVALID_PARAMETER    BlockSize or NumOfBlocks are NULL.
**/
EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN       EFI_LBA                             Lba,
     OUT   UINTN                               *BlockSize,
     OUT   UINTN                               *NumOfBlocks
  )
{
  EFI_STATUS                                   Status;
  UINTN                                        PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER                   *SmmCommunicateHeader;
  SMM_FVB_BLOCK_SIZE_HEADER                    *SmmFvbBlockSizeHeader;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *SmmFvb;
  EFI_FVB_DEVICE                               *FvbDevice;

  if ((BlockSize == NULL) || (NumOfBlocks == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  SmmFvb    = FvbDevice->SmmFvbInstance;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FVB_BLOCK_SIZE_HEADER);
  Status = InitCommunicateBuffer (
             (VOID **)&SmmCommunicateHeader,
             (VOID **)&SmmFvbBlockSizeHeader,
             PayloadSize,
             EFI_FUNCTION_GET_BLOCK_SIZE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFvbBlockSizeHeader->SmmFvb = SmmFvb;
  SmmFvbBlockSizeHeader->Lba    = Lba;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM.
  //
  *BlockSize   = SmmFvbBlockSizeHeader->BlockSize;
  *NumOfBlocks = SmmFvbBlockSizeHeader->NumOfBlocks;
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  Reads data beginning at Lba:Offset from FV. The Read terminates either
  when *NumBytes of data have been read, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This           Calling context
  @param[in]      Lba            Block in which to begin write
  @param[in]      Offset         Offset in the block at which to begin write
  @param[in,out]  NumBytes       On input, indicates the requested write size. On
                                 output, indicates the actual number of bytes written
  @param[in]      Buffer         Buffer containing source data for the write.

  @retval EFI_SUCCESS            The firmware volume was read successfully and
                                 contents are in Buffer.
  @retval EFI_BAD_BUFFER_SIZE    Read attempted across a LBA boundary. On output,
                                 NumBytes contains the total number of bytes returned
                                 in Buffer.
  @retval EFI_ACCESS_DENIED      The firmware volume is in the ReadDisabled state
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                 could not be read.
  @retval EFI_INVALID_PARAMETER  NumBytes or Buffer are NULL.

**/
EFI_STATUS
EFIAPI
FvbRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN       EFI_LBA                              Lba,
  IN       UINTN                                Offset,
  IN OUT   UINTN                                *NumBytes,
     OUT   UINT8                                *Buffer
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER                    *SmmCommunicateHeader;
  SMM_FVB_READ_WRITE_HEADER                     *SmmFvbReadWriteHeader;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *SmmFvb;
  EFI_FVB_DEVICE                                *FvbDevice;

  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  SmmFvb    = FvbDevice->SmmFvbInstance;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FVB_READ_WRITE_HEADER) + *NumBytes;
  Status = InitCommunicateBuffer (
             (VOID **)&SmmCommunicateHeader,
             (VOID **)&SmmFvbReadWriteHeader,
             PayloadSize, EFI_FUNCTION_READ
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFvbReadWriteHeader->SmmFvb   = SmmFvb;
  SmmFvbReadWriteHeader->Lba      = Lba;
  SmmFvbReadWriteHeader->Offset   = Offset;
  SmmFvbReadWriteHeader->NumBytes = *NumBytes;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM.
  //
  *NumBytes = SmmFvbReadWriteHeader->NumBytes;
  if (!EFI_ERROR (Status)) {
    CopyMem (Buffer, (UINT8 *)(SmmFvbReadWriteHeader + 1), *NumBytes);
  }
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This           Calling context.
  @param[in]      Lba            Block in which to begin write.
  @param[in]      Offset         Offset in the block at which to begin write.
  @param[in,out]  NumBytes       On input, indicates the requested write size. On
                                 output, indicates the actual number of bytes written.
  @param[in]      Buffer         Buffer containing source data for the write.

  @retval EFI_SUCCESS            The firmware volume was written successfully.
  @retval EFI_BAD_BUFFER_SIZE    Write attempted across a LBA boundary. On output,
                                 NumBytes contains the total number of bytes
                                 actually written.
  @retval EFI_ACCESS_DENIED      The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                 could not be written.
  @retval EFI_INVALID_PARAMETER  NumBytes or Buffer are NULL.

**/
EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN       EFI_LBA                              Lba,
  IN       UINTN                                Offset,
  IN OUT   UINTN                                *NumBytes,
  IN       UINT8                                *Buffer
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER                    *SmmCommunicateHeader;
  SMM_FVB_READ_WRITE_HEADER                     *SmmFvbReadWriteHeader;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *SmmFvb;
  EFI_FVB_DEVICE                                *FvbDevice;

  if ((NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  SmmFvb    = FvbDevice->SmmFvbInstance;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FVB_READ_WRITE_HEADER) + *NumBytes;
  Status = InitCommunicateBuffer (
             (VOID **)&SmmCommunicateHeader,
             (VOID **)&SmmFvbReadWriteHeader,
             PayloadSize,
             EFI_FUNCTION_WRITE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFvbReadWriteHeader->SmmFvb   = SmmFvb;
  SmmFvbReadWriteHeader->Lba      = Lba;
  SmmFvbReadWriteHeader->Offset   = Offset;
  SmmFvbReadWriteHeader->NumBytes = *NumBytes;
  CopyMem ((UINT8 *)(SmmFvbReadWriteHeader + 1), Buffer, *NumBytes);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM.
  //
  *NumBytes = SmmFvbReadWriteHeader->NumBytes;
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  The EraseBlock() function erases NumOfLba blocks started from StartingLba.

  @param[in] This            Calling context.
  @param[in] StartingLba     Starting LBA followed to erase.
  @param[in] NumOfLba        Number of block to erase.

  @retval EFI_SUCCESS        The erase request was successfully completed.
  @retval EFI_ACCESS_DENIED  The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR   The block device is not functioning correctly and
                             could not be written. Firmware device may have been
                             partially erased.

**/
EFI_STATUS
EraseBlock (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  IN       EFI_LBA                               StartingLba,
  IN       UINTN                                 NumOfLba
  )
{
  EFI_STATUS                                     Status;
  UINTN                                          PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER                    *SmmCommunicateHeader;
  SMM_FVB_BLOCKS_HEADER                         *SmmFvbBlocksHeader;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *SmmFvb;
  EFI_FVB_DEVICE                                *FvbDevice;

  FvbDevice = FVB_DEVICE_FROM_THIS (This);
  SmmFvb    = FvbDevice->SmmFvbInstance;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize  = sizeof (SMM_FVB_BLOCKS_HEADER);
  Status = InitCommunicateBuffer (
             (VOID **)&SmmCommunicateHeader,
             (VOID **)&SmmFvbBlocksHeader,
             PayloadSize,
             EFI_FUNCTION_ERASE_BLOCKS
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFvbBlocksHeader->SmmFvb   = SmmFvb;
  SmmFvbBlocksHeader->StartLba = StartingLba;
  SmmFvbBlocksHeader->NumOfLba = NumOfLba;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);

  //
  // Get data from SMM.
  //
  FreePool (SmmCommunicateHeader);

  return Status;
}


/**
  The EraseBlocks() function erases one or more blocks as denoted by the
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist
  within the associated firmware volume (it has a larger index than the last
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

  @param[in] This           Calling context/
  @param[in] ...            Starting LBA followed by Number of Lba to erase.
                            a -1 to terminate the list.
/
  @retval EFI_SUCCESS       The erase request was successfully completed
  @retval EFI_ACCESS_DENIED The firmware volume is in the WriteDisabled state/
  @retval EFI_DEVICE_ERROR  The block device is not functioning correctly and
                            could not be written. Firmware device may have been
                            partially erased/

**/
EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  )
{
  EFI_STATUS                                     Status;
  VA_LIST                                        Marker;
  EFI_LBA                                        StartingLba;
  UINTN                                          NumOfLba;

  Status = EFI_SUCCESS;

  //
  // Check the parameter.
  //
  VA_START (Marker, This);
  do {
    StartingLba = VA_ARG (Marker, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR ) {
      break;
    }

    NumOfLba = VA_ARG (Marker, UINT32);
    if (NumOfLba == 0) {
      return EFI_INVALID_PARAMETER;
    }

  } while ( 1 );
  VA_END (Marker);

  //
  // Erase the blocks.
  //
  VA_START (Marker, This);
  do {
    StartingLba = VA_ARG (Marker, EFI_LBA);
    if (StartingLba == EFI_LBA_LIST_TERMINATOR ) {
      break;
    }
    NumOfLba = VA_ARG (Marker, UINT32);
    Status = EraseBlock (This, StartingLba, NumOfLba);
    if (EFI_ERROR (Status)) {
      break;
    }
  } while ( 1 );
  VA_END (Marker);

  return Status;
}


/**
  Install the FVB protocol which based on SMM FVB protocol.

  @param[in] SmmFvb        The SMM FVB protocol.

**/
VOID
InstallFvb (
  IN    EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *SmmFvb
  )
{
  EFI_STATUS                                    Status;
  EFI_HANDLE                                    FvbHandle;
  EFI_FVB_DEVICE                                *FvbDevice;
  EFI_FIRMWARE_VOLUME_HEADER                    *VolumeHeader;
  EFI_PHYSICAL_ADDRESS                          Address;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL            *OldFvbInterface;

  FvbDevice = AllocateRuntimeCopyPool (sizeof (EFI_FVB_DEVICE), &mFvbDeviceTemplate);
  ASSERT (FvbDevice != NULL);
  FvbDevice->SmmFvbInstance = SmmFvb;

  Status = gBS->LocateProtocol (
                  &gEfiSmmCommunicationProtocolGuid,
                  NULL,
                  (VOID **) &mSmmCommunication
                  );
  ASSERT_EFI_ERROR (Status);

  Status = GetPhysicalAddress (SmmFvb, &Address);
  ASSERT_EFI_ERROR (Status);

  VolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN)Address;

  //
  // Set up the devicepath.
  //
  if (VolumeHeader->ExtHeaderOffset == 0) {
    //
    // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH.
    //
    FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_MEMMAP_DEVICE_PATH), &mFvMemmapDevicePathTemplate);
    ((FV_MEMMAP_DEVICE_PATH *) FvbDevice->DevicePath)->MemMapDevPath.StartingAddress = (UINTN)Address;
    ((FV_MEMMAP_DEVICE_PATH *) FvbDevice->DevicePath)->MemMapDevPath.EndingAddress   = (UINTN)Address + VolumeHeader->FvLength - 1;
  } else {
    FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_PIWG_DEVICE_PATH), &mFvPIWGDevicePathTemplate);
    CopyGuid (
      &((FV_PIWG_DEVICE_PATH *)FvbDevice->DevicePath)->FvDevPath.FvName,
      (GUID *)(UINTN)((UINTN)Address + VolumeHeader->ExtHeaderOffset)
      );
  }

  //
  // Find a handle with a matching device path that has supports FW Block protocol.
  //
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  &FvbDevice->DevicePath,
                  &FvbHandle
                  );
  if (EFI_ERROR (Status) ) {
    //
    // LocateDevicePath fails so install a new interface and device path.
    //
    FvbHandle = NULL;
    Status =  gBS->InstallMultipleProtocolInterfaces (
                     &FvbHandle,
                     &gEfiFirmwareVolumeBlockProtocolGuid,
                     &FvbDevice->FvbInstance,
                     &gEfiDevicePathProtocolGuid,
                     FvbDevice->DevicePath,
                     NULL
                     );
    ASSERT_EFI_ERROR (Status);
  } else if (IsDevicePathEnd (FvbDevice->DevicePath)) {
    //
    // Device allready exists, so reinstall the FVB protocol.
    //
    Status = gBS->HandleProtocol (
                    FvbHandle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID **) &OldFvbInterface
                    );
    ASSERT_EFI_ERROR (Status);

    Status =  gBS->ReinstallProtocolInterface (
                     FvbHandle,
                     &gEfiFirmwareVolumeBlockProtocolGuid,
                     OldFvbInterface,
                     &FvbDevice->FvbInstance
                     );
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // There was a FVB protocol on an End Device Path node.
    //
    ASSERT (FALSE);
  }
}


/**
  SMM Firmware Volume Block Protocol notification event handler.

  Discover NV Variable Store and install Variable Write Arch Protocol.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
SmmFvbReady (
  IN  EFI_EVENT                                 Event,
  IN  VOID                                      *Context
  )
{
  EFI_STATUS                                    Status;
  EFI_HANDLE                                    *HandleBuffer;
  UINTN                                         HandleCount;
  UINTN                                         Index;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *SmmFvb;

  //
  // Locate all handles of Smm Fvb protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Install FVB protocol.
  //
  for (Index = 0; Index < HandleCount; Index++) {
    SmmFvb = NULL;
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    (VOID **) &SmmFvb
                    );
    if (EFI_ERROR (Status)) {
      break;
    }

    InstallFvb (SmmFvb);
  }

  FreePool (HandleBuffer);
}


/**
  The driver entry point for Firmware Volume Block Driver.

  The function does the necessary initialization work
  Firmware Volume Block Driver.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       This funtion always return EFI_SUCCESS.
                                It will ASSERT on errors.

**/
EFI_STATUS
EFIAPI
FvbSmmDxeInitialize (
  IN EFI_HANDLE                                 ImageHandle,
  IN EFI_SYSTEM_TABLE                           *SystemTable
  )
{
  VOID                                          *SmmFvbRegistration;

  //
  // Smm FVB driver is ready.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
    TPL_CALLBACK,
    SmmFvbReady,
    NULL,
    &SmmFvbRegistration
    );

  return EFI_SUCCESS;
}


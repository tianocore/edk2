/** @file

  This is a simple fault tolerant write driver.

  This boot service protocol only provides fault tolerant write capability for 
  block devices.  The protocol has internal non-volatile intermediate storage 
  of the data and private information. It should be able to recover 
  automatically from a critical fault, such as power failure. 

  The implementation uses an FTW (Fault Tolerant Write) Work Space. 
  This work space is a memory copy of the work space on the Working Block,
  the size of the work space is the FTW_WORK_SPACE_SIZE bytes.
  
  The work space stores each write record as EFI_FTW_RECORD structure.
  The spare block stores the write buffer before write to the target block.
  
  The write record has three states to specify the different phase of write operation.
  1) WRITE_ALLOCATED is that the record is allocated in write space.
     The information of write operation is stored in write record structure.
  2) SPARE_COMPLETED is that the data from write buffer is writed into the spare block as the backup.
  3) WRITE_COMPLETED is that the data is copied from the spare block to the target block.

  This driver operates the data as the whole size of spare block.
  It first read the SpareAreaLength data from the target block into the spare memory buffer.
  Then copy the write buffer data into the spare memory buffer.
  Then write the spare memory buffer into the spare block.
  Final copy the data from the spare block to the target block.

  To make this drive work well, the following conditions must be satisfied:
  1. The write NumBytes data must be fit within Spare area. 
     Offset + NumBytes <= SpareAreaLength
  2. The whole flash range has the same block size.
  3. Working block is an area which contains working space in its last block and has the same size as spare block.
  4. Working Block area must be in the single one Firmware Volume Block range which FVB protocol is produced on.  
  5. Spare area must be in the single one Firmware Volume Block range which FVB protocol is produced on.
  6. Any write data area (SpareAreaLength Area) which the data will be written into must be 
     in the single one Firmware Volume Block range which FVB protocol is produced on.
  7. If write data area (such as Variable range) is enlarged, the spare area range must be enlarged.
     The spare area must be enough large to store the write data before write them into the target range.
  If one of them is not satisfied, FtwWrite may fail.
  Usually, Spare area only takes one block. That's SpareAreaLength = BlockSize, NumberOfSpareBlock = 1.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#include "FaultTolerantWrite.h"
EFI_EVENT                                 mFvbRegistration = NULL;


/**
  Retrive the FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.
  
**/
EFI_STATUS
FtwGetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the FVB protocol interface on the handle
  //
  return gBS->HandleProtocol (
                FvBlockHandle,
                &gEfiFirmwareVolumeBlockProtocolGuid,
                (VOID **) FvBlock
                );
}

/**
  Retrive the Swap Address Range protocol interface.

  @param[out] SarProtocol       The interface of SAR protocol

  @retval EFI_SUCCESS           The SAR protocol instance was found and returned in SarProtocol.
  @retval EFI_NOT_FOUND         The SAR protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
FtwGetSarProtocol (
  OUT VOID                                **SarProtocol
  )
{
  EFI_STATUS                              Status;

  //
  // Locate Swap Address Range protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSwapAddressRangeProtocolGuid, 
                  NULL, 
                  SarProtocol
                  );
  return Status;
}

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
  )
{
  EFI_STATUS                              Status;

  //
  // Locate all handles of Fvb protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  NULL,
                  NumberHandles,
                  Buffer
                  );
  return Status;
}


/**
  Firmware Volume Block Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
FvbNotificationEvent (
  IN  EFI_EVENT                           Event,
  IN  VOID                                *Context
  )
{
  EFI_STATUS                              Status;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL       *FtwProtocol;
  EFI_FTW_DEVICE                          *FtwDevice;

  //
  // Just return to avoid installing FaultTolerantWriteProtocol again
  // if Fault Tolerant Write protocol has been installed.
  //  
  Status = gBS->LocateProtocol (
                  &gEfiFaultTolerantWriteProtocolGuid, 
                  NULL, 
                  (VOID **) &FtwProtocol
                  );
  if (!EFI_ERROR (Status)) {
    return ;
  }

  //
  // Found proper FVB protocol and initialize FtwDevice for protocol installation
  //
  FtwDevice = (EFI_FTW_DEVICE *)Context;
  Status = InitFtwProtocol (FtwDevice);
  if (EFI_ERROR(Status)) {
    return ;
  }                          
    
  //
  // Install protocol interface
  //
  Status = gBS->InstallProtocolInterface (
                  &FtwDevice->Handle,
                  &gEfiFaultTolerantWriteProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FtwDevice->FtwInstance
                  );
  ASSERT_EFI_ERROR (Status);
  
  Status = gBS->CloseEvent (Event);
  ASSERT_EFI_ERROR (Status);
  
  return;
}


/**
  This function is the entry point of the Fault Tolerant Write driver.

  @param[in] ImageHandle        A handle for the image that is initializing this driver
  @param[in] SystemTable        A pointer to the EFI system table

  @retval EFI_SUCCESS           The initialization finished successfully.
  @retval EFI_OUT_OF_RESOURCES  Allocate memory error
  @retval EFI_INVALID_PARAMETER Workspace or Spare block does not exist
  
**/
EFI_STATUS
EFIAPI
FaultTolerantWriteInitialize (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS                              Status;
  EFI_FTW_DEVICE                          *FtwDevice;

  //
  // Allocate private data structure for FTW protocol and do some initialization
  //
  Status = InitFtwDevice (&FtwDevice);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Register FvbNotificationEvent () notify function.
  // 
  EfiCreateProtocolNotifyEvent (
    &gEfiFirmwareVolumeBlockProtocolGuid,
    TPL_CALLBACK,
    FvbNotificationEvent,
    (VOID *)FtwDevice,
    &mFvbRegistration
    );
  
  return EFI_SUCCESS;
}

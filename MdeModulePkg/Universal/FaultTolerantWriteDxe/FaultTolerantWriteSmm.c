/** @file

  This is a simple fault tolerant write driver that is intended to use in the SMM environment.

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

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmSwapAddressRange.h>
#include "FaultTolerantWrite.h"
#include "FaultTolerantWriteSmmCommon.h"

EFI_EVENT                                 mFvbRegistration = NULL;
EFI_FTW_DEVICE                            *mFtwDevice      = NULL;

/**
  Retrive the SMM FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of SMM FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of SMM FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the SMM FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
FtwGetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the SMM FVB protocol interface on the handle
  //
  return gSmst->SmmHandleProtocol (
                  FvBlockHandle,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  (VOID **) FvBlock
                  );
}

/**
  Retrive the SMM Swap Address Range protocol interface.

  @param[out] SarProtocol       The interface of SMM SAR protocol

  @retval EFI_SUCCESS           The SMM SAR protocol instance was found and returned in SarProtocol.
  @retval EFI_NOT_FOUND         The SMM SAR protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
FtwGetSarProtocol (
  OUT VOID                                **SarProtocol
  )
{
  EFI_STATUS                              Status;

  //
  // Locate Smm Swap Address Range protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwapAddressRangeProtocolGuid, 
                    NULL, 
                    SarProtocol
                    );
  return Status;
}

/**
  Function returns an array of handles that support the SMM FVB protocol
  in a buffer allocated from pool. 

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support SMM FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No SMM FVB handle was found.
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
  UINTN                                   BufferSize;

  if ((NumberHandles == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize     = 0;
  *NumberHandles = 0;
  *Buffer        = NULL;
  Status = gSmst->SmmLocateHandle (
                    ByProtocol,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    NULL,
                    &BufferSize,
                    *Buffer
                    );
  if (EFI_ERROR(Status) && Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_FOUND;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gSmst->SmmLocateHandle (
                    ByProtocol,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    NULL,
                    &BufferSize,
                    *Buffer
                    );

  *NumberHandles = BufferSize / sizeof(EFI_HANDLE);
  if (EFI_ERROR(Status)) {
    *NumberHandles = 0;
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return Status;
}


/**
  Get the handle of the SMM FVB protocol by the FVB base address and attributes.

  @param[in]  Address       The base address of SMM FVB protocol.
  @param[in]  Attributes    The attributes of the SMM FVB protocol.
  @param[out] SmmFvbHandle  The handle of the SMM FVB protocol.

  @retval  EFI_SUCCESS    The FVB handle is found.
  @retval  EFI_ABORTED    The FVB protocol is not found.

**/
EFI_STATUS
GetFvbByAddressAndAttribute (
  IN  EFI_PHYSICAL_ADDRESS            Address,
  IN  EFI_FVB_ATTRIBUTES_2            Attributes,
  OUT EFI_HANDLE                      *SmmFvbHandle
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FVB_ATTRIBUTES_2                FvbAttributes;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;

  //
  // Locate all handles of SMM Fvb protocol.
  //
  Status = GetFvbCountAndBuffer (&HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  
  //
  // Find the proper SMM Fvb handle by the address and attributes.
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = FtwGetFvbByHandle (HandleBuffer[Index], &Fvb);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Compare the address.
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }
    if (Address != FvbBaseAddress) {
     continue;
    }

    //
    // Compare the attribute.
    //
    Status = Fvb->GetAttributes (Fvb, &FvbAttributes);
    if (EFI_ERROR (Status)) {
      continue;
    }
    if (Attributes != FvbAttributes) {
     continue;
    }

    //
    // Found the proper FVB handle.
    //
    *SmmFvbHandle = HandleBuffer[Index];
    FreePool (HandleBuffer);
    return EFI_SUCCESS;
  }

  FreePool (HandleBuffer);
  return EFI_ABORTED;
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for the fault tolerant write wrapper driver.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will be conveyed
                                 from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers 
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should 
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still 
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
  
**/
EFI_STATUS
EFIAPI
SmmFaultTolerantWriteHandler (
  IN     EFI_HANDLE                                DispatchHandle,
  IN     CONST VOID                                *RegisterContext,
  IN OUT VOID                                      *CommBuffer,
  IN OUT UINTN                                     *CommBufferSize
  )
{
  EFI_STATUS                                       Status;
  SMM_FTW_COMMUNICATE_FUNCTION_HEADER              *SmmFtwFunctionHeader;
  SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER                *SmmGetMaxBlockSizeHeader;
  SMM_FTW_ALLOCATE_HEADER                          *SmmFtwAllocateHeader;
  SMM_FTW_WRITE_HEADER                             *SmmFtwWriteHeader;
  SMM_FTW_RESTART_HEADER                           *SmmFtwRestartHeader;
  SMM_FTW_GET_LAST_WRITE_HEADER                    *SmmFtwGetLastWriteHeader;
  VOID                                             *PrivateData;
  EFI_HANDLE                                       SmmFvbHandle;

  ASSERT (CommBuffer != NULL);
  ASSERT (CommBufferSize != NULL);

  SmmFtwFunctionHeader = (SMM_FTW_COMMUNICATE_FUNCTION_HEADER *)CommBuffer;
  switch (SmmFtwFunctionHeader->Function) {
    case FTW_FUNCTION_GET_MAX_BLOCK_SIZE:
      SmmGetMaxBlockSizeHeader = (SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER *) SmmFtwFunctionHeader->Data;     
      Status = FtwGetMaxBlockSize (
                 &mFtwDevice->FtwInstance,
                 &SmmGetMaxBlockSizeHeader->BlockSize
                 );
      break;
      
    case FTW_FUNCTION_ALLOCATE:
      SmmFtwAllocateHeader = (SMM_FTW_ALLOCATE_HEADER *) SmmFtwFunctionHeader->Data;
      Status = FtwAllocate (
                 &mFtwDevice->FtwInstance,
                 &SmmFtwAllocateHeader->CallerId,
                 SmmFtwAllocateHeader->PrivateDataSize,
                 SmmFtwAllocateHeader->NumberOfWrites
                 );
      break;
      
    case FTW_FUNCTION_WRITE:
      SmmFtwWriteHeader = (SMM_FTW_WRITE_HEADER *) SmmFtwFunctionHeader->Data;
      if (SmmFtwWriteHeader->PrivateDataSize == 0) {
        PrivateData = NULL;
      } else {
        PrivateData = (VOID *)&SmmFtwWriteHeader->Data[SmmFtwWriteHeader->Length];
      }
      Status = GetFvbByAddressAndAttribute (
                 SmmFtwWriteHeader->FvbBaseAddress, 
                 SmmFtwWriteHeader->FvbAttributes,
                 &SmmFvbHandle
                 );
      if (!EFI_ERROR (Status)) {
        Status = FtwWrite(
                   &mFtwDevice->FtwInstance,
                   SmmFtwWriteHeader->Lba,
                   SmmFtwWriteHeader->Offset,
                   SmmFtwWriteHeader->Length,
                   PrivateData,
                   SmmFvbHandle,
                   SmmFtwWriteHeader->Data
                   );
      }
      break;
      
    case FTW_FUNCTION_RESTART:
      SmmFtwRestartHeader = (SMM_FTW_RESTART_HEADER *) SmmFtwFunctionHeader->Data;
      Status = GetFvbByAddressAndAttribute (
                 SmmFtwRestartHeader->FvbBaseAddress, 
                 SmmFtwRestartHeader->FvbAttributes,
                 &SmmFvbHandle
                 );      
      if (!EFI_ERROR (Status)) {
        Status = FtwRestart (&mFtwDevice->FtwInstance, SmmFvbHandle);
      }
      break;

    case FTW_FUNCTION_ABORT:
      Status = FtwAbort (&mFtwDevice->FtwInstance);
      break;
      
    case FTW_FUNCTION_GET_LAST_WRITE:
      SmmFtwGetLastWriteHeader = (SMM_FTW_GET_LAST_WRITE_HEADER *) SmmFtwFunctionHeader->Data;
      Status = FtwGetLastWrite (
                 &mFtwDevice->FtwInstance,
                 &SmmFtwGetLastWriteHeader->CallerId,
                 &SmmFtwGetLastWriteHeader->Lba,
                 &SmmFtwGetLastWriteHeader->Offset,
                 &SmmFtwGetLastWriteHeader->Length,
                 &SmmFtwGetLastWriteHeader->PrivateDataSize,
                 (VOID *)SmmFtwGetLastWriteHeader->Data,
                 &SmmFtwGetLastWriteHeader->Complete
                 );
      break;

    default:
      ASSERT (FALSE);
      Status = EFI_UNSUPPORTED;
  }

  SmmFtwFunctionHeader->ReturnStatus = Status;

  return EFI_SUCCESS;
}


/**
  SMM Firmware Volume Block Protocol notification event handler.
  
  @param[in]  Protocol      Points to the protocol's unique identifier
  @param[in]  Interface     Points to the interface instance
  @param[in]  Handle        The handle on which the interface was installed

  @retval EFI_SUCCESS       SmmEventCallback runs successfully
  
 **/
EFI_STATUS
EFIAPI
FvbNotificationEvent (
  IN CONST EFI_GUID                       *Protocol,
  IN VOID                                 *Interface,
  IN EFI_HANDLE                           Handle
  )
{
  EFI_STATUS                              Status;
  EFI_SMM_FAULT_TOLERANT_WRITE_PROTOCOL   *FtwProtocol;
  EFI_HANDLE                              SmmFtwHandle;
  
  //
  // Just return to avoid install SMM FaultTolerantWriteProtocol again
  // if SMM Fault Tolerant Write protocol had been installed.
  //  
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmFaultTolerantWriteProtocolGuid, 
                    NULL, 
                    (VOID **) &FtwProtocol
                    );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // Found proper FVB protocol and initialize FtwDevice for protocol installation
  //
  Status = InitFtwProtocol (mFtwDevice);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  
  //
  // Install protocol interface
  //
  Status = gSmst->SmmInstallProtocolInterface (
                    &mFtwDevice->Handle,
                    &gEfiSmmFaultTolerantWriteProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mFtwDevice->FtwInstance
                    );
  ASSERT_EFI_ERROR (Status); 

  //
  // Notify the Ftw wrapper driver SMM Ftw is ready
  //
  SmmFtwHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &SmmFtwHandle,
                  &gEfiSmmFaultTolerantWriteProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
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
SmmFaultTolerantWriteInitialize (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS                              Status;
  EFI_HANDLE                              FtwHandle;
  
  //
  // Allocate private data structure for SMM FTW protocol and do some initialization
  //
  Status = InitFtwDevice (&mFtwDevice);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  
  //
  // Register FvbNotificationEvent () notify function.
  // 
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    FvbNotificationEvent,
                    &mFvbRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  FvbNotificationEvent (NULL, NULL, NULL);

  ///
  /// Register SMM FTW SMI handler
  ///
  Status = gSmst->SmiHandlerRegister (SmmFaultTolerantWriteHandler, &gEfiSmmFaultTolerantWriteProtocolGuid, &FtwHandle);
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}

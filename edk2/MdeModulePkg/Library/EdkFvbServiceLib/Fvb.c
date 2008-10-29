/**@file

  Firmware Volume Block Protocol Runtime Interface Abstraction
  And FVB Extension protocol Runtime Interface Abstraction

  mFvbEntry is an array of Handle Fvb pairs. The Fvb Lib Instance matches the
  index in the mFvbEntry array. This should be the same sequence as the FVB's
  were described in the HOB. We have to remember the handle so we can tell if
  the protocol has been reinstalled and it needs updateing.

  If you are using any of these lib functions.you must first call FvbInitialize ().

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Fvb.h"

//
// Event for Set Virtual Map Changed Event
//
STATIC EFI_EVENT mSetVirtualMapChangedEvent = NULL;

//
// Lib will ASSERT if more FVB devices than this are added to the system.
//
STATIC FVB_ENTRY          *mFvbEntry;
STATIC EFI_EVENT          mFvbRegistration;
STATIC UINTN              mFvbCount;

/**
  Check whether an address is runtime memory or not.

  @param    Address   The Address being checked.

  @retval   TRUE      The address is runtime memory.
  @retval   FALSE     The address is not runtime memory.
**/
BOOLEAN
IsRuntimeMemory (
  IN VOID   *Address
  )
{
  EFI_STATUS                           Status;
  UINT8                                TmpMemoryMap[1];
  UINTN                                MapKey;
  UINTN                                DescriptorSize;
  UINT32                               DescriptorVersion;
  UINTN                                MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR                *MemoryMap;
  EFI_MEMORY_DESCRIPTOR                *MemoryMapPtr;
  BOOLEAN                              IsRuntime;
  UINTN                                Index;

  IsRuntime = FALSE;

  //
  // Get System MemoryMapSize
  //
  MemoryMapSize = 1;
  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  (EFI_MEMORY_DESCRIPTOR *)TmpMemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  //
  // Enlarge space here, because we will allocate pool now.
  //
  MemoryMapSize += EFI_PAGE_SIZE;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  MemoryMapSize,
                  (VOID**)&MemoryMap
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Get System MemoryMap
  //
  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  ASSERT_EFI_ERROR (Status);

  MemoryMapPtr = MemoryMap;
  //
  // Search the request Address
  //
  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
    if (((EFI_PHYSICAL_ADDRESS)(UINTN)Address >= MemoryMap->PhysicalStart) &&
        ((EFI_PHYSICAL_ADDRESS)(UINTN)Address < MemoryMap->PhysicalStart
                                              + LShiftU64 (MemoryMap->NumberOfPages, EFI_PAGE_SHIFT))) {
      //
      // Found it
      //
      if (MemoryMap->Attribute & EFI_MEMORY_RUNTIME) {
        IsRuntime = TRUE;
      }
      break;
    }
    //
    // Get next item
    //
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)((UINTN)MemoryMap + DescriptorSize);
  }

  //
  // Done
  //
  gBS->FreePool (MemoryMapPtr);

  return IsRuntime;
}

/**
  Update mFvbEntry. Add new entry, or update existing entry if Fvb protocol is
  reinstalled.

  @param Event      The Event that is being processed
  @param Context    Event Context

**/
STATIC
VOID
EFIAPI
FvbNotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  EFI_HANDLE  Handle;
  UINTN       Index;
  UINTN       UpdateIndex;

  while (TRUE) {
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    mFvbRegistration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // Exit Path of While Loop....
      //
      break;
    }

    UpdateIndex = MAX_FVB_COUNT;
    for (Index = 0; Index < mFvbCount; Index++) {
      if (mFvbEntry[Index].Handle == Handle) {
        //
        //  If the handle is already in the table just update the protocol
        //
        UpdateIndex = Index;
        break;
      }
    }

    if (UpdateIndex == MAX_FVB_COUNT) {
      //
      // Use the next free slot for a new entry
      //
      UpdateIndex                   = mFvbCount++;
      //
      // Check the UpdateIndex whether exceed the maximum value.
      //
      ASSERT (UpdateIndex < MAX_FVB_COUNT);
      mFvbEntry[UpdateIndex].Handle = Handle;
    }
    //
    // The array does not have enough entries
    //
    ASSERT (UpdateIndex < MAX_FVB_COUNT);

    //
    //  Get the interface pointer and if it's ours, skip it
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID **) &mFvbEntry[UpdateIndex].Fvb
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiFvbExtensionProtocolGuid,
                    (VOID **) &mFvbEntry[UpdateIndex].FvbExtension
                    );
    if (Status != EFI_SUCCESS) {
      mFvbEntry[UpdateIndex].FvbExtension = NULL;
    }

    //
    // Check the FVB can be accessed in RUNTIME, The FVBs in FVB handle list comes
    // from two way:
    // 1) Dxe Core. (FVB information is transferred from FV HOB).
    // 2) FVB driver.
    // The FVB produced Dxe core is used for discoverying DXE driver and dispatch. These
    // FVBs can only be accessed in boot time.
    // FVB driver will discovery all FV in FLASH and these FVBs can be accessed in runtime.
    // The FVB itself produced by FVB driver is allocated in runtime memory. So we can
    // determine the what FVB can be accessed in RUNTIME by judging whether FVB itself is allocated
    // in RUNTIME memory.
    //
    mFvbEntry[UpdateIndex].IsRuntimeAccess = IsRuntimeMemory (mFvbEntry[UpdateIndex].Fvb);
  }
}

/**
  Convert all pointers in mFvbEntry after ExitBootServices.

  @param Event      The Event that is being processed
  @param Context    Event Context

**/
VOID
EFIAPI
FvbVirtualAddressChangeNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  UINTN Index;
  if (mFvbEntry != NULL) {
    for (Index = 0; Index < MAX_FVB_COUNT; Index++) {
      if (!mFvbEntry[Index].IsRuntimeAccess) {
        continue;
      }

      if (NULL != mFvbEntry[Index].Fvb) {
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->GetBlockSize);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->GetPhysicalAddress);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->GetAttributes);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->SetAttributes);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->Read);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->Write);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->EraseBlocks);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb);
      }

      if (NULL != mFvbEntry[Index].FvbExtension) {
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].FvbExtension->EraseFvbCustomBlock);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].FvbExtension);
      }
    }

    EfiConvertPointer (0x0, (VOID **) &mFvbEntry);
  }
}

/**
  Library constructor function entry.

  @param ImageHandle    The handle of image who call this libary.
  @param SystemTable    The point of System Table.

  @retval EFI_SUCESS    Sucess construct this library.
  @retval Others        Fail to contruct this libary.
**/
EFI_STATUS
EFIAPI
FvbLibInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  UINTN Status;
  mFvbCount = 0;

  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  (UINTN) sizeof (FVB_ENTRY) * MAX_FVB_COUNT,
                  (VOID *) &mFvbEntry
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (mFvbEntry, sizeof (FVB_ENTRY) * MAX_FVB_COUNT);

  EfiCreateProtocolNotifyEvent (
    &gEfiFirmwareVolumeBlockProtocolGuid,
    TPL_CALLBACK,
    FvbNotificationEvent,
    NULL,
    &mFvbRegistration
    );

  //
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  FvbVirtualAddressChangeNotifyEvent,
                  NULL,
                  &mSetVirtualMapChangedEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

//
// =============================================================================
// The following functions wrap Fvb protocol in the Runtime Lib functions.
// The Instance translates into Fvb instance. The Fvb order defined by HOBs and
// thus the sequence of FVB protocol addition define Instance.
//
// EfiFvbInitialize () must be called before any of the following functions
// must be called.
// =============================================================================
//

/**
  Reads specified number of bytes into a buffer from the specified block.

  The EfiFvbReadBlock() function reads the requested number of bytes from
  the requested block in the specified firmware volume and stores them in
  the provided buffer. Implementations should be mindful that the firmware
  volume might be in the ReadDisabled state.  If it is in this state, the 
  EfiFvbReadBlock() function must return the status code EFI_ACCESS_DENIED
  without modifying the contents of the buffer.
  
  The EfiFvbReadBlock() function must also prevent spanning block boundaries.
  If a read is requested that would span a block boundary, the read must read
  up to the boundary but not beyond.  The output parameter NumBytes must be
  set to correctly indicate the number of bytes actually read.  
  The caller must be aware that a read may be partially completed.

  If NumBytes is NULL, then ASSERT().

  If Buffer is NULL, then ASSERT().

  @param[in]      Instance         The FV instance to be read from.
  @param[in]      Lba              The logical block address to be read from
  @param[in]      Offset           The offset relative to the block, at which to begin reading.
  @param[in, out] NumBytes         Pointer to a UINTN. On input, *NumBytes contains the total
                                   size of the buffer. On output, it contains the actual number
                                   of bytes read.
  @param[out]     Buffer           Pointer to a caller allocated buffer that will be
                                   used to hold the data read.

  @retval   EFI_SUCCESS	           The firmware volume was read successfully and contents are in Buffer.
  @retval   EFI_BAD_BUFFER_SIZE    Read attempted across an LBA boundary.  On output, NumBytes contains the total number of bytes returned in Buffer.
  @retval   EFI_ACCESS_DENIED      The firmware volume is in the ReadDisabled state.
  @retval   EFI_DEVICE_ERROR       The block device is not functioning correctly and could not be read.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter, Instance is larger than the max FVB number. Lba index is larger than the last block of the firmware volume. Offset is larger than the block size.

**/
EFI_STATUS
EfiFvbReadBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  OUT UINT8                                       *Buffer
  )
{
  ASSERT (NumBytes != NULL);
  ASSERT (Buffer != NULL);
  
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->Read (mFvbEntry[Instance].Fvb, Lba, Offset, NumBytes, Buffer);
}

/**
  Writes specified number of bytes from the input buffer to the block

  The EfiFvbWriteBlock() function writes the specified number of bytes
  from the provided buffer to the specified block and offset in the 
  requested firmware volume. 

  If the firmware volume is sticky write, the caller must ensure that
  all the bits of the specified range to write are in the EFI_FVB_ERASE_POLARITY
  state before calling the EfiFvbWriteBlock() function, or else the 
  result will be unpredictable.  This unpredictability arises because,
  for a sticky-write firmware volume, a write may negate a bit in the 
  EFI_FVB_ERASE_POLARITY state but it cannot flip it back again. In 
  general, before calling the EfiFvbWriteBlock() function, the caller
  should call the EfiFvbEraseBlock() function first to erase the specified
  block to write. A block erase cycle will transition bits from the
  (NOT)EFI_FVB_ERASE_POLARITY state back to the EFI_FVB_ERASE_POLARITY state.
  Implementations should be mindful that the firmware volume might be 
  in the WriteDisabled state.  If it is in this state, the EfiFvbWriteBlock()
  function must return the status code EFI_ACCESS_DENIED without modifying
  the contents of the firmware volume.
  
  The EfiFvbWriteBlock() function must also prevent spanning block boundaries.
  If a write is requested that spans a block boundary, the write must store
  up to the boundary but not beyond. The output parameter NumBytes must be 
  set to correctly indicate the number of bytes actually written. The caller
  must be aware that a write may be partially completed.
  All writes, partial or otherwise, must be fully flushed to the hardware 
  before the EfiFvbWriteBlock() function returns. 
  
  If NumBytes is NULL, then ASSERT().

  @param Instance               The FV instance to be written to
  @param Lba                    The starting logical block index to write to
  @param Offset                 The offset relative to the block, at which to begin writting.
  @param NumBytes               Pointer to a UINTN. On input, *NumBytes contains
                                the total size of the buffer. On output, it contains
                                the actual number of bytes written.
  @param Buffer                 Pointer to a caller allocated buffer that contains
                                the source for the write

  @retval EFI_SUCCESS           The firmware volume was written successfully.
  @retval EFI_BAD_BUFFER_SIZE   The write was attempted across an LBA boundary. 
                                On output, NumBytes contains the total number of bytes actually written.
  @retval EFI_ACCESS_DENIED	    The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR      The block device is malfunctioning and could not be written.
  @retval EFI_INVALID_PARAMETER Invalid parameter, Instance is larger than the max FVB number. 
                                Lba index is larger than the last block of the firmware volume.
                                Offset is larger than the block size.
**/
EFI_STATUS
EfiFvbWriteBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
{
  ASSERT (NumBytes != NULL);
  
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->Write (mFvbEntry[Instance].Fvb, Lba, Offset, NumBytes, Buffer);
}

/**
  Erases and initializes a firmware volume block.

  The EfiFvbEraseBlock() function erases one block specified by Lba.
  Implementations should be mindful that the firmware volume might 
  be in the WriteDisabled state. If it is in this state, the EfiFvbEraseBlock()
  function must return the status code EFI_ACCESS_DENIED without 
  modifying the contents of the firmware volume. If Instance is 
  larger than the max FVB number, or Lba index is larger than the
  last block of the firmware volume, this function return the status
  code EFI_INVALID_PARAMETER.
  
  All calls to EfiFvbEraseBlock() must be fully flushed to the 
  hardware before this function returns. 

  @param[in]     Instance    The FV instance to be erased.
  @param[in]     Lba         The logical block index to be erased from.
  
  @retval EFI_SUCCESS            The erase request was successfully completed.
  @retval EFI_ACCESS_DENIED      The firmware volume is in the WriteDisabled state.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                 could not be written.  The firmware device may 
                                 have been partially erased.
  @retval EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max
                                 FVB number. Lba index is larger than the last block
                                 of the firmware volume. 

**/
EFI_STATUS
EfiFvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba
  )
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->EraseBlocks (mFvbEntry[Instance].Fvb, Lba, 1, EFI_LBA_LIST_TERMINATOR);
}

/**
  Retrieves the attributes and current settings of the specified block, 
  returns resulting attributes in output parameter.

  The EfiFvbGetAttributes() function retrieves the attributes and current
  settings of the block specified by Instance. If Instance is larger than
  the max FVB number, this function returns the status code EFI_INVALID_PARAMETER.

  If Attributes is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[out]    Attributes        Pointer to EFI_FVB_ATTRIBUTES_2 in which the
                                   attributes and current settings are returned.

  @retval   EFI_EFI_SUCCESS        The firmware volume attributes were returned.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number. 
**/
EFI_STATUS
EfiFvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES_2                *Attributes
  )
{
  ASSERT (Attributes != NULL);
  
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->GetAttributes (mFvbEntry[Instance].Fvb, Attributes);
}

/**
  Modify the attributes and current settings of the specified block
  according to the input parameter.

  The EfiFvbSetAttributes() function sets configurable firmware volume
  attributes and returns the new settings of the firmware volume specified
  by Instance. If Instance is larger than the max FVB number, this function
  returns the status code EFI_INVALID_PARAMETER.

  If Attributes is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[in, out]Attributes        On input, Attributes is a pointer to EFI_FVB_ATTRIBUTES_2
                                   that contains the desired firmware volume settings.  
                                   On successful return, it contains the new settings of the firmware volume.

  @retval   EFI_EFI_SUCCESS        The firmware volume attributes were modified successfully.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number.

**/
EFI_STATUS
EfiFvbSetVolumeAttributes (
  IN     UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  ASSERT (Attributes != NULL);
  
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->SetAttributes (mFvbEntry[Instance].Fvb, Attributes);
}

/**
  Retrieves the physical address of the specified memory mapped FV.

  Retrieve the base address of a memory-mapped firmware volume specified by Instance.
  If Instance is larger than the max FVB number, this function returns the status 
  code EFI_INVALID_PARAMETER.
  
  If BaseAddress is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[out]    BaseAddress       Pointer to a caller allocated EFI_PHYSICAL_ADDRESS 
                                   that on successful return, contains the base address
                                   of the firmware volume. 

  @retval   EFI_EFI_SUCCESS        The firmware volume base address is returned.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number. 

**/
EFI_STATUS
EfiFvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *BaseAddress
  )
{
  ASSERT (BaseAddress != NULL);
  
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->GetPhysicalAddress (mFvbEntry[Instance].Fvb, BaseAddress);
}

/**
  Retrieve the block size of the specified fv.
  
  The EfiFvbGetBlockSize() function retrieves the size of the requested block. 
  It also returns the number of additional blocks with the identical size. 
  If Instance is larger than the max FVB number, or Lba index is larger than
  the last block of the firmware volume, this function return the status code
  EFI_INVALID_PARAMETER.

  If BlockSize	is NULL, then ASSERT().
  
  If NumOfBlocks  is NULL, then ASSERT().

  @param[in]     Instance          The FV instance to be operated.
  @param[in]     Lba               Indicates which block to return the size for.
  @param[out]    BlockSize         Pointer to a caller-allocated UINTN in which the
                                   size of the block is returned.
  @param[out]    NumOfBlocks       Pointer to a caller-allocated UINTN in which the 
                                   number of consecutive blocks, starting with Lba, 
                                   is returned. All blocks in this range have a size of BlockSize.

  @retval   EFI_EFI_SUCCESS        The firmware volume base address is returned.
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number.
                                   Lba index is larger than the last block of the firmware volume.

**/
EFI_STATUS
EfiFvbGetBlockSize (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  )
{
  ASSERT (BlockSize != NULL);
  ASSERT (NumOfBlocks != NULL);
  
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->GetBlockSize (mFvbEntry[Instance].Fvb, Lba, BlockSize, NumOfBlocks);
}

/**
  Erases and initializes a specified range of a firmware volume.

  The EfiFvbEraseCustomBlockRange() function erases the specified range in the firmware
  volume index by Instance. If Instance is larger than the max FVB number, StartLba or 
  LastLba  index is larger than the last block of the firmware volume, StartLba > LastLba
  or StartLba equal to LastLba but OffsetStartLba > OffsetLastLba, this function return 
  the status code EFI_INVALID_PARAMETER.

  @param[in]     Instance          The FV instance to be operated.
  @param[in]     StartLba          The starting logical block index to be erased.
  @param[in]     OffsetStartLba    Offset into the starting block at which to 
                                   begin erasing.    
  @param[in]     LastLba           The last logical block index to be erased.
  @param[in]     OffsetLastLba     Offset into the last block at which to end erasing.   

  @retval   EFI_EFI_SUCCESS        Successfully erase custom block range
  @retval   EFI_INVALID_PARAMETER  Invalid parameter. Instance is larger than the max FVB number. 
  @retval   EFI_UNSUPPORTED        Firmware volume block device has no this capability.

**/
EFI_STATUS
EfiFvbEraseCustomBlockRange (
  IN UINTN                                Instance,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
  )
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiAtRuntime() && !mFvbEntry[Instance].IsRuntimeAccess) {
    return EFI_INVALID_PARAMETER;
  }

  if (!(mFvbEntry[Instance].FvbExtension)) {
    return EFI_UNSUPPORTED;
  }

  if (!(mFvbEntry[Instance].FvbExtension->EraseFvbCustomBlock)) {
    return EFI_UNSUPPORTED;
  }

  return mFvbEntry[Instance].FvbExtension->EraseFvbCustomBlock (
                                            mFvbEntry[Instance].FvbExtension,
                                            StartLba,
                                            OffsetStartLba,
                                            LastLba,
                                            OffsetLastLba
                                            );
}

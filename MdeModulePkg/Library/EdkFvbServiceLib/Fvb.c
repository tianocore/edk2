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
  Reads specified number of bytes into a buffer from the specified block

  @param Instance     The FV instance to be read from.
  @param Lba          The logical block address to be read from
  @param Offset       Offset into the block at which to begin reading
  @param NumBytes     Pointer that on input contains the total size of
                      the buffer. On output, it contains the total number
                      of bytes read
  @param Buffer       Pointer to a caller allocated buffer that will be
                      used to hold the data read

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCESS             Sucess to Read block
  @retval Others                 Fail to read block
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

  @param Instance         The FV instance to be written to
  @param Lba              The starting logical block index to write to
  @param Offset           Offset into the block at which to begin writing
  @param NumBytes         Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes actually written
  @param Buffer           Pointer to a caller allocated buffer that contains
                          the source for the write

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCESS             Sucess to write block
  @retval Others                 Fail to write block
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
   Erases and initializes a firmware volume block

   @param Instance      The FV instance to be erased
   @param Lba           The logical block index to be erased

   @retval EFI_INVALID_PARAMETER  Invalid parameter
   @retval EFI_SUCESS             Sucess to erase block
   @retval Others                 Fail to erase block
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
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

  @param Instance       The FV instance whose attributes is going to be returned
  @param Attributes     Output buffer which contains attributes

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCESS             Sucess to get Fv attribute
  @retval Others                 Fail to get Fv attribute
**/
EFI_STATUS
EfiFvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES                  *Attributes
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
   Modifies the current settings of the firmware volume according to the
   input parameter, and returns the new setting of the volume

   @param Instance        The FV instance whose attributes is going to be
                          modified
   @param Attributes      On input, it is a pointer to EFI_FVB_ATTRIBUTES
                          containing the desired firmware volume settings.
                          On successful return, it contains the new settings
                          of the firmware volume

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCESS             Sucess to set Fv attribute
  @retval Others                 Fail to set Fv attribute
**/
EFI_STATUS
EfiFvbSetVolumeAttributes (
  IN     UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES                   *Attributes
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
   Retrieves the physical address of a memory mapped FV

   @param Instance      The FV instance whose base address is going to be
                        returned
   @param BaseAddress   Pointer to a caller allocated EFI_PHYSICAL_ADDRESS
                        that on successful return, contains the base address
                        of the firmware volume.

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCESS             Sucess to get physical address
  @retval Others                 Fail to get physical address
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
   Retrieve the size of a logical block

   @param Instance            The FV instance whose block size is going to be
                              returned
   @param Lba                 Indicates which block to return the size for.
   @param BlockSize           A pointer to a caller allocated UINTN in which
                              the size of the block is returned
   @param NumOfBlocks         a pointer to a caller allocated UINTN in which the
                              number of consecutive blocks starting with Lba is
                              returned. All blocks in this range have a size of
                              BlockSize

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCESS             Sucess to get block size
  @retval Others                 Fail to get block size
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
   Erases and initializes a specified range of a firmware volume

   @param Instance          The FV instance to be erased
   @param StartLba          The starting logical block index to be erased
   @param OffsetStartLba    Offset into the starting block at which to
                            begin erasing
   @param LastLba           The last logical block index to be erased
   @param OffsetLastLba     Offset into the last block at which to end erasing

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCESS             Sucess to erase custom block range
  @retval Others                 Fail to erase custom block range
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

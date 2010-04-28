/*++

Copyright (c) 2005 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    Fvb.c 

Abstract:

  Firmware Volume Block Protocol Runtime Abstraction

  mFvbEntry is an array of Handle Fvb pairs. The Fvb Lib Instance matches the 
  index in the mFvbEntry array. This should be the same sequence as the FVB's
  were described in the HOB. We have to remember the handle so we can tell if 
  the protocol has been reinstalled and it needs updateing.

  If you are using any of these lib functions.you must first call FvbInitialize ().

Key:
  FVB - Firmware Volume Block

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeBlock)
#include EFI_PROTOCOL_DEFINITION (FvbExtension)

//
// Lib will ASSERT if more FVB devices than this are added to the system.
//
UINTN             mFvbCount;
VOID              *mFvbRegistration;
VOID              *mFvbExtRegistration;
//static EFI_EVENT  mEfiFvbVirtualNotifyEvent;
BOOLEAN           gEfiFvbInitialized = FALSE;
EFI_EVENT         mFvbEvent;

BOOLEAN
IsMemoryRuntime (
  IN VOID   *Address
  )
/*++

Routine Description:
  Check whether an address is runtime memory or not.

Arguments:

  Address - The Address being checked.
  
Returns: 
  TRUE    - The address is runtime memory.
  FALSE   - The address is not runtime memory.

--*/
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

VOID
EFIAPI
FvbNotificationFunction (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
/*++

Routine Description:
  Update mFvbEntry. Add new entry, or update existing entry if Fvb protocol is
  reinstalled.

Arguments:

  Event   - The Event that is being processed
  
  Context - Event Context

Returns: 
  None

--*/
{
  EFI_STATUS                         Status;
  UINTN                              BufferSize;
  EFI_HANDLE                         Handle;
  UINTN                              Index;
  UINTN                              UpdateIndex;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *Fvb;
  EFI_FVB_EXTENSION_PROTOCOL         *FvbExtension;

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
      UpdateIndex                   = mFvbCount;
    }
    //
    // The array does not have enough entries
    //
    ASSERT (UpdateIndex < MAX_FVB_COUNT);

    //
    //  Get the interface pointer and if it's ours, skip it.
    //  We check Runtime here, because it has no reason to register
    //  a boot time FVB protocol.
    //
    Status = gBS->HandleProtocol (Handle, &gEfiFirmwareVolumeBlockProtocolGuid, (VOID **) &Fvb);
    ASSERT_EFI_ERROR (Status);
    if (IsMemoryRuntime (Fvb)) {
      //
      // Increase mFvbCount if we need to add a new entry
      //
      if (UpdateIndex == mFvbCount) {
        mFvbCount++;
      }
      mFvbEntry[UpdateIndex].Handle       = Handle;
      mFvbEntry[UpdateIndex].Fvb          = Fvb;
      mFvbEntry[UpdateIndex].FvbExtension = NULL;

      Status = gBS->HandleProtocol (Handle, &gEfiFvbExtensionProtocolGuid, (VOID **) &FvbExtension);
      if ((Status == EFI_SUCCESS) && IsMemoryRuntime (FvbExtension)) {
        mFvbEntry[UpdateIndex].FvbExtension = FvbExtension;
      }
    }
  }
}

EFI_STATUS
EfiFvbInitialize (
  VOID
  )
/*++

Routine Description:
  Initialize globals and register Fvb Protocol notification function.

Arguments:
  None 

Returns: 
  EFI_SUCCESS - Fvb is successfully initialized
  others                - Fail to initialize

--*/
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

  EfiZeroMem (mFvbEntry, sizeof (FVB_ENTRY) * MAX_FVB_COUNT);

  mFvbEvent = RtEfiLibCreateProtocolNotifyEvent (
    &gEfiFirmwareVolumeBlockProtocolGuid,
    EFI_TPL_CALLBACK,
    FvbNotificationFunction,
    NULL,
    &mFvbRegistration
    );

  //
  // Register SetVirtualAddressMap () notify function
  //
  //  Status = gBS->CreateEvent (
  //                EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
  //                EFI_TPL_NOTIFY,
  //                EfiRuntimeLibFvbVirtualNotifyEvent,
  //                NULL,
  //                &mEfiFvbVirtualNotifyEvent
  //                );
  //  ASSERT_EFI_ERROR (Status);
  //
  gEfiFvbInitialized = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS
EfiFvbShutdown (
  VOID
  )
/*++

Routine Description:
  Release resources allocated in EfiFvbInitialize.

Arguments:
  None 

Returns: 
  EFI_SUCCESS

--*/
{
  gBS->FreePool ((VOID *) mFvbEntry);

  gBS->CloseEvent (mFvbEvent);

  gEfiFvbInitialized = FALSE;

  return EFI_SUCCESS;
}

//
// The following functions wrap Fvb protocol in the Runtime Lib functions.
// The Instance translates into Fvb instance. The Fvb order defined by HOBs and
// thus the sequence of FVB protocol addition define Instance.
//
// EfiFvbInitialize () must be called before any of the following functions
// must be called.
//

EFI_STATUS
EfiFvbReadBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:
  Reads specified number of bytes into a buffer from the specified block

Arguments:
  Instance              - The FV instance to be read from
  Lba                   - The logical block address to be read from
  Offset                - Offset into the block at which to begin reading
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes read
  Buffer                - Pointer to a caller allocated buffer that will be
                          used to hold the data read

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->Read (mFvbEntry[Instance].Fvb, Lba, Offset, NumBytes, Buffer);
}

EFI_STATUS
EfiFvbWriteBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:
  Writes specified number of bytes from the input buffer to the block

Arguments:
  Instance              - The FV instance to be written to
  Lba                   - The starting logical block index to write to
  Offset                - Offset into the block at which to begin writing
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes actually written
  Buffer                - Pointer to a caller allocated buffer that contains
                          the source for the write

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->Write (mFvbEntry[Instance].Fvb, Lba, Offset, NumBytes, Buffer);
}

EFI_STATUS
EfiFvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba
  )
/*++

Routine Description:
  Erases and initializes a firmware volume block

Arguments:
  Instance              - The FV instance to be erased
  Lba                   - The logical block index to be erased
  
Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->EraseBlocks (mFvbEntry[Instance].Fvb, Lba, -1);
}

EFI_STATUS
EfiFvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES                  *Attributes
  )
/*++

Routine Description:
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          returned
  Attributes            - Output buffer which contains attributes

Returns: 
  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->GetVolumeAttributes (mFvbEntry[Instance].Fvb, Attributes);
}

EFI_STATUS
EfiFvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN EFI_FVB_ATTRIBUTES                   Attributes
  )
/*++

Routine Description:
  Modifies the current settings of the firmware volume according to the 
  input parameter.

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          modified
  Attributes            - It is a pointer to EFI_FVB_ATTRIBUTES 
                          containing the desired firmware volume settings.

Returns: 
  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->SetVolumeAttributes (mFvbEntry[Instance].Fvb, &Attributes);
}

EFI_STATUS
EfiFvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *BaseAddress
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV

Arguments:
  Instance              - The FV instance whose base address is going to be
                          returned
  BaseAddress           - Pointer to a caller allocated EFI_PHYSICAL_ADDRESS 
                          that on successful return, contains the base address
                          of the firmware volume. 

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter

--*/
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->GetPhysicalAddress (mFvbEntry[Instance].Fvb, BaseAddress);
}

EFI_STATUS
EfiFvbGetBlockSize (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  )
/*++

Routine Description:
  Retrieve the size of a logical block

Arguments:
  Instance              - The FV instance whose block size is going to be
                          returned
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
                          
  EFI_INVALID_PARAMETER - invalid parameter

--*/
{
  if (Instance >= mFvbCount) {
    return EFI_INVALID_PARAMETER;
  }

  return mFvbEntry[Instance].Fvb->GetBlockSize (mFvbEntry[Instance].Fvb, Lba, BlockSize, NumOfBlocks);
}

EFI_STATUS
EfiFvbEraseCustomBlockRange (
  IN UINTN                                Instance,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
  )
/*++

Routine Description:
  Erases and initializes a specified range of a firmware volume

Arguments:
  Instance              - The FV instance to be erased
  StartLba              - The starting logical block index to be erased
  OffsetStartLba        - Offset into the starting block at which to 
                          begin erasing
  LastLba               - The last logical block index to be erased
  OffsetLastLba         - Offset into the last block at which to end erasing

Returns: 

  Status code
  
  EFI_INVALID_PARAMETER - invalid parameter
  
  EFI_UNSUPPORTED       - not support
  
--*/
{
  if (Instance >= mFvbCount) {
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

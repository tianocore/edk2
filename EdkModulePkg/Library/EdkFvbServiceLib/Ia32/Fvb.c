/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

#include "Fvb.h"

//
// Lib will ASSERT if more FVB devices than this are added to the system.
//
STATIC FVB_ENTRY          *mFvbEntry;
STATIC EFI_EVENT          mFvbRegistration;
STATIC BOOLEAN            mEfiFvbInitialized        = FALSE;
STATIC UINTN              mFvbCount;

STATIC
VOID
EFIAPI
FvbNotificationEvent (
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
    Status = gBS->HandleProtocol (Handle, &gEfiFirmwareVolumeBlockProtocolGuid, (VOID **) &mFvbEntry[UpdateIndex].Fvb);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->HandleProtocol (Handle, &gEfiFvbExtensionProtocolGuid, (VOID **) &mFvbEntry[UpdateIndex].FvbExtension);
    if (Status != EFI_SUCCESS) {
      mFvbEntry[UpdateIndex].FvbExtension = NULL;
    }
  }
}

VOID
EFIAPI
FvbVirtualAddressChangeNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Convert all pointers in mFvbEntry after ExitBootServices.

Arguments:

  Event   - The Event that is being processed
  
  Context - Event Context

Returns:

  None

--*/
{
  UINTN Index;
  if (mFvbEntry != NULL) {
    for (Index = 0; Index < MAX_FVB_COUNT; Index++) {
      if (NULL != mFvbEntry[Index].Fvb) {
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->GetBlockSize);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->GetPhysicalAddress);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->GetVolumeAttributes);
        EfiConvertPointer (0x0, (VOID **) &mFvbEntry[Index].Fvb->SetVolumeAttributes);
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

EFI_STATUS
EFIAPI
FvbLibInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  Initialize globals and register Fvb Protocol notification function.

Arguments:
  None 

Returns: 
  EFI_SUCCESS

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

  ZeroMem (mFvbEntry, sizeof (FVB_ENTRY) * MAX_FVB_COUNT);

  EfiCreateProtocolNotifyEvent (
    &gEfiFirmwareVolumeBlockProtocolGuid,
    EFI_TPL_CALLBACK,
    FvbNotificationEvent,
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

  //
  // Register SetVirtualAddressMap () notify function
  //

  ASSERT_EFI_ERROR (Status);

  mEfiFvbInitialized = TRUE;

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
  input parameter, and returns the new setting of the volume

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          modified
  Attributes            - On input, it is a pointer to EFI_FVB_ATTRIBUTES 
                          containing the desired firmware volume settings.
                          On successful return, it contains the new settings
                          of the firmware volume

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

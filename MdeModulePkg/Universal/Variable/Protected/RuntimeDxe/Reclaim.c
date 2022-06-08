/** @file
  Handles non-volatile variable store garbage collection, using FTW
  (Fault Tolerant Write) protocol.

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"
#include "VariableNonVolatile.h"
#include "VariableParsing.h"
#include "VariableRuntimeCache.h"

/**
  Gets LBA of block and offset by given address.

  This function gets the Logical Block Address (LBA) of a firmware
  volume block containing the given address, and the offset of the
  address on the block.

  @param  Address        Address which should be contained
                         by returned FVB handle.
  @param  Lba            Pointer to LBA for output.
  @param  Offset         Pointer to offset for output.

  @retval EFI_SUCCESS    LBA and offset successfully returned.
  @retval EFI_NOT_FOUND  Fail to find FVB handle by address.
  @retval EFI_ABORTED    Fail to find valid LBA and offset.

**/
EFI_STATUS
GetLbaAndOffsetByAddress (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  OUT EFI_LBA               *Lba,
  OUT UINTN                 *Offset
  )
{
  EFI_STATUS                          Status;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
  EFI_FV_BLOCK_MAP_ENTRY              *FvbMapEntry;
  UINT32                              LbaIndex;

  Fvb     = NULL;
  *Lba    = (EFI_LBA)(-1);
  *Offset = 0;

  //
  // Get the proper FVB protocol.
  //
  Status = GetFvbInfoByAddress (Address, NULL, &Fvb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the Base Address of FV.
  //
  Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)FvbBaseAddress);

  //
  // Get the (LBA, Offset) of Address.
  //
  if ((FwVolHeader->FvLength) > (FwVolHeader->HeaderLength)) {
    //
    // BUGBUG: Assume one FV has one type of BlockLength.
    //
    FvbMapEntry = &FwVolHeader->BlockMap[0];
    for (LbaIndex = 1; LbaIndex <= FvbMapEntry->NumBlocks; LbaIndex += 1) {
      if (Address < (FvbBaseAddress + FvbMapEntry->Length * LbaIndex)) {
        //
        // Found the (Lba, Offset).
        //
        *Lba    = LbaIndex - 1;
        *Offset = (UINTN)(Address - (FvbBaseAddress + FvbMapEntry->Length * (LbaIndex - 1)));
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_ABORTED;
}

/**
  Writes a buffer to variable storage space, in the working block.

  This function writes a buffer to variable storage space into a firmware
  volume block device. The destination is specified by parameter
  VariableBase. Fault Tolerant Write protocol is used for writing.

  @param  VariableBase   Base address of variable to write
  @param  VariableBuffer Point to the variable data buffer.

  @retval EFI_SUCCESS    The function completed successfully.
  @retval EFI_NOT_FOUND  Fail to locate Fault Tolerant Write protocol.
  @retval EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN VARIABLE_STORE_HEADER  *VariableBuffer
  )
{
  EFI_STATUS                         Status;
  EFI_HANDLE                         FvbHandle;
  EFI_LBA                            VarLba;
  UINTN                              VarOffset;
  UINTN                              FtwBufferSize;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL  *FtwProtocol;

  //
  // Locate fault tolerant write protocol.
  //
  Status = GetFtwProtocol ((VOID **)&FtwProtocol);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Locate Fvb handle by address.
  //
  Status = GetFvbInfoByAddress (VariableBase, &FvbHandle, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get LBA and Offset by address.
  //
  Status = GetLbaAndOffsetByAddress (VariableBase, &VarLba, &VarOffset);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  FtwBufferSize = ((VARIABLE_STORE_HEADER *)((UINTN)VariableBase))->Size;
  ASSERT (FtwBufferSize == VariableBuffer->Size);

  //
  // FTW write record.
  //
  Status = FtwProtocol->Write (
                          FtwProtocol,
                          VarLba,                // LBA
                          VarOffset,             // Offset
                          FtwBufferSize,         // NumBytes
                          NULL,                  // PrivateData NULL
                          FvbHandle,             // Fvb Handle
                          (VOID *)VariableBuffer // write buffer
                          );

  return Status;
}

/**

  Variable store garbage collection and reclaim operation.

  @param[in]      VariableBase            Base address of variable store.
  @param[out]     LastVariableOffset      Offset of last variable.
  @param[in]      IsVolatile              The variable store is volatile or not;
                                          if it is non-volatile, need FTW.
  @param[in, out] UpdatingPtrTrack        Pointer to updating variable pointer track structure.
  @param[in]      NewVariable             Pointer to new variable.
  @param[in]      NewVariableSize         New variable size.

  @return EFI_SUCCESS                  Reclaim operation has finished successfully.
  @return EFI_OUT_OF_RESOURCES         No enough memory resources or variable space.
  @return Others                       Unexpect error happened during reclaim operation.

**/
EFI_STATUS
Reclaim (
  IN     EFI_PHYSICAL_ADDRESS    VariableBase,
  OUT    UINTN                   *LastVariableOffset,
  IN     BOOLEAN                 IsVolatile,
  IN OUT VARIABLE_POINTER_TRACK  *UpdatingPtrTrack,
  IN     VARIABLE_HEADER         *NewVariable,
  IN     UINTN                   NewVariableSize
  )
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *AddedVariable;
  VARIABLE_HEADER        *NextVariable;
  VARIABLE_HEADER        *NextAddedVariable;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  UINT8                  *ValidBuffer;
  UINTN                  MaximumBufferSize;
  UINTN                  VariableSize;
  UINTN                  NameSize;
  UINT8                  *CurrPtr;
  VOID                   *Point0;
  VOID                   *Point1;
  BOOLEAN                FoundAdded;
  EFI_STATUS             Status;
  EFI_STATUS             DoneStatus;
  UINTN                  CommonVariableTotalSize;
  UINTN                  CommonUserVariableTotalSize;
  UINTN                  HwErrVariableTotalSize;
  VARIABLE_HEADER        *UpdatingVariable;
  VARIABLE_HEADER        *UpdatingInDeletedTransition;
  BOOLEAN                AuthFormat;

  AuthFormat                  = mVariableModuleGlobal->VariableGlobal.AuthFormat;
  UpdatingVariable            = NULL;
  UpdatingInDeletedTransition = NULL;
  if (UpdatingPtrTrack != NULL) {
    UpdatingVariable            = UpdatingPtrTrack->CurrPtr;
    UpdatingInDeletedTransition = UpdatingPtrTrack->InDeletedTransitionPtr;
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)VariableBase);

  CommonVariableTotalSize     = 0;
  CommonUserVariableTotalSize = 0;
  HwErrVariableTotalSize      = 0;

  if (IsVolatile || mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    //
    // Start Pointers for the variable.
    //
    Variable          = GetStartPointer (VariableStoreHeader);
    MaximumBufferSize = sizeof (VARIABLE_STORE_HEADER);

    while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader), AuthFormat)) {
      NextVariable = GetNextVariablePtr (Variable, AuthFormat);
      if (((Variable->State == VAR_ADDED) || (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) &&
          (Variable != UpdatingVariable) &&
          (Variable != UpdatingInDeletedTransition)
          )
      {
        VariableSize       = (UINTN)NextVariable - (UINTN)Variable;
        MaximumBufferSize += VariableSize;
      }

      Variable = NextVariable;
    }

    if (NewVariable != NULL) {
      //
      // Add the new variable size.
      //
      MaximumBufferSize += NewVariableSize;
    }

    //
    // Reserve the 1 Bytes with Oxff to identify the
    // end of the variable buffer.
    //
    MaximumBufferSize += 1;
    ValidBuffer        = AllocatePool (MaximumBufferSize);
    if (ValidBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // For NV variable reclaim, don't allocate pool here and just use mNvVariableCache
    // as the buffer to reduce SMRAM consumption for SMM variable driver.
    //
    MaximumBufferSize = mNvVariableCache->Size;
    ValidBuffer       = (UINT8 *)mNvVariableCache;
  }

  SetMem (ValidBuffer, MaximumBufferSize, 0xff);

  //
  // Copy variable store header.
  //
  CopyMem (ValidBuffer, VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr = (UINT8 *)GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer);

  //
  // Reinstall all ADDED variables as long as they are not identical to Updating Variable.
  //
  Variable = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader), AuthFormat)) {
    NextVariable = GetNextVariablePtr (Variable, AuthFormat);
    if ((Variable != UpdatingVariable) && (Variable->State == VAR_ADDED)) {
      VariableSize = (UINTN)NextVariable - (UINTN)Variable;
      CopyMem (CurrPtr, (UINT8 *)Variable, VariableSize);
      if (!IsVolatile) {
        (VOID)ProtectedVariableLibRefresh (
                (VARIABLE_HEADER *)CurrPtr,
                VariableSize,
                (UINTN)CurrPtr - (UINTN)ValidBuffer,
                FALSE
                );

        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
            == EFI_VARIABLE_HARDWARE_ERROR_RECORD)
        {
          HwErrVariableTotalSize += VariableSize;
        } else {
          CommonVariableTotalSize += VariableSize;
          if (IsUserVariable (Variable)) {
            CommonUserVariableTotalSize += VariableSize;
          }
        }
      }

      CurrPtr += VariableSize;
    }

    Variable = NextVariable;
  }

  //
  // Reinstall all in delete transition variables.
  //
  Variable = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader), AuthFormat)) {
    NextVariable = GetNextVariablePtr (Variable, AuthFormat);
    if ((Variable != UpdatingVariable) && (Variable != UpdatingInDeletedTransition) && (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) &&
        (ProtectedVariableLibIsHmac (GetVariableNamePtr (Variable, AuthFormat)) == FALSE))
    {
      FoundAdded    = FALSE;
      AddedVariable = GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer);
      while (IsValidVariableHeader (AddedVariable, GetEndPointer ((VARIABLE_STORE_HEADER *)ValidBuffer), AuthFormat)) {
        NextAddedVariable = GetNextVariablePtr (AddedVariable, AuthFormat);
        NameSize          = NameSizeOfVariable (AddedVariable, AuthFormat);
        if (CompareGuid (
              GetVendorGuidPtr (AddedVariable, AuthFormat),
              GetVendorGuidPtr (Variable, AuthFormat)
              ) && (NameSize == NameSizeOfVariable (Variable, AuthFormat)))
        {
          Point0 = (VOID *)GetVariableNamePtr (AddedVariable, AuthFormat);
          Point1 = (VOID *)GetVariableNamePtr (Variable, AuthFormat);
          if (CompareMem (Point0, Point1, NameSize) == 0) {
            FoundAdded = TRUE;
            break;
          }
        }

        AddedVariable = NextAddedVariable;
      }

      if (!FoundAdded) {
        //
        // Promote VAR_IN_DELETED_TRANSITION to VAR_ADDED.
        //
        VariableSize = (UINTN)NextVariable - (UINTN)Variable;
        CopyMem (CurrPtr, (UINT8 *)Variable, VariableSize);
        ((VARIABLE_HEADER *)CurrPtr)->State = VAR_ADDED;
        if (!IsVolatile) {
          (VOID)ProtectedVariableLibRefresh (
                  (VARIABLE_HEADER *)CurrPtr,
                  VariableSize,
                  (UINTN)CurrPtr - (UINTN)ValidBuffer,
                  FALSE
                  );

          if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
              == EFI_VARIABLE_HARDWARE_ERROR_RECORD)
          {
            HwErrVariableTotalSize += VariableSize;
          } else {
            CommonVariableTotalSize += VariableSize;
            if (IsUserVariable (Variable)) {
              CommonUserVariableTotalSize += VariableSize;
            }
          }
        }

        CurrPtr += VariableSize;
      }
    }

    Variable = NextVariable;
  }

  //
  // Install the new variable if it is not NULL.
  //
  if (NewVariable != NULL) {
    if (((UINTN)CurrPtr - (UINTN)ValidBuffer) + NewVariableSize > VariableStoreHeader->Size) {
      //
      // No enough space to store the new variable.
      //
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    if (!IsVolatile) {
      if ((NewVariable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        HwErrVariableTotalSize += NewVariableSize;
      } else if ((NewVariable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        CommonVariableTotalSize += NewVariableSize;
        if (IsUserVariable (NewVariable)) {
          CommonUserVariableTotalSize += NewVariableSize;
        }
      }

      if ((HwErrVariableTotalSize > PcdGet32 (PcdHwErrStorageSize)) ||
          (CommonVariableTotalSize > mVariableModuleGlobal->CommonVariableSpace) ||
          (CommonUserVariableTotalSize > mVariableModuleGlobal->CommonMaxUserVariableSpace))
      {
        //
        // No enough space to store the new variable by NV or NV+HR attribute.
        //
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }

    CopyMem (CurrPtr, (UINT8 *)NewVariable, NewVariableSize);
    ((VARIABLE_HEADER *)CurrPtr)->State = VAR_ADDED;
    if (UpdatingVariable != NULL) {
      UpdatingPtrTrack->CurrPtr                = (VARIABLE_HEADER *)((UINTN)UpdatingPtrTrack->StartPtr + ((UINTN)CurrPtr - (UINTN)GetStartPointer ((VARIABLE_STORE_HEADER *)ValidBuffer)));
      UpdatingPtrTrack->InDeletedTransitionPtr = NULL;
    }

    CurrPtr += NewVariableSize;
  }

  if (IsVolatile || mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    //
    // If volatile/emulated non-volatile variable store, just copy valid buffer.
    //
    SetMem ((UINT8 *)(UINTN)VariableBase, VariableStoreHeader->Size, 0xff);
    CopyMem ((UINT8 *)(UINTN)VariableBase, ValidBuffer, (UINTN)CurrPtr - (UINTN)ValidBuffer);
    *LastVariableOffset = (UINTN)CurrPtr - (UINTN)ValidBuffer;
    if (!IsVolatile) {
      //
      // Emulated non-volatile variable mode.
      //
      mVariableModuleGlobal->HwErrVariableTotalSize      = HwErrVariableTotalSize;
      mVariableModuleGlobal->CommonVariableTotalSize     = CommonVariableTotalSize;
      mVariableModuleGlobal->CommonUserVariableTotalSize = CommonUserVariableTotalSize;
    }

    Status = EFI_SUCCESS;
  } else {
    //
    // If non-volatile variable store, perform FTW here.
    //
    Status = FtwVariableSpace (
               VariableBase,
               (VARIABLE_STORE_HEADER *)ValidBuffer
               );
    if (!EFI_ERROR (Status)) {
      *LastVariableOffset                                = (UINTN)CurrPtr - (UINTN)ValidBuffer;
      mVariableModuleGlobal->HwErrVariableTotalSize      = HwErrVariableTotalSize;
      mVariableModuleGlobal->CommonVariableTotalSize     = CommonVariableTotalSize;
      mVariableModuleGlobal->CommonUserVariableTotalSize = CommonUserVariableTotalSize;
    } else {
      mVariableModuleGlobal->HwErrVariableTotalSize      = 0;
      mVariableModuleGlobal->CommonVariableTotalSize     = 0;
      mVariableModuleGlobal->CommonUserVariableTotalSize = 0;
      Variable                                           = GetStartPointer ((VARIABLE_STORE_HEADER *)(UINTN)VariableBase);
      while (IsValidVariableHeader (Variable, GetEndPointer ((VARIABLE_STORE_HEADER *)(UINTN)VariableBase), AuthFormat)) {
        NextVariable = GetNextVariablePtr (Variable, AuthFormat);
        VariableSize = (UINTN)NextVariable - (UINTN)Variable;
        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          mVariableModuleGlobal->HwErrVariableTotalSize += VariableSize;
        } else if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          mVariableModuleGlobal->CommonVariableTotalSize += VariableSize;
          if (IsUserVariable (Variable)) {
            mVariableModuleGlobal->CommonUserVariableTotalSize += VariableSize;
          }
        }

        Variable = NextVariable;
      }

      *LastVariableOffset = (UINTN)Variable - (UINTN)VariableBase;
    }
  }

Done:
  DoneStatus = EFI_SUCCESS;
  if (IsVolatile || mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    DoneStatus = SynchronizeRuntimeVariableCache (
                   &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeVolatileCache,
                   0,
                   VariableStoreHeader->Size
                   );
    ASSERT_EFI_ERROR (DoneStatus);
    FreePool (ValidBuffer);
  } else {
    //
    // For NV variable reclaim, we use mNvVariableCache as the buffer, so copy the data back.
    //
    CopyMem (mNvVariableCache, (UINT8 *)(UINTN)VariableBase, VariableStoreHeader->Size);
    DoneStatus = SynchronizeRuntimeVariableCache (
                   &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeNvCache,
                   0,
                   VariableStoreHeader->Size
                   );
    ASSERT_EFI_ERROR (DoneStatus);
  }

  if (!EFI_ERROR (Status) && EFI_ERROR (DoneStatus)) {
    Status = DoneStatus;
  }

  return Status;
}

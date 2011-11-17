/** @file
  Save the S3 data to S3 boot script. 
 
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "InternalBootScriptLib.h"

/**

  Data structure usage:

  +------------------------------+<-- PcdS3BootScriptTablePrivateDataPtr
  | SCRIPT_TABLE_PRIVATE_DATA    |
  |    TableBase                 |---
  |    TableLength               |--|--
  |    AtRuntime                 |  | |
  |    InSmm                     |  | |
  +------------------------------+  | |
                                    | |
  +------------------------------+<-- |
  | EFI_BOOT_SCRIPT_TABLE_HEADER |    |
  |    TableLength               |----|--
  +------------------------------+    | |
  |     ......                   |    | |
  +------------------------------+<---- |
  | EFI_BOOT_SCRIPT_TERMINATE    |      |
  +------------------------------+<------

**/

SCRIPT_TABLE_PRIVATE_DATA        *mS3BootScriptTablePtr;
EFI_EVENT                        mEnterRuntimeEvent;
//
// Allocate local copy in SMM because we can not use mS3BootScriptTablePtr when we AtRuntime in InSmm.
//
SCRIPT_TABLE_PRIVATE_DATA        mS3BootScriptTable;
UINTN                            mLockBoxLength;

EFI_GUID                         mBootScriptDataGuid = {
  0xaea6b965, 0xdcf5, 0x4311, 0xb4, 0xb8, 0xf, 0x12, 0x46, 0x44, 0x94, 0xd2
};

EFI_GUID                         mBootScriptHeaderDataGuid = {
  0x1810ab4a, 0x2314, 0x4df6, 0x81, 0xeb, 0x67, 0xc6, 0xec, 0x5, 0x85, 0x91
};

EFI_GUID                         mBootScriptInformationGuid = {
  0x2c680508, 0x2b87, 0x46ab, 0xb9, 0x8a, 0x49, 0xfc, 0x23, 0xf9, 0xf5, 0x95
};

/**
  This is an internal function to add a terminate node the entry, recalculate the table 
  length and fill into the table. 
  
  @return the base address of the boot script tble.   
 **/
UINT8*
S3BootScriptInternalCloseTable (
  VOID
  )
{
  UINT8                          *S3TableBase;
  EFI_BOOT_SCRIPT_TERMINATE      ScriptTerminate;
  EFI_BOOT_SCRIPT_TABLE_HEADER   *ScriptTableInfo;
  S3TableBase = mS3BootScriptTablePtr->TableBase;
  
  if (S3TableBase == NULL) {
    //
    // the table is not exist
    //
    return S3TableBase;
  }
  //
  // Append the termination entry.
  //
  ScriptTerminate.OpCode  = S3_BOOT_SCRIPT_LIB_TERMINATE_OPCODE;
  ScriptTerminate.Length  = (UINT8) sizeof (EFI_BOOT_SCRIPT_TERMINATE);
  CopyMem (mS3BootScriptTablePtr->TableBase + mS3BootScriptTablePtr->TableLength, &ScriptTerminate, sizeof (EFI_BOOT_SCRIPT_TERMINATE));
  //
  // fill the table length
  //
  ScriptTableInfo                = (EFI_BOOT_SCRIPT_TABLE_HEADER*)(mS3BootScriptTablePtr->TableBase);
  ScriptTableInfo->TableLength = mS3BootScriptTablePtr->TableLength + sizeof (EFI_BOOT_SCRIPT_TERMINATE);
  
 
  
  return S3TableBase;
  //
  // NOTE: Here we did NOT adjust the mS3BootScriptTablePtr->TableLength to 
  // mS3BootScriptTablePtr->TableLength + sizeof (EFI_BOOT_SCRIPT_TERMINATE). Because 
  // maybe in runtime, we still need add entries into the table, and the runtime entry should be
  // added start before this TERMINATE node.
  //
}  

/**
  This function return the total size of INFORMATION OPCODE in boot script table.

  @return InformationBufferSize The total size of INFORMATION OPCODE in boot script table.
**/
UINTN
GetBootScriptInformationBufferSize (
  VOID
  )
{
  UINT8                          *S3TableBase;
  UINT8                          *Script;
  UINTN                          TableLength;
  EFI_BOOT_SCRIPT_COMMON_HEADER  ScriptHeader;
  EFI_BOOT_SCRIPT_TABLE_HEADER   TableHeader;
  EFI_BOOT_SCRIPT_INFORMATION    Information;
  UINTN                          InformationBufferSize;

  InformationBufferSize = 0;

  S3TableBase   = mS3BootScriptTablePtr->TableBase;
  Script        = S3TableBase;
  CopyMem ((VOID*)&TableHeader, Script, sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER));
  TableLength   = TableHeader.TableLength;

  //
  // Go through the ScriptTable
  //
  while ((UINTN) Script < (UINTN) (S3TableBase + TableLength)) {
    CopyMem ((VOID*)&ScriptHeader, Script, sizeof(EFI_BOOT_SCRIPT_COMMON_HEADER));
    switch (ScriptHeader.OpCode) {
    case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
      CopyMem ((VOID*)&Information, (VOID*)Script, sizeof(Information));
      InformationBufferSize += Information.InformationLength;
      break;
    default:
      break;
    }
    Script  = Script + ScriptHeader.Length;
  }

  return InformationBufferSize;
}

/**
  This function fix INFORMATION OPCODE in boot script table.
  Originally, the Information buffer is pointer to EfiRuntimeServicesCode,
  EfiRuntimeServicesData, or EfiACPIMemoryNVS. They are seperated.
  Now, in order to save it to LockBox, we allocate a big EfiACPIMemoryNVS,
  and fix the pointer for INFORMATION opcode InformationBuffer.

  @param InformationBuffer     The address of new Information buffer.
  @param InformationBufferSize The size of new Information buffer.
**/
VOID
FixBootScriptInformation (
  IN VOID  *InformationBuffer,
  IN UINTN InformationBufferSize
  )
{
  UINT8                          *S3TableBase;
  UINT8                          *Script;
  UINTN                          TableLength;
  EFI_BOOT_SCRIPT_COMMON_HEADER  ScriptHeader;
  EFI_BOOT_SCRIPT_TABLE_HEADER   TableHeader;
  EFI_BOOT_SCRIPT_INFORMATION    Information;
  UINTN                          FixedInformationBufferSize;

  FixedInformationBufferSize = 0;

  S3TableBase   = mS3BootScriptTablePtr->TableBase;
  Script        = S3TableBase;
  CopyMem ((VOID*)&TableHeader, Script, sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER));
  TableLength   = TableHeader.TableLength;

  //
  // Go through the ScriptTable
  //
  while ((UINTN) Script < (UINTN) (S3TableBase + TableLength)) {
    CopyMem ((VOID*)&ScriptHeader, Script, sizeof(EFI_BOOT_SCRIPT_COMMON_HEADER));
    switch (ScriptHeader.OpCode) {
    case EFI_BOOT_SCRIPT_INFORMATION_OPCODE:
      CopyMem ((VOID*)&Information, (VOID*)Script, sizeof(Information));

      CopyMem (
        (VOID *)((UINTN)InformationBuffer + FixedInformationBufferSize),
        (VOID *)(UINTN)Information.Information,
        Information.InformationLength
        );
      gBS->FreePool ((VOID *)(UINTN)Information.Information);
      Information.Information = (EFI_PHYSICAL_ADDRESS)((UINTN)InformationBuffer + FixedInformationBufferSize);

      CopyMem ((VOID*)Script, (VOID*)&Information, sizeof(Information));

      FixedInformationBufferSize += Information.InformationLength;
      break;
    default:
      break;
    }
    Script  = Script + ScriptHeader.Length;
  }

  ASSERT (FixedInformationBufferSize == InformationBufferSize);

  return ;
}

/**
  This function save boot script data to LockBox.
  1. BootSriptPrivate data, BootScript data - Image and DispatchContext are handled by platform.
  2. BootScriptExecutor, BootScriptExecutor context
  - ACPI variable - (PI version) sould be handled by SMM driver. S3 Page table is handled here.
  - ACPI variable - framework version is already handled by Framework CPU driver.
**/
VOID
SaveBootScriptDataToLockBox (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  InformationBuffer;
  UINTN                 InformationBufferSize;

  //
  // We need save BootScriptInformation to LockBox, because it is in
  // EfiRuntimeServicesCode, EfiRuntimeServicesData, or EfiACPIMemoryNVS.
  // 
  //
  InformationBufferSize = GetBootScriptInformationBufferSize ();
  if (InformationBufferSize != 0) {
    InformationBuffer = 0xFFFFFFFF;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES(InformationBufferSize),
                    &InformationBuffer
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Fix BootScript information pointer
    //
    FixBootScriptInformation ((VOID *)(UINTN)InformationBuffer, InformationBufferSize);

    //
    // Save BootScript information to lockbox
    //
    Status = SaveLockBox (
               &mBootScriptInformationGuid,
               (VOID *)(UINTN)InformationBuffer,
               InformationBufferSize
               );
    ASSERT_EFI_ERROR (Status);

    Status = SetLockBoxAttributes (&mBootScriptInformationGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // mS3BootScriptTablePtr->TableLength does not include EFI_BOOT_SCRIPT_TERMINATE, because we need add entry at runtime.
  // Save all info here, just in case that no one will add boot script entry in SMM.
  //
  Status = SaveLockBox (
             &mBootScriptDataGuid,
             (VOID *)mS3BootScriptTablePtr->TableBase,
             mS3BootScriptTablePtr->TableLength + sizeof(EFI_BOOT_SCRIPT_TERMINATE)
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (&mBootScriptDataGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);

  //
  // Just need save TableBase.
  // Do not update other field because they will NOT be used in S3.
  //
  Status = SaveLockBox (
             &mBootScriptHeaderDataGuid,
             (VOID *)&mS3BootScriptTablePtr->TableBase,
             sizeof(mS3BootScriptTablePtr->TableBase)
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (&mBootScriptHeaderDataGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);
}

/**
  This is the Event call back function to notify the Library the system is entering
  run time phase.
  
  @param  Event   Pointer to this event
  @param  Context Event hanlder private data 
 **/
VOID
EFIAPI
S3BootScriptEventCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS   Status;
  VOID         *Interface;

  //
  // Try to locate it because EfiCreateProtocolNotifyEvent will trigger it once when registration.
  // Just return if it is not found.
  //
  Status = gBS->LocateProtocol (
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  NULL,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Here we should tell the library that we are enter into runtime phase. and 
  // the memory page number occupied by the table should not grow anymore.
  //
  if (!mS3BootScriptTablePtr->AtRuntime) {
    //
    // In boot time, we need not write the terminate node when adding a node to boot scipt table
    // or else, that will impact the performance. However, in runtime, we should append terminate
    // node on every add to boot script table
    //
    S3BootScriptInternalCloseTable ();
    mS3BootScriptTablePtr->AtRuntime = TRUE;

    //
    // Save BootScript data to lockbox
    //
    SaveBootScriptDataToLockBox ();
  }
} 
/**
  This is the Event call back function is triggered in SMM to notify the Library the system is entering
  run time phase and set InSmm flag.
  
  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS SmmEventCallback runs successfully
 **/
EFI_STATUS
EFIAPI
S3BootScriptSmmEventCallBack (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  //
  // Check if it is already done
  //
  if (mS3BootScriptTablePtr == &mS3BootScriptTable) {
    return EFI_SUCCESS;
  }

  //
  // Last chance to call-out, just make sure AtRuntime is set
  //
  S3BootScriptEventCallBack (NULL, NULL);

  //
  // Save a local copy
  //
  CopyMem (&mS3BootScriptTable, mS3BootScriptTablePtr, sizeof(*mS3BootScriptTablePtr));
  //
  // We should not use ACPINvs copy, because it is not safe.
  //
  mS3BootScriptTablePtr = &mS3BootScriptTable;

  //
  // Set InSmm, we allow boot script update when InSmm, but not allow boot script outside SMM.
  // InSmm will only be checked if AtRuntime is TRUE.
  //
  mS3BootScriptTablePtr->InSmm = TRUE;

  //
  // Record LockBoxLength
  //
  mLockBoxLength = mS3BootScriptTable.TableLength + sizeof(EFI_BOOT_SCRIPT_TERMINATE);

  return EFI_SUCCESS;
}

/**
  Library Constructor.
  this function just identify it is a smm driver or non-smm driver linked against 
  with the library   

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval  RETURN_SUCCESS            Allocate the global memory space to store S3 boot script table private data
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to allocated.
**/
RETURN_STATUS
EFIAPI
S3BootScriptLibInitialize (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                      Status;
  SCRIPT_TABLE_PRIVATE_DATA      *S3TablePtr;
  VOID                           *Registration;
  EFI_SMM_BASE2_PROTOCOL         *SmmBase2;
  BOOLEAN                        InSmm;
  EFI_SMM_SYSTEM_TABLE2          *Smst;
  EFI_PHYSICAL_ADDRESS           Buffer;

  S3TablePtr = (SCRIPT_TABLE_PRIVATE_DATA*)(UINTN)PcdGet64(PcdS3BootScriptTablePrivateDataPtr);
  //
  // The Boot script private data is not be initialized. create it
  //
  if (S3TablePtr == 0) {
    Buffer = SIZE_4GB - 1;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES(sizeof(SCRIPT_TABLE_PRIVATE_DATA)),
                    &Buffer
                    );
    if (EFI_ERROR (Status)) {
      return RETURN_OUT_OF_RESOURCES;
    }
    S3TablePtr = (VOID *) (UINTN) Buffer;

    PcdSet64 (PcdS3BootScriptTablePrivateDataPtr, (UINT64) (UINTN)S3TablePtr); 
    ZeroMem (S3TablePtr, sizeof(SCRIPT_TABLE_PRIVATE_DATA));  
    //
    // create event to notify the library system enter the runtime phase
    //
    mEnterRuntimeEvent = EfiCreateProtocolNotifyEvent  (
                          &gEfiDxeSmmReadyToLockProtocolGuid,
                          TPL_CALLBACK,
                          S3BootScriptEventCallBack,
                          NULL,
                          &Registration
                          );
    ASSERT (mEnterRuntimeEvent != NULL);
  } 
  mS3BootScriptTablePtr = S3TablePtr;

  //
  // Get InSmm, we need to register SmmReadyToLock if this library is linked to SMM driver.
  //
  Status = gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, (VOID**) &SmmBase2);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }
  Status = SmmBase2->InSmm (SmmBase2, &InSmm);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }
  if (!InSmm) {
    return RETURN_SUCCESS;
  }
  //
  // Good, we are in SMM
  //
  Status = SmmBase2->GetSmstLocation (SmmBase2, &Smst);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }

  //
  // Then register event after lock
  //
  Registration = NULL;
  Status = Smst->SmmRegisterProtocolNotify (
                   &gEfiSmmReadyToLockProtocolGuid,
                   S3BootScriptSmmEventCallBack,
                   &Registration
                   );
  ASSERT_EFI_ERROR (Status);

  return RETURN_SUCCESS;
}
/**
  To get the start address from which a new boot time s3 boot script entry will write into.
  If the table is not exist, the functio will first allocate a buffer for the table
  If the table buffer is not enough for the new entry, in non-smm mode, the funtion will 
  invoke reallocate to enlarge buffer.
  
  @param EntryLength      the new entry length.
  
  @retval the address from which the a new s3 boot script entry will write into 
 **/
UINT8*
S3BootScriptGetBootTimeEntryAddAddress (
  UINT8  EntryLength
  )
{
   EFI_PHYSICAL_ADDRESS              S3TableBase;
   EFI_PHYSICAL_ADDRESS              NewS3TableBase;
   UINT8                            *NewEntryPtr;
   UINT32                            TableLength;
   UINT16                            PageNumber;
   EFI_STATUS                        Status;
   EFI_BOOT_SCRIPT_TABLE_HEADER      *ScriptTableInfo;
   
   S3TableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)(mS3BootScriptTablePtr->TableBase);
   if (S3TableBase == 0) {
     // The table is not exist. This is the first to add entry. 
     // Allocate ACPI script table space under 4G memory. We need it to save
     // some settings done by CSM, which runs after normal script table closed
     //
     S3TableBase = 0xffffffff;
     Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  2 + PcdGet16(PcdS3BootScriptRuntimeTableReservePageNumber),
                  (EFI_PHYSICAL_ADDRESS*)&S3TableBase
                  );
     
     if (EFI_ERROR(Status)) {
       ASSERT_EFI_ERROR (Status);
       return 0;
     }
     //
     // Fill Table Header
     //
     ScriptTableInfo              = (EFI_BOOT_SCRIPT_TABLE_HEADER*)(UINTN)S3TableBase;
     ScriptTableInfo->OpCode      = S3_BOOT_SCRIPT_LIB_TABLE_OPCODE;
     ScriptTableInfo->Length      = (UINT8) sizeof (EFI_BOOT_SCRIPT_TABLE_HEADER);
     ScriptTableInfo->TableLength = 0;   // will be calculate at CloseTable
     mS3BootScriptTablePtr->TableLength = sizeof (EFI_BOOT_SCRIPT_TABLE_HEADER);
     mS3BootScriptTablePtr->TableBase = (UINT8*)(UINTN)S3TableBase;
     mS3BootScriptTablePtr->TableMemoryPageNumber = (UINT16)(2 + PcdGet16(PcdS3BootScriptRuntimeTableReservePageNumber));
   }
     
   // Here we do not count the reserved memory for runtime script table.
   PageNumber   = (UINT16)(mS3BootScriptTablePtr->TableMemoryPageNumber - PcdGet16(PcdS3BootScriptRuntimeTableReservePageNumber));   
   TableLength =  mS3BootScriptTablePtr->TableLength;
   if ((UINT32)(PageNumber * EFI_PAGE_SIZE) < (TableLength + EntryLength)) {
     // 
     // The buffer is too small to hold the table, Reallocate the buffer
     //
     NewS3TableBase = 0xffffffff;
     Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  2 + PageNumber + PcdGet16(PcdS3BootScriptRuntimeTableReservePageNumber),
                  (EFI_PHYSICAL_ADDRESS*)&NewS3TableBase
                  );
   
     if (EFI_ERROR(Status)) {
       ASSERT_EFI_ERROR (Status);
       return 0;
     }
     
     CopyMem ((VOID*)(UINTN)NewS3TableBase, (VOID*)(UINTN)S3TableBase, TableLength);
     gBS->FreePages (S3TableBase, mS3BootScriptTablePtr->TableMemoryPageNumber);
         
     mS3BootScriptTablePtr->TableBase = (UINT8*)(UINTN)NewS3TableBase;
     mS3BootScriptTablePtr->TableMemoryPageNumber =  (UINT16) (2 + PageNumber + PcdGet16(PcdS3BootScriptRuntimeTableReservePageNumber)); 
   }
   //
   // calculate the the start address for the new entry. 
   //
   NewEntryPtr = mS3BootScriptTablePtr->TableBase + TableLength;
   
   //
   // update the table lenghth
   //
   mS3BootScriptTablePtr->TableLength =  TableLength + EntryLength;
   
   //
   // In the boot time, we will not append the termination entry to the boot script
   // table until the callers think there is no boot time data that should be added and 
   // it is caller's responsibility to explicit call the CloseTable. 
   //
   //
  
   return NewEntryPtr;    
}
/**
  To get the start address from which a new runtime s3 boot script entry will write into.
  In this case, it should be ensured that there is enough buffer to hold the entry.
  
  @param EntryLength      the new entry length.
  
  @retval the address from which the a new s3 runtime script entry will write into
 **/
UINT8*
S3BootScriptGetRuntimeEntryAddAddress (
  UINT8  EntryLength
  )
{
   UINT8     *NewEntryPtr;
   
   NewEntryPtr = NULL;   
   //
   // Check if the memory range reserved for S3 Boot Script table is large enough to hold the node. 
   //
   if (mS3BootScriptTablePtr->TableLength + EntryLength + sizeof (EFI_BOOT_SCRIPT_TERMINATE) <= EFI_PAGES_TO_SIZE((UINT32)(mS3BootScriptTablePtr->TableMemoryPageNumber))) {
     NewEntryPtr = mS3BootScriptTablePtr->TableBase + mS3BootScriptTablePtr->TableLength;   
     mS3BootScriptTablePtr->TableLength = mS3BootScriptTablePtr->TableLength + EntryLength;
     //
     // Append a terminate node on every insert
     //
     S3BootScriptInternalCloseTable ();
   }
   return (UINT8*)NewEntryPtr;    
}
/**
  To get the start address from which a new s3 boot script entry will write into.
  
  @param EntryLength      the new entry length.
  
  @retval the address from which the a new s3 runtime script entry will write into 
 **/ 
UINT8* 
S3BootScriptGetEntryAddAddress (
  UINT8  EntryLength
  )
{
  UINT8*                         NewEntryPtr;
  EFI_BOOT_SCRIPT_TABLE_HEADER   TableHeader;
  EFI_STATUS                     Status;

  if (mS3BootScriptTablePtr->AtRuntime) {
    //
    // We need check InSmm when AtRuntime, because after SmmReadyToLock, only SMM driver is allowed to write boot script.
    //
    if (!mS3BootScriptTablePtr->InSmm) {
      //
      // Add DEBUG ERROR, so that we can find it at boot time.
      // Do not use ASSERT, because we may have test invoke this interface.
      //
      DEBUG ((EFI_D_ERROR, "FATAL ERROR: Set boot script after ReadyToLock!!!\n"));
      return NULL;
    }

    //
    // NOTE: OS will restore ACPINvs data. After S3, the table length in mS3BootScriptTable (SMM) is different with
    // table length in BootScriptTable header (ACPINvs).
    // So here we need sync them. We choose ACPINvs table length, because we want to override the boot script saved
    // in SMM every time.
    //
    ASSERT (mS3BootScriptTablePtr == &mS3BootScriptTable);
    CopyMem ((VOID*)&TableHeader, (VOID*)mS3BootScriptTablePtr->TableBase, sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER));
    if (mS3BootScriptTablePtr->TableLength + sizeof(EFI_BOOT_SCRIPT_TERMINATE) != TableHeader.TableLength) {
      //
      // Restore it to use original value
      //
      RestoreLockBox (&mBootScriptDataGuid, NULL, NULL);
      //
      // Copy it again to get original value
      // NOTE: We should NOT use TableHeader.TableLength, because it is already updated to be whole length.
      //
      mS3BootScriptTablePtr->TableLength = (UINT32)(mLockBoxLength - sizeof(EFI_BOOT_SCRIPT_TERMINATE));
    }

    NewEntryPtr  = S3BootScriptGetRuntimeEntryAddAddress (EntryLength);
    //
    // Now the length field is updated, need sync to lockbox.
    // So in S3 resume, the data can be restored correctly.
    //
    CopyMem ((VOID*)&TableHeader, (VOID*)mS3BootScriptTablePtr->TableBase, sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER));
    Status = UpdateLockBox (
               &mBootScriptDataGuid,
               OFFSET_OF(EFI_BOOT_SCRIPT_TABLE_HEADER, TableLength),
               &TableHeader.TableLength,
               sizeof(TableHeader.TableLength)
               );
    ASSERT_EFI_ERROR (Status);
  } else {   
    NewEntryPtr  = S3BootScriptGetBootTimeEntryAddAddress (EntryLength);
  }  
  return NewEntryPtr;
  
}  

/**
  Sync BootScript LockBox data.
**/
VOID
SyncBootScript (
  VOID
  )
{
  EFI_STATUS  Status;

  if (!mS3BootScriptTablePtr->AtRuntime || !mS3BootScriptTablePtr->InSmm) {
    return ;
  }
  //
  // Update Terminate
  // So in S3 resume, the data can be restored correctly.
  //
  Status = UpdateLockBox (
             &mBootScriptDataGuid,
             mLockBoxLength - sizeof(EFI_BOOT_SCRIPT_TERMINATE),
             (VOID *)((UINTN)mS3BootScriptTablePtr->TableBase + mLockBoxLength - sizeof(EFI_BOOT_SCRIPT_TERMINATE)),
             sizeof(EFI_BOOT_SCRIPT_TERMINATE)
             );
  ASSERT_EFI_ERROR (Status);
}

/** 
  This is an function to close the S3 boot script table. The function could only be called in 
  BOOT time phase. To comply with the Framework spec definition on 
  EFI_BOOT_SCRIPT_SAVE_PROTOCOL.CloseTable(), this function will fulfill following things:
  1. Closes the specified boot script table
  2. It allocates a new memory pool to duplicate all the boot scripts in the specified table. 
     Once this function is called, the table maintained by the library will be destroyed 
     after it is copied into the allocated pool.
  3. Any attempts to add a script record after calling this function will cause a new table 
     to be created by the library.
  4. The base address of the allocated pool will be returned in Address. Note that after 
     using the boot script table, the CALLER is responsible for freeing the pool that is allocated
     by this function. 

  In Spec PI1.1, this EFI_BOOT_SCRIPT_SAVE_PROTOCOL.CloseTable() is retired. To provides this API for now is 
  for Framework Spec compatibility.
  
  If anyone does call CloseTable() on a real platform, then the caller is responsible for figuring out 
  how to get the script to run on an S3 resume because the boot script maintained by the lib will be 
  destroyed.
 
  @return the base address of the new copy of the boot script tble.   
  @note this function could only called in boot time phase

**/
UINT8*
EFIAPI
S3BootScriptCloseTable (
  VOID
  )
{
  UINT8                          *S3TableBase;
  UINT32                          TableLength;
  UINT8                          *Buffer;
  EFI_STATUS                      Status;
  EFI_BOOT_SCRIPT_TABLE_HEADER      *ScriptTableInfo;
  
  S3TableBase =    mS3BootScriptTablePtr->TableBase;    
  if (S3TableBase == 0) {
    return 0; 
  }
  //
  // Append the termination record the S3 boot script table
  //
  S3BootScriptInternalCloseTable();
  TableLength = mS3BootScriptTablePtr->TableLength + sizeof (EFI_BOOT_SCRIPT_TERMINATE);
  //
  // Allocate the buffer and copy the boot script to the buffer. 
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  (UINTN)TableLength,
                  (VOID **) &Buffer
                  );
  if (EFI_ERROR (Status)) {
        return 0; 
  }
  CopyMem (Buffer, S3TableBase, TableLength);
  
  //
  // Destroy the table maintained by the library so that the next write operation 
  // will write the record to the first entry of the table.
  //
  // Fill the table header.
  ScriptTableInfo                    = (EFI_BOOT_SCRIPT_TABLE_HEADER*)S3TableBase;
  ScriptTableInfo->OpCode      = S3_BOOT_SCRIPT_LIB_TABLE_OPCODE;
  ScriptTableInfo->Length      = (UINT8) sizeof (EFI_BOOT_SCRIPT_TABLE_HEADER);
  ScriptTableInfo->TableLength = 0;   // will be calculate at close the table
  
  mS3BootScriptTablePtr->TableLength = sizeof (EFI_BOOT_SCRIPT_TABLE_HEADER);
  return Buffer;
}
/**
  Save I/O write to boot script 

  @param Width   The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Address The base address of the I/O operations.
  @param Count   The number of I/O operations to perform.
  @param Buffer  The source buffer from which to write data.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveIoWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH          Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  VOID                              *Buffer
  )

{
  UINT8                     Length;
  UINT8                    *Script;
  UINT8                     WidthInByte;
  EFI_BOOT_SCRIPT_IO_WRITE  ScriptIoWrite;

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_IO_WRITE) + (WidthInByte * Count));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // save script data
  //
  ScriptIoWrite.OpCode  = EFI_BOOT_SCRIPT_IO_WRITE_OPCODE;
  ScriptIoWrite.Length  = Length;
  ScriptIoWrite.Width   = Width;
  ScriptIoWrite.Address = Address;
  ScriptIoWrite.Count   = (UINT32) Count;
  CopyMem ((VOID*)Script, (VOID*)&ScriptIoWrite, sizeof(EFI_BOOT_SCRIPT_IO_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_IO_WRITE)), Buffer, WidthInByte * Count);

  SyncBootScript ();

  return RETURN_SUCCESS;
}

/**
  Adds a record for an I/O modify operation into a S3 boot script table

  @param Width   The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Address The base address of the I/O operations.
  @param Data    A pointer to the data to be OR-ed.
  @param DataMask  A pointer to the data mask to be AND-ed with the data read from the register

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveIoReadWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH         Width,
  IN  UINT64                           Address,
  IN  VOID                            *Data,
  IN  VOID                            *DataMask
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_IO_READ_WRITE  ScriptIoReadWrite;

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_IO_READ_WRITE) + (WidthInByte * 2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptIoReadWrite.OpCode  = EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE;
  ScriptIoReadWrite.Length  = Length;
  ScriptIoReadWrite.Width   = Width;
  ScriptIoReadWrite.Address = Address;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptIoReadWrite, sizeof(EFI_BOOT_SCRIPT_IO_READ_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_IO_READ_WRITE)), Data, WidthInByte);
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_IO_READ_WRITE) + WidthInByte), DataMask, WidthInByte);

  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for a memory write operation into a specified boot script table.

  @param Width   The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Address The base address of the memory operations
  @param Count   The number of memory operations to perform.
  @param Buffer  The source buffer from which to write the data.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveMemWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH          Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  VOID                              *Buffer
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_MEM_WRITE  ScriptMemWrite;
  
  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_MEM_WRITE) + (WidthInByte * Count));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }  
  //
  // Build script data
  //
  ScriptMemWrite.OpCode   = EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE;
  ScriptMemWrite.Length   = Length;
  ScriptMemWrite.Width    = Width;
  ScriptMemWrite.Address  = Address;
  ScriptMemWrite.Count    = (UINT32) Count;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptMemWrite, sizeof(EFI_BOOT_SCRIPT_MEM_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_MEM_WRITE)), Buffer, WidthInByte * Count);
  
  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for a memory modify operation into a specified boot script table.

  @param Width     The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Address   The base address of the memory operations. Address needs alignment if required
  @param Data      A pointer to the data to be OR-ed.
  @param DataMask  A pointer to the data mask to be AND-ed with the data read from the register.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveMemReadWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH          Width,
  IN  UINT64                            Address,
  IN  VOID                              *Data,
  IN  VOID                              *DataMask
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_MEM_READ_WRITE  ScriptMemReadWrite;
 
  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_MEM_READ_WRITE) + (WidthInByte * 2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  } 
  //
  // Build script data
  //    
  ScriptMemReadWrite.OpCode   = EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE;
  ScriptMemReadWrite.Length   = Length;
  ScriptMemReadWrite.Width    = Width;
  ScriptMemReadWrite.Address  = Address;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptMemReadWrite , sizeof (EFI_BOOT_SCRIPT_MEM_READ_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_MEM_READ_WRITE)), Data, WidthInByte);
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_MEM_READ_WRITE) + WidthInByte), DataMask, WidthInByte);

  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for a PCI configuration space write operation into a specified boot script table.

  @param Width     The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Address   The address within the PCI configuration space.
  @param Count     The number of PCI operations to perform.
  @param Buffer    The source buffer from which to write the data.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSavePciCfgWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH         Width,
  IN  UINT64                           Address,
  IN  UINTN                            Count,
  IN  VOID                            *Buffer
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE  ScriptPciWrite;

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE) + (WidthInByte * Count));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  } 
  //
  // Build script data
  //
  ScriptPciWrite.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE;
  ScriptPciWrite.Length   = Length;
  ScriptPciWrite.Width    = Width;
  ScriptPciWrite.Address  = Address;
  ScriptPciWrite.Count    = (UINT32) Count;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptPciWrite,  sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE)), Buffer, WidthInByte * Count);
  
  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for a PCI configuration space modify operation into a specified boot script table.

  @param Width     The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Address   The address within the PCI configuration space.
  @param Data      A pointer to the data to be OR-ed.The size depends on Width.
  @param DataMask    A pointer to the data mask to be AND-ed.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN__SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSavePciCfgReadWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH          Width,
  IN  UINT64                            Address,
  IN  VOID                              *Data,
  IN  VOID                              *DataMask
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE  ScriptPciReadWrite;

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE) + (WidthInByte * 2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }  
  //
  // Build script data
  // 
  ScriptPciReadWrite.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE;
  ScriptPciReadWrite.Length   = Length;
  ScriptPciReadWrite.Width    = Width;
  ScriptPciReadWrite.Address  = Address;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptPciReadWrite, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE)), Data, WidthInByte);
  CopyMem (
    (VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE) + WidthInByte),
    DataMask,
    WidthInByte
    );

  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for a PCI configuration space modify operation into a specified boot script table.

  @param Width     The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Segment   The PCI segment number for Address.
  @param Address   The address within the PCI configuration space.
  @param Count     The number of PCI operations to perform.
  @param Buffer    The source buffer from which to write the data.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSavePciCfg2Write (
  IN S3_BOOT_SCRIPT_LIB_WIDTH        Width,
  IN UINT16                          Segment,
  IN UINT64                          Address,
  IN UINTN                           Count,
  IN VOID                           *Buffer
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE  ScriptPciWrite2;
  
  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE) + (WidthInByte * Count));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }  
  //
  // Build script data
  //
  ScriptPciWrite2.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE;
  ScriptPciWrite2.Length   = Length;
  ScriptPciWrite2.Width    = Width;
  ScriptPciWrite2.Address  = Address;
  ScriptPciWrite2.Segment  = Segment;
  ScriptPciWrite2.Count    = (UINT32)Count;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptPciWrite2, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE)), Buffer, WidthInByte * Count);

  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for a PCI configuration space modify operation into a specified boot script table.

  @param Width     The width of the I/O operations.Enumerated in S3_BOOT_SCRIPT_LIB_WIDTH.
  @param Segment   The PCI segment number for Address.
  @param Address   The address within the PCI configuration space.
  @param Data      A pointer to the data to be OR-ed. The size depends on Width.
  @param DataMask    A pointer to the data mask to be AND-ed.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSavePciCfg2ReadWrite (
  IN S3_BOOT_SCRIPT_LIB_WIDTH        Width,
  IN UINT16                          Segment,
  IN UINT64                          Address,
  IN VOID                           *Data,
  IN VOID                           *DataMask
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE  ScriptPciReadWrite2;
  
  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE) + (WidthInByte * 2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }  
  //
  // Build script data
  //
  ScriptPciReadWrite2.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE;
  ScriptPciReadWrite2.Length   = Length;
  ScriptPciReadWrite2.Width    = Width;
  ScriptPciReadWrite2.Segment  = Segment;
  ScriptPciReadWrite2.Address  = Address;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptPciReadWrite2, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE)), Data, WidthInByte);
  CopyMem (
    (VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE) + WidthInByte),
    DataMask,
    WidthInByte
    );
  
  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for an SMBus command execution into a specified boot script table.

  @param  SmBusAddress  Address that encodes the SMBUS Slave Address, SMBUS Command, SMBUS Data Length, and PEC.
  @param Operation      Indicates which particular SMBus protocol it will use to execute the SMBus
                        transactions.
  @param Length         A pointer to signify the number of bytes that this operation will do.
  @param Buffer         Contains the value of data to execute to the SMBUS slave device.
  
  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveSmbusExecute (
  IN  UINTN                             SmBusAddress, 
  IN  EFI_SMBUS_OPERATION               Operation,
  IN  UINTN                             *Length,
  IN  VOID                              *Buffer
  )
{
  UINT8                 DataSize;
  UINT8                *Script;
  EFI_BOOT_SCRIPT_SMBUS_EXECUTE  ScriptSmbusExecute;

  DataSize = (UINT8)(sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE) + (*Length));
  
  Script = S3BootScriptGetEntryAddAddress (DataSize);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptSmbusExecute.OpCode       = EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE;
  ScriptSmbusExecute.Length       = DataSize;
  ScriptSmbusExecute.SmBusAddress = (UINT64) SmBusAddress;
  ScriptSmbusExecute.Operation    = Operation;
  ScriptSmbusExecute.DataSize     = (UINT32) *Length;

  CopyMem ((VOID*)Script, (VOID*)&ScriptSmbusExecute, sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE));
  CopyMem (
    (VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE)),
    Buffer,
    (*Length)
    );

  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for an execution stall on the processor into a specified boot script table.

  @param Duration   Duration in microseconds of the stall
  
  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveStall (
  IN  UINTN                             Duration
  )
{
  UINT8                 Length;
  UINT8                *Script;
  EFI_BOOT_SCRIPT_STALL  ScriptStall;

  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_STALL));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }  
  //
  // Build script data
  //
  ScriptStall.OpCode    = EFI_BOOT_SCRIPT_STALL_OPCODE;
  ScriptStall.Length    = Length;
  ScriptStall.Duration  = Duration;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptStall, sizeof (EFI_BOOT_SCRIPT_STALL));
  
  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for an execution stall on the processor into a specified boot script table.

  @param EntryPoint   Entry point of the code to be dispatched.
  @param Context      Argument to be passed into the EntryPoint of the code to be dispatched.
  
  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveDispatch2 (
  IN  VOID                      *EntryPoint,
  IN  VOID                      *Context
  )
{
  UINT8                 Length;
  UINT8                 *Script;
  EFI_BOOT_SCRIPT_DISPATCH_2  ScriptDispatch2;
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_DISPATCH_2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }  
  //
  // Build script data
  //
  ScriptDispatch2.OpCode     = EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE;
  ScriptDispatch2.Length     = Length;
  ScriptDispatch2.EntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)EntryPoint;
  ScriptDispatch2.Context =   (EFI_PHYSICAL_ADDRESS)(UINTN)Context;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptDispatch2, sizeof (EFI_BOOT_SCRIPT_DISPATCH_2));
  
  SyncBootScript ();

  return RETURN_SUCCESS;

}
/**
  Adds a record for memory reads of the memory location and continues when the exit criteria is
  satisfied or after a defined duration.
  
  @param Width     The width of the memory operations.
  @param Address   The base address of the memory operations.
  @param BitMask   A pointer to the bit mask to be AND-ed with the data read from the register.
  @param BitValue  A pointer to the data value after to be Masked.
  @param Duration  Duration in microseconds of the stall.
  @param LoopTimes The times of the register polling.

  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.

**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveMemPoll (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH          Width,
  IN  UINT64                            Address,
  IN  VOID                              *BitMask,
  IN  VOID                              *BitValue,
  IN  UINTN                             Duration,
  IN  UINTN                             LoopTimes
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte; 
  EFI_BOOT_SCRIPT_MEM_POLL      ScriptMemPoll; 

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_MEM_POLL) + (WidthInByte * 2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptMemPoll.OpCode   = EFI_BOOT_SCRIPT_MEM_POLL_OPCODE;
  ScriptMemPoll.Length   = Length;
  ScriptMemPoll.Width    = Width;  
  ScriptMemPoll.Address  = Address;
  ScriptMemPoll.Duration = Duration;
  ScriptMemPoll.LoopTimes = LoopTimes;

  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_MEM_POLL)), BitValue, WidthInByte);
  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_MEM_POLL) + WidthInByte), BitMask, WidthInByte);
  CopyMem ((VOID*)Script, (VOID*)&ScriptMemPoll, sizeof (EFI_BOOT_SCRIPT_MEM_POLL)); 

  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Store arbitrary information in the boot script table. This opcode is a no-op on dispatch and is only
  used for debugging script issues.
  
  @param InformationLength   Length of the data in bytes
  @param Information       Information to be logged in the boot scrpit
 
  @retval RETURN_UNSUPPORTED       If  entering runtime, this method will not support.
  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.

**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveInformation (
  IN  UINT32                                InformationLength, 
  IN  VOID                                 *Information
  )
{
  RETURN_STATUS         Status;
  UINT8                 Length;
  UINT8                 *Script;
  VOID                  *Buffer;
  EFI_BOOT_SCRIPT_INFORMATION  ScriptInformation;

  if (mS3BootScriptTablePtr->AtRuntime) {
    return RETURN_UNSUPPORTED;
  }
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_INFORMATION));
  
  //
  // Use BootServicesData to hold the data, just in case caller free it.
  // It will be copied into ACPINvs later.
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  InformationLength,
                  &Buffer
                  );
  if (EFI_ERROR (Status)) {
    return RETURN_OUT_OF_RESOURCES;
  }
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptInformation.OpCode     = EFI_BOOT_SCRIPT_INFORMATION_OPCODE;
  ScriptInformation.Length     = Length;


  ScriptInformation.InformationLength = InformationLength;  

  CopyMem ((VOID *)(UINTN)Buffer, Information,(UINTN) InformationLength);  
  ScriptInformation.Information = (EFI_PHYSICAL_ADDRESS) (UINTN) Buffer;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptInformation, sizeof (EFI_BOOT_SCRIPT_INFORMATION));  
  return RETURN_SUCCESS;

}
/**
  Store a string in the boot script table. This opcode is a no-op on dispatch and is only
  used for debugging script issues.
  
  @param String            The string to save to boot script table
  
  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.

**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveInformationAsciiString (
  IN  CONST CHAR8               *String
  )
{
  return S3BootScriptSaveInformation (      
           (UINT32) AsciiStrLen (String) + 1, 
           (VOID*) String
           );
}
/**
  Adds a record for dispatching specified arbitrary code into a specified boot script table.

  @param EntryPoint   Entry point of the code to be dispatched.
  
  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveDispatch (
  IN  VOID                              *EntryPoint
  )
{
  UINT8                 Length;
  UINT8                *Script;
  EFI_BOOT_SCRIPT_DISPATCH  ScriptDispatch;
  
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_DISPATCH));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }  
  //
  // Build script data
  //
  ScriptDispatch.OpCode     = EFI_BOOT_SCRIPT_DISPATCH_OPCODE;
  ScriptDispatch.Length     = Length;
  ScriptDispatch.EntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)EntryPoint;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptDispatch, sizeof (EFI_BOOT_SCRIPT_DISPATCH)); 
  
  SyncBootScript ();

  return RETURN_SUCCESS;

}
/**
  Adds a record for I/O reads the I/O location and continues when the exit criteria is satisfied or after a
  defined duration.
  
  @param  Width                 The width of the I/O operations. 
  @param  Address               The base address of the I/O operations.
  @param  Data                  The comparison value used for the polling exit criteria.
  @param  DataMask              Mask used for the polling criteria. The bits in the bytes below Width which are zero
                                in Data are ignored when polling the memory address.
  @param  Delay                 The number of 100ns units to poll. Note that timer available may be of poorer
                                granularity so the delay may be longer.

 @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
 @retval RETURN_SUCCESS          Opcode is added.

**/
RETURN_STATUS
EFIAPI
S3BootScriptSaveIoPoll (
  IN S3_BOOT_SCRIPT_LIB_WIDTH       Width,
  IN UINT64                     Address,
  IN VOID                      *Data,
  IN VOID                      *DataMask, 
  IN UINT64                     Delay   
  )
{
  UINT8                 WidthInByte;  
  UINT8                *Script;
  UINT8                 Length;
  EFI_BOOT_SCRIPT_IO_POLL  ScriptIoPoll;
  

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));  
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_IO_POLL) + (WidthInByte * 2));
 
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  } 
  //
  // Build script data
  //
  ScriptIoPoll.OpCode   = EFI_BOOT_SCRIPT_IO_POLL_OPCODE;
  ScriptIoPoll.Length   = (UINT8) (sizeof (EFI_BOOT_SCRIPT_IO_POLL) + (WidthInByte * 2));
  ScriptIoPoll.Width    = Width;  
  ScriptIoPoll.Address  = Address;
  ScriptIoPoll.Delay    = Delay;

  CopyMem ((VOID*)Script, (VOID*)&ScriptIoPoll, sizeof (EFI_BOOT_SCRIPT_IO_POLL));  
  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_IO_POLL)), Data, WidthInByte);
  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_IO_POLL) + WidthInByte), DataMask, WidthInByte);
  
  SyncBootScript ();

  return RETURN_SUCCESS;
}

/**
  Adds a record for PCI configuration space reads and continues when the exit criteria is satisfied or
  after a defined duration.

  @param  Width                 The width of the I/O operations. 
  @param  Address               The address within the PCI configuration space.
  @param  Data                  The comparison value used for the polling exit criteria.
  @param  DataMask              Mask used for the polling criteria. The bits in the bytes below Width which are zero
                                in Data are ignored when polling the memory address
  @param  Delay                 The number of 100ns units to poll. Note that timer available may be of poorer
                                granularity so the delay may be longer.

 @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
 @retval RETURN_SUCCESS           Opcode is added.

**/
RETURN_STATUS
EFIAPI
S3BootScriptSavePciPoll (
   IN S3_BOOT_SCRIPT_LIB_WIDTH   Width,
   IN UINT64                     Address,
   IN VOID                      *Data,
   IN VOID                      *DataMask,
   IN UINT64                     Delay
)
{
  UINT8                   *Script;
  UINT8                    WidthInByte;  
  UINT8                    Length;
  EFI_BOOT_SCRIPT_PCI_CONFIG_POLL  ScriptPciPoll;

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_POLL) + (WidthInByte * 2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptPciPoll.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG_POLL_OPCODE;
  ScriptPciPoll.Length   = (UINT8) (sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_POLL) + (WidthInByte * 2));
  ScriptPciPoll.Width    = Width;  
  ScriptPciPoll.Address  = Address;
  ScriptPciPoll.Delay    = Delay;

  CopyMem ((VOID*)Script, (VOID*)&ScriptPciPoll, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_POLL));
  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_POLL)), Data, WidthInByte);
  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_POLL) + WidthInByte), DataMask, WidthInByte);
  
  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Adds a record for PCI configuration space reads and continues when the exit criteria is satisfied or
  after a defined duration.

  @param  Width                 The width of the I/O operations. 
  @param  Segment               The PCI segment number for Address.
  @param  Address               The address within the PCI configuration space.
  @param  Data                  The comparison value used for the polling exit criteria.
  @param  DataMask              Mask used for the polling criteria. The bits in the bytes below Width which are zero
                                in Data are ignored when polling the memory address
  @param  Delay                 The number of 100ns units to poll. Note that timer available may be of poorer
                                granularity so the delay may be longer.

 @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
 @retval RETURN_SUCCESS           Opcode is added.
 @note   A known Limitations in the implementation: When interpreting the opcode  EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE
         EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE and EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE, the 'Segment' parameter is assumed as 
         Zero, or else, assert.
**/
RETURN_STATUS
EFIAPI
S3BootScriptSavePci2Poll (
   IN S3_BOOT_SCRIPT_LIB_WIDTH      Width,
   IN UINT16                        Segment,
   IN UINT64                        Address,
   IN VOID                         *Data,
   IN VOID                         *DataMask,
  IN UINT64                         Delay
)
{
  UINT8                    WidthInByte;  
  UINT8                   *Script;
  UINT8                    Length;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL  ScriptPci2Poll;
  
  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL) + (WidthInByte * 2));
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  } 
  //
  // Build script data
  //
  ScriptPci2Poll.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL_OPCODE;
  ScriptPci2Poll.Length   = (UINT8) (sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL) + (WidthInByte * 2));
  ScriptPci2Poll.Width    = Width; 
  ScriptPci2Poll.Segment  = Segment;
  ScriptPci2Poll.Address  = Address;
  ScriptPci2Poll.Delay    = Delay;

  CopyMem ((VOID*)Script, (VOID*)&ScriptPci2Poll, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL));
  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL)), Data, WidthInByte);
  CopyMem ((UINT8 *) (Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL) + WidthInByte), DataMask, WidthInByte);
  
  SyncBootScript ();

  return RETURN_SUCCESS;
}
/**
  Do the calculation of start address from which a new s3 boot script entry will write into.
  
  @param EntryLength      The new entry length.
  @param Position         specifies the position in the boot script table where the opcode will be
                          inserted, either before or after, depending on BeforeOrAfter. 
  @param BeforeOrAfter    The flag to indicate to insert the nod before or after the position.
                          This parameter is effective when InsertFlag is TRUE
  @param Script           return out the position from which the a new s3 boot script entry will write into
**/
VOID
S3BootScriptCalculateInsertAddress (
  IN  UINT8     EntryLength,
  IN  VOID     *Position OPTIONAL,
  IN  BOOLEAN   BeforeOrAfter OPTIONAL,
  OUT UINT8   **Script   
  )
{
   UINTN                            TableLength;
   UINT8                            *S3TableBase;
   UINTN                            PositionOffset; 
   EFI_BOOT_SCRIPT_COMMON_HEADER     ScriptHeader;
   //
   // The entry inserting to table is already added to the end of the table
   //
   TableLength =  mS3BootScriptTablePtr->TableLength - EntryLength;
   S3TableBase = mS3BootScriptTablePtr->TableBase ;
   // 
   // calculate the Position offset
   //
   if (Position != NULL) {
     PositionOffset = (UINTN) ((UINT8 *)Position - S3TableBase);
   
     //
     // If the BeforeOrAfter is FALSE, that means to insert the node right after the node.
     //
     if (!BeforeOrAfter) {
        CopyMem ((VOID*)&ScriptHeader, Position, sizeof(EFI_BOOT_SCRIPT_COMMON_HEADER));  
        PositionOffset += (ScriptHeader.Length);
     }
     //     
     // Insert the node before the adjusted Position
     //
     CopyMem (S3TableBase+PositionOffset+EntryLength, S3TableBase+PositionOffset, TableLength - PositionOffset);  
     //
     // calculate the the start address for the new entry. 
     //
     *Script = S3TableBase + PositionOffset;
       
   } else {
     if (!BeforeOrAfter) {
       //
       //  Insert the node to the end of the table
       //
       *Script = S3TableBase + TableLength; 
     } else {
       // 
       // Insert the node to the beginning of the table
       //
       PositionOffset = (UINTN) sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER);
       CopyMem (S3TableBase+PositionOffset+EntryLength, S3TableBase+PositionOffset, TableLength - PositionOffset); 
       *Script = S3TableBase + PositionOffset; 
     }
   }       
}
/**
  Move the last boot script entry to the position 

  @param  BeforeOrAfter         Specifies whether the opcode is stored before (TRUE) or after (FALSE) the position
                                in the boot script table specified by Position. If Position is NULL or points to
                                NULL then the new opcode is inserted at the beginning of the table (if TRUE) or end
                                of the table (if FALSE).
  @param  Position              On entry, specifies the position in the boot script table where the opcode will be
                                inserted, either before or after, depending on BeforeOrAfter. On exit, specifies
                                the position of the inserted opcode in the boot script table.

  @retval RETURN_OUT_OF_RESOURCES  The table is not available.
  @retval RETURN_INVALID_PARAMETER The Position is not a valid position in the boot script table.
  @retval RETURN_SUCCESS           Opcode is inserted.
**/
RETURN_STATUS
EFIAPI
S3BootScriptMoveLastOpcode (
  IN     BOOLEAN                        BeforeOrAfter,
  IN OUT VOID                         **Position OPTIONAL
)
{
  UINT8*                Script;
  VOID                  *TempPosition;  
  UINTN                 StartAddress;
  UINT32                TableLength;
  EFI_BOOT_SCRIPT_COMMON_HEADER  ScriptHeader;
  BOOLEAN               ValidatePosition;
  UINT8*                LastOpcode;
  UINT8                 TempBootScriptEntry[BOOT_SCRIPT_NODE_MAX_LENGTH];
  
  ValidatePosition = FALSE;
  TempPosition = (Position == NULL) ? NULL:(*Position);
  Script = mS3BootScriptTablePtr->TableBase;
  if (Script == 0) {    
    return EFI_OUT_OF_RESOURCES;
  }
  StartAddress  = (UINTN) Script;
  TableLength   = mS3BootScriptTablePtr->TableLength;
  Script        = Script + sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER);
  LastOpcode    = Script;
  //
  // Find the last boot Script Entry which is not the terminate node
  //
  while ((UINTN) Script < (UINTN) (StartAddress + TableLength)) {    
    CopyMem ((VOID*)&ScriptHeader, Script, sizeof(EFI_BOOT_SCRIPT_COMMON_HEADER));   
    if (TempPosition != NULL && TempPosition == Script) {
      //
      // If the position is specified, the position must be pointed to a boot script entry start address. 
      //
      ValidatePosition = TRUE;
    }
    if (ScriptHeader.OpCode != S3_BOOT_SCRIPT_LIB_TERMINATE_OPCODE) {
      LastOpcode = Script;
    } 
    Script  = Script + ScriptHeader.Length;
  }
  //
  // If the position is specified, but not the start of a boot script entry, it is a invalid input
  //
  if (TempPosition != NULL && !ValidatePosition) {
    return RETURN_INVALID_PARAMETER;
  }
  
  CopyMem ((VOID*)&ScriptHeader, LastOpcode, sizeof(EFI_BOOT_SCRIPT_COMMON_HEADER)); 
  
  CopyMem((VOID*)TempBootScriptEntry, LastOpcode, ScriptHeader.Length); 
  //
  // Find the right position to write the node in
  //
  S3BootScriptCalculateInsertAddress (
    ScriptHeader.Length,
    TempPosition,
    BeforeOrAfter,
    &Script   
  );
  //
  // Copy the node to Boot script table
  //
  CopyMem((VOID*)Script, (VOID*)TempBootScriptEntry, ScriptHeader.Length); 
  //
  // return out the Position
  //
  if (Position != NULL) {
    *Position = Script;
  }
  return RETURN_SUCCESS;
}
/**
  Create a Label node in the boot script table. 
  
  @param  BeforeOrAfter         Specifies whether the opcode is stored before (TRUE) or after (FALSE) the position
                                in the boot script table specified by Position. If Position is NULL or points to
                                NULL then the new opcode is inserted at the beginning of the table (if TRUE) or end
                                of the table (if FALSE).
  @param  Position              On entry, specifies the position in the boot script table where the opcode will be
                                inserted, either before or after, depending on BeforeOrAfter. On exit, specifies
                                the position of the inserted opcode in the boot script table.  
  @param InformationLength      Length of the label in bytes
  @param Information            Label to be logged in the boot scrpit
 
  @retval RETURN_INVALID_PARAMETER The Position is not a valid position in the boot script table.
  @retval RETURN_OUT_OF_RESOURCES  Not enough memory for the table do operation.
  @retval RETURN_SUCCESS           Opcode is added.

**/
RETURN_STATUS
EFIAPI
S3BootScriptLabelInternal (
  IN        BOOLEAN                        BeforeOrAfter,
  IN OUT    VOID                         **Position OPTIONAL, 
  IN        UINT32                         InformationLength, 
  IN CONST  CHAR8                          *Information
  )
{
  UINT8                 Length;
  UINT8                 *Script;
  VOID                  *Buffer;
  EFI_BOOT_SCRIPT_INFORMATION  ScriptInformation;
 
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_INFORMATION) + InformationLength);
  
  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  Buffer =  Script + sizeof (EFI_BOOT_SCRIPT_INFORMATION);
  //
  // Build script data
  //
  ScriptInformation.OpCode     = S3_BOOT_SCRIPT_LIB_LABEL_OPCODE;
  ScriptInformation.Length     = Length;


  ScriptInformation.InformationLength = InformationLength;  

  AsciiStrnCpy (Buffer, Information,(UINTN) InformationLength);  
  ScriptInformation.Information = (EFI_PHYSICAL_ADDRESS) (UINTN) Buffer;
  
  CopyMem ((VOID*)Script, (VOID*)&ScriptInformation, sizeof (EFI_BOOT_SCRIPT_INFORMATION));  
  
  return S3BootScriptMoveLastOpcode (BeforeOrAfter, Position);

}
/**
  Find a label within the boot script table and, if not present, optionally create it.

  @param  BeforeOrAfter         Specifies whether the opcode is stored before (TRUE)
                                or after (FALSE) the position in the boot script table 
                                specified by Position.
  @param  CreateIfNotFound      Specifies whether the label will be created if the label 
                                does not exists (TRUE) or not (FALSE).
  @param  Position              On entry, specifies the position in the boot script table
                                where the opcode will be inserted, either before or after,
                                depending on BeforeOrAfter. On exit, specifies the position
                                of the inserted opcode in the boot script table.
  @param  Label                 Points to the label which will be inserted in the boot script table.

  @retval EFI_SUCCESS           The operation succeeded. A record was added into the
                                specified script table.
  @retval EFI_INVALID_PARAMETER The parameter is illegal or the given boot script is not supported.
                                If the opcode is unknow or not supported because of the PCD 
                                Feature Flags.
  @retval EFI_OUT_OF_RESOURCES  There is insufficient memory to store the boot script.

**/
RETURN_STATUS
EFIAPI 
S3BootScriptLabel (
  IN       BOOLEAN                      BeforeOrAfter,
  IN       BOOLEAN                      CreateIfNotFound,
  IN OUT   VOID                       **Position OPTIONAL,
  IN CONST CHAR8                       *Label
  )
{
  UINT8*                Script;
  UINTN                 StartAddress;
  UINT32                TableLength;
  EFI_BOOT_SCRIPT_COMMON_HEADER  ScriptHeader;
  EFI_BOOT_SCRIPT_TABLE_HEADER   TableHeader;
  UINT32                         LabelLength;
  //
  // Check NULL Label
  //
  if (Label == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check empty Label
  //
  if (Label[0] == '\0') {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check that the script is initialized without adding an entry to the script.
  // The code must search for the label first befor it knows if a new entry needs
  // to be added.
  //
  Script = S3BootScriptGetEntryAddAddress (0);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  
  //
  // Check the header and search for existing label.
  // 
  Script = mS3BootScriptTablePtr->TableBase;
  CopyMem ((VOID*)&TableHeader, Script, sizeof(EFI_BOOT_SCRIPT_TABLE_HEADER));
  if (TableHeader.OpCode != S3_BOOT_SCRIPT_LIB_TABLE_OPCODE) {
    return EFI_INVALID_PARAMETER;
  }
  StartAddress  = (UINTN) Script;
  TableLength   = mS3BootScriptTablePtr->TableLength;
  Script    =     Script + TableHeader.Length;
  while ((UINTN) Script < (UINTN) (StartAddress + TableLength)) {
    
    CopyMem ((VOID*)&ScriptHeader, Script, sizeof(EFI_BOOT_SCRIPT_COMMON_HEADER));   
    if (ScriptHeader.OpCode == S3_BOOT_SCRIPT_LIB_LABEL_OPCODE) {
      if (AsciiStrCmp ((CHAR8 *)(UINTN)(Script+sizeof(EFI_BOOT_SCRIPT_INFORMATION)), Label) == 0) {
        (*Position) = Script; 
        return EFI_SUCCESS;
      }
    } 
    Script  = Script + ScriptHeader.Length;
  }
  if (CreateIfNotFound) {
    LabelLength = (UINT32)AsciiStrSize(Label);
    return S3BootScriptLabelInternal (BeforeOrAfter,Position, LabelLength, Label);     
  } else {
    return EFI_NOT_FOUND;
  }   
}

/**
  Compare two positions in the boot script table and return their relative position.
  @param  Position1             The positions in the boot script table to compare
  @param  Position2             The positions in the boot script table to compare
  @param  RelativePosition      On return, points to the result of the comparison

  @retval EFI_SUCCESS           The operation succeeded. A record was added into the
                                specified script table.
  @retval EFI_INVALID_PARAMETER The parameter is illegal or the given boot script is not supported.
                                If the opcode is unknow or not supported because of the PCD 
                                Feature Flags.
  @retval EFI_OUT_OF_RESOURCES  There is insufficient memory to store the boot script.

**/
RETURN_STATUS
EFIAPI 
S3BootScriptCompare (
  IN  UINT8                       *Position1,
  IN  UINT8                       *Position2,
  OUT UINTN                       *RelativePosition
  )
{
  UINT8*                    Script;
  UINT32                    TableLength; 

  Script = mS3BootScriptTablePtr->TableBase;
  if (Script == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  if (RelativePosition == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  TableLength = ((EFI_BOOT_SCRIPT_TABLE_HEADER*)Script)->TableLength;
  //
  // If in boot time, TableLength does not include the termination node. so add it up 
  //
  if (!mS3BootScriptTablePtr->AtRuntime) {
    TableLength += sizeof(EFI_BOOT_SCRIPT_TERMINATE);
  }
  if (Position1 < Script || Position1 > Script+TableLength) {
    return EFI_INVALID_PARAMETER;
  }
  if (Position2 < Script || Position2 > Script+TableLength) {
    return EFI_INVALID_PARAMETER;
  }
  *RelativePosition = (Position1 < Position2)?-1:((Position1 == Position2)?0:1);
  
  return EFI_SUCCESS;
}


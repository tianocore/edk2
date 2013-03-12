/** @file
  Implement all four UEFI Runtime Variable services for the nonvolatile
  and volatile storage space and install variable architecture protocol.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"
#include "AuthService.h"

extern VARIABLE_STORE_HEADER        *mNvVariableCache;
extern VARIABLE_INFO_ENTRY          *gVariableInfo;
EFI_HANDLE                          mHandle                    = NULL;
EFI_EVENT                           mVirtualAddressChangeEvent = NULL;
EFI_EVENT                           mFtwRegistration           = NULL;

/**
  Return TRUE if ExitBootServices () has been called.

  @retval TRUE If ExitBootServices () has been called.
**/
BOOLEAN
AtRuntime (
  VOID
  )
{
  return EfiAtRuntime ();
}


/**
  Initializes a basic mutual exclusion lock.

  This function initializes a basic mutual exclusion lock to the released state
  and returns the lock.  Each lock provides mutual exclusion access at its task
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.
  If Lock is NULL, then ASSERT().
  If Priority is not a valid TPL value, then ASSERT().

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL is associated with the lock.

  @return The lock.

**/
EFI_LOCK *
InitializeLock (
  IN OUT EFI_LOCK                         *Lock,
  IN     EFI_TPL                          Priority
  )
{
  return EfiInitializeLock (Lock, Priority);
}


/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function that will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK                             *Lock
  )
{
  if (!AtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}


/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK                             *Lock
  )
{
  if (!AtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Retrive the Fault Tolerent Write protocol interface.

  @param[out] FtwProtocol       The interface of Ftw protocol

  @retval EFI_SUCCESS           The FTW protocol instance was found and returned in FtwProtocol.
  @retval EFI_NOT_FOUND         The FTW protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID                                **FtwProtocol
  )
{
  EFI_STATUS                              Status;

  //
  // Locate Fault Tolerent Write protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiFaultTolerantWriteProtocolGuid,
                  NULL,
                  FtwProtocol
                  );
  return Status;
}

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
GetFvbByHandle (
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
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableClassAddressChangeEvent (
  IN EFI_EVENT                            Event,
  IN VOID                                 *Context
  )
{
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance->GetBlockSize);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance->GetPhysicalAddress);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance->GetAttributes);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance->SetAttributes);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance->Read);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance->Write);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance->EraseBlocks);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->FvbInstance);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->PlatformLangCodes);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->LangCodes);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->PlatformLang);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal);
  EfiConvertPointer (0x0, (VOID **) &mHashCtx);
  EfiConvertPointer (0x0, (VOID **) &mStorageArea);
  EfiConvertPointer (0x0, (VOID **) &mSerializationRuntimeBuffer);
  EfiConvertPointer (0x0, (VOID **) &mNvVariableCache);
}


/**
  Notification function of EVT_GROUP_READY_TO_BOOT event group.

  This is a notification function registered on EVT_GROUP_READY_TO_BOOT event group.
  When the Boot Manager is about to load and execute a boot option, it reclaims variable
  storage if free size is below the threshold.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
OnReadyToBoot (
  EFI_EVENT                               Event,
  VOID                                    *Context
  )
{
  ReclaimForOS ();
  if (FeaturePcdGet (PcdVariableCollectStatistics)) {
    gBS->InstallConfigurationTable (&gEfiAuthenticatedVariableGuid, gVariableInfo);
  }
}


/**
  Fault Tolerant Write protocol notification event handler.

  Non-Volatile variable write may needs FTW protocol to reclaim when
  writting variable.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
FtwNotificationEvent (
  IN  EFI_EVENT                           Event,
  IN  VOID                                *Context
  )
{
  EFI_STATUS                              Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      *FvbProtocol;
  EFI_FAULT_TOLERANT_WRITE_PROTOCOL       *FtwProtocol;
  EFI_PHYSICAL_ADDRESS                    NvStorageVariableBase;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR         GcdDescriptor;
  EFI_PHYSICAL_ADDRESS                    BaseAddress;
  UINT64                                  Length;
  EFI_PHYSICAL_ADDRESS                    VariableStoreBase;
  UINT64                                  VariableStoreLength;

  //
  // Ensure FTW protocol is installed.
  //
  Status = GetFtwProtocol ((VOID**) &FtwProtocol);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Find the proper FVB protocol for variable.
  //
  NvStorageVariableBase = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageVariableBase64);
  if (NvStorageVariableBase == 0) {
    NvStorageVariableBase = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageVariableBase);
  }
  Status = GetFvbInfoByAddress (NvStorageVariableBase, NULL, &FvbProtocol);
  if (EFI_ERROR (Status)) {
    return ;
  }
  mVariableModuleGlobal->FvbInstance = FvbProtocol;

  //
  // Mark the variable storage region of the FLASH as RUNTIME.
  //
  VariableStoreBase   = mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
  VariableStoreLength = ((VARIABLE_STORE_HEADER *)(UINTN)VariableStoreBase)->Size;
  BaseAddress = VariableStoreBase & (~EFI_PAGE_MASK);
  Length      = VariableStoreLength + (VariableStoreBase - BaseAddress);
  Length      = (Length + EFI_PAGE_SIZE - 1) & (~EFI_PAGE_MASK);

  Status      = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdDescriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Variable driver failed to add EFI_MEMORY_RUNTIME attribute to Flash.\n"));
  } else {
    Status = gDS->SetMemorySpaceAttributes (
                    BaseAddress,
                    Length,
                    GcdDescriptor.Attributes | EFI_MEMORY_RUNTIME
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Variable driver failed to add EFI_MEMORY_RUNTIME attribute to Flash.\n"));
    }
  }

  Status = VariableWriteServiceInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // Install the Variable Write Architectural protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiVariableWriteArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Close the notify event to avoid install gEfiVariableWriteArchProtocolGuid again.
  //
  gBS->CloseEvent (Event);

}


/**
  Variable Driver main entry point. The Variable driver places the 4 EFI
  runtime services in the EFI System Table and installs arch protocols
  for variable read and write services being available. It also registers
  a notification function for an EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                            Status;
  EFI_EVENT                             ReadyToBootEvent;

  Status = VariableCommonInitialize ();
  ASSERT_EFI_ERROR (Status);

  SystemTable->RuntimeServices->GetVariable         = VariableServiceGetVariable;
  SystemTable->RuntimeServices->GetNextVariableName = VariableServiceGetNextVariableName;
  SystemTable->RuntimeServices->SetVariable         = VariableServiceSetVariable;
  SystemTable->RuntimeServices->QueryVariableInfo   = VariableServiceQueryVariableInfo;

  //
  // Now install the Variable Runtime Architectural protocol on a new handle.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiVariableArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register FtwNotificationEvent () notify function.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiFaultTolerantWriteProtocolGuid,
    TPL_CALLBACK,
    FtwNotificationEvent,
    (VOID *)SystemTable,
    &mFtwRegistration
    );

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VariableClassAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register the event handling function to reclaim variable for OS usage.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_NOTIFY,
             OnReadyToBoot,
             NULL,
             &ReadyToBootEvent
             );

  return EFI_SUCCESS;
}


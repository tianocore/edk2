/** @file
  MM Core Main Entry Point

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"

EFI_STATUS
MmDispatcher (
  VOID
  );

//
// Globals used to initialize the protocol
//
EFI_HANDLE  mMmCpuHandle = NULL;

//
// MM Core global variable for MM System Table.  Only accessed as a physical structure in MMRAM.
//
EFI_MM_SYSTEM_TABLE  gMmCoreMmst = {
  // The table header for the MMST.
  {
    MM_MMST_SIGNATURE,
    EFI_MM_SYSTEM_TABLE_REVISION,
    sizeof (gMmCoreMmst.Hdr)
  },
  // MmFirmwareVendor
  NULL,
  // MmFirmwareRevision
  0,
  // MmInstallConfigurationTable
  MmInstallConfigurationTable,
  // I/O Service
  {
    {
      (EFI_MM_CPU_IO)MmEfiNotAvailableYetArg5,        // MmMemRead
      (EFI_MM_CPU_IO)MmEfiNotAvailableYetArg5         // MmMemWrite
    },
    {
      (EFI_MM_CPU_IO)MmEfiNotAvailableYetArg5,        // MmIoRead
      (EFI_MM_CPU_IO)MmEfiNotAvailableYetArg5         // MmIoWrite
    }
  },
  // Runtime memory services
  MmAllocatePool,
  MmFreePool,
  MmAllocatePages,
  MmFreePages,
  // MP service
  NULL,                          // MmStartupThisAp
  0,                             // CurrentlyExecutingCpu
  0,                             // NumberOfCpus
  NULL,                          // CpuSaveStateSize
  NULL,                          // CpuSaveState
  0,                             // NumberOfTableEntries
  NULL,                          // MmConfigurationTable
  MmInstallProtocolInterface,
  MmUninstallProtocolInterface,
  MmHandleProtocol,
  MmRegisterProtocolNotify,
  MmLocateHandle,
  MmLocateProtocol,
  MmiManage,
  MmiHandlerRegister,
  MmiHandlerUnRegister
};

//
// Table of MMI Handlers that are registered by the MM Core when it is initialized
//
MM_CORE_MMI_HANDLERS  mMmCoreMmiHandlers[] = {
  { MmDriverDispatchHandler,  &gEventMmDispatchGuid,             NULL, FALSE },
  { MmReadyToLockHandler,     &gEfiDxeMmReadyToLockProtocolGuid, NULL, TRUE  },
  { MmEndOfPeiHandler,        &gEfiMmEndOfPeiProtocol,           NULL, FALSE },
  { MmEndOfDxeHandler,        &gEfiEndOfDxeEventGroupGuid,       NULL, FALSE },
  { MmExitBootServiceHandler, &gEfiEventExitBootServicesGuid,    NULL, FALSE },
  { MmReadyToBootHandler,     &gEfiEventReadyToBootGuid,         NULL, FALSE },
  { NULL,                     NULL,                              NULL, FALSE },
};

BOOLEAN         mMmEntryPointRegistered = FALSE;
MM_COMM_BUFFER  *mMmCommunicationBuffer;
VOID            *mInternalCommBufferCopy;

/**
  Place holder function until all the MM System Table Service are available.

  Note: This function is only used by MMRAM invocation.  It is never used by DXE invocation.

  @param  Arg1                   Undefined
  @param  Arg2                   Undefined
  @param  Arg3                   Undefined
  @param  Arg4                   Undefined
  @param  Arg5                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
MmEfiNotAvailableYetArg5 (
  UINTN  Arg1,
  UINTN  Arg2,
  UINTN  Arg3,
  UINTN  Arg4,
  UINTN  Arg5
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.
  //
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Software MMI handler that is called when a ExitBoot Service event is signaled.

  @param  DispatchHandle  The unique handle assigned to this handler by MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
MmExitBootServiceHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_HANDLE      MmHandle;
  EFI_STATUS      Status;
  STATIC BOOLEAN  mInExitBootServices = FALSE;

  Status = EFI_SUCCESS;
  if (!mInExitBootServices) {
    MmHandle = NULL;
    Status   = MmInstallProtocolInterface (
                 &MmHandle,
                 &gEfiEventExitBootServicesGuid,
                 EFI_NATIVE_INTERFACE,
                 NULL
                 );
  }

  mInExitBootServices = TRUE;
  return Status;
}

/**
  Software MMI handler that is called when a ExitBoot Service event is signaled.

  @param  DispatchHandle  The unique handle assigned to this handler by MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
MmReadyToBootHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_HANDLE      MmHandle;
  EFI_STATUS      Status;
  STATIC BOOLEAN  mInReadyToBoot = FALSE;

  Status = EFI_SUCCESS;
  if (!mInReadyToBoot) {
    MmHandle = NULL;
    Status   = MmInstallProtocolInterface (
                 &MmHandle,
                 &gEfiEventReadyToBootGuid,
                 EFI_NATIVE_INTERFACE,
                 NULL
                 );
  }

  mInReadyToBoot = TRUE;
  return Status;
}

/**
  Software MMI handler that is called when the DxeMmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signaled.  This function unregisters the
  Software SMIs that are nor required after MMRAM is locked and installs the
  MM Ready To Lock Protocol so MM Drivers are informed that MMRAM is about
  to be locked.

  @param  DispatchHandle  The unique handle assigned to this handler by MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
MmReadyToLockHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_HANDLE  MmHandle;

  DEBUG ((DEBUG_INFO, "MmReadyToLockHandler\n"));

  //
  // Unregister MMI Handlers that are no longer required after the MM driver dispatch is stopped
  //
  for (Index = 0; mMmCoreMmiHandlers[Index].HandlerType != NULL; Index++) {
    if (mMmCoreMmiHandlers[Index].UnRegister) {
      MmiHandlerUnRegister (mMmCoreMmiHandlers[Index].DispatchHandle);
    }
  }

  //
  // Install MM Ready to lock protocol
  //
  MmHandle = NULL;
  Status   = MmInstallProtocolInterface (
               &MmHandle,
               &gEfiMmReadyToLockProtocolGuid,
               EFI_NATIVE_INTERFACE,
               NULL
               );

  //
  // Make sure MM CPU I/O 2 Protocol has been installed into the handle database
  //
  // Status = MmLocateProtocol (&EFI_MM_CPU_IO_PROTOCOL_GUID, NULL, &Interface);

  //
  // Print a message on a debug build if the MM CPU I/O 2 Protocol is not installed
  //
  // if (EFI_ERROR (Status)) {
  // DEBUG ((DEBUG_ERROR, "\nSMM: SmmCpuIo Arch Protocol not present!!\n"));
  // }

  //
  // Assert if the CPU I/O 2 Protocol is not installed
  //
  // ASSERT_EFI_ERROR (Status);

  //
  // Display any drivers that were not dispatched because dependency expression
  // evaluated to false if this is a debug build
  //
  // MmDisplayDiscoveredNotDispatched ();

  return Status;
}

/**
  Software MMI handler that is called when the EndOfPei event is signaled.
  This function installs the MM EndOfPei Protocol so MM Drivers are informed that
  EndOfPei event is signaled.

  @param  DispatchHandle  The unique handle assigned to this handler by MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
MmEndOfPeiHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  MmHandle;

  DEBUG ((DEBUG_INFO, "MmEndOfPeiHandler\n"));
  //
  // Install MM EndOfDxe protocol
  //
  MmHandle = NULL;
  Status   = MmInstallProtocolInterface (
               &MmHandle,
               &gEfiMmEndOfPeiProtocol,
               EFI_NATIVE_INTERFACE,
               NULL
               );
  return Status;
}

/**
  Software MMI handler that is called when the EndOfDxe event is signaled.
  This function installs the MM EndOfDxe Protocol so MM Drivers are informed that
  platform code will invoke 3rd part code.

  @param  DispatchHandle  The unique handle assigned to this handler by MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
MmEndOfDxeHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  MmHandle;

  DEBUG ((DEBUG_INFO, "MmEndOfDxeHandler\n"));
  //
  // Install MM EndOfDxe protocol
  //
  MmHandle = NULL;
  Status   = MmInstallProtocolInterface (
               &MmHandle,
               &gEfiMmEndOfDxeProtocolGuid,
               EFI_NATIVE_INTERFACE,
               NULL
               );
  return Status;
}

/**
  Install LoadedImage protocol for MM Core.

**/
VOID
MmCoreInstallLoadedImage (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_PHYSICAL_ADDRESS       MmCoreImageBaseAddress;
  UINT64                     MmCoreImageLength;
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_HANDLE                 ImageHandle;

  //
  // Searching for Memory Allocation HOB
  //
  Hob.Raw = GetHobList ();
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
    //
    // Find MM Core HOB
    //
    if (CompareGuid (
          &Hob.MemoryAllocationModule->MemoryAllocationHeader.Name,
          &gEfiHobMemoryAllocModuleGuid
          ))
    {
      if (CompareGuid (&Hob.MemoryAllocationModule->ModuleName, &gEfiCallerIdGuid)) {
        break;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  if (Hob.Raw == NULL) {
    return;
  }

  MmCoreImageBaseAddress = Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryBaseAddress;
  MmCoreImageLength      = Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryLength;

  //
  // Allocate a Loaded Image Protocol in MM
  //
  LoadedImage = AllocatePool (sizeof (EFI_LOADED_IMAGE_PROTOCOL));
  ASSERT (LoadedImage != NULL);
  if (LoadedImage == NULL) {
    return;
  }

  ZeroMem (LoadedImage, sizeof (EFI_LOADED_IMAGE_PROTOCOL));

  //
  // Fill in the remaining fields of the Loaded Image Protocol instance.
  //
  LoadedImage->Revision     = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
  LoadedImage->ParentHandle = NULL;
  LoadedImage->SystemTable  = NULL;

  LoadedImage->ImageBase     = (VOID *)(UINTN)MmCoreImageBaseAddress;
  LoadedImage->ImageSize     = MmCoreImageLength;
  LoadedImage->ImageCodeType = EfiRuntimeServicesCode;
  LoadedImage->ImageDataType = EfiRuntimeServicesData;

  //
  // Create a new image handle in the MM handle database for the MM Core
  //
  ImageHandle = NULL;
  Status      = MmInstallProtocolInterface (
                  &ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  Prepare communication buffer for MMI.
**/
VOID
MmCorePrepareCommunicationBuffer (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_HOB_GUID_TYPE     *GuidHob;
  EFI_PHYSICAL_ADDRESS  Buffer;

  mMmCommunicationBuffer  = NULL;
  mInternalCommBufferCopy = NULL;

  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return;
  }

  mMmCommunicationBuffer = (MM_COMM_BUFFER *)GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((
    DEBUG_INFO,
    "MM Communication Buffer is at %x, number of pages is %x\n",
    mMmCommunicationBuffer->PhysicalStart,
    mMmCommunicationBuffer->NumberOfPages
    ));
  ASSERT (mMmCommunicationBuffer->PhysicalStart != 0 && mMmCommunicationBuffer->NumberOfPages != 0);

  if (!MmIsBufferOutsideMmValid (
         mMmCommunicationBuffer->PhysicalStart,
         EFI_PAGES_TO_SIZE (mMmCommunicationBuffer->NumberOfPages)
         ))
  {
    mMmCommunicationBuffer = NULL;
    DEBUG ((DEBUG_ERROR, "MM Communication Buffer is invalid!\n"));
    ASSERT (FALSE);
    return;
  }

  Status = MmAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesData,
             mMmCommunicationBuffer->NumberOfPages,
             &Buffer
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return;
  }

  mInternalCommBufferCopy = (VOID *)(UINTN)Buffer;
  DEBUG ((DEBUG_INFO, "Internal Communication Buffer Copy is at %p\n", mInternalCommBufferCopy));
}

/**
  The main entry point to MM Foundation.

  Note: This function is only used by MMRAM invocation.  It is never used by DXE invocation.

  @param  MmEntryContext           Processor information and functionality
                                    needed by MM Foundation.

**/
VOID
EFIAPI
MmEntryPoint (
  IN CONST EFI_MM_ENTRY_CONTEXT  *MmEntryContext
  )
{
  EFI_STATUS                 Status;
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  MM_COMM_BUFFER_STATUS      *CommunicationStatus;
  UINTN                      BufferSize;

  DEBUG ((DEBUG_INFO, "MmEntryPoint ...\n"));

  //
  // Update MMST using the context
  //
  CopyMem (&gMmCoreMmst.MmStartupThisAp, MmEntryContext, sizeof (EFI_MM_ENTRY_CONTEXT));

  //
  // Call platform hook before Mm Dispatch
  //
  // PlatformHookBeforeMmDispatch ();

  //
  // Check to see if this is a Synchronous MMI sent through the MM Communication
  // Protocol or an Asynchronous MMI
  //
  if ((mMmCommunicationBuffer != NULL) && (mInternalCommBufferCopy != NULL)) {
    CommunicationStatus = (MM_COMM_BUFFER_STATUS *)(UINTN)mMmCommunicationBuffer->Status;
    if (CommunicationStatus->IsCommBufferValid) {
      //
      // Synchronous MMI for MM Core or request from Communicate protocol
      //
      CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)mMmCommunicationBuffer->PhysicalStart;
      BufferSize        = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + CommunicateHeader->MessageLength;
      if (BufferSize <= EFI_PAGES_TO_SIZE (mMmCommunicationBuffer->NumberOfPages)) {
        //
        // Shadow the data from MM Communication Buffer to internal buffer
        //
        CopyMem (
          mInternalCommBufferCopy,
          (VOID *)(UINTN)mMmCommunicationBuffer->PhysicalStart,
          BufferSize
          );
        ZeroMem (
          (UINT8 *)mInternalCommBufferCopy + BufferSize,
          EFI_PAGES_TO_SIZE (mMmCommunicationBuffer->NumberOfPages) - BufferSize
          );

        CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)mInternalCommBufferCopy;
        BufferSize        = CommunicateHeader->MessageLength;
        Status            = MmiManage (
                              &CommunicateHeader->HeaderGuid,
                              NULL,
                              CommunicateHeader->Data,
                              &BufferSize
                              );

        BufferSize = BufferSize + OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
        if (BufferSize <= EFI_PAGES_TO_SIZE (mMmCommunicationBuffer->NumberOfPages)) {
          //
          // Copy the data back to MM Communication Buffer
          //
          CopyMem (
            (VOID *)(UINTN)mMmCommunicationBuffer->PhysicalStart,
            mInternalCommBufferCopy,
            BufferSize
            );
        } else {
          DEBUG ((DEBUG_ERROR, "Returned buffer size is larger than the size of MM Communication Buffer\n"));
          ASSERT (FALSE);
        }

        //
        // Update CommunicationBuffer, BufferSize and ReturnStatus
        // Communicate service finished, reset the pointer to CommBuffer to NULL
        //
        CommunicationStatus->ReturnBufferSize = BufferSize;
        CommunicationStatus->ReturnStatus     = (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_NOT_FOUND;
      } else {
        DEBUG ((DEBUG_ERROR, "Input buffer size is larger than the size of MM Communication Buffer\n"));
        ASSERT (FALSE);
      }
    }
  } else {
    DEBUG ((DEBUG_ERROR, "No valid communication buffer, no Synchronous MMI will be processed\n"));
  }

  //
  // Process Asynchronous MMI sources
  //
  MmiManage (NULL, NULL, NULL, NULL);

  //
  // TBD: Do not use private data structure ?
  //

  DEBUG ((DEBUG_INFO, "MmEntryPoint Done\n"));
}

/** Register the MM Entry Point provided by the MM Core with the
    MM Configuration protocol.

  @param [in]  Protocol   Pointer to the protocol.
  @param [in]  Interface  Pointer to the MM Configuration protocol.
  @param [in]  Handle     Handle.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
MmConfigurationMmNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                     Status;
  EFI_MM_CONFIGURATION_PROTOCOL  *MmConfiguration;

  DEBUG ((DEBUG_INFO, "MmConfigurationMmNotify(%g) - %x\n", Protocol, Interface));

  MmConfiguration = Interface;

  //
  // Register the MM Entry Point provided by the MM Core with the MM COnfiguration protocol
  //
  Status = MmConfiguration->RegisterMmEntry (MmConfiguration, (EFI_MM_ENTRY_POINT)MmEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Set flag to indicate that the MM Entry Point has been registered which
  // means that MMIs are now fully operational.
  //
  mMmEntryPointRegistered = TRUE;

  //
  // Print debug message showing MM Core entry point address.
  //
  DEBUG ((DEBUG_INFO, "MM Core registered MM Entry Point address %p\n", MmEntryPoint));
  return EFI_SUCCESS;
}

/**
  Migrate MemoryBaseAddress in memory allocation HOBs with BootServiceData
  type and non-zero GUID name from Boot Service memory to MMRAM.

  @param[in]  HobStart       Pointer to the start of the HOB list.

**/
VOID
MigrateMemoryAllocationHobs (
  IN VOID  *HobStart
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;
  VOID                       *MemoryInMmram;

  MemoryAllocationHob = NULL;
  Hob.Raw             = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, HobStart);
  while (Hob.Raw != NULL) {
    MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
    if ((MemoryAllocationHob->AllocDescriptor.MemoryType == EfiBootServicesData) &&
        (MmIsBufferOutsideMmValid (
           MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress,
           MemoryAllocationHob->AllocDescriptor.MemoryLength
           ))
        )
    {
      if (!IsZeroGuid (&MemoryAllocationHob->AllocDescriptor.Name)) {
        MemoryInMmram = AllocatePages (EFI_SIZE_TO_PAGES (MemoryAllocationHob->AllocDescriptor.MemoryLength));
        if (MemoryInMmram != NULL) {
          DEBUG ((
            DEBUG_INFO,
            "Migrate Memory Allocation Hob (%g) from %08x to %08p\n",
            &MemoryAllocationHob->AllocDescriptor.Name,
            MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress,
            MemoryInMmram
            ));
          CopyMem (
            MemoryInMmram,
            (VOID *)(UINTN)MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress,
            MemoryAllocationHob->AllocDescriptor.MemoryLength
            );
          MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)MemoryInMmram;
          MemoryAllocationHob->AllocDescriptor.MemoryType        = EfiRuntimeServicesData;
        }
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "Error - Memory Allocation Hob [%08x, %08x] doesn't have a GUID name specified\n",
          MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress,
          MemoryAllocationHob->AllocDescriptor.MemoryLength
          ));
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
  }
}

/** Returns the HOB list size.

  @param [in]  HobStart   Pointer to the start of the HOB list.

  @retval Size of the HOB list.
**/
UINTN
GetHobListSize (
  IN VOID  *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *)HobStart;
  while (!END_OF_HOB_LIST (Hob)) {
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  //
  // Need plus END_OF_HOB_LIST
  //
  return (UINTN)Hob.Raw - (UINTN)HobStart + sizeof (EFI_HOB_GENERIC_HEADER);
}

/**
  The Entry Point for MM Core

  Install DXE Protocols and reload MM Core into MMRAM and register MM Core
  EntryPoint on the MMI vector.

  Note: This function is called for both DXE invocation and MMRAM invocation.

  @param  HobStart       Pointer to the start of the HOB list.

  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Unsupported operation.
**/
EFI_STATUS
EFIAPI
StandaloneMmMain (
  IN VOID  *HobStart
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  VOID                            *MmHobStart;
  UINTN                           HobSize;
  VOID                            *Registration;
  EFI_HOB_GUID_TYPE               *MmramRangesHob;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *MmramRangesHobData;
  EFI_MMRAM_DESCRIPTOR            *MmramRanges;
  UINTN                           MmramRangeCount;
  EFI_HOB_FIRMWARE_VOLUME         *BfvHob;

  ProcessLibraryConstructorList (HobStart, &gMmCoreMmst);

  DEBUG ((DEBUG_INFO, "MmMain - 0x%x\n", HobStart));

  DEBUG_CODE (
    PrintHobList (HobStart, NULL);
    );

  //
  // Extract the MMRAM ranges from the MMRAM descriptor HOB
  //
  MmramRangesHob = GetNextGuidHob (&gEfiSmmSmramMemoryGuid, HobStart);
  if (MmramRangesHob == NULL) {
    MmramRangesHob = GetNextGuidHob (&gEfiMmPeiMmramMemoryReserveGuid, HobStart);
    if (MmramRangesHob == NULL) {
      return EFI_UNSUPPORTED;
    }
  }

  MmramRangesHobData = GET_GUID_HOB_DATA (MmramRangesHob);
  ASSERT (MmramRangesHobData != NULL);
  MmramRanges     = MmramRangesHobData->Descriptor;
  MmramRangeCount = (UINTN)MmramRangesHobData->NumberOfMmReservedRegions;
  ASSERT (MmramRanges);
  ASSERT (MmramRangeCount);

  //
  // Print the MMRAM ranges passed by the caller
  //
  DEBUG ((DEBUG_INFO, "MmramRangeCount - 0x%x\n", MmramRangeCount));
  for (Index = 0; Index < MmramRangeCount; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "MmramRanges[%d]: 0x%016lx - 0x%lx\n",
      Index,
      MmramRanges[Index].CpuStart,
      MmramRanges[Index].PhysicalSize
      ));
  }

  //
  // No need to initialize memory service.
  // It is done in the constructor of StandaloneMmCoreMemoryAllocationLib(),
  // so that the library linked with StandaloneMmCore can use AllocatePool() in
  // the constructor.

  DEBUG ((DEBUG_INFO, "MmInstallConfigurationTable For HobList\n"));
  //
  // Install HobList
  //
  HobSize = GetHobListSize (HobStart);
  DEBUG ((DEBUG_INFO, "HobSize - 0x%x\n", HobSize));
  MmHobStart = AllocatePool (HobSize);
  DEBUG ((DEBUG_INFO, "MmHobStart - 0x%x\n", MmHobStart));
  ASSERT (MmHobStart != NULL);
  CopyMem (MmHobStart, HobStart, HobSize);
  Status = MmInstallConfigurationTable (&gMmCoreMmst, &gEfiHobListGuid, MmHobStart, HobSize);
  ASSERT_EFI_ERROR (Status);
  MigrateMemoryAllocationHobs (MmHobStart);
  gHobList = MmHobStart;

  //
  // Register notification for EFI_MM_CONFIGURATION_PROTOCOL registration and
  // use it to register the MM Foundation entrypoint
  //
  DEBUG ((DEBUG_INFO, "MmRegisterProtocolNotify - MmConfigurationMmProtocol\n"));
  Status = MmRegisterProtocolNotify (
             &gEfiMmConfigurationProtocolGuid,
             MmConfigurationMmNotify,
             &Registration
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Get Boot Firmware Volume address from the BFV Hob
  //
  BfvHob = GetFirstHob (EFI_HOB_TYPE_FV);
  if (BfvHob != NULL) {
    DEBUG ((DEBUG_INFO, "BFV address - 0x%x\n", BfvHob->BaseAddress));
    DEBUG ((DEBUG_INFO, "BFV size    - 0x%x\n", BfvHob->Length));
    //
    // Dispatch standalone BFV
    //
    if (BfvHob->BaseAddress != 0) {
      DEBUG ((DEBUG_INFO, "Mm Dispatch StandaloneBfvAddress - 0x%08x\n", BfvHob->BaseAddress));
      MmCoreFfsFindMmDriver ((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)BfvHob->BaseAddress, 0);
      MmDispatcher ();
    }
  }

  //
  // Register all handlers in the core table
  //
  for (Index = 0; mMmCoreMmiHandlers[Index].HandlerType != NULL; Index++) {
    Status = MmiHandlerRegister (
               mMmCoreMmiHandlers[Index].Handler,
               mMmCoreMmiHandlers[Index].HandlerType,
               &mMmCoreMmiHandlers[Index].DispatchHandle
               );
    DEBUG ((DEBUG_INFO, "MmiHandlerRegister - GUID %g - Status %d\n", mMmCoreMmiHandlers[Index].HandlerType, Status));
  }

  MmCorePrepareCommunicationBuffer ();

  //
  // Install Loaded Image Protocol form MM Core
  //
  MmCoreInstallLoadedImage ();

  DEBUG ((DEBUG_INFO, "MmMain Done!\n"));

  return EFI_SUCCESS;
}

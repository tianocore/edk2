/** @file
  MM Core Main Entry Point

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"

EFI_STATUS
MmCoreFfsFindMmDriver (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  );

EFI_STATUS
MmDispatcher (
  VOID
  );

//
// Globals used to initialize the protocol
//
EFI_HANDLE            mMmCpuHandle = NULL;

//
// Physical pointer to private structure shared between MM IPL and the MM Core
//
MM_CORE_PRIVATE_DATA  *gMmCorePrivate;

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
      (EFI_MM_CPU_IO) MmEfiNotAvailableYetArg5,       // MmMemRead
      (EFI_MM_CPU_IO) MmEfiNotAvailableYetArg5        // MmMemWrite
    },
    {
      (EFI_MM_CPU_IO) MmEfiNotAvailableYetArg5,       // MmIoRead
      (EFI_MM_CPU_IO) MmEfiNotAvailableYetArg5        // MmIoWrite
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
  { MmReadyToLockHandler,    &gEfiDxeMmReadyToLockProtocolGuid,  NULL, TRUE  },
  { MmEndOfDxeHandler,       &gEfiEndOfDxeEventGroupGuid,        NULL, FALSE },
  { MmExitBootServiceHandler,&gEfiEventExitBootServicesGuid,     NULL, FALSE },
  { MmReadyToBootHandler,    &gEfiEventReadyToBootGuid,          NULL, FALSE },
  { NULL,                    NULL,                               NULL, FALSE },
};

EFI_SYSTEM_TABLE                *mEfiSystemTable;
UINTN                           mMmramRangeCount;
EFI_MMRAM_DESCRIPTOR            *mMmramRanges;

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
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
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
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_HANDLE  MmHandle;
  EFI_STATUS  Status = EFI_SUCCESS;
  STATIC BOOLEAN mInExitBootServices = FALSE;

  if (!mInExitBootServices) {
    MmHandle = NULL;
    Status = MmInstallProtocolInterface (
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
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_HANDLE  MmHandle;
  EFI_STATUS  Status = EFI_SUCCESS;
  STATIC BOOLEAN mInReadyToBoot = FALSE;

  if (!mInReadyToBoot) {
    MmHandle = NULL;
    Status = MmInstallProtocolInterface (
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
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
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
  Status = MmInstallProtocolInterface (
             &MmHandle,
             &gEfiMmReadyToLockProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );

  //
  // Make sure MM CPU I/O 2 Protocol has been installed into the handle database
  //
  //Status = MmLocateProtocol (&EFI_MM_CPU_IO_PROTOCOL_GUID, NULL, &Interface);

  //
  // Print a message on a debug build if the MM CPU I/O 2 Protocol is not installed
  //
  //if (EFI_ERROR (Status)) {
      //DEBUG ((DEBUG_ERROR, "\nSMM: SmmCpuIo Arch Protocol not present!!\n"));
  //}


  //
  // Assert if the CPU I/O 2 Protocol is not installed
  //
  //ASSERT_EFI_ERROR (Status);

  //
  // Display any drivers that were not dispatched because dependency expression
  // evaluated to false if this is a debug build
  //
  //MmDisplayDiscoveredNotDispatched ();

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
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
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
  Status = MmInstallProtocolInterface (
             &MmHandle,
             &gEfiMmEndOfDxeProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );
  return Status;
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
  EFI_STATUS                  Status;
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;

  DEBUG ((DEBUG_INFO, "MmEntryPoint ...\n"));

  //
  // Update MMST using the context
  //
  CopyMem (&gMmCoreMmst.MmStartupThisAp, MmEntryContext, sizeof (EFI_MM_ENTRY_CONTEXT));

  //
  // Call platform hook before Mm Dispatch
  //
  //PlatformHookBeforeMmDispatch ();

  //
  // If a legacy boot has occured, then make sure gMmCorePrivate is not accessed
  //

  //
  // TBD: Mark the InMm flag as TRUE
  //
  gMmCorePrivate->InMm = TRUE;

  //
  // Check to see if this is a Synchronous MMI sent through the MM Communication
  // Protocol or an Asynchronous MMI
  //
  if (gMmCorePrivate->CommunicationBuffer != 0) {
    //
    // Synchronous MMI for MM Core or request from Communicate protocol
    //
    if (!MmIsBufferOutsideMmValid ((UINTN)gMmCorePrivate->CommunicationBuffer, gMmCorePrivate->BufferSize)) {
      //
      // If CommunicationBuffer is not in valid address scope, return EFI_INVALID_PARAMETER
      //
      gMmCorePrivate->CommunicationBuffer = 0;
      gMmCorePrivate->ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)gMmCorePrivate->CommunicationBuffer;
      gMmCorePrivate->BufferSize -= OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
      Status = MmiManage (
                 &CommunicateHeader->HeaderGuid,
                 NULL,
                 CommunicateHeader->Data,
                 (UINTN *)&gMmCorePrivate->BufferSize
                 );
      //
      // Update CommunicationBuffer, BufferSize and ReturnStatus
      // Communicate service finished, reset the pointer to CommBuffer to NULL
      //
      gMmCorePrivate->BufferSize += OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
      gMmCorePrivate->CommunicationBuffer = 0;
      gMmCorePrivate->ReturnStatus = (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_NOT_FOUND;
    }
  }

  //
  // Process Asynchronous MMI sources
  //
  MmiManage (NULL, NULL, NULL, NULL);

  //
  // TBD: Do not use private data structure ?
  //

  //
  // Clear the InMm flag as we are going to leave MM
  //
  gMmCorePrivate->InMm = FALSE;

  DEBUG ((DEBUG_INFO, "MmEntryPoint Done\n"));
}

EFI_STATUS
EFIAPI
MmConfigurationMmNotify (
  IN CONST EFI_GUID *Protocol,
  IN VOID           *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                      Status;
  EFI_MM_CONFIGURATION_PROTOCOL  *MmConfiguration;

  DEBUG ((DEBUG_INFO, "MmConfigurationMmNotify(%g) - %x\n", Protocol, Interface));

  MmConfiguration = Interface;

  //
  // Register the MM Entry Point provided by the MM Core with the MM COnfiguration protocol
  //
  Status = MmConfiguration->RegisterMmEntry (MmConfiguration, (EFI_MM_ENTRY_POINT)(UINTN)gMmCorePrivate->MmEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Set flag to indicate that the MM Entry Point has been registered which
  // means that MMIs are now fully operational.
  //
  gMmCorePrivate->MmEntryPointRegistered = TRUE;

  //
  // Print debug message showing MM Core entry point address.
  //
  DEBUG ((DEBUG_INFO, "MM Core registered MM Entry Point address %p\n", (VOID *)(UINTN)gMmCorePrivate->MmEntryPoint));
  return EFI_SUCCESS;
}

UINTN
GetHobListSize (
  IN VOID *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *) HobStart;
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

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

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
  EFI_HOB_GUID_TYPE               *GuidHob;
  MM_CORE_DATA_HOB_DATA           *DataInHob;
  EFI_HOB_GUID_TYPE               *MmramRangesHob;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *MmramRangesHobData;
  EFI_MMRAM_DESCRIPTOR            *MmramRanges;
  UINT32                          MmramRangeCount;
  EFI_HOB_FIRMWARE_VOLUME         *BfvHob;

  ProcessLibraryConstructorList (HobStart, &gMmCoreMmst);

  DEBUG ((DEBUG_INFO, "MmMain - 0x%x\n", HobStart));

  //
  // Determine if the caller has passed a reference to a MM_CORE_PRIVATE_DATA
  // structure in the Hoblist. This choice will govern how boot information is
  // extracted later.
  //
  GuidHob = GetNextGuidHob (&gMmCoreDataHobGuid, HobStart);
  if (GuidHob == NULL) {
    //
    // Allocate and zero memory for a MM_CORE_PRIVATE_DATA table and then
    // initialise it
    //
    gMmCorePrivate = (MM_CORE_PRIVATE_DATA *) AllocateRuntimePages(EFI_SIZE_TO_PAGES(sizeof (MM_CORE_PRIVATE_DATA)));
    SetMem ((VOID *)(UINTN)gMmCorePrivate, sizeof (MM_CORE_PRIVATE_DATA), 0);
    gMmCorePrivate->Signature = MM_CORE_PRIVATE_DATA_SIGNATURE;
    gMmCorePrivate->MmEntryPointRegistered = FALSE;
    gMmCorePrivate->InMm = FALSE;
    gMmCorePrivate->ReturnStatus = EFI_SUCCESS;

    //
    // Extract the MMRAM ranges from the MMRAM descriptor HOB
    //
    MmramRangesHob = GetNextGuidHob (&gEfiMmPeiMmramMemoryReserveGuid, HobStart);
    if (MmramRangesHob == NULL)
      return EFI_UNSUPPORTED;

    MmramRangesHobData = GET_GUID_HOB_DATA (MmramRangesHob);
    ASSERT (MmramRangesHobData != NULL);
    MmramRanges = MmramRangesHobData->Descriptor;
    MmramRangeCount = MmramRangesHobData->NumberOfMmReservedRegions;
    ASSERT (MmramRanges);
    ASSERT (MmramRangeCount);

    //
    // Copy the MMRAM ranges into MM_CORE_PRIVATE_DATA table just in case any
    // code relies on them being present there
    //
    gMmCorePrivate->MmramRangeCount = MmramRangeCount;
    gMmCorePrivate->MmramRanges =
      (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (MmramRangeCount * sizeof (EFI_MMRAM_DESCRIPTOR));
    ASSERT (gMmCorePrivate->MmramRanges != 0);
    CopyMem (
      (VOID *)(UINTN)gMmCorePrivate->MmramRanges,
      MmramRanges,
      MmramRangeCount * sizeof (EFI_MMRAM_DESCRIPTOR)
      );
  } else {
    DataInHob       = GET_GUID_HOB_DATA (GuidHob);
    gMmCorePrivate = (MM_CORE_PRIVATE_DATA *)(UINTN)DataInHob->Address;
    MmramRanges     = (EFI_MMRAM_DESCRIPTOR *)(UINTN)gMmCorePrivate->MmramRanges;
    MmramRangeCount = gMmCorePrivate->MmramRangeCount;
  }

  //
  // Print the MMRAM ranges passed by the caller
  //
  DEBUG ((DEBUG_INFO, "MmramRangeCount - 0x%x\n", MmramRangeCount));
  for (Index = 0; Index < MmramRangeCount; Index++) {
          DEBUG ((DEBUG_INFO, "MmramRanges[%d]: 0x%016lx - 0x%lx\n", Index,
                  MmramRanges[Index].CpuStart,
                  MmramRanges[Index].PhysicalSize));
  }

  //
  // Copy the MMRAM ranges into private MMRAM
  //
  mMmramRangeCount = MmramRangeCount;
  DEBUG ((DEBUG_INFO, "mMmramRangeCount - 0x%x\n", mMmramRangeCount));
  mMmramRanges = AllocatePool (mMmramRangeCount * sizeof (EFI_MMRAM_DESCRIPTOR));
  DEBUG ((DEBUG_INFO, "mMmramRanges - 0x%x\n", mMmramRanges));
  ASSERT (mMmramRanges != NULL);
  CopyMem (mMmramRanges, (VOID *)(UINTN)MmramRanges, mMmramRangeCount * sizeof (EFI_MMRAM_DESCRIPTOR));

  //
  // Get Boot Firmware Volume address from the BFV Hob
  //
  BfvHob = GetFirstHob (EFI_HOB_TYPE_FV);
  if (BfvHob != NULL) {
    DEBUG ((DEBUG_INFO, "BFV address - 0x%x\n", BfvHob->BaseAddress));
    DEBUG ((DEBUG_INFO, "BFV size    - 0x%x\n", BfvHob->Length));
    gMmCorePrivate->StandaloneBfvAddress = BfvHob->BaseAddress;
  }

  gMmCorePrivate->Mmst          = (EFI_PHYSICAL_ADDRESS)(UINTN)&gMmCoreMmst;
  gMmCorePrivate->MmEntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)MmEntryPoint;

  //
  // No need to initialize memory service.
  // It is done in constructor of StandaloneMmCoreMemoryAllocationLib(),
  // so that the library linked with StandaloneMmCore can use AllocatePool() in constuctor.
  //

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
  // Dispatch standalone BFV
  //
  DEBUG ((DEBUG_INFO, "Mm Dispatch StandaloneBfvAddress - 0x%08x\n", gMmCorePrivate->StandaloneBfvAddress));
  if (gMmCorePrivate->StandaloneBfvAddress != 0) {
    MmCoreFfsFindMmDriver ((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)gMmCorePrivate->StandaloneBfvAddress);
    MmDispatcher ();
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

  DEBUG ((DEBUG_INFO, "MmMain Done!\n"));

  return EFI_SUCCESS;
}

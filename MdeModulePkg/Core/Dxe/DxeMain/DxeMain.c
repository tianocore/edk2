/** @file
  DXE Core Main Entry Point

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"

//
// DXE Core Global Variables for Protocols from PEI
//
EFI_HANDLE  mDecompressHandle = NULL;

//
// DXE Core globals for Architecture Protocols
//
EFI_SECURITY_ARCH_PROTOCOL        *gSecurity      = NULL;
EFI_SECURITY2_ARCH_PROTOCOL       *gSecurity2     = NULL;
EFI_CPU_ARCH_PROTOCOL             *gCpu           = NULL;
EFI_METRONOME_ARCH_PROTOCOL       *gMetronome     = NULL;
EFI_TIMER_ARCH_PROTOCOL           *gTimer         = NULL;
EFI_BDS_ARCH_PROTOCOL             *gBds           = NULL;
EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *gWatchdogTimer = NULL;

//
// DXE Core globals for optional protocol dependencies
//
EFI_SMM_BASE2_PROTOCOL  *gSmmBase2 = NULL;

//
// DXE Core Global used to update core loaded image protocol handle
//
EFI_GUID                   *gDxeCoreFileName;
EFI_LOADED_IMAGE_PROTOCOL  *gDxeCoreLoadedImage;

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  This function copies Length bytes from SourceBuffer to DestinationBuffer, and returns
  DestinationBuffer.  The implementation must be reentrant, and it must handle the case
  where SourceBuffer overlaps DestinationBuffer.

  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then ASSERT().
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT().

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return None.

**/
STATIC
VOID
EFIAPI
EfiCopyMem (
  IN VOID   *DestinationBuffer,
  IN VOID   *SourceBuffer,
  IN UINTN  Length
  )
{
  CopyMem (
    DestinationBuffer,
    SourceBuffer,
    Length
    );
}

/**
  Fills a target buffer with a byte value, and returns the target buffer.

  This function fills Length bytes of Buffer with Value.

  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer    The memory to set.
  @param  Length    The number of bytes to set.
  @param  Value     The value with which to fill Length bytes of Buffer.

  @return None.

**/
STATIC
VOID
EFIAPI
EfiSetMem (
  IN VOID   *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  )
{
  SetMem (
    Buffer,
    Length,
    Value
    );
}

//
// DXE Core Module Variables
//
EFI_BOOT_SERVICES  mBootServices = {
  {
    EFI_BOOT_SERVICES_SIGNATURE,                                                          // Signature
    EFI_BOOT_SERVICES_REVISION,                                                           // Revision
    sizeof (EFI_BOOT_SERVICES),                                                           // HeaderSize
    0,                                                                                    // CRC32
    0                                                                                     // Reserved
  },
  (EFI_RAISE_TPL)CoreRaiseTpl,                                                            // RaiseTPL
  (EFI_RESTORE_TPL)CoreRestoreTpl,                                                        // RestoreTPL
  (EFI_ALLOCATE_PAGES)CoreAllocatePages,                                                  // AllocatePages
  (EFI_FREE_PAGES)CoreFreePages,                                                          // FreePages
  (EFI_GET_MEMORY_MAP)CoreGetMemoryMap,                                                   // GetMemoryMap
  (EFI_ALLOCATE_POOL)CoreAllocatePool,                                                    // AllocatePool
  (EFI_FREE_POOL)CoreFreePool,                                                            // FreePool
  (EFI_CREATE_EVENT)CoreCreateEvent,                                                      // CreateEvent
  (EFI_SET_TIMER)CoreSetTimer,                                                            // SetTimer
  (EFI_WAIT_FOR_EVENT)CoreWaitForEvent,                                                   // WaitForEvent
  (EFI_SIGNAL_EVENT)CoreSignalEvent,                                                      // SignalEvent
  (EFI_CLOSE_EVENT)CoreCloseEvent,                                                        // CloseEvent
  (EFI_CHECK_EVENT)CoreCheckEvent,                                                        // CheckEvent
  (EFI_INSTALL_PROTOCOL_INTERFACE)CoreInstallProtocolInterface,                           // InstallProtocolInterface
  (EFI_REINSTALL_PROTOCOL_INTERFACE)CoreReinstallProtocolInterface,                       // ReinstallProtocolInterface
  (EFI_UNINSTALL_PROTOCOL_INTERFACE)CoreUninstallProtocolInterface,                       // UninstallProtocolInterface
  (EFI_HANDLE_PROTOCOL)CoreHandleProtocol,                                                // HandleProtocol
  (VOID *)NULL,                                                                           // Reserved
  (EFI_REGISTER_PROTOCOL_NOTIFY)CoreRegisterProtocolNotify,                               // RegisterProtocolNotify
  (EFI_LOCATE_HANDLE)CoreLocateHandle,                                                    // LocateHandle
  (EFI_LOCATE_DEVICE_PATH)CoreLocateDevicePath,                                           // LocateDevicePath
  (EFI_INSTALL_CONFIGURATION_TABLE)CoreInstallConfigurationTable,                         // InstallConfigurationTable
  (EFI_IMAGE_LOAD)CoreLoadImage,                                                          // LoadImage
  (EFI_IMAGE_START)CoreStartImage,                                                        // StartImage
  (EFI_EXIT)CoreExit,                                                                     // Exit
  (EFI_IMAGE_UNLOAD)CoreUnloadImage,                                                      // UnloadImage
  (EFI_EXIT_BOOT_SERVICES)CoreExitBootServices,                                           // ExitBootServices
  (EFI_GET_NEXT_MONOTONIC_COUNT)CoreEfiNotAvailableYetGetNextMonotonicCount,              // GetNextMonotonicCount
  (EFI_STALL)CoreStall,                                                                   // Stall
  (EFI_SET_WATCHDOG_TIMER)CoreSetWatchdogTimer,                                           // SetWatchdogTimer
  (EFI_CONNECT_CONTROLLER)CoreConnectController,                                          // ConnectController
  (EFI_DISCONNECT_CONTROLLER)CoreDisconnectController,                                    // DisconnectController
  (EFI_OPEN_PROTOCOL)CoreOpenProtocol,                                                    // OpenProtocol
  (EFI_CLOSE_PROTOCOL)CoreCloseProtocol,                                                  // CloseProtocol
  (EFI_OPEN_PROTOCOL_INFORMATION)CoreOpenProtocolInformation,                             // OpenProtocolInformation
  (EFI_PROTOCOLS_PER_HANDLE)CoreProtocolsPerHandle,                                       // ProtocolsPerHandle
  (EFI_LOCATE_HANDLE_BUFFER)CoreLocateHandleBuffer,                                       // LocateHandleBuffer
  (EFI_LOCATE_PROTOCOL)CoreLocateProtocol,                                                // LocateProtocol
  (EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)CoreInstallMultipleProtocolInterfaces,        // InstallMultipleProtocolInterfaces
  (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)CoreUninstallMultipleProtocolInterfaces,    // UninstallMultipleProtocolInterfaces
  (EFI_CALCULATE_CRC32)CoreEfiNotAvailableYetCalculateCrc32,                              // CalculateCrc32
  (EFI_COPY_MEM)EfiCopyMem,                                                               // CopyMem
  (EFI_SET_MEM)EfiSetMem,                                                                 // SetMem
  (EFI_CREATE_EVENT_EX)CoreCreateEventEx                                                  // CreateEventEx
};

EFI_DXE_SERVICES  mDxeServices = {
  {
    DXE_SERVICES_SIGNATURE,                                           // Signature
    DXE_SERVICES_REVISION,                                            // Revision
    sizeof (DXE_SERVICES),                                            // HeaderSize
    0,                                                                // CRC32
    0                                                                 // Reserved
  },
  (EFI_ADD_MEMORY_SPACE)CoreAddMemorySpace,                               // AddMemorySpace
  (EFI_ALLOCATE_MEMORY_SPACE)CoreAllocateMemorySpace,                     // AllocateMemorySpace
  (EFI_FREE_MEMORY_SPACE)CoreFreeMemorySpace,                             // FreeMemorySpace
  (EFI_REMOVE_MEMORY_SPACE)CoreRemoveMemorySpace,                         // RemoveMemorySpace
  (EFI_GET_MEMORY_SPACE_DESCRIPTOR)CoreGetMemorySpaceDescriptor,          // GetMemorySpaceDescriptor
  (EFI_SET_MEMORY_SPACE_ATTRIBUTES)CoreSetMemorySpaceAttributes,          // SetMemorySpaceAttributes
  (EFI_GET_MEMORY_SPACE_MAP)CoreGetMemorySpaceMap,                        // GetMemorySpaceMap
  (EFI_ADD_IO_SPACE)CoreAddIoSpace,                                       // AddIoSpace
  (EFI_ALLOCATE_IO_SPACE)CoreAllocateIoSpace,                             // AllocateIoSpace
  (EFI_FREE_IO_SPACE)CoreFreeIoSpace,                                     // FreeIoSpace
  (EFI_REMOVE_IO_SPACE)CoreRemoveIoSpace,                                 // RemoveIoSpace
  (EFI_GET_IO_SPACE_DESCRIPTOR)CoreGetIoSpaceDescriptor,                  // GetIoSpaceDescriptor
  (EFI_GET_IO_SPACE_MAP)CoreGetIoSpaceMap,                                // GetIoSpaceMap
  (EFI_DISPATCH)CoreDispatcher,                                           // Dispatch
  (EFI_SCHEDULE)CoreSchedule,                                             // Schedule
  (EFI_TRUST)CoreTrust,                                                   // Trust
  (EFI_PROCESS_FIRMWARE_VOLUME)CoreProcessFirmwareVolume,                 // ProcessFirmwareVolume
  (EFI_SET_MEMORY_SPACE_CAPABILITIES)CoreSetMemorySpaceCapabilities,      // SetMemorySpaceCapabilities
};

EFI_SYSTEM_TABLE  mEfiSystemTableTemplate = {
  {
    EFI_SYSTEM_TABLE_SIGNATURE,                                           // Signature
    EFI_SYSTEM_TABLE_REVISION,                                            // Revision
    sizeof (EFI_SYSTEM_TABLE),                                            // HeaderSize
    0,                                                                    // CRC32
    0                                                                     // Reserved
  },
  NULL,                                                                   // FirmwareVendor
  0,                                                                      // FirmwareRevision
  NULL,                                                                   // ConsoleInHandle
  NULL,                                                                   // ConIn
  NULL,                                                                   // ConsoleOutHandle
  NULL,                                                                   // ConOut
  NULL,                                                                   // StandardErrorHandle
  NULL,                                                                   // StdErr
  NULL,                                                                   // RuntimeServices
  &mBootServices,                                                         // BootServices
  0,                                                                      // NumberOfConfigurationTableEntries
  NULL                                                                    // ConfigurationTable
};

EFI_RUNTIME_SERVICES  mEfiRuntimeServicesTableTemplate = {
  {
    EFI_RUNTIME_SERVICES_SIGNATURE,                               // Signature
    EFI_RUNTIME_SERVICES_REVISION,                                // Revision
    sizeof (EFI_RUNTIME_SERVICES),                                // HeaderSize
    0,                                                            // CRC32
    0                                                             // Reserved
  },
  (EFI_GET_TIME)CoreEfiNotAvailableYetGetTime,                                    // GetTime
  (EFI_SET_TIME)CoreEfiNotAvailableYetSetTime,                                    // SetTime
  (EFI_GET_WAKEUP_TIME)CoreEfiNotAvailableYetGetWakeupTime,                       // GetWakeupTime
  (EFI_SET_WAKEUP_TIME)CoreEfiNotAvailableYetSetWakeupTime,                       // SetWakeupTime
  (EFI_SET_VIRTUAL_ADDRESS_MAP)CoreEfiNotAvailableYetSetVirtualAddressMap,        // SetVirtualAddressMap
  (EFI_CONVERT_POINTER)CoreEfiNotAvailableYetConvertPointer,                      // ConvertPointer
  (EFI_GET_VARIABLE)CoreEfiNotAvailableYetGetVariable,                            // GetVariable
  (EFI_GET_NEXT_VARIABLE_NAME)CoreEfiNotAvailableYetGetNextVariableName,          // GetNextVariableName
  (EFI_SET_VARIABLE)CoreEfiNotAvailableYetSetVariable,                            // SetVariable
  (EFI_GET_NEXT_HIGH_MONO_COUNT)CoreEfiNotAvailableYetGetNextHighMonotonicCount,  // GetNextHighMonotonicCount
  (EFI_RESET_SYSTEM)CoreEfiNotAvailableYetResetSystem,                            // ResetSystem
  (EFI_UPDATE_CAPSULE)CoreEfiNotAvailableYetUpdateCapsule,                        // UpdateCapsule
  (EFI_QUERY_CAPSULE_CAPABILITIES)CoreEfiNotAvailableYetQueryCapsuleCapabilities, // QueryCapsuleCapabilities
  (EFI_QUERY_VARIABLE_INFO)CoreEfiNotAvailableYetQueryVariableInfo                // QueryVariableInfo
};

EFI_RUNTIME_ARCH_PROTOCOL  gRuntimeTemplate = {
  INITIALIZE_LIST_HEAD_VARIABLE (gRuntimeTemplate.ImageHead),
  INITIALIZE_LIST_HEAD_VARIABLE (gRuntimeTemplate.EventHead),

  //
  // Make sure Size != sizeof (EFI_MEMORY_DESCRIPTOR). This will
  // prevent people from having pointer math bugs in their code.
  // now you have to use *DescriptorSize to make things work.
  //
  sizeof (EFI_MEMORY_DESCRIPTOR) + sizeof (UINT64) - (sizeof (EFI_MEMORY_DESCRIPTOR) % sizeof (UINT64)),
  EFI_MEMORY_DESCRIPTOR_VERSION,
  0,
  NULL,
  NULL,
  FALSE,
  FALSE
};

EFI_RUNTIME_ARCH_PROTOCOL  *gRuntime = &gRuntimeTemplate;

//
// DXE Core Global Variables for the EFI System Table, Boot Services Table,
// DXE Services Table, and Runtime Services Table
//
EFI_DXE_SERVICES  *gDxeCoreDS = &mDxeServices;
EFI_SYSTEM_TABLE  *gDxeCoreST = NULL;

//
// For debug initialize gDxeCoreRT to template. gDxeCoreRT must be allocated from RT memory
//  but gDxeCoreRT is used for ASSERT () and DEBUG () type macros so lets give it
//  a value that will not cause debug infrastructure to crash early on.
//
EFI_RUNTIME_SERVICES  *gDxeCoreRT         = &mEfiRuntimeServicesTableTemplate;
EFI_HANDLE            gDxeCoreImageHandle = NULL;

BOOLEAN  gMemoryMapTerminated = FALSE;

static BOOLEAN  mExitBootServicesCalled = FALSE;

//
// EFI Decompress Protocol
//
EFI_DECOMPRESS_PROTOCOL  gEfiDecompress = {
  DxeMainUefiDecompressGetInfo,
  DxeMainUefiDecompress
};

//
// For Loading modules at fixed address feature, the configuration table is to cache the top address below which to load
// Runtime code&boot time code
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_LOAD_FIXED_ADDRESS_CONFIGURATION_TABLE  gLoadModuleAtFixAddressConfigurationTable = { 0, 0 };

// Main entry point to the DXE Core
//

/**
  Main entry point to DXE Core.

  @param  HobStart               Pointer to the beginning of the HOB List from PEI.

  @return This function should never return.

**/
VOID
EFIAPI
DxeMain (
  IN  VOID  *HobStart
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          MemoryBaseAddress;
  UINT64                        MemoryLength;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINTN                         Index;
  EFI_HOB_GUID_TYPE             *GuidHob;
  EFI_VECTOR_HANDOFF_INFO       *VectorInfoList;
  EFI_VECTOR_HANDOFF_INFO       *VectorInfo;
  VOID                          *EntryPoint;

  //
  // Setup the default exception handlers
  //
  VectorInfoList = NULL;
  GuidHob        = GetNextGuidHob (&gEfiVectorHandoffInfoPpiGuid, HobStart);
  if (GuidHob != NULL) {
    VectorInfoList = (EFI_VECTOR_HANDOFF_INFO *)(GET_GUID_HOB_DATA (GuidHob));
  }

  Status = InitializeCpuExceptionHandlers (VectorInfoList);
  ASSERT_EFI_ERROR (Status);

  //
  // Setup Exception Stack
  //
  Status = InitializeSeparateExceptionStacks (NULL, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize Debug Agent to support source level debug in DXE phase
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_CORE, HobStart, NULL);

  //
  // Initialize Memory Services
  //
  CoreInitializeMemoryServices (&HobStart, &MemoryBaseAddress, &MemoryLength);

  MemoryProfileInit (HobStart);

  //
  // Start the Handle Services.
  //
  Status = CoreInitializeHandleServices ();
  ASSERT_EFI_ERROR (Status);

  //
  // Start the Image Services.
  //
  Status = CoreInitializeImageServices (HobStart);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize the Global Coherency Domain Services
  //
  Status = CoreInitializeGcdServices (&HobStart, MemoryBaseAddress, MemoryLength);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate the EFI System Table and EFI Runtime Service Table from EfiRuntimeServicesData
  // Use the templates to initialize the contents of the EFI System Table and EFI Runtime Services Table
  //
  gDxeCoreST = AllocateRuntimeCopyPool (sizeof (EFI_SYSTEM_TABLE), &mEfiSystemTableTemplate);
  ASSERT (gDxeCoreST != NULL);

  gDxeCoreRT = AllocateRuntimeCopyPool (sizeof (EFI_RUNTIME_SERVICES), &mEfiRuntimeServicesTableTemplate);
  ASSERT (gDxeCoreRT != NULL);

  gDxeCoreST->RuntimeServices = gDxeCoreRT;

  //
  // Update DXE Core Loaded Image Protocol with allocated UEFI System Table
  //
  gDxeCoreLoadedImage->SystemTable = gDxeCoreST;

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (gDxeCoreImageHandle, gDxeCoreST);
  PERF_CROSSMODULE_END ("PEI");
  PERF_CROSSMODULE_BEGIN ("DXE");

  //
  // Log MemoryBaseAddress and MemoryLength again (from
  // CoreInitializeMemoryServices()), now that library constructors have
  // executed.
  //
  DEBUG ((
    DEBUG_INFO,
    "%a: MemoryBaseAddress=0x%Lx MemoryLength=0x%Lx\n",
    __func__,
    MemoryBaseAddress,
    MemoryLength
    ));

  //
  // Report DXE Core image information to the PE/COFF Extra Action Library
  //
  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.ImageAddress  = (EFI_PHYSICAL_ADDRESS)(UINTN)gDxeCoreLoadedImage->ImageBase;
  ImageContext.PdbPointer    = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageContext.ImageAddress);
  ImageContext.SizeOfHeaders = PeCoffGetSizeOfHeaders ((VOID *)(UINTN)ImageContext.ImageAddress);
  Status                     = PeCoffLoaderGetEntryPoint ((VOID *)(UINTN)ImageContext.ImageAddress, &EntryPoint);
  if (Status == EFI_SUCCESS) {
    ImageContext.EntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)EntryPoint;
  }

  ImageContext.Handle    = (VOID *)(UINTN)gDxeCoreLoadedImage->ImageBase;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Install the DXE Services Table into the EFI System Tables's Configuration Table
  //
  Status = CoreInstallConfigurationTable (&gEfiDxeServicesTableGuid, gDxeCoreDS);
  ASSERT_EFI_ERROR (Status);

  //
  // Install the HOB List into the EFI System Tables's Configuration Table
  //
  Status = CoreInstallConfigurationTable (&gEfiHobListGuid, HobStart);
  ASSERT_EFI_ERROR (Status);

  //
  // Install Memory Type Information Table into the EFI System Tables's Configuration Table
  //
  Status = CoreInstallConfigurationTable (&gEfiMemoryTypeInformationGuid, &gMemoryTypeInformation);
  ASSERT_EFI_ERROR (Status);

  //
  // If Loading modules At fixed address feature is enabled, install Load moduels at fixed address
  // Configuration Table so that user could easily to retrieve the top address to load Dxe and PEI
  // Code and Tseg base to load SMM driver.
  //
  if (PcdGet64 (PcdLoadModuleAtFixAddressEnable) != 0) {
    Status = CoreInstallConfigurationTable (&gLoadFixedAddressConfigurationTableGuid, &gLoadModuleAtFixAddressConfigurationTable);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Report Status Code here for DXE_ENTRY_POINT once it is available
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ENTRY_POINT)
    );

  //
  // Create the aligned system table pointer structure that is used by external
  // debuggers to locate the system table...  Also, install debug image info
  // configuration table.
  //
  CoreInitializeDebugImageInfoTable ();
  CoreNewDebugImageInfoEntry (
    EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL,
    gDxeCoreLoadedImage,
    gDxeCoreImageHandle
    );

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "HOBLIST address in DXE = 0x%p\n", HobStart));

  DEBUG_CODE_BEGIN ();
  EFI_PEI_HOB_POINTERS  Hob;

  for (Hob.Raw = HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      DEBUG ((
        DEBUG_INFO | DEBUG_LOAD,
        "Memory Allocation 0x%08x 0x%0lx - 0x%0lx\n", \
        Hob.MemoryAllocation->AllocDescriptor.MemoryType,                      \
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,               \
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength - 1
        ));
    }
  }

  for (Hob.Raw = HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV) {
      DEBUG ((
        DEBUG_INFO | DEBUG_LOAD,
        "FV Hob            0x%0lx - 0x%0lx\n",
        Hob.FirmwareVolume->BaseAddress,
        Hob.FirmwareVolume->BaseAddress + Hob.FirmwareVolume->Length - 1
        ));
    } else if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV2) {
      DEBUG ((
        DEBUG_INFO | DEBUG_LOAD,
        "FV2 Hob           0x%0lx - 0x%0lx\n",
        Hob.FirmwareVolume2->BaseAddress,
        Hob.FirmwareVolume2->BaseAddress + Hob.FirmwareVolume2->Length - 1
        ));
      DEBUG ((
        DEBUG_INFO | DEBUG_LOAD,
        "                  %g - %g\n",
        &Hob.FirmwareVolume2->FvName,
        &Hob.FirmwareVolume2->FileName
        ));
    } else if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV3) {
      DEBUG ((
        DEBUG_INFO | DEBUG_LOAD,
        "FV3 Hob           0x%0lx - 0x%0lx - 0x%x - 0x%x\n",
        Hob.FirmwareVolume3->BaseAddress,
        Hob.FirmwareVolume3->BaseAddress + Hob.FirmwareVolume3->Length - 1,
        Hob.FirmwareVolume3->AuthenticationStatus,
        Hob.FirmwareVolume3->ExtractedFv
        ));
      if (Hob.FirmwareVolume3->ExtractedFv) {
        DEBUG ((
          DEBUG_INFO | DEBUG_LOAD,
          "                  %g - %g\n",
          &Hob.FirmwareVolume3->FvName,
          &Hob.FirmwareVolume3->FileName
          ));
      }
    }
  }

  DEBUG_CODE_END ();

  //
  // Initialize the Event Services
  //
  Status = CoreInitializeEventServices ();
  ASSERT_EFI_ERROR (Status);

  //
  // Give the debug agent a chance to initialize with events.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_CORE_LATE, HobStart, NULL);

  MemoryProfileInstallProtocol ();

  CoreInitializeMemoryAttributesTable ();
  CoreInitializeMemoryProtection ();

  //
  // Get persisted vector hand-off info from GUIDeed HOB again due to HobStart may be updated,
  // and install configuration table
  //
  GuidHob = GetNextGuidHob (&gEfiVectorHandoffInfoPpiGuid, HobStart);
  if (GuidHob != NULL) {
    VectorInfoList = (EFI_VECTOR_HANDOFF_INFO *)(GET_GUID_HOB_DATA (GuidHob));
    VectorInfo     = VectorInfoList;
    Index          = 1;
    while (VectorInfo->Attribute != EFI_VECTOR_HANDOFF_LAST_ENTRY) {
      VectorInfo++;
      Index++;
    }

    VectorInfo = AllocateCopyPool (sizeof (EFI_VECTOR_HANDOFF_INFO) * Index, (VOID *)VectorInfoList);
    ASSERT (VectorInfo != NULL);
    Status = CoreInstallConfigurationTable (&gEfiVectorHandoffTableGuid, (VOID *)VectorInfo);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Get the Protocols that were passed in from PEI to DXE through GUIDed HOBs
  //
  // These Protocols are not architectural. This implementation is sharing code between
  // PEI and DXE in order to save FLASH space. These Protocols could also be implemented
  // as part of the DXE Core. However, that would also require the DXE Core to be ported
  // each time a different CPU is used, a different Decompression algorithm is used, or a
  // different Image type is used. By placing these Protocols in PEI, the DXE Core remains
  // generic, and only PEI and the Arch Protocols need to be ported from Platform to Platform,
  // and from CPU to CPU.
  //

  //
  // Publish the EFI, Tiano, and Custom Decompress protocols for use by other DXE components
  //
  Status = CoreInstallMultipleProtocolInterfaces (
             &mDecompressHandle,
             &gEfiDecompressProtocolGuid,
             &gEfiDecompress,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for the GUIDs of the Architectural Protocols, so the rest of the
  // EFI Boot Services and EFI Runtime Services tables can be filled in.
  // Also register for the GUIDs of optional protocols.
  //
  CoreNotifyOnProtocolInstallation ();

  //
  // Produce Firmware Volume Protocols, one for each FV in the HOB list.
  //
  Status = FwVolBlockDriverInit (gDxeCoreImageHandle, gDxeCoreST);
  ASSERT_EFI_ERROR (Status);

  Status = FwVolDriverInit (gDxeCoreImageHandle, gDxeCoreST);
  ASSERT_EFI_ERROR (Status);

  //
  // Produce the Section Extraction Protocol
  //
  Status = InitializeSectionExtraction (gDxeCoreImageHandle, gDxeCoreST);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize the DXE Dispatcher
  //
  CoreInitializeDispatcher ();

  //
  // Invoke the DXE Dispatcher
  //
  CoreDispatcher ();

  //
  // Display Architectural protocols that were not loaded if this is DEBUG build
  //
  DEBUG_CODE_BEGIN ();
  CoreDisplayMissingArchProtocols ();
  DEBUG_CODE_END ();

  //
  // Display any drivers that were not dispatched because dependency expression
  // evaluated to false if this is a debug build
  //
  DEBUG_CODE_BEGIN ();
  CoreDisplayDiscoveredNotDispatched ();
  DEBUG_CODE_END ();

  //
  // Assert if the Architectural Protocols are not present.
  //
  Status = CoreAllEfiServicesAvailable ();
  if (EFI_ERROR (Status)) {
    //
    // Report Status code that some Architectural Protocols are not present.
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MAJOR,
      (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_EC_NO_ARCH)
      );
  }

  ASSERT_EFI_ERROR (Status);

  //
  // Report Status code before transfer control to BDS
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT)
    );

  //
  // Transfer control to the BDS Architectural Protocol
  //
  gBds->Entry (gBds);

  //
  // BDS should never return
  //
  ASSERT (FALSE);
  CpuDeadLoop ();

  UNREACHABLE ();
}

/**
  Place holder function until all the Boot Services and Runtime Services are
  available.

  @param  Count                  Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetGetNextMonotonicCount (
  OUT UINT64  *Count
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  Data                   Undefined
  @param  DataSize               Undefined
  @param  Crc32                  Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetCalculateCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *Crc32
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  Time                   Undefined
  @param  Capabilities           Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetGetTime (
  OUT  EFI_TIME               *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities OPTIONAL
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  Time                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetSetTime (
  IN  EFI_TIME  *Time
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  Enabled                Undefined
  @param  Pending                Undefined
  @param  Time                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetGetWakeupTime (
  OUT BOOLEAN   *Enabled,
  OUT BOOLEAN   *Pending,
  OUT EFI_TIME  *Time
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  Enable                 Undefined
  @param  Time                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetSetWakeupTime (
  IN  BOOLEAN   Enable,
  IN  EFI_TIME  *Time   OPTIONAL
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  MemoryMapSize          Undefined
  @param  DescriptorSize         Undefined
  @param  DescriptorVersion      Undefined
  @param  VirtualMap             Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetSetVirtualAddressMap (
  IN  UINTN                  MemoryMapSize,
  IN  UINTN                  DescriptorSize,
  IN  UINT32                 DescriptorVersion,
  IN  EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  DebugDisposition       Undefined
  @param  Address                Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetConvertPointer (
  IN     UINTN  DebugDisposition,
  IN OUT VOID   **Address
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  VariableName           Undefined
  @param  VendorGuid             Undefined
  @param  Attributes             Undefined
  @param  DataSize               Undefined
  @param  Data                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes     OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data           OPTIONAL
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  VariableNameSize       Undefined
  @param  VariableName           Undefined
  @param  VendorGuid             Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  VariableName           Undefined
  @param  VendorGuid             Undefined
  @param  Attributes             Undefined
  @param  DataSize               Undefined
  @param  Data                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  HighCount              Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetGetNextHighMonotonicCount (
  OUT UINT32  *HighCount
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  ResetType              Undefined
  @param  ResetStatus            Undefined
  @param  DataSize               Undefined
  @param  ResetData              Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
VOID
EFIAPI
CoreEfiNotAvailableYetResetSystem (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  CapsuleHeaderArray     Undefined
  @param  CapsuleCount           Undefined
  @param  ScatterGatherList      Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetUpdateCapsule (
  IN EFI_CAPSULE_HEADER    **CapsuleHeaderArray,
  IN UINTN                 CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS  ScatterGatherList   OPTIONAL
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  CapsuleHeaderArray     Undefined
  @param  CapsuleCount           Undefined
  @param  MaximumCapsuleSize     Undefined
  @param  ResetType              Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetQueryCapsuleCapabilities (
  IN  EFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN  UINTN               CapsuleCount,
  OUT UINT64              *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE      *ResetType
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Place holder function until all the Boot Services and Runtime Services are available.

  @param  Attributes                        Undefined
  @param  MaximumVariableStorageSize        Undefined
  @param  RemainingVariableStorageSize      Undefined
  @param  MaximumVariableSize               Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetQueryVariableInfo (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

/**
  Calcualte the 32-bit CRC in a EFI table using the service provided by the
  gRuntime service.

  @param  Hdr                    Pointer to an EFI standard header

**/
VOID
CalculateEfiHdrCrc (
  IN  OUT EFI_TABLE_HEADER  *Hdr
  )
{
  UINT32  Crc;

  Hdr->CRC32 = 0;

  //
  // If gBS->CalculateCrce32 () == CoreEfiNotAvailableYet () then
  //  Crc will come back as zero if we set it to zero here
  //
  Crc = 0;
  gBS->CalculateCrc32 ((UINT8 *)Hdr, Hdr->HeaderSize, &Crc);
  Hdr->CRC32 = Crc;
}

/**
  Terminates all boot services.

  @param  ImageHandle            Handle that identifies the exiting image.
  @param  MapKey                 Key to the latest memory map.

  @retval EFI_SUCCESS            Boot Services terminated
  @retval EFI_INVALID_PARAMETER  MapKey is incorrect.

**/
EFI_STATUS
EFIAPI
CoreExitBootServices (
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       MapKey
  )
{
  EFI_STATUS  Status;

  //
  // Notify other drivers of their last chance to use boot services
  // before the memory map is terminated.
  //
  if (!mExitBootServicesCalled) {
    CoreNotifySignalList (&gEfiEventBeforeExitBootServicesGuid);
    mExitBootServicesCalled = TRUE;
  }

  //
  // Disable Timer
  //
  gTimer->SetTimerPeriod (gTimer, 0);

  //
  // Terminate memory services if the MapKey matches
  //
  Status = CoreTerminateMemoryMap (MapKey);
  if (EFI_ERROR (Status)) {
    //
    // Notify other drivers that ExitBootServices fail
    //
    CoreNotifySignalList (&gEventExitBootServicesFailedGuid);
    return Status;
  }

  gMemoryMapTerminated = TRUE;

  //
  // Notify other drivers that we are exiting boot services.
  //
  CoreNotifySignalList (&gEfiEventExitBootServicesGuid);

  //
  // Report that ExitBootServices() has been called
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES)
    );

  MemoryProtectionExitBootServicesCallback ();

  //
  // Disable interrupt of Debug timer.
  //
  SaveAndSetDebugTimerInterrupt (FALSE);

  //
  // Disable CPU Interrupts
  //
  gCpu->DisableInterrupt (gCpu);

  //
  // Clear the non-runtime values of the EFI System Table
  //
  gDxeCoreST->BootServices        = NULL;
  gDxeCoreST->ConIn               = NULL;
  gDxeCoreST->ConsoleInHandle     = NULL;
  gDxeCoreST->ConOut              = NULL;
  gDxeCoreST->ConsoleOutHandle    = NULL;
  gDxeCoreST->StdErr              = NULL;
  gDxeCoreST->StandardErrorHandle = NULL;

  //
  // Recompute the 32-bit CRC of the EFI System Table
  //
  CalculateEfiHdrCrc (&gDxeCoreST->Hdr);

  //
  // Zero out the Boot Service Table
  //
  ZeroMem (gBS, sizeof (EFI_BOOT_SERVICES));
  gBS = NULL;

  //
  // Update the AtRuntime field in Runtiem AP.
  //
  gRuntime->AtRuntime = TRUE;

  return Status;
}

/**
  Given a compressed source buffer, this function retrieves the size of the
  uncompressed buffer and the size of the scratch buffer required to decompress
  the compressed source buffer.

  The GetInfo() function retrieves the size of the uncompressed buffer and the
  temporary scratch buffer required to decompress the buffer specified by Source
  and SourceSize. If the size of the uncompressed buffer or the size of the
  scratch buffer cannot be determined from the compressed data specified by
  Source and SourceData, then EFI_INVALID_PARAMETER is returned. Otherwise, the
  size of the uncompressed buffer is returned in DestinationSize, the size of
  the scratch buffer is returned in ScratchSize, and EFI_SUCCESS is returned.
  The GetInfo() function does not have scratch buffer available to perform a
  thorough checking of the validity of the source data. It just retrieves the
  "Original Size" field from the beginning bytes of the source data and output
  it as DestinationSize. And ScratchSize is specific to the decompression
  implementation.

  @param  This               A pointer to the EFI_DECOMPRESS_PROTOCOL instance.
  @param  Source             The source buffer containing the compressed data.
  @param  SourceSize         The size, in bytes, of the source buffer.
  @param  DestinationSize    A pointer to the size, in bytes, of the
                             uncompressed buffer that will be generated when the
                             compressed buffer specified by Source and
                             SourceSize is decompressed.
  @param  ScratchSize        A pointer to the size, in bytes, of the scratch
                             buffer that is required to decompress the
                             compressed buffer specified by Source and
                             SourceSize.

  @retval EFI_SUCCESS        The size of the uncompressed data was returned in
                             DestinationSize and the size of the scratch buffer
                             was returned in ScratchSize.
  @retval EFI_INVALID_PARAMETER The size of the uncompressed data or the size of
                                the scratch buffer cannot be determined from the
                                compressed data specified by Source and
                                SourceSize.

**/
EFI_STATUS
EFIAPI
DxeMainUefiDecompressGetInfo (
  IN EFI_DECOMPRESS_PROTOCOL  *This,
  IN   VOID                   *Source,
  IN   UINT32                 SourceSize,
  OUT  UINT32                 *DestinationSize,
  OUT  UINT32                 *ScratchSize
  )
{
  if ((Source == NULL) || (DestinationSize == NULL) || (ScratchSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return UefiDecompressGetInfo (Source, SourceSize, DestinationSize, ScratchSize);
}

/**
  Decompresses a compressed source buffer.

  The Decompress() function extracts decompressed data to its original form.
  This protocol is designed so that the decompression algorithm can be
  implemented without using any memory services. As a result, the Decompress()
  Function is not allowed to call AllocatePool() or AllocatePages() in its
  implementation. It is the caller's responsibility to allocate and free the
  Destination and Scratch buffers.
  If the compressed source data specified by Source and SourceSize is
  successfully decompressed into Destination, then EFI_SUCCESS is returned. If
  the compressed source data specified by Source and SourceSize is not in a
  valid compressed data format, then EFI_INVALID_PARAMETER is returned.

  @param  This                A pointer to the EFI_DECOMPRESS_PROTOCOL instance.
  @param  Source              The source buffer containing the compressed data.
  @param  SourceSize          SourceSizeThe size of source data.
  @param  Destination         On output, the destination buffer that contains
                              the uncompressed data.
  @param  DestinationSize     The size of the destination buffer.  The size of
                              the destination buffer needed is obtained from
                              EFI_DECOMPRESS_PROTOCOL.GetInfo().
  @param  Scratch             A temporary scratch buffer that is used to perform
                              the decompression.
  @param  ScratchSize         The size of scratch buffer. The size of the
                              scratch buffer needed is obtained from GetInfo().

  @retval EFI_SUCCESS         Decompression completed successfully, and the
                              uncompressed buffer is returned in Destination.
  @retval EFI_INVALID_PARAMETER  The source buffer specified by Source and
                                 SourceSize is corrupted (not in a valid
                                 compressed format).

**/
EFI_STATUS
EFIAPI
DxeMainUefiDecompress (
  IN     EFI_DECOMPRESS_PROTOCOL  *This,
  IN     VOID                     *Source,
  IN     UINT32                   SourceSize,
  IN OUT VOID                     *Destination,
  IN     UINT32                   DestinationSize,
  IN OUT VOID                     *Scratch,
  IN     UINT32                   ScratchSize
  )
{
  EFI_STATUS  Status;
  UINT32      TestDestinationSize;
  UINT32      TestScratchSize;

  if ((Source == NULL) || (Destination == NULL) || (Scratch == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = UefiDecompressGetInfo (Source, SourceSize, &TestDestinationSize, &TestScratchSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((ScratchSize < TestScratchSize) || (DestinationSize < TestDestinationSize)) {
    return RETURN_INVALID_PARAMETER;
  }

  return UefiDecompress (Source, Destination, Scratch);
}

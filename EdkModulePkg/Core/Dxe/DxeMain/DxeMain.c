/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DxeMain.c

Abstract:

  DXE Core Main Entry Point

--*/

#include <DxeMain.h>

//
// DXE Core Global Variables for Protocols from PEI
//
EFI_HANDLE                                mDecompressHandle = NULL;
EFI_PEI_PE_COFF_LOADER_PROTOCOL           *gEfiPeiPeCoffLoader          = NULL;

//
// DXE Core globals for Architecture Protocols
//
EFI_SECURITY_ARCH_PROTOCOL        *gSecurity      = NULL;
EFI_CPU_ARCH_PROTOCOL             *gCpu           = NULL;
EFI_METRONOME_ARCH_PROTOCOL       *gMetronome     = NULL;
EFI_TIMER_ARCH_PROTOCOL           *gTimer         = NULL;
EFI_BDS_ARCH_PROTOCOL             *gBds           = NULL;
EFI_WATCHDOG_TIMER_ARCH_PROTOCOL  *gWatchdogTimer = NULL;


//
// BugBug: I'n not runtime, but is the PPI?
//
EFI_STATUS_CODE_PROTOCOL     gStatusCodeInstance = {
  NULL
};

EFI_STATUS_CODE_PROTOCOL     *gStatusCode    = &gStatusCodeInstance;


//
// DXE Core Global used to update core loaded image protocol handle
//
EFI_GUID                           *gDxeCoreFileName;
EFI_LOADED_IMAGE_PROTOCOL          *gDxeCoreLoadedImage;



//
// DXE Core Module Variables
//

EFI_BOOT_SERVICES mBootServices = {
  {
    EFI_BOOT_SERVICES_SIGNATURE,                                                          // Signature
    EFI_BOOT_SERVICES_REVISION,                                                           // Revision
    sizeof (EFI_BOOT_SERVICES),                                                           // HeaderSize
    0,                                                                                    // CRC32
    0                                                                                     // Reserved
  },
  (EFI_RAISE_TPL)                               CoreRaiseTpl,                             // RaiseTPL
  (EFI_RESTORE_TPL)                             CoreRestoreTpl,                           // RestoreTPL
  (EFI_ALLOCATE_PAGES)                          CoreAllocatePages,                        // AllocatePages
  (EFI_FREE_PAGES)                              CoreFreePages,                            // FreePages
  (EFI_GET_MEMORY_MAP)                          CoreGetMemoryMap,                         // GetMemoryMap
  (EFI_ALLOCATE_POOL)                           CoreAllocatePool,                         // AllocatePool
  (EFI_FREE_POOL)                               CoreFreePool,                             // FreePool
  (EFI_CREATE_EVENT)                            CoreCreateEvent,                          // CreateEvent
  (EFI_SET_TIMER)                               CoreSetTimer,                             // SetTimer
  (EFI_WAIT_FOR_EVENT)                          CoreWaitForEvent,                         // WaitForEvent
  (EFI_SIGNAL_EVENT)                            CoreSignalEvent,                          // SignalEvent
  (EFI_CLOSE_EVENT)                             CoreCloseEvent,                           // CloseEvent
  (EFI_CHECK_EVENT)                             CoreCheckEvent,                           // CheckEvent
  (EFI_INSTALL_PROTOCOL_INTERFACE)              CoreInstallProtocolInterface,             // InstallProtocolInterface
  (EFI_REINSTALL_PROTOCOL_INTERFACE)            CoreReinstallProtocolInterface,           // ReinstallProtocolInterface
  (EFI_UNINSTALL_PROTOCOL_INTERFACE)            CoreUninstallProtocolInterface,           // UninstallProtocolInterface
  (EFI_HANDLE_PROTOCOL)                         CoreHandleProtocol,                       // HandleProtocol
  (VOID *)                                      NULL,                                     // Reserved
  (EFI_REGISTER_PROTOCOL_NOTIFY)                CoreRegisterProtocolNotify,               // RegisterProtocolNotify
  (EFI_LOCATE_HANDLE)                           CoreLocateHandle,                         // LocateHandle
  (EFI_LOCATE_DEVICE_PATH)                      CoreLocateDevicePath,                     // LocateDevicePath
  (EFI_INSTALL_CONFIGURATION_TABLE)             CoreInstallConfigurationTable,            // InstallConfigurationTable
  (EFI_IMAGE_LOAD)                              CoreLoadImage,                            // LoadImage
  (EFI_IMAGE_START)                             CoreStartImage,                           // StartImage
  (EFI_EXIT)                                    CoreExit,                                 // Exit
  (EFI_IMAGE_UNLOAD)                            CoreUnloadImage,                          // UnloadImage
  (EFI_EXIT_BOOT_SERVICES)                      CoreExitBootServices,                     // ExitBootServices
  (EFI_GET_NEXT_MONOTONIC_COUNT)                CoreEfiNotAvailableYetArg1,               // GetNextMonotonicCount
  (EFI_STALL)                                   CoreStall,                                // Stall
  (EFI_SET_WATCHDOG_TIMER)                      CoreSetWatchdogTimer,                     // SetWatchdogTimer
  (EFI_CONNECT_CONTROLLER)                      CoreConnectController,                    // ConnectController
  (EFI_DISCONNECT_CONTROLLER)                   CoreDisconnectController,                 // DisconnectController
  (EFI_OPEN_PROTOCOL)                           CoreOpenProtocol,                         // OpenProtocol
  (EFI_CLOSE_PROTOCOL)                          CoreCloseProtocol,                        // CloseProtocol
  (EFI_OPEN_PROTOCOL_INFORMATION)               CoreOpenProtocolInformation,              // OpenProtocolInformation
  (EFI_PROTOCOLS_PER_HANDLE)                    CoreProtocolsPerHandle,                   // ProtocolsPerHandle
  (EFI_LOCATE_HANDLE_BUFFER)                    CoreLocateHandleBuffer,                   // LocateHandleBuffer
  (EFI_LOCATE_PROTOCOL)                         CoreLocateProtocol,                       // LocateProtocol
  (EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)    CoreInstallMultipleProtocolInterfaces,    // InstallMultipleProtocolInterfaces
  (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)  CoreUninstallMultipleProtocolInterfaces,  // UninstallMultipleProtocolInterfaces
  (EFI_CALCULATE_CRC32)                         CoreEfiNotAvailableYetArg3,               // CalculateCrc32
  (EFI_COPY_MEM)                                CopyMem,                                  // CopyMem
  (EFI_SET_MEM)                                 SetMem,                                   // SetMem
  (EFI_CREATE_EVENT_EX)                         CoreCreateEventEx                         // CreateEventEx
};

EFI_DXE_SERVICES mDxeServices = {
  {
    EFI_DXE_SERVICES_SIGNATURE,                                           // Signature
    EFI_DXE_SERVICES_REVISION,                                            // Revision
    sizeof (EFI_DXE_SERVICES),                                            // HeaderSize
    0,                                                                    // CRC32
    0                                                                     // Reserved
  },
  (EFI_ADD_MEMORY_SPACE)             CoreAddMemorySpace,                  // AddMemorySpace
  (EFI_ALLOCATE_MEMORY_SPACE)        CoreAllocateMemorySpace,             // AllocateMemorySpace
  (EFI_FREE_MEMORY_SPACE)            CoreFreeMemorySpace,                 // FreeMemorySpace
  (EFI_REMOVE_MEMORY_SPACE)          CoreRemoveMemorySpace,               // RemoveMemorySpace
  (EFI_GET_MEMORY_SPACE_DESCRIPTOR)  CoreGetMemorySpaceDescriptor,        // GetMemorySpaceDescriptor
  (EFI_SET_MEMORY_SPACE_ATTRIBUTES)  CoreSetMemorySpaceAttributes,        // SetMemorySpaceAttributes
  (EFI_GET_MEMORY_SPACE_MAP)         CoreGetMemorySpaceMap,               // GetMemorySpaceMap
  (EFI_ADD_IO_SPACE)                 CoreAddIoSpace,                      // AddIoSpace
  (EFI_ALLOCATE_IO_SPACE)            CoreAllocateIoSpace,                 // AllocateIoSpace
  (EFI_FREE_IO_SPACE)                CoreFreeIoSpace,                     // FreeIoSpace
  (EFI_REMOVE_IO_SPACE)              CoreRemoveIoSpace,                   // RemoveIoSpace
  (EFI_GET_IO_SPACE_DESCRIPTOR)      CoreGetIoSpaceDescriptor,            // GetIoSpaceDescriptor
  (EFI_GET_IO_SPACE_MAP)             CoreGetIoSpaceMap,                   // GetIoSpaceMap
  (EFI_DISPATCH)                     CoreDispatcher,                      // Dispatch
  (EFI_SCHEDULE)                     CoreSchedule,                        // Schedule
  (EFI_TRUST)                        CoreTrust,                           // Trust
  (EFI_PROCESS_FIRMWARE_VOLUME)      CoreProcessFirmwareVolume,           // ProcessFirmwareVolume
};

EFI_SYSTEM_TABLE mEfiSystemTableTemplate = {
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

EFI_RUNTIME_SERVICES mEfiRuntimeServicesTableTemplate = {
  {
    EFI_RUNTIME_SERVICES_SIGNATURE,                               // Signature
    EFI_RUNTIME_SERVICES_REVISION,                                // Revision
    sizeof (EFI_RUNTIME_SERVICES),                                // HeaderSize
    0,                                                            // CRC32
    0                                                             // Reserved
  },
  (EFI_GET_TIME)                    CoreEfiNotAvailableYetArg2,   // GetTime
  (EFI_SET_TIME)                    CoreEfiNotAvailableYetArg1,   // SetTime
  (EFI_GET_WAKEUP_TIME)             CoreEfiNotAvailableYetArg3,   // GetWakeupTime
  (EFI_SET_WAKEUP_TIME)             CoreEfiNotAvailableYetArg2,   // SetWakeupTime
  (EFI_SET_VIRTUAL_ADDRESS_MAP)     CoreEfiNotAvailableYetArg4,   // SetVirtualAddressMap
  (EFI_CONVERT_POINTER)             CoreEfiNotAvailableYetArg2,   // ConvertPointer
  (EFI_GET_VARIABLE)                CoreEfiNotAvailableYetArg5,   // GetVariable
  (EFI_GET_NEXT_VARIABLE_NAME)      CoreEfiNotAvailableYetArg3,   // GetNextVariableName
  (EFI_SET_VARIABLE)                CoreEfiNotAvailableYetArg5,   // SetVariable
  (EFI_GET_NEXT_HIGH_MONO_COUNT)    CoreEfiNotAvailableYetArg1,   // GetNextHighMonotonicCount
  (EFI_RESET_SYSTEM)                CoreEfiNotAvailableYetArg4,   // ResetSystem
  (EFI_UPDATE_CAPSULE)              CoreEfiNotAvailableYetArg3,   // UpdateCapsule
  (EFI_QUERY_CAPSULE_CAPABILITIES)  CoreEfiNotAvailableYetArg4,   // QueryCapsuleCapabilities
  (EFI_QUERY_VARIABLE_INFO)         CoreEfiNotAvailableYetArg4    // QueryVariableInfo
};

EFI_RUNTIME_ARCH_PROTOCOL gRuntimeTemplate = {
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

EFI_RUNTIME_ARCH_PROTOCOL *gRuntime = &gRuntimeTemplate;

//
// DXE Core Global Variables for the EFI System Table, Boot Services Table,
// DXE Services Table, and Runtime Services Table
//
EFI_BOOT_SERVICES     *gDxeCoreBS = &mBootServices;
EFI_DXE_SERVICES      *gDxeCoreDS = &mDxeServices;
EFI_SYSTEM_TABLE      *gDxeCoreST = NULL;

//
// For debug initialize gDxeCoreRT to template. gDxeCoreRT must be allocated from RT memory
//  but gDxeCoreRT is used for ASSERT () and DEBUG () type macros so lets give it
//  a value that will not cause debug infrastructure to crash early on.
//
EFI_RUNTIME_SERVICES  *gDxeCoreRT = &mEfiRuntimeServicesTableTemplate;
EFI_HANDLE            gDxeCoreImageHandle = NULL;

VOID  *mHobStart;

//
// EFI Decompress Protocol
//
EFI_DECOMPRESS_PROTOCOL  gEfiDecompress = {
  DxeMainUefiDecompressGetInfo,
  DxeMainUefiDecompress
};

//
// Tiano Decompress Protocol
//
EFI_TIANO_DECOMPRESS_PROTOCOL  gEfiTianoDecompress = {
  DxeMainTianoDecompressGetInfo,
  DxeMainTianoDecompress
};

//
// Customized Decompress Protocol
//
EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL  gEfiCustomizedDecompress = {
  DxeMainCustomDecompressGetInfo,
  DxeMainCustomDecompress
};

//
// Main entry point to the DXE Core
//
VOID
EFIAPI
DxeMain (
  IN  VOID *HobStart
  )
/*++

Routine Description:

  Main entry point to DXE Core.

Arguments:

  HobStart - Pointer to the beginning of the HOB List from PEI

Returns:

  This function should never return

--*/
{
  EFI_STATUS                         Status;
  EFI_PHYSICAL_ADDRESS               MemoryBaseAddress;
  UINT64                             MemoryLength;

  mHobStart = HobStart;

  //
  // Initialize Memory Services
  //
  CoreInitializeMemoryServices (&HobStart, &MemoryBaseAddress, &MemoryLength);

  //
  // Allocate the EFI System Table and EFI Runtime Service Table from EfiRuntimeServicesData
  // Use the templates to initialize the contents of the EFI System Table and EFI Runtime Services Table
  //
  gDxeCoreST = CoreAllocateRuntimeCopyPool (sizeof (EFI_SYSTEM_TABLE), &mEfiSystemTableTemplate);
  ASSERT (gDxeCoreST != NULL);

  gDxeCoreRT = CoreAllocateRuntimeCopyPool (sizeof (EFI_RUNTIME_SERVICES), &mEfiRuntimeServicesTableTemplate);
  ASSERT (gDxeCoreRT != NULL);

  gDxeCoreST->RuntimeServices = gDxeCoreRT;

  //
  // Start the Image Services.
  //
  Status = CoreInitializeImageServices (HobStart);
  ASSERT_EFI_ERROR (Status);

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (gDxeCoreImageHandle, gDxeCoreST);
  PERF_END   (0,PEI_TOK, NULL, 0) ;
  PERF_START (0,DXE_TOK, NULL, 0) ;

  //
  // Initialize the Global Coherency Domain Services
  //
  Status = CoreInitializeGcdServices (&HobStart, MemoryBaseAddress, MemoryLength);
  ASSERT_EFI_ERROR (Status);

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
  // Initialize the ReportStatusCode with PEI version, if available
  //
  CoreGetPeiProtocol (&gEfiStatusCodeRuntimeProtocolGuid, (VOID **)&gStatusCode->ReportStatusCode);

  //
  // Report Status Code here for DXE_ENTRY_POINT once it is available
  //
  CoreReportProgressCode ((EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ENTRY_POINT));

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

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "HOBLIST address in DXE = 0x%08x\n", HobStart));

  //
  // Initialize the Event Services
  //
  Status = CoreInitializeEventServices ();
  ASSERT_EFI_ERROR (Status);


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
              &gEfiDecompressProtocolGuid,           &gEfiDecompress,
              &gEfiTianoDecompressProtocolGuid,      &gEfiTianoDecompress,
              &gEfiCustomizedDecompressProtocolGuid, &gEfiCustomizedDecompress,
              NULL
              );
  ASSERT_EFI_ERROR (Status);

  gEfiPeiPeCoffLoader = GetPeCoffLoaderProtocol ();
  ASSERT (gEfiPeiPeCoffLoader != NULL);

  //
  // Register for the GUIDs of the Architectural Protocols, so the rest of the
  // EFI Boot Services and EFI Runtime Services tables can be filled in.
  //
  CoreNotifyOnArchProtocolInstallation ();

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
  PERF_START (0,"CoreInitializeDispatcher", "DxeMain", 0) ;
  CoreInitializeDispatcher ();
  PERF_END (0,"CoreInitializeDispatcher", "DxeMain", 0) ;

  //
  // Invoke the DXE Dispatcher
  //
  PERF_START (0, "CoreDispatcher", "DxeMain", 0);
  CoreDispatcher ();
  PERF_END (0, "CoreDispatcher", "DxeMain", 0);

  //
  // Display Architectural protocols that were not loaded if this is DEBUG build
  //
  DEBUG_CODE_BEGIN ();
    CoreDisplayMissingArchProtocols ();
  DEBUG_CODE_END ();

  //
  // Assert if the Architectural Protocols are not present.
  //
  ASSERT_EFI_ERROR (CoreAllEfiServicesAvailable ());

  //
  // Report Status code before transfer control to BDS
  //
  CoreReportProgressCode ((EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT));
  //
  // Display any drivers that were not dispatched because dependency expression
  // evaluated to false if this is a debug build
  //
  DEBUG_CODE_BEGIN ();
    CoreDisplayDiscoveredNotDispatched ();
  DEBUG_CODE_END ();

  //
  // Transfer control to the BDS Architectural Protocol
  //
  gBds->Entry (gBds);

  //
  // BDS should never return
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}


EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg0 (
  VOID
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  None

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg1 (
  UINTN Arg1
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg2 (
  UINTN Arg1,
  UINTN Arg2
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined

  Arg2        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg3 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined

  Arg2        - Undefined

  Arg3        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg4 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined

  Arg2        - Undefined

  Arg3        - Undefined

  Arg4        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg5 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined

  Arg2        - Undefined

  Arg3        - Undefined

  Arg4        - Undefined

  Arg5        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.  The CpuBreakpoint () is commented out for now until the
  // DXE Core and all the Architectural Protocols are complete.
  //

  return EFI_NOT_AVAILABLE_YET;
}


EFI_STATUS
CoreGetPeiProtocol (
  IN EFI_GUID  *ProtocolGuid,
  IN VOID      **Interface
  )
/*++

Routine Description:

  Searches for a Protocol Interface passed from PEI through a HOB

Arguments:

  ProtocolGuid - The Protocol GUID to search for in the HOB List

  Interface    - A pointer to the interface for the Protocol GUID

Returns:

  EFI_SUCCESS   - The Protocol GUID was found and its interface is returned in Interface

  EFI_NOT_FOUND - The Protocol GUID was not found in the HOB List

--*/
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  VOID                *Buffer;

  GuidHob = GetNextGuidHob (ProtocolGuid, mHobStart);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  Buffer = GET_GUID_HOB_DATA (GuidHob);
  ASSERT (Buffer != NULL);

  *Interface = (VOID *)(*(UINTN *)(Buffer));

  return EFI_SUCCESS;
}


VOID
CalculateEfiHdrCrc (
  IN  OUT EFI_TABLE_HEADER    *Hdr
  )
/*++

Routine Description:

  Calcualte the 32-bit CRC in a EFI table using the service provided by the
  gRuntime service.

Arguments:

  Hdr  - Pointer to an EFI standard header

Returns:

  None

--*/
{
  UINT32 Crc;

  Hdr->CRC32 = 0;

  //
  // If gDxeCoreBS->CalculateCrce32 () == CoreEfiNotAvailableYet () then
  //  Crc will come back as zero if we set it to zero here
  //
  Crc = 0;
  gDxeCoreBS->CalculateCrc32 ((UINT8 *)Hdr, Hdr->HeaderSize, &Crc);
  Hdr->CRC32 = Crc;
}



EFI_STATUS
EFIAPI
CoreExitBootServices (
  IN EFI_HANDLE   ImageHandle,
  IN UINTN        MapKey
  )
/*++

Routine Description:

  Terminates all boot services.

Arguments:

  ImageHandle   - Handle that identifies the exiting image.

  MapKey -Key to the latest memory map.

Returns:

  EFI_SUCCESS - Boot Services terminated
  EFI_INVALID_PARAMETER - MapKey is incorrect.

--*/
{
  EFI_STATUS    Status;

  //
  // Terminate memory services if the MapKey matches
  //
  Status = CoreTerminateMemoryMap (MapKey);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Notify other drivers that we are exiting boot services.
  //
  CoreNotifySignalList (&gEfiEventExitBootServicesGuid);

  //
  // Disable Timer
  //
  gTimer->SetTimerPeriod (gTimer, 0);

  //
  // Disable CPU Interrupts
  //
  gCpu->DisableInterrupt (gCpu);

  //
  // Report that ExitBootServices() has been called
  //
  // We are using gEfiDxeServicesTableGuid as the caller ID for Dxe Core
  //
  CoreReportProgressCode ((EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES));

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
  SetMem (gDxeCoreBS, sizeof (EFI_BOOT_SERVICES), 0);
  gDxeCoreBS = NULL;

  //
  // Update the AtRuntime field in Runtiem AP.
  //
  gRuntime->AtRuntime = TRUE;

  return Status;
}

EFI_STATUS
DxeMainUefiDecompressGetInfo (
  IN EFI_DECOMPRESS_PROTOCOL            *This,
  IN   VOID                             *Source,
  IN   UINT32                           SourceSize,
  OUT  UINT32                           *DestinationSize,
  OUT  UINT32                           *ScratchSize
  )
{
  if (Source == NULL 
        || DestinationSize == NULL 
        || ScratchSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  return UefiDecompressGetInfo (Source, SourceSize, DestinationSize, ScratchSize);
}

EFI_STATUS
EFIAPI
DxeMainUefiDecompress (
  IN EFI_DECOMPRESS_PROTOCOL              *This,
  IN     VOID                             *Source,
  IN     UINT32                           SourceSize,
  IN OUT VOID                             *Destination,
  IN     UINT32                           DestinationSize,
  IN OUT VOID                             *Scratch,
  IN     UINT32                           ScratchSize
  )
{
  EFI_STATUS  Status;
  UINT32      TestDestinationSize;
  UINT32      TestScratchSize;
  
  if (Source == NULL 
        || Destination== NULL 
        || Scratch == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status = UefiDecompressGetInfo (Source, SourceSize, &TestDestinationSize, &TestScratchSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ScratchSize < TestScratchSize || DestinationSize < TestDestinationSize) {
    return RETURN_INVALID_PARAMETER;
  }

  return UefiDecompress (Source, Destination, Scratch);
}

EFI_STATUS
DxeMainTianoDecompressGetInfo (
  IN EFI_TIANO_DECOMPRESS_PROTOCOL      *This,
  IN   VOID                             *Source,
  IN   UINT32                           SourceSize,
  OUT  UINT32                           *DestinationSize,
  OUT  UINT32                           *ScratchSize
  )
{
  if (Source == NULL 
        || DestinationSize == NULL 
        || ScratchSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return TianoDecompressGetInfo (Source, SourceSize, DestinationSize, ScratchSize);
}

EFI_STATUS
EFIAPI
DxeMainTianoDecompress (
  IN EFI_TIANO_DECOMPRESS_PROTOCOL        *This,
  IN     VOID                             *Source,
  IN     UINT32                           SourceSize,
  IN OUT VOID                             *Destination,
  IN     UINT32                           DestinationSize,
  IN OUT VOID                             *Scratch,
  IN     UINT32                           ScratchSize
  )
{
  EFI_STATUS  Status;
  UINT32      TestDestinationSize;
  UINT32      TestScratchSize;
  
  if (Source == NULL 
        || Destination== NULL 
        || Scratch == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status = TianoDecompressGetInfo (Source, SourceSize, &TestDestinationSize, &TestScratchSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ScratchSize < TestScratchSize || DestinationSize < TestDestinationSize) {
    return RETURN_INVALID_PARAMETER;
  }

  return TianoDecompress (Source, Destination, Scratch);
}

EFI_STATUS
DxeMainCustomDecompressGetInfo (
  IN EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL  *This,
  IN   VOID                              *Source,
  IN   UINT32                            SourceSize,
  OUT  UINT32                            *DestinationSize,
  OUT  UINT32                            *ScratchSize
  )
{
  if (Source == NULL 
        || DestinationSize == NULL 
        || ScratchSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  return CustomDecompressGetInfo (Source, SourceSize, DestinationSize, ScratchSize);
}

EFI_STATUS
EFIAPI
DxeMainCustomDecompress (
  IN EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL  *This,
  IN     VOID                            *Source,
  IN     UINT32                          SourceSize,
  IN OUT VOID                            *Destination,
  IN     UINT32                          DestinationSize,
  IN OUT VOID                            *Scratch,
  IN     UINT32                          ScratchSize
  )
{
  EFI_STATUS  Status;
  UINT32      TestDestinationSize;
  UINT32      TestScratchSize;

  if (Source == NULL 
        || Destination== NULL 
        || Scratch == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CustomDecompressGetInfo (Source, SourceSize, &TestDestinationSize, &TestScratchSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ScratchSize < TestScratchSize || DestinationSize < TestDestinationSize) {
    return RETURN_INVALID_PARAMETER;
  }

  return CustomDecompress (Source, Destination, Scratch);
}


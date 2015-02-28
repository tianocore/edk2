/**@file

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  SecMain.c

Abstract:
  WinNt emulator of SEC phase. It's really a Win32 application, but this is
  Ok since all the other modules for NT32 are NOT Win32 applications.

  This program gets NT32 PCD setting and figures out what the memory layout 
  will be, how may FD's will be loaded and also what the boot mode is.

  The SEC registers a set of services with the SEC core. gPrivateDispatchTable
  is a list of PPI's produced by the SEC that are availble for usage in PEI.

  This code produces 128 K of temporary memory for the PEI stack by directly
  allocate memory space with ReadWrite and Execute attribute.

**/

#include "SecMain.h"

#ifndef SE_TIME_ZONE_NAME
#define SE_TIME_ZONE_NAME                 TEXT("SeTimeZonePrivilege")
#endif

NT_PEI_LOAD_FILE_PPI                      mSecNtLoadFilePpi     = { SecWinNtPeiLoadFile };

PEI_NT_AUTOSCAN_PPI                       mSecNtAutoScanPpi     = { SecWinNtPeiAutoScan };

PEI_NT_THUNK_PPI                          mSecWinNtThunkPpi     = { SecWinNtWinNtThunkAddress };

EFI_PEI_PROGRESS_CODE_PPI                 mSecStatusCodePpi     = { SecPeiReportStatusCode };

NT_FWH_PPI                                mSecFwhInformationPpi = { SecWinNtFdAddress };

EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI         mSecTemporaryRamSupportPpi = {SecTemporaryRamSupport};

EFI_PEI_PPI_DESCRIPTOR  gPrivateDispatchTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gNtPeiLoadFilePpiGuid,
    &mSecNtLoadFilePpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gPeiNtAutoScanPpiGuid,
    &mSecNtAutoScanPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gPeiNtThunkPpiGuid,
    &mSecWinNtThunkPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiPeiStatusCodePpiGuid,
    &mSecStatusCodePpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamSupportPpiGuid,
    &mSecTemporaryRamSupportPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gNtFwhPpiGuid,
    &mSecFwhInformationPpi
  }
};


//
// Default information about where the FD is located.
//  This array gets filled in with information from PcdWinNtFirmwareVolume
//  The number of array elements is allocated base on parsing
//  PcdWinNtFirmwareVolume and the memory is never freed.
//
UINTN                                     gFdInfoCount = 0;
NT_FD_INFO                                *gFdInfo;

//
// Array that supports seperate memory rantes.
//  The memory ranges are set by PcdWinNtMemorySizeForSecMain.
//  The number of array elements is allocated base on parsing
//  PcdWinNtMemorySizeForSecMain value and the memory is never freed.
//
UINTN                                     gSystemMemoryCount = 0;
NT_SYSTEM_MEMORY                          *gSystemMemory;

VOID
EFIAPI
SecSwitchStack (
  UINT32   TemporaryMemoryBase,
  UINT32   PermenentMemoryBase
  );
EFI_STATUS
SecNt32PeCoffRelocateImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

VOID
EFIAPI
PeiSwitchStacks (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *Context3,  OPTIONAL
  IN      VOID                      *NewStack
  );

VOID
SecPrint (
  CHAR8  *Format,
  ...
  )
{
  va_list  Marker;
  UINTN    CharCount;
  CHAR8    Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];

  va_start (Marker, Format);
  
  _vsnprintf (Buffer, sizeof (Buffer), Format, Marker);

  va_end (Marker);

  CharCount = strlen (Buffer);
  WriteFile (
    GetStdHandle (STD_OUTPUT_HANDLE), 
    Buffer,
    (DWORD)CharCount,
    (LPDWORD)&CharCount,
    NULL
    );
}

INTN
EFIAPI
main (
  IN  INTN  Argc,
  IN  CHAR8 **Argv,
  IN  CHAR8 **Envp
  )
/*++

Routine Description:
  Main entry point to SEC for WinNt. This is a Windows program

Arguments:
  Argc - Number of command line arguments
  Argv - Array of command line argument strings
  Envp - Array of environmemt variable strings

Returns:
  0 - Normal exit
  1 - Abnormal exit

--*/
{
  EFI_STATUS            Status;
  HANDLE                Token;
  TOKEN_PRIVILEGES      TokenPrivileges;
  EFI_PHYSICAL_ADDRESS  InitialStackMemory;
  UINT64                InitialStackMemorySize;
  UINTN                 Index;
  UINTN                 Index1;
  UINTN                 Index2;
  CHAR16                *FileName;
  CHAR16                *FileNamePtr;
  BOOLEAN               Done;
  VOID                  *PeiCoreFile;
  CHAR16                *MemorySizeStr;
  CHAR16                *FirmwareVolumesStr;
  UINTN                 *StackPointer;
  UINT32                ProcessAffinityMask;
  UINT32                SystemAffinityMask;
  INT32                 LowBit;


  //
  // Enable the privilege so that RTC driver can successfully run SetTime()
  //
  OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &Token);
  if (LookupPrivilegeValue(NULL, SE_TIME_ZONE_NAME, &TokenPrivileges.Privileges[0].Luid)) {
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(Token, FALSE, &TokenPrivileges, 0, (PTOKEN_PRIVILEGES) NULL, 0);
  }

  MemorySizeStr      = (CHAR16 *) PcdGetPtr (PcdWinNtMemorySizeForSecMain);
  FirmwareVolumesStr = (CHAR16 *) PcdGetPtr (PcdWinNtFirmwareVolume);

  SecPrint ("\nEDK II SEC Main NT Emulation Environment from www.TianoCore.org\n");

  //
  // Determine the first thread available to this process.
  //
  if (GetProcessAffinityMask (GetCurrentProcess (), &ProcessAffinityMask, &SystemAffinityMask)) {
    LowBit = (INT32)LowBitSet32 (ProcessAffinityMask);
    if (LowBit != -1) {
      //
      // Force the system to bind the process to a single thread to work
      // around odd semaphore type crashes.
      //
      SetProcessAffinityMask (GetCurrentProcess (), (INTN)(BIT0 << LowBit));
    }
  }

  //
  // Make some Windows calls to Set the process to the highest priority in the
  //  idle class. We need this to have good performance.
  //
  SetPriorityClass (GetCurrentProcess (), IDLE_PRIORITY_CLASS);
  SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_HIGHEST);

  //
  // Allocate space for gSystemMemory Array
  //
  gSystemMemoryCount  = CountSeperatorsInString (MemorySizeStr, '!') + 1;
  gSystemMemory       = calloc (gSystemMemoryCount, sizeof (NT_SYSTEM_MEMORY));
  if (gSystemMemory == NULL) {
    SecPrint ("ERROR : Can not allocate memory for %S.  Exiting.\n", MemorySizeStr);
    exit (1);
  }
  //
  // Allocate space for gSystemMemory Array
  //
  gFdInfoCount  = CountSeperatorsInString (FirmwareVolumesStr, '!') + 1;
  gFdInfo       = calloc (gFdInfoCount, sizeof (NT_FD_INFO));
  if (gFdInfo == NULL) {
    SecPrint ("ERROR : Can not allocate memory for %S.  Exiting.\n", FirmwareVolumesStr);
    exit (1);
  }
  //
  // Setup Boot Mode. If BootModeStr == "" then BootMode = 0 (BOOT_WITH_FULL_CONFIGURATION)
  //
  SecPrint ("  BootMode 0x%02x\n", PcdGet32 (PcdWinNtBootMode));

  //
  //  Allocate 128K memory to emulate temp memory for PEI.
  //  on a real platform this would be SRAM, or using the cache as RAM.
  //  Set InitialStackMemory to zero so WinNtOpenFile will allocate a new mapping
  //
  InitialStackMemorySize  = STACK_SIZE;
  InitialStackMemory = (EFI_PHYSICAL_ADDRESS) (UINTN) VirtualAlloc (NULL, (SIZE_T) (InitialStackMemorySize), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (InitialStackMemory == 0) {
    SecPrint ("ERROR : Can not allocate enough space for SecStack\n");
    exit (1);
  }

  for (StackPointer = (UINTN*) (UINTN) InitialStackMemory;
       StackPointer < (UINTN*) ((UINTN)InitialStackMemory + (SIZE_T) InitialStackMemorySize);
       StackPointer ++) {
    *StackPointer = 0x5AA55AA5;
  }
  
  SecPrint ("  SEC passing in %d bytes of temp RAM to PEI\n", InitialStackMemorySize);

  //
  // Open All the firmware volumes and remember the info in the gFdInfo global
  //
  FileNamePtr = (CHAR16 *)malloc (StrLen ((CHAR16 *)FirmwareVolumesStr) * sizeof(CHAR16));
  if (FileNamePtr == NULL) {
    SecPrint ("ERROR : Can not allocate memory for firmware volume string\n");
    exit (1);
  }

  StrCpy (FileNamePtr, (CHAR16*)FirmwareVolumesStr);

  for (Done = FALSE, Index = 0, PeiCoreFile = NULL; !Done; Index++) {
    FileName = FileNamePtr;
    for (Index1 = 0; (FileNamePtr[Index1] != '!') && (FileNamePtr[Index1] != 0); Index1++)
      ;
    if (FileNamePtr[Index1] == 0) {
      Done = TRUE;
    } else {
      FileNamePtr[Index1]  = '\0';
      FileNamePtr = FileNamePtr + Index1 + 1;
    }

    //
    // Open the FD and remmeber where it got mapped into our processes address space
    //
    Status = WinNtOpenFile (
              FileName,
              0,
              OPEN_EXISTING,
              &gFdInfo[Index].Address,
              &gFdInfo[Index].Size
              );
    if (EFI_ERROR (Status)) {
      SecPrint ("ERROR : Can not open Firmware Device File %S (0x%X).  Exiting.\n", FileName, Status);
      exit (1);
    }

    SecPrint ("  FD loaded from");
    //
    // printf can't print filenames directly as the \ gets interperted as an
    //  escape character.
    //
    for (Index2 = 0; FileName[Index2] != '\0'; Index2++) {
      SecPrint ("%c", FileName[Index2]);
    }

    if (PeiCoreFile == NULL) {
      //
      // Assume the beginning of the FD is an FV and look for the PEI Core.
      // Load the first one we find.
      //
      Status = SecFfsFindPeiCore ((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) gFdInfo[Index].Address, &PeiCoreFile);
      if (!EFI_ERROR (Status)) {
        SecPrint (" contains SEC Core");
      }
    }

    SecPrint ("\n");
  }
  //
  // Calculate memory regions and store the information in the gSystemMemory
  //  global for later use. The autosizing code will use this data to
  //  map this memory into the SEC process memory space.
  //
  for (Index = 0, Done = FALSE; !Done; Index++) {
    //
    // Save the size of the memory and make a Unicode filename SystemMemory00, ...
    //
    gSystemMemory[Index].Size = _wtoi (MemorySizeStr) * 0x100000;

    //
    // Find the next region
    //
    for (Index1 = 0; MemorySizeStr[Index1] != '!' && MemorySizeStr[Index1] != 0; Index1++)
      ;
    if (MemorySizeStr[Index1] == 0) {
      Done = TRUE;
    }

    MemorySizeStr = MemorySizeStr + Index1 + 1;
  }

  SecPrint ("\n");

  //
  // Hand off to PEI Core
  //
  SecLoadFromCore ((UINTN) InitialStackMemory, (UINTN) InitialStackMemorySize, (UINTN) gFdInfo[0].Address, PeiCoreFile);

  //
  // If we get here, then the PEI Core returned. This is an error as PEI should
  //  always hand off to DXE.
  //
  SecPrint ("ERROR : PEI Core returned\n");
  exit (1);
}

EFI_STATUS
WinNtOpenFile (
  IN  CHAR16                    *FileName,
  IN  UINT32                    MapSize,
  IN  DWORD                     CreationDisposition,
  IN OUT  EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                    *Length
  )
/*++

Routine Description:
  Opens and memory maps a file using WinNt services. If BaseAddress is non zero
  the process will try and allocate the memory starting at BaseAddress.

Arguments:
  FileName            - The name of the file to open and map
  MapSize             - The amount of the file to map in bytes
  CreationDisposition - The flags to pass to CreateFile().  Use to create new files for
                        memory emulation, and exiting files for firmware volume emulation
  BaseAddress         - The base address of the mapped file in the user address space.
                         If passed in as NULL the a new memory region is used.
                         If passed in as non NULL the request memory region is used for
                          the mapping of the file into the process space.
  Length              - The size of the mapped region in bytes

Returns:
  EFI_SUCCESS      - The file was opened and mapped.
  EFI_NOT_FOUND    - FileName was not found in the current directory
  EFI_DEVICE_ERROR - An error occured attempting to map the opened file

--*/
{
  HANDLE  NtFileHandle;
  HANDLE  NtMapHandle;
  VOID    *VirtualAddress;
  UINTN   FileSize;

  //
  // Use Win API to open/create a file
  //
  NtFileHandle = CreateFile (
                  FileName,
                  GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                  FILE_SHARE_READ,
                  NULL,
                  CreationDisposition,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL
                  );
  if (NtFileHandle == INVALID_HANDLE_VALUE) {
    return EFI_NOT_FOUND;
  }
  //
  // Map the open file into a memory range
  //
  NtMapHandle = CreateFileMapping (
                  NtFileHandle,
                  NULL,
                  PAGE_EXECUTE_READWRITE,
                  0,
                  MapSize,
                  NULL
                  );
  if (NtMapHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Get the virtual address (address in the emulator) of the mapped file
  //
  VirtualAddress = MapViewOfFileEx (
                    NtMapHandle,
                    FILE_MAP_EXECUTE | FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    MapSize,
                    (LPVOID) (UINTN) *BaseAddress
                    );
  if (VirtualAddress == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (MapSize == 0) {
    //
    // Seek to the end of the file to figure out the true file size.
    //
    FileSize = SetFilePointer (
                NtFileHandle,
                0,
                NULL,
                FILE_END
                );
    if (FileSize == -1) {
      return EFI_DEVICE_ERROR;
    }

    *Length = (UINT64) FileSize;
  } else {
    *Length = (UINT64) MapSize;
  }

  *BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) VirtualAddress;

  return EFI_SUCCESS;
}


#define BYTES_PER_RECORD  512

EFI_STATUS
EFIAPI
SecPeiReportStatusCode (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN EFI_STATUS_CODE_TYPE       CodeType,
  IN EFI_STATUS_CODE_VALUE      Value,
  IN UINT32                     Instance,
  IN CONST EFI_GUID                   *CallerId,
  IN CONST EFI_STATUS_CODE_DATA       *Data OPTIONAL
  )
/*++

Routine Description:

  This routine produces the ReportStatusCode PEI service. It's passed
  up to the PEI Core via a PPI. T

  This code currently uses the NT clib printf. This does not work the same way
  as the EFI Print (), as %t, %g, %s as Unicode are not supported.

Arguments:
  (see EFI_PEI_REPORT_STATUS_CODE)

Returns:
  EFI_SUCCESS - Always return success

--*/
// TODO:    PeiServices - add argument and description to function comment
// TODO:    CodeType - add argument and description to function comment
// TODO:    Value - add argument and description to function comment
// TODO:    Instance - add argument and description to function comment
// TODO:    CallerId - add argument and description to function comment
// TODO:    Data - add argument and description to function comment
{
  CHAR8           *Format;
  BASE_LIST       Marker;
  CHAR8           PrintBuffer[BYTES_PER_RECORD * 2];
  CHAR8           *Filename;
  CHAR8           *Description;
  UINT32          LineNumber;
  UINT32          ErrorLevel;


  if (Data == NULL) {
  } else if (ReportStatusCodeExtractAssertInfo (CodeType, Value, Data, &Filename, &Description, &LineNumber)) {
    //
    // Processes ASSERT ()
    //
    SecPrint ("ASSERT %s(%d): %s\n", Filename, (int)LineNumber, Description);

  } else if (ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {
    //
    // Process DEBUG () macro 
    //
    AsciiBSPrint (PrintBuffer, BYTES_PER_RECORD, Format, Marker);
    SecPrint (PrintBuffer);
  }

  return EFI_SUCCESS;
}

#if defined (MDE_CPU_IA32)
/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  NewBsp      A pointer to the new BSP for the EntryPoint on IPF. It's
                      Reserved on other architectures.

**/
VOID
EFIAPI
PeiSwitchStacks (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *Context3,  OPTIONAL
  IN      VOID                      *NewStack
  )
{
  BASE_LIBRARY_JUMP_BUFFER  JumpBuffer;
  
  ASSERT (EntryPoint != NULL);
  ASSERT (NewStack != NULL);

  //
  // Stack should be aligned with CPU_STACK_ALIGNMENT
  //
  ASSERT (((UINTN)NewStack & (CPU_STACK_ALIGNMENT - 1)) == 0);

  JumpBuffer.Eip = (UINTN)EntryPoint;
  JumpBuffer.Esp = (UINTN)NewStack - sizeof (VOID*);
  JumpBuffer.Esp -= sizeof (Context1) + sizeof (Context2) + sizeof(Context3);
  ((VOID**)JumpBuffer.Esp)[1] = Context1;
  ((VOID**)JumpBuffer.Esp)[2] = Context2;
  ((VOID**)JumpBuffer.Esp)[3] = Context3;

  LongJump (&JumpBuffer, (UINTN)-1);
  

  //
  // InternalSwitchStack () will never return
  //
  ASSERT (FALSE);  
}
#endif

VOID
SecLoadFromCore (
  IN  UINTN   LargestRegion,
  IN  UINTN   LargestRegionSize,
  IN  UINTN   BootFirmwareVolumeBase,
  IN  VOID    *PeiCorePe32File
  )
/*++

Routine Description:
  This is the service to load the PEI Core from the Firmware Volume

Arguments:
  LargestRegion           - Memory to use for PEI.
  LargestRegionSize       - Size of Memory to use for PEI
  BootFirmwareVolumeBase  - Start of the Boot FV
  PeiCorePe32File         - PEI Core PE32

Returns:
  Success means control is transfered and thus we should never return

--*/
{
  EFI_STATUS                  Status;
  VOID                        *TopOfStack;
  UINT64                      PeiCoreSize;
  EFI_PHYSICAL_ADDRESS        PeiCoreEntryPoint;
  EFI_PHYSICAL_ADDRESS        PeiImageAddress;
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  UINTN                       PeiStackSize;

  //
  // Compute Top Of Memory for Stack and PEI Core Allocations
  //
  PeiStackSize = (UINTN)RShiftU64((UINT64)STACK_SIZE,1);

  //
  // |-----------| <---- TemporaryRamBase + TemporaryRamSize
  // |   Heap    |
  // |           |
  // |-----------| <---- StackBase / PeiTemporaryMemoryBase
  // |           |
  // |  Stack    |
  // |-----------| <---- TemporaryRamBase
  // 
  TopOfStack  = (VOID *)(LargestRegion + PeiStackSize);

  //
  // Reservet space for storing PeiCore's parament in stack.
  // 
  TopOfStack  = (VOID *)((UINTN)TopOfStack - sizeof (EFI_SEC_PEI_HAND_OFF) - CPU_STACK_ALIGNMENT);
  TopOfStack  = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // Bind this information into the SEC hand-off state
  //
  SecCoreData                        = (EFI_SEC_PEI_HAND_OFF*)(UINTN) TopOfStack;
  SecCoreData->DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
  SecCoreData->BootFirmwareVolumeBase = (VOID*)BootFirmwareVolumeBase;
  SecCoreData->BootFirmwareVolumeSize = PcdGet32(PcdWinNtFirmwareFdSize);
  SecCoreData->TemporaryRamBase       = (VOID*)(UINTN)LargestRegion; 
  SecCoreData->TemporaryRamSize       = STACK_SIZE;
  SecCoreData->StackBase              = SecCoreData->TemporaryRamBase;
  SecCoreData->StackSize              = PeiStackSize;
  SecCoreData->PeiTemporaryRamBase    = (VOID*) ((UINTN) SecCoreData->TemporaryRamBase + PeiStackSize);
  SecCoreData->PeiTemporaryRamSize    = STACK_SIZE - PeiStackSize;

  //
  // Load the PEI Core from a Firmware Volume
  //
  Status = SecWinNtPeiLoadFile (
            PeiCorePe32File,
            &PeiImageAddress,
            &PeiCoreSize,
            &PeiCoreEntryPoint
            );
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Transfer control to the PEI Core
  //
  PeiSwitchStacks (
    (SWITCH_STACK_ENTRY_POINT) (UINTN) PeiCoreEntryPoint,
    SecCoreData,
    (VOID *) (UINTN) ((EFI_PEI_PPI_DESCRIPTOR *) &gPrivateDispatchTable),
    NULL,
    TopOfStack
    );
  //
  // If we get here, then the PEI Core returned.  This is an error
  //
  return ;
}

EFI_STATUS
EFIAPI
SecWinNtPeiAutoScan (
  IN  UINTN                 Index,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBase,
  OUT UINT64                *MemorySize
  )
/*++

Routine Description:
  This service is called from Index == 0 until it returns EFI_UNSUPPORTED.
  It allows discontiguous memory regions to be supported by the emulator.
  It uses gSystemMemory[] and gSystemMemoryCount that were created by
  parsing PcdWinNtMemorySizeForSecMain value.
  The size comes from the Pcd value and the address comes from the memory space 
  with ReadWrite and Execute attributes allocated by VirtualAlloc() API.

Arguments:
  Index      - Which memory region to use
  MemoryBase - Return Base address of memory region
  MemorySize - Return size in bytes of the memory region

Returns:
  EFI_SUCCESS - If memory region was mapped
  EFI_UNSUPPORTED - If Index is not supported

--*/
{
  if (Index >= gSystemMemoryCount) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Allocate enough memory space for emulator 
  //
  gSystemMemory[Index].Memory = (EFI_PHYSICAL_ADDRESS) (UINTN) VirtualAlloc (NULL, (SIZE_T) (gSystemMemory[Index].Size), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (gSystemMemory[Index].Memory == 0) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  *MemoryBase = gSystemMemory[Index].Memory;
  *MemorySize = gSystemMemory[Index].Size;

  return EFI_SUCCESS;
}

VOID *
EFIAPI
SecWinNtWinNtThunkAddress (
  VOID
  )
/*++

Routine Description:
  Since the SEC is the only Windows program in stack it must export
  an interface to do Win API calls. That's what the WinNtThunk address
  is for. gWinNt is initailized in WinNtThunk.c.

Arguments:
  InterfaceSize - sizeof (EFI_WIN_NT_THUNK_PROTOCOL);
  InterfaceBase - Address of the gWinNt global

Returns:
  EFI_SUCCESS - Data returned

--*/
{
  return gWinNt;
}


EFI_STATUS
EFIAPI
SecWinNtPeiLoadFile (
  IN  VOID                    *Pe32Data,
  IN  EFI_PHYSICAL_ADDRESS    *ImageAddress,
  IN  UINT64                  *ImageSize,
  IN  EFI_PHYSICAL_ADDRESS    *EntryPoint
  )
/*++

Routine Description:
  Loads and relocates a PE/COFF image into memory.

Arguments:
  Pe32Data         - The base address of the PE/COFF file that is to be loaded and relocated
  ImageAddress     - The base address of the relocated PE/COFF image
  ImageSize        - The size of the relocated PE/COFF image
  EntryPoint       - The entry point of the relocated PE/COFF image

Returns:
  EFI_SUCCESS   - The file was loaded and relocated
  EFI_OUT_OF_RESOURCES - There was not enough memory to load and relocate the PE/COFF file

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle     = Pe32Data;

  ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) SecImageRead;

  Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate space in NT (not emulator) memory with ReadWrite and Execute attribue. 
  // Extra space is for alignment
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) VirtualAlloc (NULL, (SIZE_T) (ImageContext.ImageSize + (ImageContext.SectionAlignment * 2)), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (ImageContext.ImageAddress == 0) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Align buffer on section boundry
  //
  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~(ImageContext.SectionAlignment - 1);

  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SecNt32PeCoffRelocateImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // BugBug: Flush Instruction Cache Here when CPU Lib is ready
  //

  *ImageAddress = ImageContext.ImageAddress;
  *ImageSize    = ImageContext.ImageSize;
  *EntryPoint   = ImageContext.EntryPoint;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SecWinNtFdAddress (
  IN     UINTN                 Index,
  IN OUT EFI_PHYSICAL_ADDRESS  *FdBase,
  IN OUT UINT64                *FdSize
  )
/*++

Routine Description:
  Return the FD Size and base address. Since the FD is loaded from a
  file into Windows memory only the SEC will know it's address.

Arguments:
  Index  - Which FD, starts at zero.
  FdSize - Size of the FD in bytes
  FdBase - Start address of the FD. Assume it points to an FV Header

Returns:
  EFI_SUCCESS     - Return the Base address and size of the FV
  EFI_UNSUPPORTED - Index does nto map to an FD in the system

--*/
{
  if (Index >= gFdInfoCount) {
    return EFI_UNSUPPORTED;
  }

  *FdBase = gFdInfo[Index].Address;
  *FdSize = gFdInfo[Index].Size;

  if (*FdBase == 0 && *FdSize == 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SecImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:
  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:
  FileHandle - The handle to the PE/COFF file
  FileOffset - The offset, in bytes, into the file to read
  ReadSize   - The number of bytes to read from the file starting at FileOffset
  Buffer     - A pointer to the buffer to read the data into.

Returns:
  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8 *Destination8;
  CHAR8 *Source8;
  UINTN Length;

  Destination8  = Buffer;
  Source8       = (CHAR8 *) ((UINTN) FileHandle + FileOffset);
  Length        = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

CHAR16 *
AsciiToUnicode (
  IN  CHAR8   *Ascii,
  IN  UINTN   *StrLen OPTIONAL
  )
/*++

Routine Description:
  Convert the passed in Ascii string to Unicode.
  Optionally return the length of the strings.

Arguments:
  Ascii   - Ascii string to convert
  StrLen  - Length of string

Returns:
  Pointer to malloc'ed Unicode version of Ascii

--*/
{
  UINTN   Index;
  CHAR16  *Unicode;

  //
  // Allocate a buffer for unicode string
  //
  for (Index = 0; Ascii[Index] != '\0'; Index++)
    ;
  Unicode = malloc ((Index + 1) * sizeof (CHAR16));
  if (Unicode == NULL) {
    return NULL;
  }

  for (Index = 0; Ascii[Index] != '\0'; Index++) {
    Unicode[Index] = (CHAR16) Ascii[Index];
  }

  Unicode[Index] = '\0';

  if (StrLen != NULL) {
    *StrLen = Index;
  }

  return Unicode;
}

UINTN
CountSeperatorsInString (
  IN  CONST CHAR16   *String,
  IN  CHAR16         Seperator
  )
/*++

Routine Description:
  Count the number of seperators in String

Arguments:
  String    - String to process
  Seperator - Item to count

Returns:
  Number of Seperator in String

--*/
{
  UINTN Count;

  for (Count = 0; *String != '\0'; String++) {
    if (*String == Seperator) {
      Count++;
    }
  }

  return Count;
}


EFI_STATUS
SecNt32PeCoffRelocateImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  EFI_STATUS        Status;
  VOID              *DllEntryPoint;
  CHAR16            *DllFileName;
  HMODULE           Library;
  UINTN             Index;


  Status = PeCoffLoaderRelocateImage (ImageContext);
  if (EFI_ERROR (Status)) {
    //
    // We could not relocated the image in memory properly
    //
    return Status;
  }

  //
  // If we load our own PE COFF images the Windows debugger can not source
  //  level debug our code. If a valid PDB pointer exists usw it to load
  //  the *.dll file as a library using Windows* APIs. This allows 
  //  source level debug. The image is still loaded and reloaced
  //  in the Framework memory space like on a real system (by the code above),
  //  but the entry point points into the DLL loaded by the code bellow. 
  //

  DllEntryPoint = NULL;

  //
  // Load the DLL if it's not an EBC image.
  //
  if ((ImageContext->PdbPointer != NULL) &&
      (ImageContext->Machine != EFI_IMAGE_MACHINE_EBC)) {
    //
    // Convert filename from ASCII to Unicode
    //
    DllFileName = AsciiToUnicode (ImageContext->PdbPointer, &Index);

    //
    // Check that we have a valid filename
    //
    if (Index < 5 || DllFileName[Index - 4] != '.') {
      free (DllFileName);

      //
      // Never return an error if PeCoffLoaderRelocateImage() succeeded.
      // The image will run, but we just can't source level debug. If we
      // return an error the image will not run.
      //
      return EFI_SUCCESS;
    }
    //
    // Replace .PDB with .DLL on the filename
    //
    DllFileName[Index - 3]  = 'D';
    DllFileName[Index - 2]  = 'L';
    DllFileName[Index - 1]  = 'L';

    //
    // Load the .DLL file into the user process's address space for source 
    // level debug
    //
    Library = LoadLibraryEx (DllFileName, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (Library != NULL) {
      //
      // InitializeDriver is the entry point we put in all our EFI DLL's. The
      // DONT_RESOLVE_DLL_REFERENCES argument to LoadLIbraryEx() supresses the 
      // normal DLL entry point of DllMain, and prevents other modules that are
      // referenced in side the DllFileName from being loaded. There is no error 
      // checking as the we can point to the PE32 image loaded by Tiano. This 
      // step is only needed for source level debuging
      //
      DllEntryPoint = (VOID *) (UINTN) GetProcAddress (Library, "InitializeDriver");

    }

    if ((Library != NULL) && (DllEntryPoint != NULL)) {
      ImageContext->EntryPoint  = (EFI_PHYSICAL_ADDRESS) (UINTN) DllEntryPoint;
      SecPrint ("LoadLibraryEx (%S,\n               NULL, DONT_RESOLVE_DLL_REFERENCES)\n", DllFileName);
    } else {
      SecPrint ("WARNING: No source level debug %S. \n", DllFileName);
    }

    free (DllFileName);
  }

  //
  // Never return an error if PeCoffLoaderRelocateImage() succeeded.
  // The image will run, but we just can't source level debug. If we
  // return an error the image will not run.
  //
  return EFI_SUCCESS;
}




VOID
_ModuleEntryPoint (
  VOID
  )
{
}

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  //
  // Migrate the whole temporary memory to permenent memory.
  // 
  CopyMem (
    (VOID*)(UINTN)PermanentMemoryBase, 
    (VOID*)(UINTN)TemporaryMemoryBase, 
    CopySize
    );

  //
  // SecSwitchStack function must be invoked after the memory migration
  // immediatly, also we need fixup the stack change caused by new call into 
  // permenent memory.
  // 
  SecSwitchStack (
    (UINT32) TemporaryMemoryBase,
    (UINT32) PermanentMemoryBase
    );

  //
  // We need *not* fix the return address because currently, 
  // The PeiCore is excuted in flash.
  //

  //
  // Simulate to invalid temporary memory, terminate temporary memory
  // 
  //ZeroMem ((VOID*)(UINTN)TemporaryMemoryBase, CopySize);
  
  return EFI_SUCCESS;
}


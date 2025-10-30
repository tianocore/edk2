/**@file
  WinNt emulator of pre-SEC phase. It's really a Win32 application, but this is
  Ok since all the other modules for NT32 are NOT Win32 applications.

  This program gets NT32 PCD setting and figures out what the memory layout
  will be, how may FD's will be loaded and also what the boot mode is.

  This code produces 128 K of temporary memory for the SEC stack by directly
  allocate memory space with ReadWrite and Execute attribute.

Copyright (c) 2006 - 2023, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016-2020 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "WinHost.h"

#ifndef SE_TIME_ZONE_NAME
#define SE_TIME_ZONE_NAME  TEXT("SeTimeZonePrivilege")
#endif

//
// The growth size for array of module handle entries
//
#define MAX_PDB_NAME_TO_MOD_HANDLE_ARRAY_SIZE  0x100

//
// Module handle entry structure
//
typedef struct {
  CHAR8    *PdbPointer;
  VOID     *ModHandle;
} PDB_NAME_TO_MOD_HANDLE;

//
// An Array to hold the module handles
//
PDB_NAME_TO_MOD_HANDLE  *mPdbNameModHandleArray    = NULL;
UINTN                   mPdbNameModHandleArraySize = 0;

//
// Default information about where the FD is located.
//  This array gets filled in with information from PcdWinNtFirmwareVolume
//  The number of array elements is allocated base on parsing
//  PcdWinNtFirmwareVolume and the memory is never freed.
//
UINTN       gFdInfoCount = 0;
NT_FD_INFO  *gFdInfo;

//
// Array that supports separate memory ranges.
//  The memory ranges are set by PcdWinNtMemorySizeForSecMain.
//  The number of array elements is allocated base on parsing
//  PcdWinNtMemorySizeForSecMain value and the memory is never freed.
//
UINTN             gSystemMemoryCount = 0;
NT_SYSTEM_MEMORY  *gSystemMemory;

BASE_LIBRARY_JUMP_BUFFER  mResetJumpBuffer;
CHAR8                     *mResetTypeStr[] = {
  "EfiResetCold",
  "EfiResetWarm",
  "EfiResetShutdown",
  "EfiResetPlatformSpecific"
};

/*++

Routine Description:
  This service is called from Index == 0 until it returns EFI_UNSUPPORTED.
  It allows discontinuous memory regions to be supported by the emulator.
  It uses gSystemMemory[] and gSystemMemoryCount that were created by
  parsing the host environment variable EFI_MEMORY_SIZE.
  The size comes from the varaible and the address comes from the call to
  UnixOpenFile.

Arguments:
  Index      - Which memory region to use
  MemoryBase - Return Base address of memory region
  MemorySize - Return size in bytes of the memory region

Returns:
  EFI_SUCCESS - If memory region was mapped
  EFI_UNSUPPORTED - If Index is not supported

**/
EFI_STATUS
WinPeiAutoScan (
  IN  UINTN                 Index,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBase,
  OUT UINT64                *MemorySize
  )
{
  if (Index >= gSystemMemoryCount) {
    return EFI_UNSUPPORTED;
  }

  *MemoryBase = gSystemMemory[Index].Memory;
  *MemorySize = gSystemMemory[Index].Size;

  return EFI_SUCCESS;
}

/*++

Routine Description:
  Return the FD Size and base address. Since the FD is loaded from a
  file into host memory only the SEC will know its address.

Arguments:
  Index  - Which FD, starts at zero.
  FdSize - Size of the FD in bytes
  FdBase - Start address of the FD. Assume it points to an FV Header
  FixUp  - Difference between actual FD address and build address

Returns:
  EFI_SUCCESS     - Return the Base address and size of the FV
  EFI_UNSUPPORTED - Index does nto map to an FD in the system

**/
EFI_STATUS
WinFdAddress (
  IN     UINTN                 Index,
  IN OUT EFI_PHYSICAL_ADDRESS  *FdBase,
  IN OUT UINT64                *FdSize,
  IN OUT EFI_PHYSICAL_ADDRESS  *FixUp
  )
{
  if (Index >= gFdInfoCount) {
    return EFI_UNSUPPORTED;
  }

  *FdBase = (EFI_PHYSICAL_ADDRESS)(UINTN)gFdInfo[Index].Address;
  *FdSize = (UINT64)gFdInfo[Index].Size;
  *FixUp  = 0;

  if ((*FdBase == 0) && (*FdSize == 0)) {
    return EFI_UNSUPPORTED;
  }

  if (Index == 0) {
    //
    // FD 0 has XIP code and well known PCD values
    // If the memory buffer could not be allocated at the FD build address
    // the Fixup is the difference.
    //
    *FixUp = *FdBase - PcdGet64 (PcdEmuFdBaseAddress);
  }

  return EFI_SUCCESS;
}

/*++

Routine Description:
  Since the SEC is the only Unix program in stack it must export
  an interface to do POSIX calls.  gUnix is initialized in UnixThunk.c.

Arguments:
  InterfaceSize - sizeof (EFI_WIN_NT_THUNK_PROTOCOL);
  InterfaceBase - Address of the gUnix global

Returns:
  EFI_SUCCESS - Data returned

**/
VOID *
WinThunk (
  VOID
  )
{
  return &gEmuThunkProtocol;
}

EMU_THUNK_PPI  mSecEmuThunkPpi = {
  WinPeiAutoScan,
  WinFdAddress,
  WinThunk
};

VOID
SecPrint (
  CHAR8  *Format,
  ...
  )
{
  va_list  Marker;
  UINTN    CharCount;
  CHAR8    Buffer[0x1000];

  va_start (Marker, Format);

  _vsnprintf_s (Buffer, sizeof (Buffer), sizeof (Buffer) - 1, Format, Marker);

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

/**
  Resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of ResetData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                            the data buffer starts with a Null-terminated string, optionally
                            followed by additional binary data. The string is a description
                            that the caller may use to further indicate the reason for the
                            system reset.

**/
VOID
EFIAPI
WinReset (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  UINTN  Index;

  ASSERT (ResetType <= EfiResetPlatformSpecific);
  SecPrint ("  Emu ResetSystem is called: ResetType = %s\n", mResetTypeStr[ResetType]);

  if (ResetType == EfiResetShutdown) {
    exit (0);
  } else {
    //
    // Unload all DLLs
    //
    for (Index = 0; Index < mPdbNameModHandleArraySize; Index++) {
      if (mPdbNameModHandleArray[Index].PdbPointer != NULL) {
        SecPrint ("  Emu Unload DLL: %s\n", mPdbNameModHandleArray[Index].PdbPointer);
        FreeLibrary (mPdbNameModHandleArray[Index].ModHandle);
        HeapFree (GetProcessHeap (), 0, mPdbNameModHandleArray[Index].PdbPointer);
        mPdbNameModHandleArray[Index].PdbPointer = NULL;
      }
    }

    //
    // Jump back to SetJump with jump code = ResetType + 1
    //
    LongJump (&mResetJumpBuffer, ResetType + 1);
  }
}

EFI_PEI_RESET2_PPI  mEmuReset2Ppi = {
  WinReset
};

/*++

Routine Description:
 Check to see if an address range is in the EFI GCD memory map.

 This is all of GCD for system memory passed to DXE Core. FV
 mapping and other device mapped into system memory are not
 inlcuded in the check.

Arguments:
  Index      - Which memory region to use
  MemoryBase - Return Base address of memory region
  MemorySize - Return size in bytes of the memory region

Returns:
  TRUE -  Address is in the EFI GCD memory map
  FALSE - Address is NOT in memory map

**/
BOOLEAN
EfiSystemMemoryRange (
  IN  VOID  *MemoryAddress
  )
{
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  MemoryBase;

  MemoryBase = (EFI_PHYSICAL_ADDRESS)(UINTN)MemoryAddress;
  for (Index = 0; Index < gSystemMemoryCount; Index++) {
    if ((MemoryBase >= gSystemMemory[Index].Memory) &&
        (MemoryBase < (gSystemMemory[Index].Memory + gSystemMemory[Index].Size)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

EFI_STATUS
WinNtOpenFile (
  IN  CHAR16    *FileName             OPTIONAL,
  IN  UINT32    MapSize,
  IN  DWORD     CreationDisposition,
  IN OUT  VOID  **BaseAddress,
  OUT UINTN     *Length
  )

/*++

Routine Description:
  Opens and memory maps a file using WinNt services. If *BaseAddress is non zero
  the process will try and allocate the memory starting at BaseAddress.

Arguments:
  FileName            - The name of the file to open and map
  MapSize             - The amount of the file to map in bytes
  CreationDisposition - The flags to pass to CreateFile().  Use to create new files for
                        memory emulation, and exiting files for firmware volume emulation
  BaseAddress         - The base address of the mapped file in the user address space.
                         If *BaseAddress is 0, the new memory region is used.
                         If *BaseAddress is not 0, the request memory region is used for
                          the mapping of the file into the process space.
  Length              - The size of the mapped region in bytes

Returns:
  EFI_SUCCESS      - The file was opened and mapped.
  EFI_NOT_FOUND    - FileName was not found in the current directory
  EFI_DEVICE_ERROR - An error occurred attempting to map the opened file

--*/
{
  HANDLE  NtFileHandle;
  HANDLE  NtMapHandle;
  VOID    *VirtualAddress;
  UINTN   FileSize;

  //
  // Use Win API to open/create a file
  //
  NtFileHandle = INVALID_HANDLE_VALUE;
  if (FileName != NULL) {
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
                     *BaseAddress
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

    *Length = FileSize;
  } else {
    *Length = MapSize;
  }

  *BaseAddress = VirtualAddress;

  return EFI_SUCCESS;
}

INTN
EFIAPI
main (
  IN  INT    Argc,
  IN  CHAR8  **Argv,
  IN  CHAR8  **Envp
  )

/*++

Routine Description:
  Main entry point to SEC for WinNt. This is a Windows program

Arguments:
  Argc - Number of command line arguments
  Argv - Array of command line argument strings
  Envp - Array of environment variable strings

Returns:
  0 - Normal exit
  1 - Abnormal exit

--*/
{
  EFI_STATUS           Status;
  HANDLE               Token;
  TOKEN_PRIVILEGES     TokenPrivileges;
  VOID                 *TemporaryRam;
  UINT32               TemporaryRamSize;
  VOID                 *EmuMagicPage;
  UINTN                Index;
  UINTN                Index1;
  CHAR16               *FileName;
  CHAR16               *FileNamePtr;
  BOOLEAN              Done;
  EFI_PEI_FILE_HANDLE  FileHandle;
  VOID                 *SecFile;
  CHAR16               *MemorySizeStr;
  CHAR16               *FirmwareVolumesStr;
  UINTN                ProcessAffinityMask;
  UINTN                SystemAffinityMask;
  INT32                LowBit;
  UINTN                ResetJumpCode;
  EMU_THUNK_PPI        *SecEmuThunkPpi;

  //
  // If enabled use the magic page to communicate between modules
  // This replaces the PI PeiServicesTable pointer mechanism that
  // deos not work in the emulator. It also allows the removal of
  // writable globals from SEC, PEI_CORE (libraries), PEIMs
  //
  EmuMagicPage = (VOID *)(UINTN)(FixedPcdGet64 (PcdPeiServicesTablePage) & MAX_UINTN);
  if (EmuMagicPage != NULL) {
    UINT64  Size;
    Status = WinNtOpenFile (
               NULL,
               SIZE_4KB,
               0,
               &EmuMagicPage,
               &Size
               );
    if (EFI_ERROR (Status)) {
      SecPrint ("ERROR : Could not allocate PeiServicesTablePage @ %p\n\r", EmuMagicPage);
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Enable the privilege so that RTC driver can successfully run SetTime()
  //
  OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &Token);
  if (LookupPrivilegeValue (NULL, SE_TIME_ZONE_NAME, &TokenPrivileges.Privileges[0].Luid)) {
    TokenPrivileges.PrivilegeCount           = 1;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges (Token, FALSE, &TokenPrivileges, 0, (PTOKEN_PRIVILEGES)NULL, 0);
  }

  MemorySizeStr      = (CHAR16 *)PcdGetPtr (PcdEmuMemorySize);
  FirmwareVolumesStr = (CHAR16 *)PcdGetPtr (PcdEmuFirmwareVolume);

  SecPrint ("\n\rEDK II WIN Host Emulation Environment from http://www.tianocore.org/edk2/\n\r");

  //
  // Determine the first thread available to this process.
  //
  if (GetProcessAffinityMask (GetCurrentProcess (), &ProcessAffinityMask, &SystemAffinityMask)) {
    LowBit = (INT32)LowBitSet32 ((UINT32)ProcessAffinityMask);
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

  SecInitializeThunk ();
  //
  // PPIs pased into PEI_CORE
  //
  SecEmuThunkPpi = AllocateZeroPool (sizeof (EMU_THUNK_PPI) + FixedPcdGet32 (PcdPersistentMemorySize));
  if (SecEmuThunkPpi == NULL) {
    SecPrint ("ERROR : Can not allocate memory for SecEmuThunkPpi.  Exiting.\n");
    exit (1);
  }

  CopyMem (SecEmuThunkPpi, &mSecEmuThunkPpi, sizeof (EMU_THUNK_PPI));
  SecEmuThunkPpi->Argc                 = Argc;
  SecEmuThunkPpi->Argv                 = Argv;
  SecEmuThunkPpi->Envp                 = Envp;
  SecEmuThunkPpi->PersistentMemorySize = FixedPcdGet32 (PcdPersistentMemorySize);
  AddThunkPpi (EFI_PEI_PPI_DESCRIPTOR_PPI, &gEmuThunkPpiGuid, SecEmuThunkPpi);
  AddThunkPpi (EFI_PEI_PPI_DESCRIPTOR_PPI, &gEfiPeiReset2PpiGuid, &mEmuReset2Ppi);

  //
  // Emulator Bus Driver Thunks
  //
  AddThunkProtocol (&mWinNtWndThunkIo, (CHAR16 *)PcdGetPtr (PcdEmuGop), TRUE);
  AddThunkProtocol (&mWinNtFileSystemThunkIo, (CHAR16 *)PcdGetPtr (PcdEmuFileSystem), TRUE);
  AddThunkProtocol (&mWinNtBlockIoThunkIo, (CHAR16 *)PcdGetPtr (PcdEmuVirtualDisk), TRUE);
  AddThunkProtocol (&mWinNtSnpThunkIo, (CHAR16 *)PcdGetPtr (PcdEmuNetworkInterface), TRUE);

  //
  // Allocate space for gSystemMemory Array
  //
  gSystemMemoryCount = CountSeparatorsInString (MemorySizeStr, '!') + 1;
  gSystemMemory      = calloc (gSystemMemoryCount, sizeof (NT_SYSTEM_MEMORY));
  if (gSystemMemory == NULL) {
    SecPrint ("ERROR : Can not allocate memory for %S.  Exiting.\n\r", MemorySizeStr);
    exit (1);
  }

  //
  // Allocate "physical" memory space for emulator. It will be reported out later throuth MemoryAutoScan()
  //
  for (Index = 0, Done = FALSE; !Done; Index++) {
    ASSERT (Index < gSystemMemoryCount);
    gSystemMemory[Index].Size   = ((UINT64)_wtoi (MemorySizeStr)) * ((UINT64)SIZE_1MB);
    gSystemMemory[Index].Memory = (EFI_PHYSICAL_ADDRESS)(UINTN)VirtualAlloc (NULL, (SIZE_T)(gSystemMemory[Index].Size), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (gSystemMemory[Index].Memory == 0) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Find the next region
    //
    for (Index1 = 0; MemorySizeStr[Index1] != '!' && MemorySizeStr[Index1] != 0; Index1++) {
    }

    if (MemorySizeStr[Index1] == 0) {
      Done = TRUE;
    }

    MemorySizeStr = MemorySizeStr + Index1 + 1;
  }

  //
  // Allocate space for gSystemMemory Array
  //
  gFdInfoCount = CountSeparatorsInString (FirmwareVolumesStr, '!') + 1;
  gFdInfo      = calloc (gFdInfoCount, sizeof (NT_FD_INFO));
  if (gFdInfo == NULL) {
    SecPrint ("ERROR : Can not allocate memory for %S.  Exiting.\n\r", FirmwareVolumesStr);
    exit (1);
  }

  //
  // Setup Boot Mode.
  //
  SecPrint ("  BootMode 0x%02x\n\r", PcdGet32 (PcdEmuBootMode));

  //
  //  Allocate 128K memory to emulate temp memory for PEI.
  //  on a real platform this would be SRAM, or using the cache as RAM.
  //  Set TemporaryRam to zero so WinNtOpenFile will allocate a new mapping
  //
  TemporaryRamSize = TEMPORARY_RAM_SIZE;
  TemporaryRam     = VirtualAlloc (NULL, (SIZE_T)(TemporaryRamSize), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (TemporaryRam == NULL) {
    SecPrint ("ERROR : Can not allocate enough space for SecStack\n\r");
    exit (1);
  }

  //
  // Open All the firmware volumes and remember the info in the gFdInfo global
  // Meanwhile, find the SEC Core.
  //
  FileNamePtr = AllocateCopyPool (StrSize (FirmwareVolumesStr), FirmwareVolumesStr);
  if (FileNamePtr == NULL) {
    SecPrint ("ERROR : Can not allocate memory for firmware volume string\n\r");
    exit (1);
  }

  for (Done = FALSE, Index = 0, SecFile = NULL; !Done; Index++) {
    FileName = FileNamePtr;
    for (Index1 = 0; (FileNamePtr[Index1] != '!') && (FileNamePtr[Index1] != 0); Index1++) {
    }

    if (FileNamePtr[Index1] == 0) {
      Done = TRUE;
    } else {
      FileNamePtr[Index1] = '\0';
      FileNamePtr         = &FileNamePtr[Index1 + 1];
    }

    //
    // Open the FD and remember where it got mapped into our processes address space
    //
    Status = WinNtOpenFile (
               FileName,
               0,
               OPEN_EXISTING,
               &gFdInfo[Index].Address,
               &gFdInfo[Index].Size
               );
    if (EFI_ERROR (Status)) {
      SecPrint ("ERROR : Can not open Firmware Device File %S (0x%X).  Exiting.\n\r", FileName, Status);
      exit (1);
    }

    SecPrint ("  FD loaded from %S", FileName);

    if (SecFile == NULL) {
      //
      // Assume the beginning of the FD is an FV and look for the SEC Core.
      // Load the first one we find.
      //
      FileHandle = NULL;
      Status     = PeiServicesFfsFindNextFile (
                     EFI_FV_FILETYPE_SECURITY_CORE,
                     (EFI_PEI_FV_HANDLE)gFdInfo[Index].Address,
                     &FileHandle
                     );
      if (!EFI_ERROR (Status)) {
        Status = PeiServicesFfsFindSectionData (EFI_SECTION_PE32, FileHandle, &SecFile);
        if (!EFI_ERROR (Status)) {
          SecPrint (" contains SEC Core");
        }
      }
    }

    SecPrint ("\n\r");
  }

  ResetJumpCode = SetJump (&mResetJumpBuffer);

  //
  // Do not clear memory content for warm reset.
  //
  if (ResetJumpCode != EfiResetWarm + 1) {
    SecPrint ("  OS Emulator clearing temp RAM and physical RAM (to be discovered later)......\n\r");
    SetMem32 (TemporaryRam, TemporaryRamSize, PcdGet32 (PcdInitValueInTempStack));
    for (Index = 0; Index < gSystemMemoryCount; Index++) {
      SetMem32 ((VOID *)(UINTN)gSystemMemory[Index].Memory, (UINTN)gSystemMemory[Index].Size, PcdGet32 (PcdInitValueInTempStack));
    }
  }

  SecPrint (
    "  OS Emulator passing in %u KB of temp RAM at 0x%08lx to SEC\n\r",
    TemporaryRamSize / SIZE_1KB,
    TemporaryRam
    );
  //
  // Hand off to SEC Core
  //
  SecLoadSecCore ((UINTN)TemporaryRam, TemporaryRamSize, gFdInfo[0].Address, gFdInfo[0].Size, SecFile);

  //
  // If we get here, then the SEC Core returned. This is an error as SEC should
  //  always hand off to PEI Core and then on to DXE Core.
  //
  SecPrint ("ERROR : SEC returned\n\r");
  exit (1);
}

VOID
SecLoadSecCore (
  IN  UINTN  TemporaryRam,
  IN  UINTN  TemporaryRamSize,
  IN  VOID   *BootFirmwareVolumeBase,
  IN  UINTN  BootFirmwareVolumeSize,
  IN  VOID   *SecCorePe32File
  )

/*++

Routine Description:
  This is the service to load the SEC Core from the Firmware Volume

Arguments:
  TemporaryRam            - Memory to use for SEC.
  TemporaryRamSize        - Size of Memory to use for SEC
  BootFirmwareVolumeBase  - Start of the Boot FV
  SecCorePe32File         - SEC Core PE32

Returns:
  Success means control is transferred and thus we should never return

--*/
{
  EFI_STATUS            Status;
  VOID                  *TopOfStack;
  VOID                  *SecCoreEntryPoint;
  EFI_SEC_PEI_HAND_OFF  *SecCoreData;
  UINTN                 SecStackSize;

  //
  // Compute Top Of Memory for Stack and PEI Core Allocations
  //
  SecStackSize = TemporaryRamSize >> 1;

  //
  // |-----------| <---- TemporaryRamBase + TemporaryRamSize
  // |   Heap    |
  // |           |
  // |-----------| <---- StackBase / PeiTemporaryMemoryBase
  // |           |
  // |  Stack    |
  // |-----------| <---- TemporaryRamBase
  //
  TopOfStack = (VOID *)(TemporaryRam + SecStackSize);

  //
  // Reservet space for storing PeiCore's parament in stack.
  //
  TopOfStack = (VOID *)((UINTN)TopOfStack - sizeof (EFI_SEC_PEI_HAND_OFF) - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // Bind this information into the SEC hand-off state
  //
  SecCoreData                         = (EFI_SEC_PEI_HAND_OFF *)(UINTN)TopOfStack;
  SecCoreData->DataSize               = sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData->BootFirmwareVolumeBase = BootFirmwareVolumeBase;
  SecCoreData->BootFirmwareVolumeSize = BootFirmwareVolumeSize;
  SecCoreData->TemporaryRamBase       = (VOID *)TemporaryRam;
  SecCoreData->TemporaryRamSize       = TemporaryRamSize;
  SecCoreData->StackBase              = SecCoreData->TemporaryRamBase;
  SecCoreData->StackSize              = SecStackSize;
  SecCoreData->PeiTemporaryRamBase    = (VOID *)((UINTN)SecCoreData->TemporaryRamBase + SecStackSize);
  SecCoreData->PeiTemporaryRamSize    = TemporaryRamSize - SecStackSize;

  //
  // Load the PEI Core from a Firmware Volume
  //
  Status = SecPeCoffGetEntryPoint (
             SecCorePe32File,
             &SecCoreEntryPoint
             );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Transfer control to the SEC Core
  //
  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)SecCoreEntryPoint,
    SecCoreData,
    GetThunkPpiList (),
    TopOfStack
    );
  //
  // If we get here, then the SEC Core returned.  This is an error
  //
  return;
}

RETURN_STATUS
EFIAPI
SecPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  )
{
  EFI_STATUS                    Status;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle = Pe32Data;

  ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE)SecImageRead;

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // XIP for SEC and PEI_CORE
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Pe32Data;

  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeCoffLoaderRelocateImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *EntryPoint = (VOID *)(UINTN)ImageContext.EntryPoint;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SecImageRead (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
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
  CHAR8  *Destination8;
  CHAR8  *Source8;
  UINTN  Length;

  Destination8 = Buffer;
  Source8      = (CHAR8 *)((UINTN)FileHandle + FileOffset);
  Length       = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

CHAR16 *
AsciiToUnicode (
  IN  CHAR8  *Ascii,
  IN  UINTN  *StrLen OPTIONAL
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
  for (Index = 0; Ascii[Index] != '\0'; Index++) {
  }

  Unicode = malloc ((Index + 1) * sizeof (CHAR16));
  if (Unicode == NULL) {
    return NULL;
  }

  for (Index = 0; Ascii[Index] != '\0'; Index++) {
    Unicode[Index] = (CHAR16)Ascii[Index];
  }

  Unicode[Index] = '\0';

  if (StrLen != NULL) {
    *StrLen = Index;
  }

  return Unicode;
}

UINTN
CountSeparatorsInString (
  IN  CONST CHAR16  *String,
  IN  CHAR16        Separator
  )

/*++

Routine Description:
  Count the number of separators in String

Arguments:
  String    - String to process
  Separator - Item to count

Returns:
  Number of Separator in String

--*/
{
  UINTN  Count;

  for (Count = 0; *String != '\0'; String++) {
    if (*String == Separator) {
      Count++;
    }
  }

  return Count;
}

/**
  Store the ModHandle in an array indexed by the Pdb File name.
  The ModHandle is needed to unload the image.
  @param ImageContext - Input data returned from PE Laoder Library. Used to find the
                 .PDB file name of the PE Image.
  @param ModHandle    - Returned from LoadLibraryEx() and stored for call to
                 FreeLibrary().
  @return   return EFI_SUCCESS when ModHandle was stored.
--*/
EFI_STATUS
AddModHandle (
  IN  PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN  VOID                          *ModHandle
  )

{
  UINTN                   Index;
  PDB_NAME_TO_MOD_HANDLE  *Array;
  UINTN                   PreviousSize;
  PDB_NAME_TO_MOD_HANDLE  *TempArray;
  HANDLE                  Handle;
  UINTN                   Size;

  //
  // Return EFI_ALREADY_STARTED if this DLL has already been loaded
  //
  Array = mPdbNameModHandleArray;
  for (Index = 0; Index < mPdbNameModHandleArraySize; Index++, Array++) {
    if ((Array->PdbPointer != NULL) && (Array->ModHandle == ModHandle)) {
      return EFI_ALREADY_STARTED;
    }
  }

  Array = mPdbNameModHandleArray;
  for (Index = 0; Index < mPdbNameModHandleArraySize; Index++, Array++) {
    if (Array->PdbPointer == NULL) {
      //
      // Make a copy of the string and store the ModHandle
      //
      Handle            = GetProcessHeap ();
      Size              = AsciiStrLen (ImageContext->PdbPointer) + 1;
      Array->PdbPointer = HeapAlloc (Handle, HEAP_ZERO_MEMORY, Size);
      ASSERT (Array->PdbPointer != NULL);

      AsciiStrCpyS (Array->PdbPointer, Size, ImageContext->PdbPointer);
      Array->ModHandle = ModHandle;
      return EFI_SUCCESS;
    }
  }

  //
  // No free space in mPdbNameModHandleArray so grow it by
  // MAX_PDB_NAME_TO_MOD_HANDLE_ARRAY_SIZE entires.
  //
  PreviousSize                = mPdbNameModHandleArraySize * sizeof (PDB_NAME_TO_MOD_HANDLE);
  mPdbNameModHandleArraySize += MAX_PDB_NAME_TO_MOD_HANDLE_ARRAY_SIZE;
  //
  // re-allocate a new buffer and copy the old values to the new locaiton.
  //
  TempArray = HeapAlloc (
                GetProcessHeap (),
                HEAP_ZERO_MEMORY,
                mPdbNameModHandleArraySize * sizeof (PDB_NAME_TO_MOD_HANDLE)
                );

  CopyMem ((VOID *)(UINTN)TempArray, (VOID *)(UINTN)mPdbNameModHandleArray, PreviousSize);

  HeapFree (GetProcessHeap (), 0, mPdbNameModHandleArray);

  mPdbNameModHandleArray = TempArray;
  if (mPdbNameModHandleArray == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  return AddModHandle (ImageContext, ModHandle);
}

/**
  Return the ModHandle and delete the entry in the array.
   @param  ImageContext - Input data returned from PE Laoder Library. Used to find the
                 .PDB file name of the PE Image.
  @return
    ModHandle - ModHandle assoicated with ImageContext is returned
    NULL      - No ModHandle associated with ImageContext
**/
VOID *
RemoveModHandle (
  IN  PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  UINTN                   Index;
  PDB_NAME_TO_MOD_HANDLE  *Array;

  if (ImageContext->PdbPointer == NULL) {
    //
    // If no PDB pointer there is no ModHandle so return NULL
    //
    return NULL;
  }

  Array = mPdbNameModHandleArray;
  for (Index = 0; Index < mPdbNameModHandleArraySize; Index++, Array++) {
    if ((Array->PdbPointer != NULL) && (AsciiStrCmp (Array->PdbPointer, ImageContext->PdbPointer) == 0)) {
      //
      // If you find a match return it and delete the entry
      //
      HeapFree (GetProcessHeap (), 0, Array->PdbPointer);
      Array->PdbPointer = NULL;
      return Array->ModHandle;
    }
  }

  return NULL;
}

typedef struct {
  UINTN     Base;
  UINT32    Size;
  UINT32    Flags;
} IMAGE_SECTION_DATA;

VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  EFI_STATUS                           Status;
  VOID                                 *DllEntryPoint;
  CHAR16                               *DllFileName;
  HMODULE                              Library;
  UINTN                                Index;
  PE_COFF_LOADER_IMAGE_CONTEXT         PeCoffImageContext;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  EFI_IMAGE_SECTION_HEADER             *FirstSection;
  EFI_IMAGE_SECTION_HEADER             *Section;
  IMAGE_SECTION_DATA                   *SectionData;
  UINTN                                NumberOfSections;
  UINTN                                Base;
  UINTN                                End;
  UINTN                                RegionBase;
  UINTN                                RegionSize;
  UINT32                               Flags;
  DWORD                                NewProtection;
  DWORD                                OldProtection;

  ASSERT (ImageContext != NULL);
  //
  // If we load our own PE/COFF images the Windows debugger can not source
  // level debug our code. If a valid PDB pointer exists use it to load
  // the *.dll file as a library using Windows* APIs. This allows
  // source level debug. The image is still loaded and relocated
  // in the Framework memory space like on a real system (by the code above),
  // but the entry point points into the DLL loaded by the code below.
  //

  DllEntryPoint = NULL;

  //
  // Load the DLL if it's not an EBC image.
  //
  if ((ImageContext->PdbPointer != NULL) &&
      (ImageContext->Machine != EFI_IMAGE_MACHINE_EBC))
  {
    //
    // Convert filename from ASCII to Unicode
    //
    DllFileName = AsciiToUnicode (ImageContext->PdbPointer, &Index);

    //
    // Check that we have a valid filename
    //
    if ((Index < 5) || (DllFileName[Index - 4] != '.')) {
      free (DllFileName);

      //
      // Never return an error if PeCoffLoaderRelocateImage() succeeded.
      // The image will run, but we just can't source level debug. If we
      // return an error the image will not run.
      //
      return;
    }

    //
    // Replace .PDB with .DLL in the filename
    //
    DllFileName[Index - 3] = 'D';
    DllFileName[Index - 2] = 'L';
    DllFileName[Index - 1] = 'L';

    //
    // Load the .DLL file into the process's address space for source level
    // debug.
    //
    // EFI modules use the PE32 entry point for a different purpose than
    // Windows. For Windows DLLs, the PE entry point is used for the DllMain()
    // function. DllMain() has a very specific purpose; it initializes runtime
    // libraries, instance data, and thread local storage. LoadLibrary()/
    // LoadLibraryEx() will run the PE32 entry point and assume it to be a
    // DllMain() implementation by default. By passing the
    // DONT_RESOLVE_DLL_REFERENCES argument to LoadLibraryEx(), the execution
    // of the entry point as a DllMain() function will be suppressed. This
    // also prevents other modules that are referenced by the DLL from being
    // loaded. We use LoadLibraryEx() to create a copy of the PE32
    // image that the OS (and therefore the debugger) is aware of.
    // Source level debugging is the only reason to do this.
    //
    Library = LoadLibraryEx (DllFileName, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (Library != NULL) {
      //
      // Parse the PE32 image loaded by the OS and find the entry point
      //
      ZeroMem (&PeCoffImageContext, sizeof (PeCoffImageContext));
      PeCoffImageContext.Handle    = Library;
      PeCoffImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;
      Status                       = PeCoffLoaderGetImageInfo (&PeCoffImageContext);
      if (EFI_ERROR (Status) || (PeCoffImageContext.ImageError != IMAGE_ERROR_SUCCESS)) {
        SecPrint ("DLL is not a valid PE/COFF image.\n\r");
        FreeLibrary (Library);
        Library = NULL;
      } else {
        Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)Library + (UINTN)PeCoffImageContext.PeCoffHeaderOffset);
        if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
          //
          // Use PE32 offset
          //
          DllEntryPoint = (VOID *)((UINTN)Library + (UINTN)Hdr.Pe32->OptionalHeader.AddressOfEntryPoint);
        } else {
          //
          // Use PE32+ offset
          //
          DllEntryPoint = (VOID *)((UINTN)Library + (UINTN)Hdr.Pe32Plus->OptionalHeader.AddressOfEntryPoint);
        }

        //
        // Now we need to configure memory access for the copy of the PE32 image
        // loaded by the OS.
        //
        // Most Windows DLLs are linked with sections 4KB aligned but EFI
        // modules are not to reduce size. Because of this we need to compute
        // the union of memory access attributes and explicitly configure
        // each page.
        //
        FirstSection = (EFI_IMAGE_SECTION_HEADER *)(
                                                    (UINTN)Library +
                                                    PeCoffImageContext.PeCoffHeaderOffset +
                                                    sizeof (UINT32) +
                                                    sizeof (EFI_IMAGE_FILE_HEADER) +
                                                    Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                                                    );
        NumberOfSections = (UINTN)(Hdr.Pe32->FileHeader.NumberOfSections);
        Section          = FirstSection;
        SectionData      = malloc (NumberOfSections * sizeof (IMAGE_SECTION_DATA));
        if (SectionData == NULL) {
          FreeLibrary (Library);
          Library       = NULL;
          DllEntryPoint = NULL;
        }

        ZeroMem (SectionData, NumberOfSections * sizeof (IMAGE_SECTION_DATA));
        //
        // Extract the section data from the PE32 image
        //
        for (Index = 0; Index < NumberOfSections; Index++) {
          SectionData[Index].Base = (UINTN)Library + Section->VirtualAddress;
          SectionData[Index].Size = Section->Misc.VirtualSize;
          if (SectionData[Index].Size == 0) {
            SectionData[Index].Size = Section->SizeOfRawData;
          }

          SectionData[Index].Flags = (Section->Characteristics &
                                      (EFI_IMAGE_SCN_MEM_EXECUTE | EFI_IMAGE_SCN_MEM_WRITE));
          Section += 1;
        }

        //
        // Loop over every byte in memory and compute the union of the memory
        // access bits.
        //
        End        = (UINTN)Library + (UINTN)PeCoffImageContext.ImageSize;
        RegionBase = (UINTN)Library;
        RegionSize = 0;
        Flags      = 0;
        for (Base = (UINTN)Library; Base < End; Base++) {
          for (Index = 0; Index < NumberOfSections; Index++) {
            if ((SectionData[Index].Base <= Base) &&
                ((SectionData[Index].Base + SectionData[Index].Size) > Base))
            {
              Flags |= SectionData[Index].Flags;
            }
          }

          //
          // When end of current page is reached configure the memory access for
          // the current page.
          //
          if (IS_ALIGNED (Base + 1, SIZE_4KB)) {
            RegionSize += SIZE_4KB;
            if ((Flags & EFI_IMAGE_SCN_MEM_WRITE) == EFI_IMAGE_SCN_MEM_WRITE) {
              if ((Flags & EFI_IMAGE_SCN_MEM_EXECUTE) == EFI_IMAGE_SCN_MEM_EXECUTE) {
                NewProtection = PAGE_EXECUTE_READWRITE;
              } else {
                NewProtection = PAGE_READWRITE;
              }
            } else {
              if ((Flags & EFI_IMAGE_SCN_MEM_EXECUTE) == EFI_IMAGE_SCN_MEM_EXECUTE) {
                NewProtection = PAGE_EXECUTE_READ;
              } else {
                NewProtection = PAGE_READONLY;
              }
            }

            if (!VirtualProtect ((LPVOID)RegionBase, (SIZE_T)RegionSize, NewProtection, &OldProtection)) {
              SecPrint ("Setting PE32 Section Access Failed\n\r");
              FreeLibrary (Library);
              free (SectionData);
              Library       = NULL;
              DllEntryPoint = NULL;
              break;
            }

            Flags      = 0;
            RegionBase = Base + 1;
            RegionSize = 0;
          }
        }

        free (SectionData);
        //
        // Configure the last partial page
        //
        if ((Library != NULL) && ((End - RegionBase) > 0)) {
          if ((Flags & EFI_IMAGE_SCN_MEM_WRITE) == EFI_IMAGE_SCN_MEM_WRITE) {
            if ((Flags & EFI_IMAGE_SCN_MEM_EXECUTE) == EFI_IMAGE_SCN_MEM_EXECUTE) {
              NewProtection = PAGE_EXECUTE_READWRITE;
            } else {
              NewProtection = PAGE_READWRITE;
            }
          } else {
            if ((Flags & EFI_IMAGE_SCN_MEM_EXECUTE) == EFI_IMAGE_SCN_MEM_EXECUTE) {
              NewProtection = PAGE_EXECUTE_READ;
            } else {
              NewProtection = PAGE_READONLY;
            }
          }

          if (!VirtualProtect ((LPVOID)RegionBase, (SIZE_T)(End - RegionBase), NewProtection, &OldProtection)) {
            SecPrint ("Setting PE32 Section Access Failed\n\r");
            FreeLibrary (Library);
            Library       = NULL;
            DllEntryPoint = NULL;
          }
        }
      }
    }

    if ((Library != NULL) && (DllEntryPoint != NULL)) {
      Status = AddModHandle (ImageContext, Library);
      if ((Status == EFI_SUCCESS) || (Status == EFI_ALREADY_STARTED)) {
        //
        // This DLL is either not loaded or already started, so source level debugging is supported.
        //
        ImageContext->EntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)DllEntryPoint;
        SecPrint ("LoadLibraryEx (\n\r  %S,\n\r  NULL, DONT_RESOLVE_DLL_REFERENCES) @ 0x%X\n\r", DllFileName, (int)(UINTN)Library);
      }
    } else {
      SecPrint ("WARNING: No source level debug %S. \n\r", DllFileName);
    }

    free (DllFileName);
  }
}

VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  VOID  *ModHandle;

  ASSERT (ImageContext != NULL);

  ModHandle = RemoveModHandle (ImageContext);
  if (ModHandle != NULL) {
    FreeLibrary (ModHandle);
    SecPrint ("FreeLibrary (\n\r  %s)\n\r", ImageContext->PdbPointer);
  } else {
    SecPrint ("WARNING: Unload image without source level debug\n\r");
  }
}

VOID
_ModuleEntryPoint (
  VOID
  )
{
}

/*++

Copyright (c) 2006 - 2007 Intel Corporation.
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  SecMain.c

Abstract:
  WinNt emulator of SEC phase. It's really a Posix application, but this is
  Ok since all the other modules for NT32 are NOT Posix applications.

  This program processes host environment variables and figures out
  what the memory layout will be, how may FD's will be loaded and also
  what the boot mode is.

  The SEC registers a set of services with the SEC core. gPrivateDispatchTable
  is a list of PPI's produced by the SEC that are availble for usage in PEI.

  This code produces 128 K of temporary memory for the PEI stack by opening a
  host file and mapping it directly to memory addresses.

  The system.cmd script is used to set host environment variables that drive
  the configuration opitons of the SEC.

--*/

#include "SecMain.h"
#include <sys/mman.h>
#include <Ppi/UnixPeiLoadFile.h>
#include <Framework/StatusCode.h>
#include <Ppi/TemporaryRamSupport.h>
#include <dlfcn.h>
//
// Globals
//
EFI_PEI_PE_COFF_LOADER_PROTOCOL_INSTANCE  mPeiEfiPeiPeCoffLoaderInstance = {
  {
    SecNt32PeCoffGetImageInfo,
    SecNt32PeCoffLoadImage,
    SecNt32PeCoffRelocateImage,
    SecNt32PeCoffUnloadimage
  },
  NULL
};



EFI_PEI_PE_COFF_LOADER_PROTOCOL           *gPeiEfiPeiPeCoffLoader = &mPeiEfiPeiPeCoffLoaderInstance.PeCoff;

UNIX_PEI_LOAD_FILE_PPI                      mSecNtLoadFilePpi     = { SecWinNtPeiLoadFile };

PEI_UNIX_AUTOSCAN_PPI                       mSecNtAutoScanPpi     = { SecWinNtPeiAutoScan };

PEI_UNIX_THUNK_PPI                          mSecWinNtThunkPpi     = { SecWinNtWinNtThunkAddress };

EFI_PEI_PROGRESS_CODE_PPI                 mSecStatusCodePpi     = { SecPeiReportStatusCode };

UNIX_FWH_PPI                                mSecFwhInformationPpi = { SecWinNtFdAddress };

TEMPORARY_RAM_SUPPORT_PPI                 mSecTemporaryRamSupportPpi = {SecTemporaryRamSupport};

EFI_PEI_PPI_DESCRIPTOR  gPrivateDispatchTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiPeiPeCoffLoaderGuid,
    NULL
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gUnixPeiLoadFilePpiGuid,
    &mSecNtLoadFilePpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gPeiUnixAutoScanPpiGuid,
    &mSecNtAutoScanPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gPeiUnixThunkPpiGuid,
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
    &gUnixFwhPpiGuid,
    &mSecFwhInformationPpi
  }
};


//
// Default information about where the FD is located.
//  This array gets filled in with information from EFI_FIRMWARE_VOLUMES
//  EFI_FIRMWARE_VOLUMES is a host environment variable set by system.cmd.
//  The number of array elements is allocated base on parsing
//  EFI_FIRMWARE_VOLUMES and the memory is never freed.
//
UINTN                                     gFdInfoCount = 0;
UNIX_FD_INFO                                *gFdInfo;

//
// Array that supports seperate memory rantes.
//  The memory ranges are set in system.cmd via the EFI_MEMORY_SIZE variable.
//  The number of array elements is allocated base on parsing
//  EFI_MEMORY_SIZE and the memory is never freed.
//
UINTN                                     gSystemMemoryCount = 0;
UNIX_SYSTEM_MEMORY                       *gSystemMemory;

VOID
EFIAPI
SecSwitchStack (
  UINT32   TemporaryMemoryBase,
  UINT32   PermenentMemoryBase
  );

STATIC
EFI_PHYSICAL_ADDRESS *
MapMemory (
  INTN fd,
  UINT64 length,
  INTN   prot,
  INTN   flags);

STATIC
EFI_STATUS
MapFile (
  IN  CHAR8                     *FileName,
  IN OUT  EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                    *Length
  );


INTN
EFIAPI
main (
  IN  INTN  Argc,
  IN  CHAR8 **Argv,
  IN  CHAR8 **Envp
  )
/*++

Routine Description:
  Main entry point to SEC for WinNt. This is a unix program

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
  EFI_PHYSICAL_ADDRESS  InitialStackMemory;
  UINT64                InitialStackMemorySize;
  UINTN                 Index;
  UINTN                 Index1;
  UINTN                 Index2;
  UINTN                 PeiIndex;
  CHAR8                 *FileName;
  BOOLEAN               Done;
  VOID                  *PeiCoreFile;
  CHAR16                *MemorySizeStr;
  CHAR16                *FirmwareVolumesStr;
  UINTN                 *StackPointer;

  setbuf(stdout, 0);
  setbuf(stderr, 0);

  MemorySizeStr      = (CHAR16 *) FixedPcdGetPtr (PcdUnixMemorySizeForSecMain);
  FirmwareVolumesStr = (CHAR16 *) FixedPcdGetPtr (PcdUnixFirmwareVolume);

  printf ("\nEDK SEC Main UNIX Emulation Environment from www.TianoCore.org\n");

  //
  // Allocate space for gSystemMemory Array
  //
  gSystemMemoryCount  = CountSeperatorsInString (MemorySizeStr, '!') + 1;
  gSystemMemory       = calloc (gSystemMemoryCount, sizeof (UNIX_SYSTEM_MEMORY));
  if (gSystemMemory == NULL) {
    printf ("ERROR : Can not allocate memory for system.  Exiting.\n");
    exit (1);
  }
  //
  // Allocate space for gSystemMemory Array
  //
  gFdInfoCount  = CountSeperatorsInString (FirmwareVolumesStr, '!') + 1;
  gFdInfo       = calloc (gFdInfoCount, sizeof (UNIX_FD_INFO));
  if (gFdInfo == NULL) {
    printf ("ERROR : Can not allocate memory for fd info.  Exiting.\n");
    exit (1);
  }
  //
  // Setup Boot Mode. If BootModeStr == "" then BootMode = 0 (BOOT_WITH_FULL_CONFIGURATION)
  //
  printf ("  BootMode 0x%02x\n", FixedPcdGet32 (PcdUnixBootMode));

  //
  // Open up a 128K file to emulate temp memory for PEI.
  //  on a real platform this would be SRAM, or using the cache as RAM.
  //  Set InitialStackMemory to zero so WinNtOpenFile will allocate a new mapping
  //
  InitialStackMemorySize  = STACK_SIZE;
  InitialStackMemory = (UINTN)MapMemory(0, 
          (UINT32) InitialStackMemorySize,
          PROT_READ | PROT_WRITE,
          MAP_ANONYMOUS | MAP_PRIVATE);
  if (InitialStackMemory == 0) {
    printf ("ERROR : Can not open SecStack Exiting\n");
    exit (1);
  }

  printf ("  SEC passing in %u KB of temp RAM at 0x%08lx to PEI\n",
    (UINTN)(InitialStackMemorySize / 1024),
    (unsigned long)InitialStackMemory);
    
  for (StackPointer = (UINTN*) (UINTN) InitialStackMemory;
     StackPointer < (UINTN*) ((UINTN) InitialStackMemory + (UINT64) InitialStackMemorySize);
     StackPointer ++) {
    *StackPointer = 0x5AA55AA5;
  }

  //
  // Open All the firmware volumes and remember the info in the gFdInfo global
  //
  FileName = (CHAR8 *)malloc (StrLen (FirmwareVolumesStr) + 1);
  if (FileName == NULL) {
    printf ("ERROR : Can not allocate memory for firmware volume string\n");
    exit (1);
  }

  Index2 = 0;
  for (Done = FALSE, Index = 0, PeiIndex = 0, PeiCoreFile = NULL;
       FirmwareVolumesStr[Index2] != 0;
       Index++) {
    for (Index1 = 0; (FirmwareVolumesStr[Index2] != '!') && (FirmwareVolumesStr[Index2] != 0); Index2++)
      FileName[Index1++] = FirmwareVolumesStr[Index2];
    if (FirmwareVolumesStr[Index2] == '!')
      Index2++;
    FileName[Index1]  = '\0';

    //
    // Open the FD and remmeber where it got mapped into our processes address space
    //
    Status = MapFile (
		      FileName,
		      &gFdInfo[Index].Address,
		      &gFdInfo[Index].Size
		      );
    if (EFI_ERROR (Status)) {
      printf ("ERROR : Can not open Firmware Device File %s (%x).  Exiting.\n", FileName, Status);
      exit (1);
    }

    printf ("  FD loaded from %s at 0x%08lx",
	    FileName, (unsigned long)gFdInfo[Index].Address);

    if (PeiCoreFile == NULL) {
      //
      // Assume the beginning of the FD is an FV and look for the PEI Core.
      // Load the first one we find.
      //
      Status = SecFfsFindPeiCore ((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) gFdInfo[Index].Address, &PeiCoreFile);
      if (!EFI_ERROR (Status)) {
        PeiIndex = Index;
        printf (" contains SEC Core");
      }
    }

    printf ("\n");
  }
  //
  // Calculate memory regions and store the information in the gSystemMemory
  //  global for later use. The autosizing code will use this data to
  //  map this memory into the SEC process memory space.
  //
  Index1 = 0;
  Index = 0;
  while (1) {
    UINTN val = 0;
    //
    // Save the size of the memory.
    //
    while (MemorySizeStr[Index1] >= '0' && MemorySizeStr[Index1] <= '9') {
      val = val * 10 + MemorySizeStr[Index1] - '0';
      Index1++;
    }
    gSystemMemory[Index++].Size = val * 0x100000;
    if (MemorySizeStr[Index1] == 0)
      break;
    Index1++;
  }

  printf ("\n");

  //
  // Hand off to PEI Core
  //
  SecLoadFromCore ((UINTN) InitialStackMemory, (UINTN) InitialStackMemorySize, (UINTN) gFdInfo[0].Address, PeiCoreFile);

  //
  // If we get here, then the PEI Core returned. This is an error as PEI should
  //  always hand off to DXE.
  //
  printf ("ERROR : PEI Core returned\n");
  exit (1);
}

EFI_PHYSICAL_ADDRESS *
MapMemory (
  INTN fd,
  UINT64 length,
  INTN   prot,
  INTN   flags)
{
  STATIC UINTN base  = 0x40000000;
  CONST UINTN  align = (1 << 24);
  VOID         *res  = NULL;
  BOOLEAN      isAligned = 0;

  //
  // Try to get an aligned block somewhere in the address space of this
  // process.
  //
  while((!isAligned) && (base != 0)) {
    res = mmap ((void *)base, length, prot, flags, fd, 0);
    if (res == MAP_FAILED) {
      return NULL;
    }
    if ((((UINTN)res) & ~(align-1)) == (UINTN)res) {
      isAligned=1;
    }
    else {
      munmap(res, length);
      base += align;
    }
  }
  return res;
}

EFI_STATUS
MapFile (
  IN  CHAR8                     *FileName,
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
  int fd;
  VOID    *res;
  UINTN   FileSize;

  fd = open (FileName, O_RDONLY);
  if (fd < 0)
    return EFI_NOT_FOUND;
  FileSize = lseek (fd, 0, SEEK_END);

#if 0
  if (IsMain)
    {
      /* Read entry address.  */
      lseek (fd, FileSize - 0x20, SEEK_SET);
      if (read (fd, &EntryAddress, 4) != 4)
	{
	  close (fd);
	  return EFI_DEVICE_ERROR;
	}      
    }
#endif

  res = MapMemory(fd, FileSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE);
  
  close (fd);

  if (res == MAP_FAILED)
    return EFI_DEVICE_ERROR;

  *Length = (UINT64) FileSize;
  *BaseAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) res;

  return EFI_SUCCESS;
}

#define BYTES_PER_RECORD  512

EFI_STATUS
EFIAPI
SecPeiReportStatusCode (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_STATUS_CODE_TYPE       CodeType,
  IN EFI_STATUS_CODE_VALUE      Value,
  IN UINT32                     Instance,
  IN EFI_GUID                   * CallerId,
  IN EFI_STATUS_CODE_DATA       * Data OPTIONAL
  )
/*++

Routine Description:

  This routine produces the ReportStatusCode PEI service. It's passed
  up to the PEI Core via a PPI. T

  This code currently uses the UNIX clib printf. This does not work the same way
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
  VA_LIST         Marker;
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
    printf ("ASSERT %s(%d): %s\n", Filename, LineNumber, Description);

  } else if (ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {
    //
    // Process DEBUG () macro 
    //
    AsciiVSPrint (PrintBuffer, BYTES_PER_RECORD, Format, Marker);
    printf (PrintBuffer);
  }

  return EFI_SUCCESS;
}

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
  EFI_PHYSICAL_ADDRESS        TopOfMemory;
  VOID                        *TopOfStack;
  UINT64                      PeiCoreSize;
  EFI_PHYSICAL_ADDRESS        PeiCoreEntryPoint;
  EFI_PHYSICAL_ADDRESS        PeiImageAddress;
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  UINTN                       PeiStackSize;

  //
  // Compute Top Of Memory for Stack and PEI Core Allocations
  //
  TopOfMemory  = LargestRegion + LargestRegionSize;
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
  TopOfMemory = LargestRegion + PeiStackSize;

  //
  // Reservet space for storing PeiCore's parament in stack.
  // 
  TopOfStack  = (VOID *)((UINTN)TopOfStack - sizeof (EFI_SEC_PEI_HAND_OFF) - CPU_STACK_ALIGNMENT);
  TopOfStack  = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // Patch value in dispatch table values
  //
  gPrivateDispatchTable[0].Ppi = gPeiEfiPeiPeCoffLoader;

  //
  // Bind this information into the SEC hand-off state
  //
  SecCoreData                        = (EFI_SEC_PEI_HAND_OFF*)(UINTN) TopOfStack;
  SecCoreData->DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
  SecCoreData->BootFirmwareVolumeBase = (VOID*)BootFirmwareVolumeBase;
  SecCoreData->BootFirmwareVolumeSize = FixedPcdGet32(PcdUnixFirmwareFdSize);
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
  parsing the host environment variable EFI_MEMORY_SIZE.
  The size comes from the varaible and the address comes from the call to
  WinNtOpenFile.

Arguments:
  Index      - Which memory region to use
  MemoryBase - Return Base address of memory region
  MemorySize - Return size in bytes of the memory region

Returns:
  EFI_SUCCESS - If memory region was mapped
  EFI_UNSUPPORTED - If Index is not supported

--*/
{
  void *res;

  if (Index >= gSystemMemoryCount) {
    return EFI_UNSUPPORTED;
  }

  *MemoryBase = 0;
  res = MapMemory(0, gSystemMemory[Index].Size,
		  PROT_READ | PROT_WRITE | PROT_EXEC,
		  MAP_PRIVATE | MAP_ANONYMOUS);
  if (res == MAP_FAILED)
    return EFI_DEVICE_ERROR;
  *MemorySize = gSystemMemory[Index].Size;
  *MemoryBase = (UINTN)res;
  gSystemMemory[Index].Memory = *MemoryBase;

  return EFI_SUCCESS;
}

VOID *
EFIAPI
SecWinNtWinNtThunkAddress (
  VOID
  )
/*++

Routine Description:
  Since the SEC is the only Unix program in stack it must export
  an interface to do Win API calls. That's what the WinNtThunk address
  is for. gWinNt is initailized in WinNtThunk.c.

Arguments:
  InterfaceSize - sizeof (EFI_WIN_NT_THUNK_PROTOCOL);
  InterfaceBase - Address of the gWinNt global

Returns:
  EFI_SUCCESS - Data returned

--*/
{
  return gUnix;
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

  Status                  = gPeiEfiPeiPeCoffLoader->GetImageInfo (gPeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate space in UNIX (not emulator) memory. Extra space is for alignment
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) malloc ((UINTN) (ImageContext.ImageSize + (ImageContext.SectionAlignment * 2)));
  if (ImageContext.ImageAddress == 0) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Align buffer on section boundry
  //
  ImageContext.ImageAddress += ImageContext.SectionAlignment;
  ImageContext.ImageAddress &= ~(ImageContext.SectionAlignment - 1);


  Status = gPeiEfiPeiPeCoffLoader->LoadImage (gPeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gPeiEfiPeiPeCoffLoader->RelocateImage (gPeiEfiPeiPeCoffLoader, &ImageContext);
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
  file into host memory only the SEC will know it's address.

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

UINTN
CountSeperatorsInString (
  IN  const CHAR16   *String,
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
EFIAPI
SecNt32PeCoffGetImageInfo (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  EFI_STATUS  Status;

  Status = PeCoffLoaderGetImageInfo (ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (ImageContext->ImageType) {

  case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
    ImageContext->ImageCodeMemoryType = EfiLoaderCode;
    ImageContext->ImageDataMemoryType = EfiLoaderData;
    break;

  case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
    ImageContext->ImageCodeMemoryType = EfiBootServicesCode;
    ImageContext->ImageDataMemoryType = EfiBootServicesData;
    break;

  case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
  case EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER:
    ImageContext->ImageCodeMemoryType = EfiRuntimeServicesCode;
    ImageContext->ImageDataMemoryType = EfiRuntimeServicesData;
    break;

  default:
    ImageContext->ImageError = IMAGE_ERROR_INVALID_SUBSYSTEM;
    return RETURN_UNSUPPORTED;
  }

  return Status;
}

EFI_STATUS
EFIAPI
SecNt32PeCoffLoadImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  EFI_STATUS  Status;

  Status = PeCoffLoaderLoadImage (ImageContext);
  return Status;
}

VOID
SecUnixLoaderBreak (
  VOID
  )
{
}

EFI_STATUS
EFIAPI
SecNt32PeCoffRelocateImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  void * Handle;
  void * Entry;
  EFI_STATUS Status;

  Handle = NULL;
  Entry  = NULL;
  Status = PeCoffLoaderRelocateImage (ImageContext);
  fprintf (stderr, 
     "Loading %s 0x%08lx - entry point 0x%08lx\n",
     ImageContext->PdbPointer,
     (unsigned long)ImageContext->ImageAddress,
     (unsigned long)ImageContext->EntryPoint);

  SecUnixLoaderBreak ();

  return Status;
}


EFI_STATUS
EFIAPI
SecNt32PeCoffUnloadimage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL      *This,
  IN PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  return EFI_SUCCESS;
}

VOID
ModuleEntryPoint (
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
  // Simulate to invalid CAR, terminate CAR
  // 
  //ZeroMem ((VOID*)(UINTN)TemporaryMemoryBase, CopySize);
  
  return EFI_SUCCESS;
}

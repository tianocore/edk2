/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiPei.h>
#include <Pi/PiBootMode.h>

#include <Library/PeCoffLib.h>
#include <Library/PrePiLib.h>
#include <Library/PrintLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/CacheMaintenanceLib.h>

#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/ArmMpCoreInfo.h>

#include "PrePi.h"

VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

VOID
PrePiMain (
  IN  UINTN   UefiMemoryBase,
  IN  UINTN   StacksBase,
  IN  UINT64  StartTimeStamp
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  EFI_STATUS                  Status;
  CHAR8                       Buffer[100];
  UINTN                       CharCount;
  UINTN                       StacksSize;

  // Initialize the architecture specific bits
  ArchInitialize ();

  // Declare the PI/UEFI memory region
  HobList = HobConstructor (
              (VOID *)UefiMemoryBase,
              FixedPcdGet32 (PcdSystemMemoryUefiRegionSize),
              (VOID *)UefiMemoryBase,
              (VOID *)StacksBase // The top of the UEFI Memory is reserved for the stacks
              );
  PrePeiSetHobList (HobList);

  //
  // Ensure that the loaded image is invalidated in the caches, so that any
  // modifications we made with the caches and MMU off (such as the applied
  // relocations) don't become invisible once we turn them on.
  //
  InvalidateDataCacheRange ((VOID *)(UINTN)PcdGet64 (PcdFdBaseAddress), PcdGet32 (PcdFdSize));

  // Initialize MMU and Memory HOBs (Resource Descriptor HOBs)
  Status = MemoryPeim (UefiMemoryBase, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize));
  ASSERT_EFI_ERROR (Status);

  // Initialize the Serial Port
  SerialPortInitialize ();
  CharCount = AsciiSPrint (
                Buffer,
                sizeof (Buffer),
                "UEFI firmware (version %s built at %a on %a)\n\r",
                (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString),
                __TIME__,
                __DATE__
                );
  SerialPortWrite ((UINT8 *)Buffer, CharCount);

  // Create the Stacks HOB (reserve the memory for all stacks)
  StacksSize = PcdGet32 (PcdCPUCorePrimaryStackSize);
  BuildStackHob (StacksBase, StacksSize);

  // TODO: Call CpuPei as a library
  BuildCpuHob (ArmGetPhysicalAddressBits (), PcdGet8 (PcdPrePiCpuIoSize));

  // Set the Boot Mode
  SetBootMode (BOOT_WITH_FULL_CONFIGURATION);

  // Initialize Platform HOBs (CpuHob and FvHob)
  Status = PlatformPeim ();
  ASSERT_EFI_ERROR (Status);

  // Now, the HOB List has been initialized, we can register performance information
  PERF_START (NULL, "PEI", NULL, StartTimeStamp);

  // SEC phase needs to run library constructors by hand.
  ProcessLibraryConstructorList ();

  // Assume the FV that contains the SEC (our code) also contains a compressed FV.
  Status = DecompressFirstFv ();
  ASSERT_EFI_ERROR (Status);

  // Load the DXE Core and transfer control to it
  Status = LoadDxeCoreFromFv (NULL, 0);
  ASSERT_EFI_ERROR (Status);
}

VOID
CEntryPoint (
  IN  UINTN  MpId,
  IN  UINTN  UefiMemoryBase,
  IN  UINTN  StacksBase
  )
{
  UINT64  StartTimeStamp;

  if (PerformanceMeasurementEnabled ()) {
    // Initialize the Timer Library to setup the Timer HW controller
    TimerConstructor ();
    // We cannot call yet the PerformanceLib because the HOB List has not been initialized
    StartTimeStamp = GetPerformanceCounter ();
  } else {
    StartTimeStamp = 0;
  }

  // Data Cache enabled on Primary core when MMU is enabled.
  ArmDisableDataCache ();
  // Invalidate instruction cache
  ArmInvalidateInstructionCache ();
  // Enable Instruction Caches on all cores.
  ArmEnableInstructionCache ();

  PrePiMain (UefiMemoryBase, StacksBase, StartTimeStamp);

  // DXE Core should always load and never return
  ASSERT (FALSE);
}

VOID
RelocatePeCoffImage (
  IN  EFI_PEI_FV_HANDLE         FwVolHeader,
  IN  PE_COFF_LOADER_READ_FILE  ImageRead
  )
{
  EFI_PEI_FILE_HANDLE           FileHandle;
  VOID                          *SectionData;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  EFI_STATUS                    Status;

  FileHandle = NULL;
  Status     = FfsFindNextFile (
                 EFI_FV_FILETYPE_SECURITY_CORE,
                 FwVolHeader,
                 &FileHandle
                 );
  ASSERT_EFI_ERROR (Status);

  Status = FfsFindSectionData (EFI_SECTION_PE32, FileHandle, &SectionData);
  if (EFI_ERROR (Status)) {
    Status = FfsFindSectionData (EFI_SECTION_TE, FileHandle, &SectionData);
  }

  ASSERT_EFI_ERROR (Status);

  ZeroMem (&ImageContext, sizeof ImageContext);

  ImageContext.Handle    = (EFI_HANDLE)SectionData;
  ImageContext.ImageRead = ImageRead;
  PeCoffLoaderGetImageInfo (&ImageContext);

  if (ImageContext.ImageAddress != (UINTN)SectionData) {
    ImageContext.ImageAddress = (UINTN)SectionData;
    PeCoffLoaderRelocateImage (&ImageContext);
  }
}

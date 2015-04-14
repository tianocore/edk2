/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiPei.h>

#include <Library/PrePiLib.h>
#include <Library/PrintLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>
#include <Library/CacheMaintenanceLib.h>

#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <Guid/LzmaDecompress.h>
#include <Guid/ArmGlobalVariableHob.h>

#include "PrePi.h"
#include "LzmaDecompress.h"

// Not used when PrePi in run in XIP mode
UINTN mGlobalVariableBase = 0;

EFI_STATUS
EFIAPI
ExtractGuidedSectionLibConstructor (
  VOID
  );

EFI_STATUS
EFIAPI
LzmaDecompressLibConstructor (
  VOID
  );

VOID
EFIAPI
BuildGlobalVariableHob (
  IN EFI_PHYSICAL_ADDRESS         GlobalVariableBase,
  IN UINT32                       GlobalVariableSize
  )
{
  ARM_HOB_GLOBAL_VARIABLE  *Hob;

  Hob = CreateHob (EFI_HOB_TYPE_GUID_EXTENSION, sizeof (ARM_HOB_GLOBAL_VARIABLE));
  ASSERT(Hob != NULL);

  CopyGuid (&(Hob->Header.Name), &gArmGlobalVariableGuid);
  Hob->GlobalVariableBase = GlobalVariableBase;
  Hob->GlobalVariableSize = GlobalVariableSize;
}

EFI_STATUS
GetPlatformPpi (
  IN  EFI_GUID  *PpiGuid,
  OUT VOID      **Ppi
  )
{
  UINTN                   PpiListSize;
  UINTN                   PpiListCount;
  EFI_PEI_PPI_DESCRIPTOR  *PpiList;
  UINTN                   Index;

  PpiListSize = 0;
  ArmPlatformGetPlatformPpiList (&PpiListSize, &PpiList);
  PpiListCount = PpiListSize / sizeof(EFI_PEI_PPI_DESCRIPTOR);
  for (Index = 0; Index < PpiListCount; Index++, PpiList++) {
    if (CompareGuid (PpiList->Guid, PpiGuid) == TRUE) {
      *Ppi = PpiList->Ppi;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

VOID
PrePiMain (
  IN  UINTN                     UefiMemoryBase,
  IN  UINTN                     StacksBase,
  IN  UINTN                     GlobalVariableBase,
  IN  UINT64                    StartTimeStamp
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE*   HobList;
  EFI_STATUS                    Status;
  CHAR8                         Buffer[100];
  UINTN                         CharCount;
  UINTN                         StacksSize;

  // Initialize the architecture specific bits
  ArchInitialize ();

  // Declare the PI/UEFI memory region
  HobList = HobConstructor (
    (VOID*)UefiMemoryBase,
    FixedPcdGet32 (PcdSystemMemoryUefiRegionSize),
    (VOID*)UefiMemoryBase,
    (VOID*)StacksBase  // The top of the UEFI Memory is reserved for the stacks
    );
  PrePeiSetHobList (HobList);

  //
  // Ensure that the loaded image is invalidated in the caches, so that any
  // modifications we made with the caches and MMU off (such as the applied
  // relocations) don't become invisible once we turn them on.
  //
  InvalidateDataCacheRange((VOID *)(UINTN)PcdGet64 (PcdFdBaseAddress), PcdGet32 (PcdFdSize));

  // Initialize MMU and Memory HOBs (Resource Descriptor HOBs)
  Status = MemoryPeim (UefiMemoryBase, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize));
  ASSERT_EFI_ERROR (Status);

  // Initialize the Serial Port
  SerialPortInitialize ();
  CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"UEFI firmware (version %s built at %a on %a)\n\r",
    (CHAR16*)PcdGetPtr(PcdFirmwareVersionString), __TIME__, __DATE__);
  SerialPortWrite ((UINT8 *) Buffer, CharCount);

  // Create the Stacks HOB (reserve the memory for all stacks)
  StacksSize = PcdGet32 (PcdCPUCorePrimaryStackSize);
  BuildStackHob (StacksBase, StacksSize);

  // Declare the Global Variable HOB
  BuildGlobalVariableHob (GlobalVariableBase, FixedPcdGet32 (PcdPeiGlobalVariableSize));

  //TODO: Call CpuPei as a library
  BuildCpuHob (PcdGet8 (PcdPrePiCpuMemorySize), PcdGet8 (PcdPrePiCpuIoSize));

  // Set the Boot Mode
  SetBootMode (ArmPlatformGetBootMode ());

  // Initialize Platform HOBs (CpuHob and FvHob)
  Status = PlatformPeim ();
  ASSERT_EFI_ERROR (Status);

  // Now, the HOB List has been initialized, we can register performance information
  PERF_START (NULL, "PEI", NULL, StartTimeStamp);

  // SEC phase needs to run library constructors by hand.
  ExtractGuidedSectionLibConstructor ();
  LzmaDecompressLibConstructor ();

  // Build HOBs to pass up our version of stuff the DXE Core needs to save space
  BuildPeCoffLoaderHob ();
  BuildExtractSectionHob (
    &gLzmaCustomDecompressGuid,
    LzmaGuidedSectionGetInfo,
    LzmaGuidedSectionExtraction
    );

  // Assume the FV that contains the SEC (our code) also contains a compressed FV.
  Status = DecompressFirstFv ();
  ASSERT_EFI_ERROR (Status);

  // Load the DXE Core and transfer control to it
  Status = LoadDxeCoreFromFv (NULL, 0);
  ASSERT_EFI_ERROR (Status);
}

VOID
CEntryPoint (
  IN  UINTN                     MpId,
  IN  UINTN                     UefiMemoryBase,
  IN  UINTN                     StacksBase,
  IN  UINTN                     GlobalVariableBase
  )
{
  UINT64   StartTimeStamp;

  // Initialize the platform specific controllers
  ArmPlatformInitialize (MpId);

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
  // Invalidate Data cache
  ArmInvalidateDataCache ();
  // Invalidate instruction cache
  ArmInvalidateInstructionCache ();
  // Enable Instruction Caches on all cores.
  ArmEnableInstructionCache ();

  // Define the Global Variable region
  mGlobalVariableBase = GlobalVariableBase;

  PrePiMain (UefiMemoryBase, StacksBase, GlobalVariableBase, StartTimeStamp);

  // DXE Core should always load and never return
  ASSERT (FALSE);
}

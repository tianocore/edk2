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

#include <Library/DebugAgentLib.h>
#include <Library/PrePiLib.h>
#include <Library/PrintLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>

#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <Guid/LzmaDecompress.h>
#include <Guid/ArmGlobalVariableHob.h>

#include "PrePi.h"
#include "LzmaDecompress.h"

#define IS_XIP() (((UINT32)FixedPcdGet32 (PcdFdBaseAddress) > (UINT32)(FixedPcdGet64 (PcdSystemMemoryBase) + FixedPcdGet32 (PcdSystemMemorySize))) || \
                  ((FixedPcdGet32 (PcdFdBaseAddress) + FixedPcdGet32 (PcdFdSize)) < FixedPcdGet64 (PcdSystemMemoryBase)))

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
  ARM_MP_CORE_INFO_PPI*         ArmMpCoreInfoPpi;
  UINTN                         ArmCoreCount;
  ARM_CORE_INFO*                ArmCoreInfoTable;
  EFI_STATUS                    Status;
  CHAR8                         Buffer[100];
  UINTN                         CharCount;
  UINTN                         StacksSize;

  // If ensure the FD is either part of the System Memory or totally outside of the System Memory (XIP)
  ASSERT (IS_XIP() ||
          ((FixedPcdGet32 (PcdFdBaseAddress) >= FixedPcdGet64 (PcdSystemMemoryBase)) &&
           ((UINT32)(FixedPcdGet32 (PcdFdBaseAddress) + FixedPcdGet32 (PcdFdSize)) <= (UINT32)(FixedPcdGet64 (PcdSystemMemoryBase) + FixedPcdGet64 (PcdSystemMemorySize)))));

  // Initialize the architecture specific bits
  ArchInitialize ();

  // Initialize the Serial Port
  SerialPortInitialize ();
  CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"UEFI firmware (version %s built at %a on %a)\n\r",
    (CHAR16*)PcdGetPtr(PcdFirmwareVersionString), __TIME__, __DATE__);
  SerialPortWrite ((UINT8 *) Buffer, CharCount);

  // Initialize the Debug Agent for Source Level Debugging
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);
  SaveAndSetDebugTimerInterrupt (TRUE);

  // Declare the PI/UEFI memory region
  HobList = HobConstructor (
    (VOID*)UefiMemoryBase,
    FixedPcdGet32 (PcdSystemMemoryUefiRegionSize),
    (VOID*)UefiMemoryBase,
    (VOID*)StacksBase  // The top of the UEFI Memory is reserved for the stacks
    );
  PrePeiSetHobList (HobList);

  // Initialize MMU and Memory HOBs (Resource Descriptor HOBs)
  Status = MemoryPeim (UefiMemoryBase, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize));
  ASSERT_EFI_ERROR (Status);

  // Create the Stacks HOB (reserve the memory for all stacks)
  if (ArmIsMpCore ()) {
    StacksSize = PcdGet32 (PcdCPUCorePrimaryStackSize) +
                 ((FixedPcdGet32 (PcdCoreCount) - 1) * FixedPcdGet32 (PcdCPUCoreSecondaryStackSize));
  } else {
    StacksSize = PcdGet32 (PcdCPUCorePrimaryStackSize);
  }
  BuildStackHob (StacksBase, StacksSize);

  // Declare the Global Variable HOB
  BuildGlobalVariableHob (GlobalVariableBase, FixedPcdGet32 (PcdPeiGlobalVariableSize));

  //TODO: Call CpuPei as a library
  BuildCpuHob (PcdGet8 (PcdPrePiCpuMemorySize), PcdGet8 (PcdPrePiCpuIoSize));

  if (ArmIsMpCore ()) {
    // Only MP Core platform need to produce gArmMpCoreInfoPpiGuid
    Status = GetPlatformPpi (&gArmMpCoreInfoPpiGuid, (VOID**)&ArmMpCoreInfoPpi);

    // On MP Core Platform we must implement the ARM MP Core Info PPI (gArmMpCoreInfoPpiGuid)
    ASSERT_EFI_ERROR (Status);

    // Build the MP Core Info Table
    ArmCoreCount = 0;
    Status = ArmMpCoreInfoPpi->GetMpCoreInfo (&ArmCoreCount, &ArmCoreInfoTable);
    if (!EFI_ERROR(Status) && (ArmCoreCount > 0)) {
      // Build MPCore Info HOB
      BuildGuidDataHob (&gArmMpCoreInfoGuid, ArmCoreInfoTable, sizeof (ARM_CORE_INFO) * ArmCoreCount);
    }
  }

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

  ASSERT(!ArmIsMpCore() || (PcdGet32 (PcdCoreCount) > 1));

  // Initialize the platform specific controllers
  ArmPlatformInitialize (MpId);

  if (ArmPlatformIsPrimaryCore (MpId) && PerformanceMeasurementEnabled ()) {
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

  // Define the Global Variable region when we are not running in XIP
  if (!IS_XIP()) {
    if (ArmPlatformIsPrimaryCore (MpId)) {
      mGlobalVariableBase = GlobalVariableBase;
      if (ArmIsMpCore()) {
        // Signal the Global Variable Region is defined (event: ARM_CPU_EVENT_DEFAULT)
        ArmCallSEV ();
      }
    } else {
      // Wait the Primay core has defined the address of the Global Variable region (event: ARM_CPU_EVENT_DEFAULT)
      ArmCallWFE ();
    }
  }

  // If not primary Jump to Secondary Main
  if (ArmPlatformIsPrimaryCore (MpId)) {
    // Goto primary Main.
    PrimaryMain (UefiMemoryBase, StacksBase, GlobalVariableBase, StartTimeStamp);
  } else {
    SecondaryMain (MpId);
  }

  // DXE Core should always load and never return
  ASSERT (FALSE);
}


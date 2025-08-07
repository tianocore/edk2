/** @file

  Copyright (c) 2011-2017, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeilessSec.h"

#define IS_XIP()  (((UINT64)FixedPcdGet64 (PcdFdBaseAddress) > mSystemMemoryEnd) ||\
                  ((FixedPcdGet64 (PcdFdBaseAddress) + FixedPcdGet32 (PcdFdSize)) <= FixedPcdGet64 (PcdSystemMemoryBase)))

UINT64  mSystemMemoryEnd = FixedPcdGet64 (PcdSystemMemoryBase) +
                           FixedPcdGet64 (PcdSystemMemorySize) - 1;

/**
  Obtain a PPI from the list of PPIs provided by the platform code.

  @param[in]  PpiGuid   GUID of the PPI to obtain
  @param[out] Ppi       Address of GUID pointer to return the PPI

  @return     Whether the PPI was obtained successfully
**/
STATIC
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
  PpiListCount = PpiListSize / sizeof (EFI_PEI_PPI_DESCRIPTOR);
  for (Index = 0; Index < PpiListCount; Index++, PpiList++) {
    if (CompareGuid (PpiList->Guid, PpiGuid) == TRUE) {
      *Ppi = PpiList->Ppi;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  SEC main routine.

  @param[in]  UefiMemoryBase  Start of the PI/UEFI memory region
  @param[in]  StackBase      Start of the stack
  @param[in]  StartTimeStamp  Timer value at start of execution
  @param[in]  TransferListBaseAddr  Base address of the Transfer List
**/
STATIC
VOID
SecMain (
  IN  UINTN   UefiMemoryBase,
  IN  UINTN   StackBase,
  IN  UINT64  StartTimeStamp,
  IN  UINTN   TransferListBaseAddr
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  ARM_MP_CORE_INFO_PPI        *ArmMpCoreInfoPpi;
  UINTN                       ArmCoreCount;
  ARM_CORE_INFO               *ArmCoreInfoTable;
  EFI_STATUS                  Status;
  CHAR8                       Buffer[100];
  UINTN                       CharCount;
  UINTN                       StacksSize;
  FIRMWARE_SEC_PERFORMANCE    Performance;
  VOID                        *TransferListBase;
  UINTN                       *TransferListHobData;

  // If ensure the FD is either part of the System Memory or totally outside of the System Memory (XIP)
  ASSERT (
    IS_XIP () ||
    ((FixedPcdGet64 (PcdFdBaseAddress) >= FixedPcdGet64 (PcdSystemMemoryBase)) &&
     ((UINT64)(FixedPcdGet64 (PcdFdBaseAddress) + FixedPcdGet32 (PcdFdSize)) <= (UINT64)mSystemMemoryEnd))
    );

  // Initialize the architecture specific bits
  ArchInitialize ();

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

  // Initialize the Debug Agent for Source Level Debugging
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);
  SaveAndSetDebugTimerInterrupt (TRUE);

  // Declare the PI/UEFI memory region
  HobList = HobConstructor (
              (VOID *)UefiMemoryBase,
              FixedPcdGet32 (PcdSystemMemoryUefiRegionSize),
              (VOID *)UefiMemoryBase,
              (VOID *)StackBase // The top of the UEFI Memory is reserved for the stack
              );
  PrePeiSetHobList (HobList);

  // Initialize MMU and Memory HOBs (Resource Descriptor HOBs)
  Status = MemoryPeim (UefiMemoryBase, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize));
  ASSERT_EFI_ERROR (Status);

  // Create the Stacks HOB
  StacksSize = PcdGet32 (PcdCPUCorePrimaryStackSize);
  BuildStackHob (StackBase, StacksSize);

  // TODO: Call CpuPei as a library
  BuildCpuHob (ArmGetPhysicalAddressBits (), PcdGet8 (PcdPrePiCpuIoSize));

  if (ArmIsMpCore ()) {
    // Only MP Core platform need to produce gArmMpCoreInfoPpiGuid
    Status = GetPlatformPpi (&gArmMpCoreInfoPpiGuid, (VOID **)&ArmMpCoreInfoPpi);

    // On MP Core Platform we must implement the ARM MP Core Info PPI (gArmMpCoreInfoPpiGuid)
    ASSERT_EFI_ERROR (Status);

    // Build the MP Core Info Table
    ArmCoreCount = 0;
    Status       = ArmMpCoreInfoPpi->GetMpCoreInfo (&ArmCoreCount, &ArmCoreInfoTable);
    if (!EFI_ERROR (Status) && (ArmCoreCount > 0)) {
      // Build MPCore Info HOB
      BuildGuidDataHob (&gArmMpCoreInfoGuid, ArmCoreInfoTable, sizeof (ARM_CORE_INFO) * ArmCoreCount);
    }
  }

  // Dump the Transfer List
  TransferListBase = (VOID *)TransferListBaseAddr;
  if (TransferListBase != NULL) {
    if (TransferListCheckHeader (TransferListBase) != TRANSFER_LIST_OPS_INVALID) {
      DEBUG_CODE_BEGIN ();
      TransferListDump (TransferListBase);
      DEBUG_CODE_END ();

      TransferListHobData = BuildGuidHob (&gArmTransferListHobGuid, sizeof (*TransferListHobData));
      ASSERT (TransferListHobData != NULL);

      *TransferListHobData = (UINTN)TransferListBase;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: No valid operations possible on TransferList found @ 0x%p\n", __func__, TransferListBase));
    }
  } else {
    DEBUG ((DEBUG_INFO, "%a: No TransferList found, continuing boot\n", __func__));
  }

  // Store timer value logged at the beginning of firmware image execution
  Performance.ResetEnd = GetTimeInNanoSecond (StartTimeStamp);

  // Build SEC Performance Data Hob
  BuildGuidDataHob (&gEfiFirmwarePerformanceGuid, &Performance, sizeof (Performance));

  // Set the Boot Mode
  SetBootMode (ArmPlatformGetBootMode ());

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

/**
  C entrypoint into the SEC driver.

  @param[in]  UefiMemoryBase  Start of the PI/UEFI memory region
  @param[in]  StackBase       Start of the stack
  @param[in]  TransferListBaseAddr  Base address of the Transfer List
**/
VOID
CEntryPoint (
  IN  UINTN  UefiMemoryBase,
  IN  UINTN  StackBase,
  IN  UINTN  TransferListBaseAddr
  )
{
  UINT64  StartTimeStamp;

  // Initialize the platform specific controllers
  ArmPlatformInitialize (ArmReadMpidr ());

  if (PerformanceMeasurementEnabled ()) {
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

  InvalidateDataCacheRange (
    (VOID *)UefiMemoryBase,
    FixedPcdGet32 (PcdSystemMemoryUefiRegionSize)
    );

  SecMain (UefiMemoryBase, StackBase, StartTimeStamp, TransferListBaseAddr);

  // DXE Core should always load and never return
  ASSERT (FALSE);
}

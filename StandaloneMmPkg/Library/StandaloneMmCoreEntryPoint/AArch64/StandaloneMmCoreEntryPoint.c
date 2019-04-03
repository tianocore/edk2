/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms

Copyright (c) 2017 - 2018, ARM Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiMm.h>

#include <Library/AArch64/StandaloneMmCoreEntryPoint.h>

#include <PiPei.h>
#include <Guid/MmramMemoryReserve.h>
#include <Guid/MpInformation.h>

#include <Library/ArmMmuLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/ArmMmSvc.h>

#define SPM_MAJOR_VER_MASK        0xFFFF0000
#define SPM_MINOR_VER_MASK        0x0000FFFF
#define SPM_MAJOR_VER_SHIFT       16

CONST UINT32 SPM_MAJOR_VER = 0;
CONST UINT32 SPM_MINOR_VER = 1;

CONST UINT8 BOOT_PAYLOAD_VERSION = 1;

PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT      CpuDriverEntryPoint = NULL;

/**
  Retrieve a pointer to and print the boot information passed by privileged
  secure firmware

  @param  SharedBufAddress The pointer memory shared with privileged firmware

**/
EFI_SECURE_PARTITION_BOOT_INFO *
GetAndPrintBootinformation (
  IN VOID                      *SharedBufAddress
)
{
  EFI_SECURE_PARTITION_BOOT_INFO *PayloadBootInfo;
  EFI_SECURE_PARTITION_CPU_INFO  *PayloadCpuInfo;
  UINTN                          Index;

  PayloadBootInfo = (EFI_SECURE_PARTITION_BOOT_INFO *) SharedBufAddress;

  if (PayloadBootInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "PayloadBootInfo NULL\n"));
    return NULL;
  }

  if (PayloadBootInfo->Header.Version != BOOT_PAYLOAD_VERSION) {
    DEBUG ((DEBUG_ERROR, "Boot Information Version Mismatch. Current=0x%x, Expected=0x%x.\n",
            PayloadBootInfo->Header.Version, BOOT_PAYLOAD_VERSION));
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "NumSpMemRegions - 0x%x\n", PayloadBootInfo->NumSpMemRegions));
  DEBUG ((DEBUG_INFO, "SpMemBase       - 0x%lx\n", PayloadBootInfo->SpMemBase));
  DEBUG ((DEBUG_INFO, "SpMemLimit      - 0x%lx\n", PayloadBootInfo->SpMemLimit));
  DEBUG ((DEBUG_INFO, "SpImageBase     - 0x%lx\n", PayloadBootInfo->SpImageBase));
  DEBUG ((DEBUG_INFO, "SpStackBase     - 0x%lx\n", PayloadBootInfo->SpStackBase));
  DEBUG ((DEBUG_INFO, "SpHeapBase      - 0x%lx\n", PayloadBootInfo->SpHeapBase));
  DEBUG ((DEBUG_INFO, "SpNsCommBufBase - 0x%lx\n", PayloadBootInfo->SpNsCommBufBase));
  DEBUG ((DEBUG_INFO, "SpSharedBufBase - 0x%lx\n", PayloadBootInfo->SpSharedBufBase));

  DEBUG ((DEBUG_INFO, "SpImageSize     - 0x%x\n", PayloadBootInfo->SpImageSize));
  DEBUG ((DEBUG_INFO, "SpPcpuStackSize - 0x%x\n", PayloadBootInfo->SpPcpuStackSize));
  DEBUG ((DEBUG_INFO, "SpHeapSize      - 0x%x\n", PayloadBootInfo->SpHeapSize));
  DEBUG ((DEBUG_INFO, "SpNsCommBufSize - 0x%x\n", PayloadBootInfo->SpNsCommBufSize));
  DEBUG ((DEBUG_INFO, "SpPcpuSharedBufSize - 0x%x\n", PayloadBootInfo->SpPcpuSharedBufSize));

  DEBUG ((DEBUG_INFO, "NumCpus         - 0x%x\n", PayloadBootInfo->NumCpus));
  DEBUG ((DEBUG_INFO, "CpuInfo         - 0x%p\n", PayloadBootInfo->CpuInfo));

  PayloadCpuInfo = (EFI_SECURE_PARTITION_CPU_INFO *) PayloadBootInfo->CpuInfo;

  if (PayloadCpuInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "PayloadCpuInfo NULL\n"));
    return NULL;
  }

  for (Index = 0; Index < PayloadBootInfo->NumCpus; Index++) {
    DEBUG ((DEBUG_INFO, "Mpidr           - 0x%lx\n", PayloadCpuInfo[Index].Mpidr));
    DEBUG ((DEBUG_INFO, "LinearId        - 0x%x\n", PayloadCpuInfo[Index].LinearId));
    DEBUG ((DEBUG_INFO, "Flags           - 0x%x\n", PayloadCpuInfo[Index].Flags));
  }

  return PayloadBootInfo;
}

VOID
EFIAPI
DelegatedEventLoop (
  IN ARM_SVC_ARGS *EventCompleteSvcArgs
  )
{
  EFI_STATUS Status;
  UINTN SvcStatus;

  while (TRUE) {
    ArmCallSvc (EventCompleteSvcArgs);

    DEBUG ((DEBUG_INFO, "Received delegated event\n"));
    DEBUG ((DEBUG_INFO, "X0 :  0x%x\n", (UINT32) EventCompleteSvcArgs->Arg0));
    DEBUG ((DEBUG_INFO, "X1 :  0x%x\n", (UINT32) EventCompleteSvcArgs->Arg1));
    DEBUG ((DEBUG_INFO, "X2 :  0x%x\n", (UINT32) EventCompleteSvcArgs->Arg2));
    DEBUG ((DEBUG_INFO, "X3 :  0x%x\n", (UINT32) EventCompleteSvcArgs->Arg3));

    Status = CpuDriverEntryPoint (
               EventCompleteSvcArgs->Arg0,
               EventCompleteSvcArgs->Arg3,
               EventCompleteSvcArgs->Arg1
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed delegated event 0x%x, Status 0x%x\n",
              EventCompleteSvcArgs->Arg0, Status));
    }

    switch (Status) {
    case EFI_SUCCESS:
      SvcStatus = ARM_SVC_SPM_RET_SUCCESS;
      break;
    case EFI_INVALID_PARAMETER:
      SvcStatus = ARM_SVC_SPM_RET_INVALID_PARAMS;
      break;
    case EFI_ACCESS_DENIED:
      SvcStatus = ARM_SVC_SPM_RET_DENIED;
      break;
    case EFI_OUT_OF_RESOURCES:
      SvcStatus = ARM_SVC_SPM_RET_NO_MEMORY;
      break;
    case EFI_UNSUPPORTED:
      SvcStatus = ARM_SVC_SPM_RET_NOT_SUPPORTED;
      break;
    default:
      SvcStatus = ARM_SVC_SPM_RET_NOT_SUPPORTED;
      break;
    }

    EventCompleteSvcArgs->Arg0 = ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH64;
    EventCompleteSvcArgs->Arg1 = SvcStatus;
  }
}

STATIC
EFI_STATUS
GetSpmVersion (VOID)
{
  EFI_STATUS   Status;
  UINT16       SpmMajorVersion;
  UINT16       SpmMinorVersion;
  UINT32       SpmVersion;
  ARM_SVC_ARGS SpmVersionArgs;

  SpmVersionArgs.Arg0 = ARM_SVC_ID_SPM_VERSION_AARCH32;

  ArmCallSvc (&SpmVersionArgs);

  SpmVersion = SpmVersionArgs.Arg0;

  SpmMajorVersion = ((SpmVersion & SPM_MAJOR_VER_MASK) >> SPM_MAJOR_VER_SHIFT);
  SpmMinorVersion = ((SpmVersion & SPM_MINOR_VER_MASK) >> 0);

  // Different major revision values indicate possibly incompatible functions.
  // For two revisions, A and B, for which the major revision values are
  // identical, if the minor revision value of revision B is greater than
  // the minor revision value of revision A, then every function in
  // revision A must work in a compatible way with revision B.
  // However, it is possible for revision B to have a higher
  // function count than revision A.
  if ((SpmMajorVersion == SPM_MAJOR_VER) &&
      (SpmMinorVersion >= SPM_MINOR_VER))
  {
    DEBUG ((DEBUG_INFO, "SPM Version: Major=0x%x, Minor=0x%x\n",
           SpmMajorVersion, SpmMinorVersion));
    Status = EFI_SUCCESS;
  }
  else
  {
    DEBUG ((DEBUG_INFO, "Incompatible SPM Versions.\n Current Version: Major=0x%x, Minor=0x%x.\n Expected: Major=0x%x, Minor>=0x%x.\n",
            SpmMajorVersion, SpmMinorVersion, SPM_MAJOR_VER, SPM_MINOR_VER));
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  The entry point of Standalone MM Foundation.

  @param  SharedBufAddress  Pointer to the Buffer between SPM and SP.
  @param  cookie1.
  @param  cookie2.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID    *SharedBufAddress,
  IN UINT64  SharedBufSize,
  IN UINT64  cookie1,
  IN UINT64  cookie2
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT            ImageContext;
  EFI_SECURE_PARTITION_BOOT_INFO          *PayloadBootInfo;
  ARM_SVC_ARGS                            InitMmFoundationSvcArgs;
  EFI_STATUS                              Status;
  UINT32                                  SectionHeaderOffset;
  UINT16                                  NumberOfSections;
  VOID                                    *HobStart;
  VOID                                    *TeData;
  UINTN                                   TeDataSize;

  // Get Secure Partition Manager Version Information
  Status = GetSpmVersion ();
  if (EFI_ERROR (Status)) {
    goto finish;
  }

  PayloadBootInfo = GetAndPrintBootinformation (SharedBufAddress);
  if (PayloadBootInfo == NULL) {
    Status = EFI_UNSUPPORTED;
    goto finish;
  }

  // Locate PE/COFF File information for the Standalone MM core module
  Status = LocateStandaloneMmCorePeCoffData (
             (EFI_FIRMWARE_VOLUME_HEADER *) PayloadBootInfo->SpImageBase,
             &TeData,
             &TeDataSize
             );

  if (EFI_ERROR (Status)) {
    goto finish;
  }

  // Obtain the PE/COFF Section information for the Standalone MM core module
  Status = GetStandaloneMmCorePeCoffSections (
             TeData,
             &ImageContext,
             &SectionHeaderOffset,
             &NumberOfSections
             );

  if (EFI_ERROR (Status)) {
    goto finish;
  }

  // Update the memory access permissions of individual sections in the
  // Standalone MM core module
  Status = UpdateMmFoundationPeCoffPermissions (
             &ImageContext,
             SectionHeaderOffset,
             NumberOfSections,
             ArmSetMemoryRegionNoExec,
             ArmSetMemoryRegionReadOnly,
             ArmClearMemoryRegionReadOnly
             );

  if (EFI_ERROR (Status)) {
    goto finish;
  }

  //
  // Create Hoblist based upon boot information passed by privileged software
  //
  HobStart = CreateHobListFromBootInfo (&CpuDriverEntryPoint, PayloadBootInfo);

  //
  // Call the MM Core entry point
  //
  ProcessModuleEntryPointList (HobStart);

  DEBUG ((DEBUG_INFO, "Shared Cpu Driver EP 0x%lx\n", (UINT64) CpuDriverEntryPoint));

finish:
  ZeroMem (&InitMmFoundationSvcArgs, sizeof(InitMmFoundationSvcArgs));
  InitMmFoundationSvcArgs.Arg0 = ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH64;
  InitMmFoundationSvcArgs.Arg1 = Status;
  DelegatedEventLoop (&InitMmFoundationSvcArgs);
}

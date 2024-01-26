/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms

  Copyright (c) 2017 - 2024, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - SpmMM - An implementation where the Secure Partition Manager resides at EL3
              with management services running from an isolated Secure Partitions
              at S-EL0, and the communication protocol is the Management Mode(MM)
              interface.

  @par Reference(s):
  - Secure Partition Manager [https://trustedfirmware-a.readthedocs.io/en/latest/components/secure-partition-manager-mm.html].

**/

#include <PiMm.h>

#include <Library/Arm/StandaloneMmCoreEntryPoint.h>

#include <PiPei.h>
#include <Guid/MmramMemoryReserve.h>
#include <Guid/MpInformation.h>

#include <StandaloneMmCpu.h>
#include <Library/ArmSvcLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Library/StandaloneMmMmuLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/ArmMmSvc.h>
#include <IndustryStandard/ArmFfaSvc.h>

#define SPM_MAJOR_VER_MASK   0xFFFF0000
#define SPM_MINOR_VER_MASK   0x0000FFFF
#define SPM_MAJOR_VER_SHIFT  16
#define FFA_NOT_SUPPORTED    -1

STATIC CONST UINT32  mSpmMajorVer = SPM_MAJOR_VERSION;
STATIC CONST UINT32  mSpmMinorVer = SPM_MINOR_VERSION;

STATIC CONST UINT32  mSpmMajorVerFfa = SPM_MAJOR_VERSION_FFA;
STATIC CONST UINT32  mSpmMinorVerFfa = SPM_MINOR_VERSION_FFA;

#define BOOT_PAYLOAD_VERSION  1

PI_MM_CPU_DRIVER_ENTRYPOINT  CpuDriverEntryPoint = NULL;

/**
  Retrieve a pointer to and print the boot information passed by privileged
  secure firmware.

  @param  [in] SharedBufAddress   The pointer memory shared with privileged
                                  firmware.

**/
EFI_SECURE_PARTITION_BOOT_INFO *
GetAndPrintBootinformation (
  IN VOID  *SharedBufAddress
  )
{
  EFI_SECURE_PARTITION_BOOT_INFO  *PayloadBootInfo;
  EFI_SECURE_PARTITION_CPU_INFO   *PayloadCpuInfo;
  UINTN                           Index;

  PayloadBootInfo = (EFI_SECURE_PARTITION_BOOT_INFO *)SharedBufAddress;

  if (PayloadBootInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "PayloadBootInfo NULL\n"));
    return NULL;
  }

  if (PayloadBootInfo->Header.Version != BOOT_PAYLOAD_VERSION) {
    DEBUG ((
      DEBUG_ERROR,
      "Boot Information Version Mismatch. Current=0x%x, Expected=0x%x.\n",
      PayloadBootInfo->Header.Version,
      BOOT_PAYLOAD_VERSION
      ));
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
  DEBUG ((DEBUG_INFO, "SpSharedBufSize - 0x%x\n", PayloadBootInfo->SpSharedBufSize));

  DEBUG ((DEBUG_INFO, "NumCpus         - 0x%x\n", PayloadBootInfo->NumCpus));
  DEBUG ((DEBUG_INFO, "CpuInfo         - 0x%p\n", PayloadBootInfo->CpuInfo));

  PayloadCpuInfo = (EFI_SECURE_PARTITION_CPU_INFO *)PayloadBootInfo->CpuInfo;

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

/**
  Convert EFI_STATUS to SPM_MM return code.

  @param [in] Status          edk2 status code.

  @retval ARM_SVC_SPM_RET_*   return value correspond to EFI_STATUS.

**/
STATIC
UINTN
EFIAPI
EfiStatusToSpmMmStatus (
  IN EFI_STATUS  Status
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      return ARM_SVC_SPM_RET_SUCCESS;
    case EFI_INVALID_PARAMETER:
      return ARM_SVC_SPM_RET_INVALID_PARAMS;
    case EFI_ACCESS_DENIED:
      return ARM_SVC_SPM_RET_DENIED;
    case EFI_OUT_OF_RESOURCES:
      return ARM_SVC_SPM_RET_NO_MEMORY;
    default:
      return ARM_SVC_SPM_RET_NOT_SUPPORTED;
  }
}

/**
  Convert EFI_STATUS to FFA return code.

  @param [in] Status          edk2 status code.

  @retval ARM_FFA_SPM_RET_*   return value correspond to EFI_STATUS.

**/
STATIC
UINTN
EFIAPI
EfiStatusToFfaStatus (
  IN EFI_STATUS  Status
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      return ARM_FFA_SPM_RET_SUCCESS;
    case EFI_INVALID_PARAMETER:
      return ARM_FFA_SPM_RET_INVALID_PARAMETERS;
    case EFI_OUT_OF_RESOURCES:
      return ARM_FFA_SPM_RET_NO_MEMORY;
    case EFI_ALREADY_STARTED:
      return ARM_FFA_SPM_RET_BUSY;
    case EFI_INTERRUPT_PENDING:
      return ARM_FFA_SPM_RET_INTERRUPTED;
    case EFI_ACCESS_DENIED:
      return ARM_FFA_SPM_RET_DENIED;
    case EFI_ABORTED:
      return ARM_FFA_SPM_RET_ABORTED;
    default:
      return ARM_FFA_SPM_RET_NOT_SUPPORTED;
  }
}

/**
  Initialise Event Complete arguments to be returned via SVC call.

  @param[in]      FfaEnabled                Ffa enabled.
  @param[in]      Status                    Result of StMm.
  @param[out]     EventCompleteSvcArgs      Args structure.

**/
STATIC
VOID
SetEventCompleteSvcArgs (
  IN BOOLEAN        FfaEnabled,
  IN EFI_STATUS     Status,
  OUT ARM_SVC_ARGS  *EventCompleteSvcArgs
  )
{
  if (FfaEnabled) {
    EventCompleteSvcArgs->Arg0 = ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP;
    EventCompleteSvcArgs->Arg1 = 0;
    EventCompleteSvcArgs->Arg2 = 0;
    EventCompleteSvcArgs->Arg3 = ARM_SVC_ID_SP_EVENT_COMPLETE;
    EventCompleteSvcArgs->Arg4 = EfiStatusToFfaStatus (Status);
  } else {
    EventCompleteSvcArgs->Arg0 = ARM_SVC_ID_SP_EVENT_COMPLETE;
    EventCompleteSvcArgs->Arg1 = EfiStatusToSpmMmStatus (Status);
  }
}

/**
  A loop to delegated events.

  @param  [in] EventCompleteSvcArgs   Pointer to the event completion arguments.

**/
VOID
EFIAPI
DelegatedEventLoop (
  IN ARM_SVC_ARGS  *EventCompleteSvcArgs
  )
{
  BOOLEAN     FfaEnabled;
  EFI_STATUS  Status;

  FfaEnabled = FeaturePcdGet (PcdFfaEnable);

  while (TRUE) {
    ArmCallSvc (EventCompleteSvcArgs);

    DEBUG ((DEBUG_INFO, "Received delegated event\n"));
    DEBUG ((DEBUG_INFO, "X0 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg0));
    DEBUG ((DEBUG_INFO, "X1 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg1));
    DEBUG ((DEBUG_INFO, "X2 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg2));
    DEBUG ((DEBUG_INFO, "X3 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg3));
    DEBUG ((DEBUG_INFO, "X4 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg4));
    DEBUG ((DEBUG_INFO, "X5 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg5));
    DEBUG ((DEBUG_INFO, "X6 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg6));
    DEBUG ((DEBUG_INFO, "X7 :  0x%x\n", (UINT32)EventCompleteSvcArgs->Arg7));

    //
    // ARM TF passes SMC FID of the MM_COMMUNICATE interface as the Event ID upon
    // receipt of a synchronous MM request. Use the Event ID to distinguish
    // between synchronous and asynchronous events.
    //
    if ((ARM_SMC_ID_MM_COMMUNICATE != (UINT32)EventCompleteSvcArgs->Arg0) &&
        (ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ != (UINT32)EventCompleteSvcArgs->Arg0))
    {
      DEBUG ((DEBUG_ERROR, "UnRecognized Event - 0x%x\n", (UINT32)EventCompleteSvcArgs->Arg0));
      Status = EFI_INVALID_PARAMETER;
    } else {
      if (FfaEnabled) {
        Status = CpuDriverEntryPoint (
                   EventCompleteSvcArgs->Arg0,
                   EventCompleteSvcArgs->Arg6,
                   EventCompleteSvcArgs->Arg3
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_ERROR,
            "Failed delegated event 0x%x, Status 0x%x\n",
            EventCompleteSvcArgs->Arg3,
            Status
            ));
        }
      } else {
        Status = CpuDriverEntryPoint (
                   EventCompleteSvcArgs->Arg0,
                   EventCompleteSvcArgs->Arg3,
                   EventCompleteSvcArgs->Arg1
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_ERROR,
            "Failed delegated event 0x%x, Status 0x%x\n",
            EventCompleteSvcArgs->Arg0,
            Status
            ));
        }
      }
    }

    SetEventCompleteSvcArgs (FfaEnabled, Status, EventCompleteSvcArgs);
  }
}

/**
  Query the SPM version, check compatibility and return success if compatible.

  @retval EFI_SUCCESS       SPM versions compatible.
  @retval EFI_UNSUPPORTED   SPM versions not compatible.
**/
STATIC
EFI_STATUS
GetSpmVersion (
  VOID
  )
{
  EFI_STATUS    Status;
  UINT16        CalleeSpmMajorVer;
  UINT16        CallerSpmMajorVer;
  UINT16        CalleeSpmMinorVer;
  UINT16        CallerSpmMinorVer;
  UINT32        SpmVersion;
  ARM_SVC_ARGS  SpmVersionArgs;

  if (FeaturePcdGet (PcdFfaEnable)) {
    SpmVersionArgs.Arg0  = ARM_SVC_ID_FFA_VERSION_AARCH32;
    SpmVersionArgs.Arg1  = mSpmMajorVerFfa << SPM_MAJOR_VER_SHIFT;
    SpmVersionArgs.Arg1 |= mSpmMinorVerFfa;
    CallerSpmMajorVer    = mSpmMajorVerFfa;
    CallerSpmMinorVer    = mSpmMinorVerFfa;
  } else {
    SpmVersionArgs.Arg0 = ARM_SVC_ID_SPM_VERSION_AARCH32;
    CallerSpmMajorVer   = mSpmMajorVer;
    CallerSpmMinorVer   = mSpmMinorVer;
  }

  ArmCallSvc (&SpmVersionArgs);

  SpmVersion = SpmVersionArgs.Arg0;
  if (SpmVersion == FFA_NOT_SUPPORTED) {
    return EFI_UNSUPPORTED;
  }

  CalleeSpmMajorVer = ((SpmVersion & SPM_MAJOR_VER_MASK) >> SPM_MAJOR_VER_SHIFT);
  CalleeSpmMinorVer = ((SpmVersion & SPM_MINOR_VER_MASK) >> 0);

  // Different major revision values indicate possibly incompatible functions.
  // For two revisions, A and B, for which the major revision values are
  // identical, if the minor revision value of revision B is greater than
  // the minor revision value of revision A, then every function in
  // revision A must work in a compatible way with revision B.
  // However, it is possible for revision B to have a higher
  // function count than revision A.
  if ((CalleeSpmMajorVer == CallerSpmMajorVer) &&
      (CalleeSpmMinorVer >= CallerSpmMinorVer))
  {
    DEBUG ((
      DEBUG_INFO,
      "SPM Version: Major=0x%x, Minor=0x%x\n",
      CalleeSpmMajorVer,
      CalleeSpmMinorVer
      ));
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((
      DEBUG_INFO,
      "Incompatible SPM Versions.\n Callee Version: Major=0x%x, Minor=0x%x.\n Caller: Major=0x%x, Minor>=0x%x.\n",
      CalleeSpmMajorVer,
      CalleeSpmMinorVer,
      CallerSpmMajorVer,
      CallerSpmMinorVer
      ));
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  The entry point of Standalone MM Foundation.

  @param  [in]  SharedBufAddress  Pointer to the Buffer between SPM and SP.
  @param  [in]  SharedBufSize     Size of the shared buffer.
  @param  [in]  cookie1           Cookie 1
  @param  [in]  cookie2           Cookie 2

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
  PE_COFF_LOADER_IMAGE_CONTEXT    ImageContext;
  EFI_SECURE_PARTITION_BOOT_INFO  *PayloadBootInfo;
  ARM_SVC_ARGS                    EventCompleteSvcArgs;
  EFI_STATUS                      Status;
  UINT32                          SectionHeaderOffset;
  UINT16                          NumberOfSections;
  VOID                            *HobStart;
  VOID                            *TeData;
  UINTN                           TeDataSize;
  EFI_PHYSICAL_ADDRESS            ImageBase;

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
             (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PayloadBootInfo->SpImageBase,
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
             &ImageBase,
             &SectionHeaderOffset,
             &NumberOfSections
             );

  if (EFI_ERROR (Status)) {
    goto finish;
  }

  //
  // ImageBase may deviate from ImageContext.ImageAddress if we are dealing
  // with a TE image, in which case the latter points to the actual offset
  // of the image, whereas ImageBase refers to the address where the image
  // would start if the stripped PE headers were still in place. In either
  // case, we need to fix up ImageBase so it refers to the actual current
  // load address.
  //
  ImageBase += (UINTN)TeData - ImageContext.ImageAddress;

  // Update the memory access permissions of individual sections in the
  // Standalone MM core module
  Status = UpdateMmFoundationPeCoffPermissions (
             &ImageContext,
             ImageBase,
             SectionHeaderOffset,
             NumberOfSections,
             ArmSetMemoryRegionNoExec,
             ArmSetMemoryRegionReadOnly,
             ArmClearMemoryRegionReadOnly
             );

  if (EFI_ERROR (Status)) {
    goto finish;
  }

  if (ImageContext.ImageAddress != (UINTN)TeData) {
    ImageContext.ImageAddress = (UINTN)TeData;
    ArmSetMemoryRegionNoExec (ImageBase, SIZE_4KB);
    ArmClearMemoryRegionReadOnly (ImageBase, SIZE_4KB);

    Status = PeCoffLoaderRelocateImage (&ImageContext);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Create Hoblist based upon boot information passed by privileged software
  //
  HobStart = CreateHobListFromBootInfo (&CpuDriverEntryPoint, PayloadBootInfo);

  //
  // Call the MM Core entry point
  //
  ProcessModuleEntryPointList (HobStart);

  DEBUG ((DEBUG_INFO, "Shared Cpu Driver EP %p\n", (VOID *)CpuDriverEntryPoint));

finish:
  ZeroMem (&EventCompleteSvcArgs, sizeof (EventCompleteSvcArgs));
  SetEventCompleteSvcArgs (
    FeaturePcdGet (PcdFfaEnable),
    Status,
    &EventCompleteSvcArgs
    );
  DelegatedEventLoop (&EventCompleteSvcArgs);
}

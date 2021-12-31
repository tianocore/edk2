/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms

Copyright (c) 2017 - 2021, Arm Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/Arm/StandaloneMmCoreEntryPoint.h>

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

PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT  CpuDriverEntryPoint = NULL;

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
  UINTN       SvcStatus;

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

    FfaEnabled = FeaturePcdGet (PcdFfaEnable);
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

    if (FfaEnabled) {
      EventCompleteSvcArgs->Arg0 = ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP;
      EventCompleteSvcArgs->Arg1 = 0;
      EventCompleteSvcArgs->Arg2 = 0;
      EventCompleteSvcArgs->Arg3 = ARM_SVC_ID_SP_EVENT_COMPLETE;
      EventCompleteSvcArgs->Arg4 = SvcStatus;
    } else {
      EventCompleteSvcArgs->Arg0 = ARM_SVC_ID_SP_EVENT_COMPLETE;
      EventCompleteSvcArgs->Arg1 = SvcStatus;
    }
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
  Initialize parameters to be sent via SVC call.

  @param[out]     InitMmFoundationSvcArgs  Args structure
  @param[out]     Ret                      Return Code

**/
STATIC
VOID
InitArmSvcArgs (
  OUT ARM_SVC_ARGS  *InitMmFoundationSvcArgs,
  OUT INT32         *Ret
  )
{
  if (FeaturePcdGet (PcdFfaEnable)) {
    InitMmFoundationSvcArgs->Arg0 = ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP;
    InitMmFoundationSvcArgs->Arg1 = 0;
    InitMmFoundationSvcArgs->Arg2 = 0;
    InitMmFoundationSvcArgs->Arg3 = ARM_SVC_ID_SP_EVENT_COMPLETE;
    InitMmFoundationSvcArgs->Arg4 = *Ret;
  } else {
    InitMmFoundationSvcArgs->Arg0 = ARM_SVC_ID_SP_EVENT_COMPLETE;
    InitMmFoundationSvcArgs->Arg1 = *Ret;
  }
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
  ARM_SVC_ARGS                    InitMmFoundationSvcArgs;
  EFI_STATUS                      Status;
  INT32                           Ret;
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
  if (Status == RETURN_UNSUPPORTED) {
    Ret = -1;
  } else if (Status == RETURN_INVALID_PARAMETER) {
    Ret = -2;
  } else if (Status == EFI_NOT_FOUND) {
    Ret = -7;
  } else {
    Ret = 0;
  }

  ZeroMem (&InitMmFoundationSvcArgs, sizeof (InitMmFoundationSvcArgs));
  InitArmSvcArgs (&InitMmFoundationSvcArgs, &Ret);
  DelegatedEventLoop (&InitMmFoundationSvcArgs);
}

/** @file
  CPU PEI Module installs CPU Multiple Processor PPI.

  Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Loongson Technology Corporation Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuMpPei.h"

EFI_PEI_PPI_DESCRIPTOR  mPeiCpuMpPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEdkiiPeiMpServices2PpiGuid,
    &mMpServices2Ppi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMpServicesPpiGuid,
    &mMpServicesPpi
  }
};

//
// Structure for InitializeSeparateExceptionStacks
//
typedef struct {
  VOID          *Buffer;
  UINTN         BufferSize;
  EFI_STATUS    Status;
} EXCEPTION_STACK_SWITCH_CONTEXT;

/**
  Initializes CPU exceptions handlers for the sake of stack switch requirement.

  This function is a wrapper of InitializeSeparateExceptionStacks. It's mainly
  for the sake of AP's init because of EFI_AP_PROCEDURE API requirement.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
InitializeExceptionStackSwitchHandlers (
  IN OUT VOID  *Buffer
  )
{
  EXCEPTION_STACK_SWITCH_CONTEXT  *SwitchStackData;
  UINTN                           Index;
  EFI_STATUS                      Status;

  Status = MpInitLibWhoAmI (&Index);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate Switch Stack pages.\n", __func__));
    return;
  }

  SwitchStackData = (EXCEPTION_STACK_SWITCH_CONTEXT *)Buffer;

  //
  // This function may be called twice for each Cpu. Only run InitializeSeparateExceptionStacks
  // if this is the first call or the first call failed because of size too small.
  //
  if ((SwitchStackData[Index].Status == EFI_NOT_STARTED) || (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL)) {
    SwitchStackData[Index].Status = InitializeSeparateExceptionStacks (SwitchStackData[Index].Buffer, &SwitchStackData[Index].BufferSize);
  }
}

/**
  Initializes MP exceptions handlers for the sake of stack switch requirement.

  This function will allocate required resources required to setup stack switch
  and pass them through SwitchStackData to each logic processor.

**/
VOID
InitializeMpExceptionStackSwitchHandlers (
  VOID
  )
{
  UINTN                           Index;
  UINTN                           NumberOfProcessors;
  EXCEPTION_STACK_SWITCH_CONTEXT  *SwitchStackData;
  UINTN                           BufferSize;
  EFI_STATUS                      Status;
  UINT8                           *Buffer;

  if (!PcdGetBool (PcdCpuStackGuard)) {
    return;
  }

  Status = MpInitLibGetNumberOfProcessors (&NumberOfProcessors, NULL);
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    NumberOfProcessors = 1;
  }

  SwitchStackData = AllocatePages (EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT)));
  if (SwitchStackData == NULL) {
    ASSERT (SwitchStackData != NULL);
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate Switch Stack pages.\n", __func__));
    return;
  }

  ZeroMem (SwitchStackData, NumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT));
  for (Index = 0; Index < NumberOfProcessors; ++Index) {
    //
    // Because the procedure may runs multiple times, use the status EFI_NOT_STARTED
    // to indicate the procedure haven't been run yet.
    //
    SwitchStackData[Index].Status = EFI_NOT_STARTED;
  }

  Status = MpInitLibStartupAllCPUs (
             InitializeExceptionStackSwitchHandlers,
             0,
             SwitchStackData
             );
  ASSERT_EFI_ERROR (Status);

  BufferSize = 0;
  for (Index = 0; Index < NumberOfProcessors; ++Index) {
    if (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL) {
      ASSERT (SwitchStackData[Index].BufferSize != 0);
      BufferSize += SwitchStackData[Index].BufferSize;
    } else {
      ASSERT (SwitchStackData[Index].Status == EFI_SUCCESS);
      ASSERT (SwitchStackData[Index].BufferSize == 0);
    }
  }

  if (BufferSize != 0) {
    Buffer = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
    if (Buffer == NULL) {
      ASSERT (Buffer != NULL);
      DEBUG ((DEBUG_ERROR, "%a - Failed to allocate Buffer pages.\n", __func__));
      return;
    }

    BufferSize = 0;
    for (Index = 0; Index < NumberOfProcessors; ++Index) {
      if (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL) {
        SwitchStackData[Index].Buffer = (VOID *)(&Buffer[BufferSize]);
        BufferSize                   += SwitchStackData[Index].BufferSize;
        DEBUG ((
          DEBUG_INFO,
          "Buffer[cpu%lu] for InitializeExceptionStackSwitchHandlers: 0x%lX with size 0x%lX\n",
          (UINT64)(UINTN)Index,
          (UINT64)(UINTN)SwitchStackData[Index].Buffer,
          (UINT64)(UINTN)SwitchStackData[Index].BufferSize
          ));
      }
    }

    Status = MpInitLibStartupAllCPUs (
               InitializeExceptionStackSwitchHandlers,
               0,
               SwitchStackData
               );
    ASSERT_EFI_ERROR (Status);
    for (Index = 0; Index < NumberOfProcessors; ++Index) {
      ASSERT (SwitchStackData[Index].Status == EFI_SUCCESS);
    }
  }

  FreePages (SwitchStackData, EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT)));
}

/**
  Get CPU core type.

  @param[in, out] Buffer  Argument of the procedure.
**/
VOID
EFIAPI
GetProcessorCoreType (
  IN OUT VOID  *Buffer
  )
{
  EFI_STATUS                               Status;
  UINT8                                    *CoreTypes;
  CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX  NativeModelIdAndCoreTypeEax;
  UINTN                                    ProcessorIndex;

  Status = MpInitLibWhoAmI (&ProcessorIndex);
  ASSERT_EFI_ERROR (Status);

  CoreTypes = (UINT8 *)Buffer;
  AsmCpuidEx (CPUID_HYBRID_INFORMATION, CPUID_HYBRID_INFORMATION_MAIN_LEAF, &NativeModelIdAndCoreTypeEax.Uint32, NULL, NULL, NULL);
  CoreTypes[ProcessorIndex] = (UINT8)NativeModelIdAndCoreTypeEax.Bits.CoreType;
}

/**
  Create gMpInformation2HobGuid.
**/
VOID
BuildMpInformationHob (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     ProcessorIndex;
  UINTN                     NumberOfProcessors;
  UINTN                     NumberOfEnabledProcessors;
  UINTN                     NumberOfProcessorsInHob;
  UINTN                     MaxProcessorsPerHob;
  MP_INFORMATION2_HOB_DATA  *MpInformation2HobData;
  MP_INFORMATION2_ENTRY     *MpInformation2Entry;
  UINTN                     Index;
  UINT8                     *CoreTypes;
  UINT32                    CpuidMaxInput;
  UINTN                     CoreTypePages;

  ProcessorIndex        = 0;
  MpInformation2HobData = NULL;
  MpInformation2Entry   = NULL;
  CoreTypes             = NULL;
  CoreTypePages         = 0;

  Status = MpInitLibGetNumberOfProcessors (&NumberOfProcessors, &NumberOfEnabledProcessors);
  ASSERT_EFI_ERROR (Status);

  //
  // Get Processors CoreType
  //
  AsmCpuid (CPUID_SIGNATURE, &CpuidMaxInput, NULL, NULL, NULL);
  if (CpuidMaxInput >= CPUID_HYBRID_INFORMATION) {
    CoreTypePages = EFI_SIZE_TO_PAGES (sizeof (UINT8) * NumberOfProcessors);
    CoreTypes     = AllocatePages (CoreTypePages);
    ASSERT (CoreTypes != NULL);

    Status = MpInitLibStartupAllCPUs (
               GetProcessorCoreType,
               0,
               (VOID *)CoreTypes
               );
    ASSERT_EFI_ERROR (Status);
  }

  MaxProcessorsPerHob     = ((MAX_UINT16 & ~7) - sizeof (EFI_HOB_GUID_TYPE) - sizeof (MP_INFORMATION2_HOB_DATA)) / sizeof (MP_INFORMATION2_ENTRY);
  NumberOfProcessorsInHob = MaxProcessorsPerHob;

  //
  // Create MP_INFORMATION2_HOB. when the max HobLength 0xFFF8 is not enough, there
  // will be a MP_INFORMATION2_HOB series in the HOB list.
  // In the HOB list, there is a gMpInformation2HobGuid with 0 value NumberOfProcessors
  // fields to indicate it's the last MP_INFORMATION2_HOB.
  //
  while (NumberOfProcessorsInHob != 0) {
    NumberOfProcessorsInHob = MIN (NumberOfProcessors - ProcessorIndex, MaxProcessorsPerHob);
    MpInformation2HobData   = BuildGuidHob (
                                &gMpInformation2HobGuid,
                                sizeof (MP_INFORMATION2_HOB_DATA) + sizeof (MP_INFORMATION2_ENTRY) * NumberOfProcessorsInHob
                                );
    ASSERT (MpInformation2HobData != NULL);

    MpInformation2HobData->Version            = MP_INFORMATION2_HOB_REVISION;
    MpInformation2HobData->ProcessorIndex     = ProcessorIndex;
    MpInformation2HobData->NumberOfProcessors = (UINT16)NumberOfProcessorsInHob;
    MpInformation2HobData->EntrySize          = sizeof (MP_INFORMATION2_ENTRY);

    DEBUG ((DEBUG_INFO, "Creating MpInformation2 HOB...\n"));

    for (Index = 0; Index < NumberOfProcessorsInHob; Index++) {
      MpInformation2Entry = &MpInformation2HobData->Entry[Index];
      Status              = MpInitLibGetProcessorInfo (
                              (Index + ProcessorIndex) | CPU_V2_EXTENDED_TOPOLOGY,
                              &MpInformation2Entry->ProcessorInfo,
                              NULL
                              );
      ASSERT_EFI_ERROR (Status);

      MpInformation2Entry->CoreType = (CoreTypes != NULL) ? CoreTypes[Index + ProcessorIndex] : 0;

      DEBUG ((
        DEBUG_INFO,
        "  Processor[%04d]: ProcessorId = 0x%lx, StatusFlag = 0x%x, CoreType = 0x%x\n",
        Index + ProcessorIndex,
        MpInformation2Entry->ProcessorInfo.ProcessorId,
        MpInformation2Entry->ProcessorInfo.StatusFlag,
        MpInformation2Entry->CoreType
        ));
      DEBUG ((
        DEBUG_INFO,
        "    Location = Package:%d Core:%d Thread:%d\n",
        MpInformation2Entry->ProcessorInfo.Location.Package,
        MpInformation2Entry->ProcessorInfo.Location.Core,
        MpInformation2Entry->ProcessorInfo.Location.Thread
        ));
      DEBUG ((
        DEBUG_INFO,
        "    Location2 = Package:%d Die:%d Tile:%d Module:%d Core:%d Thread:%d\n",
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Package,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Die,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Tile,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Module,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Core,
        MpInformation2Entry->ProcessorInfo.ExtendedInformation.Location2.Thread
        ));
    }

    ProcessorIndex += NumberOfProcessorsInHob;
  }

  if (CoreTypes != NULL) {
    FreePages (CoreTypes, CoreTypePages);
  }
}

/**
  Initializes MP and exceptions handlers.

  @param  PeiServices                The pointer to the PEI Services Table.

  @retval EFI_SUCCESS     MP was successfully initialized.
  @retval others          Error occurred in MP initialization.

**/
EFI_STATUS
InitializeCpuMpWorker (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_VECTOR_HANDOFF_INFO          *VectorInfo;
  EFI_PEI_VECTOR_HANDOFF_INFO_PPI  *VectorHandoffInfoPpi;

  //
  // Get Vector Hand-off Info PPI
  //
  VectorInfo = NULL;
  Status     = PeiServicesLocatePpi (
                 &gEfiVectorHandoffInfoPpiGuid,
                 0,
                 NULL,
                 (VOID **)&VectorHandoffInfoPpi
                 );
  if (Status == EFI_SUCCESS) {
    VectorInfo = VectorHandoffInfoPpi->Info;
  }

  //
  // Initialize default handlers
  //
  Status = InitializeCpuExceptionHandlers (VectorInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MpInitLibInitialize ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Special initialization for the sake of Stack Guard
  //
  InitializeMpExceptionStackSwitchHandlers ();

  //
  // Update and publish CPU BIST information
  //
  CollectBistDataFromPpi (PeiServices);

  //
  // Install CPU MP PPI
  //
  Status = PeiServicesInstallPpi (mPeiCpuMpPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Create gMpInformation2HobGuid
  //
  BuildMpInformationHob ();

  return Status;
}

/**
  The Entry point of the MP CPU PEIM.

  This function will wakeup APs and collect CPU AP count and install the
  Mp Service Ppi.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   MpServicePpi is installed successfully.

**/
EFI_STATUS
EFIAPI
CpuMpPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  //
  // For the sake of special initialization needing to be done right after
  // memory discovery.
  //
  Status = PeiServicesNotifyPpi (&mPostMemNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

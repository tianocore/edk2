/** @file
  This driver is used for SMM S3 support for the bootloader that
  doesn't support SMM.
  The payload would save SMM rebase info to SMM communication area.
  The bootloader is expected to rebase the SMM and trigger SMI by
  writting 0xB2 port with given value from SMM communication area.
  The paylaod SMM handler got chance to restore regs in S3 path.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <BlSupportSmm.h>

PLD_S3_COMMUNICATION            mPldS3Hob;
EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *mSmramHob         = NULL;
PLD_SMM_REGISTERS               *mSmmRegisterHob   = NULL;
UINT64                          mSmmFeatureControl = 0;

/**
  Save SMM rebase and SMI handler information to SMM communication area

  The function detects SMM communication region for boot loader, if it is detected, it
  will save SMM rebase information and S3 SMI handler information to SMM communication
  region. Bootloader should consume these information in S3 path to restore smm base,
  and write the 0xB2 port to trigger SMI so that payload could resume S3 registers.

  @param[in] BlSwSmiHandlerInput   Value written to 0xB2 to trigger SMI handler.

  @retval    EFI_SUCCESS           Save SMM info success.
  @retval    Others                Failed to save SMM info.
**/
EFI_STATUS
SaveSmmInfoForS3 (
  IN UINT8  BlSwSmiHandlerInput
  )
{
  EFI_STATUS                 Status;
  EFI_PROCESSOR_INFORMATION  ProcessorInfo;
  EFI_MP_SERVICES_PROTOCOL   *MpService;
  CPU_SMMBASE                *SmmBaseInfo;
  PLD_TO_BL_SMM_INFO         *PldSmmInfo;
  UINTN                      Index;

  PldSmmInfo                          = (PLD_TO_BL_SMM_INFO *)(UINTN)mPldS3Hob.CommBuffer.PhysicalStart;
  PldSmmInfo->Header.Header.HobLength = (UINT16)(sizeof (PLD_TO_BL_SMM_INFO) + gSmst->NumberOfCpus * sizeof (CPU_SMMBASE));
  for (Index = 0; Index < mSmramHob->NumberOfSmmReservedRegions; Index++) {
    if ((mPldS3Hob.CommBuffer.PhysicalStart >= mSmramHob->Descriptor[Index].PhysicalStart) &&
        (mPldS3Hob.CommBuffer.PhysicalStart <  mSmramHob->Descriptor[Index].PhysicalStart + mSmramHob->Descriptor[Index].PhysicalSize))
    {
      break;
    }
  }

  if (Index == mSmramHob->NumberOfSmmReservedRegions) {
    return EFI_NOT_FOUND;
  }

  //
  // Make sure the dedicated region for SMM info communication whose attribute is "allocated" (i.e., excluded from SMM memory service)
  //
  if ((mSmramHob->Descriptor[Index].RegionState & EFI_ALLOCATED) == 0) {
    DEBUG ((DEBUG_ERROR, "SMM communication region not set to EFI_ALLOCATED\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (((UINTN)PldSmmInfo + PldSmmInfo->Header.Header.HobLength) > (mSmramHob->Descriptor[Index].PhysicalStart + mSmramHob->Descriptor[Index].PhysicalSize)) {
    DEBUG ((DEBUG_ERROR, "SMM communication buffer (0x%x) is too small.\n", (UINTN)PldSmmInfo + PldSmmInfo->Header.Header.HobLength));
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyGuid (&PldSmmInfo->Header.Name, &gS3CommunicationGuid);
  PldSmmInfo->Header.Header.HobType    = EFI_HOB_TYPE_GUID_EXTENSION;
  PldSmmInfo->S3Info.SwSmiTriggerValue = BlSwSmiHandlerInput;

  //
  // Save APIC ID and SMM base
  //
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpService);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PldSmmInfo->S3Info.CpuCount = (UINT32)gSmst->NumberOfCpus;
  SmmBaseInfo                 = &PldSmmInfo->S3Info.SmmBase[0];
  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    Status = MpService->GetProcessorInfo (MpService, Index, &ProcessorInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    SmmBaseInfo->ApicId  = (UINT32)(UINTN)ProcessorInfo.ProcessorId;
    SmmBaseInfo->SmmBase = (UINT32)(UINTN)gSmst->CpuSaveState[Index] - SMRAM_SAVE_STATE_MAP_OFFSET;
    DEBUG ((DEBUG_INFO, "CPU%d ID:%02X Base: %08X\n", Index, SmmBaseInfo->ApicId, SmmBaseInfo->SmmBase));
    SmmBaseInfo++;
  }

  return EFI_SUCCESS;
}

/**
  Get specified SMI register based on given register ID

  @param[in]  Id           The register ID to get.

  @retval NULL             The register is not found
  @return smi register

**/
PLD_GENERIC_REGISTER *
GetRegisterById (
  UINT64  Id
  )
{
  UINT32  Index;

  for (Index = 0; Index < mSmmRegisterHob->Count; Index++) {
    if (mSmmRegisterHob->Registers[Index].Id == Id) {
      return &mSmmRegisterHob->Registers[Index];
    }
  }

  return NULL;
}

/**
  Set SMM SMI Global enable lock

**/
VOID
LockSmiGlobalEn (
  VOID
  )
{
  PLD_GENERIC_REGISTER  *SmiLockReg;

  DEBUG ((DEBUG_ERROR, "LockSmiGlobalEn .....\n"));

  SmiLockReg = GetRegisterById (REGISTER_ID_SMI_GBL_EN_LOCK);
  if (SmiLockReg == NULL) {
    DEBUG ((DEBUG_ERROR, "SMI global enable lock reg not found.\n"));
    return;
  }

  //
  // Set SMM SMI lock in S3 path
  //
  if ((SmiLockReg->Address.AccessSize       == EFI_ACPI_3_0_DWORD) &&
      (SmiLockReg->Address.Address          != 0) &&
      (SmiLockReg->Address.RegisterBitWidth == 1) &&
      (SmiLockReg->Address.AddressSpaceId   == EFI_ACPI_3_0_SYSTEM_MEMORY) &&
      (SmiLockReg->Value == 1))
  {
    DEBUG ((DEBUG_ERROR, "LockSmiGlobalEn ....is locked\n"));

    MmioOr32 ((UINT32)SmiLockReg->Address.Address, 1 << SmiLockReg->Address.RegisterBitOffset);
  } else {
    DEBUG ((DEBUG_ERROR, "Unexpected SMM SMI lock register, need enhancement here.\n"));
  }
}

/**
  Check and set SMM feature lock bit and code check enable bit
  in S3 path.

**/
VOID
SmmFeatureLockOnS3 (
  VOID
  )
{
  if (mSmmFeatureControl != 0) {
    return;
  }

  mSmmFeatureControl = AsmReadMsr64 (MSR_SMM_FEATURE_CONTROL);
  if ((mSmmFeatureControl & 0x5) != 0x5) {
    //
    // Set Lock bit [BIT0] for this register and SMM code check enable bit [BIT2]
    //
    AsmWriteMsr64 (MSR_SMM_FEATURE_CONTROL, mSmmFeatureControl | 0x5);
  }

  mSmmFeatureControl = AsmReadMsr64 (MSR_SMM_FEATURE_CONTROL);
}

/**
  Function to program SMRR base and mask.

  @param[in] ProcedureArgument  Pointer to SMRR_BASE_MASK structure.
**/
VOID
EFIAPI
SetSmrr (
  IN VOID  *ProcedureArgument
  )
{
  if (ProcedureArgument != NULL) {
    AsmWriteMsr64 (MSR_IA32_SMRR_PHYSBASE, ((SMRR_BASE_MASK *)ProcedureArgument)->Base);
    AsmWriteMsr64 (MSR_IA32_SMRR_PHYSMASK, ((SMRR_BASE_MASK *)ProcedureArgument)->Mask);
  }
}

/**
  Set SMRR in S3 path.

**/
VOID
SetSmrrOnS3 (
  VOID
  )
{
  EFI_STATUS      Status;
  SMRR_BASE_MASK  Arguments;
  UINTN           Index;
  UINT32          SmmBase;
  UINT32          SmmSize;

  if ((AsmReadMsr64 (MSR_IA32_SMRR_PHYSBASE) != 0) && ((AsmReadMsr64 (MSR_IA32_SMRR_PHYSMASK) & BIT11) != 0)) {
    return;
  }

  SmmBase = (UINT32)(UINTN)mSmramHob->Descriptor[0].PhysicalStart;
  SmmSize = (UINT32)(UINTN)mSmramHob->Descriptor[0].PhysicalSize;
  if ((mSmramHob->NumberOfSmmReservedRegions > 2) || (mSmramHob->NumberOfSmmReservedRegions == 0)) {
    DEBUG ((DEBUG_ERROR, "%d SMM ranges are not supported.\n", mSmramHob->NumberOfSmmReservedRegions));
    return;
  } else if (mSmramHob->NumberOfSmmReservedRegions == 2) {
    if ((mSmramHob->Descriptor[1].PhysicalStart + mSmramHob->Descriptor[1].PhysicalSize) == SmmBase) {
      SmmBase = (UINT32)(UINTN)mSmramHob->Descriptor[1].PhysicalStart;
    } else if (mSmramHob->Descriptor[1].PhysicalStart != (SmmBase + SmmSize)) {
      DEBUG ((DEBUG_ERROR, "Two SMM regions are not continous.\n"));
      return;
    }

    SmmSize += (UINT32)(UINTN)mSmramHob->Descriptor[1].PhysicalSize;
  }

  if ((SmmBase == 0) || (SmmSize < SIZE_4KB)) {
    DEBUG ((DEBUG_ERROR, "Invalid SMM range.\n"));
    return;
  }

  //
  // SMRR size must be of length 2^n
  // SMRR base alignment cannot be less than SMRR length
  //
  if ((SmmSize != GetPowerOfTwo32 (SmmSize)) || ((SmmBase & ~(SmmSize - 1)) != SmmBase)) {
    DEBUG ((DEBUG_ERROR, " Invalid SMM range.\n"));
    return;
  }

  //
  // Calculate smrr base, mask and pass them as arguments.
  //
  Arguments.Base = (SmmSize | MTRR_CACHE_WRITE_BACK);
  Arguments.Mask = (~(SmmSize - 1) & EFI_MSR_SMRR_MASK);

  //
  // Set SMRR valid bit
  //
  Arguments.Mask |= BIT11;

  //
  // Program smrr base and mask on BSP first and then on APs
  //
  SetSmrr (&Arguments);
  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    if (Index != gSmst->CurrentlyExecutingCpu) {
      Status = gSmst->SmmStartupThisAp (SetSmrr, Index, (VOID *)&Arguments);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Programming SMRR on AP# %d status: %r\n", Index, Status));
      }
    }
  }
}

/**
  Software SMI callback for restoring SMRR base and mask in S3 path.

  @param[in]      DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]      Context         Points to an optional handler context which was specified when the
                                  handler was registered.
  @param[in, out] CommBuffer      A pointer to a collection of data in memory that will
                                  be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS             The interrupt was handled successfully.

**/
EFI_STATUS
EFIAPI
BlSwSmiHandler (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context,
  IN OUT VOID    *CommBuffer,
  IN OUT UINTN   *CommBufferSize
  )
{
  SetSmrrOnS3 ();
  SmmFeatureLockOnS3 ();
  LockSmiGlobalEn ();

  return EFI_SUCCESS;
}

/**
  Lock SMI in this SMM ready to lock event.

  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   SmmEventCallback runs successfully
  @retval EFI_NOT_FOUND The Fvb protocol for variable is not found.
 **/
EFI_STATUS
EFIAPI
BlSupportSmmReadyToLockCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  //
  // Set SMM SMI lock
  //
  LockSmiGlobalEn ();
  return EFI_SUCCESS;
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BlSupportSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT    SwContext;
  EFI_HANDLE                     SwHandle;
  EFI_HOB_GUID_TYPE              *GuidHob;
  VOID                           *SmmHob;
  VOID                           *Registration;

  //
  // Get SMM S3 communication hob and save it
  //
  GuidHob = GetFirstGuidHob (&gS3CommunicationGuid);
  if (GuidHob != NULL) {
    SmmHob = (VOID *)(GET_GUID_HOB_DATA (GuidHob));
    CopyMem (&mPldS3Hob, SmmHob, GET_GUID_HOB_DATA_SIZE (GuidHob));
  } else {
    return EFI_NOT_FOUND;
  }

  if (mPldS3Hob.PldAcpiS3Enable) {
    // Other drivers will take care of S3.
    return EFI_SUCCESS;
  }

  //
  // Get smram hob and save it
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  if (GuidHob != NULL) {
    SmmHob    = (VOID *)(GET_GUID_HOB_DATA (GuidHob));
    mSmramHob = AllocatePool (GET_GUID_HOB_DATA_SIZE (GuidHob));
    if (mSmramHob == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (mSmramHob, SmmHob, GET_GUID_HOB_DATA_SIZE (GuidHob));
  } else {
    return EFI_NOT_FOUND;
  }

  //
  // Get SMM register hob and save it
  //
  GuidHob = GetFirstGuidHob (&gSmmRegisterInfoGuid);
  if (GuidHob != NULL) {
    SmmHob          = (VOID *)(GET_GUID_HOB_DATA (GuidHob));
    mSmmRegisterHob = AllocatePool (GET_GUID_HOB_DATA_SIZE (GuidHob));
    if (mSmmRegisterHob == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (mSmmRegisterHob, SmmHob, GET_GUID_HOB_DATA_SIZE (GuidHob));
  } else {
    return EFI_NOT_FOUND;
  }

  //
  // Get the Sw dispatch protocol and register SMI handler.
  //
  Status = gSmst->SmmLocateProtocol (&gEfiSmmSwDispatch2ProtocolGuid, NULL, (VOID **)&SwDispatch);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SwContext.SwSmiInputValue = (UINTN)-1;
  Status                    = SwDispatch->Register (SwDispatch, BlSwSmiHandler, &SwContext, &SwHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Registering S3 smi handler failed: %r\n", Status));
    return Status;
  }

  //
  // Register SMM ready to lock callback
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmReadyToLockProtocolGuid,
                    BlSupportSmmReadyToLockCallback,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  SaveSmmInfoForS3 ((UINT8)SwContext.SwSmiInputValue);

  return EFI_SUCCESS;
}

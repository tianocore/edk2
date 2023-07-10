/** @file
Implementation shared across all library instances.

Copyright (c) 2010 - 2023, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuFeaturesLib.h"

#include <Library/MtrrLib.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/SmramSaveStateMap.h>

//
// Machine Specific Registers (MSRs)
//
#define  SMM_FEATURES_LIB_IA32_MTRR_CAP            0x0FE
#define  SMM_FEATURES_LIB_IA32_FEATURE_CONTROL     0x03A
#define  SMM_FEATURES_LIB_IA32_SMRR_PHYSBASE       0x1F2
#define  SMM_FEATURES_LIB_IA32_SMRR_PHYSMASK       0x1F3
#define  SMM_FEATURES_LIB_IA32_CORE_SMRR_PHYSBASE  0x0A0
#define  SMM_FEATURES_LIB_IA32_CORE_SMRR_PHYSMASK  0x0A1
#define    EFI_MSR_SMRR_MASK                       0xFFFFF000
#define    EFI_MSR_SMRR_PHYS_MASK_VALID            BIT11
#define  SMM_FEATURES_LIB_SMM_FEATURE_CONTROL      0x4E0

//
// MSRs required for configuration of SMM Code Access Check
//
#define SMM_FEATURES_LIB_IA32_MCA_CAP  0x17D
#define   SMM_CODE_ACCESS_CHK_BIT      BIT58

//
// Set default value to assume IA-32 Architectural MSRs are used
//
UINT32  mSmrrPhysBaseMsr = SMM_FEATURES_LIB_IA32_SMRR_PHYSBASE;
UINT32  mSmrrPhysMaskMsr = SMM_FEATURES_LIB_IA32_SMRR_PHYSMASK;

//
// Indicate SmBase for each Processors has been relocated or not. If TRUE,
// means no need to do the relocation in SmmCpuFeaturesInitializeProcessor().
//
BOOLEAN  mSmmCpuFeaturesSmmRelocated;

//
// Set default value to assume MTRRs need to be configured on each SMI
//
BOOLEAN  mNeedConfigureMtrrs = TRUE;

//
// Array for state of SMRR enable on all CPUs
//
BOOLEAN  *mSmrrEnabled;

/**
  Performs library initialization.

  This initialization function contains common functionality shared betwen all
  library instance constructors.

**/
VOID
CpuFeaturesLibInitialization (
  VOID
  )
{
  UINT32  RegEax;
  UINT32  RegEdx;
  UINTN   FamilyId;
  UINTN   ModelId;

  //
  // Retrieve CPU Family and Model
  //
  AsmCpuid (CPUID_VERSION_INFO, &RegEax, NULL, NULL, &RegEdx);
  FamilyId = (RegEax >> 8) & 0xf;
  ModelId  = (RegEax >> 4) & 0xf;
  if ((FamilyId == 0x06) || (FamilyId == 0x0f)) {
    ModelId = ModelId | ((RegEax >> 12) & 0xf0);
  }

  //
  // Check CPUID(CPUID_VERSION_INFO).EDX[12] for MTRR capability
  //
  if ((RegEdx & BIT12) != 0) {
    //
    // Check MTRR_CAP MSR bit 11 for SMRR support
    //
    if ((AsmReadMsr64 (SMM_FEATURES_LIB_IA32_MTRR_CAP) & BIT11) != 0) {
      ASSERT (FeaturePcdGet (PcdSmrrEnable));
    }
  }

  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 35.3 MSRs in the Intel(R) Atom(TM) Processor Family
  //
  // If CPU Family/Model is 06_1CH, 06_26H, 06_27H, 06_35H or 06_36H, then
  // SMRR Physical Base and SMM Physical Mask MSRs are not available.
  //
  if (FamilyId == 0x06) {
    if ((ModelId == 0x1C) || (ModelId == 0x26) || (ModelId == 0x27) || (ModelId == 0x35) || (ModelId == 0x36)) {
      ASSERT (!FeaturePcdGet (PcdSmrrEnable));
    }
  }

  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 35.2 MSRs in the Intel(R) Core(TM) 2 Processor Family
  //
  // If CPU Family/Model is 06_0F or 06_17, then use Intel(R) Core(TM) 2
  // Processor Family MSRs
  //
  if (FamilyId == 0x06) {
    if ((ModelId == 0x17) || (ModelId == 0x0f)) {
      mSmrrPhysBaseMsr = SMM_FEATURES_LIB_IA32_CORE_SMRR_PHYSBASE;
      mSmrrPhysMaskMsr = SMM_FEATURES_LIB_IA32_CORE_SMRR_PHYSMASK;
    }
  }

  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 34.4.2 SMRAM Caching
  //   An IA-32 processor does not automatically write back and invalidate its
  //   caches before entering SMM or before exiting SMM. Because of this behavior,
  //   care must be taken in the placement of the SMRAM in system memory and in
  //   the caching of the SMRAM to prevent cache incoherence when switching back
  //   and forth between SMM and protected mode operation.
  //
  // An IA-32 processor is a processor that does not support the Intel 64
  // Architecture.  Support for the Intel 64 Architecture can be detected from
  // CPUID(CPUID_EXTENDED_CPU_SIG).EDX[29]
  //
  // If an IA-32 processor is detected, then set mNeedConfigureMtrrs to TRUE,
  // so caches are flushed on SMI entry and SMI exit, the interrupted code
  // MTRRs are saved/restored, and MTRRs for SMM are loaded.
  //
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_EXTENDED_CPU_SIG) {
    AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & BIT29) != 0) {
      mNeedConfigureMtrrs = FALSE;
    }
  }

  //
  // Allocate array for state of SMRR enable on all CPUs
  //
  mSmrrEnabled = (BOOLEAN *)AllocatePool (sizeof (BOOLEAN) * GetCpuMaxLogicalProcessorNumber ());
  ASSERT (mSmrrEnabled != NULL);

  //
  // If gSmmBaseHobGuid found, means SmBase info has been relocated and recorded
  // in the SmBase array.
  //
  mSmmCpuFeaturesSmmRelocated = (BOOLEAN)(GetFirstGuidHob (&gSmmBaseHobGuid) != NULL);
}

/**
  Called during the very first SMI into System Management Mode to initialize
  CPU features, including SMBASE, for the currently executing CPU.  Since this
  is the first SMI, the SMRAM Save State Map is at the default address of
  SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET.  The currently executing
  CPU is specified by CpuIndex and CpuIndex can be used to access information
  about the currently executing CPU in the ProcessorInfo array and the
  HotPlugCpuData data structure.

  @param[in] CpuIndex        The index of the CPU to initialize.  The value
                             must be between 0 and the NumberOfCpus field in
                             the System Management System Table (SMST).
  @param[in] IsMonarch       TRUE if the CpuIndex is the index of the CPU that
                             was elected as monarch during System Management
                             Mode initialization.
                             FALSE if the CpuIndex is not the index of the CPU
                             that was elected as monarch during System
                             Management Mode initialization.
  @param[in] ProcessorInfo   Pointer to an array of EFI_PROCESSOR_INFORMATION
                             structures.  ProcessorInfo[CpuIndex] contains the
                             information for the currently executing CPU.
  @param[in] CpuHotPlugData  Pointer to the CPU_HOT_PLUG_DATA structure that
                             contains the ApidId and SmBase arrays.
**/
VOID
EFIAPI
SmmCpuFeaturesInitializeProcessor (
  IN UINTN                      CpuIndex,
  IN BOOLEAN                    IsMonarch,
  IN EFI_PROCESSOR_INFORMATION  *ProcessorInfo,
  IN CPU_HOT_PLUG_DATA          *CpuHotPlugData
  )
{
  SMRAM_SAVE_STATE_MAP  *CpuState;
  UINT64                FeatureControl;
  UINT32                RegEax;
  UINT32                RegEdx;
  UINTN                 FamilyId;
  UINTN                 ModelId;

  //
  // No need to configure SMBASE if SmBase relocation has been done.
  //
  if (!mSmmCpuFeaturesSmmRelocated) {
    //
    // Configure SMBASE.
    //
    CpuState             = (SMRAM_SAVE_STATE_MAP *)(UINTN)(SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET);
    CpuState->x86.SMBASE = (UINT32)CpuHotPlugData->SmBase[CpuIndex];
  }

  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 35.2 MSRs in the Intel(R) Core(TM) 2 Processor Family
  //
  // If Intel(R) Core(TM) Core(TM) 2 Processor Family MSRs are being used, then
  // make sure SMRR Enable(BIT3) of MSR_FEATURE_CONTROL MSR(0x3A) is set before
  // accessing SMRR base/mask MSRs.  If Lock(BIT0) of MSR_FEATURE_CONTROL MSR(0x3A)
  // is set, then the MSR is locked and can not be modified.
  //
  if ((FeaturePcdGet (PcdSmrrEnable)) && (mSmrrPhysBaseMsr == SMM_FEATURES_LIB_IA32_CORE_SMRR_PHYSBASE)) {
    FeatureControl = AsmReadMsr64 (SMM_FEATURES_LIB_IA32_FEATURE_CONTROL);
    if ((FeatureControl & BIT3) == 0) {
      ASSERT ((FeatureControl & BIT0) == 0);
      if ((FeatureControl & BIT0) == 0) {
        AsmWriteMsr64 (SMM_FEATURES_LIB_IA32_FEATURE_CONTROL, FeatureControl | BIT3);
      }
    }
  }

  //
  // If SMRR is supported, then program SMRR base/mask MSRs.
  // The EFI_MSR_SMRR_PHYS_MASK_VALID bit is not set until the first normal SMI.
  // The code that initializes SMM environment is running in normal mode
  // from SMRAM region.  If SMRR is enabled here, then the SMRAM region
  // is protected and the normal mode code execution will fail.
  //
  if (FeaturePcdGet (PcdSmrrEnable)) {
    //
    // SMRR size cannot be less than 4-KBytes
    // SMRR size must be of length 2^n
    // SMRR base alignment cannot be less than SMRR length
    //
    if ((CpuHotPlugData->SmrrSize < SIZE_4KB) ||
        (CpuHotPlugData->SmrrSize != GetPowerOfTwo32 (CpuHotPlugData->SmrrSize)) ||
        ((CpuHotPlugData->SmrrBase & ~(CpuHotPlugData->SmrrSize - 1)) != CpuHotPlugData->SmrrBase))
    {
      //
      // Print message and halt if CPU is Monarch
      //
      if (IsMonarch) {
        DEBUG ((DEBUG_ERROR, "SMM Base/Size does not meet alignment/size requirement!\n"));
        CpuDeadLoop ();
      }
    } else {
      AsmWriteMsr64 (mSmrrPhysBaseMsr, CpuHotPlugData->SmrrBase | MTRR_CACHE_WRITE_BACK);
      AsmWriteMsr64 (mSmrrPhysMaskMsr, (~(CpuHotPlugData->SmrrSize - 1) & EFI_MSR_SMRR_MASK));
      mSmrrEnabled[CpuIndex] = FALSE;
    }
  }

  //
  // Retrieve CPU Family and Model
  //
  AsmCpuid (CPUID_VERSION_INFO, &RegEax, NULL, NULL, &RegEdx);
  FamilyId = (RegEax >> 8) & 0xf;
  ModelId  = (RegEax >> 4) & 0xf;
  if ((FamilyId == 0x06) || (FamilyId == 0x0f)) {
    ModelId = ModelId | ((RegEax >> 12) & 0xf0);
  }

  //
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  // Volume 3C, Section 35.10.1 MSRs in 4th Generation Intel(R) Core(TM)
  // Processor Family.
  //
  // If CPU Family/Model is 06_3C, 06_45, or 06_46 then use 4th Generation
  // Intel(R) Core(TM) Processor Family MSRs.
  //
  if (FamilyId == 0x06) {
    if ((ModelId == 0x3C) || (ModelId == 0x45) || (ModelId == 0x46) ||
        (ModelId == 0x3D) || (ModelId == 0x47) || (ModelId == 0x4E) || (ModelId == 0x4F) ||
        (ModelId == 0x3F) || (ModelId == 0x56) || (ModelId == 0x57) || (ModelId == 0x5C) ||
        (ModelId == 0x8C))
    {
      //
      // Check to see if the CPU supports the SMM Code Access Check feature
      // Do not access this MSR unless the CPU supports the SmmRegFeatureControl
      //
      if ((AsmReadMsr64 (SMM_FEATURES_LIB_IA32_MCA_CAP) & SMM_CODE_ACCESS_CHK_BIT) != 0) {
        ASSERT (FeaturePcdGet (PcdSmmFeatureControlEnable));
      }
    }
  }

  //
  //  Call internal worker function that completes the CPU initialization
  //
  FinishSmmCpuFeaturesInitializeProcessor ();
}

/**
  Determines if MTRR registers must be configured to set SMRAM cache-ability
  when executing in System Management Mode.

  @retval TRUE   MTRR registers must be configured to set SMRAM cache-ability.
  @retval FALSE  MTRR registers do not need to be configured to set SMRAM
                 cache-ability.
**/
BOOLEAN
EFIAPI
SmmCpuFeaturesNeedConfigureMtrrs (
  VOID
  )
{
  return mNeedConfigureMtrrs;
}

/**
  Disable SMRR register if SMRR is supported and SmmCpuFeaturesNeedConfigureMtrrs()
  returns TRUE.
**/
VOID
EFIAPI
SmmCpuFeaturesDisableSmrr (
  VOID
  )
{
  if (FeaturePcdGet (PcdSmrrEnable) && mNeedConfigureMtrrs) {
    AsmWriteMsr64 (mSmrrPhysMaskMsr, AsmReadMsr64 (mSmrrPhysMaskMsr) & ~EFI_MSR_SMRR_PHYS_MASK_VALID);
  }
}

/**
  Enable SMRR register if SMRR is supported and SmmCpuFeaturesNeedConfigureMtrrs()
  returns TRUE.
**/
VOID
EFIAPI
SmmCpuFeaturesReenableSmrr (
  VOID
  )
{
  if (FeaturePcdGet (PcdSmrrEnable) && mNeedConfigureMtrrs) {
    AsmWriteMsr64 (mSmrrPhysMaskMsr, AsmReadMsr64 (mSmrrPhysMaskMsr) | EFI_MSR_SMRR_PHYS_MASK_VALID);
  }
}

/**
  Processor specific hook point each time a CPU enters System Management Mode.

  @param[in] CpuIndex  The index of the CPU that has entered SMM.  The value
                       must be between 0 and the NumberOfCpus field in the
                       System Management System Table (SMST).
**/
VOID
EFIAPI
SmmCpuFeaturesRendezvousEntry (
  IN UINTN  CpuIndex
  )
{
  //
  // If SMRR is supported and this is the first normal SMI, then enable SMRR
  //
  if (FeaturePcdGet (PcdSmrrEnable) && !mSmrrEnabled[CpuIndex]) {
    AsmWriteMsr64 (mSmrrPhysMaskMsr, AsmReadMsr64 (mSmrrPhysMaskMsr) | EFI_MSR_SMRR_PHYS_MASK_VALID);
    mSmrrEnabled[CpuIndex] = TRUE;
  }
}

/**
  Returns the current value of the SMM register for the specified CPU.
  If the SMM register is not supported, then 0 is returned.

  @param[in] CpuIndex  The index of the CPU to read the SMM register.  The
                       value must be between 0 and the NumberOfCpus field in
                       the System Management System Table (SMST).
  @param[in] RegName   Identifies the SMM register to read.

  @return  The value of the SMM register specified by RegName from the CPU
           specified by CpuIndex.
**/
UINT64
EFIAPI
SmmCpuFeaturesGetSmmRegister (
  IN UINTN         CpuIndex,
  IN SMM_REG_NAME  RegName
  )
{
  if (FeaturePcdGet (PcdSmmFeatureControlEnable) && (RegName == SmmRegFeatureControl)) {
    return AsmReadMsr64 (SMM_FEATURES_LIB_SMM_FEATURE_CONTROL);
  }

  return 0;
}

/**
  Sets the value of an SMM register on a specified CPU.
  If the SMM register is not supported, then no action is performed.

  @param[in] CpuIndex  The index of the CPU to write the SMM register.  The
                       value must be between 0 and the NumberOfCpus field in
                       the System Management System Table (SMST).
  @param[in] RegName   Identifies the SMM register to write.
                       registers are read-only.
  @param[in] Value     The value to write to the SMM register.
**/
VOID
EFIAPI
SmmCpuFeaturesSetSmmRegister (
  IN UINTN         CpuIndex,
  IN SMM_REG_NAME  RegName,
  IN UINT64        Value
  )
{
  if (FeaturePcdGet (PcdSmmFeatureControlEnable) && (RegName == SmmRegFeatureControl)) {
    AsmWriteMsr64 (SMM_FEATURES_LIB_SMM_FEATURE_CONTROL, Value);
  }
}

/**
  This function updates the SMRAM save state on the currently executing CPU
  to resume execution at a specific address after an RSM instruction.  This
  function must evaluate the SMRAM save state to determine the execution mode
  the RSM instruction resumes and update the resume execution address with
  either NewInstructionPointer32 or NewInstructionPoint.  The auto HALT restart
  flag in the SMRAM save state must always be cleared.  This function returns
  the value of the instruction pointer from the SMRAM save state that was
  replaced.  If this function returns 0, then the SMRAM save state was not
  modified.

  This function is called during the very first SMI on each CPU after
  SmmCpuFeaturesInitializeProcessor() to set a flag in normal execution mode
  to signal that the SMBASE of each CPU has been updated before the default
  SMBASE address is used for the first SMI to the next CPU.

  @param[in] CpuIndex                 The index of the CPU to hook.  The value
                                      must be between 0 and the NumberOfCpus
                                      field in the System Management System Table
                                      (SMST).
  @param[in] CpuState                 Pointer to SMRAM Save State Map for the
                                      currently executing CPU.
  @param[in] NewInstructionPointer32  Instruction pointer to use if resuming to
                                      32-bit execution mode from 64-bit SMM.
  @param[in] NewInstructionPointer    Instruction pointer to use if resuming to
                                      same execution mode as SMM.

  @retval 0    This function did modify the SMRAM save state.
  @retval > 0  The original instruction pointer value from the SMRAM save state
               before it was replaced.
**/
UINT64
EFIAPI
SmmCpuFeaturesHookReturnFromSmm (
  IN UINTN                 CpuIndex,
  IN SMRAM_SAVE_STATE_MAP  *CpuState,
  IN UINT64                NewInstructionPointer32,
  IN UINT64                NewInstructionPointer
  )
{
  return 0;
}

/**
  Check to see if an SMM register is supported by a specified CPU.

  @param[in] CpuIndex  The index of the CPU to check for SMM register support.
                       The value must be between 0 and the NumberOfCpus field
                       in the System Management System Table (SMST).
  @param[in] RegName   Identifies the SMM register to check for support.

  @retval TRUE   The SMM register specified by RegName is supported by the CPU
                 specified by CpuIndex.
  @retval FALSE  The SMM register specified by RegName is not supported by the
                 CPU specified by CpuIndex.
**/
BOOLEAN
EFIAPI
SmmCpuFeaturesIsSmmRegisterSupported (
  IN UINTN         CpuIndex,
  IN SMM_REG_NAME  RegName
  )
{
  if (FeaturePcdGet (PcdSmmFeatureControlEnable) && (RegName == SmmRegFeatureControl)) {
    return TRUE;
  }

  return FALSE;
}

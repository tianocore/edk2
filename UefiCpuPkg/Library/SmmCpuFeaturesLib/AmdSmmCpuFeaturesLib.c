/** @file
Implementation specific to the SmmCpuFeatureLib library instance
for AMD based platforms.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/SmmBaseHob.h>
#include <Library/SmmCpuFeaturesLib.h>
#include <Uefi/UefiBaseType.h>
#include <Register/Amd/SmramSaveStateMap.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MmSaveStateLib.h>
#include <Library/HobLib.h>

// EFER register LMA bit
#define LMA  BIT10

// Machine Specific Registers (MSRs)
#define SMMADDR_ADDRESS  0xC0010112ul
#define SMMMASK_ADDRESS  0xC0010113ul
#define EFER_ADDRESS     0XC0000080ul

// The mode of the CPU at the time an SMI occurs
STATIC UINT8  mSmmSaveStateRegisterLma;

//
// Indicate SmBase for each Processors has been relocated or not. If TRUE,
// means no need to do the relocation in SmmCpuFeaturesInitializeProcessor().
//
BOOLEAN  mSmmCpuFeaturesSmmRelocated;

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
  UINT32  LMAValue;

  LMAValue                 = (UINT32)AsmReadMsr64 (EFER_ADDRESS) & LMA;
  mSmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT;
  if (LMAValue) {
    mSmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT;
  }

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
  AMD_SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET.  The currently executing
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
  AMD_SMRAM_SAVE_STATE_MAP  *CpuState;
  UINT32                    LMAValue;

  //
  // No need to configure SMBASE if SmBase relocation has been done.
  //
  if (!mSmmCpuFeaturesSmmRelocated) {
    //
    // Configure SMBASE.
    //
    CpuState             = (AMD_SMRAM_SAVE_STATE_MAP *)(UINTN)(SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET);
    CpuState->x64.SMBASE = (UINT32)CpuHotPlugData->SmBase[CpuIndex];
  }

  // Re-initialize the value of mSmmSaveStateRegisterLma flag which might have been changed in PiCpuSmmDxeSmm Driver
  // Entry point, to make sure correct value on AMD platform is assigned to be used by SmmCpuFeaturesLib.
  LMAValue                 = (UINT32)AsmReadMsr64 (EFER_ADDRESS) & LMA;
  mSmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT;
  if (LMAValue) {
    mSmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT;
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
      AsmWriteMsr64 (SMMADDR_ADDRESS, CpuHotPlugData->SmrrBase);
      AsmWriteMsr64 (SMMMASK_ADDRESS, ((~(UINT64)(CpuHotPlugData->SmrrSize - 1)) | 0x6600));
    }
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
  UINT64                    OriginalInstructionPointer;
  AMD_SMRAM_SAVE_STATE_MAP  *AmdCpuState;

  AmdCpuState = (AMD_SMRAM_SAVE_STATE_MAP *)CpuState;

  if (mSmmSaveStateRegisterLma == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
    OriginalInstructionPointer = (UINT64)AmdCpuState->x86._EIP;
    AmdCpuState->x86._EIP      = (UINT32)NewInstructionPointer;
    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((AmdCpuState->x86.AutoHALTRestart & BIT0) != 0) {
      AmdCpuState->x86.AutoHALTRestart &= ~BIT0;
    }
  } else {
    OriginalInstructionPointer = AmdCpuState->x64._RIP;
    if ((AmdCpuState->x64.EFER & LMA) == 0) {
      AmdCpuState->x64._RIP = (UINT32)NewInstructionPointer32;
    } else {
      AmdCpuState->x64._RIP = (UINT32)NewInstructionPointer;
    }

    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((AmdCpuState->x64.AutoHALTRestart & BIT0) != 0) {
      AmdCpuState->x64.AutoHALTRestart &= ~BIT0;
    }
  }

  return OriginalInstructionPointer;
}

/**
  Return the size, in bytes, of a custom SMI Handler in bytes.  If 0 is
  returned, then a custom SMI handler is not provided by this library,
  and the default SMI handler must be used.

  @retval 0    Use the default SMI handler.
  @retval > 0  Use the SMI handler installed by SmmCpuFeaturesInstallSmiHandler()
               The caller is required to allocate enough SMRAM for each CPU to
               support the size of the custom SMI handler.
**/
UINTN
EFIAPI
SmmCpuFeaturesGetSmiHandlerSize (
  VOID
  )
{
  return 0;
}

/**
  Install a custom SMI handler for the CPU specified by CpuIndex.  This function
  is only called if SmmCpuFeaturesGetSmiHandlerSize() returns a size is greater
  than zero and is called by the CPU that was elected as monarch during System
  Management Mode initialization.

  @param[in] CpuIndex   The index of the CPU to install the custom SMI handler.
                        The value must be between 0 and the NumberOfCpus field
                        in the System Management System Table (SMST).
  @param[in] SmBase     The SMBASE address for the CPU specified by CpuIndex.
  @param[in] SmiStack   The stack to use when an SMI is processed by the
                        the CPU specified by CpuIndex.
  @param[in] StackSize  The size, in bytes, if the stack used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtBase    The base address of the GDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtSize    The size, in bytes, of the GDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtBase    The base address of the IDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtSize    The size, in bytes, of the IDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] Cr3        The base address of the page tables to use when an SMI
                        is processed by the CPU specified by CpuIndex.
**/
VOID
EFIAPI
SmmCpuFeaturesInstallSmiHandler (
  IN UINTN   CpuIndex,
  IN UINT32  SmBase,
  IN VOID    *SmiStack,
  IN UINTN   StackSize,
  IN UINTN   GdtBase,
  IN UINTN   GdtSize,
  IN UINTN   IdtBase,
  IN UINTN   IdtSize,
  IN UINT32  Cr3
  )
{
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
  return FALSE;
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
  return FALSE;
}

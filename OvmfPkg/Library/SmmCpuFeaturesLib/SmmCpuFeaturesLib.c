/** @file
  The CPU specific programming for PiSmmCpuDxeSmm module.

  Copyright (c) 2010 - 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Q35MchIch9.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/SmmCpuFeaturesLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>
#include <Pcd/CpuHotEjectData.h>
#include <PiSmm.h>
#include <Register/Amd/SmramSaveStateMap.h>
#include <Guid/SmmBaseHob.h>

//
// EFER register LMA bit
//
#define LMA  BIT10

//
// Indicate SmBase for each Processors has been relocated or not. If TRUE,
// means no need to do the relocation in SmmCpuFeaturesInitializeProcessor().
//
BOOLEAN  mSmmCpuFeaturesSmmRelocated;

/**
  The constructor function

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCpuFeaturesLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // If gSmmBaseHobGuid found, means SmBase info has been relocated and recorded
  // in the SmBase array.
  //
  mSmmCpuFeaturesSmmRelocated = (BOOLEAN)(GetFirstGuidHob (&gSmmBaseHobGuid) != NULL);

  //
  // No need to program SMRRs on our virtual platform.
  //
  return EFI_SUCCESS;
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
  AMD_SMRAM_SAVE_STATE_MAP  *CpuState;

  //
  // No need to configure SMBASE if SmBase relocation has been done.
  //
  if (!mSmmCpuFeaturesSmmRelocated) {
    //
    // Configure SMBASE.
    //
    CpuState = (AMD_SMRAM_SAVE_STATE_MAP *)(UINTN)(
                                                   SMM_DEFAULT_SMBASE +
                                                   SMRAM_SAVE_STATE_MAP_OFFSET
                                                   );
    if ((CpuState->x86.SMMRevId & 0xFFFF) == 0) {
      CpuState->x86.SMBASE = (UINT32)CpuHotPlugData->SmBase[CpuIndex];
    } else {
      CpuState->x64.SMBASE = (UINT32)CpuHotPlugData->SmBase[CpuIndex];
    }
  }

  //
  // No need to program SMRRs on our virtual platform.
  //
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
                                      field in the System Management System
                                      Table (SMST).
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
  AMD_SMRAM_SAVE_STATE_MAP  *CpuSaveState;

  CpuSaveState = (AMD_SMRAM_SAVE_STATE_MAP *)CpuState;
  if ((CpuSaveState->x86.SMMRevId & 0xFFFF) == 0) {
    OriginalInstructionPointer = (UINT64)CpuSaveState->x86._EIP;
    CpuSaveState->x86._EIP     = (UINT32)NewInstructionPointer;
    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((CpuSaveState->x86.AutoHALTRestart & BIT0) != 0) {
      CpuSaveState->x86.AutoHALTRestart &= ~BIT0;
    }
  } else {
    OriginalInstructionPointer = CpuSaveState->x64._RIP;
    if ((CpuSaveState->x64.EFER & LMA) == 0) {
      CpuSaveState->x64._RIP = (UINT32)NewInstructionPointer32;
    } else {
      CpuSaveState->x64._RIP = (UINT32)NewInstructionPointer;
    }

    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((CpuSaveState->x64.AutoHALTRestart & BIT0) != 0) {
      CpuSaveState->x64.AutoHALTRestart &= ~BIT0;
    }
  }

  return OriginalInstructionPointer;
}

STATIC CPU_HOT_EJECT_DATA  *mCpuHotEjectData = NULL;

/**
  Initialize mCpuHotEjectData if PcdCpuMaxLogicalProcessorNumber > 1.

  Also setup the corresponding PcdCpuHotEjectDataAddress.
**/
STATIC
VOID
InitCpuHotEjectData (
  VOID
  )
{
  UINTN          Size;
  UINT32         Idx;
  UINT32         MaxNumberOfCpus;
  RETURN_STATUS  PcdStatus;

  MaxNumberOfCpus = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  if (MaxNumberOfCpus == 1) {
    return;
  }

  //
  // We allocate CPU_HOT_EJECT_DATA and CPU_HOT_EJECT_DATA->QemuSelectorMap[]
  // in a single allocation, and explicitly align the QemuSelectorMap[] (which
  // is a UINT64 array) at its natural boundary.
  // Accordingly, allocate:
  //   sizeof(*mCpuHotEjectData) + (MaxNumberOfCpus * sizeof(UINT64))
  // and, add sizeof(UINT64) - 1 to use as padding if needed.
  //

  if (RETURN_ERROR (SafeUintnMult (MaxNumberOfCpus, sizeof (UINT64), &Size)) ||
      RETURN_ERROR (SafeUintnAdd (Size, sizeof (*mCpuHotEjectData), &Size)) ||
      RETURN_ERROR (SafeUintnAdd (Size, sizeof (UINT64) - 1, &Size)))
  {
    DEBUG ((DEBUG_ERROR, "%a: invalid CPU_HOT_EJECT_DATA\n", __func__));
    goto Fatal;
  }

  mCpuHotEjectData = AllocatePool (Size);
  if (mCpuHotEjectData == NULL) {
    ASSERT (mCpuHotEjectData != NULL);
    goto Fatal;
  }

  mCpuHotEjectData->Handler     = NULL;
  mCpuHotEjectData->ArrayLength = MaxNumberOfCpus;

  mCpuHotEjectData->QemuSelectorMap = ALIGN_POINTER (
                                        mCpuHotEjectData + 1,
                                        sizeof (UINT64)
                                        );
  //
  // We use mCpuHotEjectData->QemuSelectorMap to map
  // ProcessorNum -> QemuSelector. Initialize to invalid values.
  //
  for (Idx = 0; Idx < mCpuHotEjectData->ArrayLength; Idx++) {
    mCpuHotEjectData->QemuSelectorMap[Idx] = CPU_EJECT_QEMU_SELECTOR_INVALID;
  }

  //
  // Expose address of CPU Hot eject Data structure
  //
  PcdStatus = PcdSet64S (
                PcdCpuHotEjectDataAddress,
                (UINTN)(VOID *)mCpuHotEjectData
                );
  ASSERT_RETURN_ERROR (PcdStatus);

  return;

Fatal:
  CpuDeadLoop ();
}

/**
  Hook point in normal execution mode that allows the one CPU that was elected
  as monarch during System Management Mode initialization to perform additional
  initialization actions immediately after all of the CPUs have processed their
  first SMI and called SmmCpuFeaturesInitializeProcessor() relocating SMBASE
  into a buffer in SMRAM and called SmmCpuFeaturesHookReturnFromSmm().
**/
VOID
EFIAPI
SmmCpuFeaturesSmmRelocationComplete (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       MapPagesBase;
  UINTN       MapPagesCount;

  InitCpuHotEjectData ();

  if (!MemEncryptSevIsEnabled ()) {
    return;
  }

  //
  // Now that SMBASE relocation is complete, re-encrypt the original SMRAM save
  // state map's container pages, and release the pages to DXE. (The pages were
  // allocated in PlatformPei.)
  //
  Status = MemEncryptSevLocateInitialSmramSaveStateMapPages (
             &MapPagesBase,
             &MapPagesCount
             );
  ASSERT_EFI_ERROR (Status);

  Status = MemEncryptSevSetPageEncMask (
             0,             // Cr3BaseAddress -- use current CR3
             MapPagesBase,  // BaseAddress
             MapPagesCount  // NumPages
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MemEncryptSevSetPageEncMask(): %r\n",
      __func__,
      Status
      ));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  ZeroMem ((VOID *)MapPagesBase, EFI_PAGES_TO_SIZE (MapPagesCount));

  if (PcdGetBool (PcdQ35SmramAtDefaultSmbase)) {
    //
    // The initial SMRAM Save State Map has been covered as part of a larger
    // reserved memory allocation in PlatformPei's InitializeRamRegions(). That
    // allocation is supposed to survive into OS runtime; we must not release
    // any part of it. Only re-assert the containment here.
    //
    ASSERT (SMM_DEFAULT_SMBASE <= MapPagesBase);
    ASSERT (
      (MapPagesBase + EFI_PAGES_TO_SIZE (MapPagesCount) <=
       SMM_DEFAULT_SMBASE + MCH_DEFAULT_SMBASE_SIZE)
      );
  } else {
    Status = gBS->FreePages (MapPagesBase, MapPagesCount);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Return the size, in bytes, of a custom SMI Handler in bytes.  If 0 is
  returned, then a custom SMI handler is not provided by this library,
  and the default SMI handler must be used.

  @retval 0    Use the default SMI handler.
  @retval > 0  Use the SMI handler installed by
               SmmCpuFeaturesInstallSmiHandler(). The caller is required to
               allocate enough SMRAM for each CPU to support the size of the
               custom SMI handler.
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
  Install a custom SMI handler for the CPU specified by CpuIndex.  This
  function is only called if SmmCpuFeaturesGetSmiHandlerSize() returns a size
  is greater than zero and is called by the CPU that was elected as monarch
  during System Management Mode initialization.

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
  Disable SMRR register if SMRR is supported and
  SmmCpuFeaturesNeedConfigureMtrrs() returns TRUE.
**/
VOID
EFIAPI
SmmCpuFeaturesDisableSmrr (
  VOID
  )
{
  //
  // No SMRR support, nothing to do
  //
}

/**
  Enable SMRR register if SMRR is supported and
  SmmCpuFeaturesNeedConfigureMtrrs() returns TRUE.
**/
VOID
EFIAPI
SmmCpuFeaturesReenableSmrr (
  VOID
  )
{
  //
  // No SMRR support, nothing to do
  //
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
  // No SMRR support, nothing to do
  //
}

/**
  Processor specific hook point each time a CPU exits System Management Mode.

  @param[in] CpuIndex  The index of the CPU that is exiting SMM.  The value
                       must be between 0 and the NumberOfCpus field in the
                       System Management System Table (SMST).
**/
VOID
EFIAPI
SmmCpuFeaturesRendezvousExit (
  IN UINTN  CpuIndex
  )
{
  //
  // We only call the Handler if CPU hot-eject is enabled
  // (PcdCpuMaxLogicalProcessorNumber > 1), and hot-eject is needed
  // in this SMI exit (otherwise mCpuHotEjectData->Handler is not armed.)
  //

  if (mCpuHotEjectData != NULL) {
    CPU_HOT_EJECT_HANDLER  Handler;

    //
    // As the comment above mentions, mCpuHotEjectData->Handler might be
    // written to on the BSP as part of handling of the CPU-ejection.
    //
    // We know that any initial assignment to mCpuHotEjectData->Handler
    // (on the BSP, in the CpuHotplugMmi() context) is ordered-before the
    // load below, since it is guaranteed to happen before the
    // control-dependency of the BSP's SMI exit signal -- by way of a store
    // to AllCpusInSync (on the BSP, in BspHandler()) and the corresponding
    // AllCpusInSync loop (on the APs, in SmiRendezvous()) which depends on
    // that store.
    //
    // This guarantees that these pieces of code can never execute
    // simultaneously. In addition, we ensure that the following load is
    // ordered-after the AllCpusInSync loop by using a MemoryFence() with
    // acquire semantics.
    //
    MemoryFence ();

    Handler = mCpuHotEjectData->Handler;

    if (Handler != NULL) {
      Handler (CpuIndex);
    }
  }
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
  ASSERT (RegName == SmmRegFeatureControl);
  return FALSE;
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
  //
  // This is called for SmmRegSmmDelayed, SmmRegSmmBlocked, SmmRegSmmEnable.
  // The last of these should actually be SmmRegSmmDisable, so we can just
  // return FALSE.
  //
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
  ASSERT (FALSE);
}

/**
  This function is hook point called after the gEfiSmmReadyToLockProtocolGuid
  notification is completely processed.
**/
VOID
EFIAPI
SmmCpuFeaturesCompleteSmmReadyToLock (
  VOID
  )
{
}

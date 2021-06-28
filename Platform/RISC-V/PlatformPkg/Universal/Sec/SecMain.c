/** @file
  RISC-V SEC phase module.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <SecMain.h>
#include <IndustryStandard/RiscVOpensbi.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/PrintLib.h>
#include <Library/RiscVEdk2SbiLib.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_console.h>  // Reference to header file in opensbi
#include <sbi/sbi_hart.h>     // Reference to header file in opensbi
#include <sbi/sbi_hartmask.h>    // Reference to header file in opensbi
#include <sbi/sbi_scratch.h>  // Reference to header file in opensbi
#include <sbi/sbi_platform.h> // Reference to header file in opensbi
#include <sbi/sbi_init.h>     // Reference to header file in opensbi
#include <sbi/sbi_ecall.h>    // Reference to header file in opensbi
#include <sbi/sbi_trap.h>     // Reference to header file in opensbi

extern struct sbi_platform_operations Edk2OpensbiPlatformOps;

//
// Indicates the boot hart (PcdBootHartId) OpenSBI initialization is done.
//
atomic_t BootHartDone = ATOMIC_INITIALIZER(0);
atomic_t NonBootHartMessageLock = ATOMIC_INITIALIZER(0);

typedef struct sbi_scratch *(*hartid2scratch)(ulong hartid, ulong hartindex);

/**
  Locates a section within a series of sections
  with the specified section type.

  The Instance parameter indicates which instance of the section
  type to return. (0 is first instance, 1 is second...)

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[in]   Instance        The section instance number
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsSectionInstance (
  IN  VOID                             *Sections,
  IN  UINTN                            SizeOfSections,
  IN  EFI_SECTION_TYPE                 SectionType,
  IN  UINTN                            Instance,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
  )
{
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfSections;
  EFI_COMMON_SECTION_HEADER   *Section;
  EFI_PHYSICAL_ADDRESS        EndOfSection;

  //
  // Loop through the FFS file sections within the PEI Core FFS file
  //
  EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN) Sections;
  EndOfSections = EndOfSection + SizeOfSections;
  for (;;) {
    if (EndOfSection == EndOfSections) {
      break;
    }
    CurrentAddress = (EndOfSection + 3) & ~(3ULL);
    if (CurrentAddress >= EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    Section = (EFI_COMMON_SECTION_HEADER*)(UINTN) CurrentAddress;

    Size = SECTION_SIZE (Section);
    if (Size < sizeof (*Section)) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfSection = CurrentAddress + Size;
    if (EndOfSection > EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the requested section type
    //
    if (Section->Type == SectionType) {
      if (Instance == 0) {
        *FoundSection = Section;
        return EFI_SUCCESS;
      } else {
        Instance--;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Locates a section within a series of sections
  with the specified section type.

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsSectionInSections (
  IN  VOID                             *Sections,
  IN  UINTN                            SizeOfSections,
  IN  EFI_SECTION_TYPE                 SectionType,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
  )
{
  return FindFfsSectionInstance (
           Sections,
           SizeOfSections,
           SectionType,
           0,
           FoundSection
           );
}

/**
  Locates a FFS file with the specified file type and a section
  within that file with the specified section type.

  @param[in]   Fv            The firmware volume to search
  @param[in]   FileType      The file type to locate
  @param[in]   SectionType   The section type to locate
  @param[out]  FoundSection  The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsFileAndSection (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *Fv,
  IN  EFI_FV_FILETYPE                  FileType,
  IN  EFI_SECTION_TYPE                 SectionType,
  OUT EFI_COMMON_SECTION_HEADER        **FoundSection
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  EFI_PHYSICAL_ADDRESS        EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER         *File;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfFile;

  if (Fv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a: FV at %p does not have FV header signature\n", __FUNCTION__, Fv));
    return EFI_VOLUME_CORRUPTED;
  }

  CurrentAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) Fv;
  EndOfFirmwareVolume = CurrentAddress + Fv->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + Fv->HeaderLength; ; ) {

    CurrentAddress = (EndOfFile + 7) & ~(7ULL);
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    File = (EFI_FFS_FILE_HEADER*)(UINTN) CurrentAddress;
    Size = *(UINT32*) File->Size & 0xffffff;
    if (Size < (sizeof (*File) + sizeof (EFI_COMMON_SECTION_HEADER))) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the request file type
    //
    if (File->Type != FileType) {
      continue;
    }

    Status = FindFfsSectionInSections (
               (VOID*) (File + 1),
               (UINTN) EndOfFile - (UINTN) (File + 1),
               SectionType,
               FoundSection
               );
    if (!EFI_ERROR (Status) || (Status == EFI_VOLUME_CORRUPTED)) {
      return Status;
    }
  }
}

/**
  Locates the PEI Core entry point address

  @param[in]  Fv                 The firmware volume to search
  @param[out] PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindPeiCoreImageBaseInFv (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *Fv,
  OUT  EFI_PHYSICAL_ADDRESS             *PeiCoreImageBase
  )
{
  EFI_STATUS                  Status;
  EFI_COMMON_SECTION_HEADER   *Section;

  Status = FindFfsFileAndSection (
             Fv,
             EFI_FV_FILETYPE_PEI_CORE,
             EFI_SECTION_PE32,
             &Section
             );
  if (EFI_ERROR (Status)) {
    Status = FindFfsFileAndSection (
               Fv,
               EFI_FV_FILETYPE_PEI_CORE,
               EFI_SECTION_TE,
               &Section
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Unable to find PEI Core image\n", __FUNCTION__));
      return Status;
    }
  }
  DEBUG ((DEBUG_INFO, "%a: PeiCoreImageBase found\n", __FUNCTION__));
  *PeiCoreImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)(Section + 1);
  return EFI_SUCCESS;
}

/**
  Locates the PEI Core entry point address

  @param[in,out]  Fv                 The firmware volume to search
  @param[out]     PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
VOID
FindPeiCoreImageBase (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER       **BootFv,
     OUT  EFI_PHYSICAL_ADDRESS             *PeiCoreImageBase
  )
{
  *PeiCoreImageBase = 0;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));
  FindPeiCoreImageBaseInFv (*BootFv, PeiCoreImageBase);
}

/*
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug inforamtion. It will report them if
  remote debug is enabled.

**/
VOID
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER       **BootFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT         *PeiCoreEntryPoint
  )
{
  EFI_STATUS                       Status;
  EFI_PHYSICAL_ADDRESS             PeiCoreImageBase;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  FindPeiCoreImageBase (BootFirmwareVolumePtr, &PeiCoreImageBase);
  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *) (UINTN) PeiCoreImageBase, (VOID**) PeiCoreEntryPoint);
  if (EFI_ERROR(Status)) {
    *PeiCoreEntryPoint = 0;
  }
  DEBUG ((DEBUG_INFO, "%a: PeCoffLoaderGetEntryPoint success: %x\n", __FUNCTION__, *PeiCoreEntryPoint));

  return;
}

/**
  Handles SBI calls of EDK2's SBI FW extension.

  The return value is the error code returned by the SBI call.

  @param[in]  ExtId        The extension ID of the FW extension.
  @param[in]  FuncId       The called function ID.
  @param[in]  Args         The args to the function.
  @param[out] OutVal       The value the function returns to the caller.
  @param[out] OutTrap      Trap info for trapping further, see OpenSBI code.
                           Is ignored if return value is not SBI_ETRAP.

  @retval SBI_OK           If the handler succeeds.
  @retval SBI_ENOTSUPP     If there's no function with the given ID.
  @retval SBI_ETRAP        If the called SBI functions wants to trap further.
**/
STATIC int SbiEcallFirmwareHandler (
  IN  unsigned long         ExtId,
  IN  unsigned long         FuncId,
  IN  CONST struct sbi_trap_regs *TrapRegs,
  OUT unsigned long        *OutVal,
  OUT struct sbi_trap_info *OutTrap
  )
{
  int Ret = SBI_OK;

  switch (FuncId) {
    case SBI_EXT_FW_MSCRATCH_FUNC:
      *OutVal = (unsigned long) sbi_scratch_thishart_ptr();
      break;
    case SBI_EXT_FW_MSCRATCH_HARTID_FUNC:
      *OutVal = (unsigned long) sbi_hartid_to_scratch (TrapRegs->a0);
      break;
    default:
      Ret = SBI_ENOTSUPP;
      DEBUG ((DEBUG_ERROR, "%a: Called SBI firmware ecall with invalid function ID\n", __FUNCTION__));
      ASSERT (FALSE);
  };

  return Ret;
}

struct sbi_ecall_extension FirmwareEcall = {
  .extid_start = SBI_EDK2_FW_EXT,
  .extid_end = SBI_EDK2_FW_EXT,
  .handle = SbiEcallFirmwareHandler,
};

/** Register EDK2's SBI extension with OpenSBI

  This function returns EFI_STATUS, even though it only ever returns
  EFI_SUCCESS. On error it ASSERTs. Looking at OpenSBI code it appears that
  registering an extension can only fail if the extension ID is invalid or was
  already registered. Failure is therefore an error of the programmer.

  @retval EFI_SUCCESS If the extension was successfully registered.
**/
EFI_STATUS
EFIAPI
RegisterFirmwareSbiExtension (
  VOID
  )
{
  UINTN Ret;
  Ret = sbi_ecall_register_extension(&FirmwareEcall);
  if (Ret) {
    //
    // Only fails if the extension ID is invalid or already is registered.
    //
    DEBUG ((DEBUG_ERROR, "Failed to register SBI Firmware Extension for EDK2\n"));
    ASSERT(FALSE);
  }

  return EFI_SUCCESS;
}

/**
  OpenSBI platform early init hook.

**/
int
SecPostOpenSbiPlatformEarlylInit(
  IN BOOLEAN ColdBoot
  )
{
  //
  // Boot HART is already in the process of OpenSBI initialization.
  // We can let other HART to keep booting.
  //
  DEBUG ((DEBUG_INFO, "%a: Set boot hart done.\n", __FUNCTION__));
  atomic_write (&BootHartDone, (UINT64)TRUE);
  return 0;
}

/**
  OpenSBI platform final init hook.
  We restore the next_arg1 to the pointer of EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT.

**/
int
SecPostOpenSbiPlatformFinalInit (
  IN BOOLEAN ColdBoot
  )
{
  UINT32 HartId;
  struct sbi_scratch *SbiScratch;
  struct sbi_scratch *ScratchSpace;
  struct sbi_platform *SbiPlatform;
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContext;

  DEBUG((DEBUG_INFO, "%a: Entry, preparing to jump to PEI Core\n\n", __FUNCTION__));

  SbiScratch = sbi_scratch_thishart_ptr();
  SbiPlatform = (struct sbi_platform *)sbi_platform_ptr(SbiScratch);
  FirmwareContext = (EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *)SbiPlatform->firmware_context;

  //
  // Print out scratch address of each hart
  //
  DEBUG ((DEBUG_INFO, "%a: OpenSBI scratch address for each hart:\n", __FUNCTION__));
  for (HartId = 0; HartId < SBI_HARTMASK_MAX_BITS; HartId ++) {
    if (sbi_platform_hart_invalid(SbiPlatform, HartId)) {
      continue;
    }
    ScratchSpace = sbi_hartid_to_scratch (HartId);
    if(ScratchSpace != NULL) {
      DEBUG((DEBUG_INFO, "          Hart %d: 0x%x\n", HartId, ScratchSpace));
    } else {
      DEBUG((DEBUG_INFO, "          Hart %d not initialized yet\n", HartId));
    }
  }

  //
  // Set firmware context Hart-specific pointer
  //
  for (HartId = 0; HartId < SBI_HARTMASK_MAX_BITS; HartId ++) {
    if (sbi_platform_hart_invalid(SbiPlatform, HartId)) {
      continue;
    }
    ScratchSpace = sbi_hartid_to_scratch (HartId);
    if (ScratchSpace != NULL) {
      FirmwareContext->HartSpecific[HartId] =
        (EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC *)((UINT8 *)ScratchSpace - FIRMWARE_CONTEXT_HART_SPECIFIC_SIZE);
        DEBUG ((DEBUG_INFO, "%a: OpenSBI Hart %d Firmware Context Hart-specific at address: 0x%x\n",
                __FUNCTION__,
                 HartId,
                 FirmwareContext->HartSpecific [HartId]
                 ));
    }
  }

  DEBUG((DEBUG_INFO, "%a: Jump to PEI Core with \n", __FUNCTION__));
  DEBUG((DEBUG_INFO, "  sbi_scratch = %x\n", SbiScratch));
  DEBUG((DEBUG_INFO, "  sbi_platform = %x\n", SbiPlatform));
  DEBUG((DEBUG_INFO, "  FirmwareContext = %x\n", FirmwareContext));
  SbiScratch->next_arg1 = (unsigned long)FirmwareContext;

  return 0;
}

/** Transion from SEC phase to PEI phase.

  This function transits to S-mode PEI phase from M-mode SEC phase.

  @param[in]  BootHartId     Hardware thread ID of boot hart.
  @param[in]  Scratch       Pointer to sbi_scratch structure.

**/
VOID EFIAPI PeiCore (
  IN  UINTN BootHartId,
  IN  struct sbi_scratch *Scratch
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;
  EFI_PEI_CORE_ENTRY_POINT    PeiCoreEntryPoint;
  EFI_FIRMWARE_VOLUME_HEADER *BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)FixedPcdGet32(PcdRiscVPeiFvBase);
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT FirmwareContext;
  struct sbi_platform        *ThisSbiPlatform;

  FindAndReportEntryPoints (&BootFv, &PeiCoreEntryPoint);

  SecCoreData.DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = BootFv;
  SecCoreData.BootFirmwareVolumeSize = (UINTN) BootFv->FvLength;
  SecCoreData.TemporaryRamBase       = (VOID*)(UINT64) FixedPcdGet32(PcdTemporaryRamBase);
  SecCoreData.TemporaryRamSize       = (UINTN)  FixedPcdGet32(PcdTemporaryRamSize);
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize >> 1;
  SecCoreData.StackBase              = (UINT8 *)SecCoreData.TemporaryRamBase + (SecCoreData.TemporaryRamSize >> 1);
  SecCoreData.StackSize              = SecCoreData.TemporaryRamSize >> 1;

  //
  // Set up OpepSBI firmware context pointer on boot hart OpenSbi scratch.
  // Firmware context residents in stack and will be switched to memory when
  // temporary RAM migration.
  //
  ZeroMem ((VOID *)&FirmwareContext, sizeof (EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT));
  ThisSbiPlatform = (struct sbi_platform *)sbi_platform_ptr(Scratch);
  if (ThisSbiPlatform->opensbi_version > OPENSBI_VERSION) {
      DEBUG ((DEBUG_ERROR, "%a: OpenSBI platform table version 0x%x is newer than OpenSBI version 0x%x.\n"
                           "There maybe be some backward compatable issues.\n",
              __FUNCTION__,
             ThisSbiPlatform->opensbi_version,
             OPENSBI_VERSION
             ));
      ASSERT(FALSE);
  }
  DEBUG ((DEBUG_INFO, "%a: OpenSBI platform table at address: 0x%x\nFirmware Context is located at 0x%x\n",
             __FUNCTION__,
             ThisSbiPlatform,
             &FirmwareContext
             ));
  ThisSbiPlatform->firmware_context = (unsigned long)&FirmwareContext;

  //
  // Save Flattened Device tree in firmware context
  //
  FirmwareContext.FlattenedDeviceTree = Scratch->next_arg1;

  //
  // Transfer the control to the PEI core
  //
  FirmwareContext.SecPeiHandOffData = (UINT64)&SecCoreData;

  //
  // Set supervisor translation mode to Bare mode
  //
  DEBUG ((DEBUG_INFO, "%a: Set Supervisor address mode to Bare-mode.\n", __FUNCTION__));
  RiscVSetSupervisorAddressTranslationRegister ((UINT64)RISCV_SATP_MODE_OFF << RISCV_SATP_MODE_BIT_POSITION);

  //
  // Scratch->next_arg1 is the device tree.
  //
  Scratch->next_addr = (UINTN)(PeiCoreEntryPoint);
  Scratch->next_mode = PRV_S;
  DEBUG ((DEBUG_INFO, "%a: Initializing OpenSBI library for booting hart %d\n", __FUNCTION__, BootHartId));
  sbi_init(Scratch);
}

/**
  Register firmware SBI extension and launch PeiCore to the mode specified in
  PcdPeiCorePrivilegeMode;

  To register the SBI extension we stay in M-Mode and then transition here,
  rather than before in sbi_init.

  @param[in]  ThisHartId    Hardware thread ID.
  @param[in]  Scratch       Pointer to sbi_scratch structure.

**/
VOID
EFIAPI
LaunchPeiCore (
  IN  UINTN  ThisHartId,
  IN  struct sbi_scratch *Scratch
  )
{
  RegisterFirmwareSbiExtension ();
  PeiCore (ThisHartId, Scratch);
}

/**
  Interface to invoke internal mode switch function.

  To register the SBI extension we stay in M-Mode and then transition here,
  rather than before in sbi_init.

  @param[in]  FuncArg0       Arg0 to pass to next phase entry point address.
  @param[in]  FuncArg1       Arg1 to pass to next phase entry point address.
  @param[in]  NextAddr       Entry point of next phase.
  @param[in]  NextMode       Privilege mode of next phase.
  @param[in]  NextVirt       Next phase is in virtualiztion.

**/
VOID
EFIAPI
RiscVOpenSbiHartSwitchMode (
  IN  UINTN   FuncArg0,
  IN  UINTN   FuncArg1,
  IN  UINTN   NextAddr,
  IN  UINTN   NextMode,
  IN  BOOLEAN NextVirt
  )
{
  sbi_hart_switch_mode(FuncArg0, FuncArg1, NextAddr, NextMode, NextVirt);
}

/**
  Get device tree address

  @retval The address of Device Tree binary.
**/
VOID *
EFIAPI
GetDeviceTreeAddress (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_COMMON_SECTION_HEADER *FoundSection;

  if (FixedPcdGet32 (PcdDeviceTreeAddress)) {
      return (VOID *)*((unsigned long *)FixedPcdGet32 (PcdDeviceTreeAddress));
  } else if (FixedPcdGet32 (PcdRiscVDtbFvBase)) {
      Status = FindFfsFileAndSection (
                 (EFI_FIRMWARE_VOLUME_HEADER *)FixedPcdGet32 (PcdRiscVDtbFvBase),
                 EFI_FV_FILETYPE_FREEFORM,
                 EFI_SECTION_RAW,
                 &FoundSection
               );
      if (EFI_ERROR(Status)) {
        DEBUG ((DEBUG_ERROR, "Platform Device Tree is not found from FV.\n"));
        return NULL;
      }
      FoundSection ++;
      return (VOID *)FoundSection;
  } else {
      DEBUG ((DEBUG_ERROR, "Must use DTB either from memory or compiled in FW. PCDs configured incorrectly.\n"));
      ASSERT (FALSE);
  }
  return NULL;
}

/**
  This function initilizes hart specific information and SBI.
  For the boot hart, it boots system through PEI core and initial SBI in the DXE IPL.
  For others, it goes to initial SBI and halt.

  the lay out of memory region for each hart is as below delineates,

                                               _                                        ____
  |----Scratch ends                             |                                           |
  |                                             | sizeof (sbi_scratch)                      |
  |                                            _|                                           |
  |----Scratch buffer starts                   <----- *Scratch                              |
  |----Firmware Context Hart-specific ends     _                                            |
  |                                             |                                           |
  |                                             | FIRMWARE_CONTEXT_HART_SPECIFIC_SIZE       |
  |                                             |                                           |  PcdOpenSbiStackSize
  |                                            _|                                           |
  |----Firmware Context Hart-specific starts   <----- **HartFirmwareContext                 |
  |----Hart stack top                          _                                            |
  |                                             |                                           |
  |                                             |                                           |
  |                                             |  Stack                                    |
  |                                             |                                           |
  |                                            _|                                       ____|
  |----Hart stack bottom

  @param[in]  HartId          Hardware thread ID.
  @param[in]  Scratch         Pointer to sbi_scratch structure.

**/
VOID EFIAPI SecCoreStartUpWithStack(
  IN  UINTN HartId,
  IN  struct sbi_scratch *Scratch
  )
{
  UINT64 BootHartDoneSbiInit;
  UINT64 NonBootHartMessageLockValue;
  struct sbi_platform *ThisSbiPlatform;
  EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC *HartFirmwareContext;

  Scratch->next_arg1 = (unsigned long)GetDeviceTreeAddress ();
  if (Scratch->next_arg1 == (unsigned long)NULL) {
    DEBUG ((DEBUG_ERROR, "Platform Device Tree is not found\n"));
    ASSERT (FALSE);
  }
  DEBUG ((DEBUG_INFO, "DTB address: 0x%08x\n", Scratch->next_arg1));

  //
  // Setup EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC for each hart.
  //
  HartFirmwareContext = (EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC *)((UINT8 *)Scratch - FIRMWARE_CONTEXT_HART_SPECIFIC_SIZE);
  HartFirmwareContext->IsaExtensionSupported = RiscVReadMachineIsa ();
  HartFirmwareContext->MachineVendorId.Value64_L = RiscVReadMachineVendorId ();
  HartFirmwareContext->MachineVendorId.Value64_H = 0;
  HartFirmwareContext->MachineArchId.Value64_L = RiscVReadMachineArchitectureId ();
  HartFirmwareContext->MachineArchId.Value64_H = 0;
  HartFirmwareContext->MachineImplId.Value64_L = RiscVReadMachineImplementId ();
  HartFirmwareContext->MachineImplId.Value64_H = 0;
  HartFirmwareContext->HartSwitchMode = RiscVOpenSbiHartSwitchMode;

  //
  // Hook platorm_ops with EDK2 one. Thus we can have interface
  // call out to OEM EDK2 platform code in M-mode before switching
  // to S-mode in opensbo init.
  //
  ThisSbiPlatform = (struct sbi_platform *)sbi_platform_ptr(Scratch);
  ThisSbiPlatform->platform_ops_addr = (unsigned long)&Edk2OpensbiPlatformOps;

  if (HartId == FixedPcdGet32(PcdBootHartId)) {
    LaunchPeiCore (HartId, Scratch);
  }

  //
  // Initialize the non boot harts
  //
  do {
    BootHartDoneSbiInit = atomic_read (&BootHartDone);
    //
    // Below leave some memory cycles to boot hart
    // for updating BootHartDone.
    //
    CpuPause ();
    CpuPause ();
    CpuPause ();
  } while (BootHartDoneSbiInit != (UINT64)TRUE);

  NonBootHartMessageLockValue = atomic_xchg(&NonBootHartMessageLock, TRUE);
  while (NonBootHartMessageLockValue == TRUE) {
    CpuPause ();
    CpuPause ();
    CpuPause ();
    NonBootHartMessageLockValue = atomic_xchg(&NonBootHartMessageLock, TRUE);
  };
  DEBUG((DEBUG_INFO, "%a: Non boot hart %d initialization.\n", __FUNCTION__, HartId));
  NonBootHartMessageLockValue = atomic_xchg(&NonBootHartMessageLock, FALSE);
  //
  // Non boot hart wiil be halted waiting for SBI_HART_STARTING.
  // Use HSM ecall to start non boot hart (SBI_EXT_HSM_HART_START) later on,
  //
  Scratch->next_mode = PRV_S;
  sbi_init(Scratch);
}


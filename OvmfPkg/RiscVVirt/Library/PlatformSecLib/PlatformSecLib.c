/** @file
An instance of Platform Sec Lib.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2025 Ventana Micro Systems Inc.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformSecLib.h"

typedef struct {
  EFI_HOB_GUID_TYPE         HobGuidHeader;
  RISCV_SEC_HANDOFF_DATA    SecHandoffData;
  EFI_HOB_GENERIC_HEADER    HobEnd;
} SEC_HOBLIST_DATA;

EFI_STATUS
EFIAPI
RiscVSecGetHobData (
  IN CONST EFI_SEC_HOB_DATA_PPI  *This,
  OUT EFI_HOB_GENERIC_HEADER     **HobList
  );

STATIC EFI_SEC_HOB_DATA_PPI  mSecHobDataPpi = {
  RiscVSecGetHobData
};

STATIC EFI_PEI_PPI_DESCRIPTOR  mPpiSecHobData = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiSecHobDataPpiGuid,
  &mSecHobDataPpi
};

/**
  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param SizeOfRam           Size of the temporary memory available for use.
  @param TempRamBase         Base address of temporary ram
  @param BootFirmwareVolume  Base address of the Boot Firmware Volume.
**/
VOID
NORETURN
EFIAPI
SecStartup (
  IN UINT32  SizeOfRam,
  IN UINT32  TempRamBase,
  IN VOID    *BootFirmwareVolume
  );

STATIC
VOID *
GetSecHobData (
  VOID
  )
{
  return (VOID *)(FixedPcdGet32 (PcdOvmfSecPeiTempRamBase) + FixedPcdGet32 (PcdOvmfSecPeiTempRamSize) - SEC_HANDOFF_DATA_RESERVE_SIZE);
}

/**
  Entry point to the C language phase of SEC for this platform. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param BootHartId           The hardware thread (Hart) ID of the current CPU.
  @param DeviceTreeAddress    Address of the device tree provided to the SEC phase.
  @param TempRamBase          Base address of the temporary memory.
  @param TempRamSize          Size of the temporary memory region.
**/
VOID
NORETURN
EFIAPI
SecStartupPlatform (
  IN  UINTN   BootHartId,
  IN  VOID    *DeviceTreeAddress,
  IN  UINT32  TempRamBase,
  IN  UINT32  TempRamSize
  )
{
  SEC_HOBLIST_DATA  *SecHobList;

  DEBUG ((
    DEBUG_INFO,
    "%a() BootHartId: 0x%x, DeviceTreeAddress=0x%p\n",
    __func__,
    BootHartId,
    DeviceTreeAddress
    ));

  //
  // Set hob list data that will be passed to PEI
  //
  SecHobList                            = (SEC_HOBLIST_DATA *)GetSecHobData ();
  SecHobList->SecHandoffData.BootHartId = BootHartId;
  SecHobList->SecHandoffData.FdtPointer = DeviceTreeAddress;

  TempRamSize -= SEC_HANDOFF_DATA_RESERVE_SIZE;

  SecStartup (TempRamSize, TempRamBase, (VOID *)(UINTN)PcdGet32 (PcdOvmfDxeMemFvBase));

  //
  // Should not come here.
  //
  UNREACHABLE ();
}

/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI/DXE core by SEC
  core. Platform developer may modify the SecCoreData passed to PEI Core.
  It returns a platform specific PPI list that platform wishes to pass to PEI core.
  The Generic SEC core module will merge this list to join the final list passed to
  PEI core.

  @param  SecCoreData           The same parameter as passing to PEI core. It
                                could be overridden by this function.

  @return The platform specific PPI list to be passed to PEI core or
          NULL if there is no need of such platform specific PPI list.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF  *SecCoreData
  )
{
  SEC_HOBLIST_DATA  *SecHobList;
  const EFI_GUID    SecHobDataGuid = RISCV_SEC_HANDOFF_HOB_GUID;

  SecHobList = (SEC_HOBLIST_DATA *)GetSecHobData ();
  if (SecCoreData == NULL) {
    //
    // Peiless booting, initializing platform devices now
    //
    SetBootMode (BOOT_WITH_FULL_CONFIGURATION);

    MemoryInitialization (SecHobList->SecHandoffData.FdtPointer);
    CpuInitialization (SecHobList->SecHandoffData.FdtPointer);
    PlatformInitialization (SecHobList->SecHandoffData.FdtPointer);

    //
    // Pass Sec Hob data to DXE
    //
    BuildGuidDataHob (&SecHobDataGuid, &SecHobList->SecHandoffData, sizeof (SecHobList->SecHandoffData));

    return NULL;
  }

  //
  // Public our PEI PPI
  //
  return &mPpiSecHobData;
}

/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param  PeiServices               Pointer to the PEI Services Table.
  @param  StructureSize             Pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation (
  IN CONST EFI_PEI_SERVICES                  **PeiServices,
  IN OUT   UINT64                            *StructureSize,
  OUT   EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  )
{
  //
  // Not available for RISC-V
  //
  return EFI_SUCCESS;
}

/**
  This interface disables temporary memory in SEC Phase.
**/
VOID
EFIAPI
SecPlatformDisableTemporaryMemory (
  VOID
  )
{
  //
  // Not available
  //
}

EFI_STATUS
EFIAPI
RiscVSecGetHobData (
  IN CONST EFI_SEC_HOB_DATA_PPI  *This,
  OUT EFI_HOB_GENERIC_HEADER     **HobList
  )
{
  SEC_HOBLIST_DATA  *SecHobList;
  const EFI_GUID    SecHobDataGuid = RISCV_SEC_HANDOFF_HOB_GUID;

  SecHobList = (SEC_HOBLIST_DATA *)GetSecHobData ();

  SecHobList->HobGuidHeader.Header.HobType   = EFI_HOB_TYPE_GUID_EXTENSION;
  SecHobList->HobGuidHeader.Header.HobLength = sizeof (SecHobList->HobGuidHeader) + sizeof (SecHobList->SecHandoffData);
  SecHobList->HobGuidHeader.Header.Reserved  = 0;
  CopyGuid (&(SecHobList->HobGuidHeader.Name), &SecHobDataGuid);

  SecHobList->HobEnd.HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  SecHobList->HobEnd.HobLength = sizeof (SecHobList->HobEnd);
  SecHobList->HobEnd.Reserved  = 0;

  *HobList = (EFI_HOB_GENERIC_HEADER *)&(SecHobList->HobGuidHeader);

  return EFI_SUCCESS;
}

/** @file
  Common library to build up firmware context processor-specific information

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <IndustryStandard/RiscVOpensbi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <ProcessorSpecificHobData.h>
#include <RiscVImpl.h>
#include <sbi/sbi_hart.h>

/**
  Build up common firmware context processor-specific information

  @param  FirmwareContextHartSpecific  Pointer to EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC
  @param  ParentProcessorGuid          Pointer to GUID of Processor which contains this core
  @param  ParentProcessorUid           Unique ID of physical processor which owns this core.
  @param  CoreGuid                     Pointer to GUID of core
  @param  HartId                       Hart ID of this core.
  @param  IsBootHart                   This is boot hart or not
  @param  ProcessorSpecificDataHob     Pointer to RISC_V_PROCESSOR_SPECIFIC_DATA_HOB

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
CommonFirmwareContextHartSpecificInfo (
  EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC  *FirmwareContextHartSpecific,
  EFI_GUID                                  *ParentProcessorGuid,
  UINTN                                     ParentProcessorUid,
  EFI_GUID                                  *CoreGuid,
  UINTN                                     HartId,
  BOOLEAN                                   IsBootHart,
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA        *ProcessorSpecificDataHob
  )
{
  //
  // Build up RISC_V_PROCESSOR_SPECIFIC_DATA_HOB.
  //
  CopyGuid (&ProcessorSpecificDataHob->ParentProcessorGuid, ParentProcessorGuid);
  ProcessorSpecificDataHob->ParentProcessorUid = ParentProcessorUid;
  CopyGuid (&ProcessorSpecificDataHob->CoreGuid, CoreGuid);
  ProcessorSpecificDataHob->Context                        = NULL;
  ProcessorSpecificDataHob->ProcessorSpecificData.Revision =
    SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA_REVISION;
  ProcessorSpecificDataHob->ProcessorSpecificData.Length =
    sizeof (SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA);
  ProcessorSpecificDataHob->ProcessorSpecificData.HartId.Value64_L = (UINT64)HartId;
  ProcessorSpecificDataHob->ProcessorSpecificData.HartId.Value64_H = 0;
  ProcessorSpecificDataHob->ProcessorSpecificData.BootHartId       = (UINT8)IsBootHart;
  ProcessorSpecificDataHob->ProcessorSpecificData.InstSetSupported =
    FirmwareContextHartSpecific->IsaExtensionSupported;
  ProcessorSpecificDataHob->ProcessorSpecificData.PrivilegeModeSupported =
    SMBIOS_RISC_V_PSD_MACHINE_MODE_SUPPORTED;
  if ((ProcessorSpecificDataHob->ProcessorSpecificData.InstSetSupported &
       RISC_V_ISA_SUPERVISOR_MODE_IMPLEMENTED) != 0)
  {
    ProcessorSpecificDataHob->ProcessorSpecificData.PrivilegeModeSupported |=
      SMBIOS_RISC_V_PSD_SUPERVISOR_MODE_SUPPORTED;
  }

  if ((ProcessorSpecificDataHob->ProcessorSpecificData.InstSetSupported &
       RISC_V_ISA_USER_MODE_IMPLEMENTED) != 0)
  {
    ProcessorSpecificDataHob->ProcessorSpecificData.PrivilegeModeSupported |=
      SMBIOS_RISC_V_PSD_USER_MODE_SUPPORTED;
  }

  ProcessorSpecificDataHob->ProcessorSpecificData.MachineVendorId.Value64_L =
    FirmwareContextHartSpecific->MachineVendorId.Value64_L;
  ProcessorSpecificDataHob->ProcessorSpecificData.MachineVendorId.Value64_H =
    FirmwareContextHartSpecific->MachineVendorId.Value64_H;
  ProcessorSpecificDataHob->ProcessorSpecificData.MachineArchId.Value64_L =
    FirmwareContextHartSpecific->MachineArchId.Value64_L;
  ProcessorSpecificDataHob->ProcessorSpecificData.MachineArchId.Value64_H =
    FirmwareContextHartSpecific->MachineArchId.Value64_H;
  ProcessorSpecificDataHob->ProcessorSpecificData.MachineImplId.Value64_L =
    FirmwareContextHartSpecific->MachineImplId.Value64_L;
  ProcessorSpecificDataHob->ProcessorSpecificData.MachineImplId.Value64_H =
    FirmwareContextHartSpecific->MachineImplId.Value64_H;
  return EFI_SUCCESS;
}

/**
  Print debug information of the processor specific data for a hart.

  @param  ProcessorSpecificDataHob     Pointer to RISC_V_PROCESSOR_SPECIFIC_DATA_HOB
**/
VOID
EFIAPI
DebugPrintHartSpecificInfo (
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA  *ProcessorSpecificDataHob
  )
{
  DEBUG ((DEBUG_INFO, "        *HartId = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.HartId.Value64_L));
  DEBUG ((DEBUG_INFO, "        *Is Boot Hart? = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.BootHartId));
  DEBUG ((DEBUG_INFO, "        *PrivilegeModeSupported = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.PrivilegeModeSupported));
  DEBUG ((DEBUG_INFO, "        *MModeExcepDelegation = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.MModeExcepDelegation.Value64_L));
  DEBUG ((DEBUG_INFO, "        *MModeInterruptDelegation = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.MModeInterruptDelegation.Value64_L));
  DEBUG ((DEBUG_INFO, "        *HartXlen = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.HartXlen));
  DEBUG ((DEBUG_INFO, "        *MachineModeXlen = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.MachineModeXlen));
  DEBUG ((DEBUG_INFO, "        *SupervisorModeXlen = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.SupervisorModeXlen));
  DEBUG ((DEBUG_INFO, "        *UserModeXlen = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.UserModeXlen));
  DEBUG ((DEBUG_INFO, "        *InstSetSupported = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.InstSetSupported));
  DEBUG ((DEBUG_INFO, "        *MachineVendorId = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.MachineVendorId.Value64_L));
  DEBUG ((DEBUG_INFO, "        *MachineArchId = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.MachineArchId.Value64_L));
  DEBUG ((DEBUG_INFO, "        *MachineImplId = 0x%x\n", ProcessorSpecificDataHob->ProcessorSpecificData.MachineImplId.Value64_L));
}

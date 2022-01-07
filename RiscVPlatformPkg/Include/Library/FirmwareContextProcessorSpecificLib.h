/** @file
  Firmware Context Processor-specific common library

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FIRMWARE_CONTEXT_PROCESSOR_SPECIFIC_LIB_H_
#define FIRMWARE_CONTEXT_PROCESSOR_SPECIFIC_LIB_H_

#include <IndustryStandard/RiscVOpensbi.h>
#include <PiPei.h>
#include <ProcessorSpecificHobData.h>

/**
  Build up common firmware context processor-specific information

  @param  FirmwareContextHartSpecific  Pointer to EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC
  @param  ParentProcessorGuid          Pointer to GUID of Processor which contains this core
  @param  ParentProcessorUid           Unique ID of physical processor which owns this core.
  @param  CoreGuid                     Pointer to GUID of core
  @param  HartId                       Hart ID of this core.
  @param  IsBootHart                   This is boot hart or not
  @param  ProcessorSpecDataHob         Pointer to RISC_V_PROCESSOR_SPECIFIC_DATA_HOB

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
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA        *ProcessorSpecDataHob
  );

/**
  Print debug information of the processor specific data for a hart

  @param  ProcessorSpecificDataHob     Pointer to RISC_V_PROCESSOR_SPECIFIC_DATA_HOB
**/
VOID
EFIAPI
DebugPrintHartSpecificInfo (
  RISC_V_PROCESSOR_SPECIFIC_HOB_DATA  *ProcessorSpecificDataHob
  );

#endif

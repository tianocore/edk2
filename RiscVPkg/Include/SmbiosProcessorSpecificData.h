/** @file
  Industry Standard Definitions of RISC-V Processor Specific data defined in
  below link for compliant with SMBIOS Table Specification v3.3.0.
  https://github.com/riscv/riscv-smbios

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA_H_
#define SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA_H_

#include <IndustryStandard/SmBios.h>
#include <RiscVImpl.h>

#pragma pack(1)

typedef enum {
  RegisterUnsupported = 0x00,
  RegisterLen32       = 0x01,
  RegisterLen64       = 0x02,
  RegisterLen128      = 0x03
} RISC_V_REGISTER_LENGTH;

#define SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA_REVISION  0x100

#define SMBIOS_RISC_V_PSD_MACHINE_MODE_SUPPORTED     (0x01 << 0)
#define SMBIOS_RISC_V_PSD_SUPERVISOR_MODE_SUPPORTED  (0x01 << 2)
#define SMBIOS_RISC_V_PSD_USER_MODE_SUPPORTED        (0x01 << 3)
#define SMBIOS_RISC_V_PSD_DEBUG_MODE_SUPPORTED       (0x01 << 7)

///
/// RISC-V processor specific data for SMBIOS type 44
///
typedef struct {
  UINT16           Revision;
  UINT8            Length;
  RISCV_UINT128    HartId;
  UINT8            BootHartId;
  RISCV_UINT128    MachineVendorId;
  RISCV_UINT128    MachineArchId;
  RISCV_UINT128    MachineImplId;
  UINT32           InstSetSupported;
  UINT8            PrivilegeModeSupported;
  RISCV_UINT128    MModeExcepDelegation;
  RISCV_UINT128    MModeInterruptDelegation;
  UINT8            HartXlen;
  UINT8            MachineModeXlen;
  UINT8            Reserved;
  UINT8            SupervisorModeXlen;
  UINT8            UserModeXlen;
} SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA;

#pragma pack()
#endif

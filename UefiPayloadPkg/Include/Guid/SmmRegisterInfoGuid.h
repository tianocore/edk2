/** @file
  This file defines the SMM info hob structure.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PAYLOAD_SMM_REGISTER_INFO_GUID_H_
#define PAYLOAD_SMM_REGISTER_INFO_GUID_H_

#include <IndustryStandard/Acpi.h>

///
/// SMM Information GUID
///
extern EFI_GUID  gSmmRegisterInfoGuid;

///
/// Reuse ACPI definition
/// AddressSpaceId(0xC0-0xFF) is defined by OEM for MSR and other spaces
///
typedef EFI_ACPI_3_0_GENERIC_ADDRESS_STRUCTURE PLD_GENERIC_ADDRESS;

#define REGISTER_ID_SMI_GBL_EN       1
#define REGISTER_ID_SMI_GBL_EN_LOCK  2
#define REGISTER_ID_SMI_EOS          3
#define REGISTER_ID_SMI_APM_EN       4
#define REGISTER_ID_SMI_APM_STS      5

#pragma pack(1)
typedef struct {
  UINT64                 Id;
  UINT64                 Value;
  PLD_GENERIC_ADDRESS    Address;
} PLD_GENERIC_REGISTER;

typedef struct {
  UINT16                  Revision;
  UINT16                  Reserved;
  UINT32                  Count;
  PLD_GENERIC_REGISTER    Registers[0];
} PLD_SMM_REGISTERS;

#pragma pack()

#endif

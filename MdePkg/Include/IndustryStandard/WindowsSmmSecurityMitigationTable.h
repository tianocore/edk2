/** @file
  Defines Windows SMM Security Mitigation Table
  @ https://learn.microsoft.com/en-us/windows-hardware/drivers/bringup/acpi-system-description-tables#windows-smm-security-mitigations-table-wsmt

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Supported versions of Windows operating systems read the WSMT early
  during initialization, prior to start of the ACPI interpreter and
  subsequent evaluation of the _OSI method. The Protection Flags field
  indicates the presence of specific BIOS security mitigations in system
  firmware.  Firmware setting any of the Protections Flags represents an
  attestation by the platform to OSPM that the corresponding firmware
  feature or coding practice has been implemented.  OSPM may not be able
  to functionally validate that this is indeed the case, and must rely
  on system firmware to accurately represent its capabilities.
  Windows operating systems may elect to enable, disable, or de-feature
  certain security features based on the presence of these SMM
  Protections Flags.
**/

#pragma once

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE  SIGNATURE_32('W', 'S', 'M', 'T')

#pragma pack(1)

#define EFI_WSMT_TABLE_REVISION  1

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         ProtectionFlags;
} EFI_ACPI_WSMT_TABLE;

// FIXED_COMM_BUFFERS
// If set, expresses that for all synchronous SMM entries, SMM will
// validate that input and output buffers lie entirely within the
// expected fixed memory regions.
// Firmware setting this bit should refer to the SMM Communication
// ACPI Table defined in the UEFI 2.6 specification.  Firmware should
// also consider all other possible data exchanges between SMM and
// non-SMM, including but not limited to EFI_SMM_COMMUNICATION_PROTOCOL,
// ACPINVS in ASL code, general purpose registers as buffer pointers,
// etc.
#define EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS  0x1

// COMM_BUFFER_NESTED_PTR_PROTECTION
// If set, expresses that for all synchronous SMM entries, SMM will
// validate that input and output pointers embedded within the fixed
// communication buffer only refer to address ranges that lie entirely
// within the expected fixed memory regions.
// Firmware setting this bit must also set the FIXED_COMM_BUFFERS bit.
#define EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION  0x2

// SYSTEM_RESOURCE_PROTECTION
// Firmware setting this bit is an indication that it will not allow
// reconfiguration of system resources via non-architectural mechanisms.
// After ExitBootServices(), firmware setting this bit shall not allow
// any software to make changes to the locations of: IOMMU’s, interrupt
// controllers, PCI Configuration Space, the Firmware ACPI Control
// Structure (FACS), or any registers reported through ACPI fixed
// tables (e.g. PMx Control registers, reset register, etc.).
// This also includes disallowing changes to RAM layout and ensuring
// that decodes to RAM and any system resources as described above take
// priority over software configurable registers.  For example, if
// software configures a PCI Express BAR to overlay RAM, accesses by
// the CPU to the affected system physical addresses must decode to
// RAM.
#define EFI_WSMT_PROTECTION_FLAGS_SYSTEM_RESOURCE_PROTECTION  0x4

#pragma pack()

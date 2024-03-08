/** @file
  APIC ID list retrieved for an SEV-ES/SEV-SNP guest via the GHCB.

  Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GHCB_APIC_IDS_H_
#define GHCB_APIC_IDS_H_

#define GHCB_APIC_IDS_GUID \
  { 0xbc964338, 0xee39, 0x4fc8, { 0xa2, 0x24, 0x10, 0x10, 0x8b, 0x17, 0x80, 0x1b }}

extern EFI_GUID  gGhcbApicIdsGuid;

#endif

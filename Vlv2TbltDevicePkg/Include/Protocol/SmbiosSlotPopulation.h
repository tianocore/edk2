/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

    SmbiosSlotPopulation.h

Abstract:

    EFI SMBIOS slot structure control code.

GUID:
    {EF7BF7D6-F8FF-4a76-8247-C0D0D1CC49C0}
    0xef7bf7d6, 0xf8ff, 0x4a76, 0x82, 0x47, 0xc0, 0xd0, 0xd1, 0xcc, 0x49, 0xc0

Revision History

**/

#ifndef _EFI_SMBIOS_SLOT_POPULATION_H_
#define _EFI_SMBIOS_SLOT_POPULATION_H_

//
// Slot Population Protocol GUID
//
#define EFI_SMBIOS_SLOT_POPULATION_GUID \
  { 0xef7bf7d6, 0xf8ff, 0x4a76, 0x82, 0x47, 0xc0, 0xd0, 0xd1, 0xcc, 0x49, 0xc0 }

typedef struct {
  UINT16      SmbiosSlotId;   // SMBIOS Slot ID
  BOOLEAN     InUse;          // Does the slot have a card in it
  BOOLEAN     Disabled;       // Should the slot information be in SMBIOS
} EFI_SMBIOS_SLOT_ENTRY;

typedef struct {
  UINT32                NumberOfEntries;
  EFI_SMBIOS_SLOT_ENTRY *SlotEntries;
} EFI_SMBIOS_SLOT_POPULATION_INFO;

extern EFI_GUID gEfiSmbiosSlotPopulationGuid;

#endif

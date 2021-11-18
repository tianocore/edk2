/** @file
  Publishes ESRT table from Firmware Management Protocol instances

  Copyright (c) 2016, Microsoft Corporation
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Guid/SystemResourceTable.h>

/**
  Function to print a single ESRT Entry (ESRE) to the debug console.

  Print Format:
  | 00000000-0000-0000-0000-000000000000 | SSSSSSSSSSSS | 0x00000000 | 0x00000000 | 0x00000000 | 0x00000000 | 0x00000000 |

  @param[in]  Entry  - Pointer to an ESRE entry
  @retval  EFI_SUCCESS
                        EFI_INVALID_PARAMETER
**/
EFI_STATUS
EFIAPI
PrintOutEsrtEntry (
  IN EFI_SYSTEM_RESOURCE_ENTRY  *Entry
  )
{
  if (Entry == NULL) {
    DEBUG ((DEBUG_INFO, "| ERROR:  Invalid resource entry pointer                           "));
    DEBUG ((DEBUG_INFO, "                                                    |\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // GUID FW Class (36 chars plus table formatting)
  //
  DEBUG ((DEBUG_INFO, "| %g |", &Entry->FwClass));

  //
  // Entry Type (12 chars plus table formatting)
  //
  switch (Entry->FwType) {
    case (ESRT_FW_TYPE_SYSTEMFIRMWARE):
      DEBUG ((DEBUG_INFO, " System FW    |"));
      break;
    case (ESRT_FW_TYPE_DEVICEFIRMWARE):
      DEBUG ((DEBUG_INFO, " Device FW    |"));
      break;
    case (ESRT_FW_TYPE_UEFIDRIVER):
      DEBUG ((DEBUG_INFO, " Uefi Driver  |"));
      break;
    case (ESRT_FW_TYPE_UNKNOWN):
      DEBUG ((DEBUG_INFO, " Unknown Type |"));
      break;
    default:
      DEBUG ((DEBUG_INFO, " ? 0x%8X |", Entry->FwType));
      break;
  }

  //
  // FW Version (10 char UINT32 string plus table formatting)
  // Lowest Supported Version (10 char UINT32 string plus table formatting)
  // Capsule Flags (10 char UINT32 string plus table formatting)
  // Last Attempt Version (10 char UINT32 string plus table formatting)
  // Last Attempt Status (10 char UINT32 string plus table formatting)
  //
  DEBUG ((
    DEBUG_INFO,
    " 0x%8X | 0x%8X | 0x%8X | 0x%8X | 0x%8X |\n",
    Entry->FwVersion,
    Entry->LowestSupportedFwVersion,
    Entry->CapsuleFlags,
    Entry->LastAttemptVersion,
    Entry->LastAttemptStatus
    ));

  return EFI_SUCCESS;
}

/**
  Function to print the ESRT table to the debug console.

  @param[in]  Table  - Pointer to the ESRT table
**/
VOID
EFIAPI
PrintTable (
  IN EFI_SYSTEM_RESOURCE_TABLE  *Table
  )
{
  EFI_SYSTEM_RESOURCE_ENTRY  *Entry;
  UINTN                      Index;

  Entry = (EFI_SYSTEM_RESOURCE_ENTRY *)(((UINT8 *)Table) + sizeof (EFI_SYSTEM_RESOURCE_TABLE));

  //
  // Print ESRT table information
  //
  DEBUG ((DEBUG_INFO, "ESRT Table Information:\n"));
  if (Table == NULL) {
    DEBUG ((DEBUG_INFO, "ERROR:  Invalid table pointer\n"));
    return;
  }

  DEBUG ((DEBUG_INFO, "+--------------------------------------------------------+\n"));
  DEBUG ((DEBUG_INFO, "| Firmware Resource Count          : 0x%08x          |\n", Table->FwResourceCount));
  DEBUG ((DEBUG_INFO, "| Firmware Resource Count Max      : 0x%08x          |\n", Table->FwResourceCountMax));
  DEBUG ((DEBUG_INFO, "| Firmware Resource Entry Version  : 0x%016x  |\n", Table->FwResourceVersion));
  DEBUG ((DEBUG_INFO, "+--------------------------------------------------------+\n"));

  //
  // Print table entry information
  //
  DEBUG ((DEBUG_INFO, "ESRT Table Entries:\n"));
  if (Table->FwResourceVersion != EFI_SYSTEM_RESOURCE_TABLE_FIRMWARE_RESOURCE_VERSION) {
    DEBUG ((DEBUG_INFO, "ERROR:  Unsupported Resource Entry Version\n"));
    return;
  }

  DEBUG ((DEBUG_INFO, "+--------------------------------------+--------------+------------"));
  DEBUG ((DEBUG_INFO, "+------------+------------+------------+------------+\n"));
  DEBUG ((DEBUG_INFO, "|                                      |              |            "));
  DEBUG ((DEBUG_INFO, "| Lowest     |            | Last       | Last       |\n"));
  DEBUG ((DEBUG_INFO, "|                                      | Firmware     |            "));
  DEBUG ((DEBUG_INFO, "| Supported  | Capsule    | Attempted  | Attempted  |\n"));
  DEBUG ((DEBUG_INFO, "| CLASS GUID                           | Type         | Version    "));
  DEBUG ((DEBUG_INFO, "| Version    | Flags      | Version    | Status     |\n"));
  DEBUG ((DEBUG_INFO, "+--------------------------------------+--------------+------------"));
  DEBUG ((DEBUG_INFO, "+------------+------------+------------+------------+\n"));

  for (Index = 0; Index < Table->FwResourceCount; Index++) {
    PrintOutEsrtEntry (&(Entry[Index]));
  }

  DEBUG ((DEBUG_INFO, "+--------------------------------------+--------------+------------"));
  DEBUG ((DEBUG_INFO, "+------------+------------+------------+------------+\n"));
}

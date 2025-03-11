/** @file
  Dynamic Table Manager Dxe

  Copyright (c) 2017 - 2024, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DYNAMIC_TABLE_MANAGER_DXE_H_
#define DYNAMIC_TABLE_MANAGER_DXE_H_

#include <AcpiTableGenerator.h>

///
/// Bit definitions for acceptable ACPI table presence formats.
/// Currently only ACPI tables present in the ACPI info list and
/// already installed will count towards "Table Present" during
/// verification routine.
///
#define ACPI_TABLE_PRESENT_INFO_LIST  BIT0
#define ACPI_TABLE_PRESENT_INSTALLED  BIT1

/// The FADT table must be placed at index 0 in mAcpiVerifyTables.
#define ACPI_TABLE_VERIFY_FADT  0

///
/// Private data structure to verify the presence of mandatory
/// or optional ACPI tables.
///
typedef struct {
  /// ESTD ID for the ACPI table of interest.
  ESTD_ACPI_TABLE_ID    EstdTableId;
  /// Standard UINT32 ACPI signature.
  UINT32                AcpiTableSignature;
  /// 4 character ACPI table name (the 5th char8 is for null terminator).
  CHAR8                 AcpiTableName[sizeof (UINT32) + 1];
  /// Indicator on whether the ACPI table is required.
  BOOLEAN               IsMandatory;
  /// Formats of verified presences, as defined by ACPI_TABLE_PRESENT_*
  /// This field should be initialized to 0 and will be populated during
  /// verification routine.
  UINT16                Presence;
} ACPI_TABLE_PRESENCE_INFO;

/** Get the arch specific ACPI table presence information.

  @param [out] PresenceArray      Array containing the ACPI tables to check.
  @param [out] PresenceArrayCount Count of elements in the PresenceArray.
  @param [out] FadtIndex          Index of the FADT table in the PresenceArray.
                                  -1 if absent.

  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
EFIAPI
GetAcpiTablePresenceInfo (
  OUT ACPI_TABLE_PRESENCE_INFO  **PresenceArray,
  OUT UINT32                    *PresenceArrayCount,
  OUT INT32                     *FadtIndex
  );

/** ACPI table Protocol ready event handler.

  This event notification indicates that the ACPI protocol is ready.
  Therefore, dispatch the building of the ACPI tables.

  @param  [in]  Event     The Event that is signalled.
  @param  [in]  Context   The Context information.

  @retval None
**/
VOID
EFIAPI
AcpiTableProtocolReady (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/** SMBIOS table Protocol ready event handler.

  This event notification indicates that the SMBIOS protocol is ready.
  Therefore, dispatch the building of the SMBIOS tables.

  @param  [in]  Event     The Event that is signalled.
  @param  [in]  Context   The Context information.

  @retval None
**/
VOID
EFIAPI
SmbiosProtocolReady (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

#endif // DYNAMIC_TABLE_MANAGER_DXE_H_

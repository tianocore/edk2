/** @file
  BERT table parser

  Copyright (c) 2026, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.6 Specification - January 2025
**/

#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewConfig.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  mAcpiHdrInfo;
STATIC CONST UINT32                  *mBootErrorRegionLength;

/**
  Check that an entire physical address range is mapped as reserved memory.

  @param [in] BaseAddress  Physical base address of the range.
  @param [in] Length       Length of the range in bytes.

  @retval EFI_SUCCESS           The entire range is mapped as reserved memory.
  @retval EFI_INVALID_PARAMETER The range is invalid.
  @retval EFI_NOT_FOUND         Some or all of the range is not in the memory
                                map.
  @retval EFI_NO_MAPPING        Some or all of the range is not mapped as
                                reserved memory.
  @retval EFI_OUT_OF_RESOURCES  The memory map could not be allocated.
  @retval Others                An error returned by GetMemoryMap().
**/
STATIC
EFI_STATUS
CheckRangeReserved (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  EFI_PHYSICAL_ADDRESS   CurrentAddress;
  EFI_PHYSICAL_ADDRESS   DescriptorEnd;
  EFI_PHYSICAL_ADDRESS   RangeEnd;
  EFI_MEMORY_DESCRIPTOR  *Descriptor;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_STATUS             Status;
  UINT32                 DescriptorVersion;
  UINT64                 DescriptorLength;
  UINTN                  DescriptorSize;
  UINTN                  MapKey;
  UINTN                  MemoryMapSize;
  UINTN                  Offset;
  BOOLEAN                DescriptorFound;

  if ((Length == 0) || (BaseAddress > (MAX_UINT64 - Length))) {
    return EFI_INVALID_PARAMETER;
  }

  MemoryMap     = NULL;
  MemoryMapSize = 0;
  Status        = gBS->GetMemoryMap (
                         &MemoryMapSize,
                         MemoryMap,
                         &MapKey,
                         &DescriptorSize,
                         &DescriptorVersion
                         );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  do {
    MemoryMap = AllocatePool (MemoryMapSize);
    if (MemoryMap == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (MemoryMap);
      MemoryMap = NULL;
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  RangeEnd       = BaseAddress + Length;
  CurrentAddress = BaseAddress;

  while (CurrentAddress < RangeEnd) {
    DescriptorFound = FALSE;

    for (Offset = 0; Offset < MemoryMapSize; Offset += DescriptorSize) {
      Descriptor       = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + Offset);
      DescriptorLength = LShiftU64 (Descriptor->NumberOfPages, EFI_PAGE_SHIFT);
      DescriptorEnd    = Descriptor->PhysicalStart + DescriptorLength;
      if ((CurrentAddress < Descriptor->PhysicalStart) ||
          (CurrentAddress >= DescriptorEnd))
      {
        continue;
      }

      if (Descriptor->Type != EfiReservedMemoryType) {
        Status = EFI_NO_MAPPING;
        goto FreeMemoryMap;
      }

      CurrentAddress  = MIN (DescriptorEnd, RangeEnd);
      DescriptorFound = TRUE;
      break;
    }

    if (!DescriptorFound) {
      Status = EFI_NOT_FOUND;
      goto FreeMemoryMap;
    }
  }

  Status = EFI_SUCCESS;

FreeMemoryMap:
  FreePool (MemoryMap);
  return Status;
}

/**
  Validate the Boot Error Region length.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information.
**/
STATIC
VOID
EFIAPI
ValidateBootErrorRegionLength (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  CONST UINT32  RegionLength = *(UINT32 *)Ptr;

  if (RegionLength < sizeof (EFI_ACPI_6_6_GENERIC_ERROR_STATUS_STRUCTURE)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Boot Error Region Length must be at least 0x%x.",
      (UINT32)sizeof (EFI_ACPI_6_6_GENERIC_ERROR_STATUS_STRUCTURE)
      );
  }
}

/**
  Validate the Boot Error Region address.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information.
**/
STATIC
VOID
EFIAPI
ValidateBootErrorRegion (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  CONST UINT64  RegionAddress = *(UINT64 *)Ptr;
  EFI_STATUS    Status;

  if (RegionAddress == 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Boot Error Region address must not be zero.");
    return;
  }

  if ((mBootErrorRegionLength != NULL) &&
      (*mBootErrorRegionLength > (MAX_UINT64 - RegionAddress)))
  {
    IncrementErrorCount ();
    Print (L"\nERROR: Boot Error Region address range overflows.");
    return;
  }

  if ((mBootErrorRegionLength == NULL) ||
      (*mBootErrorRegionLength <
       sizeof (EFI_ACPI_6_6_GENERIC_ERROR_STATUS_STRUCTURE)))
  {
    return;
  }

  Status = CheckRangeReserved (RegionAddress, *mBootErrorRegionLength);
  if ((Status == EFI_NOT_FOUND) || (Status == EFI_NO_MAPPING)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Boot Error Region must be entirely mapped as "
      L"EfiReservedMemoryType."
      );
  } else if (EFI_ERROR (Status)) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Failed to check the Boot Error Region memory type. "
      L"Status = %r.",
      Status
      );
  }
}

/**
  An ACPI_PARSER array describing the ACPI BERT table.
**/
STATIC CONST ACPI_PARSER  mBertParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo),
  { L"Boot Error Region Length",    4,  36, L"0x%x",  NULL, (VOID **)&mBootErrorRegionLength, ValidateBootErrorRegionLength, NULL },
  { L"Boot Error Region",           8,  40, L"0x%lx", NULL, NULL,                             ValidateBootErrorRegion,       NULL }
};

/**
  This function parses the ACPI BERT table.
  When trace is enabled this function parses the BERT table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiBert (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  if (!Trace) {
    return;
  }

  ParseAcpi (
    Trace,
    0,
    "BERT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (mBertParser)
    );

  if (!GetConsistencyChecking ()) {
    return;
  }

  if (AcpiTableRevision > EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_REVISION) {
    IncrementWarningCount ();
    Print (
      L"WARNING: BERT revision is newer than the parser supports. "
      L"Newer fields might not be reported. Supported revision = %u, "
      L"Revision = %u.\n",
      EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_REVISION,
      AcpiTableRevision
      );
  }

  if (AcpiTableLength != sizeof (EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_HEADER)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: BERT length must be %u. Length = %u.\n",
      (UINT32)sizeof (EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_HEADER),
      AcpiTableLength
      );
  }
}

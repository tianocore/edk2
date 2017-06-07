/** @file
*
*  Copyright (c) 2016 - 2017, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef ACPITABLEPARSER_H_
#define ACPITABLEPARSER_H_

// ACPI Table Parsers

/** This function parses the ACPI SPCR table.
  This function parses the SPCR table and optionally traces the ACPI
  table fields.

  This function also performs validations of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiSpcr (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );


/** This function parses the ACPI GTDT table.
  This function parses the GTDT table and optionally traces the ACPI
  table fields.

  This function also parses the following platform timer structures:
    - GT Block timer
    - Watchdog timer

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiGtdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );


/** This function parses the ACPI FADT table.
  This function parses the FADT table and optionally traces the ACPI
  table fields.

  This function also parses the ACPI header for the DSDT table and
  invokes the parser for the ACPI DSDT table.

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiFadt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );


/** This function parses the ACPI DSDT table.
  This function parses the DSDT table and optionally traces the ACPI
  table fields. For the DSDT table only the ACPI header fields are
  parsed and traced.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiDsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );


/** This function parses the ACPI SSDT table.
  This function parses the SSDT table and optionally traces the ACPI
  table fields. For the SSDT table only the ACPI header fields are
  parsed and traced.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiSsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );


/** This function parses the ACPI DBG2 table.
  This function parses the DBG2 table and optionally traces the ACPI
  table fields.

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiDbg2 (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/** This function parses the ACPI MCFG table.
  This function parses the MCFG table and optionally traces the ACPI
  table fields.

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiMcfg (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );


/** This function parses the ACPI MADT table.
  This function parses the MADT table and optionally traces the ACPI
  table fields.

  This function currently parses the following Interrupt Controller
  Structures:
    - GICC
    - GICD
    - GIC MSI Frame
    - GICR
    - GIC ITS

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiMadt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );



/** This function parses the ACPI SRAT table.
  This function parses the SRAT table and optionally traces the ACPI
  table fields.

  This function parses the following Resource Allocation Structures:
    - Processor Local APIC/SAPIC Affinity Structure
    - Memory Affinity Structure
    - Processor Local x2APIC Affinity Structure
    - GICC Affinity Structure

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiSrat (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/** This function parses the ACPI SLIT table.
  This function parses the SLIT table and optionally traces the ACPI
  table fields.

  This function also validates System Localities for the following:
    - Diagonal elements have a normalized value of 10
    - Relative distance from System Locality at i*N+j is same as
      j*N+i

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiSlit (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/** This function parses the ACPI IORT table.
  This function parses the IORT table and optionally traces the ACPI
  table fields.

  This function also parses the following nodes:
    - ITS Group
    - Named Component
    - Root Complex
    - SMMUv1/2
    - SMMUv3

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiIort (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );


/** This function parses the ACPI XSDT table
  and optionally traces the ACPI table fields.

  This function also performs validation of the XSDT table.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiXsdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/** This function parses the ACPI BGRT table.
  This function parses the BGRT table and optionally traces the ACPI
  table fields.

  This function also parses the ACPI header for the DSDT table and
  invokes the parser for the ACPI DSDT table.

  This function also performs validation of the ACPI table fields.

  @params [in] Trace              If TRUE, trace the ACPI fields.
  @params [in] Ptr                Pointer to the start of the buffer.
  @params [in] AcpiTableLength    Length of the ACPI table.
  @params [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
ParseAcpiBgrt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  );

/** This function processes the ACPI tables.
  This function calls ProcessTableReportOptions() to list the ACPI
  tables, perform binary dump of the tables and determine if the
  ACPI fields should be traced.

  This function also invokes the parser for the ACPI tables.

  This function also performs a RAW dump of the ACPI table including
  the unknown/unparsed ACPI tables and validates the checksum.

  @params [in] Ptr                Pointer to the start of the ACPI
                                  table data buffer.
**/
VOID
ProcessAcpiTable (
  IN UINT8* Ptr
  );


/** This function parses the ACPI RSDP table.
  This function parses the RSDP table and optionally traces the ACPI
  table fields. ProcessTableReportOptions() is called to determine if
  the ACPI fields should be traced.

  This function invokes the parser for the XSDT table.
  * Note - This function does not support parsing of RSDT table.

  This function also performs a RAW dump of the ACPI table and
  validates the checksum.

  @params [in] Ptr                Pointer to the start of the buffer.

  @retval EFI_SUCCESS             Success.
  @retval EFI_NOT_FOUND           Valid XSDT pointer not found.
**/
EFI_STATUS
ParseRsdp (
  IN UINT8* Ptr
  );

#endif // ACPITABLEPARSER_H_

/** @file
  HEST table parser

  Copyright (c) 2024, Arm Limited.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - ACPI 6.5, Table 18.3.2 ACPI Error Source
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>

#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"

STATIC ACPI_DESCRIPTION_HEADER_INFO  mAcpiHdrInfo;
STATIC UINT32                        *mHestErrorSourceCount;
STATIC UINT16                        *mHestErrorSourceType;
STATIC UINT8                         *mHestIA32HardwareBankCount;

/**
  An String array for Error Notification Structure's type.
  Cf ACPI 6.5 Table 18.14: Hardware Error Notification Structure
**/
STATIC CONST CHAR16  *HestErrorNotificationStructureTypeStr[] = {
  L"Polled",
  L"External Interrupt",
  L"Local Interrupt",
  L"SCI",
  L"NMI",
  L"CMCI",
  L"MCE",
  L"GPIO-Signal",
  L"ARMv8 SEA",
  L"ARMv8 SEI",
  L"External Interrupt - GSIV",
  L"Software Delegated Exception",
};

/**
  An ACPI_PARSER array describing the ACPI HEST Table.
**/
STATIC CONST ACPI_PARSER  HestParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo),
  { L"Error Source Count",          4,     36, L"%d", NULL,
    (VOID **)&mHestErrorSourceCount,NULL,  NULL },
  // Error Source Descriptor 1
  // Error Source Descriptor Type
  // Error Source Descriptor Data
  // ...
  // Error Source Descriptor 2
  // Error Source Descriptor Type
  // Error Source Descriptor Data
  // ...
  // ....
  // Error Source Descriptor n
  // Error Source Descriptor Type
  // Error Source Descriptor Data
  // ...
};

/**
  An ACPI_PARSER array describing the HEST error source descriptor type.
**/
STATIC CONST ACPI_PARSER  HestErrorSourceTypeParser[] = {
  { L"Type", 2, 0, L"%d", NULL, (VOID **)&mHestErrorSourceType, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the HEST error source flags information.
**/
STATIC CONST ACPI_PARSER  HestErrorSourceFlags[] = {
  { L"Type",        1, 0, L"%d", NULL, NULL, NULL, NULL },
  { L"Global",      1, 1, L"%d", NULL, NULL, NULL, NULL },
  { L"GHES Assist", 1, 2, L"%d", NULL, NULL, NULL, NULL },
  { L"Reserved",    5, 3, NULL,  NULL, NULL, NULL, NULL }
};

/**
 An ACPI_PARSER array describing IA-32 Architecture Machine Check Bank Structure
 Cf ACPI 6.5 Table 18.4: IA-32 Architecture Machine Check Error Bank Structure
**/
STATIC CONST ACPI_PARSER  HestErrorIA32ArchMachineCheckBankStructureParser[] = {
  { L"Bank Number",                    1, 0,  L"%d",     NULL, NULL, NULL, NULL },
  { L"Clear Status On Initialization", 1, 1,  L"%d",     NULL, NULL, NULL, NULL },
  { L"Status Data Format",             1, 2,  L"%d",     NULL, NULL, NULL, NULL },
  { L"Reserved",                       1, 3,  NULL,      NULL, NULL, NULL, NULL },
  { L"Control Register MSR Address",   4, 4,  L"0x%lx",  NULL, NULL, NULL, NULL },
  { L"Control Init Data",              8, 8,  L"0x%llx", NULL, NULL, NULL, NULL },
  { L"Status Register MSR Address",    4, 16, L"0x%lx",  NULL, NULL, NULL, NULL },
  { L"Address Register MSR Address",   4, 20, L"0x%lx",  NULL, NULL, NULL, NULL },
  { L"Misc Register MSR Address",      4, 24, L"0x%lx",  NULL, NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the Hardware Error Notification Structure's
  Configuration Write Enable Field (CWE)
  Cf ACPI 6.5 Table 18.14: Hardware Error Notification Structure
**/
STATIC CONST ACPI_PARSER  HestErrorNotificationCweParser[] = {
  { L"Type",                               1,  0, L"%d",   NULL, NULL, NULL, NULL },
  { L"Poll Interval",                      1,  1, L"%d",   NULL, NULL, NULL, NULL },
  { L"Switch To Polling Threshold Value",  1,  2, L"%d",   NULL, NULL, NULL, NULL },
  { L"Switch To Polling Threshold Window", 1,  3, L"%d",   NULL, NULL, NULL, NULL },
  { L"Error Threshold Value",              1,  4, L"%d",   NULL, NULL, NULL, NULL },
  { L"Error Threshold Window",             1,  5, L"%d",   NULL, NULL, NULL, NULL },
  { L"Reserved",                           10, 6, L"0x%x", NULL, NULL, NULL, NULL },
};

/**
  This function validates the Type field of Hardware Error Notification Structure

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateErrorNotificationType (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  Type;

  Type = *(UINT8 *)Ptr;

  if (Type >
      EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_SOFTWARE_DELEGATED_EXCEPTION)
  {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Notification Structure Type must be <= 0x%x.",
      EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_SOFTWARE_DELEGATED_EXCEPTION
      );
  }
}

/**
  Dumps flags fields of error source descriptor.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpSourceFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%x\n", *Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    1,
    PARSER_PARAMS (HestErrorSourceFlags)
    );
}

/**
  Dumps type fields of Error Notification Structure

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpErrorNotificationType (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *Ptr);
    return;
  }

  if (*Ptr <= EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_SOFTWARE_DELEGATED_EXCEPTION) {
    Print (L"%s(0x%x)", HestErrorNotificationStructureTypeStr[*Ptr]);
  } else {
    Print (L"UNKNOWN(0x%x)", HestErrorNotificationStructureTypeStr[*Ptr]);
  }
}

/**
  Dumps Configuration Write Enable fields of Hardware Error Notification Structure.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpErrorNotificationCwe (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%x\n", *Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    1,
    PARSER_PARAMS (HestErrorNotificationCweParser)
    );
}

/**
  An ACPI_PARSER array describing the Hardware Error Notification Structure
  Cf ACPI 6.5 Table 18.14: Hardware Error Notification Structure
**/
STATIC CONST ACPI_PARSER  HestErrorNotificationParser[] = {
  { L"Type",                               1, 0,  NULL,     DumpErrorNotificationType, NULL, ValidateErrorNotificationType, NULL },
  { L"Length",                             1, 1,  L"%d",    NULL,                      NULL, NULL,                          NULL },
  { L"Configuration Write Enable",         2, 2,  NULL,     DumpErrorNotificationCwe,  NULL, NULL,                          NULL },
  { L"Pull Interval",                      4, 4,  L"%d ms", NULL,                      NULL, NULL,                          NULL },
  { L"Vector",                             4, 8,  L"%d",    NULL,                      NULL, NULL,                          NULL },
  { L"Switch To Polling Threshold Value",  4, 12, L"%d",    NULL,                      NULL, NULL,                          NULL },
  { L"Switch To Polling Threshold Window", 4, 16, L"%d ms", NULL,                      NULL, NULL,                          NULL },
  { L"Error Threshold Value",              4, 20, L"%d",    NULL,                      NULL, NULL,                          NULL },
  { L"Error Threshold Window",             4, 24, L"%d ms", NULL,                      NULL, NULL,                          NULL },
};

/**
  This function validates reserved bits of
  pci related Error source structure's bus field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidatePciBusReservedBits (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr != 0x00) {
    IncrementErrorCount ();
    Print (L"\nERROR: bits[31:24] should must be zero...");
  }
}

/**
  An ACPI_PARSER array describing the PCI related Error Source Bus field.
**/
STATIC CONST ACPI_PARSER  HestErrorSourcePciCommonBusParser[] = {
  { L"Bus",            8,  0,  L"%d",   NULL, NULL, NULL,                       NULL },
  { L"Segment Number", 16, 8,  L"%d",   NULL, NULL, NULL,                       NULL },
  { L"Reserved",       8,  24, L"0x%x", NULL, NULL, ValidatePciBusReservedBits, NULL },
};

/**
  This function validates the flags field of IA32 related
  error source descriptor structure.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateIA32ErrorSourceFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  SourceFlags;

  SourceFlags = *(UINT8 *)Ptr;

  if ((SourceFlags &
       ~(EFI_ACPI_6_5_ERROR_SOURCE_FLAG_FIRMWARE_FIRST |
         EFI_ACPI_6_5_ERROR_SOURCE_FLAG_GHES_ASSIST)) != 0)
  {
    IncrementErrorCount ();
    Print (L"\nERROR: Invalid IA32 source flags field value...");
  }

  if (((SourceFlags & EFI_ACPI_6_5_ERROR_SOURCE_FLAG_FIRMWARE_FIRST) != 0) &&
      ((SourceFlags & EFI_ACPI_6_5_ERROR_SOURCE_FLAG_GHES_ASSIST) != 0))
  {
    IncrementErrorCount ();
    Print (L"\nERROR: GHES_ASSIST should be reserved if FIRMWARE_FIRST is set...");
  }
}

/**
  This function validates the flags field of PCI related
  error source descriptor structure.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidatePciErrorSourceFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  SourceFlags;

  SourceFlags = *(UINT8 *)Ptr;

  if ((SourceFlags &
       ~(EFI_ACPI_6_5_ERROR_SOURCE_FLAG_FIRMWARE_FIRST |
         EFI_ACPI_6_5_ERROR_SOURCE_FLAG_GLOBAL)) != 0)
  {
    IncrementErrorCount ();
    Print (L"\nERROR: Invalid PCI source flags field value...");
  }
}

/**
  This function validates the flags field of Ghes related
  error source descriptor structure.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGhesSourceFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  SourceFlags;

  SourceFlags = *(UINT8 *)Ptr;

  if (SourceFlags != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Ghes'source flags should be reserved...");
  }
}

/**
  This function validates the enabled field of error source descriptor
  structure.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateEnabledField (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*(UINT8 *)Ptr > 1) {
    IncrementErrorCount ();
    Print (L"\nERROR: Invalid Enabled field value must be either 0 or 1.");
  }
}

/**
  This function validates the number of records to preallocated and
  max sections per record fields of error source descriptor
  structure.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRecordCount (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8    RecordCount;
  BOOLEAN  CheckRecordCount;

  RecordCount      = *Ptr;
  CheckRecordCount = ((BOOLEAN)(UINTN)Context);

  if ((CheckRecordCount) && (RecordCount == 0)) {
    IncrementErrorCount ();
    Print (L"\nERROR: Record count must be >= 1...");
  }
}

/**
  Dumps the notification structure fields.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpNotificationStructure (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  UINT32  Offset;
  UINT32  Size;

  Size = sizeof (EFI_ACPI_6_5_HARDWARE_ERROR_NOTIFICATION_STRUCTURE);
  Print (L"\n");
  Offset = ParseAcpi (
             TRUE,
             2,
             NULL,
             Ptr,
             Size,
             PARSER_PARAMS (HestErrorNotificationParser)
             );
  if (Offset != Size) {
    IncrementErrorCount ();
    Print (L"ERROR: Failed to parse Hardware Error Notification Structure!\n");
  }
}

/**
  Dumps bus field in the PCI related Error Source Structure.
                                      from HestTable.
  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpPciBus (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%x\n", *Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    1,
    PARSER_PARAMS (HestErrorSourcePciCommonBusParser)
    );
}

/**
  Dumps the IA32 Arch Machine Check Error Bank structure fields.

  @param [in]       HestTable         Start pointer to Hest table.
  @param [in]       AcpiTableLength   Length of HestTable.
  @param [in,out]   Offset            Offset to machine check bank structure
                                      from HestTable.

  @retval EFI_SUCCESS                   Success
  @retval EFI_INVALID_PARAMETER         Invalid Hest Table
**/
STATIC
EFI_STATUS
EFIAPI
DumpIA32ArchMachineCheckErrorBankStructure (
  IN UINT8  *HestTable,
  UINT32    AcpiTableLength,
  UINT32    *Offset
  )
{
  UINT8   Idx;
  UINT8   *IA32BankStructPtr;
  UINT32  TotalBankStructSize;

  TotalBankStructSize = *mHestIA32HardwareBankCount *
                        sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);

  if ((*Offset + TotalBankStructSize) > AcpiTableLength) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Not enough data for "
      "IA-32 Architecture Machine Check Exception Error source.\n"
      );
    return EFI_INVALID_PARAMETER;
  }

  for (Idx = 0; Idx < *mHestIA32HardwareBankCount; Idx++) {
    IA32BankStructPtr = HestTable + *Offset;
    ParseAcpi (
      TRUE,
      4,
      "IA-32 Architecture Machine Check Bank Structure",
      IA32BankStructPtr,
      sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE),
      PARSER_PARAMS (HestErrorIA32ArchMachineCheckBankStructureParser)
      );
    *Offset +=
      sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE);
  }

  *mHestIA32HardwareBankCount = 0;

  return EFI_SUCCESS;
}

/**
  Helper macro to populate the header fields of error source descriptor in the
  ACPI_PARSER array.
**/
#define PARSE_HEST_ERROR_SOURCE_COMMON_HEADER(FlagsValidateFunc, CheckRecordCount)    \
  { L"Type", 2, 0, L"%d", NULL, NULL, NULL, NULL },                                   \
  { L"Source Id", 2, 2, L"%d", NULL, NULL, NULL, NULL },                              \
  { L"Reserved", 2, 4, NULL, NULL, NULL, NULL, NULL },                                \
  { L"Flags", 1, 6, NULL, DumpSourceFlags, NULL,                                      \
    FlagsValidateFunc, NULL },                                                        \
  { L"Enabled", 1, 7, L"%d", NULL, NULL, ValidateEnabledField, NULL },                \
  { L"Number of Records to Pre-allocate", 4, 8, L"%d", NULL, NULL,                    \
    ValidateRecordCount, (VOID *) ((UINTN) CheckRecordCount) },                       \
  { L"Max Sections Per Record", 4, 12, L"%d", NULL, NULL,                             \
    ValidateRecordCount, (VOID *) ((UINTN) CheckRecordCount) }

/**
  Helper macro to populate the header fields of PCI related
  error source descriptor in the ACPI_PARSER array.
**/
#define PARSE_HEST_PCI_ERROR_SOURCE_COMMON_HEADER()                           \
  PARSE_HEST_ERROR_SOURCE_COMMON_HEADER(ValidatePciErrorSourceFlags, TRUE),   \
  { L"Bus", 4, 16, NULL, DumpPciBus, NULL, NULL, NULL },                      \
  { L"Device", 2, 20, L"%d", NULL, NULL, NULL, NULL },                        \
  { L"Function", 2, 22, L"%d", NULL, NULL, NULL, NULL },                      \
  { L"Device Control", 2, 24, L"%d", NULL, NULL, NULL, NULL },                \
  { L"Reserved", 2, 26, NULL, NULL, NULL, NULL, NULL },                       \
  { L"Uncorrectable Error Mask", 4, 28, L"0x%lx", NULL, NULL, NULL, NULL },   \
  { L"Uncorrectable Error Severity", 4, 32, L"%d", NULL, NULL, NULL, NULL },  \
  { L"Correctable Error Mask", 4, 36, L"0x%lx", NULL, NULL, NULL, NULL },     \
  { L"Advanced Error Capabilities and Control", 4, 40, L"%d", NULL, NULL,     \
    NULL, NULL }

/**
  Helper macro to populate the header fields of GHES related
  error source descriptor in the ACPI_PARSER array.
**/
#define PARSE_HEST_GHES_ERROR_SOURCE()                                        \
  { L"Type", 2, 0, L"%d", NULL, NULL, NULL, NULL },                           \
  { L"Source Id", 2, 2, L"%d", NULL, NULL, NULL, NULL },                      \
  { L"Related Source Id", 2, 4, L"0x%x", NULL, NULL, NULL, NULL },            \
  { L"Flags", 1, 6, L"0x%x", NULL, NULL, ValidateGhesSourceFlags, NULL },     \
  { L"Enabled", 1, 7, L"%d", NULL, NULL, ValidateEnabledField, NULL },        \
  { L"Number of Records to Pre-allocate", 4, 8, L"%d", NULL, NULL,            \
    ValidateRecordCount, (VOID *) ((UINTN) TRUE) },                           \
  { L"Max Sections Per Record", 4, 12, L"%d", NULL, NULL,                     \
    ValidateRecordCount, (VOID *) ((UINTN) TRUE) },                           \
  { L"Max Raw Data Length", 4, 16, L"%d", NULL, NULL, NULL, NULL },           \
  { L"Error Status Address", 12, 20, NULL, DumpGas, NULL, NULL, NULL },       \
  { L"Notification Structure", 28, 32, NULL, DumpNotificationStructure,       \
    NULL, NULL, NULL },                                                       \
  { L"Error Status Block Length", 4, 60, L"%d", NULL, NULL, NULL, NULL }

/**
  An ACPI_PARSER array describing the IA-32 Architecture Machine Check Exception
  error source descriptor.
  Cf ACPI 6.5 Table 18.3: IA-32 Architecture Machine Check Exception Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourceIA32ArchMachineCheckExceptionParser[] = {
  PARSE_HEST_ERROR_SOURCE_COMMON_HEADER (ValidateIA32ErrorSourceFlags, FALSE),
  { L"Global Capability Init Data",
    8,                                                                 16,    L"0x%llx",  NULL, NULL,                                 NULL, NULL },
  { L"Global Control Init Data",
    8,                                                                 24,    L"0x%llx",  NULL, NULL,                                 NULL, NULL },
  { L"Number of Hardware Banks",
    1,                                                                 32,    L"%d",      NULL, (VOID **)&mHestIA32HardwareBankCount, NULL, NULL },
  { L"Reserved",
    7,                                                                 33,    NULL,       NULL, NULL,                                 NULL, NULL },
  /// HestErrorIA32ArchMachineCheckBankStructureParser
  /// ...
};

/**
  An ACPI_PARSER array describing the IA-32 Architecture Machine Check Exception
  error source descriptor.
  Cf ACPI 6.5 Table 18.5: IA-32 Architecture Machine Check Exception Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourceIA32ArchCorrectedMachineCheckParser[] = {
  PARSE_HEST_ERROR_SOURCE_COMMON_HEADER (ValidateIA32ErrorSourceFlags, TRUE),
  { L"Notification Structure",
    28,                                                                16,   NULL,   DumpNotificationStructure, NULL,                                 NULL, NULL },
  { L"Number of Hardware Banks",
    1,                                                                 44,   L"%d",  NULL,                      (VOID **)&mHestIA32HardwareBankCount, NULL, NULL },
  { L"Reserved",
    3,                                                                 45,   NULL,   NULL,                      NULL,                                 NULL, NULL },
  /// HestErrorIA32ArchMachineCheckBankStructureParser
  /// ...
};

/**
  An ACPI_PARSER array describing the IA-32 Non-Maskable Interrupt
  error source descriptor.
  Cf ACPI 6.5 Table 18.6: IA-32 Architecture NMI Error Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourceIA32ArchNonMaskableInterruptParser[] = {
  { L"Type",                              2, 0,  L"%d", NULL, NULL, NULL, NULL },
  { L"Source Id",                         2, 2,  L"%d", NULL, NULL, NULL, NULL },
  { L"Reserved",                          4, 4,  NULL,  NULL, NULL, NULL, NULL },
  { L"Number of Records to Pre-allocate", 4, 8,  L"%d", NULL, NULL,
    ValidateRecordCount, (VOID *)((UINTN)TRUE) },
  { L"Max Sections Per Record",           4, 12, L"%d", NULL, NULL,
    ValidateRecordCount, (VOID *)((UINTN)TRUE) },
  { L"Max Raw Data Length",               4, 16, L"%d", NULL, NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the HEST PCIe Root Port AER
  error source descriptor.
  Cf ACPI 6.5 Table 18.7: PCI Express Root Port AER Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourcePciExpressRootPortAerParser[] = {
  PARSE_HEST_PCI_ERROR_SOURCE_COMMON_HEADER (),
  { L"Root Error Command",
    4,                                         44,L"%d", NULL, NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the HEST PCIe Device AER
  error source descriptor.
  Cf ACPI 6.5 Table 18.8: PCI Express Device AER Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourcePciExpressDeviceAerParser[] = {
  PARSE_HEST_PCI_ERROR_SOURCE_COMMON_HEADER (),
};

/**
  An ACPI_PARSER array describing the HEST PCIe/PCI-X Bridge AER
  error source descriptor.
  Cf ACPI 6.5 Table 18.9: PCI Express/PCI-X Bridge AER Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourcePciExpressBridgeAerParser[] = {
  PARSE_HEST_PCI_ERROR_SOURCE_COMMON_HEADER (),
  { L"Secondary Uncorrectable Error Mask",
    4,                                         44,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"Secondary Uncorrectable Error Severity",
    4,                                         48,  L"%d",    NULL, NULL, NULL, NULL },
  { L"Secondary Advanced Error Capabilities and Control",
    4,                                         52,  L"%d",    NULL, NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the HEST GHES error source descriptor.
  Cf ACPI 6.5 Table 18.10: Generic Hardware Error Source Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourceGhesParser[] = {
  PARSE_HEST_GHES_ERROR_SOURCE (),
};

/**
  An ACPI_PARSER array describing the HEST GHESv2 error source descriptor.
  Cf ACPI 6.5 Table 18.11: Generic Hardware Error Source version 2 Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourceGhesv2Parser[] = {
  PARSE_HEST_GHES_ERROR_SOURCE (),
  { L"Read Ack Register",         12, 64, NULL,    DumpGas, NULL, NULL, NULL },
  { L"Read Ack Preserve",         8,  76, L"%llx", NULL,    NULL, NULL, NULL },
  { L"Read Ack Write",            8,  84, L"%llx", NULL,    NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the IA-32 Architecture Deferred Machine Check
  error source descriptor.
  Cf ACPI 6.5 Table 18.15: IA-32 Architecture Deferred Machine Check Structure
**/
STATIC CONST ACPI_PARSER  HestErrorSourceIA32ArchDeferredMachineCheckParser[] = {
  PARSE_HEST_ERROR_SOURCE_COMMON_HEADER (ValidateIA32ErrorSourceFlags, TRUE),
  { L"Notification Structure",                                         28,   16,  NULL,  DumpNotificationStructure,
    NULL,                                                              NULL, NULL },
  { L"Number of Hardware Banks",                                       1,    44,  L"%d", NULL,
    (VOID **)&mHestIA32HardwareBankCount,                              NULL, NULL },
  { L"Reserved",                                                       3,    45,  NULL,  NULL,                     NULL,NULL, NULL },
  /// HestErrorIA32ArchMachineCheckBankStructureParser
  /// ...
};

/**
  This function parses the ACPI HEST table.
  When trace is enabled this function parses the HEST table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiHest (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  EFI_STATUS  Status;
  UINT32      Offset;
  UINT8       *ErrorSourcePtr;
  UINT32      ParsedErrorSourceCount;
  UINT32      CurErrorSourceType;

  if (Trace != TRUE) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "HEST",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (HestParser)
             );

  // Validate Error Source Descriptors Count.
  if (mHestErrorSourceCount == NULL) {
    IncrementErrorCount ();
    Print (L"ERROR: Invalid Hardware Error Source Table Header...\n");
    return;
  }

  ParsedErrorSourceCount = 0;
  CurErrorSourceType     = EFI_ACPI_6_5_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION;

  while ((Offset < AcpiTableLength) && (ParsedErrorSourceCount < *mHestErrorSourceCount)) {
    ErrorSourcePtr = Ptr + Offset;

    // Get Type of Error Source Descriptor.
    ParseAcpi (
      FALSE,
      0,
      NULL,
      ErrorSourcePtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (HestErrorSourceTypeParser)
      );

    // Validate Error Source Descriptors Type.
    if (mHestErrorSourceType == NULL) {
      IncrementErrorCount ();
      Print (L"ERROR: Invalid Error Source Structure...\n");
      return;
    }

    if (CurErrorSourceType > *mHestErrorSourceType) {
      IncrementErrorCount ();
      Print (L"ERROR: Error Source Structure must be sorted in Type with ascending order...\n");
      return;
    }

    switch (*mHestErrorSourceType) {
      case EFI_ACPI_6_5_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION:
        ParseAcpi (
          TRUE,
          2,
          "IA-32 Architecture Machine Check Exception",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE),
          PARSER_PARAMS (HestErrorSourceIA32ArchMachineCheckExceptionParser)
          );

        Offset +=
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE);

        Status = DumpIA32ArchMachineCheckErrorBankStructure (
                   Ptr,
                   AcpiTableLength,
                   &Offset
                   );
        if (EFI_ERROR (Status)) {
          return;
        }

        break;
      case EFI_ACPI_6_5_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK:
        ParseAcpi (
          TRUE,
          2,
          "IA-32 Architecture Corrected Machine Check",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE),
          PARSER_PARAMS (HestErrorSourceIA32ArchCorrectedMachineCheckParser)
          );

        Offset +=
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE);

        Status = DumpIA32ArchMachineCheckErrorBankStructure (
                   Ptr,
                   AcpiTableLength,
                   &Offset
                   );
        if (EFI_ERROR (Status)) {
          return;
        }

        break;
      case EFI_ACPI_6_5_IA32_ARCHITECTURE_NMI_ERROR:
        ParseAcpi (
          TRUE,
          2,
          "IA-32 Architecture Non-Maskable Interrupt",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE),
          PARSER_PARAMS (HestErrorSourceIA32ArchNonMaskableInterruptParser)
          );

        Offset +=
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE);
        break;
      case EFI_ACPI_6_5_PCI_EXPRESS_ROOT_PORT_AER:
        ParseAcpi (
          TRUE,
          2,
          "PCI Express RootPort AER Structure",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE),
          PARSER_PARAMS (HestErrorSourcePciExpressRootPortAerParser)
          );

        Offset += sizeof (EFI_ACPI_6_5_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE);
        break;
      case EFI_ACPI_6_5_PCI_EXPRESS_DEVICE_AER:
        ParseAcpi (
          TRUE,
          2,
          "PCI Express Device AER Structure",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_PCI_EXPRESS_DEVICE_AER_STRUCTURE),
          PARSER_PARAMS (HestErrorSourcePciExpressDeviceAerParser)
          );

        Offset += sizeof (EFI_ACPI_6_5_PCI_EXPRESS_DEVICE_AER_STRUCTURE);
        break;
      case EFI_ACPI_6_5_PCI_EXPRESS_BRIDGE_AER:
        ParseAcpi (
          TRUE,
          2,
          "PCI Express Bridge AER Structure",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_PCI_EXPRESS_BRIDGE_AER_STRUCTURE),
          PARSER_PARAMS (HestErrorSourcePciExpressBridgeAerParser)
          );

        Offset += sizeof (EFI_ACPI_6_5_PCI_EXPRESS_BRIDGE_AER_STRUCTURE);
        break;
      case EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR:
        ParseAcpi (
          TRUE,
          2,
          "Generic Hardware Error Source Structure",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE),
          PARSER_PARAMS (HestErrorSourceGhesParser)
          );

        Offset += sizeof (EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE);
        break;
      case EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_VERSION_2:
        ParseAcpi (
          TRUE,
          2,
          "Generic Hardware Error Source V2 Structure",
          ErrorSourcePtr,
          sizeof (
                  EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE
                  ),
          PARSER_PARAMS (HestErrorSourceGhesv2Parser)
          );

        Offset +=
          sizeof (
                  EFI_ACPI_6_5_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE
                  );
        break;
      case EFI_ACPI_6_5_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK:
        ParseAcpi (
          TRUE,
          2,
          "IA-32 Architecture Deferred Machine Check",
          ErrorSourcePtr,
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE),
          PARSER_PARAMS (HestErrorSourceIA32ArchDeferredMachineCheckParser)
          );

        Offset +=
          sizeof (EFI_ACPI_6_5_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE),

        Status = DumpIA32ArchMachineCheckErrorBankStructure (
                   Ptr,
                   AcpiTableLength,
                   &Offset
                   );
        if (EFI_ERROR (Status)) {
          return;
        }

        break;
      default:
        IncrementErrorCount ();
        Print (L"ERROR: Invalid Error Source Descriptor Type(%d).\n", *mHestErrorSourceType);
        return;
    } // switch

    ParsedErrorSourceCount++;
  } // while

  if (ParsedErrorSourceCount < *mHestErrorSourceCount) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid Error Source Count... Real:%d, ErrorSourceCount:%d\n",
      ParsedErrorSourceCount,
      *mHestErrorSourceCount
      );
    return;
  }

  if (Offset < AcpiTableLength) {
    IncrementErrorCount ();
    Print (L"ERROR: Invalid Error Source Count, There's more data...\n");
    return;
  }
}

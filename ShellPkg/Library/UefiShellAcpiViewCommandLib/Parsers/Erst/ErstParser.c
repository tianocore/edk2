/** @file
  ERST table parser

  Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
  Copyright (c) 2016 - 2024, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.5 Specification - August 2022
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;
STATIC UINT32                        *InstructionEntryCount;

/**
  An array of strings describing the Erst actions
**/
STATIC CONST CHAR16  *ErstActionTable[] = {
  L"BEGIN_WRITE_OPERATION",
  L"BEGIN_READ_OPERATION",
  L"BEGIN_CLEAR_OPERATION",
  L"END_OPERATION",
  L"SET_RECORD_OFFSET",
  L"EXECUTE_OPERATION",
  L"CHECK_BUSY_STATUS",
  L"GET_COMMAND_STATUS",
  L"GET_RECORD_IDENTIFIER",
  L"SET_RECORD_IDENTIFIER",
  L"GET_RECORD_COUNT",
  L"BEGIN_DUMMY_WRITE_OPERATION",
  L"RESERVED",
  L"GET_ERROR_LOG_ADDRESS_RANGE",
  L"GET_ERROR_LOG_ADDRESS_RANGE_LENGTH",
  L"GET_ERROR_LOG_ADDRESS_RANGE_ATTRIBUTES",
  L"GET_EXECUTE_OPERATION_TIMINGS"
};

/**
  An array of strings describing the Erst instructions
**/
STATIC CONST CHAR16  *ErstInstructionTable[] = {
  L"READ_REGISTER",
  L"READ_REGISTER_VALUE",
  L"WRITE_REGISTER",
  L"WRITE_REGISTER_VALUE",
  L"NOOP",
  L"LOAD_VAR1",
  L"LOAD_VAR2",
  L"STORE_VAR1",
  L"ADD",
  L"SUBTRACT",
  L"ADD_VALUE",
  L"SUBTRACT_VALUE",
  L"STALL",
  L"STALL_WHILE_TRUE",
  L"SKIP_NEXT_INSTRUCTION_IF_TRUE",
  L"GOTO",
  L"SET_SRC_ADDRESS_BASE",
  L"SET_DST_ADDRESS_BASE",
  L"MOVE_DATA"
};

/**
  Validate Erst action.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateErstAction (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr > EFI_ACPI_6_4_ERST_GET_EXECUTE_OPERATION_TIMINGS) {
    IncrementErrorCount ();
    Print (L"\nError: 0x%02x is not a valid action encoding", *Ptr);
  }
}

/**
  Validate Erst instruction.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateErstInstruction (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if (*Ptr > EFI_ACPI_6_4_ERST_MOVE_DATA) {
    IncrementErrorCount ();
    Print (L"\nError: 0x%02x is not a valid instruction encoding", *Ptr);
  }
}

/**
  Validate Erst flags.

  @param [in] Ptr       Pointer to the start of the field data.
  @param [in] Length    Length of the field.
  @param [in] Context   Pointer to context specific information e.g. this
                        could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateErstFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  if ((*Ptr & 0xfe) != 0) {
    IncrementErrorCount ();
    Print (L"\nError: Reserved Flag bits not set to 0");
  }
}

/**
  Looks up and prints the string corresponding to the index.

  @param [in] Table      Lookup table.
  @param [in] Index      Entry to print.
  @param [in] NumEntries Number of valid entries in the table.
**/
STATIC
VOID
EFIAPI
FormatByte (
  IN CONST CHAR16  *Table[],
  IN UINT8         Index,
  IN UINT8         NumEntries
  )
{
  CONST CHAR16  *String;

  if (Index < NumEntries) {
    String = Table[Index];
  } else {
    String = L"**INVALID**";
  }

  Print (
    L"0x%02x (%s)",
    Index,
    String
    );
}

/**
  Prints the Erst Action.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the Action byte.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpErstAction (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  FormatByte (ErstActionTable, *Ptr, ARRAY_SIZE (ErstActionTable));
}

/**
  Prints the Erst Instruction.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the Instruction byte.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpErstInstruction (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  FormatByte (ErstInstructionTable, *Ptr, ARRAY_SIZE (ErstInstructionTable));
}

/**
  An ACPI_PARSER array describing the ACPI ERST Table.
**/
STATIC CONST ACPI_PARSER  ErstParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Serialization Header Size",  4,  36, L"0x%x", NULL, NULL,                            NULL, NULL },
  { L"Reserved",                   4,  40, L"0x%x", NULL, NULL,                            NULL, NULL },
  { L"Instruction Entry Count",    4,  44, L"0x%x", NULL, (VOID **)&InstructionEntryCount, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Serialization Instruction Entry structure.
**/
STATIC CONST ACPI_PARSER  SerializationInstructionEntryParser[] = {
  { L"Serialization Action", 1,  0,  L"0x%x",   DumpErstAction,      NULL, ValidateErstAction,      NULL },
  { L"Instruction",          1,  1,  L"0x%x",   DumpErstInstruction, NULL, ValidateErstInstruction, NULL },
  { L"Flags",                1,  2,  L"0x%x",   NULL,                NULL, ValidateErstFlags,       NULL },
  { L"Reserved",             1,  3,  L"0x%x",   NULL,                NULL, NULL,                    NULL },
  { L"Register Region",      12, 4,  NULL,      DumpGas,             NULL, NULL,                    NULL },
  { L"Value",                8,  16, L"0x%llx", NULL,                NULL, NULL,                    NULL },
  { L"Mask",                 8,  24, L"0x%llx", NULL,                NULL, NULL,                    NULL }
};

/**
  This function parses the ACPI ERST table.
  When trace is enabled this function parses the ERST table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiErst (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             Trace,
             0,
             "ERST",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (ErstParser)
             );

  if (sizeof (EFI_ACPI_6_4_ERST_SERIALIZATION_INSTRUCTION_ENTRY)*(*InstructionEntryCount) != (AcpiTableLength - Offset)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Invalid InstructionEntryCount. " \
      L"Count = %d. Offset = %d. AcpiTableLength = %d.\n",
      *InstructionEntryCount,
      Offset,
      AcpiTableLength
      );
    return;
  }

  while (Offset < AcpiTableLength) {
    Offset += ParseAcpi (
                Trace,
                2,
                "Serialization Action",
                Ptr + Offset,
                (AcpiTableLength - Offset),
                PARSER_PARAMS (SerializationInstructionEntryParser)
                );
  }
}

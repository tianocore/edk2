/** @file
  EINJ table parser

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
STATIC UINT32                        *mEinjInjectionHdrSize;
STATIC UINT32                        *mEinjInjectionEntryCnt;

STATIC CONST CHAR16  *InstNameTable[] = {
  L"READ_REGISTER",
  L"READ_REGISTER_VALUE",
  L"WRITE_REGISTER",
  L"WRITE_REGISTER_VALUE",
  L"NOOP",
};

/**
  This function validates the flags field in the EINJ injection header.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInjectionFlags (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  Flags;

  Flags = *(UINT8 *)Ptr;

  if (Flags != 0) {
    IncrementErrorCount ();
    Print (L"\nERROR: Injection Flags must be zero...");
  }
}

/**
  An ACPI_PARSER array describing the ACPI EINJ Table.
**/
STATIC CONST ACPI_PARSER  EinjParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo),
  { L"Injection Header Size",       4,  36, L"%d",   NULL, (VOID **)&mEinjInjectionHdrSize,
    NULL,                           NULL },
  { L"Injection Flags",             1,  40, L"0x%x", NULL, NULL,                            ValidateInjectionFlags,NULL },
  { L"Reserved",                    3,  41, NULL,    NULL, NULL,                            NULL,  NULL },
  { L"Injection Entry Count",       4,  44, L"%d",   NULL, (VOID **)&mEinjInjectionEntryCnt,
    NULL,                           NULL },
  /// Injection Action Table.
  /// ...
};

/**
  This function validates the injection action field in
  the EINJ injection instruction entry.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInjectionAction (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  InjectionAction;
  UINT8  MaxInjectionAction;

  InjectionAction = *(UINT8 *)Ptr;

  /**
   * EFI_ACPI_6_5_EINJ_TRIGGER_ERROR is only used Trigger Action Table
   * not used in Injection Action Table in EINJ.
   * Cf ACPI 6.5 Table 18.24 - Error Injection Table
   * Cf ACPI 6.5 Table 18.36 - Trigger Error Action
   */
  if (*mAcpiHdrInfo.Revision < EFI_ACPI_6_5_ERROR_INJECTION_TABLE_REVISION) {
    MaxInjectionAction = EFI_ACPI_6_5_EINJ_GET_EXECUTE_OPERATION_TIMINGS;
  } else {
    MaxInjectionAction = EFI_ACPI_6_5_EINJ_EINJV2_GET_ERROR_TYPE;
  }

  if ((InjectionAction < EFI_ACPI_6_5_EINJ_BEGIN_INJECTION_OPERATION) ||
      (InjectionAction > MaxInjectionAction))
  {
    IncrementErrorCount ();
    Print (L"\nERROR: Invalid Injection Action(0x%x)...", InjectionAction);
  }
}

/**
  This function validates the instruction field in
  the EINJ injection instruction entry.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateInstruction (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  UINT8  Inst;

  Inst = *Ptr;

  if (*mAcpiHdrInfo.Revision <= EFI_ACPI_6_5_ERROR_INJECTION_TABLE_REVISION) {
    if (Inst > EFI_ACPI_6_5_EINJ_NOOP) {
      IncrementErrorCount ();
      Print (L"\nERROR: Invalid Instruction(0x%x)...", Inst);
    }
  }
}

/**
  This function validates the register region field in
  the EINJ injection instruction entry.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Length  Length of the field.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateRegisterRegion (
  IN UINT8   *Ptr,
  IN UINT32  Length,
  IN VOID    *Context
  )
{
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  *RegisterRegion;

  RegisterRegion = (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE *)Ptr;

  if ((RegisterRegion->AddressSpaceId != EFI_ACPI_6_5_SYSTEM_MEMORY) &&
      (RegisterRegion->AddressSpaceId != EFI_ACPI_6_5_SYSTEM_IO))
  {
    IncrementErrorCount ();
    Print (L"\nERROR: Register Region Must be SYSTEM_MEMORY or SYSTEM_IO...");
  }
}

/**
  Dumps the injection action fields in injection instruction entry.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpInjectionInstAction (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  UINT8         InjectionAction;
  CONST CHAR16  *ActionName;

  InjectionAction = *Ptr;

  switch (InjectionAction) {
    case EFI_ACPI_6_5_EINJ_BEGIN_INJECTION_OPERATION:
      ActionName = L"BEGIN_INJECTION_OPERATION";
      break;
    case EFI_ACPI_6_5_EINJ_GET_TRIGGER_ERROR_ACTION_TABLE:
      ActionName = L"GET_TRIGGER_ERROR_ACTION_TABLE";
      break;
    case EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE:
      ActionName = L"SET_ERROR_TYPE";
      break;
    case EFI_ACPI_6_5_EINJ_GET_ERROR_TYPE:
      ActionName = L"GET_ERROR_TYPE";
      break;
    case EFI_ACPI_6_5_EINJ_END_OPERATION:
      ActionName = L"END_OPERATION";
      break;
    case EFI_ACPI_6_5_EINJ_EXECUTE_OPERATION:
      ActionName = L"EXECUTE_OPERATION";
      break;
    case EFI_ACPI_6_5_EINJ_CHECK_BUSY_STATUS:
      ActionName = L"CHECK_BUSY_STATUS";
      break;
    case EFI_ACPI_6_5_EINJ_GET_COMMAND_STATUS:
      ActionName = L"GET_COMMAND_STATUS";
      break;
    case EFI_ACPI_6_5_EINJ_SET_ERROR_TYPE_WITH_ADDRESS:
      ActionName = L"SET_ERROR_TYPE_WITH_ADDRESS";
      break;
    case EFI_ACPI_6_5_EINJ_GET_EXECUTE_OPERATION_TIMINGS:
      ActionName = L"GET_EXECUTE_OPERATION_TIMINGS";
      break;
    case EFI_ACPI_6_5_EINJ_EINJV2_SET_ERROR_TYPE:
      ActionName = L"EINJV2_SET_ERROR_TYPE";
      break;
    case EFI_ACPI_6_5_EINJ_EINJV2_GET_ERROR_TYPE:
      ActionName = L"EINJV2_GET_ERROR_TYPE";
      break;
    case EFI_ACPI_6_5_EINJ_TRIGGER_ERROR:
      ActionName = L"TRIGGER_ERROR";
      break;
    default:
      IncrementErrorCount ();
      ActionName = L"UNKNOWN";
  }

  Print (L"%s(0x%x)", ActionName, InjectionAction);
}

/**
  Dumps the instruction fields in injection instruction entry.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field.
**/
STATIC
VOID
EFIAPI
DumpInstruction (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  UINT8         Inst;
  CONST CHAR16  *InstName;

  Inst = *Ptr;

  if (Inst < ARRAY_SIZE (InstNameTable)) {
    InstName = InstNameTable[Inst];
  } else {
    IncrementErrorCount ();
    InstName = L"UNKNOWN";
  }

  Print (L"%s(0x%x)", InstName, Inst);
}

/**
  An ACPI_PARSER array describing the EINJ Injection instruction entry.
**/
STATIC CONST ACPI_PARSER  EinjInjectionInstEntryParser[] = {
  { L"Injection Action", 1,  0,  NULL,    DumpInjectionInstAction, NULL,
    ValidateInjectionAction, NULL },
  { L"Instruction",      1,  1,  NULL,    DumpInstruction,         NULL,
    ValidateInstruction, NULL },
  { L"Flags",            1,  2,  L"0x%x", NULL,                    NULL,NULL,  NULL },
  { L"Reserved",         1,  3,  NULL,    NULL,                    NULL,NULL,  NULL },
  { L"Register Region",  12, 4,  NULL,    DumpGas,                 NULL,
    ValidateRegisterRegion, NULL },
  { L"Value",            8,  16, L"0x%x", NULL,                    NULL,NULL,  NULL },
  { L"Mask",             8,  24, L"0x%x", NULL,                    NULL,NULL,  NULL },
};

/**
  This function parses the EINJ table.
  When trace is enabled this function parses the EINJ table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiEinj (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32  Offset;
  UINT8   *InjInstEntryPtr;
  UINT32  InjInstEntrySize;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "EINJ",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (EinjParser)
             );

  // Validate Error Source Descriptors Count.
  if ((mEinjInjectionHdrSize == NULL) || (*mEinjInjectionHdrSize != Offset)) {
    IncrementErrorCount ();
    Print (L"ERROR: Invalid Injection Header...\n");
    return;
  }

  if ((mEinjInjectionEntryCnt == NULL) || (*mEinjInjectionEntryCnt == 0)) {
    IncrementErrorCount ();
    Print (L"ERROR: Injection Instruction Entry should be presented...\n");
    return;
  }

  InjInstEntrySize = sizeof (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY);

  if ((*mEinjInjectionEntryCnt * InjInstEntrySize) != (AcpiTableLength - Offset)) {
    IncrementErrorCount ();
    Print (
      L"ERROR: Incorrect count for Injection Instruction Entry.\n" \
      L"       Injection Entry Count= %d.\n" \
      L"       Present Count= %d.\n",
      *mEinjInjectionEntryCnt,
      (AcpiTableLength - Offset) / InjInstEntrySize
      );
  }

  while (Offset < AcpiTableLength) {
    InjInstEntryPtr = Ptr + Offset;

    // Get Injection Instruction Entry.
    ParseAcpi (
      TRUE,
      2,
      "Injection Instruction Entry",
      InjInstEntryPtr,
      AcpiTableLength - Offset,
      PARSER_PARAMS (EinjInjectionInstEntryParser)
      );

    Offset += InjInstEntrySize;
  } // while
}

/** @file
  GTDT table parser

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.2 Specification - Errata A, September 2017
  **/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

// Local variables
STATIC CONST UINT32* GtdtPlatformTimerCount;
STATIC CONST UINT32* GtdtPlatformTimerOffset;
STATIC CONST UINT8*  PlatformTimerType;
STATIC CONST UINT16* PlatformTimerLength;
STATIC CONST UINT32* GtBlockTimerCount;
STATIC CONST UINT32* GtBlockTimerOffset;
STATIC CONST UINT16* GtBlockLength;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  This function validates the GT Block timer count.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGtBlockTimerCount (
  IN UINT8* Ptr,
  IN VOID*  Context
  );

/**
  This function validates the GT Frame Number.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGtFrameNumber (
  IN UINT8* Ptr,
  IN VOID*  Context
  );

/**
  An ACPI_PARSER array describing the ACPI GTDT Table.
**/
STATIC CONST ACPI_PARSER GtdtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"CntControlBase Physical Address", 8, 36, L"0x%lx", NULL, NULL,
   NULL, NULL},
  {L"Reserved", 4, 44, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Secure EL1 timer GSIV", 4, 48, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Secure EL1 timer FLAGS", 4, 52, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Non-Secure EL1 timer GSIV", 4, 56, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Non-Secure EL1 timer FLAGS", 4, 60, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Virtual timer GSIV", 4, 64, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Virtual timer FLAGS", 4, 68, L"0x%x", NULL, NULL, NULL, NULL},

  {L"Non-Secure EL2 timer GSIV", 4, 72, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Non-Secure EL2 timer FLAGS", 4, 76, L"0x%x", NULL, NULL, NULL, NULL},

  {L"CntReadBase Physical address", 8, 80, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Platform Timer Count", 4, 88, L"%d", NULL,
   (VOID**)&GtdtPlatformTimerCount, NULL, NULL},
  {L"Platform Timer Offset", 4, 92, L"0x%x", NULL,
   (VOID**)&GtdtPlatformTimerOffset, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Platform timer header.
**/
STATIC CONST ACPI_PARSER GtPlatformTimerHeaderParser[] = {
  {L"Type", 1, 0, NULL, NULL, (VOID**)&PlatformTimerType, NULL, NULL},
  {L"Length", 2, 1, NULL, NULL, (VOID**)&PlatformTimerLength, NULL, NULL},
  {L"Reserved", 1, 3, NULL, NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Platform GT Block.
**/
STATIC CONST ACPI_PARSER GtBlockParser[] = {
  {L"Type", 1, 0, L"%d", NULL, NULL, NULL, NULL},
  {L"Length", 2, 1, L"%d", NULL, (VOID**)&GtBlockLength, NULL, NULL},
  {L"Reserved", 1, 3, L"%x", NULL, NULL, NULL, NULL},
  {L"Physical address (CntCtlBase)", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Timer Count", 4, 12, L"%d", NULL, (VOID**)&GtBlockTimerCount,
   ValidateGtBlockTimerCount, NULL},
  {L"Timer Offset", 4, 16, L"%d", NULL, (VOID**)&GtBlockTimerOffset, NULL,
    NULL}
};

/**
  An ACPI_PARSER array describing the GT Block timer.
**/
STATIC CONST ACPI_PARSER GtBlockTimerParser[] = {
  {L"Frame Number", 1, 0, L"%d", NULL, NULL, ValidateGtFrameNumber, NULL},
  {L"Reserved", 3, 1, L"%x %x %x", Dump3Chars, NULL, NULL, NULL},
  {L"Physical address (CntBaseX)", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Physical address (CntEL0BaseX)", 8, 12, L"0x%lx", NULL, NULL, NULL,
    NULL},
  {L"Physical Timer GSIV", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Physical Timer Flags", 4, 24, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Virtual Timer GSIV", 4, 28, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Virtual Timer Flags", 4, 32, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Common Flags", 4, 36, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  An ACPI_PARSER array describing the Platform Watchdog.
**/
STATIC CONST ACPI_PARSER SBSAGenericWatchdogParser[] = {
  {L"Type", 1, 0, L"%d", NULL, NULL, NULL, NULL},
  {L"Length", 2, 1, L"%d", NULL, NULL, NULL, NULL},
  {L"Reserved", 1, 3, L"%x", NULL, NULL, NULL, NULL},
  {L"RefreshFrame Physical address", 8, 4, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"ControlFrame Physical address", 8, 12, L"0x%lx", NULL, NULL, NULL, NULL},
  {L"Watchdog Timer GSIV", 4, 20, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Watchdog Timer Flags", 4, 24, L"0x%x", NULL, NULL, NULL, NULL}
};

/**
  This function validates the GT Block timer count.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGtBlockTimerCount (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT32 BlockTimerCount;

  BlockTimerCount = *(UINT32*)Ptr;

  if (BlockTimerCount > 8) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Timer Count = %d. Max Timer Count is 8.",
      BlockTimerCount
      );
  }
}

/**
  This function validates the GT Frame Number.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateGtFrameNumber (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT8 FrameNumber;

  FrameNumber = *(UINT8*)Ptr;

  if (FrameNumber > 7) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: GT Frame Number = %d. GT Frame Number must be in range 0-7.",
      FrameNumber
      );
  }
}

/**
  This function parses the Platform GT Block.

  @param [in] Ptr     Pointer to the start of the GT Block data.
  @param [in] Length  Length of the GT Block structure.
**/
STATIC
VOID
DumpGTBlock (
  IN UINT8* Ptr,
  IN UINT32 Length
  )
{
  UINT32 Index;
  UINT32 Offset;
  UINT32 GTBlockTimerLength;

  Offset = ParseAcpi (
             TRUE,
             2,
             "GT Block",
             Ptr,
             Length,
             PARSER_PARAMS (GtBlockParser)
             );
  GTBlockTimerLength = (*GtBlockLength - Offset) / (*GtBlockTimerCount);
  Length -= Offset;

  if (*GtBlockTimerCount != 0) {
    Ptr += (*GtBlockTimerOffset);
    Index = 0;
    while ((Index < (*GtBlockTimerCount)) && (Length >= GTBlockTimerLength)) {
      Offset = ParseAcpi (
                 TRUE,
                 2,
                 "GT Block Timer",
                 Ptr,
                 GTBlockTimerLength,
                 PARSER_PARAMS (GtBlockTimerParser)
                 );
      // Increment by GT Block Timer structure size
      Ptr += Offset;
      Length -= Offset;
      Index++;
    }

    if (Length != 0) {
      IncrementErrorCount ();
      Print (
        L"ERROR:GT Block Timer length mismatch. Unparsed %d bytes.\n",
        Length
        );
    }
  }
}

/**
  This function parses the Platform Watchdog timer.

  @param [in] Ptr     Pointer to the start of the watchdog timer data.
  @param [in] Length  Length of the watchdog timer structure.
**/
STATIC
VOID
DumpWatchdogTimer (
  IN UINT8* Ptr,
  IN UINT16 Length
  )
{
  ParseAcpi (
    TRUE,
    2,
    "SBSA Generic Watchdog",
    Ptr,
    Length,
    PARSER_PARAMS (SBSAGenericWatchdogParser)
    );
}

/**
  This function parses the ACPI GTDT table.
  When trace is enabled this function parses the GTDT table and
  traces the ACPI table fields.

  This function also parses the following platform timer structures:
    - GT Block timer
    - Watchdog timer

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiGtdt (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Index;
  UINT8* TimerPtr;

  if (!Trace) {
    return;
  }

  ParseAcpi (
    TRUE,
    0,
    "GTDT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (GtdtParser)
    );

  if (*GtdtPlatformTimerCount != 0) {
    TimerPtr = Ptr + (*GtdtPlatformTimerOffset);
    Index = 0;
    do {
      // Parse the Platform Timer Header
      ParseAcpi (
        FALSE,
        0,
        NULL,
        TimerPtr,
        4,  // GT Platform Timer structure header length.
        PARSER_PARAMS (GtPlatformTimerHeaderParser)
        );
      switch (*PlatformTimerType) {
        case EFI_ACPI_6_2_GTDT_GT_BLOCK:
          DumpGTBlock (TimerPtr, *PlatformTimerLength);
          break;
        case EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG:
          DumpWatchdogTimer (TimerPtr, *PlatformTimerLength);
          break;
        default:
          IncrementErrorCount ();
          Print (
            L"ERROR: INVALID Platform Timer Type = %d\n",
            *PlatformTimerType
            );
          break;
      } // switch
      TimerPtr += (*PlatformTimerLength);
      Index++;
    } while (Index < *GtdtPlatformTimerCount);
  }
}

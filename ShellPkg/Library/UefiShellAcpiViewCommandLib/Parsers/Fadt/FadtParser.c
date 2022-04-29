/** @file
  FADT table parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  Copyright (c) 2022, AMD Incorporated. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.3 Specification - January 2019
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"

// Local variables
STATIC CONST UINT32                  *DsdtAddress;
STATIC CONST UINT64                  *X_DsdtAddress;
STATIC CONST UINT32                  *Flags;
STATIC CONST UINT32                  *FirmwareCtrl;
STATIC CONST UINT64                  *X_FirmwareCtrl;
STATIC CONST UINT8                   *FadtMinorRevision;
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;

/**
  A macro defining the Hardware reduced ACPI flag
**/
#define HW_REDUCED_ACPI  BIT20

/**
  Offset to the FACS signature from the start of the FACS.
**/
#define FACS_SIGNATURE_OFFSET  0

/**
  Offset to the FACS revision from the start of the FACS.
**/
#define FACS_VERSION_OFFSET  32

/**
  Offset to the FACS length from the start of the FACS.
**/
#define FACS_LENGTH_OFFSET  4

/**
  Get the ACPI XSDT header info.
**/
CONST ACPI_DESCRIPTION_HEADER_INFO *
EFIAPI
GetAcpiXsdtHeaderInfo (
  VOID
  );

/**
  This function validates the Firmware Control Field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateFirmwareCtrl (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (*(UINT32 *)Ptr != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: Firmware Control must be zero for ARM platforms."
      );
  }

 #endif
}

/**
  This function validates the X_Firmware Control Field.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateXFirmwareCtrl (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (*(UINT64 *)Ptr != 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: X Firmware Control must be zero for ARM platforms."
      );
  }

 #endif
}

/**
  This function validates the flags.

  @param [in] Ptr     Pointer to the start of the field data.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateFlags (
  IN UINT8  *Ptr,
  IN VOID   *Context
  )
{
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (((*(UINT32 *)Ptr) & HW_REDUCED_ACPI) == 0) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: HW_REDUCED_ACPI flag must be set for ARM platforms."
      );
  }

 #endif
}

STATIC CONST ACPI_PARSER  FadtFlagParser[] = {
  { L"WBINVD",                               1,  0,  L"%d", NULL, NULL, NULL, NULL },
  { L"WBINVD_FLUSH",                         1,  1,  L"%d", NULL, NULL, NULL, NULL },
  { L"PROC_C1",                              1,  2,  L"%d", NULL, NULL, NULL, NULL },
  { L"P_LVL2_UP",                            1,  3,  L"%d", NULL, NULL, NULL, NULL },
  { L"PWR_BUTTON",                           1,  4,  L"%d", NULL, NULL, NULL, NULL },
  { L"SLP_BUTTON",                           1,  5,  L"%d", NULL, NULL, NULL, NULL },
  { L"FIX_RTC",                              1,  6,  L"%d", NULL, NULL, NULL, NULL },
  { L"RTC_S4",                               1,  7,  L"%d", NULL, NULL, NULL, NULL },
  { L"TMR_VAL_EXT",                          1,  8,  L"%d", NULL, NULL, NULL, NULL },
  { L"DCK_CAP",                              1,  9,  L"%d", NULL, NULL, NULL, NULL },
  { L"RESET_REG_SUP",                        1,  10, L"%d", NULL, NULL, NULL, NULL },
  { L"SEALED_CASE",                          1,  11, L"%d", NULL, NULL, NULL, NULL },
  { L"HEADLESS",                             1,  12, L"%d", NULL, NULL, NULL, NULL },
  { L"CPU_SW_SLP",                           1,  13, L"%d", NULL, NULL, NULL, NULL },
  { L"PCI_EXP_WAK",                          1,  14, L"%d", NULL, NULL, NULL, NULL },
  { L"USE_PLATFORM_CLOCK",                   1,  15, L"%d", NULL, NULL, NULL, NULL },
  { L"S4_RTC_STS_VALID",                     1,  16, L"%d", NULL, NULL, NULL, NULL },
  { L"REMOTE_POWER_ON_CAPABLE",              1,  17, L"%d", NULL, NULL, NULL, NULL },
  { L"FORCE_APIC_CLUSTER_MODEL",             1,  18, L"%d", NULL, NULL, NULL, NULL },
  { L"FORCE_APIC_PHYSICAL_DESTINATION_MODE", 1,  19, L"%d", NULL, NULL, NULL, NULL },
  { L"HW_REDUCED_ACPI",                      1,  20, L"%d", NULL, NULL, NULL, NULL },
  { L"LOW_POWER_S0_IDLE_CAPABLE",            1,  21, L"%d", NULL, NULL, NULL, NULL },
  { L"Reserved",                             10, 22, L"%d", NULL, NULL, NULL, NULL }
};

/**
  This function traces FADT Flags fields.
  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
VOID
EFIAPI
DumpFadtFlags (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr
  )
{
  if (Format != NULL) {
    Print (Format, *(UINT32 *)Ptr);
    return;
  }

  Print (L"0x%X\n", *(UINT32 *)Ptr);
  ParseAcpiBitFields (
    TRUE,
    2,
    NULL,
    Ptr,
    4,
    PARSER_PARAMS (FadtFlagParser)
    );
}

/**
  An ACPI_PARSER array describing the ACPI FADT Table.
**/
STATIC CONST ACPI_PARSER  FadtParser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"FIRMWARE_CTRL",              4,   36,  L"0x%x",  NULL,          (VOID **)&FirmwareCtrl,
    ValidateFirmwareCtrl,          NULL },
  { L"DSDT",                       4,   40,  L"0x%x",  NULL,          (VOID **)&DsdtAddress,      NULL,           NULL },
  { L"Reserved",                   1,   44,  L"%x",    NULL,          NULL,                       NULL,           NULL },
  { L"Preferred_PM_Profile",       1,   45,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"SCI_INT",                    2,   46,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"SMI_CMD",                    4,   48,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"ACPI_ENABLE",                1,   52,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"ACPI_DISABLE",               1,   53,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"S4BIOS_REQ",                 1,   54,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PSTATE_CNT",                 1,   55,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM1a_EVT_BLK",               4,   56,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM1b_EVT_BLK",               4,   60,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM1a_CNT_BLK",               4,   64,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM1b_CNT_BLK",               4,   68,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM2_CNT_BLK",                4,   72,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM_TMR_BLK",                 4,   76,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"GPE0_BLK",                   4,   80,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"GPE1_BLK",                   4,   84,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM1_EVT_LEN",                1,   88,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM1_CNT_LEN",                1,   89,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM2_CNT_LEN",                1,   90,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"PM_TMR_LEN",                 1,   91,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"GPE0_BLK_LEN",               1,   92,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"GPE1_BLK_LEN",               1,   93,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"GPE1_BASE",                  1,   94,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"CST_CNT",                    1,   95,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"P_LVL2_LAT",                 2,   96,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"P_LVL3_LAT",                 2,   98,  L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"FLUSH_SIZE",                 2,   100, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"FLUSH_STRIDE",               2,   102, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"DUTY_OFFSET",                1,   104, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"DUTY_WIDTH",                 1,   105, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"DAY_ALRM",                   1,   106, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"MON_ALRM",                   1,   107, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"CENTURY",                    1,   108, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"IAPC_BOOT_ARCH",             2,   109, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"Reserved",                   1,   111, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"Flags",                      4,   112, NULL,     DumpFadtFlags, (VOID **)&Flags,            ValidateFlags,  NULL },
  { L"RESET_REG",                  12,  116, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"RESET_VALUE",                1,   128, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"ARM_BOOT_ARCH",              2,   129, L"0x%x",  NULL,          NULL,                       NULL,           NULL },
  { L"FADT Minor Version",         1,   131, L"0x%x",  NULL,          (VOID **)&FadtMinorRevision,
    NULL,                          NULL },
  { L"X_FIRMWARE_CTRL",            8,   132, L"0x%lx", NULL,          (VOID **)&X_FirmwareCtrl,
    ValidateXFirmwareCtrl,         NULL },
  { L"X_DSDT",                     8,   140, L"0x%lx", NULL,          (VOID **)&X_DsdtAddress,    NULL,           NULL },
  { L"X_PM1a_EVT_BLK",             12,  148, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"X_PM1b_EVT_BLK",             12,  160, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"X_PM1a_CNT_BLK",             12,  172, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"X_PM1b_CNT_BLK",             12,  184, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"X_PM2_CNT_BLK",              12,  196, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"X_PM_TMR_BLK",               12,  208, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"X_GPE0_BLK",                 12,  220, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"X_GPE1_BLK",                 12,  232, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"SLEEP_CONTROL_REG",          12,  244, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"SLEEP_STATUS_REG",           12,  256, NULL,     DumpGas,       NULL,                       NULL,           NULL },
  { L"Hypervisor VendorIdentity",  8,   268, L"%lx",   NULL,          NULL,                       NULL,           NULL }
};

/**
  This function parses the ACPI FADT table.
  This function parses the FADT table and optionally traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiFadt (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  EFI_STATUS             Status;
  UINT8                  *DsdtPtr;
  UINT8                  *FirmwareCtrlPtr;
  UINT32                 FacsSignature;
  UINT32                 FacsLength;
  UINT8                  FacsRevision;
  PARSE_ACPI_TABLE_PROC  FacsParserProc;

  ParseAcpi (
    Trace,
    0,
    "FADT",
    Ptr,
    AcpiTableLength,
    PARSER_PARAMS (FadtParser)
    );

  if (Trace) {
    if (FadtMinorRevision != NULL) {
      Print (L"\nSummary:\n");
      PrintFieldName (2, L"FADT Version");
      Print (L"%d.%d\n", *AcpiHdrInfo.Revision, *FadtMinorRevision);
    }

    if (*GetAcpiXsdtHeaderInfo ()->OemTableId != *AcpiHdrInfo.OemTableId) {
      IncrementErrorCount ();
      Print (L"ERROR: OEM Table Id does not match with RSDT/XSDT.\n");
    }
  }

  // If X_FIRMWARE_CTRL is not zero then use X_FIRMWARE_CTRL and ignore
  // FIRMWARE_CTRL, else use FIRMWARE_CTRL.
  if ((X_FirmwareCtrl != NULL) && (*X_FirmwareCtrl != 0)) {
    FirmwareCtrlPtr = (UINT8 *)(UINTN)(*X_FirmwareCtrl);
  } else if ((FirmwareCtrl != NULL) && (*FirmwareCtrl != 0)) {
    FirmwareCtrlPtr = (UINT8 *)(UINTN)(*FirmwareCtrl);
  } else {
    FirmwareCtrlPtr = NULL;
    // if HW_REDUCED_ACPI flag is not set, both FIRMWARE_CTRL and
    // X_FIRMWARE_CTRL cannot be zero, and the FACS Table must be
    // present.
    if ((Trace) &&
        (Flags != NULL) &&
        ((*Flags & EFI_ACPI_6_3_HW_REDUCED_ACPI) != EFI_ACPI_6_3_HW_REDUCED_ACPI))
    {
      IncrementErrorCount ();
      Print (
        L"ERROR: No FACS table found, "
        L"both X_FIRMWARE_CTRL and FIRMWARE_CTRL are zero.\n"
        );
    }
  }

  if (FirmwareCtrlPtr != NULL) {
    // The FACS table does not have a standard ACPI table header. Therefore,
    // the signature, length and version needs to be initially parsed.
    // The FACS signature is 4 bytes starting at offset 0.
    FacsSignature = *(UINT32 *)(FirmwareCtrlPtr + FACS_SIGNATURE_OFFSET);

    // The FACS length is 4 bytes starting at offset 4.
    FacsLength = *(UINT32 *)(FirmwareCtrlPtr + FACS_LENGTH_OFFSET);

    // The FACS version is 1 byte starting at offset 32.
    FacsRevision = *(UINT8 *)(FirmwareCtrlPtr + FACS_VERSION_OFFSET);

    Trace = ProcessTableReportOptions (
              FacsSignature,
              FirmwareCtrlPtr,
              FacsLength
              );

    Status = GetParser (FacsSignature, &FacsParserProc);
    if (EFI_ERROR (Status)) {
      Print (
        L"ERROR: No registered parser found for FACS.\n"
        );
      return;
    }

    FacsParserProc (
      Trace,
      FirmwareCtrlPtr,
      FacsLength,
      FacsRevision
      );
  }

  // If X_DSDT is valid then use X_DSDT and ignore DSDT, else use DSDT.
  if ((X_DsdtAddress != NULL) && (*X_DsdtAddress != 0)) {
    DsdtPtr = (UINT8 *)(UINTN)(*X_DsdtAddress);
  } else if ((DsdtAddress != NULL) && (*DsdtAddress != 0)) {
    DsdtPtr = (UINT8 *)(UINTN)(*DsdtAddress);
  } else {
    // Both DSDT and X_DSDT cannot be invalid.
 #if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
    if (Trace) {
      // The DSDT Table is mandatory for ARM systems
      // as the CPU information MUST be presented in
      // the DSDT.
      IncrementErrorCount ();
      Print (L"ERROR: Both X_DSDT and DSDT are invalid.\n");
    }

 #endif
    return;
  }

  ProcessAcpiTable (DsdtPtr);
}

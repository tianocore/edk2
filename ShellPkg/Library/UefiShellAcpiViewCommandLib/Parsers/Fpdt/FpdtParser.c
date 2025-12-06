/** @file
  Firmware Performance Data Table (FPDT) table parser

  Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.5 Specification - August 2022
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewConfig.h"

// Local Variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  mAcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI FPDT performance record header.
**/
STATIC CONST ACPI_PARSER  FpdtPerfRecordParser[] = {
  { L"Type",     2, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",   1, 2, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Revision", 1, 3, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT boot performance pointer record.
**/
STATIC CONST ACPI_PARSER  FpdtBootPerfPointerParser[] = {
  { L"Type",                        2, 0, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                      1, 2, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",                    1, 3, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",                    4, 4, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"BootPerformanceTablePointer", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT S3 performance pointer record.
**/
STATIC CONST ACPI_PARSER  FpdtS3PerfPointerParser[] = {
  { L"Type",                      2, 0, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                    1, 2, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",                  1, 3, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",                  4, 4, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"S3PerformanceTablePointer", 8, 8, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT firmware table header.
**/
STATIC CONST ACPI_PARSER  AcpiFpdtHdrParser[] = {
  { L"Signature", 4, 0, NULL,    Dump4Chars, NULL, NULL, NULL },
  { L"Length",    4, 4, L"0x%x", NULL,       NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT firmware basic boot record.
**/
STATIC CONST ACPI_PARSER  FpdtBasicBootRecordParser[] = {
  { L"Type",                    2, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                  1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",                1, 3,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",                4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"ResetEnd",                8, 8,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"OsLoaderLoadImageStart",  8, 16, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"OsLoaderStartImageStart", 8, 24, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"ExitBootServicesEntry",   8, 32, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"ExitBootServicesExit",    8, 40, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT S3 resume record.
**/
STATIC CONST ACPI_PARSER  FpdtS3ResumeRecordParser[] = {
  { L"Type",          2, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",        1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",      1, 3,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"ResumeCount",   4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"FullResume",    8, 8,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"AverageResume", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT S3 suspend record.
**/
STATIC CONST ACPI_PARSER  FpdtS3SuspendRecordParser[] = {
  { L"Type",         2, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",       1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",     1, 3,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"SuspendStart", 4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"SuspendEnd",   8, 12, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT Table.
**/
STATIC CONST ACPI_PARSER  FpdtParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo)
};

/**
  This function parses the ACPI FPDT table.
  This function parses the FPDT table and optionally traces the ACPI
  table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiFpdt (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  EFI_ACPI_6_5_FPDT_BOOT_PERFORMANCE_TABLE_POINTER_RECORD  *FpdtBasicBootPtr;
  EFI_ACPI_6_5_FPDT_FIRMWARE_BASIC_BOOT_RECORD             *FpdtBasicBootRec;
  EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER              *FpdtRecHdr;
  EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER              *RecordHeader;
  EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER               *FpdtHdr;
  EFI_ACPI_6_5_FPDT_S3_PERFORMANCE_TABLE_POINTER_RECORD    *FpdtS3Ptr;
  EFI_ACPI_6_5_FPDT_S3_RESUME_RECORD                       *FpdtS3ResumeRec;
  EFI_ACPI_6_5_FPDT_S3_SUSPEND_RECORD                      *FpdtS3SuspendRec;
  UINT32                                                   Offset;
  UINT32                                                   RecOffset;
  UINT8                                                    *RecordPtr;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "FPDT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (FpdtParser)
             );

  RecordPtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    RecordHeader = (EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER *)RecordPtr;

    if (RecordHeader->Type == EFI_ACPI_6_5_FPDT_RECORD_TYPE_FIRMWARE_BASIC_BOOT_POINTER) {
      FpdtBasicBootPtr = (EFI_ACPI_6_5_FPDT_BOOT_PERFORMANCE_TABLE_POINTER_RECORD *)RecordPtr;
      ParseAcpi (
        TRUE,
        2,
        "FPDT Boot Performance Pointer Record",
        RecordPtr,
        FpdtBasicBootPtr->Header.Length,
        PARSER_PARAMS (FpdtBootPerfPointerParser)
        );
      ParseAcpi (
        TRUE,
        4,
        "Firmware Basic Boot Performance Table",
        (UINT8 *)(UINTN)(FpdtBasicBootPtr->BootPerformanceTablePointer),
        sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER),
        PARSER_PARAMS (AcpiFpdtHdrParser)
        );
      FpdtHdr   = (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER *)(UINTN)(FpdtBasicBootPtr->BootPerformanceTablePointer);
      RecOffset = sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER);
      while (RecOffset < FpdtHdr->Length) {
        FpdtBasicBootRec = (EFI_ACPI_6_5_FPDT_FIRMWARE_BASIC_BOOT_RECORD *)((UINT8 *)(UINTN)(FpdtBasicBootPtr->BootPerformanceTablePointer) + RecOffset);
        if (FpdtBasicBootRec->Header.Type != EFI_ACPI_6_5_FPDT_RUNTIME_RECORD_TYPE_FIRMWARE_BASIC_BOOT) {
          /// break the inner-loop if no more basic boot records
          break;
        }

        /// Parse the records inside the Boot Performance Table
        ParseAcpi (
          TRUE,
          4,
          "FPDT Boot Performance Table",
          (UINT8 *)(UINTN)(FpdtBasicBootPtr->BootPerformanceTablePointer) + RecOffset,
          FpdtBasicBootRec->Header.Length,
          PARSER_PARAMS (FpdtBasicBootRecordParser)
          );
        RecOffset += FpdtBasicBootRec->Header.Length;
      }
    } else if (RecordHeader->Type == EFI_ACPI_6_5_FPDT_RECORD_TYPE_S3_PERFORMANCE_TABLE_POINTER) {
      FpdtS3Ptr = (EFI_ACPI_6_5_FPDT_S3_PERFORMANCE_TABLE_POINTER_RECORD *)RecordPtr;
      ParseAcpi (
        TRUE,
        2,
        "FPDT S3 Performance Pointer Record",
        RecordPtr,
        FpdtS3Ptr->Header.Length,
        PARSER_PARAMS (FpdtS3PerfPointerParser)
        );

      ParseAcpi (
        TRUE,
        4,
        "S3 Performance Table Pointer Record",
        (UINT8 *)(UINTN)(FpdtS3Ptr->S3PerformanceTablePointer),
        sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER),
        PARSER_PARAMS (AcpiFpdtHdrParser)
        );
      FpdtHdr   = (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER *)(UINTN)(FpdtS3Ptr->S3PerformanceTablePointer);
      RecOffset = sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER);
      while (RecOffset < FpdtHdr->Length) {
        FpdtRecHdr = (EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8 *)(UINTN)(FpdtS3Ptr->S3PerformanceTablePointer) + RecOffset);
        if (FpdtRecHdr->Type == EFI_ACPI_6_5_FPDT_RUNTIME_RECORD_TYPE_S3_RESUME) {
          FpdtS3ResumeRec = (EFI_ACPI_6_5_FPDT_S3_RESUME_RECORD *)((UINT8 *)(UINTN)(FpdtS3Ptr->S3PerformanceTablePointer) + RecOffset);
          /// Parse the S3 Resume Record
          ParseAcpi (
            TRUE,
            4,
            "FPDT S3 Resume Record",
            (UINT8 *)(UINTN)(FpdtS3Ptr->S3PerformanceTablePointer) + RecOffset,
            FpdtS3ResumeRec->Header.Length,
            PARSER_PARAMS (FpdtS3ResumeRecordParser)
            );
        } else if (FpdtRecHdr->Type == EFI_ACPI_6_5_FPDT_RUNTIME_RECORD_TYPE_S3_SUSPEND) {
          FpdtS3SuspendRec = (EFI_ACPI_6_5_FPDT_S3_SUSPEND_RECORD *)((UINT8 *)(UINTN)(FpdtS3Ptr->S3PerformanceTablePointer) + RecOffset);
          /// Parse the S3 Suspend Record
          ParseAcpi (
            TRUE,
            4,
            "FPDT S3 Suspend Record",
            (UINT8 *)(UINTN)(FpdtS3Ptr->S3PerformanceTablePointer) + RecOffset,
            FpdtS3SuspendRec->Header.Length,
            PARSER_PARAMS (FpdtS3SuspendRecordParser)
            );
        } else {
          /// break the inner-loop if no more S3 records
          break;
        }

        RecOffset += FpdtRecHdr->Length;
      }
    } else {
      /// Parse the reserved record types
      ParseAcpi (
        TRUE,
        2,
        "FPDT Performance Record",
        RecordPtr,
        RecordHeader->Length,
        PARSER_PARAMS (FpdtPerfRecordParser)
        );
    }

    Offset    += RecordHeader->Length;
    RecordPtr += RecordHeader->Length;
  }
}

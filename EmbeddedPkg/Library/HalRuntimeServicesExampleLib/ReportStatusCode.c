/** @file
  Report status code lib on top of either SerialLib and/or EFI Serial Protocol.
  Based on PcdStatusCodeUseEfiSerial & PcdStatusCodeUseHardSerial settings

  There is just a single runtime memory buffer that contans all the data.

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

//////////#include "DxeStatusCode.h"


EFI_SERIAL_IO_PROTOCOL *mSerialIoProtocol = NULL;


EFI_STATUS
LibReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
{
  CHAR8           *Filename;
  CHAR8           *Description;
  CHAR8           *Format;
  CHAR8           Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];
  UINT32          ErrorLevel;
  UINT32          LineNumber;
  UINTN           CharCount;
  VA_LIST         Marker;
  EFI_DEBUG_INFO  *DebugInfo;
  EFI_TPL         CurrentTpl;


  if (FeaturePcdGet (PcdStatusCodeUseEfiSerial)) {
    if (EfiAtRuntime ()) {
      return EFI_DEVICE_ERROR;
    }
    CurrentTpl = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);
    gBS->RestoreTPL (CurrentTpl);

    if (CurrentTpl > EFI_TPL_CALLBACK ) {
      return EFI_DEVICE_ERROR;
    }
  }

  Buffer[0] = '\0';

  if (Data != NULL &&
      ReportStatusCodeExtractAssertInfo (CodeType, Value, Data, &Filename, &Description, &LineNumber)) {
    //
    // Print ASSERT() information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  EFI_STATUS_CODE_DATA_MAX_SIZE,
                  "\n\rDXE_ASSERT!: %a (%d): %a\n\r",
                  Filename,
                  LineNumber,
                  Description
                  );
  } else if (Data != NULL &&
             ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {
    //
    // Print DEBUG() information into output buffer.
    //
    CharCount = AsciiVSPrint (
                  Buffer,
                  EFI_STATUS_CODE_DATA_MAX_SIZE,
                  Format,
                  Marker
                  );
  } else if (Data != NULL &&
             CompareGuid (&Data->Type, &gEfiStatusCodeSpecificDataGuid) &&
             (CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE) {
    //
    // Print specific data into output buffer.
    //
    DebugInfo = (EFI_DEBUG_INFO *) (Data + 1);
    Marker    = (VA_LIST) (DebugInfo + 1);
    Format    = (CHAR8 *) (((UINT64 *) (DebugInfo + 1)) + 12);

    CharCount = AsciiVSPrint (Buffer, EFI_STATUS_CODE_DATA_MAX_SIZE, Format, Marker);
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
    //
    // Print ERROR information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  EFI_STATUS_CODE_DATA_MAX_SIZE,
                  "ERROR: C%x:V%x I%x",
                  CodeType,
                  Value,
                  Instance
                  );

    //
    // Make sure we don't try to print values that weren't
    // intended to be printed, especially NULL GUID pointers.
    //

    if (CallerId != NULL) {
      CharCount += AsciiSPrint (
                     &Buffer[CharCount - 1],
                     (EFI_STATUS_CODE_DATA_MAX_SIZE - (sizeof (Buffer[0]) * CharCount)),
                     " %g",
                     CallerId
                     );
    }

    if (Data != NULL) {
      CharCount += AsciiSPrint (
                     &Buffer[CharCount - 1],
                     (EFI_STATUS_CODE_DATA_MAX_SIZE - (sizeof (Buffer[0]) * CharCount)),
                     " %x",
                     Data
                     );
    }

    CharCount += AsciiSPrint (
                   &Buffer[CharCount - 1],
                   (EFI_STATUS_CODE_DATA_MAX_SIZE - (sizeof (Buffer[0]) * CharCount)),
                   "\n\r"
                   );
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
    CharCount = AsciiSPrint (
                  Buffer,
                  EFI_STATUS_CODE_DATA_MAX_SIZE,
                  "PROGRESS CODE: V%x I%x\n\r",
                  Value,
                  Instance
                  );
  } else {
    CharCount = AsciiSPrint (
                  Buffer,
                  EFI_STATUS_CODE_DATA_MAX_SIZE,
                  "Undefined: C%x:V%x I%x\n\r",
                  CodeType,
                  Value,
                  Instance
                  );
  }


  if (FeaturePcdGet (PcdStatusCodeUseHardSerial)) {
    //
    // Callout to SerialPort Lib function to do print.
    //
    SerialPortWrite ((UINT8 *) Buffer, CharCount);
  }
  if (FeaturePcdGet (PcdStatusCodeUseEfiSerial)) {
    if (mSerialIoProtocol == NULL) {
      gBS->LocateProtocol (&gEfiSerialIoProtocolGuid, NULL, (VOID **) &mSerialIoProtocol);
    }

    if (mSerialIoProtocol == NULL) {
      mSerialIoProtocol->Write (
        mSerialIoProtocol,
        &CharCount,
        Buffer
        );
    }
  }

  return EFI_SUCCESS;
}


VOID
LibReportStatusCodeVirtualAddressChangeEvent (
  VOID
  )
{
  return;
}

VOID
LibReportStatusCodeInitialize (
  VOID
  )
{
  return;
}




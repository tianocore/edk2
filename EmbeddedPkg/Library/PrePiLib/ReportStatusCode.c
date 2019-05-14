/** @file
  Library that helps implement monolithic PEI

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrePi.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>

#include <Protocol/StatusCode.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>

EFI_STATUS
EFIAPI
SerialReportStatusCode (
  IN EFI_STATUS_CODE_TYPE             CodeType,
  IN EFI_STATUS_CODE_VALUE            Value,
  IN UINT32                           Instance,
  IN CONST EFI_GUID                   *CallerId,
  IN CONST EFI_STATUS_CODE_DATA       *Data OPTIONAL
  );


EFI_STATUS_CODE_PROTOCOL gStatusCode = {
  (EFI_REPORT_STATUS_CODE)SerialReportStatusCode
};

/**
  Extracts ASSERT() information from a status code structure.

  Converts the status code specified by CodeType, Value, and Data to the ASSERT()
  arguments specified by Filename, Description, and LineNumber.  If CodeType is
  an EFI_ERROR_CODE, and CodeType has a severity of EFI_ERROR_UNRECOVERED, and
  Value has an operation mask of EFI_SW_EC_ILLEGAL_SOFTWARE_STATE, extract
  Filename, Description, and LineNumber from the optional data area of the
  status code buffer specified by Data.  The optional data area of Data contains
  a Null-terminated ASCII string for the FileName, followed by a Null-terminated
  ASCII string for the Description, followed by a 32-bit LineNumber.  If the
  ASSERT() information could be extracted from Data, then return TRUE.
  Otherwise, FALSE is returned.

  If Data is NULL, then ASSERT().
  If Filename is NULL, then ASSERT().
  If Description is NULL, then ASSERT().
  If LineNumber is NULL, then ASSERT().

  @param  CodeType     The type of status code being converted.
  @param  Value        The status code value being converted.
  @param  Data         Pointer to status code data buffer.
  @param  Filename     Pointer to the source file name that generated the ASSERT().
  @param  Description  Pointer to the description of the ASSERT().
  @param  LineNumber   Pointer to source line number that generated the ASSERT().

  @retval  TRUE   The status code specified by CodeType, Value, and Data was
                  converted ASSERT() arguments specified by Filename, Description,
                  and LineNumber.
  @retval  FALSE  The status code specified by CodeType, Value, and Data could
                  not be converted to ASSERT() arguments.

**/
BOOLEAN
EFIAPI
ReportStatusCodeExtractAssertInfo (
  IN EFI_STATUS_CODE_TYPE        CodeType,
  IN EFI_STATUS_CODE_VALUE       Value,
  IN CONST EFI_STATUS_CODE_DATA  *Data,
  OUT CHAR8                      **Filename,
  OUT CHAR8                      **Description,
  OUT UINT32                     *LineNumber
  )
{
  EFI_DEBUG_ASSERT_DATA  *AssertData;

  ASSERT (Data        != NULL);
  ASSERT (Filename    != NULL);
  ASSERT (Description != NULL);
  ASSERT (LineNumber  != NULL);

  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK)      == EFI_ERROR_CODE) &&
      ((CodeType & EFI_STATUS_CODE_SEVERITY_MASK)  == EFI_ERROR_UNRECOVERED) &&
      ((Value    & EFI_STATUS_CODE_OPERATION_MASK) == EFI_SW_EC_ILLEGAL_SOFTWARE_STATE)) {
    AssertData   = (EFI_DEBUG_ASSERT_DATA *)(Data + 1);
    *Filename    = (CHAR8 *)(AssertData + 1);
    *Description = *Filename + AsciiStrLen (*Filename) + 1;
    *LineNumber  = AssertData->LineNumber;
    return TRUE;
  }
  return FALSE;
}


/**
  Extracts DEBUG() information from a status code structure.

  Converts the status code specified by Data to the DEBUG() arguments specified
  by ErrorLevel, Marker, and Format.  If type GUID in Data is
  EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID, then extract ErrorLevel, Marker, and
  Format from the optional data area of the status code buffer specified by Data.
  The optional data area of Data contains a 32-bit ErrorLevel followed by Marker
  which is 12 UINTN parameters, followed by a Null-terminated ASCII string for
  the Format.  If the DEBUG() information could be extracted from Data, then
  return TRUE.  Otherwise, FALSE is returned.

  If Data is NULL, then ASSERT().
  If ErrorLevel is NULL, then ASSERT().
  If Marker is NULL, then ASSERT().
  If Format is NULL, then ASSERT().

  @param  Data        Pointer to status code data buffer.
  @param  ErrorLevel  Pointer to error level mask for a debug message.
  @param  Marker      Pointer to the variable argument list associated with Format.
  @param  Format      Pointer to a Null-terminated ASCII format string of a
                      debug message.

  @retval  TRUE   The status code specified by Data was converted DEBUG() arguments
                  specified by ErrorLevel, Marker, and Format.
  @retval  FALSE  The status code specified by Data could not be converted to
                  DEBUG() arguments.

**/
BOOLEAN
EFIAPI
ReportStatusCodeExtractDebugInfo (
  IN CONST EFI_STATUS_CODE_DATA  *Data,
  OUT UINT32                     *ErrorLevel,
  OUT BASE_LIST                  *Marker,
  OUT CHAR8                      **Format
  )
{
  EFI_DEBUG_INFO  *DebugInfo;

  ASSERT (Data          != NULL);
  ASSERT (ErrorLevel    != NULL);
  ASSERT (Marker     != NULL);
  ASSERT (Format     != NULL);

  //
  // If the GUID type is not EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID then return FALSE
  //
  if (!CompareGuid (&Data->Type, &gEfiStatusCodeDataTypeDebugGuid)) {
    return FALSE;
  }

  //
  // Retrieve the debug information from the status code record
  //
  DebugInfo = (EFI_DEBUG_INFO *)(Data + 1);

  *ErrorLevel = DebugInfo->ErrorLevel;

  //
  // The first 12 * UINTN bytes of the string are really an
  // argument stack to support varargs on the Format string.
  //
  *Marker = (BASE_LIST) (DebugInfo + 1);
  *Format = (CHAR8 *)(((UINT64 *)*Marker) + 12);

  return TRUE;
}




EFI_STATUS
EFIAPI
SerialReportStatusCode (
  IN EFI_STATUS_CODE_TYPE             CodeType,
  IN EFI_STATUS_CODE_VALUE            Value,
  IN UINT32                           Instance,
  IN CONST EFI_GUID                   *CallerId,
  IN CONST EFI_STATUS_CODE_DATA       *Data OPTIONAL
  )
{
  CHAR8           *Filename;
  CHAR8           *Description;
  CHAR8           *Format;
  CHAR8           Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];
  UINT32          ErrorLevel;
  UINT32          LineNumber;
  UINTN           CharCount;
  BASE_LIST       Marker;
  EFI_DEBUG_INFO  *DebugInfo;

  Buffer[0] = '\0';


  if (Data != NULL &&
    ReportStatusCodeExtractAssertInfo (CodeType, Value, Data, &Filename, &Description, &LineNumber)) {

    //
    // Print ASSERT() information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  EFI_STATUS_CODE_DATA_MAX_SIZE,
                  "\n\rASSERT!: %a (%d): %a\n\r",
                  Filename,
                  LineNumber,
                  Description
                  );


    //
    // Callout to standard output.
    //
    SerialPortWrite ((UINT8 *)Buffer, CharCount);
    return EFI_SUCCESS;

  } else if (Data != NULL &&
    ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {

    //
    // Print DEBUG() information into output buffer.
    //
    CharCount = AsciiBSPrint (
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
    Marker    = (BASE_LIST) (DebugInfo + 1);
    Format    = (CHAR8 *) (((UINT64 *) (DebugInfo + 1)) + 12);

    CharCount = AsciiBSPrint (Buffer, EFI_STATUS_CODE_DATA_MAX_SIZE, Format, Marker);

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
    // Make sure we don't try to print values that weren't intended to be printed, especially NULL GUID pointers.
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

  SerialPortWrite ((UINT8 *)Buffer, CharCount);
  return EFI_SUCCESS;

}


VOID
EFIAPI
AddDxeCoreReportStatusCodeCallback (
  VOID
  )
{
  BuildGuidDataHob (&gEfiStatusCodeRuntimeProtocolGuid, &gStatusCode, sizeof(VOID *));
}


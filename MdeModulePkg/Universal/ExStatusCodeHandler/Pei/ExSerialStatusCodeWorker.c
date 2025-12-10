/** @file
  Serial I/O status code reporting worker.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "ExStatusCodeHandlerPei.h"

/**
  Convert status code value and extended data to readable ASCII string, send string to serial I/O device.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or
                           software entity. This includes information about the class and
                           subclass that is used to classify the entity as well as an operation.
                           For progress codes, the operation is the current activity.
                           For error codes, it is the exception.For debug codes,it is not defined at this time.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. A system may contain multiple entities that match a class/subclass
                           pairing. The instance differentiates between them. An instance of 0 indicates
                           that instance information is unavailable, not meaningful, or not relevant.
                           Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS      Status code reported to serial I/O successfully.

**/
EFI_STATUS
EFIAPI
ExSerialStatusCodeReportWorker (
  IN CONST  EFI_PEI_SERVICES     **PeiServices,
  IN EFI_STATUS_CODE_TYPE        CodeType,
  IN EFI_STATUS_CODE_VALUE       Value,
  IN UINT32                      Instance,
  IN CONST EFI_GUID              *CallerId,
  IN CONST EFI_STATUS_CODE_DATA  *Data OPTIONAL
  )
{
  CHAR8          *Filename;
  CHAR8          *Description;
  CHAR8          *Format;
  CHAR8          Buffer[MAX_EX_DEBUG_STR_LEN];
  CHAR8          *BufferPtr;
  UINT32         ErrorLevel;
  UINT32         LineNumber;
  UINTN          CharCount;
  BASE_LIST      Marker;
  EX_DEBUG_INFO  *ExDebugInfo = NULL;

  Buffer[0] = '\0';
  CharCount = 0;

  if ((Data != NULL) &&
      CompareGuid (&Data->Type, &gStatusCodeDataTypeExDebugGuid))
  {
    ExDebugInfo = (EX_DEBUG_INFO *)(Data + 1);
  } else if ((Data != NULL) &&
             ReportStatusCodeExtractAssertInfo (CodeType, Value, Data, &Filename, &Description, &LineNumber))
  {
    //
    // Print ASSERT() information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "\n\rPEI_ASSERT!: %a (%d): %a\n\r",
                  Filename,
                  LineNumber,
                  Description
                  );
  } else if ((Data != NULL) &&
             ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format))
  {
    //
    // Print DEBUG() information into output buffer.
    //
    CharCount = AsciiBSPrint (
                  Buffer,
                  sizeof (Buffer),
                  Format,
                  Marker
                  );
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
    //
    // Print ERROR information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "ERROR: C%08x:V%08x I%x",
                  CodeType,
                  Value,
                  Instance
                  );

    ASSERT (CharCount > 0);

    if (CallerId != NULL) {
      CharCount += AsciiSPrint (
                     &Buffer[CharCount],
                     (sizeof (Buffer) - (sizeof (Buffer[0]) * CharCount)),
                     " %g",
                     CallerId
                     );
    }

    if (Data != NULL) {
      CharCount += AsciiSPrint (
                     &Buffer[CharCount],
                     (sizeof (Buffer) - (sizeof (Buffer[0]) * CharCount)),
                     " %x",
                     Data
                     );
    }

    CharCount += AsciiSPrint (
                   &Buffer[CharCount],
                   (sizeof (Buffer) - (sizeof (Buffer[0]) * CharCount)),
                   "\n\r"
                   );
  } else if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
    //
    // Print PROGRESS information into output buffer.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "PROGRESS CODE: V%08x I%x\n\r",
                  Value,
                  Instance
                  );
  } else if ((Data != NULL) &&
             CompareGuid (&Data->Type, &gEfiStatusCodeDataTypeStringGuid) &&
             (((EFI_STATUS_CODE_STRING_DATA *)Data)->StringType == EfiStringAscii))
  {
    //
    // EFI_STATUS_CODE_STRING_DATA
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "%a",
                  ((EFI_STATUS_CODE_STRING_DATA *)Data)->String.Ascii
                  );
  } else {
    //
    // Code type is not defined.
    //
    CharCount = AsciiSPrint (
                  Buffer,
                  sizeof (Buffer),
                  "Undefined: C%08x:V%08x I%x\n\r",
                  CodeType,
                  Value,
                  Instance
                  );
  }

  //
  // No EX info, just call SerialPort Lib function to do print and exit.
  //
  if (ExDebugInfo == NULL) {
    SerialPortWrite ((UINT8 *)Buffer, CharCount);
    return EFI_SUCCESS;
  }

  //
  // EX handling here - point at correct string and then take action.
  // Acquire print, process buffer, write to serial, release print.
  // Skip any if not requested.
  //
  CharCount = ExDebugInfo->DebugStringLen;
  BufferPtr = ExDebugInfo->DebugString;
  if (ExDebugInfo->PrintSyncAcquire != NULL) {
    ExDebugInfo->PrintSyncAcquire ();
  }

  if (ExDebugInfo->ProcessBuffer != NULL) {
    BufferPtr = ExDebugInfo->ProcessBuffer (
                               ExDebugInfo->ProcessDataPtr,
                               Buffer,
                               &CharCount
                               );
  }

  if (BufferPtr != NULL) {
    SerialPortWrite ((UINT8 *)BufferPtr, CharCount);
  }

  if ((ExDebugInfo != NULL) && (ExDebugInfo->PrintSyncRelease != NULL)) {
    ExDebugInfo->PrintSyncRelease ();
  }

  return EFI_SUCCESS;
}

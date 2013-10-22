/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ReportStatusCode.c

Abstract:

  Worker functions for ReportStatusCode

--*/

#include "TianoCommon.h"
#include "EfiCommonLib.h"
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)


VOID *
EfiConstructStatusCodeData (
  IN  UINT16                    DataSize,
  IN  EFI_GUID                  *TypeGuid,
  IN OUT  EFI_STATUS_CODE_DATA  *Data
  )
/*++

Routine Description:

  Construct stanader header for optional data passed into ReportStatusCode

Arguments:

  DataSize - Size of optional data. Does not include EFI_STATUS_CODE_DATA header
  TypeGuid - GUID to place in EFI_STATUS_CODE_DATA
  Data     - Buffer to use.

Returns:

  Return pointer to Data buffer pointing past the end of EFI_STATUS_CODE_DATA

--*/
{
  Data->HeaderSize = (UINT16) sizeof (EFI_STATUS_CODE_DATA);
  Data->Size = (UINT16)(DataSize - sizeof (EFI_STATUS_CODE_DATA));
  EfiCommonLibCopyMem (&Data->Type, TypeGuid, sizeof (EFI_GUID));
  
  return (VOID *)(Data + 1); 
}

EFI_STATUS
EfiDebugVPrintWorker (
  IN  UINTN                   ErrorLevel,
  IN  CHAR8                   *Format,
  IN  VA_LIST                 Marker,
  IN  UINTN                   BufferSize,
  IN OUT VOID                 *Buffer
  )
/*++

Routine Description:

  Worker function for DEBUG(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.

  The Format string might be truncated to fit into the status code struture
  which has the max size of EFI_STATUS_CODE_DATA_MAX_SIZE.

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  Marker     - VarArgs

  BufferSize - Size of Buffer.

  Buffer     - Caller allocated buffer, contains ReportStatusCode extended data
  
Returns:
  
  Status code
  
  EFI_SUCCESS             - Successfully printed

--*/
{
  UINTN                   Index;
  UINTN                   FormatStrLen;
  UINTN                   RemainingStrLen;
  UINT64                  *Ptr;
  EFI_DEBUG_INFO          *EfiDebug;

  
  //
  // Build the type specific EFI_STATUS_CODE_DATA in order
  //

  //
  // Fill in EFI_STATUS_CODE_DATA to Buffer.
  //
  EfiDebug = (EFI_DEBUG_INFO *)EfiConstructStatusCodeData (
                                (UINT16)BufferSize, 
                                &gEfiStatusCodeDataTypeDebugGuid, 
                                Buffer
                                );

  //
  // Then EFI_DEBUG_INFO
  //
  EfiDebug->ErrorLevel = (UINT32)ErrorLevel;

  //
  // 12 * sizeof (UINT64) byte mini Var Arg stack.
  // That is followed by the format string.
  //
  for (Index = 0, Ptr = (UINT64 *)(EfiDebug + 1); Index < 12; Index++, Ptr++) {
    *Ptr = VA_ARG (Marker, UINT64);
  }

  //
  // Place Ascii Format string at the end
  // Truncate it to fit into the status code structure
  //
  FormatStrLen    = EfiAsciiStrLen (Format);
  RemainingStrLen = EFI_STATUS_CODE_DATA_MAX_SIZE
    - sizeof (EFI_STATUS_CODE_DATA)
    - sizeof (EFI_DEBUG_INFO)
    - 12 * sizeof (UINT64) - 1;
  if (FormatStrLen > RemainingStrLen) {
    FormatStrLen = RemainingStrLen;
  }
  EfiCommonLibCopyMem (Ptr, Format, FormatStrLen);
  *((CHAR8 *) Ptr + FormatStrLen) = '\0';

  return EFI_SUCCESS;
}



EFI_STATUS
EfiDebugAssertWorker (
  IN CHAR8                    *Filename,
  IN INTN                     LineNumber,
  IN CHAR8                    *Description,
  IN UINTN                    BufferSize,
  IN OUT VOID                 *Buffer
  )
/*++

Routine Description:

  Worker function for ASSERT (). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded DEADLOOP ().

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  Filename    - File name of failing routine.

  LineNumber  - Line number of failing ASSERT().

  Description - Description, usually the assertion,
  
  BufferSize - Size of Buffer.

  Buffer     - Caller allocated buffer, contains ReportStatusCode extendecd data

Returns:
  
  Status code
  
  EFI_BUFFER_TOO_SMALL      - Buffer not large enough
  
  EFI_SUCCESS               - Function successfully done.

--*/
{
  EFI_DEBUG_ASSERT_DATA   *AssertData;
  UINTN                   TotalSize;
  CHAR8                   *EndOfStr;

  //
  // Make sure it will all fit in the passed in buffer
  //
  TotalSize = sizeof (EFI_STATUS_CODE_DATA) + sizeof (EFI_DEBUG_ASSERT_DATA);
  TotalSize += EfiAsciiStrLen (Filename);
  TotalSize += EfiAsciiStrLen (Description);
  if (TotalSize > BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Fill in EFI_STATUS_CODE_DATA
  //
  AssertData =  (EFI_DEBUG_ASSERT_DATA *)
                EfiConstructStatusCodeData (
                  (UINT16)(TotalSize - sizeof (EFI_STATUS_CODE_DATA)),
                  &gEfiStatusCodeDataTypeAssertGuid, 
                  Buffer
                  );

  //
  // Fill in EFI_DEBUG_ASSERT_DATA
  //
  AssertData->LineNumber = (UINT32)LineNumber;

  //
  // Copy Ascii FileName including NULL.
  //
  EndOfStr = EfiAsciiStrCpy ((CHAR8 *)(AssertData + 1), Filename);

  //
  // Copy Ascii Description 
  //
  EfiAsciiStrCpy (EndOfStr, Description);
  return EFI_SUCCESS;
}



BOOLEAN
ReportStatusCodeExtractAssertInfo (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,  
  IN EFI_STATUS_CODE_DATA     *Data, 
  OUT CHAR8                   **Filename,
  OUT CHAR8                   **Description,
  OUT UINT32                  *LineNumber
  )
/*++

Routine Description:

  Extract assert information from status code data.

Arguments:

  CodeType    - Code type
  Value       - Code value
  Data        - Optional data associated with this status code.
  Filename    - Filename extracted from Data
  Description - Description extracted from Data
  LineNumber  - Line number extracted from Data

Returns:

  TRUE      - Successfully extracted
  
  FALSE     - Extraction failed

--*/
{
  EFI_DEBUG_ASSERT_DATA   *AssertData;

  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) && 
        ((CodeType & EFI_STATUS_CODE_SEVERITY_MASK) == EFI_ERROR_UNRECOVERED)) {
    //
    // Assume if we have an uncontained unrecoverable error that the data hub
    // may not work. So we will print out data here. If we had an IPMI controller,
    // or error log we could wack the hardware here.
    //
    if ((Value & EFI_STATUS_CODE_OPERATION_MASK) == EFI_SW_EC_ILLEGAL_SOFTWARE_STATE && (Data != NULL)) {
      //
      // ASSERT (Expresion) - 
      // ExtendedData == FileName
      // Instance     == Line Nuber
      // NULL         == String of Expresion
      //
      AssertData = (EFI_DEBUG_ASSERT_DATA *)(Data + 1);
      *Filename = (CHAR8 *)(AssertData + 1);
      *Description = *Filename + EfiAsciiStrLen (*Filename) + 1;
      *LineNumber = AssertData->LineNumber;
      return TRUE;
    } 
  }
  return FALSE;
}


BOOLEAN
ReportStatusCodeExtractDebugInfo (
  IN EFI_STATUS_CODE_DATA     *Data,
  OUT UINT32                  *ErrorLevel,
  OUT VA_LIST                 *Marker,
  OUT CHAR8                   **Format
  )
/*++

Routine Description:

  Extract debug information from status code data.

Arguments:

  Data        - Optional data associated with status code.
  ErrorLevel  - Error level extracted from Data
  Marker      - VA_LIST extracted from Data
  Format      - Format string extracted from Data

Returns:

  TRUE      - Successfully extracted
  
  FALSE     - Extraction failed

--*/
{
  EFI_DEBUG_INFO      *DebugInfo;

  if ((Data == NULL) || (!EfiCompareGuid (&Data->Type, &gEfiStatusCodeDataTypeDebugGuid))) {
    return FALSE;
  }
  
  DebugInfo = (EFI_DEBUG_INFO *)(Data + 1);

  *ErrorLevel = DebugInfo->ErrorLevel;

  //
  // The first 12 * UINTN bytes of the string are really an 
  // arguement stack to support varargs on the Format string.
  //
#if (defined (EFIARM) || defined(__APPLE__))
  // It is not legal C code to case VA_LIST to a pointer. VA_LIST can 
  // be a structure. 
  return FALSE;
#else
  *Marker = (VA_LIST) (DebugInfo + 1);
  *Format = (CHAR8 *)(((UINT64 *)*Marker) + 12);
  return TRUE;
#endif
}

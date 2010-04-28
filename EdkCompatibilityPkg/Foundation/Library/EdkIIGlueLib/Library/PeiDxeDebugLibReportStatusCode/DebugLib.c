/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.   


Module Name:

  DebugLib.c
  
Abstract: 

  Debug Library that fowards all messages to ReportStatusCode()
  
--*/

#include "EdkIIGlueDxe.h"

/**

  Prints a debug message to the debug output device if the specified error level is enabled.

  If any bit in ErrorLevel is also set in PcdDebugPrintErrorLevel, then print 
  the message specified by Format and the associated variable argument list to 
  the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.

**/
VOID
EFIAPI
DebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  UINT64          Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE / sizeof (UINT64)];
  EFI_DEBUG_INFO  *DebugInfo;
  UINTN           TotalSize;
  UINTN           Index;
  VA_LIST         Marker;
  UINT64          *ArgumentPointer;

  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  //
  // Check driver Debug Level value and global debug level
  //
  if ((ErrorLevel & PcdGet32(PcdDebugPrintErrorLevel)) == 0) {
    return;
  }

  TotalSize = sizeof (EFI_DEBUG_INFO) + 12 * sizeof (UINT64) + AsciiStrLen (Format) + 1;
  if (TotalSize > EFI_STATUS_CODE_DATA_MAX_SIZE) {
    return;
  }

  //
  // Then EFI_DEBUG_INFO
  //
  DebugInfo = (EFI_DEBUG_INFO *)Buffer;
  DebugInfo->ErrorLevel = (UINT32)ErrorLevel;

  //
  // 256 byte mini Var Arg stack. That is followed by the format string.
  //
  VA_START (Marker, Format);
  for (Index = 0, ArgumentPointer = (UINT64 *)(DebugInfo + 1); Index < 12; Index++, ArgumentPointer++) {
    WriteUnaligned64(ArgumentPointer, VA_ARG (Marker, UINT64));
  }
  VA_END (Marker);
  AsciiStrCpy ((CHAR8 *)ArgumentPointer, Format);

  REPORT_STATUS_CODE_EX (
    EFI_DEBUG_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_DC_UNSPECIFIED),
    0,
    NULL,
    &gEfiStatusCodeDataTypeDebugGuid,
    DebugInfo,
    TotalSize
    );
}


/**

  Prints an assert message containing a filename, line number, and description.  
  This may be followed by a breakpoint or a dead loop.

  Print a message of the form "ASSERT <FileName>(<LineNumber>): <Description>\n" 
  to the debug output device.  If DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED bit of 
  PcdDebugProperyMask is set then CpuBreakpoint() is called. Otherwise, if 
  DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED bit of PcdDebugProperyMask is set then 
  CpuDeadLoop() is called.  If neither of these bits are set, then this function 
  returns immediately after the message is printed to the debug output device.
  DebugAssert() must actively prevent recusrsion.  If DebugAssert() is called while
  processing another DebugAssert(), then DebugAssert() must return immediately.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.

  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     Pointer to the name of the source file that generated the assert condition.
  @param  LineNumber   The line number in the source file that generated the assert condition
  @param  Description  Pointer to the description of the assert condition.

**/
VOID
EFIAPI
DebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  UINT64                 Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE / sizeof(UINT64)];
  EFI_DEBUG_ASSERT_DATA  *AssertData;
  UINTN                  TotalSize;
  CHAR8                  *Temp;
  UINTN                  FileNameLength;
  UINTN                  DescriptionLength;

  //
  // Make sure it will all fit in the passed in buffer
  //
  FileNameLength    = AsciiStrLen (FileName);
  DescriptionLength = AsciiStrLen (Description);
  TotalSize = sizeof (EFI_DEBUG_ASSERT_DATA) + FileNameLength + 1 + DescriptionLength + 1;
  if (TotalSize <= EFI_STATUS_CODE_DATA_MAX_SIZE) {
    //
    // Fill in EFI_DEBUG_ASSERT_DATA
    //
    AssertData = (EFI_DEBUG_ASSERT_DATA *)Buffer;
    AssertData->LineNumber = (UINT32)LineNumber;

    //
    // Copy Ascii FileName including NULL.
    //
    Temp = AsciiStrCpy ((CHAR8 *)(AssertData + 1), FileName);

    //
    // Copy Ascii Description 
    //
    AsciiStrCpy (Temp + AsciiStrLen (FileName) + 1, Description);

    REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
      (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
      (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE),
      AssertData,
      TotalSize
      );
  }

  //
  // Generate a Breakpoint, DeadLoop, or NOP based on PCD settings
  //
  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED) != 0) {
    CpuBreakpoint ();
  } else if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED) != 0) {
    CpuDeadLoop ();
  }
}


/**

  Fills a target buffer with PcdDebugClearMemoryValue, and returns the target buffer.

  This function fills Length bytes of Buffer with the value specified by 
  PcdDebugClearMemoryValue, and returns Buffer.

  If Buffer is NULL, then ASSERT().

  If Length is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

  @param   Buffer  Pointer to the target buffer to fill with PcdDebugClearMemoryValue.
  @param   Length  Number of bytes in Buffer to fill with zeros PcdDebugClearMemoryValue. 

  @return  Buffer

**/
VOID *
EFIAPI
DebugClearMemory (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
  //
  // If Buffer is NULL, then ASSERT().
  //
  ASSERT (Buffer != NULL);

  //
  // SetMem() checks for the the ASSERT() condition on Length and returns Buffer
  //
  return SetMem (Buffer, Length, PcdGet8(PcdDebugClearMemoryValue));
}


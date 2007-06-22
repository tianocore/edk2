/** @file
  UEFI Debug Library that uses PrintLib to send messages to STDERR.

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

//
// Define the maximum debug and assert message length that this library supports 
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100


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
  CHAR16   Buffer[MAX_DEBUG_MESSAGE_LENGTH];
  VA_LIST  Marker;

  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  //
  // Check driver debug mask value and global mask
  //
  if ((ErrorLevel & PcdGet32(PcdDebugPrintErrorLevel)) == 0) {
    return;
  }

  //
  // Convert the DEBUG() message to a Unicode String
  //
  VA_START (Marker, Format);
  UnicodeVSPrintAsciiFormat (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  //
  // Send the print string to the Standard Error device
  //
  if ((gST != NULL) && (gST->StdErr != NULL)) {
    gST->StdErr->OutputString (gST->StdErr, Buffer);
  }
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
  CHAR16  Buffer[MAX_DEBUG_MESSAGE_LENGTH];

  //
  // Generate the ASSERT() message in Unicode format
  //
  UnicodeSPrintAsciiFormat (Buffer, sizeof (Buffer), "ASSERT %s(%d): %s\n", FileName, LineNumber, Description);

  //
  // Send the print string to the Standard Error device
  //
  if ((gST != NULL) && (gST->StdErr != NULL)) {
    gST->StdErr->OutputString (gST->StdErr, Buffer);
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


/**
  
  Returns TRUE if ASSERT() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugAssertEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED) != 0);
}


/**
  
  Returns TRUE if DEBUG()macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_PRINT_ENABLED) != 0);
}


/**
  
  Returns TRUE if DEBUG_CODE()macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugCodeEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_CODE_ENABLED) != 0);
}


/**
  
  Returns TRUE if DEBUG_CLEAR_MEMORY()macro is enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_CLEAR_MEMORY_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugClearMemoryEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED) != 0);
}

/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugLib.c

Abstract:

  EFI Debug Library 

--*/

static BOOLEAN                   mDebugLevelInstalled = FALSE;
static EFI_DEBUG_LEVEL_PROTOCOL  mDebugLevel = { 0 };

EFI_STATUS
DebugLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_STATUS               Status;

  //
  // Initialize Debug Level Protocol
  //
  mDebugLevel.DebugLevel = PcdGet32(PcdDebugPrintErrorLevel);

  //
  // Install Debug Level Protocol 
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiDebugLevelProtocolGuid, &mDebugLevel,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Set flag to show that the Debug Level Protocol has been installed
  //
  mDebugLevelInstalled = TRUE;

  return EFI_SUCCESS;
}

VOID
EFIAPI
DebugAssert (
  IN CHAR8  *FileName,
  IN INTN   LineNumber,
  IN CHAR8  *Description
  )
/*++

Routine Description:

  Worker function for ASSERT(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded CpuBreakpoint ().

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  FileName    - File name of failing routine.

  LineNumber  - Line number of failing ASSERT().

  Description - Descritption, usally the assertion,
  
Returns:
  
  None

--*/
{
  UINT64                 Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE / sizeof(UINT64)];
  EFI_DEBUG_ASSERT_DATA  *AssertData;
  UINTN                  TotalSize;
  CHAR8                  *Temp;

  if ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED) == 0) {
    return;
  }

  //
  // Make sure it will all fit in the passed in buffer
  //
  TotalSize = sizeof (EFI_DEBUG_ASSERT_DATA) + AsciiStrLen (FileName) + 1 + AsciiStrLen (Description) + 1;
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
    AsciiStrCpy (Temp + AsciiStrLen(FileName) + 1, Description);

    REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
      (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
      (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE),
      AssertData,
      TotalSize
      );
  }

  //
  // Put break point in module that contained the error.
  //
  CpuBreakpoint ();
}

VOID
DebugVPrint (
  IN  UINTN    ErrorLevel,
  IN  CHAR8    *Format,
  IN  VA_LIST  Marker
  )
/*++

Routine Description:

  Worker function for DEBUG(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  Marker     - VarArgs
  
Returns:
  
  None

--*/
{
  UINT64          Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE / sizeof (UINT64)];
  EFI_DEBUG_INFO  *DebugInfo;
  UINTN           TotalSize;
  UINTN           Index;
  UINT64          *ArgumentPointer;

  //
  // Check driver Debug Level value and global debug level
  //
  if (mDebugLevelInstalled) {
    if ((ErrorLevel & mDebugLevel.DebugLevel) == 0) {
      return;
    }
  } else {
    if ((ErrorLevel & PcdGet32(PcdDebugPrintErrorLevel)) == 0) {
      return;
    }
  }

  TotalSize = sizeof (EFI_DEBUG_INFO) + 12 * sizeof (UINT64 *) + AsciiStrLen (Format) + 1;
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
  for (Index = 0, ArgumentPointer = (UINT64 *)(DebugInfo + 1); Index < 12; Index++, ArgumentPointer++) {
    *ArgumentPointer = VA_ARG (Marker, UINT64);
  }
  AsciiStrCpy ((CHAR8 *)ArgumentPointer, Format);

  //
  //
  //
  REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
    EFI_DEBUG_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_DC_UNSPECIFIED),
    DebugInfo,
    TotalSize
    );
}

VOID
EFIAPI
DebugPrint (
  IN  UINTN  ErrorLevel,
  IN  CHAR8  *Format,
  ...
  )
/*++

Routine Description:

  Wrapper for DebugVPrint ()
  
Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  ...        - Print arguments.

Returns:
  
  None

--*/
{
  VA_LIST Marker;

  VA_START (Marker, Format);
  DebugVPrint (ErrorLevel, Format, Marker);
  VA_END (Marker);
}

/**
  Fills a target buffer with PcdDebugClearMemoryValue, and returns the target buffer.

  This function fills Length bytes of Buffer with the value specified by 
  PcdDebugClearMemoryValue, and returns Buffer.

  If Buffer is NULL, then ASSERT().

  If Length is greater than (MAX_ADDRESS – Buffer + 1), then ASSERT(). 

  @param  Buffer  Pointer to the target buffer to fill with PcdDebugClearMemoryValue.
  @param  Length  Number of bytes in Buffer to fill with zeros PcdDebugClearMemoryValue. 

  @return  Buffer

**/
VOID *
EFIAPI
DebugClearMemory (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
//  SetMem (Buffer, Length, PcdGet8(PcdDebugClearMemoryValue));
  SetMem (Buffer, Length, 0xAF);
  return Buffer;
}

BOOLEAN
EFIAPI
DebugAssertEnabled (
  VOID
  )
{
  return ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED) != 0);
}

BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  )
{
  return ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_PRINT_ENABLED) != 0);
}

BOOLEAN
EFIAPI
DebugCodeEnabled (
  VOID
  )
{
  return ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_CODE_ENABLED) != 0);
}

BOOLEAN
EFIAPI
DebugClearMemoryEnabled (
  VOID
  )
{
  return ((PcdGet8(PcdDebugPropertyMask) & DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED) != 0);
}


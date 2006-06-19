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

  UEFI Debug Library that uses PrintLib to send messages to CONOUT

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
  CHAR16   Buffer[0x100];
  CHAR16   UnicodeBuffer[0x100];
  UINT32   Index;
  VA_LIST  Marker;

  //
  // Check to see if CONOUT is avilable
  //
  if (gST->ConOut == NULL) {
    return;
  }

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

  //
  // BUGBUG: Need print that take CHAR8 Format and returns CHAR16 Buffer
  //
  for (Index = 0; Format[Index] != 0; Index++) {
    UnicodeBuffer[Index] = Format[Index];
  }
  UnicodeBuffer[Index] = Format[Index];

  //
  // Convert the DEBUG() message to a Unicode String
  //
  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), UnicodeBuffer, Marker);
  VA_END (Marker);

  //
  // Send the print string to the Standard Error device
  //
  gST->ConOut->OutputString (gST->ConOut, Buffer);
}

VOID
EFIAPI
DebugAssert (
  IN CHAR8  *FileName,
  IN UINTN  LineNumber,
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
  CHAR16  Buffer[0x100];

  //
  // Check to see if CONOUT is avilable
  //
  if (gST->ConOut == NULL) {
    return;
  }

  //
  // Generate the ASSERT() message in Unicode format
  //
  UnicodeSPrint (Buffer, sizeof (Buffer), (CHAR16 *)L"ASSERT %s(%d): %s\n", FileName, LineNumber, Description);

  //
  // Send the print string to the Standard Error device
  //
  gST->ConOut->OutputString (gST->ConOut, Buffer);

  //
  // Put break point in module that contained the error.
  //
  CpuBreakpoint ();
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

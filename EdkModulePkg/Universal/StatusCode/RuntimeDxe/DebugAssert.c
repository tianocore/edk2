/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugAssert.c

Abstract:

  Produce EfiDebugAssertProtocol to enable EfiUtilityLib to function.
  The EfiUtilityLib is used by the EFI shell!

--*/

#include "StatusCode.h"

EFI_STATUS
EFIAPI
StatusCodeDebugAssert (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN CHAR8                              *FileName,
  IN INTN                               LineNumber,
  IN CHAR8                              *Description
  );

EFI_STATUS
EFIAPI
StatusCodeDebugPrint (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN UINTN                              ErrorLevel,
  IN CHAR8                              *Format,
  IN VA_LIST                            Marker
  );

EFI_STATUS
EFIAPI
StatusCodePostCode (
  IN EFI_DEBUG_ASSERT_PROTOCOL          * This,
  IN  UINT16                            PostCode,
  IN  CHAR8                             *PostCodeString OPTIONAL
  );

EFI_STATUS
EFIAPI
StatusCodeGetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             *ErrorLevel
  );

EFI_STATUS
EFIAPI
StatusCodeSetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             ErrorLevel
  );

//
// Protocol instance, there can be only one.
//
EFI_HANDLE                mHandle = NULL;
EFI_DEBUG_ASSERT_PROTOCOL mDebugAssertProtocol = {
  StatusCodeDebugAssert,
  StatusCodeDebugPrint,
  StatusCodePostCode,
  StatusCodeGetErrorLevel,
  StatusCodeSetErrorLevel
};

//
// Function implementations
//
EFI_STATUS
EFIAPI
StatusCodeDebugAssert (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN CHAR8                              *FileName,
  IN INTN                               LineNumber,
  IN CHAR8                              *Description
  )
/*++

Routine Description:

  Worker function for ASSERT (). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded CpuBreakpoint ().
  
Arguments:

  This        - Protocol instance.
  FileName    - File name of failing routine.
  LineNumber  - Line number of failing ASSERT().
  Description - Description, usually the assertion,

Returns:

  EFI_SUCCESS   The function always completes successfully.

--*/
{
  DebugAssert (FileName, LineNumber, Description);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
StatusCodeDebugPrint (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN UINTN                              ErrorLevel,
  IN CHAR8                              *Format,
  IN VA_LIST                            Marker
  )
/*++

Routine Description:

  Worker function for DEBUG (). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.
  
Arguments:

  This       - Protocol Instance.
  ErrorLevel - If error level is set do the debug print.
  Format     - String to use for the print, followed by Print arguments.

Returns:

  EFI_SUCCESS   The function always completes successfully.

--*/
{
  CHAR8  Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];

  AsciiVSPrint (Buffer, EFI_STATUS_CODE_DATA_MAX_SIZE, Format, Marker);
  DebugPrint (ErrorLevel, Buffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
StatusCodeGetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             *ErrorLevel
  )
{
  *ErrorLevel = PcdGet32(PcdDebugPrintErrorLevel);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
StatusCodeSetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             ErrorLevel
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
StatusCodePostCode (
  IN EFI_DEBUG_ASSERT_PROTOCOL          * This,
  IN  UINT16                            PostCode,
  IN  CHAR8                             *PostCodeString OPTIONAL
  )
/*++

Routine Description:

  Write the code to IO ports 80 and 81.

Arguments:

  This            - Protocol Instance.
  PostCode        - Code to write
  PostCodeString  - String, currently ignored.

Returns:

  EFI_SUCCESS   The function always completes successfully.

--*/
{
  IoWrite8 (0x80, (UINT8) (PostCode & 0xff));
  IoWrite8 (0x81, (UINT8) (PostCode >> 8));

  return EFI_SUCCESS;
}

EFI_STATUS
InstallStatusCodeDebugAssert (
  VOID
  )
/*++

Routine Description:

  Install the status code debug assert protocol

Arguments:

  None

Returns:

  Results of call to InstallProtocolInterface.

--*/
{

  DEBUG_CODE (
    gBS->InstallProtocolInterface (
          &mHandle,
          &gEfiDebugAssertProtocolGuid,
          EFI_NATIVE_INTERFACE,
          &mDebugAssertProtocol
          );
  );

  return EFI_SUCCESS;
}

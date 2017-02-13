/** @file
  This driver produces Print2 protocols layered on top of the PrintLib from the MdePkg.

Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Protocol/Print2.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>

/**
  Implementaion of the UnicodeValueToString service in EFI_PRINT2_PROTOCOL.

  If the macro DISABLE_NEW_DEPRECATED_INTERFACES is defined, then ASSERT().

  @param  Buffer  The pointer to the output buffer for the produced
                  Null-terminated Unicode string.
  @param  Flags   The bitmask of flags that specify left justification, zero
                  pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width   The maximum number of Unicode characters to place in Buffer,
                  not including the Null-terminator.

  @return If the macro DISABLE_NEW_DEPRECATED_INTERFACES is defined, return 0.
          Otherwise, return the number of Unicode characters in Buffer not
          including the Null-terminator.

**/
UINTN
EFIAPI
PrintDxeUnicodeValueToString (
  IN OUT CHAR16  *Buffer,
  IN UINTN       Flags,
  IN INT64       Value,
  IN UINTN       Width
  )
{
#ifdef DISABLE_NEW_DEPRECATED_INTERFACES
  //
  // If the macro DISABLE_NEW_DEPRECATED_INTERFACES is defined, then the
  // PrintLib API UnicodeValueToString is already deprecated.
  // In this case, ASSERT will be triggered and zero will be returned for the
  // implementation of the UnicodeValueToString service in EFI_PRINT2_PROTOCOL
  // to indicate that the service is no longer supported.
  //
  DEBUG ((DEBUG_ERROR, "PrintDxe: The UnicodeValueToString service in EFI_PRINT2_PROTOCOL is no longer supported for security reason.\n"));
  DEBUG ((DEBUG_ERROR, "PrintDxe: Please consider using the UnicodeValueToStringS service in EFI_PRINT2S_PROTOCOL.\n"));
  ASSERT (FALSE);
  return 0;
#else
  return UnicodeValueToString (Buffer, Flags, Value, Width);
#endif
}

/**
  Implementaion of the AsciiValueToString service in EFI_PRINT2_PROTOCOL.

  If the macro DISABLE_NEW_DEPRECATED_INTERFACES is defined, then ASSERT().

  @param  Buffer  A pointer to the output buffer for the produced
                  Null-terminated ASCII string.
  @param  Flags   The bitmask of flags that specify left justification, zero
                  pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width   The maximum number of ASCII characters to place in Buffer,
                  not including the Null-terminator.

  @return If the macro DISABLE_NEW_DEPRECATED_INTERFACES is defined, return 0.
          Otherwise, return the number of ASCII characters in Buffer not
          including the Null-terminator.

**/
UINTN
EFIAPI
PrintDxeAsciiValueToString (
  OUT CHAR8      *Buffer,
  IN  UINTN      Flags,
  IN  INT64      Value,
  IN  UINTN      Width
  )
{
#ifdef DISABLE_NEW_DEPRECATED_INTERFACES
  //
  // If the macro DISABLE_NEW_DEPRECATED_INTERFACES is defined, then the
  // PrintLib API AsciiValueToString is already deprecated.
  // In this case, ASSERT will be triggered and zero will be returned for the
  // implementation of the AsciiValueToString service in EFI_PRINT2_PROTOCOL
  // to indicate that the service is no longer supported.
  //
  DEBUG ((DEBUG_ERROR, "PrintDxe: The AsciiValueToString service in EFI_PRINT2_PROTOCOL is no longer supported for security reason.\n"));
  DEBUG ((DEBUG_ERROR, "PrintDxe: Please consider using the AsciiValueToStringS service in EFI_PRINT2S_PROTOCOL.\n"));
  ASSERT (FALSE);
  return 0;
#else
  return AsciiValueToString (Buffer, Flags, Value, Width);
#endif
}

EFI_HANDLE  mPrintThunkHandle = NULL;

CONST EFI_PRINT2_PROTOCOL mPrint2Protocol = {
  UnicodeBSPrint,
  UnicodeSPrint,
  UnicodeBSPrintAsciiFormat,
  UnicodeSPrintAsciiFormat,
  PrintDxeUnicodeValueToString,
  AsciiBSPrint,
  AsciiSPrint,
  AsciiBSPrintUnicodeFormat,
  AsciiSPrintUnicodeFormat,
  PrintDxeAsciiValueToString
};

CONST EFI_PRINT2S_PROTOCOL mPrint2SProtocol = {
  UnicodeBSPrint,
  UnicodeSPrint,
  UnicodeBSPrintAsciiFormat,
  UnicodeSPrintAsciiFormat,
  UnicodeValueToStringS,
  AsciiBSPrint,
  AsciiSPrint,
  AsciiBSPrintUnicodeFormat,
  AsciiSPrintUnicodeFormat,
  AsciiValueToStringS
};

/**
  The user Entry Point for Print module.

  This is the entry point for Print DXE Driver. It installs the Print2 Protocol.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Others            Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
PrintEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mPrintThunkHandle,
                  &gEfiPrint2ProtocolGuid, &mPrint2Protocol,
                  &gEfiPrint2SProtocolGuid, &mPrint2SProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** @file
  This driver produces Print2 protocols layered on top of the PrintLib from the MdePkg.

Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/Print2.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>

/**
  Implementaion of the UnicodeValueToString service in EFI_PRINT2_PROTOCOL.


  @param  Buffer  The pointer to the output buffer for the produced
                  Null-terminated Unicode string.
  @param  Flags   The bitmask of flags that specify left justification, zero
                  pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width   The maximum number of Unicode characters to place in Buffer,
                  not including the Null-terminator.

  @return  0.


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
  DEBUG ((DEBUG_ERROR, "PrintDxe: The UnicodeValueToString service in EFI_PRINT2_PROTOCOL is no longer supported for security reason.\n"));
  DEBUG ((DEBUG_ERROR, "PrintDxe: Please consider using the UnicodeValueToStringS service in EFI_PRINT2S_PROTOCOL.\n"));
  ASSERT (FALSE);
  return 0;

}

/**
  Implementaion of the AsciiValueToString service in EFI_PRINT2_PROTOCOL.

  @param  Buffer  A pointer to the output buffer for the produced
                  Null-terminated ASCII string.
  @param  Flags   The bitmask of flags that specify left justification, zero
                  pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width   The maximum number of ASCII characters to place in Buffer,
                  not including the Null-terminator.

  @return 0.

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

  DEBUG ((DEBUG_ERROR, "PrintDxe: The AsciiValueToString service in EFI_PRINT2_PROTOCOL is no longer supported for security reason.\n"));
  DEBUG ((DEBUG_ERROR, "PrintDxe: Please consider using the AsciiValueToStringS service in EFI_PRINT2S_PROTOCOL.\n"));
  ASSERT (FALSE);
  return 0;

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

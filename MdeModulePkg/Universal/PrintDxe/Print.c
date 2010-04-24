/** @file
  This driver produces Print2 protocol layered on top of the PrintLib from the MdePkg.

Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
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

EFI_HANDLE  mPrintThunkHandle = NULL;

CONST EFI_PRINT2_PROTOCOL mPrint2Protocol = {
  UnicodeBSPrint,
  UnicodeSPrint,
  UnicodeBSPrintAsciiFormat,
  UnicodeSPrintAsciiFormat,
  UnicodeValueToString,
  AsciiBSPrint,
  AsciiSPrint,
  AsciiBSPrintUnicodeFormat,
  AsciiSPrintUnicodeFormat,
  AsciiValueToString
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
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

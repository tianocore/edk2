/** @file
  Module produces EDK gEfiPrintProtocolGuid for backward compatibility support.

  EDK II retires old EDK Print Protocol and this module produces
  gEfiPrintProtocolGuid based on PrintLib:
  1) If it links against BasePrintLib in MdePkg, it produces gEfiPrintProtocolGuid
     without any prerequisites.
  2) If it links against DxePrintLibPrint2Protocol in MdeModulePkg, it produces
     gEfiPrintProtocolGuid on top of gEfiPrint2ProtocolGuid.

Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include <PiDxe.h>

#include <Protocol/Print.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_HANDLE  mPrintThunkHandle = NULL;

CONST EFI_PRINT_PROTOCOL mPrintProtocol = {
  UnicodeVSPrint,
};


/**
  The user Entry Point for Print thunk module.

  This is the entry point for Print thunk DXE Driver. It installs the Print Protocol on
  top of PrintLib class in MdePkg.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Others            Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitPrintThunk (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mPrintThunkHandle,
                  &gEfiPrintProtocolGuid, &mPrintProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

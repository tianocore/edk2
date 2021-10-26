//# @file
//#    A simple, basic, application showing how the Hello application
//#
//# Copyright (c) 2010 - 2020, Intel Corporation. All rights reserved.<BR>
//#

#include <Library/UefiLib.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print(L"Hello World\n");

  return EFI_SUCCESS;
}


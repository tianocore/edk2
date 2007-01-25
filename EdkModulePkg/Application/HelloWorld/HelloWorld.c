/** @file
  This driver supports platform security service

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

STATIC
VOID
Print (
  IN CONST CHAR16  *Format,
  ...
  )
{
  CHAR16  PrintBuffer[0x100];
  VA_LIST Marker;

  VA_START (Marker, Format);
  UnicodeVSPrint (PrintBuffer, sizeof (PrintBuffer), Format, Marker);
  gST->ConOut->OutputString (gST->ConOut, PrintBuffer);
  return;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  Print ((CHAR16 *)L"UEFI Hello World!\n");
  return EFI_SUCCESS;
}

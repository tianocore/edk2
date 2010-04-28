/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Print.h

Abstract:

  This file defines the Print protocol

--*/

#ifndef _PPRINT_H_
#define _PPRINT_H_

#define EFI_PRINT_PROTOCOL_GUID  \
   { 0xdf2d868e, 0x32fc, 0x4cf0, {0x8e, 0x6b, 0xff, 0xd9, 0x5d, 0x13, 0x43, 0xd0} }


typedef struct _EFI_PRINT_PROTOCOL EFI_PRINT_PROTOCOL;

typedef
UINTN
(EFIAPI *EFI_VSPRINT) (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  );

struct _EFI_PRINT_PROTOCOL {
  EFI_VSPRINT                                   VSPrint;
};


extern EFI_GUID gEfiPrintProtocolGuid;

#endif

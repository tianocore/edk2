/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  UnicodeCollationEng.h

Abstract:

  Head file for Unicode Collation Protocol (English)

Revision History

--*/

#ifndef _UNICODE_COLLATION_ENG_H
#define _UNICODE_COLLATION_ENG_H



//
// Defines
//
#define CHAR_FAT_VALID  0x01

#define ToUpper(a)      (CHAR16) (a <= 0xFF ? mEngUpperMap[a] : a)
#define ToLower(a)      (CHAR16) (a <= 0xFF ? mEngLowerMap[a] : a)

//
// Prototypes
//
INTN
EFIAPI
EngStriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL           *This,
  IN CHAR16                                   *s1,
  IN CHAR16                                   *s2
  )
;

BOOLEAN
EFIAPI
EngMetaiMatch (
  IN EFI_UNICODE_COLLATION_PROTOCOL          *This,
  IN CHAR16                                  *String,
  IN CHAR16                                  *Pattern
  )
;

VOID
EFIAPI
EngStrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL          *This,
  IN OUT CHAR16                              *Str
  )
;

VOID
EFIAPI
EngStrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL          *This,
  IN OUT CHAR16                              *Str
  )
;

VOID
EFIAPI
EngFatToStr (
  IN EFI_UNICODE_COLLATION_PROTOCOL          *This,
  IN UINTN                                   FatSize,
  IN CHAR8                                   *Fat,
  OUT CHAR16                                 *String
  )
;

BOOLEAN
EFIAPI
EngStrToFat (
  IN EFI_UNICODE_COLLATION_PROTOCOL          *This,
  IN CHAR16                                  *String,
  IN UINTN                                   FatSize,
  OUT CHAR8                                  *Fat
  )
;

EFI_STATUS
EFIAPI
InitializeUnicodeCollationEng (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
;

#endif

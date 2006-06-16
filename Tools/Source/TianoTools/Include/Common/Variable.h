/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiVariable.h
  
Abstract:
  
  Header file for EFI Variable Services

--*/

#ifndef _EFI_VARIABLE_H_
#define _EFI_VARIABLE_H_

#define VARIABLE_STORE_SIGNATURE  EFI_SIGNATURE_32 ('$', 'V', 'S', 'S')

#define MAX_VARIABLE_SIZE         1024

#define VARIABLE_DATA             0x55AA

//
// Variable Store Header flags
//
#define VARIABLE_STORE_FORMATTED  0x5a
#define VARIABLE_STORE_HEALTHY    0xfe

//
// Variable Store Status
//
typedef enum {
  EfiRaw,
  EfiValid,
  EfiInvalid,
  EfiUnknown
} VARIABLE_STORE_STATUS;

//
// Variable State flags
//
#define VAR_IN_DELETED_TRANSITION     0xfe  // Variable is in obsolete transistion
#define VAR_DELETED                   0xfd  // Variable is obsolete
#define VAR_ADDED                     0x7f  // Variable has been completely added
#define IS_VARIABLE_STATE(_c, _Mask)  (BOOLEAN) (((~_c) & (~_Mask)) != 0)

#pragma pack(1)

typedef struct {
  UINT32  Signature;
  UINT32  Size;
  UINT8   Format;
  UINT8   State;
  UINT16  Reserved;
  UINT32  Reserved1;
} VARIABLE_STORE_HEADER;

typedef struct {
  UINT16    StartId;
  UINT8     State;
  UINT8     Reserved;
  UINT32    Attributes;
  UINTN     NameSize;
  UINTN     DataSize;
  EFI_GUID  VendorGuid;
} VARIABLE_HEADER;

#pragma pack()

#endif // _EFI_VARIABLE_H_

/** @file
  Platform Configuration Database (PCD) Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Pcd.h

**/

#ifndef __PCD_H__
#define __PCD_H__

extern EFI_GUID gPcdProtocolGuid;

#define PCD_PROTOCOL_GUID \
  { 0x11b34006, 0xd85b, 0x4d0a, { 0xa2, 0x90, 0xd5, 0xa5, 0x71, 0x31, 0xe, 0xf7 } }

typedef UINT8   SKU_ID;

typedef 
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_SKU) (
  IN  UINTN                  SkuId
  );

typedef
UINT8
(EFIAPI *PCD_PROTOCOL_GET8) (
  IN UINTN  TokenNumber
  );

typedef
UINT16
(EFIAPI *PCD_PROTOCOL_GET16) (
  IN UINTN  TokenNumber
  );

typedef
UINT32
(EFIAPI *PCD_PROTOCOL_GET32) (
  IN UINTN  TokenNumber
  );

typedef
UINT64
(EFIAPI *PCD_PROTOCOL_GET64) (
  IN UINTN  TokenNumber
  );

typedef
VOID *
(EFIAPI *PCD_PROTOCOL_GET_POINTER) (
  IN UINTN  TokenNumber
  );

typedef
BOOLEAN
(EFIAPI *PCD_PROTOCOL_GET_BOOLEAN) (
  IN UINTN  TokenNumber
  );

typedef
UINTN
(EFIAPI *PCD_PROTOCOL_GET_SIZE) (
  IN UINTN  TokenNumber
  );

typedef
UINT8
(EFIAPI *PCD_PROTOCOL_GET_EX_8) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN  TokenNumber
  );

typedef
UINT16
(EFIAPI *PCD_PROTOCOL_GET_EX_16) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN  TokenNumber
  );

typedef
UINT32
(EFIAPI *PCD_PROTOCOL_GET_EX_32) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN  TokenNumber
  );

typedef
UINT64
(EFIAPI *PCD_PROTOCOL_GET_EX_64) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
VOID *
(EFIAPI *PCD_PROTOCOL_GET_EX_POINTER) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
BOOLEAN
(EFIAPI *PCD_PROTOCOL_GET_EX_BOOLEAN) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
UINTN
(EFIAPI *PCD_PROTOCOL_GET_EX_SIZE) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET8) (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET16) (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET32) (
  IN UINTN              TokenNumber,
  IN UINT32             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET64) (
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_POINTER) (
  IN UINTN             TokenNumber,
  IN CONST VOID        *Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_BOOLEAN) (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_EX_8) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_EX_16) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_EX_32) (
  IN CONST EFI_GUID     *Guid,
  IN UINTN              TokenNumber,
  IN UINT32             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_EX_64) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_EX_POINTER) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN CONST VOID        *Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_SET_EX_BOOLEAN) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  );
  
/**
  Callback on SET function prototype definition.

  @param[in]  CallBackGuid The PCD token GUID being set.
  @param[in]  CallBackToken The PCD token number being set.
  @param[in]  TokenData A pointer to the token data being set.
  @param[in]  TokenDataSize The size, in bytes, of the data being set.

  @retval VOID

--*/
typedef
VOID
(EFIAPI *PCD_PROTOCOL_CALLBACK) (
  IN  CONST EFI_GUID   *CallBackGuid, OPTIONAL
  IN  UINTN            CallBackToken,
  IN  VOID             *TokenData,
  IN  UINTN            TokenDataSize
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_CALLBACK_ONSET) (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_CANCEL_CALLBACK) (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  );

typedef 
EFI_STATUS
(EFIAPI *PCD_PROTOCOL_GET_NEXT_TOKEN) (
  IN      CONST EFI_GUID      *Guid, OPTIONAL
  IN OUT  UINTN               *TokenNumber
  );

typedef struct {
  PCD_PROTOCOL_SET_SKU              SetSku;

  PCD_PROTOCOL_GET8                 Get8;
  PCD_PROTOCOL_GET16                Get16;
  PCD_PROTOCOL_GET32                Get32;
  PCD_PROTOCOL_GET64                Get64;
  PCD_PROTOCOL_GET_POINTER          GetPtr;
  PCD_PROTOCOL_GET_BOOLEAN          GetBool;
  PCD_PROTOCOL_GET_SIZE             GetSize;

  PCD_PROTOCOL_GET_EX_8             Get8Ex;
  PCD_PROTOCOL_GET_EX_16            Get16Ex;
  PCD_PROTOCOL_GET_EX_32            Get32Ex;
  PCD_PROTOCOL_GET_EX_64            Get64Ex;
  PCD_PROTOCOL_GET_EX_POINTER       GetPtrEx;
  PCD_PROTOCOL_GET_EX_BOOLEAN       GetBoolEx;
  PCD_PROTOCOL_GET_EX_SIZE          GetSizeEx;

  PCD_PROTOCOL_SET8                 Set8;
  PCD_PROTOCOL_SET16                Set16;
  PCD_PROTOCOL_SET32                Set32;
  PCD_PROTOCOL_SET64                Set64;
  PCD_PROTOCOL_SET_POINTER          SetPtr;
  PCD_PROTOCOL_SET_BOOLEAN          SetBool;

  PCD_PROTOCOL_SET_EX_8             Set8Ex;
  PCD_PROTOCOL_SET_EX_16            Set16Ex;
  PCD_PROTOCOL_SET_EX_32            Set32Ex;
  PCD_PROTOCOL_SET_EX_64            Set64Ex;
  PCD_PROTOCOL_SET_EX_POINTER       SetPtrEx;
  PCD_PROTOCOL_SET_EX_BOOLEAN       SetBoolEx;

  PCD_PROTOCOL_CALLBACK_ONSET       CallbackOnSet;
  PCD_PROTOCOL_CANCEL_CALLBACK      CancelCallback;
  PCD_PROTOCOL_GET_NEXT_TOKEN       GetNextToken;
} PCD_PROTOCOL;

#endif

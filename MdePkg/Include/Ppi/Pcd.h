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

extern EFI_GUID gPcdPpiGuid;

#define PCD_PPI_GUID \
  { 0x6e81c58, 0x4ad7, 0x44bc, { 0x83, 0x90, 0xf1, 0x2, 0x65, 0xf7, 0x24, 0x80 } }

#define PCD_INVALID_TOKEN_NUMBER ((UINTN) 0)

typedef 
VOID
(EFIAPI *PCD_PPI_SET_SKU) (
  IN  UINTN          SkuId
  );

typedef
UINT8
(EFIAPI *PCD_PPI_GET8) (
  IN UINTN             TokenNumber
  );

typedef
UINT16
(EFIAPI *PCD_PPI_GET16) (
  IN UINTN             TokenNumber
  );

typedef
UINT32
(EFIAPI *PCD_PPI_GET32) (
  IN UINTN             TokenNumber
  );

typedef
UINT64
(EFIAPI *PCD_PPI_GET64) (
  IN UINTN             TokenNumber
  );

typedef
VOID *
(EFIAPI *PCD_PPI_GET_POINTER) (
  IN UINTN             TokenNumber
  );

typedef
BOOLEAN
(EFIAPI *PCD_PPI_GET_BOOLEAN) (
  IN UINTN             TokenNumber
  );

typedef
UINTN
(EFIAPI *PCD_PPI_GET_SIZE) (
  IN UINTN             TokenNumber
  );

typedef
UINT8
(EFIAPI *PCD_PPI_GET_EX_8) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
UINT16
(EFIAPI *PCD_PPI_GET_EX_16) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN              TokenNumber
  );

typedef
UINT32
(EFIAPI *PCD_PPI_GET_EX_32) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
UINT64
(EFIAPI *PCD_PPI_GET_EX_64) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
VOID *
(EFIAPI *PCD_PPI_GET_EX_POINTER) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
BOOLEAN
(EFIAPI *PCD_PPI_GET_EX_BOOLEAN) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
UINTN
(EFIAPI *PCD_PPI_GET_EX_SIZE) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET8) (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET16) (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET32) (
  IN UINTN             TokenNumber,
  IN UINT32            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET64) (
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_POINTER) (
  IN        UINTN             TokenNumber,
  IN OUT    UINTN             *SizeOfBuffer,
  IN        VOID              *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_BOOLEAN) (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_8) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_16) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_32) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT32            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_64) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_POINTER) (
  IN        CONST EFI_GUID    *Guid,
  IN        UINTN             TokenNumber,
  IN OUT    UINTN             *SizeOfBuffer,
  IN        VOID              *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_BOOLEAN) (
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
(EFIAPI *PCD_PPI_CALLBACK) (
  IN      CONST EFI_GUID   *CallBackGuid, OPTIONAL
  IN      UINTN            CallBackToken,
  IN  OUT VOID             *TokenData,
  IN      UINTN            TokenDataSize
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_CALLBACK_ONSET) (
  IN  UINTN                  TokenNumber,
  IN  CONST EFI_GUID         *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK       CallBackFunction
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_CANCEL_CALLBACK) (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK        CallBackFunction
  );

typedef 
EFI_STATUS
(EFIAPI *PCD_PPI_GET_NEXT_TOKEN) (
  IN CONST EFI_GUID           *Guid, OPTIONAL
  IN OUT  UINTN               *TokenNumber
  );

typedef
EFI_STATUS
(EFIAPI *PCD_PPI_GET_NEXT_TOKENSPACE) (
  IN OUT CONST EFI_GUID         **Guid
  );

typedef struct {
  PCD_PPI_SET_SKU              SetSku;

  PCD_PPI_GET8                 Get8;
  PCD_PPI_GET16                Get16;
  PCD_PPI_GET32                Get32;
  PCD_PPI_GET64                Get64;
  PCD_PPI_GET_POINTER          GetPtr;
  PCD_PPI_GET_BOOLEAN          GetBool;
  PCD_PPI_GET_SIZE             GetSize;

  PCD_PPI_GET_EX_8             Get8Ex;
  PCD_PPI_GET_EX_16            Get16Ex;
  PCD_PPI_GET_EX_32            Get32Ex;
  PCD_PPI_GET_EX_64            Get64Ex;
  PCD_PPI_GET_EX_POINTER       GetPtrEx;
  PCD_PPI_GET_EX_BOOLEAN       GetBoolEx;
  PCD_PPI_GET_EX_SIZE          GetSizeEx;

  PCD_PPI_SET8                 Set8;
  PCD_PPI_SET16                Set16;
  PCD_PPI_SET32                Set32;
  PCD_PPI_SET64                Set64;
  PCD_PPI_SET_POINTER          SetPtr;
  PCD_PPI_SET_BOOLEAN          SetBool;

  PCD_PPI_SET_EX_8             Set8Ex;
  PCD_PPI_SET_EX_16            Set16Ex;
  PCD_PPI_SET_EX_32            Set32Ex;
  PCD_PPI_SET_EX_64            Set64Ex;
  PCD_PPI_SET_EX_POINTER       SetPtrEx;
  PCD_PPI_SET_EX_BOOLEAN       SetBoolEx;

  PCD_PPI_CALLBACK_ONSET       CallbackOnSet;
  PCD_PPI_CANCEL_CALLBACK      CancelCallback;
  PCD_PPI_GET_NEXT_TOKEN       GetNextToken;
  PCD_PPI_GET_NEXT_TOKENSPACE  GetNextTokenSpace;
} PCD_PPI;


#endif

/** @file
Private functions used by PCD PEIM.

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name: Service.h

**/

#ifndef _SERVICE_H
#define _SERVICE_H

//
// Offset of StateByte
//
#define PCD_STATEBYTE_HIIENABLE           0x01
#define PCD_STATEBYTE_SKUENABLE           0x02
#define PCD_STATEBYTE_VPDENABLE           0x04
#define PCD_STATEBYTE_SKUDATAARRAYENABLE  0x08
#define PCD_STATEBYTE_DATUMTYPE           0x70
#define PCD_STATEBYTE_EXTENDEDGUIDPRESENT 0x80

#define PCD_DATUMTYPE_OFFSET 4

//
// The definitions for interpreting DatumType
//
#define PCD_BYTE8   (0x00 << PCD_DATUMTYPE_OFFSET)
#define PCD_BYTE16  (0x01 << PCD_DATUMTYPE_OFFSET)
#define PCD_BYTE32  (0x02 << PCD_DATUMTYPE_OFFSET)
#define PCD_BYTE64  (0x03 << PCD_DATUMTYPE_OFFSET)
#define PCD_POINTER (0x04 << PCD_DATUMTYPE_OFFSET)
#define PCD_BOOLEAN (0x05 << PCD_DATUMTYPE_OFFSET)

extern GUID gEfiPcdImageHobGuid;

/* Internal Function definitions */

VOID
PeiGetPcdEntryWorker (
  IN UINTN Token,
  IN CONST GUID       *Guid,  OPTIONAL
  IN PCD_DATA_TYPE    Type,
  OUT VOID            *Data
  );

EFI_STATUS
PeiSetPcdEntryWorker (
  IN UINTN Token,
  IN CONST GUID       *Guid,  OPTIONAL
  IN PCD_DATA_TYPE    Type,
  IN VOID       *Data
  );

UINTN
PeiGetPcdEntrySizeWorker (
  IN UINTN Token,
  IN CONST GUID       *Guid  OPTIONAL
  );

EFI_STATUS
PeiRegisterCallBackWorker (
  IN  UINTN        TokenNumber,
  IN  CONST GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK   CallBackFunction,
  IN  BOOLEAN                 Register
);

EFI_STATUS
PeiSetSku (
  UINTN Id
);

EFI_STATUS
PeiGetNextTokenWorker (
  IN OUT UINTN *Token,
  IN CONST GUID           *Guid     OPTIONAL
  );

UINT8 *
LocatePcdImage (
  VOID
);

VOID
BuildPcdDatabase (
  UINT8 *PcdImageOnFlash
  )
;


extern EFI_GUID gPcdImageFileGuid;

//
// PPI Interface Implementation Declaration.
//
EFI_STATUS
EFIAPI
PeiPcdSetSku (
  IN  UINTN                  SkuId
  )
;


UINT8
EFIAPI
PeiPcdGet8 (
  IN UINTN  TokenNumber
  )
;


UINT16
EFIAPI
PeiPcdGet16 (
  IN UINTN  TokenNumber
  )
;


UINT32
EFIAPI
PeiPcdGet32 (
  IN UINTN  TokenNumber
  )
;


UINT64
EFIAPI
PeiPcdGet64 (
  IN UINTN  TokenNumber
  )
;


VOID *
EFIAPI
PeiPcdGetPtr (
  IN UINTN  TokenNumber
  )
;


BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN UINTN  TokenNumber
  )
;


UINTN
EFIAPI
PeiPcdGetSize (
  IN UINTN  TokenNumber
  )
;


UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;

UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN UINTN  TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN UINTN  TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN UINTN  TokenNumber,
  IN UINT32            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN UINTN  TokenNumber,
  IN UINT64            Value
  )
;

EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN UINTN  TokenNumber,
  IN CONST VOID        *Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN UINTN  TokenNumber,
  IN BOOLEAN           Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT8             Value
  )
;

EFI_STATUS
EFIAPI
PeiPcdSet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT32            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN CONST VOID        *Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN BOOLEAN           Value
  )
;



EFI_STATUS
EFIAPI
PcdRegisterCallBackOnSet (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT  UINTN    *TokenNumber
  )
;
#endif

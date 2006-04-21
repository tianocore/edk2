/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PcdEmulator.c

Abstract:
  Platform Configuration Database (PCD) Protocol

--*/

#include <PcdEmulator.h>

UINTN                    mSkuId = 0;

STATIC UINTN 
GetPcdDataEntryCount (
	VOID
) {
	return gEmulatedPcdDatabaseEx->Count;
}

EFI_STATUS
EFIAPI
PcdEmulatorSetSku (
  IN  UINTN             SkuId
  )
{
  mSkuId = SkuId;
  return EFI_SUCCESS;
}

UINT8
EFIAPI
PcdEmulatorGet8 (
  IN  UINTN  TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);
  ASSERT (Pcd->DatumSize == 1);

  return (UINT8)Pcd->Datum;
}

UINT16
EFIAPI
PcdEmulatorGet16 (
  IN  UINTN  TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);
  ASSERT (Pcd->DatumSize == 2);

  return (UINT16)Pcd->Datum;
}

UINT32
EFIAPI
PcdEmulatorGet32 (
  IN  UINTN  TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);
  ASSERT (Pcd->DatumSize == 4);

  return (UINT32)Pcd->Datum;
}

UINT64
EFIAPI
PcdEmulatorGet64 (
  IN  UINTN  TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);
  ASSERT (Pcd->DatumSize == sizeof (UINT64));

  return (UINT64)Pcd->Datum;
}

VOID *
EFIAPI
PcdEmulatorGetPtr (
  IN  UINTN  TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);

  return (VOID *)(UINTN)Pcd->ExtendedData;
}

BOOLEAN
EFIAPI
PcdEmulatorGetBoolean (
  IN  UINTN  TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);
  ASSERT (Pcd->DatumSize == 1);

  return (BOOLEAN)Pcd->Datum;
}

UINTN
EFIAPI
PcdEmulatorGetSize (
  IN  UINTN  TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);
  return Pcd->DatumSize;
}

UINT8
EFIAPI
PcdEmulatorGet8Ex (
  IN  CONST EFI_GUID      *PcdDataBaseName,
  IN  UINTN               TokenNumber
  )
{
  ASSERT (FALSE);
  return 0;
}

UINT16
EFIAPI
PcdEmulatorGet16Ex (
  IN  CONST EFI_GUID      *PcdDataBaseName,
  IN  UINTN               TokenNumber
  )
{
  ASSERT (FALSE);
  return 0;
}

UINT32
EFIAPI
PcdEmulatorGet32Ex (
  IN  CONST EFI_GUID      *PcdDataBaseName,
  IN  UINTN               TokenNumber
  )
{
  ASSERT (FALSE);
  return 0;
}

UINT64
EFIAPI
PcdEmulatorGet64Ex (
  IN  CONST EFI_GUID      *PcdDataBaseName,
  IN  UINTN               TokenNumber
  )
{
  ASSERT (FALSE);
  return 0;
}

VOID *
EFIAPI
PcdEmulatorGetPtrEx (
  IN  CONST EFI_GUID    *PcdDataBaseName,
  IN  UINTN             TokenNumber
  )
{
  ASSERT (FALSE);
  return 0;
}

BOOLEAN
EFIAPI
PcdEmulatorGetBooleanEx (
  IN  CONST EFI_GUID    *PcdDataBaseName,
  IN  UINTN             TokenNumber
  )
{
  ASSERT (FALSE);
  return 0;
}

UINTN
EFIAPI
PcdEmulatorGetSizeEx (
  IN  CONST EFI_GUID    *PcdDataBaseName,
  IN  UINTN             TokenNumber
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);
  return Pcd->DatumSize;
}


EFI_STATUS
EFIAPI
PcdEmulatorSet8 (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{  

  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);

  ASSERT (Pcd->DatumSize == sizeof (UINT8));

  Pcd->Datum = Value;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSet16 (
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSet32 (
  IN UINTN              TokenNumber,
  IN UINT32             Value
  )
{  

  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);

  ASSERT (Pcd->DatumSize == sizeof (UINT32));

  Pcd->Datum = Value;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSet64 (
  IN UINTN             TokenNumber,
  IN UINT64            Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSetPtr (
  IN UINTN             TokenNumber,
  IN CONST VOID        *Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSetBoolean (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSet8Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSet16Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSet32Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT32             Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSet64Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSetPtrEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN CONST VOID        *Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorSetBooleanEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  )
{  

  ASSERT (FALSE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorCallBackOnSet (
  IN  UINTN               TokenNumber,
  IN  CONST EFI_GUID      *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK  CallBackFunction
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);

  if (Pcd->CallBackListSize == Pcd->CallBackEntries) {
    return EFI_OUT_OF_RESOURCES;
  }

  Pcd->CallBackList[Pcd->CallBackEntries++] = CallBackFunction;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PcdEmulatorUnregisterCallBackOnSet (
  IN  UINTN                         TokenNumber,
  IN  CONST EFI_GUID                *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK         CallBackfunction
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;
  UINT32                 Index;

  Pcd = GetPcdEntry (TokenNumber);
  ASSERT (Pcd != NULL);

  for (Index = 0; Index < Pcd->CallBackListSize; Index++) {
    if (Pcd->CallBackList[Index] == CallBackfunction) {
      Pcd->CallBackList[Index] = NULL;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
PcdEmulatorGetNextToken (
  IN  CONST EFI_GUID      *Guid, OPTIONAL
  IN  UINTN    *Token
  )
{
  EMULATED_PCD_ENTRY_EX  *Pcd;
  EMULATED_PCD_ENTRY_EX  *LastPcdEntry;

  if (*Token == PCD_INVALID_TOKEN) {
    //
    // BugBug: Due to variable size array, ensure we convert this to a reasonable database
    //         that can accomodate array references for simplicity's sake
    *Token = gEmulatedPcdEntryEx[0].Token;
    return EFI_SUCCESS;
  }

  Pcd = GetPcdEntry (*Token);
  if (Pcd == NULL) {
    return EFI_NOT_FOUND;
  }

  LastPcdEntry = gEmulatedPcdEntryEx + GetPcdDataEntryCount ();
  if (++Pcd >= LastPcdEntry) {
    return EFI_NOT_FOUND;
  }
  
  *Token = Pcd->Token;
  return EFI_SUCCESS;
}

PCD_PROTOCOL mPcdProtocolInstance = {
  PcdEmulatorSetSku,

  PcdEmulatorGet8,
  PcdEmulatorGet16,          
  PcdEmulatorGet32,          
  PcdEmulatorGet64,          
  PcdEmulatorGetPtr,         
  PcdEmulatorGetBoolean,     
  PcdEmulatorGetSize,

  PcdEmulatorGet8Ex,
  PcdEmulatorGet16Ex,          
  PcdEmulatorGet32Ex,          
  PcdEmulatorGet64Ex,          
  PcdEmulatorGetPtrEx,         
  PcdEmulatorGetBooleanEx,     
  PcdEmulatorGetSizeEx,
  
  PcdEmulatorSet8,
  PcdEmulatorSet16,          
  PcdEmulatorSet32,          
  PcdEmulatorSet64,          
  PcdEmulatorSetPtr,         
  PcdEmulatorSetBoolean,     

  PcdEmulatorSet8Ex,
  PcdEmulatorSet16Ex,          
  PcdEmulatorSet32Ex,          
  PcdEmulatorSet64Ex,          
  PcdEmulatorSetPtrEx,         
  PcdEmulatorSetBooleanEx,     

  PcdEmulatorCallBackOnSet,
  PcdEmulatorUnregisterCallBackOnSet,
  PcdEmulatorGetNextToken
};


EFI_STATUS
EFIAPI
PcdEmulatorEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;
  EFI_HOB_GUID_TYPE       *GuidHob;

  GuidHob = GetFirstGuidHob (&gPcdHobGuid);
  gEmulatedPcdDatabaseEx = (EMULATED_PCD_DATABASE_EX *) GET_GUID_HOB_DATA(GuidHob);
  ASSERT (gEmulatedPcdDatabaseEx != NULL);
  gEmulatedPcdEntryEx = gEmulatedPcdDatabaseEx->Entry;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gPcdProtocolGuid, &mPcdProtocolInstance,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}


EMULATED_PCD_ENTRY_EX *
GetPcdEntry (
  IN      UINTN    TokenNumber
  )
{
  UINTN Index;
  UINTN Count;

  Count = GetPcdDataEntryCount ();
  for (Index = 0; Index < Count; Index++) {
    if (gEmulatedPcdEntryEx[Index].Token == TokenNumber) {
      return &gEmulatedPcdEntryEx[Index];
    }
  }
  return NULL;
}

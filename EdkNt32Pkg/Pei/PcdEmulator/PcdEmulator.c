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
  Platform Configuration Database (PCD) Service PEIM

--*/

#include <PcdEmulator.h>

//
// BugBug: PEI early phase does not support global variable!!!
//         This is only a temperary solution.
//

UINTN                    mSkuId = 0;


STATIC EMULATED_PCD_DATABASE_EX *
GetPcdDataBaseEx (
  VOID
) {
  EFI_HOB_GUID_TYPE         *GuidHob;
  EMULATED_PCD_DATABASE_EX  *EmulatedPcdDatabaseEx;

  GuidHob = GetFirstGuidHob (&gPcdHobGuid);
  EmulatedPcdDatabaseEx = (EMULATED_PCD_DATABASE_EX *) GET_GUID_HOB_DATA(GuidHob);

  return EmulatedPcdDatabaseEx;
}

STATIC UINTN 
GetPcdDataBaseExEntryCount (
	EMULATED_PCD_DATABASE_EX * Database
) {
	return Database->Count;
}

STATIC UINTN 
GetPcdDataBaseExSize (
	EMULATED_PCD_DATABASE_EX * Database
) {
	UINTN Size;
	
	Size = sizeof (Database->Count)
			+ (sizeof (Database->Entry[0]) * Database->Count);
			
	return Size;
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
  IN  PCD_PPI_CALLBACK    CallBackFunction
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
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK        CallBackfunction
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
  IN  UINTN               *Token
  )
{
  EMULATED_PCD_ENTRY_EX    *Pcd;
  EMULATED_PCD_ENTRY_EX    *LastPcdEntry;
  EMULATED_PCD_DATABASE_EX *PcdDatabase;
  EMULATED_PCD_ENTRY_EX    *PcdEntry;

  PcdDatabase = GetPcdDataBaseEx ();  
  PcdEntry    = PcdDatabase->Entry;

  if (*Token == PCD_INVALID_TOKEN) {
    //
    // BugBug: Due to variable size array, ensure we convert this to a reasonable database
    //         that can accomodate array references for simplicity's sake
    *Token = PcdEntry[0].Token;
    return EFI_SUCCESS;
  }

  Pcd = GetPcdEntry (*Token);
  if (Pcd == NULL) {
    return EFI_NOT_FOUND;
  }

  LastPcdEntry = PcdEntry + GetPcdDataBaseExEntryCount (PcdDatabase);
  if (++Pcd >= LastPcdEntry) {
    return EFI_NOT_FOUND;
  }
  
  *Token = Pcd->Token;
  return EFI_SUCCESS;
}

PCD_PPI mPcdPpiInstance = {
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

STATIC EFI_PEI_PPI_DESCRIPTOR     mPpiPCD = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPcdPpiGuid,
  &mPcdPpiInstance
};

EFI_STATUS
EFIAPI
PeimPcdEmulatorEntry (
  IN EFI_FFS_FILE_HEADER      *FfsHeader,
  IN EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  UINTN                       Count;
  UINTN                       Calculation;
  UINT8                       *AllocatedBuffer;
  EMULATED_PCD_DATABASE_EX    *EmulatedPcdDatabaseEx;
  EMULATED_PCD_ENTRY_EX       *EmulatedPcdEntryEx;

  //
  // BugBug: Normally, we would read an FFS file for this data
  // We need to remember, that when we read the FFS file, items such as the VariableName will not be encoded as a pointer
  // but as an array of content.  In this emulation, our init is encoding this data as a pointer.
  // In the FFS version, we will depend on the proper Entry Count in the FFS data since the structures will
  // now be variable length.
  //
  //

  //
  // We should now read from the FFS file into the cache - for now, we fake this.
  //
  Count = GetPcdDataBaseSize () / sizeof (EMULATED_PCD_ENTRY);

  //
  // Let's now determine how big of a buffer we need for our database
  // For the FFS version, we need to calculate/consider the VariableName/ExtendedData size!!!
  //
  Calculation = sizeof (UINTN) + (Count * sizeof (EMULATED_PCD_ENTRY_EX));

  EmulatedPcdDatabaseEx = (EMULATED_PCD_DATABASE_EX *) BuildGuidHob (&gPcdHobGuid, Calculation);

  EmulatedPcdDatabaseEx->Count = Count;
  EmulatedPcdEntryEx    = EmulatedPcdDatabaseEx->Entry;

  AllocatedBuffer = AllocatePool (Count * sizeof (PCD_PPI_CALLBACK) * MAX_PCD_CALLBACK);
  ASSERT (AllocatedBuffer != NULL);  

  for (Index = 0; Index < Count; Index++) {
    //
    // Copy from source to our own allocated buffer - normally an FFS read
    //
    (*PeiServices)->CopyMem (
                      (VOID *) (EmulatedPcdEntryEx + Index),
                      (VOID *) (gEmulatedPcdEntry + Index),
                      sizeof (EMULATED_PCD_ENTRY)
                      );

    //
    // All the CallBackList worker functions refer to this CallBackList as CallBackList[CallbackEntry]
    // so we seed the same buffer address here.
    //
    EmulatedPcdEntryEx[Index].CallBackList = (PCD_PPI_CALLBACK *)AllocatedBuffer;
    AllocatedBuffer+= (sizeof (PCD_PPI_CALLBACK) * MAX_PCD_CALLBACK);
    EmulatedPcdEntryEx[Index].CallBackEntries  = 0;
    EmulatedPcdEntryEx[Index].CallBackListSize = MAX_PCD_CALLBACK;
  }

  //
  // Install PCD service PPI
  //
  Status = PeiServicesInstallPpi (&mPpiPCD);

  ASSERT_EFI_ERROR (Status);
  return Status;
}


EMULATED_PCD_ENTRY_EX *
GetPcdEntry (
  IN      UINTN    TokenNumber
  )
{
  UINTN                     Index;
  UINTN                     Count;
  EMULATED_PCD_DATABASE_EX  *EmulatedPcdDatabaseEx;
  EMULATED_PCD_ENTRY_EX     *EmulatedPcdEntryEx;

  CpuBreakpoint (); 

  EmulatedPcdDatabaseEx = GetPcdDataBaseEx ();  
  //
  // BugBug: This Count will change when we flip over to FFS version
  //
  Count = EmulatedPcdDatabaseEx->Count;
  EmulatedPcdEntryEx = EmulatedPcdDatabaseEx->Entry;
  for (Index = 0; Index < Count; Index++) {
    if (EmulatedPcdEntryEx[Index].Token == TokenNumber) {
      return &EmulatedPcdEntryEx[Index];
    }
  }
  return NULL;
}



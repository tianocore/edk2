/** @file
  PCD DXE driver manage all PCD entry initialized in PEI phase and DXE phase, and
  produce the implementation of native PCD protocol and EFI_PCD_PROTOCOL defined in
  PI 1.2 Vol3.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Service.h"

//
// Just pre-allocate a memory buffer that is big enough to
// host all distinct TokenSpace guid in both
// PEI ExMap and DXE ExMap.
//
EFI_GUID *TmpTokenSpaceBuffer[PEI_EXMAPPING_TABLE_SIZE + DXE_EXMAPPING_TABLE_SIZE] = { 0 };

///
/// PCD database lock.
///
EFI_LOCK mPcdDatabaseLock = EFI_INITIALIZE_LOCK_VARIABLE(TPL_NOTIFY);

///
/// PCD_PROTOCOL the EDKII native implementation which support dynamic 
/// type and dynamicEx type PCDs.
///
PCD_PROTOCOL mPcdInstance = {
  DxePcdSetSku,

  DxePcdGet8,
  DxePcdGet16,
  DxePcdGet32,
  DxePcdGet64,
  DxePcdGetPtr,
  DxePcdGetBool,
  DxePcdGetSize,

  DxePcdGet8Ex,
  DxePcdGet16Ex,
  DxePcdGet32Ex,
  DxePcdGet64Ex,
  DxePcdGetPtrEx,
  DxePcdGetBoolEx,
  DxePcdGetSizeEx,

  DxePcdSet8,
  DxePcdSet16,
  DxePcdSet32,
  DxePcdSet64,
  DxePcdSetPtr,
  DxePcdSetBool,

  DxePcdSet8Ex,
  DxePcdSet16Ex,
  DxePcdSet32Ex,
  DxePcdSet64Ex,
  DxePcdSetPtrEx,
  DxePcdSetBoolEx,

  DxeRegisterCallBackOnSet,
  DxeUnRegisterCallBackOnSet,
  DxePcdGetNextToken,
  DxePcdGetNextTokenSpace
};

///
/// EFI_PCD_PROTOCOL is defined in PI 1.2 Vol 3 which only support dynamicEx type
/// PCD.
///
EFI_PCD_PROTOCOL mEfiPcdInstance = {
  DxePcdSetSku,
  DxePcdGet8Ex,
  DxePcdGet16Ex,
  DxePcdGet32Ex,
  DxePcdGet64Ex,
  DxePcdGetPtrEx,
  DxePcdGetBoolEx,
  DxePcdGetSizeEx,
  DxePcdSet8Ex,
  DxePcdSet16Ex,
  DxePcdSet32Ex,
  DxePcdSet64Ex,
  DxePcdSetPtrEx,
  DxePcdSetBoolEx,
  (EFI_PCD_PROTOCOL_CALLBACK_ON_SET) DxeRegisterCallBackOnSet,
  (EFI_PCD_PROTOCOL_CANCEL_CALLBACK) DxeUnRegisterCallBackOnSet,
  DxePcdGetNextToken,
  DxePcdGetNextTokenSpace
};

EFI_HANDLE mPcdHandle = NULL;

/**
  Main entry for PCD DXE driver.
  
  This routine initialize the PCD database and install PCD_PROTOCOL.
  
  @param ImageHandle     Image handle for PCD DXE driver.
  @param SystemTable     Pointer to SystemTable.

  @return Status of gBS->InstallProtocolInterface()

**/
EFI_STATUS
EFIAPI
PcdDxeInit (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS Status;
  
  //
  // Make sure the Pcd Protocol is not already installed in the system
  //

  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gPcdProtocolGuid);

  BuildPcdDxeDataBase ();

  //
  // Install PCD_PROTOCOL to handle dynamic type PCD
  // Install EFI_PCD_PROTOCOL to handle dynamicEx type PCD
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mPcdHandle,
                  &gPcdProtocolGuid,     &mPcdInstance,
                  &gEfiPcdProtocolGuid,  &mEfiPcdInstance,
                  NULL
                  );
                 
  ASSERT_EFI_ERROR (Status);

  return Status;

}

/**
  Sets the SKU value for subsequent calls to set or get PCD token values.

  SetSku() sets the SKU Id to be used for subsequent calls to set or get PCD values. 
  SetSku() is normally called only once by the system.

  For each item (token), the database can hold a single value that applies to all SKUs, 
  or multiple values, where each value is associated with a specific SKU Id. Items with multiple, 
  SKU-specific values are called SKU enabled. 
  
  The SKU Id of zero is reserved as a default. The valid SkuId range is 1 to 255.  
  For tokens that are not SKU enabled, the system ignores any set SKU Id and works with the 
  single value for that token. For SKU-enabled tokens, the system will use the SKU Id set by the 
  last call to SetSku(). If no SKU Id is set or the currently set SKU Id isn't valid for the specified token, 
  the system uses the default SKU Id. If the system attempts to use the default SKU Id and no value has been 
  set for that Id, the results are unpredictable.

  @param[in]  SkuId The SKU value that will be used when the PCD service will retrieve and 
              set values associated with a PCD token.

**/
VOID
EFIAPI
DxePcdSetSku (
  IN  UINTN         SkuId
  )
{
  mPcdDatabase->PeiDb.Init.SystemSkuId = (SKU_ID) SkuId;
  
  return;
}

/**
  Retrieves an 8-bit value for a given PCD token.

  Retrieves the current byte-sized value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param[in]  TokenNumber The PCD token number. 

  @return The UINT8 value.
  
**/
UINT8
EFIAPI
DxePcdGet8 (
  IN UINTN                    TokenNumber
  )
{
  return *((UINT8 *) GetWorker (TokenNumber, sizeof (UINT8)));
}

/**
  Retrieves an 16-bit value for a given PCD token.

  Retrieves the current 16-bits value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param[in]  TokenNumber The PCD token number. 

  @return The UINT16 value.
  
**/
UINT16
EFIAPI
DxePcdGet16 (
  IN UINTN                    TokenNumber
  )
{
  return ReadUnaligned16 (GetWorker (TokenNumber, sizeof (UINT16)));
}

/**
  Retrieves an 32-bit value for a given PCD token.

  Retrieves the current 32-bits value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param[in]  TokenNumber The PCD token number. 

  @return The UINT32 value.
  
**/
UINT32
EFIAPI
DxePcdGet32 (
  IN UINTN                    TokenNumber
  )
{
  return ReadUnaligned32 (GetWorker (TokenNumber, sizeof (UINT32)));
}

/**
  Retrieves an 64-bit value for a given PCD token.

  Retrieves the current 64-bits value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param[in]  TokenNumber The PCD token number. 

  @return The UINT64 value.
  
**/
UINT64
EFIAPI
DxePcdGet64 (
  IN UINTN                     TokenNumber
  )
{
  return ReadUnaligned64(GetWorker (TokenNumber, sizeof (UINT64)));
}

/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param[in]  TokenNumber The PCD token number. 

  @return The pointer to the buffer to be retrived.
  
**/
VOID *
EFIAPI
DxePcdGetPtr (
  IN UINTN                     TokenNumber
  )
{
  return GetWorker (TokenNumber, 0);
}

/**
  Retrieves a Boolean value for a given PCD token.

  Retrieves the current boolean value for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param[in]  TokenNumber The PCD token number. 

  @return The Boolean value.
  
**/
BOOLEAN
EFIAPI
DxePcdGetBool (
  IN UINTN                     TokenNumber
  )
{
  return *((BOOLEAN *) GetWorker (TokenNumber, sizeof (BOOLEAN)));
}

/**
  Retrieves the size of the value for a given PCD token.

  Retrieves the current size of a particular PCD token.  
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  TokenNumber The PCD token number. 

  @return The size of the value for the PCD token.
  
**/
UINTN
EFIAPI
DxePcdGetSize (
  IN UINTN                     TokenNumber
  )
{
  UINTN   Size;
  UINT32  *LocalTokenNumberTable;
  BOOLEAN IsPeiDb;
  UINTN   MaxSize;
  UINTN   TmpTokenNumber;
  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  //
  // Backup the TokenNumber passed in as GetPtrTypeSize need the original TokenNumber
  // 
  TmpTokenNumber = TokenNumber;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  ASSERT (TokenNumber + 1 < PCD_TOTAL_TOKEN_NUMBER + 1);

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  IsPeiDb = (BOOLEAN) (TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1);
  
  TokenNumber = IsPeiDb ? TokenNumber : 
                          (TokenNumber - PEI_LOCAL_TOKEN_NUMBER);

  LocalTokenNumberTable = IsPeiDb ? mPcdDatabase->PeiDb.Init.LocalTokenNumberTable 
                                  : mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;

  Size = (LocalTokenNumberTable[TokenNumber] & PCD_DATUM_TYPE_ALL_SET) >> PCD_DATUM_TYPE_SHIFT;

  if (Size == 0) {
    //
    // For pointer type, we need to scan the SIZE_TABLE to get the current size.
    //
    return GetPtrTypeSize (TmpTokenNumber, &MaxSize);
  } else {
    return Size;
  }

}

/**
  Retrieves an 8-bit value for a given PCD token.

  Retrieves the 8-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid          The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number. 

  @return The size 8-bit value for the PCD token.
  
**/
UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  return *((UINT8 *) ExGetWorker (Guid, ExTokenNumber, sizeof(UINT8)));
}

/**
  Retrieves an 16-bit value for a given PCD token.

  Retrieves the 16-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number. 

  @return The size 16-bit value for the PCD token.
  
**/
UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                ExTokenNumber
  )
{
  return ReadUnaligned16 (ExGetWorker (Guid, ExTokenNumber, sizeof(UINT16)));
}

/**
  Retrieves an 32-bit value for a given PCD token.

  Retrieves the 32-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number. 

  @return The size 32-bit value for the PCD token.
  
**/
UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return ReadUnaligned32 (ExGetWorker (Guid, ExTokenNumber, sizeof(UINT32)));
}

/**
  Retrieves an 64-bit value for a given PCD token.

  Retrieves the 64-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number. 

  @return The size 64-bit value for the PCD token.
  
**/
UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return ReadUnaligned64 (ExGetWorker (Guid, ExTokenNumber, sizeof(UINT64)));
}

/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number. 

  @return The pointer to the buffer to be retrived.
  
**/
VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return  ExGetWorker (Guid, ExTokenNumber, 0);
}

/**
  Retrieves an Boolean value for a given PCD token.

  Retrieves the Boolean value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number. 

  @return The size Boolean value for the PCD token.
  
**/
BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return *((BOOLEAN *) ExGetWorker (Guid, ExTokenNumber, sizeof(BOOLEAN)));
}

/**
  Retrieves the size of the value for a given PCD token.

  Retrieves the current size of a particular PCD token.  
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number. 

  @return The size of the value for the PCD token.
  
**/
UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return DxePcdGetSize(GetExPcdTokenNumber (Guid, (UINT32) ExTokenNumber));
}

/**
  Sets an 8-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet8 (
  IN UINTN              TokenNumber,
  IN UINT8              Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 16-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet16 (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 32-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet32 (
  IN UINTN              TokenNumber,
  IN UINT32             Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 64-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet64 (
  IN UINTN              TokenNumber,
  IN UINT64             Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets a value of a specified size for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number. 
  @param[in, out] SizeOfBuffer A pointer to the length of the value being set for the PCD token.  
                              On input, if the SizeOfValue is greater than the maximum size supported 
                              for this TokenNumber then the output value of SizeOfValue will reflect 
                              the maximum size supported for this TokenNumber.
  @param[in]  Buffer The buffer to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSetPtr (
  IN          UINTN              TokenNumber,
  IN OUT      UINTN              *SizeOfBuffer,
  IN          VOID               *Buffer
  )
{
  return SetWorker (TokenNumber, Buffer, SizeOfBuffer, TRUE);
}

/**
  Sets an Boolean value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSetBool (
  IN UINTN              TokenNumber,
  IN BOOLEAN            Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 8-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet8Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINT8                  Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets an 16-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet16Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
  IN UINT16            Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets an 32-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet32Ex (
  IN CONST EFI_GUID     *Guid,
  IN UINTN              ExTokenNumber,
  IN UINT32             Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets an 64-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSet64Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
  IN UINT64            Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets a value of a specified size for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number. 
  @param[in, out] SizeOfBuffer A pointer to the length of the value being set for the PCD token.  
                              On input, if the SizeOfValue is greater than the maximum size supported 
                              for this TokenNumber then the output value of SizeOfValue will reflect 
                              the maximum size supported for this TokenNumber.
  @param[in]  Buffer The buffer to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSetPtrEx (
  IN            CONST EFI_GUID         *Guid,
  IN            UINTN                  ExTokenNumber,
  IN OUT        UINTN                  *SizeOfBuffer,
  IN            VOID                   *Buffer
  )
{
  return  ExSetWorker(ExTokenNumber, Guid, Buffer, SizeOfBuffer, TRUE);
}

/**
  Sets an Boolean value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number. 
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
  IN BOOLEAN           Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Specifies a function to be called anytime the value of a designated token is changed.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  TokenNumber The PCD token number. 
  @param[in]  CallBackFunction The function prototype called when the value associated with the CallBackToken is set.  

  @retval EFI_SUCCESS  The PCD service has successfully established a call event 
                        for the CallBackToken requested.
  @retval EFI_NOT_FOUND The PCD service could not find the referenced token number.

**/
EFI_STATUS
EFIAPI
DxeRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  EFI_STATUS Status;
  
  if (CallBackFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);

  Status = DxeRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction);

  EfiReleaseLock (&mPcdDatabaseLock);
  
  return Status;
}

/**
  Cancels a previously set callback function for a particular PCD token number.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  TokenNumber The PCD token number. 
  @param[in]  CallBackFunction The function prototype called when the value associated with the CallBackToken is set.  

  @retval EFI_SUCCESS  The PCD service has successfully established a call event 
                        for the CallBackToken requested.
  @retval EFI_NOT_FOUND The PCD service could not find the referenced token number.

**/
EFI_STATUS
EFIAPI
DxeUnRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  EFI_STATUS Status;
  
  if (CallBackFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);
  
  Status = DxeUnRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction);

  EfiReleaseLock (&mPcdDatabaseLock);
  
  return Status;
}

/**
  Retrieves the next valid token number in a given namespace.  
  
  This is useful since the PCD infrastructure contains a sparse list of token numbers, 
  and one cannot a priori know what token numbers are valid in the database. 
  
  If TokenNumber is 0 and Guid is not NULL, then the first token from the token space specified by Guid is returned.  
  If TokenNumber is not 0 and Guid is not NULL, then the next token in the token space specified by Guid is returned.  
  If TokenNumber is 0 and Guid is NULL, then the first token in the default token space is returned.  
  If TokenNumber is not 0 and Guid is NULL, then the next token in the default token space is returned.  
  The token numbers in the default token space may not be related to token numbers in token spaces that are named by Guid.  
  If the next token number can be retrieved, then it is returned in TokenNumber, and EFI_SUCCESS is returned.  
  If TokenNumber represents the last token number in the token space specified by Guid, then EFI_NOT_FOUND is returned.  
  If TokenNumber is not present in the token space specified by Guid, then EFI_NOT_FOUND is returned.


  @param[in]      Guid    The 128-bit unique value that designates the namespace from which to retrieve the next token. 
                          This is an optional parameter that may be NULL.  If this parameter is NULL, then a request is 
                          being made to retrieve tokens from the default token space.
  @param[in, out] TokenNumber 
                          A pointer to the PCD token number to use to find the subsequent token number.  

  @retval EFI_SUCCESS   The PCD service retrieved the next valid token number. Or the input token number 
                        is already the last valid token number in the PCD database. 
                        In the later case, *TokenNumber is updated with the value of 0.
  @retval EFI_NOT_FOUND If this input token number and token namespace does not exist on the platform.

**/
EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID         *Guid, OPTIONAL
  IN OUT   UINTN            *TokenNumber
  )
{
  EFI_STATUS          Status;
  BOOLEAN             PeiExMapTableEmpty;
  BOOLEAN             DxeExMapTableEmpty;

  Status = EFI_NOT_FOUND;
  PeiExMapTableEmpty = PEI_EXMAP_TABLE_EMPTY;
  DxeExMapTableEmpty = DXE_EXMAP_TABLE_EMPTY;

  //
  // Scan the local token space
  //
  if (Guid == NULL) {
    // EBC compiler is very choosy. It may report warning about comparison
    // between UINTN and 0 . So we add 1 in each size of the 
    // comparison.
    if (((*TokenNumber + 1 > PEI_NEX_TOKEN_NUMBER + 1) && (*TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1)) ||
        ((*TokenNumber + 1 > (PEI_LOCAL_TOKEN_NUMBER + DXE_NEX_TOKEN_NUMBER + 1)))) {
        return EFI_NOT_FOUND;
    }
    
    (*TokenNumber)++;
    if ((*TokenNumber + 1 > PEI_NEX_TOKEN_NUMBER + 1) &&
        (*TokenNumber <= PEI_LOCAL_TOKEN_NUMBER)) {
      //
      // The first Non-Ex type Token Number for DXE PCD 
      // database is PEI_LOCAL_TOKEN_NUMBER
      //
      *TokenNumber = PEI_LOCAL_TOKEN_NUMBER;
    } else if (*TokenNumber + 1 > DXE_NEX_TOKEN_NUMBER + PEI_LOCAL_TOKEN_NUMBER + 1) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
    }
    return EFI_SUCCESS;
  }

  if (PeiExMapTableEmpty && DxeExMapTableEmpty) {
    *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
    return EFI_NOT_FOUND;
  }

  if (!PeiExMapTableEmpty) {
    Status = ExGetNextTokeNumber (
                        Guid,
                        TokenNumber,
                        mPcdDatabase->PeiDb.Init.GuidTable,
                        sizeof(mPcdDatabase->PeiDb.Init.GuidTable),
                        mPcdDatabase->PeiDb.Init.ExMapTable,
                        sizeof(mPcdDatabase->PeiDb.Init.ExMapTable)
                        );
  }

  if (Status == EFI_SUCCESS) {
    return Status;
  }

  if (!DxeExMapTableEmpty) {
    Status = ExGetNextTokeNumber (
                        Guid,
                        TokenNumber,
                        mPcdDatabase->DxeDb.Init.GuidTable,
                        sizeof(mPcdDatabase->DxeDb.Init.GuidTable),
                        mPcdDatabase->DxeDb.Init.ExMapTable,
                        sizeof(mPcdDatabase->DxeDb.Init.ExMapTable)
                        );
  }

  return Status;
}

/**
  Get all token space guid table which is different with given token space guid.

  @param ExMapTableSize  The size of guid table
  @param ExMapTable      Token space guid table that want to be scaned.
  @param GuidTable       Guid table

  @return all token space guid table which is different with given token space guid.

**/
EFI_GUID **
GetDistinctTokenSpace (
  IN OUT    UINTN             *ExMapTableSize,
  IN        DYNAMICEX_MAPPING *ExMapTable,
  IN        EFI_GUID          *GuidTable
  )
{
  EFI_GUID  **DistinctTokenSpace;
  UINTN     OldGuidIndex;
  UINTN     TsIdx;
  UINTN     Idx;


  DistinctTokenSpace = AllocateZeroPool (*ExMapTableSize * sizeof (EFI_GUID *));
  ASSERT (DistinctTokenSpace != NULL);

  TsIdx = 0;
  OldGuidIndex = ExMapTable[0].ExGuidIndex;
  DistinctTokenSpace[TsIdx] = &GuidTable[OldGuidIndex];
  for (Idx = 1; Idx < *ExMapTableSize; Idx++) {
    if (ExMapTable[Idx].ExGuidIndex != OldGuidIndex) {
      OldGuidIndex = ExMapTable[Idx].ExGuidIndex;
      DistinctTokenSpace[++TsIdx] = &GuidTable[OldGuidIndex];
    }
  }

  //
  // The total number of Distinct Token Space
  // is TsIdx + 1 because we use TsIdx as a index
  // to the DistinctTokenSpace[]
  //
  *ExMapTableSize = TsIdx + 1;
  return DistinctTokenSpace;
    
}
  
/**
  Get next token space in PCD database according to given token space guid.
  
  @param Guid            Given token space guid. If NULL, then Guid will be set to 
                         the first PCD token space in PCD database, If not NULL, then
                         Guid will be set to next PCD token space.

  @retval EFI_UNSUPPORTED 
  @retval EFI_NOT_FOUND   If PCD database has no token space table or can not find given
                          token space in PCD database.
  @retval EFI_SUCCESS     Success to get next token space guid.
**/
EFI_STATUS
EFIAPI
DxePcdGetNextTokenSpace (
  IN OUT CONST EFI_GUID               **Guid
  )
{
  UINTN               Idx;
  UINTN               Idx2;
  UINTN               Idx3;
  UINTN               PeiTokenSpaceTableSize;
  UINTN               DxeTokenSpaceTableSize;
  EFI_GUID            **PeiTokenSpaceTable;
  EFI_GUID            **DxeTokenSpaceTable;
  BOOLEAN             Match;
  BOOLEAN             PeiExMapTableEmpty;
  BOOLEAN             DxeExMapTableEmpty;

  ASSERT (Guid != NULL);
  
  PeiExMapTableEmpty = PEI_EXMAP_TABLE_EMPTY;
  DxeExMapTableEmpty = DXE_EXMAP_TABLE_EMPTY;

  if (PeiExMapTableEmpty && DxeExMapTableEmpty) {
    if (*Guid != NULL) {
      return EFI_NOT_FOUND;
    } else {
      return EFI_SUCCESS;
    }
  }
  
  
  if (TmpTokenSpaceBuffer[0] == NULL) {
    PeiTokenSpaceTableSize = 0;

    if (!PeiExMapTableEmpty) {
      PeiTokenSpaceTableSize = PEI_EXMAPPING_TABLE_SIZE;
      PeiTokenSpaceTable = GetDistinctTokenSpace (&PeiTokenSpaceTableSize,
                            mPcdDatabase->PeiDb.Init.ExMapTable,
                            mPcdDatabase->PeiDb.Init.GuidTable
                            );
      CopyMem (TmpTokenSpaceBuffer, PeiTokenSpaceTable, sizeof (EFI_GUID*) * PeiTokenSpaceTableSize);
    }

    if (!DxeExMapTableEmpty) {
      DxeTokenSpaceTableSize = DXE_EXMAPPING_TABLE_SIZE;
      DxeTokenSpaceTable = GetDistinctTokenSpace (&DxeTokenSpaceTableSize,
                            mPcdDatabase->DxeDb.Init.ExMapTable,
                            mPcdDatabase->DxeDb.Init.GuidTable
                            );

      //
      // Make sure EFI_GUID in DxeTokenSpaceTable does not exist in PeiTokenSpaceTable
      //
      for (Idx2 = 0, Idx3 = PeiTokenSpaceTableSize; Idx2 < DxeTokenSpaceTableSize; Idx2++) {
        Match = FALSE;
        for (Idx = 0; Idx < PeiTokenSpaceTableSize; Idx++) {
          if (CompareGuid (TmpTokenSpaceBuffer[Idx], DxeTokenSpaceTable[Idx2])) {
            Match = TRUE;
            break;
          }
        }
        if (!Match) {
          TmpTokenSpaceBuffer[Idx3++] = DxeTokenSpaceTable[Idx2];
        }
      }
    }
  }

  if (*Guid == NULL) {
    *Guid = TmpTokenSpaceBuffer[0];
    return EFI_SUCCESS;
  }
  
  for (Idx = 0; Idx < (PEI_EXMAPPING_TABLE_SIZE + DXE_EXMAPPING_TABLE_SIZE); Idx++) {
    if(CompareGuid (*Guid, TmpTokenSpaceBuffer[Idx])) {
      Idx++;
      *Guid = TmpTokenSpaceBuffer[Idx];
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}



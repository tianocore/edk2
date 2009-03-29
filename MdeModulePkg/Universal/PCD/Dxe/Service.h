/** @file
Private functions used by PCD DXE driver.

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PCD_DXE_SERVICE_H_
#define _PCD_DXE_SERVICE_H_

#include <PiDxe.h>
#include <Guid/PcdDataBaseHobGuid.h>
#include <Protocol/Pcd.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Protocol Interface function declaration.
//
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
  IN  UINTN                  SkuId
  );

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
  IN UINTN             TokenNumber
  );

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
  IN UINTN             TokenNumber
  );

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
  IN UINTN             TokenNumber
  );

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
  IN UINTN             TokenNumber
  );

/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param[in]  TokenNumber The PCD token number. 

  @return The pointer to the buffer to be retrieved.
  
**/
VOID *
EFIAPI
DxePcdGetPtr (
  IN UINTN             TokenNumber
  );

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
  IN UINTN             TokenNumber
  );

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
  IN UINTN             TokenNumber
  );

/**
  Retrieves an 8-bit value for a given PCD token.

  Retrieves the 8-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  TokenNumber The PCD token number. 

  @return The size 8-bit value for the PCD token.
  
**/
UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  );

/**
  Retrieves an 16-bit value for a given PCD token.

  Retrieves the 16-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  TokenNumber The PCD token number. 

  @return The size 16-bit value for the PCD token.
  
**/
UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  );

/**
  Retrieves an 32-bit value for a given PCD token.

  Retrieves the 32-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  TokenNumber The PCD token number. 

  @return The size 32-bit value for the PCD token.
  
**/
UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  );

/**
  Retrieves an 64-bit value for a given PCD token.

  Retrieves the 64-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  TokenNumber The PCD token number. 

  @return The size 64-bit value for the PCD token.
  
**/
UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  );

/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  TokenNumber The PCD token number. 

  @return The pointer to the buffer to be retrieved.
  
**/
VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  );

/**
  Retrieves an Boolean value for a given PCD token.

  Retrieves the Boolean value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  TokenNumber The PCD token number. 

  @return The size Boolean value for the PCD token.
  
**/
BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  );

/**
  Retrieves the size of the value for a given PCD token.

  Retrieves the current size of a particular PCD token.  
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  TokenNumber The PCD token number. 

  @return The size of the value for the PCD token.
  
**/
UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber
  );

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
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );

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
  IN UINTN             TokenNumber,
  IN UINT16             Value
  );

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
  IN UINTN             TokenNumber,
  IN UINT32             Value
  );

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
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );


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
  IN        UINTN             TokenNumber,
  IN OUT    UINTN             *SizeOfBuffer,
  IN        VOID              *Buffer
  );

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
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  );


/**
  Sets an 8-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
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
DxePcdSet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );

/**
  Sets an 16-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
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
DxePcdSet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  );

/**
  Sets an 32-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
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
DxePcdSet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT32             Value
  );

/**
  Sets an 64-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
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
DxePcdSet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );

/**
  Sets a value of a specified size for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
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
DxePcdSetPtrEx (
  IN        CONST EFI_GUID    *Guid,
  IN        UINTN             TokenNumber,
  IN OUT    UINTN             *SizeOfBuffer,
  IN        VOID              *Buffer
  );

/**
  Sets an Boolean value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
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
DxePcdSetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  );

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
  );

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
  );

/**
  Retrieves the next valid PCD token for a given namespace.

  @param[in]      Guid          The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in, out]  TokenNumber   A pointer to the PCD token number to use to find the subsequent token number.  
                                If the input token namespace or token number does not exist on the platform, 
                                an error is returned and the value of *TokenNumber is undefined. To retrieve the "first" token, 
                                have the pointer reference a TokenNumber value of 0. If the input token number is 0 and 
                                there is no valid token number for this token namespace,  *TokenNumber will be assigned to 
                                0 and the function return EFI_SUCCESS. If the token number is the last valid token number, 
                                *TokenNumber will be assigned to 0 and the function return EFI_SUCCESS.

  @retval EFI_SUCCESS  The PCD service retrieved the next valid token number. Or the input token number 
                        is already the last valid token number in the PCD database. 
                        In the later case, *TokenNumber is updated with the value of 0.
  @retval EFI_NOT_FOUND If this input token number and token namespace does not exist on the platform.

**/
EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT   UINTN                  *TokenNumber
  );

/**
  Get next token space in PCD database according to given token space guid.
  
  This routine is enable only when feature flag PCD PcdDxePcdDatabaseTraverseEnabled 
  is TRUE.
  
  @param Guid            Given token space guid. If NULL, then Guid will be set to 
                         the first PCD token space in PCD database, If not NULL, then
                         Guid will be set to next PCD token space.

  @retval EFI_UNSUPPORTED If feature flag PCD PcdDxePcdDatabaseTraverseEnabled is FALSE.
  @retval EFI_NOT_FOUND   If PCD database has no token space table or can not find given
                          token space in PCD database.
  @retval EFI_SUCCESS     Success to get next token space guid.
**/
EFI_STATUS
EFIAPI
DxePcdGetNextTokenSpace (
  IN OUT CONST EFI_GUID               **Guid
  );

typedef struct {
  LIST_ENTRY              Node;
  PCD_PROTOCOL_CALLBACK   CallbackFn;
} CALLBACK_FN_ENTRY;

#define CR_FNENTRY_FROM_LISTNODE(Record, Type, Field) BASE_CR(Record, Type, Field)

//
// Internal Functions
//

/**
  Wrapper function for setting non-pointer type value for a PCD entry.

  @param TokenNumber     Pcd token number autogenerated by build tools.
  @param Data            Value want to be set for PCD entry
  @param Size            Size of value.

  @return status of SetWorker.

**/
EFI_STATUS
SetValueWorker (
  IN UINTN                   TokenNumber,
  IN VOID                    *Data,
  IN UINTN                   Size
  );

/**
  Set value for an PCD entry

  @param TokenNumber     Pcd token number autogenerated by build tools.
  @param Data            Value want to be set for PCD entry
  @param Size            Size of value.
  @param PtrType         If TRUE, the type of PCD entry's value is Pointer.
                         If False, the type of PCD entry's value is not Pointer.

  @retval EFI_INVALID_PARAMETER  If this PCD type is VPD, VPD PCD can not be set.
  @retval EFI_INVALID_PARAMETER  If Size can not be set to size table.
  @retval EFI_NOT_FOUND          If value type of PCD entry is intergrate, but not in
                                 range of UINT8, UINT16, UINT32, UINT64
  @retval EFI_NOT_FOUND          Can not find the PCD type according to token number.                                
**/
EFI_STATUS
SetWorker (
  IN          UINTN                     TokenNumber,
  IN          VOID                      *Data,
  IN OUT      UINTN                     *Size,
  IN          BOOLEAN                   PtrType
  );

/**
  Wrapper function for set PCD value for non-Pointer type dynamic-ex PCD.

  @param ExTokenNumber   Token number for dynamic-ex PCD.
  @param Guid            Token space guid for dynamic-ex PCD.
  @param Data            Value want to be set.
  @param SetSize         The size of value.

  @return status of ExSetWorker().

**/
EFI_STATUS
ExSetValueWorker (
  IN          UINTN                ExTokenNumber,
  IN          CONST EFI_GUID       *Guid,
  IN          VOID                 *Data,
  IN          UINTN                SetSize
  );

/**
  Set value for a dynamic PCD entry.
  
  This routine find the local token number according to dynamic-ex PCD's token 
  space guid and token number firstly, and invoke callback function if this PCD
  entry registered callback function. Finally, invoken general SetWorker to set
  PCD value.
  
  @param ExTokenNumber   Dynamic-ex PCD token number.
  @param Guid            Token space guid for dynamic-ex PCD.
  @param Data            PCD value want to be set
  @param SetSize         Size of value.
  @param PtrType         If TRUE, this PCD entry is pointer type.
                         If FALSE, this PCD entry is not pointer type.

  @return status of SetWorker().

**/
EFI_STATUS
ExSetWorker (
  IN      UINTN                ExTokenNumber,
  IN      CONST EFI_GUID       *Guid,
  IN      VOID                 *Data,
  IN OUT  UINTN                *Size,
  IN      BOOLEAN              PtrType
  );

/**
  Get the PCD entry pointer in PCD database.
  
  This routine will visit PCD database to find the PCD entry according to given
  token number. The given token number is autogened by build tools and it will be 
  translated to local token number. Local token number contains PCD's type and 
  offset of PCD entry in PCD database.

  @param TokenNumber     Token's number, it is autogened by build tools
  @param GetSize         The size of token's value

  @return PCD entry pointer in PCD database

**/
VOID *
GetWorker (
  IN UINTN             TokenNumber,
  IN UINTN             GetSize
  );

/**
  Wrapper function for get PCD value for dynamic-ex PCD.

  @param Guid            Token space guid for dynamic-ex PCD.
  @param ExTokenNumber   Token number for dynamic-ex PCD.
  @param GetSize         The size of dynamic-ex PCD value.

  @return PCD entry in PCD database.

**/
VOID *
ExGetWorker (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINTN                  GetSize
  );

/**
  Find the local token number according to system SKU ID.

  @param LocalTokenNumber PCD token number
  @param Size             The size of PCD entry.
  @param IsPeiDb          If TRUE, the PCD entry is initialized in PEI phase.
                          If False, the PCD entry is initialized in DXE phase.

  @return Token number according to system SKU ID.

**/
UINT32
GetSkuEnabledTokenNumber (
  UINT32 LocalTokenNumber,
  UINTN  Size,
  BOOLEAN IsPeiDb
  );

/**
  Get Variable which contains HII type PCD entry.

  @param VariableGuid    Variable's guid
  @param VariableName    Variable's unicode name string
  @param VariableData    Variable's data pointer, 
  @param VariableSize    Variable's size.

  @return the status of gRT->GetVariable
**/
EFI_STATUS
GetHiiVariable (
  IN  EFI_GUID      *VariableGuid,
  IN  UINT16        *VariableName,
  OUT UINT8          **VariableData,
  OUT UINTN         *VariableSize
  );

/**
  Set value for HII-type PCD.

  A HII-type PCD's value is stored in a variable. Setting/Getting the value of 
  HII-type PCD is to visit this variable.
  
  @param VariableGuid    Guid of variable which stored value of a HII-type PCD.
  @param VariableName    Unicode name of variable which stored value of a HII-type PCD.
  @param Data            Value want to be set.
  @param DataSize        Size of value
  @param Offset          Value offset of HII-type PCD in variable.

  @return status of GetVariable()/SetVariable().

**/
EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  IN  CONST VOID   *Data,
  IN  UINTN        DataSize,
  IN  UINTN        Offset
  );

/**
  Register the callback function for a PCD entry.

  This routine will register a callback function to a PCD entry by given token number
  and token space guid.
  
  @param TokenNumber        PCD token's number, it is autogened by build tools.
  @param Guid               PCD token space's guid, 
                            if not NULL, this PCD is dynamicEx type PCD.
  @param CallBackFunction   Callback function pointer

  @return EFI_SUCCESS Always success for registering callback function.

**/
EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  );

/**
  UnRegister the callback function for a PCD entry.

  This routine will unregister a callback function to a PCD entry by given token number
  and token space guid.

  @param TokenNumber        PCD token's number, it is autogened by build tools.
  @param Guid               PCD token space's guid.
                            if not NULL, this PCD is dynamicEx type PCD.
  @param CallBackFunction   Callback function pointer

  @retval EFI_SUCCESS               Callback function is success to be unregister.
  @retval EFI_INVALID_PARAMETER     Can not find the PCD entry by given token number.
**/
EFI_STATUS
DxeUnRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  );

/**
  Initialize the PCD database in DXE phase.
  
  PCD database in DXE phase also contains PCD database in PEI phase which is copied
  from GUID Hob.

**/
VOID
BuildPcdDxeDataBase (
  VOID
  );

/**
  Get local token number according to dynamic-ex PCD's {token space guid:token number}

  A dynamic-ex type PCD, developer must provide pair of token space guid: token number
  in DEC file. PCD database maintain a mapping table that translate pair of {token
  space guid: token number} to local token number.
  
  @param Guid            Token space guid for dynamic-ex PCD entry.
  @param ExTokenNumber   Dynamic-ex PCD token number.

  @return local token number for dynamic-ex PCD.

**/
UINTN
GetExPcdTokenNumber (
  IN CONST EFI_GUID             *Guid,
  IN UINT32                     ExTokenNumber
  );

/**
  Get next token number in given token space.
  
  This routine is used for dynamicEx type PCD. It will firstly scan token space
  table to get token space according to given token space guid. Then scan given 
  token number in found token space, if found, then return next token number in 
  this token space.

  @param Guid            Token space guid. Next token number will be scaned in 
                         this token space.
  @param TokenNumber     Token number. 
                         If PCD_INVALID_TOKEN_NUMBER, return first token number in 
                         token space table.
                         If not PCD_INVALID_TOKEN_NUMBER, return next token number
                         in token space table.
  @param GuidTable       Token space guid table. It will be used for scan token space
                         by given token space guid.
  @param SizeOfGuidTable The size of guid table.
  @param ExMapTable      DynamicEx token number mapping table.
  @param SizeOfExMapTable The size of dynamicEx token number mapping table.

  @retval EFI_NOT_FOUND  Can not given token space or token number.
  @retval EFI_SUCCESS    Success to get next token number.

**/
EFI_STATUS
ExGetNextTokeNumber (
  IN      CONST EFI_GUID    *Guid,
  IN OUT  UINTN             *TokenNumber,
  IN      EFI_GUID          *GuidTable,
  IN      UINTN             SizeOfGuidTable,
  IN      DYNAMICEX_MAPPING *ExMapTable,
  IN      UINTN             SizeOfExMapTable
  );

/**
  Get size of POINTER type PCD value.

  @param LocalTokenNumberTableIdx Index of local token number in local token number table.
  @param MaxSize                  Maximum size of POINTER type PCD value.

  @return size of POINTER type PCD value.

**/
UINTN
GetPtrTypeSize (
  IN    UINTN             LocalTokenNumberTableIdx,
  OUT   UINTN             *MaxSize
  );

/**
  Set size of POINTER type PCD value. The size should not exceed the maximum size
  of this PCD value.

  @param LocalTokenNumberTableIdx Index of local token number in local token number table.
  @param CurrentSize              Size of POINTER type PCD value.

  @retval TRUE  Success to set size of PCD value.
  @retval FALSE Fail to set size of PCD value.
**/
BOOLEAN
SetPtrTypeSize (
  IN          UINTN             LocalTokenNumberTableIdx,
  IN    OUT   UINTN             *CurrentSize
  );

extern PCD_DATABASE * mPcdDatabase;

extern DXE_PCD_DATABASE_INIT gDXEPcdDbInit;

extern EFI_LOCK mPcdDatabaseLock;

#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                      Introduction of PCD database                          //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
/**
 1, Introduction
    PEI PCD database hold all dynamic type PCD information used in PEI phase.
    The structure of PEI PCD database is generated by build tools according to
    dynamic PCD usage for specified platform.
    
 2, Dynamic Type PCD
    Dynamic type PCD is designed for accessing setting which value is determined
    dynamic. In contrast, the value of static type PCD (FeatureFlag, FixedPcd, 
    PatchablePcd) is fixed in final generated FD image in build time. 
        
    2.1 The "dynamic" determination means:
      a) The PCD value is produced by someone driver and consumed by other driver
         in execution time.
      b) The PCD value is set/get by user from FrontPage.
      c) The PCD value is produced by platform OEM specified area.
    
    2.2 According to distribution mehod, dynamic PCD could be classfied as:
      a) Dynamic:
         This type PCD is used for module in source distribution which will be 
         built in platform.
      b) DynamicEx:
         This type PCD is used for module in binary distribution which will be 
         will not built.
         
    2.3 According to storage method, dynamic PCD could be classfied as:
      a) Default Storage: 
         - The value is stored in PCD database maintained by PCD database in boot 
           time memory which is built as a guid hob in PEI phase.
         - This type is used for communication between PEIM/DXE driver, DXE/DXE 
           driver. But all set/get value will be losted after boot-time memory 
           is turn off.
         - [PcdsDynamicDefault]/[PcdsDynamicExDefault] is used as section name 
           for this type PCD in platform DSC file.
         
      b) Variable Storage: 
         - The value is stored in variable area. 
         - As default storage type, this type PCD could used for communication.
           But beside it, this type PCD could be used store the value associating 
           with HII setting via variable technology.
         - In PEI phase, the PCD value could only be got but can not be set due 
           to variable area is readonly for PEI phase.
         - [PcdsDynamicHii]/[PcdsDynamicExHii] is used as section name for this 
           type PCD in platform DSC file.
           
      c) OEM specificed storage area:
         - The value is stored in OEM specified area, the base address is specified
           by a FixedAtBuild PCD PcdVpdBaseAddress.
         - The area is read only for PEI and DXE phase.
         - [PcdsDynamicVpd]/[PcdsDynamicExVpd] is used as section name for this 
           type PCD in platform DSC file.
      
      Note: The default value of dynamic PCD are storaged in memory maintained
            by PEI/DXE PCD drvier.
            
    2.4 When and how to use dynamic PCD
      Module developer do not care the used PCD is dynamic or static when writting
      source code/INF. Dynamic PCD and dynamic type is pointed by platform integrator 
      in platform DSC file. Please ref section 2.3 to get matching between dynamic
      PCD type and section name in DSC file.
    
 3, PCD database:
    Although dynamic PCD could be in different storage type as above description, 
    but the basic information and default value for all dynamic PCD is hold
    by PCD database maintained by PEI/DXE driver.
    
    As whole EFI BIOS boot path is divided into PEI/DXE phase, the PCD database
    also is divided into Pei/Dxe database maintaied by PcdPeim/PcdDxe driver separatly.
    To make PcdPeim's driver image smaller, PEI PCD database only hold all dynamic
    PCD information used in PEI phase or use in both PEI/DXE phase. And DXE PCD
    database contains all PCDs used in PEI/DXE phase in memory.
    
    Build tool will generate PCD database into some C structure and variable for 
    PEI/DXE PCD driver according to dynamic PCD section in platform DSC file. 
    
    3.1 PcdPeim and PcdDxe
      PEI PCD database is maintained by PcdPeim driver run from flash. PcdPeim driver
      build guid hob in temporary memory and copy auto-generated C structure 
      to temporary memory for PEI PCD database. 
      DXE PCD database is maintained by PcdDxe driver.At entry point of PcdDxe driver,
      a new PCD database is allocated in boot-time memory which including all
      PEI PCD and DXE PCD entry.
      
      Pcd driver should run as early as possible before any other driver access
      dynamic PCD's value. PEI/DXE "Apriori File" mechanism make it possible by
      making PcdPeim/PcdDxe as first dispatching driver in PEI/DXE phase.
      
    3.2 Token space Guid/Token number, Platform token, Local token number
           Dynamic PCD
          +-----------+               +---------+
          |TokenSpace |               |Platform |
          |   Guid    |  build tool   | Token   |
          |    +      +-------------->| Number  |
          |  Token    |               +---------+`._
          |  Number   |                             `.
          +-----------+                               `.  +------+
                                                        `-|Local |
                                                          |Token |
                               DynamicEx PCD            ,-|Number|
                               +-----------+         ,-'  +------+
                               |TokenSpace |      ,-'
                               |   Guid    |  _,-'
                               |    +      +.'
                               |  Token    |
                               |  Number   |
                               +-----------+
    
    
      3.2.1 Pair of Token space guid + Token number
        Any type PCD is identified by pair of "TokenSpaceGuid + TokeNumber". But it
        is not easy maintained by PCD driver, and hashed token number will make 
        searching slowly. 

      3.2.2 Platform Token Number
        "Platform token number" concept is introduced for mapping to a pair of 
        "TokenSpaceGuid + TokenNumber". The platform token number is generated by 
        build tool in autogen.h and all of them are continual in a platform scope 
        started from 1.(0 meaning invalid internal token number)
        With auto-generated "platform token number", PcdGet(PcdSampleDynamicPcd)
        in source code is translated to LibPcdGet(_PCD_TOKEN_PcdSampleDynamicPcd) 
        in autogen.h.
        Notes: The mapping between pair of "tokenspace guid + token number" and
        "internal token number" need build tool establish, so "platform token number"
        mechanism is not suitable for binary module which use DynamicEx type PCD.
        To access a dynamicEx type PCD, pair of "token space guid/token number" all need
        to be specificed for PcdSet/PcdGet accessing macro.
      
        Platform Token Number is started from 1, and inceased continuous. From whole 
        platform scope, there are two zones: PEI Zone and DXE Zone
                  |                      Platform Token Number
        ----------|----------------------------------------------------------------
        PEI Zone: |            1                 ~  PEI_LOCAL_TOKEN_NUMBER
        DXE Zone: | (PEI_LOCAL_TOKEN_NUMBER + 1) ~ (PEI_LOCAL_TOKEN_NUMBER + DXE_LOCAL_TOKEN_NUMBER)
        
      3.2.3 Local Token Number
        To fast searching a PCD entry in PCD database, PCD driver translate 
        platform token number to local token number via a mapping table.
        For binary DynamicEx type PCD, there is a another mapping table to translate
        "token space guid + token number" to local token number directly.
        Local token number is identifier for all internal interface in PCD PEI/DXE
        driver.
        
        A local token number is a 32-bit value in following meaning:
         32 ------------- 28 ---------- 24 -------- 0
          | PCD type mask  | Datum Type  |  Offset  |
          +-----------------------------------------+
        where:
          PCd type mask: indicate Pcd type from following macro:
                         PCD_TYPE_DATA
                         PCD_TYPE_HII
                         PCD_TYPE_VPD
                         PCD_TYPE_SKU_ENABLED
                         PCD_TYPE_STRING
          Datum Type   : indicate PCD vaue type from following macro:
                         PCD_DATUM_TYPE_POINTER
                         PCD_DATUM_TYPE_UINT8
                         PCD_DATUM_TYPE_UINT16
                         PCD_DATUM_TYPE_UINT32
                         PCD_DATUM_TYPE_UINT64
          Offset      : indicate the related offset of PCD value in PCD database array.
       Based on local token number, PCD driver could fast determine PCD type, value
       type and get PCD entry from PCD database.
       
    3.3 PCD Database C structure.
      PCD Database C structure is generated by build tools in PCD driver's autogen.h/
      autogen.c file. In generated C structure, following information is stored:
      - ExMapTable: This table is used translate a binary dynamicex type PCD's 
                    "tokenguid + token" to local token number.
      - LocalTokenNumberTable:
                    This table stores all local token number in array, use "Internal
                    token number" as array index to get PCD entry's offset fastly.
      - SizeTable:  This table stores the size information for all PCD entry.
      - GuidTable:  This table stores guid value for DynamicEx's token space,
                    HII type PCD's variable.
      - SkuIdTable: TBD
      - SystemSkuId: TBD
      - PCD value structure:  
                    Every PCD has a value record in PCD database. For different
                    datum type PCD has different record structure which will be 
                    introduced in 3.3.1
      
      In a PCD database structure, there are two major area: Init and UnInit. 
      Init area is use stored above PCD internal structure such as ExMapTable, 
      LocalTokenNumberTable etc and the (default) value of PCD which has default 
      value specified in platform DSC file.
      Unint area is used stored the value of PCD which has no default value in
      platform DSC file, the value of NULL, 0 specified in platform DSC file can
      be seemed as "no default value".
      
      3.3.1 Simple Sample PCD Database C Structure
        A general sample of PCD database structue is as follows:
        typedef struct _PCD_DATABASE {
          typedef struct _PCD_DATABASE_INIT {
            //===== Following is PCD database internal maintain structures
            DYNAMICEX_MAPPING ExMapTable[PEI_EXMAPPING_TABLE_SIZE];
            UINT32            LocalTokenNumberTable[PEI_LOCAL_TOKEN_NUMBER_TABLE_SIZE];
            GUID              GuidTable[PEI_GUID_TABLE_SIZE];
            SIZE_INFO         SizeTable[PEI_SIZE_TABLE_SIZE];
            UINT8             SkuIdTable[PEI_SKUID_TABLE_SIZE];
            SKU_ID            SystemSkuId;
            
            //===== Following is value structure for PCD with default value
            ....
            ....
            ....
          } Init;
          typedef struct _PCD_DATABSE_UNINIT {
            //==== Following is value structure for PCD without default value
            ....
            ....
          } UnInit;
        }
      
      3.3.2 PCD value structure in PCD database C structure
        The value's structure is generated by build tool in PCD database C structure.
        The PCDs in different datum type has different value structure.
        
        3.3.2.1 UINT8/UINT16/UINT32/UINT64 datum type PCD
          The C structure for these datum type PCD is just a UINT8/UINT16/UINT32/UINT64
          data member in PCD database, For example:
          UINT16  PcdHardwareErrorRecordLevel_d3705011_bc19_4af7_be16_f68030378c15_VariableDefault_0;
          Above structure is generated by build tool, the member name is "PcdCName_Guidvalue"
          Member type is UINT16 according to PcdHardwareErrorRecordLevel declaration
          in DEC file.
          
        3.3.2.2 VOID* datum type PCD
          The value of VOID* datum type PCD is a UINT8/UINT16 array in PCD database.
          
          3.3.2.2.1 VOID* - string type
            If the default value for VOID* datum type PCD like L"xxx", the PCD is 
            used for unicode string, and C structure of this datum type PCD is 
            UINT16 string array in PCD database, for example:
            UINT16 StringTable[29];
            The number of 29 in above sample is max size of a unicode string.
            
            If the default value for VOID* datum type PCD like "xxx", the PCD is
            used for ascii string, and C structure of this datum type PCD is 
            UINT8 string array in PCD database, for example:
            UINT8 StringTable[20];
            The number of 20 in above sample is max size of a ascii string.
            
          3.3.2.2.2 VOID* - byte array
            If the default value of VOID* datum type PCD like {'0x29', '0x01', '0xf2'}
            the PCD is used for byte array. The generated structrue is same as 
            above ascii string table,
            UINT8 StringTable[13];
            The number of 13 in above sample is max size of byte array.
       
      3.3.3 Some utility structures in PCD Database
        3.3.3.1 GuidTable
          GuidTable array is used to store all related GUID value in PCD database:
            - Variable GUID for HII type PCD
            - Token space GUID for dynamicex type PCD 
    
    3.4 PEI PCD Database      
    
**/

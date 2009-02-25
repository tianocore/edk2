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

////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                              ///
///  Following code is an example for auto-generated PCD database to hold all dynamic/dynamicex  ///
///  PCD's value for one given platform. And following sample comes from autogen.h/autogen.c of  ///
///  PCD Dxe driver for NT32 platform.                                                           ///
///                                                                                              ///
///  The PCD database is stored into a big structure - PCD_DATABASE which is consisted of        ///
///  PEI PCD database structure and DXE PCD database structure.                                  ///
///                                                                                              ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// //
// // Common definitions
// //
// typedef UINT8 SKU_ID;
// 
// //
// // A PCD offset value is consisted as follows:
// // 32 ------------- 28 ---------- 24 -------- 0
// //  | PCD type mask  | Datum Type  |  Offset  |
// //  -------------------------------------------
// 
// #define PCD_TYPE_SHIFT        28             // 28 ~ 32 bit hold PCD type information
// 
// #define PCD_TYPE_DATA         (0x0 << PCD_TYPE_SHIFT)
// #define PCD_TYPE_HII          (0x8 << PCD_TYPE_SHIFT)
// #define PCD_TYPE_VPD          (0x4 << PCD_TYPE_SHIFT)
// #define PCD_TYPE_SKU_ENABLED  (0x2 << PCD_TYPE_SHIFT)
// #define PCD_TYPE_STRING       (0x1 << PCD_TYPE_SHIFT)
// 
// #define PCD_TYPE_ALL_SET      (PCD_TYPE_DATA | PCD_TYPE_HII | PCD_TYPE_VPD | PCD_TYPE_SKU_ENABLED | PCD_TYPE_STRING)
// 
// #define PCD_DATUM_TYPE_SHIFT  24             // 24 ~ 28 bit hold datum type information
// 
// #define PCD_DATUM_TYPE_POINTER  (0x0 << PCD_DATUM_TYPE_SHIFT)
// #define PCD_DATUM_TYPE_UINT8    (0x1 << PCD_DATUM_TYPE_SHIFT)
// #define PCD_DATUM_TYPE_UINT16   (0x2 << PCD_DATUM_TYPE_SHIFT)
// #define PCD_DATUM_TYPE_UINT32   (0x4 << PCD_DATUM_TYPE_SHIFT)
// #define PCD_DATUM_TYPE_UINT64   (0x8 << PCD_DATUM_TYPE_SHIFT)
// 
// #define PCD_DATUM_TYPE_ALL_SET  (PCD_DATUM_TYPE_POINTER | \
//                                  PCD_DATUM_TYPE_UINT8   | \
//                                  PCD_DATUM_TYPE_UINT16  | \
//                                  PCD_DATUM_TYPE_UINT32  | \
//                                  PCD_DATUM_TYPE_UINT64)
// 
// #define PCD_DATABASE_OFFSET_MASK (~(PCD_TYPE_ALL_SET | PCD_DATUM_TYPE_ALL_SET))
// 
// typedef struct  {
//   UINT32  ExTokenNumber;
//   UINT16  LocalTokenNumber;   // PCD Number of this particular platform build
//   UINT16  ExGuidIndex;        // Index of GuidTable
// } DYNAMICEX_MAPPING;
// 
// typedef struct {
//   UINT32  SkuDataStartOffset; //We have to use offsetof MACRO as we don't know padding done by compiler
//   UINT32  SkuIdTableOffset;   //Offset from the PCD_DB
// } SKU_HEAD;
// 
// typedef struct {
//   UINT16  GuidTableIndex;     // Offset in Guid Table in units of GUID.
//   UINT16  StringIndex;        // Offset in String Table in units of UINT16.
//   UINT16  Offset;             // Offset in Variable
//   UINT16  DefaultValueOffset; // Offset of the Default Value
// } VARIABLE_HEAD;
// 
// typedef  struct {
//   UINT32  Offset;
// } VPD_HEAD;
// 
// typedef UINT16 STRING_HEAD;
// 
// typedef UINT16 SIZE_INFO;
// 
// #define offsetof(s,m)  (UINT32) (UINTN) &(((s *)0)->m)
// 
// 
// #define PEI_GUID_TABLE_SIZE                1
// #define PEI_STRING_TABLE_SIZE              1
// #define PEI_SKUID_TABLE_SIZE               1
// #define PEI_LOCAL_TOKEN_NUMBER_TABLE_SIZE  3
// #define PEI_LOCAL_TOKEN_NUMBER             3
// #define PEI_EXMAPPING_TABLE_SIZE           1
// #define PEI_EX_TOKEN_NUMBER                0
// #define PEI_SIZE_TABLE_SIZE                2
// #define PEI_GUID_TABLE_EMPTY               TRUE
// #define PEI_STRING_TABLE_EMPTY             TRUE
// #define PEI_SKUID_TABLE_EMPTY              TRUE
// #define PEI_DATABASE_EMPTY                 FALSE
// #define PEI_EXMAP_TABLE_EMPTY              TRUE
// 
// //
// // PEI database structure for dynamic/dynamicex PCD which has default value.
// //
// typedef struct {
// 
// 
// 
// 
// 
//   DYNAMICEX_MAPPING  ExMapTable[PEI_EXMAPPING_TABLE_SIZE];                       // table for mapping dynamicex token number to local token number
//   UINT32             LocalTokenNumberTable[PEI_LOCAL_TOKEN_NUMBER_TABLE_SIZE];   // table for local token number
//   GUID               GuidTable[PEI_GUID_TABLE_SIZE];                             // table for token guid
// 
// 
//   UINT16             StringTable[1]; /* _ */                                     // table for unicode string type PCD's value
// 
//   SIZE_INFO          SizeTable[PEI_SIZE_TABLE_SIZE];                             // table for PCD size information
// 
// 
// 
// 
// 
// 
//   UINT8              SkuIdTable[PEI_SKUID_TABLE_SIZE];                           // table for SKU IDs 
//   SKU_ID             SystemSkuId;                                                // system SKU ID
// } PEI_PCD_DATABASE_INIT;
// 
// //
// // PEI database structure for dynamic/dynamicex PCD which has no default value.
// //
// typedef struct {
// 
// 
//   UINT32   PcdFlashNvStorageVariableBase_a1aff049_fdeb_442a_b320_13ab4cb72bbc[1];        // PCD entry for PcdFlashNvStorageVariableBase
//   UINT32   PcdFlashNvStorageFtwSpareBase_a1aff049_fdeb_442a_b320_13ab4cb72bbc[1];        // PCD entry for PcdFlashNvStorageFtwSpareBase
//   UINT32   PcdFlashNvStorageFtwWorkingBase_a1aff049_fdeb_442a_b320_13ab4cb72bbc[1];      // PCD entry for PcdFlashNvStorageFtwWorkingBase
// 
// 
// 
// 
// } PEI_PCD_DATABASE_UNINIT;
// 
// #define PCD_PEI_SERVICE_DRIVER_VERSION         2
// 
// typedef struct {
//   PEI_PCD_DATABASE_INIT    Init;
//   PEI_PCD_DATABASE_UNINIT  Uninit;
// } PEI_PCD_DATABASE;
// 
// #define PEI_NEX_TOKEN_NUMBER (PEI_LOCAL_TOKEN_NUMBER - PEI_EX_TOKEN_NUMBER)
// 
// #define DXE_GUID_TABLE_SIZE                1
// #define DXE_STRING_TABLE_SIZE              212
// #define DXE_SKUID_TABLE_SIZE               1
// #define DXE_LOCAL_TOKEN_NUMBER_TABLE_SIZE  9
// #define DXE_LOCAL_TOKEN_NUMBER             9
// #define DXE_EXMAPPING_TABLE_SIZE           1
// #define DXE_EX_TOKEN_NUMBER                0
// #define DXE_SIZE_TABLE_SIZE                16
// #define DXE_GUID_TABLE_EMPTY               TRUE
// #define DXE_STRING_TABLE_EMPTY             FALSE
// #define DXE_SKUID_TABLE_EMPTY              TRUE
// #define DXE_DATABASE_EMPTY                 FALSE
// #define DXE_EXMAP_TABLE_EMPTY              TRUE
// 
// typedef struct {
// 
// 
// 
// 
// 
//   DYNAMICEX_MAPPING  ExMapTable[DXE_EXMAPPING_TABLE_SIZE];
//   UINT32             LocalTokenNumberTable[DXE_LOCAL_TOKEN_NUMBER_TABLE_SIZE];
//   GUID               GuidTable[DXE_GUID_TABLE_SIZE];
//   STRING_HEAD        PcdWinNtMemorySize_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
//   STRING_HEAD        PcdWinNtGop_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
//   STRING_HEAD        PcdWinNtSerialPort_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
//   STRING_HEAD        PcdWinNtVirtualDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
//   STRING_HEAD        PcdWinNtUga_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
//   STRING_HEAD        PcdWinNtPhysicalDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
//   STRING_HEAD        PcdWinNtFileSystem_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
//   STRING_HEAD        PcdWinNtConsole_0d79a645_1d91_40a6_a81f_61e6982b32b4[1];
// 
// 
//   UINT16             StringTable[6]; /* PcdWinNtMemorySize_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//   UINT16             StringTable_1[26]; /* PcdWinNtGop_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//   UINT16             StringTable_2[10]; /* PcdWinNtSerialPort_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//   UINT16             StringTable_3[13]; /* PcdWinNtVirtualDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//   UINT16             StringTable_4[26]; /* PcdWinNtUga_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//   UINT16             StringTable_5[51]; /* PcdWinNtPhysicalDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//   UINT16             StringTable_6[54]; /* PcdWinNtFileSystem_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//   UINT16             StringTable_7[26]; /* PcdWinNtConsole_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
// 
//   SIZE_INFO          SizeTable[DXE_SIZE_TABLE_SIZE];
//   UINT16             PcdPlatformBootTimeOutDefault_a1aff049_fdeb_442a_b320_13ab4cb72bbc[1];
// 
// 
// 
// 
// 
// 
//   UINT8              SkuIdTable[DXE_SKUID_TABLE_SIZE];
// 
// } DXE_PCD_DATABASE_INIT;
// 
// typedef struct {
//   UINT8  dummy; /* PCD_DATABASE_UNINIT is emptry */
// 
// 
// 
// 
// 
// } DXE_PCD_DATABASE_UNINIT;
// 
// #define PCD_DXE_SERVICE_DRIVER_VERSION         2
// 
// typedef struct {
//   DXE_PCD_DATABASE_INIT    Init;
//   DXE_PCD_DATABASE_UNINIT  Uninit;
// } DXE_PCD_DATABASE;
// 
// #define DXE_NEX_TOKEN_NUMBER (DXE_LOCAL_TOKEN_NUMBER - DXE_EX_TOKEN_NUMBER)
// 
// typedef struct {
//   PEI_PCD_DATABASE PeiDb;
//   DXE_PCD_DATABASE DxeDb;
// } PCD_DATABASE;
// 
// #define PCD_TOTAL_TOKEN_NUMBER (PEI_LOCAL_TOKEN_NUMBER + DXE_LOCAL_TOKEN_NUMBER)
// 
// 
// DXE_PCD_DATABASE_INIT gDXEPcdDbInit = {
// 
// 
// 
// 
//   /* VPD */
// 
//   /* ExMapTable */
//   {
//     { 0, 0, 0 },
// 
//   },
//   /* LocalTokenNumberTable */
//   {
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtMemorySize_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtGop_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtSerialPort_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtVirtualDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtUga_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtPhysicalDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtFileSystem_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdWinNtConsole_0d79a645_1d91_40a6_a81f_61e6982b32b4) | PCD_DATUM_TYPE_POINTER | PCD_TYPE_STRING,
//     offsetof(DXE_PCD_DATABASE, Init.PcdPlatformBootTimeOutDefault_a1aff049_fdeb_442a_b320_13ab4cb72bbc) | PCD_TYPE_DATA | PCD_DATUM_TYPE_UINT16,
//
//   },
//   /* GuidTable */
//   {
//     {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
// 
//   },
//  { 0 }, /* PcdWinNtMemorySize_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//  { 6 }, /* PcdWinNtGop_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//  { 32 }, /* PcdWinNtSerialPort_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//  { 42 }, /* PcdWinNtVirtualDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//  { 55 }, /* PcdWinNtUga_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//  { 81 }, /* PcdWinNtPhysicalDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//  { 132 }, /* PcdWinNtFileSystem_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//  { 186 }, /* PcdWinNtConsole_0d79a645_1d91_40a6_a81f_61e6982b32b4[1] */
//
//
// /* StringTable */
//  L"64!64", /* PcdWinNtMemorySize_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//  L"UGA Window 1!UGA Window 2", /* PcdWinNtGop_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//  L"COM1!COM2", /* PcdWinNtSerialPort_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//  L"FW;40960;512", /* PcdWinNtVirtualDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//  L"UGA Window 1!UGA Window 2", /* PcdWinNtUga_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//  L"a:RW;2880;512!d:RO;307200;2048!j:RW;262144;512", /* PcdWinNtPhysicalDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//  L".!..\\..\\..\\..\\EdkShellBinPkg\\Bin\\Ia32\\Apps", /* PcdWinNtFileSystem_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//  L"Bus Driver Console Window", /* PcdWinNtConsole_0d79a645_1d91_40a6_a81f_61e6982b32b4 */

//  /* SizeTable */
//  {
//    10, 10, /* PcdWinNtMemorySize_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//    50, 50, /* PcdWinNtGop_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//    18, 18, /* PcdWinNtSerialPort_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//    24, 24, /* PcdWinNtVirtualDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//    50, 50, /* PcdWinNtUga_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//    100, 92, /* PcdWinNtPhysicalDisk_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//    106, 98, /* PcdWinNtFileSystem_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//    50, 50, /* PcdWinNtConsole_0d79a645_1d91_40a6_a81f_61e6982b32b4 */
//
//  },
//  { 10 }, /*  PcdPlatformBootTimeOutDefault_a1aff049_fdeb_442a_b320_13ab4cb72bbc[1] */
//
//
//
//
//
//
//  /* SkuIdTable */
//  { 0,  },
//  
//};


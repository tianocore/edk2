/** @file
Private functions used by PCD DXE driver.

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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
#include <Guid/PcdDataBaseSignatureGuid.h>
#include <Protocol/Pcd.h>
#include <Protocol/PiPcd.h>
#include <Protocol/PcdInfo.h>
#include <Protocol/PiPcdInfo.h>
#include <Protocol/VarCheck.h>
#include <Protocol/VariableLock.h>
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
// Please make sure the PCD Serivce DXE Version is consistent with
// the version of the generated DXE PCD Database by build tool.
//
#define PCD_SERVICE_DXE_VERSION      5

//
// PCD_DXE_SERVICE_DRIVER_VERSION is defined in Autogen.h.
//
#if (PCD_SERVICE_DXE_VERSION != PCD_DXE_SERVICE_DRIVER_VERSION)
  #error "Please make sure the version of PCD DXE Service and the generated PCD DXE Database match."
#endif

/**
  Retrieve additional information associated with a PCD token in the default token space.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.

  @retval  EFI_SUCCESS      The PCD information was returned successfully.
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
EFIAPI
DxeGetPcdInfoGetInfo (
  IN        UINTN           TokenNumber,
  OUT       EFI_PCD_INFO    *PcdInfo
  );

/**
  Retrieve additional information associated with a PCD token.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  @param[in]    Guid        The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.

  @retval  EFI_SUCCESS      The PCD information was returned successfully.
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
EFIAPI
DxeGetPcdInfoGetInfoEx (
  IN CONST  EFI_GUID        *Guid,
  IN        UINTN           TokenNumber,
  OUT       EFI_PCD_INFO    *PcdInfo
  );

/**
  Retrieve the currently set SKU Id.

  @return   The currently set SKU Id. If the platform has not set at a SKU Id, then the
            default SKU Id value of 0 is returned. If the platform has set a SKU Id, then the currently set SKU
            Id is returned.
**/
UINTN
EFIAPI
DxeGetPcdInfoGetSku (
  VOID
  );

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
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT   UINTN                  *TokenNumber
  );

/**
  Get next token space in PCD database according to given token space guid.
  
  @param Guid            Given token space guid. If NULL, then Guid will be set to 
                         the first PCD token space in PCD database, If not NULL, then
                         Guid will be set to next PCD token space.

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
  Retrieve additional information associated with a PCD token.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  @param[in]    Guid        The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName. 

  @retval  EFI_SUCCESS      The PCD information was returned successfully
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
DxeGetPcdInfo (
  IN CONST  EFI_GUID        *Guid,
  IN        UINTN           TokenNumber,
  OUT       EFI_PCD_INFO    *PcdInfo
  );

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
  @retval EFI_INVALID_PARAMETER  If Size of non-Ptr type PCD does not match the size information in PCD database.  
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
  @param SetAttributes   Attributes bitmask to set for the variable.
  @param Data            Value want to be set.
  @param DataSize        Size of value
  @param Offset          Value offset of HII-type PCD in variable.

  @return status of GetVariable()/SetVariable().

**/
EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  IN  UINT32       SetAttributes,
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
  Get Token Number according to dynamic-ex PCD's {token space guid:token number}

  A dynamic-ex type PCD, developer must provide pair of token space guid: token number
  in DEC file. PCD database maintain a mapping table that translate pair of {token
  space guid: token number} to Token Number.
  
  @param Guid            Token space guid for dynamic-ex PCD entry.
  @param ExTokenNumber   Dynamic-ex PCD token number.

  @return Token Number for dynamic-ex PCD.

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

/**
  VariableLockProtocol callback
  to lock the variables referenced by DynamicHii PCDs with RO property set in *.dsc.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableLockCallBack (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  );

extern  PCD_DATABASE   mPcdDatabase;

extern  UINT32         mPcdTotalTokenCount; 
extern  UINT32         mPeiLocalTokenCount; 
extern  UINT32         mDxeLocalTokenCount; 
extern  UINT32         mPeiNexTokenCount;   
extern  UINT32         mDxeNexTokenCount;  
extern  UINT32         mPeiExMapppingTableSize;
extern  UINT32         mDxeExMapppingTableSize;
extern  UINT32         mPeiGuidTableSize;
extern  UINT32         mDxeGuidTableSize;

extern  BOOLEAN        mPeiExMapTableEmpty; 
extern  BOOLEAN        mDxeExMapTableEmpty; 
extern  BOOLEAN        mPeiDatabaseEmpty;

extern  EFI_GUID     **TmpTokenSpaceBuffer;
extern  UINTN          TmpTokenSpaceBufferCount;

extern EFI_LOCK mPcdDatabaseLock;

#endif


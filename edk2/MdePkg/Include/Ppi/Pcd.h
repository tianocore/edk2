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

  @param  SkuId The SKU value that will be used when the PCD service will retrieve and 
              set values associated with a PCD token.

  @retval VOID

**/
typedef 
VOID
(EFIAPI *PCD_PPI_SET_SKU) (
  IN  UINTN          SkuId
  );



/**
  Retrieves an 8-bit value for a given PCD token.

  Retrieves the current byte-sized value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param  TokenNumber The PCD token number. 

  @return The UINT8 value.
  
**/
typedef
UINT8
(EFIAPI *PCD_PPI_GET8) (
  IN UINTN             TokenNumber
  );



/**
  Retrieves an 16-bit value for a given PCD token.

  Retrieves the current 16-bits value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param  TokenNumber The PCD token number. 

  @return The UINT16 value.
  
**/
typedef
UINT16
(EFIAPI *PCD_PPI_GET16) (
  IN UINTN             TokenNumber
  );



/**
  Retrieves an 32-bit value for a given PCD token.

  Retrieves the current 32-bits value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param  TokenNumber The PCD token number. 

  @return The UINT32 value.
  
**/
typedef
UINT32
(EFIAPI *PCD_PPI_GET32) (
  IN UINTN             TokenNumber
  );



/**
  Retrieves an 64-bit value for a given PCD token.

  Retrieves the current 64-bits value for a PCD token number.  
  If the TokenNumber is invalid, the results are unpredictable.
  
  @param  TokenNumber The PCD token number. 

  @return The UINT64 value.
  
**/
typedef
UINT64
(EFIAPI *PCD_PPI_GET64) (
  IN UINTN             TokenNumber
  );



/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param  TokenNumber The PCD token number. 

  @return The pointer to the buffer to be retrived.
  
**/
typedef
VOID *
(EFIAPI *PCD_PPI_GET_POINTER) (
  IN UINTN             TokenNumber
  );



/**
  Retrieves a Boolean value for a given PCD token.

  Retrieves the current boolean value for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param  TokenNumber The PCD token number. 

  @return The Boolean value.
  
**/
typedef
BOOLEAN
(EFIAPI *PCD_PPI_GET_BOOLEAN) (
  IN UINTN             TokenNumber
  );



/**
  Retrieves the size of the value for a given PCD token.

  Retrieves the current size of a particular PCD token.  
  If the TokenNumber is invalid, the results are unpredictable.

  @param  TokenNumber The PCD token number. 

  @return The size of the value for the PCD token.
  
**/
typedef
UINTN
(EFIAPI *PCD_PPI_GET_SIZE) (
  IN UINTN             TokenNumber
  );



/**
  Retrieves an 8-bit value for a given PCD token.

  Retrieves the 8-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param  Guid The token space for the token number.
  @param  TokenNumber The PCD token number. 

  @return The size 8-bit value for the PCD token.
  
**/
typedef
UINT8
(EFIAPI *PCD_PPI_GET_EX_8) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );



/**
  Retrieves an 16-bit value for a given PCD token.

  Retrieves the 16-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param  Guid The token space for the token number.
  @param  TokenNumber The PCD token number. 

  @return The size 16-bit value for the PCD token.
  
**/
typedef
UINT16
(EFIAPI *PCD_PPI_GET_EX_16) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN              TokenNumber
  );



/**
  Retrieves an 32-bit value for a given PCD token.

  Retrieves the 32-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param  Guid The token space for the token number.
  @param  TokenNumber The PCD token number. 

  @return The size 32-bit value for the PCD token.
  
**/
typedef
UINT32
(EFIAPI *PCD_PPI_GET_EX_32) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );



/**
  Retrieves an 64-bit value for a given PCD token.

  Retrieves the 64-bit value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param  Guid The token space for the token number.
  @param  TokenNumber The PCD token number. 

  @return The size 64-bit value for the PCD token.
  
**/
typedef
UINT64
(EFIAPI *PCD_PPI_GET_EX_64) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );



/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.  
  Do not make any assumptions about the alignment of the pointer that 
  is returned by this function call.  If the TokenNumber is invalid, 
  the results are unpredictable.

  @param  Guid The token space for the token number.
  @param  TokenNumber The PCD token number. 

  @return The pointer to the buffer to be retrived.
  
**/
typedef
VOID *
(EFIAPI *PCD_PPI_GET_EX_POINTER) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );



/**
  Retrieves an Boolean value for a given PCD token.

  Retrieves the Boolean value of a particular PCD token.  
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are 
  unpredictable.

  @param  Guid The token space for the token number.
  @param  TokenNumber The PCD token number. 

  @return The size Boolean value for the PCD token.
  
**/
typedef
BOOLEAN
(EFIAPI *PCD_PPI_GET_EX_BOOLEAN) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );



/**
  Retrieves the size of the value for a given PCD token.

  Retrieves the current size of a particular PCD token.  
  If the TokenNumber is invalid, the results are unpredictable.

  @param  Guid The token space for the token number.
  @param  TokenNumber The PCD token number. 

  @return The size of the value for the PCD token.
  
**/
typedef
UINTN
(EFIAPI *PCD_PPI_GET_EX_SIZE) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber
  );



/**
  Sets an 8-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET8) (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );



/**
  Sets an 16-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET16) (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  );



/**
  Sets an 32-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET32) (
  IN UINTN             TokenNumber,
  IN UINT32            Value
  );



/**
  Sets an 64-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET64) (
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );




/**
  Sets a value of a specified size for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  TokenNumber The PCD token number. 
  @param SizeOfBuffer A pointer to the length of the value being set for the PCD token.  
                              On input, if the SizeOfValue is greater than the maximum size supported 
                              for this TokenNumber then the output value of SizeOfValue will reflect 
                              the maximum size supported for this TokenNumber.
  @param  Buffer The buffer to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_POINTER) (
  IN        UINTN             TokenNumber,
  IN OUT    UINTN             *SizeOfBuffer,
  IN        VOID              *Buffer
  );



/**
  Sets an Boolean value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_BOOLEAN) (
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  );



/**
  Sets an 8-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_8) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT8             Value
  );



/**
  Sets an 16-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_16) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT16            Value
  );



/**
  Sets an 32-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_32) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT32            Value
  );



/**
  Sets an 64-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_64) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT64            Value
  );



/**
  Sets a value of a specified size for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the 
  size of the value being set is compatible with the Token's existing definition.  
  If it is not, an error will be returned.

  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  TokenNumber The PCD token number. 
  @param SizeOfBuffer A pointer to the length of the value being set for the PCD token.  
                              On input, if the SizeOfValue is greater than the maximum size supported 
                              for this TokenNumber then the output value of SizeOfValue will reflect 
                              the maximum size supported for this TokenNumber.
  @param  Buffer The buffer to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_POINTER) (
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

  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  TokenNumber The PCD token number. 
  @param  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data 
                                  being set was incompatible with a call to this function.  
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.
  
**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_SET_EX_BOOLEAN) (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN BOOLEAN           Value
  );



/**
  Callback on SET function prototype definition.

  @param  CallBackGuid The PCD token GUID being set.
  @param  CallBackToken The PCD token number being set.
  @param  TokenData A pointer to the token data being set.
  @param  TokenDataSize The size, in bytes, of the data being set.

  @retval VOID

**/
typedef
VOID
(EFIAPI *PCD_PPI_CALLBACK) (
  IN      CONST EFI_GUID   *CallBackGuid, OPTIONAL
  IN      UINTN            CallBackToken,
  IN  OUT VOID             *TokenData,
  IN      UINTN            TokenDataSize
  );



/**
  Specifies a function to be called anytime the value of a designated token is changed.

  @param  TokenNumber The PCD token number. 
  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  CallBackFunction The function prototype called when the value associated with the CallBackToken is set.  

  @retval EFI_SUCCESS  The PCD service has successfully established a call event 
                        for the CallBackToken requested.
  @retval EFI_NOT_FOUND The PCD service could not find the referenced token number.

**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_CALLBACK_ONSET) (
  IN  UINTN                  TokenNumber,
  IN  CONST EFI_GUID         *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK       CallBackFunction
  );



/**
  Cancels a previously set callback function for a particular PCD token number.

  @param  TokenNumber The PCD token number. 
  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  CallBackFunction The function prototype called when the value associated with the CallBackToken is set.  

  @retval EFI_SUCCESS  The PCD service has successfully established a call event 
                        for the CallBackToken requested.
  @retval EFI_NOT_FOUND The PCD service could not find the referenced token number.

**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_CANCEL_CALLBACK) (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK        CallBackFunction
  );



/**
  Retrieves the next valid PCD token for a given namespace.

  @param  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param  TokenNumber A pointer to the PCD token number to use to find the subsequent token number.  
                              To retrieve the "first" token, have the pointer reference a TokenNumber value of 0.

  @retval EFI_SUCCESS  The PCD service retrieved the value requested.
  @retval EFI_NOT_FOUND The PCD service could not find data from the requested token number.

**/
typedef 
EFI_STATUS
(EFIAPI *PCD_PPI_GET_NEXT_TOKEN) (
  IN CONST EFI_GUID           *Guid, OPTIONAL
  IN OUT  UINTN               *TokenNumber
  );



/**
  Retrieves the next valid PCD token namespace for a given namespace.

  @param  Guid An indirect pointer to EFI_GUID.  On input it designates a known 
                    token namespace from which the search will start. On output, 
                    it designates the next valid token namespace on the platform. 
                    If *Guid is NULL, then the GUID of the first token space of the current platform is returned.
                    If this input token namespace is the last tokenspace on the platform,
                    *Guid will be assigned to NULL and the function return EFI_SUCCESS.
                    If the search cannot locate the input token namespace, an error is returned and 
                    the value of *Guid is undefined. 

  @retval EFI_SUCCESS  The PCD service retrieved the value requested.
  @retval EFI_NOT_FOUND The PCD service could not find the input token namespace.

**/
typedef
EFI_STATUS
(EFIAPI *PCD_PPI_GET_NEXT_TOKENSPACE) (
  IN OUT CONST EFI_GUID         **Guid
  );



//
// Interface structure for the PCD PPI
//
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

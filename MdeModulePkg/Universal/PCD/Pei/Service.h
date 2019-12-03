/** @file
  The internal header file declares the private functions used by PeiPcd driver.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PEI_PCD_SERVICE_H_
#define _PEI_PCD_SERVICE_H_

#include <PiPei.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/Pcd.h>
#include <Ppi/PiPcd.h>
#include <Ppi/PcdInfo.h>
#include <Ppi/PiPcdInfo.h>
#include <Guid/PcdDataBaseHobGuid.h>
#include <Guid/PcdDataBaseSignatureGuid.h>
#include <Guid/VariableFormat.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

//
// Please make sure the PCD Serivce PEIM Version is consistent with
// the version of the generated PEIM PCD Database by build tool.
//
#define PCD_SERVICE_PEIM_VERSION      7

//
// PCD_PEI_SERVICE_DRIVER_VERSION is defined in Autogen.h.
//
#if (PCD_SERVICE_PEIM_VERSION != PCD_PEI_SERVICE_DRIVER_VERSION)
  #error "Please make sure the version of PCD PEIM Service and the generated PCD PEI Database match."
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
PeiGetPcdInfoGetInfo (
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
PeiGetPcdInfoGetInfoEx (
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
PeiGetPcdInfoGetSku (
  VOID
  );

//
// PPI Interface Implementation Declaration.
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
PeiPcdSetSku (
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
PeiPcdGet8 (
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
PeiPcdGet16 (
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
PeiPcdGet32 (
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
PeiPcdGet64 (
  IN UINTN             TokenNumber
  );

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
PeiPcdGetPtr (
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
PeiPcdGetBool (
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
PeiPcdGetSize (
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
PeiPcdGet8Ex (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdGet16Ex (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdGet32Ex (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdGet64Ex (
  IN CONST EFI_GUID    *Guid,
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

  @return The pointer to the buffer to be retrived.

**/
VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdGetBoolEx (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdGetSizeEx (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdSet8 (
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
PeiPcdSet16 (
  IN UINTN             TokenNumber,
  IN UINT16            Value
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
PeiPcdSet32 (
  IN UINTN             TokenNumber,
  IN UINT32            Value
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
PeiPcdSet64 (
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
PeiPcdSetPtr (
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
PeiPcdSetBool (
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
PeiPcdSet8Ex (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdSet16Ex (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdSet32Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             TokenNumber,
  IN UINT32            Value
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
PeiPcdSet64Ex (
  IN CONST EFI_GUID    *Guid,
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
PeiPcdSetPtrEx (
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
PeiPcdSetBoolEx (
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
PeiRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PPI_CALLBACK        CallBackFunction
  );

/**
  Cancels a previously set callback function for a particular PCD token number.

  @param [in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param [in]  TokenNumber The PCD token number.
  @param [in]  CallBackFunction The function prototype called when the value associated with the CallBackToken is set.

  @retval EFI_SUCCESS  The PCD service has successfully established a call event
                        for the CallBackToken requested.
  @retval EFI_NOT_FOUND The PCD service could not find the referenced token number.

**/
EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PPI_CALLBACK        CallBackFunction
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


  @param[in]       Guid        The 128-bit unique value that designates the namespace from which to extract the value.
                               This is an optional parameter that may be NULL.  If this parameter is NULL, then a request
                               is being made to retrieve tokens from the default token space.
  @param[in, out]  TokenNumber A pointer to the PCD token number to use to find the subsequent token number.

  @retval EFI_SUCCESS   The PCD service has retrieved the next valid token number.
                        Or the input token number is already the last valid token number in the PCD database.
                        In the later case, *TokenNumber is updated with the value of 0.
  @retval EFI_NOT_FOUND If this input token number and token namespace does not exist on the platform.

**/
EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID           *Guid, OPTIONAL
  IN OUT  UINTN               *TokenNumber
  );

/**
  Retrieves the next valid PCD token namespace for a given namespace.

  @param[in, out]  Guid An indirect pointer to EFI_GUID.  On input it designates
                    a known token namespace from which the search will start. On output,
                    it designates the next valid token namespace on the platform. If the input
                    token namespace does not exist on the platform, an error is returned and
                    the value of *Guid is undefined. If *Guid is NULL, then the GUID of the
                    first token space of the current platform is assigned to *Guid the function
                    return EFI_SUCCESS. If  *Guid is NULL  and there is no namespace exist in
                    the platform other than the default (NULL) tokennamespace, *Guid is unchanged
                    and the function return EFI_SUCCESS. If this input token namespace is the last
                    namespace on the platform, *Guid will be assigned to NULL and the function return
                    EFI_SUCCESS.

  @retval EFI_SUCCESS  The PCD service retrieved the next valid token space Guid.
                        Or the input token space Guid is already the last valid token space Guid
                        in the PCD database. In the later case, *Guid is updated with the value of NULL.
  @retval EFI_NOT_FOUND If the input token namespace does not exist on the platform.

**/
EFI_STATUS
EFIAPI
PeiPcdGetNextTokenSpace (
  IN OUT CONST EFI_GUID           **Guid
  );

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
PeiGetPcdInfo (
  IN CONST  EFI_GUID        *Guid,
  IN        UINTN           TokenNumber,
  OUT       EFI_PCD_INFO    *PcdInfo
  );

/* Internal Function definitions */
/**
  Get PCD database from GUID HOB in PEI phase.

  @return Pointer to PCD database.

**/
PEI_PCD_DATABASE *
GetPcdDatabase (
  VOID
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
  IN          UINTN              TokenNumber,
  IN          VOID               *Data,
  IN          UINTN              Size
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
  IN          UINTN              TokenNumber,
  IN          VOID               *Data,
  IN OUT      UINTN              *Size,
  IN          BOOLEAN            PtrType
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
  IN          UINTN                Size
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
  IN UINTN                TokenNumber,
  IN UINTN                GetSize
  );

/**
  Wrapper function for get PCD value for dynamic-ex PCD.

  @param Guid            Token space guid for dynamic-ex PCD.
  @param ExTokenNumber   Token number for dyanmic-ex PCD.
  @param GetSize         The size of dynamic-ex PCD value.

  @return PCD entry in PCD database.

**/
VOID *
ExGetWorker (
  IN CONST EFI_GUID   *Guid,
  IN UINTN            ExTokenNumber,
  IN UINTN            GetSize
  );

typedef struct {
  UINTN   TokenNumber;
  UINTN   Size;
  UINT32  LocalTokenNumberAlias;
} EX_PCD_ENTRY_ATTRIBUTE;

/**
  Get Token Number according to dynamic-ex PCD's {token space guid:token number}

  A dynamic-ex type PCD, developer must provide pair of token space guid: token number
  in DEC file. PCD database maintain a mapping table that translate pair of {token
  space guid: token number} to Token Number.

  @param Guid            Token space guid for dynamic-ex PCD entry.
  @param ExTokenNumber   Token number for dynamic-ex PCD.

  @return Token Number for dynamic-ex PCD.

**/
UINTN
GetExPcdTokenNumber (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  );

/**
  The function registers the CallBackOnSet fucntion
  according to TokenNumber and EFI_GUID space.

  @param  TokenNumber       The token number.
  @param  Guid              The GUID space.
  @param  CallBackFunction  The Callback function to be registered.
  @param  Register          To register or unregister the callback function.

  @retval EFI_SUCCESS If the Callback function is registered.
  @retval EFI_NOT_FOUND If the PCD Entry is not found according to Token Number and GUID space.
  @retval EFI_OUT_OF_RESOURCES If the callback function can't be registered because there is not free
                                slot left in the CallbackFnTable.
**/
EFI_STATUS
PeiRegisterCallBackWorker (
  IN  UINTN              TokenNumber,
  IN  CONST EFI_GUID         *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK   CallBackFunction,
  IN  BOOLEAN            Register
  );

/**
  The function builds the PCD database.

  @param  FileHandle  Handle of the file the external PCD database binary located.

  @return Pointer to PCD database.

**/
PEI_PCD_DATABASE *
BuildPcdDatabase (
  IN EFI_PEI_FILE_HANDLE    FileHandle
  );

/**
  Get index of PCD entry in size table.

  @param LocalTokenNumberTableIdx Index of this PCD in local token number table.
  @param Database                 Pointer to PCD database.

  @return index of PCD entry in size table.

**/
UINTN
GetSizeTableIndex (
  IN    UINTN             LocalTokenNumberTableIdx,
  IN    PEI_PCD_DATABASE  *Database
  );

/**
  Get PCD value's size for POINTER type PCD.

  The POINTER type PCD's value will be stored into a buffer in specificed size.
  The max size of this PCD's value is described in PCD's definition in DEC file.

  @param LocalTokenNumberTableIdx Index of PCD token number in PCD token table
  @param MaxSize                  Maxmium size of PCD's value
  @param Database                 Pcd database in PEI phase.

  @return PCD value's size for POINTER type PCD.

**/
UINTN
GetPtrTypeSize (
  IN    UINTN             LocalTokenNumberTableIdx,
  OUT   UINTN             *MaxSize,
  IN    PEI_PCD_DATABASE  *Database
  );

/**
  Set PCD value's size for POINTER type PCD.

  The POINTER type PCD's value will be stored into a buffer in specificed size.
  The max size of this PCD's value is described in PCD's definition in DEC file.

  @param LocalTokenNumberTableIdx Index of PCD token number in PCD token table
  @param CurrentSize              Maxmium size of PCD's value
  @param Database                 Pcd database in PEI phase.

  @retval TRUE  Success to set PCD's value size, which is not exceed maxmium size
  @retval FALSE Fail to set PCD's value size, which maybe exceed maxmium size

**/
BOOLEAN
SetPtrTypeSize (
  IN          UINTN             LocalTokenNumberTableIdx,
  IN    OUT   UINTN             *CurrentSize,
  IN          PEI_PCD_DATABASE  *Database
  );

#endif


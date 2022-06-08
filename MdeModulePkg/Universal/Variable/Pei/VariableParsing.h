/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by PeiVariable module.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEI_VARIABLE_PARSING_H_
#define PEI_VARIABLE_PARSING_H_

#include "Variable.h"

/**

  Gets the pointer to the first variable header in given variable store area.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  );

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  );

/**
  This code checks if variable header is valid or not.

  @param[in]  Variable  Pointer to the Variable Header.

  @retval TRUE      Variable header is valid.
  @retval FALSE     Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER  *Variable
  );

/**
  This code gets the pointer to the next variable header.

  @param[in]  StoreInfo         Pointer to variable store info structure.
  @param[in]  Variable          Pointer to the Variable Header.
  @param[in]  VariableHeader    Pointer to the Variable Header that has consecutive content.

  @return  A VARIABLE_HEADER* pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_STORE_INFO  *StoreInfo,
  IN  VARIABLE_HEADER      *Variable,
  IN  VARIABLE_HEADER      *VariableHeader
  );

/**
  This code gets the pointer to the variable guid.

  @param[in]  Variable   Pointer to the Variable Header.
  @param[in]  AuthFlag   Authenticated variable flag.

  @return A EFI_GUID* pointer to Vendor Guid.

**/
EFI_GUID *
GetVendorGuidPtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          AuthFlag
  );

/**
  This code gets the pointer to the variable name.

  @param[in]   Variable  Pointer to the Variable Header.
  @param[in]   AuthFlag  Authenticated variable flag.

  @return  A CHAR16* pointer to Variable Name.

**/
CHAR16 *
GetVariableNamePtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          AuthFlag
  );

/**
  This code gets the size of name of variable.

  @param[in]  Variable  Pointer to the Variable Header.
  @param[in]  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  );

/**
  This code gets the size of data of variable.

  @param[in]  Variable  Pointer to the Variable Header.
  @param[in]  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  );

/**
  This code gets the pointer to the variable data.

  @param[in]   Variable         Pointer to the Variable Header.
  @param[in]   VariableHeader   Pointer to the Variable Header that has consecutive content.
  @param[in]   AuthFlag         Authenticated variable flag.

  @return  A UINT8* pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  VARIABLE_HEADER  *VariableHeader,
  IN  BOOLEAN          AuthFlag
  );

/**
  Get variable header that has consecutive content.

  @param[in]  StoreInfo      Pointer to variable store info structure.
  @param[in]  Variable       Pointer to the Variable Header.
  @param[out] VariableHeader Pointer to Pointer to the Variable Header that has consecutive content.

  @retval TRUE          Variable header is valid.
  @retval FALSE         Variable header is not valid.

**/
BOOLEAN
GetVariableHeader (
  IN VARIABLE_STORE_INFO  *StoreInfo,
  IN VARIABLE_HEADER      *Variable,
  OUT VARIABLE_HEADER     **VariableHeader
  );

/**
  This code gets the size of variable header.

  @param[in] AuthFlag   Authenticated variable flag.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  IN  BOOLEAN  AuthFlag
  );

/**
  Get variable name or data to output buffer.

  @param[in]  StoreInfo     Pointer to variable store info structure.
  @param[in]  NameOrData    Pointer to the variable name/data that may be inconsecutive.
  @param[in]  Size          Variable name/data size.
  @param[out] Buffer        Pointer to output buffer to hold the variable name/data.

**/
VOID
GetVariableNameOrData (
  IN VARIABLE_STORE_INFO  *StoreInfo,
  IN UINT8                *NameOrData,
  IN UINTN                Size,
  OUT UINT8               *Buffer
  );

/**
  This function compares a variable with variable entries in database.

  @param[in]  StoreInfo     Pointer to variable store info structure.
  @param[in]  Variable      Pointer to the variable in our database
  @param[in]  VariableHeader Pointer to the Variable Header that has consecutive content.
  @param[in]  VariableName  Name of the variable to compare to 'Variable'
  @param[in]  VendorGuid    GUID of the variable to compare to 'Variable'
  @param[out] PtrTrack      Variable Track Pointer structure that contains Variable Information.

  @retval EFI_SUCCESS    Found match variable
  @retval EFI_NOT_FOUND  Variable not found

**/
EFI_STATUS
CompareWithValidVariable (
  IN  VARIABLE_STORE_INFO     *StoreInfo,
  IN  VARIABLE_HEADER         *Variable,
  IN  VARIABLE_HEADER         *VariableHeader,
  IN  CONST CHAR16            *VariableName,
  IN  CONST EFI_GUID          *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack
  );

/**

  Retrieve details of the variable next to given variable within VariableStore.

  If VariableInfo->StoreIndex is invalid, the first one in VariableStore is returned.

  @param[in,out] VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo or VariableStore is NULL.
  @retval EFI_NOT_FOUND          If the end of VariableStore is reached.
  @retval EFI_SUCCESS            The next variable is retrieved successfully.

**/
EFI_STATUS
EFIAPI
GetNextVariableInfo (
  IN  OUT PROTECTED_VARIABLE_INFO  *VariableInfo
  );

/**

  Retrieve details about a variable, given by VariableInfo->Buffer or
  VariableInfo->Index, and pass the details back in VariableInfo->Header.

  This function is used to resolve the variable data structure into
  VariableInfo->Header, for easier access later without revisiting the variable
  data in variable store. If pointers in the structure of VariableInfo->Header
  are not NULL, it's supposed that they are buffers passed in to hold a copy of
  data of corresponding data fields in variable data structure. Otherwise, this
  function simply returns pointers pointing to address of those data fields.

  The variable is specified by either VariableInfo->Index or VariableInfo->Buffer.
  If VariableInfo->Index is given, this function finds the corresponding variable
  first from variable storage according to the Index.

  If both VariableInfo->Index and VariableInfo->Buffer are given, it's supposed
  that VariableInfo->Buffer is a buffer passed in to hold a whole copy of
  requested variable data to be returned.

  @param[in,out] VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo is NULL or both VariableInfo->Buffer
                                 and VariableInfo->Index are NULL (0).
  @retval EFI_NOT_FOUND          If given Buffer or Index is out of range of
                                 any given or internal storage copies.
  @retval EFI_SUCCESS            Variable details are retrieved successfully.

**/
EFI_STATUS
EFIAPI
GetVariableInfo (
  IN  OUT PROTECTED_VARIABLE_INFO  *VariableInfo
  );

/**

  Find variable specified with input parameters.

  @param[in] StoreInfo             Pointer to variable information.
  @param[in] VariableName          Pointer to variable name.
  @param[in] VendorGuid            Pointer to variable GUID.
  @param[in] PtrTrack              Pointer to variable track.

  @retval EFI_INVALID_PARAMETER  VariableInfo is NULL or both VariableInfo->Address
                                 and VariableInfo->Offset are NULL (0).
  @retval EFI_NOT_FOUND          If given Address or Offset is out of range of
                                 any given or internal storage copies.
  @retval EFI_SUCCESS            Variable details are retrieved successfully.

**/
EFI_STATUS
FindVariableEx (
  IN VARIABLE_STORE_INFO      *StoreInfo,
  IN CONST CHAR16             *VariableName,
  IN CONST EFI_GUID           *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack
  );

#endif

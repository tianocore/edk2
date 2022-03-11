/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by PeiVariable module.

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PEI_VARIABLE_PARSING_H_
#define _PEI_VARIABLE_PARSING_H_

#include "Variable.h"

/**

  Gets the pointer to the first variable header in given variable store area.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
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
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  );

/**
  This code checks if variable header is valid or not.

  @param  Variable  Pointer to the Variable Header.

  @retval TRUE      Variable header is valid.
  @retval FALSE     Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER   *Variable
  );

/**
  This code gets the pointer to the next variable header.

  @param  StoreInfo         Pointer to variable store info structure.
  @param  Variable          Pointer to the Variable Header.
  @param  VariableHeader    Pointer to the Variable Header that has consecutive content.

  @return  A VARIABLE_HEADER* pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_STORE_INFO   *StoreInfo,
  IN  VARIABLE_HEADER       *Variable,
  IN  VARIABLE_HEADER       *VariableHeader
  );

/**
  This code gets the pointer to the variable guid.

  @param Variable   Pointer to the Variable Header.
  @param AuthFlag   Authenticated variable flag.

  @return A EFI_GUID* pointer to Vendor Guid.

**/
EFI_GUID *
GetVendorGuidPtr (
  IN VARIABLE_HEADER    *Variable,
  IN BOOLEAN            AuthFlag
  );

/**
  This code gets the pointer to the variable name.

  @param   Variable  Pointer to the Variable Header.
  @param   AuthFlag  Authenticated variable flag.

  @return  A CHAR16* pointer to Variable Name.

**/
CHAR16 *
GetVariableNamePtr (
  IN VARIABLE_HEADER    *Variable,
  IN BOOLEAN            AuthFlag
  );

/**
  This code gets the size of name of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable,
  IN  BOOLEAN           AuthFlag
  );

/**
  This code gets the size of data of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable,
  IN  BOOLEAN           AuthFlag
  );

/**
  This code gets the pointer to the variable data.

  @param   Variable         Pointer to the Variable Header.
  @param   VariableHeader   Pointer to the Variable Header that has consecutive content.
  @param   AuthFlag         Authenticated variable flag.

  @return  A UINT8* pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable,
  IN  VARIABLE_HEADER   *VariableHeader,
  IN  BOOLEAN           AuthFlag
  );

/**
  Get variable header that has consecutive content.

  @param StoreInfo      Pointer to variable store info structure.
  @param Variable       Pointer to the Variable Header.
  @param VariableHeader Pointer to Pointer to the Variable Header that has consecutive content.

  @retval TRUE          Variable header is valid.
  @retval FALSE         Variable header is not valid.

**/
BOOLEAN
GetVariableHeader (
  IN VARIABLE_STORE_INFO    *StoreInfo,
  IN VARIABLE_HEADER        *Variable,
  OUT VARIABLE_HEADER       **VariableHeader
  );

/**
  This code gets the size of variable header.

  @param AuthFlag   Authenticated variable flag.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  IN  BOOLEAN       AuthFlag
  );

/**
  Get variable name or data to output buffer.

  @param  StoreInfo     Pointer to variable store info structure.
  @param  NameOrData    Pointer to the variable name/data that may be inconsecutive.
  @param  Size          Variable name/data size.
  @param  Buffer        Pointer to output buffer to hold the variable name/data.

**/
VOID
GetVariableNameOrData (
  IN VARIABLE_STORE_INFO    *StoreInfo,
  IN UINT8                  *NameOrData,
  IN UINTN                  Size,
  OUT UINT8                 *Buffer
  );

/**
  This function compares a variable with variable entries in database.

  @param  StoreInfo     Pointer to variable store info structure.
  @param  Variable      Pointer to the variable in our database
  @param  VariableHeader Pointer to the Variable Header that has consecutive content.
  @param  VariableName  Name of the variable to compare to 'Variable'
  @param  VendorGuid    GUID of the variable to compare to 'Variable'
  @param  PtrTrack      Variable Track Pointer structure that contains Variable Information.

  @retval EFI_SUCCESS    Found match variable
  @retval EFI_NOT_FOUND  Variable not found

**/
EFI_STATUS
CompareWithValidVariable (
  IN  VARIABLE_STORE_INFO           *StoreInfo,
  IN  VARIABLE_HEADER               *Variable,
  IN  VARIABLE_HEADER               *VariableHeader,
  IN  CONST CHAR16                  *VariableName,
  IN  CONST EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK        *PtrTrack
  );

/**

  Retrieve details of the variable next to given variable within VariableStore.

  If VarInfo->Address is NULL, the first one in VariableStore is returned.

  VariableStart and/or VariableEnd can be given optionally for the situation
  in which the valid storage space is smaller than the VariableStore->Size.
  This usually happens when PEI variable services make a compact variable
  cache to save memory, which cannot make use VariableStore->Size to determine
  the correct variable storage range.

  @param VariableStore            Pointer to a variable storage. It's optional.
  @param VariableStart            Start point of valid range in VariableStore.
  @param VariableEnd              End point of valid range in VariableStore.
  @param VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo or VariableStore is NULL.
  @retval EFI_NOT_FOUND          If the end of VariableStore is reached.
  @retval EFI_SUCCESS            The next variable is retrieved successfully.

**/
EFI_STATUS
EFIAPI
GetNextVariableInfo (
  IN  OUT PROTECTED_VARIABLE_INFO   *VariableInfo
  );

/**

  Retrieve details about a variable and return them in VariableInfo->Header.

  If VariableInfo->Address is given, this function will calculate its offset
  relative to given variable storage via VariableStore; Otherwise, it will try
  other internal variable storages or cached copies. It's assumed that, for all
  copies of NV variable storage, all variables are stored in the same relative
  position. If VariableInfo->Address is found in the range of any storage copies,
  its offset relative to that storage should be the same in other copies.

  If VariableInfo->Offset is given (non-zero) but not VariableInfo->Address,
  this function will return the variable memory address inside VariableStore,
  if given, via VariableInfo->Address; Otherwise, the address of other storage
  copies will be returned, if any.

  For a new variable whose offset has not been determined, a value of -1 as
  VariableInfo->Offset should be passed to skip the offset calculation.

  @param VariableStore            Pointer to a variable storage. It's optional.
  @param VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo is NULL or both VariableInfo->Address
                                 and VariableInfo->Offset are NULL (0).
  @retval EFI_NOT_FOUND          If given Address or Offset is out of range of
                                 any given or internal storage copies.
  @retval EFI_SUCCESS            Variable details are retrieved successfully.

**/
EFI_STATUS
EFIAPI
GetVariableInfo (
  IN  OUT PROTECTED_VARIABLE_INFO   *VariableInfo
  );

EFI_STATUS
FindVariableEx (
  IN VARIABLE_STORE_INFO         *StoreInfo,
  IN CONST CHAR16                *VariableName,
  IN CONST EFI_GUID              *VendorGuid,
  OUT VARIABLE_POINTER_TRACK     *PtrTrack
  );

#endif

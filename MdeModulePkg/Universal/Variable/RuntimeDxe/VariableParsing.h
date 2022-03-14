/** @file
  Functions in this module are associated with variable parsing operations and
  are intended to be usable across variable driver source files.

Copyright (c) 2019 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_PARSING_H_
#define _VARIABLE_PARSING_H_

#include <Guid/ImageAuthentication.h>
#include "Variable.h"

/**

  This code checks if variable header is valid or not.

  @param[in] Variable           Pointer to the Variable Header.
  @param[in] VariableStoreEnd   Pointer to the Variable Store End.
  @param[in] AuthFormat         Auth-variable indicator.

  @retval TRUE              Variable header is valid.
  @retval FALSE             Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER  *Variable,
  IN  VARIABLE_HEADER  *VariableStoreEnd,
  IN  BOOLEAN          AuthFormat
  );

/**

  This code gets the current status of Variable Store.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @retval EfiRaw         Variable store status is raw.
  @retval EfiValid       Variable store status is valid.
  @retval EfiInvalid     Variable store status is invalid.

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  );

/**
  This code gets the size of variable header.

  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  IN  BOOLEAN  AuthFormat
  );

/**

  This code gets the size of name of variable.

  @param[in]  Variable      Pointer to the variable header.
  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

  @return UINTN          Size of variable in bytes.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  );

/**
  This code sets the size of name of variable.

  @param[in]  Variable      Pointer to the Variable Header.
  @param[in]  NameSize      Name size to set.
  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

**/
VOID
SetNameSizeOfVariable (
  IN VARIABLE_HEADER  *Variable,
  IN UINTN            NameSize,
  IN BOOLEAN          AuthFormat
  );

/**

  This code gets the size of variable data.

  @param[in]  Variable      Pointer to the Variable Header.
  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

  @return Size of variable in bytes.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  );

/**
  This code sets the size of variable data.

  @param[in] Variable   Pointer to the Variable Header.
  @param[in] DataSize   Data size to set.
  @param[in] AuthFormat TRUE indicates authenticated variables are used.
                        FALSE indicates authenticated variables are not used.

**/
VOID
SetDataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  UINTN            DataSize,
  IN  BOOLEAN          AuthFormat
  );

/**

  This code gets the pointer to the variable name.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Pointer to Variable Name which is Unicode encoding.

**/
CHAR16 *
GetVariableNamePtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  );

/**
  This code gets the pointer to the variable guid.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return A EFI_GUID* pointer to Vendor Guid.

**/
EFI_GUID *
GetVendorGuidPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  );

/**

  This code gets the pointer to the variable data.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  );

/**
  This code gets the variable data offset related to variable header.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Variable Data offset.

**/
UINTN
GetVariableDataOffset (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  );

/**
  Get variable data payload.

  @param[in]      Variable     Pointer to the Variable Header.
  @param[out]     Data         Pointer to buffer used to store the variable data.
  @param[in]      DataSize     Size of buffer passed by Data.
  @param[out]     DataSize     Size of data copied into Data buffer.
  @param[in]      AuthFlag     Auth-variable indicator.

  @return EFI_SUCCESS             Data was fetched.
  @return EFI_INVALID_PARAMETER   DataSize is NULL.
  @return EFI_BUFFER_TOO_SMALL    DataSize is smaller than size of variable data.

**/
EFI_STATUS
GetVariableData (
  IN      VARIABLE_HEADER  *Variable,
  IN  OUT VOID             *Data,
  IN  OUT UINT32           *DataSize,
  IN      BOOLEAN          AuthFlag
  );

/**

  This code gets the pointer to the next variable header.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  );

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
  Compare two EFI_TIME data.


  @param[in] FirstTime       A pointer to the first EFI_TIME data.
  @param[in] SecondTime      A pointer to the second EFI_TIME data.

  @retval  TRUE              The FirstTime is not later than the SecondTime.
  @retval  FALSE             The FirstTime is later than the SecondTime.

**/
BOOLEAN
VariableCompareTimeStampInternal (
  IN EFI_TIME  *FirstTime,
  IN EFI_TIME  *SecondTime
  );

/**
  Find the variable in the specified variable store.

  @param[in]       VariableName        Name of the variable to be found
  @param[in]       VendorGuid          Vendor GUID to be found.
  @param[in]       IgnoreRtCheck       Ignore EFI_VARIABLE_RUNTIME_ACCESS attribute
                                       check at runtime when searching variable.
  @param[in, out]  PtrTrack            Variable Track Pointer structure that contains Variable Information.
  @param[in]       AuthFormat          TRUE indicates authenticated variables are used.
                                       FALSE indicates authenticated variables are not used.

  @retval          EFI_SUCCESS         Variable found successfully
  @retval          EFI_NOT_FOUND       Variable not found
**/
EFI_STATUS
FindVariableEx (
  IN     CHAR16                  *VariableName,
  IN     EFI_GUID                *VendorGuid,
  IN     BOOLEAN                 IgnoreRtCheck,
  IN OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN     BOOLEAN                 AuthFormat
  );

/**
  This code finds the next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param[in]  VariableName      Pointer to variable name.
  @param[in]  VendorGuid        Variable Vendor Guid.
  @param[in]  VariableStoreList A list of variable stores that should be used to get the next variable.
                                The maximum number of entries is the max value of VARIABLE_STORE_TYPE.
  @param[out] VariablePtr       Pointer to variable header address.
  @param[in]  AuthFormat        TRUE indicates authenticated variables are used.
                                FALSE indicates authenticated variables are not used.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The next variable was not found.
  @retval EFI_INVALID_PARAMETER If VariableName is not an empty string, while VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER The input values of VariableName and VendorGuid are not a name and
                                GUID of an existing variable.

**/
EFI_STATUS
EFIAPI
VariableServiceGetNextVariableInternal (
  IN  CHAR16                 *VariableName,
  IN  EFI_GUID               *VendorGuid,
  IN  VARIABLE_STORE_HEADER  **VariableStoreList,
  OUT VARIABLE_HEADER        **VariablePtr,
  IN  BOOLEAN                AuthFormat
  );

/**
  Routine used to track statistical information about variable usage.
  The data is stored in the EFI system table so it can be accessed later.
  VariableInfo.efi can dump out the table. Only Boot Services variable
  accesses are tracked by this code. The PcdVariableCollectStatistics
  build flag controls if this feature is enabled.

  A read that hits in the cache will have Read and Cache true for
  the transaction. Data is allocated by this routine, but never
  freed.

  @param[in]      VariableName   Name of the Variable to track.
  @param[in]      VendorGuid     Guid of the Variable to track.
  @param[in]      Volatile       TRUE if volatile FALSE if non-volatile.
  @param[in]      Read           TRUE if GetVariable() was called.
  @param[in]      Write          TRUE if SetVariable() was called.
  @param[in]      Delete         TRUE if deleted via SetVariable().
  @param[in]      Cache          TRUE for a cache hit.
  @param[in,out]  VariableInfo   Pointer to a pointer of VARIABLE_INFO_ENTRY structures.

**/
VOID
UpdateVariableInfo (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  IN  BOOLEAN                 Volatile,
  IN  BOOLEAN                 Read,
  IN  BOOLEAN                 Write,
  IN  BOOLEAN                 Delete,
  IN  BOOLEAN                 Cache,
  IN OUT VARIABLE_INFO_ENTRY  **VariableInfo
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
  IN OUT  PROTECTED_VARIABLE_INFO  *VarInfo
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
  IN OUT  PROTECTED_VARIABLE_INFO  *VarInfo
  );

#endif

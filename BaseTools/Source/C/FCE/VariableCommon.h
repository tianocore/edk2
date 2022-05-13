/** @file

 The header of common Variable.c TimeBasedVariable.c and MonotonicBasedVariable.c.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VARIABLE_COMMON_H__
#define __VARIABLE_COMMON_H__

/**
  Check the store variable is no-authenticated or not

  @param VarToList     The pointer to the header of Variable Store.

  @retval TRUE         If no-authenticated, return TRUE.
  @retval FALSE        Otherwise, return FALSE.
**/

BOOLEAN
CheckNormalVarStoreOrNot (
  IN VOID  *VariableStoreHeader
  );
/**
  Check the store variable is Monotonic based authenticated or not

  @param VarToList     The pointer to the header of Variable Store.

  @retval TRUE         If authenticated, return TRUE.
  @retval FALSE        Otherwise, return FALSE.
**/

BOOLEAN
CheckMonotonicBasedVarStore (
  IN VOID  *VariableStoreHeader
  );

/**
  Check the store variable is Time stamp authenticated or not

  @param VarToList     The pointer to the header of Variable Store.

  @retval TRUE         If authenticated, return TRUE.
  @retval FALSE        Otherwise, return FALSE.
**/
BOOLEAN
CheckTimeBasedVarStoreOrNot (
  IN VOID  *VariableStoreHeader
  );



#endif // _EFI_VARIABLE_COMMON_H_

/** @file

  Variable worker functions specific for IA32, X64 and EBC.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"

/**
  Get one variable by the index count.

  @param  IndexTable  The pointer to variable index table.
  @param  Count       The index count of variable in index table.

  @return The pointer to variable header indexed by count.

**/
VARIABLE_HEADER *
GetVariableByIndex (
  IN VARIABLE_INDEX_TABLE        *IndexTable,
  IN UINT32                      Count
  )
{
  return (VARIABLE_HEADER *) (UINTN) (IndexTable->Index[Count] + ((UINTN) IndexTable->StartPtr & 0xFFFF0000));
}

/**
  Record Variable in VariableIndex HOB.

  Record Variable in VariableIndex HOB and update the length of variable index table.

  @param  IndexTable  The pointer to variable index table.
  @param  Variable    The pointer to the variable that will be recorded.

**/
VOID
VariableIndexTableUpdate (
  IN OUT  VARIABLE_INDEX_TABLE   *IndexTable,
  IN      VARIABLE_HEADER        *Variable
  )
{
  IndexTable->Index[IndexTable->Length++] = (UINT16) (UINTN) Variable;

  return;
}


/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by PeiVariable module.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_VARIABLE_H
#define _PEI_VARIABLE_H

#include <PiPei.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <VariableFormat.h>

//
// Define GET_PAD_SIZE to optimize compiler
//
#if ((ALIGNMENT == 0) || (ALIGNMENT == 1))
#define GET_PAD_SIZE(a) (0)
#else
#define GET_PAD_SIZE(a) (((~a) + 1) & (ALIGNMENT - 1))
#endif

#define HEADER_ALIGN(Header)  (((UINTN) (Header) + HEADER_ALIGNMENT - 1) & (~(HEADER_ALIGNMENT - 1)))

typedef struct {
  VARIABLE_HEADER *CurrPtr;
  VARIABLE_HEADER *EndPtr;
  VARIABLE_HEADER *StartPtr;
} VARIABLE_POINTER_TRACK;

#define VARIABLE_INDEX_TABLE_VOLUME 122

#define EFI_VARIABLE_INDEX_TABLE_GUID \
  { 0x8cfdb8c8, 0xd6b2, 0x40f3, { 0x8e, 0x97, 0x02, 0x30, 0x7c, 0xc9, 0x8b, 0x7c } }

typedef struct {
  UINT16          Length;
  UINT16          GoneThrough;
  VARIABLE_HEADER *EndPtr;
  VARIABLE_HEADER *StartPtr;
  UINT16          Index[VARIABLE_INDEX_TABLE_VOLUME];
} VARIABLE_INDEX_TABLE;


//
// Functions
//
EFI_STATUS
EFIAPI
PeimInitializeVariableServices (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  FfsHeader   - TODO: add argument description
  PeiServices - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PeiGetVariable (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
  IN CONST  CHAR16                          *VariableName,
  IN CONST  EFI_GUID                        *VariableGuid,
  OUT       UINT32                          *Attributes,
  IN OUT    UINTN                           *DataSize,
  OUT       VOID                            *Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PeiServices   - TODO: add argument description
  VariableName  - TODO: add argument description
  VendorGuid    - TODO: add argument description
  Attributes    - TODO: add argument description
  DataSize      - TODO: add argument description
  Data          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PeiGetNextVariableName (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
  IN OUT UINTN                              *VariableNameSize,
  IN OUT CHAR16                             *VariableName,
  IN OUT EFI_GUID                           *VariableGuid
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PeiServices       - TODO: add argument description
  VariableNameSize  - TODO: add argument description
  VariableName      - TODO: add argument description
  VendorGuid        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

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
  );

/**
  Record Variable in VariableIndex HOB.

  Record Variable in VariableIndex HOB and update the length of variable index table.

  @param  IndexTable  The pointer to variable index table.
  @param  Variable    The pointer to the variable that will be recorded.

  @retval VOID

**/
VOID
VariableIndexTableUpdate (
  IN OUT  VARIABLE_INDEX_TABLE   *IndexTable,
  IN      VARIABLE_HEADER        *Variable
  );

#endif // _PEI_VARIABLE_H

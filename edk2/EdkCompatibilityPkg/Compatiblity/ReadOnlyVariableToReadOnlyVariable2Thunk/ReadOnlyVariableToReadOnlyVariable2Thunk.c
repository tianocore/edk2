/*++

Copyright (c) 2006 - 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

  Variable.c

Abstract:

  PEIM to provide the Variable functionality

--*/

#include <PiPei.h>
#include <Ppi/ReadOnlyVariable.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/DebugLib.h>

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
PeiGetVariable (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  CHAR16            *VariableName,
  IN  EFI_GUID          *VendorGuid,
  OUT UINT32            *Attributes OPTIONAL,
  IN  OUT UINTN         *DataSize,
  OUT VOID              *Data
  );

EFI_STATUS
EFIAPI
PeiGetNextVariableName (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN OUT UINTN         *VariableNameSize,
  IN OUT CHAR16        *VariableName,
  IN OUT EFI_GUID      *VendorGuid
  );

//
// Module globals
//
EFI_PEI_READ_ONLY_VARIABLE_PPI mVariablePpi = {
  PeiGetVariable,
  PeiGetNextVariableName
};

EFI_PEI_PPI_DESCRIPTOR     mPpiListVariable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariablePpiGuid,
  &mVariablePpi
};

EFI_STATUS
EFIAPI
PeimInitializeReadOnlyVariable (
  IN EFI_FFS_FILE_HEADER     *FfsHeader,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Provide the functionality of the variable services.

Arguments:

  FfsHeadher  - The FFS file header
  PeiServices - General purpose services available to every PEIM.

Returns:

  Status -  EFI_SUCCESS if the interface could be successfully
            installed

--*/
{
  //
  // Publish the variable capability to other modules
  //
  return (*PeiServices)->InstallPpi (PeiServices, &mPpiListVariable);
}

EFI_STATUS
EFIAPI
PeiGetVariable (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  CHAR16            *VariableName,
  IN  EFI_GUID          *VendorGuid,
  OUT UINT32            *Attributes OPTIONAL,
  IN  OUT UINTN         *DataSize,
  OUT VOID              *Data
  )
/*++

Routine Description:

  Provide the read variable functionality of the variable services.

Arguments:

  PeiServices - General purpose services available to every PEIM.

  VariableName     - The variable name

  VendorGuid       - The vendor's GUID

  Attributes       - Pointer to the attribute

  DataSize         - Size of data

  Data             - Pointer to data

Returns:

  EFI_SUCCESS           - The interface could be successfully installed

  EFI_NOT_FOUND         - The variable could not be discovered

  EFI_BUFFER_TOO_SMALL  - The caller buffer is not large enough

--*/
{
  EFI_STATUS                      Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *ReadOnlyVariable2;

  Status = (*PeiServices)->LocatePpi (
                             (CONST EFI_PEI_SERVICES **)PeiServices, 
                             &gEfiPeiReadOnlyVariable2PpiGuid, 
                             0, 
                             NULL, 
                             (VOID **)&ReadOnlyVariable2
                             );
  ASSERT_EFI_ERROR (Status);

  return ReadOnlyVariable2->GetVariable (
                              ReadOnlyVariable2,
                              VariableName,
                              VendorGuid,
                              Attributes,
                              DataSize,
                              Data
                              );
}

EFI_STATUS
EFIAPI
PeiGetNextVariableName (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN OUT UINTN         *VariableNameSize,
  IN OUT CHAR16        *VariableName,
  IN OUT EFI_GUID      *VendorGuid
  )
/*++

Routine Description:

  Provide the get next variable functionality of the variable services.

Arguments:

  PeiServices        - General purpose services available to every PEIM.
  VariabvleNameSize  - The variable name's size.
  VariableName       - A pointer to the variable's name.
  VariableGuid       - A pointer to the EFI_GUID structure.

  VariableNameSize - Size of the variable name

  VariableName     - The variable name

  VendorGuid       - The vendor's GUID

Returns:

  EFI_SUCCESS - The interface could be successfully installed

  EFI_NOT_FOUND - The variable could not be discovered

--*/
{
  EFI_STATUS                      Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *ReadOnlyVariable2;

  Status = (*PeiServices)->LocatePpi (
                             (CONST EFI_PEI_SERVICES **)PeiServices, 
                             &gEfiPeiReadOnlyVariable2PpiGuid, 
                             0, 
                             NULL, 
                             (VOID **)&ReadOnlyVariable2
                             );
  ASSERT_EFI_ERROR (Status);

  return ReadOnlyVariable2->NextVariableName (
                              ReadOnlyVariable2,
                              VariableNameSize,
                              VariableName,
                              VendorGuid
                              );
}

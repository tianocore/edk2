/** @file
Module produce EFI_PEI_READ_ONLY_VARIABLE_PPI on top of EFI_PEI_READ_ONLY_VARIABLE2_PPI.
UEFI PI Spec supersedes Intel's Framework Specs. 
# EFI_PEI_READ_ONLY_VARIABLE_PPI defined in Intel Framework Pkg is replaced by EFI_PEI_READ_ONLY_VARIABLE2_PPI
# in MdePkg.
# This module produces EFI_PEI_READ_ONLY_VARIABLE_PPI on top of EFI_PEI_READ_ONLY_VARIABLE2_PPI. 
# This module is used on platform when both of these two conditions are true:
# 1) Framework module consumes EFI_PEI_READ_ONLY_VARIABLE_PPI is present.
# 2) The platform has a PI module that only produces EFI_PEI_READ_ONLY_VARIABLE2_PPI.

This module can't be used together with ReadOnlyVariable2ToReadOnlyVariableThunk module.

Copyright (c) 2006 - 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include <PiPei.h>
#include <Ppi/ReadOnlyVariable.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/ReadOnlyVariableThunkPresent.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

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

/**
  Standard entry point of a PEIM.

  @param FfsHeadher  The FFS file header
  @param PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS If the gEfiPeiReadOnlyVariablePpiGuid interface could be successfully installed.

**/
EFI_STATUS
EFIAPI
PeimInitializeReadOnlyVariable (
  IN EFI_PEI_FILE_HANDLE     FfsHeader,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  VOID        *Interface;
  EFI_STATUS  Status;

  //
  // Make sure ReadOnlyVariableToReadOnlyVariable2 module is not present. If so, the call chain will form a
  // infinite loop: ReadOnlyVariable -> ReadOnlyVariable2 -> ReadOnlyVariable -> ....
  //
  Status = PeiServicesLocatePpi (&gPeiReadonlyVariableThunkPresentPpiGuid, 0, NULL, &Interface);
  ASSERT (Status == EFI_NOT_FOUND);

  //
  // Publish the variable capability to other modules
  //
  return (*PeiServices)->InstallPpi (PeiServices, &mPpiListVariable);
}

/**
  Provide the read variable functionality of the variable services.

  @param  PeiServices           General purpose services available to every PEIM.
  @param  VariableName          The variable name
  @param  VendorGuid            The vendor's GUID
  @param  Attributes            Pointer to the attribute
  @param  DataSize              Size of data
  @param  Data                  Pointer to data

  @retval EFI_SUCCESS           The interface could be successfully installed
  @retval EFI_NOT_FOUND         The variable could not be discovered
  @retval EFI_BUFFER_TOO_SMALL  The caller buffer is not large enough

**/
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

/**
  Provide the get next variable functionality of the variable services.

  @param  PeiServices           General purpose services available to every PEIM.
  @param  VariabvleNameSize     The variable name's size.
  @param  VariableName          A pointer to the variable's name.
  @param  VariableGuid          A pointer to the EFI_GUID structure.
  @param  VariableNameSize      Size of the variable name
  @param  VariableName          The variable name
  @param  VendorGuid            The vendor's GUID

  @retval EFI_SUCCESS           The interface could be successfully installed
  @retval EFI_NOT_FOUND         The variable could not be discovered

**/
EFI_STATUS
EFIAPI
PeiGetNextVariableName (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN OUT UINTN         *VariableNameSize,
  IN OUT CHAR16        *VariableName,
  IN OUT EFI_GUID      *VendorGuid
  )
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

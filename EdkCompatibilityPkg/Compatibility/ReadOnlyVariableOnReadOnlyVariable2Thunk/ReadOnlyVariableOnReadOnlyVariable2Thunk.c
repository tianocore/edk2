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

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

/**
  Provide the read variable functionality of the variable services.

  @param[in]  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param[in]  VariableName   A NULL-terminated Unicode string that is the name of the vendor's variable.
  @param[in]  VendorGuid     A unique identifier for the vendor.
  @param[out] Attributes     This OPTIONAL parameter may be either NULL or
                             a pointer to the location in which to return
                             the attributes bitmask for the variable.
  @param[in, out]  DataSize   On input, the size in bytes of the return Data buffer.
                             On output, the size of data returned in Data.
  @param[out] Data           The buffer to return the contents of the variable.

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

  @param[in]     PeiServices       An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param[in, out] VariableNameSize  The size of the VariableName buffer.
  @param[in, out] VariableName      On input, supplies the last VariableName that was
                                   returned by GetNextVariableName(). On output, returns the Null-terminated
                                   Unicode string of the current variable.
  @param[in, out] VendorGuid        On input, supplies the last VendorGuid that was
                                   returned by GetNextVariableName(). On output, returns the VendorGuid
                                   of the current variable.

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

  @param FileHandle   Handle of the file being invoked.
  @param PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS If the gEfiPeiReadOnlyVariablePpiGuid interface could be successfully installed.

**/
EFI_STATUS
EFIAPI
PeimInitializeReadOnlyVariable (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  //
  //Developer should make sure ReadOnlyVariableToReadOnlyVariable2 module is not present. If so, the call chain will form a
  // infinite loop: ReadOnlyVariable -> ReadOnlyVariable2 -> ReadOnlyVariable -> ....
  //
  //
  // Publish the variable capability to other modules
  //
  return (*PeiServices)->InstallPpi (PeiServices, &mPpiListVariable);
}

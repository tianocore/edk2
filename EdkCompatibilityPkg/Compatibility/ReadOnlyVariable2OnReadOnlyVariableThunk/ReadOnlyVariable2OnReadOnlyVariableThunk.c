/** @file
Module produce EFI_PEI_READ_ONLY_VARIABLE2_PPI on top of EFI_PEI_READ_ONLY_VARIABLE_PPI.
UEFI PI Spec supersedes Intel's Framework Specs. 
EFI_PEI_READ_ONLY_VARIABLE_PPI defined in Intel Framework Pkg is replaced by EFI_PEI_READ_ONLY_VARIABLE2_PPI
in MdePkg.
This module produces EFI_PEI_READ_ONLY_VARIABLE2_PPI on top of EFI_PEI_READ_ONLY_VARIABLE_PPI. 
This module is used on platform when both of these two conditions are true:
1) Framework module produces EFI_PEI_READ_ONLY_VARIABLE_PPI is present.
2) The platform has PI modules that only consumes EFI_PEI_READ_ONLY_VARIABLE2_PPI.

This module can't be used together with ReadOnlyVariableToReadOnlyVariable2Thunk module.


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
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/ReadOnlyVariable.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>

/**
  Provide the read variable functionality of the variable services.

  @param  This                  A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.
  @param  VariableName          A pointer to a null-terminated string that is the variable's name.
  @param  VariableGuid          A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                VariableGuid and VariableName must be unique.
  @param  Attributes            If non-NULL, on return, points to the variable's attributes.
  @param  DataSize              On entry, points to the size in bytes of the Data buffer.
                                On return, points to the size of the data returned in Data.
  @param  Data                  Points to the buffer which will hold the returned variable value.

  @retval EFI_SUCCESS           The interface could be successfully installed
  @retval EFI_NOT_FOUND         The variable could not be discovered
  @retval EFI_BUFFER_TOO_SMALL  The caller buffer is not large enough

**/
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
{
  EFI_STATUS                     Status;
  EFI_PEI_READ_ONLY_VARIABLE_PPI *ReadOnlyVariable;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariablePpiGuid,
             0,
             NULL,
             (VOID **)&ReadOnlyVariable
             );
  ASSERT_EFI_ERROR (Status);

  return ReadOnlyVariable->PeiGetVariable (
                             (EFI_PEI_SERVICES **) GetPeiServicesTablePointer (),
                             (CHAR16 *)VariableName,
                             (EFI_GUID *)VariableGuid,
                             Attributes,
                             DataSize,
                             Data
                             );
}

/**
  Provide the get next variable functionality of the variable services.

  @param  This              A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.

  @param  VariableNameSize  On entry, points to the size of the buffer pointed to by VariableName.
  @param  VariableName      On entry, a pointer to a null-terminated string that is the variable's name.
                            On return, points to the next variable's null-terminated name string.

  @param  VariableGuid      On entry, a pointer to an EFI_GUID that is the variable's GUID. 
                            On return, a pointer to the next variable's GUID.

  @retval EFI_SUCCESS       The interface could be successfully installed
  @retval EFI_NOT_FOUND     The variable could not be discovered

**/
EFI_STATUS
EFIAPI
PeiGetNextVariableName (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
  IN OUT UINTN                              *VariableNameSize,
  IN OUT CHAR16                             *VariableName,
  IN OUT EFI_GUID                           *VariableGuid
  )
{
  EFI_STATUS                     Status;
  EFI_PEI_READ_ONLY_VARIABLE_PPI *ReadOnlyVariable;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariablePpiGuid,
             0,
             NULL,
             (VOID **)&ReadOnlyVariable
             );
  ASSERT_EFI_ERROR (Status);

  return ReadOnlyVariable->PeiGetNextVariableName (
                             (EFI_PEI_SERVICES **) GetPeiServicesTablePointer (),
                             VariableNameSize,
                             VariableName,
                             VariableGuid
                             );
}

//
// Module globals
//
EFI_PEI_READ_ONLY_VARIABLE2_PPI mVariablePpi = {
  PeiGetVariable,
  PeiGetNextVariableName
};

EFI_PEI_PPI_DESCRIPTOR     mPpiListVariable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariable2PpiGuid,
  &mVariablePpi
};

/**
  User entry for this PEIM driver.
  
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS ReadOnlyVariable2 PPI is successfully installed.
  @return Others      ReadOnlyVariable2 PPI is not successfully installed.

**/
EFI_STATUS
EFIAPI
PeimInitializeReadOnlyVariable2 (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  //
  // This thunk module can only be used together with a PI PEI core, as we 
  // assume PeiServices Pointer Table can be located in a standard way defined
  // in PI spec.
  //
  ASSERT ((*PeiServices)->Hdr.Revision >= 0x00010000);

  //
  // Developer should make sure ReadOnlyVariable2ToReadOnlyVariable module is not present. or else, the call chain will form a
  // infinite loop: ReadOnlyVariable2 -> ReadOnlyVariable -> ReadOnlyVariable2 -> .....
  //
  //
  // Publish the variable capability to other modules
  //
  return PeiServicesInstallPpi (&mPpiListVariable);
}

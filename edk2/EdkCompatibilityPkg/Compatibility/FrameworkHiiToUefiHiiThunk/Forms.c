/**@file
  This file contains the form processing code to the HII database.

Copyright (c) 2006 - 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  )
/*++

Routine Description:

  This function allows a program to extract a form or form package that has
  previously been registered with the EFI HII database.

Arguments:

Returns:

--*/
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL   *This,
  IN     FRAMEWORK_EFI_HII_HANDLE      Handle,
  IN     EFI_FORM_ID        FormId,
  IN OUT UINTN              *BufferLengthTemp,
  OUT    UINT8              *Buffer
  )
/*++

Routine Description:

  This function allows a program to extract a form or form package that has
  previously been registered with the EFI HII database.

Arguments:
  This         - A pointer to the EFI_HII_PROTOCOL instance.

  Handle       - Handle on which the form resides. Type FRAMEWORK_EFI_HII_HANDLE  is defined in
                 EFI_HII_PROTOCOL.NewPack() in the Packages section.

  FormId       - The ID of the form to return. If the ID is zero, the entire form package is returned.
                 Type EFI_FORM_ID is defined in "Related Definitions" below.

  BufferLength - On input, the length of the Buffer. On output, the length of the returned buffer, if
                 the length was sufficient and, if it was not, the length that is required to fit the
                 requested form(s).

  Buffer       - The buffer designed to receive the form(s).

Returns:

  EFI_SUCCESS           -  Buffer filled with the requested forms. BufferLength
                           was updated.

  EFI_INVALID_PARAMETER -  The handle is unknown.

  EFI_NOT_FOUND         -  A form on the requested handle cannot be found with the
                           requested FormId.

  EFI_BUFFER_TOO_SMALL  - The buffer provided was not large enough to allow the form to be stored.

--*/
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL            *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN     UINTN                       DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  )
/*++

  Routine Description:

  This function allows a program to extract the NV Image
  that represents the default storage image

  Arguments:
    This             - A pointer to the EFI_HII_PROTOCOL instance.
    Handle           - The HII handle from which will have default data retrieved.
    UINTN            - Mask used to retrieve the default image.
    VariablePackList - Callee allocated, tightly-packed, link list data
                         structure that contain all default varaible packs
                         from the Hii Database.

  Returns:
    EFI_NOT_FOUND         - If Hii database does not contain any default images.
    EFI_INVALID_PARAMETER - Invalid input parameter.
    EFI_SUCCESS           - Operation successful.

--*/
{
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_PROTOCOL       *This,
  IN FRAMEWORK_EFI_HII_HANDLE          Handle,
  IN EFI_FORM_LABEL         Label,
  IN BOOLEAN                AddData,
  IN EFI_HII_UPDATE_DATA    *Data
  )
/*++

Routine Description:
  This function allows the caller to update a form that has
  previously been registered with the EFI HII database.

Arguments:
  Handle     - Hii Handle associated with the Formset to modify
  Label      - Update information starting immediately after this label in the IFR
  AddData    - If TRUE, add data.  If FALSE, remove data
  Data       - If adding data, this is the pointer to the data to add

Returns:
  EFI_SUCCESS - Update success.
  Other       - Update fail.

--*/
{
  return EFI_SUCCESS;
}

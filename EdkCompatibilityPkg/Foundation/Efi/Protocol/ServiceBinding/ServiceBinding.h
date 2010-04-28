/*++ 

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ServiceBinding.h

Abstract:

  UEFI Service Binding protocol definition.

--*/

#ifndef _EFI_SERVICE_BINDING_H_
#define _EFI_SERVICE_BINDING_H_

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_SERVICE_BINDING_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_SERVICE_BINDING_CREATE_CHILD) (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
/*++

  Routine Description:
    Creates a child handle with a set of I/O services.

  Arguments:
    This         - Protocol instance pointer.
    ChildHandle  - Pointer to the handle of the child to create.  If it is NULL,
                   then a new handle is created.  If it is not NULL, then the
                   I/O services are added to the existing child handle.

  Returns:
    EFI_SUCCES           - The child handle was created with the I/O services.
    EFI_OUT_OF_RESOURCES - There are not enough resources availabe to create
                           the child.
    other                - The child handle was not created.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SERVICE_BINDING_DESTROY_CHILD) (
  IN struct _EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                            ChildHandle
  )
/*++

  Routine Description:
    Destroys a child handle with a set of I/O services.

  Arguments:
    This         - Protocol instance pointer.
    ChildHandle  - Handle of the child to destroy.

  Returns:
    EFI_SUCCES            - The I/O services were removed from the child handle.
    EFI_UNSUPPORTED       - The child handle does not support the I/O services
                            that are being removed.
    EFI_INVALID_PARAMETER - Child handle is not a valid EFI Handle.
    EFI_ACCESS_DENIED     - The child handle could not be destroyed because its
                            I/O services are being used.
    other                 - The child handle was not destroyed.

--*/
;

struct _EFI_SERVICE_BINDING_PROTOCOL {
  EFI_SERVICE_BINDING_CREATE_CHILD   CreateChild;
  EFI_SERVICE_BINDING_DESTROY_CHILD  DestroyChild;
};

#endif

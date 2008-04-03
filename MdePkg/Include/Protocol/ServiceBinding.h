/** @file

  The file defines the generic Service Binding Protocol
  functions.


  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_SERVICE_BINDING_H__
#define __EFI_SERVICE_BINDING_H__

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_SERVICE_BINDING_PROTOCOL EFI_SERVICE_BINDING_PROTOCOL;

/**
  Creates a child handle with a set of I/O services.

  @param  This        Protocol instance pointer.
  @param  ChildHandle Pointer to the handle of the child to create.  If it is NULL,
                      then a new handle is created.  If it is not NULL, then the
                      I/O services are added to the existing child handle.

  @retval EFI_SUCCES            The child handle was created with the I/O services
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child
  @retval other                 The child handle was not created

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SERVICE_BINDING_CREATE_CHILD) (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
;

/**
  Destroys a child handle with a set of I/O services.

  @param  This        Protocol instance pointer.
  @param  ChildHandle Handle of the child to destroy

  @retval EFI_SUCCES            The I/O services were removed from the child handle
  @retval EFI_UNSUPPORTED       The child handle does not support the I/O services
                                that are being removed.
  @retval EFI_INVALID_PARAMETER Child handle is not a valid EFI Handle.
  @retval EFI_ACCESS_DENIED     The child handle could not be destroyed because its
                                I/O services are being used.
  @retval other                 The child handle was not destroyed

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SERVICE_BINDING_DESTROY_CHILD) (
  IN EFI_SERVICE_BINDING_PROTOCOL          *This,
  IN EFI_HANDLE                            ChildHandle
  )
;

struct _EFI_SERVICE_BINDING_PROTOCOL {
  EFI_SERVICE_BINDING_CREATE_CHILD         CreateChild;
  EFI_SERVICE_BINDING_DESTROY_CHILD        DestroyChild;
};

#endif

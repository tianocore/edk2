/** @file
  This is definition for service binding for Hash driver.

Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HASH2_DRIVER_H_
#define _HASH2_DRIVER_H_

#include <Uefi.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Hash2.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>

#define HASH2_SERVICE_DATA_SIGNATURE  SIGNATURE_32 ('H', 'S', '2', 'S')

typedef struct {
  UINT32                          Signature;
  EFI_HANDLE                      ServiceHandle;
  EFI_SERVICE_BINDING_PROTOCOL    ServiceBinding;

  LIST_ENTRY                      ChildrenList;
} HASH2_SERVICE_DATA;

#define HASH2_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  HASH2_SERVICE_DATA, \
  ServiceBinding, \
  HASH2_SERVICE_DATA_SIGNATURE \
  )

#define HASH2_INSTANCE_DATA_SIGNATURE  SIGNATURE_32 ('H', 's', '2', 'I')

typedef struct {
  UINT32                Signature;
  HASH2_SERVICE_DATA    *Hash2ServiceData;
  EFI_HANDLE            Handle;
  LIST_ENTRY            InstEntry;
  EFI_HASH2_PROTOCOL    Hash2Protocol;
  VOID                  *HashContext;
  VOID                  *HashInfoContext;
  BOOLEAN               Updated;
} HASH2_INSTANCE_DATA;

#define HASH2_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  HASH2_INSTANCE_DATA, \
  Hash2Protocol, \
  HASH2_INSTANCE_DATA_SIGNATURE \
  )

#define HASH2_INSTANCE_DATA_FROM_LINK(a) \
  CR ( \
  (a), \
  HASH2_INSTANCE_DATA, \
  InstEntry, \
  HASH2_INSTANCE_DATA_SIGNATURE \
  )

/**
  Creates a child handle with a set of I/O services.

  @param[in]       This              Protocol instance pointer.
  @param[in, out]  ChildHandle       Pointer to the handle of the child to create. If
                                     it is NULL, then a new handle is created. If
                                     it is not NULL, then the I/O services are added
                                     to the existing child handle.

  @retval EFI_SUCCESS                The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER      ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources available to
                                     create the child.
  @retval Others                     The child handle was not created.

**/
EFI_STATUS
EFIAPI
Hash2ServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a set of I/O services.

  The DestroyChild() function does the opposite of CreateChild(). It removes a
  protocol that was installed by CreateChild() from ChildHandle. If the removed
  protocol is the last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This               Pointer to the EFI_SERVICE_BINDING_PROTOCOL
                                 instance.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCESS            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED        ChildHandle does not support the protocol that
                                 is being removed.
  @retval EFI_INVALID_PARAMETER  ChildHandle is NULL.
  @retval EFI_ACCESS_DENIED      The protocol could not be removed from the
                                 ChildHandle because its services are being
                                 used.
  @retval Others                 The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
Hash2ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

extern EFI_HASH2_PROTOCOL  mHash2Protocol;

#endif

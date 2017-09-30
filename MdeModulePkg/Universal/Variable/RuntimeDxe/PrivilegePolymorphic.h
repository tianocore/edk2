/** @file
  Polymorphic functions that are called from both the privileged driver (i.e.,
  the DXE_SMM variable module) and the non-privileged drivers (i.e., one or
  both of the DXE_RUNTIME variable modules).

  Each of these functions has two implementations, appropriate for privileged
  vs. non-privileged driver code.

  Copyright (c) 2017, Red Hat, Inc.<BR>
  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _PRIVILEGE_POLYMORPHIC_H_
#define _PRIVILEGE_POLYMORPHIC_H_

#include <Uefi/UefiBaseType.h>

/**
  SecureBoot Hook for auth variable update.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
**/
VOID
EFIAPI
SecureBootHook (
  IN CHAR16                                 *VariableName,
  IN EFI_GUID                               *VendorGuid
  );

#endif

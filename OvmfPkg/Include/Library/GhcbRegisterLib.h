/** @file

  Declarations of utility functions used for GHCB GPA registration.

  Copyright (C) 2021, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _GHCB_REGISTER_LIB_H_
#define _GHCB_REGISTER_LIB_H_

/**

  This function can be used to register the GHCB GPA.

  @param[in]  Address           The physical address to registered.

**/
VOID
EFIAPI
GhcbRegister (
  IN  EFI_PHYSICAL_ADDRESS   Address
  );

#endif // _GHCB_REGISTER_LIB_H_

/** @file
  Constructor to initialize CPUID data for OpenSSL assembly operations.

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

/**
  An internal OpenSSL function which fetches a local copy of the hardware
  capability flags.

**/
extern
VOID
OPENSSL_cpuid_setup (
  VOID
  );

/**
  Constructor routine for OpensslLib.

  The constructor calls an internal OpenSSL function which fetches a local copy
  of the hardware capability flags, used to enable native crypto instructions.

  @param  None

  @retval EFI_SUCCESS         The construction succeeded.

**/
RETURN_STATUS
EFIAPI
OpensslLibConstructor (
  VOID
  )
{
  OPENSSL_cpuid_setup ();

  return RETURN_SUCCESS;
}

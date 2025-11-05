/** @file
  C Run-Time Libraries (CRT) Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>

#include <Base.h>
#include <Library/DebugLib.h>

/* Read formatted data from a string */
int
sscanf (
  const char  *buffer,
  const char  *format,
  ...
  )
{
  //
  // Null sscanf() function implementation to satisfy the linker, since
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

uid_t
getuid (
  void
  )
{
  return 0;
}

uid_t
geteuid (
  void
  )
{
  return 0;
}

gid_t
getgid (
  void
  )
{
  return 0;
}

gid_t
getegid (
  void
  )
{
  return 0;
}

int  errno = 0;

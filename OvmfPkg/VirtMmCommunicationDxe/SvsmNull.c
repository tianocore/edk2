/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

BOOLEAN
EFIAPI
VirtMmSvsmProbe (
  VOID
  )
{
  return FALSE;
}

EFI_STATUS
EFIAPI
VirtMmSvsmComm (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

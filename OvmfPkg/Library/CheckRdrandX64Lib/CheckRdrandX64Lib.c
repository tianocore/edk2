/** @file
  check for rdrand instruction support via cpuid

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

RETURN_STATUS
EFIAPI
CheckRdRandX64LibConstructor (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "%a: OK\n", __func__));
  return RETURN_SUCCESS;
}

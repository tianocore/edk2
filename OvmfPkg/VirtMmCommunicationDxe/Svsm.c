/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

#include <Library/AmdSvsmLib.h>
#include <Register/Amd/Msr.h>
#include <Register/Amd/Svsm.h>

#include "VirtMmCommunication.h"

BOOLEAN
EFIAPI
VirtMmSvsmProbe (
  VOID
  )
{
  UINT32  Min, Max;

  if (!AmdSvsmIsSvsmPresent ()) {
    DEBUG ((DEBUG_VERBOSE, "%a: no SVSM present\n", __func__));
    return FALSE;
  }

  if (!AmdSvsmQueryProtocol (SVSM_UEFI_MM_PROTOCOL, 1, &Min, &Max)) {
    DEBUG ((DEBUG_VERBOSE, "%a: SVSM UEFI MM protocol not supported\n", __func__));
    return FALSE;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: SVSM UEFI MM protocol available (min %d, max %d)\n",
    __func__,
    Min,
    Max
    ));
  return TRUE;
}

EFI_STATUS
EFIAPI
VirtMmSvsmComm (
  VOID
  )
{
  UINT64  Addr, Size;

  Addr = (UINT64)(UINTN)mCommunicateBufferPhys;
  Size = MAX_BUFFER_SIZE;

  DEBUG ((DEBUG_VERBOSE, "%a: request\n", __func__));
  if (!AmdSvsmUefiMmRequest (Addr, Size)) {
    DEBUG ((DEBUG_ERROR, "%a: SVSM_UEFI_MM_REQUEST failed\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

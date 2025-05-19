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
  if (!AmdSvsmIsSvsmPresent ()) {
    DEBUG ((DEBUG_VERBOSE, "%a: no SVSM present\n", __func__));
    return FALSE;
  }

  if (!AmdSvsmUefiMmCall (SVSM_UEFI_MM_QUERY, 0, 0)) {
    DEBUG ((DEBUG_VERBOSE, "%a: SVSM UEFI MM protocol not supported\n", __func__));
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "%a: query ok\n", __func__));
  return TRUE;
}

EFI_STATUS
EFIAPI
VirtMmSvsmInit (
  VOID
  )
{
  UINT64  Rcx, Rdx;

  ASSERT (AmdSvsmIsSvsmPresent ());

  AmdSvsmUefiMmCall (SVSM_UEFI_MM_RESET, 0, 0);

  Rcx = (UINT64)(UINTN)mCommunicateBufferPhys;
  Rdx = MAX_BUFFER_SIZE;
  if (!AmdSvsmUefiMmCall (SVSM_UEFI_MM_SETUP, Rcx, Rdx)) {
    DEBUG ((DEBUG_ERROR, "%a: SVSM_UEFI_MM_SETUP failed\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtMmSvsmVirtMap (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "%a: going virtual\n", __func__));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtMmSvsmComm (
  VOID
  )
{
  ASSERT (AmdSvsmIsSvsmPresent ());

  DEBUG ((DEBUG_INFO, "%a: request ...\n", __func__));
  if (!AmdSvsmUefiMmCall (SVSM_UEFI_MM_REQUEST, 0, 0)) {
    DEBUG ((DEBUG_ERROR, "%a: SVSM_UEFI_MM_REQUEST failed\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

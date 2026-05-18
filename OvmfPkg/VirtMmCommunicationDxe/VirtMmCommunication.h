/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/* communication buffer */

#define MAX_BUFFER_SIZE  (64 * 1024)

extern VOID                  *mCommunicateBuffer;
extern EFI_PHYSICAL_ADDRESS  mCommunicateBufferPhys;
extern UINT64                mUefiVarsAddr;
extern BOOLEAN               mUsePioTransfer;

/* arch specific hooks */

EFI_STATUS
EFIAPI
VirtMmHwFind (
  VOID
  );

EFI_STATUS
EFIAPI
VirtMmHwInit (
  VOID
  );

EFI_STATUS
EFIAPI
VirtMmHwComm (
  VOID
  );

EFI_STATUS
EFIAPI
VirtMmHwVirtMap (
  VOID
  );

EFI_STATUS
EFIAPI
VirtMmHwPioTransfer (
  VOID     *Buffer,
  UINT32   BufferSize,
  BOOLEAN  ToDevice
  );

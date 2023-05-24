/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRT_MM_COMM_DXE_H_
#define _VIRT_MM_COMM_DXE_H_

/* communication buffer */

#define MAX_BUFFER_SIZE  (64 * 1024)

extern VOID                  *mCommunicateBuffer;
extern EFI_PHYSICAL_ADDRESS  mCommunicateBufferPhys;

/* arch specific hooks */

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

#endif /* _VIRT_MM_COMM_DXE_H_ */

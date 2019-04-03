/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  TcoReset.h

Abstract:

  Protocol to communicate with ICH TCO.

GUID Info:
 {A6A79162-E325-4c30-BCC3-59373064EFB3}
 0xa6a79162, 0xe325, 0x4c30, 0xbc, 0xc3, 0x59, 0x37, 0x30, 0x64, 0xef, 0xb3);


--*/

#ifndef _TCO_RESET_H_
#define _TCO_RESET_H_


#define EFI_TCO_RESET_PROTOCOL_GUID  \
  {0xa6a79162, 0xe325, 0x4c30, 0xbc, 0xc3, 0x59, 0x37, 0x30, 0x64, 0xef, 0xb3}

typedef struct _EFI_TCO_RESET_PROTOCOL EFI_TCO_RESET_PROTOCOL;

/**
  Enables the TCO timer to reset the system in case of a system hang.  This is
  used when writing the clock registers.

  @param[in] RcrbGcsSaveValue  This is the value of the RCRB GCS register before it is
                               changed by this procedure.  This will be used to restore
                               the settings of this register in PpiDisableTcoReset.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCO_RESET_PROTOCOL_ENABLE_TCO_RESET) (
  IN      UINT32            *RcrbGcsSaveValue
  );

/**
  Disables the TCO timer.  This is used after writing the clock registers.

  @param[in] RcrbGcsRestoreValue  Value saved in PpiEnableTcoReset so that it can
                                  restored.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_TCO_RESET_PROTOCOL_DISABLE_TCO_RESET) (
  OUT     UINT32    RcrbGcsRestoreValue
  );

typedef struct _EFI_TCO_RESET_PROTOCOL {
  EFI_TCO_RESET_PROTOCOL_ENABLE_TCO_RESET       EnableTcoReset;
  EFI_TCO_RESET_PROTOCOL_DISABLE_TCO_RESET    	DisableTcoReset;
} EFI_TCO_RESET_PROTOCOL;

extern EFI_GUID gEfiTcoResetProtocolGuid;

#endif

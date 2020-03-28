/** @file

  Internal definitions for the PVSCSI driver, which produces Extended SCSI
  Pass Thru Protocol instances for pvscsi devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PVSCSI_DXE_H_
#define __PVSCSI_DXE_H_

#include <Library/DebugLib.h>
#include <Protocol/ScsiPassThruExt.h>

#define PVSCSI_SIG SIGNATURE_32 ('P', 'S', 'C', 'S')

typedef struct {
  UINT32                          Signature;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL PassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     PassThruMode;
} PVSCSI_DEV;

#define PVSCSI_FROM_PASS_THRU(PassThruPointer) \
  CR (PassThruPointer, PVSCSI_DEV, PassThru, PVSCSI_SIG)

#endif // __PVSCSI_DXE_H_

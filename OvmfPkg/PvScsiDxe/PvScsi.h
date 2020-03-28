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

typedef struct {
  EFI_PHYSICAL_ADDRESS DeviceAddress;
  VOID                 *Mapping;
} PVSCSI_DMA_DESC;

typedef struct {
  PVSCSI_RINGS_STATE   *RingState;
  PVSCSI_DMA_DESC      RingStateDmaDesc;

  PVSCSI_RING_REQ_DESC *RingReqs;
  PVSCSI_DMA_DESC      RingReqsDmaDesc;

  PVSCSI_RING_CMP_DESC *RingCmps;
  PVSCSI_DMA_DESC      RingCmpsDmaDesc;
} PVSCSI_RING_DESC;

#define PVSCSI_SIG SIGNATURE_32 ('P', 'S', 'C', 'S')

typedef struct {
  UINT32                          Signature;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  UINT64                          OriginalPciAttributes;
  PVSCSI_RING_DESC                RingDesc;
  UINT8                           MaxTarget;
  UINT8                           MaxLun;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL PassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     PassThruMode;
} PVSCSI_DEV;

#define PVSCSI_FROM_PASS_THRU(PassThruPointer) \
  CR (PassThruPointer, PVSCSI_DEV, PassThru, PVSCSI_SIG)

#endif // __PVSCSI_DXE_H_

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

typedef struct {
  //
  // As EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET.SenseDataLength is defined
  // as UINT8, defining here SenseData size to MAX_UINT8 will guarantee it
  // cannot overflow when passed to device.
  //
  UINT8     SenseData[MAX_UINT8];
  //
  // This size of the data is arbitrarily chosen.
  // It seems to be sufficient for all I/O requests sent through
  // EFI_SCSI_PASS_THRU_PROTOCOL.PassThru() for common boot scenarios.
  //
  UINT8     Data[0x2000];
} PVSCSI_DMA_BUFFER;

#define PVSCSI_SIG SIGNATURE_32 ('P', 'S', 'C', 'S')

typedef struct {
  UINT32                          Signature;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  EFI_EVENT                       ExitBoot;
  UINT64                          OriginalPciAttributes;
  PVSCSI_RING_DESC                RingDesc;
  PVSCSI_DMA_BUFFER               *DmaBuf;
  PVSCSI_DMA_DESC                 DmaBufDmaDesc;
  UINT8                           MaxTarget;
  UINT8                           MaxLun;
  UINTN                           WaitForCmpStallInUsecs;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL PassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     PassThruMode;
} PVSCSI_DEV;

#define PVSCSI_FROM_PASS_THRU(PassThruPointer) \
  CR (PassThruPointer, PVSCSI_DEV, PassThru, PVSCSI_SIG)

#define PVSCSI_DMA_BUF_DEV_ADDR(Dev, MemberName) \
  (Dev->DmaBufDmaDesc.DeviceAddress + OFFSET_OF(PVSCSI_DMA_BUFFER, MemberName))

#endif // __PVSCSI_DXE_H_

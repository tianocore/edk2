/** @file
    Event handler to check for available packet.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Snp.h"


/**
  Nofication call back function for WaitForPacket event.

  @param  Event       EFI Event.
  @param  SnpPtr      Pointer to SNP_DRIVER structure.

**/
VOID
EFIAPI
SnpWaitForPacketNotify (
  EFI_EVENT     Event,
  VOID          *SnpPtr
  )
{
  PXE_DB_GET_STATUS PxeDbGetStatus;

  //
  // Do nothing if either parameter is a NULL pointer.
  //
  if (Event == NULL || SnpPtr == NULL) {
    return ;
  }
  //
  // Do nothing if the SNP interface is not initialized.
  //
  switch (((SNP_DRIVER *) SnpPtr)->Mode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
  case EfiSimpleNetworkStarted:
  default:
    return ;
  }
  //
  // Fill in CDB for UNDI GetStatus().
  //
  ((SNP_DRIVER *) SnpPtr)->Cdb.OpCode     = PXE_OPCODE_GET_STATUS;
  ((SNP_DRIVER *) SnpPtr)->Cdb.OpFlags    = 0;
  ((SNP_DRIVER *) SnpPtr)->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  ((SNP_DRIVER *) SnpPtr)->Cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  ((SNP_DRIVER *) SnpPtr)->Cdb.DBsize     = (UINT16) (sizeof (UINT32) * 2);
  ((SNP_DRIVER *) SnpPtr)->Cdb.DBaddr     = (UINT64)(UINTN) (((SNP_DRIVER *) SnpPtr)->Db);
  ((SNP_DRIVER *) SnpPtr)->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  ((SNP_DRIVER *) SnpPtr)->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  ((SNP_DRIVER *) SnpPtr)->Cdb.IFnum      = ((SNP_DRIVER *) SnpPtr)->IfNum;
  ((SNP_DRIVER *) SnpPtr)->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Clear contents of DB buffer.
  //
  ZeroMem (((SNP_DRIVER *) SnpPtr)->Db, sizeof (UINT32) * 2);

  //
  // Issue UNDI command and check result.
  //
  (*((SNP_DRIVER *) SnpPtr)->IssueUndi32Command) ((UINT64)(UINTN) &((SNP_DRIVER *) SnpPtr)->Cdb);

  if (((SNP_DRIVER *) SnpPtr)->Cdb.StatCode != EFI_SUCCESS) {
    return ;
  }
  //
  // We might have a packet.  Check the receive length and signal
  // the event if the length is not zero.
  //
  CopyMem (
    &PxeDbGetStatus,
    ((SNP_DRIVER *) SnpPtr)->Db,
    sizeof (UINT32) * 2
    );

  if (PxeDbGetStatus.RxFrameLen != 0) {
    gBS->SignalEvent (Event);
  }
}

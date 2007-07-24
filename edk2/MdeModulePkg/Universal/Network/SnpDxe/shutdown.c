/** @file
Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module name:
  shutdown.c

Abstract:

Revision history:
  2000-Feb-14 M(f)J   Genesis.

**/

#include "Snp.h"


/**
  this routine calls undi to shut down the interface.

  @param  snp   pointer to snp driver structure


**/
EFI_STATUS
pxe_shutdown (
  IN SNP_DRIVER *snp
  )
{
  snp->cdb.OpCode     = PXE_OPCODE_SHUTDOWN;
  snp->cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;
  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  snp->cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  snp->cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  snp->cdb.DBaddr     = PXE_DBADDR_NOT_USED;
  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.shutdown()  "));

  (*snp->issue_undi32_command) ((UINT64)(UINTN) &snp->cdb);

  if (snp->cdb.StatCode != PXE_STATCODE_SUCCESS) {
    //
    // UNDI could not be shutdown. Return UNDI error.
    //
    DEBUG ((EFI_D_WARN, "\nsnp->undi.shutdown()  %xh:%xh\n", snp->cdb.StatFlags, snp->cdb.StatCode));

    return EFI_DEVICE_ERROR;
  }
  //
  // Free allocated memory.
  //
  if (snp->tx_rx_buffer != NULL) {
    snp->IoFncs->FreeBuffer (
                  snp->IoFncs,
                  SNP_MEM_PAGES (snp->tx_rx_bufsize),
                  (VOID *) snp->tx_rx_buffer
                  );
  }

  snp->tx_rx_buffer   = NULL;
  snp->tx_rx_bufsize  = 0;

  return EFI_SUCCESS;
}


/**
  This is the SNP interface routine for shutting down the interface
  This routine basically retrieves snp structure, checks the SNP state and
  calls the pxe_shutdown routine to actually do the undi shutdown

  @param  this  context pointer


**/
EFI_STATUS
EFIAPI
snp_undi32_shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *this
  )
{
  SNP_DRIVER  *snp;
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  //
  //
  //
  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  //
  //
  switch (snp->mode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;

  default:
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }
  //
  //
  //
  Status                          = pxe_shutdown (snp);

  snp->mode.State                 = EfiSimpleNetworkStarted;
  snp->mode.ReceiveFilterSetting  = 0;

  snp->mode.MCastFilterCount      = 0;
  snp->mode.ReceiveFilterSetting  = 0;
  ZeroMem (snp->mode.MCastFilter, sizeof snp->mode.MCastFilter);
  CopyMem (
    &snp->mode.CurrentAddress,
    &snp->mode.PermanentAddress,
    sizeof (EFI_MAC_ADDRESS)
    );

  gBS->CloseEvent (snp->snp.WaitForPacket);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

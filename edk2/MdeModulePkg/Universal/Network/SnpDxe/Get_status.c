/** @file
Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module name:
  get_status.c

Abstract:

Revision history:
  2000-Feb-03 M(f)J   Genesis.

**/

#include "Snp.h"

/**
  this routine calls undi to get the status of the interrupts, get the list of
  transmit buffers that completed transmitting!

  @param  snp                     pointer to snp driver structure
  @param  InterruptStatusPtr      a non null pointer gets the interrupt status
  @param  TransmitBufferListPtrs  a non null ointer gets the list of pointers of
                                  previously  transmitted buffers whose
                                  transmission was completed  asynchrnously.


**/
EFI_STATUS
pxe_getstatus (
  SNP_DRIVER *snp,
  UINT32     *InterruptStatusPtr,
  VOID       **TransmitBufferListPtr
  )
{
  PXE_DB_GET_STATUS *db;
  UINT16            InterruptFlags;

  db                = snp->db;
  snp->cdb.OpCode   = PXE_OPCODE_GET_STATUS;

  snp->cdb.OpFlags  = 0;

  if (TransmitBufferListPtr != NULL) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS;
  }

  if (InterruptStatusPtr != NULL) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_GET_INTERRUPT_STATUS;
  }

  snp->cdb.CPBsize  = PXE_CPBSIZE_NOT_USED;
  snp->cdb.CPBaddr  = PXE_CPBADDR_NOT_USED;

  //
  // size DB for return of one buffer
  //
  snp->cdb.DBsize = (UINT16) (((UINT16) (sizeof (PXE_DB_GET_STATUS)) - (UINT16) (sizeof db->TxBuffer)) + (UINT16) (sizeof db->TxBuffer[0]));

  snp->cdb.DBaddr     = (UINT64)(UINTN) db;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.get_status()  "));

  (*snp->issue_undi32_command) ((UINT64)(UINTN) &snp->cdb);

  if (snp->cdb.StatCode != EFI_SUCCESS) {
    DEBUG (
      (EFI_D_NET,
      "\nsnp->undi.get_status()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatFlags)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // report the values back..
  //
  if (InterruptStatusPtr != NULL) {
    InterruptFlags      = (UINT16) (snp->cdb.StatFlags & PXE_STATFLAGS_GET_STATUS_INTERRUPT_MASK);

    *InterruptStatusPtr = 0;

    if (InterruptFlags & PXE_STATFLAGS_GET_STATUS_RECEIVE) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
    }

    if (InterruptFlags & PXE_STATFLAGS_GET_STATUS_TRANSMIT) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
    }

    if (InterruptFlags & PXE_STATFLAGS_GET_STATUS_COMMAND) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_COMMAND_INTERRUPT;
    }

    if (InterruptFlags & PXE_STATFLAGS_GET_STATUS_SOFTWARE) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_COMMAND_INTERRUPT;
    }

  }

  if (TransmitBufferListPtr != NULL) {
    *TransmitBufferListPtr =
      (
        (snp->cdb.StatFlags & PXE_STATFLAGS_GET_STATUS_NO_TXBUFS_WRITTEN) ||
        (snp->cdb.StatFlags & PXE_STATFLAGS_GET_STATUS_TXBUF_QUEUE_EMPTY)
      ) ? 0 : (VOID *) (UINTN) db->TxBuffer[0];

  }

  return EFI_SUCCESS;
}


/**
  This is the SNP interface routine for getting the status
  This routine basically retrieves snp structure, checks the SNP state and
  calls the pxe_getstatus routine to actually get the undi status

  @param  this                    context pointer
  @param  InterruptStatusPtr      a non null pointer gets the interrupt status
  @param  TransmitBufferListPtrs  a non null ointer gets the list of pointers of
                                  previously  transmitted buffers whose
                                  transmission was completed  asynchrnously.


**/
EFI_STATUS
EFIAPI
snp_undi32_get_status (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  OUT UINT32                     *InterruptStatusPtr OPTIONAL,
  OUT VOID                       **TransmitBufferListPtr OPTIONAL
  )
{
  SNP_DRIVER  *snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterruptStatusPtr == NULL && TransmitBufferListPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (snp == NULL) {
    return EFI_DEVICE_ERROR;
  }

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

  Status = pxe_getstatus (snp, InterruptStatusPtr, TransmitBufferListPtr);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

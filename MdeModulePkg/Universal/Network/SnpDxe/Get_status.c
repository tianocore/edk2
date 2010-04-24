/** @file
  Implementation of reading the current interrupt status and recycled transmit
  buffer status from a network interface.

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed
and made available under the terms and conditions of the BSD License which
accompanies this distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Snp.h"

/**
  Call undi to get the status of the interrupts, get the list of transmit
  buffers that completed transmitting.

  @param  Snp                     Pointer to snp driver structure.
  @param  InterruptStatusPtr      A non null pointer to contain the interrupt
                                  status.
  @param  TransmitBufferListPtrs  A non null pointer to contain the list of
                                  pointers of previous transmitted buffers whose
                                  transmission was completed asynchrnously.

  @retval EFI_SUCCESS         The status of the network interface was retrieved.
  @retval EFI_DEVICE_ERROR    The command could not be sent to the network
                              interface.

**/
EFI_STATUS
PxeGetStatus (
  SNP_DRIVER *Snp,
  UINT32     *InterruptStatusPtr,
  VOID       **TransmitBufferListPtr
  )
{
  PXE_DB_GET_STATUS *Db;
  UINT16            InterruptFlags;

  Db                = Snp->Db;
  Snp->Cdb.OpCode   = PXE_OPCODE_GET_STATUS;

  Snp->Cdb.OpFlags  = 0;

  if (TransmitBufferListPtr != NULL) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS;
  }

  if (InterruptStatusPtr != NULL) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_GET_INTERRUPT_STATUS;
  }

  if (Snp->MediaStatusSupported) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_GET_MEDIA_STATUS;
  }

  Snp->Cdb.CPBsize  = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.CPBaddr  = PXE_CPBADDR_NOT_USED;

  //
  // size DB for return of one buffer
  //
  Snp->Cdb.DBsize     = (UINT16) ((sizeof (PXE_DB_GET_STATUS) - sizeof (Db->TxBuffer)) + sizeof (Db->TxBuffer[0]));

  Snp->Cdb.DBaddr     = (UINT64)(UINTN) Db;

  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nSnp->undi.get_status()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != EFI_SUCCESS) {
    DEBUG (
      (EFI_D_NET,
      "\nSnp->undi.get_status()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatFlags)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // report the values back..
  //
  if (InterruptStatusPtr != NULL) {
    InterruptFlags      = (UINT16) (Snp->Cdb.StatFlags & PXE_STATFLAGS_GET_STATUS_INTERRUPT_MASK);

    *InterruptStatusPtr = 0;

    if ((InterruptFlags & PXE_STATFLAGS_GET_STATUS_RECEIVE) == PXE_STATFLAGS_GET_STATUS_RECEIVE) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
    }

    if ((InterruptFlags & PXE_STATFLAGS_GET_STATUS_TRANSMIT) == PXE_STATFLAGS_GET_STATUS_TRANSMIT) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
    }

    if ((InterruptFlags & PXE_STATFLAGS_GET_STATUS_COMMAND) == PXE_STATFLAGS_GET_STATUS_COMMAND) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_COMMAND_INTERRUPT;
    }

    if ((InterruptFlags & PXE_STATFLAGS_GET_STATUS_SOFTWARE) == PXE_STATFLAGS_GET_STATUS_SOFTWARE) {
      *InterruptStatusPtr |= EFI_SIMPLE_NETWORK_COMMAND_INTERRUPT;
    }

  }

  if (TransmitBufferListPtr != NULL) {
    *TransmitBufferListPtr =
      (
        ((Snp->Cdb.StatFlags & PXE_STATFLAGS_GET_STATUS_NO_TXBUFS_WRITTEN) != 0) ||
        ((Snp->Cdb.StatFlags & PXE_STATFLAGS_GET_STATUS_TXBUF_QUEUE_EMPTY) != 0)
      ) ? 0 : (VOID *) (UINTN) Db->TxBuffer[0];

  }

  //
  // Update MediaPresent field of EFI_SIMPLE_NETWORK_MODE if the UNDI support
  // returning media status from GET_STATUS command
  //
  if (Snp->MediaStatusSupported) {
    Snp->Snp.Mode->MediaPresent =
      (BOOLEAN) (((Snp->Cdb.StatFlags & PXE_STATFLAGS_GET_STATUS_NO_MEDIA) != 0) ? FALSE : TRUE);
  }

  return EFI_SUCCESS;
}

/**
  Reads the current interrupt status and recycled transmit buffer status from a
  network interface.

  This function gets the current interrupt and recycled transmit buffer status
  from the network interface. The interrupt status is returned as a bit mask in
  InterruptStatus. If InterruptStatus is NULL, the interrupt status will not be
  read. If TxBuf is not NULL, a recycled transmit buffer address will be retrieved.
  If a recycled transmit buffer address is returned in TxBuf, then the buffer has
  been successfully transmitted, and the status for that buffer is cleared. If
  the status of the network interface is successfully collected, EFI_SUCCESS
  will be returned. If the driver has not been initialized, EFI_DEVICE_ERROR will
  be returned.

  @param This            A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param InterruptStatus A pointer to the bit mask of the currently active
                         interrupts (see "Related Definitions"). If this is NULL,
                         the interrupt status will not be read from the device.
                         If this is not NULL, the interrupt status will be read
                         from the device. When the interrupt status is read, it
                         will also be cleared. Clearing the transmit interrupt does
                         not empty the recycled transmit buffer array.
  @param TxBuf           Recycled transmit buffer address. The network interface
                         will not transmit if its internal recycled transmit
                         buffer array is full. Reading the transmit buffer does
                         not clear the transmit interrupt. If this is NULL, then
                         the transmit buffer status will not be read. If there
                         are no transmit buffers to recycle and TxBuf is not NULL,
                         TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER This parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32GetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINT32                     *InterruptStatus, OPTIONAL
  OUT VOID                       **TxBuf           OPTIONAL
  )
{
  SNP_DRIVER  *Snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterruptStatus == NULL && TxBuf == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Snp == NULL) {
    return EFI_DEVICE_ERROR;
  }

  switch (Snp->Mode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;

  default:
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  Status = PxeGetStatus (Snp, InterruptStatus, TxBuf);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

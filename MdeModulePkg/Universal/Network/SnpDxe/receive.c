/** @file
Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module name:
    receive.c

Abstract:

Revision history:
    2000-Feb-03 M(f)J   Genesis.

**/


#include "Snp.h"

/**
  this routine calls undi to receive a packet and fills in the data in the
  input pointers!

  @param  snp                 pointer to snp driver structure
  @param  BufferPtr           pointer to the memory for the received data
  @param  BuffSizePtr         is a pointer to the length of the buffer on entry and
                              contains the length of the received data on return
  @param  HeaderSizePtr       pointer to the header portion of the data received.
  @param  SourceAddrPtr       optional parameter, is a pointer to contain the
                              source ethernet address on return
  @param  DestinationAddrPtr  optional parameter, is a pointer to contain the
                              destination ethernet address on return
  @param  ProtocolPtr         optional parameter, is a pointer to contain the
                              protocol type from the ethernet header on return


**/
STATIC
EFI_STATUS
pxe_receive (
  SNP_DRIVER      *snp,
  VOID            *BufferPtr,
  UINTN           *BuffSizePtr,
  UINTN           *HeaderSizePtr,
  EFI_MAC_ADDRESS *SourceAddrPtr,
  EFI_MAC_ADDRESS *DestinationAddrPtr,
  UINT16          *ProtocolPtr
  )
{
  PXE_CPB_RECEIVE *cpb;
  PXE_DB_RECEIVE  *db;
  UINTN           buf_size;

  cpb       = snp->cpb;
  db        = snp->db;
  buf_size  = *BuffSizePtr;

  cpb->BufferAddr = (UINT64)(UINTN) BufferPtr;
  cpb->BufferLen  = (UINT32) *BuffSizePtr;

  cpb->reserved       = 0;

  snp->cdb.OpCode     = PXE_OPCODE_RECEIVE;
  snp->cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;

  snp->cdb.CPBsize    = sizeof (PXE_CPB_RECEIVE);
  snp->cdb.CPBaddr    = (UINT64)(UINTN) cpb;

  snp->cdb.DBsize     = sizeof (PXE_DB_RECEIVE);
  snp->cdb.DBaddr     = (UINT64)(UINTN) db;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.receive ()  "));

  (*snp->issue_undi32_command) ((UINT64)(UINTN) &snp->cdb);

  switch (snp->cdb.StatCode) {
  case PXE_STATCODE_SUCCESS:
    break;

  case PXE_STATCODE_NO_DATA:
    DEBUG (
      (EFI_D_NET,
      "\nsnp->undi.receive ()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_NOT_READY;

  default:
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.receive()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }

  *BuffSizePtr = db->FrameLen;

  if (HeaderSizePtr != NULL) {
    *HeaderSizePtr = db->MediaHeaderLen;
  }

  if (SourceAddrPtr != NULL) {
    CopyMem (SourceAddrPtr, &db->SrcAddr, snp->mode.HwAddressSize);
  }

  if (DestinationAddrPtr != NULL) {
    CopyMem (DestinationAddrPtr, &db->DestAddr, snp->mode.HwAddressSize);
  }

  if (ProtocolPtr != NULL) {
    *ProtocolPtr = (UINT16) PXE_SWAP_UINT16 (db->Protocol); /*  we need to do the byte swapping */
  }

  return (*BuffSizePtr <= buf_size) ? EFI_SUCCESS : EFI_BUFFER_TOO_SMALL;
}


/**
  This is the SNP interface routine for receiving network data.
  This routine basically retrieves snp structure, checks the SNP state and
  calls the pxe_receive routine to actually do the receive!

  @param  this                context pointer
  @param  HeaderSizePtr       optional parameter and is a pointer to the header
                              portion of the data received.
  @param  BuffSizePtr         is a pointer to the length of the buffer on entry and
                              contains the length of the received data on return
  @param  BufferPtr           pointer to the memory for the received data
  @param  SourceAddrPtr       optional parameter, is a pointer to contain the
                              source ethernet address on return
  @param  DestinationAddrPtr  optional parameter, is a pointer to contain the
                              destination ethernet address on return
  @param  ProtocolPtr         optional parameter, is a pointer to contain the
                              protocol type from the ethernet header on return


**/
EFI_STATUS
EFIAPI
snp_undi32_receive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  OUT UINTN                      *HeaderSizePtr OPTIONAL,
  IN OUT UINTN                   *BuffSizePtr,
  OUT VOID                       *BufferPtr,
  OUT EFI_MAC_ADDRESS            * SourceAddrPtr OPTIONAL,
  OUT EFI_MAC_ADDRESS            * DestinationAddrPtr OPTIONAL,
  OUT UINT16                     *ProtocolPtr OPTIONAL
  )
{
  SNP_DRIVER  *snp;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

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

  if ((BuffSizePtr == NULL) || (BufferPtr == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (!snp->mode.ReceiveFilterSetting) {
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  Status = pxe_receive (
             snp,
             BufferPtr,
             BuffSizePtr,
             HeaderSizePtr,
             SourceAddrPtr,
             DestinationAddrPtr,
             ProtocolPtr
             );

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

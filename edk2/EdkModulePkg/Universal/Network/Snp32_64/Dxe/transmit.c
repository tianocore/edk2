/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module name:
  
    transmit.c

Abstract:

Revision history:
  2000-Feb-03 M(f)J   Genesis.
--*/


#include "Snp.h"

STATIC
EFI_STATUS
pxe_fillheader (
  SNP_DRIVER      *snp,
  VOID            *MacHeaderPtr,
  UINTN           MacHeaderSize,
  VOID            *BufferPtr,
  UINTN           BufferLength,
  EFI_MAC_ADDRESS *DestinationAddrPtr,
  EFI_MAC_ADDRESS *SourceAddrPtr,
  UINT16          *ProtocolPtr
  )
/*++

Routine Description:
 This routine calls undi to create the meadia header for the given data buffer.
 
Arguments:
 snp - pointer to SNP driver structure
 MacHeaderPtr - address where the media header will be filled in.
 MacHeaderSize - size of the memory at MacHeaderPtr
 BufferPtr - data buffer pointer
 BufferLength - Size of data in the BufferPtr
 DestinationAddrPtr - address of the destination mac address buffer
 SourceAddrPtr - address of the source mac address buffer
 ProtocolPtr - address of the protocol type
 
Returns:
 EFI_SUCCESS - if successfully completed the undi call
 Other - error return from undi call.
 
--*/
{
  PXE_CPB_FILL_HEADER_FRAGMENTED  *cpb;
  EFI_STATUS                      Status;
  struct s_v2p                    *pkt_v2p;
  UINT64                          TempData;

  cpb = snp->cpb;
  if (SourceAddrPtr) {
    CopyMem (
      (VOID *) cpb->SrcAddr,
      (VOID *) SourceAddrPtr,
      snp->mode.HwAddressSize
      );
  } else {
    CopyMem (
      (VOID *) cpb->SrcAddr,
      (VOID *) &(snp->mode.CurrentAddress),
      snp->mode.HwAddressSize
      );
  }

  CopyMem (
    (VOID *) cpb->DestAddr,
    (VOID *) DestinationAddrPtr,
    snp->mode.HwAddressSize
    );

  //
  // we need to do the byte swapping
  //
  cpb->Protocol             = (UINT16) PXE_SWAP_UINT16 (*ProtocolPtr);

  cpb->PacketLen            = (UINT32) (BufferLength);
  cpb->MediaHeaderLen       = (UINT16) MacHeaderSize;

  cpb->FragCnt              = 2;
  cpb->reserved             = 0;

  cpb->FragDesc[0].FragAddr = (UINT64) (UINTN) MacHeaderPtr;
  cpb->FragDesc[0].FragLen  = (UINT32) MacHeaderSize;
  cpb->FragDesc[1].FragAddr = (UINT64) (UINTN) BufferPtr;
  cpb->FragDesc[1].FragLen  = (UINT32) BufferLength;

  cpb->FragDesc[0].reserved = cpb->FragDesc[1].reserved = 0;

  if (snp->IsOldUndi) {
    TempData = (UINT64) (UINTN) MacHeaderPtr;
    if (TempData >= FOUR_GIGABYTES) {
      cpb->FragDesc[0].FragAddr = (UINT64) (UINTN) snp->fill_hdr_buf;
      cpb->FragDesc[0].FragLen  = (UINT32) snp->init_info.MediaHeaderLen;
    }

    TempData = (UINT64) (UINTN) (BufferPtr);
    if (TempData >= FOUR_GIGABYTES) {
      //
      // Let the device just read this buffer
      //
      Status = add_v2p (
                &pkt_v2p,
                EfiPciIoOperationBusMasterRead,
                BufferPtr,
                BufferLength
                );
      if (Status != EFI_SUCCESS) {
        return Status;
      }
      //
      // give the virtual address to UNDI and it will call back on Virt2Phys
      // to get the mapped address, if it needs it
      //
      cpb->FragDesc[1].FragLen = (UINT32) pkt_v2p->bsize;
    }
  }

  snp->cdb.OpCode     = PXE_OPCODE_FILL_HEADER;
  snp->cdb.OpFlags    = PXE_OPFLAGS_FILL_HEADER_FRAGMENTED;

  snp->cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  snp->cdb.DBaddr     = PXE_DBADDR_NOT_USED;

  snp->cdb.CPBsize    = sizeof (PXE_CPB_FILL_HEADER_FRAGMENTED);
  snp->cdb.CPBaddr    = (UINT64) (UINTN) cpb;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.fill_header()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->IsOldUndi) {
    TempData = (UINT64) (UINTN) (BufferPtr);
    if (TempData >= FOUR_GIGABYTES) {
      del_v2p (BufferPtr);
    }
    //
    // if we used the global buffer for header, copy the contents
    //
    TempData = (UINT64) (UINTN) MacHeaderPtr;
    if (TempData >= FOUR_GIGABYTES) {
      CopyMem (
        MacHeaderPtr,
        snp->fill_hdr_buf,
        snp->init_info.MediaHeaderLen
        );
    }
  }

  switch (snp->cdb.StatCode) {
  case PXE_STATCODE_SUCCESS:
    return EFI_SUCCESS;

  case PXE_STATCODE_INVALID_PARAMETER:
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.fill_header()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_INVALID_PARAMETER;

  default:
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.fill_header()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }
}

STATIC
EFI_STATUS
pxe_transmit (
  SNP_DRIVER *snp,
  VOID       *BufferPtr,
  UINTN      BufferLength
  )
/*++

Routine Description:
 This routine calls undi to transmit the given data buffer
 
Arguments:
 snp - pointer to SNP driver structure
 BufferPtr - data buffer pointer
 BufferLength - Size of data in the BufferPtr
 
Returns:
 EFI_SUCCESS - if successfully completed the undi call
 Other - error return from undi call.
 
--*/
{
  PXE_CPB_TRANSMIT  *cpb;
  EFI_STATUS        Status;
  struct s_v2p      *v2p;
  UINT64            TempData;

  cpb             = snp->cpb;
  cpb->FrameAddr  = (UINT64) (UINTN) BufferPtr;
  cpb->DataLen    = (UINT32) BufferLength;
  
  TempData = (UINT64) (UINTN) BufferPtr;
  if (snp->IsOldUndi && (TempData >= FOUR_GIGABYTES)) {
    //
    // we need to create a mapping now and give it to the undi when it calls
    // the Virt2Phys on this address.
    // this is a transmit, just map it for the device to READ
    //
    Status = add_v2p (
              &v2p,
              EfiPciIoOperationBusMasterRead,
              BufferPtr,
              BufferLength
              );
    if (Status != EFI_SUCCESS) {
      return Status;
    }

    cpb->DataLen = (UINT32) v2p->bsize;
  }

  cpb->MediaheaderLen = 0;
  cpb->reserved       = 0;

  snp->cdb.OpFlags    = PXE_OPFLAGS_TRANSMIT_WHOLE;

  snp->cdb.CPBsize    = sizeof (PXE_CPB_TRANSMIT);
  snp->cdb.CPBaddr    = (UINT64) (UINTN) cpb;

  snp->cdb.OpCode     = PXE_OPCODE_TRANSMIT;
  snp->cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  snp->cdb.DBaddr     = PXE_DBADDR_NOT_USED;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.transmit()  "));
  DEBUG ((EFI_D_NET, "\nsnp->cdb.OpCode  == %x", snp->cdb.OpCode));
  DEBUG ((EFI_D_NET, "\nsnp->cdb.CPBaddr == %X", snp->cdb.CPBaddr));
  DEBUG ((EFI_D_NET, "\nsnp->cdb.DBaddr  == %X", snp->cdb.DBaddr));
  DEBUG ((EFI_D_NET, "\ncpb->FrameAddr   == %X\n", cpb->FrameAddr));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  DEBUG ((EFI_D_NET, "\nexit snp->undi.transmit()  "));
  DEBUG ((EFI_D_NET, "\nsnp->cdb.StatCode == %r", snp->cdb.StatCode));

  //
  // we will unmap the buffers in get_status call, not here
  //
  switch (snp->cdb.StatCode) {
  case PXE_STATCODE_SUCCESS:
    return EFI_SUCCESS;

  case PXE_STATCODE_QUEUE_FULL:
  case PXE_STATCODE_BUSY:
    Status = EFI_NOT_READY;
    break;

  default:
    Status = EFI_DEVICE_ERROR;
  }

  DEBUG (
    (EFI_D_ERROR,
    "\nsnp->undi.transmit()  %xh:%xh\n",
    snp->cdb.StatFlags,
    snp->cdb.StatCode)
    );

  return Status;
}

EFI_STATUS
EFIAPI
snp_undi32_transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  IN UINTN                       MacHeaderSize,
  IN UINTN                       BufferLength,
  IN VOID                        *BufferPtr,
  IN EFI_MAC_ADDRESS             * SourceAddrPtr OPTIONAL,
  IN EFI_MAC_ADDRESS             * DestinationAddrPtr OPTIONAL,
  IN UINT16                      *ProtocolPtr OPTIONAL
  )
/*++

Routine Description:
 This is the snp interface routine for transmitting a packet. this routine 
 basically retrieves the snp structure, checks the snp state and calls
 pxe_fill_header and pxe_transmit calls to complete the transmission.
 
Arguments:
 this - pointer to SNP driver context
 MacHeaderSize - size of the memory at MacHeaderPtr
 BufferLength - Size of data in the BufferPtr
 BufferPtr - data buffer pointer
 SourceAddrPtr - address of the source mac address buffer
 DestinationAddrPtr - address of the destination mac address buffer
 ProtocolPtr - address of the protocol type
 
Returns:
 EFI_SUCCESS - if successfully completed the undi call
 Other - error return from undi call.
 
--*/
{
  SNP_DRIVER  *snp;
  EFI_STATUS  Status;

  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  if (snp == NULL) {
    return EFI_DEVICE_ERROR;
  }

  switch (snp->mode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
    return EFI_DEVICE_ERROR;

  default:
    return EFI_DEVICE_ERROR;
  }

  if (BufferPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferLength < snp->mode.MediaHeaderSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // if the MacHeaderSize is non-zero, we need to fill up the header and for that
  // we need the destination address and the protocol
  //
  if (MacHeaderSize != 0) {
    if (MacHeaderSize != snp->mode.MediaHeaderSize || DestinationAddrPtr == 0 || ProtocolPtr == 0) {
      return EFI_INVALID_PARAMETER;
    }

    Status = pxe_fillheader (
              snp,
              BufferPtr,
              MacHeaderSize,
              (UINT8 *) BufferPtr + MacHeaderSize,
              BufferLength - MacHeaderSize,
              DestinationAddrPtr,
              SourceAddrPtr,
              ProtocolPtr
              );

    if (Status != EFI_SUCCESS) {
      return Status;
    }
  }

  return pxe_transmit (snp, BufferPtr, BufferLength);
}

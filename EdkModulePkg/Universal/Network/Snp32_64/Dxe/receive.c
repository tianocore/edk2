/*++
Copyright (c) 2006, Intel Corporation                                                         
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
--*/


#include "Snp.h"

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
/*++

Routine Description:
 this routine calls undi to receive a packet and fills in the data in the
 input pointers!
 
Arguments:
  snp  - pointer to snp driver structure
  BufferPtr   - pointer to the memory for the received data
  BuffSizePtr - is a pointer to the length of the buffer on entry and contains
                the length of the received data on return
  HeaderSizePtr - pointer to the header portion of the data received.
  SourceAddrPtr    - optional parameter, is a pointer to contain the source
                ethernet address on return
  DestinationAddrPtr   - optional parameter, is a pointer to contain the destination
                ethernet address on return
  ProtocolPtr    - optional parameter, is a pointer to contain the protocol type
                from the ethernet header on return


Returns:

--*/
{
  PXE_CPB_RECEIVE *cpb;
  PXE_DB_RECEIVE  *db;
  UINTN           buf_size;
  UINT64          TempData;

  cpb       = snp->cpb;
  db        = snp->db;
  buf_size  = *BuffSizePtr;
  //
  // IMPORTANT NOTE:
  // In case of the older 3.0 UNDI, if the input buffer address is beyond 4GB,
  // DO NOT call the map function on the given buffer, instead use
  // a global buffer. The reason is that UNDI3.0 has some unnecessary check of
  // making sure that all the addresses (whether or not they will be given
  // to the NIC ) supplied to it are below 4GB. It may or may not use
  // the mapped address after all (like in case of CPB and DB)!
  // Instead of using the global buffer whose address is allocated within the
  // 2GB limit if I start mapping the given buffer we lose the data, here is
  // why!!!
  // if our address is > 4GB, the map call creates another buffer below 2GB and
  // copies data to/from the original buffer to the mapped buffer either at
  // map time or unmap time depending on the map direction.
  // UNDI will not complain since we already mapped the buffer to be
  // within the 2GB limit but will not use (I know undi) the mapped address
  // since it does not give the user buffers to the NIC's receive unit,
  // It just copies the received packet into the user buffer using the virtual
  // (CPU) address rather than the mapped (device or physical) address.
  // When the UNDI call returns, if we then unmap the buffer, we will lose
  // the contents because unmap copies the contents of the mapped buffer into
  // the original buffer (since the direction is FROM_DEVICE) !!!
  //
  // this is not a problem in Undi 3.1 because this undi uses it's map callback
  // routine to map a cpu address to device address and it does it only if
  // it is giving the address to the device and unmaps it before using the cpu
  // address!
  //
  TempData = (UINT64) (UINTN) BufferPtr;
  if (snp->IsOldUndi && (TempData >= FOUR_GIGABYTES)) {
    cpb->BufferAddr = (UINT64) (UINTN) snp->receive_buf;
    cpb->BufferLen  = (UINT32) (snp->init_info.MediaHeaderLen + snp->init_info.FrameDataLen);
  } else {
    cpb->BufferAddr = (UINT64) (UINTN) BufferPtr;
    cpb->BufferLen  = (UINT32) *BuffSizePtr;
  }

  cpb->reserved       = 0;

  snp->cdb.OpCode     = PXE_OPCODE_RECEIVE;
  snp->cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;

  snp->cdb.CPBsize    = sizeof (PXE_CPB_RECEIVE);
  snp->cdb.CPBaddr    = (UINT64) (UINTN) cpb;

  snp->cdb.DBsize     = sizeof (PXE_DB_RECEIVE);
  snp->cdb.DBaddr     = (UINT64) (UINTN) db;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_INFO, "\nsnp->undi.receive ()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  switch (snp->cdb.StatCode) {
  case PXE_STATCODE_SUCCESS:
    break;

  case PXE_STATCODE_NO_DATA:
    DEBUG (
      (EFI_D_INFO,
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

  TempData = (UINT64) (UINTN) BufferPtr;
  if (snp->IsOldUndi && (TempData >= FOUR_GIGABYTES)) {
    CopyMem (BufferPtr, snp->receive_buf, snp->init_info.MediaHeaderLen + snp->init_info.FrameDataLen);
  }

  return (*BuffSizePtr <= buf_size) ? EFI_SUCCESS : EFI_BUFFER_TOO_SMALL;
}

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
/*++

Routine Description:
 This is the SNP interface routine for receiving network data.
 This routine basically retrieves snp structure, checks the SNP state and
 calls the pxe_receive routine to actually do the receive!

Arguments:
  this  - context pointer
  HeaderSizePtr - optional parameter and is a pointer to the header portion of
                the data received.
  BuffSizePtr - is a pointer to the length of the buffer on entry and contains
                the length of the received data on return
  BufferPtr   - pointer to the memory for the received data
  SourceAddrPtr    - optional parameter, is a pointer to contain the source
                ethernet address on return
  DestinationAddrPtr   - optional parameter, is a pointer to contain the destination
                ethernet address on return
  ProtocolPtr    - optional parameter, is a pointer to contain the protocol type
                from the ethernet header on return

Returns:

--*/
{
  SNP_DRIVER  *snp;

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

  if ((BuffSizePtr == NULL) || (BufferPtr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!snp->mode.ReceiveFilterSetting) {
    return EFI_DEVICE_ERROR;
  }

  return pxe_receive (
          snp,
          BufferPtr,
          BuffSizePtr,
          HeaderSizePtr,
          SourceAddrPtr,
          DestinationAddrPtr,
          ProtocolPtr
          );
}

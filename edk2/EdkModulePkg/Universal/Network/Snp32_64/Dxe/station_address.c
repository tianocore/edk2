/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module name:
  station_address.c

Abstract:

Revision history:
  2000-Feb-17 M(f)J   Genesis.
--*/


#include "Snp.h"

EFI_STATUS
pxe_get_stn_addr (
  SNP_DRIVER *snp
  )
/*++

Routine Description:
 this routine calls undi to read the MAC address of the NIC and updates the
 mode structure with the address.

Arguments:
  snp  - pointer to snp driver structure

Returns:
 
--*/
{
  PXE_DB_STATION_ADDRESS  *db;

  db                  = snp->db;
  snp->cdb.OpCode     = PXE_OPCODE_STATION_ADDRESS;
  snp->cdb.OpFlags    = PXE_OPFLAGS_STATION_ADDRESS_READ;

  snp->cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;

  snp->cdb.DBsize     = sizeof (PXE_DB_STATION_ADDRESS);
  snp->cdb.DBaddr     = (UINT64) (UINTN) db;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.station_addr()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.station_addr()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // Set new station address in SNP->Mode structure and return success.
  //
  CopyMem (
    &(snp->mode.CurrentAddress),
    &db->StationAddr,
    snp->mode.HwAddressSize
    );

  CopyMem (
    &snp->mode.BroadcastAddress,
    &db->BroadcastAddr,
    snp->mode.HwAddressSize
    );

  CopyMem (
    &snp->mode.PermanentAddress,
    &db->PermanentAddr,
    snp->mode.HwAddressSize
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
pxe_set_stn_addr (
  SNP_DRIVER      *snp,
  EFI_MAC_ADDRESS *NewMacAddr
  )
/*++

Routine Description:
 this routine calls undi to set a new MAC address for the NIC,

Arguments:
  snp  - pointer to snp driver structure
  NewMacAddr - pointer to a mac address to be set for the nic, if this is NULL
               then this routine resets the mac address to the NIC's original
               address.

Returns:

--*/
{
  PXE_CPB_STATION_ADDRESS *cpb;
  PXE_DB_STATION_ADDRESS  *db;

  cpb             = snp->cpb;
  db              = snp->db;
  snp->cdb.OpCode = PXE_OPCODE_STATION_ADDRESS;

  if (NewMacAddr == NULL) {
    snp->cdb.OpFlags  = PXE_OPFLAGS_STATION_ADDRESS_RESET;
    snp->cdb.CPBsize  = PXE_CPBSIZE_NOT_USED;
    snp->cdb.CPBaddr  = PXE_CPBADDR_NOT_USED;
  } else {
    snp->cdb.OpFlags = PXE_OPFLAGS_STATION_ADDRESS_READ;
    //
    // even though the OPFLAGS are set to READ, supplying a new address
    // in the CPB will make undi change the mac address to the new one.
    //
    CopyMem (&cpb->StationAddr, NewMacAddr, snp->mode.HwAddressSize);

    snp->cdb.CPBsize  = sizeof (PXE_CPB_STATION_ADDRESS);
    snp->cdb.CPBaddr  = (UINT64) (UINTN) cpb;
  }

  snp->cdb.DBsize     = sizeof (PXE_DB_STATION_ADDRESS);
  snp->cdb.DBaddr     = (UINT64) (UINTN) db;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.station_addr()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.station_addr()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    return EFI_DEVICE_ERROR;
  }
  //
  // read the changed address and save it in SNP->Mode structure
  //
  pxe_get_stn_addr (snp);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
snp_undi32_station_address (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  IN BOOLEAN                     ResetFlag,
  IN EFI_MAC_ADDRESS             * NewMacAddr OPTIONAL
  )
/*++

Routine Description:
 This is the SNP interface routine for changing the NIC's mac address.
 This routine basically retrieves snp structure, checks the SNP state and
 calls the above routines to actually do the work

Arguments:
 this  - context pointer
 NewMacAddr - pointer to a mac address to be set for the nic, if this is NULL
              then this routine resets the mac address to the NIC's original
              address.
 ResetFlag - If true, the mac address will change to NIC's original address

Returns:

--*/
{
  SNP_DRIVER  *snp;
  EFI_STATUS  Status;

  //
  // Get pointer to SNP driver instance for *this.
  //
  if (this == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (this);

  if (snp == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Return error if the SNP is not initialized.
  //
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
  //
  // Check for invalid parameter combinations.
  //
  if (!ResetFlag && NewMacAddr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ResetFlag) {
    Status = pxe_set_stn_addr (snp, NULL);
  } else {
    Status = pxe_set_stn_addr (snp, NewMacAddr);

  }

  return Status;
}

/** @file
    Implementation of reading the MAC address of a network adapter.
 
Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed 
and made available under the terms and conditions of the BSD License which 
accompanies this distribution. The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php 

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Snp.h"


/**
  Call UNDI to read the MAC address of the NIC and update the mode structure 
  with the address. 

  @param  Snp         Pointer to snp driver structure.
   
  @retval EFI_SUCCESS       The MAC address of the NIC is read successfully.
  @retval EFI_DEVICE_ERROR  Failed to read the MAC address of the NIC.

**/
EFI_STATUS
PxeGetStnAddr (
  SNP_DRIVER *Snp
  )
{
  PXE_DB_STATION_ADDRESS  *Db;

  Db                  = Snp->Db;
  Snp->Cdb.OpCode     = PXE_OPCODE_STATION_ADDRESS;
  Snp->Cdb.OpFlags    = PXE_OPFLAGS_STATION_ADDRESS_READ;

  Snp->Cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  Snp->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;

  Snp->Cdb.DBsize     = (UINT16) sizeof (PXE_DB_STATION_ADDRESS);
  Snp->Cdb.DBaddr     = (UINT64)(UINTN) Db;

  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.station_addr()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.station_addr()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // Set new station address in SNP->Mode structure and return success.
  //
  CopyMem (
    &(Snp->Mode.CurrentAddress),
    &Db->StationAddr,
    Snp->Mode.HwAddressSize
    );

  CopyMem (
    &Snp->Mode.BroadcastAddress,
    &Db->BroadcastAddr,
    Snp->Mode.HwAddressSize
    );

  CopyMem (
    &Snp->Mode.PermanentAddress,
    &Db->PermanentAddr,
    Snp->Mode.HwAddressSize
    );

  return EFI_SUCCESS;
}


/**
  Call UNDI to set a new MAC address for the NIC.

  @param  Snp         Pointer to Snp driver structure.
  @param  NewMacAddr  Pointer to a MAC address to be set for the NIC, if this is
                      NULL then this routine resets the mac address to the NIC's
                      original address.


**/
EFI_STATUS
PxeSetStnAddr (
  SNP_DRIVER      *Snp,
  EFI_MAC_ADDRESS *NewMacAddr
  )
{
  PXE_CPB_STATION_ADDRESS *Cpb;
  PXE_DB_STATION_ADDRESS  *Db;

  Cpb             = Snp->Cpb;
  Db              = Snp->Db;
  Snp->Cdb.OpCode = PXE_OPCODE_STATION_ADDRESS;

  if (NewMacAddr == NULL) {
    Snp->Cdb.OpFlags  = PXE_OPFLAGS_STATION_ADDRESS_RESET;
    Snp->Cdb.CPBsize  = PXE_CPBSIZE_NOT_USED;
    Snp->Cdb.CPBaddr  = PXE_CPBADDR_NOT_USED;
  } else {
    Snp->Cdb.OpFlags = PXE_OPFLAGS_STATION_ADDRESS_READ;
    //
    // even though the OPFLAGS are set to READ, supplying a new address
    // in the CPB will make undi change the mac address to the new one.
    //
    CopyMem (&Cpb->StationAddr, NewMacAddr, Snp->Mode.HwAddressSize);

    Snp->Cdb.CPBsize  = (UINT16) sizeof (PXE_CPB_STATION_ADDRESS);
    Snp->Cdb.CPBaddr  = (UINT64)(UINTN) Cpb;
  }

  Snp->Cdb.DBsize     = (UINT16) sizeof (PXE_DB_STATION_ADDRESS);
  Snp->Cdb.DBaddr     = (UINT64)(UINTN) Db;

  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.station_addr()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.station_addr()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    return EFI_DEVICE_ERROR;
  }
  //
  // read the changed address and save it in SNP->Mode structure
  //
  PxeGetStnAddr (Snp);

  return EFI_SUCCESS;
}


/**
  Modifies or resets the current station address, if supported.
  
  This function modifies or resets the current station address of a network 
  interface, if supported. If Reset is TRUE, then the current station address is
  set to the network interface's permanent address. If Reset is FALSE, and the 
  network interface allows its station address to be modified, then the current 
  station address is changed to the address specified by New. If the network 
  interface does not allow its station address to be modified, then 
  EFI_INVALID_PARAMETER will be returned. If the station address is successfully
  updated on the network interface, EFI_SUCCESS will be returned. If the driver
  has not been initialized, EFI_DEVICE_ERROR will be returned.

  @param This  A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param Reset Flag used to reset the station address to the network interface's 
               permanent address.
  @param New   New station address to be used for the network interface.


  @retval EFI_SUCCESS           The network interface's station address was updated.
  @retval EFI_NOT_STARTED       The Simple Network Protocol interface has not been 
                                started by calling Start().
  @retval EFI_INVALID_PARAMETER The New station address was not accepted by the NIC.
  @retval EFI_INVALID_PARAMETER Reset is FALSE and New is NULL.
  @retval EFI_DEVICE_ERROR      The Simple Network Protocol interface has not 
                                been initialized by calling Initialize().
  @retval EFI_DEVICE_ERROR      An error occurred attempting to set the new 
                                station address.
  @retval EFI_UNSUPPORTED       The NIC does not support changing the network 
                                interface's station address.

**/
EFI_STATUS
EFIAPI
SnpUndi32StationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *New OPTIONAL
  )
{
  SNP_DRIVER  *Snp;
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  //
  // Check for invalid parameter combinations.
  //
  if ((This == NULL) ||
    (!Reset && (New == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Return error if the SNP is not initialized.
  //
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

  if (Reset) {
    Status = PxeSetStnAddr (Snp, NULL);
  } else {
    Status = PxeSetStnAddr (Snp, New);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module name:
  receive_filters.c

Abstract:

Revision history:
  2000-Feb-17 M(f)J   Genesis.
--*/


#include "Snp.h"

STATIC
EFI_STATUS
pxe_rcvfilter_enable (
  SNP_DRIVER      *snp,
  UINT32          EnableFlags,
  UINTN           MCastAddressCount,
  EFI_MAC_ADDRESS *MCastAddressList
  )
/*++

Routine Description:
 this routine calls undi to enable the receive filters.

Arguments:
  snp  - pointer to snp driver structure
  EnableFlags - bit mask for enabling the receive filters
  MCastAddressCount - multicast address count for a new multicast address list
  MCastAddressList  - list of new multicast addresses

Returns:

--*/
{
  snp->cdb.OpCode     = PXE_OPCODE_RECEIVE_FILTERS;
  snp->cdb.OpFlags    = PXE_OPFLAGS_RECEIVE_FILTER_ENABLE;
  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  snp->cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  snp->cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  snp->cdb.DBaddr     = PXE_DBADDR_NOT_USED;
  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_UNICAST;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
  }

  if (MCastAddressCount != 0) {
    snp->cdb.CPBsize  = (UINT16) (MCastAddressCount * sizeof (EFI_MAC_ADDRESS));
    snp->cdb.CPBaddr  = (UINT64) (UINTN) snp->cpb;
    CopyMem (snp->cpb, MCastAddressList, snp->cdb.CPBsize);
  }
  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.receive_filters()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->cdb.StatCode != EFI_SUCCESS) {
    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.receive_filters()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    switch (snp->cdb.StatCode) {
    case PXE_STATCODE_INVALID_CDB:
    case PXE_STATCODE_INVALID_CPB:
    case PXE_STATCODE_INVALID_PARAMETER:
      return EFI_INVALID_PARAMETER;

    case PXE_STATCODE_UNSUPPORTED:
      return EFI_UNSUPPORTED;
    }

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
pxe_rcvfilter_disable (
  SNP_DRIVER *snp,
  UINT32     DisableFlags,
  BOOLEAN    ResetMCastList
  )
/*++

Routine Description:
 this routine calls undi to disable the receive filters.

Arguments:
  snp  - pointer to snp driver structure
  DisableFlags - bit mask for disabling the receive filters
  ResetMCastList - boolean flag to reset/delete the multicast filter list

Returns:

--*/
{
  snp->cdb.OpCode     = PXE_OPCODE_RECEIVE_FILTERS;
  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  snp->cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  snp->cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  snp->cdb.DBaddr     = PXE_DBADDR_NOT_USED;
  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  snp->cdb.OpFlags    = (UINT16) (DisableFlags ? PXE_OPFLAGS_RECEIVE_FILTER_DISABLE : PXE_OPFLAGS_NOT_USED);

  if (ResetMCastList) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_UNICAST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    snp->cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
  }
  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.receive_filters()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->cdb.StatCode != EFI_SUCCESS) {
    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.receive_filters()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
pxe_rcvfilter_read (
  SNP_DRIVER *snp
  )
/*++

Routine Description:
 this routine calls undi to read the receive filters.

Arguments:
  snp  - pointer to snp driver structure

Returns:

--*/
{
  snp->cdb.OpCode   = PXE_OPCODE_RECEIVE_FILTERS;
  snp->cdb.OpFlags  = PXE_OPFLAGS_RECEIVE_FILTER_READ;
  snp->cdb.CPBsize  = PXE_CPBSIZE_NOT_USED;
  snp->cdb.DBsize   = (UINT16) (snp->mode.MaxMCastFilterCount * sizeof (EFI_MAC_ADDRESS));
  snp->cdb.CPBaddr  = PXE_CPBADDR_NOT_USED;
  if (snp->cdb.DBsize == 0) {
    snp->cdb.DBaddr = (UINT64) NULL;
  } else {
    snp->cdb.DBaddr = (UINT64) (UINTN) snp->db;
    ZeroMem (snp->db, snp->cdb.DBsize);
  }

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  DEBUG ((EFI_D_NET, "\nsnp->undi.receive_filters()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->cdb.StatCode != EFI_SUCCESS) {
    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.receive_filters()  %xh:%xh\n",
      snp->cdb.StatFlags,
      snp->cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // Convert UNDI32 StatFlags to EFI SNP filter flags.
  //
  snp->mode.ReceiveFilterSetting = 0;

  if ((snp->cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_UNICAST) != 0) {
    snp->mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;
  }

  if ((snp->cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_BROADCAST) != 0) {
    snp->mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;
  }

  if ((snp->cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_PROMISCUOUS) != 0) {
    snp->mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  }

  if ((snp->cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0) {
    snp->mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
  }

  if ((snp->cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) != 0) {
    snp->mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST;
  }

  CopyMem (snp->mode.MCastFilter, snp->db, snp->cdb.DBsize);

  //
  // Count number of active entries in multicast filter list.
  //
  {
    EFI_MAC_ADDRESS ZeroMacAddr;

    SetMem (&ZeroMacAddr, sizeof ZeroMacAddr, 0);

    for (snp->mode.MCastFilterCount = 0;
         snp->mode.MCastFilterCount < snp->mode.MaxMCastFilterCount;
         snp->mode.MCastFilterCount++
        ) {
      if (CompareMem (
            &snp->mode.MCastFilter[snp->mode.MCastFilterCount],
            &ZeroMacAddr,
            sizeof ZeroMacAddr
            ) == 0) {
        break;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
snp_undi32_receive_filters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * this,
  IN UINT32                      EnableFlags,
  IN UINT32                      DisableFlags,
  IN BOOLEAN                     ResetMCastList,
  IN UINTN                       MCastAddressCount OPTIONAL,
  IN EFI_MAC_ADDRESS             * MCastAddressList OPTIONAL
  )
/*++

Routine Description:
 This is the SNP interface routine for reading/enabling/disabling the
 receive filters.
 This routine basically retrieves snp structure, checks the SNP state and
 checks the parameter validity, calls one of the above routines to actually
 do the work

Arguments:
  this  - context pointer
  EnableFlags - bit mask for enabling the receive filters
  DisableFlags - bit mask for disabling the receive filters
  ResetMCastList - boolean flag to reset/delete the multicast filter list
  MCastAddressCount - multicast address count for a new multicast address list
  MCastAddressList  - list of new multicast addresses

Returns:

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
  //
  // check if we are asked to enable or disable something that the UNDI
  // does not even support!
  //
  if ((EnableFlags &~snp->mode.ReceiveFilterMask) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DisableFlags &~snp->mode.ReceiveFilterMask) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (ResetMCastList) {
    DisableFlags |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST & snp->mode.ReceiveFilterMask;
    MCastAddressCount = 0;
    MCastAddressList  = NULL;
  } else {
    if (MCastAddressCount != 0) {
      if (MCastAddressCount > snp->mode.MaxMCastFilterCount) {
        return EFI_INVALID_PARAMETER;
      }

      if (MCastAddressList == NULL) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  if (EnableFlags == 0 && DisableFlags == 0 && !ResetMCastList && MCastAddressCount == 0) {
    return EFI_SUCCESS;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0 && MCastAddressCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EnableFlags != 0) || (MCastAddressCount != 0)) {
    Status = pxe_rcvfilter_enable (
              snp,
              EnableFlags,
              MCastAddressCount,
              MCastAddressList
              );

    if (Status != EFI_SUCCESS) {
      return Status;
    }
  }

  if ((DisableFlags != 0) || ResetMCastList) {
    Status = pxe_rcvfilter_disable (snp, DisableFlags, ResetMCastList);

    if (Status != EFI_SUCCESS) {
      return Status;
    }
  }

  return pxe_rcvfilter_read (snp);
}

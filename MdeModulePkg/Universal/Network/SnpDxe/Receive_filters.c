/** @file
    Implementation of managing the multicast receive filters of a network
    interface.

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under the 
terms and conditions of the BSD License which accompanies this distribution. The 
full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php 

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/



#include "Snp.h"

/**
  Call undi to enable the receive filters.

  @param  Snp                Pointer to snp driver structure.
  @param  EnableFlags        Bit mask for enabling the receive filters.
  @param  MCastAddressCount  Multicast address count for a new multicast address
                             list.
  @param  MCastAddressList   List of new multicast addresses. 
   
  @retval EFI_SUCCESS           The multicast receive filter list was updated.
  @retval EFI_INVALID_PARAMETER Invalid UNDI command.
  @retval EFI_UNSUPPORTED       Command is not supported by UNDI.
  @retval EFI_DEVICE_ERROR      Fail to execute UNDI command.

**/
EFI_STATUS
PxeRecvFilterEnable (
  SNP_DRIVER      *Snp,
  UINT32          EnableFlags,
  UINTN           MCastAddressCount,
  EFI_MAC_ADDRESS *MCastAddressList
  )
{
  Snp->Cdb.OpCode     = PXE_OPCODE_RECEIVE_FILTERS;
  Snp->Cdb.OpFlags    = PXE_OPFLAGS_RECEIVE_FILTER_ENABLE;
  Snp->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  Snp->Cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  Snp->Cdb.DBaddr     = PXE_DBADDR_NOT_USED;
  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_UNICAST;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;
  }

  if ((EnableFlags & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
  }

  if (MCastAddressCount != 0) {
    Snp->Cdb.CPBsize  = (UINT16) (MCastAddressCount * sizeof (EFI_MAC_ADDRESS));
    Snp->Cdb.CPBaddr  = (UINT64)(UINTN)Snp->Cpb;
    CopyMem (Snp->Cpb, MCastAddressList, Snp->Cdb.CPBsize);
  }
  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.receive_filters()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != EFI_SUCCESS) {
    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.receive_filters()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    switch (Snp->Cdb.StatCode) {
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

/**
  Call undi to disable the receive filters.

  @param  Snp             Pointer to snp driver structure
  @param  DisableFlags    Bit mask for disabling the receive filters
  @param  ResetMCastList  Boolean flag to reset/delete the multicast filter 
                          list.

  @retval EFI_SUCCESS           The multicast receive filter list was updated.
  @retval EFI_DEVICE_ERROR      Fail to execute UNDI command.
   
**/ 
EFI_STATUS
PxeRecvFilterDisable (
  SNP_DRIVER *Snp,
  UINT32     DisableFlags,
  BOOLEAN    ResetMCastList
  )
{
  Snp->Cdb.OpCode     = PXE_OPCODE_RECEIVE_FILTERS;
  Snp->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.DBsize     = PXE_DBSIZE_NOT_USED;
  Snp->Cdb.CPBaddr    = PXE_CPBADDR_NOT_USED;
  Snp->Cdb.DBaddr     = PXE_DBADDR_NOT_USED;
  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  Snp->Cdb.OpFlags    = (UINT16) ((DisableFlags != 0) ? PXE_OPFLAGS_RECEIVE_FILTER_DISABLE : PXE_OPFLAGS_NOT_USED);

  if (ResetMCastList) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_UNICAST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;
  }

  if ((DisableFlags & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    Snp->Cdb.OpFlags |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
  }
  //
  // Issue UNDI command and check result.
  //
  DEBUG ((EFI_D_NET, "\nsnp->undi.receive_filters()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != EFI_SUCCESS) {
    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.receive_filters()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Call undi to read the receive filters.

  @param  Snp                Pointer to snp driver structure.

  @retval EFI_SUCCESS           The receive filter was read.
  @retval EFI_DEVICE_ERROR      Fail to execute UNDI command. 
   
**/
EFI_STATUS
PxeRecvFilterRead (
  SNP_DRIVER *Snp
  )
{
  Snp->Cdb.OpCode   = PXE_OPCODE_RECEIVE_FILTERS;
  Snp->Cdb.OpFlags  = PXE_OPFLAGS_RECEIVE_FILTER_READ;
  Snp->Cdb.CPBsize  = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.DBsize   = (UINT16) (Snp->Mode.MaxMCastFilterCount * sizeof (EFI_MAC_ADDRESS));
  Snp->Cdb.CPBaddr  = PXE_CPBADDR_NOT_USED;
  if (Snp->Cdb.DBsize == 0) {
    Snp->Cdb.DBaddr = (UINT64)(UINTN) NULL;
  } else {
    Snp->Cdb.DBaddr = (UINT64)(UINTN) Snp->Db;
    ZeroMem (Snp->Db, Snp->Cdb.DBsize);
  }

  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  DEBUG ((EFI_D_NET, "\nsnp->undi.receive_filters()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != EFI_SUCCESS) {
    //
    // UNDI command failed.  Return UNDI status to caller.
    //
    DEBUG (
      (EFI_D_ERROR,
      "\nsnp->undi.receive_filters()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    return EFI_DEVICE_ERROR;
  }
  //
  // Convert UNDI32 StatFlags to EFI SNP filter flags.
  //
  Snp->Mode.ReceiveFilterSetting = 0;

  if ((Snp->Cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_UNICAST) != 0) {
    Snp->Mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;
  }

  if ((Snp->Cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_BROADCAST) != 0) {
    Snp->Mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;
  }

  if ((Snp->Cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_PROMISCUOUS) != 0) {
    Snp->Mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  }

  if ((Snp->Cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0) {
    Snp->Mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
  }

  if ((Snp->Cdb.StatFlags & PXE_STATFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) != 0) {
    Snp->Mode.ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST;
  }

  CopyMem (Snp->Mode.MCastFilter, Snp->Db, Snp->Cdb.DBsize);

  //
  // Count number of active entries in multicast filter list.
  //
  {
    EFI_MAC_ADDRESS ZeroMacAddr;

    SetMem (&ZeroMacAddr, sizeof ZeroMacAddr, 0);

    for (Snp->Mode.MCastFilterCount = 0;
         Snp->Mode.MCastFilterCount < Snp->Mode.MaxMCastFilterCount;
         Snp->Mode.MCastFilterCount++
        ) {
      if (CompareMem (
            &Snp->Mode.MCastFilter[Snp->Mode.MCastFilterCount],
            &ZeroMacAddr,
            sizeof ZeroMacAddr
            ) == 0) {
        break;
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  Manages the multicast receive filters of a network interface.
  
  This function is used enable and disable the hardware and software receive 
  filters for the underlying network device.
  The receive filter change is broken down into three steps: 
  * The filter mask bits that are set (ON) in the Enable parameter are added to 
    the current receive filter settings. 
  * The filter mask bits that are set (ON) in the Disable parameter are subtracted
    from the updated receive filter settings.
  * If the resulting receive filter setting is not supported by the hardware a
    more liberal setting is selected.
  If the same bits are set in the Enable and Disable parameters, then the bits 
  in the Disable parameter takes precedence.
  If the ResetMCastFilter parameter is TRUE, then the multicast address list 
  filter is disabled (irregardless of what other multicast bits are set in the 
  Enable and Disable parameters). The SNP->Mode->MCastFilterCount field is set 
  to zero. The Snp->Mode->MCastFilter contents are undefined.
  After enabling or disabling receive filter settings, software should verify 
  the new settings by checking the Snp->Mode->ReceiveFilterSettings, 
  Snp->Mode->MCastFilterCount and Snp->Mode->MCastFilter fields.
  Note: Some network drivers and/or devices will automatically promote receive 
    filter settings if the requested setting can not be honored. For example, if
    a request for four multicast addresses is made and the underlying hardware 
    only supports two multicast addresses the driver might set the promiscuous 
    or promiscuous multicast receive filters instead. The receiving software is
    responsible for discarding any extra packets that get through the hardware 
    receive filters.
    Note: Note: To disable all receive filter hardware, the network driver must 
      be Shutdown() and Stopped(). Calling ReceiveFilters() with Disable set to
      Snp->Mode->ReceiveFilterSettings will make it so no more packets are 
      returned by the Receive() function, but the receive hardware may still be 
      moving packets into system memory before inspecting and discarding them.
      Unexpected system errors, reboots and hangs can occur if an OS is loaded 
      and the network devices are not Shutdown() and Stopped().
  If ResetMCastFilter is TRUE, then the multicast receive filter list on the 
  network interface will be reset to the default multicast receive filter list.
  If ResetMCastFilter is FALSE, and this network interface allows the multicast 
  receive filter list to be modified, then the MCastFilterCnt and MCastFilter 
  are used to update the current multicast receive filter list. The modified 
  receive filter list settings can be found in the MCastFilter field of 
  EFI_SIMPLE_NETWORK_MODE. If the network interface does not allow the multicast
  receive filter list to be modified, then EFI_INVALID_PARAMETER will be returned.
  If the driver has not been initialized, EFI_DEVICE_ERROR will be returned.
  If the receive filter mask and multicast receive filter list have been 
  successfully updated on the network interface, EFI_SUCCESS will be returned.

  @param This             A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param Enable           A bit mask of receive filters to enable on the network
                          interface.
  @param Disable          A bit mask of receive filters to disable on the network
                          interface. For backward compatibility with EFI 1.1 
                          platforms, the EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST bit
                          must be set when the ResetMCastFilter parameter is TRUE.
  @param ResetMCastFilter Set to TRUE to reset the contents of the multicast 
                          receive filters on the network interface to their 
                          default values. 
  @param MCastFilterCnt   Number of multicast HW MAC addresses in the new MCastFilter
                          list. This value must be less than or equal to the 
                          MCastFilterCnt field of EFI_SIMPLE_NETWORK_MODE. 
                          This field is optional if ResetMCastFilter is TRUE.
  @param MCastFilter      A pointer to a list of new multicast receive filter HW
                          MAC addresses. This list will replace any existing 
                          multicast HW MAC address list. This field is optional 
                          if ResetMCastFilter is TRUE.
   
  @retval EFI_SUCCESS            The multicast receive filter list was updated.
  @retval EFI_NOT_STARTED        The network interface has not been started.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 * This is NULL
                                 * There are bits set in Enable that are not set
                                   in Snp->Mode->ReceiveFilterMask
                                 * There are bits set in Disable that are not set
                                   in Snp->Mode->ReceiveFilterMask
                                 * Multicast is being enabled (the 
                                   EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST bit is 
                                   set in Enable, it is not set in Disable, and 
                                   ResetMCastFilter is FALSE) and MCastFilterCount
                                   is zero
                                 * Multicast is being enabled and MCastFilterCount
                                   is greater than Snp->Mode->MaxMCastFilterCount
                                 * Multicast is being enabled and MCastFilter is NULL
                                 * Multicast is being enabled and one or more of
                                   the addresses in the MCastFilter list are not
                                   valid multicast MAC addresses
  @retval EFI_DEVICE_ERROR       One or more of the following conditions is TRUE:
                                 * The network interface has been started but has
                                   not been initialized
                                 * An unexpected error was returned by the 
                                   underlying network driver or device
  @retval EFI_UNSUPPORTED        This function is not supported by the network
                                 interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32ReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINT32                      Enable,
  IN UINT32                      Disable,
  IN BOOLEAN                     ResetMCastFilter,
  IN UINTN                       MCastFilterCnt,  OPTIONAL
  IN EFI_MAC_ADDRESS             *MCastFilter     OPTIONAL
  )
{
  SNP_DRIVER  *Snp;
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

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
  //
  // check if we are asked to enable or disable something that the UNDI
  // does not even support!
  //
  if (((Enable &~Snp->Mode.ReceiveFilterMask) != 0) ||
    ((Disable &~Snp->Mode.ReceiveFilterMask) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (ResetMCastFilter) {

    Disable |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST & Snp->Mode.ReceiveFilterMask;
    MCastFilterCnt = 0;
    MCastFilter    = NULL;
  } else {
    if (MCastFilterCnt != 0) {
      if ((MCastFilterCnt > Snp->Mode.MaxMCastFilterCount) ||
          (MCastFilter == NULL)) {

        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }
    }
  }

  if (Enable == 0 && Disable == 0 && !ResetMCastFilter && MCastFilterCnt == 0) {
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  if ((Enable & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0 && MCastFilterCnt == 0) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if ((Enable != 0) || (MCastFilterCnt != 0)) {
    Status = PxeRecvFilterEnable (
               Snp,
               Enable,
               MCastFilterCnt,
               MCastFilter
               );

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  if ((Disable != 0) || ResetMCastFilter) {
    Status = PxeRecvFilterDisable (Snp, Disable, ResetMCastFilter);

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  Status = PxeRecvFilterRead (Snp);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

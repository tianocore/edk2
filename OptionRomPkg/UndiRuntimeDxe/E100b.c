/** @file
  Provides basic function upon network adapter card.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Undi32.h"

UINT8 basic_config_cmd[22] = {
                    22,        0x08,
                    0,           0,
                    0, (UINT8)0x80,
                    0x32,        0x03,
                    1,            0,
                    0x2E,           0,
                    0x60,           0,
                    (UINT8)0xf2,        0x48,
                    0,        0x40,
                    (UINT8)0xf2, (UINT8)0x80, // 0x40=Force full-duplex
                    0x3f,       0x05,
};

//
// How to wait for the command unit to accept a command.
// Typically this takes 0 ticks.
//
#define wait_for_cmd_done(cmd_ioaddr) \
{                      \
  INT16 wait_count = 2000;              \
  while ((InByte (AdapterInfo, cmd_ioaddr) != 0) && --wait_count >= 0)  \
    DelayIt (AdapterInfo, 10);  \
  if (wait_count == 0) \
    DelayIt (AdapterInfo, 50);    \
}


/**
  This function calls the MemIo callback to read a byte from the device's
  address space
  Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
  which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
  to make undi3.0 a special case

  @param  Port                            Which port to read from.

  @retval Results                         The data read from the port.

**/
// TODO:    AdapterInfo - add argument and description to function comment
UINT8
InByte (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT32            Port
  )
{
  UINT8 Results;

  (*AdapterInfo->Mem_Io) (
    AdapterInfo->Unique_ID,
    PXE_MEM_READ,
    1,
    (UINT64)Port,
    (UINT64) (UINTN) &Results
    );
  return Results;
}


/**
  This function calls the MemIo callback to read a word from the device's
  address space
  Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
  which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
  to make undi3.0 a special case

  @param  Port                            Which port to read from.

  @retval Results                         The data read from the port.

**/
// TODO:    AdapterInfo - add argument and description to function comment
UINT16
InWord (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT32            Port
  )
{
  UINT16  Results;

  (*AdapterInfo->Mem_Io) (
    AdapterInfo->Unique_ID,
    PXE_MEM_READ,
    2,
    (UINT64)Port,
    (UINT64)(UINTN)&Results
    );
  return Results;
}


/**
  This function calls the MemIo callback to read a dword from the device's
  address space
  Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
  which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
  to make undi3.0 a special case

  @param  Port                            Which port to read from.

  @retval Results                         The data read from the port.

**/
// TODO:    AdapterInfo - add argument and description to function comment
UINT32
InLong (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT32            Port
  )
{
  UINT32  Results;

  (*AdapterInfo->Mem_Io) (
    AdapterInfo->Unique_ID,
    PXE_MEM_READ,
    4,
    (UINT64)Port,
    (UINT64)(UINTN)&Results
    );
  return Results;
}


/**
  This function calls the MemIo callback to write a byte from the device's
  address space
  Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
  which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
  to make undi3.0 a special case

  @param  Data                            Data to write to Port.
  @param  Port                            Which port to write to.

  @return none

**/
// TODO:    AdapterInfo - add argument and description to function comment
VOID
OutByte (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT8             Data,
  IN UINT32            Port
  )
{
  UINT8 Val;

  Val = Data;
  (*AdapterInfo->Mem_Io) (
     AdapterInfo->Unique_ID,
     PXE_MEM_WRITE,
     1,
     (UINT64)Port,
     (UINT64)(UINTN)(UINTN)&Val
     );
  return ;
}


/**
  This function calls the MemIo callback to write a word from the device's
  address space
  Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
  which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
  to make undi3.0 a special case

  @param  Data                            Data to write to Port.
  @param  Port                            Which port to write to.

  @return none

**/
// TODO:    AdapterInfo - add argument and description to function comment
VOID
OutWord (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT16            Data,
  IN UINT32            Port
  )
{
  UINT16  Val;

  Val = Data;
  (*AdapterInfo->Mem_Io) (
     AdapterInfo->Unique_ID,
     PXE_MEM_WRITE,
     2,
     (UINT64)Port,
     (UINT64)(UINTN)&Val
     );
  return ;
}


/**
  This function calls the MemIo callback to write a dword from the device's
  address space
  Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
  which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
  to make undi3.0 a special case

  @param  Data                            Data to write to Port.
  @param  Port                            Which port to write to.

  @return none

**/
// TODO:    AdapterInfo - add argument and description to function comment
VOID
OutLong (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT32            Data,
  IN UINT32            Port
  )
{
  UINT32  Val;

  Val = Data;
  (*AdapterInfo->Mem_Io) (
     AdapterInfo->Unique_ID,
     PXE_MEM_WRITE,
     4,
     (UINT64)Port,
     (UINT64)(UINTN)&Val
     );
  return ;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  MemAddr                         TODO: add argument description
  @param  Size                            TODO: add argument description
  @param  Direction                       TODO: add argument description
  @param  MappedAddr                      TODO: add argument description

  @return TODO: add return values

**/
UINTN
MapIt (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT64            MemAddr,
  IN UINT32            Size,
  IN UINT32            Direction,
  OUT UINT64           MappedAddr
  )
{
  UINT64  *PhyAddr;

  PhyAddr = (UINT64 *) (UINTN) MappedAddr;
  //
  // mapping is different for theold and new NII protocols
  //
  if (AdapterInfo->VersionFlag == 0x30) {
    if (AdapterInfo->Virt2Phys_30 == (VOID *) NULL) {
      *PhyAddr = (UINT64) AdapterInfo->MemoryPtr;
    } else {
      (*AdapterInfo->Virt2Phys_30) (MemAddr, (UINT64) (UINTN) PhyAddr);
    }

    if (*PhyAddr > FOUR_GIGABYTE) {
      return PXE_STATCODE_INVALID_PARAMETER;
    }
  } else {
    if (AdapterInfo->Map_Mem == (VOID *) NULL) {
      //
      // this UNDI cannot handle addresses beyond 4 GB without a map routine
      //
      if (MemAddr > FOUR_GIGABYTE) {
        return PXE_STATCODE_INVALID_PARAMETER;
      } else {
        *PhyAddr = MemAddr;
      }
    } else {
      (*AdapterInfo->Map_Mem) (
        AdapterInfo->Unique_ID,
        MemAddr,
        Size,
        Direction,
        MappedAddr
        );
    }
  }

  return PXE_STATCODE_SUCCESS;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  MemAddr                         TODO: add argument description
  @param  Size                            TODO: add argument description
  @param  Direction                       TODO: add argument description
  @param  MappedAddr                      TODO: add argument description

  @return TODO: add return values

**/
VOID
UnMapIt (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT64            MemAddr,
  IN UINT32            Size,
  IN UINT32            Direction,
  IN UINT64            MappedAddr
  )
{
  if (AdapterInfo->VersionFlag > 0x30) {
    //
    // no mapping service
    //
    if (AdapterInfo->UnMap_Mem != (VOID *) NULL) {
      (*AdapterInfo->UnMap_Mem) (
        AdapterInfo->Unique_ID,
        MemAddr,
        Size,
        Direction,
        MappedAddr
        );

    }
  }

  return ;
}


/**

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..


**/
// TODO:    MicroSeconds - add argument and description to function comment
VOID
DelayIt (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  UINT16               MicroSeconds
  )
{
  if (AdapterInfo->VersionFlag == 0x30) {
    (*AdapterInfo->Delay_30) (MicroSeconds);
  } else {
    (*AdapterInfo->Delay) (AdapterInfo->Unique_ID, MicroSeconds);
  }
}


/**

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..


**/
// TODO:    flag - add argument and description to function comment
VOID
BlockIt (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  UINT32               flag
  )
{
  if (AdapterInfo->VersionFlag == 0x30) {
    (*AdapterInfo->Block_30) (flag);
  } else {
    (*AdapterInfo->Block) (AdapterInfo->Unique_ID, flag);
  }
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT8
Load_Base_Regs (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  //
  // we will use the linear (flat) memory model and fill our base registers
  // with 0's so that the entire physical address is our offset
  //
  //
  // we reset the statistics totals here because this is where we are loading stats addr
  //
  AdapterInfo->RxTotals = 0;
  AdapterInfo->TxTotals = 0;

  //
  // Load the statistics block address.
  //
  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);
  OutLong (AdapterInfo, (UINT32) AdapterInfo->stat_phy_addr, AdapterInfo->ioaddr + SCBPointer);
  OutByte (AdapterInfo, CU_STATSADDR, AdapterInfo->ioaddr + SCBCmd);
  AdapterInfo->statistics->done_marker = 0;

  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);
  OutLong (AdapterInfo, 0, AdapterInfo->ioaddr + SCBPointer);
  OutByte (AdapterInfo, RX_ADDR_LOAD, AdapterInfo->ioaddr + SCBCmd);

  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);
  OutLong (AdapterInfo, 0, AdapterInfo->ioaddr + SCBPointer);
  OutByte (AdapterInfo, CU_CMD_BASE, AdapterInfo->ioaddr + SCBCmd);

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  cmd_ptr                         TODO: add argument description

  @return TODO: add return values

**/
UINT8
IssueCB (
  NIC_DATA_INSTANCE *AdapterInfo,
  TxCB              *cmd_ptr
  )
{
  UINT16  status;

  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);

  //
  // read the CU status, if it is idle, write the address of cb_ptr
  // in the scbpointer and issue a cu_start,
  // if it is suspended, remove the suspend bit in the previous command
  // block and issue a resume
  //
  // Ensure that the CU Active Status bit is not on from previous CBs.
  //
  status = InWord (AdapterInfo, AdapterInfo->ioaddr + SCBStatus);

  //
  // Skip acknowledging the interrupt if it is not already set
  //

  //
  // ack only the cna the integer
  //
  if ((status & SCB_STATUS_CNA) != 0) {
    OutWord (AdapterInfo, SCB_STATUS_CNA, AdapterInfo->ioaddr + SCBStatus);

  }

  if ((status & SCB_STATUS_CU_MASK) == SCB_STATUS_CU_IDLE) {
    //
    // give a cu_start
    //
    OutLong (AdapterInfo, cmd_ptr->PhysTCBAddress, AdapterInfo->ioaddr + SCBPointer);
    OutByte (AdapterInfo, CU_START, AdapterInfo->ioaddr + SCBCmd);
  } else {
    //
    // either active or suspended, give a resume
    //

    cmd_ptr->PrevTCBVirtualLinkPtr->cb_header.command &= ~(CmdSuspend | CmdIntr);
    OutByte (AdapterInfo, CU_RESUME, AdapterInfo->ioaddr + SCBCmd);
  }

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT8
Configure (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  //
  // all command blocks are of TxCB format
  //
  TxCB  *cmd_ptr;
  UINT8 *data_ptr;
  volatile INT16 Index;
  UINT8 my_filter;

  cmd_ptr   = GetFreeCB (AdapterInfo);
  ASSERT (cmd_ptr != NULL);
  data_ptr  = (UINT8 *) cmd_ptr + sizeof (struct CB_Header);

  //
  // start the config data right after the command header
  //
  for (Index = 0; Index < sizeof (basic_config_cmd); Index++) {
    data_ptr[Index] = basic_config_cmd[Index];
  }

  my_filter = (UINT8) ((AdapterInfo->Rx_Filter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) ? 1 : 0);
  my_filter = (UINT8) (my_filter | ((AdapterInfo->Rx_Filter & PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST) ? 0 : 2));

  data_ptr[15]  = (UINT8) (data_ptr[15] | my_filter);
  data_ptr[19]  = (UINT8) (AdapterInfo->Duplex ? 0xC0 : 0x80);
  data_ptr[21]  = (UINT8) ((AdapterInfo->Rx_Filter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) ? 0x0D : 0x05);

  //
  // check if we have to use the AUI port instead
  //
  if ((AdapterInfo->PhyRecord[0] & 0x8000) != 0) {
    data_ptr[15] |= 0x80;
    data_ptr[8] = 0;
  }

  BlockIt (AdapterInfo, TRUE);
  cmd_ptr->cb_header.command = CmdSuspend | CmdConfigure;

  IssueCB (AdapterInfo, cmd_ptr);
  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);

  BlockIt (AdapterInfo, FALSE);

  CommandWaitForCompletion (cmd_ptr, AdapterInfo);

  //
  // restore the cb values for tx
  //
  cmd_ptr->PhysTBDArrayAddres = cmd_ptr->PhysArrayAddr;
  cmd_ptr->ByteCount = cmd_ptr->Threshold = cmd_ptr->TBDCount = 0;
  //
  // fields beyond the immediatedata are assumed to be safe
  // add the CB to the free list again
  //
  SetFreeCB (AdapterInfo, cmd_ptr);
  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT8
E100bSetupIAAddr (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  //
  // all command blocks are of TxCB format
  //
  TxCB    *cmd_ptr;
  UINT16  *data_ptr;
  UINT16  *eaddrs;

  eaddrs    = (UINT16 *) AdapterInfo->CurrentNodeAddress;

  cmd_ptr   = GetFreeCB (AdapterInfo);
  ASSERT (cmd_ptr != NULL);
  data_ptr  = (UINT16 *) ((UINT8 *) cmd_ptr +sizeof (struct CB_Header));

  //
  // AVOID a bug (?!) here by marking the command already completed.
  //
  cmd_ptr->cb_header.command  = (CmdSuspend | CmdIASetup);
  cmd_ptr->cb_header.status   = 0;
  data_ptr[0]                 = eaddrs[0];
  data_ptr[1]                 = eaddrs[1];
  data_ptr[2]                 = eaddrs[2];

  BlockIt (AdapterInfo, TRUE);
  IssueCB (AdapterInfo, cmd_ptr);
  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);
  BlockIt (AdapterInfo, FALSE);

  CommandWaitForCompletion (cmd_ptr, AdapterInfo);

  //
  // restore the cb values for tx
  //
  cmd_ptr->PhysTBDArrayAddres = cmd_ptr->PhysArrayAddr;
  cmd_ptr->ByteCount = cmd_ptr->Threshold = cmd_ptr->TBDCount = 0;
  //
  // fields beyond the immediatedata are assumed to be safe
  // add the CB to the free list again
  //
  SetFreeCB (AdapterInfo, cmd_ptr);
  return 0;
}


/**
  Instructs the NIC to stop receiving packets.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..


**/
VOID
StopRU (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  if (AdapterInfo->Receive_Started) {

    //
    // Todo: verify that we must wait for previous command completion.
    //
    wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);

    //
    // Disable interrupts, and stop the chip's Rx process.
    //
    OutWord (AdapterInfo, INT_MASK, AdapterInfo->ioaddr + SCBCmd);
    OutWord (AdapterInfo, INT_MASK | RX_ABORT, AdapterInfo->ioaddr + SCBCmd);

    AdapterInfo->Receive_Started = FALSE;
  }

  return ;
}


/**
  Instructs the NIC to start receiving packets.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..

  @retval 0                               Successful
  @retval -1                              Already Started

**/
INT8
StartRU (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{

  if (AdapterInfo->Receive_Started) {
    //
    // already started
    //
    return -1;
  }

  AdapterInfo->cur_rx_ind = 0;
  AdapterInfo->Int_Status = 0;

  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);

  OutLong (AdapterInfo, (UINT32) AdapterInfo->rx_phy_addr, AdapterInfo->ioaddr + SCBPointer);
  OutByte (AdapterInfo, RX_START, AdapterInfo->ioaddr + SCBCmd);

  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);

  AdapterInfo->Receive_Started = TRUE;
  return 0;
}


/**
  Configures the chip.  This routine expects the NIC_DATA_INSTANCE structure to be filled in.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..

  @retval 0                               Successful
  @retval PXE_STATCODE_NOT_ENOUGH_MEMORY  Insufficient length of locked memory
  @retval other                           Failure initializing chip

**/
UINTN
E100bInit (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PCI_CONFIG_HEADER *CfgHdr;
  UINTN             stat;
  UINTN             rx_size;
  UINTN             tx_size;

  if (AdapterInfo->MemoryLength < MEMORY_NEEDED) {
    return PXE_STATCODE_NOT_ENOUGH_MEMORY;
  }

  stat = MapIt (
          AdapterInfo,
          AdapterInfo->MemoryPtr,
          AdapterInfo->MemoryLength,
          TO_AND_FROM_DEVICE,
          (UINT64)(UINTN) &AdapterInfo->Mapped_MemoryPtr
          );

  if (stat != 0) {
    return stat;
  }

  CfgHdr = (PCI_CONFIG_HEADER *) &(AdapterInfo->Config[0]);

  //
  // fill in the ioaddr, int... from the config space
  //
  AdapterInfo->int_num = CfgHdr->int_line;

  //
  // we don't need to validate integer number, what if they don't want to assign one?
  // if (AdapterInfo->int_num == 0 || AdapterInfo->int_num == 0xff)
  // return PXE_STATCODE_DEVICE_FAILURE;
  //
  AdapterInfo->ioaddr       = 0;
  AdapterInfo->VendorID     = CfgHdr->VendorID;
  AdapterInfo->DeviceID     = CfgHdr->DeviceID;
  AdapterInfo->RevID        = CfgHdr->RevID;
  AdapterInfo->SubVendorID  = CfgHdr->SubVendorID;
  AdapterInfo->SubSystemID  = CfgHdr->SubSystemID;
  AdapterInfo->flash_addr   = 0;

  //
  // Read the station address EEPROM before doing the reset.
  // Perhaps this should even be done before accepting the device,
  // then we wouldn't have a device name with which to report the error.
  //
  if (E100bReadEepromAndStationAddress (AdapterInfo) != 0) {
    return PXE_STATCODE_DEVICE_FAILURE;

  }
  //
  // ## calculate the buffer #s depending on memory given
  // ## calculate the rx and tx ring pointers
  //

  AdapterInfo->TxBufCnt       = TX_BUFFER_COUNT;
  AdapterInfo->RxBufCnt       = RX_BUFFER_COUNT;
  rx_size                     = (AdapterInfo->RxBufCnt * sizeof (RxFD));
  tx_size                     = (AdapterInfo->TxBufCnt * sizeof (TxCB));
  AdapterInfo->rx_ring        = (RxFD *) (UINTN) (AdapterInfo->MemoryPtr);
  AdapterInfo->tx_ring        = (TxCB *) (UINTN) (AdapterInfo->MemoryPtr + rx_size);
  AdapterInfo->statistics     = (struct speedo_stats *) (UINTN) (AdapterInfo->MemoryPtr + rx_size + tx_size);

  AdapterInfo->rx_phy_addr    = AdapterInfo->Mapped_MemoryPtr;
  AdapterInfo->tx_phy_addr    = AdapterInfo->Mapped_MemoryPtr + rx_size;
  AdapterInfo->stat_phy_addr  = AdapterInfo->tx_phy_addr + tx_size;

  //
  // auto detect.
  //
  AdapterInfo->PhyAddress     = 0xFF;
  AdapterInfo->Rx_Filter            = PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST;
  AdapterInfo->Receive_Started      = FALSE;
  AdapterInfo->mcast_list.list_len  = 0;
  return InitializeChip (AdapterInfo);
}


/**
  Sets the interrupt state for the NIC.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..

  @retval 0                               Successful

**/
UINT8
E100bSetInterruptState (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  //
  // don't set receive interrupt if receiver is disabled...
  //
  UINT16  cmd_word;

  if ((AdapterInfo->int_mask & PXE_OPFLAGS_INTERRUPT_RECEIVE) != 0) {
    cmd_word = InWord (AdapterInfo, AdapterInfo->ioaddr + SCBCmd);
    cmd_word &= ~INT_MASK;
    OutWord (AdapterInfo, cmd_word, AdapterInfo->ioaddr + SCBCmd);
  } else {
    //
    // disable ints, should not be given for SW Int.
    //
    OutWord (AdapterInfo, INT_MASK, AdapterInfo->ioaddr + SCBCmd);
  }

  if ((AdapterInfo->int_mask & PXE_OPFLAGS_INTERRUPT_SOFTWARE) != 0) {
    //
    // reset the bit in our mask, it is only one time!!
    //
    AdapterInfo->int_mask &= ~(PXE_OPFLAGS_INTERRUPT_SOFTWARE);
    cmd_word = InWord (AdapterInfo, AdapterInfo->ioaddr + SCBCmd);
    cmd_word |= DRVR_INT;
    OutWord (AdapterInfo, cmd_word, AdapterInfo->ioaddr + SCBCmd);
  }

  return 0;
}
//
// we are not going to disable broadcast for the WOL's sake!
//

/**
  Instructs the NIC to start receiving packets.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on.. new_filter
                                              - cpb                             -
                                          cpbsize                         -

  @retval 0                               Successful
  @retval -1                              Already Started

**/
UINTN
E100bSetfilter (
  NIC_DATA_INSTANCE *AdapterInfo,
  UINT16            new_filter,
  UINT64            cpb,
  UINT32            cpbsize
  )
{
  PXE_CPB_RECEIVE_FILTERS *mc_list = (PXE_CPB_RECEIVE_FILTERS *) (UINTN)cpb;
  UINT16                  cfg_flt;
  UINT16                  old_filter;
  UINT16                  Index;
  UINT16                  Index2;
  UINT16                  mc_count;
  TxCB                    *cmd_ptr;
  struct MC_CB_STRUCT     *data_ptr;
  UINT16                  mc_byte_cnt;

  old_filter  = AdapterInfo->Rx_Filter;

  //
  // only these bits need a change in the configuration
  // actually change in bcast requires configure but we ignore that change
  //
  cfg_flt = PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS |
            PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;

  if ((old_filter & cfg_flt) != (new_filter & cfg_flt)) {
    XmitWaitForCompletion (AdapterInfo);

    if (AdapterInfo->Receive_Started) {
      StopRU (AdapterInfo);
    }

    AdapterInfo->Rx_Filter = (UINT8) (new_filter | PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST);
    Configure (AdapterInfo);
  }

  //
  // check if mcast setting changed
  //
  if ( ((new_filter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) !=
       (old_filter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) ) ||
       (mc_list != NULL) ) {


    if (mc_list != NULL) {
      mc_count = AdapterInfo->mcast_list.list_len = (UINT16) (cpbsize / PXE_MAC_LENGTH);

      for (Index = 0; (Index < mc_count && Index < MAX_MCAST_ADDRESS_CNT); Index++) {
        for (Index2 = 0; Index2 < PXE_MAC_LENGTH; Index2++) {
          AdapterInfo->mcast_list.mc_list[Index][Index2] = mc_list->MCastList[Index][Index2];
        }
      }
    }

    //
    // are we setting the list or resetting??
    //
    if ((new_filter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) != 0) {
      //
      // we are setting a new list!
      //
      mc_count = AdapterInfo->mcast_list.list_len;
      //
      // count should be the actual # of bytes in the list
      // so multiply this with 6
      //
      mc_byte_cnt = (UINT16) ((mc_count << 2) + (mc_count << 1));
      AdapterInfo->Rx_Filter |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
    } else {
      //
      // disabling the list in the NIC.
      //
      mc_byte_cnt = mc_count = 0;
      AdapterInfo->Rx_Filter &= (~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST);
    }

    //
    // before issuing any new command!
    //
    XmitWaitForCompletion (AdapterInfo);

    if (AdapterInfo->Receive_Started) {
      StopRU (AdapterInfo);

    }

    cmd_ptr = GetFreeCB (AdapterInfo);
    if (cmd_ptr == NULL) {
      return PXE_STATCODE_QUEUE_FULL;
    }
    //
    // fill the command structure and issue
    //
    data_ptr = (struct MC_CB_STRUCT *) (&cmd_ptr->PhysTBDArrayAddres);
    //
    // first 2 bytes are the count;
    //
    data_ptr->count = mc_byte_cnt;
    for (Index = 0; Index < mc_count; Index++) {
      for (Index2 = 0; Index2 < PXE_HWADDR_LEN_ETHER; Index2++) {
        data_ptr->m_list[Index][Index2] = AdapterInfo->mcast_list.mc_list[Index][Index2];
      }
    }

    cmd_ptr->cb_header.command  = CmdSuspend | CmdMulticastList;
    cmd_ptr->cb_header.status   = 0;

    BlockIt (AdapterInfo, TRUE);
    IssueCB (AdapterInfo, cmd_ptr);
    wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);

    BlockIt (AdapterInfo, FALSE);

    CommandWaitForCompletion (cmd_ptr, AdapterInfo);

    cmd_ptr->PhysTBDArrayAddres = cmd_ptr->PhysArrayAddr;
    cmd_ptr->ByteCount = cmd_ptr->Threshold = cmd_ptr->TBDCount = 0;
    //
    // fields beyond the immediatedata are assumed to be safe
    // add the CB to the free list again
    //
    SetFreeCB (AdapterInfo, cmd_ptr);
  }

  if (new_filter != 0) {
    //
    // enable unicast and start the RU
    //
    AdapterInfo->Rx_Filter = (UINT8) (AdapterInfo->Rx_Filter | (new_filter | PXE_OPFLAGS_RECEIVE_FILTER_UNICAST));
    StartRU (AdapterInfo);
  } else {
    //
    // may be disabling everything!
    //
    if (AdapterInfo->Receive_Started) {
      StopRU (AdapterInfo);
    }

    AdapterInfo->Rx_Filter |= (~PXE_OPFLAGS_RECEIVE_FILTER_UNICAST);
  }

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  cpb                             TODO: add argument description
  @param  opflags                         TODO: add argument description

  @return TODO: add return values

**/
UINTN
E100bTransmit (
  NIC_DATA_INSTANCE *AdapterInfo,
  UINT64            cpb,
  UINT16            opflags
  )
{
  PXE_CPB_TRANSMIT_FRAGMENTS  *tx_ptr_f;
  PXE_CPB_TRANSMIT            *tx_ptr_1;
  TxCB                        *tcb_ptr;
  UINT64                      Tmp_ptr;
  UINTN                       stat;
  INT32                       Index;
  UINT16                      wait_sec;

  tx_ptr_1  = (PXE_CPB_TRANSMIT *) (UINTN) cpb;
  tx_ptr_f  = (PXE_CPB_TRANSMIT_FRAGMENTS *) (UINTN) cpb;
  Tmp_ptr = 0;

  //
  // stop reentrancy here
  //
  if (AdapterInfo->in_transmit) {
    return PXE_STATCODE_BUSY;

  }

  AdapterInfo->in_transmit = TRUE;

  //
  // Prevent interrupts from changing the Tx ring from underneath us.
  //
  // Calculate the Tx descriptor entry.
  //
  if ((tcb_ptr = GetFreeCB (AdapterInfo)) == NULL) {
    AdapterInfo->in_transmit = FALSE;
    return PXE_STATCODE_QUEUE_FULL;
  }

  AdapterInfo->TxTotals++;

  tcb_ptr->cb_header.command  = (CmdSuspend | CmdTx | CmdTxFlex);
  tcb_ptr->cb_header.status   = 0;

  //
  // no immediate data, set EOF in the ByteCount
  //
  tcb_ptr->ByteCount = 0x8000;

  //
  // The data region is always in one buffer descriptor, Tx FIFO
  // threshold of 256.
  // 82557 multiplies the threashold value by 8, so give 256/8
  //
  tcb_ptr->Threshold = 32;
  if ((opflags & PXE_OPFLAGS_TRANSMIT_FRAGMENTED) != 0) {

    if (tx_ptr_f->FragCnt > MAX_XMIT_FRAGMENTS) {
      SetFreeCB (AdapterInfo, tcb_ptr);
      AdapterInfo->in_transmit = FALSE;
      return PXE_STATCODE_INVALID_PARAMETER;
    }

    tcb_ptr->TBDCount = (UINT8) tx_ptr_f->FragCnt;

    for (Index = 0; Index < tx_ptr_f->FragCnt; Index++) {
      stat = MapIt (
              AdapterInfo,
              tx_ptr_f->FragDesc[Index].FragAddr,
              tx_ptr_f->FragDesc[Index].FragLen,
              TO_DEVICE,
              (UINT64)(UINTN) &Tmp_ptr
              );
      if (stat != 0) {
        SetFreeCB (AdapterInfo, tcb_ptr);
        AdapterInfo->in_transmit = FALSE;
        return PXE_STATCODE_INVALID_PARAMETER;
      }

      tcb_ptr->TBDArray[Index].phys_buf_addr  = (UINT32) Tmp_ptr;
      tcb_ptr->TBDArray[Index].buf_len        = tx_ptr_f->FragDesc[Index].FragLen;
    }

    tcb_ptr->free_data_ptr = tx_ptr_f->FragDesc[0].FragAddr;

  } else {
    //
    // non fragmented case
    //
    tcb_ptr->TBDCount = 1;
    stat = MapIt (
            AdapterInfo,
            tx_ptr_1->FrameAddr,
            tx_ptr_1->DataLen + tx_ptr_1->MediaheaderLen,
            TO_DEVICE,
            (UINT64)(UINTN) &Tmp_ptr
            );
    if (stat != 0) {
      SetFreeCB (AdapterInfo, tcb_ptr);
      AdapterInfo->in_transmit = FALSE;
      return PXE_STATCODE_INVALID_PARAMETER;
    }

    tcb_ptr->TBDArray[0].phys_buf_addr  = (UINT32) (Tmp_ptr);
    tcb_ptr->TBDArray[0].buf_len        = tx_ptr_1->DataLen + tx_ptr_1->MediaheaderLen;
    tcb_ptr->free_data_ptr              = tx_ptr_1->FrameAddr;
  }

  //
  // must wait for previous command completion only if it was a non-transmit
  //
  BlockIt (AdapterInfo, TRUE);
  IssueCB (AdapterInfo, tcb_ptr);
  BlockIt (AdapterInfo, FALSE);

  //
  // see if we need to wait for completion here
  //
  if ((opflags & PXE_OPFLAGS_TRANSMIT_BLOCK) != 0) {
    //
    // don't wait for more than 1 second!!!
    //
    wait_sec = 1000;
    while (tcb_ptr->cb_header.status == 0) {
      DelayIt (AdapterInfo, 10);
      wait_sec--;
      if (wait_sec == 0) {
        break;
      }
    }
    //
    // we need to un-map any mapped buffers here
    //
    if ((opflags & PXE_OPFLAGS_TRANSMIT_FRAGMENTED) != 0) {

      for (Index = 0; Index < tx_ptr_f->FragCnt; Index++) {
        Tmp_ptr = tcb_ptr->TBDArray[Index].phys_buf_addr;
        UnMapIt (
          AdapterInfo,
          tx_ptr_f->FragDesc[Index].FragAddr,
          tx_ptr_f->FragDesc[Index].FragLen,
          TO_DEVICE,
          (UINT64) Tmp_ptr
          );
      }
    } else {
      Tmp_ptr = tcb_ptr->TBDArray[0].phys_buf_addr;
      UnMapIt (
        AdapterInfo,
        tx_ptr_1->FrameAddr,
        tx_ptr_1->DataLen + tx_ptr_1->MediaheaderLen,
        TO_DEVICE,
        (UINT64) Tmp_ptr
        );
    }

    if (tcb_ptr->cb_header.status == 0) {
      SetFreeCB (AdapterInfo, tcb_ptr);
      AdapterInfo->in_transmit = FALSE;
      return PXE_STATCODE_DEVICE_FAILURE;
    }

    SetFreeCB (AdapterInfo, tcb_ptr);
  }
  //
  // CB will be set free later in get_status (or when we run out of xmit buffers
  //
  AdapterInfo->in_transmit = FALSE;

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  cpb                             TODO: add argument description
  @param  db                              TODO: add argument description

  @return TODO: add return values

**/
UINTN
E100bReceive (
  NIC_DATA_INSTANCE *AdapterInfo,
  UINT64            cpb,
  UINT64            db
  )
{
  PXE_CPB_RECEIVE *rx_cpbptr;
  PXE_DB_RECEIVE  *rx_dbptr;
  RxFD            *rx_ptr;
  INT32           status;
  INT32           Index;
  UINT16          pkt_len;
  UINT16          ret_code;
  PXE_FRAME_TYPE  pkt_type;
  UINT16          Tmp_len;
  EtherHeader     *hdr_ptr;
  ret_code  = PXE_STATCODE_NO_DATA;
  pkt_type  = PXE_FRAME_TYPE_NONE;
  status    = InWord (AdapterInfo, AdapterInfo->ioaddr + SCBStatus);
  AdapterInfo->Int_Status = (UINT16) (AdapterInfo->Int_Status | status);
  //
  // acknoledge the interrupts
  //
  OutWord (AdapterInfo, (UINT16) (status & 0xfc00), (UINT32) (AdapterInfo->ioaddr + SCBStatus));

  //
  // include the prev ints as well
  //
  status = AdapterInfo->Int_Status;
  rx_cpbptr = (PXE_CPB_RECEIVE *) (UINTN) cpb;
  rx_dbptr  = (PXE_DB_RECEIVE *) (UINTN) db;

  rx_ptr    = &AdapterInfo->rx_ring[AdapterInfo->cur_rx_ind];

  //
  // be in a loop just in case (we may drop a pkt)
  //
  while ((status = rx_ptr->cb_header.status) & RX_COMPLETE) {

    AdapterInfo->RxTotals++;
    //
    // If we own the next entry, it's a new packet. Send it up.
    //
    if (rx_ptr->forwarded) {
      goto FreeRFD;

    }

    //
    // discard bad frames
    //

    //
    // crc, align, dma overrun, too short, receive error (v22 no coll)
    //
    if ((status & 0x0D90) != 0) {
      goto FreeRFD;

    }

    //
    // make sure the status is OK
    //
    if ((status & 0x02000) == 0) {
      goto FreeRFD;
    }

    pkt_len = (UINT16) (rx_ptr->ActualCount & 0x3fff);

    if (pkt_len != 0) {

      Tmp_len = pkt_len;
      if (pkt_len > rx_cpbptr->BufferLen) {
        Tmp_len = (UINT16) rx_cpbptr->BufferLen;
      }

      CopyMem ((INT8 *) (UINTN) rx_cpbptr->BufferAddr, (INT8 *) &rx_ptr->RFDBuffer, Tmp_len);

      hdr_ptr = (EtherHeader *) &rx_ptr->RFDBuffer;
      //
      // fill the CDB and break the loop
      //

      //
      // includes header
      //
      rx_dbptr->FrameLen = pkt_len;
      rx_dbptr->MediaHeaderLen = PXE_MAC_HEADER_LEN_ETHER;

      for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
        if (hdr_ptr->dest_addr[Index] != AdapterInfo->CurrentNodeAddress[Index]) {
          break;
        }
      }

      if (Index >= PXE_HWADDR_LEN_ETHER) {
        pkt_type = PXE_FRAME_TYPE_UNICAST;
      } else {
        for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
          if (hdr_ptr->dest_addr[Index] != AdapterInfo->BroadcastNodeAddress[Index]) {
            break;
          }
        }

        if (Index >= PXE_HWADDR_LEN_ETHER) {
          pkt_type = PXE_FRAME_TYPE_BROADCAST;
        } else {
          if ((hdr_ptr->dest_addr[0] & 1) == 1) {
            //
            // mcast
            //

            pkt_type = PXE_FRAME_TYPE_FILTERED_MULTICAST;
          } else {
            pkt_type = PXE_FRAME_TYPE_PROMISCUOUS;
          }
        }
      }

      rx_dbptr->Type      = pkt_type;
      rx_dbptr->Protocol  = hdr_ptr->type;

      for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
        rx_dbptr->SrcAddr[Index]  = hdr_ptr->src_addr[Index];
        rx_dbptr->DestAddr[Index] = hdr_ptr->dest_addr[Index];
      }

      rx_ptr->forwarded = TRUE;
      //
      // success
      //
      ret_code          = 0;
      Recycle_RFD (AdapterInfo, AdapterInfo->cur_rx_ind);
      AdapterInfo->cur_rx_ind++;
      if (AdapterInfo->cur_rx_ind == AdapterInfo->RxBufCnt) {
        AdapterInfo->cur_rx_ind = 0;
      }
      break;
    }

FreeRFD:
    Recycle_RFD (AdapterInfo, AdapterInfo->cur_rx_ind);
    AdapterInfo->cur_rx_ind++;
    if (AdapterInfo->cur_rx_ind == AdapterInfo->RxBufCnt) {
      AdapterInfo->cur_rx_ind = 0;
    }

    rx_ptr = &AdapterInfo->rx_ring[AdapterInfo->cur_rx_ind];
  }

  if (pkt_type == PXE_FRAME_TYPE_NONE) {
    AdapterInfo->Int_Status &= (~SCB_STATUS_FR);
  }

  status = InWord (AdapterInfo, AdapterInfo->ioaddr + SCBStatus);
  if ((status & SCB_RUS_NO_RESOURCES) != 0) {
    //
    // start the receive unit here!
    // leave all the filled frames,
    //
    SetupReceiveQueues (AdapterInfo);
    OutLong (AdapterInfo, (UINT32) AdapterInfo->rx_phy_addr, AdapterInfo->ioaddr + SCBPointer);
    OutWord (AdapterInfo, RX_START, AdapterInfo->ioaddr + SCBCmd);
    AdapterInfo->cur_rx_ind = 0;
  }

  return ret_code;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
INT16
E100bReadEepromAndStationAddress (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  INT32   Index;
  INT32   Index2;
  UINT16  sum;
  UINT16  eeprom_len;
  UINT8   addr_len;
  UINT16  *eedata;

  eedata    = (UINT16 *) (&AdapterInfo->NVData[0]);

  sum       = 0;
  addr_len  = E100bGetEepromAddrLen (AdapterInfo);

  //
  // in words
  //
  AdapterInfo->NVData_Len = eeprom_len = (UINT16) (1 << addr_len);
  for (Index2 = 0, Index = 0; ((Index2 < PXE_MAC_LENGTH - 1) && (Index < eeprom_len)); Index++) {
    UINT16  value;
    value         = E100bReadEeprom (AdapterInfo, Index, addr_len);
    eedata[Index] = value;
    sum           = (UINT16) (sum + value);
    if (Index < 3) {
      AdapterInfo->PermNodeAddress[Index2++]  = (UINT8) value;
      AdapterInfo->PermNodeAddress[Index2++]  = (UINT8) (value >> 8);
    }
  }

  if (sum != 0xBABA) {
    return -1;
  }

  for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
    AdapterInfo->CurrentNodeAddress[Index] = AdapterInfo->PermNodeAddress[Index];
  }

  for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
    AdapterInfo->BroadcastNodeAddress[Index] = 0xff;
  }

  for (Index = PXE_HWADDR_LEN_ETHER; Index < PXE_MAC_LENGTH; Index++) {
    AdapterInfo->CurrentNodeAddress[Index]    = 0;
    AdapterInfo->PermNodeAddress[Index]       = 0;
    AdapterInfo->BroadcastNodeAddress[Index]  = 0;
  }

  return 0;
}

//
//  CBList is a circular linked list
//  1) When all are free, Tail->next == Head and FreeCount == # allocated
//  2) When none are free, Tail == Head and FreeCount == 0
//  3) when one is free, Tail == Head and Freecount == 1
//  4) First non-Free frame is always at Tail->next
//

/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT8
SetupCBlink (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  TxCB  *head_ptr;
  TxCB  *tail_ptr;
  TxCB  *cur_ptr;
  INT32 Index;
  UINTN array_off;

  cur_ptr   = &(AdapterInfo->tx_ring[0]);
  array_off = (UINTN) (&cur_ptr->TBDArray) - (UINTN) cur_ptr;
  for (Index = 0; Index < AdapterInfo->TxBufCnt; Index++) {
    cur_ptr[Index].cb_header.status   = 0;
    cur_ptr[Index].cb_header.command  = 0;

    cur_ptr[Index].PhysTCBAddress     =
    (UINT32) AdapterInfo->tx_phy_addr + (Index * sizeof (TxCB));

    cur_ptr[Index].PhysArrayAddr      = (UINT32)(cur_ptr[Index].PhysTCBAddress + array_off);
    cur_ptr[Index].PhysTBDArrayAddres = (UINT32)(cur_ptr[Index].PhysTCBAddress + array_off);

    cur_ptr->free_data_ptr = (UINT64) 0;

    if (Index < AdapterInfo->TxBufCnt - 1) {
      cur_ptr[Index].cb_header.link             = cur_ptr[Index].PhysTCBAddress + sizeof (TxCB);
      cur_ptr[Index].NextTCBVirtualLinkPtr      = &cur_ptr[Index + 1];
      cur_ptr[Index + 1].PrevTCBVirtualLinkPtr  = &cur_ptr[Index];
    }
  }

  head_ptr                        = &cur_ptr[0];
  tail_ptr                        = &cur_ptr[AdapterInfo->TxBufCnt - 1];
  tail_ptr->cb_header.link        = head_ptr->PhysTCBAddress;
  tail_ptr->NextTCBVirtualLinkPtr = head_ptr;
  head_ptr->PrevTCBVirtualLinkPtr = tail_ptr;

  AdapterInfo->FreeCBCount        = AdapterInfo->TxBufCnt;
  AdapterInfo->FreeTxHeadPtr      = head_ptr;
  //
  // set tail of the free list, next to this would be either in use
  // or the head itself
  //
  AdapterInfo->FreeTxTailPtr  = tail_ptr;

  AdapterInfo->xmit_done_head = AdapterInfo->xmit_done_tail = 0;

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
TxCB *
GetFreeCB (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  TxCB  *free_cb_ptr;

  //
  // claim any hanging free CBs
  //
  if (AdapterInfo->FreeCBCount <= 1) {
    CheckCBList (AdapterInfo);
  }

  //
  // don't use up the last CB problem if the previous CB that the CU used
  // becomes the last CB we submit because of the SUSPEND bit we set.
  // the CU thinks it was never cleared.
  //

  if (AdapterInfo->FreeCBCount <= 1) {
    return NULL;
  }

  BlockIt (AdapterInfo, TRUE);
  free_cb_ptr                 = AdapterInfo->FreeTxHeadPtr;
  AdapterInfo->FreeTxHeadPtr  = free_cb_ptr->NextTCBVirtualLinkPtr;
  --AdapterInfo->FreeCBCount;
  BlockIt (AdapterInfo, FALSE);
  return free_cb_ptr;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  cb_ptr                          TODO: add argument description

  @return TODO: add return values

**/
VOID
SetFreeCB (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN TxCB              *cb_ptr
  )
{
  //
  // here we assume cb are returned in the order they are taken out
  // and we link the newly freed cb at the tail of free cb list
  //
  cb_ptr->cb_header.status    = 0;
  cb_ptr->free_data_ptr       = (UINT64) 0;

  AdapterInfo->FreeTxTailPtr  = cb_ptr;
  ++AdapterInfo->FreeCBCount;
  return ;
}


/**
  TODO: Add function description

  @param  ind                             TODO: add argument description

  @return TODO: add return values

**/
UINT16
next (
  IN UINT16 ind
  )
{
  UINT16  Tmp;

  Tmp = (UINT16) (ind + 1);
  if (Tmp >= (TX_BUFFER_COUNT << 1)) {
    Tmp = 0;
  }

  return Tmp;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT16
CheckCBList (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  TxCB    *Tmp_ptr;
  UINT16  cnt;

  cnt = 0;
  while (1) {
    Tmp_ptr = AdapterInfo->FreeTxTailPtr->NextTCBVirtualLinkPtr;
    if ((Tmp_ptr->cb_header.status & CMD_STATUS_MASK) != 0) {
      //
      // check if Q is full
      //
      if (next (AdapterInfo->xmit_done_tail) != AdapterInfo->xmit_done_head) {
        ASSERT (AdapterInfo->xmit_done_tail < TX_BUFFER_COUNT << 1);
        AdapterInfo->xmit_done[AdapterInfo->xmit_done_tail] = Tmp_ptr->free_data_ptr;

        UnMapIt (
          AdapterInfo,
          Tmp_ptr->free_data_ptr,
          Tmp_ptr->TBDArray[0].buf_len,
          TO_DEVICE,
          (UINT64) Tmp_ptr->TBDArray[0].phys_buf_addr
          );

        AdapterInfo->xmit_done_tail = next (AdapterInfo->xmit_done_tail);
      }

      SetFreeCB (AdapterInfo, Tmp_ptr);
    } else {
      break;
    }
  }

  return cnt;
}
//
// Description : Initialize the RFD list list by linking each element together
//               in a circular list.  The simplified memory model is used.
//               All data is in the RFD.  The RFDs are linked together and the
//               last one points back to the first one.  When the current RFD
//               is processed (frame received), its EL bit is set and the EL
//               bit in the previous RXFD is cleared.
//               Allocation done during INIT, this is making linked list.
//

/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT8
SetupReceiveQueues (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  RxFD    *rx_ptr;
  RxFD    *tail_ptr;
  UINT16  Index;

  AdapterInfo->cur_rx_ind = 0;
  rx_ptr                  = (&AdapterInfo->rx_ring[0]);

  for (Index = 0; Index < AdapterInfo->RxBufCnt; Index++) {
    rx_ptr[Index].cb_header.status  = 0;
    rx_ptr[Index].cb_header.command = 0;
    rx_ptr[Index].RFDSize           = RX_BUFFER_SIZE;
    rx_ptr[Index].ActualCount       = 0;
    //
    // RBDs not used, simple memory model
    //
    rx_ptr[Index].rx_buf_addr       = (UINT32) (-1);

    //
    // RBDs not used, simple memory model
    //
    rx_ptr[Index].forwarded = FALSE;

    //
    // don't use Tmp_ptr if it is beyond the last one
    //
    if (Index < AdapterInfo->RxBufCnt - 1) {
      rx_ptr[Index].cb_header.link = (UINT32) AdapterInfo->rx_phy_addr + ((Index + 1) * sizeof (RxFD));
    }
  }

  tail_ptr                    = (&AdapterInfo->rx_ring[AdapterInfo->RxBufCnt - 1]);
  tail_ptr->cb_header.link    = (UINT32) AdapterInfo->rx_phy_addr;

  //
  // set the EL bit
  //
  tail_ptr->cb_header.command = 0xC000;
  AdapterInfo->RFDTailPtr = tail_ptr;
  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  rx_index                        TODO: add argument description

  @return TODO: add return values

**/
VOID
Recycle_RFD (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT16            rx_index
  )
{
  RxFD  *rx_ptr;
  RxFD  *tail_ptr;
  //
  // change the EL bit and change the AdapterInfo->RxTailPtr
  // rx_ptr is assumed to be the head of the Q
  // AdapterInfo->rx_forwarded[rx_index] = FALSE;
  //
  rx_ptr                    = &AdapterInfo->rx_ring[rx_index];
  tail_ptr                  = AdapterInfo->RFDTailPtr;
  //
  // set el_bit and suspend bit
  //
  rx_ptr->cb_header.command = 0xc000;
  rx_ptr->cb_header.status    = 0;
  rx_ptr->ActualCount         = 0;
  rx_ptr->forwarded           = FALSE;
  AdapterInfo->RFDTailPtr     = rx_ptr;
  //
  // resetting the el_bit.
  //
  tail_ptr->cb_header.command = 0;
  //
  // check the receive unit, fix if there is any problem
  //
  return ;
}
//
// Serial EEPROM section.
//
//  EEPROM_Ctrl bits.
//
#define EE_SHIFT_CLK  0x01  /* EEPROM shift clock. */
#define EE_CS         0x02  /* EEPROM chip select. */
#define EE_DI         0x04  /* EEPROM chip data in. */
#define EE_WRITE_0    0x01
#define EE_WRITE_1    0x05
#define EE_DO         0x08  /* EEPROM chip data out. */
#define EE_ENB        (0x4800 | EE_CS)

//
// Delay between EEPROM clock transitions.
// This will actually work with no delay on 33Mhz PCI.
//
#define eeprom_delay(nanosec) DelayIt (AdapterInfo, nanosec);

//
// The EEPROM commands include the alway-set leading bit.
//
#define EE_WRITE_CMD  5 // 101b
#define EE_READ_CMD   6 // 110b
#define EE_ERASE_CMD  (7 << 6)

VOID
shift_bits_out (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT16            val,
  IN UINT8             num_bits
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  AdapterInfo - TODO: add argument description
  val         - TODO: add argument description
  num_bits    - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  INT32   Index;
  UINT8   Tmp;
  UINT32  EEAddr;

  EEAddr = AdapterInfo->ioaddr + SCBeeprom;

  for (Index = num_bits; Index >= 0; Index--) {
    INT16 dataval;

    //
    // will be 0 or 4
    //
    dataval = (INT16) ((val & (1 << Index)) ? EE_DI : 0);

    //
    // mask off the data_in bit
    //
    Tmp = (UINT8) (InByte (AdapterInfo, EEAddr) &~EE_DI);
    Tmp = (UINT8) (Tmp | dataval);
    OutByte (AdapterInfo, Tmp, EEAddr);
    eeprom_delay (100);
    //
    // raise the eeprom clock
    //
    OutByte (AdapterInfo, (UINT8) (Tmp | EE_SHIFT_CLK), EEAddr);
    eeprom_delay (150);
    //
    // lower the eeprom clock
    //
    OutByte (AdapterInfo, (UINT8) (Tmp &~EE_SHIFT_CLK), EEAddr);
    eeprom_delay (150);
  }
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT16
shift_bits_in (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT8   Tmp;
  INT32   Index;
  UINT16  retval;
  UINT32  EEAddr;

  EEAddr  = AdapterInfo->ioaddr + SCBeeprom;

  retval  = 0;
  for (Index = 15; Index >= 0; Index--) {
    //
    // raise the clock
    //

    //
    // mask off the data_in bit
    //
    Tmp = InByte (AdapterInfo, EEAddr);
    OutByte (AdapterInfo, (UINT8) (Tmp | EE_SHIFT_CLK), EEAddr);
    eeprom_delay (100);
    Tmp     = InByte (AdapterInfo, EEAddr);
    retval  = (UINT16) ((retval << 1) | ((Tmp & EE_DO) ? 1 : 0));
    //
    // lower the clock
    //
    OutByte (AdapterInfo, (UINT8) (Tmp &~EE_SHIFT_CLK), EEAddr);
    eeprom_delay (100);
  }

  return retval;
}


/**
  This routine sets the EEPROM lockout bit to gain exclusive access to the
  eeprom. the access bit is the most significant bit in the General Control
  Register 2 in the SCB space.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..

  @retval TRUE                            if it got the access
  @retval FALSE                           if it fails to get the exclusive access

**/
BOOLEAN
E100bSetEepromLockOut (
  IN NIC_DATA_INSTANCE  *AdapterInfo
  )
{
  UINTN wait;
  UINT8 tmp;

  if ((AdapterInfo->DeviceID == D102_DEVICE_ID) ||
      (AdapterInfo->RevID >= D102_REVID)) {

    wait = 500;

    while (wait--) {

      tmp = InByte (AdapterInfo, AdapterInfo->ioaddr + SCBGenCtrl2);
      tmp |= GCR2_EEPROM_ACCESS_SEMAPHORE;
      OutByte (AdapterInfo, tmp, AdapterInfo->ioaddr + SCBGenCtrl2);

      DelayIt (AdapterInfo, 50);
      tmp = InByte (AdapterInfo, AdapterInfo->ioaddr + SCBGenCtrl2);

      if (tmp & GCR2_EEPROM_ACCESS_SEMAPHORE) {
        return TRUE;
      }
    }

    return FALSE;
  }

  return TRUE;
}


/**
  This routine Resets the EEPROM lockout bit to giveup access to the
  eeprom. the access bit is the most significant bit in the General Control
  Register 2 in the SCB space.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..

  @return None

**/
VOID
E100bReSetEepromLockOut (
  IN NIC_DATA_INSTANCE  *AdapterInfo
  )
{
  UINT8 tmp;

  if ((AdapterInfo->DeviceID == D102_DEVICE_ID) ||
      (AdapterInfo->RevID >= D102_REVID)) {

    tmp = InByte (AdapterInfo, AdapterInfo->ioaddr + SCBGenCtrl2);
    tmp &= ~(GCR2_EEPROM_ACCESS_SEMAPHORE);
    OutByte (AdapterInfo, tmp, AdapterInfo->ioaddr + SCBGenCtrl2);

    DelayIt (AdapterInfo, 50);
  }
}


/**
  Using the NIC data structure information, read the EEPROM to get a Word of data for the MAC address.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..
  @param  Location                        Word offset into the MAC address to read.
  @param  AddrLen                         Number of bits of address length.

  @retval RetVal                          The word read from the EEPROM.

**/
UINT16
E100bReadEeprom (
  IN NIC_DATA_INSTANCE  *AdapterInfo,
  IN INT32              Location,
  IN UINT8              AddrLen
  )
{
  UINT16  RetVal;
  UINT8   Tmp;

  UINT32  EEAddr;
  UINT16  ReadCmd;

  EEAddr  = AdapterInfo->ioaddr + SCBeeprom;
  ReadCmd = (UINT16) (Location | (EE_READ_CMD << AddrLen));

  RetVal  = 0;

  //
  // get exclusive access to the eeprom first!
  //
  E100bSetEepromLockOut (AdapterInfo);

  //
  // eeprom control reg bits: x,x,x,x,DO,DI,CS,SK
  // to write the opcode+data value out one bit at a time in DI starting at msb
  // and then out a 1 to sk, wait, out 0 to SK and wait
  // repeat this for all the bits to be written
  //

  //
  // 11110010b
  //
  Tmp = (UINT8) (InByte (AdapterInfo, EEAddr) & 0xF2);
  OutByte (AdapterInfo, (UINT8) (Tmp | EE_CS), EEAddr);

  //
  // 3 for the read opcode 110b
  //
  shift_bits_out (AdapterInfo, ReadCmd, (UINT8) (3 + AddrLen));

  //
  // read the eeprom word one bit at a time
  //
  RetVal = shift_bits_in (AdapterInfo);

  //
  // Terminate the EEPROM access and leave eeprom in a clean state.
  //
  Tmp = InByte (AdapterInfo, EEAddr);
  Tmp &= ~(EE_CS | EE_DI);
  OutByte (AdapterInfo, Tmp, EEAddr);

  //
  // raise the clock and lower the eeprom shift clock
  //
  OutByte (AdapterInfo, (UINT8) (Tmp | EE_SHIFT_CLK), EEAddr);
  eeprom_delay (100);

  OutByte (AdapterInfo, (UINT8) (Tmp &~EE_SHIFT_CLK), EEAddr);
  eeprom_delay (100);

  //
  // giveup access to the eeprom
  //
  E100bReSetEepromLockOut (AdapterInfo);

  return RetVal;
}


/**
  Using the NIC data structure information, read the EEPROM to determine how many bits of address length
  this EEPROM is in Words.

  @param  AdapterInfo                     Pointer to the NIC data structure
                                          information which the UNDI driver is
                                          layering on..

  @retval RetVal                          The word read from the EEPROM.

**/
UINT8
E100bGetEepromAddrLen (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT8   Tmp;
  UINT8   AddrLen;
  UINT32  EEAddr;
  //
  // assume 64word eeprom (so,6 bits of address_length)
  //
  UINT16  ReadCmd;

  EEAddr  = AdapterInfo->ioaddr + SCBeeprom;
  ReadCmd = (EE_READ_CMD << 6);

  //
  // get exclusive access to the eeprom first!
  //
  E100bSetEepromLockOut (AdapterInfo);

  //
  // address we are trying to read is 0
  // eeprom control reg bits: x,x,x,x,DO,,DI,,CS,SK
  // to write the opcode+data value out one bit at a time in DI starting at msb
  // and then out a 1 to sk, wait, out 0 to SK and wait
  // repeat this for all the bits to be written
  //
  Tmp = (UINT8) (InByte (AdapterInfo, EEAddr) & 0xF2);

  //
  // enable eeprom access
  //
  OutByte (AdapterInfo, (UINT8) (Tmp | EE_CS), EEAddr);

  //
  // 3 for opcode, 6 for the default address len
  //
  shift_bits_out (AdapterInfo, ReadCmd, (UINT8) (3 + 6));

  //
  // (in case of a 64 word eeprom).
  // read the "dummy zero" from EE_DO to say that the address we wrote
  // (six 0s) is accepted, write more zeros (until 8) to get a "dummy zero"
  //

  //
  // assume the smallest
  //
  AddrLen = 6;
  Tmp     = InByte (AdapterInfo, EEAddr);
  while ((AddrLen < 8) && ((Tmp & EE_DO) != 0)) {
    OutByte (AdapterInfo, (UINT8) (Tmp &~EE_DI), EEAddr);
    eeprom_delay (100);

    //
    // raise the eeprom clock
    //
    OutByte (AdapterInfo, (UINT8) (Tmp | EE_SHIFT_CLK), EEAddr);
    eeprom_delay (150);

    //
    // lower the eeprom clock
    //
    OutByte (AdapterInfo, (UINT8) (Tmp &~EE_SHIFT_CLK), EEAddr);
    eeprom_delay (150);
    Tmp = InByte (AdapterInfo, EEAddr);
    AddrLen++;
  }

  //
  // read the eeprom word, even though we don't need this
  //
  shift_bits_in (AdapterInfo);

  //
  // Terminate the EEPROM access.
  //
  Tmp = InByte (AdapterInfo, EEAddr);
  Tmp &= ~(EE_CS | EE_DI);
  OutByte (AdapterInfo, Tmp, EEAddr);

  //
  // raise the clock and lower the eeprom shift clock
  //
  OutByte (AdapterInfo, (UINT8) (Tmp | EE_SHIFT_CLK), EEAddr);
  eeprom_delay (100);

  OutByte (AdapterInfo, (UINT8) (Tmp &~EE_SHIFT_CLK), EEAddr);
  eeprom_delay (100);

  //
  // giveup access to the eeprom!
  //
  E100bReSetEepromLockOut (AdapterInfo);

  return AddrLen;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  DBaddr                          TODO: add argument description
  @param  DBsize                          TODO: add argument description

  @return TODO: add return values

**/
UINTN
E100bStatistics (
  NIC_DATA_INSTANCE *AdapterInfo,
  UINT64            DBaddr,
  UINT16            DBsize
  )
{
  PXE_DB_STATISTICS db;
  //
  // wait upto one second (each wait is 100 micro s)
  //
  UINT32            Wait;
  Wait = 10000;
  wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);

  //
  // Clear statistics done marker.
  //
  AdapterInfo->statistics->done_marker = 0;

  //
  // Issue statistics dump (or dump w/ reset) command.
  //
  OutByte (
    AdapterInfo,
    (UINT8) (DBsize ? CU_SHOWSTATS : CU_DUMPSTATS),
    (UINT32) (AdapterInfo->ioaddr + SCBCmd)
    );

  //
  // Wait for command to complete.
  //
  // zero the db here just to chew up a little more time.
  //

  ZeroMem ((VOID *) &db, sizeof db);

  while (Wait != 0) {
    //
    // Wait a bit before checking.
    //

    DelayIt (AdapterInfo, 100);

    //
    // Look for done marker at end of statistics.
    //

    switch (AdapterInfo->statistics->done_marker) {
    case 0xA005:
    case 0xA007:
      break;

    default:
      Wait--;
      continue;
    }

    //
    // if we did not "continue" from the above switch, we are done,
    //
    break;
  }

  //
  // If this is a reset, we are out of here!
  //
  if (DBsize == 0) {
    return PXE_STATCODE_SUCCESS;
  }

  //
  // Convert NIC statistics counter format to EFI/UNDI
  // specification statistics counter format.
  //

  //
  //                54 3210 fedc ba98 7654 3210
  // db.Supported = 01 0000 0100 1101 0001 0111;
  //
  db.Supported = 0x104D17;

  //
  // Statistics from the NIC
  //

  db.Data[0x01] = AdapterInfo->statistics->rx_good_frames;

  db.Data[0x02] = AdapterInfo->statistics->rx_runt_errs;

  db.Data[0x08] = AdapterInfo->statistics->rx_crc_errs +
                  AdapterInfo->statistics->rx_align_errs;

  db.Data[0x04] = db.Data[0x02] +
                  db.Data[0x08] +
                  AdapterInfo->statistics->rx_resource_errs +
                  AdapterInfo->statistics->rx_overrun_errs;

  db.Data[0x00] = db.Data[0x01] + db.Data[0x04];

  db.Data[0x0B] = AdapterInfo->statistics->tx_good_frames;

  db.Data[0x0E] = AdapterInfo->statistics->tx_coll16_errs +
    AdapterInfo->statistics->tx_late_colls +
    AdapterInfo->statistics->tx_underruns +
    AdapterInfo->statistics->tx_one_colls +
    AdapterInfo->statistics->tx_multi_colls;

  db.Data[0x14] = AdapterInfo->statistics->tx_total_colls;

  db.Data[0x0A] = db.Data[0x0B] +
                  db.Data[0x0E] +
                  AdapterInfo->statistics->tx_lost_carrier;

  if (DBsize > sizeof db) {
    DBsize = (UINT16) sizeof (db);
  }

  CopyMem ((VOID *) (UINTN) DBaddr, (VOID *) &db, (UINTN) DBsize);

  return PXE_STATCODE_SUCCESS;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description
  @param  OpFlags                         TODO: add argument description

  @return TODO: add return values

**/
UINTN
E100bReset (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN INT32             OpFlags
  )
{

  UINT16  save_filter;
  //
  // disable the interrupts
  //
  OutWord (AdapterInfo, INT_MASK, AdapterInfo->ioaddr + SCBCmd);

  //
  // wait for the tx queue to complete
  //
  CheckCBList (AdapterInfo);

  XmitWaitForCompletion (AdapterInfo);

  if (AdapterInfo->Receive_Started) {
    StopRU (AdapterInfo);
  }

  InitializeChip (AdapterInfo);

  //
  // check the opflags and restart receive filters
  //
  if ((OpFlags & PXE_OPFLAGS_RESET_DISABLE_FILTERS) == 0) {

    save_filter = AdapterInfo->Rx_Filter;
    //
    // if we give the filter same as Rx_Filter,
    // this routine will not set mcast list (it thinks there is no change)
    // to force it, we will reset that flag in the Rx_Filter
    //
    AdapterInfo->Rx_Filter &= (~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST);
    E100bSetfilter (AdapterInfo, save_filter, (UINT64) 0, (UINT32) 0);
  }

  if ((OpFlags & PXE_OPFLAGS_RESET_DISABLE_INTERRUPTS) != 0) {
    //
    // disable the interrupts
    //
    AdapterInfo->int_mask = 0;
  }
  //
  // else leave the interrupt in the pre-set state!!!
  //
  E100bSetInterruptState (AdapterInfo);

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINTN
E100bShutdown (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  //
  // disable the interrupts
  //
  OutWord (AdapterInfo, INT_MASK, AdapterInfo->ioaddr + SCBCmd);

  //
  // stop the receive unit
  //
  if (AdapterInfo->Receive_Started) {
    StopRU (AdapterInfo);
  }

  //
  // wait for the tx queue to complete
  //
  CheckCBList (AdapterInfo);
  if (AdapterInfo->FreeCBCount != AdapterInfo->TxBufCnt) {
    wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);
  }

  //
  // we do not want to reset the phy, it takes a long time to renegotiate the
  // link after that (3-4 seconds)
  //
  InitializeChip (AdapterInfo);
  SelectiveReset (AdapterInfo);
  return 0;
}


/**
  This routine will write a value to the specified MII register
  of an external MDI compliant device (e.g. PHY 100).  The command will
  execute in polled mode.

  @param  AdapterInfo                     pointer to the structure that contains
                                          the NIC's context.
  @param  RegAddress                      The MII register that we are writing to
  @param  PhyAddress                      The MDI address of the Phy component.
  @param  DataValue                       The value that we are writing to the MII
                                          register.

  @return nothing

**/
VOID
MdiWrite (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT8             RegAddress,
  IN UINT8             PhyAddress,
  IN UINT16            DataValue
  )
{
  UINT32  WriteCommand;

  WriteCommand = ((UINT32) DataValue) |
                 ((UINT32)(RegAddress << 16)) |
                 ((UINT32)(PhyAddress << 21)) |
                 ((UINT32)(MDI_WRITE << 26));

  //
  // Issue the write command to the MDI control register.
  //
  OutLong (AdapterInfo, WriteCommand, AdapterInfo->ioaddr + SCBCtrlMDI);

  //
  // wait 20usec before checking status
  //
  DelayIt (AdapterInfo, 20);

  //
  // poll for the mdi write to complete
  while ((InLong (AdapterInfo, AdapterInfo->ioaddr + SCBCtrlMDI) &
                    MDI_PHY_READY) == 0){
    DelayIt (AdapterInfo, 20);
  }
}


/**
  This routine will read a value from the specified MII register
  of an external MDI compliant device (e.g. PHY 100), and return
  it to the calling routine.  The command will execute in polled mode.

  @param  AdapterInfo                     pointer to the structure that contains
                                          the NIC's context.
  @param  RegAddress                      The MII register that we are reading from
  @param  PhyAddress                      The MDI address of the Phy component.
  @param  DataValue                       pointer to the value that we read from
                                          the MII register.


**/
VOID
MdiRead (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT8             RegAddress,
  IN UINT8             PhyAddress,
  IN OUT UINT16        *DataValue
  )
{
  UINT32  ReadCommand;

  ReadCommand = ((UINT32) (RegAddress << 16)) |
                ((UINT32) (PhyAddress << 21)) |
                ((UINT32) (MDI_READ << 26));

  //
  // Issue the read command to the MDI control register.
  //
  OutLong (AdapterInfo, ReadCommand, AdapterInfo->ioaddr + SCBCtrlMDI);

  //
  // wait 20usec before checking status
  //
  DelayIt (AdapterInfo, 20);

  //
  // poll for the mdi read to complete
  //
  while ((InLong (AdapterInfo, AdapterInfo->ioaddr + SCBCtrlMDI) &
          MDI_PHY_READY) == 0) {
    DelayIt (AdapterInfo, 20);

  }

  *DataValue = InWord (AdapterInfo, AdapterInfo->ioaddr + SCBCtrlMDI);
}


/**
  This routine will reset the PHY that the adapter is currently
  configured to use.

  @param  AdapterInfo                     pointer to the structure that contains
                                          the NIC's context.


**/
VOID
PhyReset (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT16  MdiControlReg;

  MdiControlReg = (MDI_CR_AUTO_SELECT |
                  MDI_CR_RESTART_AUTO_NEG |
                  MDI_CR_RESET);

  //
  // Write the MDI control register with our new Phy configuration
  //
  MdiWrite (
    AdapterInfo,
    MDI_CONTROL_REG,
    AdapterInfo->PhyAddress,
    MdiControlReg
    );

  return ;
}


/**
  This routine will detect what phy we are using, set the line
  speed, FDX or HDX, and configure the phy if necessary.
  The following combinations are supported:
  - TX or T4 PHY alone at PHY address 1
  - T4 or TX PHY at address 1 and MII PHY at address 0
  - 82503 alone (10Base-T mode, no full duplex support)
  - 82503 and MII PHY (TX or T4) at address 0
  The sequence / priority of detection is as follows:
  - PHY 1 with cable termination
  - PHY 0 with cable termination
  - PHY 1 (if found) without cable termination
  - 503 interface
  Additionally auto-negotiation capable (NWAY) and parallel
  detection PHYs are supported. The flow-chart is described in
  the 82557 software writer's manual.
  NOTE:  1.  All PHY MDI registers are read in polled mode.
  2.  The routines assume that the 82557 has been RESET and we have
  obtained the virtual memory address of the CSR.
  3.  PhyDetect will not RESET the PHY.
  4.  If FORCEFDX is set, SPEED should also be set. The driver will
  check the values for inconsistency with the detected PHY
  technology.
  5.  PHY 1 (the PHY on the adapter) may have an address in the range
  1 through 31 inclusive. The driver will accept addresses in
  this range.
  6.  Driver ignores FORCEFDX and SPEED overrides if a 503 interface
  is detected.

  @param  AdapterInfo                     pointer to the structure that contains
                                          the NIC's context.

  @retval TRUE                            If a Phy was detected, and configured
                                          correctly.
  @retval FALSE                           If a valid phy could not be detected and
                                          configured.

**/
BOOLEAN
PhyDetect (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT16  *eedata;
  UINT16  MdiControlReg;
  UINT16  MdiStatusReg;
  BOOLEAN FoundPhy1;
  UINT8   ReNegotiateTime;

  eedata          = (UINT16 *) (&AdapterInfo->NVData[0]);

  FoundPhy1       = FALSE;
  ReNegotiateTime = 35;
  //
  // EEPROM word [6] contains the Primary PHY record in which the least 3 bits
  // indicate the PHY address
  // and word [7] contains the secondary PHY record
  //
  AdapterInfo->PhyRecord[0] = eedata[6];
  AdapterInfo->PhyRecord[1] = eedata[7];
  AdapterInfo->PhyAddress   = (UINT8) (AdapterInfo->PhyRecord[0] & 7);

  //
  // Check for a phy address over-ride of 32 which indicates force use of 82503
  // not detecting the link in this case
  //
  if (AdapterInfo->PhyAddress == 32) {
    //
    // 503 interface over-ride
    // Record the current speed and duplex.  We will be in half duplex
    // mode unless the user used the force full duplex over-ride.
    //
    AdapterInfo->LinkSpeed = 10;
    return (TRUE);
  }

  //
  // If the Phy Address is between 1-31 then we must first look for phy 1,
  // at that address.
  //
  if ((AdapterInfo->PhyAddress > 0) && (AdapterInfo->PhyAddress < 32)) {

    //
    // Read the MDI control and status registers at phy 1
    // and check if we found a valid phy
    //
    MdiRead (
      AdapterInfo,
      MDI_CONTROL_REG,
      AdapterInfo->PhyAddress,
      &MdiControlReg
      );

    MdiRead (
      AdapterInfo,
      MDI_STATUS_REG,
      AdapterInfo->PhyAddress,
      &MdiStatusReg
      );

    if (!((MdiControlReg == 0xffff) ||
          ((MdiStatusReg == 0) && (MdiControlReg == 0)))) {

      //
      // we have a valid phy1
      // Read the status register again because of sticky bits
      //
      FoundPhy1 = TRUE;
      MdiRead (
        AdapterInfo,
        MDI_STATUS_REG,
        AdapterInfo->PhyAddress,
        &MdiStatusReg
        );

      //
      // If there is a valid link then use this Phy.
      //
      if (MdiStatusReg & MDI_SR_LINK_STATUS) {
        return (SetupPhy(AdapterInfo));
      }
    }
  }

  //
  // Next try to detect a PHY at address 0x00 because there was no Phy 1,
  // or Phy 1 didn't have link, or we had a phy 0 over-ride
  //

  //
  // Read the MDI control and status registers at phy 0
  //
  MdiRead (AdapterInfo, MDI_CONTROL_REG, 0, &MdiControlReg);
  MdiRead (AdapterInfo, MDI_STATUS_REG, 0, &MdiStatusReg);

  //
  // check if we found a valid phy 0
  //
  if (((MdiControlReg == 0xffff) ||
       ((MdiStatusReg == 0) && (MdiControlReg == 0)))) {

    //
    // we don't have a valid phy at address 0
    // if phy address was forced to 0, then error out because we
    // didn't find a phy at that address
    //
    if (AdapterInfo->PhyAddress == 0x0000) {
      return (FALSE);
    } else {
      //
      // at this point phy1 does not have link and there is no phy 0 at all
      // if we are forced to detect the cable, error out here!
      //
      if (AdapterInfo->CableDetect != 0) {
        return FALSE;

      }

      if (FoundPhy1) {
        //
        // no phy 0, but there is a phy 1 (no link I guess), so use phy 1
        //
        return SetupPhy (AdapterInfo);
      } else {
        //
        // didn't find phy 0 or phy 1, so assume a 503 interface
        //
        AdapterInfo->PhyAddress = 32;

        //
        // Record the current speed and duplex.  We'll be in half duplex
        // mode unless the user used the force full duplex over-ride.
        //
        AdapterInfo->LinkSpeed = 10;
        return (TRUE);
      }
    }
  } else {
    //
    // We have a valid phy at address 0.  If phy 0 has a link then we use
    // phy 0.  If Phy 0 doesn't have a link then we use Phy 1 (no link)
    // if phy 1 is present, or phy 0 if phy 1 is not present
    // If phy 1 was present, then we must isolate phy 1 before we enable
    // phy 0 to see if Phy 0 has a link.
    //
    if (FoundPhy1) {
      //
      // isolate phy 1
      //
      MdiWrite (
        AdapterInfo,
        MDI_CONTROL_REG,
        AdapterInfo->PhyAddress,
        MDI_CR_ISOLATE
        );

      //
      // wait 100 microseconds for the phy to isolate.
      //
      DelayIt (AdapterInfo, 100);
    }

    //
    // Since this Phy is at address 0, we must enable it.  So clear
    // the isolate bit, and set the auto-speed select bit
    //
    MdiWrite (
      AdapterInfo,
      MDI_CONTROL_REG,
      0,
      MDI_CR_AUTO_SELECT
      );

    //
    // wait 100 microseconds for the phy to be enabled.
    //
    DelayIt (AdapterInfo, 100);

    //
    // restart the auto-negotion process
    //
    MdiWrite (
      AdapterInfo,
      MDI_CONTROL_REG,
      0,
      MDI_CR_RESTART_AUTO_NEG | MDI_CR_AUTO_SELECT
      );

    //
    // wait no more than 3.5 seconds for auto-negotiation to complete
    //
    while (ReNegotiateTime) {
      //
      // Read the status register twice because of sticky bits
      //
      MdiRead (AdapterInfo, MDI_STATUS_REG, 0, &MdiStatusReg);
      MdiRead (AdapterInfo, MDI_STATUS_REG, 0, &MdiStatusReg);

      if (MdiStatusReg & MDI_SR_AUTO_NEG_COMPLETE) {
        break;
      }

      DelayIt (AdapterInfo, 100);
      ReNegotiateTime--;
    }

    //
    // Read the status register again because of sticky bits
    //
    MdiRead (AdapterInfo, MDI_STATUS_REG, 0, &MdiStatusReg);

    //
    // If the link was not set
    //
    if ((MdiStatusReg & MDI_SR_LINK_STATUS) == 0) {
      //
      // PHY1 does not have a link and phy 0 does not have a link
      // do not proceed if we need to detect the link!
      //
      if (AdapterInfo->CableDetect != 0) {
        return FALSE;
      }

      //
      // the link wasn't set, so use phy 1 if phy 1 was present
      //
      if (FoundPhy1) {
        //
        // isolate phy 0
        //
        MdiWrite (AdapterInfo, MDI_CONTROL_REG, 0, MDI_CR_ISOLATE);

        //
        // wait 100 microseconds for the phy to isolate.
        //
        DelayIt (AdapterInfo, 100);

        //
        // Now re-enable PHY 1
        //
        MdiWrite (
          AdapterInfo,
          MDI_CONTROL_REG,
          AdapterInfo->PhyAddress,
          MDI_CR_AUTO_SELECT
          );

        //
        // wait 100 microseconds for the phy to be enabled
        //
        DelayIt (AdapterInfo, 100);

        //
        // restart the auto-negotion process
        //
        MdiWrite (
          AdapterInfo,
          MDI_CONTROL_REG,
          AdapterInfo->PhyAddress,
          MDI_CR_RESTART_AUTO_NEG | MDI_CR_AUTO_SELECT
          );

        //
        // Don't wait for it to complete (we didn't have link earlier)
        //
        return (SetupPhy (AdapterInfo));
      }
    }

    //
    // Definitely using Phy 0
    //
    AdapterInfo->PhyAddress = 0;
    return (SetupPhy(AdapterInfo));
  }
}


/**
  This routine will setup phy 1 or phy 0 so that it is configured
  to match a speed and duplex over-ride option.  If speed or
  duplex mode is not explicitly specified in the registry, the
  driver will skip the speed and duplex over-ride code, and
  assume the adapter is automatically setting the line speed, and
  the duplex mode.  At the end of this routine, any truly Phy
  specific code will be executed (each Phy has its own quirks,
  and some require that certain special bits are set).
  NOTE:  The driver assumes that SPEED and FORCEFDX are specified at the
  same time. If FORCEDPX is set without speed being set, the driver
  will encouter a fatal error and log a message into the event viewer.

  @param  AdapterInfo                     pointer to the structure that contains
                                          the NIC's context.

  @retval TRUE                            If the phy could be configured correctly
  @retval FALSE                           If the phy couldn't be configured
                                          correctly, because an unsupported
                                          over-ride option was used

**/
BOOLEAN
SetupPhy (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT16  MdiControlReg;
  UINT16  MdiStatusReg;
  UINT16  MdiIdLowReg;
  UINT16  MdiIdHighReg;
  UINT16  MdiMiscReg;
  UINT32  PhyId;
  BOOLEAN ForcePhySetting;

  ForcePhySetting = FALSE;

  //
  // If we are NOT forcing a setting for line speed or full duplex, then
  // we won't force a link setting, and we'll jump down to the phy
  // specific code.
  //
  if (((AdapterInfo->LinkSpeedReq) || (AdapterInfo->DuplexReq))) {
    //
    // Find out what kind of technology this Phy is capable of.
    //
    MdiRead (
      AdapterInfo,
      MDI_STATUS_REG,
      AdapterInfo->PhyAddress,
      &MdiStatusReg
      );

    //
    // Read the MDI control register at our phy
    //
    MdiRead (
      AdapterInfo,
      MDI_CONTROL_REG,
      AdapterInfo->PhyAddress,
      &MdiControlReg
      );

    //
    // Now check the validity of our forced option.  If the force option is
    // valid, then force the setting.  If the force option is not valid,
    // we'll set a flag indicating that we should error out.
    //

    //
    // If speed is forced to 10mb
    //
    if (AdapterInfo->LinkSpeedReq == 10) {
      //
      // If half duplex is forced
      //
      if ((AdapterInfo->DuplexReq & PXE_FORCE_HALF_DUPLEX) != 0) {
        if (MdiStatusReg & MDI_SR_10T_HALF_DPX) {

          MdiControlReg &= ~(MDI_CR_10_100 | MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
          ForcePhySetting = TRUE;
        }
      } else if ((AdapterInfo->DuplexReq & PXE_FORCE_FULL_DUPLEX) != 0) {

        //
        // If full duplex is forced
        //
        if (MdiStatusReg & MDI_SR_10T_FULL_DPX) {

          MdiControlReg &= ~(MDI_CR_10_100 | MDI_CR_AUTO_SELECT);
          MdiControlReg |= MDI_CR_FULL_HALF;
          ForcePhySetting = TRUE;
        }
      } else {
        //
        // If auto duplex (we actually set phy to 1/2)
        //
        if (MdiStatusReg & (MDI_SR_10T_FULL_DPX | MDI_SR_10T_HALF_DPX)) {

          MdiControlReg &= ~(MDI_CR_10_100 | MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
          ForcePhySetting = TRUE;
        }
      }
    }

    //
    // If speed is forced to 100mb
    //
    else if (AdapterInfo->LinkSpeedReq == 100) {
      //
      // If half duplex is forced
      //
      if ((AdapterInfo->DuplexReq & PXE_FORCE_HALF_DUPLEX) != 0) {
        if (MdiStatusReg & (MDI_SR_TX_HALF_DPX | MDI_SR_T4_CAPABLE)) {

          MdiControlReg &= ~(MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
          MdiControlReg |= MDI_CR_10_100;
          ForcePhySetting = TRUE;
        }
      } else if ((AdapterInfo->DuplexReq & PXE_FORCE_FULL_DUPLEX) != 0) {
        //
        // If full duplex is forced
        //
        if (MdiStatusReg & MDI_SR_TX_FULL_DPX) {
          MdiControlReg &= ~MDI_CR_AUTO_SELECT;
          MdiControlReg |= (MDI_CR_10_100 | MDI_CR_FULL_HALF);
          ForcePhySetting = TRUE;
        }
      } else {
        //
        // If auto duplex (we set phy to 1/2)
        //
        if (MdiStatusReg & (MDI_SR_TX_HALF_DPX | MDI_SR_T4_CAPABLE)) {

          MdiControlReg &= ~(MDI_CR_AUTO_SELECT | MDI_CR_FULL_HALF);
          MdiControlReg |= MDI_CR_10_100;
          ForcePhySetting = TRUE;
        }
      }
    }

    if (!ForcePhySetting) {
      return (FALSE);
    }

    //
    // Write the MDI control register with our new Phy configuration
    //
    MdiWrite (
      AdapterInfo,
      MDI_CONTROL_REG,
      AdapterInfo->PhyAddress,
      MdiControlReg
      );

    //
    // wait 100 milliseconds for auto-negotiation to complete
    //
    DelayIt (AdapterInfo, 100);
  }

  //
  // Find out specifically what Phy this is.  We do this because for certain
  // phys there are specific bits that must be set so that the phy and the
  // 82557 work together properly.
  //

  MdiRead (
    AdapterInfo,
    PHY_ID_REG_1,
    AdapterInfo->PhyAddress,
    &MdiIdLowReg
    );
  MdiRead (
    AdapterInfo,
    PHY_ID_REG_2,
    AdapterInfo->PhyAddress,
    &MdiIdHighReg
    );

  PhyId = ((UINT32) MdiIdLowReg | ((UINT32) MdiIdHighReg << 16));

  //
  // And out the revsion field of the Phy ID so that we'll be able to detect
  // future revs of the same Phy.
  //
  PhyId &= PHY_MODEL_REV_ID_MASK;

  //
  // Handle the National TX
  //
  if (PhyId == PHY_NSC_TX) {

    MdiRead (
      AdapterInfo,
      NSC_CONG_CONTROL_REG,
      AdapterInfo->PhyAddress,
      &MdiMiscReg
      );

    MdiMiscReg |= (NSC_TX_CONG_TXREADY | NSC_TX_CONG_F_CONNECT);

    MdiWrite (
      AdapterInfo,
      NSC_CONG_CONTROL_REG,
      AdapterInfo->PhyAddress,
      MdiMiscReg
      );
  }

  FindPhySpeedAndDpx (AdapterInfo, PhyId);

  //
  // We put a hardware fix on to our adapters to work-around the PHY_100 errata
  // described below.  The following code is only compiled in, if we wanted
  // to attempt a software workaround to the PHY_100 A/B step problem.
  //

  return (TRUE);
}


/**
  This routine will figure out what line speed and duplex mode
  the PHY is currently using.

  @param  AdapterInfo                     pointer to the structure that contains
                                          the NIC's context.
  @param  PhyId                           The ID of the PHY in question.

  @return NOTHING

**/
VOID
FindPhySpeedAndDpx (
  IN NIC_DATA_INSTANCE *AdapterInfo,
  IN UINT32            PhyId
  )
{
  UINT16  MdiStatusReg;
  UINT16  MdiMiscReg;
  UINT16  MdiOwnAdReg;
  UINT16  MdiLinkPartnerAdReg;

  //
  // If there was a speed and/or duplex override, then set our current
  // value accordingly
  //
  AdapterInfo->LinkSpeed  = AdapterInfo->LinkSpeedReq;
  AdapterInfo->Duplex = (UINT8) ((AdapterInfo->DuplexReq & PXE_FORCE_FULL_DUPLEX) ?
                        FULL_DUPLEX : HALF_DUPLEX);

  //
  // If speed and duplex were forced, then we know our current settings, so
  // we'll just return.  Otherwise, we'll need to figure out what NWAY set
  // us to.
  //
  if (AdapterInfo->LinkSpeed && AdapterInfo->Duplex) {
    return ;

  }
  //
  // If we didn't have a valid link, then we'll assume that our current
  // speed is 10mb half-duplex.
  //

  //
  // Read the status register twice because of sticky bits
  //
  MdiRead (
    AdapterInfo,
    MDI_STATUS_REG,
    AdapterInfo->PhyAddress,
    &MdiStatusReg
    );
  MdiRead (
    AdapterInfo,
    MDI_STATUS_REG,
    AdapterInfo->PhyAddress,
    &MdiStatusReg
    );

  //
  // If there wasn't a valid link then use default speed & duplex
  //
  if (!(MdiStatusReg & MDI_SR_LINK_STATUS)) {

    AdapterInfo->LinkSpeed  = 10;
    AdapterInfo->Duplex     = HALF_DUPLEX;
    return ;
  }

  //
  // If this is an Intel PHY (a T4 PHY_100 or a TX PHY_TX), then read bits
  // 1 and 0 of extended register 0, to get the current speed and duplex
  // settings.
  //
  if ((PhyId == PHY_100_A) || (PhyId == PHY_100_C) || (PhyId == PHY_TX_ID)) {
    //
    // Read extended register 0
    //
    MdiRead (
      AdapterInfo,
      EXTENDED_REG_0,
      AdapterInfo->PhyAddress,
      &MdiMiscReg
      );

    //
    // Get current speed setting
    //
    if (MdiMiscReg & PHY_100_ER0_SPEED_INDIC) {
      AdapterInfo->LinkSpeed = 100;
    } else {
      AdapterInfo->LinkSpeed = 10;
    }

    //
    // Get current duplex setting -- if bit is set then FDX is enabled
    //
    if (MdiMiscReg & PHY_100_ER0_FDX_INDIC) {
      AdapterInfo->Duplex = FULL_DUPLEX;
    } else {
      AdapterInfo->Duplex = HALF_DUPLEX;
    }

    return ;
  }
  //
  // Read our link partner's advertisement register
  //
  MdiRead (
    AdapterInfo,
    AUTO_NEG_LINK_PARTNER_REG,
    AdapterInfo->PhyAddress,
    &MdiLinkPartnerAdReg
    );

  //
  // See if Auto-Negotiation was complete (bit 5, reg 1)
  //
  MdiRead (
    AdapterInfo,
    MDI_STATUS_REG,
    AdapterInfo->PhyAddress,
    &MdiStatusReg
    );

  //
  // If a True NWAY connection was made, then we can detect speed/duplex by
  // ANDing our adapter's advertised abilities with our link partner's
  // advertised ablilities, and then assuming that the highest common
  // denominator was chosed by NWAY.
  //
  if ((MdiLinkPartnerAdReg & NWAY_LP_ABILITY) &&
      (MdiStatusReg & MDI_SR_AUTO_NEG_COMPLETE)) {

    //
    // Read our advertisement register
    //
    MdiRead (
      AdapterInfo,
      AUTO_NEG_ADVERTISE_REG,
      AdapterInfo->PhyAddress,
      &MdiOwnAdReg
      );

    //
    // AND the two advertisement registers together, and get rid of any
    // extraneous bits.
    //
    MdiOwnAdReg = (UINT16) (MdiOwnAdReg & (MdiLinkPartnerAdReg & NWAY_LP_ABILITY));

    //
    // Get speed setting
    //
    if (MdiOwnAdReg & (NWAY_AD_TX_HALF_DPX | NWAY_AD_TX_FULL_DPX | NWAY_AD_T4_CAPABLE)) {
      AdapterInfo->LinkSpeed = 100;
    } else {
      AdapterInfo->LinkSpeed = 10;
    }

    //
    // Get duplex setting -- use priority resolution algorithm
    //
    if (MdiOwnAdReg & (NWAY_AD_T4_CAPABLE)) {
      AdapterInfo->Duplex = HALF_DUPLEX;
      return ;
    } else if (MdiOwnAdReg & (NWAY_AD_TX_FULL_DPX)) {
      AdapterInfo->Duplex = FULL_DUPLEX;
      return ;
    } else if (MdiOwnAdReg & (NWAY_AD_TX_HALF_DPX)) {
      AdapterInfo->Duplex = HALF_DUPLEX;
      return ;
    } else if (MdiOwnAdReg & (NWAY_AD_10T_FULL_DPX)) {
      AdapterInfo->Duplex = FULL_DUPLEX;
      return ;
    } else {
      AdapterInfo->Duplex = HALF_DUPLEX;
      return ;
    }
  }

  //
  // If we are connected to a dumb (non-NWAY) repeater or hub, and the line
  // speed was determined automatically by parallel detection, then we have
  // no way of knowing exactly what speed the PHY is set to unless that PHY
  // has a propietary register which indicates speed in this situation.  The
  // NSC TX PHY does have such a register.  Also, since NWAY didn't establish
  // the connection, the duplex setting should HALF duplex.
  //
  AdapterInfo->Duplex = HALF_DUPLEX;

  if (PhyId == PHY_NSC_TX) {
    //
    // Read register 25 to get the SPEED_10 bit
    //
    MdiRead (
      AdapterInfo,
      NSC_SPEED_IND_REG,
      AdapterInfo->PhyAddress,
      &MdiMiscReg
      );

    //
    // If bit 6 was set then we're at 10mb
    //
    if (MdiMiscReg & NSC_TX_SPD_INDC_SPEED) {
      AdapterInfo->LinkSpeed = 10;
    } else {
      AdapterInfo->LinkSpeed = 100;
    }
  }

  //
  // If we don't know what line speed we are set at, then we'll default to
  // 10mbs
  //
  else {
    AdapterInfo->LinkSpeed = 10;
  }
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
VOID
XmitWaitForCompletion (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  TxCB  *TxPtr;

  if (AdapterInfo->FreeCBCount == AdapterInfo->TxBufCnt) {
    return ;
  }

  //
  // used xmit cb list starts right after the free tail (ends before the
  // free head ptr)
  //
  TxPtr = AdapterInfo->FreeTxTailPtr->NextTCBVirtualLinkPtr;
  while (TxPtr != AdapterInfo->FreeTxHeadPtr) {
    CommandWaitForCompletion (TxPtr, AdapterInfo);
    SetFreeCB (AdapterInfo, TxPtr);
    TxPtr = TxPtr->NextTCBVirtualLinkPtr;
  }
}


/**
  TODO: Add function description

  @param  cmd_ptr                         TODO: add argument description
  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
INT8
CommandWaitForCompletion (
  TxCB              *cmd_ptr,
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  INT16 wait;
  wait = 5000;
  while ((cmd_ptr->cb_header.status == 0) && (--wait > 0)) {
    DelayIt (AdapterInfo, 10);
  }

  if (cmd_ptr->cb_header.status == 0) {
    return -1;
  }

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
INT8
SoftwareReset (
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT8   tco_stat;
  UINT16  wait;

  tco_stat = 0;

  //
  // Reset the chip: stop Tx and Rx processes and clear counters.
  // This takes less than 10usec and will easily finish before the next
  // action.
  //

  OutLong (AdapterInfo, PORT_RESET, AdapterInfo->ioaddr + SCBPort);
  //
  // wait for 5 milli seconds here!
  //
  DelayIt (AdapterInfo, 5000);
  //
  // TCO Errata work around for 559s only
  // -----------------------------------------------------------------------------------
  // TCO Workaround Code
  //  haifa workaround
  // -----------------------------------------------------------------------------------
  //    1. Issue SW-RST ^^^ (already done above)
  //    2. Issue a redundant Set CU Base CMD immediately
  //       Do not set the General Pointer before the Set CU Base cycle
  //       Do not check the SCB CMD before the Set CU Base cycle
  //    3. Wait for the SCB-CMD to be cleared
  //       this indicates the transition to post-driver
  //    4. Poll the TCO-Req bit in the PMDR to be cleared
  //       this indicates the tco activity has stopped for real
  //    5. Proceed with the nominal Driver Init:
  //       Actual Set CU & RU Base ...
  //
  // Check for ICH2 device ID.  If this is an ICH2,
  // do the TCO workaround code.
  //
  if (AdapterInfo->VendorID == D102_DEVICE_ID ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_1 ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_2 ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_3 ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_4 ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_5 ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_6 ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_7 ||
      AdapterInfo->VendorID == ICH3_DEVICE_ID_8 ||
      AdapterInfo->RevID >= 8) {  // do the TCO fix
    //
    // donot load the scb pointer but just give load_cu cmd.
    //
    OutByte (AdapterInfo, CU_CMD_BASE, AdapterInfo->ioaddr + SCBCmd);
    //
    // wait for command to be accepted.
    //
    wait_for_cmd_done (AdapterInfo->ioaddr + SCBCmd);
    //
    // read PMDR register and check bit 1 in it to see if TCO is active
    //

    //
    // wait for 5 milli seconds
    //
    wait = 5000;
    while (wait) {
      tco_stat = InByte (AdapterInfo, AdapterInfo->ioaddr + 0x1b);
      if ((tco_stat & 2) == 0) {
        //
        // is the activity bit clear??
        //
        break;
      }

      wait--;
      DelayIt (AdapterInfo, 1);
    }

    if ((tco_stat & 2) != 0) {
      //
      // not zero??
      //
      return -1;
    }
  }

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT8
SelectiveReset (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT16  wait;
  UINT32  stat;

  wait  = 10;
  stat  = 0;
  OutLong (AdapterInfo, POR_SELECTIVE_RESET, AdapterInfo->ioaddr + SCBPort);
  //
  // wait for this to complete
  //

  //
  // wait for 2 milli seconds here!
  //
  DelayIt (AdapterInfo, 2000);
  while (wait > 0) {
    wait--;
    stat = InLong (AdapterInfo, AdapterInfo->ioaddr + SCBPort);
    if (stat == 0) {
      break;
    }

    //
    // wait for 1 milli second
    //
    DelayIt (AdapterInfo, 1000);
  }

  if (stat != 0) {
    return PXE_STATCODE_DEVICE_FAILURE;
  }

  return 0;
}


/**
  TODO: Add function description

  @param  AdapterInfo                     TODO: add argument description

  @return TODO: add return values

**/
UINT16
InitializeChip (
  IN NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT16  ret_val;
  if (SoftwareReset (AdapterInfo) != 0) {
    return PXE_STATCODE_DEVICE_FAILURE;
  }

  //
  // disable interrupts
  //
  OutWord (AdapterInfo, INT_MASK, AdapterInfo->ioaddr + SCBCmd);

  //
  // Load the base registers with 0s (we will give the complete address as
  // offset later when we issue any command
  //
  if ((ret_val = Load_Base_Regs (AdapterInfo)) != 0) {
    return ret_val;
  }

  if ((ret_val = SetupCBlink (AdapterInfo)) != 0) {
    return ret_val;
  }

  if ((ret_val = SetupReceiveQueues (AdapterInfo)) != 0) {
    return ret_val;
  }

  //
  // detect the PHY only if we need to detect the cable as requested by the
  // initialize parameters
  //
  AdapterInfo->PhyAddress = 0xFF;

  if (AdapterInfo->CableDetect != 0) {
    if (!PhyDetect (AdapterInfo)) {
      return PXE_STATCODE_DEVICE_FAILURE;
    }
  }

  if ((ret_val = E100bSetupIAAddr (AdapterInfo)) != 0) {
    return ret_val;
  }

  if ((ret_val = Configure (AdapterInfo)) != 0) {
    return ret_val;
  }

  return 0;
}

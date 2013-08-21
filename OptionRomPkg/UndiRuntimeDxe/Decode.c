/** @file
  Provides the basic UNID functions.

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Undi32.h"

//
// Global variables defined in this file
//
UNDI_CALL_TABLE api_table[PXE_OPCODE_LAST_VALID+1] = { \
  {PXE_CPBSIZE_NOT_USED,PXE_DBSIZE_NOT_USED,0, (UINT16)(ANY_STATE),UNDI_GetState },\
  {(UINT16)(DONT_CHECK),PXE_DBSIZE_NOT_USED,0,(UINT16)(ANY_STATE),UNDI_Start },\
  {PXE_CPBSIZE_NOT_USED,PXE_DBSIZE_NOT_USED,0,MUST_BE_STARTED,UNDI_Stop },\
  {PXE_CPBSIZE_NOT_USED,sizeof(PXE_DB_GET_INIT_INFO),0,MUST_BE_STARTED, UNDI_GetInitInfo },\
  {PXE_CPBSIZE_NOT_USED,sizeof(PXE_DB_GET_CONFIG_INFO),0,MUST_BE_STARTED, UNDI_GetConfigInfo },\
  {sizeof(PXE_CPB_INITIALIZE),(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK),MUST_BE_STARTED,UNDI_Initialize },\
  {PXE_CPBSIZE_NOT_USED,PXE_DBSIZE_NOT_USED,(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED,UNDI_Reset },\
  {PXE_CPBSIZE_NOT_USED,PXE_DBSIZE_NOT_USED,0, MUST_BE_INITIALIZED,UNDI_Shutdown },\
  {PXE_CPBSIZE_NOT_USED,PXE_DBSIZE_NOT_USED,(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED,UNDI_Interrupt },\
  {(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED, UNDI_RecFilter },\
  {(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED, UNDI_StnAddr },\
  {PXE_CPBSIZE_NOT_USED, (UINT16)(DONT_CHECK), (UINT16)(DONT_CHECK), MUST_BE_INITIALIZED, UNDI_Statistics },\
  {sizeof(PXE_CPB_MCAST_IP_TO_MAC),sizeof(PXE_DB_MCAST_IP_TO_MAC), (UINT16)(DONT_CHECK),MUST_BE_INITIALIZED, UNDI_ip2mac },\
  {(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED, UNDI_NVData },\
  {PXE_CPBSIZE_NOT_USED,(UINT16)(DONT_CHECK),(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED, UNDI_Status },\
  {(UINT16)(DONT_CHECK),PXE_DBSIZE_NOT_USED,(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED, UNDI_FillHeader },\
  {(UINT16)(DONT_CHECK),PXE_DBSIZE_NOT_USED,(UINT16)(DONT_CHECK), MUST_BE_INITIALIZED, UNDI_Transmit },\
  {sizeof(PXE_CPB_RECEIVE),sizeof(PXE_DB_RECEIVE),0,MUST_BE_INITIALIZED, UNDI_Receive } \
};

//
// end of global variables
//


/**
  This routine determines the operational state of the UNDI.  It updates the state flags in the
  Command Descriptor Block based on information derived from the AdapterInfo instance data.
  To ensure the command has completed successfully, CdbPtr->StatCode will contain the result of
  the command execution.
  The CdbPtr->StatFlags will contain a STOPPED, STARTED, or INITIALIZED state once the command
  has successfully completed.
  Keep in mind the AdapterInfo->State is the active state of the adapter (based on software
  interrogation), and the CdbPtr->StateFlags is the passed back information that is reflected
  to the caller of the UNDI API.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_GetState (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  CdbPtr->StatFlags = (PXE_STATFLAGS) (CdbPtr->StatFlags | AdapterInfo->State);
  return ;
}


/**
  This routine is used to change the operational state of the UNDI from stopped to started.
  It will do this as long as the adapter's state is PXE_STATFLAGS_GET_STATE_STOPPED, otherwise
  the CdbPtr->StatFlags will reflect a command failure, and the CdbPtr->StatCode will reflect the
  UNDI as having already been started.
  This routine is modified to reflect the undi 1.1 specification changes. The
  changes in the spec are mainly in the callback routines, the new spec adds
  3 more callbacks and a unique id.
  Since this UNDI supports both old and new undi specifications,
  The NIC's data structure is filled in with the callback routines (depending
  on the version) pointed to in the caller's CpbPtr.  This seeds the Delay,
  Virt2Phys, Block, and Mem_IO for old and new versions and Map_Mem, UnMap_Mem
  and Sync_Mem routines and a unique id variable for the new version.
  This is the function which an external entity (SNP, O/S, etc) would call
  to provide it's I/O abstraction to the UNDI.
  It's final action is to change the AdapterInfo->State to PXE_STATFLAGS_GET_STATE_STARTED.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Start (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_CPB_START_30  *CpbPtr;
  PXE_CPB_START_31  *CpbPtr_31;

  //
  // check if it is already started.
  //
  if (AdapterInfo->State != PXE_STATFLAGS_GET_STATE_STOPPED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_ALREADY_STARTED;
    return ;
  }

  if (CdbPtr->CPBsize != sizeof(PXE_CPB_START_30) &&
      CdbPtr->CPBsize != sizeof(PXE_CPB_START_31)) {

    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  CpbPtr    = (PXE_CPB_START_30 *) (UINTN) (CdbPtr->CPBaddr);
  CpbPtr_31 = (PXE_CPB_START_31 *) (UINTN) (CdbPtr->CPBaddr);

  if (AdapterInfo->VersionFlag == 0x30) {
    AdapterInfo->Delay_30     = (bsptr_30) (UINTN) CpbPtr->Delay;
    AdapterInfo->Virt2Phys_30 = (virtphys_30) (UINTN) CpbPtr->Virt2Phys;
    AdapterInfo->Block_30     = (block_30) (UINTN) CpbPtr->Block;
    //
    // patch for old buggy 3.0 code:
    // In EFI1.0 undi used to provide the full (absolute) I/O address to the
    // i/o calls and SNP used to provide a callback that used GlobalIoFncs and
    // everything worked fine! In EFI 1.1, UNDI is not using the full
    // i/o or memory address to access the device, The base values for the i/o
    // and memory address is abstracted by the device specific PciIoFncs and
    // UNDI only uses the offset values. Since UNDI3.0 cannot provide any
    // identification to SNP, SNP cannot use nic specific PciIoFncs callback!
    //
    // To fix this and make undi3.0 work with SNP in EFI1.1 we
    // use a TmpMemIo function that is defined in init.c
    // This breaks the runtime driver feature of undi, but what to do
    // if we have to provide the 3.0 compatibility (including the 3.0 bugs)
    //
    // This TmpMemIo function also takes a UniqueId parameter
    // (as in undi3.1 design) and so initialize the UniqueId as well here
    // Note: AdapterInfo->Mem_Io_30 is just filled for consistency with other
    // parameters but never used, we only use Mem_Io field in the In/Out routines
    // inside e100b.c.
    //
    AdapterInfo->Mem_Io_30  = (mem_io_30) (UINTN) CpbPtr->Mem_IO;
    AdapterInfo->Mem_Io     = (mem_io) (UINTN) TmpMemIo;
    AdapterInfo->Unique_ID  = (UINT64) (UINTN) AdapterInfo;

  } else {
    AdapterInfo->Delay      = (bsptr) (UINTN) CpbPtr_31->Delay;
    AdapterInfo->Virt2Phys  = (virtphys) (UINTN) CpbPtr_31->Virt2Phys;
    AdapterInfo->Block      = (block) (UINTN) CpbPtr_31->Block;
    AdapterInfo->Mem_Io     = (mem_io) (UINTN) CpbPtr_31->Mem_IO;

    AdapterInfo->Map_Mem    = (map_mem) (UINTN) CpbPtr_31->Map_Mem;
    AdapterInfo->UnMap_Mem  = (unmap_mem) (UINTN) CpbPtr_31->UnMap_Mem;
    AdapterInfo->Sync_Mem   = (sync_mem) (UINTN) CpbPtr_31->Sync_Mem;
    AdapterInfo->Unique_ID  = CpbPtr_31->Unique_ID;
  }

  AdapterInfo->State = PXE_STATFLAGS_GET_STATE_STARTED;

  return ;
}


/**
  This routine is used to change the operational state of the UNDI from started to stopped.
  It will not do this if the adapter's state is PXE_STATFLAGS_GET_STATE_INITIALIZED, otherwise
  the CdbPtr->StatFlags will reflect a command failure, and the CdbPtr->StatCode will reflect the
  UNDI as having already not been shut down.
  The NIC's data structure will have the Delay, Virt2Phys, and Block, pointers zero'd out..
  It's final action is to change the AdapterInfo->State to PXE_STATFLAGS_GET_STATE_STOPPED.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Stop (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  if (AdapterInfo->State == PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_NOT_SHUTDOWN;
    return ;
  }

  AdapterInfo->Delay_30     = 0;
  AdapterInfo->Virt2Phys_30 = 0;
  AdapterInfo->Block_30     = 0;

  AdapterInfo->Delay        = 0;
  AdapterInfo->Virt2Phys    = 0;
  AdapterInfo->Block        = 0;

  AdapterInfo->Map_Mem      = 0;
  AdapterInfo->UnMap_Mem    = 0;
  AdapterInfo->Sync_Mem     = 0;

  AdapterInfo->State        = PXE_STATFLAGS_GET_STATE_STOPPED;

  return ;
}


/**
  This routine is used to retrieve the initialization information that is needed by drivers and
  applications to initialize the UNDI.  This will fill in data in the Data Block structure that is
  pointed to by the caller's CdbPtr->DBaddr.  The fields filled in are as follows:
  MemoryRequired, FrameDataLen, LinkSpeeds[0-3], NvCount, NvWidth, MediaHeaderLen, HWaddrLen,
  MCastFilterCnt, TxBufCnt, TxBufSize, RxBufCnt, RxBufSize, IFtype, Duplex, and LoopBack.
  In addition, the CdbPtr->StatFlags ORs in that this NIC supports cable detection.  (APRIORI knowledge)

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_GetInitInfo (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_DB_GET_INIT_INFO  *DbPtr;

  DbPtr = (PXE_DB_GET_INIT_INFO *) (UINTN) (CdbPtr->DBaddr);

  DbPtr->MemoryRequired = MEMORY_NEEDED;
  DbPtr->FrameDataLen = PXE_MAX_TXRX_UNIT_ETHER;
  DbPtr->LinkSpeeds[0] = 10;
  DbPtr->LinkSpeeds[1] = 100;
  DbPtr->LinkSpeeds[2] = DbPtr->LinkSpeeds[3] = 0;
  DbPtr->NvCount = MAX_EEPROM_LEN;
  DbPtr->NvWidth = 4;
  DbPtr->MediaHeaderLen = PXE_MAC_HEADER_LEN_ETHER;
  DbPtr->HWaddrLen = PXE_HWADDR_LEN_ETHER;
  DbPtr->MCastFilterCnt = MAX_MCAST_ADDRESS_CNT;

  DbPtr->TxBufCnt = TX_BUFFER_COUNT;
  DbPtr->TxBufSize = (UINT16) sizeof (TxCB);
  DbPtr->RxBufCnt = RX_BUFFER_COUNT;
  DbPtr->RxBufSize = (UINT16) sizeof (RxFD);

  DbPtr->IFtype = PXE_IFTYPE_ETHERNET;
  DbPtr->SupportedDuplexModes = PXE_DUPLEX_ENABLE_FULL_SUPPORTED |
                  PXE_DUPLEX_FORCE_FULL_SUPPORTED;
  DbPtr->SupportedLoopBackModes = PXE_LOOPBACK_INTERNAL_SUPPORTED |
                    PXE_LOOPBACK_EXTERNAL_SUPPORTED;

  CdbPtr->StatFlags |= (PXE_STATFLAGS_CABLE_DETECT_SUPPORTED |
                        PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED);
  return ;
}


/**
  This routine is used to retrieve the configuration information about the NIC being controlled by
  this driver.  This will fill in data in the Data Block structure that is pointed to by the caller's CdbPtr->DBaddr.
  The fields filled in are as follows:
  DbPtr->pci.BusType, DbPtr->pci.Bus, DbPtr->pci.Device, and DbPtr->pci.
  In addition, the DbPtr->pci.Config.Dword[0-63] grabs a copy of this NIC's PCI configuration space.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_GetConfigInfo (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT16                  Index;
  PXE_DB_GET_CONFIG_INFO  *DbPtr;

  DbPtr               = (PXE_DB_GET_CONFIG_INFO *) (UINTN) (CdbPtr->DBaddr);

  DbPtr->pci.BusType  = PXE_BUSTYPE_PCI;
  DbPtr->pci.Bus      = AdapterInfo->Bus;
  DbPtr->pci.Device   = AdapterInfo->Device;
  DbPtr->pci.Function = AdapterInfo->Function;

  for (Index = 0; Index < MAX_PCI_CONFIG_LEN; Index++) {
    DbPtr->pci.Config.Dword[Index] = AdapterInfo->Config[Index];
  }

  return ;
}


/**
  This routine resets the network adapter and initializes the UNDI using the parameters supplied in
  the CPB.  This command must be issued before the network adapter can be setup to transmit and
  receive packets.
  Once the memory requirements of the UNDI are obtained by using the GetInitInfo command, a block
  of non-swappable memory may need to be allocated.  The address of this memory must be passed to
  UNDI during the Initialize in the CPB.  This memory is used primarily for transmit and receive buffers.
  The fields CableDetect, LinkSpeed, Duplex, LoopBack, MemoryPtr, and MemoryLength are set with information
  that was passed in the CPB and the NIC is initialized.
  If the NIC initialization fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED
  Otherwise, AdapterInfo->State is updated with PXE_STATFLAGS_GET_STATE_INITIALIZED showing the state of
  the UNDI is now initialized.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Initialize (
  IN  PXE_CDB       *CdbPtr,
  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_CPB_INITIALIZE  *CpbPtr;

  if ((CdbPtr->OpFlags != PXE_OPFLAGS_INITIALIZE_DETECT_CABLE) &&
      (CdbPtr->OpFlags != PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE)) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  //
  // check if it is already initialized
  //
  if (AdapterInfo->State == PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_ALREADY_INITIALIZED;
    return ;
  }

  CpbPtr  = (PXE_CPB_INITIALIZE *) (UINTN) CdbPtr->CPBaddr;

  if (CpbPtr->MemoryLength < (UINT32) MEMORY_NEEDED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CPB;
    return ;
  }

  //
  // default behaviour is to detect the cable, if the 3rd param is 1,
  // do not do that
  //
  AdapterInfo->CableDetect = (UINT8) ((CdbPtr->OpFlags == (UINT16) PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE) ? (UINT8) 0 : (UINT8) 1);
  AdapterInfo->LinkSpeedReq = (UINT16) CpbPtr->LinkSpeed;
  AdapterInfo->DuplexReq    = CpbPtr->DuplexMode;
  AdapterInfo->LoopBack     = CpbPtr->LoopBackMode;
  AdapterInfo->MemoryPtr    = CpbPtr->MemoryAddr;
  AdapterInfo->MemoryLength = CpbPtr->MemoryLength;

  CdbPtr->StatCode          = (PXE_STATCODE) E100bInit (AdapterInfo);

  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  } else {
    AdapterInfo->State = PXE_STATFLAGS_GET_STATE_INITIALIZED;
  }

  return ;
}


/**
  This routine resets the network adapter and initializes the UNDI using the parameters supplied in
  the CPB.  The transmit and receive queues are emptied and any pending interrupts are cleared.
  If the NIC reset fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Reset (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  if (CdbPtr->OpFlags != PXE_OPFLAGS_NOT_USED &&
      CdbPtr->OpFlags != PXE_OPFLAGS_RESET_DISABLE_INTERRUPTS &&
      CdbPtr->OpFlags != PXE_OPFLAGS_RESET_DISABLE_FILTERS ) {

    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  CdbPtr->StatCode = (UINT16) E100bReset (AdapterInfo, CdbPtr->OpFlags);

  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  }
}


/**
  This routine resets the network adapter and leaves it in a safe state for another driver to
  initialize.  Any pending transmits or receives are lost.  Receive filters and external
  interrupt enables are disabled.  Once the UNDI has been shutdown, it can then be stopped
  or initialized again.
  If the NIC reset fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED
  Otherwise, AdapterInfo->State is updated with PXE_STATFLAGS_GET_STATE_STARTED showing the state of
  the NIC as being started.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Shutdown (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  //
  // do the shutdown stuff here
  //
  CdbPtr->StatCode = (UINT16) E100bShutdown (AdapterInfo);

  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  } else {
    AdapterInfo->State = PXE_STATFLAGS_GET_STATE_STARTED;
  }

  return ;
}


/**
  This routine can be used to read and/or change the current external interrupt enable
  settings.  Disabling an external interrupt enable prevents and external (hardware)
  interrupt from being signaled by the network device.  Internally the interrupt events
  can still be polled by using the UNDI_GetState command.
  The resulting information on the interrupt state will be passed back in the CdbPtr->StatFlags.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Interrupt (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT8 IntMask;

  IntMask = (UINT8)(UINTN)(CdbPtr->OpFlags & (PXE_OPFLAGS_INTERRUPT_RECEIVE |
                                              PXE_OPFLAGS_INTERRUPT_TRANSMIT |
                                              PXE_OPFLAGS_INTERRUPT_COMMAND |
                                              PXE_OPFLAGS_INTERRUPT_SOFTWARE));

  switch (CdbPtr->OpFlags & PXE_OPFLAGS_INTERRUPT_OPMASK) {
  case PXE_OPFLAGS_INTERRUPT_READ:
    break;

  case PXE_OPFLAGS_INTERRUPT_ENABLE:
    if (IntMask == 0) {
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
      return ;
    }

    AdapterInfo->int_mask = IntMask;
    E100bSetInterruptState (AdapterInfo);
    break;

  case PXE_OPFLAGS_INTERRUPT_DISABLE:
    if (IntMask != 0) {
      AdapterInfo->int_mask = (UINT16) (AdapterInfo->int_mask & ~(IntMask));
      E100bSetInterruptState (AdapterInfo);
      break;
    }

  //
  // else fall thru.
  //
  default:
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  if ((AdapterInfo->int_mask & PXE_OPFLAGS_INTERRUPT_RECEIVE) != 0) {
    CdbPtr->StatFlags |= PXE_STATFLAGS_INTERRUPT_RECEIVE;

  }

  if ((AdapterInfo->int_mask & PXE_OPFLAGS_INTERRUPT_TRANSMIT) != 0) {
    CdbPtr->StatFlags |= PXE_STATFLAGS_INTERRUPT_TRANSMIT;

  }

  if ((AdapterInfo->int_mask & PXE_OPFLAGS_INTERRUPT_COMMAND) != 0) {
    CdbPtr->StatFlags |= PXE_STATFLAGS_INTERRUPT_COMMAND;

  }

  return ;
}


/**
  This routine is used to read and change receive filters and, if supported, read
  and change multicast MAC address filter list.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_RecFilter (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  UINT16                  NewFilter;
  UINT16                  OpFlags;
  PXE_DB_RECEIVE_FILTERS  *DbPtr;
  UINT8                   *MacAddr;
  UINTN                   MacCount;
  UINT16                  Index;
  UINT16                  copy_len;
  UINT8                   *ptr1;
  UINT8                   *ptr2;
  BOOLEAN                 InvalidMacAddr;
    
  OpFlags   = CdbPtr->OpFlags;
  NewFilter = (UINT16) (OpFlags & 0x1F);

  switch (OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_OPMASK) {
  case PXE_OPFLAGS_RECEIVE_FILTER_READ:

    //
    // not expecting a cpb, not expecting any filter bits
    //
    if ((NewFilter != 0) || (CdbPtr->CPBsize != 0)) {
      goto BadCdb;

    }

    if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) == 0) {
      goto JustRead;

    }

    NewFilter = (UINT16) (NewFilter | AdapterInfo->Rx_Filter);
    //
    // all other flags are ignored except mcast_reset
    //
    break;

  case PXE_OPFLAGS_RECEIVE_FILTER_ENABLE:
    //
    // there should be atleast one other filter bit set.
    //
    if (NewFilter == 0) {
      //
      // nothing to enable
      //
      goto BadCdb;
    }

    if (CdbPtr->CPBsize != 0) {
      //
      // this must be a multicast address list!
      // don't accept the list unless selective_mcast is set
      // don't accept confusing mcast settings with this
      //
      if (((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) == 0) ||
          ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) != 0) ||
          ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0) ||
          ((CdbPtr->CPBsize % sizeof (PXE_MAC_ADDR)) != 0) ) {
        goto BadCdb;
      }

      MacAddr   = (UINT8 *) ((UINTN) (CdbPtr->CPBaddr));
      MacCount  = CdbPtr->CPBsize / sizeof (PXE_MAC_ADDR);

      //
      // The format of Ethernet multicast address for IPv6 is defined in RFC2464,
      // for IPv4 is defined in RFC1112. Check whether the address is valid.
      //
      InvalidMacAddr = FALSE;
      
      for (; MacCount-- != 0; MacAddr += sizeof (PXE_MAC_ADDR)) {
        if (MacAddr[0] == 0x01) {
          //
          // This multicast MAC address is mapped from IPv4 address.
          //
          if (MacAddr[1] != 0x00 || MacAddr[2] != 0x5E || (MacAddr[3] & 0x80) != 0) {
            InvalidMacAddr = TRUE;
          }          
        } else if (MacAddr[0] == 0x33) {
          //
          // This multicast MAC address is mapped from IPv6 address.
          //
          if (MacAddr[1] != 0x33) {
            InvalidMacAddr = TRUE;
          }
        } else {
          InvalidMacAddr = TRUE;
        }

        if (InvalidMacAddr) {
          CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
          CdbPtr->StatCode  = PXE_STATCODE_INVALID_CPB;
          return ;
        }
      }
    }

    //
    // check selective mcast case enable case
    //
    if ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) != 0) {
      if (((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) != 0) ||
          ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0) ) {
        goto BadCdb;

      }
      //
      // if no cpb, make sure we have an old list
      //
      if ((CdbPtr->CPBsize == 0) && (AdapterInfo->mcast_list.list_len == 0)) {
        goto BadCdb;
      }
    }
    //
    // if you want to enable anything, you got to have unicast
    // and you have what you already enabled!
    //
    NewFilter = (UINT16) (NewFilter | (PXE_OPFLAGS_RECEIVE_FILTER_UNICAST | AdapterInfo->Rx_Filter));

    break;

  case PXE_OPFLAGS_RECEIVE_FILTER_DISABLE:

    //
    // mcast list not expected, i.e. no cpb here!
    //
    if (CdbPtr->CPBsize != PXE_CPBSIZE_NOT_USED) {
      goto BadCdb;
    }

    NewFilter = (UINT16) ((~(CdbPtr->OpFlags & 0x1F)) & AdapterInfo->Rx_Filter);

    break;

  default:
    goto BadCdb;
  }

  if ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) != 0) {
    AdapterInfo->mcast_list.list_len = 0;
    NewFilter &= (~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST);
  }

  E100bSetfilter (AdapterInfo, NewFilter, CdbPtr->CPBaddr, CdbPtr->CPBsize);

JustRead:
  //
  // give the current mcast list
  //
  if ((CdbPtr->DBsize != 0) && (AdapterInfo->mcast_list.list_len != 0)) {
    //
    // copy the mc list to db
    //

    DbPtr = (PXE_DB_RECEIVE_FILTERS *) (UINTN) CdbPtr->DBaddr;
    ptr1  = (UINT8 *) (&DbPtr->MCastList[0]);

    //
    // DbPtr->mc_count = AdapterInfo->mcast_list.list_len;
    //
    copy_len = (UINT16) (AdapterInfo->mcast_list.list_len * PXE_MAC_LENGTH);

    if (copy_len > CdbPtr->DBsize) {
      copy_len = CdbPtr->DBsize;

    }

    ptr2 = (UINT8 *) (&AdapterInfo->mcast_list.mc_list[0]);
    for (Index = 0; Index < copy_len; Index++) {
      ptr1[Index] = ptr2[Index];
    }
  }
  //
  // give the stat flags here
  //
  if (AdapterInfo->Receive_Started) {
    CdbPtr->StatFlags = (PXE_STATFLAGS) (CdbPtr->StatFlags | AdapterInfo->Rx_Filter);

  }

  return ;

BadCdb:
  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
}


/**
  This routine is used to get the current station and broadcast MAC addresses, and to change the
  current station MAC address.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_StnAddr (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_CPB_STATION_ADDRESS *CpbPtr;
  PXE_DB_STATION_ADDRESS  *DbPtr;
  UINT16                  Index;

  if (CdbPtr->OpFlags == PXE_OPFLAGS_STATION_ADDRESS_RESET) {
    //
    // configure the permanent address.
    // change the AdapterInfo->CurrentNodeAddress field.
    //
    if (CompareMem (
          &AdapterInfo->CurrentNodeAddress[0],
          &AdapterInfo->PermNodeAddress[0],
          PXE_MAC_LENGTH
          ) != 0) {
      for (Index = 0; Index < PXE_MAC_LENGTH; Index++) {
        AdapterInfo->CurrentNodeAddress[Index] = AdapterInfo->PermNodeAddress[Index];
      }

      E100bSetupIAAddr (AdapterInfo);
    }
  }

  if (CdbPtr->CPBaddr != (UINT64) 0) {
    CpbPtr = (PXE_CPB_STATION_ADDRESS *) (UINTN) (CdbPtr->CPBaddr);
    //
    // configure the new address
    //
    for (Index = 0; Index < PXE_MAC_LENGTH; Index++) {
      AdapterInfo->CurrentNodeAddress[Index] = CpbPtr->StationAddr[Index];
    }

    E100bSetupIAAddr (AdapterInfo);
  }

  if (CdbPtr->DBaddr != (UINT64) 0) {
    DbPtr = (PXE_DB_STATION_ADDRESS *) (UINTN) (CdbPtr->DBaddr);
    //
    // fill it with the new values
    //
    for (Index = 0; Index < PXE_MAC_LENGTH; Index++) {
      DbPtr->StationAddr[Index]   = AdapterInfo->CurrentNodeAddress[Index];
      DbPtr->BroadcastAddr[Index] = AdapterInfo->BroadcastNodeAddress[Index];
      DbPtr->PermanentAddr[Index] = AdapterInfo->PermNodeAddress[Index];
    }
  }

  return ;
}


/**
  This routine is used to read and clear the NIC traffic statistics.  This command is supported only
  if the !PXE structure's Implementation flags say so.
  Results will be parsed out in the following manner:
  CdbPtr->DBaddr.Data[0]   R  Total Frames (Including frames with errors and dropped frames)
  CdbPtr->DBaddr.Data[1]   R  Good Frames (All frames copied into receive buffer)
  CdbPtr->DBaddr.Data[2]   R  Undersize Frames (Frames below minimum length for media <64 for ethernet)
  CdbPtr->DBaddr.Data[4]   R  Dropped Frames (Frames that were dropped because receive buffers were full)
  CdbPtr->DBaddr.Data[8]   R  CRC Error Frames (Frames with alignment or CRC errors)
  CdbPtr->DBaddr.Data[A]   T  Total Frames (Including frames with errors and dropped frames)
  CdbPtr->DBaddr.Data[B]   T  Good Frames (All frames copied into transmit buffer)
  CdbPtr->DBaddr.Data[C]   T  Undersize Frames (Frames below minimum length for media <64 for ethernet)
  CdbPtr->DBaddr.Data[E]   T  Dropped Frames (Frames that were dropped because of collisions)
  CdbPtr->DBaddr.Data[14]  T  Total Collision Frames (Total collisions on this subnet)

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Statistics (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  if ((CdbPtr->OpFlags &~(PXE_OPFLAGS_STATISTICS_RESET)) != 0) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  if ((CdbPtr->OpFlags & PXE_OPFLAGS_STATISTICS_RESET) != 0) {
    //
    // Reset the statistics
    //
    CdbPtr->StatCode = (UINT16) E100bStatistics (AdapterInfo, 0, 0);
  } else {
    CdbPtr->StatCode = (UINT16) E100bStatistics (AdapterInfo, CdbPtr->DBaddr, CdbPtr->DBsize);
  }

  return ;
}


/**
  This routine is used to translate a multicast IP address to a multicast MAC address.
  This results in a MAC address composed of 25 bits of fixed data with the upper 23 bits of the IP
  address being appended to it.  Results passed back in the equivalent of CdbPtr->DBaddr->MAC[0-5].

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_ip2mac (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_CPB_MCAST_IP_TO_MAC *CpbPtr;
  PXE_DB_MCAST_IP_TO_MAC  *DbPtr;
  UINT8                   *TmpPtr;

  CpbPtr  = (PXE_CPB_MCAST_IP_TO_MAC *) (UINTN) CdbPtr->CPBaddr;
  DbPtr   = (PXE_DB_MCAST_IP_TO_MAC *) (UINTN) CdbPtr->DBaddr;

  if ((CdbPtr->OpFlags & PXE_OPFLAGS_MCAST_IPV6_TO_MAC) != 0) {
    //
    // for now this is not supported
    //
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_UNSUPPORTED;
    return ;
  }

  TmpPtr = (UINT8 *) (&CpbPtr->IP.IPv4);
  //
  // check if the ip given is a mcast IP
  //
  if ((TmpPtr[0] & 0xF0) != 0xE0) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CPB;
  }
  //
  // take the last 23 bits in IP.
  // be very careful. accessing word on a non-word boundary will hang motherboard codenamed Big Sur
  // casting the mac array (in the middle) to a UINT32 pointer and accessing
  // the UINT32 content hung the system...
  //
  DbPtr->MAC[0] = 0x01;
  DbPtr->MAC[1] = 0x00;
  DbPtr->MAC[2] = 0x5e;
  DbPtr->MAC[3] = (UINT8) (TmpPtr[1] & 0x7f);
  DbPtr->MAC[4] = (UINT8) TmpPtr[2];
  DbPtr->MAC[5] = (UINT8) TmpPtr[3];

  return ;
}


/**
  This routine is used to read and write non-volatile storage on the NIC (if supported).  The NVRAM
  could be EEPROM, FLASH, or battery backed RAM.
  This is an optional function according to the UNDI specification  (or will be......)

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_NVData (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_DB_NVDATA *DbPtr;
  UINT16        Index;

  if ((CdbPtr->OpFlags == PXE_OPFLAGS_NVDATA_READ) != 0) {

    if ((CdbPtr->DBsize == PXE_DBSIZE_NOT_USED) != 0) {
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
      return ;
    }

    DbPtr = (PXE_DB_NVDATA *) (UINTN) CdbPtr->DBaddr;

    for (Index = 0; Index < MAX_PCI_CONFIG_LEN; Index++) {
      DbPtr->Data.Dword[Index] = AdapterInfo->NVData[Index];

    }

  } else {
    //
    // no write for now
    //
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_UNSUPPORTED;
  }

  return ;
}


/**
  This routine returns the current interrupt status and/or the transmitted buffer addresses.
  If the current interrupt status is returned, pending interrupts will be acknowledged by this
  command.  Transmitted buffer addresses that are written to the DB are removed from the transmit
  buffer queue.
  Normally, this command would be polled with interrupts disabled.
  The transmit buffers are returned in CdbPtr->DBaddr->TxBufer[0 - NumEntries].
  The interrupt status is returned in CdbPtr->StatFlags.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Status (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_DB_GET_STATUS *DbPtr;
  PXE_DB_GET_STATUS TmpGetStatus;
  UINT16            Index;
  UINT16            Status;
  UINT16            NumEntries;
  RxFD              *RxPtr;

  //
  // Fill in temporary GetStatus storage.
  //
  RxPtr = &AdapterInfo->rx_ring[AdapterInfo->cur_rx_ind];

  if ((RxPtr->cb_header.status & RX_COMPLETE) != 0) {
    TmpGetStatus.RxFrameLen = RxPtr->ActualCount & 0x3fff;
  } else {
    TmpGetStatus.RxFrameLen = 0;
  }

  TmpGetStatus.reserved = 0;

  //
  // Fill in size of next available receive packet and
  // reserved field in caller's DB storage.
  //
  DbPtr = (PXE_DB_GET_STATUS *) (UINTN) CdbPtr->DBaddr;

  if (CdbPtr->DBsize > 0 && CdbPtr->DBsize < sizeof (UINT32) * 2) {
    CopyMem (DbPtr, &TmpGetStatus, CdbPtr->DBsize);
  } else {
    CopyMem (DbPtr, &TmpGetStatus, sizeof (UINT32) * 2);
  }

  //
  //
  //
  if ((CdbPtr->OpFlags & PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS) != 0) {
    //
    // DBsize of zero is invalid if Tx buffers are requested.
    //
    if (CdbPtr->DBsize == 0) {
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
      return ;
    }

    //
    // remember this b4 we overwrite
    //
    NumEntries = (UINT16) (CdbPtr->DBsize - sizeof (UINT64));

    //
    // We already filled in 2 UINT32s.
    //
    CdbPtr->DBsize = (UINT16) (sizeof (UINT32) * 2);

    //
    // will claim any hanging free CBs
    //
    CheckCBList (AdapterInfo);

    if (AdapterInfo->xmit_done_head == AdapterInfo->xmit_done_tail) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_TXBUF_QUEUE_EMPTY;
    } else {
      for (Index = 0; ((Index < MAX_XMIT_BUFFERS) && (NumEntries >= sizeof (UINT64))); Index++, NumEntries -= sizeof (UINT64)) {
        if (AdapterInfo->xmit_done_head != AdapterInfo->xmit_done_tail) {
          DbPtr->TxBuffer[Index]      = AdapterInfo->xmit_done[AdapterInfo->xmit_done_head];
          AdapterInfo->xmit_done_head = next (AdapterInfo->xmit_done_head);
          CdbPtr->DBsize += sizeof (UINT64);
        } else {
          break;
        }
      }
    }

    if (AdapterInfo->xmit_done_head != AdapterInfo->xmit_done_tail) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_DB_WRITE_TRUNCATED;

    }
    //
    // check for a receive buffer and give it's size in db
    //
  }
  //
  //
  //
  if ((CdbPtr->OpFlags & PXE_OPFLAGS_GET_INTERRUPT_STATUS) != 0) {

    Status = InWord (AdapterInfo, AdapterInfo->ioaddr + SCBStatus);
    AdapterInfo->Int_Status = (UINT16) (AdapterInfo->Int_Status | Status);

    //
    // acknoledge the interrupts
    //
    OutWord (AdapterInfo, (UINT16) (Status & 0xfc00), (UINT32) (AdapterInfo->ioaddr + SCBStatus));

    //
    // report all the outstanding interrupts
    //
    Status = AdapterInfo->Int_Status;
    if ((Status & SCB_STATUS_FR) != 0) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_RECEIVE;
    }

    if ((Status & SCB_STATUS_SWI) != 0) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_SOFTWARE;
    }
  }

  //
  // Return current media status
  //
  if ((CdbPtr->OpFlags & PXE_OPFLAGS_GET_MEDIA_STATUS) != 0) {
    AdapterInfo->PhyAddress = 0xFF;
    AdapterInfo->CableDetect = 1;

    if (!PhyDetect (AdapterInfo)) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_NO_MEDIA;
    }
  }

  return ;
}


/**
  This routine is used to fill media header(s) in transmit packet(s).
  Copies the MAC address into the media header whether it is dealing
  with fragmented or non-fragmented packets.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_FillHeader (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{
  PXE_CPB_FILL_HEADER             *Cpb;
  PXE_CPB_FILL_HEADER_FRAGMENTED  *Cpbf;
  EtherHeader                     *MacHeader;
  UINTN                           Index;

  if (CdbPtr->CPBsize == PXE_CPBSIZE_NOT_USED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  if ((CdbPtr->OpFlags & PXE_OPFLAGS_FILL_HEADER_FRAGMENTED) != 0) {
    Cpbf = (PXE_CPB_FILL_HEADER_FRAGMENTED *) (UINTN) CdbPtr->CPBaddr;

    //
    // assume 1st fragment is big enough for the mac header
    //
    if ((Cpbf->FragCnt == 0) || (Cpbf->FragDesc[0].FragLen < PXE_MAC_HEADER_LEN_ETHER)) {
      //
      // no buffers given
      //
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
      return ;
    }

    MacHeader = (EtherHeader *) (UINTN) Cpbf->FragDesc[0].FragAddr;
    //
    // we don't swap the protocol bytes
    //
    MacHeader->type = Cpbf->Protocol;

    for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
      MacHeader->dest_addr[Index] = Cpbf->DestAddr[Index];
      MacHeader->src_addr[Index]  = Cpbf->SrcAddr[Index];
    }
  } else {
    Cpb       = (PXE_CPB_FILL_HEADER *) (UINTN) CdbPtr->CPBaddr;

    MacHeader = (EtherHeader *) (UINTN) Cpb->MediaHeader;
    //
    // we don't swap the protocol bytes
    //
    MacHeader->type = Cpb->Protocol;

    for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
      MacHeader->dest_addr[Index] = Cpb->DestAddr[Index];
      MacHeader->src_addr[Index]  = Cpb->SrcAddr[Index];
    }
  }

  return ;
}


/**
  This routine is used to place a packet into the transmit queue.  The data buffers given to
  this command are to be considered locked and the application or network driver loses
  ownership of these buffers and must not free or relocate them until the ownership returns.
  When the packets are transmitted, a transmit complete interrupt is generated (if interrupts
  are disabled, the transmit interrupt status is still set and can be checked using the UNDI_Status
  command.
  Some implementations and adapters support transmitting multiple packets with one transmit
  command.  If this feature is supported, the transmit CPBs can be linked in one transmit
  command.
  All UNDIs support fragmented frames, now all network devices or protocols do.  If a fragmented
  frame CPB is given to UNDI and the network device does not support fragmented frames
  (see !PXE.Implementation flag), the UNDI will have to copy the fragments into a local buffer
  before transmitting.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Transmit (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{

  if (CdbPtr->CPBsize == PXE_CPBSIZE_NOT_USED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  CdbPtr->StatCode = (PXE_STATCODE) E100bTransmit (AdapterInfo, CdbPtr->CPBaddr, CdbPtr->OpFlags);

  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  }

  return ;
}


/**
  When the network adapter has received a frame, this command is used to copy the frame
  into the driver/application storage location.  Once a frame has been copied, it is
  removed from the receive queue.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
VOID
UNDI_Receive (
  IN  PXE_CDB           *CdbPtr,
  IN  NIC_DATA_INSTANCE *AdapterInfo
  )
{

  //
  // check if RU has started...
  //
  if (!AdapterInfo->Receive_Started) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_NOT_INITIALIZED;
    return ;
  }


  CdbPtr->StatCode  = (UINT16) E100bReceive (AdapterInfo, CdbPtr->CPBaddr, CdbPtr->DBaddr);
  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;

  }

  return ;
}



/**
  This is the main SW UNDI API entry using the newer nii protocol.
  The parameter passed in is a 64 bit flat model virtual
  address of the cdb.  We then jump into the common routine for both old and
  new nii protocol entries.

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
// TODO:    cdb - add argument and description to function comment
VOID
UNDI_APIEntry_new (
  IN  UINT64 cdb
  )
{
  PXE_CDB           *CdbPtr;
  NIC_DATA_INSTANCE *AdapterInfo;

  if (cdb == (UINT64) 0) {
    return ;

  }

  CdbPtr = (PXE_CDB *) (UINTN) cdb;

  if (CdbPtr->IFnum >= (pxe_31->IFcnt | pxe_31->IFcntExt << 8) ) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return ;
  }

  AdapterInfo               = &(UNDI32DeviceList[CdbPtr->IFnum]->NicInfo);
  //
  // entering from older entry point
  //
  AdapterInfo->VersionFlag  = 0x31;
  UNDI_APIEntry_Common (cdb);
}


/**
  This is the common routine for both old and new entry point procedures.
  The parameter passed in is a 64 bit flat model virtual
  address of the cdb.  We then jump into the service routine pointed to by the
  Api_Table[OpCode].

  @param  CdbPtr               Pointer to the command descriptor block.
  @param  AdapterInfo          Pointer to the NIC data structure information which
                               the UNDI driver is layering on..

  @return None

**/
// TODO:    cdb - add argument and description to function comment
VOID
UNDI_APIEntry_Common (
  IN  UINT64 cdb
  )
{
  PXE_CDB           *CdbPtr;
  NIC_DATA_INSTANCE *AdapterInfo;
  UNDI_CALL_TABLE   *tab_ptr;

  CdbPtr = (PXE_CDB *) (UINTN) cdb;

  //
  // check the OPCODE range
  //
  if ((CdbPtr->OpCode > PXE_OPCODE_LAST_VALID) ||
      (CdbPtr->StatCode != PXE_STATCODE_INITIALIZE) ||
      (CdbPtr->StatFlags != PXE_STATFLAGS_INITIALIZE) ||
      (CdbPtr->IFnum >= (pxe_31->IFcnt |  pxe_31->IFcntExt << 8))) {
    goto badcdb;

  }

  if (CdbPtr->CPBsize == PXE_CPBSIZE_NOT_USED) {
    if (CdbPtr->CPBaddr != PXE_CPBADDR_NOT_USED) {
      goto badcdb;
    }
  } else if (CdbPtr->CPBaddr == PXE_CPBADDR_NOT_USED) {
    goto badcdb;
  }

  if (CdbPtr->DBsize == PXE_DBSIZE_NOT_USED) {
    if (CdbPtr->DBaddr != PXE_DBADDR_NOT_USED) {
      goto badcdb;
    }
  } else if (CdbPtr->DBaddr == PXE_DBADDR_NOT_USED) {
    goto badcdb;
  }

  //
  // check if cpbsize and dbsize are as needed
  // check if opflags are as expected
  //
  tab_ptr = &api_table[CdbPtr->OpCode];

  if (tab_ptr->cpbsize != (UINT16) (DONT_CHECK) && tab_ptr->cpbsize != CdbPtr->CPBsize) {
    goto badcdb;
  }

  if (tab_ptr->dbsize != (UINT16) (DONT_CHECK) && tab_ptr->dbsize != CdbPtr->DBsize) {
    goto badcdb;
  }

  if (tab_ptr->opflags != (UINT16) (DONT_CHECK) && tab_ptr->opflags != CdbPtr->OpFlags) {
    goto badcdb;

  }

  AdapterInfo = &(UNDI32DeviceList[CdbPtr->IFnum]->NicInfo);

  //
  // check if UNDI_State is valid for this call
  //
  if (tab_ptr->state != (UINT16) (-1)) {
    //
    // should atleast be started
    //
    if (AdapterInfo->State == PXE_STATFLAGS_GET_STATE_STOPPED) {
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode  = PXE_STATCODE_NOT_STARTED;
      return ;
    }
    //
    // check if it should be initialized
    //
    if (tab_ptr->state == 2) {
      if (AdapterInfo->State != PXE_STATFLAGS_GET_STATE_INITIALIZED) {
        CdbPtr->StatCode  = PXE_STATCODE_NOT_INITIALIZED;
        CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
        return ;
      }
    }
  }
  //
  // set the return variable for success case here
  //
  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode  = PXE_STATCODE_SUCCESS;

  tab_ptr->api_ptr (CdbPtr, AdapterInfo);
  return ;
  //
  // %% AVL - check for command linking
  //
badcdb:
  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
  return ;
}


/**
  When called with a null NicPtr, this routine decrements the number of NICs
  this UNDI is supporting and removes the NIC_DATA_POINTER from the array.
  Otherwise, it increments the number of NICs this UNDI is supported and
  updates the pxe.Fudge to ensure a proper check sum results.

  @param  NicPtr               Pointer to the NIC data structure.

  @return None

**/
VOID
PxeUpdate (
  IN  NIC_DATA_INSTANCE *NicPtr,
  IN PXE_SW_UNDI        *PxePtr
  )
{
  UINT16 NicNum;
  NicNum = (PxePtr->IFcnt | PxePtr->IFcntExt << 8);
  
  if (NicPtr == NULL) {
    if (NicNum > 0) {
      //
      // number of NICs this undi supports
      //
      NicNum --;
    }
    goto done;
  }

  //
  // number of NICs this undi supports
  //
  NicNum++;
  
done: 
  PxePtr->IFcnt = (UINT8)(NicNum & 0xFF);
  PxePtr->IFcntExt = (UINT8) ((NicNum & 0xFF00) >> 8);
  PxePtr->Fudge = (UINT8) (PxePtr->Fudge - CalculateSum8 ((VOID *) PxePtr, PxePtr->Len));
  return ;
}


/**
  Initialize the !PXE structure

  @param  PxePtr               Pointer to SW_UNDI data structure.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval other                This driver does not support this device.

**/
VOID
PxeStructInit (
  IN PXE_SW_UNDI *PxePtr
  )
{
  //
  // Initialize the !PXE structure
  //
  PxePtr->Signature = PXE_ROMID_SIGNATURE;
  PxePtr->Len       = (UINT8) sizeof (PXE_SW_UNDI);
  //
  // cksum
  //
  PxePtr->Fudge     = 0;
  //
  // number of NICs this undi supports
  //
  PxePtr->IFcnt = 0;
  PxePtr->IFcntExt = 0;
  PxePtr->Rev       = PXE_ROMID_REV;
  PxePtr->MajorVer  = PXE_ROMID_MAJORVER;
  PxePtr->MinorVer  = PXE_ROMID_MINORVER;
  PxePtr->reserved1 = 0;

  PxePtr->Implementation = PXE_ROMID_IMP_SW_VIRT_ADDR |
    PXE_ROMID_IMP_FRAG_SUPPORTED |
    PXE_ROMID_IMP_CMD_LINK_SUPPORTED |
    PXE_ROMID_IMP_NVDATA_READ_ONLY |
    PXE_ROMID_IMP_STATION_ADDR_SETTABLE |
    PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED |
    PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED |
    PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED |
    PXE_ROMID_IMP_FILTERED_MULTICAST_RX_SUPPORTED |
    PXE_ROMID_IMP_SOFTWARE_INT_SUPPORTED |
    PXE_ROMID_IMP_PACKET_RX_INT_SUPPORTED;

  PxePtr->EntryPoint  = (UINT64) (UINTN) UNDI_APIEntry_new;
  PxePtr->MinorVer    = PXE_ROMID_MINORVER_31;

  PxePtr->reserved2[0]  = 0;
  PxePtr->reserved2[1]  = 0;
  PxePtr->reserved2[2]  = 0;
  PxePtr->BusCnt        = 1;
  PxePtr->BusType[0]    = PXE_BUSTYPE_PCI;

  PxePtr->Fudge         = (UINT8) (PxePtr->Fudge - CalculateSum8 ((VOID *) PxePtr, PxePtr->Len));
}


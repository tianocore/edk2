/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  SnpNt32.c

Abstract:

-**/

#include "SnpNt32.h"

EFI_DRIVER_BINDING_PROTOCOL gSnpNt32DriverBinding = {
  SnpNt32DriverBindingSupported,
  SnpNt32DriverBindingStart,
  SnpNt32DriverBindingStop,
  0xa,
  NULL,
  NULL
};

SNPNT32_GLOBAL_DATA         gSnpNt32GlobalData = {
  SNP_NT32_DRIVER_SIGNATURE,  //  Signature
  {
    NULL,
    NULL
  },                          //  InstanceList
  NULL,                       //  WinNtThunk
  NULL,                       //  NetworkLibraryHandle
  {
    0
  },                          //  NtNetUtilityTable
  {
    0,
    0,
    0
  },                          //  Lock
  //
  //  Private functions
  //
  SnpNt32InitializeGlobalData,            //  InitializeGlobalData
  SnpNt32InitializeInstanceData,          //  InitializeInstanceData
  SnpNt32CloseInstance                    //  CloseInstance
};


/**
  Test to see if this driver supports ControllerHandle.

  @param  This                  Protocol instance pointer.
  @param  ControllerHandle      Handle of device to test.
  @param  RemainingDevicePath   Optional parameter use to pick a specific child
                                device to start.

  @retval EFI_SUCCES            This driver supports this device.
  @retval other                 This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SnpNt32DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{

  SNPNT32_GLOBAL_DATA   *GlobalData;
  NET_LIST_ENTRY        *Entry;
  SNPNT32_INSTANCE_DATA *Instance;

  GlobalData = &gSnpNt32GlobalData;

  NET_LIST_FOR_EACH (Entry, &GlobalData->InstanceList) {

    Instance = NET_LIST_USER_STRUCT_S (Entry, SNPNT32_INSTANCE_DATA, Entry, SNP_NT32_INSTANCE_SIGNATURE);

    if (Instance->DeviceHandle == ControllerHandle) {
      return EFI_SUCCESS;
    }

  }

  return EFI_UNSUPPORTED;
}


/**
  Start this driver on ControllerHandle.

  @param  This                  Protocol instance pointer.
  @param  ControllerHandle      Handle of device to bind driver to.
  @param  RemainingDevicePath   Optional parameter use to pick a specific child
                                device to start.

  @retval EFI_SUCCES            This driver is added to ControllerHandle.

**/
EFI_STATUS
EFIAPI
SnpNt32DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  return EFI_SUCCESS;
}


/**
  Stop this driver on ControllerHandle.

  @param  This                  Protocol instance pointer.
  @param  ControllerHandle      Handle of device to stop driver on.
  @param  NumberOfChildren      Number of Handles in ChildHandleBuffer. If number
                                of children is zero stop the entire bus driver.
  @param  ChildHandleBuffer     List of Child Handles to Stop.

  @retval EFI_SUCCES            This driver is removed ControllerHandle.

**/
EFI_STATUS
EFIAPI
SnpNt32DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{

  return EFI_SUCCESS;
}


/**
  Start the SnpNt32 interface.

  @param  This                  Context pointer.

  @retval EFI_SUCCESS           The interface is started.

**/
EFI_STATUS
SnpNt32Start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}


/**
  Stop the SnpNt32 interface.

  @param  This                  Context pointer.

  @retval EFI_SUCCESS           The interface is stopped.

**/
EFI_STATUS
SnpNt32Stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}


/**
  Initialize the SnpNt32 interface.

  @param  This                  Context pointer.
  @param  ExtraRxBufferSize     Number of extra receive buffer.
  @param  ExtraTxBufferSize     Number of extra transmit buffer.

  @retval EFI_SUCCESS           The interface is initialized.

**/
EFI_STATUS
SnpNt32Initialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       ExtraRxBufferSize OPTIONAL,
  IN UINTN                       ExtraTxBufferSize OPTIONAL
  )
{
  return EFI_SUCCESS;
}


/**
  Reset the snpnt32 interface.

  @param  This                  Context pointer.
  @param  ExtendedVerification  Not implemented.

  @retval EFI_SUCCESS           The interface is reseted.

**/
EFI_STATUS
SnpNt32Reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ExtendedVerification
  )
{
  return EFI_SUCCESS;
}


/**
  Shut down the snpnt32 interface.

  @param  This                  Context pointer.

  @retval EFI_SUCCESS           The interface is shut down.

**/
EFI_STATUS
SnpNt32Shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}


/**
  Change the interface's receive filter setting.

  @param  This                  Context pointer.
  @param  EnableBits            The receive filters to enable.
  @param  DisableBits           The receive filters to disable
  @param  ResetMcastFilter      Reset the multicast filters or not.
  @param  McastFilterCount      The count of multicast filter to set.
  @param  McastFilter           Pointer to the arrya of multicast addresses to set.

  @retval EFI_SUCCESS           The receive filter is updated.
  @retval EFI_ACCESS_DENIED     The snpnt32 lock is already owned by another
                                routine.
  @retval EFI_DEVICE_ERROR      Failed to update the receive filter.

**/
EFI_STATUS
SnpNt32ReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINT32                      EnableBits,
  IN UINT32                      DisableBits,
  IN BOOLEAN                     ResetMcastFilter,
  IN UINTN                       McastFilterCount OPTIONAL,
  IN EFI_MAC_ADDRESS             *McastFilter OPTIONAL
  )
{
  SNPNT32_INSTANCE_DATA *Instance;
  SNPNT32_GLOBAL_DATA   *GlobalData;
  INT32                 ReturnValue;

  Instance    = SNP_NT32_INSTANCE_DATA_FROM_SNP_THIS (This);

  GlobalData  = Instance->GlobalData;

  if (EFI_ERROR (NET_TRYLOCK (&GlobalData->Lock))) {
    return EFI_ACCESS_DENIED;
  }

  ReturnValue = GlobalData->NtNetUtilityTable.SetReceiveFilter (
                                                Instance->InterfaceInfo.InterfaceIndex,
                                                EnableBits,
                                                McastFilterCount,
                                                McastFilter
                                                );

  NET_UNLOCK (&GlobalData->Lock);

  if (ReturnValue <= 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Change or reset the mac address of the interface.

  @param  This                  Context pointer.
  @param  reset                 Reset the mac address to the original one or not.
  @param  NewMacAddr            Pointer to the new mac address to set.

  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
SnpNt32StationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *NewMacAddr OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Get or reset the statistics data.

  @param  This                  Context pointer.
  @param  Reset                 Reset the statistics or not.
  @param  StatisticsSize        The size of the buffer used to receive the
                                statistics data.
  @param  StatisticsTable       Pointer to the table used to receive the statistics
                                data.

  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
SnpNt32Statistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * This,
  IN BOOLEAN                      Reset,
  IN OUT UINTN                    *StatisticsSize OPTIONAL,
  IN OUT EFI_NETWORK_STATISTICS   *StatisticsTable OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Convert a multicast ip address to the multicast mac address.

  @param  This                  Context pointer.
  @param  Ipv6                  The Ip is an Ipv6 address or not.
  @param  Ip                    Pointer to the Ip address to convert.
  @param  Mac                   Pointer to the buffer used to hold the converted
                                mac address.

  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
SnpNt32McastIptoMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Ipv6,
  IN EFI_IP_ADDRESS              *Ip,
  OUT EFI_MAC_ADDRESS            *Mac
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Read or write the nv data.

  @param  This                  Context pinter.
  @param  ReadOrWrite           Read or write the nv data.
  @param  Offset                The offset to the start of the nv data.
  @param  BufferSize            Size of the buffer.
  @param  Buffer                Pointer to the buffer containing the data to write
                                or used to receive the data read.

  @retval EFI_UNSUPPORTED       Not supported yet.

**/
EFI_STATUS
SnpNt32Nvdata (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ReadOrWrite,
  IN UINTN                       Offset,
  IN UINTN                       BufferSize,
  IN OUT VOID                    *Buffer
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Get the status information of the interface.

  @param  This                  Context pointer.
  @param  InterruptStatus       The storage to hold the interrupt status.
  @param  TxBuffer              Pointer to get the list of pointers of previously
                                transmitted buffers whose transmission was
                                completed asynchrnously.

  @retval EFI_SUCCESS           The status is got.

**/
EFI_STATUS
SnpNt32GetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINT32                     *InterruptStatus,
  OUT VOID                       **TxBuffer
  )
{

  if (TxBuffer != NULL) {
    *((UINT8 **) TxBuffer) = (UINT8 *) 1;
  }

  if (InterruptStatus != NULL) {
    *InterruptStatus = EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
  }

  return EFI_SUCCESS;
}


/**
  Transmit a packet.

  @param  This                  Context pointer.
  @param  HeaderSize            The media header size contained in the packet
                                buffer.
  @param  BufferSize            The size of the packet buffer.
  @param  Buffer                Pointer to the buffer containing the packet data.
  @param  SrcAddr               If non null, points to the source address of this
                                packet.
  @param  DestAddr              If non null, points to the destination address of
                                this packet.
  @param  Protocol              The protocol type of this packet.

  @retval EFI_SUCCESS           The packet is transmitted or put into the transmit
                                queue.
  @retval other                 Some error occurs.

**/
EFI_STATUS
SnpNt32Transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       HeaderSize,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer,
  IN EFI_MAC_ADDRESS             *SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS             *DestAddr OPTIONAL,
  IN UINT16                      *Protocol OPTIONAL
  )
{
  SNPNT32_INSTANCE_DATA *Instance;
  SNPNT32_GLOBAL_DATA   *GlobalData;
  INT32                 ReturnValue;

  Instance    = SNP_NT32_INSTANCE_DATA_FROM_SNP_THIS (This);

  GlobalData  = Instance->GlobalData;

  if ((HeaderSize != 0) && (SrcAddr == NULL)) {
    SrcAddr = &Instance->Mode.CurrentAddress;
  }

  if (EFI_ERROR (NET_TRYLOCK (&GlobalData->Lock))) {
    return EFI_ACCESS_DENIED;
  }

  ReturnValue = GlobalData->NtNetUtilityTable.Transmit (
                                                Instance->InterfaceInfo.InterfaceIndex,
                                                HeaderSize,
                                                BufferSize,
                                                Buffer,
                                                SrcAddr,
                                                DestAddr,
                                                Protocol
                                                );

  NET_UNLOCK (&GlobalData->Lock);

  if (ReturnValue < 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Receive network data.

  @param  This                  Context pointer.
  @param  HeaderSize            Optional parameter and is a pointer to the header
                                portion of the data received.
  @param  BuffSize              Pointer to the length of the Buffer on entry and
                                contains the length of the received data on return
  @param  Buffer                Pointer to the memory for the received data
  @param  SourceAddr            Optional parameter, is a pointer to contain the
                                source ethernet address on return
  @param  DestinationAddr       Optional parameter, is a pointer to contain the
                                destination ethernet address on return.
  @param  Protocol              Optional parameter, is a pointer to contain the
                                Protocol type from the ethernet header on return.

  @retval EFI_SUCCESS           A packet is received and put into the buffer.
  @retval EFI_BUFFER_TOO_SMALL  The provided buffer is too small to receive the
                                packet.
  @retval EFI_NOT_READY         There is no packet received.

**/
EFI_STATUS
SnpNt32Receive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINTN                      *HeaderSize,
  IN OUT UINTN                   *BuffSize,
  OUT VOID                       *Buffer,
  OUT EFI_MAC_ADDRESS            *SourceAddr,
  OUT EFI_MAC_ADDRESS            *DestinationAddr,
  OUT UINT16                     *Protocol
  )
{
  SNPNT32_INSTANCE_DATA *Instance;
  SNPNT32_GLOBAL_DATA   *GlobalData;
  INT32                 ReturnValue;

  Instance    = SNP_NT32_INSTANCE_DATA_FROM_SNP_THIS (This);

  GlobalData  = Instance->GlobalData;

  ASSERT (GlobalData->NtNetUtilityTable.Receive != NULL);

  if (EFI_ERROR (NET_TRYLOCK (&GlobalData->Lock))) {
    return EFI_ACCESS_DENIED;
  }

  ReturnValue = GlobalData->NtNetUtilityTable.Receive (
                                                Instance->InterfaceInfo.InterfaceIndex,
                                                BuffSize,
                                                Buffer
                                                );

  NET_UNLOCK (&GlobalData->Lock);

  if (ReturnValue < 0) {
    if (ReturnValue == -100) {
      return EFI_BUFFER_TOO_SMALL;
    }

    return EFI_DEVICE_ERROR;
  } else if (ReturnValue == 0) {
    return EFI_NOT_READY;
  }

  if (HeaderSize != NULL) {
    *HeaderSize = 14;
  }

  if (SourceAddr != NULL) {
    NetZeroMem (SourceAddr, sizeof (EFI_MAC_ADDRESS));
    NetCopyMem (SourceAddr, ((UINT8 *) Buffer) + 6, 6);
  }

  if (DestinationAddr != NULL) {
    NetZeroMem (DestinationAddr, sizeof (EFI_MAC_ADDRESS));
    NetCopyMem (DestinationAddr, ((UINT8 *) Buffer), 6);
  }

  if (Protocol != NULL) {
    *Protocol = NTOHS (*((UINT16 *) (((UINT8 *) Buffer) + 12)));
  }

  return EFI_SUCCESS;
}

SNPNT32_INSTANCE_DATA gSnpNt32InstanceTemplate = {
  SNP_NT32_INSTANCE_SIGNATURE,            //  Signature
  {
    NULL,
    NULL
  },                                      //  Entry
  NULL,                                   //  GlobalData
  NULL,                                   //  DeviceHandle
  NULL,                                   //  DevicePath
  {                                       //  Snp
    EFI_SIMPLE_NETWORK_PROTOCOL_REVISION, //  Revision
    SnpNt32Start,                         //  Start
    SnpNt32Stop,                          //  Stop
    SnpNt32Initialize,                    //  Initialize
    SnpNt32Reset,                         //  Reset
    SnpNt32Shutdown,                      //  Shutdown
    SnpNt32ReceiveFilters,                //  ReceiveFilters
    SnpNt32StationAddress,                //  StationAddress
    SnpNt32Statistics,                    //  Statistics
    SnpNt32McastIptoMac,                  //  MCastIpToMac
    SnpNt32Nvdata,                        //  NvData
    SnpNt32GetStatus,                     //  GetStatus
    SnpNt32Transmit,                      //  Transmit
    SnpNt32Receive,                       //  Receive
    NULL,                                 //  WaitForPacket
    NULL                                  //  Mode
  },
  {                                       //  Mode
    EfiSimpleNetworkInitialized,          //  State
    NET_ETHER_ADDR_LEN,                   //  HwAddressSize
    NET_ETHER_HEADER_SIZE,                //  MediaHeaderSize
    1500,                                 //  MaxPacketSize
    0,                                    //  NvRamSize
    0,                                    //  NvRamAccessSize
    0,                                    //  ReceiveFilterMask
    0,                                    //  ReceiveFilterSetting
    MAX_MCAST_FILTER_CNT,                 //  MaxMCastFilterCount
    0,                                    //  MCastFilterCount
    {
      0
    },                                    //  MCastFilter
    {
      0
    },                                    //  CurrentAddress
    {
      0
    },                                    //  BroadcastAddress
    {
      0
    },                                    //  PermanentAddress
    NET_IFTYPE_ETHERNET,                  //  IfType
    FALSE,                                //  MacAddressChangeable
    FALSE,                                //  MultipleTxSupported
    FALSE,                                //  MediaPresentSupported
    TRUE                                  //  MediaPresent
  },
  {
    0
  }                                       //  InterfaceInfo
};


/**
  Initialize the driver's global data.

  @param  This                  Pointer to the global context data.

  @retval EFI_SUCCESS           The global data is initialized.
  @retval EFI_NOT_FOUND         The required DLL is not found.

**/
EFI_STATUS
SnpNt32InitializeGlobalData (
  IN SNPNT32_GLOBAL_DATA *This
  )
{
  EFI_STATUS            Status;
  CHAR16                *DllFileNameU;
  UINT32                Index;
  INT32                 ReturnValue;
  BOOLEAN               NetUtilityLibInitDone;
  NT_NET_INTERFACE_INFO NetInterfaceInfoBuffer[MAX_INTERFACE_INFO_NUMBER];
  SNPNT32_INSTANCE_DATA *Instance;
  NET_LIST_ENTRY        *Entry;
  UINT32                InterfaceCount;

  ASSERT (This != NULL);

  NetUtilityLibInitDone = FALSE;
  InterfaceCount        = MAX_INTERFACE_INFO_NUMBER;

  NetListInit (&This->InstanceList);
  NET_LOCK_INIT (&This->Lock);

  //
  //  Get the WinNT thunk
  //
  Status = gBS->LocateProtocol (&gEfiWinNtThunkProtocolGuid, NULL, &This->WinNtThunk);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (This->WinNtThunk != NULL);

  DllFileNameU = NETWORK_LIBRARY_NAME_U;

  //
  //  Load network utility library
  //
  This->NetworkLibraryHandle = This->WinNtThunk->LoadLibraryEx (DllFileNameU, NULL, 0);

  if (NULL == This->NetworkLibraryHandle) {
    return EFI_NOT_FOUND;
  }

  This->NtNetUtilityTable.Initialize = (NT_NET_INITIALIZE) This->WinNtThunk->GetProcAddress (
                                                                              This->NetworkLibraryHandle,
                                                                              NETWORK_LIBRARY_INITIALIZE
                                                                              );

  if (NULL == This->NtNetUtilityTable.Initialize) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.Finalize = (NT_NET_FINALIZE) This->WinNtThunk->GetProcAddress (
                                                                          This->NetworkLibraryHandle,
                                                                          NETWORK_LIBRARY_FINALIZE
                                                                          );

  if (NULL == This->NtNetUtilityTable.Finalize) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.SetReceiveFilter = (NT_NET_SET_RECEIVE_FILTER) This->WinNtThunk->GetProcAddress (
                                                                                            This->NetworkLibraryHandle,
                                                                                            NETWORK_LIBRARY_SET_RCV_FILTER
                                                                                            );

  if (NULL == This->NtNetUtilityTable.SetReceiveFilter) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.Receive = (NT_NET_RECEIVE) This->WinNtThunk->GetProcAddress (
                                                                        This->NetworkLibraryHandle,
                                                                        NETWORK_LIBRARY_RECEIVE
                                                                        );

  if (NULL == This->NtNetUtilityTable.Receive) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  This->NtNetUtilityTable.Transmit = (NT_NET_TRANSMIT) This->WinNtThunk->GetProcAddress (
                                                                          This->NetworkLibraryHandle,
                                                                          NETWORK_LIBRARY_TRANSMIT
                                                                          );

  if (NULL == This->NtNetUtilityTable.Transmit) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }
  //
  //  Initialize the network utility library
  //  And enumerate the interfaces in NT32 host
  //
  ReturnValue = This->NtNetUtilityTable.Initialize (&InterfaceCount, &NetInterfaceInfoBuffer[0]);
  if (ReturnValue <= 0) {
    Status = EFI_DEVICE_ERROR;
    goto ErrorReturn;
  }

  NetUtilityLibInitDone = TRUE;

  if (InterfaceCount == 0) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }
  //
  //  Create fake SNP instances
  //
  for (Index = 0; Index < InterfaceCount; Index++) {

    Instance = NetAllocatePool (sizeof (SNPNT32_INSTANCE_DATA));

    if (NULL == Instance) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorReturn;
    }
    //
    //  Copy the content from a template
    //
    NetCopyMem (Instance, &gSnpNt32InstanceTemplate, sizeof (SNPNT32_INSTANCE_DATA));

    //
    //  Set the interface information.
    //
    Instance->InterfaceInfo = NetInterfaceInfoBuffer[Index];
    //
    //  Initialize this instance
    //
    Status = This->InitializeInstanceData (This, Instance);
    if (EFI_ERROR (Status)) {

      NetFreePool (Instance);
      goto ErrorReturn;
    }
    //
    //  Insert this instance into the instance list
    //
    NetListInsertTail (&This->InstanceList, &Instance->Entry);
  }

  return EFI_SUCCESS;

ErrorReturn:

  while (!NetListIsEmpty (&This->InstanceList)) {

    Entry     = This->InstanceList.ForwardLink;

    Instance  = NET_LIST_USER_STRUCT_S (Entry, SNPNT32_INSTANCE_DATA, Entry, SNP_NT32_INSTANCE_SIGNATURE);

    NetListRemoveEntry (Entry);

    This->CloseInstance (This, Instance);
    NetFreePool (Instance);
  }

  if (NetUtilityLibInitDone) {

    ASSERT (This->WinNtThunk != NULL);

    if (This->NtNetUtilityTable.Finalize != NULL) {
      This->NtNetUtilityTable.Finalize ();
      This->NtNetUtilityTable.Finalize = NULL;
    }
  }

  return Status;
}


/**
  Initialize the snpnt32 driver instance.

  @param  This                  Pointer to the SnpNt32 global data.
  @param  Instance              Pointer to the instance context data.

  @retval EFI_SUCCESS           The driver instance is initialized.

**/
EFI_STATUS
SnpNt32InitializeInstanceData (
  IN SNPNT32_GLOBAL_DATA    *This,
  IN SNPNT32_INSTANCE_DATA  *Instance
  )
{
  EFI_STATUS    Status;
  EFI_DEV_PATH  EndNode;
  EFI_DEV_PATH  Node;

  Instance->GlobalData  = This;
  Instance->Snp.Mode    = &Instance->Mode;
  //
  //  Set broadcast address
  //
  SetMem (&Instance->Mode.BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xFF);

  //
  //  Copy Current/PermanentAddress MAC address
  //
  Instance->Mode.CurrentAddress   = Instance->InterfaceInfo.MacAddr;
  Instance->Mode.PermanentAddress = Instance->InterfaceInfo.MacAddr;

  //
  //  Since the fake SNP is based on a real NIC, to avoid conflict with the host
  //  NIC network stack, we use a different MAC address.
  //  So just change the last byte of the MAC address for the real NIC.
  //
  Instance->Mode.CurrentAddress.Addr[NET_ETHER_ADDR_LEN - 1]++;

  //
  //  Create a fake device path for the instance
  //
  NetZeroMem (&Node, sizeof (Node));

  Node.DevPath.Type     = MESSAGING_DEVICE_PATH;
  Node.DevPath.SubType  = MSG_MAC_ADDR_DP;
  SetDevicePathNodeLength (&Node.DevPath, sizeof (MAC_ADDR_DEVICE_PATH));

  NetCopyMem (
    &Node.MacAddr.MacAddress,
    &Instance->Mode.CurrentAddress,
    sizeof (EFI_MAC_ADDRESS)
    );

  Node.MacAddr.IfType = Instance->Mode.IfType;

  SetDevicePathEndNode (&EndNode.DevPath);

  Instance->DevicePath = AppendDevicePathNode (
                          &EndNode.DevPath,
                          &Node.DevPath
                          );

  //
  //  Create a fake device handle for the fake SNP
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Instance->DeviceHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  &Instance->Snp,
                  &gEfiDevicePathProtocolGuid,
                  Instance->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorReturn;
  }

  return EFI_SUCCESS;

ErrorReturn:
  return Status;
}


/**
  Close the SnpNt32 driver instance.

  @param  This                  Pointer to the SnpNt32 global data.
  @param  Instance              Pointer to the instance context data.

  @retval EFI_SUCCESS           The instance is closed.

**/
EFI_STATUS
SnpNt32CloseInstance (
  IN SNPNT32_GLOBAL_DATA    *This,
  IN SNPNT32_INSTANCE_DATA  *Instance
  )
{
  ASSERT (This != NULL);
  ASSERT (Instance != NULL);

  gBS->UninstallMultipleProtocolInterfaces (
        Instance->DeviceHandle,
        &gEfiSimpleNetworkProtocolGuid,
        &Instance->Snp,
        &gEfiDevicePathProtocolGuid,
        Instance->DevicePath,
        NULL
        );

  if (Instance->DevicePath != NULL) {
    gBS->FreePool (Instance->DevicePath);
  }

  return EFI_SUCCESS;
}


/**
  Unload the SnpNt32 driver.

  @param  ImageHandle           The handle of the driver image.

  @retval EFI_SUCCESS           The driver is unloaded.
  @retval other                 Some error occurs.

**/
EFI_STATUS
EFIAPI
SnpNt32Unload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS            Status;
  SNPNT32_GLOBAL_DATA   *This;
  NET_LIST_ENTRY        *Entry;
  SNPNT32_INSTANCE_DATA *Instance;

  This    = &gSnpNt32GlobalData;

  Status  = NetLibDefaultUnload (ImageHandle);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (!NetListIsEmpty (&This->InstanceList)) {
    //
    //  Walkthrough the interfaces and remove all the SNP instance
    //
    Entry     = This->InstanceList.ForwardLink;

    Instance  = NET_LIST_USER_STRUCT_S (Entry, SNPNT32_INSTANCE_DATA, Entry, SNP_NT32_INSTANCE_SIGNATURE);

    NetListRemoveEntry (Entry);

    This->CloseInstance (This, Instance);
    NetFreePool (Instance);
  }

  if (This->NtNetUtilityTable.Finalize != NULL) {
    This->NtNetUtilityTable.Finalize ();
  }

  This->WinNtThunk->FreeLibrary (This->NetworkLibraryHandle);

  return EFI_SUCCESS;
}


EFI_STATUS
InitializeSnpNt32river (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Install DriverBinding Protocol for the Win NT Bus driver on the drivers
  image handle.

Arguments:

  ImageHandle - The handle of this image.
  SystemTable - Pointer to the EFI system table.

Returns:

  EFI_SUCEESS -  The protocols are installed and the SnpNt32 is initialized.
  other       -  Some error occurs.

--*/
{

  EFI_STATUS  Status;

  //
  // Install the Driver Protocols
  //

  Status = EfiLibInstallAllDriverProtocols (
            ImageHandle,
            SystemTable,
            &gSnpNt32DriverBinding,
            ImageHandle,
            &gSnpNt32DriverComponentName,
            NULL,
            NULL
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //  Initialize the global data
  //
  Status = SnpNt32InitializeGlobalData (&gSnpNt32GlobalData);
  if (EFI_ERROR (Status)) {
    SnpNt32Unload (ImageHandle);
  }

  return Status;
}

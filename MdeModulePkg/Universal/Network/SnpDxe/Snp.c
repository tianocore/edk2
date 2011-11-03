/** @file
  Implementation of driver entry point and driver binding protocol.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed
and made available under the terms and conditions of the BSD License which
accompanies this distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Snp.h"

//
//  Module global variables needed to support undi 3.0 interface
//
EFI_PCI_IO_PROTOCOL         *mPciIo;
V2P                         *mV2p = NULL; // undi3.0 map_list head
// End Global variables
//

/**
  One notified function to stop UNDI device when gBS->ExitBootServices() called.

  @param  Event                   Pointer to this event
  @param  Context                 Event hanlder private data

**/
VOID
EFIAPI
SnpNotifyExitBootServices (
  EFI_EVENT Event,
  VOID      *Context
  )
{
  SNP_DRIVER             *Snp;

  Snp  = (SNP_DRIVER *)Context;

  //
  // Shutdown and stop UNDI driver
  //
  PxeShutdown (Snp);
  PxeStop (Snp);
}

/**
  Send command to UNDI. It does nothing currently.

  @param Cdb   command to be sent to UNDI.

  @retval EFI_INVALID_PARAMETER  The command is 0.
  @retval EFI_UNSUPPORTED        Default return status because it's not
                                 supported currently.

**/
EFI_STATUS
EFIAPI
IssueHwUndiCommand (
  UINT64 Cdb
  )
{
  DEBUG ((EFI_D_ERROR, "\nIssueHwUndiCommand() - This should not be called!"));

  if (Cdb == 0) {
    return EFI_INVALID_PARAMETER;

  }
  //
  //  %%TBD - For now, nothing is done.
  //
  return EFI_UNSUPPORTED;
}


/**
  Compute 8-bit checksum of a buffer.

  @param  Buffer               Pointer to buffer.
  @param  Length               Length of buffer in bytes.

  @return 8-bit checksum of all bytes in buffer, or zero if ptr is NULL or len
          is zero.

**/
UINT8
Calc8BitCksum (
  VOID  *Buffer,
  UINTN Length
  )
{
  UINT8 *Ptr;
  UINT8 Cksum;

  Ptr   = Buffer;
  Cksum = 0;

  if (Ptr == NULL || Length == 0) {
    return 0;
  }

  while (Length-- != 0) {
    Cksum = (UINT8) (Cksum + *Ptr++);
  }

  return Cksum;
}

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_ALREADY_STARTED This driver is already running on this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SimpleNetworkDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                                Status;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *NiiProtocol;
  PXE_UNDI                                  *Pxe;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  (VOID **) &NiiProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    if (Status == EFI_ALREADY_STARTED) {
      DEBUG ((EFI_D_INFO, "Support(): Already Started. on handle %p\n", Controller));
    }
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Support(): UNDI3.1 found on handle %p\n", Controller));

  //
  // check the version, we don't want to connect to the undi16
  //
  if (NiiProtocol->Type != EfiNetworkInterfaceUndi) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }
  //
  // Check to see if !PXE structure is valid. Paragraph alignment of !PXE structure is required.
  //
  if ((NiiProtocol->Id & 0x0F) != 0) {
    DEBUG ((EFI_D_NET, "\n!PXE structure is not paragraph aligned.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Pxe = (PXE_UNDI *) (UINTN) (NiiProtocol->Id);

  //
  //  Verify !PXE revisions.
  //
  if (Pxe->hw.Signature != PXE_ROMID_SIGNATURE) {
    DEBUG ((EFI_D_NET, "\n!PXE signature is not valid.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (Pxe->hw.Rev < PXE_ROMID_REV) {
    DEBUG ((EFI_D_NET, "\n!PXE.Rev is not supported.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (Pxe->hw.MajorVer < PXE_ROMID_MAJORVER) {

    DEBUG ((EFI_D_NET, "\n!PXE.MajorVer is not supported.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;

  } else if (Pxe->hw.MajorVer == PXE_ROMID_MAJORVER && Pxe->hw.MinorVer < PXE_ROMID_MINORVER) {
    DEBUG ((EFI_D_NET, "\n!PXE.MinorVer is not supported."));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }
  //
  // Do S/W UNDI specific checks.
  //
  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_HW_UNDI) == 0) {
    if (Pxe->sw.EntryPoint < Pxe->sw.Len) {
      DEBUG ((EFI_D_NET, "\n!PXE S/W entry point is not valid."));
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    if (Pxe->sw.BusCnt == 0) {
      DEBUG ((EFI_D_NET, "\n!PXE.BusCnt is zero."));
      Status = EFI_UNSUPPORTED;
      goto Done;
    }
  }

  Status = EFI_SUCCESS;
  DEBUG ((EFI_D_INFO, "Support(): supported on %p\n", Controller));

Done:
  gBS->CloseProtocol (
        Controller,
        &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
SimpleNetworkDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *Nii;
  EFI_DEVICE_PATH_PROTOCOL                  *NiiDevicePath;
  EFI_STATUS                                Status;
  PXE_UNDI                                  *Pxe;
  SNP_DRIVER                                *Snp;
  VOID                                      *Address;
  EFI_HANDLE                                Handle;
  PXE_PCI_CONFIG_INFO                       ConfigInfo;
  PCI_TYPE00                                *ConfigHeader;
  UINT32                                    *TempBar;
  UINT8                                     BarIndex;
  PXE_STATFLAGS                             InitStatFlags;

  DEBUG ((EFI_D_NET, "\nSnpNotifyNetworkInterfaceIdentifier()  "));

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &NiiDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiPciIoProtocolGuid,
                  &NiiDevicePath,
                  &Handle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &mPciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get the NII interface.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  (VOID **) &Nii,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Start(): UNDI3.1 found\n"));

  Pxe = (PXE_UNDI *) (UINTN) (Nii->Id);

  if (Calc8BitCksum (Pxe, Pxe->hw.Len) != 0) {
    DEBUG ((EFI_D_NET, "\n!PXE checksum is not correct.\n"));
    goto NiiError;
  }

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED) != 0) {
    //
    //  We can get any packets.
    //
  } else if ((Pxe->hw.Implementation & PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED) != 0) {
    //
    //  We need to be able to get broadcast packets for DHCP.
    //  If we do not have promiscuous support, we must at least have
    //  broadcast support or we cannot do DHCP!
    //
  } else {
    DEBUG ((EFI_D_NET, "\nUNDI does not have promiscuous or broadcast support."));
    goto NiiError;
  }
  //
  // OK, we like this UNDI, and we know snp is not already there on this handle
  // Allocate and initialize a new simple network protocol structure.
  //
  Status = mPciIo->AllocateBuffer (
                        mPciIo,
                        AllocateAnyPages,
                        EfiBootServicesData,
                        SNP_MEM_PAGES (sizeof (SNP_DRIVER)),
                        &Address,
                        0
                        );

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nCould not allocate SNP_DRIVER structure.\n"));
    goto NiiError;
  }

  Snp = (SNP_DRIVER *) (UINTN) Address;

  ZeroMem (Snp, sizeof (SNP_DRIVER));

  Snp->PciIo      = mPciIo;
  Snp->Signature  = SNP_DRIVER_SIGNATURE;

  EfiInitializeLock (&Snp->Lock, TPL_NOTIFY);

  Snp->Snp.Revision       = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  Snp->Snp.Start          = SnpUndi32Start;
  Snp->Snp.Stop           = SnpUndi32Stop;
  Snp->Snp.Initialize     = SnpUndi32Initialize;
  Snp->Snp.Reset          = SnpUndi32Reset;
  Snp->Snp.Shutdown       = SnpUndi32Shutdown;
  Snp->Snp.ReceiveFilters = SnpUndi32ReceiveFilters;
  Snp->Snp.StationAddress = SnpUndi32StationAddress;
  Snp->Snp.Statistics     = SnpUndi32Statistics;
  Snp->Snp.MCastIpToMac   = SnpUndi32McastIpToMac;
  Snp->Snp.NvData         = SnpUndi32NvData;
  Snp->Snp.GetStatus      = SnpUndi32GetStatus;
  Snp->Snp.Transmit       = SnpUndi32Transmit;
  Snp->Snp.Receive        = SnpUndi32Receive;
  Snp->Snp.WaitForPacket  = NULL;

  Snp->Snp.Mode           = &Snp->Mode;

  Snp->TxRxBufferSize     = 0;
  Snp->TxRxBuffer         = NULL;

  Snp->IfNum              = Nii->IfNum;

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_HW_UNDI) != 0) {
    Snp->IsSwUndi             = FALSE;
    Snp->IssueUndi32Command   = &IssueHwUndiCommand;
  } else {
    Snp->IsSwUndi = TRUE;

    if ((Pxe->sw.Implementation & PXE_ROMID_IMP_SW_VIRT_ADDR) != 0) {
      Snp->IssueUndi32Command = (ISSUE_UNDI32_COMMAND) (UINTN) Pxe->sw.EntryPoint;
    } else {
      Snp->IssueUndi32Command = (ISSUE_UNDI32_COMMAND) (UINTN) ((UINT8) (UINTN) Pxe + Pxe->sw.EntryPoint);
    }
  }
  //
  // Allocate a global CPB and DB buffer for this UNDI interface.
  // we do this because:
  //
  // -UNDI 3.0 wants all the addresses passed to it (even the cpb and db) to be
  // within 2GB limit, create them here and map them so that when undi calls
  // v2p callback to check if the physical address is < 2gb, we will pass.
  //
  // -This is not a requirement for 3.1 or later UNDIs but the code looks
  // simpler if we use the same cpb, db variables for both old and new undi
  // interfaces from all the SNP interface calls (we don't map the buffers
  // for the newer undi interfaces though)
  // .
  // -it is OK to allocate one global set of CPB, DB pair for each UNDI
  // interface as EFI does not multi-task and so SNP will not be re-entered!
  //
  Status = mPciIo->AllocateBuffer (
                        mPciIo,
                        AllocateAnyPages,
                        EfiBootServicesData,
                        SNP_MEM_PAGES (4096),
                        &Address,
                        0
                        );

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nCould not allocate CPB and DB structures.\n"));
    goto Error_DeleteSNP;
  }

  Snp->Cpb  = (VOID *) (UINTN) Address;
  Snp->Db   = (VOID *) ((UINTN) Address + 2048);

  //
  // PxeStart call is going to give the callback functions to UNDI, these callback
  // functions use the BarIndex values from the snp structure, so these must be initialized
  // with default values before doing a PxeStart. The correct values can be obtained after
  // getting the config information from UNDI
  //
  Snp->MemoryBarIndex = 0;
  Snp->IoBarIndex     = 1;

  //
  // we need the undi init information many times in this snp code, just get it
  // once here and store it in the snp driver structure. to get Init Info
  // from UNDI we have to start undi first.
  //
  Status = PxeStart (Snp);

  if (Status != EFI_SUCCESS) {
    goto Error_DeleteSNP;
  }

  Snp->Cdb.OpCode     = PXE_OPCODE_GET_INIT_INFO;
  Snp->Cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;

  Snp->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.CPBaddr    = PXE_DBADDR_NOT_USED;

  Snp->Cdb.DBsize     = (UINT16) sizeof (Snp->InitInfo);
  Snp->Cdb.DBaddr     = (UINT64)(UINTN) (&Snp->InitInfo);

  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;

  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  DEBUG ((EFI_D_NET, "\nSnp->undi.get_init_info()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  //
  // Save the INIT Stat Code...
  //
  InitStatFlags = Snp->Cdb.StatFlags;

  if (Snp->Cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nSnp->undi.init_info()  %xh:%xh\n", Snp->Cdb.StatFlags, Snp->Cdb.StatCode));
    PxeStop (Snp);
    goto Error_DeleteSNP;
  }

  Snp->Cdb.OpCode     = PXE_OPCODE_GET_CONFIG_INFO;
  Snp->Cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;

  Snp->Cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  Snp->Cdb.CPBaddr    = PXE_DBADDR_NOT_USED;

  Snp->Cdb.DBsize     = (UINT16) sizeof (ConfigInfo);
  Snp->Cdb.DBaddr     = (UINT64)(UINTN) &ConfigInfo;

  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;

  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  DEBUG ((EFI_D_NET, "\nSnp->undi.get_config_info()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  if (Snp->Cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nSnp->undi.config_info()  %xh:%xh\n", Snp->Cdb.StatFlags, Snp->Cdb.StatCode));
    PxeStop (Snp);
    goto Error_DeleteSNP;
  }
  //
  // Find the correct BAR to do IO.
  //
  //
  // Enumerate through the PCI BARs for the device to determine which one is
  // the IO BAR.  Save the index of the BAR into the adapter info structure.
  // for  regular 32bit BARs, 0 is memory mapped, 1 is io mapped
  //
  ConfigHeader  = (PCI_TYPE00 *) &ConfigInfo.Config.Byte[0];
  TempBar       = (UINT32 *) &ConfigHeader->Device.Bar[0];
  for (BarIndex = 0; BarIndex <= 5; BarIndex++) {
    if ((*TempBar & PCI_BAR_MEM_MASK) == PCI_BAR_MEM_64BIT) {
      //
      // This is a 64-bit memory bar, skip this and the
      // next bar as well.
      //
      TempBar++;
    }

    if ((*TempBar & PCI_BAR_IO_MASK) == PCI_BAR_IO_MODE) {
      Snp->IoBarIndex = BarIndex;
      break;
    }

    TempBar++;
  }

  //
  //  Initialize simple network protocol mode structure
  //
  Snp->Mode.State               = EfiSimpleNetworkStopped;
  Snp->Mode.HwAddressSize       = Snp->InitInfo.HWaddrLen;
  Snp->Mode.MediaHeaderSize     = Snp->InitInfo.MediaHeaderLen;
  Snp->Mode.MaxPacketSize       = Snp->InitInfo.FrameDataLen;
  Snp->Mode.NvRamAccessSize     = Snp->InitInfo.NvWidth;
  Snp->Mode.NvRamSize           = Snp->InitInfo.NvCount * Snp->Mode.NvRamAccessSize;
  Snp->Mode.IfType              = Snp->InitInfo.IFtype;
  Snp->Mode.MaxMCastFilterCount = Snp->InitInfo.MCastFilterCnt;
  Snp->Mode.MCastFilterCount    = 0;

  switch (InitStatFlags & PXE_STATFLAGS_CABLE_DETECT_MASK) {
  case PXE_STATFLAGS_CABLE_DETECT_SUPPORTED:
    Snp->Mode.MediaPresentSupported = TRUE;
    break;

  case PXE_STATFLAGS_CABLE_DETECT_NOT_SUPPORTED:
  default:
    Snp->Mode.MediaPresentSupported = FALSE;
  }

  switch (InitStatFlags & PXE_STATFLAGS_GET_STATUS_NO_MEDIA_MASK) {
  case PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED:
    Snp->MediaStatusSupported = TRUE;
    break;

  case PXE_STATFLAGS_GET_STATUS_NO_MEDIA_NOT_SUPPORTED:
  default:
    Snp->MediaStatusSupported = FALSE;
  }

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_STATION_ADDR_SETTABLE) != 0) {
    Snp->Mode.MacAddressChangeable = TRUE;
  } else {
    Snp->Mode.MacAddressChangeable = FALSE;
  }

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_MULTI_FRAME_SUPPORTED) != 0) {
    Snp->Mode.MultipleTxSupported = TRUE;
  } else {
    Snp->Mode.MultipleTxSupported = FALSE;
  }

  Snp->Mode.ReceiveFilterMask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED) != 0) {
    Snp->Mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;

  }

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED) != 0) {
    Snp->Mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;

  }

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED) != 0) {
    Snp->Mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;

  }

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_FILTERED_MULTICAST_RX_SUPPORTED) != 0) {
    Snp->Mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST;

  }

  if ((Pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED) != 0) {
    Snp->Mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;

  }

  Snp->Mode.ReceiveFilterSetting = 0;

  //
  //  need to get the station address to save in the mode structure. we need to
  // initialize the UNDI first for this.
  //
  Snp->TxRxBufferSize = Snp->InitInfo.MemoryRequired;
  Status              = PxeInit (Snp, PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE);

  if (EFI_ERROR (Status)) {
    PxeStop (Snp);
    goto Error_DeleteSNP;
  }

  Status = PxeGetStnAddr (Snp);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "\nSnp->undi.get_station_addr() failed.\n"));
    PxeShutdown (Snp);
    PxeStop (Snp);
    goto Error_DeleteSNP;
  }

  Snp->Mode.MediaPresent = FALSE;

  //
  // We should not leave UNDI started and initialized here. this DriverStart()
  // routine must only find and attach the SNP interface to UNDI layer that it
  // finds on the given handle!
  // The UNDI layer will be started when upper layers call Snp->start.
  // How ever, this DriverStart() must fill up the snp mode structure which
  // contains the MAC address of the NIC. For this reason we started and
  // initialized UNDI here, now we are done, do a shutdown and stop of the
  // UNDI interface!
  //
  PxeShutdown (Snp);
  PxeStop (Snp);

  //
  // Create EXIT_BOOT_SERIVES Event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SnpNotifyExitBootServices,
                  Snp,
                  &gEfiEventExitBootServicesGuid,
                  &Snp->ExitBootServicesEvent
                  );
  if (EFI_ERROR (Status)) {
    goto Error_DeleteSNP;
  }

  //
  //  add SNP to the undi handle
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiSimpleNetworkProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &(Snp->Snp)
                  );

  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = mPciIo->FreeBuffer (
                        mPciIo,
                        SNP_MEM_PAGES (4096),
                        Snp->Cpb
                        );

Error_DeleteSNP:

  mPciIo->FreeBuffer (
                mPciIo,
                SNP_MEM_PAGES (sizeof (SNP_DRIVER)),
                Snp
                );
NiiError:
  gBS->CloseProtocol (
        Controller,
        &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
        This->DriverBindingHandle,
        Controller
        );

  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
SimpleNetworkDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_NETWORK_PROTOCOL *SnpProtocol;
  SNP_DRIVER                  *Snp;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &SnpProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (SnpProtocol);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSimpleNetworkProtocolGuid,
                  &Snp->Snp
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close EXIT_BOOT_SERIVES Event
  //
  gBS->CloseEvent (Snp->ExitBootServicesEvent);

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  This->DriverBindingHandle,
                  Controller
                  );

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );

  PxeShutdown (Snp);
  PxeStop (Snp);

  mPciIo->FreeBuffer (
                mPciIo,
                SNP_MEM_PAGES (4096),
                Snp->Cpb
                );

  mPciIo->FreeBuffer (
                mPciIo,
                SNP_MEM_PAGES (sizeof (SNP_DRIVER)),
                Snp
                );

  return Status;
}

//
// Simple Network Protocol Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL mSimpleNetworkDriverBinding = {
  SimpleNetworkDriverSupported,
  SimpleNetworkDriverStart,
  SimpleNetworkDriverStop,
  0xa,
  NULL,
  NULL
};


/**
  This routine maps the given CPU address to a Device address. It creates a
  an entry in the map list with the virtual and physical addresses and the
  un map cookie.

  @param  V2p                  pointer to return a map list node pointer.
  @param  Type                 the direction in which the data flows from the given
                               virtual address device->cpu or cpu->device or both
                               ways.
  @param  VirtualAddress       virtual address (or CPU address) to be mapped.
  @param  BufferSize           size of the buffer to be mapped.

  @retval EFI_SUCEESS           routine has completed the mapping.
  @retval EFI_INVALID_PARAMETER invalid parameter.
  @retval EFI_OUT_OF_RESOURCES  out of resource.
  @retval other                 error as indicated.

**/
EFI_STATUS
AddV2P (
  IN OUT V2P                    **V2p,
  EFI_PCI_IO_PROTOCOL_OPERATION Type,
  VOID                          *VirtualAddress,
  UINTN                         BufferSize
  )
{
  EFI_STATUS  Status;

  if ((V2p == NULL) || (VirtualAddress == NULL) || (BufferSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  *V2p = AllocatePool (sizeof (V2P));
  if (*V2p == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = mPciIo->Map (
                     mPciIo,
                     Type,
                     VirtualAddress,
                     &BufferSize,
                     &(*V2p)->PhysicalAddress,
                     &(*V2p)->Unmap
                     );
  if (Status != EFI_SUCCESS) {
    FreePool (*V2p);
    return Status;
  }
  (*V2p)->VirtualAddress = VirtualAddress;
  (*V2p)->BufferSize     = BufferSize;
  (*V2p)->Next           = mV2p;
  mV2p                   = *V2p;

  return EFI_SUCCESS;
}


/**
  This routine searches the linked list of mapped address nodes (for undi3.0
  interface) to find the node that corresponds to the given virtual address and
  returns a pointer to that node.

  @param  V2p                  pointer to return a map list node pointer.
  @param  VirtualAddr          virtual address (or CPU address) to be searched in
                               the map list

  @retval EFI_SUCEESS          A match was found.
  @retval Other                A match cannot be found.

**/
EFI_STATUS
FindV2p (
  V2P          **V2p,
  VOID         *VirtualAddr
  )
{
  V2P    *Ptr;

  if (V2p == NULL || VirtualAddr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Ptr = mV2p; Ptr != NULL; Ptr = Ptr->Next) {
    if (Ptr->VirtualAddress == VirtualAddr) {
      *V2p = Ptr;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Unmap the given virtual address and free the memory allocated for the map list
  node corresponding to that address.

  @param  VirtualAddress       virtual address (or CPU address) to be unmapped.

  @retval EFI_SUCEESS          Successfully unmapped.
  @retval Other                Other errors as indicated.

**/
EFI_STATUS
DelV2p (
  VOID *VirtualAddress
  )
{
  V2P           *Current;
  V2P           *Next;
  EFI_STATUS    Status;

  if (VirtualAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mV2p == NULL) {
    return EFI_NOT_FOUND;
  }
  //
  // Is our node at the head of the list??
  //
  if ((Current = mV2p)->VirtualAddress == VirtualAddress) {
    mV2p    = mV2p->Next;

    Status  = mPciIo->Unmap (mPciIo, Current->Unmap);

    FreePool (Current);

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Unmap failed with status = %r\n", Status));
    }
    return Status;
  }

  for (; Current->Next != NULL; Current = Next) {
    if ((Next = Current->Next)->VirtualAddress == VirtualAddress) {
      Current->Next = Next->Next;
      Status  = mPciIo->Unmap (mPciIo, Next->Unmap);
      FreePool (Next);

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Unmap failed with status = %r\n", Status));
      }
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  The SNP driver entry point.

  @param ImageHandle       The driver image handle.
  @param SystemTable       The system table.

  @retval EFI_SUCEESS      Initialization routine has found UNDI hardware,
                           loaded it's ROM, and installed a notify event for
                           the Network Indentifier Interface Protocol
                           successfully.
  @retval Other            Return value from HandleProtocol for
                           DeviceIoProtocol or LoadedImageProtocol

**/
EFI_STATUS
EFIAPI
InitializeSnpNiiDriver (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mSimpleNetworkDriverBinding,
           ImageHandle,
           &gSimpleNetworkComponentName,
           &gSimpleNetworkComponentName2
           );
}

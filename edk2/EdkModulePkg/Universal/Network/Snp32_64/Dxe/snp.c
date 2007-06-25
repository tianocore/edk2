/*++
Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module name:
    snp.c

Abstract:

--*/


#include "Snp.h"

//
//  Module global variables needed to support undi 3.0 interface
//
EFI_PCI_IO_PROTOCOL         *mPciIoFncs;
struct s_v2p                *_v2p = NULL; // undi3.0 map_list head
// End Global variables
//

STATIC
EFI_STATUS
issue_hwundi_command (
  UINT64 cdb
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  DEBUG ((EFI_D_ERROR, "\nissue_hwundi_command() - This should not be called!"));

  if (cdb == 0) {
    return EFI_INVALID_PARAMETER;

  }
  //
  //  %%TBD - For now, nothing is done.
  //
  return EFI_UNSUPPORTED;
}

STATIC
UINT8
calc_8bit_cksum (
  VOID  *ptr,
  UINTN len
  )
/*++

Routine Description:
 Compute 8-bit checksum of a buffer.

Arguments:
 ptr - Pointer to buffer.
 len - Length of buffer in bytes.

Returns:
 8-bit checksum of all bytes in buffer.
 If ptr is NULL or len is zero, zero is returned.

--*/
{
  UINT8 *bptr;
  UINT8 cksum;

  bptr  = ptr;
  cksum = 0;

  if (ptr == NULL || len == 0) {
    return 0;
  }

  while (len--) {
    cksum = (UINT8) (cksum +*bptr++);
  }

  return cksum;
}

STATIC
EFI_STATUS
EFIAPI
SimpleNetworkDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports Controller. Any Controller
    that contains a Nii protocol can be supported.

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test.
    RemainingDevicePath - Not used.

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_ALREADY_STARTED - This driver is already running on this device.
    other               - This driver does not support this device.

--*/
{
  EFI_STATUS                                Status;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *NiiProtocol;
  PXE_UNDI                                  *pxe;
  BOOLEAN                                   IsUndi31;

  IsUndi31 = FALSE;
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
  if (Status == EFI_ALREADY_STARTED)
  {
    DEBUG ((EFI_D_INFO, "Support(): Already Started. on handle %x\n", Controller));
    return EFI_ALREADY_STARTED;
  }

  if (!EFI_ERROR (Status))
  {
    DEBUG ((EFI_D_INFO, "Support(): UNDI3.1 found on handle %x\n", Controller));
    IsUndi31 = TRUE;
  } else {
    //
    // try the older 3.0 driver
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid,
                    (VOID **) &NiiProtocol,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((EFI_D_INFO, "Support(): UNDI3.0 found on handle %x\n", Controller));
  }
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
  if (NiiProtocol->ID & 0x0F) {
    DEBUG ((EFI_D_NET, "\n!PXE structure is not paragraph aligned.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  pxe = (PXE_UNDI *) (UINTN) (NiiProtocol->ID);

  //
  //  Verify !PXE revisions.
  //
  if (pxe->hw.Signature != PXE_ROMID_SIGNATURE) {
    DEBUG ((EFI_D_NET, "\n!PXE signature is not valid.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (pxe->hw.Rev < PXE_ROMID_REV) {
    DEBUG ((EFI_D_NET, "\n!PXE.Rev is not supported.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (pxe->hw.MajorVer < PXE_ROMID_MAJORVER) {

    DEBUG ((EFI_D_NET, "\n!PXE.MajorVer is not supported.\n"));
    Status = EFI_UNSUPPORTED;
    goto Done;

  } else if (pxe->hw.MajorVer == PXE_ROMID_MAJORVER && pxe->hw.MinorVer < PXE_ROMID_MINORVER) {
    DEBUG ((EFI_D_NET, "\n!PXE.MinorVer is not supported."));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }
  //
  // Do S/W UNDI specific checks.
  //
  if ((pxe->hw.Implementation & PXE_ROMID_IMP_HW_UNDI) == 0) {
    if (pxe->sw.EntryPoint < pxe->sw.Len) {
      DEBUG ((EFI_D_NET, "\n!PXE S/W entry point is not valid."));
      Status = EFI_UNSUPPORTED;
      goto Done;
    }

    if (pxe->sw.BusCnt == 0) {
      DEBUG ((EFI_D_NET, "\n!PXE.BusCnt is zero."));
      Status = EFI_UNSUPPORTED;
      goto Done;
    }
  }

  Status = EFI_SUCCESS;
  DEBUG ((EFI_D_INFO, "Support(): supported on %x\n", Controller));

Done:
  if (IsUndi31) {
    gBS->CloseProtocol (
          Controller,
          &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
          This->DriverBindingHandle,
          Controller
          );

  } else {
    gBS->CloseProtocol (
          Controller,
          &gEfiNetworkInterfaceIdentifierProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
SimpleNetworkDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

Routine Description:
 called for any handle that we said "supported" in the above call!

Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to start
    RemainingDevicePath - Not used.

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    other               - This driver failed to start this device.

--*/
{
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *Nii;
  EFI_DEVICE_PATH_PROTOCOL                  *NiiDevicePath;
  EFI_STATUS                                Status;
  PXE_UNDI                                  *pxe;
  SNP_DRIVER                                *snp;
  VOID                                      *addr;
  VOID                                      *addrUnmap;
  EFI_PHYSICAL_ADDRESS                      paddr;
  EFI_HANDLE                                Handle;
  UINTN                                     Size;
  BOOLEAN                                   UndiNew;
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
                  (VOID **) &mPciIoFncs,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get the NII interface. look for 3.1 undi first, if it is not there
  // then look for 3.0, validate the interface.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  (VOID **) &Nii,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    gBS->CloseProtocol (
          Controller,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }

  if (!EFI_ERROR (Status)) {
    //
    // probably not a 3.1 UNDI
    //
    UndiNew = TRUE;
    DEBUG ((EFI_D_INFO, "Start(): UNDI3.1 found\n"));

  } else {
    UndiNew = FALSE;
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid,
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

    DEBUG ((EFI_D_INFO, "Start(): UNDI3.0 found\n"));
  }

  pxe = (PXE_UNDI *) (UINTN) (Nii->ID);

  if (calc_8bit_cksum (pxe, pxe->hw.Len) != 0) {
    DEBUG ((EFI_D_NET, "\n!PXE checksum is not correct.\n"));
    goto NiiError;
  }

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED) != 0) {
    //
    //  We can get any packets.
    //
  } else if ((pxe->hw.Implementation & PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED) != 0) {
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
  Status = mPciIoFncs->AllocateBuffer (
                        mPciIoFncs,
                        AllocateAnyPages,
                        EfiBootServicesData,
                        SNP_MEM_PAGES (sizeof (SNP_DRIVER)),
                        &addr,
                        0
                        );

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nCould not allocate SNP_DRIVER structure.\n"));
    goto NiiError;
  }

  snp = (SNP_DRIVER *) (UINTN) addr;

  if (!UndiNew) {
    Size = SNP_MEM_PAGES (sizeof (SNP_DRIVER));

    Status = mPciIoFncs->Map (
                          mPciIoFncs,
                          EfiPciIoOperationBusMasterCommonBuffer,
                          addr,
                          &Size,
                          &paddr,
                          &addrUnmap
                          );

    ASSERT (paddr);

    DEBUG ((EFI_D_NET, "\nSNP_DRIVER @ %Xh, sizeof(SNP_DRIVER) == %d", addr, sizeof (SNP_DRIVER)));
    snp                 = (SNP_DRIVER *) (UINTN) paddr;
    snp->SnpDriverUnmap = addrUnmap;
  }

  ZeroMem (snp, sizeof (SNP_DRIVER));

  snp->IoFncs     = mPciIoFncs;
  snp->IsOldUndi  = (BOOLEAN) (!UndiNew);

  snp->Signature  = SNP_DRIVER_SIGNATURE;

  EfiInitializeLock (&snp->lock, TPL_NOTIFY);

  snp->snp.Revision       = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  snp->snp.Start          = snp_undi32_start;
  snp->snp.Stop           = snp_undi32_stop;
  snp->snp.Initialize     = snp_undi32_initialize;
  snp->snp.Reset          = snp_undi32_reset;
  snp->snp.Shutdown       = snp_undi32_shutdown;
  snp->snp.ReceiveFilters = snp_undi32_receive_filters;
  snp->snp.StationAddress = snp_undi32_station_address;
  snp->snp.Statistics     = snp_undi32_statistics;
  snp->snp.MCastIpToMac   = snp_undi32_mcast_ip_to_mac;
  snp->snp.NvData         = snp_undi32_nvdata;
  snp->snp.GetStatus      = snp_undi32_get_status;
  snp->snp.Transmit       = snp_undi32_transmit;
  snp->snp.Receive        = snp_undi32_receive;
  snp->snp.WaitForPacket  = NULL;

  snp->snp.Mode           = &snp->mode;

  snp->tx_rx_bufsize      = 0;
  snp->tx_rx_buffer       = NULL;

  snp->if_num             = Nii->IfNum;

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_HW_UNDI) != 0) {
    snp->is_swundi            = FALSE;
    snp->issue_undi32_command = &issue_hwundi_command;
  } else {
    snp->is_swundi = TRUE;

    if ((pxe->sw.Implementation & PXE_ROMID_IMP_SW_VIRT_ADDR) != 0) {
      snp->issue_undi32_command = (issue_undi32_command) (UINTN) pxe->sw.EntryPoint;
    } else {
      snp->issue_undi32_command = (issue_undi32_command) (UINTN) ((UINT8) (UINTN) pxe + pxe->sw.EntryPoint);
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
  Status = mPciIoFncs->AllocateBuffer (
                        mPciIoFncs,
                        AllocateAnyPages,
                        EfiBootServicesData,
                        SNP_MEM_PAGES (4096),
                        &addr,
                        0
                        );

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nCould not allocate CPB and DB structures.\n"));
    goto Error_DeleteSNP;
  }

  if (snp->IsOldUndi) {
    Size = SNP_MEM_PAGES (4096);

    Status = mPciIoFncs->Map (
                          mPciIoFncs,
                          EfiPciIoOperationBusMasterCommonBuffer,
                          addr,
                          &Size,
                          &paddr,
                          &snp->CpbUnmap
                          );

    ASSERT (paddr);

    snp->cpb  = (VOID *) (UINTN) paddr;
    snp->db   = (VOID *) ((UINTN) paddr + 2048);
  } else {
    snp->cpb  = (VOID *) (UINTN) addr;
    snp->db   = (VOID *) ((UINTN) addr + 2048);
  }
  //
  // pxe_start call is going to give the callback functions to UNDI, these callback
  // functions use the BarIndex values from the snp structure, so these must be initialized
  // with default values before doing a pxe_start. The correct values can be obtained after
  // getting the config information from UNDI
  //
  snp->MemoryBarIndex = 0;
  snp->IoBarIndex     = 1;

  //
  // we need the undi init information many times in this snp code, just get it
  // once here and store it in the snp driver structure. to get Init Info
  // from UNDI we have to start undi first.
  //
  Status = pxe_start (snp);

  if (Status != EFI_SUCCESS) {
    goto Error_DeleteCPBDB;
  }

  snp->cdb.OpCode     = PXE_OPCODE_GET_INIT_INFO;
  snp->cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;

  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  snp->cdb.CPBaddr    = PXE_DBADDR_NOT_USED;

  snp->cdb.DBsize     = sizeof snp->init_info;
  snp->cdb.DBaddr     = (UINT64) (UINTN) &snp->init_info;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;

  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  DEBUG ((EFI_D_NET, "\nsnp->undi.get_init_info()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  //
  // Save the INIT Stat Code...
  //
  InitStatFlags = snp->cdb.StatFlags;

  if (snp->cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nsnp->undi.init_info()  %xh:%xh\n", snp->cdb.StatFlags, snp->cdb.StatCode));
    pxe_stop (snp);
    goto Error_DeleteCPBDB;
  }

  snp->cdb.OpCode     = PXE_OPCODE_GET_CONFIG_INFO;
  snp->cdb.OpFlags    = PXE_OPFLAGS_NOT_USED;

  snp->cdb.CPBsize    = PXE_CPBSIZE_NOT_USED;
  snp->cdb.CPBaddr    = PXE_DBADDR_NOT_USED;

  snp->cdb.DBsize     = sizeof ConfigInfo;
  snp->cdb.DBaddr     = (UINT64) (UINTN) &ConfigInfo;

  snp->cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  snp->cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;

  snp->cdb.IFnum      = snp->if_num;
  snp->cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  DEBUG ((EFI_D_NET, "\nsnp->undi.get_config_info()  "));

  (*snp->issue_undi32_command) ((UINT64) (UINTN) &snp->cdb);

  if (snp->cdb.StatCode != PXE_STATCODE_SUCCESS) {
    DEBUG ((EFI_D_NET, "\nsnp->undi.config_info()  %xh:%xh\n", snp->cdb.StatFlags, snp->cdb.StatCode));
    pxe_stop (snp);
    goto Error_DeleteCPBDB;
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
      snp->IoBarIndex = BarIndex;
      break;
    }

    TempBar++;
  }

  //
  // We allocate 2 more global buffers for undi 3.0 interface. We use these
  // buffers to pass to undi when the user buffers are beyond 4GB.
  // UNDI 3.0 wants all the addresses passed to it to be
  // within 2GB limit, create them here and map them so that when undi calls
  // v2p callback to check if the physical address is < 2gb, we will pass.
  //
  // For 3.1 and later UNDIs, we do not do this because undi is
  // going to call the map() callback if and only if it wants to use the
  // device address for any address it receives.
  //
  if (snp->IsOldUndi) {
    //
    // buffer for receive
    //
    Size = SNP_MEM_PAGES (snp->init_info.MediaHeaderLen + snp->init_info.FrameDataLen);
    Status = mPciIoFncs->AllocateBuffer (
                          mPciIoFncs,
                          AllocateAnyPages,
                          EfiBootServicesData,
                          Size,
                          &addr,
                          0
                          );

    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "\nCould not allocate receive buffer.\n"));
      goto Error_DeleteCPBDB;
    }

    Status = mPciIoFncs->Map (
                          mPciIoFncs,
                          EfiPciIoOperationBusMasterCommonBuffer,
                          addr,
                          &Size,
                          &paddr,
                          &snp->ReceiveBufUnmap
                          );

    ASSERT (paddr);

    snp->receive_buf = (UINT8 *) (UINTN) paddr;

    //
    // buffer for fill_header
    //
    Size = SNP_MEM_PAGES (snp->init_info.MediaHeaderLen);
    Status = mPciIoFncs->AllocateBuffer (
                          mPciIoFncs,
                          AllocateAnyPages,
                          EfiBootServicesData,
                          Size,
                          &addr,
                          0
                          );

    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "\nCould not allocate fill_header buffer.\n"));
      goto Error_DeleteRCVBuf;
    }

    Status = mPciIoFncs->Map (
                          mPciIoFncs,
                          EfiPciIoOperationBusMasterCommonBuffer,
                          addr,
                          &Size,
                          &paddr,
                          &snp->FillHdrBufUnmap
                          );

    ASSERT (paddr);
    snp->fill_hdr_buf = (UINT8 *) (UINTN) paddr;
  }
  //
  //  Initialize simple network protocol mode structure
  //
  snp->mode.State               = EfiSimpleNetworkStopped;
  snp->mode.HwAddressSize       = snp->init_info.HWaddrLen;
  snp->mode.MediaHeaderSize     = snp->init_info.MediaHeaderLen;
  snp->mode.MaxPacketSize       = snp->init_info.FrameDataLen;
  snp->mode.NvRamAccessSize     = snp->init_info.NvWidth;
  snp->mode.NvRamSize           = snp->init_info.NvCount * snp->mode.NvRamAccessSize;
  snp->mode.IfType              = snp->init_info.IFtype;
  snp->mode.MaxMCastFilterCount = snp->init_info.MCastFilterCnt;
  snp->mode.MCastFilterCount    = 0;

  switch (InitStatFlags & PXE_STATFLAGS_CABLE_DETECT_MASK) {
  case PXE_STATFLAGS_CABLE_DETECT_SUPPORTED:
    snp->mode.MediaPresentSupported = TRUE;
    break;

  case PXE_STATFLAGS_CABLE_DETECT_NOT_SUPPORTED:
  default:
    snp->mode.MediaPresentSupported = FALSE;
  }

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_STATION_ADDR_SETTABLE) != 0) {
    snp->mode.MacAddressChangeable = TRUE;
  } else {
    snp->mode.MacAddressChangeable = FALSE;
  }

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_MULTI_FRAME_SUPPORTED) != 0) {
    snp->mode.MultipleTxSupported = TRUE;
  } else {
    snp->mode.MultipleTxSupported = FALSE;
  }

  snp->mode.ReceiveFilterMask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED) != 0) {
    snp->mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;

  }

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED) != 0) {
    snp->mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;

  }

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED) != 0) {
    snp->mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;

  }

  if ((pxe->hw.Implementation & PXE_ROMID_IMP_FILTERED_MULTICAST_RX_SUPPORTED) != 0) {
    snp->mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST;

  }

  if (pxe->hw.Implementation & PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED) {
    snp->mode.ReceiveFilterMask |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;

  }

  snp->mode.ReceiveFilterSetting = 0;

  //
  //  need to get the station address to save in the mode structure. we need to
  // initialize the UNDI first for this.
  //
  snp->tx_rx_bufsize  = snp->init_info.MemoryRequired;
  Status              = pxe_init (snp, PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE);

  if (Status) {
    pxe_stop (snp);
    goto Error_DeleteHdrBuf;
  }

  Status = pxe_get_stn_addr (snp);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "\nsnp->undi.get_station_addr()  failed.\n"));
    pxe_shutdown (snp);
    pxe_stop (snp);
    goto Error_DeleteHdrBuf;
  }

  snp->mode.MediaPresent = FALSE;

  //
  // We should not leave UNDI started and initialized here. this DriverStart()
  // routine must only find and attach the SNP interface to UNDI layer that it
  // finds on the given handle!
  // The UNDI layer will be started when upper layers call snp->start.
  // How ever, this DriverStart() must fill up the snp mode structure which
  // contains the MAC address of the NIC. For this reason we started and
  // initialized UNDI here, now we are done, do a shutdown and stop of the
  // UNDI interface!
  //
  pxe_shutdown (snp);
  pxe_stop (snp);

  //
  //  add SNP to the undi handle
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiSimpleNetworkProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &(snp->snp)
                  );

  if (!EFI_ERROR (Status)) {
    return Status;
  }

Error_DeleteHdrBuf:
  if (snp->IsOldUndi) {
    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          snp->FillHdrBufUnmap
                          );
    Size = SNP_MEM_PAGES (snp->init_info.MediaHeaderLen);
    mPciIoFncs->FreeBuffer (
                  mPciIoFncs,
                  Size,
                  snp->fill_hdr_buf
                  );
  }

Error_DeleteRCVBuf:
  if (snp->IsOldUndi) {
    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          snp->ReceiveBufUnmap
                          );
    Size = SNP_MEM_PAGES (snp->init_info.MediaHeaderLen + snp->init_info.FrameDataLen);
    mPciIoFncs->FreeBuffer (
                  mPciIoFncs,
                  Size,
                  snp->receive_buf
                  );

  }

Error_DeleteCPBDB:
  if (snp->IsOldUndi) {
    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          snp->CpbUnmap
                          );
  }

  Status = mPciIoFncs->FreeBuffer (
                        mPciIoFncs,
                        SNP_MEM_PAGES (4096),
                        snp->cpb
                        );

Error_DeleteSNP:
  if (snp->IsOldUndi) {
    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          snp->SnpDriverUnmap
                          );
  }

  mPciIoFncs->FreeBuffer (
                mPciIoFncs,
                SNP_MEM_PAGES (sizeof (SNP_DRIVER)),
                snp
                );
NiiError:
  if (!UndiNew) {
    gBS->CloseProtocol (
          Controller,
          &gEfiNetworkInterfaceIdentifierProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  } else {
    gBS->CloseProtocol (
          Controller,
          &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
          This->DriverBindingHandle,
          Controller
          );
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
SimpleNetworkDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
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
                  &Snp->snp
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!Snp->IsOldUndi) {
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    This->DriverBindingHandle,
                    Controller
                    );
  } else {
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
  }

  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );

  pxe_shutdown (Snp);
  pxe_stop (Snp);

  if (Snp->IsOldUndi) {
    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          Snp->FillHdrBufUnmap
                          );

    mPciIoFncs->FreeBuffer (
                  mPciIoFncs,
                  SNP_MEM_PAGES (Snp->init_info.MediaHeaderLen),
                  Snp->fill_hdr_buf
                  );
    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          Snp->ReceiveBufUnmap
                          );

    mPciIoFncs->FreeBuffer (
                  mPciIoFncs,
                  SNP_MEM_PAGES (Snp->init_info.MediaHeaderLen + Snp->init_info.FrameDataLen),
                  Snp->receive_buf
                  );

    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          Snp->CpbUnmap
                          );
    Status = mPciIoFncs->Unmap (
                          mPciIoFncs,
                          Snp->SnpDriverUnmap
                          );
  }

  mPciIoFncs->FreeBuffer (
                mPciIoFncs,
                SNP_MEM_PAGES (4096),
                Snp->cpb
                );

  mPciIoFncs->FreeBuffer (
                mPciIoFncs,
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

EFI_STATUS
add_v2p (
  IN OUT struct s_v2p           **v2p,
  EFI_PCI_IO_PROTOCOL_OPERATION type,
  VOID                          *vaddr,
  UINTN                         bsize
  )
/*++

Routine Description:
 This routine maps the given CPU address to a Device address. It creates a
 an entry in the map list with the virtual and physical addresses and the
 un map cookie.

Arguments:
 v2p - pointer to return a map list node pointer.
 type - the direction in which the data flows from the given virtual address
        device->cpu or cpu->device or both ways.
 vaddr - virtual address (or CPU address) to be mapped
 bsize - size of the buffer to be mapped.

Returns:

  EFI_SUCEESS - routine has completed the mapping
  other - error as indicated.

--*/
{
  EFI_STATUS  Status;

  if ((v2p == NULL) || (vaddr == NULL) || (bsize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  *v2p = AllocatePool (sizeof (struct s_v2p));
  if (*v2p != NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = mPciIoFncs->Map (
                        mPciIoFncs,
                        type,
                        vaddr,
                        &bsize,
                        &(*v2p)->paddr,
                        &(*v2p)->unmap
                        );
  if (Status != EFI_SUCCESS) {
    FreePool (*v2p);
    return Status;
  }
  (*v2p)->vaddr = vaddr;
  (*v2p)->bsize = bsize;
  (*v2p)->next  = _v2p;
  _v2p          = *v2p;

  return EFI_SUCCESS;
}

EFI_STATUS
find_v2p (
  struct s_v2p **v2p,
  VOID         *vaddr
  )
/*++

Routine Description:
 This routine searches the linked list of mapped address nodes (for undi3.0
 interface) to find the node that corresponds to the given virtual address and
 returns a pointer to that node.

Arguments:
 v2p - pointer to return a map list node pointer.
 vaddr - virtual address (or CPU address) to be searched in the map list

Returns:

  EFI_SUCEESS - if a match found!
  Other       - match not found

--*/
{
  struct s_v2p  *v;

  if (v2p == NULL || vaddr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (v = _v2p; v != NULL; v = v->next) {
    if (v->vaddr == vaddr) {
      *v2p = v;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
del_v2p (
  VOID *vaddr
  )
/*++

Routine Description:
 This routine unmaps the given virtual address and frees the memory allocated
 for the map list node corresponding to that address.

Arguments:
 vaddr - virtual address (or CPU address) to be unmapped

Returns:
 EFI_SUCEESS -  if successfully unmapped
 Other - as indicated by the error


--*/
{
  struct s_v2p  *v;
  struct s_v2p  *t;
  EFI_STATUS    Status;

  if (vaddr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (_v2p == NULL) {
    return EFI_NOT_FOUND;
  }
  //
  // Is our node at the head of the list??
  //
  if ((v = _v2p)->vaddr == vaddr) {
    _v2p    = _v2p->next;

    Status  = mPciIoFncs->Unmap (mPciIoFncs, v->unmap);

    FreePool (v);

    if (Status) {
      DEBUG ((EFI_D_ERROR, "Unmap failed with status = %x\n", Status));
    }
    return Status;
  }

  for (; v->next != NULL; v = t) {
    if ((t = v->next)->vaddr == vaddr) {
      v->next = t->next;
      Status  = mPciIoFncs->Unmap (mPciIoFncs, t->unmap);
      FreePool (t);

      if (Status) {
        DEBUG ((EFI_D_ERROR, "Unmap failed with status = %x\n", Status));
      }
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}

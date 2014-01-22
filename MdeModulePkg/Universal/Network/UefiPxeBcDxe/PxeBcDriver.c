/** @file
  The driver binding for UEFI PXEBC protocol.

Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "PxeBcImpl.h"

EFI_DRIVER_BINDING_PROTOCOL gPxeBcDriverBinding = {
  PxeBcDriverBindingSupported,
  PxeBcDriverBindingStart,
  PxeBcDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
PxeBcDriverEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
          ImageHandle,
          SystemTable,
          &gPxeBcDriverBinding,
          ImageHandle,
          &gPxeBcComponentName,
          &gPxeBcComponentName2
          );
}


/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.
  PxeBc requires DHCP4 and MTFTP4 protocols.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
PxeBcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_STATUS                  Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPxeBaseCodeProtocolGuid,
                  (VOID **) &PxeBc,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {

    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiMtftp4ServiceBindingProtocolGuid,
                    NULL,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );

  }

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
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
PxeBcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  PXEBC_PRIVATE_DATA  *Private;
  UINTN               Index;
  EFI_STATUS          Status;
  EFI_IP4_MODE_DATA   Ip4ModeData;

  Private = AllocateZeroPool (sizeof (PXEBC_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature                    = PXEBC_PRIVATE_DATA_SIGNATURE;
  Private->Controller                   = ControllerHandle;
  Private->Image                        = This->DriverBindingHandle;
  CopyMem (&Private->PxeBc, &mPxeBcProtocolTemplate, sizeof (Private->PxeBc));
  Private->PxeBc.Mode                   = &Private->Mode;
  CopyMem (&Private->LoadFile, &mLoadFileProtocolTemplate, sizeof (Private->LoadFile));

  Private->ProxyOffer.Packet.Offer.Size = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;
  Private->Dhcp4Ack.Packet.Ack.Size     = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;
  Private->PxeReply.Packet.Ack.Size     = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;

  for (Index = 0; Index < PXEBC_MAX_OFFER_NUM; Index++) {
    Private->Dhcp4Offers[Index].Packet.Offer.Size = PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE;
  }

  //
  // Get the NII interface if it exists.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  (VOID **) &Private->Nii,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    Private->Nii = NULL;
  }

  Status = NetLibCreateServiceChild (
            ControllerHandle,
            This->DriverBindingHandle,
            &gEfiArpServiceBindingProtocolGuid,
            &Private->ArpChild
            );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->ArpChild,
                  &gEfiArpProtocolGuid,
                  (VOID **) &Private->Arp,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = NetLibCreateServiceChild (
            ControllerHandle,
            This->DriverBindingHandle,
            &gEfiDhcp4ServiceBindingProtocolGuid,
            &Private->Dhcp4Child
            );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Dhcp4Child,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **) &Private->Dhcp4,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiIp4ServiceBindingProtocolGuid,
             &Private->Ip4Child
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Ip4Child,
                  &gEfiIp4ProtocolGuid,
                  (VOID **) &Private->Ip4,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Get max packet size from Ip4 to calculate block size for Tftp later.
  //
  Status = Private->Ip4->GetModeData (Private->Ip4, &Ip4ModeData, NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Private->Ip4MaxPacketSize = Ip4ModeData.MaxPacketSize;

  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiMtftp4ServiceBindingProtocolGuid,
             &Private->Mtftp4Child
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Mtftp4Child,
                  &gEfiMtftp4ProtocolGuid,
                  (VOID **) &Private->Mtftp4,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiUdp4ServiceBindingProtocolGuid,
             &Private->Udp4ReadChild
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // The UDP instance for EfiPxeBcUdpRead
  //
  Status = gBS->OpenProtocol (
                  Private->Udp4ReadChild,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **) &Private->Udp4Read,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // The UDP instance for EfiPxeBcUdpWrite
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiUdp4ServiceBindingProtocolGuid,
             &Private->Udp4WriteChild
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Udp4WriteChild,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **) &Private->Udp4Write,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  ZeroMem (&Private->Udp4CfgData, sizeof (EFI_UDP4_CONFIG_DATA));
  Private->Udp4CfgData.AcceptBroadcast    = FALSE;
  Private->Udp4CfgData.AcceptPromiscuous  = FALSE;
  Private->Udp4CfgData.AcceptAnyPort      = TRUE;
  Private->Udp4CfgData.AllowDuplicatePort = TRUE;
  Private->Udp4CfgData.TypeOfService      = DEFAULT_ToS;
  Private->Udp4CfgData.TimeToLive         = DEFAULT_TTL;
  Private->Udp4CfgData.DoNotFragment      = FALSE;
  Private->Udp4CfgData.ReceiveTimeout     = PXEBC_DEFAULT_LIFETIME;
  Private->Udp4CfgData.UseDefaultAddress  = FALSE;

  PxeBcInitSeedPacket (&Private->SeedPacket, Private->Udp4Read);
  Private->MacLen = Private->SeedPacket.Dhcp4.Header.HwAddrLen;
  CopyMem (&Private->Mac, &Private->SeedPacket.Dhcp4.Header.ClientHwAddr[0], Private->MacLen);


  ZeroMem (&Private->Ip4ConfigData, sizeof (EFI_IP4_CONFIG_DATA));
  Private->Ip4ConfigData.DefaultProtocol   = EFI_IP_PROTO_ICMP;
  Private->Ip4ConfigData.AcceptIcmpErrors  = TRUE;
  Private->Ip4ConfigData.TypeOfService     = DEFAULT_ToS;
  Private->Ip4ConfigData.TimeToLive        = DEFAULT_TTL;
  Private->Ip4ConfigData.DoNotFragment     = FALSE;
  Private->Ip4ConfigData.RawData           = FALSE;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiPxeBaseCodeProtocolGuid,
                  &Private->PxeBc,
                  &gEfiLoadFileProtocolGuid,
                  &Private->LoadFile,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (Private->Udp4WriteChild != NULL) {
    gBS->CloseProtocol (
           Private->Udp4WriteChild,
           &gEfiUdp4ProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    NetLibDestroyServiceChild (
      ControllerHandle,
      This->DriverBindingHandle,
      &gEfiUdp4ServiceBindingProtocolGuid,
      Private->Udp4WriteChild
      );
  }

  if (Private->Udp4ReadChild != NULL) {
    gBS->CloseProtocol (
          Private->Udp4ReadChild,
          &gEfiUdp4ProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    NetLibDestroyServiceChild (
      ControllerHandle,
      This->DriverBindingHandle,
      &gEfiUdp4ServiceBindingProtocolGuid,
      Private->Udp4ReadChild
      );
  }

  if (Private->Mtftp4Child != NULL) {
    gBS->CloseProtocol (
          Private->Mtftp4Child,
          &gEfiMtftp4ProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

    NetLibDestroyServiceChild (
      ControllerHandle,
      This->DriverBindingHandle,
      &gEfiMtftp4ServiceBindingProtocolGuid,
      Private->Mtftp4Child
      );
  }

  if (Private->Ip4Child != NULL) {
    gBS->CloseProtocol (
          Private->Ip4Child,
          &gEfiIp4ProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

    NetLibDestroyServiceChild (
      ControllerHandle,
      This->DriverBindingHandle,
      &gEfiIp4ServiceBindingProtocolGuid,
      Private->Ip4Child
      );
  }

  if (Private->Dhcp4Child != NULL) {
    gBS->CloseProtocol (
          Private->Dhcp4Child,
          &gEfiDhcp4ProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

    NetLibDestroyServiceChild (
      ControllerHandle,
      This->DriverBindingHandle,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Private->Dhcp4Child
      );
  }

  if (Private->ArpChild != NULL) {
    gBS->CloseProtocol (
          Private->ArpChild,
          &gEfiArpProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

    NetLibDestroyServiceChild (
      ControllerHandle,
      This->DriverBindingHandle,
      &gEfiArpServiceBindingProtocolGuid,
      Private->ArpChild
      );
  }

  FreePool (Private);

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
PxeBcDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  PXEBC_PRIVATE_DATA          *Private;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_HANDLE                  NicHandle;
  EFI_STATUS                  Status;

  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiArpProtocolGuid);
  if (NicHandle == NULL) {
    NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiDhcp4ProtocolGuid);

    if (NicHandle == NULL) {
      NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiIp4ProtocolGuid);

      if (NicHandle == NULL) {
        NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiUdp4ProtocolGuid);

        if (NicHandle == NULL) {
          NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiMtftp4ProtocolGuid);

          if (NicHandle == NULL) {
            return EFI_SUCCESS;
          }
        }
      }
    }
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiPxeBaseCodeProtocolGuid,
                  (VOID **) &PxeBc,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Stop functionality of PXE Base Code protocol
  //
  Status = PxeBc->Stop (PxeBc);
  if (Status != EFI_SUCCESS && Status != EFI_NOT_STARTED) {
    return Status;
  }

  Private = PXEBC_PRIVATE_DATA_FROM_PXEBC (PxeBc);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  NicHandle,
                  &gEfiPxeBaseCodeProtocolGuid,
                  &Private->PxeBc,
                  &gEfiLoadFileProtocolGuid,
                  &Private->LoadFile,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {

    gBS->CloseProtocol (
           Private->Udp4WriteChild,
           &gEfiUdp4ProtocolGuid,
           This->DriverBindingHandle,
           NicHandle
           );
    NetLibDestroyServiceChild (
      ControllerHandle,
      This->DriverBindingHandle,
      &gEfiUdp4ServiceBindingProtocolGuid,
      Private->Udp4WriteChild
      );

    gBS->CloseProtocol (
          Private->Udp4ReadChild,
          &gEfiUdp4ProtocolGuid,
          This->DriverBindingHandle,
          NicHandle
          );
    NetLibDestroyServiceChild (
      NicHandle,
      This->DriverBindingHandle,
      &gEfiUdp4ServiceBindingProtocolGuid,
      Private->Udp4ReadChild
      );

    gBS->CloseProtocol (
          Private->Dhcp4Child,
          &gEfiDhcp4ProtocolGuid,
          This->DriverBindingHandle,
          NicHandle
          );
    NetLibDestroyServiceChild (
      NicHandle,
      This->DriverBindingHandle,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Private->Dhcp4Child
      );

    gBS->CloseProtocol (
          Private->Mtftp4Child,
          &gEfiMtftp4ProtocolGuid,
          This->DriverBindingHandle,
          NicHandle
          );
    NetLibDestroyServiceChild (
      NicHandle,
      This->DriverBindingHandle,
      &gEfiMtftp4ServiceBindingProtocolGuid,
      Private->Mtftp4Child
      );

    gBS->CloseProtocol (
          Private->Ip4Child,
          &gEfiIp4ProtocolGuid,
          This->DriverBindingHandle,
          NicHandle
          );
    NetLibDestroyServiceChild (
      NicHandle,
      This->DriverBindingHandle,
      &gEfiIp4ServiceBindingProtocolGuid,
      Private->Ip4Child
      );

    gBS->CloseProtocol (
          Private->ArpChild,
          &gEfiArpProtocolGuid,
          This->DriverBindingHandle,
          NicHandle
          );
    NetLibDestroyServiceChild (
      NicHandle,
      This->DriverBindingHandle,
      &gEfiArpServiceBindingProtocolGuid,
      Private->ArpChild
      );

    FreePool (Private);
  }

  return Status;
}



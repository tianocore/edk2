/** @file
  The entry point of IScsi driver.

Copyright (c) 2019, NVIDIA Corporation. All rights reserved.
Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2017 Hewlett Packard Enterprise Development LP<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IScsiImpl.h"

EFI_DRIVER_BINDING_PROTOCOL gIScsiIp4DriverBinding = {
  IScsiIp4DriverBindingSupported,
  IScsiIp4DriverBindingStart,
  IScsiIp4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL gIScsiIp6DriverBinding = {
  IScsiIp6DriverBindingSupported,
  IScsiIp6DriverBindingStart,
  IScsiIp6DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_GUID                    gIScsiV4PrivateGuid = ISCSI_V4_PRIVATE_GUID;
EFI_GUID                    gIScsiV6PrivateGuid = ISCSI_V6_PRIVATE_GUID;
ISCSI_PRIVATE_DATA          *mPrivate           = NULL;

/**
  Tests to see if this driver supports the RemainingDevicePath.

  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The RemainingDevicePath is supported or NULL.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
IScsiIsDevicePathSupported (
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *CurrentDevicePath;

  CurrentDevicePath = RemainingDevicePath;
  if (CurrentDevicePath != NULL) {
    while (!IsDevicePathEnd (CurrentDevicePath)) {
      if ((CurrentDevicePath->Type == MESSAGING_DEVICE_PATH) && (CurrentDevicePath->SubType == MSG_ISCSI_DP)) {
        return EFI_SUCCESS;
      }

      CurrentDevicePath = NextDevicePathNode (CurrentDevicePath);
    }

    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Check whether an iSCSI HBA adapter already installs an AIP instance with
  network boot policy matching the value specified in PcdIScsiAIPNetworkBootPolicy.
  If yes, return EFI_SUCCESS.

  @retval EFI_SUCCESS              Found an AIP with matching network boot policy.
  @retval EFI_NOT_FOUND            AIP is unavailable or the network boot policy
                                   not matched.
**/
EFI_STATUS
IScsiCheckAip (
  VOID
  )
{
  UINTN                            AipHandleCount;
  EFI_HANDLE                       *AipHandleBuffer;
  UINTN                            AipIndex;
  EFI_ADAPTER_INFORMATION_PROTOCOL *Aip;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *ExtScsiPassThru;
  EFI_GUID                         *InfoTypesBuffer;
  UINTN                            InfoTypeBufferCount;
  UINTN                            TypeIndex;
  VOID                             *InfoBlock;
  UINTN                            InfoBlockSize;
  BOOLEAN                          Supported;
  EFI_ADAPTER_INFO_NETWORK_BOOT    *NetworkBoot;
  EFI_STATUS                       Status;
  UINT8                            NetworkBootPolicy;

  //
  // Check any AIP instances exist in system.
  //
  AipHandleCount  = 0;
  AipHandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiAdapterInformationProtocolGuid,
                  NULL,
                  &AipHandleCount,
                  &AipHandleBuffer
                  );
  if (EFI_ERROR (Status) || AipHandleCount == 0) {
    return EFI_NOT_FOUND;
  }

  ASSERT (AipHandleBuffer != NULL);

  InfoBlock = NULL;

  for (AipIndex = 0; AipIndex < AipHandleCount; AipIndex++) {
    Status = gBS->HandleProtocol (
                    AipHandleBuffer[AipIndex],
                    &gEfiAdapterInformationProtocolGuid,
                    (VOID *) &Aip
                    );
    ASSERT_EFI_ERROR (Status);
    ASSERT (Aip != NULL);

    Status = gBS->HandleProtocol (
                    AipHandleBuffer[AipIndex],
                    &gEfiExtScsiPassThruProtocolGuid,
                    (VOID *) &ExtScsiPassThru
                    );
    if (EFI_ERROR (Status) || ExtScsiPassThru == NULL) {
      continue;
    }

    InfoTypesBuffer     = NULL;
    InfoTypeBufferCount = 0;
    Status = Aip->GetSupportedTypes (Aip, &InfoTypesBuffer, &InfoTypeBufferCount);
    if (EFI_ERROR (Status) || InfoTypesBuffer == NULL) {
      continue;
    }
    //
    // Check whether the AIP instance has Network boot information block.
    //
    Supported = FALSE;
    for (TypeIndex = 0; TypeIndex < InfoTypeBufferCount; TypeIndex++) {
      if (CompareGuid (&InfoTypesBuffer[TypeIndex], &gEfiAdapterInfoNetworkBootGuid)) {
        Supported = TRUE;
        break;
      }
    }

    FreePool (InfoTypesBuffer);
    if (!Supported) {
      continue;
    }

    //
    // We now have network boot information block.
    //
    InfoBlock     = NULL;
    InfoBlockSize = 0;
    Status = Aip->GetInformation (Aip, &gEfiAdapterInfoNetworkBootGuid, &InfoBlock, &InfoBlockSize);
    if (EFI_ERROR (Status) || InfoBlock == NULL) {
      continue;
    }

    //
    // Check whether the network boot policy matches.
    //
    NetworkBoot = (EFI_ADAPTER_INFO_NETWORK_BOOT *) InfoBlock;
    NetworkBootPolicy = PcdGet8 (PcdIScsiAIPNetworkBootPolicy);

    if (NetworkBootPolicy == STOP_UEFI_ISCSI_IF_HBA_INSTALL_AIP) {
      Status = EFI_SUCCESS;
      goto Exit;
    }
    if (((NetworkBootPolicy & STOP_UEFI_ISCSI_IF_AIP_SUPPORT_IP4) != 0 &&
         !NetworkBoot->iScsiIpv4BootCapablity) ||
         ((NetworkBootPolicy & STOP_UEFI_ISCSI_IF_AIP_SUPPORT_IP6) != 0 &&
         !NetworkBoot->iScsiIpv6BootCapablity) ||
         ((NetworkBootPolicy & STOP_UEFI_ISCSI_IF_AIP_SUPPORT_OFFLOAD) != 0 &&
         !NetworkBoot->OffloadCapability) ||
         ((NetworkBootPolicy & STOP_UEFI_ISCSI_IF_AIP_SUPPORT_MPIO) != 0 &&
         !NetworkBoot->iScsiMpioCapability) ||
         ((NetworkBootPolicy & STOP_UEFI_ISCSI_IF_AIP_CONFIGURED_IP4) != 0 &&
         !NetworkBoot->iScsiIpv4Boot) ||
         ((NetworkBootPolicy & STOP_UEFI_ISCSI_IF_AIP_CONFIGURED_IP6) != 0 &&
         !NetworkBoot->iScsiIpv6Boot)) {
      FreePool (InfoBlock);
      continue;
    }

    Status = EFI_SUCCESS;
    goto Exit;
  }

  Status = EFI_NOT_FOUND;

Exit:
  if (InfoBlock != NULL) {
    FreePool (InfoBlock);
  }
  if (AipHandleBuffer != NULL) {
    FreePool (AipHandleBuffer);
  }
  return Status;
}

/**
  Tests to see if this driver supports a given controller. This is the worker function for
  IScsiIp4(6)DriverBindingSupported.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
IScsiSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL,
  IN UINT8                        IpVersion
  )
{
  EFI_STATUS                Status;
  EFI_GUID                  *IScsiServiceBindingGuid;
  EFI_GUID                  *TcpServiceBindingGuid;
  EFI_GUID                  *DhcpServiceBindingGuid;
  EFI_GUID                  *DnsServiceBindingGuid;

  if (IpVersion == IP_VERSION_4) {
    IScsiServiceBindingGuid  = &gIScsiV4PrivateGuid;
    TcpServiceBindingGuid    = &gEfiTcp4ServiceBindingProtocolGuid;
    DhcpServiceBindingGuid   = &gEfiDhcp4ServiceBindingProtocolGuid;
    DnsServiceBindingGuid    = &gEfiDns4ServiceBindingProtocolGuid;

  } else {
    IScsiServiceBindingGuid  = &gIScsiV6PrivateGuid;
    TcpServiceBindingGuid    = &gEfiTcp6ServiceBindingProtocolGuid;
    DhcpServiceBindingGuid   = &gEfiDhcp6ServiceBindingProtocolGuid;
    DnsServiceBindingGuid    = &gEfiDns6ServiceBindingProtocolGuid;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  IScsiServiceBindingGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  TcpServiceBindingGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = IScsiIsDevicePathSupported (RemainingDevicePath);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (IScsiDhcpIsConfigured (ControllerHandle, IpVersion)) {
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    DhcpServiceBindingGuid,
                    NULL,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  if (IScsiDnsIsConfigured (ControllerHandle)) {
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    DnsServiceBindingGuid,
                    NULL,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}


/**
  Start to manage the controller. This is the worker function for
  IScsiIp4(6)DriverBindingStart.

  @param[in]  Image                Handle of the image.
  @param[in]  ControllerHandle     Handle of the controller.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_SUCCES            This driver was started.
  @retval EFI_ALREADY_STARTED   This driver is already running on this device.
  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval EFI_NOT_FOUND         There is no sufficient information to establish
                                the iScsi session.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_DEVICE_ERROR      Failed to get TCP connection device path.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the Handle
                                because its interfaces are being used.

**/
EFI_STATUS
IScsiStart (
  IN EFI_HANDLE                   Image,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINT8                        IpVersion
  )
{
  EFI_STATUS                      Status;
  ISCSI_DRIVER_DATA               *Private;
  LIST_ENTRY                      *Entry;
  LIST_ENTRY                      *NextEntry;
  ISCSI_ATTEMPT_CONFIG_NVDATA     *AttemptConfigData;
  ISCSI_SESSION                   *Session;
  UINT8                           Index;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *ExistIScsiExtScsiPassThru;
  ISCSI_DRIVER_DATA               *ExistPrivate;
  UINT8                           *AttemptConfigOrder;
  UINTN                           AttemptConfigOrderSize;
  UINT8                           BootSelected;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           NumberOfHandles;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_GUID                        *IScsiPrivateGuid;
  EFI_GUID                        *TcpServiceBindingGuid;
  BOOLEAN                         NeedUpdate;
  VOID                            *Interface;
  EFI_GUID                        *ProtocolGuid;
  UINT8                           NetworkBootPolicy;
  ISCSI_SESSION_CONFIG_NVDATA     *NvData;

  //
  // Test to see if iSCSI driver supports the given controller.
  //

  if (IpVersion == IP_VERSION_4) {
    IScsiPrivateGuid      = &gIScsiV4PrivateGuid;
    TcpServiceBindingGuid = &gEfiTcp4ServiceBindingProtocolGuid;
    ProtocolGuid          = &gEfiTcp4ProtocolGuid;
  } else if (IpVersion == IP_VERSION_6) {
    IScsiPrivateGuid      = &gIScsiV6PrivateGuid;
    TcpServiceBindingGuid = &gEfiTcp6ServiceBindingProtocolGuid;
    ProtocolGuid          = &gEfiTcp6ProtocolGuid;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  IScsiPrivateGuid,
                  NULL,
                  Image,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  TcpServiceBindingGuid,
                  NULL,
                  Image,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  NetworkBootPolicy = PcdGet8 (PcdIScsiAIPNetworkBootPolicy);
  if (NetworkBootPolicy == ALWAYS_USE_ISCSI_HBA_AND_IGNORE_UEFI_ISCSI) {
    return EFI_ABORTED;
  }

  if (NetworkBootPolicy != ALWAYS_USE_UEFI_ISCSI_AND_IGNORE_ISCSI_HBA) {
    //
    // Check existing iSCSI AIP.
    //
    Status = IScsiCheckAip ();
    if (!EFI_ERROR (Status)) {
      //
      // Find iSCSI AIP with specified network boot policy. return EFI_ABORTED.
      //
      return EFI_ABORTED;
    }
  }

  //
  // Record the incoming NIC info.
  //
  Status = IScsiAddNic (ControllerHandle, Image);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create the instance private data.
  //
  Private = IScsiCreateDriverData (Image, ControllerHandle);
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create a underlayer child instance, but not need to configure it. Just open ChildHandle
  // via BY_DRIVER. That is, establishing the relationship between ControllerHandle and ChildHandle.
  // Therefore, when DisconnectController(), especially VLAN virtual controller handle,
  // IScsiDriverBindingStop() will be called.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             Image,
             TcpServiceBindingGuid,
             &Private->ChildHandle
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->ChildHandle, /// Default Tcp child
                  ProtocolGuid,
                  &Interface,
                  Image,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Always install private protocol no matter what happens later. We need to
  // keep the relationship between ControllerHandle and ChildHandle.
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  IScsiPrivateGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->IScsiIdentifier
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (IpVersion == IP_VERSION_4) {
    mPrivate->Ipv6Flag = FALSE;
  } else {
    mPrivate->Ipv6Flag = TRUE;
  }

  //
  // Get the current iSCSI configuration data.
  //
  Status = IScsiGetConfigData (Private);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // If there is already a successul attempt, check whether this attempt is the
  // first "enabled for MPIO" attempt. If not, still try the first attempt.
  // In single path mode, try all attempts.
  //
  ExistPrivate = NULL;
  Status       = EFI_NOT_FOUND;

  if (mPrivate->OneSessionEstablished && mPrivate->EnableMpio) {
    AttemptConfigData = NULL;
    NET_LIST_FOR_EACH (Entry, &mPrivate->AttemptConfigs) {
     AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
      if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO) {
        break;
      }
    }

    if (AttemptConfigData == NULL) {
      goto ON_ERROR;
    }

    if (AttemptConfigData->AttemptConfigIndex == mPrivate->BootSelectedIndex) {
      goto ON_EXIT;
    }

    //
    // Uninstall the original ExtScsiPassThru first.
    //

    //
    // Locate all ExtScsiPassThru protocol instances.
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiExtScsiPassThruProtocolGuid,
                    NULL,
                    &NumberOfHandles,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Find ExtScsiPassThru protocol instance produced by this driver.
    //
    ExistIScsiExtScsiPassThru = NULL;
    for (Index = 0; Index < NumberOfHandles && ExistIScsiExtScsiPassThru == NULL; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **) &DevicePath
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      while (!IsDevicePathEnd (DevicePath)) {
        if ((DevicePath->Type == MESSAGING_DEVICE_PATH) && (DevicePath->SubType == MSG_MAC_ADDR_DP)) {
          //
          // Get the ExtScsiPassThru protocol instance.
          //
          Status = gBS->HandleProtocol (
                          HandleBuffer[Index],
                          &gEfiExtScsiPassThruProtocolGuid,
                          (VOID **) &ExistIScsiExtScsiPassThru
                          );
          ASSERT_EFI_ERROR (Status);
          break;
        }

        DevicePath = NextDevicePathNode (DevicePath);
      }
    }

    FreePool (HandleBuffer);

    if (ExistIScsiExtScsiPassThru == NULL) {
      Status = EFI_NOT_FOUND;
      goto ON_ERROR;
    }

    ExistPrivate = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (ExistIScsiExtScsiPassThru);

    Status = gBS->UninstallProtocolInterface (
                    ExistPrivate->ExtScsiPassThruHandle,
                    &gEfiExtScsiPassThruProtocolGuid,
                    &ExistPrivate->IScsiExtScsiPassThru
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  //
  // Install the Ext SCSI PASS THRU protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &Private->ExtScsiPassThruHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->IScsiExtScsiPassThru
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  BootSelected = 0;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &mPrivate->AttemptConfigs) {
    AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    //
    // Don't process the attempt that does not associate with the current NIC or
    // this attempt is disabled or established.
    //
    if (AttemptConfigData->NicIndex != mPrivate->CurrentNic ||
        AttemptConfigData->SessionConfigData.Enabled == ISCSI_DISABLED ||
        AttemptConfigData->ValidPath) {
      continue;
    }

    //
    // In multipath mode, don't process attempts configured for single path.
    // In default single path mode, don't process attempts configured for multipath.
    //
    if ((mPrivate->EnableMpio &&
         AttemptConfigData->SessionConfigData.Enabled != ISCSI_ENABLED_FOR_MPIO) ||
        (!mPrivate->EnableMpio &&
         AttemptConfigData->SessionConfigData.Enabled != ISCSI_ENABLED)) {
      continue;
    }

    //
    // Don't process the attempt that fails to get the init/target information from DHCP.
    //
    if (AttemptConfigData->SessionConfigData.InitiatorInfoFromDhcp &&
        !AttemptConfigData->DhcpSuccess) {
      if (!mPrivate->EnableMpio && mPrivate->ValidSinglePathCount > 0) {
        mPrivate->ValidSinglePathCount--;
      }
      continue;
    }

    //
    // Don't process the autoconfigure path if it is already established.
    //
    if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG &&
        AttemptConfigData->AutoConfigureSuccess) {
      continue;
    }

    //
    // Don't process the attempt if its IP mode is not in the current IP version.
    //
    if (!mPrivate->Ipv6Flag) {
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP6) {
        continue;
      }
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG &&
          AttemptConfigData->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP6) {
        continue;
      }
    } else {
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP4) {
        continue;
      }
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG &&
          AttemptConfigData->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP4) {
        continue;
      }
    }

    //
    // Fill in the Session and init it.
    //
    Session = (ISCSI_SESSION *) AllocateZeroPool (sizeof (ISCSI_SESSION));
    if (Session == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    Session->Private    = Private;
    Session->ConfigData = AttemptConfigData;
    Session->AuthType   = AttemptConfigData->AuthenticationType;

    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"Attempt %d",
      (UINTN) AttemptConfigData->AttemptConfigIndex
      );

    if (Session->AuthType == ISCSI_AUTH_TYPE_CHAP) {
      Session->AuthData.CHAP.AuthConfig = &AttemptConfigData->AuthConfigData.CHAP;
    }

    IScsiSessionInit (Session, FALSE);

    //
    // Try to login and create an iSCSI session according to the configuration.
    //
    Status = IScsiSessionLogin (Session);
    if (Status == EFI_MEDIA_CHANGED) {
      //
      // The specified target is not available, and the redirection information is
      // received. Login the session again with the updated target address.
      //
      Status = IScsiSessionLogin (Session);
    } else if (Status == EFI_NOT_READY) {
      Status = IScsiSessionReLogin (Session);
    }

    //
    // Restore the origial user setting which specifies the proxy/virtual iSCSI target to NV region.
    //
    NvData = &AttemptConfigData->SessionConfigData;
    if (NvData->RedirectFlag) {
      NvData->TargetPort = NvData->OriginalTargetPort;
      CopyMem (&NvData->TargetIp, &NvData->OriginalTargetIp, sizeof (EFI_IP_ADDRESS));
      NvData->RedirectFlag = FALSE;

      gRT->SetVariable (
             mPrivate->PortString,
             &gEfiIScsiInitiatorNameProtocolGuid,
             ISCSI_CONFIG_VAR_ATTR,
             sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
             AttemptConfigData
             );
    }

    if (EFI_ERROR (Status)) {
      //
      // In Single path mode, only the successful attempt will be recorded in iBFT;
      // in multi-path mode, all the attempt entries in MPIO will be recorded in iBFT.
      //
      if (!mPrivate->EnableMpio && mPrivate->ValidSinglePathCount > 0) {
        mPrivate->ValidSinglePathCount--;
      }

      FreePool (Session);

    } else {
      AttemptConfigData->ValidPath = TRUE;

      //
      // Do not record the attempt in iBFT if it login with KRB5.
      // TODO: record KRB5 attempt information in the iSCSI device path.
      //
      if (Session->AuthType == ISCSI_AUTH_TYPE_KRB) {
        if (!mPrivate->EnableMpio && mPrivate->ValidSinglePathCount > 0) {
          mPrivate->ValidSinglePathCount--;
        }

        AttemptConfigData->ValidiBFTPath = FALSE;
      } else {
        AttemptConfigData->ValidiBFTPath = TRUE;
      }

      //
      // IScsi session success. Update the attempt state to NVR.
      //
      if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG) {
        AttemptConfigData->AutoConfigureSuccess = TRUE;
      }

      gRT->SetVariable (
             mPrivate->PortString,
             &gEfiIScsiInitiatorNameProtocolGuid,
             ISCSI_CONFIG_VAR_ATTR,
             sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
             AttemptConfigData
             );

      //
      // Select the first login session. Abort others.
      //
      if (Private->Session == NULL) {
        Private->Session = Session;
        BootSelected     = AttemptConfigData->AttemptConfigIndex;
        //
        // Don't validate other attempt in multipath mode if one is success.
        //
        if (mPrivate->EnableMpio) {
          break;
        }
      } else {
        IScsiSessionAbort (Session);
        FreePool (Session);
      }
    }
  }

  //
  // All attempts configured for this driver instance are not valid.
  //
  if (Private->Session == NULL) {
    Status = gBS->UninstallProtocolInterface (
                    Private->ExtScsiPassThruHandle,
                    &gEfiExtScsiPassThruProtocolGuid,
                    &Private->IScsiExtScsiPassThru
                    );
    ASSERT_EFI_ERROR (Status);
    Private->ExtScsiPassThruHandle = NULL;

    //
    // Reinstall the original ExtScsiPassThru back.
    //
    if (mPrivate->OneSessionEstablished && ExistPrivate != NULL) {
      Status = gBS->InstallProtocolInterface (
                      &ExistPrivate->ExtScsiPassThruHandle,
                      &gEfiExtScsiPassThruProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &ExistPrivate->IScsiExtScsiPassThru
                      );
      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }

      goto ON_EXIT;
    }

    Status = EFI_NOT_FOUND;

    goto ON_ERROR;
  }

  NeedUpdate = TRUE;
  //
  // More than one attempt successes.
  //
  if (Private->Session != NULL && mPrivate->OneSessionEstablished) {

    AttemptConfigOrder = IScsiGetVariableAndSize (
                           L"AttemptOrder",
                           &gIScsiConfigGuid,
                           &AttemptConfigOrderSize
                           );
    if (AttemptConfigOrder == NULL) {
      goto ON_ERROR;
    }
    for (Index = 0; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {
      if (AttemptConfigOrder[Index] == mPrivate->BootSelectedIndex ||
          AttemptConfigOrder[Index] == BootSelected) {
        break;
      }
    }

    if (mPrivate->EnableMpio) {
      //
      // Use the attempt in earlier order. Abort the later one in MPIO.
      //
      if (AttemptConfigOrder[Index] == mPrivate->BootSelectedIndex) {
        IScsiSessionAbort (Private->Session);
        FreePool (Private->Session);
        Private->Session = NULL;
        gBS->UninstallProtocolInterface (
               Private->ExtScsiPassThruHandle,
               &gEfiExtScsiPassThruProtocolGuid,
               &Private->IScsiExtScsiPassThru
               );
        Private->ExtScsiPassThruHandle = NULL;

        //
        // Reinstall the original ExtScsiPassThru back.
        //
        Status = gBS->InstallProtocolInterface (
                        &ExistPrivate->ExtScsiPassThruHandle,
                        &gEfiExtScsiPassThruProtocolGuid,
                        EFI_NATIVE_INTERFACE,
                        &ExistPrivate->IScsiExtScsiPassThru
                        );
        if (EFI_ERROR (Status)) {
          goto ON_ERROR;
        }

        goto ON_EXIT;
      } else {
        if (AttemptConfigOrder[Index] != BootSelected) {
          goto ON_ERROR;
        }
        mPrivate->BootSelectedIndex = BootSelected;
        //
        // Clear the resource in ExistPrivate.
        //
        gBS->UninstallProtocolInterface (
               ExistPrivate->Controller,
               IScsiPrivateGuid,
               &ExistPrivate->IScsiIdentifier
               );

        IScsiRemoveNic (ExistPrivate->Controller);
        if (ExistPrivate->Session != NULL) {
          IScsiSessionAbort (ExistPrivate->Session);
        }

        if (ExistPrivate->DevicePath != NULL) {
          Status = gBS->UninstallProtocolInterface (
                          ExistPrivate->ExtScsiPassThruHandle,
                          &gEfiDevicePathProtocolGuid,
                          ExistPrivate->DevicePath
                          );
          if (EFI_ERROR (Status)) {
            goto ON_ERROR;
          }

          FreePool (ExistPrivate->DevicePath);
        }

        gBS->CloseEvent (ExistPrivate->ExitBootServiceEvent);
        FreePool (ExistPrivate);

      }
    } else {
      //
      // Use the attempt in earlier order as boot selected in single path mode.
      //
      if (AttemptConfigOrder[Index] == mPrivate->BootSelectedIndex) {
        NeedUpdate = FALSE;
      }
    }

  }

  if (NeedUpdate) {
    mPrivate->OneSessionEstablished = TRUE;
    mPrivate->BootSelectedIndex     = BootSelected;
  }

  //
  // Duplicate the Session's tcp connection device path. The source port field
  // will be set to zero as one iSCSI session is comprised of several iSCSI
  // connections.
  //
  Private->DevicePath = IScsiGetTcpConnDevicePath (Private->Session);
  if (Private->DevicePath == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_ERROR;
  }
  //
  // Install the updated device path onto the ExtScsiPassThruHandle.
  //
  Status = gBS->InstallProtocolInterface (
                  &Private->ExtScsiPassThruHandle,
                  &gEfiDevicePathProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  Private->DevicePath
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // ISCSI children should share the default Tcp child, just open the default Tcp child via BY_CHILD_CONTROLLER.
  //
  Status = gBS->OpenProtocol (
                  Private->ChildHandle, /// Default Tcp child
                  ProtocolGuid,
                  &Interface,
                  Image,
                  Private->ExtScsiPassThruHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Private->ExtScsiPassThruHandle,
           &gEfiExtScsiPassThruProtocolGuid,
           &Private->IScsiExtScsiPassThru,
           &gEfiDevicePathProtocolGuid,
           Private->DevicePath,
           NULL
           );

    goto ON_ERROR;
  }

ON_EXIT:

  //
  // Update/Publish the iSCSI Boot Firmware Table.
  //
  if (mPrivate->BootSelectedIndex != 0) {
    IScsiPublishIbft ();
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (Private->Session != NULL) {
    IScsiSessionAbort (Private->Session);
  }

  return Status;
}

/**
  Stops a device controller or a bus controller. This is the worker function for
  IScsiIp4(6)DriverBindingStop.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.
  @param[in]  IpVersion         IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the Handle
                                because its interfaces are being used.

**/
EFI_STATUS
EFIAPI
IScsiStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL,
  IN UINT8                        IpVersion
  )
{
  EFI_HANDLE                      IScsiController;
  EFI_STATUS                      Status;
  ISCSI_PRIVATE_PROTOCOL          *IScsiIdentifier;
  ISCSI_DRIVER_DATA               *Private;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  ISCSI_CONNECTION                *Conn;
  EFI_GUID                        *ProtocolGuid;
  EFI_GUID                        *TcpServiceBindingGuid;
  EFI_GUID                        *TcpProtocolGuid;


  if (NumberOfChildren != 0) {
    //
    // We should have only one child.
    //
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[0],
                    &gEfiExtScsiPassThruProtocolGuid,
                    (VOID **) &PassThru,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Private = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (PassThru);
    Conn    = NET_LIST_HEAD (&Private->Session->Conns, ISCSI_CONNECTION, Link);

    //
    // Previously the TCP protocol is opened BY_CHILD_CONTROLLER. Just close
    // the protocol here, but do not uninstall the device path protocol and
    // EXT SCSI PASS THRU protocol installed on ExtScsiPassThruHandle.
    //
    if (IpVersion == IP_VERSION_4) {
      ProtocolGuid = &gEfiTcp4ProtocolGuid;
    } else {
      ProtocolGuid = &gEfiTcp6ProtocolGuid;
    }

    gBS->CloseProtocol (
           Private->ChildHandle,
           ProtocolGuid,
           Private->Image,
           Private->ExtScsiPassThruHandle
           );

    gBS->CloseProtocol (
           Conn->TcpIo.Handle,
           ProtocolGuid,
           Private->Image,
           Private->ExtScsiPassThruHandle
           );

    return EFI_SUCCESS;
  }

  //
  // Get the handle of the controller we are controling.
  //
  if (IpVersion == IP_VERSION_4) {
    ProtocolGuid            = &gIScsiV4PrivateGuid;
    TcpProtocolGuid         = &gEfiTcp4ProtocolGuid;
    TcpServiceBindingGuid   = &gEfiTcp4ServiceBindingProtocolGuid;
  } else {
    ProtocolGuid            = &gIScsiV6PrivateGuid;
    TcpProtocolGuid         = &gEfiTcp6ProtocolGuid;
    TcpServiceBindingGuid   = &gEfiTcp6ServiceBindingProtocolGuid;
  }
  IScsiController = NetLibGetNicHandle (ControllerHandle, TcpProtocolGuid);
  if (IScsiController == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  IScsiController,
                  ProtocolGuid,
                  (VOID **) &IScsiIdentifier,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private = ISCSI_DRIVER_DATA_FROM_IDENTIFIER (IScsiIdentifier);
  ASSERT (Private != NULL);

  if (Private->ChildHandle != NULL) {
    Status = gBS->CloseProtocol (
                    Private->ChildHandle,
                    TcpProtocolGuid,
                    This->DriverBindingHandle,
                    IScsiController
                    );

    ASSERT (!EFI_ERROR (Status));

    Status = NetLibDestroyServiceChild (
               IScsiController,
               This->DriverBindingHandle,
               TcpServiceBindingGuid,
               Private->ChildHandle
               );

    ASSERT (!EFI_ERROR (Status));
  }

  gBS->UninstallProtocolInterface (
         IScsiController,
         ProtocolGuid,
         &Private->IScsiIdentifier
         );

  //
  // Remove this NIC.
  //
  IScsiRemoveNic (IScsiController);

  //
  // Update the iSCSI Boot Firware Table.
  //
  IScsiPublishIbft ();

  if (Private->Session != NULL) {
    IScsiSessionAbort (Private->Session);
  }

  Status = IScsiCleanDriverData (Private);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
IScsiIp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return IScsiSupported (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_4
           );
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error. Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
IScsiIp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;

  Status = IScsiStart (This->DriverBindingHandle, ControllerHandle, IP_VERSION_4);
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
IScsiIp4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return IScsiStop (
           This,
           ControllerHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_4
           );
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
IScsiIp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return IScsiSupported (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_6
           );
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error. Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
IScsiIp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;

  Status = IScsiStart (This->DriverBindingHandle, ControllerHandle, IP_VERSION_6);
  if (Status == EFI_ALREADY_STARTED) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
IScsiIp6DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return IScsiStop (
           This,
           ControllerHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_6
           );
}

/**
  Unload the iSCSI driver.

  @param[in]  ImageHandle          The handle of the driver image.

  @retval     EFI_SUCCESS          The driver is unloaded.
  @retval     EFI_DEVICE_ERROR     An unexpected error occurred.

**/
EFI_STATUS
EFIAPI
IScsiUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                        Status;
  UINTN                             DeviceHandleCount;
  EFI_HANDLE                        *DeviceHandleBuffer;
  UINTN                             Index;
  EFI_COMPONENT_NAME_PROTOCOL       *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL      *ComponentName2;

  //
  // Try to disonnect the driver from the devices it's controlling.
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Disconnect the iSCSI4 driver from the controlled device.
  //
  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = IScsiTestManagedDevice (
               DeviceHandleBuffer[Index],
               gIScsiIp4DriverBinding.DriverBindingHandle,
               &gEfiTcp4ProtocolGuid)
               ;
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = gBS->DisconnectController (
                    DeviceHandleBuffer[Index],
                    gIScsiIp4DriverBinding.DriverBindingHandle,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Disconnect the iSCSI6 driver from the controlled device.
  //
  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = IScsiTestManagedDevice (
               DeviceHandleBuffer[Index],
               gIScsiIp6DriverBinding.DriverBindingHandle,
               &gEfiTcp6ProtocolGuid
               );
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = gBS->DisconnectController (
                    DeviceHandleBuffer[Index],
                    gIScsiIp6DriverBinding.DriverBindingHandle,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Unload the iSCSI configuration form.
  //
  Status = IScsiConfigFormUnload (gIScsiIp4DriverBinding.DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Uninstall the protocols installed by iSCSI driver.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiAuthenticationInfoProtocolGuid,
                  &gIScsiAuthenticationInfo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  if (gIScsiControllerNameTable!= NULL) {
    Status = FreeUnicodeStringTable (gIScsiControllerNameTable);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    gIScsiControllerNameTable = NULL;
  }

  //
  // Uninstall the ComponentName and ComponentName2 protocol from iSCSI4 driver binding handle
  // if it has been installed.
  //
  Status = gBS->HandleProtocol (
                  gIScsiIp4DriverBinding.DriverBindingHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **) &ComponentName
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
           gIScsiIp4DriverBinding.DriverBindingHandle,
           &gEfiComponentNameProtocolGuid,
           ComponentName,
           NULL
           );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  Status = gBS->HandleProtocol (
                  gIScsiIp4DriverBinding.DriverBindingHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **) &ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           gIScsiIp4DriverBinding.DriverBindingHandle,
           &gEfiComponentName2ProtocolGuid,
           ComponentName2,
           NULL
           );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Uninstall the ComponentName and ComponentName2 protocol from iSCSI6 driver binding handle
  // if it has been installed.
  //
  Status = gBS->HandleProtocol (
                  gIScsiIp6DriverBinding.DriverBindingHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **) &ComponentName
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
           gIScsiIp6DriverBinding.DriverBindingHandle,
           &gEfiComponentNameProtocolGuid,
           ComponentName,
           NULL
           );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  Status = gBS->HandleProtocol (
                  gIScsiIp6DriverBinding.DriverBindingHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **) &ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           gIScsiIp6DriverBinding.DriverBindingHandle,
           &gEfiComponentName2ProtocolGuid,
           ComponentName2,
           NULL
           );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Uninstall the IScsiInitiatorNameProtocol and all the driver binding protocols.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gIScsiIp4DriverBinding.DriverBindingHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gIScsiIp4DriverBinding,
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  &gIScsiInitiatorName,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gIScsiIp6DriverBinding.DriverBindingHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gIScsiIp6DriverBinding,
                  NULL
                  );

ON_EXIT:

  if (DeviceHandleBuffer != NULL) {
    FreePool (DeviceHandleBuffer);
  }

  return Status;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  The entry point for iSCSI driver which initializes the global variables and
  installs the driver binding, component name protocol, iSCSI initiator name
  protocol and Authentication Info protocol on its image.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
IScsiDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                         Status;
  EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *IScsiInitiatorName;
  EFI_AUTHENTICATION_INFO_PROTOCOL   *AuthenticationInfo;

  //
  // There should be only one EFI_ISCSI_INITIATOR_NAME_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  NULL,
                  (VOID **) &IScsiInitiatorName
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Initialize the EFI Driver Library.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIScsiIp4DriverBinding,
             ImageHandle,
             &gIScsiComponentName,
             &gIScsiComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIScsiIp6DriverBinding,
             NULL,
             &gIScsiComponentName,
             &gIScsiComponentName2
             );
  if (EFI_ERROR (Status)) {
    goto Error1;
  }

  //
  // Install the iSCSI Initiator Name Protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gIScsiInitiatorName
                  );
  if (EFI_ERROR (Status)) {
    goto Error2;
  }

  //
  // Create the private data structures.
  //
  mPrivate = AllocateZeroPool (sizeof (ISCSI_PRIVATE_DATA));
  if (mPrivate == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error3;
  }

  InitializeListHead (&mPrivate->NicInfoList);
  InitializeListHead (&mPrivate->AttemptConfigs);

  //
  // Initialize the configuration form of iSCSI.
  //
  Status = IScsiConfigFormInit (gIScsiIp4DriverBinding.DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    goto Error4;
  }

  //
  // Create the Maximum Attempts.
  //
  Status = IScsiCreateAttempts (PcdGet8 (PcdMaxIScsiAttemptNumber));
  if (EFI_ERROR (Status)) {
    goto Error5;
  }

  //
  // Create Keywords for all the Attempts.
  //
  Status = IScsiCreateKeywords (PcdGet8 (PcdMaxIScsiAttemptNumber));
  if (EFI_ERROR (Status)) {
    goto Error6;
  }

  //
  // There should be only one EFI_AUTHENTICATION_INFO_PROTOCOL. If already exists,
  // do not produce the protocol instance.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAuthenticationInfoProtocolGuid,
                  NULL,
                  (VOID **) &AuthenticationInfo
                  );
  if (Status == EFI_NOT_FOUND) {
    Status = gBS->InstallProtocolInterface (
                    &ImageHandle,
                    &gEfiAuthenticationInfoProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gIScsiAuthenticationInfo
                    );
    if (EFI_ERROR (Status)) {
      goto Error6;
    }
  }

  return EFI_SUCCESS;

Error6:
  IScsiCleanAttemptVariable ();

Error5:
  IScsiConfigFormUnload (gIScsiIp4DriverBinding.DriverBindingHandle);

Error4:
  if (mPrivate != NULL) {
    FreePool (mPrivate);
    mPrivate = NULL;
  }

Error3:
  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEfiIScsiInitiatorNameProtocolGuid,
         &gIScsiInitiatorName,
         NULL
         );

Error2:
  EfiLibUninstallDriverBindingComponentName2 (
    &gIScsiIp6DriverBinding,
    &gIScsiComponentName,
    &gIScsiComponentName2
    );

Error1:
  EfiLibUninstallDriverBindingComponentName2 (
    &gIScsiIp4DriverBinding,
    &gIScsiComponentName,
    &gIScsiComponentName2
    );

  return Status;
}


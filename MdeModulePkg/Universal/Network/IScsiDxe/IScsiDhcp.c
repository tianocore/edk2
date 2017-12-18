/** @file
  iSCSI DHCP related configuration routines.

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

/**
  Extract the Root Path option and get the required target information.

  @param[in]        RootPath         The RootPath.
  @param[in]        Length           Length of the RootPath option payload.
  @param[in, out]   ConfigNvData     The iSCSI session configuration data read from nonvolatile device.

  @retval EFI_SUCCESS           All required information is extracted from the RootPath option.
  @retval EFI_NOT_FOUND         The RootPath is not an iSCSI RootPath.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER The RootPath is mal-formatted.
**/
EFI_STATUS
IScsiDhcpExtractRootPath (
  IN      CHAR8                        *RootPath,
  IN      UINT8                        Length,
  IN OUT  ISCSI_SESSION_CONFIG_NVDATA  *ConfigNvData
  )
{
  EFI_STATUS            Status;
  UINT8                 IScsiRootPathIdLen;
  CHAR8                 *TmpStr;
  ISCSI_ROOT_PATH_FIELD Fields[RP_FIELD_IDX_MAX];
  ISCSI_ROOT_PATH_FIELD *Field;
  UINT32                FieldIndex;
  UINT8                 Index;

  //
  // "iscsi:"<servername>":"<protocol>":"<port>":"<LUN>":"<targetname>
  //
  IScsiRootPathIdLen = (UINT8) AsciiStrLen (ISCSI_ROOT_PATH_ID);

  if ((Length <= IScsiRootPathIdLen) || (CompareMem (RootPath, ISCSI_ROOT_PATH_ID, IScsiRootPathIdLen) != 0)) {
    return EFI_NOT_FOUND;
  }
  //
  // Skip the iSCSI RootPath ID "iscsi:".
  //
  RootPath += IScsiRootPathIdLen;
  Length  = (UINT8) (Length - IScsiRootPathIdLen);

  TmpStr  = (CHAR8 *) AllocatePool (Length + 1);
  if (TmpStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TmpStr, RootPath, Length);
  TmpStr[Length]  = '\0';

  Index           = 0;
  FieldIndex      = RP_FIELD_IDX_SERVERNAME;
  ZeroMem (&Fields[0], sizeof (Fields));

  //
  // Extract the fields in the Root Path option string.
  //
  for (FieldIndex = RP_FIELD_IDX_SERVERNAME; (FieldIndex < RP_FIELD_IDX_MAX) && (Index < Length); FieldIndex++) {
    if (TmpStr[Index] != ISCSI_ROOT_PATH_FIELD_DELIMITER) {
      Fields[FieldIndex].Str = &TmpStr[Index];
    }

    while ((TmpStr[Index] != ISCSI_ROOT_PATH_FIELD_DELIMITER) && (Index < Length)) {
      Index++;
    }

    if (TmpStr[Index] == ISCSI_ROOT_PATH_FIELD_DELIMITER) {
      if (FieldIndex != RP_FIELD_IDX_TARGETNAME) {
        TmpStr[Index] = '\0';
        Index++;
      }

      if (Fields[FieldIndex].Str != NULL) {
        Fields[FieldIndex].Len = (UINT8) AsciiStrLen (Fields[FieldIndex].Str);
      }
    }
  }

  if (FieldIndex != RP_FIELD_IDX_MAX) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  
    if ((Fields[RP_FIELD_IDX_SERVERNAME].Str == NULL) ||
      (Fields[RP_FIELD_IDX_TARGETNAME].Str == NULL) ||
      (Fields[RP_FIELD_IDX_PROTOCOL].Len > 1)
      ) {

    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  //
  // Get the IP address of the target.
  //
  Field   = &Fields[RP_FIELD_IDX_SERVERNAME];
  Status  = IScsiAsciiStrToIp (Field->Str, &ConfigNvData->TargetIp);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Check the protocol type.
  //
  Field = &Fields[RP_FIELD_IDX_PROTOCOL];
  if ((Field->Str != NULL) && ((*(Field->Str) - '0') != EFI_IP_PROTO_TCP)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  //
  // Get the port of the iSCSI target.
  //
  Field = &Fields[RP_FIELD_IDX_PORT];
  if (Field->Str != NULL) {
    ConfigNvData->TargetPort = (UINT16) AsciiStrDecimalToUintn (Field->Str);
  } else {
    ConfigNvData->TargetPort = ISCSI_WELL_KNOWN_PORT;
  }
  //
  // Get the LUN.
  //
  Field = &Fields[RP_FIELD_IDX_LUN];
  if (Field->Str != NULL) {
    Status = IScsiAsciiStrToLun (Field->Str, ConfigNvData->BootLun);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  } else {
    ZeroMem (ConfigNvData->BootLun, sizeof (ConfigNvData->BootLun));
  }
  //
  // Get the target iSCSI Name.
  //
  Field = &Fields[RP_FIELD_IDX_TARGETNAME];

  if (AsciiStrLen (Field->Str) > ISCSI_NAME_MAX_SIZE - 1) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  //
  // Validate the iSCSI name.
  //
  Status = IScsiNormalizeName (Field->Str, AsciiStrLen (Field->Str));
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  AsciiStrCpyS (ConfigNvData->TargetName, ISCSI_NAME_MAX_SIZE, Field->Str);

ON_EXIT:

  FreePool (TmpStr);

  return Status;
}

/**
  The callback function registerd to the DHCP4 instance which is used to select
  the qualified DHCP OFFER.
  
  @param[in]  This         The DHCP4 protocol.
  @param[in]  Context      The context set when configuring the DHCP4 protocol.
  @param[in]  CurrentState The current state of the DHCP4 protocol.
  @param[in]  Dhcp4Event   The event occurs in the current state.
  @param[in]  Packet       The DHCP packet that is to be sent or already received. 
  @param[out] NewPacket    The packet used to replace the above Packet.
  
  @retval EFI_SUCCESS      Either the DHCP OFFER is qualified or we're not intereseted
                           in the Dhcp4Event.
  @retval EFI_NOT_READY    The DHCP OFFER packet doesn't match our requirements.
  @retval Others           Other errors as indicated.
**/
EFI_STATUS
EFIAPI
IScsiDhcpSelectOffer (
  IN  EFI_DHCP4_PROTOCOL  * This,
  IN  VOID                *Context,
  IN  EFI_DHCP4_STATE     CurrentState,
  IN  EFI_DHCP4_EVENT     Dhcp4Event,
  IN  EFI_DHCP4_PACKET    * Packet, OPTIONAL
  OUT EFI_DHCP4_PACKET    **NewPacket OPTIONAL
  )
{
  EFI_STATUS              Status;
  UINT32                  OptionCount;
  EFI_DHCP4_PACKET_OPTION **OptionList;
  UINT32                  Index;

  if ((Dhcp4Event != Dhcp4RcvdOffer) && (Dhcp4Event != Dhcp4SelectOffer)) {
    return EFI_SUCCESS;
  }

  OptionCount = 0;

  Status      = This->Parse (This, Packet, &OptionCount, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_READY;
  }

  OptionList = AllocatePool (OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *));
  if (OptionList == NULL) {
    return EFI_NOT_READY;
  }

  Status = This->Parse (This, Packet, &OptionCount, OptionList);
  if (EFI_ERROR (Status)) {
    FreePool (OptionList);
    return EFI_NOT_READY;
  }

  for (Index = 0; Index < OptionCount; Index++) {
    if (OptionList[Index]->OpCode != DHCP4_TAG_ROOTPATH) {
      continue;
    }

    Status = IScsiDhcpExtractRootPath (
              (CHAR8 *) &OptionList[Index]->Data[0],
              OptionList[Index]->Length,
              (ISCSI_SESSION_CONFIG_NVDATA *) Context
              );

    break;
  }

  if (Index == OptionCount) {
    Status = EFI_NOT_READY;
  }

  FreePool (OptionList);

  return Status;
}

/**
  Parse the DHCP ACK to get the address configuration and DNS information.

  @param[in]       Dhcp4        The DHCP4 protocol.
  @param[in, out]  ConfigData   The session configuration data.

  @retval EFI_SUCCESS           The DNS information is got from the DHCP ACK.
  @retval EFI_NO_MAPPING        DHCP failed to acquire address and other information.
  @retval EFI_INVALID_PARAMETER The DHCP ACK's DNS option is mal-formatted.
  @retval EFI_DEVICE_ERROR      Other errors as indicated.
**/
EFI_STATUS
IScsiParseDhcpAck (
  IN      EFI_DHCP4_PROTOCOL         *Dhcp4,
  IN OUT  ISCSI_SESSION_CONFIG_DATA  *ConfigData
  )
{
  EFI_STATUS              Status;
  EFI_DHCP4_MODE_DATA     Dhcp4ModeData;
  UINT32                  OptionCount;
  EFI_DHCP4_PACKET_OPTION **OptionList;
  UINT32                  Index;

  Status = Dhcp4->GetModeData (Dhcp4, &Dhcp4ModeData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Dhcp4ModeData.State != Dhcp4Bound) {
    return EFI_NO_MAPPING;
  }

  CopyMem (&ConfigData->NvData.LocalIp, &Dhcp4ModeData.ClientAddress, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&ConfigData->NvData.SubnetMask, &Dhcp4ModeData.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&ConfigData->NvData.Gateway, &Dhcp4ModeData.RouterAddress, sizeof (EFI_IPv4_ADDRESS));

  OptionCount = 0;
  OptionList  = NULL;

  Status      = Dhcp4->Parse (Dhcp4, Dhcp4ModeData.ReplyPacket, &OptionCount, OptionList);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_DEVICE_ERROR;
  }

  OptionList = AllocatePool (OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *));
  if (OptionList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Dhcp4->Parse (Dhcp4, Dhcp4ModeData.ReplyPacket, &OptionCount, OptionList);
  if (EFI_ERROR (Status)) {
    FreePool (OptionList);
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < OptionCount; Index++) {
    //
    // Get DNS server addresses and DHCP server address from this offer.
    //
    if (OptionList[Index]->OpCode == DHCP4_TAG_DNS_SERVER) {

      if (((OptionList[Index]->Length & 0x3) != 0) || (OptionList[Index]->Length == 0)) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }
      //
      // Primary DNS server address.
      //
      CopyMem (&ConfigData->PrimaryDns, &OptionList[Index]->Data[0], sizeof (EFI_IPv4_ADDRESS));

      if (OptionList[Index]->Length > 4) {
        //
        // Secondary DNS server address
        //
        CopyMem (&ConfigData->SecondaryDns, &OptionList[Index]->Data[4], sizeof (EFI_IPv4_ADDRESS));
      }
    } else if (OptionList[Index]->OpCode == DHCP4_TAG_SERVER_ID) {
      if (OptionList[Index]->Length != 4) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      CopyMem (&ConfigData->DhcpServer, &OptionList[Index]->Data[0], sizeof (EFI_IPv4_ADDRESS));
    }
  }

  FreePool (OptionList);

  return Status;
}

/**
  Parse the DHCP ACK to get the address configuration and DNS information.
  
  @param[in]       Image            The handle of the driver image.
  @param[in]       Controller       The handle of the controller;
  @param[in, out]  ConfigData       The session configuration data.

  @retval EFI_SUCCESS           The DNS information is got from the DHCP ACK.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_NO_MEDIA          There was a media error.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
IScsiDoDhcp (
  IN     EFI_HANDLE                 Image,
  IN     EFI_HANDLE                 Controller,
  IN OUT ISCSI_SESSION_CONFIG_DATA  *ConfigData
  )
{
  EFI_HANDLE              Dhcp4Handle;
  EFI_DHCP4_PROTOCOL      *Dhcp4;
  EFI_STATUS              Status;
  EFI_DHCP4_PACKET_OPTION *ParaList;
  EFI_DHCP4_CONFIG_DATA   Dhcp4ConfigData;
  EFI_STATUS              MediaStatus;
  UINT8                   *Data;

  Dhcp4Handle = NULL;
  Dhcp4       = NULL;
  ParaList    = NULL;

  //
  // Check media status before do DHCP
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Controller, ISCSI_CHECK_MEDIA_GET_DHCP_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    return EFI_NO_MEDIA;
  }

  //
  // Create a DHCP4 child instance and get the protocol.
  //
  Status = NetLibCreateServiceChild (
            Controller,
            Image,
            &gEfiDhcp4ServiceBindingProtocolGuid,
            &Dhcp4Handle
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Dhcp4Handle,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **)&Dhcp4,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  ParaList = AllocatePool (sizeof (EFI_DHCP4_PACKET_OPTION) + 3);
  if (ParaList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  //
  // Ask the server to reply with Netmask, Router, DNS and RootPath options.
  //
  ParaList->OpCode  = DHCP4_TAG_PARA_LIST;
  ParaList->Length  = (UINT8) (ConfigData->NvData.TargetInfoFromDhcp ? 4 : 3);
  Data = &ParaList->Data[0];
  Data[0] = DHCP4_TAG_NETMASK;
  Data[1] = DHCP4_TAG_ROUTER;
  Data[2] = DHCP4_TAG_DNS_SERVER;
  Data[3] = DHCP4_TAG_ROOTPATH;

  ZeroMem (&Dhcp4ConfigData, sizeof (EFI_DHCP4_CONFIG_DATA));
  Dhcp4ConfigData.OptionCount = 1;
  Dhcp4ConfigData.OptionList  = &ParaList;

  if (ConfigData->NvData.TargetInfoFromDhcp) {
    //
    // Use callback to select an offer which contains target information.
    //
    Dhcp4ConfigData.Dhcp4Callback   = IScsiDhcpSelectOffer;
    Dhcp4ConfigData.CallbackContext = &ConfigData->NvData;
  }

  Status = Dhcp4->Configure (Dhcp4, &Dhcp4ConfigData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Dhcp4->Start (Dhcp4, NULL);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Parse the ACK to get required information.
  //
  Status = IScsiParseDhcpAck (Dhcp4, ConfigData);

ON_EXIT:

  if (ParaList != NULL) {
    FreePool (ParaList);
  }

  if (Dhcp4 != NULL) {
    Dhcp4->Stop (Dhcp4);
    Dhcp4->Configure (Dhcp4, NULL);

    gBS->CloseProtocol (
          Dhcp4Handle,
          &gEfiDhcp4ProtocolGuid,
          Image,
          Controller
          );
  }

  NetLibDestroyServiceChild (
    Controller,
    Image,
    &gEfiDhcp4ServiceBindingProtocolGuid,
    Dhcp4Handle
    );

  return Status;
}

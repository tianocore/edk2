/** @file
  The IScsi's EFI_EXT_SCSI_PASS_THRU_PROTOCOL driver

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiExtScsiPassThru.c

Abstract:
  The IScsi's EFI_EXT_SCSI_PASS_THRU_PROTOCOL driver

**/

#include "IScsiImpl.h"

/**
  This function sends out the SCSI command via iSCSI transport layer and returned
  back the data received from the iSCSI target. 

  @param  This[in]        The EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.

  @param  Target[in]      The Target ID of device to send the SCSI Request Packet. 

  @param  Lun[in]         The LUN of the device to send the SCSI Request Packet.

  @param  Packet[in][out] The SCSI Request Packet to send to the device.

  @param  Event[in]       The event used in non-blocking mode, it should be always NULL.

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruFunction (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                          *This,
  IN UINT8                                                    *Target,
  IN UINT64                                                   Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET           *Packet,
  IN EFI_EVENT                                                Event     OPTIONAL
  )
{
  if (Target[0] != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet == NULL) || (Packet->Cdb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return IScsiExecuteScsiCommand (This, Target, Lun, Packet);
}

/**
  Retrieve the list of legal Target IDs for SCSI devices on a SCSI channel.

  @param  This[in]              The EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.

  @param  Target[in][out]       On input, a pointer to the Target ID of a SCSI device present on the
                                SCSI channel. On output, a pointer to the Target ID of the next SCSI
                                device present on a SCSI channel. An input value of 0xFFFFFFFF retrieves
                                the Target ID of the first SCSI device present on a SCSI channel.

  @param  Lun[in][out]          On input, a pointer to the LUN of a SCSI device present on the SCSI
                                channel. On output, a pointer to the LUN of the next SCSI device
                                present on a SCSI channel.

  @retval EFI_SUCCESS           The Target ID and Lun of the next SCSI device 
                                on the SCSI channel was returned in Target and Lun.

  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

  @retval EFI_INVALID_PARAMETER Target is not 0xFFFFFFFF,and Target and Lun were not
                                returned on a previous call to GetNextDevice().

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **Target,
  IN OUT UINT64                       *Lun
  )
{
  ISCSI_DRIVER_DATA           *Private;
  ISCSI_SESSION_CONFIG_NVDATA *ConfigNvData;
  UINT8                       TargetId[TARGET_MAX_BYTES];

  Private       = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (This);
  ConfigNvData  = &Private->Session.ConfigData.NvData;

  if ((*Target)[0] == 0 && (CompareMem (Lun, ConfigNvData->BootLun, sizeof (UINT64)) == 0)) {
    //
    // Only one <Target, Lun> pair per iSCSI Driver instance.
    //
    return EFI_NOT_FOUND;
  }

  SetMem (TargetId, TARGET_MAX_BYTES, 0xFF);
  if (CompareMem (*Target, TargetId, TARGET_MAX_BYTES) == 0) {
    (*Target)[0] = 0;
    CopyMem (Lun, ConfigNvData->BootLun, sizeof (UINT64));

    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Allocate and build a device path node for a SCSI device on a SCSI channel.

  @param  This[in]              Protocol instance pointer.

  @param  Target[in]            The Target ID of the SCSI device for which
                                a device path node is to be allocated and built.

  @param  Lun[in]               The LUN of the SCSI device for which a device 
                                path node is to be allocated and built.

  @param  DevicePath[in][out]   A pointer to a single device path node that 
                                describes the SCSI device specified by 
                                Target and Lun. This function is responsible 
                                for allocating the buffer DevicePath with the boot
                                service AllocatePool().  It is the caller's 
                                responsibility to free DevicePath when the caller
                                is finished with DevicePath.    

  @retval EFI_SUCCESS           The device path node that describes the SCSI device
                                specified by Target and Lun was allocated and 
                                returned in DevicePath.

  @retval EFI_NOT_FOUND         The SCSI devices specified by Target and Lun does
                                not exist on the SCSI channel.

  @retval EFI_INVALID_PARAMETER DevicePath is NULL.

  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to allocate 
                                DevicePath.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **DevicePath
  )
{
  ISCSI_DRIVER_DATA             *Private;
  ISCSI_SESSION                 *Session;
  ISCSI_SESSION_CONFIG_NVDATA   *ConfigNvData;
  ISCSI_CHAP_AUTH_CONFIG_NVDATA *AuthConfig;
  EFI_DEV_PATH                  *Node;
  UINTN                         DevPathNodeLen;

  if ((DevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Target[0] != 0) {
    return EFI_NOT_FOUND;
  }

  Private       = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (This);
  Session       = &Private->Session;
  ConfigNvData  = &Session->ConfigData.NvData;
  AuthConfig    = &Session->AuthData.AuthConfig;

  if (CompareMem (&Lun, ConfigNvData->BootLun, sizeof (UINT64)) != 0) {
    return EFI_NOT_FOUND;
  }

  DevPathNodeLen  = sizeof (ISCSI_DEVICE_PATH) + AsciiStrLen (ConfigNvData->TargetName) + 1;
  Node            = AllocatePool (DevPathNodeLen);
  if (Node == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
  Node->DevPath.SubType = MSG_ISCSI_DP;
  SetDevicePathNodeLength (&Node->DevPath, (UINT16)DevPathNodeLen);

  //
  // 0 for TCP, others are reserved.
  //
  Node->Iscsi.NetworkProtocol = 0;

  Node->Iscsi.LoginOption     = 0;
  switch (AuthConfig->CHAPType) {
  case ISCSI_CHAP_NONE:
    Node->Iscsi.LoginOption |= 0x0800;
    break;

  case ISCSI_CHAP_UNI:
    Node->Iscsi.LoginOption |= 0x1000;
    break;

  default:
    break;
  }

  CopyMem (&Node->Iscsi.Lun, ConfigNvData->BootLun, sizeof (UINT64));
  Node->Iscsi.TargetPortalGroupTag = Session->TargetPortalGroupTag;
  AsciiStrCpy ((CHAR8 *) Node + sizeof (ISCSI_DEVICE_PATH), ConfigNvData->TargetName);

  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Node;

  return EFI_SUCCESS;
}

/**
  Translate a device path node to a Target ID and LUN.

  @param  This[in]              Protocol instance pointer.

  @param  DevicePath[in]        A pointer to the device path node that 
                                describes a SCSI device on the SCSI channel.

  @param  Target[out]           A pointer to the Target ID of a SCSI device 
                                on the SCSI channel. 

  @param  Lun[out]              A pointer to the LUN of a SCSI device on 
                                the SCSI channel.    

  @retval EFI_SUCCESS           DevicePath was successfully translated to a 
                                Target ID and LUN, and they were returned 
                                in Target and Lun.

  @retval EFI_INVALID_PARAMETER DevicePath/Target/Lun is NULL.

  @retval EFI_UNSUPPORTED       This driver does not support the device path 
                                node type in DevicePath.

  @retval EFI_NOT_FOUND         A valid translation from DevicePath to a 
                                Target ID and LUN does not exist.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruGetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT UINT8                           **Target,
  OUT UINT64                          *Lun
  )
{
  ISCSI_DRIVER_DATA           *Private;
  ISCSI_SESSION_CONFIG_NVDATA *ConfigNvData;

  if ((DevicePath == NULL) || (Target == NULL) || (Lun == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePath->Type != MESSAGING_DEVICE_PATH) ||
      (DevicePath->SubType != MSG_ISCSI_DP) ||
      (DevicePathNodeLength (DevicePath) <= sizeof (ISCSI_DEVICE_PATH))
      ) {
    return EFI_UNSUPPORTED;
  }

  Private       = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (This);
  ConfigNvData  = &Private->Session.ConfigData.NvData;

  ZeroMem (*Target, TARGET_MAX_BYTES);

  if (AsciiStrCmp (ConfigNvData->TargetName, (CHAR8 *) DevicePath + sizeof (ISCSI_DEVICE_PATH)) != 0) {
    return EFI_UNSUPPORTED;
  }

  CopyMem (Lun, ConfigNvData->BootLun, sizeof (UINT64));

  return EFI_SUCCESS;
}

/**
  Resets a SCSI channel.This operation resets all the SCSI devices connected to
  the SCSI channel.

  @param  This[in]        Protocol instance pointer.

  @retval EFI_UNSUPPORTED It's not supported.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Resets a SCSI device that is connected to a SCSI channel.

  @param  This[in]        Protocol instance pointer.

  @param  Target[in]      The Target ID of the SCSI device to reset.

  @param  Lun[in]         The LUN of the SCSI device to reset.

  @retval EFI_UNSUPPORTED It's not supported.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Retrieve the list of legal Target IDs for SCSI devices on a SCSI channel.

  @param  This[in]              Protocol instance pointer.

  @param  Target[in]            On input, a pointer to the Target ID of a SCSI 
                                device present on the SCSI channel.  On output, 
                                a pointer to the Target ID of the next SCSI device
                                present on a SCSI channel.  An input value of 
                                0xFFFFFFFF retrieves the Target ID of the first 
                                SCSI device present on a SCSI channel.

  @retval EFI_SUCCESS           The Target ID and Lun of the next SCSI device 
                                on the SCSI channel was returned in Target and Lun.

  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

  @retval EFI_INVALID_PARAMETER Target is not 0xFFFFFFFF,and Target and Lun were not
                                returned on a previous call to GetNextDevice().

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **Target
  )
{
  UINT8 TargetId[TARGET_MAX_BYTES];

  SetMem (TargetId, TARGET_MAX_BYTES, 0xFF);

  if (CompareMem (*Target, TargetId, TARGET_MAX_BYTES) == 0) {
    (*Target)[0] = 0;
    return EFI_SUCCESS;
  } else if ((*Target)[0] == 0) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

EFI_EXT_SCSI_PASS_THRU_PROTOCOL gIScsiExtScsiPassThruProtocolTemplate = {
  NULL,
  IScsiExtScsiPassThruFunction,
  IScsiExtScsiPassThruGetNextTargetLun,
  IScsiExtScsiPassThruBuildDevicePath,
  IScsiExtScsiPassThruGetTargetLun,
  IScsiExtScsiPassThruResetChannel,
  IScsiExtScsiPassThruResetTargetLun,
  IScsiExtScsiPassThruGetNextTarget
};

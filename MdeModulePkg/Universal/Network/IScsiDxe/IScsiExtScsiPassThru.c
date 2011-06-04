/** @file
  The IScsi's EFI_EXT_SCSI_PASS_THRU_PROTOCOL driver.

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

/**
  Sends a SCSI Request Packet to a SCSI device that is attached to the SCSI channel. This function   
  supports both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the
  nonblocking I/O functionality is optional.                                                             

  @param[in]      This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in]      Target  The Target is an array of size TARGET_MAX_BYTES and it represents
                          the id of the SCSI device to send the SCSI Request Packet. Each
                          transport driver may chose to utilize a subset of this size to suit the needs
                          of transport target representation. For example, a Fibre Channel driver
                          may use only 8 bytes (WWN) to represent an FC target.
  @param[in]       Lun    The LUN of the SCSI device to send the SCSI Request Packet.
  @param[in, out]  Packet A pointer to the SCSI Request Packet to send to the SCSI device
                          specified by Target and Lun.
  @param[in]       Event  If nonblocking I/O is not supported then Event is ignored, and blocking
                          I/O is performed. If Event is NULL, then blocking I/O is performed. If
                          Event is not NULL and non blocking I/O is supported, then
                          nonblocking I/O is performed, and Event will be signaled when the
                          SCSI Request Packet completes.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For bi-directional
                                commands, InTransferLength bytes were transferred from
                                InDataBuffer. For write and bi-directional commands,
                                OutTransferLength bytes were transferred by
                                OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE   The SCSI Request Packet was not executed. The number of bytes that
                                could be transferred is returned in InTransferLength. For write
                                and bi-directional commands, OutTransferLength bytes were
                                transferred by OutDataBuffer. Currently not implemeted.                                
  @retval EFI_NOT_READY         The SCSI Request Packet could not be sent because there are too many
                                SCSI Request Packets already queued. The caller may retry again later.                             
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet. 
  @retval EFI_INVALID_PARAMETER Target, Lun, or the contents of ScsiRequestPacket are invalid.
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet is not supported
                                by the host adapter. This includes the case of Bi-directional SCSI
                                commands not supported by the implementation. The SCSI Request
                                Packet was not sent, so no additional status information is available.
                                Currently not implemeted.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.                              
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
  ISCSI_DRIVER_DATA           *Private;
  ISCSI_SESSION_CONFIG_NVDATA *ConfigNvData;

  Private       = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (This);
  ConfigNvData  = &Private->Session.ConfigData.NvData;

  if (Target[0] != 0 || (CompareMem (&Lun, ConfigNvData->BootLun, sizeof (UINT64)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet == NULL) || (Packet->Cdb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return IScsiExecuteScsiCommand (This, Target, Lun, Packet);
}

/**
  Used to retrieve the list of legal Target IDs and LUNs for SCSI devices on a SCSI channel. These       
  can either be the list SCSI devices that are actually present on the SCSI channel, or the list of legal
  Target Ids and LUNs for the SCSI channel. Regardless, the caller of this function must probe the       
  Target ID and LUN returned to see if a SCSI device is actually present at that location on the SCSI    
  channel.                                                                                               

  @param[in]       This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in, out]  Target  On input, a pointer to the Target ID (an array of size
                           TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                           On output, a pointer to the Target ID (an array of
                           TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                           channel. An input value of 0xF(all bytes in the array are 0xF) in the
                           Target array retrieves the Target ID of the first SCSI device present on a
                           SCSI channel.
  @param[in, out]  Lun     On input, a pointer to the LUN of a SCSI device present on the SCSI
                           channel. On output, a pointer to the LUN of the next SCSI device present
                           on a SCSI channel.

  @retval EFI_SUCCESS           The Target ID and LUN of the next SCSI device on the SCSI
                                channel was returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER Target array is not all 0xF, and Target and Lun were
                                not returned on a previous call to GetNextTargetLun().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.
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
  Used to allocate and build a device path node for a SCSI device on a SCSI channel.

  @param[in]      This        A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in]      Target      The Target is an array of size TARGET_MAX_BYTES and it specifies the
                              Target ID of the SCSI device for which a device path node is to be
                              allocated and built. Transport drivers may chose to utilize a subset of
                              this size to suit the representation of targets. For example, a Fibre
                              Channel driver may use only 8 bytes (WWN) in the array to represent a
                              FC target.
  @param[in]       Lun        The LUN of the SCSI device for which a device path node is to be
                              allocated and built.
  @param[in, out]  DevicePath A pointer to a single device path node that describes the SCSI device
                              specified by Target and Lun. This function is responsible for
                              allocating the buffer DevicePath with the boot service
                              AllocatePool(). It is the caller's responsibility to free
                              DevicePath when the caller is finished with DevicePath.

  @retval EFI_SUCCESS           The device path node that describes the SCSI device specified by
                                Target and Lun was allocated and returned in
                                DevicePath.
  @retval EFI_INVALID_PARAMETER DevicePath is NULL.
  @retval EFI_NOT_FOUND         The SCSI devices specified by Target and Lun does not exist
                                on the SCSI channel.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to allocate DevicePath.

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

  if (DevicePath == NULL) {
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
  Used to translate a device path node to a Target ID and LUN.

  @param[in]  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in]  DevicePath A pointer to a single device path node that describes the SCSI device
                         on the SCSI channel.
  @param[out] Target     A pointer to the Target Array which represents the ID of a SCSI device
                         on the SCSI channel.
  @param[out]  Lun       A pointer to the LUN of a SCSI device on the SCSI channel.

  @retval EFI_SUCCESS           DevicePath was successfully translated to a Target ID and
                                LUN, and they were returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER DevicePath or Target or Lun is NULL.
  @retval EFI_NOT_FOUND         A valid translation from DevicePath to a Target ID and LUN
                                does not exist.Currently not implemented.
  @retval EFI_UNSUPPORTED       This driver does not support the device path node type in
                                DevicePath.
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

  SetMem (*Target, TARGET_MAX_BYTES, 0xFF);
  (*Target)[0] = 0;

  if (AsciiStrCmp (ConfigNvData->TargetName, (CHAR8 *) DevicePath + sizeof (ISCSI_DEVICE_PATH)) != 0) {
    return EFI_UNSUPPORTED;
  }

  CopyMem (Lun, ConfigNvData->BootLun, sizeof (UINT64));

  return EFI_SUCCESS;
}

/**
  Resets a SCSI channel. This operation resets all the SCSI devices connected to the SCSI channel.
  Currently not implemented.
  
  @param[in]  This A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.

  @retval EFI_SUCCESS      The SCSI channel was reset.
  @retval EFI_DEVICE_ERROR A device error occurred while attempting to reset the SCSI channel.
  @retval EFI_TIMEOUT      A timeout occurred while attempting to reset the SCSI channel.
  @retval EFI_UNSUPPORTED  The SCSI channel does not support a channel reset operation.
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
  Resets a SCSI logical unit that is connected to a SCSI channel. Currently not implemented.

  @param[in]  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in]  Target The Target is an array of size TARGET_MAX_BYTE and it represents the
                     target port ID of the SCSI device containing the SCSI logical unit to
                     reset. Transport drivers may chose to utilize a subset of this array to suit
                     the representation of their targets.
  @param[in]  Lun    The LUN of the SCSI device to reset.

  @retval EFI_SUCCESS           The SCSI device specified by Target and Lun was reset.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           A timeout occurred while attempting to reset the SCSI device
                                specified by Target and Lun.
  @retval EFI_UNSUPPORTED       The SCSI channel does not support a target reset operation.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to reset the SCSI device
                                specified by Target and Lun.

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
  Used to retrieve the list of legal Target IDs for SCSI devices on a SCSI channel. These can either     
  be the list SCSI devices that are actually present on the SCSI channel, or the list of legal Target IDs
  for the SCSI channel. Regardless, the caller of this function must probe the Target ID returned to     
  see if a SCSI device is actually present at that location on the SCSI channel.                         

  @param[in]       This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in, out]  Target  (TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                           On output, a pointer to the Target ID (an array of
                           TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                           channel. An input value of 0xF(all bytes in the array are 0xF) in the
                           Target array retrieves the Target ID of the first SCSI device present on a
                           SCSI channel.

  @retval EFI_SUCCESS           The Target ID of the next SCSI device on the SCSI
                                channel was returned in Target.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           Target array is not all 0xF, and Target were not
                                returned on a previous call to GetNextTarget().
                                Currently not implemented.
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.
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

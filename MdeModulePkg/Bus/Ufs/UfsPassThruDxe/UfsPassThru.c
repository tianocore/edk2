/** @file

  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UfsPassThru.h"

//
// Template for Ufs Pass Thru private data.
//
UFS_PASS_THRU_PRIVATE_DATA  gUfsPassThruTemplate = {
  UFS_PASS_THRU_SIG,                                                                                                                      // Signature
  NULL,                                                                                                                                   // Handle
  {                               // ExtScsiPassThruMode
    0xFFFFFFFF,
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL | EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL | EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_NONBLOCKIO,
    sizeof (UINTN)
  },
  {                               // ExtScsiPassThru
    NULL,
    UfsPassThruPassThru,
    UfsPassThruGetNextTargetLun,
    UfsPassThruBuildDevicePath,
    UfsPassThruGetTargetLun,
    UfsPassThruResetChannel,
    UfsPassThruResetTargetLun,
    UfsPassThruGetNextTarget
  },
  {                               // UfsDevConfig
    UfsRwUfsDescriptor,
    UfsRwUfsFlag,
    UfsRwUfsAttribute
  },
  0,                                                                                                                                               // UfsHostController
  0,                                                                                                                                               // UfsHcBase
  { 0,                                                                                                                                    0     }, // UfsHcInfo
  { NULL,                                                                                                                                 NULL  }, // UfsHcDriverInterface
  0,                                                                                                                                               // TaskTag
  0,                                                                                                                                               // UtpTrlBase
  0,                                                                                                                                               // Nutrs
  0,                                                                                                                                               // TrlMapping
  0,                                                                                                                                               // UtpTmrlBase
  0,                                                                                                                                               // Nutmrs
  0,                                                                                                                                               // TmrlMapping
  {                               // Luns
    {
      UFS_LUN_0,                      // Ufs Common Lun 0
      UFS_LUN_1,                      // Ufs Common Lun 1
      UFS_LUN_2,                      // Ufs Common Lun 2
      UFS_LUN_3,                      // Ufs Common Lun 3
      UFS_LUN_4,                      // Ufs Common Lun 4
      UFS_LUN_5,                      // Ufs Common Lun 5
      UFS_LUN_6,                      // Ufs Common Lun 6
      UFS_LUN_7,                      // Ufs Common Lun 7
      UFS_WLUN_REPORT_LUNS,           // Ufs Reports Luns Well Known Lun
      UFS_WLUN_UFS_DEV,               // Ufs Device Well Known Lun
      UFS_WLUN_BOOT,                  // Ufs Boot Well Known Lun
      UFS_WLUN_RPMB                   // RPMB Well Known Lun
    },
    0x0000,                                                                                                                               // By default don't expose any Luns.
    0x0
  },
  NULL,                                                                                                                                   // TimerEvent
  {                               // Queue
    NULL,
    NULL
  }
};

EFI_DRIVER_BINDING_PROTOCOL  gUfsPassThruDriverBinding = {
  UfsPassThruDriverBindingSupported,
  UfsPassThruDriverBindingStart,
  UfsPassThruDriverBindingStop,
  0x10,
  NULL,
  NULL
};

UFS_DEVICE_PATH  mUfsDevicePathTemplate = {
  {
    MESSAGING_DEVICE_PATH,
    MSG_UFS_DP,
    {
      (UINT8)(sizeof (UFS_DEVICE_PATH)),
      (UINT8)((sizeof (UFS_DEVICE_PATH)) >> 8)
    }
  },
  0,
  0
};

UINT8  mUfsTargetId[TARGET_MAX_BYTES];

GLOBAL_REMOVE_IF_UNREFERENCED EDKII_UFS_HC_PLATFORM_PROTOCOL  *mUfsHcPlatform;

/**
  Sends a SCSI Request Packet to a SCSI device that is attached to the SCSI channel. This function
  supports both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the
  nonblocking I/O functionality is optional.

  @param  This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target  The Target is an array of size TARGET_MAX_BYTES and it represents
                  the id of the SCSI device to send the SCSI Request Packet. Each
                  transport driver may choose to utilize a subset of this size to suit the needs
                  of transport target representation. For example, a Fibre Channel driver
                  may use only 8 bytes (WWN) to represent an FC target.
  @param  Lun     The LUN of the SCSI device to send the SCSI Request Packet.
  @param  Packet  A pointer to the SCSI Request Packet to send to the SCSI device
                  specified by Target and Lun.
  @param  Event   If nonblocking I/O is not supported then Event is ignored, and blocking
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
                                transferred by OutDataBuffer.
  @retval EFI_NOT_READY         The SCSI Request Packet could not be sent because there are too many
                                SCSI Request Packets already queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet.
  @retval EFI_INVALID_PARAMETER Target, Lun, or the contents of ScsiRequestPacket are invalid.
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet is not supported
                                by the host adapter. This includes the case of Bi-directional SCSI
                                commands not supported by the implementation. The SCSI Request
                                Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.

**/
EFI_STATUS
EFIAPI
UfsPassThruPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                 *This,
  IN UINT8                                           *Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN EFI_EVENT                                       Event OPTIONAL
  )
{
  EFI_STATUS                  Status;
  UFS_PASS_THRU_PRIVATE_DATA  *Private;
  UINT8                       UfsLun;
  UINT16                      Index;

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if ((Packet == NULL) || (Packet->Cdb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Don't support variable length CDB
  //
  if ((Packet->CdbLength != 6) && (Packet->CdbLength != 10) &&
      (Packet->CdbLength != 12) && (Packet->CdbLength != 16))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->SenseDataLength != 0) && (Packet->SenseData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !IS_ALIGNED (Packet->InDataBuffer, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !IS_ALIGNED (Packet->OutDataBuffer, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((This->Mode->IoAlign > 1) && !IS_ALIGNED (Packet->SenseData, This->Mode->IoAlign)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For UFS 2.0 compatible device, 0 is always used to represent the location of the UFS device.
  //
  SetMem (mUfsTargetId, TARGET_MAX_BYTES, 0x00);
  if ((Target == NULL) || (CompareMem (Target, mUfsTargetId, TARGET_MAX_BYTES) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // UFS 2.0 spec Section 10.6.7 - Translation of 8-bit UFS LUN to 64-bit SCSI LUN Address
  // 0xC1 in the first 8 bits of the 64-bit address indicates a well known LUN address in the SAM SCSI format.
  // The second 8 bits of the 64-bit address saves the corresponding 8-bit UFS LUN.
  //
  if ((UINT8)Lun == UFS_WLUN_PREFIX) {
    UfsLun = BIT7 | (((UINT8 *)&Lun)[1] & 0xFF);
  } else if ((UINT8)Lun == 0) {
    UfsLun = ((UINT8 *)&Lun)[1] & 0xFF;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < UFS_MAX_LUNS; Index++) {
    if ((Private->Luns.BitMask & (BIT0 << Index)) == 0) {
      continue;
    }

    if (Private->Luns.Lun[Index] == UfsLun) {
      break;
    }
  }

  if (Index == UFS_MAX_LUNS) {
    return EFI_INVALID_PARAMETER;
  }

  Status = UfsExecScsiCmds (Private, UfsLun, Packet, Event);

  return Status;
}

/**
  Used to retrieve the list of legal Target IDs and LUNs for SCSI devices on a SCSI channel. These
  can either be the list SCSI devices that are actually present on the SCSI channel, or the list of legal
  Target Ids and LUNs for the SCSI channel. Regardless, the caller of this function must probe the
  Target ID and LUN returned to see if a SCSI device is actually present at that location on the SCSI
  channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target On input, a pointer to the Target ID (an array of size
                 TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.
  @param  Lun    On input, a pointer to the LUN of a SCSI device present on the SCSI
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
UfsPassThruGetNextTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                         **Target,
  IN OUT UINT64                        *Lun
  )
{
  UFS_PASS_THRU_PRIVATE_DATA  *Private;
  UINT8                       UfsLun;
  UINT16                      Index;
  UINT16                      Next;

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  if ((Target == NULL) || (Lun == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UfsLun = 0;
  SetMem (mUfsTargetId, TARGET_MAX_BYTES, 0xFF);
  if (CompareMem (*Target, mUfsTargetId, TARGET_MAX_BYTES) == 0) {
    //
    // If the array is all 0xFF's, return the first exposed Lun to caller.
    //
    SetMem (*Target, TARGET_MAX_BYTES, 0x00);
    for (Index = 0; Index < UFS_MAX_LUNS; Index++) {
      if ((Private->Luns.BitMask & (BIT0 << Index)) != 0) {
        UfsLun = Private->Luns.Lun[Index];
        break;
      }
    }

    if (Index != UFS_MAX_LUNS) {
      *Lun = 0;
      if ((UfsLun & BIT7) == BIT7) {
        ((UINT8 *)Lun)[0] = UFS_WLUN_PREFIX;
        ((UINT8 *)Lun)[1] = UfsLun & ~BIT7;
      } else {
        ((UINT8 *)Lun)[1] = UfsLun;
      }

      return EFI_SUCCESS;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  SetMem (mUfsTargetId, TARGET_MAX_BYTES, 0x00);
  if (CompareMem (*Target, mUfsTargetId, TARGET_MAX_BYTES) == 0) {
    if (((UINT8 *)Lun)[0] == UFS_WLUN_PREFIX) {
      UfsLun = BIT7 | (((UINT8 *)Lun)[1] & 0xFF);
    } else if (((UINT8 *)Lun)[0] == 0) {
      UfsLun = ((UINT8 *)Lun)[1] & 0xFF;
    } else {
      return EFI_NOT_FOUND;
    }

    for (Index = 0; Index < UFS_MAX_LUNS; Index++) {
      if ((Private->Luns.BitMask & (BIT0 << Index)) == 0) {
        continue;
      }

      if (Private->Luns.Lun[Index] != UfsLun) {
        continue;
      }

      for (Next = Index + 1; Next < UFS_MAX_LUNS; Next++) {
        if ((Private->Luns.BitMask & (BIT0 << Next)) != 0) {
          UfsLun = Private->Luns.Lun[Next];
          break;
        }
      }

      if (Next == UFS_MAX_LUNS) {
        return EFI_NOT_FOUND;
      } else {
        break;
      }
    }

    if (Index != UFS_MAX_LUNS) {
      *Lun = 0;
      if ((UfsLun & BIT7) == BIT7) {
        ((UINT8 *)Lun)[0] = UFS_WLUN_PREFIX;
        ((UINT8 *)Lun)[1] = UfsLun & ~BIT7;
      } else {
        ((UINT8 *)Lun)[1] = UfsLun;
      }

      return EFI_SUCCESS;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Used to allocate and build a device path node for a SCSI device on a SCSI channel.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target     The Target is an array of size TARGET_MAX_BYTES and it specifies the
                     Target ID of the SCSI device for which a device path node is to be
                     allocated and built. Transport drivers may chose to utilize a subset of
                     this size to suit the representation of targets. For example, a Fibre
                     Channel driver may use only 8 bytes (WWN) in the array to represent a
                     FC target.
  @param  Lun        The LUN of the SCSI device for which a device path node is to be
                     allocated and built.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
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
UfsPassThruBuildDevicePath (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN     UINT8                            *Target,
  IN     UINT64                           Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL         **DevicePath
  )
{
  UFS_PASS_THRU_PRIVATE_DATA  *Private;
  EFI_DEV_PATH                *DevicePathNode;
  UINT8                       UfsLun;
  UINT16                      Index;

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  //
  // Validate parameters passed in.
  //
  SetMem (mUfsTargetId, TARGET_MAX_BYTES, 0x00);
  if (CompareMem (Target, mUfsTargetId, TARGET_MAX_BYTES) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT8)Lun == UFS_WLUN_PREFIX) {
    UfsLun = BIT7 | (((UINT8 *)&Lun)[1] & 0xFF);
  } else if ((UINT8)Lun == 0) {
    UfsLun = ((UINT8 *)&Lun)[1] & 0xFF;
  } else {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < UFS_MAX_LUNS; Index++) {
    if ((Private->Luns.BitMask & (BIT0 << Index)) == 0) {
      continue;
    }

    if (Private->Luns.Lun[Index] == UfsLun) {
      break;
    }
  }

  if (Index == UFS_MAX_LUNS) {
    return EFI_NOT_FOUND;
  }

  DevicePathNode = AllocateCopyPool (sizeof (UFS_DEVICE_PATH), &mUfsDevicePathTemplate);
  if (DevicePathNode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DevicePathNode->Ufs.Pun = 0;
  DevicePathNode->Ufs.Lun = UfsLun;

  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePathNode;

  return EFI_SUCCESS;
}

/**
  Used to translate a device path node to a Target ID and LUN.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
                     on the SCSI channel.
  @param  Target     A pointer to the Target Array which represents the ID of a SCSI device
                     on the SCSI channel.
  @param  Lun        A pointer to the LUN of a SCSI device on the SCSI channel.

  @retval EFI_SUCCESS           DevicePath was successfully translated to a Target ID and
                                LUN, and they were returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER DevicePath or Target or Lun is NULL.
  @retval EFI_NOT_FOUND         A valid translation from DevicePath to a Target ID and LUN
                                does not exist.
  @retval EFI_UNSUPPORTED       This driver does not support the device path node type in
                                 DevicePath.

**/
EFI_STATUS
EFIAPI
UfsPassThruGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT UINT8                            **Target,
  OUT UINT64                           *Lun
  )
{
  UFS_PASS_THRU_PRIVATE_DATA  *Private;
  EFI_DEV_PATH                *DevicePathNode;
  UINT8                       Pun;
  UINT8                       UfsLun;
  UINT16                      Index;

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_THIS (This);

  //
  // Validate parameters passed in.
  //
  if ((DevicePath == NULL) || (Target == NULL) || (Lun == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether the DevicePath belongs to UFS_DEVICE_PATH
  //
  if ((DevicePath->Type != MESSAGING_DEVICE_PATH) || (DevicePath->SubType != MSG_UFS_DP) ||
      (DevicePathNodeLength (DevicePath) != sizeof (UFS_DEVICE_PATH)))
  {
    return EFI_UNSUPPORTED;
  }

  DevicePathNode = (EFI_DEV_PATH *)DevicePath;

  Pun    = (UINT8)DevicePathNode->Ufs.Pun;
  UfsLun = (UINT8)DevicePathNode->Ufs.Lun;

  if (Pun != 0) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < UFS_MAX_LUNS; Index++) {
    if ((Private->Luns.BitMask & (BIT0 << Index)) == 0) {
      continue;
    }

    if (Private->Luns.Lun[Index] == UfsLun) {
      break;
    }
  }

  if (Index == UFS_MAX_LUNS) {
    return EFI_NOT_FOUND;
  }

  SetMem (*Target, TARGET_MAX_BYTES, 0x00);
  *Lun = 0;
  if ((UfsLun & BIT7) == BIT7) {
    ((UINT8 *)Lun)[0] = UFS_WLUN_PREFIX;
    ((UINT8 *)Lun)[1] = UfsLun & ~BIT7;
  } else {
    ((UINT8 *)Lun)[1] = UfsLun;
  }

  return EFI_SUCCESS;
}

/**
  Resets a SCSI channel. This operation resets all the SCSI devices connected to the SCSI channel.

  @param  This A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.

  @retval EFI_SUCCESS      The SCSI channel was reset.
  @retval EFI_DEVICE_ERROR A device error occurred while attempting to reset the SCSI channel.
  @retval EFI_TIMEOUT      A timeout occurred while attempting to reset the SCSI channel.
  @retval EFI_UNSUPPORTED  The SCSI channel does not support a channel reset operation.

**/
EFI_STATUS
EFIAPI
UfsPassThruResetChannel (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This
  )
{
  //
  // Return success directly then upper layer driver could think reset channel operation is done.
  //
  return EFI_SUCCESS;
}

/**
  Resets a SCSI logical unit that is connected to a SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target The Target is an array of size TARGET_MAX_BYTE and it represents the
                 target port ID of the SCSI device containing the SCSI logical unit to
                 reset. Transport drivers may chose to utilize a subset of this array to suit
                 the representation of their targets.
  @param  Lun    The LUN of the SCSI device to reset.

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
UfsPassThruResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun
  )
{
  //
  // Return success directly then upper layer driver could think reset target LUN operation is done.
  //
  return EFI_SUCCESS;
}

/**
  Used to retrieve the list of legal Target IDs for SCSI devices on a SCSI channel. These can either
  be the list SCSI devices that are actually present on the SCSI channel, or the list of legal Target IDs
  for the SCSI channel. Regardless, the caller of this function must probe the Target ID returned to
  see if a SCSI device is actually present at that location on the SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target (TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.

  @retval EFI_SUCCESS           The Target ID of the next SCSI device on the SCSI
                                channel was returned in Target.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           Target array is not all 0xF, and Target was not
                                returned on a previous call to GetNextTarget().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
UfsPassThruGetNextTarget (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                         **Target
  )
{
  if ((Target == NULL) || (*Target == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SetMem (mUfsTargetId, TARGET_MAX_BYTES, 0xFF);
  if (CompareMem (*Target, mUfsTargetId, TARGET_MAX_BYTES) == 0) {
    SetMem (*Target, TARGET_MAX_BYTES, 0x00);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
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
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
UfsPassThruDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHostController;

  //
  // Ufs Pass Thru driver is a device driver, and should ingore the
  // "RemainingDevicePath" according to UEFI spec
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID *)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }

  //
  // Close the protocol because we don't use it here
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEdkiiUfsHostControllerProtocolGuid,
                  (VOID **)&UfsHostController,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    //
    // EFI_ALREADY_STARTED is also an error
    //
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEdkiiUfsHostControllerProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

/**
  Finishes device initialization by setting fDeviceInit flag and waiting untill device responds by
  clearing it.

  @param[in] Private  Pointer to the UFS_PASS_THRU_PRIVATE_DATA.

  @retval EFI_SUCCESS  The operation succeeds.
  @retval Others       The operation fails.

**/
EFI_STATUS
UfsFinishDeviceInitialization (
  IN UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceInitStatus;
  UINT32      Timeout;

  DeviceInitStatus = 0xFF;

  //
  // The host enables the device initialization completion by setting fDeviceInit flag.
  //
  Status = UfsSetFlag (Private, UfsFlagDevInit);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // There are cards that can take upto 600ms to clear fDeviceInit flag.
  //
  Timeout = UFS_INIT_COMPLETION_TIMEOUT;
  do {
    Status = UfsReadFlag (Private, UfsFlagDevInit, &DeviceInitStatus);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    MicroSecondDelay (1);
    Timeout--;
  } while (DeviceInitStatus != 0 && Timeout != 0);

  if (Timeout == 0) {
    DEBUG ((DEBUG_ERROR, "UfsFinishDeviceInitialization DeviceInitStatus=%x EFI_TIMEOUT \n", DeviceInitStatus));
    return EFI_TIMEOUT;
  } else {
    DEBUG ((DEBUG_INFO, "UfsFinishDeviceInitialization Timeout left=%x EFI_SUCCESS \n", Timeout));
    return EFI_SUCCESS;
  }
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
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
UfsPassThruDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                             Status;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL     *UfsHc;
  UFS_PASS_THRU_PRIVATE_DATA             *Private;
  UINTN                                  UfsHcBase;
  UINT32                                 Index;
  UFS_UNIT_DESC                          UnitDescriptor;
  UFS_DEV_DESC                           DeviceDescriptor;
  UINT32                                 UnitDescriptorSize;
  UINT32                                 DeviceDescriptorSize;
  EDKII_UFS_CARD_REF_CLK_FREQ_ATTRIBUTE  Attributes;
  UINT8                                  RefClkAttr;

  Status    = EFI_SUCCESS;
  UfsHc     = NULL;
  Private   = NULL;
  UfsHcBase = 0;

  DEBUG ((DEBUG_INFO, "==UfsPassThru Start== Controller = %x\n", Controller));

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEdkiiUfsHostControllerProtocolGuid,
                  (VOID **)&UfsHc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Open Ufs Host Controller Protocol Error, Status = %r\n", Status));
    goto Error;
  }

  //
  // Get the UFS Host Controller MMIO Bar Base Address.
  //
  Status = UfsHc->GetUfsHcMmioBar (UfsHc, &UfsHcBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Get Ufs Host Controller Mmio Bar Error, Status = %r\n", Status));
    goto Error;
  }

  //
  // Initialize Ufs Pass Thru private data for managed UFS Host Controller.
  //
  Private = AllocateCopyPool (sizeof (UFS_PASS_THRU_PRIVATE_DATA), &gUfsPassThruTemplate);
  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "Unable to allocate Ufs Pass Thru private data\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Private->ExtScsiPassThru.Mode                   = &Private->ExtScsiPassThruMode;
  Private->UfsHostController                      = UfsHc;
  Private->UfsHcBase                              = UfsHcBase;
  Private->Handle                                 = Controller;
  Private->UfsHcDriverInterface.UfsHcProtocol     = UfsHc;
  Private->UfsHcDriverInterface.UfsExecUicCommand = UfsHcDriverInterfaceExecUicCommand;
  InitializeListHead (&Private->Queue);

  //
  // This has to be done before initializing UfsHcInfo or calling the UfsControllerInit
  //
  if (mUfsHcPlatform == NULL) {
    Status = gBS->LocateProtocol (&gEdkiiUfsHcPlatformProtocolGuid, NULL, (VOID **)&mUfsHcPlatform);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "No UfsHcPlatformProtocol present\n"));
    }
  }

  Status = GetUfsHcInfo (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialize UfsHcInfo\n"));
    goto Error;
  }

  //
  // Initialize UFS Host Controller H/W.
  //
  Status = UfsControllerInit (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ufs Host Controller Initialization Error, Status = %r\n", Status));
    goto Error;
  }

  //
  // UFS 2.0 spec Section 13.1.3.3:
  // At the end of the UFS Interconnect Layer initialization on both host and device side,
  // the host shall send a NOP OUT UPIU to verify that the device UTP Layer is ready.
  //
  Status = UfsExecNopCmds (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ufs Sending NOP IN command Error, Status = %r\n", Status));
    goto Error;
  }

  Status = UfsFinishDeviceInitialization (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Device failed to finish initialization, Status = %r\n", Status));
    goto Error;
  }

  if ((mUfsHcPlatform != NULL) &&
      ((mUfsHcPlatform->RefClkFreq == EdkiiUfsCardRefClkFreq19p2Mhz) ||
       (mUfsHcPlatform->RefClkFreq == EdkiiUfsCardRefClkFreq26Mhz) ||
       (mUfsHcPlatform->RefClkFreq == EdkiiUfsCardRefClkFreq38p4Mhz)))
  {
    RefClkAttr = UfsAttrRefClkFreq;
    Attributes = EdkiiUfsCardRefClkFreqObsolete;
    Status     = UfsRwAttributes (Private, TRUE, RefClkAttr, 0, 0, (UINT32 *)&Attributes);
    if (!EFI_ERROR (Status)) {
      if (Attributes != mUfsHcPlatform->RefClkFreq) {
        Attributes = mUfsHcPlatform->RefClkFreq;
        DEBUG (
          (DEBUG_INFO,
           "Setting bRefClkFreq attribute(%x) to %x\n  0 -> 19.2 Mhz\n  1 -> 26 Mhz\n  2 -> 38.4 Mhz\n  3 -> Obsolete\n",
           RefClkAttr,
           Attributes)
          );
        Status = UfsRwAttributes (Private, FALSE, RefClkAttr, 0, 0, (UINT32 *)&Attributes);
        if (EFI_ERROR (Status)) {
          DEBUG (
            (DEBUG_ERROR,
             "Failed to Change Reference Clock Attribute to %d, Status = %r \n",
             mUfsHcPlatform->RefClkFreq,
             Status)
            );
        }
      }
    } else {
      DEBUG (
        (DEBUG_ERROR,
         "Failed to Read Reference Clock Attribute, Status = %r \n",
         Status)
        );
    }
  }

  if ((mUfsHcPlatform != NULL) && (mUfsHcPlatform->Callback != NULL)) {
    Status = mUfsHcPlatform->Callback (Private->Handle, EdkiiUfsHcPostLinkStartup, &Private->UfsHcDriverInterface);
    if (EFI_ERROR (Status)) {
      DEBUG (
        (DEBUG_ERROR,
         "Failure from platform driver during EdkiiUfsHcPostLinkStartup, Status = %r\n",
         Status)
        );
      return Status;
    }
  }

  //
  // Check if 8 common luns are active and set corresponding bit mask.
  //
  UnitDescriptorSize = sizeof (UFS_UNIT_DESC);
  for (Index = 0; Index < 8; Index++) {
    Status = UfsRwDeviceDesc (Private, TRUE, UfsUnitDesc, (UINT8)Index, 0, &UnitDescriptor, &UnitDescriptorSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to read unit descriptor, index = %X, status = %r\n", Index, Status));
      continue;
    }

    if (UnitDescriptor.LunEn == 0x1) {
      DEBUG ((DEBUG_INFO, "UFS LUN %X is enabled\n", Index));
      Private->Luns.BitMask |= (BIT0 << Index);
    }
  }

  //
  // Check if RPMB WLUN is supported and set corresponding bit mask.
  //
  DeviceDescriptorSize = sizeof (UFS_DEV_DESC);
  Status               = UfsRwDeviceDesc (Private, TRUE, UfsDeviceDesc, 0, 0, &DeviceDescriptor, &DeviceDescriptorSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to read device descriptor, status = %r\n", Status));
  } else {
    if (DeviceDescriptor.SecurityLun == 0x1) {
      DEBUG ((DEBUG_INFO, "UFS WLUN RPMB is supported\n"));
      Private->Luns.BitMask |= BIT11;
    }
  }

  //
  // Start the asynchronous interrupt monitor
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ProcessAsyncTaskList,
                  Private,
                  &Private->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ufs Create Async Tasks Event Error, Status = %r\n", Status));
    goto Error;
  }

  Status = gBS->SetTimer (
                  Private->TimerEvent,
                  TimerPeriodic,
                  UFS_HC_ASYNC_TIMER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Ufs Set Periodic Timer Error, Status = %r\n", Status));
    goto Error;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &(Private->ExtScsiPassThru),
                  &gEfiUfsDeviceConfigProtocolGuid,
                  &(Private->UfsDevConfig),
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;

Error:
  if (Private != NULL) {
    if (Private->TmrlMapping != NULL) {
      UfsHc->Unmap (UfsHc, Private->TmrlMapping);
    }

    if (Private->UtpTmrlBase != NULL) {
      UfsHc->FreeBuffer (UfsHc, EFI_SIZE_TO_PAGES (Private->Nutmrs * sizeof (UTP_TMRD)), Private->UtpTmrlBase);
    }

    if (Private->TrlMapping != NULL) {
      UfsHc->Unmap (UfsHc, Private->TrlMapping);
    }

    if (Private->UtpTrlBase != NULL) {
      UfsHc->FreeBuffer (UfsHc, EFI_SIZE_TO_PAGES (Private->Nutrs * sizeof (UTP_TMRD)), Private->UtpTrlBase);
    }

    if (Private->TimerEvent != NULL) {
      gBS->CloseEvent (Private->TimerEvent);
    }

    FreePool (Private);
  }

  if (UfsHc != NULL) {
    gBS->CloseProtocol (
           Controller,
           &gEdkiiUfsHostControllerProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
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
UfsPassThruDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                          Status;
  UFS_PASS_THRU_PRIVATE_DATA          *Private;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL     *ExtScsiPassThru;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;
  UFS_PASS_THRU_TRANS_REQ             *TransReq;
  LIST_ENTRY                          *Entry;
  LIST_ENTRY                          *NextEntry;

  DEBUG ((DEBUG_INFO, "==UfsPassThru Stop== Controller Controller = %x\n", Controller));

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiExtScsiPassThruProtocolGuid,
                  (VOID **)&ExtScsiPassThru,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_THIS (ExtScsiPassThru);
  UfsHc   = Private->UfsHostController;

  //
  // Cleanup the resources of I/O requests in the async I/O queue
  //
  if (!IsListEmpty (&Private->Queue)) {
    BASE_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->Queue) {
      TransReq = UFS_PASS_THRU_TRANS_REQ_FROM_THIS (Entry);

      //
      // TODO: Should find/add a proper host adapter return status for this
      // case.
      //
      TransReq->Packet->HostAdapterStatus =
        EFI_EXT_SCSI_STATUS_HOST_ADAPTER_PHASE_ERROR;

      SignalCallerEvent (Private, TransReq);
    }
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &(Private->ExtScsiPassThru),
                  &gEfiUfsDeviceConfigProtocolGuid,
                  &(Private->UfsDevConfig),
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Stop Ufs Host Controller
  //
  Status = UfsControllerStop (Private);
  ASSERT_EFI_ERROR (Status);

  if (Private->TmrlMapping != NULL) {
    UfsHc->Unmap (UfsHc, Private->TmrlMapping);
  }

  if (Private->UtpTmrlBase != NULL) {
    UfsHc->FreeBuffer (UfsHc, EFI_SIZE_TO_PAGES (Private->Nutmrs * sizeof (UTP_TMRD)), Private->UtpTmrlBase);
  }

  if (Private->TrlMapping != NULL) {
    UfsHc->Unmap (UfsHc, Private->TrlMapping);
  }

  if (Private->UtpTrlBase != NULL) {
    UfsHc->FreeBuffer (UfsHc, EFI_SIZE_TO_PAGES (Private->Nutrs * sizeof (UTP_TMRD)), Private->UtpTrlBase);
  }

  if (Private->TimerEvent != NULL) {
    gBS->CloseEvent (Private->TimerEvent);
  }

  FreePool (Private);

  //
  // Close protocols opened by UfsPassThru controller driver
  //
  gBS->CloseProtocol (
         Controller,
         &gEdkiiUfsHostControllerProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  The user Entry Point for module UfsPassThru. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUfsPassThru (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUfsPassThruDriverBinding,
             ImageHandle,
             &gUfsPassThruComponentName,
             &gUfsPassThruComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

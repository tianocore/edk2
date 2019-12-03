/** @file
  The AhciPei driver is used to manage ATA hard disk device working under AHCI
  mode at PEI phase.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _AHCI_PEI_STORAGE_SECURITY_H_
#define _AHCI_PEI_STORAGE_SECURITY_H_

/**
  Gets the count of storage security devices that one specific driver detects.

  @param[in]  This               The PPI instance pointer.
  @param[out] NumberofDevices    The number of storage security devices discovered.

  @retval EFI_SUCCESS              The operation performed successfully.
  @retval EFI_INVALID_PARAMETER    The parameters are invalid.

**/
EFI_STATUS
EFIAPI
AhciStorageSecurityGetDeviceNo (
  IN  EDKII_PEI_STORAGE_SECURITY_CMD_PPI    *This,
  OUT UINTN                                 *NumberofDevices
  );

/**
  Gets the device path of a specific storage security device.

  @param[in]  This                 The PPI instance pointer.
  @param[in]  DeviceIndex          Specifies the storage security device to which
                                   the function wants to talk. Because the driver
                                   that implements Storage Security Command PPIs
                                   will manage multiple storage devices, the PPIs
                                   that want to talk to a single device must specify
                                   the device index that was assigned during the
                                   enumeration process. This index is a number from
                                   one to NumberofDevices.
  @param[out] DevicePathLength     The length of the device path in bytes specified
                                   by DevicePath.
  @param[out] DevicePath           The device path of storage security device.
                                   This field re-uses EFI Device Path Protocol as
                                   defined by Section 10.2 EFI Device Path Protocol
                                   of UEFI 2.7 Specification.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    DevicePathLength or DevicePath is NULL.
  @retval EFI_NOT_FOUND            The specified storage security device not found.
  @retval EFI_OUT_OF_RESOURCES     The operation fails due to lack of resources.

**/
EFI_STATUS
EFIAPI
AhciStorageSecurityGetDevicePath (
  IN  EDKII_PEI_STORAGE_SECURITY_CMD_PPI    *This,
  IN  UINTN                                 DeviceIndex,
  OUT UINTN                                 *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL              **DevicePath
  );

/**
  Send a security protocol command to a device that receives data and/or the result
  of one or more commands sent by SendData.

  The ReceiveData function sends a security protocol command to the given DeviceIndex.
  The security protocol command sent is defined by SecurityProtocolId and contains
  the security protocol specific data SecurityProtocolSpecificData. The function
  returns the data from the security protocol command in PayloadBuffer.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL IN command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED RECEIVE commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero.

  If the PayloadBufferSize is zero, the security protocol command is sent using the
  Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBufferSize is too small to store the available data from the security
  protocol command, the function shall copy PayloadBufferSize bytes into the
  PayloadBuffer and return EFI_WARN_BUFFER_TOO_SMALL.

  If PayloadBuffer or PayloadTransferSize is NULL and PayloadBufferSize is non-zero,
  the function shall return EFI_INVALID_PARAMETER.

  If the given DeviceIndex does not support security protocol commands, the function
  shall return EFI_UNSUPPORTED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error, the
  function shall return EFI_DEVICE_ERROR.

  @param[in]  This             The PPI instance pointer.
  @param[in]  DeviceIndex      Specifies the storage security device to which the
                               function wants to talk. Because the driver that
                               implements Storage Security Command PPIs will manage
                               multiple storage devices, the PPIs that want to talk
                               to a single device must specify the device index
                               that was assigned during the enumeration process.
                               This index is a number from one to NumberofDevices.
  @param[in]  Timeout          The timeout, in 100ns units, to use for the execution
                               of the security protocol command. A Timeout value
                               of 0 means that this function will wait indefinitely
                               for the security protocol command to execute. If
                               Timeout is greater than zero, then this function
                               will return EFI_TIMEOUT if the time required to
                               execute the receive data command is greater than
                               Timeout.
  @param[in]  SecurityProtocolId
                               The value of the "Security Protocol" parameter of
                               the security protocol command to be sent.
  @param[in]  SecurityProtocolSpecificData
                               The value of the "Security Protocol Specific"
                               parameter of the security protocol command to be
                               sent.
  @param[in]  PayloadBufferSize
                               Size in bytes of the payload data buffer.
  @param[out] PayloadBuffer    A pointer to a destination buffer to store the
                               security protocol command specific payload data
                               for the security protocol command. The caller is
                               responsible for having either implicit or explicit
                               ownership of the buffer.
  @param[out] PayloadTransferSize
                               A pointer to a buffer to store the size in bytes
                               of the data written to the payload data buffer.

  @retval EFI_SUCCESS                  The security protocol command completed
                                       successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL    The PayloadBufferSize was too small to
                                       store the available data from the device.
                                       The PayloadBuffer contains the truncated
                                       data.
  @retval EFI_UNSUPPORTED              The given DeviceIndex does not support
                                       security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed
                                       with an error.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer or PayloadTransferSize
                                       is NULL and PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the
                                       security protocol command to execute.

**/
EFI_STATUS
EFIAPI
AhciStorageSecurityReceiveData (
  IN  EDKII_PEI_STORAGE_SECURITY_CMD_PPI    *This,
  IN  UINTN                                 DeviceIndex,
  IN  UINT64                                Timeout,
  IN  UINT8                                 SecurityProtocolId,
  IN  UINT16                                SecurityProtocolSpecificData,
  IN  UINTN                                 PayloadBufferSize,
  OUT VOID                                  *PayloadBuffer,
  OUT UINTN                                 *PayloadTransferSize
  );

/**
  Send a security protocol command to a device.

  The SendData function sends a security protocol command containing the payload
  PayloadBuffer to the given DeviceIndex. The security protocol command sent is
  defined by SecurityProtocolId and contains the security protocol specific data
  SecurityProtocolSpecificData. If the underlying protocol command requires a
  specific padding for the command payload, the SendData function shall add padding
  bytes to the command payload to satisfy the padding requirements.

  For devices supporting the SCSI command set, the security protocol command is
  sent using the SECURITY PROTOCOL OUT command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is
  sent using one of the TRUSTED SEND commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero. If the PayloadBufferSize is zero, the security protocol command
  is sent using the Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBuffer is NULL and PayloadBufferSize is non-zero, the function shall
  return EFI_INVALID_PARAMETER.

  If the given DeviceIndex does not support security protocol commands, the function
  shall return EFI_UNSUPPORTED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error,
  the functio shall return EFI_DEVICE_ERROR.

  @param[in] This              The PPI instance pointer.
  @param[in] DeviceIndex       The ID of the device.
  @param[in] Timeout           The timeout, in 100ns units, to use for the execution
                               of the security protocol command. A Timeout value
                               of 0 means that this function will wait indefinitely
                               for the security protocol command to execute. If
                               Timeout is greater than zero, then this function
                               will return EFI_TIMEOUT if the time required to
                               execute the receive data command is greater than
                               Timeout.
  @param[in] SecurityProtocolId
                               The value of the "Security Protocol" parameter of
                               the security protocol command to be sent.
  @param[in] SecurityProtocolSpecificData
                               The value of the "Security Protocol Specific"
                               parameter of the security protocol command to be
                               sent.
  @param[in] PayloadBufferSize Size in bytes of the payload data buffer.
  @param[in] PayloadBuffer     A pointer to a destination buffer to store the
                               security protocol command specific payload data
                               for the security protocol command.

  @retval EFI_SUCCESS              The security protocol command completed successfully.
  @retval EFI_UNSUPPORTED          The given DeviceIndex does not support security
                                   protocol commands.
  @retval EFI_DEVICE_ERROR         The security protocol command completed with
                                   an error.
  @retval EFI_INVALID_PARAMETER    The PayloadBuffer is NULL and PayloadBufferSize
                                   is non-zero.
  @retval EFI_TIMEOUT              A timeout occurred while waiting for the security
                                   protocol command to execute.

**/
EFI_STATUS
EFIAPI
AhciStorageSecuritySendData (
  IN EDKII_PEI_STORAGE_SECURITY_CMD_PPI  *This,
  IN UINTN                               DeviceIndex,
  IN UINT64                              Timeout,
  IN UINT8                               SecurityProtocolId,
  IN UINT16                              SecurityProtocolSpecificData,
  IN UINTN                               PayloadBufferSize,
  IN VOID                                *PayloadBuffer
  );

#endif

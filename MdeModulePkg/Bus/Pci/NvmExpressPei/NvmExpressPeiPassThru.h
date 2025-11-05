/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _NVM_EXPRESS_PEI_PASSTHRU_H_
#define _NVM_EXPRESS_PEI_PASSTHRU_H_

/**
  Sends an NVM Express Command Packet to an NVM Express controller or namespace. This function only
  supports blocking execution of the command.

  @param[in] Private        The pointer to the NVME_CONTEXT Data structure.
  @param[in] NamespaceId    Is a 32 bit Namespace ID to which the Express HCI command packet will
                            be sent.
                            A Value of 0 denotes the NVM Express controller, a Value of all 0FFh in
                            the namespace ID specifies that the command packet should be sent to all
                            valid namespaces.
  @param[in,out] Packet     A pointer to the EDKII PEI NVM Express PassThru Command Packet to send
                            to the NVMe namespace specified by NamespaceId.

  @retval EFI_SUCCESS              The EDKII PEI NVM Express Command Packet was sent by the host.
                                   TransferLength bytes were transferred to, or from DataBuffer.
  @retval EFI_NOT_READY            The EDKII PEI NVM Express Command Packet could not be sent because
                                   the controller is not ready. The caller may retry again later.
  @retval EFI_DEVICE_ERROR         A device error occurred while attempting to send the EDKII PEI NVM
                                   Express Command Packet.
  @retval EFI_INVALID_PARAMETER    Namespace, or the contents of EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET
                                   are invalid.
                                   The EDKII PEI NVM Express Command Packet was not sent, so no
                                   additional status information is available.
  @retval EFI_UNSUPPORTED          The command described by the EDKII PEI NVM Express Command Packet
                                   is not supported by the host adapter.
                                   The EDKII PEI NVM Express Command Packet was not sent, so no
                                   additional status information is available.
  @retval EFI_TIMEOUT              A timeout occurred while waiting for the EDKII PEI NVM Express Command
                                   Packet to execute.

**/
EFI_STATUS
NvmePassThruExecute (
  IN     PEI_NVME_CONTROLLER_PRIVATE_DATA          *Private,
  IN     UINT32                                    NamespaceId,
  IN OUT EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  *Packet
  );

/**
  Gets the device path information of the underlying NVM Express host controller.

  @param[in]  This                 The PPI instance pointer.
  @param[out] DevicePathLength     The length of the device path in bytes specified
                                   by DevicePath.
  @param[out] DevicePath           The device path of the underlying NVM Express
                                   host controller.
                                   This field re-uses EFI Device Path Protocol as
                                   defined by Section 10.2 EFI Device Path Protocol
                                   of UEFI 2.7 Specification.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    DevicePathLength or DevicePath is NULL.
  @retval EFI_OUT_OF_RESOURCES     The operation fails due to lack of resources.

**/
EFI_STATUS
EFIAPI
NvmePassThruGetDevicePath (
  IN  EDKII_PEI_NVM_EXPRESS_PASS_THRU_PPI  *This,
  OUT UINTN                                *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL             **DevicePath
  );

/**
  Used to retrieve the next namespace ID for this NVM Express controller.

  If on input the value pointed to by NamespaceId is 0xFFFFFFFF, then the first
  valid namespace ID defined on the NVM Express controller is returned in the
  location pointed to by NamespaceId and a status of EFI_SUCCESS is returned.

  If on input the value pointed to by NamespaceId is an invalid namespace ID
  other than 0xFFFFFFFF, then EFI_INVALID_PARAMETER is returned.

  If on input the value pointed to by NamespaceId is a valid namespace ID, then
  the next valid namespace ID on the NVM Express controller is returned in the
  location pointed to by NamespaceId, and EFI_SUCCESS is returned.

  If the value pointed to by NamespaceId is the namespace ID of the last
  namespace on the NVM Express controller, then EFI_NOT_FOUND is returned.

  @param[in]     This              The PPI instance pointer.
  @param[in,out] NamespaceId       On input, a pointer to a legal NamespaceId
                                   for an NVM Express namespace present on the
                                   NVM Express controller. On output, a pointer
                                   to the next NamespaceId of an NVM Express
                                   namespace on an NVM Express controller. An
                                   input value of 0xFFFFFFFF retrieves the
                                   first NamespaceId for an NVM Express
                                   namespace present on an NVM Express
                                   controller.

  @retval EFI_SUCCESS            The Namespace ID of the next Namespace was
                                 returned.
  @retval EFI_NOT_FOUND          There are no more namespaces defined on this
                                 controller.
  @retval EFI_INVALID_PARAMETER  NamespaceId is an invalid value other than
                                 0xFFFFFFFF.

**/
EFI_STATUS
EFIAPI
NvmePassThruGetNextNameSpace (
  IN     EDKII_PEI_NVM_EXPRESS_PASS_THRU_PPI  *This,
  IN OUT UINT32                               *NamespaceId
  );

/**
  Sends an NVM Express Command Packet to an NVM Express controller or namespace. This function only
  supports blocking execution of the command.

  @param[in] This                  The PPI instance pointer.
  @param[in] NamespaceId           Is a 32 bit Namespace ID to which the Nvm Express command packet will
                                   be sent.
                                   A Value of 0 denotes the NVM Express controller, a Value of all 0FFh in
                                   the namespace ID specifies that the command packet should be sent to all
                                   valid namespaces.
  @param[in,out] Packet            A pointer to the EDKII PEI NVM Express PassThru Command Packet to send
                                   to the NVMe namespace specified by NamespaceId.

  @retval EFI_SUCCESS              The EDKII PEI NVM Express Command Packet was sent by the host.
                                   TransferLength bytes were transferred to, or from DataBuffer.
  @retval EFI_NOT_READY            The EDKII PEI NVM Express Command Packet could not be sent because
                                   the controller is not ready. The caller may retry again later.
  @retval EFI_DEVICE_ERROR         A device error occurred while attempting to send the EDKII PEI NVM
                                   Express Command Packet.
  @retval EFI_INVALID_PARAMETER    Namespace, or the contents of EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET
                                   are invalid.
                                   The EDKII PEI NVM Express Command Packet was not sent, so no
                                   additional status information is available.
  @retval EFI_UNSUPPORTED          The command described by the EDKII PEI NVM Express Command Packet
                                   is not supported by the host adapter.
                                   The EDKII PEI NVM Express Command Packet was not sent, so no
                                   additional status information is available.
  @retval EFI_TIMEOUT              A timeout occurred while waiting for the EDKII PEI NVM Express Command
                                   Packet to execute.

**/
EFI_STATUS
EFIAPI
NvmePassThru (
  IN     EDKII_PEI_NVM_EXPRESS_PASS_THRU_PPI       *This,
  IN     UINT32                                    NamespaceId,
  IN OUT EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  *Packet
  );

#endif

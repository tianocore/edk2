/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _NVM_EXPRESS_PEI_PASSTHRU_H_
#define _NVM_EXPRESS_PEI_PASSTHRU_H_

#define NVME_CONTROLLER_NSID        0

typedef struct {
  UINT8                             Opcode;
  UINT8                             FusedOperation;
    #define NORMAL_CMD              0x00
    #define FUSED_FIRST_CMD         0x01
    #define FUSED_SECOND_CMD        0x02
  UINT16                            Cid;
} NVME_CDW0;

typedef struct {
  NVME_CDW0                         Cdw0;
  UINT8                             Flags;
    #define CDW10_VALID             0x01
    #define CDW11_VALID             0x02
    #define CDW12_VALID             0x04
    #define CDW13_VALID             0x08
    #define CDW14_VALID             0x10
    #define CDW15_VALID             0x20
  UINT32                            Nsid;
  UINT32                            Cdw10;
  UINT32                            Cdw11;
  UINT32                            Cdw12;
  UINT32                            Cdw13;
  UINT32                            Cdw14;
  UINT32                            Cdw15;
} EDKII_PEI_NVM_EXPRESS_COMMAND;

typedef struct {
  UINT32                            Cdw0;
  UINT32                            Cdw1;
  UINT32                            Cdw2;
  UINT32                            Cdw3;
} EDKII_PEI_NVM_EXPRESS_COMPLETION;

typedef struct {
  UINT64                            CommandTimeout;
  VOID                              *TransferBuffer;
  UINT32                            TransferLength;
  VOID                              *MetadataBuffer;
  UINT32                            MetadataLength;
  UINT8                             QueueType;
  EDKII_PEI_NVM_EXPRESS_COMMAND     *NvmeCmd;
  EDKII_PEI_NVM_EXPRESS_COMPLETION  *NvmeCompletion;
} EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET;


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
NvmePassThru (
  IN     PEI_NVME_CONTROLLER_PRIVATE_DATA                  *Private,
  IN     UINT32                                            NamespaceId,
  IN OUT EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    *Packet
  );

#endif

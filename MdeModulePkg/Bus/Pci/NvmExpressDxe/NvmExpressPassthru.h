/** @file
  NvmExpressDxe driver is used to manage non-volatile memory subsystem which follows
  NVM Express specification.

  Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _NVM_EXPRESS_PASS_THRU_H_
#define _NVM_EXPRESS_PASS_THRU_H_

#define NVM_EXPRESS_PASS_THRU_PROTOCOL_GUID \
  { \
    0xec51ef5c, 0x2cf3, 0x4a55, {0xbf, 0x85, 0xb6, 0x3c, 0xa3, 0xb1, 0x3f, 0x44 } \
  }

typedef struct _NVM_EXPRESS_PASS_THRU_PROTOCOL NVM_EXPRESS_PASS_THRU_PROTOCOL;

typedef struct {
  UINT32          AdapterId;
  UINT32          Attributes;
  UINT32          IoAlign;
  UINT32          HciVersion;
  UINT64          Timeout;
  UINT32          MaxNamespace;
} NVM_EXPRESS_PASS_THRU_MODE;

//
// If this bit is set, then the NVM_EXPRESS_PASS_THRU_PROTOCOL interface is for directly addressable namespaces.
//
#define NVM_EXPRESS_PASS_THRU_ATTRIBUTES_PHYSICAL        0x0001
//
// If this bit is set, then the NVM_EXPRESS_PASS_THRU_PROTOCOL interface is for a single volume logical namespace
// comprised of multiple namespaces.
//
#define NVM_EXPRESS_PASS_THRU_ATTRIBUTES_LOGICAL         0x0002
//
// If this bit is set, then the NVM_EXPRESS_PASS_THRU_PROTOCOL interface supports non blocking I/O.
//
#define NVM_EXPRESS_PASS_THRU_ATTRIBUTES_NONBLOCKIO      0x0004
//
// If this bit is set, then the NVM_EXPRESS_PASS_THRU_PROTOCOL interface supports NVM command set commands.
//
#define NVM_EXPRESS_PASS_THRU_ATTRIBUTES_CMD_SET_NVME    0x0008

//
// QueueId
//
#define NVME_ADMIN_QUEUE                                 0x00
#define NVME_IO_QUEUE                                    0x01

//
// ControllerStatus
//
#define NVM_EXPRESS_STATUS_CONTROLLER_READY              0x00
#define NVM_EXPRESS_STATUS_CONTROLLER_CMD_ERROR          0x01
#define NVM_EXPRESS_STATUS_CONTROLLER_FATAL              0x02
#define NVM_EXPRESS_STATUS_CONTROLLER_CMD_DATA_ERROR     0x04
#define NVM_EXPRESS_STATUS_CONTROLLER_CMD_ABORT          0x05
#define NVM_EXPRESS_STATUS_CONTROLLER_DEVICE_ERROR       0x06
#define NVM_EXPRESS_STATUS_CONTROLLER_TIMEOUT_COMMAND    0x09
#define NVM_EXPRESS_STATUS_CONTROLLER_INVALID_NAMESPACE  0x0B
#define NVM_EXPRESS_STATUS_CONTROLLER_NOT_READY          0x0C
#define NVM_EXPRESS_STATUS_CONTROLLER_OTHER              0x7F

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
} NVM_EXPRESS_COMMAND;

typedef struct {
  UINT32                            Cdw0;
  UINT32                            Cdw1;
  UINT32                            Cdw2;
  UINT32                            Cdw3;
} NVM_EXPRESS_RESPONSE;

typedef struct {
  UINT64                            CommandTimeout;
  VOID                              *TransferBuffer;
  UINT32                            TransferLength;
  VOID                              *MetadataBuffer;
  UINT32                            MetadataLength;
  UINT8                             QueueId;
  NVM_EXPRESS_COMMAND               *NvmeCmd;
  NVM_EXPRESS_RESPONSE              *NvmeResponse;
  UINT8                             ControllerStatus;
} NVM_EXPRESS_PASS_THRU_COMMAND_PACKET;

//
// Protocol funtion prototypes
//
/**
  Sends an NVM Express Command Packet to an NVM Express controller or namespace. This function supports
  both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the nonblocking
  I/O functionality is optional.

  @param[in]     This                A pointer to the NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in]     NamespaceId         Is a 32 bit Namespace ID to which the Express HCI command packet will be sent.
                                     A value of 0 denotes the NVM Express controller, a value of all 0FFh in the namespace
                                     ID specifies that the command packet should be sent to all valid namespaces.
  @param[in]     NamespaceUuid       Is a 64 bit Namespace UUID to which the Express HCI command packet will be sent.
                                     A value of 0 denotes the NVM Express controller, a value of all 0FFh in the namespace
                                     UUID specifies that the command packet should be sent to all valid namespaces.
  @param[in,out] Packet              A pointer to the NVM Express HCI Command Packet to send to the NVMe namespace specified
                                     by NamespaceId.
  @param[in]     Event               If nonblocking I/O is not supported then Event is ignored, and blocking I/O is performed.
                                     If Event is NULL, then blocking I/O is performed. If Event is not NULL and non blocking I/O
                                     is supported, then nonblocking I/O is performed, and Event will be signaled when the NVM
                                     Express Command Packet completes.

  @retval EFI_SUCCESS                The NVM Express Command Packet was sent by the host. TransferLength bytes were transferred
                                     to, or from DataBuffer.
  @retval EFI_BAD_BUFFER_SIZE        The NVM Express Command Packet was not executed. The number of bytes that could be transferred
                                     is returned in TransferLength.
  @retval EFI_NOT_READY              The NVM Express Command Packet could not be sent because the controller is not ready. The caller
                                     may retry again later.
  @retval EFI_DEVICE_ERROR           A device error occurred while attempting to send the NVM Express Command Packet.
  @retval EFI_INVALID_PARAMETER      Namespace, or the contents of NVM_EXPRESS_PASS_THRU_COMMAND_PACKET are invalid. The NVM
                                     Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_UNSUPPORTED            The command described by the NVM Express Command Packet is not supported by the host adapter.
                                     The NVM Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT                A timeout occurred while waiting for the NVM Express Command Packet to execute.

**/
typedef
EFI_STATUS
(EFIAPI *NVM_EXPRESS_PASS_THRU_PASSTHRU)(
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN     UINT32                                      NamespaceId,
  IN     UINT64                                      NamespaceUuid,
  IN OUT NVM_EXPRESS_PASS_THRU_COMMAND_PACKET        *Packet,
  IN     EFI_EVENT                                   Event OPTIONAL
  );

/**
  Used to retrieve the list of namespaces defined on an NVM Express controller.

  The NVM_EXPRESS_PASS_THRU_PROTOCOL.GetNextNamespace() function retrieves a list of namespaces
  defined on an NVM Express controller. If on input a NamespaceID is specified by all 0xFF in the
  namespace buffer, then the first namespace defined on the NVM Express controller is returned in
  NamespaceID, and a status of EFI_SUCCESS is returned.

  If NamespaceId is a Namespace value that was returned on a previous call to GetNextNamespace(),
  then the next valid NamespaceId  for an NVM Express SSD namespace on the NVM Express controller
  is returned in NamespaceId, and EFI_SUCCESS is returned.

  If Namespace array is not a 0xFFFFFFFF and NamespaceId was not returned on a previous call to
  GetNextNamespace(), then EFI_INVALID_PARAMETER is returned.

  If NamespaceId is the NamespaceId of the last SSD namespace on the NVM Express controller, then
  EFI_NOT_FOUND is returned

  @param[in]     This           A pointer to the NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in,out] NamespaceId    On input, a pointer to a legal NamespaceId for an NVM Express
                                namespace present on the NVM Express controller. On output, a
                                pointer to the next NamespaceId of an NVM Express namespace on
                                an NVM Express controller. An input value of 0xFFFFFFFF retrieves
                                the first NamespaceId for an NVM Express namespace present on an
                                NVM Express controller.
  @param[out]    NamespaceUuid  On output, the UUID associated with the next namespace, if a UUID
                                is defined for that NamespaceId, otherwise, zero is returned in
                                this parameter. If the caller does not require a UUID, then a NULL
                                pointer may be passed.

  @retval EFI_SUCCESS           The NamespaceId of the next Namespace was returned.
  @retval EFI_NOT_FOUND         There are no more namespaces defined on this controller.
  @retval EFI_INVALID_PARAMETER Namespace array is not a 0xFFFFFFFF and NamespaceId was not returned
                                on a previous call to GetNextNamespace().

**/
typedef
EFI_STATUS
(EFIAPI *NVM_EXPRESS_PASS_THRU_GET_NEXT_NAMESPACE)(
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN OUT UINT32                                      *NamespaceId,
     OUT UINT64                                      *NamespaceUuid  OPTIONAL
  );

/**
  Used to allocate and build a device path node for an NVM Express namespace on an NVM Express controller.

  The NVM_EXPRESS_PASS_THRU_PROTOCOL.BuildDevicePath() function allocates and builds a single device
  path node for the NVM Express namespace specified by NamespaceId.

  If the namespace device specified by NamespaceId is not valid , then EFI_NOT_FOUND is returned.

  If DevicePath is NULL, then EFI_INVALID_PARAMETER is returned.

  If there are not enough resources to allocate the device path node, then EFI_OUT_OF_RESOURCES is returned.

  Otherwise, DevicePath is allocated with the boot service AllocatePool(), the contents of DevicePath are
  initialized to describe the NVM Express namespace specified by NamespaceId, and EFI_SUCCESS is returned.

  @param[in]     This                A pointer to the NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in]     NamespaceId         The NVM Express namespace ID  for which a device path node is to be
                                     allocated and built. Caller must set the NamespaceId to zero if the
                                     device path node will contain a valid UUID.
  @param[in]     NamespaceUuid       The NVM Express namespace UUID for which a device path node is to be
                                     allocated and built. UUID will only be valid of the Namespace ID is zero.
  @param[in,out] DevicePath          A pointer to a single device path node that describes the NVM Express
                                     namespace specified by NamespaceId. This function is responsible for
                                     allocating the buffer DevicePath with the boot service AllocatePool().
                                     It is the caller's responsibility to free DevicePath when the caller
                                     is finished with DevicePath.
  @retval EFI_SUCCESS                The device path node that describes the NVM Express namespace specified
                                     by NamespaceId was allocated and returned in DevicePath.
  @retval EFI_NOT_FOUND              The NVM Express namespace specified by NamespaceId does not exist on the
                                     NVM Express controller.
  @retval EFI_INVALID_PARAMETER      DevicePath is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources to allocate the DevicePath node.

**/
typedef
EFI_STATUS
(EFIAPI *NVM_EXPRESS_PASS_THRU_BUILD_DEVICE_PATH)(
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN     UINT32                                      NamespaceId,
  IN     UINT64                                      NamespaceUuid,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                    **DevicePath
  );

/**
  Used to translate a device path node to a Namespace ID and Namespace UUID.

  The NVM_EXPRESS_PASS_THRU_PROTOCOL.GetNamwspace() function determines the Namespace ID and Namespace UUID
  associated with the NVM Express SSD namespace described by DevicePath. If DevicePath is a device path node type
  that the NVM Express Pass Thru driver supports, then the NVM Express Pass Thru driver will attempt to translate
  the contents DevicePath into a Namespace ID and UUID. If this translation is successful, then that Namespace ID
  and UUID are returned in NamespaceID and NamespaceUUID, and EFI_SUCCESS is returned.

  @param[in]  This                A pointer to the NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in]  DevicePath          A pointer to the device path node that describes an NVM Express namespace on
                                  the NVM Express controller.
  @param[out] NamespaceId         The NVM Express namespace ID contained in the device path node.
  @param[out] NamespaceUuid       The NVM Express namespace contained in the device path node.

  @retval EFI_SUCCESS             DevicePath was successfully translated to NamespaceId and NamespaceUuid.
  @retval EFI_INVALID_PARAMETER   If DevicePath, NamespaceId, or NamespaceUuid are NULL, then EFI_INVALID_PARAMETER
                                  is returned.
  @retval EFI_UNSUPPORTED         If DevicePath is not a device path node type that the NVM Express Pass Thru driver
                                  supports, then EFI_UNSUPPORTED is returned.
  @retval EFI_NOT_FOUND           If DevicePath is a device path node type that the Nvm Express Pass Thru driver
                                  supports, but there is not a valid translation from DevicePath to a NamespaceID
                                  and NamespaceUuid, then EFI_NOT_FOUND is returned.
**/
typedef
EFI_STATUS
(EFIAPI *NVM_EXPRESS_PASS_THRU_GET_NAMESPACE)(
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN     EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
     OUT UINT32                                      *NamespaceId,
     OUT UINT64                                      *NamespaceUuid
  );

//
// Protocol Interface Structure
//
struct _NVM_EXPRESS_PASS_THRU_PROTOCOL {
  NVM_EXPRESS_PASS_THRU_MODE                     *Mode;
  NVM_EXPRESS_PASS_THRU_PASSTHRU                 PassThru;
  NVM_EXPRESS_PASS_THRU_GET_NEXT_NAMESPACE       GetNextNamespace;
  NVM_EXPRESS_PASS_THRU_BUILD_DEVICE_PATH        BuildDevicePath;
  NVM_EXPRESS_PASS_THRU_GET_NAMESPACE            GetNamespace;
};

//extern EFI_GUID gNvmExpressPassThruProtocolGuid;

#endif


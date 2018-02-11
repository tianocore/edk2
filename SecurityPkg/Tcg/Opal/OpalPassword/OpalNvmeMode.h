/** @file
  Header file for NVMe function definitions

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OPAL_PASSWORD_NVME_MODE_H__
#define __OPAL_PASSWORD_NVME_MODE_H__


#include "OpalNvmeReg.h"

#define NVME_MAX_SECTORS            0x10000
//
// QueueId
//
#define NVME_ADMIN_QUEUE            0x00
#define NVME_IO_QUEUE               0x01

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
  UINT64                            TransferBuffer;
  UINT32                            TransferLength;
  UINT64                            MetadataBuffer;
  UINT32                            MetadataLength;
  UINT8                             QueueId;
  NVM_EXPRESS_COMMAND               *NvmeCmd;
  NVM_EXPRESS_RESPONSE              *NvmeResponse;
} NVM_EXPRESS_PASS_THRU_COMMAND_PACKET;


#pragma pack(1)

// Internal fields
typedef enum {
  NvmeStatusUnknown,
  NvmeStatusInit,
  NvmeStatusInuse,
  NvmeStatusMax,
} NVME_STATUS;

typedef struct {
  UINT32                            Nbar;
  VOID                              *BaseMem;
  VOID                              *BaseMemMapping;
  BOOLEAN                           PollCancellation;
  UINT16                            NvmeInitWaitTime;

  NVME_STATUS                       State;
  UINT8                             BusID;
  UINT8                             DeviceID;
  UINT8                             FuncID;
  UINTN                             PciBase;

  UINT32                            Nsid;
  UINT64                            Nsuuid;
  UINT32                            BlockSize;
  EFI_LBA                           LastBlock;

  //
  // Pointers to 4kB aligned submission & completion queues.
  //
  NVME_SQ                           *SqBuffer[NVME_MAX_IO_QUEUES];
  NVME_CQ                           *CqBuffer[NVME_MAX_IO_QUEUES];
  UINT16                            Cid[NVME_MAX_IO_QUEUES];

  //
  // Submission and completion queue indices.
  //
  NVME_SQTDBL                       SqTdbl[NVME_MAX_IO_QUEUES];
  NVME_CQHDBL                       CqHdbl[NVME_MAX_IO_QUEUES];
  UINT8                             Pt[NVME_MAX_IO_QUEUES];

  UINTN                             SqeCount[NVME_MAX_IO_QUEUES];

  //
  // Nvme controller capabilities
  //
  NVME_CAP                          Cap;

  //
  // pointer to identify controller Data
  //
  NVME_ADMIN_CONTROLLER_DATA        *ControllerData;
  NVME_ADMIN_NAMESPACE_DATA         *NamespaceData;
} NVME_CONTEXT;

#pragma pack()

/**
  Transfer MMIO Data to memory.

  @param[in,out] MemBuffer - Destination: Memory address
  @param[in] MmioAddr      - Source: MMIO address
  @param[in] Size          - Size for read

  @retval EFI_SUCCESS - MMIO read sucessfully
**/
EFI_STATUS
NvmeMmioRead (
  IN OUT VOID *MemBuffer,
  IN     UINTN MmioAddr,
  IN     UINTN Size
  );

/**
  Transfer memory Data to MMIO.

  @param[in,out] MmioAddr - Destination: MMIO address
  @param[in] MemBuffer    - Source: Memory address
  @param[in] Size         - Size for write

  @retval EFI_SUCCESS - MMIO write sucessfully
**/
EFI_STATUS
NvmeMmioWrite (
  IN OUT UINTN MmioAddr,
  IN     VOID *MemBuffer,
  IN     UINTN Size
  );

/**
  Transfer memory data to MMIO.

  @param[in,out] MmioAddr - Destination: MMIO address
  @param[in] MemBuffer    - Source: Memory address
  @param[in] Size         - Size for write

  @retval EFI_SUCCESS - MMIO write sucessfully
**/
EFI_STATUS
OpalPciWrite (
  IN OUT UINTN MmioAddr,
  IN     VOID *MemBuffer,
  IN     UINTN Size
  );

/**
  Transfer MMIO data to memory.

  @param[in,out] MemBuffer - Destination: Memory address
  @param[in] MmioAddr      - Source: MMIO address
  @param[in] Size          - Size for read

  @retval EFI_SUCCESS - MMIO read sucessfully
**/
EFI_STATUS
OpalPciRead (
  IN OUT VOID *MemBuffer,
  IN     UINTN MmioAddr,
  IN     UINTN Size
  );

/**
  Allocate transfer-related Data struct which is used at Nvme.

  @param[in, out] Nvme          The pointer to the NVME_CONTEXT Data structure.

  @retval EFI_OUT_OF_RESOURCE   No enough resource.
  @retval EFI_SUCCESS           Successful to allocate resource.

**/
EFI_STATUS
EFIAPI
NvmeAllocateResource (
  IN OUT NVME_CONTEXT       *Nvme
  );

/**
  Free allocated transfer-related Data struct which is used at NVMe.

  @param[in, out] Nvme          The pointer to the NVME_CONTEXT Data structure.

**/
VOID
EFIAPI
NvmeFreeResource (
  IN OUT NVME_CONTEXT       *Nvme
  );

/**
  Sends an NVM Express Command Packet to an NVM Express controller or namespace. This function supports
  both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the nonblocking
  I/O functionality is optional.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] NamespaceId            - Is a 32 bit Namespace ID to which the Express HCI command packet will be sent.
                                      A Value of 0 denotes the NVM Express controller, a Value of all 0FFh in the namespace
                                      ID specifies that the command packet should be sent to all valid namespaces.
  @param[in] NamespaceUuid          - Is a 64 bit Namespace UUID to which the Express HCI command packet will be sent.
                                      A Value of 0 denotes the NVM Express controller, a Value of all 0FFh in the namespace
                                      UUID specifies that the command packet should be sent to all valid namespaces.
  @param[in,out] Packet             - A pointer to the NVM Express HCI Command Packet to send to the NVMe namespace specified
                                      by NamespaceId.

  @retval EFI_SUCCESS               - The NVM Express Command Packet was sent by the host. TransferLength bytes were transferred
                                      to, or from DataBuffer.
  @retval EFI_NOT_READY             - The NVM Express Command Packet could not be sent because the controller is not ready. The caller
                                      may retry again later.
  @retval EFI_DEVICE_ERROR          - A device error occurred while attempting to send the NVM Express Command Packet.
  @retval EFI_INVALID_PARAMETER     - Namespace, or the contents of NVM_EXPRESS_PASS_THRU_COMMAND_PACKET are invalid. The NVM
                                      Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_UNSUPPORTED           - The command described by the NVM Express Command Packet is not supported by the host adapter.
                                      The NVM Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT               - A timeout occurred while waiting for the NVM Express Command Packet to execute.

**/
EFI_STATUS
NvmePassThru (
  IN     NVME_CONTEXT                         *Nvme,
  IN     UINT32                               NamespaceId,
  IN     UINT64                               NamespaceUuid,
  IN OUT NVM_EXPRESS_PASS_THRU_COMMAND_PACKET *Packet
  );

/**
  Waits until all NVME commands completed.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Qid                    - Queue index

  @retval EFI_SUCCESS               - All NVME commands have completed
  @retval EFI_TIMEOUT               - Timeout occured
  @retval EFI_NOT_READY             - Not all NVME commands have completed
  @retval others                    - Error occurred on device side.
**/
EFI_STATUS
NvmeWaitAllComplete (
  IN NVME_CONTEXT       *Nvme,
  IN UINT8              Qid
  );

/**
  Initialize the Nvm Express controller.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @retval EFI_SUCCESS               - The NVM Express Controller is initialized successfully.
  @retval Others                    - A device error occurred while initializing the controller.

**/
EFI_STATUS
NvmeControllerInit (
  IN NVME_CONTEXT       *Nvme
  );

/**
  Un-initialize the Nvm Express controller.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @retval EFI_SUCCESS               - The NVM Express Controller is un-initialized successfully.
  @retval Others                    - A device error occurred while un-initializing the controller.

**/
EFI_STATUS
NvmeControllerExit (
  IN NVME_CONTEXT       *Nvme
  );

/**
  Check whether there are available command slots.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Qid                    - Queue index

  @retval EFI_SUCCESS               - Available command slot is found
  @retval EFI_NOT_READY             - No available command slot is found
  @retval EFI_DEVICE_ERROR          - Error occurred on device side.

**/
EFI_STATUS
NvmeHasFreeCmdSlot (
  IN NVME_CONTEXT       *Nvme,
  IN UINT8              Qid
  );

/**
  Check whether all command slots are clean.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Qid                    - Queue index

  @retval EFI_SUCCESS               - All command slots are clean
  @retval EFI_NOT_READY             - Not all command slots are clean
  @retval EFI_DEVICE_ERROR          - Error occurred on device side.

**/
EFI_STATUS
NvmeIsAllCmdSlotClean (
  IN NVME_CONTEXT       *Nvme,
  IN UINT8              Qid
  );

/**
  Read sector Data from the NVMe device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in,out] Buffer             - The Buffer used to store the Data read from the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be read.

  @retval EFI_SUCCESS               - Datum are read from the device.
  @retval Others                    - Fail to read all the datum.

**/
EFI_STATUS
NvmeReadSectors (
  IN NVME_CONTEXT                          *Nvme,
  IN OUT UINT64                            Buffer,
  IN UINT64                                Lba,
  IN UINT32                                Blocks
  );

/**
  Write sector Data to the NVMe device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Buffer                 - The Buffer to be written into the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be written.

  @retval EFI_SUCCESS               - Datum are written into the Buffer.
  @retval Others                    - Fail to write all the datum.

**/
EFI_STATUS
NvmeWriteSectors (
  IN NVME_CONTEXT                          *Nvme,
  IN UINT64                                Buffer,
  IN UINT64                                Lba,
  IN UINT32                                Blocks
  );

/**
  Flushes all modified Data to the device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.

  @retval EFI_SUCCESS               - Datum are written into the Buffer.
  @retval Others                    - Fail to write all the datum.

**/
EFI_STATUS
NvmeFlush (
  IN NVME_CONTEXT                          *Nvme
  );

/**
  Read some blocks from the device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[out] Buffer                - The Buffer used to store the Data read from the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be read.

  @retval EFI_SUCCESS               - Datum are read from the device.
  @retval Others                    - Fail to read all the datum.

**/
EFI_STATUS
NvmeRead (
  IN NVME_CONTEXT                  *Nvme,
  OUT UINT64                       Buffer,
  IN UINT64                        Lba,
  IN UINTN                         Blocks
  );

/**
  Write some blocks to the device.

  @param[in] Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in] Buffer                 - The Buffer to be written into the device.
  @param[in] Lba                    - The start block number.
  @param[in] Blocks                 - Total block number to be written.

  @retval EFI_SUCCESS               - Datum are written into the Buffer.
  @retval Others                    - Fail to write all the datum.

**/
EFI_STATUS
NvmeWrite (
  IN NVME_CONTEXT                  *Nvme,
  IN UINT64                        Buffer,
  IN UINT64                        Lba,
  IN UINTN                         Blocks
  );

/**
  Security send and receive commands.

  @param[in]     Nvme                   - The pointer to the NVME_CONTEXT Data structure.
  @param[in]     SendCommand            - The flag to indicate the command type, TRUE for Send command and FALSE for receive command
  @param[in]     SecurityProtocol       - Security Protocol
  @param[in]     SpSpecific             - Security Protocol Specific
  @param[in]     TransferLength         - Transfer Length of Buffer (in bytes) - always a multiple of 512
  @param[in,out] TransferBuffer         - Address of Data to transfer

  @return EFI_SUCCESS               - Successfully create io submission queue.
  @return others                    - Fail to send/receive commands.

**/
EFI_STATUS
NvmeSecuritySendReceive (
  IN NVME_CONTEXT                          *Nvme,
  IN BOOLEAN                               SendCommand,
  IN UINT8                                 SecurityProtocol,
  IN UINT16                                SpSpecific,
  IN UINTN                                 TransferLength,
  IN OUT VOID                              *TransferBuffer
  );

#endif

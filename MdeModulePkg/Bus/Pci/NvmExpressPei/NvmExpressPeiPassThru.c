/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpressPei.h"

/**
  Create PRP lists for Data transfer which is larger than 2 memory pages.

  @param[in] Private          The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] PhysicalAddr     The physical base address of Data Buffer.
  @param[in] Pages            The number of pages to be transfered.

  @retval The pointer Value to the first PRP List of the PRP lists.

**/
UINT64
NvmeCreatePrpList (
  IN     PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN     EFI_PHYSICAL_ADDRESS                PhysicalAddr,
  IN     UINTN                               Pages
  )
{
  UINTN                   PrpEntryNo;
  UINTN                   PrpListNo;
  UINT64                  PrpListBase;
  VOID                    *PrpListHost;
  UINTN                   PrpListIndex;
  UINTN                   PrpEntryIndex;
  UINT64                  Remainder;
  EFI_PHYSICAL_ADDRESS    PrpListPhyAddr;
  UINTN                   Bytes;
  UINT8                   *PrpEntry;
  EFI_PHYSICAL_ADDRESS    NewPhyAddr;

  //
  // The number of Prp Entry in a memory page.
  //
  PrpEntryNo = EFI_PAGE_SIZE / sizeof (UINT64);

  //
  // Calculate total PrpList number.
  //
  PrpListNo = (UINTN) DivU64x64Remainder ((UINT64)Pages, (UINT64)PrpEntryNo, &Remainder);
  if (Remainder != 0) {
    PrpListNo += 1;
  }

  if (PrpListNo > NVME_PRP_SIZE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: The implementation only supports PrpList number up to 4."
      " But %d are needed here.\n",
      __FUNCTION__,
      PrpListNo
      ));
    return 0;
  }
  PrpListHost = (VOID *)(UINTN) NVME_PRP_BASE (Private);

  Bytes = EFI_PAGES_TO_SIZE (PrpListNo);
  PrpListPhyAddr = (UINT64)(UINTN)(PrpListHost);

  //
  // Fill all PRP lists except of last one.
  //
  ZeroMem (PrpListHost, Bytes);
  for (PrpListIndex = 0; PrpListIndex < PrpListNo - 1; ++PrpListIndex) {
    PrpListBase = (UINTN)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;

    for (PrpEntryIndex = 0; PrpEntryIndex < PrpEntryNo; ++PrpEntryIndex) {
      PrpEntry = (UINT8 *)(UINTN) (PrpListBase + PrpEntryIndex * sizeof(UINT64));
      if (PrpEntryIndex != PrpEntryNo - 1) {
        //
        // Fill all PRP entries except of last one.
        //
        CopyMem (PrpEntry, (VOID *)(UINTN) (&PhysicalAddr), sizeof (UINT64));
        PhysicalAddr += EFI_PAGE_SIZE;
      } else {
        //
        // Fill last PRP entries with next PRP List pointer.
        //
        NewPhyAddr = (PrpListPhyAddr + (PrpListIndex + 1) * EFI_PAGE_SIZE);
        CopyMem (PrpEntry, (VOID *)(UINTN) (&NewPhyAddr), sizeof (UINT64));
      }
    }
  }

  //
  // Fill last PRP list.
  //
  PrpListBase = (UINTN)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;
  for (PrpEntryIndex = 0; PrpEntryIndex < ((Remainder != 0) ? Remainder : PrpEntryNo); ++PrpEntryIndex) {
    PrpEntry = (UINT8 *)(UINTN) (PrpListBase + PrpEntryIndex * sizeof(UINT64));
    CopyMem (PrpEntry, (VOID *)(UINTN) (&PhysicalAddr), sizeof (UINT64));

    PhysicalAddr += EFI_PAGE_SIZE;
  }

  return PrpListPhyAddr;
}

/**
  Check the execution status from a given completion queue entry.

  @param[in] Cq    A pointer to the NVME_CQ item.

**/
EFI_STATUS
NvmeCheckCqStatus (
  IN NVME_CQ             *Cq
  )
{
  if (Cq->Sct == 0x0 && Cq->Sc == 0x0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "Dump NVMe Completion Entry Status from [0x%x]:\n", (UINTN)Cq));
  DEBUG ((
    DEBUG_INFO,
    "  SQ Identifier : [0x%x], Phase Tag : [%d], Cmd Identifier : [0x%x]\n",
    Cq->Sqid,
    Cq->Pt,
    Cq->Cid
    ));
  DEBUG ((DEBUG_INFO, "  Status Code Type : [0x%x], Status Code : [0x%x]\n", Cq->Sct, Cq->Sc));
  DEBUG ((DEBUG_INFO, "  NVMe Cmd Execution Result - "));

  switch (Cq->Sct) {
    case 0x0:
      switch (Cq->Sc) {
        case 0x0:
          DEBUG ((DEBUG_INFO, "Successful Completion\n"));
          return EFI_SUCCESS;
        case 0x1:
          DEBUG ((DEBUG_INFO, "Invalid Command Opcode\n"));
          break;
        case 0x2:
          DEBUG ((DEBUG_INFO, "Invalid Field in Command\n"));
          break;
        case 0x3:
          DEBUG ((DEBUG_INFO, "Command ID Conflict\n"));
          break;
        case 0x4:
          DEBUG ((DEBUG_INFO, "Data Transfer Error\n"));
          break;
        case 0x5:
          DEBUG ((DEBUG_INFO, "Commands Aborted due to Power Loss Notification\n"));
          break;
        case 0x6:
          DEBUG ((DEBUG_INFO, "Internal Device Error\n"));
          break;
        case 0x7:
          DEBUG ((DEBUG_INFO, "Command Abort Requested\n"));
          break;
        case 0x8:
          DEBUG ((DEBUG_INFO, "Command Aborted due to SQ Deletion\n"));
          break;
        case 0x9:
          DEBUG ((DEBUG_INFO, "Command Aborted due to Failed Fused Command\n"));
          break;
        case 0xA:
          DEBUG ((DEBUG_INFO, "Command Aborted due to Missing Fused Command\n"));
          break;
        case 0xB:
          DEBUG ((DEBUG_INFO, "Invalid Namespace or Format\n"));
          break;
        case 0xC:
          DEBUG ((DEBUG_INFO, "Command Sequence Error\n"));
          break;
        case 0xD:
          DEBUG ((DEBUG_INFO, "Invalid SGL Last Segment Descriptor\n"));
          break;
        case 0xE:
          DEBUG ((DEBUG_INFO, "Invalid Number of SGL Descriptors\n"));
          break;
        case 0xF:
          DEBUG ((DEBUG_INFO, "Data SGL Length Invalid\n"));
          break;
        case 0x10:
          DEBUG ((DEBUG_INFO, "Metadata SGL Length Invalid\n"));
          break;
        case 0x11:
          DEBUG ((DEBUG_INFO, "SGL Descriptor Type Invalid\n"));
          break;
        case 0x80:
          DEBUG ((DEBUG_INFO, "LBA Out of Range\n"));
          break;
        case 0x81:
          DEBUG ((DEBUG_INFO, "Capacity Exceeded\n"));
          break;
        case 0x82:
          DEBUG ((DEBUG_INFO, "Namespace Not Ready\n"));
          break;
        case 0x83:
          DEBUG ((DEBUG_INFO, "Reservation Conflict\n"));
          break;
      }
      break;

    case 0x1:
      switch (Cq->Sc) {
        case 0x0:
          DEBUG ((DEBUG_INFO, "Completion Queue Invalid\n"));
          break;
        case 0x1:
          DEBUG ((DEBUG_INFO, "Invalid Queue Identifier\n"));
          break;
        case 0x2:
          DEBUG ((DEBUG_INFO, "Maximum Queue Size Exceeded\n"));
          break;
        case 0x3:
          DEBUG ((DEBUG_INFO, "Abort Command Limit Exceeded\n"));
          break;
        case 0x5:
          DEBUG ((DEBUG_INFO, "Asynchronous Event Request Limit Exceeded\n"));
          break;
        case 0x6:
          DEBUG ((DEBUG_INFO, "Invalid Firmware Slot\n"));
          break;
        case 0x7:
          DEBUG ((DEBUG_INFO, "Invalid Firmware Image\n"));
          break;
        case 0x8:
          DEBUG ((DEBUG_INFO, "Invalid Interrupt Vector\n"));
          break;
        case 0x9:
          DEBUG ((DEBUG_INFO, "Invalid Log Page\n"));
          break;
        case 0xA:
          DEBUG ((DEBUG_INFO, "Invalid Format\n"));
          break;
        case 0xB:
          DEBUG ((DEBUG_INFO, "Firmware Application Requires Conventional Reset\n"));
          break;
        case 0xC:
          DEBUG ((DEBUG_INFO, "Invalid Queue Deletion\n"));
          break;
        case 0xD:
          DEBUG ((DEBUG_INFO, "Feature Identifier Not Saveable\n"));
          break;
        case 0xE:
          DEBUG ((DEBUG_INFO, "Feature Not Changeable\n"));
          break;
        case 0xF:
          DEBUG ((DEBUG_INFO, "Feature Not Namespace Specific\n"));
          break;
        case 0x10:
          DEBUG ((DEBUG_INFO, "Firmware Application Requires NVM Subsystem Reset\n"));
          break;
        case 0x80:
          DEBUG ((DEBUG_INFO, "Conflicting Attributes\n"));
          break;
        case 0x81:
          DEBUG ((DEBUG_INFO, "Invalid Protection Information\n"));
          break;
        case 0x82:
          DEBUG ((DEBUG_INFO, "Attempted Write to Read Only Range\n"));
          break;
      }
      break;

    case 0x2:
      switch (Cq->Sc) {
        case 0x80:
          DEBUG ((DEBUG_INFO, "Write Fault\n"));
          break;
        case 0x81:
          DEBUG ((DEBUG_INFO, "Unrecovered Read Error\n"));
          break;
        case 0x82:
          DEBUG ((DEBUG_INFO, "End-to-end Guard Check Error\n"));
          break;
        case 0x83:
          DEBUG ((DEBUG_INFO, "End-to-end Application Tag Check Error\n"));
          break;
        case 0x84:
          DEBUG ((DEBUG_INFO, "End-to-end Reference Tag Check Error\n"));
          break;
        case 0x85:
          DEBUG ((DEBUG_INFO, "Compare Failure\n"));
          break;
        case 0x86:
          DEBUG ((DEBUG_INFO, "Access Denied\n"));
          break;
      }
      break;

    default:
      DEBUG ((DEBUG_INFO, "Unknown error\n"));
      break;
  }

  return EFI_DEVICE_ERROR;
}

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
  )
{
  EFI_STATUS               Status;
  NVME_SQ                  *Sq;
  NVME_CQ                  *Cq;
  UINT8                    QueueId;
  UINTN                    SqSize;
  UINTN                    CqSize;
  EDKII_IOMMU_OPERATION    MapOp;
  UINTN                    MapLength;
  EFI_PHYSICAL_ADDRESS     PhyAddr;
  VOID                     *MapData;
  VOID                     *MapMeta;
  UINT32                   Bytes;
  UINT32                   Offset;
  UINT32                   Data32;
  UINT64                   Timer;

  //
  // Check the data fields in Packet parameter
  //
  if (Packet == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a, Invalid parameter: Packet(%lx)\n",
      __FUNCTION__,
      (UINTN)Packet
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->NvmeCmd == NULL) || (Packet->NvmeCompletion == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a, Invalid parameter: NvmeCmd (%lx)/NvmeCompletion(%lx)\n",
      __FUNCTION__,
      (UINTN)Packet->NvmeCmd,
      (UINTN)Packet->NvmeCompletion
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (Packet->QueueType != NVME_ADMIN_QUEUE && Packet->QueueType != NVME_IO_QUEUE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a, Invalid parameter: QueueId(%lx)\n",
      __FUNCTION__,
      (UINTN)Packet->QueueType
      ));
    return EFI_INVALID_PARAMETER;
  }

  QueueId = Packet->QueueType;
  Sq      = Private->SqBuffer[QueueId] + Private->SqTdbl[QueueId].Sqt;
  Cq      = Private->CqBuffer[QueueId] + Private->CqHdbl[QueueId].Cqh;
  if (QueueId == NVME_ADMIN_QUEUE) {
    SqSize = NVME_ASQ_SIZE + 1;
    CqSize = NVME_ACQ_SIZE + 1;
  } else {
    SqSize = NVME_CSQ_SIZE + 1;
    CqSize = NVME_CCQ_SIZE + 1;
  }

  if (Packet->NvmeCmd->Nsid != NamespaceId) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Nsid mismatch (%x, %x)\n",
      __FUNCTION__,
      Packet->NvmeCmd->Nsid,
      NamespaceId
      ));
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Sq, sizeof (NVME_SQ));
  Sq->Opc  = Packet->NvmeCmd->Cdw0.Opcode;
  Sq->Fuse = Packet->NvmeCmd->Cdw0.FusedOperation;
  Sq->Cid  = Packet->NvmeCmd->Cdw0.Cid;
  Sq->Nsid = Packet->NvmeCmd->Nsid;

  //
  // Currently we only support PRP for data transfer, SGL is NOT supported
  //
  ASSERT (Sq->Psdt == 0);
  if (Sq->Psdt != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Does not support SGL mechanism.\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  Sq->Prp[0] = (UINT64)(UINTN)Packet->TransferBuffer;
  Sq->Prp[1] = 0;
  MapData    = NULL;
  MapMeta    = NULL;
  Status     = EFI_SUCCESS;
  //
  // If the NVMe cmd has data in or out, then mapping the user buffer to the PCI controller
  // specific addresses.
  //
  if ((Sq->Opc & (BIT0 | BIT1)) != 0) {
    if (((Packet->TransferLength != 0) && (Packet->TransferBuffer == NULL)) ||
        ((Packet->TransferLength == 0) && (Packet->TransferBuffer != NULL))) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Currently, we only support creating IO submission/completion queues that are
    // allocated internally by the driver.
    //
    if ((Packet->QueueType == NVME_ADMIN_QUEUE) &&
        ((Sq->Opc == NVME_ADMIN_CRIOCQ_CMD) || (Sq->Opc == NVME_ADMIN_CRIOSQ_CMD))) {
      if ((Packet->TransferBuffer != Private->SqBuffer[NVME_IO_QUEUE]) &&
          (Packet->TransferBuffer != Private->CqBuffer[NVME_IO_QUEUE])) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Does not support external IO queues creation request.\n",
          __FUNCTION__
          ));
        return EFI_UNSUPPORTED;
      }
    } else {
      if ((Sq->Opc & BIT0) != 0) {
        MapOp = EdkiiIoMmuOperationBusMasterRead;
      } else {
        MapOp = EdkiiIoMmuOperationBusMasterWrite;
      }

      if ((Packet->TransferLength != 0) && (Packet->TransferBuffer != NULL)) {
        MapLength = Packet->TransferLength;
        Status = IoMmuMap (
                   MapOp,
                   Packet->TransferBuffer,
                   &MapLength,
                   &PhyAddr,
                   &MapData
                   );
        if (EFI_ERROR (Status) || (MapLength != Packet->TransferLength)) {
          Status = EFI_OUT_OF_RESOURCES;
          DEBUG ((DEBUG_ERROR, "%a: Fail to map data buffer.\n", __FUNCTION__));
          goto Exit;
        }

        Sq->Prp[0] = PhyAddr;
      }

      if((Packet->MetadataLength != 0) && (Packet->MetadataBuffer != NULL)) {
        MapLength = Packet->MetadataLength;
        Status = IoMmuMap (
                   MapOp,
                   Packet->MetadataBuffer,
                   &MapLength,
                   &PhyAddr,
                   &MapMeta
                   );
        if (EFI_ERROR (Status) || (MapLength != Packet->MetadataLength)) {
          Status = EFI_OUT_OF_RESOURCES;
          DEBUG ((DEBUG_ERROR, "%a: Fail to map meta data buffer.\n", __FUNCTION__));
          goto Exit;
        }
        Sq->Mptr = PhyAddr;
      }
    }
  }

  //
  // If the Buffer Size spans more than two memory pages (page Size as defined in CC.Mps),
  // then build a PRP list in the second PRP submission queue entry.
  //
  Offset = ((UINT32)Sq->Prp[0]) & (EFI_PAGE_SIZE - 1);
  Bytes  = Packet->TransferLength;

  if ((Offset + Bytes) > (EFI_PAGE_SIZE * 2)) {
    //
    // Create PrpList for remaining Data Buffer.
    //
    PhyAddr = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
    Sq->Prp[1] = NvmeCreatePrpList (
                   Private,
                   PhyAddr,
                   EFI_SIZE_TO_PAGES(Offset + Bytes) - 1
                   );
    if (Sq->Prp[1] == 0) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((DEBUG_ERROR, "%a: Create PRP list fail, Status - %r\n", __FUNCTION__, Status));
      goto Exit;
    }

  } else if ((Offset + Bytes) > EFI_PAGE_SIZE) {
    Sq->Prp[1] = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
  }

  if (Packet->NvmeCmd->Flags & CDW10_VALID) {
    Sq->Payload.Raw.Cdw10 = Packet->NvmeCmd->Cdw10;
  }
  if (Packet->NvmeCmd->Flags & CDW11_VALID) {
    Sq->Payload.Raw.Cdw11 = Packet->NvmeCmd->Cdw11;
  }
  if (Packet->NvmeCmd->Flags & CDW12_VALID) {
    Sq->Payload.Raw.Cdw12 = Packet->NvmeCmd->Cdw12;
  }
  if (Packet->NvmeCmd->Flags & CDW13_VALID) {
    Sq->Payload.Raw.Cdw13 = Packet->NvmeCmd->Cdw13;
  }
  if (Packet->NvmeCmd->Flags & CDW14_VALID) {
    Sq->Payload.Raw.Cdw14 = Packet->NvmeCmd->Cdw14;
  }
  if (Packet->NvmeCmd->Flags & CDW15_VALID) {
    Sq->Payload.Raw.Cdw15 = Packet->NvmeCmd->Cdw15;
  }

  //
  // Ring the submission queue doorbell.
  //
  Private->SqTdbl[QueueId].Sqt++;
  if (Private->SqTdbl[QueueId].Sqt == SqSize) {
    Private->SqTdbl[QueueId].Sqt = 0;
  }
  Data32 = ReadUnaligned32 ((UINT32 *)&Private->SqTdbl[QueueId]);
  Status = NVME_SET_SQTDBL (Private, QueueId, &Data32);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NVME_SET_SQTDBL fail, Status - %r\n", __FUNCTION__, Status));
    goto Exit;
  }

  //
  // Wait for completion queue to get filled in.
  //
  Status = EFI_TIMEOUT;
  Timer  = 0;
  while (Timer < Packet->CommandTimeout) {
    if (Cq->Pt != Private->Pt[QueueId]) {
      Status = EFI_SUCCESS;
      break;
    }

    MicroSecondDelay (NVME_POLL_INTERVAL);
    Timer += NVME_POLL_INTERVAL;
  }

  if (Status == EFI_TIMEOUT) {
    //
    // Timeout occurs for an NVMe command, reset the controller to abort the outstanding command
    //
    DEBUG ((DEBUG_ERROR, "%a: Timeout occurs for the PassThru command.\n", __FUNCTION__));
    Status = NvmeControllerInit (Private);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
    } else {
      //
      // Return EFI_TIMEOUT to indicate a timeout occurs for PassThru command
      //
      Status = EFI_TIMEOUT;
    }
    goto Exit;
  }

  //
  // Move forward the Completion Queue head
  //
  Private->CqHdbl[QueueId].Cqh++;
  if (Private->CqHdbl[QueueId].Cqh == CqSize) {
    Private->CqHdbl[QueueId].Cqh = 0;
    Private->Pt[QueueId] ^= 1;
  }

  //
  // Copy the Respose Queue entry for this command to the callers response buffer
  //
  CopyMem (Packet->NvmeCompletion, Cq, sizeof (EDKII_PEI_NVM_EXPRESS_COMPLETION));

  //
  // Check the NVMe cmd execution result
  //
  Status = NvmeCheckCqStatus (Cq);
  NVME_SET_CQHDBL (Private, QueueId, &Private->CqHdbl[QueueId]);

Exit:
  if (MapMeta != NULL) {
    IoMmuUnmap (MapMeta);
  }

  if (MapData != NULL) {
    IoMmuUnmap (MapData);
  }

  return Status;
}

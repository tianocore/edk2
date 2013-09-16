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

#include "NvmExpress.h"

//
// Page size should be set in the Controller Configuration register
// during controller init, and the controller configuration save in
// the controller's private data.  The Max and Min supported page sizes
// for the controller are specified in the Controller Capabilities register.
//

GLOBAL_REMOVE_IF_UNREFERENCED NVM_EXPRESS_PASS_THRU_MODE gNvmExpressPassThruMode = {
  0,
  NVM_EXPRESS_PASS_THRU_ATTRIBUTES_PHYSICAL | NVM_EXPRESS_PASS_THRU_ATTRIBUTES_CMD_SET_NVME,
  sizeof (UINTN),
  0x10000,
  0,
  0
};


/**
  Dump the execution status from a given completion queue entry.

  @param[in]     Cq               A pointer to the NVME_CQ item.

**/
VOID
NvmeDumpStatus (
  IN NVME_CQ             *Cq
  )
{
  DEBUG ((EFI_D_VERBOSE, "Dump NVMe Completion Entry Status from [0x%x]:\n", Cq));

  DEBUG ((EFI_D_VERBOSE, "  SQ Identifier : [0x%x], Phase Tag : [%d], Cmd Identifier : [0x%x]\n", Cq->Sqid, Cq->Pt, Cq->Cid));

  DEBUG ((EFI_D_VERBOSE, "  NVMe Cmd Execution Result - "));

  switch (Cq->Sct) {
    case 0x0:
      switch (Cq->Sc) {
        case 0x0:
          DEBUG ((EFI_D_VERBOSE, "Successful Completion\n"));
          break;
        case 0x1:
          DEBUG ((EFI_D_VERBOSE, "Invalid Command Opcode\n"));
          break;
        case 0x2:
          DEBUG ((EFI_D_VERBOSE, "Invalid Field in Command\n"));
          break;
        case 0x3:
          DEBUG ((EFI_D_VERBOSE, "Command ID Conflict\n"));
          break;
        case 0x4:
          DEBUG ((EFI_D_VERBOSE, "Data Transfer Error\n"));
          break;
        case 0x5:
          DEBUG ((EFI_D_VERBOSE, "Commands Aborted due to Power Loss Notification\n"));
          break;
        case 0x6:
          DEBUG ((EFI_D_VERBOSE, "Internal Device Error\n"));
          break;
        case 0x7:
          DEBUG ((EFI_D_VERBOSE, "Command Abort Requested\n"));
          break;
        case 0x8:
          DEBUG ((EFI_D_VERBOSE, "Command Aborted due to SQ Deletion\n"));
          break;
        case 0x9:
          DEBUG ((EFI_D_VERBOSE, "Command Aborted due to Failed Fused Command\n"));
          break;
        case 0xA:
          DEBUG ((EFI_D_VERBOSE, "Command Aborted due to Missing Fused Command\n"));
          break;
        case 0xB:
          DEBUG ((EFI_D_VERBOSE, "Invalid Namespace or Format\n"));
          break;
        case 0xC:
          DEBUG ((EFI_D_VERBOSE, "Command Sequence Error\n"));
          break;
        case 0xD:
          DEBUG ((EFI_D_VERBOSE, "Invalid SGL Last Segment Descriptor\n"));
          break;
        case 0xE:
          DEBUG ((EFI_D_VERBOSE, "Invalid Number of SGL Descriptors\n"));
          break;
        case 0xF:
          DEBUG ((EFI_D_VERBOSE, "Data SGL Length Invalid\n"));
          break;
        case 0x10:
          DEBUG ((EFI_D_VERBOSE, "Metadata SGL Length Invalid\n"));
          break;
        case 0x11:
          DEBUG ((EFI_D_VERBOSE, "SGL Descriptor Type Invalid\n"));
          break;
        case 0x80:
          DEBUG ((EFI_D_VERBOSE, "LBA Out of Range\n"));
          break;
        case 0x81:
          DEBUG ((EFI_D_VERBOSE, "Capacity Exceeded\n"));
          break;
        case 0x82:
          DEBUG ((EFI_D_VERBOSE, "Namespace Not Ready\n"));
          break;
        case 0x83:
          DEBUG ((EFI_D_VERBOSE, "Reservation Conflict\n"));
          break;
      }
      break;

    case 0x1:
      switch (Cq->Sc) {
        case 0x0:
          DEBUG ((EFI_D_VERBOSE, "Completion Queue Invalid\n"));
          break;
        case 0x1:
          DEBUG ((EFI_D_VERBOSE, "Invalid Queue Identifier\n"));
          break;
        case 0x2:
          DEBUG ((EFI_D_VERBOSE, "Maximum Queue Size Exceeded\n"));
          break;
        case 0x3:
          DEBUG ((EFI_D_VERBOSE, "Abort Command Limit Exceeded\n"));
          break;
        case 0x5:
          DEBUG ((EFI_D_VERBOSE, "Asynchronous Event Request Limit Exceeded\n"));
          break;
        case 0x6:
          DEBUG ((EFI_D_VERBOSE, "Invalid Firmware Slot\n"));
          break;
        case 0x7:
          DEBUG ((EFI_D_VERBOSE, "Invalid Firmware Image\n"));
          break;
        case 0x8:
          DEBUG ((EFI_D_VERBOSE, "Invalid Interrupt Vector\n"));
          break;
        case 0x9:
          DEBUG ((EFI_D_VERBOSE, "Invalid Log Page\n"));
          break;
        case 0xA:
          DEBUG ((EFI_D_VERBOSE, "Invalid Format\n"));
          break;
        case 0xB:
          DEBUG ((EFI_D_VERBOSE, "Firmware Application Requires Conventional Reset\n"));
          break;
        case 0xC:
          DEBUG ((EFI_D_VERBOSE, "Invalid Queue Deletion\n"));
          break;
        case 0xD:
          DEBUG ((EFI_D_VERBOSE, "Feature Identifier Not Saveable\n"));
          break;
        case 0xE:
          DEBUG ((EFI_D_VERBOSE, "Feature Not Changeable\n"));
          break;
        case 0xF:
          DEBUG ((EFI_D_VERBOSE, "Feature Not Namespace Specific\n"));
          break;
        case 0x10:
          DEBUG ((EFI_D_VERBOSE, "Firmware Application Requires NVM Subsystem Reset\n"));
          break;
        case 0x80:
          DEBUG ((EFI_D_VERBOSE, "Conflicting Attributes\n"));
          break;
        case 0x81:
          DEBUG ((EFI_D_VERBOSE, "Invalid Protection Information\n"));
          break;
        case 0x82:
          DEBUG ((EFI_D_VERBOSE, "Attempted Write to Read Only Range\n"));
          break;
      }
      break;

    case 0x2:
      switch (Cq->Sc) {
        case 0x80:
          DEBUG ((EFI_D_VERBOSE, "Write Fault\n"));
          break;
        case 0x81:
          DEBUG ((EFI_D_VERBOSE, "Unrecovered Read Error\n"));
          break;
        case 0x82:
          DEBUG ((EFI_D_VERBOSE, "End-to-end Guard Check Error\n"));
          break;
        case 0x83:
          DEBUG ((EFI_D_VERBOSE, "End-to-end Application Tag Check Error\n"));
          break;
        case 0x84:
          DEBUG ((EFI_D_VERBOSE, "End-to-end Reference Tag Check Error\n"));
          break;
        case 0x85:
          DEBUG ((EFI_D_VERBOSE, "Compare Failure\n"));
          break;
        case 0x86:
          DEBUG ((EFI_D_VERBOSE, "Access Denied\n"));
          break;
      }
      break;

    default:
      break;
  }
}

/**
  Create PRP lists for data transfer which is larger than 2 memory pages.
  Note here we calcuate the number of required PRP lists and allocate them at one time.

  @param[in]     PciIo               A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in]     PhysicalAddr        The physical base address of data buffer.
  @param[in]     Pages               The number of pages to be transfered.
  @param[out]    PrpListHost         The host base address of PRP lists.
  @param[in,out] PrpListNo           The number of PRP List.
  @param[out]    Mapping             The mapping value returned from PciIo.Map().

  @retval The pointer to the first PRP List of the PRP lists.

**/
VOID*
NvmeCreatePrpList (
  IN     EFI_PCI_IO_PROTOCOL          *PciIo,
  IN     EFI_PHYSICAL_ADDRESS         PhysicalAddr,
  IN     UINTN                        Pages,
     OUT VOID                         **PrpListHost,
  IN OUT UINTN                        *PrpListNo,
     OUT VOID                         **Mapping
  )
{
  UINTN                       PrpEntryNo;
  UINT64                      PrpListBase;
  UINTN                       PrpListIndex;
  UINTN                       PrpEntryIndex;
  UINT64                      Remainder;
  EFI_PHYSICAL_ADDRESS        PrpListPhyAddr;
  UINTN                       Bytes;
  EFI_STATUS                  Status;

  //
  // The number of Prp Entry in a memory page.
  //
  PrpEntryNo = EFI_PAGE_SIZE / sizeof (UINT64);

  //
  // Calculate total PrpList number.
  //
  *PrpListNo = (UINTN)DivU64x64Remainder ((UINT64)Pages, (UINT64)PrpEntryNo, &Remainder);
  if (Remainder != 0) {
    *PrpListNo += 1;
  }

  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    *PrpListNo,
                    PrpListHost,
                    0
                    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Bytes = EFI_PAGES_TO_SIZE (*PrpListNo);
  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    *PrpListHost,
                    &Bytes,
                    &PrpListPhyAddr,
                    Mapping
                    );

  if (EFI_ERROR (Status) || (Bytes != EFI_PAGES_TO_SIZE (*PrpListNo))) {
    DEBUG ((EFI_D_ERROR, "NvmeCreatePrpList: create PrpList failure!\n"));
    goto EXIT;
  }
  //
  // Fill all PRP lists except of last one.
  //
  ZeroMem (*PrpListHost, Bytes);
  for (PrpListIndex = 0; PrpListIndex < *PrpListNo - 1; ++PrpListIndex) {
    PrpListBase = *(UINT8*)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;

    for (PrpEntryIndex = 0; PrpEntryIndex < PrpEntryNo; ++PrpEntryIndex) {
      if (PrpEntryIndex != PrpEntryNo - 1) {
        //
        // Fill all PRP entries except of last one.
        //
        *((UINT64*)(UINTN)PrpListBase + PrpEntryIndex) = PhysicalAddr;
        PhysicalAddr += EFI_PAGE_SIZE;
      } else {
        //
        // Fill last PRP entries with next PRP List pointer.
        //
        *((UINT64*)(UINTN)PrpListBase + PrpEntryIndex) = PrpListPhyAddr + (PrpListIndex + 1) * EFI_PAGE_SIZE;
      }
    }
  }
  //
  // Fill last PRP list.
  //
  PrpListBase = *(UINT64*)PrpListHost + PrpListIndex * EFI_PAGE_SIZE;
  for (PrpEntryIndex = 0; PrpEntryIndex < ((Remainder != 0) ? Remainder : PrpEntryNo); ++PrpEntryIndex) {
    *((UINT64*)(UINTN)PrpListBase + PrpEntryIndex) = PhysicalAddr;
    PhysicalAddr += EFI_PAGE_SIZE;
  }

  return (VOID*)(UINTN)PrpListPhyAddr;

EXIT:
  PciIo->FreeBuffer (PciIo, *PrpListNo, *PrpListHost);
  return NULL;
}


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
EFI_STATUS
EFIAPI
NvmExpressPassThru (
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN     UINT32                                      NamespaceId,
  IN     UINT64                                      NamespaceUuid,
  IN OUT NVM_EXPRESS_PASS_THRU_COMMAND_PACKET        *Packet,
  IN     EFI_EVENT                                   Event OPTIONAL
  )
{
  NVME_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  NVME_SQ                       *Sq;
  NVME_CQ                       *Cq;
  UINT8                         Qid;
  UINT32                        Bytes;
  UINT16                        Offset;
  EFI_EVENT                     TimerEvent;
  EFI_PCI_IO_PROTOCOL_OPERATION Flag;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  VOID                          *MapData;
  VOID                          *MapMeta;
  VOID                          *MapPrpList;
  UINTN                         MapLength;
  UINT64                        *Prp;
  VOID                          *PrpListHost;
  UINTN                         PrpListNo;
  UINT32                        Data;

  //
  // check the data fields in Packet parameter.
  //
  if ((This == NULL) || (Packet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->NvmeCmd == NULL) || (Packet->NvmeResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Packet->QueueId != NVME_ADMIN_QUEUE && Packet->QueueId != NVME_IO_QUEUE) {
    return EFI_INVALID_PARAMETER;
  }

  Private     = NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU (This);
  PciIo       = Private->PciIo;
  MapData     = NULL;
  MapMeta     = NULL;
  MapPrpList  = NULL;
  PrpListHost = NULL;
  PrpListNo   = 0;
  Prp         = NULL;
  TimerEvent  = NULL;
  Status      = EFI_SUCCESS;

  Qid = Packet->QueueId;
  Sq  = Private->SqBuffer[Qid] + Private->SqTdbl[Qid].Sqt;
  Cq  = Private->CqBuffer[Qid] + Private->CqHdbl[Qid].Cqh;

  if (Packet->NvmeCmd->Nsid != NamespaceId) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Sq, sizeof (NVME_SQ));
  Sq->Opc  = Packet->NvmeCmd->Cdw0.Opcode;
  Sq->Fuse = Packet->NvmeCmd->Cdw0.FusedOperation;
  Sq->Cid  = Packet->NvmeCmd->Cdw0.Cid;
  Sq->Nsid = Packet->NvmeCmd->Nsid;

  //
  // Currently we only support PRP for data transfer, SGL is NOT supported.
  //
  ASSERT (Sq->Psdt == 0);
  if (Sq->Psdt != 0) {
    DEBUG ((EFI_D_ERROR, "NvmExpressPassThru: doesn't support SGL mechanism\n"));
    return EFI_UNSUPPORTED;
  }

  Sq->Prp[0] = (UINT64)(UINTN)Packet->TransferBuffer;
  //
  // If the NVMe cmd has data in or out, then mapping the user buffer to the PCI controller specific addresses.
  // Note here we don't handle data buffer for CreateIOSubmitionQueue and CreateIOCompletionQueue cmds because
  // these two cmds are special which requires their data buffer must support simultaneous access by both the
  // processor and a PCI Bus Master. It's caller's responsbility to ensure this.
  //
  if (((Sq->Opc & (BIT0 | BIT1)) != 0) && (Sq->Opc != NVME_ADMIN_CRIOCQ_OPC) && (Sq->Opc != NVME_ADMIN_CRIOSQ_OPC)) {
    if ((Sq->Opc & BIT0) != 0) {
      Flag = EfiPciIoOperationBusMasterRead;
    } else {
      Flag = EfiPciIoOperationBusMasterWrite;
    }

    MapLength = Packet->TransferLength;
    Status = PciIo->Map (
                      PciIo,
                      Flag,
                      Packet->TransferBuffer,
                      &MapLength,
                      &PhyAddr,
                      &MapData
                      );
    if (EFI_ERROR (Status) || (Packet->TransferLength != MapLength)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Sq->Prp[0] = PhyAddr;
    Sq->Prp[1] = 0;

    MapLength = Packet->MetadataLength;
    if(Packet->MetadataBuffer != NULL) {
      MapLength = Packet->MetadataLength;
      Status = PciIo->Map (
                        PciIo,
                        Flag,
                        Packet->MetadataBuffer,
                        &MapLength,
                        &PhyAddr,
                        &MapMeta
                        );
      if (EFI_ERROR (Status) || (Packet->MetadataLength != MapLength)) {
        PciIo->Unmap (
                 PciIo,
                 MapData
                 );

        return EFI_OUT_OF_RESOURCES;
      }
      Sq->Mptr = PhyAddr;
    }
  }
  //
  // If the buffer size spans more than two memory pages (page size as defined in CC.Mps),
  // then build a PRP list in the second PRP submission queue entry.
  //
  Offset = ((UINT16)Sq->Prp[0]) & (EFI_PAGE_SIZE - 1);
  Bytes  = Packet->TransferLength;

  if ((Offset + Bytes) > (EFI_PAGE_SIZE * 2)) {
    //
    // Create PrpList for remaining data buffer.
    //
    PhyAddr = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
    Prp = NvmeCreatePrpList (PciIo, PhyAddr, EFI_SIZE_TO_PAGES(Offset + Bytes) - 1, &PrpListHost, &PrpListNo, &MapPrpList);
    if (Prp == NULL) {
      goto EXIT;
    }

    Sq->Prp[1] = (UINT64)(UINTN)Prp;
  } else if ((Offset + Bytes) > EFI_PAGE_SIZE) {
    Sq->Prp[1] = (Sq->Prp[0] + EFI_PAGE_SIZE) & ~(EFI_PAGE_SIZE - 1);
  }

  if(Packet->NvmeCmd->Flags & CDW10_VALID) {
    Sq->Payload.Raw.Cdw10 = Packet->NvmeCmd->Cdw10;
  }
  if(Packet->NvmeCmd->Flags & CDW11_VALID) {
    Sq->Payload.Raw.Cdw11 = Packet->NvmeCmd->Cdw11;
  }
  if(Packet->NvmeCmd->Flags & CDW12_VALID) {
    Sq->Payload.Raw.Cdw12 = Packet->NvmeCmd->Cdw12;
  }
  if(Packet->NvmeCmd->Flags & CDW13_VALID) {
    Sq->Payload.Raw.Cdw13 = Packet->NvmeCmd->Cdw13;
  }
  if(Packet->NvmeCmd->Flags & CDW14_VALID) {
    Sq->Payload.Raw.Cdw14 = Packet->NvmeCmd->Cdw14;
  }
  if(Packet->NvmeCmd->Flags & CDW15_VALID) {
    Sq->Payload.Raw.Cdw15 = Packet->NvmeCmd->Cdw15;
  }

  //
  // Ring the submission queue doorbell.
  //
  Private->SqTdbl[Qid].Sqt ^= 1;
  Data = ReadUnaligned32 ((UINT32*)&Private->SqTdbl[Qid]);
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint32,
               NVME_BAR,
               NVME_SQTDBL_OFFSET(Qid, Private->Cap.Dstrd),
               1,
               &Data
               );

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  Status = gBS->SetTimer(TimerEvent, TimerRelative, Packet->CommandTimeout);

  if (EFI_ERROR(Status)) {
    Packet->ControllerStatus = NVM_EXPRESS_STATUS_CONTROLLER_DEVICE_ERROR;
    goto EXIT;
  }

  //
  // Wait for completion queue to get filled in.
  //
  Status = EFI_TIMEOUT;
  Packet->ControllerStatus = NVM_EXPRESS_STATUS_CONTROLLER_TIMEOUT_COMMAND;
  while (EFI_ERROR (gBS->CheckEvent (TimerEvent))) {
    if (Cq->Pt != Private->Pt[Qid]) {
      Status = EFI_SUCCESS;
      Packet->ControllerStatus = NVM_EXPRESS_STATUS_CONTROLLER_READY;
      break;
    }
  }

  if ((Private->CqHdbl[Qid].Cqh ^= 1) == 0) {
    Private->Pt[Qid] ^= 1;
  }

  //
  // Copy the Respose Queue entry for this command to the callers response buffer
  //
  CopyMem(Packet->NvmeResponse, Cq, sizeof(NVM_EXPRESS_RESPONSE));

  //
  // Dump every completion entry status for debugging.
  //
  DEBUG_CODE_BEGIN();
    NvmeDumpStatus(Cq);
  DEBUG_CODE_END();

  Data = ReadUnaligned32 ((UINT32*)&Private->CqHdbl[Qid]);
  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint32,
               NVME_BAR,
               NVME_CQHDBL_OFFSET(Qid, Private->Cap.Dstrd),
               1,
               &Data
               );

EXIT:
  if (MapData != NULL) {
    PciIo->Unmap (
             PciIo,
             MapData
             );
  }

  if (MapMeta != NULL) {
    PciIo->Unmap (
             PciIo,
             MapMeta
             );
  }

  if (MapPrpList != NULL) {
    PciIo->Unmap (
             PciIo,
             MapPrpList
             );
  }

  if (Prp != NULL) {
    PciIo->FreeBuffer (PciIo, PrpListNo, PrpListHost);
  }

  if (TimerEvent != NULL) {
    gBS->CloseEvent (TimerEvent);
  }
  return Status;
}

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
EFI_STATUS
EFIAPI
NvmExpressGetNextNamespace (
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN OUT UINT32                                      *NamespaceId,
     OUT UINT64                                      *NamespaceUuid  OPTIONAL
  )
{
  NVME_CONTROLLER_PRIVATE_DATA     *Private;
  NVME_ADMIN_NAMESPACE_DATA        *NamespaceData;
  UINT32                           NextNamespaceId;
  EFI_STATUS                       Status;

  if ((This == NULL) || (NamespaceId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NamespaceData = NULL;
  Status        = EFI_NOT_FOUND;

  Private = NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU (This);
  //
  // If the NamespaceId input value is 0xFFFFFFFF, then get the first valid namespace ID
  //
  if (*NamespaceId == 0xFFFFFFFF) {
    //
    // Start with the first namespace ID
    //
    NextNamespaceId = 1;
    //
    // Allocate buffer for Identify Namespace data.
    //
    NamespaceData = (NVME_ADMIN_NAMESPACE_DATA *)AllocateZeroPool (sizeof (NVME_ADMIN_NAMESPACE_DATA));

    if (NamespaceData == NULL) {
      return EFI_NOT_FOUND;
    }

    Status = NvmeIdentifyNamespace (Private, NextNamespaceId, NamespaceData);
    if (EFI_ERROR(Status)) {
      goto Done;
    }

    *NamespaceId = NextNamespaceId;
    if (NamespaceUuid != NULL) {
      *NamespaceUuid = NamespaceData->Eui64;
    }
  } else {
    if (*NamespaceId >= Private->ControllerData->Nn) {
      return EFI_INVALID_PARAMETER;
    }

    NextNamespaceId = *NamespaceId + 1;
    //
    // Allocate buffer for Identify Namespace data.
    //
    NamespaceData = (NVME_ADMIN_NAMESPACE_DATA *)AllocateZeroPool (sizeof (NVME_ADMIN_NAMESPACE_DATA));
    if (NamespaceData == NULL) {
      return EFI_NOT_FOUND;
    }

    Status = NvmeIdentifyNamespace (Private, NextNamespaceId, NamespaceData);
    if (EFI_ERROR(Status)) {
      goto Done;
    }

    *NamespaceId = NextNamespaceId;
    if (NamespaceUuid != NULL) {
      *NamespaceUuid = NamespaceData->Eui64;
    }
  }

Done:
  if (NamespaceData != NULL) {
    FreePool(NamespaceData);
  }

  return Status;
}

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
EFI_STATUS
EFIAPI
NvmExpressGetNamespace (
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN     EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
     OUT UINT32                                      *NamespaceId,
     OUT UINT64                                      *NamespaceUuid
  )
{
  NVME_NAMESPACE_DEVICE_PATH       *Node;

  if ((This == NULL) || (DevicePath == NULL) || (NamespaceId == NULL) || (NamespaceUuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DevicePath->Type != MESSAGING_DEVICE_PATH) {
    return EFI_UNSUPPORTED;
  }

  Node = (NVME_NAMESPACE_DEVICE_PATH *)DevicePath;

  if (DevicePath->SubType == MSG_NVME_NAMESPACE_DP) {
    if (DevicePathNodeLength(DevicePath) != sizeof(NVME_NAMESPACE_DEVICE_PATH)) {
      return EFI_NOT_FOUND;
    }

    *NamespaceId   = Node->NamespaceId;
    *NamespaceUuid = Node->NamespaceUuid;

    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

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
EFI_STATUS
EFIAPI
NvmExpressBuildDevicePath (
  IN     NVM_EXPRESS_PASS_THRU_PROTOCOL              *This,
  IN     UINT32                                      NamespaceId,
  IN     UINT64                                      NamespaceUuid,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                    **DevicePath
  )
{
  NVME_CONTROLLER_PRIVATE_DATA   *Private;
  NVME_NAMESPACE_DEVICE_PATH     *Node;

  //
  // Validate parameters
  //
  if ((This == NULL) || (DevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU (This);

  if (NamespaceId == 0) {
    return EFI_NOT_FOUND;
  }

  Node = (NVME_NAMESPACE_DEVICE_PATH *)AllocateZeroPool (sizeof (NVME_NAMESPACE_DEVICE_PATH));

  if (Node == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Node->Header.Type    = MESSAGING_DEVICE_PATH;
  Node->Header.SubType = MSG_NVME_NAMESPACE_DP;
  SetDevicePathNodeLength (&Node->Header, sizeof (NVME_NAMESPACE_DEVICE_PATH));
  Node->NamespaceId    = NamespaceId;
  Node->NamespaceUuid  = NamespaceUuid;

  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)Node;
  return EFI_SUCCESS;
}


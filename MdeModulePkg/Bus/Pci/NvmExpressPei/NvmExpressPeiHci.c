/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpressPei.h"

/**
  Transfer MMIO Data to memory.

  @param[in,out] MemBuffer    Destination: Memory address.
  @param[in] MmioAddr         Source: MMIO address.
  @param[in] Size             Size for read.

  @retval EFI_SUCCESS         MMIO read sucessfully.

**/
EFI_STATUS
NvmeMmioRead (
  IN OUT VOID *MemBuffer,
  IN     UINTN MmioAddr,
  IN     UINTN Size
  )
{
  UINTN    Offset;
  UINT8    Data;
  UINT8    *Ptr;

  // priority has adjusted
  switch (Size) {
    case 4:
      *((UINT32 *)MemBuffer) = MmioRead32 (MmioAddr);
      break;

    case 8:
      *((UINT64 *)MemBuffer) = MmioRead64 (MmioAddr);
      break;

    case 2:
      *((UINT16 *)MemBuffer) = MmioRead16 (MmioAddr);
      break;

    case 1:
      *((UINT8 *)MemBuffer) = MmioRead8 (MmioAddr);
      break;

    default:
      Ptr = (UINT8 *)MemBuffer;
      for (Offset = 0; Offset < Size; Offset += 1) {
        Data = MmioRead8 (MmioAddr + Offset);
        Ptr[Offset] = Data;
      }
      break;
  }

  return EFI_SUCCESS;
}

/**
  Transfer memory data to MMIO.

  @param[in,out] MmioAddr    Destination: MMIO address.
  @param[in] MemBuffer       Source: Memory address.
  @param[in] Size            Size for write.

  @retval EFI_SUCCESS        MMIO write sucessfully.

**/
EFI_STATUS
NvmeMmioWrite (
  IN OUT UINTN MmioAddr,
  IN     VOID *MemBuffer,
  IN     UINTN Size
  )
{
  UINTN    Offset;
  UINT8    Data;
  UINT8    *Ptr;

  // priority has adjusted
  switch (Size) {
    case 4:
      MmioWrite32 (MmioAddr, *((UINT32 *)MemBuffer));
      break;

    case 8:
      MmioWrite64 (MmioAddr, *((UINT64 *)MemBuffer));
      break;

    case 2:
      MmioWrite16 (MmioAddr, *((UINT16 *)MemBuffer));
      break;

    case 1:
      MmioWrite8 (MmioAddr, *((UINT8 *)MemBuffer));
      break;

    default:
      Ptr = (UINT8 *)MemBuffer;
      for (Offset = 0; Offset < Size; Offset += 1) {
        Data = Ptr[Offset];
        MmioWrite8 (MmioAddr + Offset, Data);
      }
      break;
  }

  return EFI_SUCCESS;
}

/**
  Get the page offset for specific NVME based memory.

  @param[in] BaseMemIndex    The Index of BaseMem (0-based).

  @retval - The page count for specific BaseMem Index

**/
UINT32
NvmeBaseMemPageOffset (
  IN UINTN              BaseMemIndex
  )
{
  UINT32                Pages;
  UINTN                 Index;
  UINT32                PageSizeList[5];

  PageSizeList[0] = 1;  /* ASQ */
  PageSizeList[1] = 1;  /* ACQ */
  PageSizeList[2] = 1;  /* SQs */
  PageSizeList[3] = 1;  /* CQs */
  PageSizeList[4] = NVME_PRP_SIZE;  /* PRPs */

  if (BaseMemIndex > MAX_BASEMEM_COUNT) {
    DEBUG ((DEBUG_ERROR, "%a: The input BaseMem index is invalid.\n", __FUNCTION__));
    ASSERT (FALSE);
    return 0;
  }

  Pages = 0;
  for (Index = 0; Index < BaseMemIndex; Index++) {
    Pages += PageSizeList[Index];
  }

  return Pages;
}

/**
  Wait for NVME controller status to be ready or not.

  @param[in] Private      The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] WaitReady    Flag for waitting status ready or not.

  @return EFI_SUCCESS     Successfully to wait specific status.
  @return others          Fail to wait for specific controller status.

**/
EFI_STATUS
NvmeWaitController (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN BOOLEAN                             WaitReady
  )
{
  NVME_CSTS              Csts;
  EFI_STATUS             Status;
  UINT32                 Index;
  UINT8                  Timeout;

  //
  // Cap.To specifies max delay time in 500ms increments for Csts.Rdy to set after
  // Cc.Enable. Loop produces a 1 millisecond delay per itteration, up to 500 * Cap.To.
  //
  if (Private->Cap.To == 0) {
    Timeout = 1;
  } else {
    Timeout = Private->Cap.To;
  }

  Status = EFI_SUCCESS;
  for(Index = (Timeout * 500); Index != 0; --Index) {
    MicroSecondDelay (1000);

    //
    // Check if the controller is initialized
    //
    Status = NVME_GET_CSTS (Private, &Csts);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "%a: NVME_GET_CSTS fail, Status - %r\n", __FUNCTION__, Status));
      return Status;
    }

    if ((BOOLEAN) Csts.Rdy == WaitReady) {
      break;
    }
  }

  if (Index == 0) {
    Status = EFI_TIMEOUT;
  }

  return Status;
}

/**
  Disable the Nvm Express controller.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS    Successfully disable the controller.
  @return others         Fail to disable the controller.

**/
EFI_STATUS
NvmeDisableController (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  NVME_CC       Cc;
  NVME_CSTS     Csts;
  EFI_STATUS    Status;

  Status = NVME_GET_CSTS (Private, &Csts);

  //
  // Read Controller Configuration Register.
  //
  Status = NVME_GET_CC (Private, &Cc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NVME_GET_CC fail, Status - %r\n", __FUNCTION__, Status));
    goto ErrorExit;
  }

  if (Cc.En == 1) {
    Cc.En = 0;
    //
    // Disable the controller.
    //
    Status = NVME_SET_CC (Private, &Cc);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: NVME_SET_CC fail, Status - %r\n", __FUNCTION__, Status));
      goto ErrorExit;
    }
  }

  Status = NvmeWaitController (Private, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NvmeWaitController fail, Status - %r\n", __FUNCTION__, Status));
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  DEBUG ((DEBUG_ERROR, "%a fail, Status - %r\n", __FUNCTION__, Status));
  return Status;
}

/**
  Enable the Nvm Express controller.

  @param[in] Private    The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS         Successfully enable the controller.
  @return EFI_DEVICE_ERROR    Fail to enable the controller.
  @return EFI_TIMEOUT         Fail to enable the controller in given time slot.

**/
EFI_STATUS
NvmeEnableController (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  NVME_CC       Cc;
  EFI_STATUS    Status;

  //
  // Enable the controller
  // CC.AMS, CC.MPS and CC.CSS are all set to 0
  //
  ZeroMem (&Cc, sizeof (NVME_CC));
  Cc.En     = 1;
  Cc.Iosqes = 6;
  Cc.Iocqes = 4;
  Status    = NVME_SET_CC (Private, &Cc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NVME_SET_CC fail, Status - %r\n", __FUNCTION__, Status));
    goto ErrorExit;
  }

  Status = NvmeWaitController (Private, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NvmeWaitController fail, Status - %r\n", __FUNCTION__, Status));
    goto ErrorExit;
  }

  return EFI_SUCCESS;

ErrorExit:
  DEBUG ((DEBUG_ERROR, "%a fail, Status: %r\n", __FUNCTION__, Status));
  return Status;
}

/**
  Get the Identify Controller data.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Buffer      The Buffer used to store the Identify Controller data.

  @return EFI_SUCCESS    Successfully get the Identify Controller data.
  @return others         Fail to get the Identify Controller data.

**/
EFI_STATUS
NvmeIdentifyController (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN VOID                                *Buffer
  )
{
  EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    CommandPacket;
  EDKII_PEI_NVM_EXPRESS_COMMAND                     Command;
  EDKII_PEI_NVM_EXPRESS_COMPLETION                  Completion;
  EFI_STATUS                                        Status;

  ZeroMem (&CommandPacket, sizeof(EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(EDKII_PEI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof(EDKII_PEI_NVM_EXPRESS_COMPLETION));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  //
  // According to Nvm Express 1.1 spec Figure 38, When not used, the field shall be cleared to 0h.
  // For the Identify command, the Namespace Identifier is only used for the Namespace Data structure.
  //
  Command.Nsid        = 0;

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.TransferBuffer = Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_CONTROLLER_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify the controller
  //
  CommandPacket.NvmeCmd->Cdw10 = 1;
  CommandPacket.NvmeCmd->Flags = CDW10_VALID;

  Status = NvmePassThru (
             Private,
             NVME_CONTROLLER_NSID,
             &CommandPacket
             );
  return Status;
}

/**
  Get specified identify namespace data.

  @param[in] Private        The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] NamespaceId    The specified namespace identifier.
  @param[in] Buffer         The buffer used to store the identify namespace data.

  @return EFI_SUCCESS         Successfully get the identify namespace data.
  @return EFI_DEVICE_ERROR    Fail to get the identify namespace data.

**/
EFI_STATUS
NvmeIdentifyNamespace (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN UINT32                              NamespaceId,
  IN VOID                                *Buffer
  )
{
  EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    CommandPacket;
  EDKII_PEI_NVM_EXPRESS_COMMAND                     Command;
  EDKII_PEI_NVM_EXPRESS_COMPLETION                  Completion;
  EFI_STATUS                                        Status;

  ZeroMem (&CommandPacket, sizeof(EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(EDKII_PEI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof(EDKII_PEI_NVM_EXPRESS_COMPLETION));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  Command.Nsid        = NamespaceId;

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.TransferBuffer = Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_NAMESPACE_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a namespace
  //
  CommandPacket.NvmeCmd->Cdw10 = 0;
  CommandPacket.NvmeCmd->Flags = CDW10_VALID;

  Status = NvmePassThru (
             Private,
             NamespaceId,
             &CommandPacket
             );
  return Status;
}

/**
  Dump the Identify Controller data.

  @param[in] ControllerData    The pointer to the NVME_ADMIN_CONTROLLER_DATA data structure.

**/
VOID
NvmeDumpControllerData (
  IN NVME_ADMIN_CONTROLLER_DATA    *ControllerData
  )
{
  UINT8    Sn[21];
  UINT8    Mn[41];

  CopyMem (Sn, ControllerData->Sn, sizeof (ControllerData->Sn));
  Sn[20] = 0;
  CopyMem (Mn, ControllerData->Mn, sizeof (ControllerData->Mn));
  Mn[40] = 0;

  DEBUG ((DEBUG_INFO, " == NVME IDENTIFY CONTROLLER DATA ==\n"));
  DEBUG ((DEBUG_INFO, "    PCI VID   : 0x%x\n", ControllerData->Vid));
  DEBUG ((DEBUG_INFO, "    PCI SSVID : 0x%x\n", ControllerData->Ssvid));
  DEBUG ((DEBUG_INFO, "    SN        : %a\n",   Sn));
  DEBUG ((DEBUG_INFO, "    MN        : %a\n",   Mn));
  DEBUG ((DEBUG_INFO, "    FR        : 0x%lx\n", *((UINT64*)ControllerData->Fr)));
  DEBUG ((DEBUG_INFO, "    RAB       : 0x%x\n", ControllerData->Rab));
  DEBUG ((DEBUG_INFO, "    IEEE      : 0x%x\n", *(UINT32*)ControllerData->Ieee_oui));
  DEBUG ((DEBUG_INFO, "    AERL      : 0x%x\n", ControllerData->Aerl));
  DEBUG ((DEBUG_INFO, "    SQES      : 0x%x\n", ControllerData->Sqes));
  DEBUG ((DEBUG_INFO, "    CQES      : 0x%x\n", ControllerData->Cqes));
  DEBUG ((DEBUG_INFO, "    NN        : 0x%x\n", ControllerData->Nn));
  return;
}

/**
  Create IO completion queue.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS    Successfully create io completion queue.
  @return others         Fail to create io completion queue.

**/
EFI_STATUS
NvmeCreateIoCompletionQueue (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    CommandPacket;
  EDKII_PEI_NVM_EXPRESS_COMMAND                     Command;
  EDKII_PEI_NVM_EXPRESS_COMPLETION                  Completion;
  EFI_STATUS                                        Status;
  NVME_ADMIN_CRIOCQ                                 CrIoCq;

  ZeroMem (&CommandPacket, sizeof(EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(EDKII_PEI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof(EDKII_PEI_NVM_EXPRESS_COMPLETION));
  ZeroMem (&CrIoCq, sizeof(NVME_ADMIN_CRIOCQ));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;

  Command.Cdw0.Opcode = NVME_ADMIN_CRIOCQ_CMD;
  Command.Cdw0.Cid    = Private->Cid[NVME_ADMIN_QUEUE]++;
  CommandPacket.TransferBuffer = Private->CqBuffer[NVME_IO_QUEUE];
  CommandPacket.TransferLength = EFI_PAGE_SIZE;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;

  CrIoCq.Qid   = NVME_IO_QUEUE;
  CrIoCq.Qsize = NVME_CCQ_SIZE;
  CrIoCq.Pc    = 1;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoCq, sizeof (NVME_ADMIN_CRIOCQ));
  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  Status = NvmePassThru (
             Private,
             NVME_CONTROLLER_NSID,
             &CommandPacket
             );
  return Status;
}

/**
  Create IO submission queue.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS    Successfully create io submission queue.
  @return others         Fail to create io submission queue.

**/
EFI_STATUS
NvmeCreateIoSubmissionQueue (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    CommandPacket;
  EDKII_PEI_NVM_EXPRESS_COMMAND                     Command;
  EDKII_PEI_NVM_EXPRESS_COMPLETION                  Completion;
  EFI_STATUS                                        Status;
  NVME_ADMIN_CRIOSQ                                 CrIoSq;

  ZeroMem (&CommandPacket, sizeof(EDKII_PEI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof(EDKII_PEI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof(EDKII_PEI_NVM_EXPRESS_COMPLETION));
  ZeroMem (&CrIoSq, sizeof(NVME_ADMIN_CRIOSQ));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;

  Command.Cdw0.Opcode = NVME_ADMIN_CRIOSQ_CMD;
  Command.Cdw0.Cid    = Private->Cid[NVME_ADMIN_QUEUE]++;
  CommandPacket.TransferBuffer = Private->SqBuffer[NVME_IO_QUEUE];
  CommandPacket.TransferLength = EFI_PAGE_SIZE;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;

  CrIoSq.Qid   = NVME_IO_QUEUE;
  CrIoSq.Qsize = NVME_CSQ_SIZE;
  CrIoSq.Pc    = 1;
  CrIoSq.Cqid  = NVME_IO_QUEUE;
  CrIoSq.Qprio = 0;
  CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoSq, sizeof (NVME_ADMIN_CRIOSQ));
  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

  Status = NvmePassThru (
             Private,
             NVME_CONTROLLER_NSID,
             &CommandPacket
             );
  return Status;
}

/**
  Initialize the Nvm Express controller.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS    The NVM Express Controller is initialized successfully.
  @retval Others         A device error occurred while initializing the controller.

**/
EFI_STATUS
NvmeControllerInit (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS    Status;
  UINTN         Index;
  NVME_AQA      Aqa;
  NVME_ASQ      Asq;
  NVME_ACQ      Acq;
  NVME_VER      Ver;

  //
  // Dump the NVME controller implementation version
  //
  NVME_GET_VER (Private, &Ver);
  DEBUG ((DEBUG_INFO, "NVME controller implementation version: %d.%d\n", Ver.Mjr, Ver.Mnr));

  //
  // Read the controller Capabilities register and verify that the NVM command set is supported
  //
  NVME_GET_CAP (Private, &Private->Cap);
  if (Private->Cap.Css != 0x01) {
    DEBUG ((DEBUG_ERROR, "%a: The NVME controller doesn't support NVMe command set.\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  //
  // Currently, the driver only supports 4k page size
  //
  if ((Private->Cap.Mpsmin + 12) > EFI_PAGE_SHIFT) {
    DEBUG ((DEBUG_ERROR, "%a: The driver doesn't support page size other than 4K.\n", __FUNCTION__));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < NVME_MAX_QUEUES; Index++) {
    Private->Pt[Index]  = 0;
    Private->Cid[Index] = 0;
    ZeroMem ((VOID *)(UINTN)(&Private->SqTdbl[Index]), sizeof (NVME_SQTDBL));
    ZeroMem ((VOID *)(UINTN)(&Private->CqHdbl[Index]), sizeof (NVME_CQHDBL));
  }
  ZeroMem (Private->Buffer, EFI_PAGE_SIZE * NVME_MEM_MAX_PAGES);

  //
  // Disable the NVME controller first
  //
  Status = NvmeDisableController (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NvmeDisableController fail, Status - %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Set the number of entries in admin submission & completion queues
  //
  Aqa.Asqs  = NVME_ASQ_SIZE;
  Aqa.Rsvd1 = 0;
  Aqa.Acqs  = NVME_ACQ_SIZE;
  Aqa.Rsvd2 = 0;

  //
  // Address of admin submission & completion queues
  //
  Asq = (UINT64)(UINTN)(NVME_ASQ_BASE (Private) & ~0xFFF);
  Acq = (UINT64)(UINTN)(NVME_ACQ_BASE (Private) & ~0xFFF);

  //
  // Address of I/O submission & completion queues
  //
  Private->SqBuffer[0] = (NVME_SQ *)(UINTN)NVME_ASQ_BASE (Private);    // NVME_ADMIN_QUEUE
  Private->CqBuffer[0] = (NVME_CQ *)(UINTN)NVME_ACQ_BASE (Private);    // NVME_ADMIN_QUEUE
  Private->SqBuffer[1] = (NVME_SQ *)(UINTN)NVME_SQ_BASE (Private, 0);  // NVME_IO_QUEUE
  Private->CqBuffer[1] = (NVME_CQ *)(UINTN)NVME_CQ_BASE (Private, 0);  // NVME_IO_QUEUE
  DEBUG ((DEBUG_INFO, "Admin Submission Queue Size (Aqa.Asqs) = [%08X]\n", Aqa.Asqs));
  DEBUG ((DEBUG_INFO, "Admin Completion Queue Size (Aqa.Acqs) = [%08X]\n", Aqa.Acqs));
  DEBUG ((DEBUG_INFO, "Admin Submission Queue (SqBuffer[0]) =   [%08X]\n", Private->SqBuffer[0]));
  DEBUG ((DEBUG_INFO, "Admin Completion Queue (CqBuffer[0]) =   [%08X]\n", Private->CqBuffer[0]));
  DEBUG ((DEBUG_INFO, "I/O   Submission Queue (SqBuffer[1]) =   [%08X]\n", Private->SqBuffer[1]));
  DEBUG ((DEBUG_INFO, "I/O   Completion Queue (CqBuffer[1]) =   [%08X]\n", Private->CqBuffer[1]));

  //
  // Program admin queue attributes
  //
  NVME_SET_AQA (Private, &Aqa);

  //
  // Program admin submission & completion queues address
  //
  NVME_SET_ASQ (Private, &Asq);
  NVME_SET_ACQ (Private, &Acq);

  //
  // Enable the NVME controller
  //
  Status = NvmeEnableController (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NvmeEnableController fail, Status - %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Get the Identify Controller data
  //
  if (Private->ControllerData == NULL) {
    Private->ControllerData = (NVME_ADMIN_CONTROLLER_DATA *)AllocateZeroPool (sizeof (NVME_ADMIN_CONTROLLER_DATA));
    if (Private->ControllerData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }
  Status = NvmeIdentifyController (Private, Private->ControllerData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NvmeIdentifyController fail, Status - %r\n", __FUNCTION__, Status));
    return Status;
  }
  NvmeDumpControllerData (Private->ControllerData);

  //
  // Check the namespace number for storing the namespaces information
  //
  if (Private->ControllerData->Nn > MAX_UINT32 / sizeof (PEI_NVME_NAMESPACE_INFO)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Number of Namespaces field in Identify Controller data not supported by the driver.\n",
      __FUNCTION__
      ));
    return EFI_UNSUPPORTED;
  }

  //
  // Create one I/O completion queue and one I/O submission queue
  //
  Status = NvmeCreateIoCompletionQueue (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Create IO completion queue fail, Status - %r\n", __FUNCTION__, Status));
    return Status;
  }
  Status = NvmeCreateIoSubmissionQueue (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Create IO submission queue fail, Status - %r\n", __FUNCTION__, Status));
  }

  return Status;
}

/**
  Free the DMA resources allocated by an NVME controller.

  @param[in] Private     The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

**/
VOID
NvmeFreeDmaResource (
  IN PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  ASSERT (Private != NULL);

  if (Private->BufferMapping != NULL) {
    IoMmuFreeBuffer (
       NVME_MEM_MAX_PAGES,
       Private->Buffer,
       Private->BufferMapping
       );
  }

  return;
}

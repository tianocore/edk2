/** @file
  NvmExpressDxe driver is used to manage non-volatile memory subsystem which follows
  NVM Express specification.

  Copyright (c) 2013 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpress.h"
#include <Guid/NVMeEventGroup.h>

#define NVME_SHUTDOWN_PROCESS_TIMEOUT  45

//
// The number of NVME controllers managed by this driver, used by
// NvmeRegisterShutdownNotification() and NvmeUnregisterShutdownNotification().
//
UINTN  mNvmeControllerNumber = 0;

/**
  Read Nvm Express controller capability register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Cap              The buffer used to store capability register content.

  @return EFI_SUCCESS      Successfully read the controller capability register content.
  @return EFI_DEVICE_ERROR Fail to read the controller capability register.

**/
EFI_STATUS
ReadNvmeControllerCapabilities (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_CAP                      *Cap
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT64               Data;

  PciIo  = Private->PciIo;
  Status = PciIo->Mem.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CAP_OFFSET,
                        2,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteUnaligned64 ((UINT64 *)Cap, Data);
  return EFI_SUCCESS;
}

/**
  Read Nvm Express controller configuration register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Cc               The buffer used to store configuration register content.

  @return EFI_SUCCESS      Successfully read the controller configuration register content.
  @return EFI_DEVICE_ERROR Fail to read the controller configuration register.

**/
EFI_STATUS
ReadNvmeControllerConfiguration (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_CC                       *Cc
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Status = PciIo->Mem.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CC_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteUnaligned32 ((UINT32 *)Cc, Data);
  return EFI_SUCCESS;
}

/**
  Write Nvm Express controller configuration register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Cc               The buffer used to store the content to be written into configuration register.

  @return EFI_SUCCESS      Successfully write data into the controller configuration register.
  @return EFI_DEVICE_ERROR Fail to write data into the controller configuration register.

**/
EFI_STATUS
WriteNvmeControllerConfiguration (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_CC                       *Cc
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Data   = ReadUnaligned32 ((UINT32 *)Cc);
  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CC_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Cc.En: %d\n", Cc->En));
  DEBUG ((DEBUG_INFO, "Cc.Css: %d\n", Cc->Css));
  DEBUG ((DEBUG_INFO, "Cc.Mps: %d\n", Cc->Mps));
  DEBUG ((DEBUG_INFO, "Cc.Ams: %d\n", Cc->Ams));
  DEBUG ((DEBUG_INFO, "Cc.Shn: %d\n", Cc->Shn));
  DEBUG ((DEBUG_INFO, "Cc.Iosqes: %d\n", Cc->Iosqes));
  DEBUG ((DEBUG_INFO, "Cc.Iocqes: %d\n", Cc->Iocqes));

  return EFI_SUCCESS;
}

/**
  Read Nvm Express controller status register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Csts             The buffer used to store status register content.

  @return EFI_SUCCESS      Successfully read the controller status register content.
  @return EFI_DEVICE_ERROR Fail to read the controller status register.

**/
EFI_STATUS
ReadNvmeControllerStatus (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_CSTS                     *Csts
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Status = PciIo->Mem.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_CSTS_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  WriteUnaligned32 ((UINT32 *)Csts, Data);
  return EFI_SUCCESS;
}

/**
  Write Nvm Express admin queue attributes register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Aqa              The buffer used to store the content to be written into admin queue attributes register.

  @return EFI_SUCCESS      Successfully write data into the admin queue attributes register.
  @return EFI_DEVICE_ERROR Fail to write data into the admin queue attributes register.

**/
EFI_STATUS
WriteNvmeAdminQueueAttributes (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_AQA                      *Aqa
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT32               Data;

  PciIo  = Private->PciIo;
  Data   = ReadUnaligned32 ((UINT32 *)Aqa);
  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_AQA_OFFSET,
                        1,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Aqa.Asqs: %d\n", Aqa->Asqs));
  DEBUG ((DEBUG_INFO, "Aqa.Acqs: %d\n", Aqa->Acqs));

  return EFI_SUCCESS;
}

/**
  Write Nvm Express admin submission queue base address register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Asq              The buffer used to store the content to be written into admin submission queue base address register.

  @return EFI_SUCCESS      Successfully write data into the admin submission queue base address register.
  @return EFI_DEVICE_ERROR Fail to write data into the admin submission queue base address register.

**/
EFI_STATUS
WriteNvmeAdminSubmissionQueueBaseAddress (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_ASQ                      *Asq
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT64               Data;

  PciIo = Private->PciIo;
  Data  = ReadUnaligned64 ((UINT64 *)Asq);

  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_ASQ_OFFSET,
                        2,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Asq: %lx\n", *Asq));

  return EFI_SUCCESS;
}

/**
  Write Nvm Express admin completion queue base address register.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Acq              The buffer used to store the content to be written into admin completion queue base address register.

  @return EFI_SUCCESS      Successfully write data into the admin completion queue base address register.
  @return EFI_DEVICE_ERROR Fail to write data into the admin completion queue base address register.

**/
EFI_STATUS
WriteNvmeAdminCompletionQueueBaseAddress (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN NVME_ACQ                      *Acq
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT64               Data;

  PciIo = Private->PciIo;
  Data  = ReadUnaligned64 ((UINT64 *)Acq);

  Status = PciIo->Mem.Write (
                        PciIo,
                        EfiPciIoWidthUint32,
                        NVME_BAR,
                        NVME_ACQ_OFFSET,
                        2,
                        &Data
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Acq: %lxh\n", *Acq));

  return EFI_SUCCESS;
}

/**
  Disable the Nvm Express controller.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS      Successfully disable the controller.
  @return EFI_DEVICE_ERROR Fail to disable the controller.

**/
EFI_STATUS
NvmeDisableController (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  NVME_CC     Cc;
  NVME_CSTS   Csts;
  EFI_STATUS  Status;
  UINT32      Index;
  UINT8       Timeout;

  //
  // Read Controller Configuration Register.
  //
  Status = ReadNvmeControllerConfiguration (Private, &Cc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Cc.En = 0;

  //
  // Disable the controller.
  //
  Status = WriteNvmeControllerConfiguration (Private, &Cc);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Cap.To specifies max delay time in 500ms increments for Csts.Rdy to transition from 1 to 0 after
  // Cc.Enable transition from 1 to 0. Loop produces a 1 millisecond delay per itteration, up to 500 * Cap.To.
  //
  if (Private->Cap.To == 0) {
    Timeout = 1;
  } else {
    Timeout = Private->Cap.To;
  }

  for (Index = (Timeout * 500); Index != 0; --Index) {
    gBS->Stall (1000);

    //
    // Check if the controller is initialized
    //
    Status = ReadNvmeControllerStatus (Private, &Csts);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Csts.Rdy == 0) {
      break;
    }
  }

  if (Index == 0) {
    Status = EFI_DEVICE_ERROR;
    REPORT_STATUS_CODE (
      (EFI_ERROR_CODE | EFI_ERROR_MAJOR),
      (EFI_IO_BUS_SCSI | EFI_IOB_EC_INTERFACE_ERROR)
      );
  }

  DEBUG ((DEBUG_INFO, "NVMe controller is disabled with status [%r].\n", Status));
  return Status;
}

/**
  Enable the Nvm Express controller.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS      Successfully enable the controller.
  @return EFI_DEVICE_ERROR Fail to enable the controller.
  @return EFI_TIMEOUT      Fail to enable the controller in given time slot.

**/
EFI_STATUS
NvmeEnableController (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  NVME_CC     Cc;
  NVME_CSTS   Csts;
  EFI_STATUS  Status;
  UINT32      Index;
  UINT8       Timeout;

  EfiEventGroupSignal (&gNVMeEnableStartEventGroupGuid);

  //
  // Enable the controller.
  // CC.AMS, CC.MPS and CC.CSS are all set to 0.
  //
  ZeroMem (&Cc, sizeof (NVME_CC));
  Cc.En     = 1;
  Cc.Iosqes = 6;
  Cc.Iocqes = 4;

  Status = WriteNvmeControllerConfiguration (Private, &Cc);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  //
  // Cap.To specifies max delay time in 500ms increments for Csts.Rdy to set after
  // Cc.Enable. Loop produces a 1 millisecond delay per itteration, up to 500 * Cap.To.
  //
  if (Private->Cap.To == 0) {
    Timeout = 1;
  } else {
    Timeout = Private->Cap.To;
  }

  for (Index = (Timeout * 500); Index != 0; --Index) {
    gBS->Stall (1000);

    //
    // Check if the controller is initialized
    //
    Status = ReadNvmeControllerStatus (Private, &Csts);

    if (EFI_ERROR (Status)) {
      goto Cleanup;
    }

    if (Csts.Rdy) {
      break;
    }
  }

  if (Index == 0) {
    Status = EFI_TIMEOUT;
    REPORT_STATUS_CODE (
      (EFI_ERROR_CODE | EFI_ERROR_MAJOR),
      (EFI_IO_BUS_SCSI | EFI_IOB_EC_INTERFACE_ERROR)
      );
  }

  DEBUG ((DEBUG_INFO, "NVMe controller is enabled with status [%r].\n", Status));

Cleanup:
  EfiEventGroupSignal (&gNVMeEnableCompleteEventGroupGuid);
  return Status;
}

/**
  Get identify controller data.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  Buffer           The buffer used to store the identify controller data.

  @return EFI_SUCCESS      Successfully get the identify controller data.
  @return EFI_DEVICE_ERROR Fail to get the identify controller data.

**/
EFI_STATUS
NvmeIdentifyController (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN VOID                          *Buffer
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  //
  // According to Nvm Express 1.1 spec Figure 38, When not used, the field shall be cleared to 0h.
  // For the Identify command, the Namespace Identifier is only used for the Namespace data structure.
  //
  Command.Nsid = 0;

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.TransferBuffer = Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_CONTROLLER_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a controller
  //
  Command.Cdw10 = 1;
  Command.Flags = CDW10_VALID;

  Status = Private->Passthru.PassThru (
                               &Private->Passthru,
                               NVME_CONTROLLER_ID,
                               &CommandPacket,
                               NULL
                               );

  return Status;
}

/**
  Get specified identify namespace data.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param  NamespaceId      The specified namespace identifier.
  @param  Buffer           The buffer used to store the identify namespace data.

  @return EFI_SUCCESS      Successfully get the identify namespace data.
  @return EFI_DEVICE_ERROR Fail to get the identify namespace data.

**/
EFI_STATUS
NvmeIdentifyNamespace (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT32                        NamespaceId,
  IN VOID                          *Buffer
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;

  Command.Cdw0.Opcode          = NVME_ADMIN_IDENTIFY_CMD;
  Command.Nsid                 = NamespaceId;
  CommandPacket.TransferBuffer = Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_NAMESPACE_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a namespace
  //
  CommandPacket.NvmeCmd->Cdw10 = 0;
  CommandPacket.NvmeCmd->Flags = CDW10_VALID;

  Status = Private->Passthru.PassThru (
                               &Private->Passthru,
                               NamespaceId,
                               &CommandPacket,
                               NULL
                               );

  return Status;
}

/**
  Create io completion queue.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS      Successfully create io completion queue.
  @return EFI_DEVICE_ERROR Fail to create io completion queue.

**/
EFI_STATUS
NvmeCreateIoCompletionQueue (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;
  NVME_ADMIN_CRIOCQ                         CrIoCq;
  UINT32                                    Index;
  UINT16                                    QueueSize;

  Status                 = EFI_SUCCESS;
  Private->CreateIoQueue = TRUE;

  for (Index = 1; Index < NVME_MAX_QUEUES; Index++) {
    ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
    ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
    ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));
    ZeroMem (&CrIoCq, sizeof (NVME_ADMIN_CRIOCQ));

    CommandPacket.NvmeCmd        = &Command;
    CommandPacket.NvmeCompletion = &Completion;

    Command.Cdw0.Opcode          = NVME_ADMIN_CRIOCQ_CMD;
    CommandPacket.TransferBuffer = Private->CqBufferPciAddr[Index];
    CommandPacket.TransferLength = EFI_PAGE_SIZE;
    CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
    CommandPacket.QueueType      = NVME_ADMIN_QUEUE;

    if (Index == 1) {
      QueueSize = NVME_CCQ_SIZE;
    } else {
      if (Private->Cap.Mqes > NVME_ASYNC_CCQ_SIZE) {
        QueueSize = NVME_ASYNC_CCQ_SIZE;
      } else {
        QueueSize = Private->Cap.Mqes;
      }
    }

    CrIoCq.Qid   = Index;
    CrIoCq.Qsize = QueueSize;
    CrIoCq.Pc    = 1;
    CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoCq, sizeof (NVME_ADMIN_CRIOCQ));
    CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

    Status = Private->Passthru.PassThru (
                                 &Private->Passthru,
                                 0,
                                 &CommandPacket,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  Private->CreateIoQueue = FALSE;

  return Status;
}

/**
  Create io submission queue.

  @param  Private          The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @return EFI_SUCCESS      Successfully create io submission queue.
  @return EFI_DEVICE_ERROR Fail to create io submission queue.

**/
EFI_STATUS
NvmeCreateIoSubmissionQueue (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EFI_STATUS                                Status;
  NVME_ADMIN_CRIOSQ                         CrIoSq;
  UINT32                                    Index;
  UINT16                                    QueueSize;

  Status                 = EFI_SUCCESS;
  Private->CreateIoQueue = TRUE;

  for (Index = 1; Index < NVME_MAX_QUEUES; Index++) {
    ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
    ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
    ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));
    ZeroMem (&CrIoSq, sizeof (NVME_ADMIN_CRIOSQ));

    CommandPacket.NvmeCmd        = &Command;
    CommandPacket.NvmeCompletion = &Completion;

    Command.Cdw0.Opcode          = NVME_ADMIN_CRIOSQ_CMD;
    CommandPacket.TransferBuffer = Private->SqBufferPciAddr[Index];
    CommandPacket.TransferLength = EFI_PAGE_SIZE;
    CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
    CommandPacket.QueueType      = NVME_ADMIN_QUEUE;

    if (Index == 1) {
      QueueSize = NVME_CSQ_SIZE;
    } else {
      if (Private->Cap.Mqes > NVME_ASYNC_CSQ_SIZE) {
        QueueSize = NVME_ASYNC_CSQ_SIZE;
      } else {
        QueueSize = Private->Cap.Mqes;
      }
    }

    CrIoSq.Qid   = Index;
    CrIoSq.Qsize = QueueSize;
    CrIoSq.Pc    = 1;
    CrIoSq.Cqid  = Index;
    CrIoSq.Qprio = 0;
    CopyMem (&CommandPacket.NvmeCmd->Cdw10, &CrIoSq, sizeof (NVME_ADMIN_CRIOSQ));
    CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID;

    Status = Private->Passthru.PassThru (
                                 &Private->Passthru,
                                 0,
                                 &CommandPacket,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  Private->CreateIoQueue = FALSE;

  return Status;
}

/**
  Initialize the Nvm Express controller.

  @param[in] Private                 The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The NVM Express Controller is initialized successfully.
  @retval Others                     A device error occurred while initializing the controller.

**/
EFI_STATUS
NvmeControllerInit (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT64               Supports;
  NVME_AQA             Aqa;
  NVME_ASQ             Asq;
  NVME_ACQ             Acq;
  UINT8                Sn[21];
  UINT8                Mn[41];

  //
  // Enable this controller.
  //
  PciIo  = Private->PciIo;
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );

  if (!EFI_ERROR (Status)) {
    Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status    = PciIo->Attributes (
                         PciIo,
                         EfiPciIoAttributeOperationEnable,
                         Supports,
                         NULL
                         );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NvmeControllerInit: failed to enable controller\n"));
    return Status;
  }

  //
  // Read the Controller Capabilities register and verify that the NVM command set is supported
  //
  Status = ReadNvmeControllerCapabilities (Private, &Private->Cap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Private->Cap.Css & BIT0) == 0) {
    DEBUG ((DEBUG_INFO, "NvmeControllerInit: the controller doesn't support NVMe command set\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Currently the driver only supports 4k page size.
  //
  ASSERT ((Private->Cap.Mpsmin + 12) <= EFI_PAGE_SHIFT);

  Private->Cid[0]        = 0;
  Private->Cid[1]        = 0;
  Private->Cid[2]        = 0;
  Private->Pt[0]         = 0;
  Private->Pt[1]         = 0;
  Private->Pt[2]         = 0;
  Private->SqTdbl[0].Sqt = 0;
  Private->SqTdbl[1].Sqt = 0;
  Private->SqTdbl[2].Sqt = 0;
  Private->CqHdbl[0].Cqh = 0;
  Private->CqHdbl[1].Cqh = 0;
  Private->CqHdbl[2].Cqh = 0;
  Private->AsyncSqHead   = 0;

  Status = NvmeDisableController (Private);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // set number of entries admin submission & completion queues.
  //
  Aqa.Asqs  = NVME_ASQ_SIZE;
  Aqa.Rsvd1 = 0;
  Aqa.Acqs  = NVME_ACQ_SIZE;
  Aqa.Rsvd2 = 0;

  //
  // Address of admin submission queue.
  //
  Asq = (UINT64)(UINTN)(Private->BufferPciAddr) & ~0xFFF;

  //
  // Address of admin completion queue.
  //
  Acq = (UINT64)(UINTN)(Private->BufferPciAddr + EFI_PAGE_SIZE) & ~0xFFF;

  //
  // Address of I/O submission & completion queue.
  //
  ZeroMem (Private->Buffer, EFI_PAGES_TO_SIZE (6));
  Private->SqBuffer[0]        = (NVME_SQ *)(UINTN)(Private->Buffer);
  Private->SqBufferPciAddr[0] = (NVME_SQ *)(UINTN)(Private->BufferPciAddr);
  Private->CqBuffer[0]        = (NVME_CQ *)(UINTN)(Private->Buffer + 1 * EFI_PAGE_SIZE);
  Private->CqBufferPciAddr[0] = (NVME_CQ *)(UINTN)(Private->BufferPciAddr + 1 * EFI_PAGE_SIZE);
  Private->SqBuffer[1]        = (NVME_SQ *)(UINTN)(Private->Buffer + 2 * EFI_PAGE_SIZE);
  Private->SqBufferPciAddr[1] = (NVME_SQ *)(UINTN)(Private->BufferPciAddr + 2 * EFI_PAGE_SIZE);
  Private->CqBuffer[1]        = (NVME_CQ *)(UINTN)(Private->Buffer + 3 * EFI_PAGE_SIZE);
  Private->CqBufferPciAddr[1] = (NVME_CQ *)(UINTN)(Private->BufferPciAddr + 3 * EFI_PAGE_SIZE);
  Private->SqBuffer[2]        = (NVME_SQ *)(UINTN)(Private->Buffer + 4 * EFI_PAGE_SIZE);
  Private->SqBufferPciAddr[2] = (NVME_SQ *)(UINTN)(Private->BufferPciAddr + 4 * EFI_PAGE_SIZE);
  Private->CqBuffer[2]        = (NVME_CQ *)(UINTN)(Private->Buffer + 5 * EFI_PAGE_SIZE);
  Private->CqBufferPciAddr[2] = (NVME_CQ *)(UINTN)(Private->BufferPciAddr + 5 * EFI_PAGE_SIZE);

  DEBUG ((DEBUG_INFO, "Private->Buffer = [%016X]\n", (UINT64)(UINTN)Private->Buffer));
  DEBUG ((DEBUG_INFO, "Admin     Submission Queue size (Aqa.Asqs) = [%08X]\n", Aqa.Asqs));
  DEBUG ((DEBUG_INFO, "Admin     Completion Queue size (Aqa.Acqs) = [%08X]\n", Aqa.Acqs));
  DEBUG ((DEBUG_INFO, "Admin     Submission Queue (SqBuffer[0]) = [%016X]\n", Private->SqBuffer[0]));
  DEBUG ((DEBUG_INFO, "Admin     Completion Queue (CqBuffer[0]) = [%016X]\n", Private->CqBuffer[0]));
  DEBUG ((DEBUG_INFO, "Sync  I/O Submission Queue (SqBuffer[1]) = [%016X]\n", Private->SqBuffer[1]));
  DEBUG ((DEBUG_INFO, "Sync  I/O Completion Queue (CqBuffer[1]) = [%016X]\n", Private->CqBuffer[1]));
  DEBUG ((DEBUG_INFO, "Async I/O Submission Queue (SqBuffer[2]) = [%016X]\n", Private->SqBuffer[2]));
  DEBUG ((DEBUG_INFO, "Async I/O Completion Queue (CqBuffer[2]) = [%016X]\n", Private->CqBuffer[2]));

  //
  // Program admin queue attributes.
  //
  Status = WriteNvmeAdminQueueAttributes (Private, &Aqa);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Program admin submission queue address.
  //
  Status = WriteNvmeAdminSubmissionQueueBaseAddress (Private, &Asq);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Program admin completion queue address.
  //
  Status = WriteNvmeAdminCompletionQueueBaseAddress (Private, &Acq);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = NvmeEnableController (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate buffer for Identify Controller data
  //
  if (Private->ControllerData == NULL) {
    Private->ControllerData = (NVME_ADMIN_CONTROLLER_DATA *)AllocateZeroPool (sizeof (NVME_ADMIN_CONTROLLER_DATA));

    if (Private->ControllerData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Get current Identify Controller Data
  //
  Status = NvmeIdentifyController (Private, Private->ControllerData);

  if (EFI_ERROR (Status)) {
    FreePool (Private->ControllerData);
    Private->ControllerData = NULL;
    return EFI_NOT_FOUND;
  }

  //
  // Dump NvmExpress Identify Controller Data
  //
  CopyMem (Sn, Private->ControllerData->Sn, sizeof (Private->ControllerData->Sn));
  Sn[20] = 0;
  CopyMem (Mn, Private->ControllerData->Mn, sizeof (Private->ControllerData->Mn));
  Mn[40] = 0;
  DEBUG ((DEBUG_INFO, " == NVME IDENTIFY CONTROLLER DATA ==\n"));
  DEBUG ((DEBUG_INFO, "    PCI VID   : 0x%x\n", Private->ControllerData->Vid));
  DEBUG ((DEBUG_INFO, "    PCI SSVID : 0x%x\n", Private->ControllerData->Ssvid));
  DEBUG ((DEBUG_INFO, "    SN        : %a\n", Sn));
  DEBUG ((DEBUG_INFO, "    MN        : %a\n", Mn));
  DEBUG ((DEBUG_INFO, "    FR        : 0x%x\n", *((UINT64 *)Private->ControllerData->Fr)));
  DEBUG ((DEBUG_INFO, "    TNVMCAP (high 8-byte) : 0x%lx\n", *((UINT64 *)(Private->ControllerData->Tnvmcap + 8))));
  DEBUG ((DEBUG_INFO, "    TNVMCAP (low 8-byte)  : 0x%lx\n", *((UINT64 *)Private->ControllerData->Tnvmcap)));
  DEBUG ((DEBUG_INFO, "    RAB       : 0x%x\n", Private->ControllerData->Rab));
  DEBUG ((DEBUG_INFO, "    IEEE      : 0x%x\n", *(UINT32 *)Private->ControllerData->Ieee_oui));
  DEBUG ((DEBUG_INFO, "    AERL      : 0x%x\n", Private->ControllerData->Aerl));
  DEBUG ((DEBUG_INFO, "    SQES      : 0x%x\n", Private->ControllerData->Sqes));
  DEBUG ((DEBUG_INFO, "    CQES      : 0x%x\n", Private->ControllerData->Cqes));
  DEBUG ((DEBUG_INFO, "    NN        : 0x%x\n", Private->ControllerData->Nn));

  //
  // Create two I/O completion queues.
  // One for blocking I/O, one for non-blocking I/O.
  //
  Status = NvmeCreateIoCompletionQueue (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create two I/O Submission queues.
  // One for blocking I/O, one for non-blocking I/O.
  //
  Status = NvmeCreateIoSubmissionQueue (Private);

  return Status;
}

/**
 This routine is called to properly shutdown the Nvm Express controller per NVMe spec.

  @param[in]  ResetType         The type of reset to perform.
  @param[in]  ResetStatus       The status code for the reset.
  @param[in]  DataSize          The size, in bytes, of ResetData.
  @param[in]  ResetData         For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.
                                The string is a description that the caller may use to further
                                indicate the reason for the system reset.
                                For a ResetType of EfiResetPlatformSpecific the data buffer
                                also starts with a Null-terminated string that is followed
                                by an EFI_GUID that describes the specific type of reset to perform.
**/
VOID
EFIAPI
NvmeShutdownAllControllers (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  EFI_STATUS                           Status;
  EFI_HANDLE                           *Handles;
  UINTN                                HandleCount;
  UINTN                                HandleIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfos;
  UINTN                                OpenInfoCount;
  UINTN                                OpenInfoIndex;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL   *NvmePassThru;
  NVME_CC                              Cc;
  NVME_CSTS                            Csts;
  UINTN                                Index;
  NVME_CONTROLLER_PRIVATE_DATA         *Private;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    HandleCount = 0;
  }

  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    Status = gBS->OpenProtocolInformation (
                    Handles[HandleIndex],
                    &gEfiPciIoProtocolGuid,
                    &OpenInfos,
                    &OpenInfoCount
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
      //
      // Find all the NVME controller managed by this driver.
      // gImageHandle equals to DriverBinding handle for this driver.
      //
      if (((OpenInfos[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) &&
          (OpenInfos[OpenInfoIndex].AgentHandle == gImageHandle))
      {
        Status = gBS->OpenProtocol (
                        OpenInfos[OpenInfoIndex].ControllerHandle,
                        &gEfiNvmExpressPassThruProtocolGuid,
                        (VOID **)&NvmePassThru,
                        NULL,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }

        Private = NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU (NvmePassThru);

        //
        // Read Controller Configuration Register.
        //
        Status = ReadNvmeControllerConfiguration (Private, &Cc);
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // The host should set the Shutdown Notification (CC.SHN) field to 01b
        // to indicate a normal shutdown operation.
        //
        Cc.Shn = NVME_CC_SHN_NORMAL_SHUTDOWN;
        Status = WriteNvmeControllerConfiguration (Private, &Cc);
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // The controller indicates when shutdown processing is completed by updating the
        // Shutdown Status (CSTS.SHST) field to 10b.
        // Wait up to 45 seconds (break down to 4500 x 10ms) for the shutdown to complete.
        //
        for (Index = 0; Index < NVME_SHUTDOWN_PROCESS_TIMEOUT * 100; Index++) {
          Status = ReadNvmeControllerStatus (Private, &Csts);
          if (!EFI_ERROR (Status) && (Csts.Shst == NVME_CSTS_SHST_SHUTDOWN_COMPLETED)) {
            DEBUG ((DEBUG_INFO, "NvmeShutdownController: shutdown processing is completed after %dms.\n", Index * 10));
            break;
          }

          //
          // Stall for 10ms
          //
          gBS->Stall (10 * 1000);
        }

        if (Index == NVME_SHUTDOWN_PROCESS_TIMEOUT * 100) {
          DEBUG ((DEBUG_ERROR, "NvmeShutdownController: shutdown processing is timed out\n"));
        }
      }
    }
  }
}

/**
  Register the shutdown notification through the ResetNotification protocol.

  Register the shutdown notification when mNvmeControllerNumber increased from 0 to 1.
**/
VOID
NvmeRegisterShutdownNotification (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotify;

  mNvmeControllerNumber++;
  if (mNvmeControllerNumber == 1) {
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid, NULL, (VOID **)&ResetNotify);
    if (!EFI_ERROR (Status)) {
      Status = ResetNotify->RegisterResetNotify (ResetNotify, NvmeShutdownAllControllers);
      ASSERT_EFI_ERROR (Status);
    } else {
      DEBUG ((DEBUG_WARN, "NVME: ResetNotification absent! Shutdown notification cannot be performed!\n"));
    }
  }
}

/**
  Unregister the shutdown notification through the ResetNotification protocol.

  Unregister the shutdown notification when mNvmeControllerNumber decreased from 1 to 0.
**/
VOID
NvmeUnregisterShutdownNotification (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotify;

  mNvmeControllerNumber--;
  if (mNvmeControllerNumber == 0) {
    Status = gBS->LocateProtocol (&gEfiResetNotificationProtocolGuid, NULL, (VOID **)&ResetNotify);
    if (!EFI_ERROR (Status)) {
      Status = ResetNotify->UnregisterResetNotify (ResetNotify, NvmeShutdownAllControllers);
      ASSERT_EFI_ERROR (Status);
    }
  }
}

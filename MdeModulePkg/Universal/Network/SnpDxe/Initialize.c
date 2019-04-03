/** @file
     Implementation of initializing a network adapter.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "Snp.h"

/**
  Call UNDI to initialize the interface.

  @param  Snp                   Pointer to snp driver structure.
  @param  CableDetectFlag       Do/don't detect the cable (depending on what
                                undi supports).

  @retval EFI_SUCCESS           UNDI is initialized successfully.
  @retval EFI_DEVICE_ERROR      UNDI could not be initialized.
  @retval Other                 Other errors as indicated.

**/
EFI_STATUS
PxeInit (
  SNP_DRIVER *Snp,
  UINT16     CableDetectFlag
  )
{
  PXE_CPB_INITIALIZE  *Cpb;
  VOID                *Addr;
  EFI_STATUS          Status;

  Status = EFI_SUCCESS;

  Cpb = Snp->Cpb;
  if (Snp->TxRxBufferSize != 0) {
    Status = Snp->PciIo->AllocateBuffer (
                           Snp->PciIo,
                           AllocateAnyPages,
                           EfiBootServicesData,
                           SNP_MEM_PAGES (Snp->TxRxBufferSize),
                           &Addr,
                           0
                           );

    if (Status != EFI_SUCCESS) {
      DEBUG (
        (EFI_D_ERROR,
        "\nSnp->PxeInit()  AllocateBuffer  %xh (%r)\n",
        Status,
        Status)
        );

      return Status;
    }

    ASSERT (Addr);

    Snp->TxRxBuffer = Addr;
  }

  Cpb->MemoryAddr   = (UINT64)(UINTN) Snp->TxRxBuffer;

  Cpb->MemoryLength = Snp->TxRxBufferSize;

  //
  // let UNDI decide/detect these values
  //
  Cpb->LinkSpeed      = 0;
  Cpb->TxBufCnt       = 0;
  Cpb->TxBufSize      = 0;
  Cpb->RxBufCnt       = 0;
  Cpb->RxBufSize      = 0;

  Cpb->DuplexMode         = PXE_DUPLEX_DEFAULT;

  Cpb->LoopBackMode       = LOOPBACK_NORMAL;

  Snp->Cdb.OpCode     = PXE_OPCODE_INITIALIZE;
  Snp->Cdb.OpFlags    = CableDetectFlag;

  Snp->Cdb.CPBsize    = (UINT16) sizeof (PXE_CPB_INITIALIZE);
  Snp->Cdb.DBsize     = (UINT16) sizeof (PXE_DB_INITIALIZE);

  Snp->Cdb.CPBaddr    = (UINT64)(UINTN) Snp->Cpb;
  Snp->Cdb.DBaddr     = (UINT64)(UINTN) Snp->Db;

  Snp->Cdb.StatCode   = PXE_STATCODE_INITIALIZE;
  Snp->Cdb.StatFlags  = PXE_STATFLAGS_INITIALIZE;
  Snp->Cdb.IFnum      = Snp->IfNum;
  Snp->Cdb.Control    = PXE_CONTROL_LAST_CDB_IN_LIST;

  DEBUG ((EFI_D_NET, "\nSnp->undi.initialize()  "));

  (*Snp->IssueUndi32Command) ((UINT64)(UINTN) &Snp->Cdb);

  //
  // There are two fields need to be checked here:
  // First is the upper two bits (14 & 15) in the CDB.StatFlags field. Until these bits change to report
  // PXE_STATFLAGS_COMMAND_COMPLETE or PXE_STATFLAGS_COMMAND_FAILED, the command has not been executed by the UNDI.
  // Second is the CDB.StatCode field. After command execution completes, either successfully or not,
  // the CDB.StatCode field contains the result of the command execution.
  //
  if ((((Snp->Cdb.StatFlags) & PXE_STATFLAGS_STATUS_MASK) == PXE_STATFLAGS_COMMAND_COMPLETE) &&
      (Snp->Cdb.StatCode == PXE_STATCODE_SUCCESS)) {
    //
    // If cable detect feature is enabled in CDB.OpFlags, check the CDB.StatFlags to see if there is an
    // active connection to this network device. If the no media StatFlag is set, the UNDI and network
    // device are still initialized.
    //
    if (CableDetectFlag == PXE_OPFLAGS_INITIALIZE_DETECT_CABLE) {
      if(((Snp->Cdb.StatFlags) & PXE_STATFLAGS_INITIALIZED_NO_MEDIA) != PXE_STATFLAGS_INITIALIZED_NO_MEDIA) {
        Snp->Mode.MediaPresent = TRUE;
      } else {
        Snp->Mode.MediaPresent = FALSE;
      }
    }

    Snp->Mode.State   = EfiSimpleNetworkInitialized;
    Status            = EFI_SUCCESS;
  } else {
    DEBUG (
      (EFI_D_WARN,
      "\nSnp->undi.initialize()  %xh:%xh\n",
      Snp->Cdb.StatFlags,
      Snp->Cdb.StatCode)
      );

    if (Snp->TxRxBuffer != NULL) {
      Snp->PciIo->FreeBuffer (
                    Snp->PciIo,
                    SNP_MEM_PAGES (Snp->TxRxBufferSize),
                    (VOID *) Snp->TxRxBuffer
                    );
    }

    Snp->TxRxBuffer = NULL;

    Status          = EFI_DEVICE_ERROR;
  }

  return Status;
}


/**
  Resets a network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation of
  additional transmit and receive buffers.

  This function allocates the transmit and receive buffers required by the network
  interface. If this allocation fails, then EFI_OUT_OF_RESOURCES is returned.
  If the allocation succeeds and the network interface is successfully initialized,
  then EFI_SUCCESS will be returned.

  @param This               A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.

  @param ExtraRxBufferSize  The size, in bytes, of the extra receive buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the
                            extra buffer, and the caller will not know if it is
                            actually being used.
  @param ExtraTxBufferSize  The size, in bytes, of the extra transmit buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the
                            extra buffer, and the caller will not know if it is
                            actually being used.

  @retval EFI_SUCCESS           The network interface was initialized.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory for the transmit and
                                receive buffers.
  @retval EFI_INVALID_PARAMETER This parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SnpUndi32Initialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       ExtraRxBufferSize OPTIONAL,
  IN UINTN                       ExtraTxBufferSize OPTIONAL
  )
{
  EFI_STATUS  EfiStatus;
  SNP_DRIVER  *Snp;
  EFI_TPL     OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Snp = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Snp == NULL) {
    EfiStatus = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  switch (Snp->Mode.State) {
  case EfiSimpleNetworkStarted:
    break;

  case EfiSimpleNetworkStopped:
    EfiStatus = EFI_NOT_STARTED;
    goto ON_EXIT;

  default:
    EfiStatus = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  EfiStatus = gBS->CreateEvent (
                    EVT_NOTIFY_WAIT,
                    TPL_NOTIFY,
                    &SnpWaitForPacketNotify,
                    Snp,
                    &Snp->Snp.WaitForPacket
                    );

  if (EFI_ERROR (EfiStatus)) {
    Snp->Snp.WaitForPacket = NULL;
    EfiStatus = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }
  //
  //
  //
  Snp->Mode.MCastFilterCount      = 0;
  Snp->Mode.ReceiveFilterSetting  = 0;
  ZeroMem (Snp->Mode.MCastFilter, sizeof Snp->Mode.MCastFilter);
  CopyMem (
    &Snp->Mode.CurrentAddress,
    &Snp->Mode.PermanentAddress,
    sizeof (EFI_MAC_ADDRESS)
    );

  //
  // Compute tx/rx buffer sizes based on UNDI init info and parameters.
  //
  Snp->TxRxBufferSize = (UINT32) (Snp->InitInfo.MemoryRequired + ExtraRxBufferSize + ExtraTxBufferSize);

  //
  // If UNDI support cable detect for INITIALIZE command, try it first.
  //
  if (Snp->CableDetectSupported) {
    if (PxeInit (Snp, PXE_OPFLAGS_INITIALIZE_DETECT_CABLE) == EFI_SUCCESS) {
      goto ON_EXIT;
    }
  }

  Snp->Mode.MediaPresent  = FALSE;

  EfiStatus               = PxeInit (Snp, PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE);

  if (EFI_ERROR (EfiStatus)) {
    gBS->CloseEvent (Snp->Snp.WaitForPacket);
    goto ON_EXIT;
  }

  //
  // Try to update the MediaPresent field of EFI_SIMPLE_NETWORK_MODE if the UNDI support it.
  //
  if (Snp->MediaStatusSupported) {
    PxeGetStatus (Snp, NULL, FALSE);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return EfiStatus;
}

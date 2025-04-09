/** @file
  Entry point to the Standalone MM Foundation on RISCV platforms

Copyright (c) 2025, Rivos Inc<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Protocol/PiMmCpuDriverEp.h>
#include <StandaloneMmCoreEntryPoint.h>

#include <PiPei.h>
#include <Guid/MmramMemoryReserve.h>
#include <Guid/MpInformation.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>

#include <Library/BaseRiscVSbiLib.h>
#include <Library/DxeRiscvMpxy.h>

extern EFI_MM_SYSTEM_TABLE  gMmCoreMmst;

/** RPMI Messages Types */
enum rpmi_message_type {
  /* Normal request backed with ack */
  RPMI_MSG_NORMAL_REQUEST = 0x0,
  /* Request without any ack */
  RPMI_MSG_POSTED_REQUEST = 0x1,
  /* Acknowledgment for normal request message */
  RPMI_MSG_ACKNOWLDGEMENT = 0x2,
  /* Notification message */
  RPMI_MSG_NOTIFICATION = 0x3,
};

/*
 * RPMI SERVICEGROUPS AND SERVICES
 */

/** RPMI ServiceGroups IDs */
enum rpmi_servicegroup_id {
  RPMI_SRVGRP_ID_MIN          = 0,
  RPMI_SRVGRP_BASE            = 0x00001,
  RPMI_SRVGRP_SYSTEM_RESET    = 0x00002,
  RPMI_SRVGRP_SYSTEM_SUSPEND  = 0x00003,
  RPMI_SRVGRP_HSM             = 0x00004,
  RPMI_SRVGRP_CPPC            = 0x00005,
  RPMI_SRVGRP_CLOCK           = 0x00007,
  RPMI_SRVGRP_REQUEST_FORWARD = 0x0000D,
  RPMI_SRVGRP_ID_MAX_COUNT,
};

/** RPMI Error Types */
enum rpmi_error {
  RPMI_SUCCESS        = 0,
  RPMI_ERR_FAILED     = -1,
  RPMI_ERR_NOTSUPP    = -2,
  RPMI_ERR_INVAL      = -3,
  RPMI_ERR_DENIED     = -4,
  RPMI_ERR_NOTFOUND   = -5,
  RPMI_ERR_OUTOFRANGE = -6,
  RPMI_ERR_OUTOFRES   = -7,
  RPMI_ERR_HWFAULT    = -8,
};

/** RPMI ReqFwd ServiceGroup Service IDs */
enum rpmi_reqfwd_service_id {
  RPMI_REQFWD_ENABLE_NOTIFICATION      = 1,
  RPMI_REQFWD_RETRIEVE_CURRENT_MESSAGE = 2,
  RPMI_REQFWD_COMPLETE_CURRENT_MESSAGE = 3,
};

/**
  Retrieve and print boot information provided by privileged firmware.

  This function extracts and logs the RISC-V SMM boot info structure passed by
  the secure firmware. It prints memory layout, communication buffer, and CPU topology.

  @param[in] PayloadInfoAddress  Address of the EFI_RISCV_SMM_PAYLOAD_INFO structure.

  @retval EFI_RISCV_SMM_PAYLOAD_INFO*   Pointer to parsed boot information if valid.
  @retval NULL                          If the input is NULL.
**/
EFI_RISCV_SMM_PAYLOAD_INFO *
GetAndPrintBootinformation (
  IN VOID  *PayloadInfoAddress
  )
{
  EFI_RISCV_SMM_PAYLOAD_INFO  *BootInfo;
  EFI_RISCV_SMM_CPU_INFO      *CpuInfo;
  UINTN                       Index;

  BootInfo = (EFI_RISCV_SMM_PAYLOAD_INFO *)PayloadInfoAddress;

  if (BootInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "GetAndPrintBootinformation: PayloadInfoAddress is NULL\n"));
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "=== SMM Boot Info ===\n"));
  DEBUG ((DEBUG_INFO, "  NumMmMemRegions : 0x%x\n", BootInfo->NumMmMemRegions));
  DEBUG ((DEBUG_INFO, "  MmMemBase       : 0x%lx\n", BootInfo->MmMemBase));
  DEBUG ((DEBUG_INFO, "  MmMemLimit      : 0x%lx\n", BootInfo->MmMemLimit));
  DEBUG ((DEBUG_INFO, "  MmImageBase     : 0x%lx\n", BootInfo->MmImageBase));
  DEBUG ((DEBUG_INFO, "  MmStackBase     : 0x%lx\n", BootInfo->MmStackBase));
  DEBUG ((DEBUG_INFO, "  MmHeapBase      : 0x%lx\n", BootInfo->MmHeapBase));
  DEBUG ((DEBUG_INFO, "  MmNsCommBufBase : 0x%lx\n", BootInfo->MmNsCommBufBase));
  DEBUG ((DEBUG_INFO, "  MmImageSize     : 0x%x\n", BootInfo->MmImageSize));
  DEBUG ((DEBUG_INFO, "  MmPcpuStackSize : 0x%x\n", BootInfo->MmPcpuStackSize));
  DEBUG ((DEBUG_INFO, "  MmHeapSize      : 0x%x\n", BootInfo->MmHeapSize));
  DEBUG ((DEBUG_INFO, "  MmNsCommBufSize : 0x%x\n", BootInfo->MmNsCommBufSize));
  DEBUG ((DEBUG_INFO, "  NumCpus         : 0x%x\n", BootInfo->NumCpus));

  CpuInfo = (EFI_RISCV_SMM_CPU_INFO *)&BootInfo->CpuInfo;

  for (Index = 0; Index < BootInfo->NumCpus; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "  CPU[%u] ProcessorId: 0x%lx, Package: %u, Core: %u\n",
      Index,
      CpuInfo[Index].ProcessorId,
      CpuInfo[Index].Package,
      CpuInfo[Index].Core
      ));
  }

  return BootInfo;
}

/**
  Prepare an RPMI message header for a forwarded MM request.

  This routine fills out the RPMI header with the correct service group
  and message ID, using the standard flags for a normal request. In future
  this can be extended by passing required flags etc.

  @param[out] Hdr  Pointer to the RPMI message header structure to initialize.
  @param[in]  Msg  Message ID to assign to the header.
**/
STATIC
VOID
PrepareRpmiHeader (
  OUT RPMI_MESSAGE_HEADER  *Hdr,
  IN  UINT8                ServiceId,
  IN  UINT16               DataLen
  )
{
  Hdr->Flags          = RPMI_MSG_NORMAL_REQUEST;
  Hdr->ServiceGroupId = RPMI_SRVGRP_REQUEST_FORWARD;
  Hdr->ServiceId      = ServiceId;
  Hdr->DataLen        = DataLen;
}

/**
  Send a completion message to the host after MM handling completes.

  This function uses MPXY to send a completion response for the last forwarded
  message, indicating success or failure back to the host firmware.

  @param[in] ChannelId           MPXY communication channel.
  @param[in] EventCompleteArgs   Pointer to the completion response structure.

  @retval EFI_SUCCESS            Completion message successfully sent.
  @retval EFI_ACCESS_DENIED      Communication failed.
**/
EFI_STATUS
EFIAPI
SendMMComplete (
  IN UINTN                  ChannelId,
  IN RPMI_SMM_MSG_CMPL_CMD  *EventCompleteArgs
  )
{
  EFI_STATUS              Status;
  UINTN                   SmmMsgLen  = sizeof (RPMI_SMM_MSG_CMPL_CMD);
  UINTN                   SmmRespLen = sizeof (RPMI_SMM_MSG_CMPL_RESP);
  RPMI_SMM_MSG_CMPL_RESP  EventCompleteResp;

  PrepareRpmiHeader (&EventCompleteArgs->hdr, RPMI_REQFWD_COMPLETE_CURRENT_MESSAGE, sizeof (RISCV_SMM_MSG_COMM_RESP));
  EventCompleteArgs->mm_resp.Status = RPMI_SUCCESS;

  Status = SbiMpxySendMessage (
             ChannelId,
             RPMI_REQFWD_COMPLETE_CURRENT_MESSAGE,
             EventCompleteArgs,
             SmmMsgLen,
             &EventCompleteResp,
             &SmmRespLen
             );

  // TODO: Add size verification for the response.
  if (EFI_ERROR (Status) || (EventCompleteResp.Status == RPMI_SUCCESS)) {
    DEBUG ((DEBUG_ERROR, "SendMMComplete: MPXY communication failed - %r\n", Status));
    Status = EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Retrieve a forwarded RAS message from the host over MPXY.

  This function sends a retrieve request and waits for a valid forwarded message
  to be placed in the shared buffer. It also ensures the message is structurally
  valid before allowing it to proceed.

  @param[in]  ChannelId     MPXY communication channel.
  @param[out] pReqFwdResp   Pointer to buffer where the forwarded response is stored.

  @retval EFI_SUCCESS           Message successfully retrieved and verified.
  @retval EFI_ACCESS_DENIED     MPXY communication failed.
  @retval EFI_COMPROMISED_DATA  Message contents failed verification.
**/
EFI_STATUS
EFIAPI
RetrieveReqFwdMessage (
  IN UINTN                    ChannelId,
  OUT RPMI_SMM_MSG_COMM_ARGS  *pReqFwdResp
  )
{
  EFI_STATUS           Status;
  UINTN                SmmMsgLen, SmmRespLen;
  REQFWD_RETRIEVE_CMD  ReqFwdCmd;

  if (pReqFwdResp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrepareRpmiHeader (&ReqFwdCmd.hdr, RPMI_REQFWD_RETRIEVE_CURRENT_MESSAGE, sizeof (UINT32));
  SmmMsgLen  = sizeof (REQFWD_RETRIEVE_CMD);
  SmmRespLen = sizeof (RPMI_SMM_MSG_COMM_ARGS);

  Status = SbiMpxySendMessage (
             ChannelId,
             RPMI_REQFWD_RETRIEVE_CURRENT_MESSAGE,
             &ReqFwdCmd,
             SmmMsgLen,
             pReqFwdResp,
             &SmmRespLen
             );
  if (EFI_ERROR (Status) || (SmmRespLen == 0)) {
    DEBUG ((DEBUG_ERROR, "RetrieveReqFwdMessage: MPXY communication failed or empty response\n"));
    return EFI_NO_RESPONSE;
  }

  // TODO: Verify structure/content/response size etc to make it more robust
  if (pReqFwdResp->rpmi_resp.hdr.ServiceGroupId != RPMI_SRVGRP_REQUEST_FORWARD) {
    DEBUG ((
      DEBUG_ERROR,
      "RetrieveReqFwdMessage: Unexpected ServiceGroupId = 0x%x\n",
      pReqFwdResp->rpmi_resp.hdr.ServiceGroupId
      ));
    return EFI_NO_RESPONSE;
  }

  return EFI_SUCCESS;
}

/**
  Delegated MM event loop for processing forwarded messages.

  @param[in] CpuId                Logical CPU ID handling the request.
  @param[in] ChannelId            MPXY communication channel.
  @param[in] CompletionMessage    Pointer to completion message structure.
  @param[in] CpuDriverEntryPoint  Function pointer to invoke MM service handler.

  TODO: Pass proper HartID
**/
STATIC
VOID
DelegatedEventLoop (
  IN UINTN                              CpuId,
  IN UINTN                              ChannelId,
  IN RPMI_SMM_MSG_CMPL_CMD              *CompletionMessage,
  IN EDKII_PI_MM_CPU_DRIVER_ENTRYPOINT  CpuDriverEntryPoint
  )
{
  EFI_STATUS              Status;
  RPMI_SMM_MSG_COMM_ARGS  ForwardedMsg;

  Status = SendMMComplete (ChannelId, CompletionMessage);
  while (TRUE) {
    Status = RetrieveReqFwdMessage (ChannelId, &ForwardedMsg);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Req Fwd Command Failed"));
    }

    Status                            = CpuDriverEntryPoint (0, CpuId, 0);
    CompletionMessage->mm_resp.Status = EFI_ERROR (Status) ? RPMI_ERR_FAILED : RPMI_SUCCESS;

    Status = SendMMComplete (ChannelId, CompletionMessage);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MM Complete Command Failed"));
    }
  }
}

/**
  Main entry point to the Standalone MM Foundation on RISC-V platforms.

  @param[in] CpuId                Logical CPU ID.
  @param[in] PayloadInfoAddress  Address of payload boot information passed by firmware.
**/
VOID
EFIAPI
CModuleEntryPoint (
  IN UINT64  CpuId,
  IN VOID    *PayloadInfoAddress
  )
{
  EFI_RISCV_SMM_PAYLOAD_INFO          *BootInfo;
  VOID                                *HobStart;
  EFI_STATUS                          Status;
  RPMI_SMM_MSG_CMPL_CMD               *CompletionMessage;
  EDKII_PI_MM_CPU_DRIVER_ENTRYPOINT   CpuDriverEntryPoint;
  EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL  *PiMmCpuDriverEpProtocol;

  BootInfo = GetAndPrintBootinformation (PayloadInfoAddress);
  if (BootInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "CModuleEntryPoint: Invalid boot info\n"));
    goto Finish;
  }

  CpuDriverEntryPoint = NULL;

  //
  // Create Hoblist based upon boot information passed by privileged software
  //
  HobStart = CreateHobListFromBootInfo (BootInfo);

  //
  // Call the MM Core entry point
  //
  ProcessModuleEntryPointList (HobStart);

  //
  // Find out cpu driver entry point used in DelegatedEventLoop
  // to handle MMI request.
  //
  Status = gMmCoreMmst.MmLocateProtocol (
                         &gEdkiiPiMmCpuDriverEpProtocolGuid,
                         NULL,
                         (VOID **)&PiMmCpuDriverEpProtocol
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Could not locate CPU MM handler: Runtime MM service will fail %p\n"
      ));
    // If we can not locate driver, we should still resume other flow.
    goto Finish;
  }

  CpuDriverEntryPoint = PiMmCpuDriverEpProtocol->PiMmCpuDriverEntryPoint;

  DEBUG ((DEBUG_INFO, "Shared Cpu Driver EP %p\n", CpuDriverEntryPoint));

  CompletionMessage = AllocateZeroPool (sizeof (RPMI_SMM_MSG_CMPL_CMD));
  ASSERT (CompletionMessage != NULL);

  // Ensure MPXY has been initialized for future communications
  if (EFI_ERROR (SbiMpxyInit ())) {
    CompletionMessage->mm_resp.Status = RPMI_ERR_FAILED;
  } else {
    CompletionMessage->mm_resp.Status = RPMI_SUCCESS;
  }

Finish:
  DelegatedEventLoop ((UINTN)CpuId, BootInfo->MpxyChannelId, CompletionMessage, PiMmCpuDriverEpProtocol->PiMmCpuDriverEntryPoint);
  ASSERT (FALSE);
}

/** @file
  This module provides communication with RAS Agent over RPMI/MPXY

  Copyright (c) 2024, Ventana Micro Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>

#include <IndustryStandard/Acpi.h>

#include <Protocol/AcpiTable.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/MmCommunication2.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/BaseRiscVSbiLib.h>

#include <Library/DxeRiscvMpxy.h>
#include <Library/DxeRasAgentClient.h>

#define MAX_SOURCES    512
#define MAX_DESC_SIZE  1024

/* RAS Agent Services on MPXY/RPMI */
#define RAS_GET_NUM_ERR_SRCS      0x1
#define RAS_GET_ERR_SRCS_ID_LIST  0x2
#define RAS_GET_ERR_SRC_DESC      0x3

#define MM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data))

STATIC EFI_MM_COMMUNICATION2_PROTOCOL  *mMmCommunication2 = NULL;

EFI_STATUS (EFIAPI *gSendCommand)(VOID *CommBuffer, UINTN CmdLen, UINTN *RespLen, UINT8 FuncId);

#define __packed32  __attribute__((packed,aligned(__alignof__(UINT32))))

static ErrorSourceListResp  gErrorSourceListResp;
static ErrDescResp          gErrDescResp;
UINT32                      gMpxyChannelId = 0;

STATIC
EFI_STATUS
EFIAPI
ProbeRasAgentMpxyChannelId (
  OUT UINT32  *ChannelId
  )
{
  #define MAX_MPXY_CHANNELS  64
  UINTN       ChannelList[MAX_MPXY_CHANNELS];
  UINTN       Returned, Remaining, StartIndex = 0;
  EFI_STATUS  Status;
  BOOLEAN     Found = FALSE, ParsingDone = FALSE;
  UINTN       i, Id;
  UINT32      RasSrvGroup;

  while (!ParsingDone) {
    Status = SbiMpxyGetChannelList (
               StartIndex, /* Start index */
               &ChannelList[0],
               &Remaining,
               &Returned
               );

    if (Status != EFI_SUCCESS) {
      return Status;
    }

    /* This read has returned zero and we still haven't got what we need */
    if (Returned == 0) {
      return EFI_UNSUPPORTED;
    }

    for (i = 0; i < Returned; i++) {
      Id     = ChannelList[0];
      Status = SbiMpxyReadChannelAttrs (
                 Id,
                 MPXY_MSG_PROTO_ATTR_START, /* Base attribute Id */
                 1,                         /* Number of attributes to be read */
                 &RasSrvGroup
                 );

      if (Status != EFI_SUCCESS) {
        continue;
      }

      if (RasSrvGroup == 0xC) {
        Found       = TRUE;
        ParsingDone = TRUE;
        break;
      }
    }

    /* Read if some more to be read else we are done parsing */
    if (Remaining) {
      StartIndex = Returned;
      continue;
    } else {
      ParsingDone = TRUE;
    }
  }

  if (Found == TRUE) {
    *ChannelId = Id;
    DEBUG ((
      DEBUG_INFO,
      "Found RAS MPXY channel: %x\n",
      Id
      ));
  }

  return Status;
}

EFI_STATUS
EFIAPI
GetRasAgentMpxyChannelId (
  OUT UINT32  *ChannelId
  )
{
  return ProbeRasAgentMpxyChannelId (ChannelId);
}

/**
  TODO: This is a placeholder method that sends a RAS command to the SMM handler
  via MM communication, for now this will return EFI_NOT_FOUND. When the downstream
  logic that will handle the MM Communication calls is merged then this method
  should be updated.
  @param[in,out] CommBuffer   Command and response buffer.
  @param[in]     CmdLen       Command length.
  @param[out]    RespLen      Length of response received.
  @param[in]     FuncId       Function ID for the RAS operation.
  @retval EFI_SUCCESS         Command sent and response received successfully.
  @retval EFI_OUT_OF_RESOURCES  Allocation failure.
**/
STATIC
EFI_STATUS
EFIAPI
RacSendMMCommand (
  VOID   *CommBuffer,
  UINTN  CmdLen,
  UINTN  *RespLen,
  UINT8  FuncId
  )
{
  EFI_STATUS                 Status;
  UINTN                      CommBufferSize;
  EFI_MM_COMMUNICATE_HEADER  *SmmCommunicateHeader;

  CommBufferSize       = MM_COMMUNICATE_HEADER_SIZE + *RespLen;
  SmmCommunicateHeader = AllocateZeroPool (CommBufferSize);
  if (SmmCommunicateHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gMmHestGetErrorSourceInfoGuid);
  CopyMem (SmmCommunicateHeader->Data, (const void *)CommBuffer, *RespLen);
  SmmCommunicateHeader->MessageLength = *RespLen;

  Status = mMmCommunication2->Communicate (
                                mMmCommunication2,
                                SmmCommunicateHeader,
                                SmmCommunicateHeader,
                                &CommBufferSize
                                );

  *RespLen = CommBufferSize - MM_COMMUNICATE_HEADER_SIZE;
  CopyMem (CommBuffer, SmmCommunicateHeader->Data, *RespLen);
  FreePool (SmmCommunicateHeader);

  return Status;
}

/**
  Sends a RAS command using SBI MPXY messaging.
  @param[in,out] CommBuffer   Buffer for both command and response.
  @param[in]     CmdLen       Length of command data.
  @param[out]    RespLen      Length of response data.
  @param[in]     FuncId       Function ID for the command.
  @retval EFI_SUCCESS         Communication successful.
**/
STATIC
EFI_STATUS
EFIAPI
RacSendPassThroughCommand (
  VOID   *CommBuffer,
  UINTN  CmdLen,
  UINTN  *RespLen,
  UINT8  FuncId
  )
{
  EFI_STATUS  Status;

  Status = SbiMpxySendMessage (
             gMpxyChannelId,
             FuncId,
             CommBuffer,
             CmdLen,
             CommBuffer,
             RespLen
             );

  return Status;
}

EFI_STATUS
EFIAPI
RacInit (
  VOID
  )
{
  EFI_STATUS  Status;

  if (!PcdGetBool (PcdMMPassThroughEnable)) {
    Status = gBS->LocateProtocol (
                    &gEfiMmCommunication2ProtocolGuid,
                    NULL,
                    (VOID **)&mMmCommunication2
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    gSendCommand = RacSendMMCommand;
  } else {
    if (SbiMpxyInit () != EFI_SUCCESS) {
      return EFI_NOT_READY;
    }

    if (GetRasAgentMpxyChannelId (&gMpxyChannelId) != EFI_SUCCESS) {
      return EFI_NOT_READY;
    }

    DEBUG ((DEBUG_ERROR, "Rac Init Done\n"));

    gSendCommand = RacSendPassThroughCommand;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RacGetNumberErrorSources (
  UINT32  *NumErrorSources
  )
{
  struct __packed32 _NumErrSrc {
    RasRpmiRespHeader    RespHdr;
    UINT32               NumErrorSources;
  } RasMsgBuf;

  EFI_STATUS         Status;
  RasRpmiRespHeader  *RespHdr = &RasMsgBuf.RespHdr;
  UINTN              RespLen  = sizeof (RasMsgBuf);

  ZeroMem (&RasMsgBuf, sizeof (RasMsgBuf));
  RespHdr->func_id = RAS_GET_NUM_ERR_SRCS;
  Status           = gSendCommand (&RasMsgBuf, 0, &RespLen, RAS_GET_NUM_ERR_SRCS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RespHdr->status != 0) {
    return EFI_DEVICE_ERROR;
  }

  *NumErrorSources = RasMsgBuf.NumErrorSources;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RacGetErrorSourceIDList (
  OUT UINT32  **ErrorSourceList,
  OUT UINT32  *NumSources
  )
{
  UINT32             *RespData = &gErrorSourceListResp.ErrSourceList[0];
  RasRpmiRespHeader  *RespHdr  = &gErrorSourceListResp.RespHdr;
  EFI_STATUS         Status;
  UINTN              RespLen = sizeof (gErrorSourceListResp);

  ZeroMem (&gErrorSourceListResp, sizeof (gErrorSourceListResp));

  if (!ErrorSourceList) {
    return EFI_INVALID_PARAMETER;
  }

  gErrorSourceListResp.RespHdr.func_id = RAS_GET_ERR_SRCS_ID_LIST;
  Status                               = gSendCommand (&gErrorSourceListResp, 0, &RespLen, RAS_GET_ERR_SRCS_ID_LIST);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (RespHdr->status != 0) {
    return EFI_DEVICE_ERROR;
  }

  *NumSources      = RespHdr->returned;
  *ErrorSourceList = RespData;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RacGetErrorSourceDescriptor (
  IN UINT32   SourceID,
  OUT UINTN   *DescriptorType,
  OUT VOID    **ErrorDescriptor,
  OUT UINT32  *ErrorDescriptorSize
  )
{
  UINTN              RespLen = sizeof (gErrDescResp);
  EFI_STATUS         Status;
  RasRpmiRespHeader  *RspHdr = &gErrDescResp.RspHdr;
  UINT8              *Desc   = &gErrDescResp.desc[0];

  ZeroMem (&gErrDescResp, sizeof (gErrDescResp));

  *Desc                       = (UINT8)SourceID;
  gErrDescResp.RspHdr.func_id = RAS_GET_ERR_SRC_DESC;

  Status = gSendCommand (&gErrDescResp, RespLen, &RespLen, RAS_GET_ERR_SRC_DESC);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RspHdr->status != 0) {
    return EFI_DEVICE_ERROR;
  }

  if (RspHdr->remaining != 0) {
    return EFI_DEVICE_ERROR;
  }

  *DescriptorType = RspHdr->flags & ERROR_DESCRIPTOR_TYPE_MASK;

  ASSERT (*DescriptorType < MAX_ERROR_DESCRIPTOR_TYPES);

  *ErrorDescriptor     = (VOID *)Desc;
  *ErrorDescriptorSize = RspHdr->returned;

  return EFI_SUCCESS;
}

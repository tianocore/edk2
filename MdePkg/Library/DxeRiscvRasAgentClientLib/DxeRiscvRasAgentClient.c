/** @file
  This module provides communication with RAS Agent over RPMI/MPXY.

  @par Glossary:
    - RAS  - Reliability, Availability, and Serviceability
    - RPMI - RAS Platform Management Interface
    - MPXY - Message Proxy extension in the RISC-V SBI specification

  Copyright (c) 2026, Qualcomm Technologies, Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>

#include <IndustryStandard/Acpi.h>

#include <Protocol/AcpiTable.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/BaseRiscVSbiLib.h>

#include <Library/DxeRiscvMpxy.h>
#include <Library/DxeRasAgentClient.h>

#include "DxeRiscvRasAgentClientPrivate.h"

STATIC RAS_AGENT_CONTEXT  RasAgentContexts[MAX_RAS_AGENTS];
STATIC BOOLEAN            mRacLibraryInitialized = FALSE;
STATIC UINT32             mNumRasAgents;
STATIC UINT32             mRasAgentChannelIds[MAX_RAS_AGENTS];
#define RAS_AGENT_CHANNEL_LIST_SIZE  ARRAY_SIZE (mRasAgentChannelIds)

/**
   Probe RAS agent and return its MPXY channel id.

   @param[in]  ChannelIdSize   Size of ChannelId buffer.
   @param[out] ChannelId       Buffer to hold channel IDs.
   @param[out] NumChannelIds   Number of channel IDs found.

   @retval EFI_SUCCESS            Operation succeeded.
   @retval EFI_UNSUPPORTED        No RAS agent found.
   @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
   @retval Other                  Error returned by SBI, translated to EFI.
**/
STATIC
EFI_STATUS
ProbeRasAgentMpxyChannelId (
  IN  UINT32  ChannelIdSize,
  OUT UINT32  *ChannelId,
  OUT UINT32  *NumChannelIds
  )
{
  UINTN       ChannelList[MAX_MPXY_CHANNELS];
  UINTN       Returned;
  UINTN       Remaining;
  UINTN       StartIndex;
  EFI_STATUS  Status;
  BOOLEAN     ParsingDone;
  UINTN       OIndex;
  UINTN       Index;
  UINT32      RasSrvGroup;
  UINT32      NumSrvGroups;

  StartIndex   = 0;
  ParsingDone  = FALSE;
  NumSrvGroups = 0;

  if ((ChannelId == NULL) || (NumChannelIds == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ChannelIdSize < MAX_RAS_AGENTS) {
    return EFI_INVALID_PARAMETER;
  }

  while (!ParsingDone) {
    Status = SbiMpxyGetChannelList (
               StartIndex, /* Start index */
               &ChannelList[0],
               &Remaining,
               &Returned
               );

    if (Status != EFI_SUCCESS) {
      if ((Status == EFI_LOAD_ERROR) ||
          (Status == EFI_NOT_READY)  ||
          (Status == EFI_UNSUPPORTED))
      {
        return EFI_UNSUPPORTED;
      }

      return Status;
    }

    /* This read has returned zero and we still haven't got what we need */
    if (Returned == 0) {
      return EFI_UNSUPPORTED;
    }

    for (OIndex = 0; OIndex < Returned; OIndex++) {
      Index  = ChannelList[OIndex];
      Status = SbiMpxyReadChannelAttrs (
                 Index,
                 MPXY_MSG_PROTO_ATTR_START, /* Base attribute Id */
                 1,                         /* Number of attributes to be read */
                 &RasSrvGroup
                 );

      if (Status != EFI_SUCCESS) {
        continue;
      }

      /* Unlikely, but if more RAS agents are found than supported,
         move forward with only max number of RAS agents supported.
         Display a warning in such case.
      */
      if (RasSrvGroup == RAS_SRV_GRP_ID) {
        if (NumSrvGroups >= MAX_RAS_AGENTS) {
          DEBUG ((
            DEBUG_WARN,
            "Total RAS agents so far %u is more than supported (%u)\n",
            NumSrvGroups,
            MAX_RAS_AGENTS
            ));
          NumSrvGroups--;
          Status = EFI_SUCCESS;
          goto ParseDone;
        }

        ChannelId[NumSrvGroups] = Index;
        NumSrvGroups++;
        continue;
      }
    }

    /* Read if some more to be read else we are done parsing */
    if (Remaining != 0) {
      StartIndex = Returned;
      continue;
    } else {
      ParsingDone = TRUE;
    }
  }

ParseDone:
  *NumChannelIds = NumSrvGroups;

  return Status;
}

/**
   Get number of RAS agents present.

   @retval Returns number of RAS agents present.
**/
EFI_STATUS
EFIAPI
RacGetNumRasAgents (
  VOID
  )
{
  if (!mRacLibraryInitialized) {
    DEBUG ((DEBUG_ERROR, "%a: library not initialized\n", __func__));
    return EFI_NOT_READY;
  }

  return mNumRasAgents;
}

/**
   Get the list of RAS agent MPXY channel IDs.

   @param[in]  ChannelIdSize   Size of buffer used to fetch channel IDs.
   @param[out] ChannelId       Pointer to buffer used to fetch channel IDs.
   @param[out] NumChannelIds   Number of channel IDs put in the buffer.

   @retval EFI_SUCCESS            Operation succeeded.
   @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
RacGetRasAgentMpxyChannelId (
  IN  UINT32  ChannelIdSize,
  OUT UINT32  *ChannelId,
  OUT UINT32  *NumChannelIds
  )
{
  UINT32  Index;

  if ((ChannelId == NULL) || (NumChannelIds == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ChannelIdSize < mNumRasAgents) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < mNumRasAgents; Index++) {
    ChannelId[Index] = mRasAgentChannelIds[Index];
  }

  *NumChannelIds = mNumRasAgents;

  return EFI_SUCCESS;
}

/**
   Initialize RAS agent client library.

   @retval EFI_SUCCESS  Operation succeeded.
   @retval Other        Error returned by ProbeRasAgentMpxyChannelId().
**/
EFI_STATUS
EFIAPI
RacInit (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  Status = SbiMpxyLibInit ();
  if ((Status != EFI_SUCCESS) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  if (!mRacLibraryInitialized) {
    Status = ProbeRasAgentMpxyChannelId (
               RAS_AGENT_CHANNEL_LIST_SIZE,
               mRasAgentChannelIds,
               &mNumRasAgents
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    for (Index = 0; Index < MAX_RAS_AGENTS; Index++) {
      RasAgentContexts[Index].RefCount      = 0;
      RasAgentContexts[Index].MpxyChannelId = INVALID_MPXY_CHANNELID;
    }

    mRacLibraryInitialized = TRUE;
  }

  return EFI_SUCCESS;
}

/**
   Check if a channel id is valid.

   @param[in]  ChannelId  RAS agent's MPXY channel.

   @retval TRUE   Channel id is valid.
   @retval FALSE  Channel id is invalid.
**/
STATIC
BOOLEAN
RacChannelIdValid (
  IN  UINT32  ChannelId
  )
{
  UINT32  Index;

  if (!mRacLibraryInitialized) {
    return FALSE;
  }

  for (Index = 0; Index < mNumRasAgents; Index++) {
    if (mRasAgentChannelIds[Index] == ChannelId) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
   Find a RAS agent context for a given MPXY channel ID.

   @param[in] ChannelId  RAS agent's MPXY channel

   @retval Pointer to RAS context if found
   @retval NULL if not found
**/
STATIC
RAS_AGENT_CONTEXT *
FindRasAgentContext (
  IN UINT32  ChannelId
  )
{
  UINT32  Index;

  for (Index = 0; Index < MAX_RAS_AGENTS; Index++) {
    if (RasAgentContexts[Index].MpxyChannelId == ChannelId) {
      return &RasAgentContexts[Index];
    }
  }

  return NULL;
}

/**
   Allocate a free RAS agent context for a given channel ID.

   @param[in] ChannelId  Channel ID for which context is to be allocated.

   @retval Pointer to the allocated RAS context.
   @retval NULL if no free context is found.
**/
STATIC
RAS_AGENT_CONTEXT *
AllocRasAgentContext (
  IN  UINT32  ChannelId
  )
{
  UINT32  Index;

  for (Index = 0; Index < MAX_RAS_AGENTS; Index++) {
    if (RasAgentContexts[Index].MpxyChannelId == INVALID_MPXY_CHANNELID) {
      RasAgentContexts[Index].MpxyChannelId = ChannelId;
      RasAgentContexts[Index].RefCount++;

      return &RasAgentContexts[Index];
    }
  }

  return NULL;
}

/**
   Free a RAS agent context when the reference count becomes zero.

   @param[in]  Context  Pointer to the RAS agent context to be freed
**/
STATIC
VOID
FreeRasAgentContext (
  IN  RAS_AGENT_CONTEXT  *Context
  )
{
  if (Context == NULL) {
    return;
  }

  if (Context->RefCount == 0) {
    return;
  }

  Context->RefCount--;

  if (Context->RefCount == 0) {
    Context->MpxyChannelId = INVALID_MPXY_CHANNELID;
    Context->RefCount      = 0;
  }
}

/**
   Open a RAS agent's MPXY channel.

   @param[in] ChannelId  RAS agent's MPXY channel
   @retval    EFI_SUCCESS on success
   @retval    EFI_ABORTED if a found context has zero reference count
   @retval    EFI errors as returned by `SbiMpxyChannelOpen`
**/
EFI_STATUS
EFIAPI
RacOpenRasAgentChannel (
  IN  UINT32  ChannelId
  )
{
  RAS_AGENT_CONTEXT  *ChannelContext;
  EFI_STATUS         Status;

  if (!RacChannelIdValid (ChannelId)) {
    return EFI_INVALID_PARAMETER;
  }

  ChannelContext = FindRasAgentContext (ChannelId);
  if (ChannelContext != NULL) {
    ChannelContext->RefCount++;
    return EFI_SUCCESS;
  }

  ChannelContext = AllocRasAgentContext (ChannelId);
  if (ChannelContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SbiMpxyChannelOpen (ChannelId);
  if (Status != EFI_SUCCESS) {
    FreeRasAgentContext (ChannelContext);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
   Close a RAS agent's MPXY channel.

   @param[in] ChannelId  RAS agent's MPXY channel
   @retval    EFI_SUCCESS on success
   @retval    EFI_ABORTED if a found context has zero reference count
   @retval    EFI errors as returned by `SbiMpxyChannelClose`
**/
EFI_STATUS
EFIAPI
RacCloseRasAgentChannel (
  IN  UINT32  ChannelId
  )
{
  RAS_AGENT_CONTEXT  *ChannelContext;
  EFI_STATUS         Status;

  if (!RacChannelIdValid (ChannelId)) {
    return EFI_INVALID_PARAMETER;
  }

  ChannelContext = FindRasAgentContext (ChannelId);
  if (ChannelContext != NULL) {
    ChannelContext->RefCount++;
    return EFI_SUCCESS;
  }

  ChannelContext = AllocRasAgentContext (ChannelId);
  if (ChannelContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SbiMpxyChannelClose (ChannelId);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  FreeRasAgentContext (ChannelContext);

  return EFI_SUCCESS;
}

/**
   Get the number of error sources present in the system.

   @param[in]  ChannelId        RAS agent's MPXY channel.
   @param[out] NumErrorSources  Number of error sources present.

   @retval EFI_SUCCESS            Operation succeeded.
   @retval EFI_NOT_READY          Library/channel is not initialized.
   @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
   @retval EFI_DEVICE_ERROR       RAS agent returned an error status.
**/
EFI_STATUS
EFIAPI
RacGetNumberErrorSources (
  IN  UINT32  ChannelId,
  OUT UINT32  *NumErrorSources
  )
{
  NUM_ERR_SRC_RESP      RasMsgBuf;
  EFI_STATUS            Status;
  RAS_RPMI_RESP_HEADER  *RespHdr;
  UINTN                 RespLen;
  RAS_AGENT_CONTEXT     *Context;

  RespHdr = &RasMsgBuf.RespHdr;
  RespLen = sizeof (RasMsgBuf);
  Context = NULL;

  if (!mRacLibraryInitialized) {
    return EFI_NOT_READY;
  }

  if (!RacChannelIdValid (ChannelId)) {
    return EFI_INVALID_PARAMETER;
  }

  Context = FindRasAgentContext (ChannelId);

  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  /* Check if the channel is opened */
  if (Context->RefCount == 0) {
    return EFI_NOT_READY;
  }

  ZeroMem (&RasMsgBuf, sizeof (RasMsgBuf));

  Status = SbiMpxySendMessage (
             ChannelId,
             RAS_GET_NUM_ERR_SRCS,
             &RasMsgBuf,
             sizeof (UINT32),
             (VOID *)&RasMsgBuf,
             &RespLen
             );
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RespHdr->Status != 0) {
    return EFI_DEVICE_ERROR;
  }

  *NumErrorSources = RasMsgBuf.NumErrorSources;

  return EFI_SUCCESS;
}

/**
   Get the list of hardware error source IDs from the RAS Agent.

   @param[in]   ChannelId        RAS agent's MPXY channel.
   @param[out]  ErrorSourceList  Returned pointer to list of source IDs.
   @param[out]  NumSources       Number of source IDs in ErrorSourceList.

   @retval EFI_SUCCESS            The error source list was fetched.
   @retval EFI_NOT_READY          Library/channel is not initialized.
   @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
   @retval EFI_DEVICE_ERROR       RAS agent returned an error status.
**/
EFI_STATUS
EFIAPI
RacGetErrorSourceIDList (
  IN  UINT32  ChannelId,
  OUT UINT32  **ErrorSourceList,
  OUT UINT32  *NumSources
  )
{
  UINT32                *RespData;
  RAS_RPMI_RESP_HEADER  *RespHdr;
  EFI_STATUS            Status;
  UINTN                 RespLen;
  RAS_AGENT_CONTEXT     *Context;

  if (!mRacLibraryInitialized) {
    return EFI_NOT_READY;
  }

  if (!RacChannelIdValid (ChannelId)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ErrorSourceList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Context = FindRasAgentContext (ChannelId);
  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Context->RefCount == 0) {
    return EFI_NOT_READY;
  }

  ZeroMem (&Context->SourceListResp, sizeof (ERROR_SOURCE_LIST_RESP));

  RespData = &Context->SourceListResp.ErrSourceList[0];
  RespHdr  = &Context->SourceListResp.RespHdr;
  RespLen  = sizeof (ERROR_SOURCE_LIST_RESP);

  Status = SbiMpxySendMessage (
             ChannelId,
             RAS_GET_ERR_SRCS_ID_LIST,
             &Context->SourceListResp,
             sizeof (ERROR_SOURCE_LIST_RESP),
             &Context->SourceListResp,
             &RespLen
             );

  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RespHdr->Status != 0) {
    return EFI_DEVICE_ERROR;
  }

  *NumSources      = RespHdr->Returned;
  *ErrorSourceList = RespData;

  return EFI_SUCCESS;
}

/**
   Get the hardware error source descriptor for an error source ID.

   @param[in]   ChannelId             RAS agent's MPXY channel.
   @param[in]   SourceID              Error source ID.
   @param[out]  DescriptorType        Descriptor type.
   @param[out]  ErrorDescriptor       Pointer to descriptor buffer.
   @param[out]  ErrorDescriptorSize   Size of descriptor buffer.

   @retval EFI_SUCCESS            Descriptor was fetched.
   @retval EFI_NOT_READY          Library/channel is not initialized.
   @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
   @retval EFI_DEVICE_ERROR       RAS agent returned an error status.
**/
EFI_STATUS
EFIAPI
RacGetErrorSourceDescriptor (
  IN  UINT32  ChannelId,
  IN  UINT32  SourceID,
  OUT UINTN   *DescriptorType,
  OUT VOID    **ErrorDescriptor,
  OUT UINT32  *ErrorDescriptorSize
  )
{
  ERR_DESC_RESP         *Resp;
  UINTN                 RespLen;
  EFI_STATUS            Status;
  RAS_RPMI_RESP_HEADER  *RspHdr;
  UINT8                 *Desc;
  UINT32                *EID;
  RAS_AGENT_CONTEXT     *Context;

  RespLen = sizeof (ERR_DESC_RESP);
  Resp    = NULL;
  Context = NULL;

  if (!mRacLibraryInitialized) {
    return EFI_NOT_READY;
  }

  if (!RacChannelIdValid (ChannelId)) {
    return EFI_INVALID_PARAMETER;
  }

  Context = FindRasAgentContext (ChannelId);
  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Context->RefCount == 0) {
    return EFI_NOT_READY;
  }

  Resp = &Context->DescResp;
  EID  = (UINT32 *)Resp;
  ZeroMem (Resp, sizeof (ERR_DESC_RESP));
  RspHdr = &Resp->RspHdr;
  Desc   = &Resp->Desc[0];
  *EID   = SourceID;

  Status = SbiMpxySendMessage (
             ChannelId,
             RAS_GET_ERR_SRC_DESC,
             Resp,
             sizeof (ERR_DESC_RESP),
             Resp,
             &RespLen
             );

  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (RspHdr->Status != 0) {
    return EFI_DEVICE_ERROR;
  }

  if (RspHdr->Remaining != 0) {
    return EFI_DEVICE_ERROR;
  }

  *DescriptorType = RspHdr->Flags & ERROR_DESCRIPTOR_TYPE_MASK;

  ASSERT (*DescriptorType < MAX_ERROR_DESCRIPTOR_TYPES);

  *ErrorDescriptor     = (VOID *)Desc;
  *ErrorDescriptorSize = RspHdr->Returned;

  return EFI_SUCCESS;
}

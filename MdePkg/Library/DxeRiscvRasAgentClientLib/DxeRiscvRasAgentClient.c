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

#define __packed32  __attribute__((packed,aligned(__alignof__(UINT32))))

typedef struct __packed32 {
  UINT32    status;
  UINT32    flags;
  UINT32    remaining;
  UINT32    returned;
} RasRpmiRespHeader;

typedef struct __packed32 {
  RasRpmiRespHeader    RespHdr;
  UINT32               ErrSourceList[MAX_SOURCES];
} ErrorSourceListResp;

typedef struct __packed32 {
  RasRpmiRespHeader    RspHdr;
  UINT8                desc[MAX_DESC_SIZE];
} ErrDescResp;

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

EFI_STATUS
EFIAPI
RacInit (
  VOID
  )
{
  if (GetRasAgentMpxyChannelId (&gMpxyChannelId) != EFI_SUCCESS) {
    return EFI_NOT_READY;
  }

  if (SbiMpxyChannelOpen (gMpxyChannelId) != EFI_SUCCESS) {
    return EFI_NOT_READY;
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

  Status = SbiMpxySendMessage (
             gMpxyChannelId,
             RAS_GET_NUM_ERR_SRCS,
             &RasMsgBuf,
             sizeof (UINT32),
             (VOID *)&RasMsgBuf,
             &RespLen
             );
  if (Status != EFI_SUCCESS) {
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

  Status = SbiMpxySendMessage (
             gMpxyChannelId,
             RAS_GET_ERR_SRCS_ID_LIST,
             &gErrorSourceListResp,
             sizeof (gErrorSourceListResp),
             &gErrorSourceListResp,
             &RespLen
             );

  if (Status != EFI_SUCCESS) {
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
  UINT8              *desc   = &gErrDescResp.desc[0];
  UINT32             *EID    = (UINT32 *)&gErrDescResp;

  ZeroMem (&gErrDescResp, sizeof (gErrDescResp));

  *EID = SourceID;

  Status = SbiMpxySendMessage (
             gMpxyChannelId,
             RAS_GET_ERR_SRC_DESC,
             &gErrDescResp,
             sizeof (gErrDescResp),
             &gErrDescResp,
             &RespLen
             );

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

  *ErrorDescriptor     = (VOID *)desc;
  *ErrorDescriptorSize = RspHdr->returned;

  return EFI_SUCCESS;
}

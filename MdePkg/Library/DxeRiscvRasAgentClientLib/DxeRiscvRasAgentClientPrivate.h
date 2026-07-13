/** @file
  Private definitions for the DXE RAS agent client library.

  @par Glossary:
    - RAS  - Reliability, Availability, and Serviceability
    - RPMI - RAS Platform Management Interface
    - MPXY - Message Proxy extension in the RISC-V SBI specification

  Copyright (c) 2026, Qualcomm Technologies, Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

#define MAX_MPXY_CHANNELS  64

#define RAS_SRV_GRP_ID  0xC
#define MAX_SOURCES     512
#define MAX_DESC_SIZE   1024

#define INVALID_MPXY_CHANNELID  0xFFFFFFFFUL

/* RAS Agent Services on MPXY/RPMI */
#define RAS_ENABLE_NOTIFICATION   0x1
#define RAS_GET_NUM_ERR_SRCS      0x2
#define RAS_GET_ERR_SRCS_ID_LIST  0x3
#define RAS_GET_ERR_SRC_DESC      0x4

#pragma pack(push, 4)
typedef struct {
  UINT32    Status;
  UINT32    Flags;
  UINT32    Remaining;
  UINT32    Returned;
} RAS_RPMI_RESP_HEADER;

typedef struct {
  RAS_RPMI_RESP_HEADER    RespHdr;
  UINT32                  NumErrorSources;
} NUM_ERR_SRC_RESP;

typedef struct {
  RAS_RPMI_RESP_HEADER    RespHdr;
  UINT32                  ErrSourceList[MAX_SOURCES];
} ERROR_SOURCE_LIST_RESP;

typedef struct {
  RAS_RPMI_RESP_HEADER    RspHdr;
  UINT8                   Desc[MAX_DESC_SIZE];
} ERR_DESC_RESP;

typedef struct {
  UINT32                    MpxyChannelId;
  UINT32                    RefCount;
  ERR_DESC_RESP             DescResp;
  ERROR_SOURCE_LIST_RESP    SourceListResp;
} RAS_AGENT_CONTEXT;
#pragma pack(pop)

/** @file
  Define ArmMmHandlerContext passed to MmHandler in Arm platform.

  Copyright (c) 2025, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_MM_HANDLER_CONTEXT_H_
#define ARM_MM_HANDLER_CONTEXT_H_

/*
 * Communication ABI protocol to communicate between normal/secure partition.
 */
typedef enum {
  /// Unknown Communication ABI protocol
  CommProtocolUnknown,

  /// Communicate via SPM_MM ABI protocol
  CommProtocolSpmMm,

  /// Communicate via FF-A ABI protocol
  CommProtocolFfa,

  CommProtocolMax,
} COMM_PROTOCOL;

/** When using FF-A ABI, there are ways to request service to StandaloneMm
      - FF-A with MmCommunication protocol.
      - FF-A service with each specification.
   MmCommunication Protocol can use FFA_MSG_SEND_DIRECT_REQ or REQ2,
   Other FF-A services should use FFA_MSG_SEND_DIRECT_REQ2.
   In case of FF-A with MmCommunication protocol via FFA_MSG_SEND_DIRECT_REQ,
   register x3 saves Communication Buffer with gEfiMmCommunication2ProtocolGuid.
   In case of FF-A with MmCommunication protocol via FFA_MSG_SEND_DIRECT_REQ2,
   register x2/x3 save gEfiMmCommunication2ProtocolGuid and
   register x4 saves Communication Buffer with Service Guid.

   Other FF-A services (ServiceTypeMisc) delivers register values according to
   there own service specification.
   That means it doesn't use MmCommunication Buffer with MmCommunication Header
   format.
   (i.e) Tpm service via FF-A or Firmware Update service via FF-A.
   To support latter services by StandaloneMm, it defines SERVICE_TYPE_MISC.
   So that StandaloneMmEntryPointCore.c generates MmCommunication Header
   with delivered register values to dispatch service provided StandaloneMmCore.
   So that service handler can get proper information from delivered register.

   In case of SPM_MM Abi, it only supports MmCommunication service.
 */
typedef enum {
  /// Unknown
  ServiceTypeUnknown,

  /// MmCommunication services
  ServiceTypeMmCommunication,

  /// Misc services
  ServiceTypeMisc,

  ServiceTypeMax,
} SERVICE_TYPE;

/** Direct message request/response version
 */
typedef enum {
  /// Direct message version 1. Use FFA_DIRECT_MSG_REQ/RESP
  DirectMsgV1,

  /// Direct message version 2. Use FFA_DIRECT_MSG_REQ2/RESP2
  DirectMsgV2,

  DirectMsgMax,
} DIRECT_MSG_VERSION;

/**
 * SPM_MM ABI data
 */
typedef struct {
  /// Service Type
  SERVICE_TYPE    ServiceType;

  /// Secure Request
  BOOLEAN         SecureRequest;
} SPM_MM_MSG_INFO;

/** Ffa Abi data used in FFA_MSG_SEND_DIRECT_RESP/RESP2.
 */
typedef struct FfaMsgInfo {
  /// Source partition id
  UINT16                SourcePartId;

  /// Destination partition id
  UINT16                DestPartId;

  /// Direct Message version
  DIRECT_MSG_VERSION    DirectMsgVersion;

  /// Service Type
  SERVICE_TYPE          ServiceType;
} FFA_MSG_INFO;

typedef union {
  /// SPM_MM message info
  SPM_MM_MSG_INFO    SpmMmInfo;

  /// FF-A message info
  FFA_MSG_INFO       FfaMsgInfo;
} ARM_MM_HANDLER_CTX_DATA;

typedef struct {
  /// Communication Protocol
  COMM_PROTOCOL              CommProtocol;

  /// Context Data according to CommProtocol
  ARM_MM_HANDLER_CTX_DATA    CtxData;
} ARM_MM_HANDLER_CONTEXT;

#endif // ARM_MM_HANDLER_CONTEXT_H_

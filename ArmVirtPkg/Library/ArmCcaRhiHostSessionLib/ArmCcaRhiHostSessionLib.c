/** @file
  Library that implements the Realm Host Interface Host Session protocol.

  Copyright (c) 2024 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rhi or RHI   - Realm Host Interface
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
   - Realm Host Interface (RHI) Specification, version 1.0-bet0
     (https://developer.arm.com/documentation/den0148/)
**/

#include <Base.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmCcaRhiHostSessionLib.h>

STATIC CONST CHAR8  *ArmCcaRhiSessionCmds[] = {
  "ARM_CCA_FID_RHI_SESSION_VERSION",
  "ARM_CCA_FID_RHI_SESSION_FEATURES",
  "ARM_CCA_FID_RHI_SESSION_OPEN",
  "ARM_CCA_FID_RHI_SESSION_CLOSE",
  "ARM_CCA_FID_RHI_SESSION_SEND",
  "ARM_CCA_FID_RHI_SESSION_RECEIVE",
};

/** A macro to print the RSI Session Error and call arguments.

  @param[in] RhiFid   Fid for the RHI protocol call.
  @param[in] Status   The status code returned by the RSI Host Call.
  @param[in] Args     Pointer to the Host Call arguments.
*/
#define ARM_CCA_RHI_PRINT_ERROR(Fid, Status, Args)                        \
          DEBUG ((                                                        \
            DEBUG_ERROR,                                                  \
            "Error: RsiHost Call %a Returned %r, "                        \
            "Args: Gprs0 = 0x%lx, Gprs1 = 0x%lx, "                        \
            "Gprs2 = 0x%lx, Gprs3 = 0x%lx\n",                             \
            ArmCcaRhiSessionCmds[Fid - ARM_CCA_FID_RHI_SESSION_VERSION],  \
            Status,                                                       \
            Args->Gprs0,                                                  \
            Args->Gprs1,                                                  \
            Args->Gprs2,                                                  \
            Args->Gprs3                                                   \
            ));

/** An enum defining the RHI command return status.
*/
typedef enum ArmCcaRhiSessionReturnStatus {
  RhiSessionStatusSuccess,                     ///< Success
  RhiSessionStatusPeerNotAvailable,            ///< Peer not available
  RhiSessionStatusInvalidStateForOperation,    ///< Invalid state for operation
  RhiSessionStatusInvalidSessionId,            ///< Invalid Session ID
  RhiSessionStatusConnectionTypeNotSupported,  ///< Connection type not supported
  RhiSessionStatusAccessFailed,                ///< Buffer not readable/writable or address not granule aligned
  RhiSessionStatusMax
} ARM_CCA_RHI_SESSION_RETURN_STATUS;

/**
  Return EFI Status code corresponding to RHI Host Session Return Code.

  @param[in]     ReturnCode    Return Code.

  @retval RETURN_SUCCESS             Success.
  @retval RETURN_NOT_FOUND           Peer not available.
  @retval RETURN_PROTOCOL_ERROR      Invalid state for operation.
  @retval RETURN_NO_MAPPING          An invalid session ID was provided.
  @retval RETURN_UNSUPPORTED         The requested connection mode is not
                                  supported.
  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_ABORTED             Invalid return code.
**/
STATIC
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionReturnCodeToEfiStatus (
  IN  ARM_CCA_RHI_SESSION_RETURN_STATUS  ReturnCode
  )
{
  switch (ReturnCode) {
    case RhiSessionStatusSuccess:
      return RETURN_SUCCESS;

    case RhiSessionStatusPeerNotAvailable:
      return RETURN_NOT_FOUND;

    case RhiSessionStatusInvalidStateForOperation:
      return RETURN_PROTOCOL_ERROR;

    case RhiSessionStatusInvalidSessionId:
      return RETURN_NO_MAPPING;

    case RhiSessionStatusConnectionTypeNotSupported:
      return RETURN_UNSUPPORTED;

    case RhiSessionStatusAccessFailed:
      return RETURN_INVALID_PARAMETER;

    case RhiSessionStatusMax:
    default:
      ASSERT (0);
      return RETURN_ABORTED;
  }
}

/**
  Get the RHI Host Session protocol version information.

  @param[out] ProtocolVersion       The version of the protocol.

  @retval RETURN_UNSUPPORTED         RHI Host Session protocol not supported.
  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_OUT_OF_RESOURCES    Insufficient memory.
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionVersion (
  OUT UINT64  *ProtocolVersion
  )
{
  RETURN_STATUS               Status;
  ARM_CCA_RSI_HOST_CALL_ARGS  *Args;

  if (ProtocolVersion == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Args = AllocateAlignedPages (
           EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)),
           ARM_CCA_REALM_GRANULE_SIZE
           );
  if (Args == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Args, sizeof (ARM_CCA_RSI_HOST_CALL_ARGS));
  Args->Gprs0 = ARM_CCA_FID_RHI_SESSION_VERSION;

  Status = ArmCcaRsiHostCall (Args);
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_VERSION, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  if (Args->Gprs0 == ARM_SMC_MM_RET_NOT_SUPPORTED) {
    Status = EFI_UNSUPPORTED;
  } else {
    // Protocol version
    *ProtocolVersion = Args->Gprs0;
  }

exit_handler:
  FreeAlignedPages (Args, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)));
  return Status;
}

/**
  Perform RHI Host Session protocol feature discovery.

  @param[out] AbiSupportBitmap      A bitmap of supported ABI calls.
  @param[out] ConnectionModeBitmap  A bitmap of supported Connection modes.

  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_OUT_OF_RESOURCES    Insufficient memory.
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionFeatures (
  OUT UINT64  *AbiSupportBitmap,
  OUT UINT64  *ConnectionModeBitmap
  )
{
  RETURN_STATUS               Status;
  ARM_CCA_RSI_HOST_CALL_ARGS  *Args;

  if ((AbiSupportBitmap == NULL) ||
      (ConnectionModeBitmap == NULL))
  {
    return RETURN_INVALID_PARAMETER;
  }

  Args = AllocateAlignedPages (
           EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)),
           ARM_CCA_REALM_GRANULE_SIZE
           );
  if (Args == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Args, sizeof (ARM_CCA_RSI_HOST_CALL_ARGS));
  Args->Gprs0 = ARM_CCA_FID_RHI_SESSION_FEATURES;

  Status = ArmCcaRsiHostCall (Args);
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_FEATURES, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  // ABI call support bitmap
  *AbiSupportBitmap = Args->Gprs0 & ARM_CCA_RHI_SESSION_ABI_MASK;

  // Connection Mode support bitmap
  *ConnectionModeBitmap = Args->Gprs1 & ARM_CCA_RHI_SESSION_CONNECTION_MODE_MASK;

exit_handler:
  FreeAlignedPages (Args, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)));
  return Status;
}

/**
  Open a session with the host.

  @param[in]      ConnectionMode  The requested connection mode, must be either
                                  Blocking or Non-Blocking.
  @param[in, out] SessionId       On first invocation, SessionId is passed as
                                  zero and the ABI allocates a Session Id and
                                  returns.
                                  If the connection mode is Non-Blocking, the
                                  SessionState may be returned as
                                  RhiConnectionInProgress and the
                                  caller is expected to call this function again
                                  with the Session ID that was returned in the
                                  first invocation.
  @param[out]     SessionState    State of the session.

  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_OUT_OF_RESOURCES    Insufficient memory.
  @retval RETURN_UNSUPPORTED         The requested connection mode is not
                                     supported.
  @retval RETURN_NO_MAPPING          An invalid session ID was provided.
  @retval RETURN_PROTOCOL_ERROR      A protocol error was detected.
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionOpen (
  IN      UINT64                     ConnectionMode,
  IN OUT  ARM_CCA_RHI_SESSION_ID     *SessionId,
  OUT     ARM_CCA_RHI_SESSION_STATE  *SessionState OPTIONAL
  )
{
  RETURN_STATUS               Status;
  ARM_CCA_RSI_HOST_CALL_ARGS  *Args;

  if (SessionId == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((ConnectionMode != ARM_CCA_RHI_SESSION_CONNECTION_MODE_BLOCKING) &&
      (ConnectionMode != ARM_CCA_RHI_SESSION_CONNECTION_MODE_NON_BLOCKING))
  {
    return RETURN_UNSUPPORTED;
  }

  // invalid_session_id: 0 not passed on initial call or
  // unknown SessionID passed (NON_BLOCKING)
  if ((ConnectionMode == ARM_CCA_RHI_SESSION_CONNECTION_MODE_BLOCKING) &&
      (*SessionId != 0))
  {
    return RETURN_NO_MAPPING;
  }

  Args = AllocateAlignedPages (
           EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)),
           ARM_CCA_REALM_GRANULE_SIZE
           );
  if (Args == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Args, sizeof (ARM_CCA_RSI_HOST_CALL_ARGS));
  Args->Gprs0 = ARM_CCA_FID_RHI_SESSION_OPEN;
  Args->Gprs1 = *SessionId;
  Args->Gprs2 = ConnectionMode;

  Status = ArmCcaRsiHostCall (Args);
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_OPEN, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  Status = ArmCcaRhiSessionReturnCodeToEfiStatus (
             (ARM_CCA_RHI_SESSION_RETURN_STATUS)Args->Gprs0
             );
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_OPEN, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  if (*SessionId == 0) {
    *SessionId = Args->Gprs1;
  } else if (*SessionId != Args->Gprs1) {
    Status = RETURN_PROTOCOL_ERROR;
    ASSERT (0);
    goto exit_handler;
  }

  if (SessionState != NULL) {
    *SessionState = Args->Gprs2;
  }

exit_handler:
  FreeAlignedPages (Args, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)));
  return Status;
}

/**
  Close a session with the host.

  @param[in]    SessionId         Id of the session to close.
  @param[out]   SessionState      State of the session.

  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_OUT_OF_RESOURCES    Insufficient memory.
  @retval RETURN_NO_MAPPING          An invalid session ID was provided.
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionClose (
  IN    ARM_CCA_RHI_SESSION_ID     SessionId,
  OUT   ARM_CCA_RHI_SESSION_STATE  *SessionState   OPTIONAL
  )
{
  RETURN_STATUS               Status;
  ARM_CCA_RSI_HOST_CALL_ARGS  *Args;

  if (SessionId == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  Args = AllocateAlignedPages (
           EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)),
           ARM_CCA_REALM_GRANULE_SIZE
           );
  if (Args == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Args, sizeof (ARM_CCA_RSI_HOST_CALL_ARGS));
  Args->Gprs0 = ARM_CCA_FID_RHI_SESSION_CLOSE;
  Args->Gprs1 = SessionId;

  Status = ArmCcaRsiHostCall (Args);
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_CLOSE, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  Status = ArmCcaRhiSessionReturnCodeToEfiStatus (
             (ARM_CCA_RHI_SESSION_RETURN_STATUS)Args->Gprs0
             );
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_CLOSE, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  ASSERT (SessionId == Args->Gprs1);

  if (SessionState != NULL) {
    *SessionState = Args->Gprs2;
  }

exit_handler:
  FreeAlignedPages (Args, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)));
  return Status;
}

/**
  Transmit data on previously opened communication channel.

  @param[in]      SessionId     The session ID for the communication channel.
  @param[in]      Data          Realm IPA for buffer containing data.
  @param[in, out] DataLen       Length of data to send in bytes on input and
                                Length of data transmitted on return.
  @param[in]      Offset        Offset in buffer from which to send data.
  @param[out]     SessionState  State of the session.

  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_OUT_OF_RESOURCES    Insufficient memory.
  @retval RETURN_NO_MAPPING          An invalid session ID was provided.
  @retval RETURN_PROTOCOL_ERROR      A protocol error was detected.
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionSend (
  IN      ARM_CCA_RHI_SESSION_ID     SessionId,
  IN      UINT8                      *Data,
  IN OUT  UINTN                      *DataLen,
  IN      UINTN                      Offset,
  OUT     ARM_CCA_RHI_SESSION_STATE  *SessionState OPTIONAL
  )
{
  RETURN_STATUS               Status;
  ARM_CCA_RSI_HOST_CALL_ARGS  *Args;

  if ((SessionId == 0)  ||
      (Data == NULL)    ||
      (DataLen == NULL) ||
      (*DataLen == 0)   ||
      (!IS_ALIGNED (Data, ARM_CCA_REALM_GRANULE_SIZE)))
  {
    return RETURN_INVALID_PARAMETER;
  }

  Args = AllocateAlignedPages (
           EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)),
           ARM_CCA_REALM_GRANULE_SIZE
           );
  if (Args == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Args, sizeof (ARM_CCA_RSI_HOST_CALL_ARGS));
  Args->Gprs0 = ARM_CCA_FID_RHI_SESSION_SEND;
  Args->Gprs1 = SessionId;
  Args->Gprs2 = (UINT64)(UINT64 *)Data;
  Args->Gprs3 = *DataLen;
  Args->Gprs4 = Offset;

  Status = ArmCcaRsiHostCall (Args);
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_SEND, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  Status = ArmCcaRhiSessionReturnCodeToEfiStatus (
             (ARM_CCA_RHI_SESSION_RETURN_STATUS)Args->Gprs0
             );
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_SEND, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  ASSERT (SessionId == Args->Gprs1);

  if (SessionState != NULL) {
    *SessionState = Args->Gprs2;
  }

  *DataLen = Args->Gprs3;

exit_handler:
  FreeAlignedPages (Args, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)));
  return Status;
}

/**
  Receive data on previously opened communication channel.

  @param[in]      SessionId     The session ID for the communication channel.
  @param[in]      Data          Realm IPA for buffer used to receive data.
  @param[in, out] DataLen       Size of receiving data buffer.
  @param[in]      Offset        Offset in buffer where received data is to be
                                written.
  @param[out]     SessionState  State of the session.

  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_OUT_OF_RESOURCES    Insufficient memory.
  @retval RETURN_NO_MAPPING          An invalid session ID was provided.
  @retval RETURN_PROTOCOL_ERROR      A protocol error was detected.
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionReceive (
  IN      ARM_CCA_RHI_SESSION_ID     SessionId,
  IN      UINT8                      *Data,
  IN OUT  UINTN                      *DataLen,
  IN      UINTN                      Offset,
  OUT     ARM_CCA_RHI_SESSION_STATE  *SessionState OPTIONAL
  )
{
  RETURN_STATUS               Status;
  ARM_CCA_RSI_HOST_CALL_ARGS  *Args;

  if ((SessionId == 0)  ||
      (Data == NULL)    ||
      (DataLen == NULL) ||
      (*DataLen == 0)   ||
      (!IS_ALIGNED (Data, ARM_CCA_REALM_GRANULE_SIZE)))
  {
    return RETURN_INVALID_PARAMETER;
  }

  Args = AllocateAlignedPages (
           EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)),
           ARM_CCA_REALM_GRANULE_SIZE
           );
  if (Args == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  ZeroMem (Args, sizeof (ARM_CCA_RSI_HOST_CALL_ARGS));

  Args->Gprs0 = ARM_CCA_FID_RHI_SESSION_RECEIVE;
  Args->Gprs1 = SessionId;
  Args->Gprs2 = (UINT64)(UINT64 *)Data;
  Args->Gprs3 = (UINT64)*DataLen;
  Args->Gprs4 = Offset;

  Status = ArmCcaRsiHostCall (Args);
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_RECEIVE, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  Status = ArmCcaRhiSessionReturnCodeToEfiStatus (
             (ARM_CCA_RHI_SESSION_RETURN_STATUS)Args->Gprs0
             );
  if (RETURN_ERROR (Status)) {
    ARM_CCA_RHI_PRINT_ERROR (ARM_CCA_FID_RHI_SESSION_RECEIVE, Status, Args);
    ASSERT (0);
    goto exit_handler;
  }

  ASSERT (SessionId == Args->Gprs1);

  if (SessionState != NULL) {
    *SessionState = Args->Gprs2;
  }

  *DataLen = Args->Gprs3;

exit_handler:
  FreeAlignedPages (Args, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_RSI_HOST_CALL_ARGS)));
  return Status;
}

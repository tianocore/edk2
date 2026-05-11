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

#pragma once

/**
  Macros defining the RHI Session ABIs.
*/
#define ARM_CCA_RHI_SESSION_ABI_MASK               0xFULL
#define ARM_CCA_RHI_SESSION_ABI_OPEN_SUPPORTED     BIT0
#define ARM_CCA_RHI_SESSION_ABI_CLOSE_SUPPORTED    BIT1
#define ARM_CCA_RHI_SESSION_ABI_SEND_SUPPORTED     BIT2
#define ARM_CCA_RHI_SESSION_ABI_RECEIVE_SUPPORTED  BIT3

#define ARM_CCA_RHI_SESSION_MANDATORY_ABIS  (           \
          ARM_CCA_RHI_SESSION_ABI_OPEN_SUPPORTED      | \
          ARM_CCA_RHI_SESSION_ABI_CLOSE_SUPPORTED     | \
          ARM_CCA_RHI_SESSION_ABI_SEND_SUPPORTED      | \
          ARM_CCA_RHI_SESSION_ABI_RECEIVE_SUPPORTED     \
          )

/**
  Macros defining the connection mode of the RHI Session.
*/
#define ARM_CCA_RHI_SESSION_CONNECTION_MODE_MASK          0x3ULL
#define ARM_CCA_RHI_SESSION_CONNECTION_MODE_BLOCKING      BIT0
#define ARM_CCA_RHI_SESSION_CONNECTION_MODE_NON_BLOCKING  BIT1

/** A type defining the RHI session ID.
*/
typedef UINT64 ARM_CCA_RHI_SESSION_ID;

/** An enum defining the RHI session states.
*/
typedef enum ArmCcaRhiSessionState {
  RhiSessionStateUnconnected,                ///< Session not connected
  RhiSessionStateConnectionInProgress,       ///< Connection in progress
  RhiSessionStateConnectionEstablished,      ///< Connection established
  RhiSessionStateIoInProgress,               ///< IO in progress
  RhiSessionStateIoComplete,                 ///< IO completed
  RhiSessionStateBufferSizeDetermined,       ///< Buffer size determined
  RhiSessionStateConnectionCloseInProgress,  ///< Connection close in progress
  RhiSessionStateMax
} ARM_CCA_RHI_SESSION_STATE;

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
  );

/**
  Perform RHI Host Session protocol feature discovery.

  @param[out] AbiSupportBitmap      A bitmap of supported ABI calls.
  @param[out] ConnectionModeBitmap  A bitmap of supported Connection modes.

  @retval RETURN_INVALID_PARAMETER   A parameter was invalid.
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionFeatures (
  OUT UINT64  *AbiSupportBitmap,
  OUT UINT64  *ConnectionModeBitmap
  );

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
  @retval RETURN_SUCCESS             Success.
**/
RETURN_STATUS
EFIAPI
ArmCcaRhiSessionOpen (
  IN      UINT64                     ConnectionMode,
  IN OUT  ARM_CCA_RHI_SESSION_ID     *SessionId,
  OUT     ARM_CCA_RHI_SESSION_STATE  *SessionState OPTIONAL
  );

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
  OUT   ARM_CCA_RHI_SESSION_STATE  *SessionState OPTIONAL
  );

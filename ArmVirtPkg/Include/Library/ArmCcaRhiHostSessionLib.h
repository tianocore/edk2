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

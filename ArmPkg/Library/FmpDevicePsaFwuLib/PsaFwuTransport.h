/** @file
  Private transport interface for the PSA firmware update client.

  Copyright (C) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

/**
  Initialize the transport and acquire its communication buffer.

  Call only before ExitBootServices(). Repeated successful acquisition returns
  the same buffer. The transport owns the buffer until reset, including after
  a client initialization failure. Failure clears both outputs. A failed
  sharing cleanup may retain resources that remain accessible to the receiver.

  The client converts its copy of Buffer on virtual-address change. Transport
  submission uses scalar endpoint state and does not access a cached CPU buffer
  pointer. The platform must keep the buffer mapped for runtime access.

  Serialize acquisition and each complete encode/submit/decode transaction.
  State is local to each linked library instance. BufferSize is the usable
  communication capacity, including command headers, not a payload limit.

  @param[out] Buffer      CPU-accessible communication buffer.
  @param[out] BufferSize  Usable buffer capacity in bytes.

  @retval EFI_SUCCESS            The transport is ready.
  @retval EFI_INVALID_PARAMETER  An output pointer or configuration is invalid.
  @retval EFI_UNSUPPORTED        The selected transport is not supported.
  @retval Others                 Endpoint discovery or provisioning failed.
**/
EFI_STATUS
EFIAPI
GetPsaFwuTransportBuffer (
  OUT VOID   **Buffer,
  OUT UINTN  *BufferSize
  );

/**
  Submit a request encoded in the acquired communication buffer.

  May be called at runtime after successful acquisition. The caller supplies
  an encoded request and validates the PSA response.
  The transport does not interpret PSA response status. Requests contain
  EFI_GUID values; the selected instance applies any transport-specific wire
  representation. Re-encode the request before each submission.

  ResponseSize is the reported response length when the transport supplies
  one. For direct requests v2, which do not report a length, it is the buffer
  capacity. It is therefore an accessible bound, not proof of a complete
  response. On failure it is zero and the buffer contents are unspecified.

  @param[in,out] Buffer    Acquired buffer using its current CPU address.
  @param[in]  RequestSize   Encoded request size including its header.
  @param[out] ResponseSize  Accessible response bound in bytes.

  @retval EFI_SUCCESS            The transport transaction completed.
  @retval EFI_NOT_READY          Acquisition has not succeeded.
  @retval EFI_INVALID_PARAMETER  An argument or request size is invalid.
  @retval EFI_DEVICE_ERROR       The RPC reported a transport error.
  @retval EFI_PROTOCOL_ERROR     The reported response exceeds the buffer.
  @retval Others                 The FF-A request failed.
**/
EFI_STATUS
EFIAPI
PsaFwuTransportSendRequest (
  IN OUT VOID  *Buffer,
  IN UINTN     RequestSize,
  OUT UINTN    *ResponseSize
  );

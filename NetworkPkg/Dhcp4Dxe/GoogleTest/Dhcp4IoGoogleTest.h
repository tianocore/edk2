/** @file
  Tests for Dhcp4Io.c.

  Copyright (c) 2026 Loongson Technology Corporation Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

////////////////////////////////////////////////////////////////////////////////
// These are the functions that are being unit tested
////////////////////////////////////////////////////////////////////////////////

#include <Uefi.h>

#define DHCP_HEADER_SIZE  (sizeof (EFI_DHCP4_HEADER) + sizeof (UINT32))

/**
  Remove malformed non-Alert options from the private packet copy.

  Some DHCP servers send optional options with a format that does not
  strictly comply with RFC standards. Alert options consumed by the
  DHCP state machine must remain strictly validated, but malformed
  non-Alert options can be safely removed from the private packet copy
  before strict validation.

  RFC 3396 allows an option to be split across several same-tag
  instances. The merged length is used for validation; if the merged
  value fails, all instances of that tag are removed.

  @param[in, out] Packet  The DHCP packet to sanitize.

**/
VOID
EFIAPI
DhcpRemoveMalformedNonAlertOptions (
  IN OUT EFI_DHCP4_PACKET  *Packet
  );

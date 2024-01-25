/** @file
  This file exposes the internal interfaces which may be unit tested
  for the PxeBcDhcp6Dxe driver.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PXE_BC_DHCP6_GOOGLE_TEST_H_
#define PXE_BC_DHCP6_GOOGLE_TEST_H_

//
// Minimal includes needed to compile
//
#include <Uefi.h>
#include "../PxeBcImpl.h"

/**
  Handle the DHCPv6 offer packet.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

  @retval     EFI_SUCCESS           Handled the DHCPv6 offer packet successfully.
  @retval     EFI_NO_RESPONSE       No response to the following request packet.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resources.
  @retval     EFI_BUFFER_TOO_SMALL  Can't cache the offer pacet.

**/
EFI_STATUS
PxeBcHandleDhcp6Offer (
  IN PXEBC_PRIVATE_DATA  *Private
  );

/**
  Cache the DHCPv6 Server address

  @param[in] Private               The pointer to PXEBC_PRIVATE_DATA.
  @param[in] Cache6                The pointer to PXEBC_DHCP6_PACKET_CACHE.

  @retval    EFI_SUCCESS           Cache the DHCPv6 Server address successfully.
  @retval    EFI_OUT_OF_RESOURCES  Failed to allocate resources.
  @retval    EFI_DEVICE_ERROR      Failed to cache the DHCPv6 Server address.
**/
EFI_STATUS
PxeBcCacheDnsServerAddresses (
  IN PXEBC_PRIVATE_DATA        *Private,
  IN PXEBC_DHCP6_PACKET_CACHE  *Cache6
  );

#endif // PXE_BC_DHCP6_GOOGLE_TEST_H_

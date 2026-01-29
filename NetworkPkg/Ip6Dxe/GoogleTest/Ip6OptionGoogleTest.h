/** @file
  Exposes the functions needed to test the Ip6Option module.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef IP6_OPTION_HEADER_GOOGLE_TEST_H_
#define IP6_OPTION_HEADER_GOOGLE_TEST_H_

#include <Uefi.h>
#include "../Ip6Impl.h"

/**
  Validate the IP6 option format for both the packets we received
  and that we will transmit. It will compute the ICMPv6 error message fields
  if the option is malformatted.

  @param[in]  IpSb              The IP6 service data.
  @param[in]  Packet            The to be validated packet.
  @param[in]  Option            The first byte of the option.
  @param[in]  OptionLen         The length of the whole option.
  @param[in]  Pointer           Identifies the octet offset within
                                the invoking packet where the error was detected.


  @retval TRUE     The option is properly formatted.
  @retval FALSE    The option is malformatted.

**/
BOOLEAN
Ip6IsOptionValid (
  IN IP6_SERVICE  *IpSb,
  IN NET_BUF      *Packet,
  IN UINT8        *Option,
  IN UINT16       OptionLen,
  IN UINT32       Pointer
  );

#endif // __IP6_OPTION_HEADER_GOOGLE_TEST_H__

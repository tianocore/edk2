/** @file
  The header file of Ip6ConfigNv.c.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IP6_CONFIGNV_H_
#define _IP6_CONFIGNV_H_

#include "Ip6NvData.h"
#include "Ip6ConfigImpl.h"

extern UINT8  Ip6ConfigBin[];
extern UINT8  Ip6DxeStrings[];

#define IP6_ETHERNET               L"Ethernet"
#define IP6_EXPERIMENTAL_ETHERNET  L"Experimental Ethernet"
#define IP6_ADDRESS_DELIMITER      L' '
#define IP6_LINK_LOCAL_PREFIX      L"FE80::"

typedef enum {
  Ip6InterfaceTypeEthernet = 1,
  Ip6InterfaceTypeExperimentalEthernet
} IP6_INTERFACE_TYPE;

typedef enum {
  Ip6ConfigNvHostAddress,
  Ip6ConfigNvGatewayAddress,
  Ip6ConfigNvDnsAddress,
  Ip6ConfigNvRouteTable
} IP6_CONFIG_NV_ADDRESS_TYPE;

/**
  Install HII Config Access protocol for network device and allocate resources.

  @param[in, out]  Instance      The IP6_CONFIG_INSTANCE to create a form.

  @retval EFI_SUCCESS            The HII Config Access protocol is installed.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
Ip6ConfigFormInit (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  );

/**
  Uninstall HII Config Access protocol for network device and free resource.

  @param[in, out]  Instance      The IP6_CONFIG_INSTANCE to unload a form.

**/
VOID
Ip6ConfigFormUnload (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  );

#endif

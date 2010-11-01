/** @file
  The header file of Ip6ConfigNv.c.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IP6_CONFIGNV_H_
#define _IP6_CONFIGNV_H_

#include "Ip6NvData.h"
#include "Ip6ConfigImpl.h"

extern UINT8  Ip6ConfigBin[];
extern UINT8  Ip6DxeStrings[];

#define IP6_HII_VENDOR_DEVICE_PATH_GUID \
  { \
    0x13288098, 0xb11f, 0x45b9, { 0xbc, 0x4f, 0x91, 0xb5, 0x4b, 0xa3, 0x39, 0xb9 } \
  }

#define IP6_ETHERNET              L"Ethernet"
#define IP6_EXPERIMENTAL_ETHERNET L"Experimental Ethernet"
#define IP6_ADDRESS_DELIMITER     L' '
#define IP6_LINK_LOCAL_PREFIX     L"FE80::"

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
  IN OUT IP6_CONFIG_INSTANCE     *Instance
  );

/**
  Uninstall HII Config Access protocol for network device and free resource.

  @param[in, out]  Instance      The IP6_CONFIG_INSTANCE to unload a form.

**/
VOID
Ip6ConfigFormUnload (
  IN OUT IP6_CONFIG_INSTANCE     *Instance
  );

#endif

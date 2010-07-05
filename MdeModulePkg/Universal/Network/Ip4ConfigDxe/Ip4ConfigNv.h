/** @file
  The header file of IP4ConfigNv.c

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IP4_CONFIGNV_H_
#define _IP4_CONFIGNV_H_

#include "Ip4Config.h"
#include "Ip4NvData.h"

extern UINT8  Ip4ConfigDxeBin[];
extern UINT8  Ip4ConfigDxeStrings[];

#define NIC_ITEM_CONFIG_SIZE   (sizeof (NIC_IP4_CONFIG_INFO) + (sizeof (EFI_IP4_ROUTE_TABLE) * MAX_IP4_CONFIG_IN_VARIABLE))


/**
  Install HII Config Access protocol for network device and allocate resource.

  @param[in]  Instance            The IP4 Config instance.

  @retval EFI_SUCCESS              The HII Config Access protocol is installed.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
**/
EFI_STATUS
Ip4ConfigDeviceInit (
    IN IP4_CONFIG_INSTANCE                   *Instance
  );

/**
  Uninstall HII Config Access protocol for network device and free resource.

  @param[in]  Instance            The IP4 Config instance.

  @retval EFI_SUCCESS             The HII Config Access protocol is uninstalled.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
Ip4ConfigDeviceUnload (
    IN IP4_CONFIG_INSTANCE                   *Instance
  );

#endif

/** @file
  The header file of IP4Config2Nv.c

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IP4_CONFIG2NV_H_
#define _IP4_CONFIG2NV_H_

#include "Ip4Impl.h"

extern UINT8  Ip4Config2Bin[];
extern UINT8  Ip4DxeStrings[];

#define NIC_ITEM_CONFIG_SIZE  (sizeof (IP4_CONFIG2_INSTANCE) + (sizeof (EFI_IPv4_ADDRESS) * MAX_IP4_CONFIG_DNS))

/**
  Install HII Config Access protocol for network device and allocate resource.

  @param[in, out]  Instance         The IP4 config2 Instance.

  @retval EFI_SUCCESS              The HII Config Access protocol is installed.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
Ip4Config2FormInit (
  IN OUT IP4_CONFIG2_INSTANCE  *Instance
  );

/**
  Uninstall the HII Config Access protocol for network devices and free up the resources.

  @param[in, out]  Instance      The IP4 config2 instance to unload a form.

**/
VOID
Ip4Config2FormUnload (
  IN OUT IP4_CONFIG2_INSTANCE  *Instance
  );

#endif

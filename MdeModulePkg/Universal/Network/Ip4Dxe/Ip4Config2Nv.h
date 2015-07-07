/** @file
  The header file of IP4Config2Nv.c

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IP4_CONFIG2NV_H_
#define _IP4_CONFIG2NV_H_

#include "Ip4Impl.h"

extern UINT8  Ip4Config2Bin[];
extern UINT8  Ip4DxeStrings[];

#define NIC_ITEM_CONFIG_SIZE   (sizeof (IP4_CONFIG2_INSTANCE) + (sizeof (EFI_IPv4_ADDRESS) * MAX_IP4_CONFIG_DNS))

/**
  Install HII Config Access protocol for network device and allocate resource.

  @param[in, out]  Instance         The IP4 config2 Instance.

  @retval EFI_SUCCESS              The HII Config Access protocol is installed.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
  
**/
EFI_STATUS
Ip4Config2FormInit (
  IN OUT IP4_CONFIG2_INSTANCE     *Instance
  );

/**
  Uninstall the HII Config Access protocol for network devices and free up the resources.

  @param[in, out]  Instance      The IP4 config2 instance to unload a form.

**/
VOID
Ip4Config2FormUnload (
  IN OUT IP4_CONFIG2_INSTANCE     *Instance
  );

#endif

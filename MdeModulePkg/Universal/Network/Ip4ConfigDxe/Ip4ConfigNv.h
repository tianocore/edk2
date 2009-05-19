/** @file
  The header file of IP4ConfigNv.c

Copyright (c) 2009, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
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

#define NIC_ITEM_CONFIG_SIZE   sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * MAX_IP4_CONFIG_IN_VARIABLE


///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

/**
  Updates the network configuration form to add/delete an entry for the network
  device specified by the Instance.

  @param[in]  Instance            The IP4 Config instance.
  @param[in]  AddForm             Whether to add or delete a form entry.

  @retval EFI_SUCCESS             The network configuration form is updated.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
Ip4ConfigUpdateForm (
  IN IP4_CONFIG_INSTANCE                   *Instance,
  IN BOOLEAN     AddForm
  );

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

/**
  Initialize the network configuration form, this includes: delete all the network
  device configuration entries, install the form callback protocol and
  allocate the resources used.

  @retval EFI_SUCCESS             The network configuration form is unloaded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
Ip4ConfigFormInit (
    VOID
  );

/**
  Unload the network configuration form, this includes: delete all the network
  device configuration entries, uninstall the form callback protocol and
  free the resources used.

  @retval EFI_SUCCESS             The network configuration form is unloaded.
**/
EFI_STATUS
Ip4ConfigFormUnload (
  VOID
  );

#endif

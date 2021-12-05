/** @file
  The header file of functions for configuring or getting the parameters
  relating to HTTP Boot.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HTTP_BOOT_CONFIG_H_
#define _HTTP_BOOT_CONFIG_H_

#include "HttpBootConfigNVDataStruc.h"

typedef struct _HTTP_BOOT_FORM_CALLBACK_INFO HTTP_BOOT_FORM_CALLBACK_INFO;

extern   UINT8  HttpBootDxeStrings[];
extern   UINT8  HttpBootConfigVfrBin[];

#pragma pack()

#define HTTP_BOOT_FORM_CALLBACK_INFO_SIGNATURE  SIGNATURE_32 ('H', 'B', 'f', 'c')

#define HTTP_BOOT_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS(Callback) \
  CR ( \
  Callback, \
  HTTP_BOOT_FORM_CALLBACK_INFO, \
  ConfigAccess, \
  HTTP_BOOT_FORM_CALLBACK_INFO_SIGNATURE \
  )

struct _HTTP_BOOT_FORM_CALLBACK_INFO {
  UINT32                            Signature;
  BOOLEAN                           Initialized;
  EFI_HANDLE                        ChildHandle;
  EFI_DEVICE_PATH_PROTOCOL          *HiiVendorDevicePath;
  EFI_HII_HANDLE                    RegisteredHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  HTTP_BOOT_CONFIG_IFR_NVDATA       HttpBootNvData;
};

/**
  Initialize the configuration form.

  @param[in]  Private             Pointer to the driver private data.

  @retval EFI_SUCCESS             The configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
HttpBootConfigFormInit (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  );

/**
  Unload the configuration form, this includes: delete all the configuration
  entries, uninstall the form callback protocol, and free the resources used.
  The form will only be unload completely when both IP4 and IP6 stack are stopped.

  @param[in]  Private             Pointer to the driver private data.

  @retval EFI_SUCCESS             The configuration form is unloaded.
  @retval Others                  Failed to unload the form.

**/
EFI_STATUS
HttpBootConfigFormUnload (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  );

#endif

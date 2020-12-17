/** @file
  This library provides help functions for REST EX Protocol.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REST_EX_LIB_H_
#define REST_EX_LIB_H_

#include <Protocol/RestEx.h>

///
/// Library class public functions
///

/**
  This function allows the caller to create child handle for specific
  REST server.

  @param[in]  Image                The image handle used to open service.
  @param[in]  AccessMode           Access mode of REST server.
  @param[in]  ConfigType           Underlying configuration to communicate with REST server.
  @param[in]  ServiceType          REST service type.
  @param[out] ChildInstanceHandle  The handle to receive the create child.

  @retval  EFI_SUCCESS            Can't create the corresponding REST EX child instance.
  @retval  EFI_INVALID_PARAMETERS Any of input parameters is improper.

**/
EFI_STATUS
RestExLibCreateChild (
  IN EFI_HANDLE Image,
  IN EFI_REST_EX_SERVICE_ACCESS_MODE  AccessMode,
  IN EFI_REST_EX_CONFIG_TYPE ConfigType,
  IN EFI_REST_EX_SERVICE_TYPE ServiceType,
  OUT EFI_HANDLE *ChildInstanceHandle
);

#endif

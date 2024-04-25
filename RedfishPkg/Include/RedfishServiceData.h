/** @file
  This header file defines Redfish service and Redfish data structures that
  are used to communicate with Redfish Ex Protocol.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_SERVICE_DATA_H_
#define REDFISH_SERVICE_DATA_H_

#include <Uefi.h>
#include <Protocol/Http.h>

typedef  VOID  *REDFISH_SERVICE;
typedef  VOID  *REDFISH_PAYLOAD;

///
/// REDFISH_REQUEST definition.
///
typedef struct {
  UINTN              HeaderCount;
  EFI_HTTP_HEADER    *Headers;
  CHAR8              *Content;
  CHAR8              *ContentType;
  UINTN              ContentLength;
} REDFISH_REQUEST;

///
/// REDFISH_REQUEST definition.
///
typedef struct {
  EFI_HTTP_STATUS_CODE    *StatusCode;
  UINTN                   HeaderCount;
  EFI_HTTP_HEADER         *Headers;
  REDFISH_PAYLOAD         Payload;
} REDFISH_RESPONSE;

#endif

/** @file
  This file is cloned from DMTF libredfish library tag v1.0.0 and maintained
  by EDKII.

//----------------------------------------------------------------------------
// Copyright Notice:
// Copyright 2017 Distributed Management Task Force, Inc. All rights reserved.
// License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libredfish/LICENSE.md
//----------------------------------------------------------------------------

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LIBREDFISH_REDFISH_PAYLOAD_H_
#define LIBREDFISH_REDFISH_PAYLOAD_H_

#include <Include/Library/RedfishCrtLib.h>
#include <Library/JsonLib.h>
#include <jansson.h>
#include <redfishService.h>
#include <redpath.h>

redfishPayload *
createRedfishPayload (
  json_t          *value,
  redfishService  *service
  );

redfishPayload *
getPayloadByNodeName (
  redfishPayload        *payload,
  const char            *nodeName,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
getPayloadByIndex (
  redfishPayload        *payload,
  size_t                index,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
getPayloadForPath (
  redfishPayload        *payload,
  redPathNode           *redpath,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
getPayloadForPathString (
  redfishPayload        *payload,
  const char            *string,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
patchPayload (
  redfishPayload        *target,
  redfishPayload        *payload,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
postContentToPayload (
  redfishPayload        *target,
  const char            *data,
  size_t                dataSize,
  const char            *contentType,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
postPayload (
  redfishPayload        *target,
  redfishPayload        *payload,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

void
cleanupPayload (
  redfishPayload  *payload
  );

bool
isPayloadCollection (
  redfishPayload  *Payload
  );

size_t
getCollectionSize (
  redfishPayload  *payload
  );

redfishPayload *
getPayloadByIndex (
  redfishPayload        *payload,
  size_t                index,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

#endif

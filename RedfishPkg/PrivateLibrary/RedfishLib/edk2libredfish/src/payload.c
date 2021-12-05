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

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <redfishPayload.h>

static redfishPayload *
getOpResult (
  redfishPayload        *payload,
  const char            *propName,
  const char            *op,
  const char            *value,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

static redfishPayload *
collectionEvalOp (
  redfishPayload        *payload,
  const char            *propName,
  const char            *op,
  const char            *value,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

static redfishPayload *
arrayEvalOp (
  redfishPayload        *payload,
  const char            *propName,
  const char            *op,
  const char            *value,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

static redfishPayload *
createCollection (
  redfishService  *service,
  size_t          count,
  redfishPayload  **payloads
  );

static json_t *
json_object_get_by_index (
  json_t  *json,
  size_t  index
  );

bool
isPayloadCollection (
  redfishPayload  *payload
  )
{
  json_t  *members;
  json_t  *count;

  if (!payload || !json_is_object (payload->json)) {
    return false;
  }

  members = json_object_get (payload->json, "Members");
  count   = json_object_get (payload->json, "Members@odata.count");
  return ((members != NULL) && (count != NULL));
}

size_t
getCollectionSize (
  redfishPayload  *payload
  )
{
  json_t  *members;
  json_t  *count;

  if (!payload || !json_is_object (payload->json)) {
    return 0;
  }

  members = json_object_get (payload->json, "Members");
  count   = json_object_get (payload->json, "Members@odata.count");
  if (!members || !count) {
    return 0;
  }

  return (size_t)json_integer_value (count);
}

bool
isPayloadArray (
  redfishPayload  *payload
  )
{
  if (!payload || !json_is_array (payload->json)) {
    return false;
  }

  return true;
}

char *
payloadToString (
  redfishPayload  *payload,
  bool            prettyPrint
  )
{
  size_t  flags = 0;

  if (!payload) {
    return NULL;
  }

  if (prettyPrint) {
    flags = JSON_INDENT (2);
  }

  return json_dumps (payload->json, flags);
}

redfishPayload *
createRedfishPayload (
  json_t          *value,
  redfishService  *service
  )
{
  redfishPayload  *payload;

  payload = (redfishPayload *)malloc (sizeof (redfishPayload));
  if (payload != NULL) {
    payload->json    = value;
    payload->service = service;
  }

  return payload;
}

redfishPayload *
getPayloadByNodeName (
  redfishPayload        *payload,
  const char            *nodeName,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  json_t      *value;
  json_t      *odataId;
  const char  *uri;

  if (!payload || !nodeName || (StatusCode == NULL)) {
    return NULL;
  }

  *StatusCode = NULL;

  value = json_object_get (payload->json, nodeName);
  if (value == NULL) {
    return NULL;
  }

  json_incref (value);
  if (json_object_size (value) == 1) {
    odataId = json_object_get (value, "@odata.id");
    if (odataId != NULL) {
      json_incref (odataId);
      uri = json_string_value (odataId);
      json_decref (value);
      value = getUriFromService (payload->service, uri, StatusCode);
      json_decref (odataId);
      if ((value == NULL) || (*StatusCode == NULL)) {
        return NULL;
      }
    }
  }

  if ((*StatusCode == NULL) || ((**StatusCode >= HTTP_STATUS_200_OK) && (**StatusCode <= HTTP_STATUS_206_PARTIAL_CONTENT))) {
    if (json_is_string (value)) {
      odataId = json_object ();
      json_object_set (odataId, nodeName, value);
      json_decref (value);
      value = odataId;
    }
  }

  return createRedfishPayload (value, payload->service);
}

redfishPayload *
getPayloadByIndex (
  redfishPayload        *payload,
  size_t                index,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  json_t      *value = NULL;
  json_t      *odataId;
  const char  *uri;
  BOOLEAN     FromServerFlag = FALSE;

  if (!payload || (StatusCode == NULL)) {
    return NULL;
  }

  *StatusCode = NULL;

  if (isPayloadCollection (payload)) {
    redfishPayload  *members = getPayloadByNodeName (payload, "Members", StatusCode);
    if (((*StatusCode == NULL) && (members == NULL)) ||
        ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
    {
      return members;
    }

    if (*StatusCode != NULL) {
      //
      // The Payload (members) are retrived from server.
      //
      FreePool (*StatusCode);
      *StatusCode    = NULL;
      FromServerFlag = TRUE;
    }

    redfishPayload  *ret = getPayloadByIndex (members, index, StatusCode);
    if ((*StatusCode == NULL) && (ret != NULL) && FromServerFlag) {
      //
      // In such a case, the Redfish resource is parsed from the input payload (members) directly.
      // Since the members are retrived from server, we still return HTTP_STATUS_200_OK.
      //
      *StatusCode = AllocateZeroPool (sizeof (EFI_HTTP_STATUS_CODE));
      if (*StatusCode == NULL) {
        ret = NULL;
      } else {
        **StatusCode = HTTP_STATUS_200_OK;
      }
    }

    cleanupPayload (members);
    return ret;
  }

  if (json_is_array (payload->json)) {
    //
    // The valid range for index is from 0 to the return value of json_array_size() minus 1
    //
    value = json_array_get (payload->json, index);
  } else if (json_is_object (payload->json)) {
    value = json_object_get_by_index (payload->json, index);
  }

  if (value == NULL) {
    return NULL;
  }

  json_incref (value);
  if (json_object_size (value) == 1) {
    odataId = json_object_get (value, "@odata.id");
    if (odataId != NULL) {
      uri = json_string_value (odataId);
      json_decref (value);
      value = getUriFromService (payload->service, uri, StatusCode);
      if (value == NULL) {
        return NULL;
      }
    }
  }

  return createRedfishPayload (value, payload->service);
}

redfishPayload *
getPayloadForPath (
  redfishPayload        *payload,
  redPathNode           *redpath,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  redfishPayload  *ret = NULL;
  redfishPayload  *tmp;

  if (!payload || !redpath || (StatusCode == NULL)) {
    return NULL;
  }

  *StatusCode = NULL;
  BOOLEAN  FromServerFlag = FALSE;

  if (redpath->nodeName) {
    ret = getPayloadByNodeName (payload, redpath->nodeName, StatusCode);
    if (((*StatusCode == NULL) && (ret == NULL)) ||
        ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
    {
      //
      // Any error happen, return directly.
      //
      return ret;
    }
  } else if (redpath->isIndex) {
    ASSERT (redpath->index >= 1);
    ret = getPayloadByIndex (payload, redpath->index - 1, StatusCode);
    if (((*StatusCode == NULL) && (ret == NULL)) ||
        ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
    {
      //
      // Any error happen, return directly.
      //
      return ret;
    }
  } else if (redpath->op) {
    ret = getOpResult (payload, redpath->propName, redpath->op, redpath->value, StatusCode);
    if (((*StatusCode == NULL) && (ret == NULL)) ||
        ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
    {
      //
      // Any error happen, return directly.
      //
      return ret;
    }
  } else {
    return NULL;
  }

  if ((redpath->next == NULL) || (ret == NULL)) {
    return ret;
  } else {
    if (*StatusCode != NULL) {
      FreePool (*StatusCode);
      *StatusCode    = NULL;
      FromServerFlag = TRUE;
    }

    tmp = getPayloadForPath (ret, redpath->next, StatusCode);
    if ((*StatusCode == NULL) && (tmp != NULL) && FromServerFlag) {
      //
      // In such a case, the Redfish resource is parsed from the input payload (ret) directly.
      // Since the ret are retrived from server, we still return HTTP_STATUS_200_OK.
      //
      *StatusCode = AllocateZeroPool (sizeof (EFI_HTTP_STATUS_CODE));
      if (*StatusCode == NULL) {
        tmp = NULL;
      } else {
        **StatusCode = HTTP_STATUS_200_OK;
      }
    }

    cleanupPayload (ret);
    return tmp;
  }
}

redfishPayload *
getPayloadForPathString (
  redfishPayload        *payload,
  const char            *string,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  redPathNode     *redpath;
  redfishPayload  *ret;

  if (!string || (StatusCode == NULL)) {
    return NULL;
  }

  *StatusCode = NULL;

  redpath = parseRedPath (string);
  if (redpath == NULL) {
    return NULL;
  }

  ret = getPayloadForPath (payload, redpath, StatusCode);
  cleanupRedPath (redpath);
  return ret;
}

redfishPayload *
patchPayload (
  redfishPayload        *target,
  redfishPayload        *payload,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  json_t  *json;
  char    *content;
  char    *uri;

  if (!target || !payload || (StatusCode == NULL)) {
    return NULL;
  }

  *StatusCode = NULL;

  json = json_object_get (target->json, "@odata.id");
  if (json == NULL) {
    return NULL;
  }

  uri = strdup (json_string_value (json));

  content = json_dumps (payload->json, 0);
  json_decref (json);

  json = patchUriFromService (target->service, uri, content, StatusCode);
  free (uri);
  free (content);
  if (json == NULL) {
    return NULL;
  }

  return createRedfishPayload (json, target->service);
}

redfishPayload *
postContentToPayload (
  redfishPayload        *target,
  const char            *data,
  size_t                dataSize,
  const char            *contentType,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  json_t  *json;
  char    *uri;

  if (!target || !data || (StatusCode == NULL)) {
    return NULL;
  }

  *StatusCode = NULL;

  json = json_object_get (target->json, "@odata.id");
  if (json == NULL) {
    json = json_object_get (target->json, "target");
    if (json == NULL) {
      return NULL;
    }
  }

  uri  = strdup (json_string_value (json));
  json = postUriFromService (target->service, uri, data, dataSize, contentType, StatusCode);
  free (uri);
  if (json == NULL) {
    return NULL;
  }

  return createRedfishPayload (json, target->service);
}

redfishPayload *
postPayload (
  redfishPayload        *target,
  redfishPayload        *payload,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  char            *content;
  redfishPayload  *ret;

  if (!target || !payload || (StatusCode == NULL)) {
    return NULL;
  }

  *StatusCode = NULL;

  if (!json_is_object (payload->json)) {
    return NULL;
  }

  content = payloadToString (payload, false);
  ret     = postContentToPayload (target, content, strlen (content), NULL, StatusCode);
  free (content);
  return ret;
}

void
cleanupPayload (
  redfishPayload  *payload
  )
{
  if (!payload) {
    return;
  }

  json_decref (payload->json);
  // Don't free payload->service, let the caller handle cleaning up the service
  free (payload);
}

static redfishPayload *
getOpResult (
  redfishPayload        *payload,
  const char            *propName,
  const char            *op,
  const char            *value,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  const char      *propStr;
  json_t          *stringProp;
  bool            ret = false;
  redfishPayload  *prop;
  long long       intVal, intPropVal;
  json_type       jsonType;

  if (isPayloadCollection (payload)) {
    return collectionEvalOp (payload, propName, op, value, StatusCode);
  }

  if (isPayloadArray (payload)) {
    return arrayEvalOp (payload, propName, op, value, StatusCode);
  }

  prop = getPayloadByNodeName (payload, propName, StatusCode);
  if (((*StatusCode == NULL) && (prop == NULL)) ||
      ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
  {
    return prop;
  }

  stringProp = prop->json;
  jsonType   = prop->json->type;
  switch (jsonType) {
    case JSON_OBJECT:
      stringProp = json_object_get (prop->json, propName);
    case JSON_STRING:
      if (strcmp (op, "=") == 0) {
        propStr = json_string_value (stringProp);
        if (propStr == NULL) {
          cleanupPayload (prop);
          return NULL;
        }

        ret = (strcmp (propStr, value) == 0);
      } else if (strcmp (op, "~") == 0) {
        propStr = json_string_value (stringProp);
        if (propStr == NULL) {
          cleanupPayload (prop);
          return NULL;
        }

        ret = (strcasecmp (propStr, value) == 0);
      }

      break;
    case JSON_TRUE:
      if (strcmp (op, "=") == 0) {
        ret = (strcmp (value, "true") == 0);
      }

      break;
    case JSON_FALSE:
      if (strcmp (op, "=") == 0) {
        ret = (strcmp (value, "false") == 0);
      }

      break;
    case JSON_INTEGER:
      intPropVal = json_integer_value (prop->json);
      intVal     = strtoll (value, NULL, 0);
      if (strcmp (op, "=") == 0) {
        ret = (intPropVal == intVal);
      } else if (strcmp (op, "<") == 0) {
        ret = (intPropVal < intVal);
      } else if (strcmp (op, ">") == 0) {
        ret = (intPropVal > intVal);
      } else if (strcmp (op, "<=") == 0) {
        ret = (intPropVal <= intVal);
      } else if (strcmp (op, ">=") == 0) {
        ret = (intPropVal >= intVal);
      }

      break;
    default:
      break;
  }

  cleanupPayload (prop);
  if (ret) {
    return payload;
  } else {
    return NULL;
  }
}

static redfishPayload *
collectionEvalOp (
  redfishPayload        *payload,
  const char            *propName,
  const char            *op,
  const char            *value,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  redfishPayload  *ret;
  redfishPayload  *tmp;
  redfishPayload  *members;
  redfishPayload  **valid;
  size_t          validMax;
  size_t          validCount = 0;
  size_t          i;

  validMax = getCollectionSize (payload);
  if (validMax == 0) {
    return NULL;
  }

  valid = (redfishPayload **)calloc (validMax, sizeof (redfishPayload *));
  if (valid == NULL) {
    return NULL;
  }

  /*Technically getPayloadByIndex would do this, but this optimizes things*/
  members = getPayloadByNodeName (payload, "Members", StatusCode);
  if (((*StatusCode == NULL) && (members == NULL)) ||
      ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
  {
    return members;
  }

  for (i = 0; i < validMax; i++) {
    if (*StatusCode != NULL) {
      FreePool (*StatusCode);
      *StatusCode = NULL;
    }

    tmp = getPayloadByIndex (members, i, StatusCode);
    if (((*StatusCode == NULL) && (tmp == NULL)) ||
        ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
    {
      return tmp;
    }

    if (*StatusCode != NULL) {
      FreePool (*StatusCode);
      *StatusCode = NULL;
    }

    valid[validCount] = getOpResult (tmp, propName, op, value, StatusCode);

    /*
    if ((*StatusCode == NULL && valid[validCount] == NULL) ||
        (*StatusCode != NULL && (**StatusCode < HTTP_STATUS_200_OK || **StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))) {
      return valid[validCount];
    }
    */
    if (valid[validCount] != NULL) {
      validCount++;
    } else {
      cleanupPayload (tmp);
    }
  }

  cleanupPayload (members);
  if (validCount == 0) {
    free (valid);
    return NULL;
  }

  if (validCount == 1) {
    ret = valid[0];
    free (valid);
    return ret;
  } else {
    ret = createCollection (payload->service, validCount, valid);
    free (valid);
    return ret;
  }
}

static redfishPayload *
arrayEvalOp (
  redfishPayload        *payload,
  const char            *propName,
  const char            *op,
  const char            *value,
  EFI_HTTP_STATUS_CODE  **StatusCode
  )
{
  redfishPayload  *ret;
  redfishPayload  *tmp;
  redfishPayload  **valid;
  size_t          validMax;
  size_t          validCount = 0;
  size_t          i;

  validMax = json_array_size (payload->json);
  if (validMax == 0) {
    return NULL;
  }

  valid = (redfishPayload **)calloc (validMax, sizeof (redfishPayload *));
  if (valid == NULL) {
    return NULL;
  }

  for (i = 0; i < validMax; i++) {
    if (*StatusCode != NULL) {
      FreePool (*StatusCode);
      *StatusCode = NULL;
    }

    tmp = getPayloadByIndex (payload, i, StatusCode);
    if (((*StatusCode == NULL) && (tmp == NULL)) ||
        ((*StatusCode != NULL) && ((**StatusCode < HTTP_STATUS_200_OK) || (**StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))))
    {
      return tmp;
    }

    if (*StatusCode != NULL) {
      FreePool (*StatusCode);
      *StatusCode = NULL;
    }

    valid[validCount] = getOpResult (tmp, propName, op, value, StatusCode);

    /*
    if ((*StatusCode == NULL && valid[validCount] == NULL) ||
        (*StatusCode != NULL && (**StatusCode < HTTP_STATUS_200_OK || **StatusCode > HTTP_STATUS_206_PARTIAL_CONTENT))) {
      return valid[validCount];
    }
    */

    if (valid[validCount] != NULL) {
      validCount++;
    } else {
      cleanupPayload (tmp);
    }
  }

  if (validCount == 0) {
    free (valid);
    return NULL;
  }

  if (validCount == 1) {
    ret = valid[0];
    free (valid);
    return ret;
  } else {
    ret = createCollection (payload->service, validCount, valid);
    free (valid);
    return ret;
  }
}

static redfishPayload *
createCollection (
  redfishService  *service,
  size_t          count,
  redfishPayload  **payloads
  )
{
  redfishPayload  *ret;
  json_t          *collectionJson = json_object ();
  json_t          *jcount         = json_integer ((json_int_t)count);
  json_t          *members        = json_array ();
  size_t          i;

  if (!collectionJson) {
    return NULL;
  }

  if (!members) {
    json_decref (collectionJson);
    return NULL;
  }

  json_object_set (collectionJson, "Members@odata.count", jcount);
  json_decref (jcount);
  for (i = 0; i < count; i++) {
    json_array_append (members, payloads[i]->json);
    cleanupPayload (payloads[i]);
  }

  json_object_set (collectionJson, "Members", members);
  json_decref (members);

  ret = createRedfishPayload (collectionJson, service);
  return ret;
}

static json_t *
json_object_get_by_index (
  json_t  *json,
  size_t  index
  )
{
  void    *iter;
  size_t  i;

  iter = json_object_iter (json);
  for (i = 0; i < index; i++) {
    iter = json_object_iter_next (json, iter);
    if (iter == NULL) {
      break;
    }
  }

  if (iter == NULL) {
    return NULL;
  }

  return json_object_iter_value (iter);
}

/* vim: set tabstop=4 shiftwidth=4 expandtab: */

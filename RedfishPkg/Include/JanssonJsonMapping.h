/** @file
  This is the wrapper to map funcitons and definitions used in
  native jannson applications to edk2 JsonLib. This avoids the
  modifications on native jannson applications to be built under
  edk2 environment.

 (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LIBREDFISH_JSON_SUPPORT_H_
#define LIBREDFISH_JSON_SUPPORT_H_

#include <Library/JsonLib.h>

typedef EDKII_JSON_VALUE   json_t;
typedef EDKII_JSON_INT_T   json_int_t;
typedef EDKII_JSON_TYPE    json_type;

///
/// JSON type mapping
///
#define JSON_OBJECT    EdkiiJsonTypeObject
#define JSON_ARRAY     EdkiiJsonTypeArray
#define JSON_STRING    EdkiiJsonTypeString
#define JSON_INTEGER   EdkiiJsonTypeInteger
#define JSON_REAL      EdkiiJsonTypeReal
#define JSON_TRUE      EdkiiJsonTypeTrue
#define JSON_FALSE     EdkiiJsonTypeFalse
#define JSON_NULL      EdkiiJsonTypeNull

#define JSON_INDENT(n) EDKII_JSON_INDENT(n)

///
/// JSON function mapping
///
#define json_object_get(JsonObj,key)             JsonObjectGetValue(JsonObj,key)
#define json_is_object(JsonValue)                JsonValueIsObject(JsonValue)
#define json_is_array(JsonValue)                 JsonValueIsArray(JsonValue)
#define json_is_string(JsonValue)                JsonValueIsString(JsonValue)
#define json_integer(JsonValue)                  JsonValueInitNumber(JsonValue)
#define json_object_set(JsonObj,Key,JsonValue)   JsonObjectSetValue(JsonObj,Key,JsonValue)
#define json_object()                            JsonValueInitObject()
#define json_object_size(JsonObject)             JsonObjectSize(JsonObject)
#define json_array_get(JsonArray,Index)          JsonArrayGetValue(JsonArray,Index)
#define json_array_append(JsonArray,JsonValue)   JsonArrayAppendValue(JsonArray,JsonValue)
#define json_dumps(JsonValue,Flags)              JsonDumpString(JsonValue,Flags)
#define json_string_value(JsonValue)             JsonValueGetString(JsonValue)
#define json_array_size(JsonArray)               JsonArrayCount(JsonArray)
#define json_array()                             JsonValueInitArray()
#define json_loadb(Buffer,BufferLen,Flags,Error) JsonLoadBuffer(Buffer,BufferLen,Flags,Error)
#define json_decref(JsonValue)                   JsonDecreaseReference(JsonValue)
#define json_incref(JsonValue)                   JsonIncreaseReference(JsonValue)
#define json_string(AsciiString)                 JsonValueInitAsciiString(AsciiString)
#define json_object_iter(JsonValue)              JsonObjectIterator(JsonValue)
#define json_object_iter_value(Iterator)         JsonObjectIteratorValue(Iterator)
#define json_object_iter_next(JsonValue,Iterator) JsonObjectIteratorNext(JsonValue,Iterator)
#define json_integer_value(JsonValue)            JsonValueGetNumber(JsonValue)
#define json_get_type(JsonValue)                 JsonGetType(JsonValue)

#endif

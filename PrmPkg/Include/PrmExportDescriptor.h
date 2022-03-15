/** @file

  Definitions for the Platform Runtime Mechanism (PRM) export descriptor structures.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_EXPORT_DESCRIPTOR_H_
#define PRM_EXPORT_DESCRIPTOR_H_

#include <Prm.h>

#define PRM_MODULE_EXPORT_DESCRIPTOR_NAME       PrmModuleExportDescriptor
#define PRM_MODULE_EXPORT_DESCRIPTOR_SIGNATURE  SIGNATURE_64 ('P', 'R', 'M', '_', 'M', 'E', 'D', 'T')
#define PRM_MODULE_EXPORT_REVISION              0x0

//
// Platform Runtime Mechanism (PRM) Export Descriptor Structures
//
#pragma pack(push, 1)

typedef struct {
  GUID     PrmHandlerGuid;
  CHAR8    PrmHandlerName[PRM_HANDLER_NAME_MAXIMUM_LENGTH];
} PRM_HANDLER_EXPORT_DESCRIPTOR_STRUCT;

typedef struct {
  UINT64    Signature;
  UINT16    Revision;
  UINT16    NumberPrmHandlers;
  GUID      PlatformGuid;
  GUID      ModuleGuid;
} PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT_HEADER;

typedef struct {
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT_HEADER    Header;
  PRM_HANDLER_EXPORT_DESCRIPTOR_STRUCT          PrmHandlerExportDescriptors[1];
} PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT;

#pragma pack(pop)

#if defined (_MSC_VER)
#define PRM_PACKED_STRUCT(definition) \
  __pragma(pack(push, 1)) typedef struct definition __pragma(pack(pop))
#elif defined (__GNUC__) || defined (__clang__)
#define PRM_PACKED_STRUCT(definition) \
  typedef struct __attribute__((packed)) definition
#endif

/**
  A macro that declares a PRM Handler Export Descriptor for a PRM Handler.

  This macro is intended to be used once per PRM Handler to describe the handler when the
  module description is defined. It should be provided as an argument to PRM_MODULE_EXPORT.

  @param  Guid    The GUID of the PRM Handler being exported.

  @param  Name    The name of the PRM Handler being exported. This string should exactly
                  match the function name.

**/
#define PRM_HANDLER_EXPORT_ENTRY(Guid, Name)  \
      {                                       \
        Guid,                                 \
        PRM_STRING_(Name)                     \
      }                                       \


/**
  A macro that returns the count of the number of variable-length arguments given.

  @param  VariableArgumentList  A variable argument list of elements that will be included
                                in the return value of the list count.
**/
#define VA_ARG_COUNT(...)  (sizeof((PRM_HANDLER_EXPORT_DESCRIPTOR_STRUCT[]){__VA_ARGS__})/sizeof(PRM_HANDLER_EXPORT_DESCRIPTOR_STRUCT))

/**
  A macro that declares the PRM Module Export Descriptor for a PRM Module.

  This macro is intended to be used once in a PRM Module after all of the PRM Handler definitions
  to describe the PRM Handlers being exported in the module.

  @param  PrmHandlerExportEntries   A variable argument list of PRM_HANDLER_EXPORT_ENTRY values.
                                    This list should include all PRM Handlers being exported by
                                    this module.

**/
#define PRM_MODULE_EXPORT(...)                                                                            \
  PRM_PACKED_STRUCT(                                                                                      \
    {                                                                                                     \
      PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT_HEADER  Header;                                                 \
      PRM_HANDLER_EXPORT_DESCRIPTOR_STRUCT        PrmHandlerExportDescriptors[VA_ARG_COUNT(__VA_ARGS__)]; \
    } PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT_                                                                \
  );                                                                                                      \
                                                                                                          \
  PRM_EXPORT_API PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT_ PRM_MODULE_EXPORT_DESCRIPTOR_NAME = {               \
    {                                                                                                     \
      PRM_MODULE_EXPORT_DESCRIPTOR_SIGNATURE,                                                             \
      PRM_MODULE_EXPORT_REVISION,                                                                         \
      VA_ARG_COUNT(__VA_ARGS__),                                                                          \
      EDKII_DSC_PLATFORM_GUID,                                                                            \
      EFI_CALLER_ID_GUID                                                                                  \
    },                                                                                                    \
    { __VA_ARGS__ }                                                                                       \
  }                                                                                                       \

#endif

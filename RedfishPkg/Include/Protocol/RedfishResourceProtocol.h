/** @file                                                                                    // [CODE_FIRST] 13357
  EDKII Redfish Resource Protocol definition.                                                // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  Copyright (C) 2026 Jabil Inc. All rights reserved.<BR>                                     // [CODE_FIRST] 13357
  SPDX-License-Identifier: BSD-2-Clause-Patent                                               // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
// [CODE_FIRST] 13357
#pragma once                                                                                 // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
#include <Uefi.h>                                                                            // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
#define EDKII_REDFISH_RESOURCE_PROTOCOL_GUID                                       /* [CODE_FIRST] 13357 */ \
  {                                                                                /* [CODE_FIRST] 13357 */ \
    0x574527BC, 0xB976, 0x0816, { 0xA1, 0x3B, 0x91, 0xBD, 0x9C, 0x9B, 0xA8, 0x99 } /* [CODE_FIRST] 13357 */ \
  }                                                                                // [CODE_FIRST] 13357
// [CODE_FIRST] 13357
typedef struct _EDKII_REDFISH_RESOURCE_PROTOCOL EDKII_REDFISH_RESOURCE_PROTOCOL;             // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357

/**                                                                                          // [CODE_FIRST] 13357
  Returns the Redfish resource data models published by the platform.                        // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  The GetSupportedResourceTypes() function returns a list of Redfish data models             // [CODE_FIRST] 13357
  the resource is provided by the platform. On success, this function allocates an           // [CODE_FIRST] 13357
  array of pointers and the corresponding ASCII strings. The caller is responsible           // [CODE_FIRST] 13357
  for freeing the returned array and each string in the array. The ASCII string              // [CODE_FIRST] 13357
  returned in the array is the identification of the Redfish data model. To align            // [CODE_FIRST] 13357
  with the Redfish data model defined by DMTF Redfish working group, the ASCII               // [CODE_FIRST] 13357
  indicated in each array string member is the Redfish resource type. For example,           // [CODE_FIRST] 13357
  the string could be "ComputerSystem" if the platform provides the resource for             // [CODE_FIRST] 13357
  Redfish computer system data model.                                                        // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  @param[in]   This                Pointer to the EDKII_REDFISH_RESOURCE_PROTOCOL instance.  // [CODE_FIRST] 13357
  @param[out]  ResourceTypeCount   On output, the number of supported Redfish data models    // [CODE_FIRST] 13357
                                   returned in SupportedResources.                           // [CODE_FIRST] 13357
  @param[out]  SupportedResources  On output, a pointer to an array of ResourceTypeCount     // [CODE_FIRST] 13357
                                   pointers to NULL terminated ASCII strings. Each string    // [CODE_FIRST] 13357
                                   identifies a supported Redfish data model. The caller is  // [CODE_FIRST] 13357
                                   responsible for freeing the returned array and each       // [CODE_FIRST] 13357
                                   returned string.                                          // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  @retval EFI_SUCCESS            The list of supported resource types was returned           // [CODE_FIRST] 13357
                                 successfully.                                               // [CODE_FIRST] 13357
  @retval EFI_INVALID_PARAMETER  This, ResourceTypeCount, or SupportedResources is NULL.     // [CODE_FIRST] 13357
  @retval EFI_OUT_OF_RESOURCES   The memory required of the list of supported resource       // [CODE_FIRST] 13357
                                 types could not be allocated.                               // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
typedef    // [CODE_FIRST] 13357
EFI_STATUS // [CODE_FIRST] 13357
(EFIAPI *EDKII_REDFISH_RESOURCE_GET_SUPPORTED_RESOURCE_TYPES)(
                                                              // [CODE_FIRST] 13357
  IN  EDKII_REDFISH_RESOURCE_PROTOCOL  *This,                                                // [CODE_FIRST] 13357
  OUT UINTN                            *ResourceTypeCount,                                   // [CODE_FIRST] 13357
  OUT CHAR8                            ***SupportedResources                                 // [CODE_FIRST] 13357
  );                                                                                         // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357

/**                                                                                          // [CODE_FIRST] 13357
  Collect the resource data for a specified Redfish resource type provided by the platform.  // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  The GetResource() function collects the Redfish resource identified by                     // [CODE_FIRST] 13357
  ResourceTypeName. This function returns MajorVersion, MinorVersion, and                    // [CODE_FIRST] 13357
  ErrataVersion that identifies the Redfish resource type the platform provides.             // [CODE_FIRST] 13357
  If the resource data is not associated with a version-controlled schema, these             // [CODE_FIRST] 13357
  parameters may be NULL. Only one instance of resource data is stored for each              // [CODE_FIRST] 13357
  ResourceTypeName. The implementation of this function returns the data and data            // [CODE_FIRST] 13357
  size set by the platform implementation through SetResource(). Caller shouldn't            // [CODE_FIRST] 13357
  modify the data or free the memory buffer of the data because the buffer belongs           // [CODE_FIRST] 13357
  to the platform provider that published it.                                                // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  @param[in]   This              Pointer to the EDKII_REDFISH_RESOURCE_PROTOCOL instance.    // [CODE_FIRST] 13357
  @param[in]   ResourceTypeName  NULL-terminated ASCII string identifying the Redfish        // [CODE_FIRST] 13357
                                 schema resource type. The string should be the resource     // [CODE_FIRST] 13357
                                 type of Redfish data model defined by DMTF Redfish          // [CODE_FIRST] 13357
                                 working group. For example, "ComputerSystem".               // [CODE_FIRST] 13357
  @param[out]  MajorVersion      A pointer to receive a NULL-terminated ASCII string         // [CODE_FIRST] 13357
                                 identifying the Redfish schema major version, or NULL if    // [CODE_FIRST] 13357
                                 the schema is not version-controlled. If the caller does    // [CODE_FIRST] 13357
                                 not need this information, it may pass NULL for this        // [CODE_FIRST] 13357
                                 parameter.                                                  // [CODE_FIRST] 13357
  @param[out]  MinorVersion      A pointer to receive a NULL-terminated ASCII string         // [CODE_FIRST] 13357
                                 identifying the Redfish schema minor version, or NULL if    // [CODE_FIRST] 13357
                                 the schema is not version-controlled. If the caller does    // [CODE_FIRST] 13357
                                 not need this information, it may pass NULL for this        // [CODE_FIRST] 13357
                                 parameter.                                                  // [CODE_FIRST] 13357
  @param[out]  ErrataVersion     A pointer to receive a NULL-terminated ASCII string         // [CODE_FIRST] 13357
                                 identifying the Redfish schema errata version, or NULL if   // [CODE_FIRST] 13357
                                 the schema is not version-controlled. If the caller does    // [CODE_FIRST] 13357
                                 not need this information, it may pass NULL for this        // [CODE_FIRST] 13357
                                 parameter.                                                  // [CODE_FIRST] 13357
  @param[out]  DataSize          A pointer to receive the size in bytes of the resource      // [CODE_FIRST] 13357
                                 data pointed to by Data.                                    // [CODE_FIRST] 13357
  @param[out]  Data              A pointer to retrieve a pointer to the resource data        // [CODE_FIRST] 13357
                                 maintained by this protocol. The memory is owned by the     // [CODE_FIRST] 13357
                                 platform provider which called SetResource() earlier. The   // [CODE_FIRST] 13357
                                 caller must NOT free or modify this buffer.                 // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  @retval EFI_SUCCESS            Resource data was returned successfully.                    // [CODE_FIRST] 13357
  @retval EFI_NOT_FOUND          No resource data is available for the specified Redfish     // [CODE_FIRST] 13357
                                 resource type.                                              // [CODE_FIRST] 13357
  @retval EFI_INVALID_PARAMETER  This, ResourceTypeName, DataSize, or Data is NULL.          // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
typedef    // [CODE_FIRST] 13357
EFI_STATUS // [CODE_FIRST] 13357
(EFIAPI *EDKII_REDFISH_RESOURCE_GET_RESOURCE)(
                                              // [CODE_FIRST] 13357
  IN  EDKII_REDFISH_RESOURCE_PROTOCOL  *This,                                                // [CODE_FIRST] 13357
  IN  CHAR8                            *ResourceTypeName,                                    // [CODE_FIRST] 13357
  OUT CHAR8                            **MajorVersion   OPTIONAL,                            // [CODE_FIRST] 13357
  OUT CHAR8                            **MinorVersion   OPTIONAL,                            // [CODE_FIRST] 13357
  OUT CHAR8                            **ErrataVersion  OPTIONAL,                            // [CODE_FIRST] 13357
  OUT UINTN                            *DataSize,                                            // [CODE_FIRST] 13357
  OUT VOID                             **Data                                                // [CODE_FIRST] 13357
  );                                                                                         // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357

/**                                                                                          // [CODE_FIRST] 13357
  Sets the Redfish resource for a specified Redfish resource type.                           // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  The SetResource() function maintains the Redfish resource which is identified by           // [CODE_FIRST] 13357
  ResourceTypeName. If the resource already exists with the specified                        // [CODE_FIRST] 13357
  ResourceTypeName, the existing resource is replaced by the new resource. If                // [CODE_FIRST] 13357
  DataSize is 0, with Data set to NULL, the Redfish resource associated with                 // [CODE_FIRST] 13357
  ResourceTypeName is removed. The memory buffer provided through Data is owned by           // [CODE_FIRST] 13357
  the caller, this protocol interface just maintains the pointer to the data and the         // [CODE_FIRST] 13357
  data size. The implementation of this protocol interface shouldn't modify the data         // [CODE_FIRST] 13357
  or free the memory buffer of the data.                                                     // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  @param[in]  This               Pointer to the EDKII_REDFISH_RESOURCE_PROTOCOL instance.    // [CODE_FIRST] 13357
  @param[in]  ResourceTypeName   NULL-terminated ASCII string identifying the Redfish        // [CODE_FIRST] 13357
                                 schema resource type. The string should be the resource     // [CODE_FIRST] 13357
                                 type of Redfish data model defined by DMTF Redfish          // [CODE_FIRST] 13357
                                 working group.                                              // [CODE_FIRST] 13357
  @param[in]  MajorVersion       NULL-terminated ASCII string identifying the Redfish        // [CODE_FIRST] 13357
                                 schema major version, or NULL if the schema is not          // [CODE_FIRST] 13357
                                 version-controlled.                                         // [CODE_FIRST] 13357
  @param[in]  MinorVersion       NULL-terminated ASCII string identifying the Redfish        // [CODE_FIRST] 13357
                                 schema minor version, or NULL if the schema is not          // [CODE_FIRST] 13357
                                 version-controlled.                                         // [CODE_FIRST] 13357
  @param[in]  ErrataVersion      NULL-terminated ASCII string identifying the Redfish        // [CODE_FIRST] 13357
                                 schema errata version, or NULL if the schema is not         // [CODE_FIRST] 13357
                                 version-controlled.                                         // [CODE_FIRST] 13357
  @param[in]  DataSize           Size in bytes of the resource data pointed to by Data. If   // [CODE_FIRST] 13357
                                 DataSize is 0, the existing resource data for               // [CODE_FIRST] 13357
                                 ResourceTypeName shall be removed.                          // [CODE_FIRST] 13357
  @param[in]  Data               Pointer to the resource data provided by the platform. If   // [CODE_FIRST] 13357
                                 DataSize is 0, Data must be NULL, to remove the existing    // [CODE_FIRST] 13357
                                 Redfish resource.                                           // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
  @retval EFI_SUCCESS            Resource data was set successfully.                         // [CODE_FIRST] 13357
  @retval EFI_NOT_FOUND          Removal was requested but no resource is currently          // [CODE_FIRST] 13357
                                 stored for ResourceTypeName.                                // [CODE_FIRST] 13357
  @retval EFI_INVALID_PARAMETER  This or ResourceTypeName is NULL, DataSize > 0 and Data     // [CODE_FIRST] 13357
                                 is NULL, or DataSize = 0 and Data is not NULL.              // [CODE_FIRST] 13357
  @retval EFI_OUT_OF_RESOURCES   The resource database has reached the maximum allowed       // [CODE_FIRST] 13357
                                 number of entries and no new resource type can be added,    // [CODE_FIRST] 13357
                                 or the memory required for the internal metadata could      // [CODE_FIRST] 13357
                                 not be allocated.                                           // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
typedef    // [CODE_FIRST] 13357
EFI_STATUS // [CODE_FIRST] 13357
(EFIAPI *EDKII_REDFISH_RESOURCE_SET_RESOURCE)(
                                              // [CODE_FIRST] 13357
  IN  EDKII_REDFISH_RESOURCE_PROTOCOL  *This,                                                // [CODE_FIRST] 13357
  IN  CHAR8                            *ResourceTypeName,                                    // [CODE_FIRST] 13357
  IN  CHAR8                            *MajorVersion,                                        // [CODE_FIRST] 13357
  IN  CHAR8                            *MinorVersion,                                        // [CODE_FIRST] 13357
  IN  CHAR8                            *ErrataVersion,                                       // [CODE_FIRST] 13357
  IN  UINTN                            DataSize,                                             // [CODE_FIRST] 13357
  IN  VOID                             *Data                                                 // [CODE_FIRST] 13357
  );                                                                                         // [CODE_FIRST] 13357
                                                                                             // [CODE_FIRST] 13357
struct _EDKII_REDFISH_RESOURCE_PROTOCOL {
  // [CODE_FIRST] 13357
  EDKII_REDFISH_RESOURCE_GET_SUPPORTED_RESOURCE_TYPES    GetSupportedResourceTypes;          // [CODE_FIRST] 13357
  EDKII_REDFISH_RESOURCE_GET_RESOURCE                    GetResource;                        // [CODE_FIRST] 13357
  EDKII_REDFISH_RESOURCE_SET_RESOURCE                    SetResource;                        // [CODE_FIRST] 13357
};                                                                                           // [CODE_FIRST] 13357

// [CODE_FIRST] 13357
extern EFI_GUID  gEdkIIRedfishResourceProtocolGuid;                                          // [CODE_FIRST] 13357

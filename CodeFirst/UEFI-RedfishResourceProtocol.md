# Title: EFI Redfish Resource Protocol

## Status: Draft

## Document: UEFI Specification Version 2.11

## License

SPDX-License-Identifier: CC-BY-4.0

## Submitter: [TianoCore Community](https://www.tianocore.org)

## Summary of the change

Adds `EFI_REDFISH_RESOURCE_PROTOCOL` (new section 31.3). The protocol provides a unified interface for provisioning and collecting platform Redfish resource, and sits as a high-level abstract layer over industry-standard platform management data such as SMBIOS, Device Tree, ACPI, or platform-proprietary formats.

- **Provider** (platform implementation): collects Redfish resource from platform data sources (such as PCIe, SMBIOS, ACPI, NVMe, SATA, or a platform-defined format) and calls `SetResource()` to publish the data.
- **Consumer** (EFI Redfish client): retrieves the resource for a specific Redfish data model with `GetResource()` and builds the Redfish resource according to the schema defined by the DMTF Redfish working group.

## Benefits of the change

- Unifies and centralizes the interfaces used by Redfish firmware implementations for resource provisioning.
- The data format is left to the firmware implementation, which keeps the protocol simple and flexible and avoids frequent UEFI specification updates.

## Impact of the change

- This is an additive change. No existing protocol GUID, structure, or calling convention changes.

## Detailed description of the change [normative updates]

### 31.2 EFI Redfish JSON Structure Converter

#### 31.2.1 The Guidance of Writing EFI Redfish JSON Structure Converter

**Datatype:** "ComputerSystemCollection"

- Determine Redfish resource type according to the given JsonRsrcIdentifier. If the given JsonRsrcIdentifier is non-NULL, the Redfish resource structure converter must convert the JSON resource to the Redfish JSON structure according to the resource type and revision specified in JsonRsrcIdentifier. The converter should not refer to the resource type and revision according to Redfish namespace and datatype indicated in "odata.type" in JSON text resource. This prevents from the returned structure format is different with what consumer expects.

- Automatically determine the Redfish resource type. If the given JsonRsrcIdentifier is NULL, the EFI Redfish JSON structure converter should check the namespace and datatype indicated in "odata.type" in the JSON text resource. Parse this identifier property to retrieve the corresponding Redfish schema name space and data type, then decode the JSON text resource into the corresponding structure. EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER in JsonStructure returned to consumer should be filled with the correct Redfish schema resource type information following the guidance mentioned above.

- All structure members for Redfish schema must be declared as C pointers. With this, the converter consumer can get the partial Redfish JSON properties from the converter. The consumer just initializes certain structure members, and the converter producer only converts non-NULL pointers in the given structure into corresponding Redfish JSON properties in text format. The non-NULL pointers mean either caller or callee has allocated the memory block for it. The memory block pointed by the member must be freed accordingly.


### 31.3 EFI Redfish Resource Protocol

#### 31.3.1 Overview

The EFI Redfish Resource Protocol provides a unified interface for provisioning and collecting platform Redfish resource, such as the computer system, PCIe, memory, storage, etc. The Redfish resource provided by the platform through this protocol could be retrieved by the EFI Redfish client and provisioned to the Redfish service. EFI Redfish Resource Protocol is designed as a high level and abstract layer on top of the industry standard platform management specifications, such as SMBIOS, Device Tree, ACPI or the platform proprietary formats. This protocol unifies and centralizes the interfaces for the Redfish firmware implementation for the Redfish resource provisioning. The architecture consists of the provider and the consumer. On the provider side, the platform implementation is responsible for collecting Redfish resource from platform data sources (such as PCIe, SMBIOS, ACPI, NVMe, SATA, or platform-defined format) and invoking `SetResource()` to publish the data to this protocol. The implementation of this protocol must maintain the data provided by the platform in the database for the consumer to retrieve the desired Redfish resource type. On the consumer side, the EFI Redfish client collects the Redfish resource of specific Redfish data model and constructs it to the Redfish resource according to Redfish schema defined by DMTF Redfish working group. The above behavior between the provider and consumer can be governed via `GetSupportedResourceTypes()`, `GetResource()`, and `SetResource()` functions published by EFI Redfish Resource Protocol driver.

(The ECR contains a figure in this section. It was not read; see the notes at the end. [TBD])

#### 31.3.2 EFI_REDFISH_RESOURCE_PROTOCOL

**Summary**

The EFI Redfish Resource Protocol provides the services to EFI Redfish client for querying the supported Redfish resource on the platform, collecting the Redfish resource from other data source, and set the Redfish resource for a specified Redfish data model through the interface. Only one instance of this protocol is expected to be published on a UEFI system.

**Protocol GUID**

```c
#define EFI_REDFISH_RESOURCE_PROTOCOL_GUID \
  { 0x574527BC, 0xB976, 0x0816, \
    { 0xA1, 0x3B, 0x91, 0xBD, 0x9C, 0x9B, 0xA8, 0x99 } }
```

**Protocol Interface Structure**

```c
typedef struct _EFI_REDFISH_RESOURCE_PROTOCOL {
  EFI_REDFISH_RESOURCE_GET_SUPPORTED_RESOURCE_TYPES  GetSupportedResourceTypes;
  EFI_REDFISH_RESOURCE_GET_RESOURCE                  GetResource;
  EFI_REDFISH_RESOURCE_SET_RESOURCE                  SetResource;
} EFI_REDFISH_RESOURCE_PROTOCOL;
```

**Parameters**

`GetSupportedResourceTypes`
: Returns the types of Redfish resource provided by the platform. See the `GetSupportedResourceTypes()` function description.

`GetResource`
: Collect the Redfish resource for a specified Redfish data model. See the `GetResource()` function description.

`SetResource`
: Sets the Redfish resource for a specified Redfish data model. See the `SetResource()` function description.

#### 31.3.3 EFI_REDFISH_RESOURCE_PROTOCOL.GetSupportedResourceTypes()

**Summary**

Returns the Redfish resource data models published by the platform.

**Prototype**

```c
typedef
EFI_STATUS
(EFIAPI *EFI_REDFISH_RESOURCE_GET_SUPPORTED_RESOURCE_TYPES)(
  IN  EFI_REDFISH_RESOURCE_PROTOCOL  *This,
  OUT UINTN                          *ResourceTypeCount,
  OUT CHAR8                          ***SupportedResources
  );
```

**Parameters**

`This`
: Pointer to the `EFI_REDFISH_RESOURCE_PROTOCOL` instance.

`ResourceTypeCount`
: On output, the number of supported Redfish data models returned in `SupportedResources`.

`SupportedResources`
: On output, a pointer to an array of `ResourceTypeCount` pointers to NULL terminated ASCII strings. Each string identifies a supported Redfish data model. The caller is responsible for freeing the returned array and each returned string.

**Description**

The `GetSupportedResourceTypes()` function returns a list of Redfish data models the resource is provided by the platform. On success, this function allocates an array of pointers and the corresponding ASCII strings. The caller is responsible for freeing the returned array and each string in the array. The ASCII string returned in the array is the identification of the Redfish data model. To align with the Redfish data model defined by DMTF Redfish working group, the ASCII indicated in each array string member is the Redfish resource type. For example, the string could be "ComputerSystem" if the platform provides the resource for Redfish computer system data model.

**Status Codes Returned**

| Status | Description |
|---|---|
| `EFI_SUCCESS` | The list of supported resource types was returned successfully. |
| `EFI_INVALID_PARAMETER` | `This`, `ResourceTypeCount`, or `SupportedResources` is NULL. |
| `EFI_OUT_OF_RESOURCES` | The memory required of the list of supported resource types could not be allocated. |

#### 31.3.4 EFI_REDFISH_RESOURCE_PROTOCOL.GetResource()

**Summary**

Collect the resource data for a specified Redfish resource type provided by the platform. This function is designed for the EFI Redfish client implementation to collect the platform defined Redfish resource data and combine it with the default implementation, then provision the resource to Redfish service.

**Prototype**

```c
typedef
EFI_STATUS
(EFIAPI *EFI_REDFISH_RESOURCE_GET_RESOURCE)(
  IN  EFI_REDFISH_RESOURCE_PROTOCOL  *This,
  IN  CHAR8                          *ResourceTypeName,
  OUT CHAR8                          **MajorVersion   OPTIONAL,
  OUT CHAR8                          **MinorVersion   OPTIONAL,
  OUT CHAR8                          **ErrataVersion  OPTIONAL,
  OUT UINTN                          *DataSize,
  OUT VOID                           **Data
  );
```

**Parameters**

`This`
: Pointer to the `EFI_REDFISH_RESOURCE_PROTOCOL` instance.

`ResourceTypeName`
: NULL-terminated ASCII string identifying the Redfish schema resource type. The string should be the resource type of Redfish data model defined by DMTF Redfish working group. For example, "ComputerSystem".

`MajorVersion`
: A pointer to receive a NULL-terminated ASCII string identifying the Redfish schema major version, or NULL if the schema is not version-controlled. If the caller does not need this information, it may pass NULL for this parameter.

`MinorVersion`
: A pointer to receive a NULL-terminated ASCII string identifying the Redfish schema minor version, or NULL if the schema is not version-controlled. If the caller does not need this information, it may pass NULL for this parameter.

`ErrataVersion`
: A pointer to receive a NULL-terminated ASCII string identifying the Redfish schema errata version, or NULL if the schema is not version-controlled. If the caller does not need this information, it may pass NULL for this parameter.

`DataSize`
: A pointer to receive the size in bytes of the resource data pointed to by `Data`.

`Data`
: A pointer to retrieve a pointer to the resource data maintained by this protocol. The memory is owned by the platform provider which called `SetResource()` earlier. The caller must NOT free or modify this buffer. See the related definitions for the format of data.

**Description**

The `GetResource()` function collects the Redfish resource identified by `ResourceTypeName`. This function returns `MajorVersion`, `MinorVersion`, and `ErrataVersion` that identifies the Redfish resource type the platform provides. If the resource data is not associated with a version-controlled schema, these parameters may be NULL. Only one instance of resource data is stored for each `ResourceTypeName`. The implementation of this function returns the data and data size set by the platform implementation through `SetResource()`. Caller shouldn't modify the data or free the memory buffer of the data because the buffer belongs to the platform provider that published it.

**Related Definitions**

*Format of the Redfish Resource*

The format of the Redfish resource returned in `Data` is not specified by this specification. To keep the `EFI_REDFISH_RESOURCE_PROTOCOL` simple and flexible, also not updating UEFI spec frequently; an explicit definition of the data format is at UEFI firmware system discretion. The recommendation is to use the C header files generated by DMTF tool based on the Redfish schema. The implementation can leverage the Redfish resource C header file generated by [Redfish-Schema-C-Struct-Generator](https://github.com/DMTF/Redfish-Schema-C-Struct-Generator) as the format of `Data`.

**Status Codes Returned**

| Status | Description |
|---|---|
| `EFI_SUCCESS` | Resource data was returned successfully. |
| `EFI_NOT_FOUND` | No resource data is available for the specified Redfish resource type. |
| `EFI_INVALID_PARAMETER` | `This`, `ResourceTypeName`, `DataSize`, or `Data` is NULL. |

#### 31.3.5 EFI_REDFISH_RESOURCE_PROTOCOL.SetResource()

**Summary**

Sets the Redfish resource for a specified Redfish resource type. The user of this function is usually the platform Redfish resource provider. The given `ResourceTypeName`, `MajorVersion`, `MinorVersion`, `ErrataVersion` and `Data` is maintained by this protocol and consumed by the EFI Redfish client. Only one instance of the resource type can be installed on this protocol. The later installed Redfish resource overrides the existing one. Unless `DataSize` is specified as zero, which means to remove the existing record from this protocol. That is at EFI Redfish resource provider's discretion to determine proper timing to remove the records from the database and release the resource according to the platform design.

**Prototype**

```c
typedef
EFI_STATUS
(EFIAPI *EFI_REDFISH_RESOURCE_SET_RESOURCE)(
  IN EFI_REDFISH_RESOURCE_PROTOCOL  *This,
  IN CHAR8                          *ResourceTypeName,
  IN CHAR8                          *MajorVersion,
  IN CHAR8                          *MinorVersion,
  IN CHAR8                          *ErrataVersion,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  );
```

**Parameters**

`This`
: Pointer to the `EFI_REDFISH_RESOURCE_PROTOCOL` instance.

`ResourceTypeName`
: NULL-terminated ASCII string identifying the Redfish schema resource type. The string should be the resource type of Redfish data model defined by DMTF Redfish working group.

`MajorVersion`
: NULL-terminated ASCII string identifying the Redfish schema major version, or NULL if the schema is not version-controlled.

`MinorVersion`
: NULL-terminated ASCII string identifying the Redfish schema minor version, or NULL if the schema is not version-controlled.

`ErrataVersion`
: NULL-terminated ASCII string identifying the Redfish schema errata version, or NULL if the schema is not version-controlled.

`DataSize`
: Size in bytes of the resource data pointed to by `Data`. If `DataSize` is 0, the existing resource data for `ResourceTypeName` shall be removed.

`Data`
: Pointer to the resource data provided by the platform. If `DataSize` is 0, `Data` must be NULL, to remove the existing Redfish resource.

**Description**

The `SetResource()` function maintains the Redfish resource which is identified by `ResourceTypeName`. If the resource already exists with the specified `ResourceTypeName`, the existing resource is replaced by the new resource. If `DataSize` is 0, with `Data` set to NULL, the Redfish resource associated with `ResourceTypeName` is removed. The memory buffer provided through `Data` is owned by the caller, this protocol interface just maintains the pointer to the data and the data size. The implementation of this protocol interface shouldn't modify the data or free the memory buffer of the data.

**Status Codes Returned**

| Status | Description |
|---|---|
| `EFI_SUCCESS` | Resource data was set successfully. |
| `EFI_NOT_FOUND` | Removal was requested but no resource is currently stored for `ResourceTypeName`. |
| `EFI_INVALID_PARAMETER` | `This` or `ResourceTypeName` is NULL, `DataSize` > 0 and `Data` is NULL, or `DataSize` = 0 and `Data` is not NULL. |
| `EFI_OUT_OF_RESOURCES` | The resource database has reached the maximum allowed number of entries and no new resource type can be added, or the memory required for the internal metadata could not be allocated. |

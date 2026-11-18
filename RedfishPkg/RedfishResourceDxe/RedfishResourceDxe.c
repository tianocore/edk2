/** @file                                                                                      // [CODE_FIRST] 13357
  RedfishResourceDxe installs EDKII_REDFISH_RESOURCE_PROTOCOL.                                 // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  This driver provides a data repository for platform Redfish resource.                        // [CODE_FIRST] 13357
  Platform porting code calls SetResource() to publish data; Redfish feature                   // [CODE_FIRST] 13357
  drivers call GetResource() to retrieve it.                                                   // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  The protocol is type-agnostic: it stores opaque VOID* blobs keyed by                         // [CODE_FIRST] 13357
  ResourceTypeName. The protocol does NOT own the Data buffer - caller                         // [CODE_FIRST] 13357
  retains ownership and must keep the buffer valid throughout the DXE phase.                   // [CODE_FIRST] 13357
  GetResource() returns a direct pointer to the caller-owned data - consumer                   // [CODE_FIRST] 13357
  must NOT free or modify it.                                                                  // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  Copyright (C) 2026 Jabil Inc. All rights reserved.<BR>                                       // [CODE_FIRST] 13357
  SPDX-License-Identifier: BSD-2-Clause-Patent                                                 // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
// [CODE_FIRST] 13357
#include <Uefi.h>                                                                              // [CODE_FIRST] 13357
#include <Library/UefiBootServicesTableLib.h>                                                  // [CODE_FIRST] 13357
#include <Library/MemoryAllocationLib.h>                                                       // [CODE_FIRST] 13357
#include <Library/BaseMemoryLib.h>                                                             // [CODE_FIRST] 13357
#include <Library/BaseLib.h>                                                                   // [CODE_FIRST] 13357
#include <Library/DebugLib.h>                                                                  // [CODE_FIRST] 13357
#include <Library/PcdLib.h>                                                                    // [CODE_FIRST] 13357
#include <Protocol/RedfishResourceProtocol.h>                                                  // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
#include "RedfishResourceDxeInternal.h"                                                        // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
STATIC_ASSERT (
  // [CODE_FIRST] 13357
  REDFISH_RESOURCE_MAX_ENTRIES > 0,                                                            // [CODE_FIRST] 13357
  "PcdRedfishResourceMaxEntries must be greater than zero"                                     // [CODE_FIRST] 13357
  );                                                                                           // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
STATIC_ASSERT (
  // [CODE_FIRST] 13357
  REDFISH_RESOURCE_MAX_ENTRIES <= (MAX_UINTN / sizeof (RESOURCE_DATA_ENTRY)),                  // [CODE_FIRST] 13357
  "PcdRedfishResourceMaxEntries would overflow the resource entry array size"                  // [CODE_FIRST] 13357
  );                                                                                           // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
STATIC REDFISH_RESOURCE_PRIVATE  mPrivate;                                                     // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357

/**                                                                                            // [CODE_FIRST] 13357
  Find an existing resource entry by its type name.                                            // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  Performs a linear search over the active entries and returns the first                       // [CODE_FIRST] 13357
  entry whose TypeName matches.                                                                // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @param[in]  TypeName  NULL-terminated ASCII resource type name.                              // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @return  The matching entry, or NULL when no entry holds this type name.                     // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
STATIC                                                                                         // [CODE_FIRST] 13357
RESOURCE_DATA_ENTRY *
// [CODE_FIRST] 13357
FindEntry (
  // [CODE_FIRST] 13357
  IN CHAR8  *TypeName                                                                          // [CODE_FIRST] 13357
  )                                                                                            // [CODE_FIRST] 13357
{
  // [CODE_FIRST] 13357
  UINTN  Index;                                                                                // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (TypeName == NULL) {
    // [CODE_FIRST] 13357
    return NULL;                                                                               // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  for (Index = 0; Index < mPrivate.EntryCount; Index++) {
    // [CODE_FIRST] 13357
    if ((mPrivate.Entries[Index].TypeName != NULL) &&                                          // [CODE_FIRST] 13357
        (AsciiStrCmp (mPrivate.Entries[Index].TypeName, TypeName) == 0))                       // [CODE_FIRST] 13357
    {
      // [CODE_FIRST] 13357
      return &mPrivate.Entries[Index];                                                         // [CODE_FIRST] 13357
    }                                                                                          // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  return NULL;                                                                                 // [CODE_FIRST] 13357
}                                                                                              // [CODE_FIRST] 13357

// [CODE_FIRST] 13357

/**                                                                                            // [CODE_FIRST] 13357
  Free the schema version strings owned by an entry and reset them to NULL.                    // [CODE_FIRST] 13357
  Does nothing if Entry is NULL.                                                               // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @param[in,out]  Entry  Entry whose version strings are freed.                                // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
STATIC                                                                                         // [CODE_FIRST] 13357
VOID
// [CODE_FIRST] 13357
FreeEntryVersions (
  // [CODE_FIRST] 13357
  IN OUT RESOURCE_DATA_ENTRY  *Entry                                                           // [CODE_FIRST] 13357
  )                                                                                            // [CODE_FIRST] 13357
{
  // [CODE_FIRST] 13357
  if (Entry == NULL) {
    // [CODE_FIRST] 13357
    return;                                                                                    // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (Entry->MajorVersion != NULL) {
    // [CODE_FIRST] 13357
    FreePool (Entry->MajorVersion);                                                            // [CODE_FIRST] 13357
    Entry->MajorVersion = NULL;                                                                // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (Entry->MinorVersion != NULL) {
    // [CODE_FIRST] 13357
    FreePool (Entry->MinorVersion);                                                            // [CODE_FIRST] 13357
    Entry->MinorVersion = NULL;                                                                // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (Entry->ErrataVersion != NULL) {
    // [CODE_FIRST] 13357
    FreePool (Entry->ErrataVersion);                                                           // [CODE_FIRST] 13357
    Entry->ErrataVersion = NULL;                                                               // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357
}                                                                                              // [CODE_FIRST] 13357

// [CODE_FIRST] 13357

/**                                                                                            // [CODE_FIRST] 13357
  Make a private copy of an ASCII string so the driver owns the memory.                        // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  If Str is NULL, *Dst is set to NULL and EFI_SUCCESS is returned.                             // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @param[in]   Str  ASCII string to copy, or NULL.                                             // [CODE_FIRST] 13357
  @param[out]  Dst  Receives the allocated copy (NULL if Str is NULL). Must not be NULL.       // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @retval  EFI_SUCCESS           Copied successfully, or Str was NULL.                         // [CODE_FIRST] 13357
  @retval  EFI_OUT_OF_RESOURCES  Allocation failed.                                            // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
STATIC                                                                                         // [CODE_FIRST] 13357
EFI_STATUS
// [CODE_FIRST] 13357
DupAsciiString (
  // [CODE_FIRST] 13357
  IN CHAR8   *Str  OPTIONAL,                                                                   // [CODE_FIRST] 13357
  OUT CHAR8  **Dst                                                                             // [CODE_FIRST] 13357
  )                                                                                            // [CODE_FIRST] 13357
{
  // [CODE_FIRST] 13357
  if (Str == NULL) {
    // [CODE_FIRST] 13357
    *Dst = NULL;                                                                               // [CODE_FIRST] 13357
    return EFI_SUCCESS;                                                                        // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  *Dst = AllocateCopyPool (AsciiStrSize (Str), Str);                                           // [CODE_FIRST] 13357
  return (*Dst != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;                                  // [CODE_FIRST] 13357
}                                                                                              // [CODE_FIRST] 13357

// [CODE_FIRST] 13357

/**                                                                                            // [CODE_FIRST] 13357
  Returns the Redfish resource data models published by the platform.                          // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  The GetSupportedResourceTypes() function returns a list of Redfish data models               // [CODE_FIRST] 13357
  the resource is provided by the platform. On success, this function allocates an             // [CODE_FIRST] 13357
  array of pointers and the corresponding ASCII strings. The caller is responsible             // [CODE_FIRST] 13357
  for freeing the returned array and each string in the array. The ASCII string                // [CODE_FIRST] 13357
  returned in the array is the identification of the Redfish data model. To align              // [CODE_FIRST] 13357
  with the Redfish data model defined by DMTF Redfish working group, the ASCII                 // [CODE_FIRST] 13357
  indicated in each array string member is the Redfish resource type. For example,             // [CODE_FIRST] 13357
  the string could be "ComputerSystem" if the platform provides the resource for               // [CODE_FIRST] 13357
  Redfish computer system data model.                                                          // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @param[in]   This                Pointer to the EDKII_REDFISH_RESOURCE_PROTOCOL instance.    // [CODE_FIRST] 13357
  @param[out]  ResourceTypeCount   On output, the number of supported Redfish data models      // [CODE_FIRST] 13357
                                   returned in SupportedResources.                             // [CODE_FIRST] 13357
  @param[out]  SupportedResources  On output, a pointer to an array of ResourceTypeCount       // [CODE_FIRST] 13357
                                   pointers to NULL terminated ASCII strings. Each string      // [CODE_FIRST] 13357
                                   identifies a supported Redfish data model. The caller is    // [CODE_FIRST] 13357
                                   responsible for freeing the returned array and each         // [CODE_FIRST] 13357
                                   returned string.                                            // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @retval EFI_SUCCESS            The list of supported resource types was returned             // [CODE_FIRST] 13357
                                 successfully.                                                 // [CODE_FIRST] 13357
  @retval EFI_INVALID_PARAMETER  This, ResourceTypeCount, or SupportedResources is NULL.       // [CODE_FIRST] 13357
  @retval EFI_OUT_OF_RESOURCES   The memory required of the list of supported resource         // [CODE_FIRST] 13357
                                 types could not be allocated.                                 // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
STATIC                                                                                         // [CODE_FIRST] 13357
EFI_STATUS                                                                                     // [CODE_FIRST] 13357
EFIAPI
// [CODE_FIRST] 13357
RedfishResourceGetSupportedResourceTypes (
  // [CODE_FIRST] 13357
  IN  EDKII_REDFISH_RESOURCE_PROTOCOL  *This,                                                  // [CODE_FIRST] 13357
  OUT UINTN                            *ResourceTypeCount,                                     // [CODE_FIRST] 13357
  OUT CHAR8                            ***SupportedResources                                   // [CODE_FIRST] 13357
  )                                                                                            // [CODE_FIRST] 13357
{
  // [CODE_FIRST] 13357
  UINTN  Index;                                                                                // [CODE_FIRST] 13357
  CHAR8  **TypeArray;                                                                          // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if ((This == NULL) || (ResourceTypeCount == NULL) || (SupportedResources == NULL)) {
    // [CODE_FIRST] 13357
    return EFI_INVALID_PARAMETER;                                                              // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Keep the two outputs consistent on every path: a non-zero count is only                   // [CODE_FIRST] 13357
  // published together with the array it describes.                                           // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  *ResourceTypeCount  = 0;                                                                     // [CODE_FIRST] 13357
  *SupportedResources = NULL;                                                                  // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  if (mPrivate.EntryCount == 0) {
    // [CODE_FIRST] 13357
    return EFI_SUCCESS;                                                                        // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  TypeArray = AllocateZeroPool (sizeof (CHAR8 *) * mPrivate.EntryCount);                       // [CODE_FIRST] 13357
  if (TypeArray == NULL) {
    // [CODE_FIRST] 13357
    return EFI_OUT_OF_RESOURCES;                                                               // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  for (Index = 0; Index < mPrivate.EntryCount; Index++) {
    // [CODE_FIRST] 13357
    TypeArray[Index] = AllocateCopyPool (
                                                                          // [CODE_FIRST] 13357
                         AsciiStrSize (mPrivate.Entries[Index].TypeName), // [CODE_FIRST] 13357
                         mPrivate.Entries[Index].TypeName                 // [CODE_FIRST] 13357
                         );                                               // [CODE_FIRST] 13357
    //                                                                                         // [CODE_FIRST] 13357
    // If any allocation fails, free the strings already copied and the array                  // [CODE_FIRST] 13357
    // itself, then return EFI_OUT_OF_RESOURCES. No partial array is left.                     // [CODE_FIRST] 13357
    //                                                                                         // [CODE_FIRST] 13357
    if (TypeArray[Index] == NULL) {
      // [CODE_FIRST] 13357
      while (Index > 0) {
        // [CODE_FIRST] 13357
        FreePool (TypeArray[--Index]);                                                         // [CODE_FIRST] 13357
      }                                                                                        // [CODE_FIRST] 13357

      // [CODE_FIRST] 13357
      FreePool (TypeArray);                                                                    // [CODE_FIRST] 13357
      return EFI_OUT_OF_RESOURCES;                                                             // [CODE_FIRST] 13357
    }                                                                                          // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  *ResourceTypeCount  = mPrivate.EntryCount;                                                   // [CODE_FIRST] 13357
  *SupportedResources = TypeArray;                                                             // [CODE_FIRST] 13357
  return EFI_SUCCESS;                                                                          // [CODE_FIRST] 13357
}                                                                                              // [CODE_FIRST] 13357

// [CODE_FIRST] 13357

/**                                                                                            // [CODE_FIRST] 13357
  Collect the resource data for a specified Redfish resource type provided by the platform.    // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  The GetResource() function collects the Redfish resource identified by                       // [CODE_FIRST] 13357
  ResourceTypeName. This function returns MajorVersion, MinorVersion, and                      // [CODE_FIRST] 13357
  ErrataVersion that identifies the Redfish resource type the platform provides.               // [CODE_FIRST] 13357
  If the resource data is not associated with a version-controlled schema, these               // [CODE_FIRST] 13357
  parameters may be NULL. Only one instance of resource data is stored for each                // [CODE_FIRST] 13357
  ResourceTypeName. The implementation of this function returns the data and data              // [CODE_FIRST] 13357
  size set by the platform implementation through SetResource(). Caller shouldn't              // [CODE_FIRST] 13357
  modify the data or free the memory buffer of the data because the buffer belongs             // [CODE_FIRST] 13357
  to the platform provider that published it.                                                  // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @param[in]   This              Pointer to the EDKII_REDFISH_RESOURCE_PROTOCOL instance.      // [CODE_FIRST] 13357
  @param[in]   ResourceTypeName  NULL-terminated ASCII string identifying the Redfish          // [CODE_FIRST] 13357
                                 schema resource type. The string should be the resource       // [CODE_FIRST] 13357
                                 type of Redfish data model defined by DMTF Redfish            // [CODE_FIRST] 13357
                                 working group. For example, "ComputerSystem".                 // [CODE_FIRST] 13357
  @param[out]  MajorVersion      A pointer to receive a NULL-terminated ASCII string           // [CODE_FIRST] 13357
                                 identifying the Redfish schema major version, or NULL if      // [CODE_FIRST] 13357
                                 the schema is not version-controlled. If the caller does      // [CODE_FIRST] 13357
                                 not need this information, it may pass NULL for this          // [CODE_FIRST] 13357
                                 parameter.                                                    // [CODE_FIRST] 13357
  @param[out]  MinorVersion      A pointer to receive a NULL-terminated ASCII string           // [CODE_FIRST] 13357
                                 identifying the Redfish schema minor version, or NULL if      // [CODE_FIRST] 13357
                                 the schema is not version-controlled. If the caller does      // [CODE_FIRST] 13357
                                 not need this information, it may pass NULL for this          // [CODE_FIRST] 13357
                                 parameter.                                                    // [CODE_FIRST] 13357
  @param[out]  ErrataVersion     A pointer to receive a NULL-terminated ASCII string           // [CODE_FIRST] 13357
                                 identifying the Redfish schema errata version, or NULL if     // [CODE_FIRST] 13357
                                 the schema is not version-controlled. If the caller does      // [CODE_FIRST] 13357
                                 not need this information, it may pass NULL for this          // [CODE_FIRST] 13357
                                 parameter.                                                    // [CODE_FIRST] 13357
  @param[out]  DataSize          A pointer to receive the size in bytes of the resource        // [CODE_FIRST] 13357
                                 data pointed to by Data.                                      // [CODE_FIRST] 13357
  @param[out]  Data              A pointer to retrieve a pointer to the resource data          // [CODE_FIRST] 13357
                                 maintained by this protocol. The memory is owned by the       // [CODE_FIRST] 13357
                                 platform provider which called SetResource() earlier. The     // [CODE_FIRST] 13357
                                 caller must NOT free or modify this buffer.                   // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @retval EFI_SUCCESS            Resource data was returned successfully.                      // [CODE_FIRST] 13357
  @retval EFI_NOT_FOUND          No resource data is available for the specified Redfish       // [CODE_FIRST] 13357
                                 resource type.                                                // [CODE_FIRST] 13357
  @retval EFI_INVALID_PARAMETER  This, ResourceTypeName, DataSize, or Data is NULL.            // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
STATIC                                                                                         // [CODE_FIRST] 13357
EFI_STATUS                                                                                     // [CODE_FIRST] 13357
EFIAPI
// [CODE_FIRST] 13357
RedfishResourceGetResource (
  // [CODE_FIRST] 13357
  IN  EDKII_REDFISH_RESOURCE_PROTOCOL  *This,                                                  // [CODE_FIRST] 13357
  IN  CHAR8                            *ResourceTypeName,                                      // [CODE_FIRST] 13357
  OUT CHAR8                            **MajorVersion   OPTIONAL,                              // [CODE_FIRST] 13357
  OUT CHAR8                            **MinorVersion   OPTIONAL,                              // [CODE_FIRST] 13357
  OUT CHAR8                            **ErrataVersion  OPTIONAL,                              // [CODE_FIRST] 13357
  OUT UINTN                            *DataSize,                                              // [CODE_FIRST] 13357
  OUT VOID                             **Data                                                  // [CODE_FIRST] 13357
  )                                                                                            // [CODE_FIRST] 13357
{
  // [CODE_FIRST] 13357
  RESOURCE_DATA_ENTRY  *Entry;                                                                 // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if ((This == NULL) || (ResourceTypeName == NULL) || (DataSize == NULL) || (Data == NULL)) {
    // [CODE_FIRST] 13357
    return EFI_INVALID_PARAMETER;                                                              // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Establish a defined output state before any path that can fail, so a caller               // [CODE_FIRST] 13357
  // that mishandles the status reads NULL rather than an indeterminate value.                 // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  *DataSize = 0;                                                                               // [CODE_FIRST] 13357
  *Data     = NULL;                                                                            // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  if (MajorVersion != NULL) {
    // [CODE_FIRST] 13357
    *MajorVersion = NULL;                                                                      // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (MinorVersion != NULL) {
    // [CODE_FIRST] 13357
    *MinorVersion = NULL;                                                                      // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (ErrataVersion != NULL) {
    // [CODE_FIRST] 13357
    *ErrataVersion = NULL;                                                                     // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // No record for this type. Not EFI_UNSUPPORTED: any type is serviceable here,               // [CODE_FIRST] 13357
  // this one simply has no published data.                                                    // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  Entry = FindEntry (ResourceTypeName);                                                        // [CODE_FIRST] 13357
  if (Entry == NULL) {
    // [CODE_FIRST] 13357
    return EFI_NOT_FOUND;                                                                      // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // SetResource() never commits an empty payload, so a stored entry always                    // [CODE_FIRST] 13357
  // carries data. Kept as a guard against a future change to that invariant.                  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  if ((Entry->Data == NULL) || (Entry->DataSize == 0)) {
    // [CODE_FIRST] 13357
    return EFI_NOT_FOUND;                                                                      // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Return direct pointer to internal data - caller must NOT free                             // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  *DataSize = Entry->DataSize;                                                                 // [CODE_FIRST] 13357
  *Data     = Entry->Data;                                                                     // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  if (MajorVersion != NULL) {
    // [CODE_FIRST] 13357
    *MajorVersion = Entry->MajorVersion;                                                       // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (MinorVersion != NULL) {
    // [CODE_FIRST] 13357
    *MinorVersion = Entry->MinorVersion;                                                       // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (ErrataVersion != NULL) {
    // [CODE_FIRST] 13357
    *ErrataVersion = Entry->ErrataVersion;                                                     // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  return EFI_SUCCESS;                                                                          // [CODE_FIRST] 13357
}                                                                                              // [CODE_FIRST] 13357

// [CODE_FIRST] 13357

/**                                                                                            // [CODE_FIRST] 13357
  Sets the Redfish resource for a specified Redfish resource type.                             // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  The SetResource() function maintains the Redfish resource which is identified by             // [CODE_FIRST] 13357
  ResourceTypeName. If the resource already exists with the specified                          // [CODE_FIRST] 13357
  ResourceTypeName, the existing resource is replaced by the new resource. If                  // [CODE_FIRST] 13357
  DataSize is 0, with Data set to NULL, the Redfish resource associated with                   // [CODE_FIRST] 13357
  ResourceTypeName is removed. The memory buffer provided through Data is owned by             // [CODE_FIRST] 13357
  the caller, this protocol interface just maintains the pointer to the data and the           // [CODE_FIRST] 13357
  data size. The implementation of this protocol interface shouldn't modify the data           // [CODE_FIRST] 13357
  or free the memory buffer of the data.                                                       // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @param[in]  This               Pointer to the EDKII_REDFISH_RESOURCE_PROTOCOL instance.      // [CODE_FIRST] 13357
  @param[in]  ResourceTypeName   NULL-terminated ASCII string identifying the Redfish          // [CODE_FIRST] 13357
                                 schema resource type. The string should be the resource       // [CODE_FIRST] 13357
                                 type of Redfish data model defined by DMTF Redfish            // [CODE_FIRST] 13357
                                 working group.                                                // [CODE_FIRST] 13357
  @param[in]  MajorVersion       NULL-terminated ASCII string identifying the Redfish          // [CODE_FIRST] 13357
                                 schema major version, or NULL if the schema is not            // [CODE_FIRST] 13357
                                 version-controlled.                                           // [CODE_FIRST] 13357
  @param[in]  MinorVersion       NULL-terminated ASCII string identifying the Redfish          // [CODE_FIRST] 13357
                                 schema minor version, or NULL if the schema is not            // [CODE_FIRST] 13357
                                 version-controlled.                                           // [CODE_FIRST] 13357
  @param[in]  ErrataVersion      NULL-terminated ASCII string identifying the Redfish          // [CODE_FIRST] 13357
                                 schema errata version, or NULL if the schema is not           // [CODE_FIRST] 13357
                                 version-controlled.                                           // [CODE_FIRST] 13357
  @param[in]  DataSize           Size in bytes of the resource data pointed to by Data. If     // [CODE_FIRST] 13357
                                 DataSize is 0, the existing resource data for                 // [CODE_FIRST] 13357
                                 ResourceTypeName shall be removed.                            // [CODE_FIRST] 13357
  @param[in]  Data               Pointer to the resource data provided by the platform. If     // [CODE_FIRST] 13357
                                 DataSize is 0, Data must be NULL, to remove the existing      // [CODE_FIRST] 13357
                                 Redfish resource.                                             // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @retval EFI_SUCCESS            Resource data was set successfully.                           // [CODE_FIRST] 13357
  @retval EFI_NOT_FOUND          Removal was requested but no resource is currently            // [CODE_FIRST] 13357
                                 stored for ResourceTypeName.                                  // [CODE_FIRST] 13357
  @retval EFI_INVALID_PARAMETER  This or ResourceTypeName is NULL, DataSize > 0 and Data       // [CODE_FIRST] 13357
                                 is NULL, or DataSize = 0 and Data is not NULL.                // [CODE_FIRST] 13357
  @retval EFI_OUT_OF_RESOURCES   The resource database has reached the maximum allowed         // [CODE_FIRST] 13357
                                 number of entries and no new resource type can be added,      // [CODE_FIRST] 13357
                                 or the memory required for the internal metadata could        // [CODE_FIRST] 13357
                                 not be allocated.                                             // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
STATIC                                                                                         // [CODE_FIRST] 13357
EFI_STATUS                                                                                     // [CODE_FIRST] 13357
EFIAPI
// [CODE_FIRST] 13357
RedfishResourceSetResource (
  // [CODE_FIRST] 13357
  IN  EDKII_REDFISH_RESOURCE_PROTOCOL  *This,                                                  // [CODE_FIRST] 13357
  IN  CHAR8                            *ResourceTypeName,                                      // [CODE_FIRST] 13357
  IN  CHAR8                            *MajorVersion,                                          // [CODE_FIRST] 13357
  IN  CHAR8                            *MinorVersion,                                          // [CODE_FIRST] 13357
  IN  CHAR8                            *ErrataVersion,                                         // [CODE_FIRST] 13357
  IN  UINTN                            DataSize,                                               // [CODE_FIRST] 13357
  IN  VOID                             *Data                                                   // [CODE_FIRST] 13357
  )                                                                                            // [CODE_FIRST] 13357
{
  // [CODE_FIRST] 13357
  EFI_STATUS           Status;                                                                 // [CODE_FIRST] 13357
  RESOURCE_DATA_ENTRY  *Entry;                                                                 // [CODE_FIRST] 13357
  CHAR8                *NewTypeName;                                                           // [CODE_FIRST] 13357
  CHAR8                *NewMajor;                                                              // [CODE_FIRST] 13357
  CHAR8                *NewMinor;                                                              // [CODE_FIRST] 13357
  CHAR8                *NewErrata;                                                             // [CODE_FIRST] 13357
  BOOLEAN              IsNew;                                                                  // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if ((This == NULL) || (ResourceTypeName == NULL)) {
    // [CODE_FIRST] 13357
    return EFI_INVALID_PARAMETER;                                                              // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if ((DataSize > 0) && (Data == NULL)) {
    // [CODE_FIRST] 13357
    return EFI_INVALID_PARAMETER;                                                              // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if ((DataSize == 0) && (Data != NULL)) {
    // [CODE_FIRST] 13357
    return EFI_INVALID_PARAMETER;                                                              // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  NewTypeName = NULL;                                                                          // [CODE_FIRST] 13357
  NewMajor    = NULL;                                                                          // [CODE_FIRST] 13357
  NewMinor    = NULL;                                                                          // [CODE_FIRST] 13357
  NewErrata   = NULL;                                                                          // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  Entry = FindEntry (ResourceTypeName);                                                        // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // DataSize 0 with NULL Data is the caller's request to remove the stored resource.          // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  if (DataSize == 0) {
    // [CODE_FIRST] 13357
    if (Entry != NULL) {
      // [CODE_FIRST] 13357
      //                                                                                       // [CODE_FIRST] 13357
      // Release only what this driver allocated: TypeName and the version                     // [CODE_FIRST] 13357
      // strings. Data is the caller's buffer and must not be freed here.                      // [CODE_FIRST] 13357
      //                                                                                       // [CODE_FIRST] 13357
      if (Entry->TypeName != NULL) {
        // [CODE_FIRST] 13357
        FreePool (Entry->TypeName);                                                            // [CODE_FIRST] 13357
      }                                                                                        // [CODE_FIRST] 13357

      // [CODE_FIRST] 13357
      FreeEntryVersions (Entry);                                                               // [CODE_FIRST] 13357
      ZeroMem (Entry, sizeof (RESOURCE_DATA_ENTRY));                                           // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
      //                                                                                       // [CODE_FIRST] 13357
      // Move the last entry into the position of the deleted entry                            // [CODE_FIRST] 13357
      // (only one entry is moved).                                                            // [CODE_FIRST] 13357
      // If the entry being deleted happens to be the last one, skip the copy;                 // [CODE_FIRST] 13357
      // the ZeroMem above has already cleared it.                                             // [CODE_FIRST] 13357
      //                                                                                       // [CODE_FIRST] 13357
      if (Entry != &mPrivate.Entries[mPrivate.EntryCount - 1]) {
        // [CODE_FIRST] 13357
        CopyMem (Entry, &mPrivate.Entries[mPrivate.EntryCount - 1], sizeof (RESOURCE_DATA_ENTRY)); // [CODE_FIRST] 13357
        ZeroMem (&mPrivate.Entries[mPrivate.EntryCount - 1], sizeof (RESOURCE_DATA_ENTRY));        // [CODE_FIRST] 13357
      }                                                                                        // [CODE_FIRST] 13357

      // [CODE_FIRST] 13357
      mPrivate.EntryCount--;                                                                   // [CODE_FIRST] 13357
      return EFI_SUCCESS;                                                                      // [CODE_FIRST] 13357
    }                                                                                          // [CODE_FIRST] 13357

    // [CODE_FIRST] 13357
    return EFI_NOT_FOUND;                                                                      // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // DataSize is non-zero from here on; the removal cases returned above.                      // [CODE_FIRST] 13357
  // A missing entry means this call adds a resource, an existing one means                    // [CODE_FIRST] 13357
  // it replaces the resource already stored for this type.                                    // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  if (Entry == NULL) {
    // [CODE_FIRST] 13357
    IsNew = TRUE;                                                                              // [CODE_FIRST] 13357
  } else {
    // [CODE_FIRST] 13357
    IsNew = FALSE;                                                                             // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (IsNew && (mPrivate.EntryCount >= REDFISH_RESOURCE_MAX_ENTRIES)) {
    // [CODE_FIRST] 13357
    DEBUG ((
      // [CODE_FIRST] 13357
      DEBUG_ERROR,                                                                             // [CODE_FIRST] 13357
      "%a: Resource repository full (%u entries); increase PcdRedfishResourceMaxEntries\n",    // [CODE_FIRST] 13357
      __func__,                                                                                // [CODE_FIRST] 13357
      REDFISH_RESOURCE_MAX_ENTRIES                                                             // [CODE_FIRST] 13357
      ));                                                                                      // [CODE_FIRST] 13357
    return EFI_OUT_OF_RESOURCES;                                                               // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Allocate every string before touching the repository: a failure then leaves               // [CODE_FIRST] 13357
  // an existing resource unchanged and adds nothing for a new one.                            // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  Status = EFI_SUCCESS;                                                                        // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Only a new entry needs a TypeName copy.                                                   // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  if (IsNew) {
    // [CODE_FIRST] 13357
    Status = DupAsciiString (ResourceTypeName, &NewTypeName);                                  // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (!EFI_ERROR (Status)) {
    // [CODE_FIRST] 13357
    Status = DupAsciiString (MajorVersion, &NewMajor);                                         // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (!EFI_ERROR (Status)) {
    // [CODE_FIRST] 13357
    Status = DupAsciiString (MinorVersion, &NewMinor);                                         // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  if (!EFI_ERROR (Status)) {
    // [CODE_FIRST] 13357
    Status = DupAsciiString (ErrataVersion, &NewErrata);                                       // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Unwind the copies made above. The repository has not been touched yet, so                 // [CODE_FIRST] 13357
  // releasing these is the whole rollback: any resource already stored for this               // [CODE_FIRST] 13357
  // type is left exactly as it was.                                                           // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Each pointer is tested because the chain above stops at the first failure,                // [CODE_FIRST] 13357
  // leaving that copy and every later one NULL, and FreePool () asserts on NULL.              // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  if (EFI_ERROR (Status)) {
    // [CODE_FIRST] 13357
    if (NewTypeName != NULL) {
      // [CODE_FIRST] 13357
      FreePool (NewTypeName);                                                                  // [CODE_FIRST] 13357
    }                                                                                          // [CODE_FIRST] 13357

    // [CODE_FIRST] 13357
    if (NewMajor != NULL) {
      // [CODE_FIRST] 13357
      FreePool (NewMajor);                                                                     // [CODE_FIRST] 13357
    }                                                                                          // [CODE_FIRST] 13357

    // [CODE_FIRST] 13357
    if (NewMinor != NULL) {
      // [CODE_FIRST] 13357
      FreePool (NewMinor);                                                                     // [CODE_FIRST] 13357
    }                                                                                          // [CODE_FIRST] 13357

    // [CODE_FIRST] 13357
    if (NewErrata != NULL) {
      // [CODE_FIRST] 13357
      FreePool (NewErrata);                                                                    // [CODE_FIRST] 13357
    }                                                                                          // [CODE_FIRST] 13357

    // [CODE_FIRST] 13357
    return Status;                                                                             // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // A new entry occupies the next free slot and takes ownership of NewTypeName,               // [CODE_FIRST] 13357
  // the copy allocated above.                                                                 // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  if (IsNew) {
    // [CODE_FIRST] 13357
    Entry           = &mPrivate.Entries[mPrivate.EntryCount];                                  // [CODE_FIRST] 13357
    Entry->TypeName = NewTypeName;                                                             // [CODE_FIRST] 13357
    mPrivate.EntryCount++;                                                                     // [CODE_FIRST] 13357
  } else {
    // [CODE_FIRST] 13357
    //                                                                                         // [CODE_FIRST] 13357
    // An existing entry already holds the same TypeName, so no copy was allocated for it.     // [CODE_FIRST] 13357
    // Release the superseded strings.                                                         // [CODE_FIRST] 13357
    //                                                                                         // [CODE_FIRST] 13357
    FreeEntryVersions (Entry);                                                                 // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  // Data is stored by pointer and remains the caller's buffer.                                // [CODE_FIRST] 13357
  // The version strings are this driver's own copies, and the entry owns them from here on.   // [CODE_FIRST] 13357
  //                                                                                           // [CODE_FIRST] 13357
  Entry->Data          = Data;                                                                                                                                            // [CODE_FIRST] 13357
  Entry->DataSize      = DataSize;                                                                                                                                        // [CODE_FIRST] 13357
  Entry->MajorVersion  = NewMajor;                                                                                                                                        // [CODE_FIRST] 13357
  Entry->MinorVersion  = NewMinor;                                                                                                                                        // [CODE_FIRST] 13357
  Entry->ErrataVersion = NewErrata;                                                                                                                                       // [CODE_FIRST] 13357
                                                                                                                                                                          // [CODE_FIRST] 13357
  DEBUG ((DEBUG_INFO, "%a: %a resource for %a (%u bytes), total entries: %u\n", __func__, IsNew ? "Added" : "Updated", ResourceTypeName, DataSize, mPrivate.EntryCount)); // [CODE_FIRST] 13357
  return EFI_SUCCESS;                                                                                                                                                     // [CODE_FIRST] 13357
}                                                                                              // [CODE_FIRST] 13357

// [CODE_FIRST] 13357

/**                                                                                            // [CODE_FIRST] 13357
  Entry point.                                                                                 // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  Publishes EDKII_REDFISH_RESOURCE_PROTOCOL. The repository itself is a                        // [CODE_FIRST] 13357
  statically allocated structure sized by PcdRedfishResourceMaxEntries at build                // [CODE_FIRST] 13357
  time, so there is nothing to allocate or release here: the driver either ends                // [CODE_FIRST] 13357
  up with the protocol published, or leaves no trace.                                          // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @param[in]  ImageHandle  Image handle.                                                       // [CODE_FIRST] 13357
  @param[in]  SystemTable  Pointer to EFI System Table.                                        // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  @retval EFI_SUCCESS      The protocol interface was installed.                               // [CODE_FIRST] 13357
  @retval Other            No protocol installed, unload driver.                               // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
EFI_STATUS                                                                                     // [CODE_FIRST] 13357
EFIAPI
// [CODE_FIRST] 13357
RedfishResourceDxeEntryPoint (
  // [CODE_FIRST] 13357
  IN EFI_HANDLE        ImageHandle,                                                            // [CODE_FIRST] 13357
  IN EFI_SYSTEM_TABLE  *SystemTable                                                            // [CODE_FIRST] 13357
  )                                                                                            // [CODE_FIRST] 13357
{
  // [CODE_FIRST] 13357
  EFI_STATUS  Status;                                                                          // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  mPrivate.EntryCount                         = 0;                                             // [CODE_FIRST] 13357
  mPrivate.Protocol.GetSupportedResourceTypes = RedfishResourceGetSupportedResourceTypes;      // [CODE_FIRST] 13357
  mPrivate.Protocol.GetResource               = RedfishResourceGetResource;                    // [CODE_FIRST] 13357
  mPrivate.Protocol.SetResource               = RedfishResourceSetResource;                    // [CODE_FIRST] 13357
                                                                                               // [CODE_FIRST] 13357
  Status = gBS->InstallMultipleProtocolInterfaces (
                  // [CODE_FIRST] 13357
                  &ImageHandle,                                                                // [CODE_FIRST] 13357
                  &gEdkIIRedfishResourceProtocolGuid,                                          // [CODE_FIRST] 13357
                  &mPrivate.Protocol,                                                          // [CODE_FIRST] 13357
                  NULL                                                                         // [CODE_FIRST] 13357
                  );                                                                           // [CODE_FIRST] 13357
  if (EFI_ERROR (Status)) {
    // [CODE_FIRST] 13357
    DEBUG ((DEBUG_ERROR, "%a: Failed to install protocol: %r\n", __func__, Status));           // [CODE_FIRST] 13357
    return Status;                                                                             // [CODE_FIRST] 13357
  }                                                                                            // [CODE_FIRST] 13357

  // [CODE_FIRST] 13357
  DEBUG ((DEBUG_INFO, "%a: EDKII_REDFISH_RESOURCE_PROTOCOL installed\n", __func__));           // [CODE_FIRST] 13357
  return EFI_SUCCESS;                                                                          // [CODE_FIRST] 13357
}                                                                                              // [CODE_FIRST] 13357

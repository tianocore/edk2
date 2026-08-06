/** @file
    File for SMMU config structures.

    This SMMU_CONFIG structure is used to pass the SMMU configuration data from
    the platform to the SMMU driver. The Smmu driver will use this data to install
    the IORT table and configure the SMMU hardware.

    Given the IORT is configurable and platform dependent, the SMMU_CONFIG structure contains
    all info relevant to the IORT table and SMMUv3 platform specific configuration.

    See <https://developer.arm.com/documentation/den0049/latest/> for IORT spec.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

// Increment SMMU_CONFIG version when the structure changes.
// Backwards compatibility is currently not supported.
// Future backwards compatibility is only possible if new fields are added to the end of the structure and existing fields are not modified.
// SmmuDxe driver will check and enforce the version of the SMMU_CONFIG structure to this current version set here.
#define CURRENT_SMMU_CONFIG_VERSION_MAJOR  1
#define CURRENT_SMMU_CONFIG_VERSION_MINOR  0

#pragma pack(push, 1)

// Maximum length (including NULL) of the ACPI namespace path stored for a
// NonDiscoverable device entry. Must be large enough to hold the
// IORT Named Component ObjectName
#define SMMU_NC_DEVICE_OBJNAME_MAX  32

// Platform-provided lookup entry mapping a NonDiscoverable device's
// PciIo->GetLocation()-derived UniqueId to the SMMU StreamID's associated
// with the NamedComponent node in the IORT.
//
// Each NonDiscoverable device exposes a UniqueId from the NonDiscoverableDeviceRegistrationLib.
// This is used to determine a determinstic PciIo->GetLocation().
typedef struct _SMMU_NC_DEVICE_ENTRY {
  UINT64    UniqueId;                                   // Value from the NON_DISCOVERABLE_DEVICE_UNIQUE_ID protocol on the handle.
  CHAR8     ObjName[SMMU_NC_DEVICE_OBJNAME_MAX];        // IORT Named Component ObjectName (NUL-terminated). SmmuDxe walks the IORT for the matching NC node to recover the owning SMMU base and the full StreamID list.
} SMMU_NC_DEVICE_ENTRY;

// SMMU_CONFIG structure to pass the SMMU configuration data from the platform to the SMMU driver.
// Platform will configure SmmuDisabledList size and offset to the SMMU disabled list appropriatley
// for any SMMU that needs be disabled in UEFI and set to bypass.
typedef struct _SMMU_CONFIG {
  UINT32    VersionMajor;
  UINT32    VersionMinor;
  UINT32    SmmuDisabledListSize;   // Size of SmmuDisabledList in bytes.
  UINT32    SmmuDisabledListOffset; // Offset in bytes to the SmmuDisabledList from the start of the HOB structure.
  UINT32    IortSize;
  UINT32    IortOffset;             // Offset in bytes to the IORT table from the start of the HOB structure.
  UINT32    NcDeviceListSize;       // Size of the NonDiscoverable device lookup array in bytes (multiple of sizeof(SMMU_NC_DEVICE_ENTRY)).
  UINT32    NcDeviceListOffset;     // Offset in bytes to the NonDiscoverable device lookup array from the start of the HOB structure. 0 if absent.
} SMMU_CONFIG;

#pragma pack(pop)

#define SMMU_CONFIG_HOB_GUID \
  { 0xcd56ec8f, 0x75f1, 0x440a, { 0xaa, 0x48, 0x09, 0x58, 0xb1, 0x1c, 0x9a, 0xa7 } }

extern EFI_GUID  gSmmuConfigHobGuid;

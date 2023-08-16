/** @file
Library for setting the memory protection settings for DXE.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SetMemoryProtectionsLib.h>

#pragma pack(1)

typedef struct {
  // Protection settings
  MEMORY_PROTECTION_SETTINGS    Mps;
  // Extra byte for tracking if protection settings have been locked
  BOOLEAN                       MemoryProtectionSettingsLocked;
} MEMORY_PROTECTION_SETTINGS_PRIVATE;

#pragma pack()

/////////////////////////////
// DXE PROFILE DEFINITIONS //
/////////////////////////////

//
//  A memory profile with strict settings ideal for development scenarios.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_DEBUG          \
{                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
  TRUE, /* Stack Guard */                             \
  TRUE, /* Stack Execution Protection */              \
  {     /* NULL Pointer Detection */                  \
    .Enabled                                = TRUE,   \
    .DisableEndOfDxe                        = FALSE,  \
    .NonstopModeEnabled                     = TRUE    \
  },                                                  \
  { /* Image Protection */                            \
    .ProtectImageFromUnknown                = TRUE,   \
    .ProtectImageFromFv                     = TRUE    \
  },                                                  \
  { /* Execution Protection */                        \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = TRUE,   \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = TRUE,   \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  },                                                  \
  { /* Heap Guard */                                  \
    .PageGuardEnabled                       = TRUE,   \
    .PoolGuardEnabled                       = TRUE,   \
    .FreedMemoryGuardEnabled                = FALSE,  \
    .NonstopModeEnabled                     = TRUE,   \
    .GuardAlignedToTail                     = TRUE    \
  },                                                  \
  { /* Pool Guard */                                  \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = TRUE,   \
      [EfiLoaderData]                       = TRUE,   \
      [EfiBootServicesCode]                 = TRUE,   \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = TRUE,   \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = FALSE,  \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  },                                                  \
  { /* Page Guard */                                  \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = TRUE,   \
      [EfiLoaderData]                       = TRUE,   \
      [EfiBootServicesCode]                 = TRUE,   \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = TRUE,   \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = FALSE,  \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  }                                                   \
}

//
//  A memory profile recommended for production. Compared to the debug
//  settings, this profile removes the pool guards and uses page guards
//  for fewer memory types.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE      \
{                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
  TRUE, /* Stack Guard */                             \
  TRUE, /* Stack Execution Protection */              \
  {     /* NULL Pointer Detection */                  \
    .Enabled                                = TRUE,   \
    .DisableEndOfDxe                        = FALSE,  \
    .NonstopModeEnabled                     = FALSE   \
  },                                                  \
  { /* Image Protection */                            \
    .ProtectImageFromUnknown                = FALSE,  \
    .ProtectImageFromFv                     = TRUE    \
  },                                                  \
  { /* Execution Protection */                        \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = TRUE,   \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = TRUE,   \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  },                                                  \
  { /* Heap Guard */                                  \
    .PageGuardEnabled                       = TRUE,   \
    .PoolGuardEnabled                       = FALSE,  \
    .FreedMemoryGuardEnabled                = FALSE,  \
    .NonstopModeEnabled                     = FALSE,  \
    .GuardAlignedToTail                     = TRUE    \
  },                                                  \
  { /* Pool Guard */                                  \
    0                                                 \
  },                                                  \
  { /* Page Guard */                                  \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = FALSE,  \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = FALSE,  \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = FALSE,  \
      [EfiUnusableMemory]                   = FALSE,  \
      [EfiACPIReclaimMemory]                = FALSE,  \
      [EfiACPIMemoryNVS]                    = FALSE,  \
      [EfiMemoryMappedIO]                   = FALSE,  \
      [EfiMemoryMappedIOPortSpace]          = FALSE,  \
      [EfiPalCode]                          = FALSE,  \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = FALSE   \
    }                                                 \
  }                                                   \
}

//
//  A memory profile which mirrors DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE
//  but doesn't include page guards.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE_NO_PAGE_GUARDS   \
{                                                                 \
  DXE_MEMORY_PROTECTION_SIGNATURE,                                \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,                 \
  TRUE, /* Stack Guard */                                         \
  TRUE, /* Stack Execution Protection */                          \
  {     /* NULL Pointer Detection */                              \
    .Enabled                                = TRUE,               \
    .DisableEndOfDxe                        = FALSE,              \
    .NonstopModeEnabled                     = FALSE               \
  },                                                              \
  { /* Image Protection */                                        \
    .ProtectImageFromUnknown                = FALSE,              \
    .ProtectImageFromFv                     = TRUE                \
  },                                                              \
  { /* Execution Protection */                                    \
    .EnabledForType = {                                           \
      [EfiReservedMemoryType]               = TRUE,               \
      [EfiLoaderCode]                       = FALSE,              \
      [EfiLoaderData]                       = TRUE,               \
      [EfiBootServicesCode]                 = FALSE,              \
      [EfiBootServicesData]                 = TRUE,               \
      [EfiRuntimeServicesCode]              = FALSE,              \
      [EfiRuntimeServicesData]              = TRUE,               \
      [EfiConventionalMemory]               = TRUE,               \
      [EfiUnusableMemory]                   = TRUE,               \
      [EfiACPIReclaimMemory]                = TRUE,               \
      [EfiACPIMemoryNVS]                    = TRUE,               \
      [EfiMemoryMappedIO]                   = TRUE,               \
      [EfiMemoryMappedIOPortSpace]          = TRUE,               \
      [EfiPalCode]                          = TRUE,               \
      [EfiPersistentMemory]                 = FALSE,              \
      [EfiUnacceptedMemoryType]             = TRUE,               \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,               \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE                \
    }                                                             \
  },                                                              \
  { /* Heap Guard */                                              \
    0                                                             \
  },                                                              \
  { /* Pool Guard */                                              \
    0                                                             \
  },                                                              \
  { /* Page Guard */                                              \
    0                                                             \
  }                                                               \
}

//
//  A memory profile which uses the fixed at build PCDs defined in MdeModulePkg.dec
//
#define DXE_MEMORY_PROTECTION_SETTINGS_PCD                                                                                            \
{                                                                                                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                                                                                                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,                                                                                     \
  FixedPcdGetBool (PcdCpuStackGuard), /* Stack Guard */                                                                               \
  TRUE,                               /* Stack Execution Protection (MUST BE POPULATED) */                                            \
  {                                   /* NULL Pointer Detection */                                                                    \
    .Enabled            = ((FixedPcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT0) != 0),                                         \
    .DisableEndOfDxe    = ((FixedPcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT7) != 0),                                         \
    .NonstopModeEnabled = ((FixedPcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT6) != 0)                                          \
  },                                                                                                                                  \
  { /* Image Protection */                                                                                                            \
    .ProtectImageFromUnknown = ((FixedPcdGet32 (PcdImageProtectionPolicy) & BIT0) != 0),                                              \
    .ProtectImageFromFv      = ((FixedPcdGet32 (PcdImageProtectionPolicy) & BIT1) != 0)                                               \
  },                                                                                                                                  \
  { /* Execution Protection */                                                                                                        \
    .EnabledForType = {                                                                                                               \
      [EfiReservedMemoryType]               = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiReservedMemoryType) != 0),        \
      [EfiLoaderCode]                       = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiLoaderCode) != 0),                \
      [EfiLoaderData]                       = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiLoaderData) != 0),                \
      [EfiBootServicesCode]                 = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiBootServicesCode) != 0),          \
      [EfiBootServicesData]                 = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiBootServicesData) != 0),          \
      [EfiRuntimeServicesCode]              = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiRuntimeServicesCode) != 0),       \
      [EfiRuntimeServicesData]              = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiRuntimeServicesData) != 0),       \
      [EfiConventionalMemory]               = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiConventionalMemory) != 0),        \
      [EfiUnusableMemory]                   = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiUnusableMemory) != 0),            \
      [EfiACPIReclaimMemory]                = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiACPIReclaimMemory) != 0),         \
      [EfiACPIMemoryNVS]                    = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiACPIMemoryNVS) != 0),             \
      [EfiMemoryMappedIO]                   = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiMemoryMappedIO) != 0),            \
      [EfiMemoryMappedIOPortSpace]          = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiMemoryMappedIOPortSpace) != 0),   \
      [EfiPalCode]                          = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiPalCode) != 0),                   \
      [EfiPersistentMemory]                 = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiPersistentMemory) != 0),          \
      [EfiUnacceptedMemoryType]             = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & EfiUnacceptedMemoryType) != 0),      \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & BIT62) != 0),                        \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = ((FixedPcdGet64 (PcdDxeNxMemoryProtectionPolicy) & BIT63) != 0)                         \
    }                                                                                                                                 \
  },                                                                                                                                  \
  { /* Heap Guard */                                                                                                                  \
    .PageGuardEnabled                       = ((FixedPcdGet8 (PcdHeapGuardPropertyMask) & BIT0) != 0),                                \
    .PoolGuardEnabled                       = ((FixedPcdGet8 (PcdHeapGuardPropertyMask) & BIT1) != 0),                                \
    .FreedMemoryGuardEnabled                = ((FixedPcdGet8 (PcdHeapGuardPropertyMask) & BIT4) != 0),                                \
    .NonstopModeEnabled                     = ((FixedPcdGet8 (PcdHeapGuardPropertyMask) & BIT6) != 0),                                \
    .GuardAlignedToTail                     = ((FixedPcdGet8 (PcdHeapGuardPropertyMask) & BIT7) == 0)                                 \
  },                                                                                                                                  \
  { /* Pool Guard */                                                                                                                  \
    .EnabledForType = {                                                                                                               \
      [EfiReservedMemoryType]               = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiReservedMemoryType) != 0),                  \
      [EfiLoaderCode]                       = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiLoaderCode) != 0),                          \
      [EfiLoaderData]                       = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiLoaderData) != 0),                          \
      [EfiBootServicesCode]                 = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiBootServicesCode) != 0),                    \
      [EfiBootServicesData]                 = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiBootServicesData) != 0),                    \
      [EfiRuntimeServicesCode]              = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiRuntimeServicesCode) != 0),                 \
      [EfiRuntimeServicesData]              = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiRuntimeServicesData) != 0),                 \
      [EfiConventionalMemory]               = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiConventionalMemory) != 0),                  \
      [EfiUnusableMemory]                   = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiUnusableMemory) != 0),                      \
      [EfiACPIReclaimMemory]                = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiACPIReclaimMemory) != 0),                   \
      [EfiACPIMemoryNVS]                    = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiACPIMemoryNVS) != 0),                       \
      [EfiMemoryMappedIO]                   = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiMemoryMappedIO) != 0),                      \
      [EfiMemoryMappedIOPortSpace]          = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiMemoryMappedIOPortSpace) != 0),             \
      [EfiPalCode]                          = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiPalCode) != 0),                             \
      [EfiPersistentMemory]                 = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiPersistentMemory) != 0),                    \
      [EfiUnacceptedMemoryType]             = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiUnacceptedMemoryType) != 0),                \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = ((FixedPcdGet64 (PcdHeapGuardPoolType) & BIT62) != 0),                                  \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = ((FixedPcdGet64 (PcdHeapGuardPoolType) & BIT63) != 0)                                   \
    }                                                                                                                                 \
  },                                                                                                                                  \
  { /* Page Guard */                                                                                                                  \
    .EnabledForType = {                                                                                                               \
      [EfiReservedMemoryType]               = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiReservedMemoryType) != 0),                  \
      [EfiLoaderCode]                       = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiLoaderCode) != 0),                          \
      [EfiLoaderData]                       = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiLoaderData) != 0),                          \
      [EfiBootServicesCode]                 = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiBootServicesCode) != 0),                    \
      [EfiBootServicesData]                 = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiBootServicesData) != 0),                    \
      [EfiRuntimeServicesCode]              = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiRuntimeServicesCode) != 0),                 \
      [EfiRuntimeServicesData]              = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiRuntimeServicesData) != 0),                 \
      [EfiConventionalMemory]               = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiConventionalMemory) != 0),                  \
      [EfiUnusableMemory]                   = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiUnusableMemory) != 0),                      \
      [EfiACPIReclaimMemory]                = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiACPIReclaimMemory) != 0),                   \
      [EfiACPIMemoryNVS]                    = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiACPIMemoryNVS) != 0),                       \
      [EfiMemoryMappedIO]                   = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiMemoryMappedIO) != 0),                      \
      [EfiMemoryMappedIOPortSpace]          = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiMemoryMappedIOPortSpace) != 0),             \
      [EfiPalCode]                          = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiPalCode) != 0),                             \
      [EfiPersistentMemory]                 = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiPersistentMemory) != 0),                    \
      [EfiUnacceptedMemoryType]             = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiUnacceptedMemoryType) != 0),                \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = ((FixedPcdGet64 (PcdHeapGuardPageType) & BIT62) != 0),                                  \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = ((FixedPcdGet64 (PcdHeapGuardPageType) & BIT63) != 0)                                   \
    }                                                                                                                                 \
  }                                                                                                                                   \
}

//  A memory profile recommended for compatibility with older
//  versions of Grub.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_GRUB_COMPAT    \
{                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
  TRUE, /* Stack Guard */                             \
  TRUE, /* Stack Execution Protection */              \
  {     /* NULL Pointer Detection */                  \
    .Enabled                                = TRUE,   \
    .DisableEndOfDxe                        = TRUE,   \
    .NonstopModeEnabled                     = FALSE   \
  },                                                  \
  { /* Image Protection */                            \
    .ProtectImageFromUnknown                = FALSE,  \
    .ProtectImageFromFv                     = TRUE    \
  },                                                  \
  { /* Execution Protection */                        \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = FALSE,  \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = TRUE,   \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  },                                                  \
  { /* Heap Guard */                                  \
    .PageGuardEnabled                       = TRUE,   \
    .PoolGuardEnabled                       = FALSE,  \
    .FreedMemoryGuardEnabled                = FALSE,  \
    .NonstopModeEnabled                     = FALSE,  \
    .GuardAlignedToTail                     = TRUE    \
  },                                                  \
  { /* Pool Guard */                                  \
    0                                                 \
  },                                                  \
  { /* Page Guard */                                  \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = FALSE,  \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = FALSE,  \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = FALSE,  \
      [EfiUnusableMemory]                   = FALSE,  \
      [EfiACPIReclaimMemory]                = FALSE,  \
      [EfiACPIMemoryNVS]                    = FALSE,  \
      [EfiMemoryMappedIO]                   = FALSE,  \
      [EfiMemoryMappedIOPortSpace]          = FALSE,  \
      [EfiPalCode]                          = FALSE,  \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = FALSE   \
    }                                                 \
  }                                                   \
}

//
//  A memory profile which disables all DXE memory protection settings.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_OFF            \
{                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
  FALSE, /* Stack Guard */                            \
  FALSE, /* Stack Execution Protection */             \
  {      /* NULL Pointer Detection */                 \
    0                                                 \
  },                                                  \
  { /* Image Protection */                            \
    0                                                 \
  },                                                  \
  { /* Execution Protection */                        \
    0                                                 \
  },                                                  \
  { /* Heap Guard */                                  \
    0                                                 \
  },                                                  \
  { /* Pool Guard */                                  \
    0                                                 \
  },                                                  \
  { /* Page Guard */                                  \
    0                                                 \
  }                                                   \
}

////////////////////////////
// MM PROFILE DEFINITIONS //
////////////////////////////

//
//  A memory profile ideal for development scenarios.
//
#define MM_MEMORY_PROTECTION_SETTINGS_DEBUG        \
{                                                  \
  MM_MEMORY_PROTECTION_SIGNATURE,                  \
  MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,   \
  { /* NULL Pointer Detection */                   \
    .Enabled                             = TRUE,   \
    .NonstopModeEnabled                  = TRUE    \
  },                                               \
  { /* Heap Guard */                               \
    .PageGuardEnabled                    = TRUE,   \
    .PoolGuardEnabled                    = TRUE,   \
    .NonstopModeEnabled                  = TRUE,   \
    .GuardAlignedToTail                  = TRUE    \
  },                                               \
  { /* Pool Guard */                               \
    .EnabledForType = {                            \
      [EfiReservedMemoryType]            = FALSE,  \
      [EfiLoaderCode]                    = FALSE,  \
      [EfiLoaderData]                    = FALSE,  \
      [EfiBootServicesCode]              = FALSE,  \
      [EfiBootServicesData]              = TRUE,   \
      [EfiRuntimeServicesCode]           = FALSE,  \
      [EfiRuntimeServicesData]           = TRUE,   \
      [EfiConventionalMemory]            = FALSE,  \
      [EfiUnusableMemory]                = FALSE,  \
      [EfiACPIReclaimMemory]             = FALSE,  \
      [EfiACPIMemoryNVS]                 = FALSE,  \
      [EfiMemoryMappedIO]                = FALSE,  \
      [EfiMemoryMappedIOPortSpace]       = FALSE,  \
      [EfiPalCode]                       = FALSE,  \
      [EfiPersistentMemory]              = FALSE,  \
      [EfiUnacceptedMemoryType]          = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]     = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]      = FALSE   \
    }                                              \
  },                                               \
  { /* Page Guard */                               \
    .EnabledForType = {                            \
      [EfiReservedMemoryType]            = FALSE,  \
      [EfiLoaderCode]                    = FALSE,  \
      [EfiLoaderData]                    = FALSE,  \
      [EfiBootServicesCode]              = FALSE,  \
      [EfiBootServicesData]              = TRUE,   \
      [EfiRuntimeServicesCode]           = FALSE,  \
      [EfiRuntimeServicesData]           = TRUE,   \
      [EfiConventionalMemory]            = FALSE,  \
      [EfiUnusableMemory]                = FALSE,  \
      [EfiACPIReclaimMemory]             = FALSE,  \
      [EfiACPIMemoryNVS]                 = FALSE,  \
      [EfiMemoryMappedIO]                = FALSE,  \
      [EfiMemoryMappedIOPortSpace]       = FALSE,  \
      [EfiPalCode]                       = FALSE,  \
      [EfiPersistentMemory]              = FALSE,  \
      [EfiUnacceptedMemoryType]          = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]     = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]      = FALSE   \
    }                                              \
  }                                                \
}

//
//  A memory profile ideal for production scenarios.
//
#define MM_MEMORY_PROTECTION_SETTINGS_PROD_MODE    \
{                                                  \
  MM_MEMORY_PROTECTION_SIGNATURE,                  \
  MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,   \
  { /* NULL Pointer Detection */                   \
    .Enabled                             = TRUE,   \
    .NonstopModeEnabled                  = FALSE   \
  },                                               \
  { /* Heap Guard */                               \
    .PageGuardEnabled                    = TRUE,   \
    .PoolGuardEnabled                    = FALSE,  \
    .NonstopModeEnabled                  = FALSE,  \
    .GuardAlignedToTail                  = TRUE    \
  },                                               \
  { /* Pool Guard */                               \
    0                                              \
  },                                               \
  { /* Page Guard */                               \
    .EnabledForType = {                            \
      [EfiReservedMemoryType]            = FALSE,  \
      [EfiLoaderCode]                    = FALSE,  \
      [EfiLoaderData]                    = FALSE,  \
      [EfiBootServicesCode]              = FALSE,  \
      [EfiBootServicesData]              = TRUE,   \
      [EfiRuntimeServicesCode]           = FALSE,  \
      [EfiRuntimeServicesData]           = TRUE,   \
      [EfiConventionalMemory]            = FALSE,  \
      [EfiUnusableMemory]                = FALSE,  \
      [EfiACPIReclaimMemory]             = FALSE,  \
      [EfiACPIMemoryNVS]                 = FALSE,  \
      [EfiMemoryMappedIO]                = FALSE,  \
      [EfiMemoryMappedIOPortSpace]       = FALSE,  \
      [EfiPalCode]                       = FALSE,  \
      [EfiPersistentMemory]              = FALSE,  \
      [EfiUnacceptedMemoryType]          = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]     = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]      = FALSE   \
    }                                              \
  }                                                \
}

//
//  A memory profile which uses the fixed at build PCDs defined in MdeModulePkg.dec
//
#define MM_MEMORY_PROTECTION_SETTINGS_PCD                                                                                 \
{                                                                                                                         \
  MM_MEMORY_PROTECTION_SIGNATURE,                                                                                         \
  MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,                                                                          \
  { /* NULL Pointer Detection */                                                                                          \
    .Enabled            = ((FixedPcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT1) != 0),                             \
    .NonstopModeEnabled = ((FixedPcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT6) != 0)                              \
  },                                                                                                                      \
  { /* Heap Guard */                                                                                                      \
    .PageGuardEnabled                       = ((FixedPcdGet8(PcdHeapGuardPropertyMask) & BIT2) != 0),                     \
    .PoolGuardEnabled                       = ((FixedPcdGet8(PcdHeapGuardPropertyMask) & BIT3) != 0),                     \
    .NonstopModeEnabled                     = ((FixedPcdGet8(PcdHeapGuardPropertyMask) & BIT6) != 0),                     \
    .GuardAlignedToTail                     = ((FixedPcdGet8(PcdHeapGuardPropertyMask) & BIT7) == 0)                      \
  },                                                                                                                      \
  { /* Pool Guard */                                                                                                      \
    .EnabledForType = {                                                                                                   \
      [EfiReservedMemoryType]               = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiReservedMemoryType) != 0),      \
      [EfiLoaderCode]                       = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiLoaderCode) != 0),              \
      [EfiLoaderData]                       = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiLoaderData) != 0),              \
      [EfiBootServicesCode]                 = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiBootServicesCode) != 0),        \
      [EfiBootServicesData]                 = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiBootServicesData) != 0),        \
      [EfiRuntimeServicesCode]              = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiRuntimeServicesCode) != 0),     \
      [EfiRuntimeServicesData]              = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiRuntimeServicesData) != 0),     \
      [EfiConventionalMemory]               = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiConventionalMemory) != 0),      \
      [EfiUnusableMemory]                   = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiUnusableMemory) != 0),          \
      [EfiACPIReclaimMemory]                = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiACPIReclaimMemory) != 0),       \
      [EfiACPIMemoryNVS]                    = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiACPIMemoryNVS) != 0),           \
      [EfiMemoryMappedIO]                   = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiMemoryMappedIO) != 0),          \
      [EfiMemoryMappedIOPortSpace]          = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiMemoryMappedIOPortSpace) != 0), \
      [EfiPalCode]                          = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiPalCode) != 0),                 \
      [EfiPersistentMemory]                 = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiPersistentMemory) != 0),        \
      [EfiUnacceptedMemoryType]             = ((FixedPcdGet64 (PcdHeapGuardPoolType) & EfiUnacceptedMemoryType) != 0),    \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = ((FixedPcdGet64 (PcdHeapGuardPoolType) & BIT62) != 0),                      \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = ((FixedPcdGet64 (PcdHeapGuardPoolType) & BIT63) != 0)                       \
    }                                                                                                                     \
  },                                                                                                                      \
  { /* Page Guard */                                                                                                      \
    .EnabledForType = {                                                                                                   \
      [EfiReservedMemoryType]               = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiReservedMemoryType) != 0),      \
      [EfiLoaderCode]                       = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiLoaderCode) != 0),              \
      [EfiLoaderData]                       = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiLoaderData) != 0),              \
      [EfiBootServicesCode]                 = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiBootServicesCode) != 0),        \
      [EfiBootServicesData]                 = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiBootServicesData) != 0),        \
      [EfiRuntimeServicesCode]              = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiRuntimeServicesCode) != 0),     \
      [EfiRuntimeServicesData]              = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiRuntimeServicesData) != 0),     \
      [EfiConventionalMemory]               = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiConventionalMemory) != 0),      \
      [EfiUnusableMemory]                   = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiUnusableMemory) != 0),          \
      [EfiACPIReclaimMemory]                = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiACPIReclaimMemory) != 0),       \
      [EfiACPIMemoryNVS]                    = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiACPIMemoryNVS) != 0),           \
      [EfiMemoryMappedIO]                   = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiMemoryMappedIO) != 0),          \
      [EfiMemoryMappedIOPortSpace]          = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiMemoryMappedIOPortSpace) != 0), \
      [EfiPalCode]                          = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiPalCode) != 0),                 \
      [EfiPersistentMemory]                 = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiPersistentMemory) != 0),        \
      [EfiUnacceptedMemoryType]             = ((FixedPcdGet64 (PcdHeapGuardPageType) & EfiUnacceptedMemoryType) != 0),    \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = ((FixedPcdGet64 (PcdHeapGuardPageType) & BIT62) != 0),                      \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = ((FixedPcdGet64 (PcdHeapGuardPageType) & BIT63) != 0)                       \
    }                                                                                                                     \
  }                                                                                                                       \
}

//
//  A memory profile which disables all MM memory protection settings.
//
#define MM_MEMORY_PROTECTION_SETTINGS_OFF           \
{                                                   \
  MM_MEMORY_PROTECTION_SIGNATURE,                   \
  MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,    \
  { /* NULL Pointer Detection */                    \
    0                                               \
  },                                                \
  { /* Heap Guard */                                \
    0                                               \
  },                                                \
  { /* Pool Guard */                                \
    0                                               \
  },                                                \
  { /* Page Guard */                                \
    0                                               \
  }                                                 \
}

////////////////////////////
// PROFILE CONFIGURATIONS //
////////////////////////////

DXE_MEMORY_PROTECTION_PROFILES  DxeMemoryProtectionProfiles[DxeMemoryProtectionSettingsMax] = {
  [DxeMemoryProtectionSettingsDebug] =               {
    .Name        = "Debug",
    .Description = "Development profile ideal for debug scenarios",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_DEBUG
  },
  [DxeMemoryProtectionSettingsRelease] =             {
    .Name        = "Release",
    .Description = "Release profile recommended for production scenarios",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE
  },
  [DxeMemoryProtectionSettingsReleaseNoPageGuards] = {
    .Name        = "ReleaseNoPageGuards",
    .Description = "Release profile without page guards recommended for performance sensitive production scenarios",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE_NO_PAGE_GUARDS
  },
  [DxeMemoryProtectionSettingsPcd] =                 {
    .Name        = "Pcd",
    .Description = "Memory protection settings from PCDs",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_PCD
  },
  [DxeMemoryProtectionSettingsGrubCompat] =          {
    .Name        = "GrubCompat",
    .Description = "DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE with some protections disabled for legacy Grub compatibility",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_GRUB_COMPAT
  },
  [DxeMemoryProtectionSettingsOff] =                 {
    .Name        = "Off",
    .Description = "Disables all memory protection settings",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_OFF
  }
};

MM_MEMORY_PROTECTION_PROFILES  MmMemoryProtectionProfiles[MmMemoryProtectionSettingsMax] = {
  [MmMemoryProtectionSettingsDebug] =   {
    .Name        = "Debug",
    .Description = "Development profile ideal for debug scenarios",
    .Settings    = MM_MEMORY_PROTECTION_SETTINGS_DEBUG
  },
  [MmMemoryProtectionSettingsRelease] = {
    .Name        = "Release",
    .Description = "Release profile recommended for production scenarios",
    .Settings    = MM_MEMORY_PROTECTION_SETTINGS_PROD_MODE
  },
  [MmMemoryProtectionSettingsPcd] =     {
    .Name        = "Pcd",
    .Description = "Memory protection settings from PCDs",
    .Settings    = MM_MEMORY_PROTECTION_SETTINGS_PCD
  },
  [MmMemoryProtectionSettingsOff] =     {
    .Name        = "Off",
    .Description = "Disables all memory protection settings",
    .Settings    = MM_MEMORY_PROTECTION_SETTINGS_OFF
  }
};

/////////////////////////////////////
//    GET/SET SUPPORT FUNCTIONS    //
/////////////////////////////////////

/**
  Gets the memory protection HOB entry. This function will create the entry
  if it is not found.

  @retval NULL    Unable to create the memory protection HOB entry.
  @retval Other   Pointer to the memory protection HOB entry.
**/
STATIC
MEMORY_PROTECTION_SETTINGS_PRIVATE *
GetOrCreateMemoryProtectionSettings (
  VOID
  )
{
  VOID                                *Ptr;
  MEMORY_PROTECTION_SETTINGS_PRIVATE  Mpsp;

  Ptr = GetFirstGuidHob (&gMemoryProtectionSettingsGuid);

  if (Ptr != NULL) {
    return (MEMORY_PROTECTION_SETTINGS_PRIVATE *)GET_GUID_HOB_DATA (Ptr);
  }

  ZeroMem (&Mpsp, sizeof (Mpsp));
  Mpsp.Mps.Dxe                                 = DxeMemoryProtectionProfiles[DxeMemoryProtectionSettingsPcd].Settings;
  Mpsp.Mps.Mm                                  = MmMemoryProtectionProfiles[MmMemoryProtectionSettingsPcd].Settings;
  Mpsp.Mps.Dxe.StackExecutionProtectionEnabled = PcdGetBool (PcdSetNxForStack);

  Ptr = BuildGuidDataHob (
          &gMemoryProtectionSettingsGuid,
          &Mpsp,
          sizeof (Mpsp)
          );

  return (MEMORY_PROTECTION_SETTINGS_PRIVATE *)Ptr;
}

/**
  Gets the memory protection HOB entry and checks the version number
  to ensure it is compatible with this module.

  @retval NULL    Unable to create the memory protection HOB entry.
  @retval Other   Pointer to the memory protection HOB entry.
**/
STATIC
MEMORY_PROTECTION_SETTINGS_PRIVATE  *
FetchAndCheckMpsp (
  VOID
  )
{
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  Mpsp = GetOrCreateMemoryProtectionSettings ();

  if (Mpsp == NULL) {
    return NULL;
  }

  if (Mpsp->Mps.Dxe.StructVersion != DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Version number of the DXE Memory Protection Settings is invalid!\n"
      "This module was compiled with version %d but the current version is %d.\n",
      __func__,
      Mpsp->Mps.Dxe.StructVersion,
      DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION
      ));
    ASSERT (Mpsp->Mps.Dxe.StructVersion == DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION);
    return NULL;
  }

  if (Mpsp->Mps.Mm.StructVersion != MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Version number of the MM Memory Protection Settings is invalid!\n"
      "This module was compiled with version %d but the current version is %d.\n",
      __func__,
      Mpsp->Mps.Mm.StructVersion,
      MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION
      ));
    ASSERT (Mpsp->Mps.Mm.StructVersion == MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION);
    return NULL;
  }

  return Mpsp;
}

/**
  Prevent further changes to the memory protection settings via this
  library API.

  @retval EFI_SUCCESS           The memory protection settings are locked.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
LockMemoryProtectionSettings (
  VOID
  )
{
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  Mpsp = FetchAndCheckMpsp ();

  if (Mpsp  == NULL) {
    ASSERT (Mpsp != NULL);
    return EFI_ABORTED;
  }

  Mpsp->MemoryProtectionSettingsLocked = TRUE;

  return EFI_SUCCESS;
}

/**
  Sets the DXE memory protection settings. If DxeMps is NULL, the settings will be set based
  on ProfileIndex.

  @param[in] DxeMps        Pointer to the memory protection settings to publish. If NULL, the
                           settings will be created based on ProfileIndex.
  @param[in] ProfileIndex  The index of the memory protection profile to use if DxeMps is NULL.

  @retval EFI_SUCCESS           The memory protection HOB was successfully created.
  @retval EFI_INVALID_PARAMETER The ProfileIndex was invalid or the version number of the
                                input DxeMps was not equal to the version currently present
                                in the settings.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_ACCESS_DENIED     The memory protection settings are locked.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
SetDxeMemoryProtectionSettings (
  IN DXE_MEMORY_PROTECTION_SETTINGS       *DxeMps OPTIONAL,
  IN DXE_MEMORY_PROTECTION_PROFILE_INDEX  ProfileIndex
  )
{
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  Mpsp = FetchAndCheckMpsp ();

  if (Mpsp == NULL) {
    ASSERT (Mpsp != NULL);
    return EFI_ABORTED;
  }

  if (Mpsp->MemoryProtectionSettingsLocked) {
    return EFI_ACCESS_DENIED;
  }

  if (DxeMps == NULL) {
    if (ProfileIndex >= DxeMemoryProtectionSettingsMax) {
      return EFI_INVALID_PARAMETER;
    }

    DxeMps = &DxeMemoryProtectionProfiles[ProfileIndex].Settings;
  } else if (DxeMps->StructVersion != Mpsp->Mps.Dxe.StructVersion) {
    ASSERT (DxeMps->StructVersion == Mpsp->Mps.Dxe.StructVersion);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&Mpsp->Mps.Dxe, DxeMps, sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
  return EFI_SUCCESS;
}

/**
  Sets the MM memory protection HOB entry. If MmMps is NULL, the settings will be set based
  on ProfileIndex.

  @param[in] MmMps         Pointer to the memory protection settings to publish. If NULL, the
                           settings will be created based on ProfileIndex.
  @param[in] ProfileIndex  The index of the memory protection profile to use if MmMps is NULL.

  @retval EFI_SUCCESS           The memory protection HOB was successfully created.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to create the HOB.
  @retval EFI_INVALID_PARAMETER The ProfileIndex was invalid or the version number of the
                                input MmMps was not equal to the version currently present
                                in the settings.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_ACCESS_DENIED     The memory protection settings are locked.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
SetMmMemoryProtectionSettings (
  IN MM_MEMORY_PROTECTION_SETTINGS       *MmMps OPTIONAL,
  IN MM_MEMORY_PROTECTION_PROFILE_INDEX  ProfileIndex
  )
{
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  Mpsp = FetchAndCheckMpsp ();

  if (Mpsp == NULL) {
    ASSERT (Mpsp != NULL);
    return EFI_ABORTED;
  }

  if (Mpsp->MemoryProtectionSettingsLocked) {
    return EFI_ACCESS_DENIED;
  }

  if (MmMps == NULL) {
    if (ProfileIndex >= MmMemoryProtectionSettingsMax) {
      return EFI_INVALID_PARAMETER;
    }

    MmMps = &MmMemoryProtectionProfiles[ProfileIndex].Settings;
  } else if (MmMps->StructVersion != Mpsp->Mps.Mm.StructVersion) {
    ASSERT (MmMps->StructVersion == Mpsp->Mps.Mm.StructVersion);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&Mpsp->Mps.Mm, MmMps, sizeof (MM_MEMORY_PROTECTION_SETTINGS));
  return EFI_SUCCESS;
}

/**
  Copies the current memory protection settings into the input buffer.

  NOTE: The returned settings may not be the final settings used by the
        platform on this boot. Unless LockMemoryProtectionSettings() has
        been called, settings may be modified by drivers until DXE handoff.

  @param[out] Mps  The memory protection settings pointer to populate.

  @retval EFI_SUCCESS           The memory protection settings were copied
                                into the input buffer.
  @retval EFI_INVALID_PARAMETER Mps was NULL.
  @retval EFI_ABORTED           Unable to get/create the memory protection settings.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
GetCurrentMemoryProtectionSettings (
  OUT MEMORY_PROTECTION_SETTINGS  *Mps
  )
{
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  if (Mps == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Mpsp = FetchAndCheckMpsp ();

  if (Mpsp == NULL) {
    ASSERT (Mpsp != NULL);
    return EFI_ABORTED;
  }

  CopyMem (Mps, &Mpsp->Mps, sizeof (MEMORY_PROTECTION_SETTINGS));
  return EFI_SUCCESS;
}

/**
  Returns TRUE any form of DXE memory protection is currently active.

  NOTE: The returned value may reflect the final settings used by the
        platform on this boot. Unless LockMemoryProtectionSettings() has
        been called, settings may be modified by drivers until DXE handoff.

  @retval TRUE   DXE Memory protection is active.
  @retval FALSE  DXE Memory protection is not active.
**/
BOOLEAN
EFIAPI
IsDxeMemoryProtectionActive (
  VOID
  )
{
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  Mpsp = FetchAndCheckMpsp ();

  if (Mpsp == NULL) {
    ASSERT (Mpsp != NULL);
    return FALSE;
  }

  return Mpsp->Mps.Dxe.CpuStackGuardEnabled                                                                                                     ||
         Mpsp->Mps.Dxe.StackExecutionProtectionEnabled                                                                                          ||
         Mpsp->Mps.Dxe.NullPointerDetection.Enabled                                                                                             ||
         Mpsp->Mps.Dxe.HeapGuard.FreedMemoryGuardEnabled                                                                                        ||
         Mpsp->Mps.Dxe.ImageProtection.ProtectImageFromFv                                                                                       ||
         Mpsp->Mps.Dxe.ImageProtection.ProtectImageFromUnknown                                                                                  ||
         !IsZeroBuffer (&Mpsp->Mps.Dxe.ExecutionProtection.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE)                                         ||
         (Mpsp->Mps.Dxe.HeapGuard.PageGuardEnabled && !IsZeroBuffer (&Mpsp->Mps.Dxe.PageGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE))     ||
         (Mpsp->Mps.Dxe.HeapGuard.PoolGuardEnabled && !IsZeroBuffer (&Mpsp->Mps.Dxe.PoolGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE));
}

/**
  Returns TRUE any form of MM memory protection is currently active.

  NOTE: The returned value may reflect the final settings used by the
        platform on this boot. Unless LockMemoryProtectionSettings() has
        been called, settings may be modified by drivers until DXE handoff.

  @retval TRUE   MM Memory protection is active.
  @retval FALSE  MM Memory protection is not active.
**/
BOOLEAN
EFIAPI
IsMmMemoryProtectionActive (
  VOID
  )
{
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  Mpsp = FetchAndCheckMpsp ();

  if (Mpsp == NULL) {
    ASSERT (Mpsp != NULL);
    return FALSE;
  }

  return Mpsp->Mps.Mm.NullPointerDetection.Enabled                                                                                          ||
         (Mpsp->Mps.Mm.HeapGuard.PageGuardEnabled && !IsZeroBuffer (&Mpsp->Mps.Mm.PageGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE))   ||
         (Mpsp->Mps.Dxe.HeapGuard.PoolGuardEnabled && !IsZeroBuffer (&Mpsp->Mps.Mm.PoolGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE));
}

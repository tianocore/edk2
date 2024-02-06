/** @file
Defines memory protection settings guid and struct for DXE and MM.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEMORY_PROTECTION_SETTINGS_H_
#define MEMORY_PROTECTION_SETTINGS_H_

#define OEM_RESERVED_MPS_MEMORY_TYPE  EfiMaxMemoryType
#define OS_RESERVED_MPS_MEMORY_TYPE   (EfiMaxMemoryType + 1)
#define MAX_MPS_MEMORY_TYPE           (EfiMaxMemoryType + 2)
#define MPS_MEMORY_TYPE_BUFFER_SIZE   (MAX_MPS_MEMORY_TYPE * sizeof (BOOLEAN))

// Current DXE iteration of MEMORY_PROTECTION_SETTINGS
#define DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION  1

// Current MM iteration of MEMORY_PROTECTION_SETTINGS
#define MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION  1

#define DXE_MEMORY_PROTECTION_SIGNATURE  SIGNATURE_32('D', 'M', 'P', 'S')
#define MM_MEMORY_PROTECTION_SIGNATURE   SIGNATURE_32('M', 'M', 'P', 'S')

typedef UINT8   MEMORY_PROTECTION_SETTINGS_VERSION;
typedef UINT32  MEMORY_PROTECTION_SETTINGS_SIGNATURE;

typedef struct {
  BOOLEAN    Enabled            : 1;
  BOOLEAN    DisableEndOfDxe    : 1;
  BOOLEAN    NonstopModeEnabled : 1;
} DXE_NULL_DETECTION_POLICY;

typedef struct {
  BOOLEAN    ProtectImageFromUnknown : 1;
  BOOLEAN    ProtectImageFromFv      : 1;
} DXE_IMAGE_PROTECTION_POLICY;

typedef struct {
  BOOLEAN    PageGuardEnabled        : 1;
  BOOLEAN    PoolGuardEnabled        : 1;
  BOOLEAN    FreedMemoryGuardEnabled : 1;
  BOOLEAN    NonstopModeEnabled      : 1;
  BOOLEAN    GuardAlignedToTail      : 1;
} DXE_HEAP_GUARD_POLICY;

typedef struct {
  BOOLEAN    Enabled            : 1;
  BOOLEAN    NonstopModeEnabled : 1;
} MM_NULL_DETECTION_POLICY;

typedef struct {
  BOOLEAN    PageGuardEnabled   : 1;
  BOOLEAN    PoolGuardEnabled   : 1;
  BOOLEAN    NonstopModeEnabled : 1;
  BOOLEAN    GuardAlignedToTail : 1;
} MM_HEAP_GUARD_POLICY;

typedef struct {
  BOOLEAN    EnabledForType[MAX_MPS_MEMORY_TYPE];
} MPS_MEMORY_TYPES;

//
// Memory Protection Settings struct
//
typedef struct {
  // This signature is used to identify the memory protection settings structure.
  MEMORY_PROTECTION_SETTINGS_SIGNATURE    Signature;

  // The current version of the structure definition. This is used to ensure there isn't a
  // definition mismatch if modules have differing iterations of this header. When creating
  // this struct, use the DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION macro.
  MEMORY_PROTECTION_SETTINGS_VERSION      StructVersion;

  // If enabled, the page at the top of the stack will be invalidated to catch stack overflow.
  BOOLEAN                                 CpuStackGuardEnabled;

  // If enabled, the stack will be marked non-executable.
  BOOLEAN                                 StackExecutionProtectionEnabled;

  // If enabled, accessing the NULL address in UEFI will be caught by marking
  // the NULL page as not present.
  //   .NullDetectionEnabled    : Enable NULL pointer detection.
  //   .DisableEndOfDxe         : Disable NULL pointer detection just after EndOfDxe.
  //                              This is a workaround for those unsolvable NULL access issues in
  //                              OptionROM, boot loader, etc. It can also help to avoid unnecessary
  //                              exception caused by legacy memory (0-4095) access after EndOfDxe,
  //                              such as Windows 7 boot on Qemu.
  //   .NonstopModeEnabled      : If enabled the debug flag will be raised when a fault occurs
  //                              to break into debugger.
  DXE_NULL_DETECTION_POLICY    NullPointerDetection;

  // Set image protection policy.
  //
  //  .ProtectImageFromUnknown          : If set, images from unknown devices will be protected by
  //                                      DxeCore if they are aligned. The code section becomes
  //                                      read-only, and the data section becomes non-executable.
  //  .ProtectImageFromFv               : If set, images from firmware volumes will be protected by
  //                                      DxeCore if they are aligned. The code section becomes
  //                                      read-only, and the data section becomes non-executable.
  DXE_IMAGE_PROTECTION_POLICY    ImageProtection;

  // If a bit is set, memory regions of the associated type will be mapped non-executable.
  //
  // The execution protection setting for EfiBootServicesData and EfiConventionalMemory must
  // be the same.
  MPS_MEMORY_TYPES               ExecutionProtection;

  //  Configures general heap guard behavior.
  //
  //  .PageGuardEnabled         : Enable page guard.
  //  .PoolGuardEnabled         : Enable pool guard.
  //  .FreedMemoryGuardEnabled  : Enable freed-memory guard (Use-After-Free memory detection).
  //  .NonstopModeEnabled       : If enabled the debug flag will be raised when a fault occurs
  //                              to break into debugger.
  //  .GuardAlignedToTail       : TRUE if the pool is aligned to tail guard page. If FALSE, the
  //                              pool is aligned to head guard page.
  //
  //  Note:
  //  a) Due to the limit of pool memory implementation and the alignment
  //     requirement of UEFI spec, HeapGuard.GuardAlignedToTail is a try-best
  //     setting which cannot guarantee that the returned pool is exactly
  //     adjacent to head or tail guard page.
  //  b) Freed-memory guard and pool/page guard cannot be enabled
  //     at the same time.
  DXE_HEAP_GUARD_POLICY    HeapGuard;

  // Indicates which type allocation need guard page.
  //
  // If bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages which the allocated pool occupies,
  // if there's enough free memory for all of them.
  //
  // These settings are only valid if HeapGuard.PoolGuardEnabled is TRUE.
  MPS_MEMORY_TYPES    PoolGuard;

  // Indicates which type allocation need guard page.
  //
  // If a bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages allocated if there's enough
  // free pages for all of them.
  //
  // These settings are only valid if HeapGuard.PageGuardEnabled is TRUE.
  MPS_MEMORY_TYPES    PageGuard;
} DXE_MEMORY_PROTECTION_SETTINGS;

//
// Memory Protection Settings struct
//
typedef struct {
  // This signature is used to identify the memory protection settings structure.
  MEMORY_PROTECTION_SETTINGS_SIGNATURE    Signature;

  // The current version of the structure definition. This is used to ensure there isn't a
  // definition mismatch if modules have differing iterations of this header. When creating
  // this struct, use the MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION macro.
  MEMORY_PROTECTION_SETTINGS_VERSION      StructVersion;

  // If enabled, accessing the NULL address in MM will be caught by marking
  // the NULL page as not present.
  //   .NullDetectionEnabled    : Enable NULL pointer detection.
  //   .NonstopModeEnabled      : If enabled the debug flag will be raised when a fault occurs
  //                              to break into debugger.
  MM_NULL_DETECTION_POLICY                NullPointerDetection;

  //  Configures general heap guard behavior.
  //
  // Note:
  //  a) Due to the limit of pool memory implementation and the alignment
  //     requirement of UEFI spec, HeapGuard.GuardAlignedToTail is a try-best
  //     setting which cannot guarantee that the returned pool is exactly
  //     adjacent to head or tail guard page.
  //
  //  .PageGuardEnabled          : Enable page guard.
  //  .PoolGuardEnabled          : Enable pool guard.
  //  .NonstopModeEnabled        : If enabled the debug flag will be raised when a fault occurs
  //                               to break into debugger.
  //  .GuardAlignedToTail        : TRUE if the pool is aligned to tail guard page. If FALSE, the
  //                               pool is aligned to head guard page.
  MM_HEAP_GUARD_POLICY    HeapGuard;

  // Indicates which type allocation need guard page.
  //
  // If bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages which the allocated pool occupies,
  // if there's enough free memory for all of them.
  //
  // These settings are only valid if PoolGuardEnabled is TRUE in HeapGuard.
  MPS_MEMORY_TYPES    PoolGuard;

  // Indicates which type allocation need guard page.
  //
  // If a bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages allocated if there's enough
  // free pages for all of them.
  //
  // This bitfield is only valid if PageGuardEnabled is TRUE in HeapGuard.
  MPS_MEMORY_TYPES    PageGuard;
} MM_MEMORY_PROTECTION_SETTINGS;

typedef struct {
  // The memory protection settings in the SMM and Standalone MM environment
  MM_MEMORY_PROTECTION_SETTINGS     Mm;
  // The memory protection settings in the DXE environment
  DXE_MEMORY_PROTECTION_SETTINGS    Dxe;
} MEMORY_PROTECTION_SETTINGS;

#define MEMORY_PROTECTION_SETTINGS_GUID  \
  { \
    { 0x9ABFD639, 0xD1D0, 0x4EFF, { 0xBD, 0xB6, 0x7E, 0xC4, 0x19, 0x0D, 0x17, 0xD5 } } \
  }

extern GUID  gMemoryProtectionSettingsGuid;

#endif

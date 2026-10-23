/** @file

  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#pragma once

#include <AcpiObjects.h>
#include <StandardNameSpaceObjects.h>

#pragma pack(1)

/** The ELOONGARCH64_OBJECT_ID enum describes the Object IDs
    in the LoongArch64 Namespace.
*/
typedef enum LoongArch64ObjectID {
  ELoongArch64ObjReserved,     ///< 0 - Reserved
  ELoongArch64ObjMadtInfo,     ///< 1 - MADT table information
  ELoongArch64ObjCorePicInfo,  ///< 2 - Core Programmable Interrupt Controller Info
  ELoongArch64ObjLioPicInfo,   ///< 3 - Legacy I/O Programmable Interrupt Controller Info
  ELoongArch64ObjHtPicInfo,    ///< 4 - HyperTransport Programmable Interrupt Controller Info
  ELoongArch64ObjEioPicInfo,   ///< 5 - Extend I/O Programmable Interrupt Controller Info
  ELoongArch64ObjMsiPicInfo,   ///< 6 - MSI Programmable Interrupt Controller Info
  ELoongArch64ObjBioPicInfo,   ///< 7 - Bridge I/O Programmable Interrupt Controller Info
  ELoongArch64ObjLpcPicInfo,   ///< 8 - Low Pin Count Programmable Interrupt Controller Info
  ELoongArch64ObjMax
} ELOONGARCH64_OBJECT_ID;

/** A structure that describes MADT table-wide LoongArch64 information.

    ID: ELoongArch64ObjMadtInfo
*/
typedef struct CmLoongArch64MadtInfo {
  /// Address of the local interrupt controller.
  UINT32    LocalApicAddress;

  /// MADT flags.
  UINT32    Flags;
} CM_LOONGARCH64_MADT_INFO;

/** A structure that describes a Core PIC.

    ID: ELoongArch64ObjCorePicInfo
*/
typedef struct CmLoongArch64CorePicInfo {
  /// PIC structure version. Currently, ACPI defines only version 1.
  UINT8     Version;

  /// ACPI processor UID matching the processor device's _UID.
  UINT32    AcpiProcessorUid;

  /// Physical processor ID. MAX_UINT32 is only valid for a disabled processor.
  UINT32    CoreId;

  /// CORE PIC flags. BIT0 is Enabled; all other bits are reserved.
  UINT32    Flags;
} CM_LOONGARCH64_CORE_PIC_INFO;

/** A structure that describes a LIO PIC.

    ID: ELoongArch64ObjLioPicInfo
*/
typedef struct CmLoongArch64LioPicInfo {
  /// PIC structure version. Currently, ACPI defines only version 1.
  UINT8     Version;

  /// Base address
  UINT64    Address;

  /// Register space size
  UINT16    Size;

  /// Cascade vector
  UINT8     Cascade[2];

  /// Cascade map
  UINT32    CascadeMap[2];
} CM_LOONGARCH64_LIO_PIC_INFO;

/** A structure that describes a HT PIC.

    ID: ELoongArch64ObjHtPicInfo
*/
typedef struct CmLoongArch64HtPicInfo {
  /// PIC structure version. Currently, ACPI defines only version 1.
  UINT8     Version;

  /// Base address
  UINT64    Address;

  /// Register space size
  UINT16    Size;

  /// Cascade vector
  UINT8     Cascade[8];
} CM_LOONGARCH64_HT_PIC_INFO;

/** A structure that describes an EIO PIC.

    ID: ELoongArch64ObjEioPicInfo
*/
typedef struct CmLoongArch64EioPicInfo {
  /// PIC structure version. Currently, ACPI defines only version 1.
  UINT8     Version;

  /// Cascade vector
  UINT8     Cascade;

  /// Node ID
  UINT8     Node;

  /// Node map
  UINT64    NodeMap;
} CM_LOONGARCH64_EIO_PIC_INFO;

/** A structure that describes a MSI PIC.

    ID: ELoongArch64ObjMsiPicInfo
*/
typedef struct CmLoongArch64MsiPicInfo {
  /// PIC structure version. Currently, ACPI defines only version 1.
  UINT8     Version;

  /// MSI message address
  UINT64    MsgAddress;

  /// MSI vector base
  UINT32    Start;

  /// MSI vector count
  UINT32    Count;
} CM_LOONGARCH64_MSI_PIC_INFO;

/** A structure that describes a BIO PIC.

    ID: ELoongArch64ObjBioPicInfo
*/
typedef struct CmLoongArch64BioPicInfo {
  /// PIC structure version. Currently, ACPI defines only version 1.
  UINT8     Version;

  /// Base address
  UINT64    Address;

  /// Register space size
  UINT16    Size;

  /// Controller ID
  UINT16    Id;

  /// GSI base
  UINT16    GsiBase;
} CM_LOONGARCH64_BIO_PIC_INFO;

/** A structure that describes a LPC PIC.

    ID: ELoongArch64ObjLpcPicInfo
*/
typedef struct CmLoongArch64LpcPicInfo {
  /// PIC structure version. Currently, ACPI defines only version 1.
  UINT8     Version;

  /// Base address
  UINT64    Address;

  /// Register space size
  UINT16    Size;

  /// Cascade vector
  UINT8     Cascade;
} CM_LOONGARCH64_LPC_PIC_INFO;

#pragma pack()

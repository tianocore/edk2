/** @file

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef RISCV_NAMESPACE_OBJECTS_H_
#define RISCV_NAMESPACE_OBJECTS_H_

#include <AcpiObjects.h>
#include <StandardNameSpaceObjects.h>

#pragma pack(1)

/** The ERISCV_OBJECT_ID enum describes the Object IDs
    in the RISCV Namespace

  Note: Whenever an entry in this enum is updated,
        the following data structures must also be
        updated:
        - CM_OBJECT_TOKEN_FIXER TokenFixer[] in
          Library\Common\DynamicPlatRepoLib\CmObjectTokenFixer.c
*/
typedef enum RiscVObjectID {
  ERiscVObjReserved,                                             ///< 0 - Reserved
  ERiscVObjRintcInfo,                                            ///< 1 - RISC-V RINTC Info
  ERiscVObjImsicInfo,                                            ///< 2 - RISC-V IMSIC Info
  ERiscVObjAplicInfo,                                            ///< 3 - RISC-V APLIC Frame Info
  ERiscVObjPlicInfo,                                             ///< 4 - RISC-V PLIC Info
  ERiscVObjIsaStringInfo,                                        ///< 5 - RISC-V ISA String Info
  ERiscVObjCmoInfo,                                              ///< 6 - RISC-V CMO Info
  ERiscVObjMmuInfo,                                              ///< 7 - RISC-V MMU Type Info
  ERiscVObjTimerInfo,                                            ///< 8 - RISC-V Timer Type Info
  ERiscVObjMax
} ERISCV_OBJECT_ID;

/** A structure that describes the
    RINTC for the Platform.

    ID: ERiscVObjRintcInfo
*/
typedef struct CmRiscVRintcInfo {
  /// Version
  UINT8              Version;

  /// Reserved1
  UINT8              Reserved1;

  /** The flags field as described by the RINTC structure
      in the ACPI Specification.
  */
  UINT32             Flags;

  // Hart ID
  UINT64             HartId;

  /** The ACPI Processor UID. This must match the
      _UID of the CPU Device object information described
      in the DSDT/SSDT for the CPU.
  */
  UINT32             AcpiProcessorUid;

  // External Interrupt Controller ID
  UINT32             ExtIntCId;

  // IMSIC Base address
  UINT64             ImsicBaseAddress;

  // IMSIC Size
  UINT32             ImsicSize;

  // riscv,intc phandle
  INT32              IntcPhandle;

  /** The proximity domain to which the logical processor belongs.
      This field is used to populate the RINTC affinity structure
      in the SRAT table.
  */
  UINT32             ProximityDomain;

  /** The clock domain to which the logical processor belongs.
      This field is used to populate the RINTC affinity structure
      in the SRAT table.
  */
  UINT32             ClockDomain;

  /** The GICC Affinity flags field as described by the RINTC Affinity structure
      in the SRAT table.
  */
  UINT32             AffinityFlags;

  /** Optional field: Reference Token for the Cpc info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_CPC_INFO object.
  */
  CM_OBJECT_TOKEN    CpcToken;
} CM_RISCV_RINTC_INFO;

/** A structure that describes the
    IMSIC information for the Platform.

    ID: ERiscVObjImsicInfo
*/
typedef struct CmRiscVImsicInfo {
  /// Version
  UINT8     Version;

  /// Reserved1
  UINT8     Reserved1;

  /** The flags field as described by the IMSIC structure
      in the ACPI Specification.
  */
  UINT32    Flags;

  // Number of S-mode Interrupt Identities
  UINT16    NumIds;

  // Number of guest mode Interrupt Identities
  UINT16    NumGuestIds;

  // Guest Index Bits
  UINT8     GuestIndexBits;

  // Hart Index Bits
  UINT8     HartIndexBits;

  // Group Index Bits
  UINT8     GroupIndexBits;

  // Group Index Shift
  UINT8     GroupIndexShift;
} CM_RISCV_IMSIC_INFO;

/** A structure that describes the
    APLIC information for the Platform.

    ID: ERiscVObjAplicInfo
*/
typedef struct CmRiscVAplicInfo {
  /// Version
  UINT8     Version;

  /// APLIC ID
  UINT8     AplicId;

  /** The flags field as described by the APLIC structure
      in the ACPI Specification.
  */
  UINT32    Flags;

  /// Hardware ID
  UINT8     HwId[8];

  // Number of IDCs
  UINT16    NumIdcs;

  // Number of Interrupt Sources
  UINT16    NumSources;

  /// GSI Base
  UINT32    GsiBase;

  /// APLIC Address
  UINT64    AplicAddress;

  /// APLIC size
  UINT32    AplicSize;

  /// APLIC Phandle
  INT32     Phandle;
} CM_RISCV_APLIC_INFO;

/** A structure that describes the
    PLIC information for the Platform.

    ID: ERiscVObjPlicInfo
*/
typedef struct CmRiscVPlicInfo {
  /// Version
  UINT8     Version;

  /// PLIC ID
  UINT8     PlicId;

  /// Hardware ID
  UINT8     HwId[8];

  // Number of Interrupt Sources
  UINT16    NumSources;

  // Max Priority
  UINT16    MaxPriority;

  /** The flags field as described by the PLIC structure
      in the ACPI Specification.
  */
  UINT32    Flags;

  /// PLIC Size
  UINT32    PlicSize;

  /// PLIC Address
  UINT64    PlicAddress;

  /// GSI Base
  UINT32    GsiBase;

  /// PLIC Phandle
  INT32     Phandle;
} CM_RISCV_PLIC_INFO;

/** A structure that describes the
    ISA string for the Platform.

    ID: ERiscVObjIsaStringInfo
*/
typedef struct CmRiscVIsaStringInfo {
  UINT16    Length;

  CHAR8     *IsaString;
} CM_RISCV_ISA_STRING_NODE;

/** A structure that describes the
    CMO for the Platform.

    ID: ERiscVObjCmoInfo
*/
typedef struct CmRiscVCmoInfo {
  /// CbomBlockSize
  UINT8    CbomBlockSize;

  /// CbopBlockSize
  UINT8    CbopBlockSize;

  /// CbozBlockSize
  UINT8    CbozBlockSize;
} CM_RISCV_CMO_NODE;

/** A structure that describes the
    Timer for the Platform.

    ID: ERiscVObjTimerInfo
*/
typedef struct CmRiscVTimerInfo {
  UINT8     TimerCannotWakeCpu;

  UINT64    TimeBaseFrequency;
} CM_RISCV_TIMER_INFO;

#pragma pack()

#endif // RISCV_NAMESPACE_OBJECTS_H_

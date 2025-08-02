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

#define MAX_ISA_STRING_LENGTH  1024
#define RISCV_HWID_LENGTH      8

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
  UINT32             ExtIntcId;

  // IMSIC Base address
  UINT64             ImsicBaseAddress;

  // IMSIC Size
  UINT32             ImsicSize;

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

  /** The RINTC Affinity flags field as described by the RINTC Affinity structure
      in the SRAT table.
  */
  UINT32             AffinityFlags;

  // Reference Token for the Rintc info of this processor.
  CM_OBJECT_TOKEN    CpcToken;
} CM_RISCV_RINTC_INFO;

/** A structure that describes the
    IMSIC information for the Platform.

    ID: ERiscVObjImsicInfo
*/
typedef struct CmRiscVImsicInfo {
  /// Version
  UINT8     Version;

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

typedef struct CmRiscVPlicAplicCommonInfo {
  /// Version
  UINT8     Version;

  /// PLIC/APLIC ID
  UINT8     Id;

  // Number of Interrupt Sources
  UINT16    NumSources;

  /** The flags field as described by the PLIC/APLIC structure
      in the ACPI Specification.
  */
  UINT32    Flags;

  /// Hardware ID
  UINT8     HwId[RISCV_HWID_LENGTH];

  /// Base address
  UINT64    BaseAddress;

  /// Size
  UINT32    Size;

  /// GSI Base
  UINT32    GsiBase;
} PLIC_APLIC_COMMON_INFO;

/** A structure that describes the
    APLIC information for the Platform.

    ID: ERiscVObjAplicInfo
*/
typedef struct CmRiscVAplicInfo {
  PLIC_APLIC_COMMON_INFO    PlicAplicCommonInfo;
  // Number of IDCs
  UINT16                    NumIdcs;
} CM_RISCV_APLIC_INFO;

/** A structure that describes the
    PLIC information for the Platform.

    ID: ERiscVObjPlicInfo
*/
typedef struct CmRiscVPlicInfo {
  PLIC_APLIC_COMMON_INFO    PlicAplicCommonInfo;
  // Max Priority
  UINT16                    MaxPriority;
} CM_RISCV_PLIC_INFO;

/** A structure that describes the
    ISA string for the Platform.

    ID: ERiscVObjIsaStringInfo
*/
typedef struct CmRiscVIsaStringInfo {
  // ISA Length
  UINT16    Length;

  // ISA String
  CHAR8     *IsaString;
} CM_RISCV_ISA_STRING_NODE;

/** A structure that describes the
    CMO for the Platform.

    ID: ERiscVObjCmoInfo
*/
typedef struct CmRiscVCmoInfo {
  /// Cbom Block Size
  UINT8    CbomBlockSize;

  /// Cbop Block Size
  UINT8    CbopBlockSize;

  /// Cboz Block Size
  UINT8    CbozBlockSize;
} CM_RISCV_CMO_NODE;

/** A structure that describes the
    Timer for the Platform.

    ID: ERiscVObjTimerInfo
*/
typedef struct CmRiscVTimerInfo {
  // Timer cannot wake up CPU capability
  UINT8     TimerCannotWakeCpu;

  // Time Base Frequency
  UINT64    TimeBaseFrequency;
} CM_RISCV_TIMER_INFO;

#pragma pack()

#endif // RISCV_NAMESPACE_OBJECTS_H_

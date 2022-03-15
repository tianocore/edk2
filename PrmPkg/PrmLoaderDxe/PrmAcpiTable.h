/** @file

  Definition for the Platform Runtime Mechanism (PRM) ACPI table (PRMT).

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRMT_ACPI_TABLE_H_
#define PRMT_ACPI_TABLE_H_

#include <Base.h>
#include <IndustryStandard/Acpi10.h>

#define PRM_TABLE_SIGNATURE  SIGNATURE_32 ('P', 'R', 'M', 'T')

#define PRM_TABLE_REVISION                       0x0
#define PRM_MODULE_INFORMATION_STRUCT_REVISION   0x00
#define PRM_HANDLER_INFORMATION_STRUCT_REVISION  0x00

#pragma pack(push, 1)

//
// Platform Runtime Mechanism (PRM) ACPI Table (PRMT) structures
//
typedef struct {
  UINT16    StructureRevision;                                    ///< Revision of this structure
  UINT16    StructureLength;                                      ///< Length in bytes of this structure
  GUID      Identifier;                                           ///< GUID of the PRM handler for this structure
  UINT64    PhysicalAddress;                                      ///< Physical address of this PRM handler
  UINT64    StaticDataBuffer;                                     ///< Physical address of the static data buffer for
                                                                  ///< this PRM handler (PRM_DATA_BUFFER *)
  UINT64    AcpiParameterBuffer;                                  ///< Physical address of the parameter buffer
                                                                  ///< for this PRM handler (PRM_DATA_BUFFER *)
                                                                  ///< that is only used in the case of _DSM invocation.
                                                                  ///< If _DSM invocation is not used, this value is
                                                                  ///< ignored.
} PRM_HANDLER_INFORMATION_STRUCT;

typedef struct {
  UINT16                            StructureRevision;            ///< Revision of this structure
  UINT16                            StructureLength;              ///< Length in bytes of this structure including the
                                                                  ///< variable length PRM Handler Info array
  GUID                              Identifier;                   ///< GUID of the PRM module for this structure
  UINT16                            MajorRevision;                ///< PRM module major revision
  UINT16                            MinorRevision;                ///< PRM module minor revision
  UINT16                            HandlerCount;                 ///< Number of entries in the Handler Info array
  UINT32                            HandlerInfoOffset;            ///< Offset in bytes from the beginning of this
                                                                  ///< structure to the Handler Info array
  UINT64                            RuntimeMmioRanges;            ///< Physical address of the PRM MMIO Ranges
                                                                  ///< structure (PRM_MODULE_RUNTIME_MMIO_RANGES *)
  PRM_HANDLER_INFORMATION_STRUCT    HandlerInfoStructure[1];
} PRM_MODULE_INFORMATION_STRUCT;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER      Header;                        ///< Standard ACPI description header
  GUID                             PrmPlatformGuid;               ///< A GUID that uniquely identifies this platform.
                                                                  ///< Used to check for compatibility in PRM module
                                                                  ///< runtime updates.
  UINT32                           PrmModuleInfoOffset;           ///< Offset in bytes from the beginning of this
                                                                  ///< structure to the PRM Module Info array
  UINT32                           PrmModuleInfoCount;            ///< Number of entries in the PRM Module Info array
  PRM_MODULE_INFORMATION_STRUCT    PrmModuleInfoStructure[1];
} PRM_ACPI_DESCRIPTION_TABLE;

#pragma pack(pop)

//
// Helper macros to build PRM Information structures
//
// Todo: Revisit whether to use; currently both macros are not used
//
#define PRM_MODULE_INFORMATION_STRUCTURE(ModuleGuid, ModuleRevision, HandlerCount, PrmHanderInfoStructureArray)  {                      \
    {                                                                                                                                   \
      PRM_MODULE_INFORMATION_STRUCT_REVISION,                                                                                      /* UINT16    StructureRevision;   */                         \
      (OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoStructure) + (HandlerCount * sizeof (PRM_HANDLER_INFORMATION_STRUCT))) /* UINT16    StructureLength;     */ \
      ModuleGuid,                                                                                                                  /* GUID      ModuleGuid;          */                         \
      ModuleRevision,                                                                                                              /* UINT16    ModuleRevision       */                         \
      HandlerCount,                                                                                                                /* UINT16    HandlerCount         */                         \
      OFFSET_OF (PRM_MODULE_INFORMATION_STRUCT, HandlerInfoOffset),                                                                /* UINT32    HandlerInfoOffset    */                         \
      PrmHanderInfoStructureArray                                                                                                  /* PRM_HANDLER_INFORMATION_STRUCT HandlerInfoStructure */    \
    } \
  }

#define PRM_HANDLER_INFORMATION_STRUCTURE(HandlerGuid, PhysicalAddress)  {                                                  \
    {                                                                                                                              \
      PRM_HANDLER_INFORMATION_STRUCT_REVISION,                             /* UINT16                  StructureRevision;      */   \
      sizeof (PRM_HANDLER_INFORMATION_STRUCT),                             /* UINT16                  StructureLength;        */   \
      HandlerGuid,                                                         /* GUID                    HandlerGuid;            */   \
      PhysicalAddress,                                                     /* UINT64                  PhysicalAddress         */   \
    } \
  }

#endif // _PRMT_ACPI_TABLE_H_

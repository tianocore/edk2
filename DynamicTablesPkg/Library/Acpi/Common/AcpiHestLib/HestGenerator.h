/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#pragma once

#if !defined (MDE_CPU_IA32) && !defined (MDE_CPU_X64)
#define   HEST_IA32_SUBTABLE_UNSUPPORTED  TRUE
#else
#define   HEST_IA32_SUBTABLE_UNSUPPORTED  FALSE
#endif

typedef struct ErrSourceOps       ERR_SOURCE_OPS;
typedef struct AcpiHestGenerator  ACPI_HEST_GENERATOR;

/** Get Error source CM objects to generate HEST

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [out]      ErrSourceOps         Error Source Operation.

  @retval EFI_SUCCESS
  @retval Others                         Failed to initialise
**/
typedef
EFI_STATUS
(EFIAPI *GET_ERR_SOURCE_CM_OBJ)(
  IN            ACPI_HEST_GENERATOR                                    *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  OUT           ERR_SOURCE_OPS                                         *ErrSourceOps
  );

/** Get the additional size required for error source entry.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       ErrSourceCmObj       Error Source CM object.
  @param [out]      Size                 Additional size of Error source.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *GET_ADDITIONAL_SIZE_OF_ERR_SOURCE)(
  IN            ACPI_HEST_GENERATOR                                    *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST           CfgMgrProtocol,
  IN      CONST ERROR_SOURCE_COMMON_INFO                               *ErrSourceCmObj,
  OUT           UINT32                                                 *Size
  );

/** Add error source entry into HEST table.

  @param [in]       Generator            Pointer to the HEST Generator.
  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       SourceId             Error Source Id.
  @param [in]       ErrSourceOps         Error Source Operation.
  @param [in]       ErrSourceCmObj       Error Source CM Object.
  @param [out]      Hest                 HEST Table.

  @retval EFI_SUCCESS                    Machine Check Exception Error source generated successfully.
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *ADD_ERR_SOURCE)(
  IN            ACPI_HEST_GENERATOR                              *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN            UINT16                                           SourceId,
  IN      CONST ERR_SOURCE_OPS                                   *ErrSourceOps,
  IN      CONST ERROR_SOURCE_COMMON_INFO                         *ErrSourceCmObj,
  OUT           EFI_ACPI_6_6_HARDWARE_ERROR_SOURCE_TABLE_HEADER  *Hest
  );

/** A structure that describes operation relevant error source.
*/
typedef struct ErrSourceOps {
  /// Error Source Type
  UINT16                               ErrSourceType;

  /// Error Source Name
  CHAR16                               *ErrSourceName;

  /// Size of an error source sub-table
  UINT32                               ErrSourceSize;

  /// Size of CM object for Error Source
  UINT32                               ErrSourceCmObjSize;

  /// Supported Flags of Error Source
  UINT32                               Flags;

  /// Get CM objects for Error source.
  GET_ERR_SOURCE_CM_OBJ                GetErrSourceCmObj;

  /// Get additional size of Error source to generate Error source.
  GET_ADDITIONAL_SIZE_OF_ERR_SOURCE    GetAdditionalSizeofErrSource;

  /// Add Error source to HEST
  ADD_ERR_SOURCE                       AddErrSource;

  /// If FALSE, generate sub-tables for this error source type.
  BOOLEAN                              Unsupported;

  /// CM object list for Error Source.
  VOID                                 *ErrSourceCmObjList;

  /// CM object count for Error Source.
  UINT32                               ErrSourceCmObjCount;

  /// Offset to the start of the first object in the Hest table.
  UINT32                               ErrSourceOffset;
} ERR_SOURCE_OPS;

/** A structure that describes Source indexer
    used to get related source for GHES.
*/
typedef struct HestSourceIndexer {
  /// Index token for the Source
  CM_OBJECT_TOKEN    Token;

  /// Source offset from the start of the HEST table
  UINT32             Offset;

  /// Pointer to the Source
  VOID               *Object;

  /// Source Id
  UINT16             SourceId;
} HEST_SOURCE_INDEXER;

typedef struct HestSourceIndexerInfo {
  /// Pointer to the Source indexer array
  HEST_SOURCE_INDEXER    *SourceIndexer;

  /// Number of Source Indexer Count
  UINT32                 SourceIndexerCount;
} HEST_SOURCE_INDEXER_INFO;

typedef struct AcpiHestGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR        Header;

  /// Hest Generator private data

  /// Next source id for next error source.
  UINT16                      NextSourceId;

  /// Source indexers
  HEST_SOURCE_INDEXER_INFO    SourceIndexerInfos;
} ACPI_HEST_GENERATOR;

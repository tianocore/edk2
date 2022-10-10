/** @file

  Copyright (c) 2017 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef ACPI_TABLE_GENERATOR_H_
#define ACPI_TABLE_GENERATOR_H_

#include <IndustryStandard/Acpi.h>

// Module specific include files.
#include <TableGenerator.h>

#pragma pack(1)

/**
The Dynamic Tables Framework provisions two classes of ACPI table
generators.
 - Standard generators: The ACPI table generators implemented by the
                        Dynamic Tables Framework.
 - OEM generators:      The ACPI table generators customized by the OEM.

The Dynamic Tables Framework implements the following ACPI table generators:
  - RAW   : This is the simplest ACPI table generator. It simply installs
            the ACPI table provided in the AcpiTableData member of the
            CM_STD_OBJ_ACPI_TABLE_INFO. The ACPI table data is provided by
            the Configuration Manager and is generated using an implementation
            defined mechanism.
  - DSDT  : The DSDT generator is a clone of the RAW generator. The difference
            is in the way the ACPI Table Data is generated from an AML file.
  - SSDT  : The SSDT generator is a clone of the RAW generator. The difference
            is in the way the ACPI Table Data is generated from an AML file.
  - FADT  : The FADT generator collates the required platform information from
            the Configuration Manager and builds the FADT table.
  - MADT  : The MADT generator collates the GIC information  from the
            Configuration Manager and builds the MADT table.
  - GTDT  : The GTDT generator collates the Timer information from the
            Configuration Manager and builds the GTDT table.
  - DBG2  : The DBG2 generator collates the debug serial port information from
            the Configuration Manager and builds the DBG2 table.
  - SPCR  : The SPCR generator collates the serial port information from the
            Configuration Manager and builds the SPCR table.
  - MCFG  : The MCFG generator collates the PCI configuration space information
            from the Configuration Manager and builds the MCFG table.
  - IORT  : The IORT generator collates the IO Topology information from the
            Configuration Manager and builds the IORT table.
  - PPTT  : The PPTT generator collates the processor topology information from
            the Configuration Manager and builds the PPTT table.
  - SRAT  : The SRAT generator collates the system resource affinity information
            from the Configuration Manager and builds the SRAT table.
  - SSDT Serial-Port:
            The SSDT Serial generator collates the Serial port information
            from the Configuration Manager and patches the SSDT Serial Port
            template to build the SSDT Serial port table.
  - SSDT CMN-600:
            The SSDT CMN-600 generator collates the CMN-600 information
            from the Configuration Manager and patches the SSDT CMN-600
            template to build the SSDT CMN-600 table.
  - SSDT Cpu-Topology:
            The SSDT Cpu-Topology generator collates the cpu and LPI
            information from the Configuration Manager and generates a
            SSDT table describing the CPU hierarchy.
  - SSDT Pci-Express:
            The SSDT Pci Express generator collates the Pci Express
            information from the Configuration Manager and generates a
            SSDT table describing a Pci Express bus.
*/

/** The ACPI_TABLE_GENERATOR_ID type describes ACPI table generator ID.
*/
typedef TABLE_GENERATOR_ID ACPI_TABLE_GENERATOR_ID;

/** The ESTD_ACPI_TABLE_ID enum describes the ACPI table IDs reserved for
  the standard generators.
*/
typedef enum StdAcpiTableId {
  EStdAcpiTableIdReserved = 0x0000,             ///< Reserved
  EStdAcpiTableIdRaw,                           ///< RAW Generator
  EStdAcpiTableIdDsdt = EStdAcpiTableIdRaw,     ///< DSDT Generator
  EStdAcpiTableIdSsdt = EStdAcpiTableIdRaw,     ///< SSDT Generator
  EStdAcpiTableIdFadt,                          ///< FADT Generator
  EStdAcpiTableIdMadt,                          ///< MADT Generator
  EStdAcpiTableIdGtdt,                          ///< GTDT Generator
  EStdAcpiTableIdDbg2,                          ///< DBG2 Generator
  EStdAcpiTableIdSpcr,                          ///< SPCR Generator
  EStdAcpiTableIdMcfg,                          ///< MCFG Generator
  EStdAcpiTableIdIort,                          ///< IORT Generator
  EStdAcpiTableIdPptt,                          ///< PPTT Generator
  EStdAcpiTableIdSrat,                          ///< SRAT Generator
  EStdAcpiTableIdSsdtSerialPort,                ///< SSDT Serial-Port Generator
  EStdAcpiTableIdSsdtCmn600,                    ///< SSDT Cmn-600 Generator
  EStdAcpiTableIdSsdtCpuTopology,               ///< SSDT Cpu Topology
  EStdAcpiTableIdSsdtPciExpress,                ///< SSDT Pci Express Generator
  EStdAcpiTableIdPcct,                          ///< PCCT Generator
  EStdAcpiTableIdMax
} ESTD_ACPI_TABLE_ID;

/** This macro checks if the Table Generator ID is for an ACPI Table Generator.

  @param [in] TableGeneratorId  The table generator ID.

  @return TRUE if the table generator ID is for an ACPI Table
        Generator.
**/
#define IS_GENERATOR_TYPE_ACPI(TableGeneratorId) \
          (GET_TABLE_TYPE (TableGeneratorId) == ETableGeneratorTypeAcpi)

/** This macro checks if the Table Generator ID is for a standard ACPI
    Table Generator.

  @param [in] TableGeneratorId  The table generator ID.

  @return TRUE if the table generator ID is for a standard ACPI
          Table Generator.
**/
#define IS_VALID_STD_ACPI_GENERATOR_ID(TableGeneratorId)           \
          (                                                        \
          IS_GENERATOR_NAMESPACE_STD (TableGeneratorId) &&         \
          IS_GENERATOR_TYPE_ACPI (TableGeneratorId)     &&         \
          ((GET_TABLE_ID (GeneratorId) >= EStdAcpiTableIdRaw) &&   \
           (GET_TABLE_ID (GeneratorId) < EStdAcpiTableIdMax))      \
          )

/** This macro creates a standard ACPI Table Generator ID.

  @param [in] TableId  The table generator ID.

  @return a standard ACPI table generator ID.
**/
#define CREATE_STD_ACPI_TABLE_GEN_ID(TableId) \
          CREATE_TABLE_GEN_ID (               \
            ETableGeneratorTypeAcpi,          \
            ETableGeneratorNameSpaceStd,      \
            TableId                           \
            )

/** This macro creates an OEM ACPI Table Generator ID.

  @param [in] TableId  The table generator ID.

  @return an OEM ACPI table generator ID.
**/
#define CREATE_OEM_ACPI_TABLE_GEN_ID(TableId) \
          CREATE_TABLE_GEN_ID (               \
            ETableGeneratorTypeAcpi,          \
            ETableGeneratorNameSpaceOem,      \
            TableId                           \
            )

/** The Creator ID for the ACPI tables generated using
  the standard ACPI table generators.
*/
#define TABLE_GENERATOR_CREATOR_ID_ARM  SIGNATURE_32('A', 'R', 'M', 'H')

/** A macro to initialise the common header part of EFI ACPI tables as
    defined by the EFI_ACPI_DESCRIPTION_HEADER structure.

  @param [in] Signature The ACPI table signature.
  @param [in] Type      The ACPI table structure.
  @param [in] Revision  The ACPI table revision.
**/
#define ACPI_HEADER(Signature, Type, Revision)  {             \
          Signature,             /* UINT32  Signature */      \
          sizeof (Type),         /* UINT32  Length */         \
          Revision,              /* UINT8   Revision */       \
          0,                     /* UINT8   Checksum */       \
          { 0, 0, 0, 0, 0, 0 },  /* UINT8   OemId[6] */       \
          0,                     /* UINT64  OemTableId */     \
          0,                     /* UINT32  OemRevision */    \
          0,                     /* UINT32  CreatorId */      \
          0                      /* UINT32  CreatorRevision */ \
          }

/** A macro to dump the common header part of EFI ACPI tables as
    defined by the EFI_ACPI_DESCRIPTION_HEADER structure.

  @param [in] AcpiHeader The pointer to the ACPI table header.
**/
#define DUMP_ACPI_TABLE_HEADER(AcpiHeader)                        \
          DEBUG ((                                                \
            DEBUG_INFO,                                           \
            "ACPI TABLE %c%c%c%c : Rev 0x%x : Length : 0x%x\n",   \
            (AcpiHeader->Signature & 0xFF),                       \
            ((AcpiHeader->Signature >> 8) & 0xFF),                \
            ((AcpiHeader->Signature >> 16) & 0xFF),               \
            ((AcpiHeader->Signature >> 24) & 0xFF),               \
            AcpiHeader->Revision,                                 \
            AcpiHeader->Length                                    \
            ));

/** Forward declarations.
*/
typedef struct ConfigurationManagerProtocol EDKII_CONFIGURATION_MANAGER_PROTOCOL;
typedef struct CmAStdObjAcpiTableInfo       CM_STD_OBJ_ACPI_TABLE_INFO;
typedef struct AcpiTableGenerator           ACPI_TABLE_GENERATOR;

/** This function pointer describes the interface to ACPI table build
    functions provided by the ACPI table generator and called by the
    Table Manager to build an ACPI table.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to the generated ACPI table.

  @return  EFI_SUCCESS If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (*ACPI_TABLE_GENERATOR_BUILD_TABLE) (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    **Table
  );

/** This function pointer describes the interface used by the
    Table Manager to give the generator an opportunity to free
    any resources allocated for building the ACPI table.

  @param [in]      This           Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the ACPI Table.

  @return EFI_SUCCESS  If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (*ACPI_TABLE_GENERATOR_FREE_TABLE) (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  );

/** This function pointer describes an extended interface to build
    ACPI Tables. The ACPI table generator can generate multiple
    ACPI Tables and return a pointer to the list of ACPI tables.
    The FreeTableResourcesEx() must be called to free any resources
    that may have been allocated using this interface.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to a list of generated ACPI table(s).
  @param [out] TableCount      Number of generated ACPI table(s).

  @return  EFI_SUCCESS If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (*ACPI_TABLE_GENERATOR_BUILD_TABLEEX) (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  );

/** This function pointer describes an extended interface used by the
    Table Manager to give the generator an opportunity to free
    any resources allocated for building the ACPI table. This interface
    must be used in conjunction with the BuildAcpiTableEx interface.

  @param [in]      This           Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the list of ACPI Table(s).
  @param [in]      TableCount     Number of ACPI table(s).

  @return EFI_SUCCESS  If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (*ACPI_TABLE_GENERATOR_FREE_TABLEEX) (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ***CONST  Table,
  IN      CONST UINTN                                          TableCount
  );

/** The ACPI_TABLE_GENERATOR structure provides an interface that the
    Table Manager can use to invoke the functions to build ACPI tables.

    Note: Although the Generator is required to implement at least
          one pair of interfaces (BuildAcpiTable & FreeTableResources or
          BuildAcpiTableEx & FreeTableResourcesEx) for generating the ACPI
          table(s), if both pair of interfaces are implemented the extended
          version will take precedence.
**/
typedef struct AcpiTableGenerator {
  /// The ACPI table generator ID.
  ACPI_TABLE_GENERATOR_ID               GeneratorID;

  /// String describing the ACPI table generator.
  CONST CHAR16                          *Description;

  /// The ACPI table signature.
  UINT32                                AcpiTableSignature;

  /// The ACPI table revision.
  UINT8                                 AcpiTableRevision;

  /// The minimum supported ACPI table revision.
  UINT8                                 MinAcpiTableRevision;

  /// The ACPI table creator ID.
  UINT32                                CreatorId;

  /// The ACPI table creator revision.
  UINT32                                CreatorRevision;

  /// ACPI table build function pointer.
  ACPI_TABLE_GENERATOR_BUILD_TABLE      BuildAcpiTable;

  /** The function to free any resources
      allocated for building the ACPI table.
  */
  ACPI_TABLE_GENERATOR_FREE_TABLE       FreeTableResources;

  /// ACPI table extended build function pointer.
  ACPI_TABLE_GENERATOR_BUILD_TABLEEX    BuildAcpiTableEx;

  /** The function to free any resources
      allocated for building the ACPI table
      using the extended interface.
  */
  ACPI_TABLE_GENERATOR_FREE_TABLEEX     FreeTableResourcesEx;
} ACPI_TABLE_GENERATOR;

/** Register ACPI table factory generator.

  The ACPI table factory maintains a list of the Standard and OEM ACPI
  table generators.

  @param [in]  Generator       Pointer to the ACPI table generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterAcpiTableGenerator (
  IN CONST ACPI_TABLE_GENERATOR                 *CONST  Generator
  );

/** Deregister ACPI generator.

  This function is called by the ACPI table generator to deregister itself
  from the ACPI table factory.

  @param [in]  Generator       Pointer to the ACPI table generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterAcpiTableGenerator (
  IN CONST ACPI_TABLE_GENERATOR                 *CONST  Generator
  );

#pragma pack()

#endif // ACPI_TABLE_GENERATOR_H_

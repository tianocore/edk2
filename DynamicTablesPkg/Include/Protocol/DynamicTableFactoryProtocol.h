/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - ACPI   - Advanced Configuration and Power Interface
    - SMBIOS - System Management BIOS
    - DT     - Device Tree
**/

#ifndef DYNAMIC_TABLE_FACTORY_PROTOCOL_H_
#define DYNAMIC_TABLE_FACTORY_PROTOCOL_H_

#include <AcpiTableGenerator.h>
#include <SmbiosTableGenerator.h>
#include <DeviceTreeTableGenerator.h>

/** This macro defines the Dynamic Table Factory Protocol GUID.

  GUID: {91D1E327-FE5A-49B8-AB65-0ECE2DDB45EC}
*/
#define EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL_GUID       \
  { 0x91d1e327, 0xfe5a, 0x49b8,                         \
    { 0xab, 0x65, 0xe, 0xce, 0x2d, 0xdb, 0x45, 0xec }   \
  };

/** This macro defines the Configuration Manager Protocol Revision.
*/
#define EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL_REVISION  CREATE_REVISION (1, 0)

#pragma pack(1)

/**
  Forward declarations:
*/
typedef struct DynamicTableFactoryProtocol EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL;
typedef struct DynamicTableFactoryInfo EDKII_DYNAMIC_TABLE_FACTORY_INFO;

/** Return a pointer to the ACPI table generator.

  @param [in]  This       Pointer to the Dynamic Table Factory Protocol.
  @param [in]  TableId    The ACPI table generator ID for the
                          requested generator.
  @param [out] Generator  Pointer to the requested ACPI table
                          generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_GET_ACPI_TABLE_GENERATOR) (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  * CONST This,
  IN  CONST ACPI_TABLE_GENERATOR_ID                       GeneratorId,
  OUT CONST ACPI_TABLE_GENERATOR                 ** CONST Generator
  );

/** Registers an ACPI table generator.

  @param [in]  Generator        Pointer to the ACPI table generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_REGISTER_ACPI_TABLE_GENERATOR) (
  IN  CONST ACPI_TABLE_GENERATOR                * CONST Generator
  );

/** Deregister an ACPI table generator.

  @param [in]  Generator       Pointer to the ACPI table generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_DEREGISTER_ACPI_TABLE_GENERATOR) (
  IN  CONST ACPI_TABLE_GENERATOR                * CONST Generator
  );

/** Return a pointer to the SMBIOS table generator.

  @param [in]  This       Pointer to the Dynamic Table Factory Protocol.
  @param [in]  TableId    The SMBIOS table generator ID for the
                          requested generator.
  @param [out] Generator  Pointer to the requested SMBIOS table
                          generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_GET_SMBIOS_TABLE_GENERATOR) (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  * CONST This,
  IN  CONST SMBIOS_TABLE_GENERATOR_ID                     GeneratorId,
  OUT CONST SMBIOS_TABLE_GENERATOR               ** CONST Generator
  );

/** Register a SMBIOS table generator.

  @param [in]  Generator       Pointer to the SMBIOS table generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_REGISTER_SMBIOS_TABLE_GENERATOR) (
  IN  CONST SMBIOS_TABLE_GENERATOR              * CONST Generator
  );

/** Deregister a SMBIOS table generator.

  @param [in]  Generator       Pointer to the SMBIOS table generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_DEREGISTER_SMBIOS_TABLE_GENERATOR) (
  IN  CONST SMBIOS_TABLE_GENERATOR              * CONST Generator
  );

/** Return a pointer to the Device Tree table generator.

  @param [in]  This       Pointer to the Dynamic Table Factory Protocol.
  @param [in]  TableId    The Device Tree table generator ID for the
                          requested generator.
  @param [out] Generator  Pointer to the requested Device Tree table
                          generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_GET_DT_TABLE_GENERATOR) (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  * CONST This,
  IN  CONST DT_TABLE_GENERATOR_ID                         GeneratorId,
  OUT CONST DT_TABLE_GENERATOR                   ** CONST Generator
  );

/** Register a DT table generator.

  @param [in]  Generator       Pointer to the DT table generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_REGISTER_DT_TABLE_GENERATOR) (
  IN  CONST DT_TABLE_GENERATOR                * CONST Generator
  );

/** Deregister a DT table generator.

  This function is called by the DT table generator to deregister itself
  from the DT table factory.

  @param [in]  Generator       Pointer to the DT table generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
typedef
EFI_STATUS
(EFIAPI * EDKII_DYNAMIC_TABLE_FACTORY_DEREGISTER_DT_TABLE_GENERATOR) (
  IN  CONST DT_TABLE_GENERATOR                * CONST Generator
  );

/** A structure describing the Dynamic Table Factory Protocol interface.
*/
typedef struct DynamicTableFactoryProtocol {
  /// The Dynamic Table Factory Protocol revision.
  UINT32                                                 Revision;

  /// The interface used to request an ACPI Table Generator.
  EDKII_DYNAMIC_TABLE_FACTORY_GET_ACPI_TABLE_GENERATOR   GetAcpiTableGenerator;

  /// Register an ACPI table Generator
  EDKII_DYNAMIC_TABLE_FACTORY_REGISTER_ACPI_TABLE_GENERATOR
    RegisterAcpiTableGenerator;

  /// Deregister an ACPI table Generator
  EDKII_DYNAMIC_TABLE_FACTORY_DEREGISTER_ACPI_TABLE_GENERATOR
    DeregisterAcpiTableGenerator;

  /// The interface used to request a SMBIOS Table Generator.
  EDKII_DYNAMIC_TABLE_FACTORY_GET_SMBIOS_TABLE_GENERATOR GetSmbiosTableGenerator;

  /// Register an SMBIOS table Generator
  EDKII_DYNAMIC_TABLE_FACTORY_REGISTER_SMBIOS_TABLE_GENERATOR
    RegisterSmbiosTableGenerator;

  /// Deregister an SMBIOS table Generator
  EDKII_DYNAMIC_TABLE_FACTORY_REGISTER_SMBIOS_TABLE_GENERATOR
    DeregisterSmbiosTableGenerator;

  /// The interface used to request a Device Tree Table Generator.
  EDKII_DYNAMIC_TABLE_FACTORY_GET_DT_TABLE_GENERATOR     GetDtTableGenerator;

  /// Register a DT generator
  EDKII_DYNAMIC_TABLE_FACTORY_REGISTER_DT_TABLE_GENERATOR
    RegisterDtTableGenerator;

  /// Deregister a DT generator
  EDKII_DYNAMIC_TABLE_FACTORY_DEREGISTER_DT_TABLE_GENERATOR
    DeregisterDtTableGenerator;

  /** Pointer to the data structure that holds the
      list of registered table generators
  */
  EDKII_DYNAMIC_TABLE_FACTORY_INFO          * TableFactoryInfo;
} EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL;

/** The Dynamic Table Factory Protocol GUID.
*/
extern EFI_GUID gEdkiiDynamicTableFactoryProtocolGuid;

#pragma pack()

#endif // DYNAMIC_TABLE_FACTORY_PROTOCOL_H_

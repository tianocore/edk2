/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Std    - Standard
    - ACPI   - Advanced Configuration and Power Interface
    - SMBIOS - System Management BIOS
    - DT     - Device Tree
**/

#ifndef DYNAMIC_TABLE_FACTORY_H_
#define DYNAMIC_TABLE_FACTORY_H_

#pragma pack(1)

/** A structure that holds the list of registered ACPI and
    SMBIOS table generators.
*/
typedef struct DynamicTableFactoryInfo {
  /// An array for holding the list of Standard ACPI Table  Generators.
  CONST ACPI_TABLE_GENERATOR *
        StdAcpiTableGeneratorList[EStdAcpiTableIdMax];

  /// An array for holding the list of Custom ACPI Table Generators.
  CONST ACPI_TABLE_GENERATOR *
        CustomAcpiTableGeneratorList[FixedPcdGet16 (
                                   PcdMaxCustomACPIGenerators
                                   )];

  /// An array for holding the list of Standard SMBIOS Table Generators.
  CONST SMBIOS_TABLE_GENERATOR *
        StdSmbiosTableGeneratorList[EStdSmbiosTableIdMax];

  /// An array for holding the list of Custom SMBIOS Table Generators.
  CONST SMBIOS_TABLE_GENERATOR *
        CustomSmbiosTableGeneratorList[FixedPcdGet16 (
                                     PcdMaxCustomSMBIOSGenerators
                                     )];

  /// An array for holding the list of Standard DT Table Generators.
  CONST DT_TABLE_GENERATOR *
        StdDtTableGeneratorList[EStdDtTableIdMax];

  /// An array for holding the list of Custom DT Table Generators.
  CONST DT_TABLE_GENERATOR *
        CustomDtTableGeneratorList[FixedPcdGet16 (
                                 PcdMaxCustomDTGenerators
                                 )];

  /// An array for holding a map of SMBIOS handles and the CM Object
  /// token used to build the SMBIOS record.
  SMBIOS_HANDLE_MAP
        SmbiosHandleMap[FixedPcdGet16 (
                      PcdMaxSmbiosHandleMapEntries
                      )];
} EDKII_DYNAMIC_TABLE_FACTORY_INFO;

/** Return a pointer to the ACPI table generator.

  @param [in]  This         Pointer to the Dynamic Table Factory Protocol.
  @param [in]  GeneratorId  The ACPI table generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested ACPI table
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetAcpiTableGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  This,
  IN  CONST ACPI_TABLE_GENERATOR_ID                       GeneratorId,
  OUT CONST ACPI_TABLE_GENERATOR                 **CONST  Generator
  );

/** Return a pointer to the SMBIOS table generator.

  @param [in]  This         Pointer to the Dynamic Table Factory Protocol.
  @param [in]  GeneratorId  The SMBIOS table generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested SMBIOS table
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetSmbiosTableGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  This,
  IN  CONST SMBIOS_TABLE_GENERATOR_ID                     GeneratorId,
  OUT CONST SMBIOS_TABLE_GENERATOR               **CONST  Generator
  );

/** Return a pointer to the DT table generator.

  @param [in]  This         Pointer to the Dynamic Table Factory Protocol.
  @param [in]  GeneratorId  The DT table generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested DT table
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetDtTableGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  This,
  IN  CONST DT_TABLE_GENERATOR_ID                         GeneratorId,
  OUT CONST DT_TABLE_GENERATOR                   **CONST  Generator
  );

#pragma pack()

#endif // DYNAMIC_TABLE_FACTORY_H_

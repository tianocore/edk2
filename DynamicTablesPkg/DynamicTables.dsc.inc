## @file
#  Dsc include file for Dynamic Tables Framework.
#
#  Copyright (c) 2017 - 2022, Arm Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]

[BuildOptions]
  RELEASE_*_*_CC_FLAGS     = -DMDEPKG_NDEBUG

[LibraryClasses.common]
  AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf
  AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  SsdtPcieSupportLib|DynamicTablesPkg/Library/Common/SsdtPcieSupportLib/SsdtPcieSupportLib.inf
  SsdtSerialPortFixupLib|DynamicTablesPkg/Library/Common/SsdtSerialPortFixupLib/SsdtSerialPortFixupLib.inf
  TableHelperLib|DynamicTablesPkg/Library/Common/TableHelperLib/TableHelperLib.inf

[Components.common]
  #
  # Generators
  #
  DynamicTablesPkg/Library/Acpi/Arm/AcpiDbg2LibArm/AcpiDbg2LibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiFadtLibArm/AcpiFadtLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiGtdtLibArm/AcpiGtdtLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiIortLibArm/AcpiIortLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiMadtLibArm/AcpiMadtLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiMcfgLibArm/AcpiMcfgLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiPpttLibArm/AcpiPpttLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiRawLibArm/AcpiRawLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiSpcrLibArm/AcpiSpcrLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiSratLibArm/AcpiSratLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiPcctLibArm/AcpiPcctLibArm.inf

  # AML Fixup
  DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtSerialPortLibArm/SsdtSerialPortLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtCmn600LibArm/SsdtCmn600LibArm.inf

  # AML Codegen
  DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtCpuTopologyLibArm/SsdtCpuTopologyLibArm.inf
  DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtPcieLibArm/SsdtPcieLibArm.inf

  #
  # Dynamic Table Factory Dxe
  #
  DynamicTablesPkg/Drivers/DynamicTableFactoryDxe/DynamicTableFactoryDxe.inf {
    <LibraryClasses>
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiDbg2LibArm/AcpiDbg2LibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiFadtLibArm/AcpiFadtLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiGtdtLibArm/AcpiGtdtLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiIortLibArm/AcpiIortLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiMadtLibArm/AcpiMadtLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiMcfgLibArm/AcpiMcfgLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiPpttLibArm/AcpiPpttLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiRawLibArm/AcpiRawLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiSpcrLibArm/AcpiSpcrLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiSratLibArm/AcpiSratLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiPcctLibArm/AcpiPcctLibArm.inf

      # AML Fixup
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtSerialPortLibArm/SsdtSerialPortLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtCmn600LibArm/SsdtCmn600LibArm.inf

      # AML Codegen
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtCpuTopologyLibArm/SsdtCpuTopologyLibArm.inf
      NULL|DynamicTablesPkg/Library/Acpi/Arm/AcpiSsdtPcieLibArm/SsdtPcieLibArm.inf
  }

  #
  # Dynamic Tables Manager Dxe
  #
  DynamicTablesPkg/Drivers/DynamicTableManagerDxe/DynamicTableManagerDxe.inf


## @file
# DynamicTablesPkg DSC file used to build host-based unit tests.
#
# Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved. <BR>
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = DynamicTablesPkgHostTest
  PLATFORM_GUID           = 47c66bc0-e9ad-4d83-ae31-945eb128beee
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/DynamicTablesPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  AcpiHelperLib|DynamicTablesPkg/Library/Common/AcpiHelperLib/AcpiHelperLib.inf
  AcpiLib|EmbeddedPkg/Library/AcpiLib/AcpiLib.inf
  AmlLib|DynamicTablesPkg/Library/Common/AmlLib/AmlLib.inf
  SsdtSerialPortFixupLib|DynamicTablesPkg/Library/Common/SsdtSerialPortFixupLib/SsdtSerialPortFixupLib.inf
  TableHelperLib|DynamicTablesPkg/Library/Common/TableHelperLib/TableHelperLib.inf

[Components]
  DynamicTablesPkg/Library/Acpi/Common/AcpiDbg2Lib/GoogleTest/Dbg2GeneratorGoogleTest.inf {
    <LibraryClasses>
      NULL|DynamicTablesPkg/Library/Acpi/Common/AcpiDbg2Lib/AcpiDbg2Lib.inf
  }

/** @file
  Unit tests for DBG2 Generator

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved. <BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
  #include <Library/FunctionMockLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/TableHelperLib.h>
  #include <Library/SsdtSerialPortFixupLib.h>
  #include <AcpiTableGenerator.h>
  #include <ConfigurationManagerObject.h>
  #include <ConfigurationManagerHelper.h>
  #include <StandardNameSpaceObjects.h>
  #include <IndustryStandard/DebugPort2Table.h>
  #include <Protocol/SerialIo.h>
  #include "GoogleTest/Protocol/MockConfigurationManagerProtocol.h"
  #include "../Dbg2Generator.h"

  #define SERIAL_PORT_BASE_ADDRESS(i)  (0x1000ULL * (i + 1))
  #define SERIAL_PORT_BASE_ADDRESS_LENGTH  (0x1000ULL)
  #define SERIAL_PORT_BAUD_RATE            (115200)

  #define DBG2_BASE_ADDRESS(i)  (0x1000ULL * (i + 1))
  #define DBG2_BASE_ADDRESS_LENGTH  (0x1000ULL)

  EFI_STATUS
  EFIAPI
  AcpiDbg2LibConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  EFI_STATUS
  EFIAPI
  AcpiDbg2LibDestructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    );

  // Global generator instance
  static ACPI_TABLE_GENERATOR  *gDbg2Generator = NULL;

  // C++ wrapper functions for C linkage functions
  EFI_STATUS
  RegisterAcpiTableGenerator (
    IN  CONST ACPI_TABLE_GENERATOR  *CONST  TableGenerator
    )
  {
    if (TableGenerator == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    // Store the generator for use in tests
    gDbg2Generator = const_cast<ACPI_TABLE_GENERATOR *>(TableGenerator);
    return EFI_SUCCESS;
  }

  EFI_STATUS
  DeregisterAcpiTableGenerator (
    IN  CONST ACPI_TABLE_GENERATOR  *CONST  TableGenerator
    )
  {
    if (TableGenerator == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    // Clear the stored generator
    gDbg2Generator = NULL;
    return EFI_SUCCESS;
  }
}

using namespace testing;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::AtLeast;

// Add base class before the test classes
class Dbg2GeneratorTestBase {
protected:
  void
  ValidateDbg2TableHeader (
    IN EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE  *Dbg2Table
    )
  {
    EXPECT_NE (Dbg2Table, nullptr);
    EXPECT_EQ (Dbg2Table->Header.Signature, (UINT32)EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE);
    EXPECT_EQ (Dbg2Table->Header.Revision, EFI_ACPI_DEBUG_PORT_2_TABLE_REVISION);
    EXPECT_EQ (Dbg2Table->Header.OemId[0], (UINT8)'T');
    EXPECT_EQ (Dbg2Table->Header.OemId[1], (UINT8)'E');
    EXPECT_EQ (Dbg2Table->Header.OemId[2], (UINT8)'S');
    EXPECT_EQ (Dbg2Table->Header.OemId[3], (UINT8)'T');
    EXPECT_EQ (Dbg2Table->Header.OemId[4], (UINT8)'I');
    EXPECT_EQ (Dbg2Table->Header.OemId[5], (UINT8)'D');
  }

  void
  ValidateSerialDeviceInfo (
    IN EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *DeviceInfo,
    IN UINT64                                         ExpectedBaseAddress,
    IN UINT32                                         ExpectedAddressSize
    )
  {
    EXPECT_EQ (DeviceInfo->Revision, EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION);
    EXPECT_EQ (DeviceInfo->PortType, EFI_ACPI_DBG2_PORT_TYPE_SERIAL);
    EXPECT_EQ (DeviceInfo->PortSubtype, EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550);
    EXPECT_EQ (DeviceInfo->NumberofGenericAddressRegisters, 1U);

    // Validate base address register
    EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE  *Gas = (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE *)((UINT8 *)DeviceInfo + DeviceInfo->BaseAddressRegisterOffset);

    EXPECT_EQ (Gas->AddressSpaceId, (UINT8)EFI_ACPI_6_3_SYSTEM_MEMORY);
    EXPECT_EQ (Gas->RegisterBitWidth, 32U);
    EXPECT_EQ (Gas->RegisterBitOffset, 0U);
    EXPECT_EQ (Gas->AccessSize, (UINT8)EFI_ACPI_6_3_DWORD);
    EXPECT_EQ (Gas->Address, ExpectedBaseAddress);

    // Validate address size
    UINT32  *AddressSize = (UINT32 *)((UINT8 *)DeviceInfo + DeviceInfo->AddressSizeOffset);

    EXPECT_EQ (*AddressSize, ExpectedAddressSize);
  }

  void
  ValidateNonSerialDeviceInfo (
    IN EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT              *DeviceInfo,
    IN const std::vector<CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR>  &MemoryRanges
    )
  {
    EXPECT_EQ (DeviceInfo->Revision, (UINT8)EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION);
    // Cast both sides to UINT16 to ensure proper comparison
    EXPECT_EQ ((UINT16)DeviceInfo->PortType, (UINT16)EFI_ACPI_DBG2_PORT_TYPE_NET);
    EXPECT_EQ (DeviceInfo->PortSubtype, (UINT16)0x0000);
    EXPECT_EQ ((UINT32)DeviceInfo->NumberofGenericAddressRegisters, (UINT32)MemoryRanges.size ());

    // Validate base address registers
    EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE  *Gas = (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE *)((UINT8 *)DeviceInfo + DeviceInfo->BaseAddressRegisterOffset);

    // Validate each memory range
    for (size_t j = 0; j < MemoryRanges.size (); j++) {
      EXPECT_EQ (Gas->AddressSpaceId, (UINT8)EFI_ACPI_6_3_SYSTEM_MEMORY);
      EXPECT_EQ (Gas->RegisterBitWidth, (UINT8)32);
      EXPECT_EQ (Gas->RegisterBitOffset, (UINT8)0);
      EXPECT_EQ (Gas->AccessSize, (UINT8)EFI_ACPI_6_3_DWORD);
      EXPECT_EQ (Gas->Address, MemoryRanges[j].BaseAddress);

      // Move to next GAS structure
      Gas = (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE *)((UINT8 *)Gas + sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE));
    }

    // Validate address sizes
    UINT32  *AddressSize = (UINT32 *)((UINT8 *)DeviceInfo + DeviceInfo->AddressSizeOffset);

    for (size_t j = 0; j < MemoryRanges.size (); j++) {
      EXPECT_EQ (AddressSize[j], MemoryRanges[j].Length);
    }
  }

  void
  ValidateSsdtTableHeader (
    IN EFI_ACPI_DESCRIPTION_HEADER  *SsdtTable
    )
  {
    EXPECT_NE (SsdtTable, nullptr);
    EXPECT_EQ (SsdtTable->Signature, (UINT32)EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE);
    EXPECT_EQ (SsdtTable->Revision, EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_REVISION);
    EXPECT_EQ (SsdtTable->OemId[0], (UINT8)'A');
    EXPECT_EQ (SsdtTable->OemId[1], (UINT8)'R');
    EXPECT_EQ (SsdtTable->OemId[2], (UINT8)'M');
    EXPECT_EQ (SsdtTable->OemId[3], (UINT8)'L');
    EXPECT_EQ (SsdtTable->OemId[4], (UINT8)'T');
    EXPECT_EQ (SsdtTable->OemId[5], (UINT8)'D');
  }
};

// Update test class declarations to inherit from base class
class Dbg2GeneratorTest : public ::testing::Test, public Dbg2GeneratorTestBase {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CfgMgrInfo = { 0 };

  void
  SetUp (
    ) override
  {
    // Set up default behavior for GetObject
    ON_CALL (MockConfigMgrProtocol, GetObject (_, _, _, _))
      .WillByDefault (Return (EFI_NOT_FOUND));

    // Set up configuration manager info

    CfgMgrInfo.Revision = CREATE_REVISION (1, 0);
    CfgMgrInfo.OemId[0] = 'T';
    CfgMgrInfo.OemId[1] = 'E';
    CfgMgrInfo.OemId[2] = 'S';
    CfgMgrInfo.OemId[3] = 'T';
    CfgMgrInfo.OemId[4] = 'I';
    CfgMgrInfo.OemId[5] = 'D';

    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo),
      sizeof (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO),
      &CfgMgrInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Initialize the DBG2 library with our mock protocol
    EXPECT_EQ (AcpiDbg2LibConstructor (NULL, NULL), EFI_SUCCESS);

    // Setup common test data with proper initialization
    mAcpiTableInfo.TableGeneratorId   = CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2);
    mAcpiTableInfo.AcpiTableSignature = EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE;
    mAcpiTableInfo.AcpiTableRevision  = EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION;
  }

  void
  TearDown (
    ) override
  {
    // Clean up the DBG2 library
    AcpiDbg2LibDestructor (NULL, NULL);
  }

  void
  SetupSerialPortInfo (
    UINT32  Count
    )
  {
    mSerialPortInfo.resize (Count);
    for (UINT32 i = 0; i < Count; i++) {
      CM_ARCH_COMMON_SERIAL_PORT_INFO  info = { 0 };
      info.BaseAddress       = SERIAL_PORT_BASE_ADDRESS (i);
      info.BaseAddressLength = SERIAL_PORT_BASE_ADDRESS_LENGTH;
      info.AccessSize        = EFI_ACPI_6_3_DWORD;
      info.BaudRate          = SERIAL_PORT_BAUD_RATE;
      info.PortSubtype       = EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550;
      mSerialPortInfo[i]     = info;
    }

    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo), CM_NULL_TOKEN, _))
      .WillOnce (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      (UINT32)sizeof (CM_ARCH_COMMON_SERIAL_PORT_INFO) * Count,
      &mSerialPortInfo[0],
      Count
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Set up expectation for DBG2 device info
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo), CM_NULL_TOKEN, _))
      .WillOnce (Return (EFI_NOT_FOUND));
  }

  void
  SetupNonSerialDbg2DeviceInfo (
    VOID
    )
  {
    // Set up memory range descriptors
    mMemoryRangeDescriptors.clear ();
    for (UINT32 i = 0; i < 2; i++) {
      CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  desc = { 0 };
      desc.BaseAddress = DBG2_BASE_ADDRESS (i);
      desc.Length      = DBG2_BASE_ADDRESS_LENGTH;
      mMemoryRangeDescriptors.push_back (desc);
    }

    // Set up DBG2 device info
    mDbg2DeviceInfo.clear ();
    CM_ARCH_COMMON_DBG2_DEVICE_INFO  info = { 0 };

    info.AddressResourceToken = 1;  // Unique token for each device
    info.PortType             = EFI_ACPI_DBG2_PORT_TYPE_NET;
    info.PortSubtype          = 0;
    info.AccessSize           = EFI_ACPI_6_3_DWORD;
    CopyMem (info.ObjectName, "DBG2", sizeof ("DBG2"));
    mDbg2DeviceInfo.push_back (info);

    // Set up mock expectations
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo), CM_NULL_TOKEN, _))
      .WillOnce (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      sizeof (CM_ARCH_COMMON_DBG2_DEVICE_INFO),
      &mDbg2DeviceInfo[0],
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Set up mock expectations
    EXPECT_CALL (
      MockConfigMgrProtocol,
      GetObject (
        _,
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
        1,
        _
        )
      ).WillOnce (
          DoAll (
            SetArgPointee<3> (
              CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
      sizeof (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR) * 2,
      &mMemoryRangeDescriptors[0],
      2
    }
              ),
            Return (EFI_SUCCESS)
            )
          );
  }

  CM_STD_OBJ_ACPI_TABLE_INFO mAcpiTableInfo;
  std::vector<CM_ARCH_COMMON_SERIAL_PORT_INFO> mSerialPortInfo;
  std::vector<CM_ARCH_COMMON_DBG2_DEVICE_INFO> mDbg2DeviceInfo;
  std::vector<CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR> mMemoryRangeDescriptors;
};

TEST_F (Dbg2GeneratorTest, BuildDbg2TableEx_SingleSerialPort) {
  SetupSerialPortInfo (1U);

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_SUCCESS
    );

  EXPECT_NE (Table, nullptr);
  EXPECT_EQ (TableCount, 2U);

  // Find the DBG2 table
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE  *Dbg2Table = nullptr;
  EFI_ACPI_DESCRIPTION_HEADER              *SsdtTable = nullptr;

  for (UINTN i = 0; i < TableCount; i++) {
    if (Table[i]->Signature == EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE) {
      Dbg2Table = (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE *)Table[i];
    } else if (Table[i]->Signature == EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      SsdtTable = Table[i];
    }
  }

  ValidateDbg2TableHeader (Dbg2Table);
  ValidateSsdtTableHeader (SsdtTable);

  // Validate DBG2 table structure
  EXPECT_EQ (Dbg2Table->NumberDbgDeviceInfo, 1U);
  EXPECT_NE (Dbg2Table->OffsetDbgDeviceInfo, 0U);

  // Get pointer to device information structure
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *DeviceInfo = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)Dbg2Table + Dbg2Table->OffsetDbgDeviceInfo);

  ValidateSerialDeviceInfo (DeviceInfo, SERIAL_PORT_BASE_ADDRESS (0), SERIAL_PORT_BASE_ADDRESS_LENGTH);

  gDbg2Generator->FreeTableResourcesEx (
                    gDbg2Generator,
                    &mAcpiTableInfo,
                    gConfigurationManagerProtocol,
                    &Table,
                    TableCount
                    );
}

TEST_F (Dbg2GeneratorTest, BuildDbg2TableEx_NoDevices) {
  // Setup expectation for no devices
  EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo), CM_NULL_TOKEN, _))
    .WillOnce (Return (EFI_NOT_FOUND));

  EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo), CM_NULL_TOKEN, _))
    .WillOnce (Return (EFI_NOT_FOUND));

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_NOT_FOUND
    );

  EXPECT_EQ (Table, nullptr);
  EXPECT_EQ (TableCount, 0U);
}

TEST_F (Dbg2GeneratorTest, BuildDbg2TableEx_NonSerialDbg2Device) {
  EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo), CM_NULL_TOKEN, _))
    .WillOnce (Return (EFI_NOT_FOUND));

  SetupNonSerialDbg2DeviceInfo ();

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_SUCCESS
    );

  EXPECT_NE (Table, nullptr);
  EXPECT_EQ (TableCount, 1U);

  // Find the DBG2 table
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE  *Dbg2Table = nullptr;

  for (UINTN i = 0; i < TableCount; i++) {
    if (Table[i]->Signature == EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE) {
      Dbg2Table = (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE *)Table[i];
      break;
    }
  }

  ValidateDbg2TableHeader (Dbg2Table);

  // Validate DBG2 table structure
  EXPECT_EQ (Dbg2Table->NumberDbgDeviceInfo, 1U);
  EXPECT_NE (Dbg2Table->OffsetDbgDeviceInfo, 0U);

  // Get pointer to device information structure
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *DeviceInfo = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)Dbg2Table + Dbg2Table->OffsetDbgDeviceInfo);

  ValidateNonSerialDeviceInfo (DeviceInfo, mMemoryRangeDescriptors);

  gDbg2Generator->FreeTableResourcesEx (
                    gDbg2Generator,
                    &mAcpiTableInfo,
                    gConfigurationManagerProtocol,
                    &Table,
                    TableCount
                    );
}

// Replace tuple-based test structure with a simpler struct
struct DeviceTestConfig {
  UINT32     DeviceCount;
  UINT32     RangesPerDevice;
  BOOLEAN    HasSerialPort; // Changed from SerialPortCount to HasSerialPort since only one port is supported
};

// Update test class declarations to inherit from base class
class Dbg2GeneratorParameterizedTest : public ::testing::TestWithParam<DeviceTestConfig>, public Dbg2GeneratorTestBase {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CfgMgrInfo = { 0 };
  CM_STD_OBJ_ACPI_TABLE_INFO mAcpiTableInfo;
  std::vector<CM_ARCH_COMMON_DBG2_DEVICE_INFO> mDevices;
  std::vector<CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR> mMemoryRanges;
  std::vector<std::vector<CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR> > mDeviceSpecificRanges;
  std::vector<CM_ARCH_COMMON_SERIAL_PORT_INFO> mSerialPortInfo;

  void
  SetUp (
    ) override
  {
    // Set up default behavior for GetObject
    ON_CALL (MockConfigMgrProtocol, GetObject (_, _, _, _))
      .WillByDefault (Return (EFI_NOT_FOUND));

    // Set up configuration manager info
    CfgMgrInfo.Revision = CREATE_REVISION (1, 0);
    CfgMgrInfo.OemId[0] = 'T';
    CfgMgrInfo.OemId[1] = 'E';
    CfgMgrInfo.OemId[2] = 'S';
    CfgMgrInfo.OemId[3] = 'T';
    CfgMgrInfo.OemId[4] = 'I';
    CfgMgrInfo.OemId[5] = 'D';

    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo),
      sizeof (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO),
      &CfgMgrInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Initialize the DBG2 library with our mock protocol
    EXPECT_EQ (AcpiDbg2LibConstructor (NULL, NULL), EFI_SUCCESS);

    // Setup common test data
    mAcpiTableInfo.TableGeneratorId   = CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2);
    mAcpiTableInfo.AcpiTableSignature = EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE;
    mAcpiTableInfo.AcpiTableRevision  = EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION;

    // Set up devices and memory ranges based on test parameters
    mDevices.clear ();
    mMemoryRanges.clear ();
    mDeviceSpecificRanges.clear ();
    mSerialPortInfo.clear ();

    UINT32                  totalRanges = 0;
    const DeviceTestConfig  &config     = GetParam ();

    // Set up serial port info if configured
    if (config.HasSerialPort) {
      mSerialPortInfo.resize (1);  // Always create just one serial port
      CM_ARCH_COMMON_SERIAL_PORT_INFO  info = { 0 };
      info.BaseAddress       = SERIAL_PORT_BASE_ADDRESS (0);
      info.BaseAddressLength = SERIAL_PORT_BASE_ADDRESS_LENGTH;
      info.AccessSize        = EFI_ACPI_6_3_DWORD;
      info.BaudRate          = SERIAL_PORT_BAUD_RATE;
      info.PortSubtype       = EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550;
      mSerialPortInfo[0]     = info;

      EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo), CM_NULL_TOKEN, _))
        .WillOnce (
           DoAll (
             SetArgPointee<3>(
               CM_OBJ_DESCRIPTOR {
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
        sizeof (CM_ARCH_COMMON_SERIAL_PORT_INFO),
        &mSerialPortInfo[0],
        1
      }
               ),
             Return (EFI_SUCCESS)
             )
           );
    } else {
      EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo), CM_NULL_TOKEN, _))
        .WillOnce (Return (EFI_NOT_FOUND));
    }

    // Create devices
    for (UINT32 i = 0; i < config.DeviceCount; i++) {
      CM_ARCH_COMMON_DBG2_DEVICE_INFO  device = { 0 };
      device.AddressResourceToken = mDevices.size () + 1;
      device.PortType             = EFI_ACPI_DBG2_PORT_TYPE_NET;
      device.PortSubtype          = 0;
      device.AccessSize           = EFI_ACPI_6_3_DWORD;
      CopyMem (device.ObjectName, "DBG2", sizeof (device.ObjectName) - 1);
      device.ObjectName[sizeof (device.ObjectName) - 1] = '\0';
      mDevices.push_back (device);

      // Create memory ranges for this device
      std::vector<CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR>  deviceRanges;
      for (UINT32 j = 0; j < config.RangesPerDevice; j++) {
        CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  range = { 0 };
        range.BaseAddress = DBG2_BASE_ADDRESS (totalRanges + j);
        range.Length      = DBG2_BASE_ADDRESS_LENGTH;
        deviceRanges.push_back (range);
        mMemoryRanges.push_back (range);
      }

      mDeviceSpecificRanges.push_back (deviceRanges);
      totalRanges += config.RangesPerDevice;
    }

    // Set up mock expectations for DBG2 device info
    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo), CM_NULL_TOKEN, _))
      .WillOnce (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      (UINT32)(sizeof (CM_ARCH_COMMON_DBG2_DEVICE_INFO) * mDevices.size ()),
      &mDevices[0],
      (UINT32)mDevices.size ()
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Set up mock expectations for memory range descriptors
    for (size_t i = 0; i < mDevices.size (); i++) {
      EXPECT_CALL (
        MockConfigMgrProtocol,
        GetObject (
          _,
          CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
          mDevices[i].AddressResourceToken,
          _
          )
        ).WillOnce (
            DoAll (
              SetArgPointee<3> (
                CM_OBJ_DESCRIPTOR {
        CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
        (UINT32)(sizeof (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR) * mDeviceSpecificRanges[i].size ()),
        &mDeviceSpecificRanges[i][0],
        (UINT32)mDeviceSpecificRanges[i].size ()
      }
                ),
              Return (EFI_SUCCESS)
              )
            );
    }
  }

  void
  TearDown (
    ) override
  {
    // Clean up the DBG2 library
    AcpiDbg2LibDestructor (NULL, NULL);
  }
};

TEST_P (Dbg2GeneratorParameterizedTest, BuildDbg2TableEx_MultipleNonSerialDevices) {
  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_SUCCESS
    );

  EXPECT_NE (Table, nullptr);
  EXPECT_EQ (TableCount, GetParam ().HasSerialPort ? 2U : 1U);

  // Find the DBG2 table
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE  *Dbg2Table = nullptr;
  EFI_ACPI_DESCRIPTION_HEADER              *SsdtTable = nullptr;

  for (UINTN i = 0; i < TableCount; i++) {
    if (Table[i]->Signature == EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE) {
      Dbg2Table = (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE *)Table[i];
    } else if (Table[i]->Signature == EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      SsdtTable = Table[i];
    }
  }

  ValidateDbg2TableHeader (Dbg2Table);

  // Validate DBG2 table structure
  UINT32  expectedDeviceCount = (UINT32)(mDevices.size () + (GetParam ().HasSerialPort ? 1 : 0));

  EXPECT_EQ (Dbg2Table->NumberDbgDeviceInfo, expectedDeviceCount);
  EXPECT_NE (Dbg2Table->OffsetDbgDeviceInfo, 0U);

  // Get pointer to first device information structure
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *DeviceInfo = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)Dbg2Table + Dbg2Table->OffsetDbgDeviceInfo);

  // Validate each device
  for (size_t i = 0; i < mDevices.size (); i++) {
    ValidateNonSerialDeviceInfo (DeviceInfo, mDeviceSpecificRanges[i]);
    DeviceInfo = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)DeviceInfo + DeviceInfo->Length);
  }

  // Validate SSDT table if present
  if (GetParam ().HasSerialPort) {
    ValidateSsdtTableHeader (SsdtTable);
  }

  gDbg2Generator->FreeTableResourcesEx (
                    gDbg2Generator,
                    &mAcpiTableInfo,
                    gConfigurationManagerProtocol,
                    &Table,
                    TableCount
                    );
}

INSTANTIATE_TEST_SUITE_P (
  Dbg2GeneratorTests,
  Dbg2GeneratorParameterizedTest,
  ::testing::Values (
               DeviceTestConfig { 1, 2, false }, // 1 device, 2 ranges per device, no serial port
               DeviceTestConfig { 2, 3, false }, // 2 devices, 3 ranges per device, no serial port
               DeviceTestConfig { 2, 2, true } // 2 devices, 2 ranges per device, with serial port
               )
  );

// Test class for adversarial test cases
class Dbg2GeneratorAdversarialTest : public ::testing::Test, public Dbg2GeneratorTestBase {
protected:
  MockConfigurationManagerProtocol MockConfigMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO CfgMgrInfo = { 0 };
  CM_STD_OBJ_ACPI_TABLE_INFO mAcpiTableInfo;

  void
  SetUp (
    ) override
  {
    // Set up default behavior for GetObject
    ON_CALL (MockConfigMgrProtocol, GetObject (_, _, _, _))
      .WillByDefault (Return (EFI_NOT_FOUND));

    // Set up configuration manager info
    CfgMgrInfo.Revision = CREATE_REVISION (1, 0);
    CfgMgrInfo.OemId[0] = 'T';
    CfgMgrInfo.OemId[1] = 'E';
    CfgMgrInfo.OemId[2] = 'S';
    CfgMgrInfo.OemId[3] = 'T';
    CfgMgrInfo.OemId[4] = 'I';
    CfgMgrInfo.OemId[5] = 'D';

    EXPECT_CALL (MockConfigMgrProtocol, GetObject (_, CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo), CM_NULL_TOKEN, _))
      .WillRepeatedly (
         DoAll (
           SetArgPointee<3>(
             CM_OBJ_DESCRIPTOR {
      CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo),
      sizeof (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO),
      &CfgMgrInfo,
      1
    }
             ),
           Return (EFI_SUCCESS)
           )
         );

    // Initialize the DBG2 library with our mock protocol
    EXPECT_EQ (AcpiDbg2LibConstructor (NULL, NULL), EFI_SUCCESS);

    // Setup common test data
    mAcpiTableInfo.TableGeneratorId   = CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2);
    mAcpiTableInfo.AcpiTableSignature = EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE;
    mAcpiTableInfo.AcpiTableRevision  = EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION;
  }

  void
  TearDown (
    ) override
  {
    // Clean up the DBG2 library
    AcpiDbg2LibDestructor (NULL, NULL);
  }
};

// Test invalid memory range base address (0)
TEST_F (Dbg2GeneratorAdversarialTest, InvalidMemoryRangeBaseAddress) {
  // Set up DBG2 device info
  CM_ARCH_COMMON_DBG2_DEVICE_INFO  device = { 0 };

  device.AddressResourceToken = 1;
  device.PortType             = EFI_ACPI_DBG2_PORT_TYPE_NET;
  device.PortSubtype          = 0;
  device.AccessSize           = EFI_ACPI_6_3_DWORD;

  // Set up memory range with invalid base address
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  range = { 0 };

  range.BaseAddress = 0; // Invalid - must be non-zero
  range.Length      = DBG2_BASE_ADDRESS_LENGTH;

  // Mock serial port info query to return not found
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (Return (EFI_NOT_FOUND));

  // Mock DBG2 device info query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
    sizeof (CM_ARCH_COMMON_DBG2_DEVICE_INFO),
    &device,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  // Mock memory range descriptor query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
      1,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
    sizeof (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR),
    &range,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_INVALID_PARAMETER
    );
}

// Test memory range length too large
TEST_F (Dbg2GeneratorAdversarialTest, MemoryRangeTooLarge) {
  // Set up DBG2 device info
  CM_ARCH_COMMON_DBG2_DEVICE_INFO  device = { 0 };

  device.AddressResourceToken = 1;
  device.PortType             = EFI_ACPI_DBG2_PORT_TYPE_NET;
  device.PortSubtype          = 0;
  device.AccessSize           = EFI_ACPI_6_3_DWORD;

  // Set up memory range with length > MAX_UINT32
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  range = { 0 };

  range.BaseAddress = DBG2_BASE_ADDRESS (0);
  range.Length      = ((UINT64)MAX_UINT32) + 1; // Too large for DBG2 table

  // Mock serial port info query to return not found
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (Return (EFI_NOT_FOUND));

  // Mock DBG2 device info query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
    sizeof (CM_ARCH_COMMON_DBG2_DEVICE_INFO),
    &device,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  // Mock memory range descriptor query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
      1,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
    sizeof (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR),
    &range,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_INVALID_PARAMETER
    );
}

// Test too many memory ranges
TEST_F (Dbg2GeneratorAdversarialTest, TooManyMemoryRanges) {
  // Set up DBG2 device info
  CM_ARCH_COMMON_DBG2_DEVICE_INFO  device = { 0 };

  device.AddressResourceToken = 1;
  device.PortType             = EFI_ACPI_DBG2_PORT_TYPE_NET;
  device.PortSubtype          = 0;
  device.AccessSize           = EFI_ACPI_6_3_DWORD;

  // Create array of memory ranges with count > MAX_UINT8
  std::vector<CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR>  ranges;

  for (UINT32 i = 0; i <= MAX_UINT8 + 1; i++) {
    CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  range = { 0 };
    range.BaseAddress = DBG2_BASE_ADDRESS (i);
    range.Length      = DBG2_BASE_ADDRESS_LENGTH;
    ranges.push_back (range);
  }

  // Mock serial port info query to return not found
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (Return (EFI_NOT_FOUND));

  // Mock DBG2 device info query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
    sizeof (CM_ARCH_COMMON_DBG2_DEVICE_INFO),
    &device,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  // Mock memory range descriptor query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
      1,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
    (UINT32)(sizeof (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR) * ranges.size ()),
    ranges.data (),
    (UINT32)ranges.size ()
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_INVALID_PARAMETER
    );
}

// Test invalid resource token
TEST_F (Dbg2GeneratorAdversarialTest, InvalidResourceToken) {
  // Set up DBG2 device info with CM_NULL_TOKEN for AddressResourceToken
  CM_ARCH_COMMON_DBG2_DEVICE_INFO  device = { 0 };

  device.AddressResourceToken = CM_NULL_TOKEN; // Invalid - must be non-zero
  device.PortType             = EFI_ACPI_DBG2_PORT_TYPE_NET;
  device.PortSubtype          = 0;
  device.AccessSize           = EFI_ACPI_6_3_DWORD;

  // Mock serial port info query to return not found
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (Return (EFI_NOT_FOUND));

  // Mock DBG2 device info query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
    sizeof (CM_ARCH_COMMON_DBG2_DEVICE_INFO),
    &device,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_INVALID_PARAMETER
    );
}

// Test invalid serial port access size
TEST_F (Dbg2GeneratorAdversarialTest, InvalidSerialPortAccessSize) {
  CM_ARCH_COMMON_SERIAL_PORT_INFO  serialPort = { 0 };

  serialPort.BaseAddress       = SERIAL_PORT_BASE_ADDRESS (0);
  serialPort.BaseAddressLength = SERIAL_PORT_BASE_ADDRESS_LENGTH;
  serialPort.AccessSize        = EFI_ACPI_6_3_QWORD; // Invalid - must be <= DWORD
  serialPort.BaudRate          = SERIAL_PORT_BAUD_RATE;
  serialPort.PortSubtype       = EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550;

  // Mock serial port info query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
    sizeof (CM_ARCH_COMMON_SERIAL_PORT_INFO),
    &serialPort,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  // Mock DBG2 device info query to return not found
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (Return (EFI_NOT_FOUND));

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_INVALID_PARAMETER
    );
}

// Test non-serial DBG2 device with missing object name (should default to ".")
TEST_F (Dbg2GeneratorAdversarialTest, MissingObjectName) {
  // Set up DBG2 device info with empty object name
  CM_ARCH_COMMON_DBG2_DEVICE_INFO  device = { 0 };

  device.AddressResourceToken = 1;
  device.PortType             = EFI_ACPI_DBG2_PORT_TYPE_NET;
  device.PortSubtype          = 0;
  device.AccessSize           = EFI_ACPI_6_3_DWORD;
  // Intentionally leave ObjectName empty

  // Set up memory range
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  range = { 0 };

  range.BaseAddress = DBG2_BASE_ADDRESS (0);
  range.Length      = DBG2_BASE_ADDRESS_LENGTH;

  // Mock serial port info query to return not found
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (Return (EFI_NOT_FOUND));

  // Mock DBG2 device info query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
    sizeof (CM_ARCH_COMMON_DBG2_DEVICE_INFO),
    &device,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  // Mock memory range descriptor query
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
      1,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjMemoryRangeDescriptor),
    sizeof (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR),
    &range,
    1
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_SUCCESS
    );

  EXPECT_NE (Table, nullptr);
  EXPECT_EQ (TableCount, 1U);

  // Find the DBG2 table
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE  *Dbg2Table = nullptr;

  for (UINTN i = 0; i < TableCount; i++) {
    if (Table[i]->Signature == EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE) {
      Dbg2Table = (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE *)Table[i];
      break;
    }
  }

  ValidateDbg2TableHeader (Dbg2Table);

  // Validate DBG2 table structure
  EXPECT_EQ (Dbg2Table->NumberDbgDeviceInfo, 1U);
  EXPECT_NE (Dbg2Table->OffsetDbgDeviceInfo, 0U);

  // Get pointer to device information structure
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *DeviceInfo = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)Dbg2Table + Dbg2Table->OffsetDbgDeviceInfo);

  // Validate device info
  EXPECT_EQ (DeviceInfo->Revision, EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION);
  EXPECT_EQ ((UINT16)DeviceInfo->PortType, (UINT16)EFI_ACPI_DBG2_PORT_TYPE_NET);
  EXPECT_EQ (DeviceInfo->PortSubtype, (UINT16)0x0000);
  EXPECT_EQ (DeviceInfo->NumberofGenericAddressRegisters, 1U);

  // Validate that the name string is "."
  CHAR8  *NameString = (CHAR8 *)((UINT8 *)DeviceInfo + DeviceInfo->NameSpaceStringOffset);

  EXPECT_EQ (NameString[0], '.');
  EXPECT_EQ (NameString[1], '\0');

  // Validate memory range
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE  *Gas = (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE *)((UINT8 *)DeviceInfo + DeviceInfo->BaseAddressRegisterOffset);

  EXPECT_EQ (Gas->AddressSpaceId, (UINT8)EFI_ACPI_6_3_SYSTEM_MEMORY);
  EXPECT_EQ (Gas->RegisterBitWidth, (UINT8)32);
  EXPECT_EQ (Gas->RegisterBitOffset, (UINT8)0);
  EXPECT_EQ (Gas->AccessSize, (UINT8)EFI_ACPI_6_3_DWORD);
  EXPECT_EQ (Gas->Address, range.BaseAddress);

  // Validate address size
  UINT32  *AddressSize = (UINT32 *)((UINT8 *)DeviceInfo + DeviceInfo->AddressSizeOffset);

  EXPECT_EQ (*AddressSize, range.Length);

  gDbg2Generator->FreeTableResourcesEx (
                    gDbg2Generator,
                    &mAcpiTableInfo,
                    gConfigurationManagerProtocol,
                    &Table,
                    TableCount
                    );
}

// Test multiple serial ports where only first one is used
TEST_F (Dbg2GeneratorTest, MultipleSerialPorts) {
  // Create two serial ports
  std::vector<CM_ARCH_COMMON_SERIAL_PORT_INFO>  serialPorts;

  // First serial port
  CM_ARCH_COMMON_SERIAL_PORT_INFO  port1 = { 0 };

  port1.BaseAddress       = SERIAL_PORT_BASE_ADDRESS (0);
  port1.BaseAddressLength = SERIAL_PORT_BASE_ADDRESS_LENGTH;
  port1.AccessSize        = EFI_ACPI_6_3_DWORD;
  port1.BaudRate          = SERIAL_PORT_BAUD_RATE;
  port1.PortSubtype       = EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550;
  serialPorts.push_back (port1);

  // Second serial port
  CM_ARCH_COMMON_SERIAL_PORT_INFO  port2 = { 0 };

  port2.BaseAddress       = SERIAL_PORT_BASE_ADDRESS (1);
  port2.BaseAddressLength = SERIAL_PORT_BASE_ADDRESS_LENGTH;
  port2.AccessSize        = EFI_ACPI_6_3_DWORD;
  port2.BaudRate          = SERIAL_PORT_BAUD_RATE;
  port2.PortSubtype       = EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550;
  serialPorts.push_back (port2);

  // Mock serial port info query to return both ports
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(
           CM_OBJ_DESCRIPTOR {
    CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjSerialDebugPortInfo),
    (UINT32)(sizeof (CM_ARCH_COMMON_SERIAL_PORT_INFO) * serialPorts.size ()),
    serialPorts.data (),
    (UINT32)serialPorts.size ()
  }
           ),
         Return (EFI_SUCCESS)
         )
       );

  // Mock DBG2 device info query to return not found
  EXPECT_CALL (
    MockConfigMgrProtocol,
    GetObject (
      _,
      CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjGenericDbg2DeviceInfo),
      CM_NULL_TOKEN,
      _
      )
    )
    .WillOnce (Return (EFI_NOT_FOUND));

  EFI_ACPI_DESCRIPTION_HEADER  **Table    = nullptr;
  UINTN                        TableCount = 0;

  EXPECT_EQ (
    gDbg2Generator->BuildAcpiTableEx (
                      gDbg2Generator,
                      &mAcpiTableInfo,
                      gConfigurationManagerProtocol,
                      &Table,
                      &TableCount
                      ),
    EFI_SUCCESS
    );

  EXPECT_NE (Table, nullptr);
  EXPECT_EQ (TableCount, 2U);  // DBG2 table and SSDT table

  // Find the DBG2 table
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE  *Dbg2Table = nullptr;
  EFI_ACPI_DESCRIPTION_HEADER              *SsdtTable = nullptr;

  for (UINTN i = 0; i < TableCount; i++) {
    if (Table[i]->Signature == EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE) {
      Dbg2Table = (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE *)Table[i];
    } else if (Table[i]->Signature == EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      SsdtTable = Table[i];
    }
  }

  ValidateDbg2TableHeader (Dbg2Table);
  ValidateSsdtTableHeader (SsdtTable);

  // Validate DBG2 table structure - should only have one device
  EXPECT_EQ (Dbg2Table->NumberDbgDeviceInfo, 1U);
  EXPECT_NE (Dbg2Table->OffsetDbgDeviceInfo, 0U);

  // Get pointer to device information structure
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *DeviceInfo = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)Dbg2Table + Dbg2Table->OffsetDbgDeviceInfo);

  // Validate that only the first serial port is included
  ValidateSerialDeviceInfo (DeviceInfo, port1.BaseAddress, (UINT32)port1.BaseAddressLength);

  gDbg2Generator->FreeTableResourcesEx (
                    gDbg2Generator,
                    &mAcpiTableInfo,
                    gConfigurationManagerProtocol,
                    &Table,
                    TableCount
                    );
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}

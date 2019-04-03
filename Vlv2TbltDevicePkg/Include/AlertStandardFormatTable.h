/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  AlertStandardFormatTable.h

Abstract:

  ACPI Alert Standard Format Description Table ASF! as described
  in the ASF2.0 Specification

--*/

#ifndef _ALERT_STANDARD_FORMAT_TABLE_H
#define _ALERT_STANDARD_FORMAT_TABLE_H

#include <IndustryStandard/Acpi20.h>

//
// Ensure proper structure formats.
//
#pragma pack (1)

//
// Information Record header that appears at the beginning of each record.
//
typedef struct {
  UINT8                                Type;
  UINT8                                Reserved;
  UINT16                               RecordLength;
} EFI_ACPI_ASF_RECORD_HEADER;

//
// This structure contains information that identifies the system type
// and configuration.
//
typedef struct {
  EFI_ACPI_ASF_RECORD_HEADER           RecordHeader;
  UINT8                                MinWatchDogResetValue;
  UINT8                                MinPollingInterval;
  UINT16                               SystemID;
  UINT32                               IANAManufactureID;
  UINT8                                FeatureFlags;
  UINT8                                Reserved[3];
} EFI_ACPI_ASF_INFO;

//
// Alert sensors definition.
//
#define ASF_ALRT_SENSOR_ARRAY_LENGTH     36

typedef struct {
  EFI_ACPI_ASF_RECORD_HEADER           RecordHeader;
  UINT8                                AssertionEventBitMask;
  UINT8                                DeassertionEventBitMask;
  UINT8                                NumberOfAlerts;
  UINT8                                ArrayElementLength;
  UINT8                                DeviceArray[ASF_ALRT_SENSOR_ARRAY_LENGTH];
} EFI_ACPI_ASF_ALRT;

//
// Alert Remote Control System Actions.
//
#define ASF_RCTL_DEVICES_ARRAY_LENGTH      16

typedef struct {
  EFI_ACPI_ASF_RECORD_HEADER           RecordHeader;
  UINT8                                NumberOfControls;
  UINT8                                ArrayElementLength;
  UINT16                               RctlReserved;
  UINT8                                ControlArray[ASF_RCTL_DEVICES_ARRAY_LENGTH];
} EFI_ACPI_ASF_RCTL;

//
// Remote Control Capabilities.
//
typedef struct {
  EFI_ACPI_ASF_RECORD_HEADER           RecordHeader;
  UINT8                                RemoteControlCapabilities[7];
  UINT8                                RMCPCompletionCode;
  UINT32                               RMCPIANA;
  UINT8                                RMCPSpecialCommand;
  UINT8                                RMCPSpecialCommandParameter[2];
  UINT8                                RMCPBootOptions[2];
  UINT8                                RMCPOEMParameters[2];
} EFI_ACPI_ASF_RMCP;

//
// SMBus Devices with fixed addresses.
//
#define ASF_ADDR_DEVICE_ARRAY_LENGTH      16

typedef struct {
  EFI_ACPI_ASF_RECORD_HEADER           RecordHeader;
  UINT8                                SEEPROMAddress;
  UINT8                                NumberOfDevices;
  UINT8                                FixedSmbusAddresses[ASF_ADDR_DEVICE_ARRAY_LENGTH];
} EFI_ACPI_ASF_ADDR;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER          Header;
  EFI_ACPI_ASF_INFO                    AsfInfo;
  EFI_ACPI_ASF_ALRT                    AsfAlert;
  EFI_ACPI_ASF_RCTL                    AsfRctl;
  EFI_ACPI_ASF_RMCP                    AsfRmcp;
  EFI_ACPI_ASF_ADDR                    AsfAddr;
} EFI_ACPI_1_0_ASF_DESCRIPTION_TABLE;

//
// "ASF!" ASF Description Table Signature.
//
#define EFI_ACPI_1_0_ASF_DESCRIPTION_TABLE_SIGNATURE  0x21465341

#pragma pack ()

#endif // _ALERT_STANDARD_FORMAT_TABLE_H

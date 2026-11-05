/** @file
  SSDT SCMI AML Table Generator definitions.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for System Control and Management Interface
    (https://support.arm.com/documentation/111115/latest)
**/

#pragma once

/** SCMI SSDT generator revision. */
#define SSDT_SCMI_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** SCMI Device Properties package revision. */
#define SCMI_DEVICE_PROPERTIES_REVISION  1

/** SCMI ACPI device hardware identifier. */
#define SCMI_DEVICE_HID  "ARML0001"

/** SCMI power capping and monitoring protocol property key. */
#define SCMI_PCAP_PROTOCOL_PROPERTY  "arm-arml0001-protocol-pcap"

/** SCMI system telemetry protocol property key. */
#define SCMI_TELEMETRY_PROTOCOL_PROPERTY  \
  "arm-arml0001-protocol-telemetry"

/** SCMI PCC transport descriptor flags. */
#define SCMI_PCC_TRANSPORT_FLAG_COMMON  BIT0
#define SCMI_PCC_TRANSPORT_FLAG_P2A     BIT1

/** Indicates that a transport is common to all protocols. */
#define SCMI_COMMON_PROTOCOL_INDEX  MAX_UINT32

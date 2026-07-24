/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference:
    - DSP0256 version 2.0.0. (https://www.dmtf.org/dsp/DSP0256)

**/

#pragma once

#define MCTP_PROTOCOL_MAJOR_VERSION_SHIFT  (8)
#define MCTP_PROTOCOL_MAJOR_VERSION_MASK   (0xff00)
#define MCTP_PROTOCOL_MINOR_VERSION_SHIFT  (0)
#define MCTP_PROTOCOL_MINOR_VERSION_MASK   (0x00ff)
#define MCTP_PROTOCOL_VERSION_INFO(major, minor) \
        ((UINT16)((((major) & ((MCTP_PROTOCOL_MAJOR_VERSION_MASK >> \
                                MCTP_PROTOCOL_MAJOR_VERSION_SHIFT))) << \
                  MCTP_PROTOCOL_MAJOR_VERSION_SHIFT) | \
                  (((minor) & MCTP_PROTOCOL_MINOR_VERSION_MASK) << \
                   MCTP_PROTOCOL_MINOR_VERSION_SHIFT)))

#define MCTP_CHARACTERISTIC_HAS_ACPI_DSDT_DEVICE  (BIT0)
#define MCTP_CHARACTERISTIC_RESERVED_BITS         (~(BIT0))

#pragma pack(1)

/** A structure that describes MCHI MCTP MMBI specific data.

    See DSP0256 9.2 Table 3: Interface Type Specific Data.
**/
typedef struct MchiMctpMmbiSpecificData {
  /// A pointer to the MMBI capability pointer.
  UINT64    MmbiCapDesPointer;
} MCHI_MCTP_MMBI_SPECIFIC_DATA;

/** A structure that describes MCTP protocol specific data.

    See DSP0256 9.1 Table 2: Protocol Type Specific Data.
**/
typedef struct MctpProtocolData {
  /// Version (Major[15:8], Minor[7:0]).
  UINT16    Version;

  /// Link-layer type.
  UINT8     LinkLayerType;

  /// Reserved.
  UINT8     Reserved0;

  /// Instance Number.
  UINT32    Instance;

  /// Characteristics
  UINT32    Characteristics;
} MCTP_PROTOCOL_DATA;

#pragma pack()

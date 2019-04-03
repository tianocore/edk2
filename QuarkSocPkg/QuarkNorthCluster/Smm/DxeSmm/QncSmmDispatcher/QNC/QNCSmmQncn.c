/** @file
File to contain all the hardware specific stuff for the Smm QNCn dispatch protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmmHelpers.h"

QNC_SMM_SOURCE_DESC QNCN_SOURCE_DESCS[NUM_ICHN_TYPES] = {

  // QNCnMch (0)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnPme (1)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnRtcAlarm (2)
  {
    QNC_SMM_NO_FLAGS,
    {
      {{ACPI_ADDR_TYPE, {R_QNC_PM1BLK_PM1E}}, S_QNC_PM1BLK_PM1E, N_QNC_PM1BLK_PM1E_RTC},
      NULL_BIT_DESC_INITIALIZER
    },
    {
      {{ACPI_ADDR_TYPE, {R_QNC_PM1BLK_PM1S}}, S_QNC_PM1BLK_PM1S, N_QNC_PM1BLK_PM1S_RTC}
    }
  },

  // QNCnRingIndicate (3)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnAc97Wake (4)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnSerialIrq (5)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnY2KRollover (6)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnTcoTimeout (7)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnOsTco (8)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnNmi (9)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnIntruderDetect (10)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnBiosWp (11)
  {
    QNC_SMM_CLEAR_WITH_ZERO,
    {
      {
        {
          PCI_ADDR_TYPE,
          {
            (
              (PCI_BUS_NUMBER_QNC << 24) |
              (PCI_DEVICE_NUMBER_QNC_LPC << 16) |
              (PCI_FUNCTION_NUMBER_QNC_LPC << 8) |
              R_QNC_LPC_BIOS_CNTL
            )
          }
        },
        S_QNC_LPC_BIOS_CNTL,
        N_QNC_LPC_BIOS_CNTL_BLE
      },
      NULL_BIT_DESC_INITIALIZER
    },
    {
      {
        {
          PCI_ADDR_TYPE,
          {
            (
              (PCI_BUS_NUMBER_QNC << 24) |
              (PCI_DEVICE_NUMBER_QNC_LPC << 16) |
              (PCI_FUNCTION_NUMBER_QNC_LPC << 8) |
              R_QNC_LPC_BIOS_CNTL
            )
          }
        },
        S_QNC_LPC_BIOS_CNTL,
        N_QNC_LPC_BIOS_CNTL_BIOSWE
      }
    }
  },

  // QNCnMcSmi (12)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnPmeB0 (13)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnThrmSts (14)
  {
    QNC_SMM_SCI_EN_DEPENDENT,
    {
      {{GPE_ADDR_TYPE, {R_QNC_GPE0BLK_GPE0E}}, S_QNC_GPE0BLK_GPE0E, N_QNC_GPE0BLK_GPE0E_THRM},
      NULL_BIT_DESC_INITIALIZER
    },
    {
      {{GPE_ADDR_TYPE, {R_QNC_GPE0BLK_GPE0S}}, S_QNC_GPE0BLK_GPE0S, N_QNC_GPE0BLK_GPE0S_THRM}
    }
  },

  // QNCnSmBus (15)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnIntelUsb2 (16)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnMonSmi7 (17)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnMonSmi6 (18)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnMonSmi5 (19)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnMonSmi4 (20)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap13 (21)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap12 (22)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap11 (23)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap10 (24)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap9 (25)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap8 (26)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap7 (27)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap6 (28)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap5 (29)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap3 (30)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap2 (31)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap1 (32)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnDevTrap0 (33)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnIoTrap3 (34)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnIoTrap2 (35)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnIoTrap1 (36)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnIoTrap0 (37)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnPciExpress (38)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnMonitor (39)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnSpi (40)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnQRT (41)
  NULL_SOURCE_DESC_INITIALIZER,

  // QNCnGpioUnlock (42)
  NULL_SOURCE_DESC_INITIALIZER
};

VOID
QNCSmmQNCnClearSource(
  QNC_SMM_SOURCE_DESC   *SrcDesc
  )
{
    QNCSmmClearSource (SrcDesc);
}

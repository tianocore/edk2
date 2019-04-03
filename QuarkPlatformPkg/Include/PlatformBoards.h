/** @file
Board config definitions for each of the boards supported by this platform
package.

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#include "Platform.h"

#ifndef __PLATFORM_BOARDS_H__
#define __PLATFORM_BOARDS_H__

//
// Constant definition
//

//
// Default resume well TPM reset.
//
#define PLATFORM_RESUMEWELL_TPM_RST_GPIO                5

//
// Basic Configs for GPIO table definitions.
//
#define NULL_LEGACY_GPIO_INITIALIZER                    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
#define ALL_INPUT_LEGACY_GPIO_INITIALIZER               {0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x3f,0x3f,0x00,0x00,0x00,0x00,0x00,0x3f,0x00}
#define QUARK_EMULATION_LEGACY_GPIO_INITIALIZER         ALL_INPUT_LEGACY_GPIO_INITIALIZER
#define CLANTON_PEAK_SVP_LEGACY_GPIO_INITIALIZER        {0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x3f,0x3f,0x00,0x00,0x3f,0x3f,0x00,0x3f,0x00}
#define KIPS_BAY_LEGACY_GPIO_INITIALIZER                {0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x3f,0x25,0x10,0x00,0x00,0x00,0x00,0x3f,0x00}
#define CROSS_HILL_LEGACY_GPIO_INITIALIZER              {0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x3f,0x03,0x10,0x00,0x03,0x03,0x00,0x3f,0x00}
#define CLANTON_HILL_LEGACY_GPIO_INITIALIZER            {0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x3f,0x06,0x10,0x00,0x04,0x04,0x00,0x3f,0x00}
#define GALILEO_LEGACY_GPIO_INITIALIZER                 {0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x3f,0x21,0x14,0x00,0x00,0x00,0x00,0x3f,0x00}
#define GALILEO_GEN2_LEGACY_GPIO_INITIALIZER            {0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x3f,0x1c,0x02,0x00,0x00,0x00,0x00,0x3f,0x00}

#define NULL_GPIO_CONTROLLER_INITIALIZER                {0,0,0,0,0,0,0,0}
#define ALL_INPUT_GPIO_CONTROLLER_INITIALIZER           NULL_GPIO_CONTROLLER_INITIALIZER
#define QUARK_EMULATION_GPIO_CONTROLLER_INITIALIZER     NULL_GPIO_CONTROLLER_INITIALIZER
#define CLANTON_PEAK_SVP_GPIO_CONTROLLER_INITIALIZER    NULL_GPIO_CONTROLLER_INITIALIZER
#define KIPS_BAY_GPIO_CONTROLLER_INITIALIZER            {0x05,0x05,0,0,0,0,0,0}
#define CROSS_HILL_GPIO_CONTROLLER_INITIALIZER          {0x0D,0x2D,0,0,0,0,0,0}
#define CLANTON_HILL_GPIO_CONTROLLER_INITIALIZER        {0x01,0x39,0,0,0,0,0,0}
#define GALILEO_GPIO_CONTROLLER_INITIALIZER             {0x05,0x15,0,0,0,0,0,0}
#define GALILEO_GEN2_GPIO_CONTROLLER_INITIALIZER        {0x05,0x05,0,0,0,0,0,0}

//
// Legacy Gpio to be used to assert / deassert PCI express PERST# signal
// on Galileo Gen 2 platform.
//
#define GALILEO_GEN2_PCIEXP_PERST_RESUMEWELL_GPIO       0

//
// Io expander slave address.
//

//
// On Galileo value of Jumper J2 determines slave address of io expander.
//
#define GALILEO_DETERMINE_IOEXP_SLA_RESUMEWELL_GPIO     5
#define GALILEO_IOEXP_J2HI_7BIT_SLAVE_ADDR              0x20
#define GALILEO_IOEXP_J2LO_7BIT_SLAVE_ADDR              0x21

//
// Three IO Expmanders at fixed addresses on Galileo Gen2.
//
#define GALILEO_GEN2_IOEXP0_7BIT_SLAVE_ADDR             0x25
#define GALILEO_GEN2_IOEXP1_7BIT_SLAVE_ADDR             0x26
#define GALILEO_GEN2_IOEXP2_7BIT_SLAVE_ADDR             0x27

//
// Led GPIOs for flash update / recovery.
//
#define GALILEO_FLASH_UPDATE_LED_RESUMEWELL_GPIO        1
#define GALILEO_GEN2_FLASH_UPDATE_LED_RESUMEWELL_GPIO   5

//
// Legacy GPIO config struct for each element in PLATFORM_LEGACY_GPIO_TABLE_DEFINITION.
//
typedef struct {
  UINT32  CoreWellEnable;               ///< Value for QNC NC Reg R_QNC_GPIO_CGEN_CORE_WELL.
  UINT32  CoreWellIoSelect;             ///< Value for QNC NC Reg R_QNC_GPIO_CGIO_CORE_WELL.
  UINT32  CoreWellLvlForInputOrOutput;  ///< Value for QNC NC Reg R_QNC_GPIO_CGLVL_CORE_WELL.
  UINT32  CoreWellTriggerPositiveEdge;  ///< Value for QNC NC Reg R_QNC_GPIO_CGTPE_CORE_WELL.
  UINT32  CoreWellTriggerNegativeEdge;  ///< Value for QNC NC Reg R_QNC_GPIO_CGTNE_CORE_WELL.
  UINT32  CoreWellGPEEnable;            ///< Value for QNC NC Reg R_QNC_GPIO_CGGPE_CORE_WELL.
  UINT32  CoreWellSMIEnable;            ///< Value for QNC NC Reg R_QNC_GPIO_CGSMI_CORE_WELL.
  UINT32  CoreWellTriggerStatus;        ///< Value for QNC NC Reg R_QNC_GPIO_CGTS_CORE_WELL.
  UINT32  CoreWellNMIEnable;            ///< Value for QNC NC Reg R_QNC_GPIO_CGNMIEN_CORE_WELL.
  UINT32  ResumeWellEnable;             ///< Value for QNC NC Reg R_QNC_GPIO_RGEN_RESUME_WELL.
  UINT32  ResumeWellIoSelect;           ///< Value for QNC NC Reg R_QNC_GPIO_RGIO_RESUME_WELL.
  UINT32  ResumeWellLvlForInputOrOutput;///< Value for QNC NC Reg R_QNC_GPIO_RGLVL_RESUME_WELL.
  UINT32  ResumeWellTriggerPositiveEdge;///< Value for QNC NC Reg R_QNC_GPIO_RGTPE_RESUME_WELL.
  UINT32  ResumeWellTriggerNegativeEdge;///< Value for QNC NC Reg R_QNC_GPIO_RGTNE_RESUME_WELL.
  UINT32  ResumeWellGPEEnable;          ///< Value for QNC NC Reg R_QNC_GPIO_RGGPE_RESUME_WELL.
  UINT32  ResumeWellSMIEnable;          ///< Value for QNC NC Reg R_QNC_GPIO_RGSMI_RESUME_WELL.
  UINT32  ResumeWellTriggerStatus;      ///< Value for QNC NC Reg R_QNC_GPIO_RGTS_RESUME_WELL.
  UINT32  ResumeWellNMIEnable;          ///< Value for QNC NC Reg R_QNC_GPIO_RGNMIEN_RESUME_WELL.
} BOARD_LEGACY_GPIO_CONFIG;

//
// GPIO controller config struct for each element in PLATFORM_GPIO_CONTROLLER_CONFIG_DEFINITION.
//
typedef struct {
  UINT32  PortADR;                      ///< Value for IOH REG GPIO_SWPORTA_DR.
  UINT32  PortADir;                     ///< Value for IOH REG GPIO_SWPORTA_DDR.
  UINT32  IntEn;                        ///< Value for IOH REG GPIO_INTEN.
  UINT32  IntMask;                      ///< Value for IOH REG GPIO_INTMASK.
  UINT32  IntType;                      ///< Value for IOH REG GPIO_INTTYPE_LEVEL.
  UINT32  IntPolarity;                  ///< Value for IOH REG GPIO_INT_POLARITY.
  UINT32  Debounce;                     ///< Value for IOH REG GPIO_DEBOUNCE.
  UINT32  LsSync;                       ///< Value for IOH REG GPIO_LS_SYNC.
} BOARD_GPIO_CONTROLLER_CONFIG;

///
/// Table of BOARD_LEGACY_GPIO_CONFIG structures for each board supported
/// by this platform package.
/// Table indexed with EFI_PLATFORM_TYPE enum value.
///
#define PLATFORM_LEGACY_GPIO_TABLE_DEFINITION \
  /* EFI_PLATFORM_TYPE - TypeUnknown*/\
  NULL_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - QuarkEmulation*/\
  QUARK_EMULATION_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - ClantonPeakSVP*/\
  CLANTON_PEAK_SVP_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - KipsBay*/\
  KIPS_BAY_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - CrossHill*/\
  CROSS_HILL_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - ClantonHill*/\
  CLANTON_HILL_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - Galileo*/\
  GALILEO_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - TypePlatformRsv7*/\
  NULL_LEGACY_GPIO_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - GalileoGen2*/\
  GALILEO_GEN2_LEGACY_GPIO_INITIALIZER,\

///
/// Table of BOARD_GPIO_CONTROLLER_CONFIG structures for each board
/// supported by this platform package.
/// Table indexed with EFI_PLATFORM_TYPE enum value.
///
#define PLATFORM_GPIO_CONTROLLER_CONFIG_DEFINITION \
  /* EFI_PLATFORM_TYPE - TypeUnknown*/\
  NULL_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - QuarkEmulation*/\
  QUARK_EMULATION_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - ClantonPeakSVP*/\
  CLANTON_PEAK_SVP_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - KipsBay*/\
  KIPS_BAY_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - CrossHill*/\
  CROSS_HILL_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - ClantonHill*/\
  CLANTON_HILL_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - Galileo*/\
  GALILEO_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - TypePlatformRsv7 */\
  NULL_GPIO_CONTROLLER_INITIALIZER,\
  /* EFI_PLATFORM_TYPE - GalileoGen2*/\
  GALILEO_GEN2_GPIO_CONTROLLER_INITIALIZER,\

#endif

/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Omap3530/Omap3530.h>
#include <BeagleBoard.h>

#define NUM_PINS_SHARED 232
#define NUM_PINS_ABC 6
#define NUM_PINS_XM 12

PAD_CONFIGURATION PadConfigurationTableShared[] = {
  //Pin,           MuxMode,    PullConfig,                      InputEnable
  { SDRC_D0,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D1,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D2,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D3,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D4,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D5,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D6,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D7,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D8,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D9,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D10,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D11,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D12,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D13,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D14,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D15,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D16,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D17,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D18,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D19,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D20,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D21,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D22,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D23,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D24,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D25,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D26,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D27,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D28,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D29,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D30,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_D31,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_CLK,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_DQS0,     MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_CKE0,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { SDRC_CKE1,     MUXMODE7,   PULL_DISABLED,                INPUT  },
  { SDRC_DQS1,     MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_DQS2,     MUXMODE0,   PULL_DISABLED,                INPUT  },
  { SDRC_DQS3,     MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_A1,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A2,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A3,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A4,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A5,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A6,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A7,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A8,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A9,       MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_A10,      MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_D0,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D1,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D2,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D3,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D4,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D5,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D6,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D7,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D8,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D9,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D10,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D11,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D12,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D13,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D14,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_D15,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_NCS0,     MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_NCS1,     MUXMODE0,   PULL_UP_SELECTED,             OUTPUT },
  { GPMC_NCS2,     MUXMODE0,   PULL_UP_SELECTED,             OUTPUT },
  { GPMC_NCS3,     MUXMODE0,   PULL_UP_SELECTED,             OUTPUT },
  { GPMC_NCS4,     MUXMODE0,   PULL_UP_SELECTED,             OUTPUT },
  { GPMC_NCS5,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_NCS6,     MUXMODE1,   PULL_DISABLED,                INPUT  },
  { GPMC_NCS7,     MUXMODE1,   PULL_UP_SELECTED,             INPUT  },
  { GPMC_CLK,      MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_NADV_ALE, MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_NOE,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_NWE,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_NBE0_CLE, MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { GPMC_NBE1,     MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_NWP,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { GPMC_WAIT0,    MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { GPMC_WAIT1,    MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { GPMC_WAIT2,    MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { GPMC_WAIT3,    MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { DSS_PCLK,      MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_HSYNC,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_PSYNC,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_ACBIAS,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA0,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA1,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA2,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA3,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA4,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA5,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA6,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA7,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA8,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA9,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA10,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA11,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA12,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA13,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA14,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA15,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA16,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA17,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { CAM_HS,        MUXMODE0,   PULL_UP_SELECTED,             INPUT },
  { CAM_VS,        MUXMODE0,   PULL_UP_SELECTED,             INPUT },
  { CAM_XCLKA,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { CAM_PCLK,      MUXMODE0,   PULL_UP_SELECTED,             INPUT },
  { CAM_FLD,       MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { CAM_D0,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D1,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D2,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D3,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D4,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D5,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D6,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D7,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D8,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D9,        MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D10,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_D11,       MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CAM_XCLKB,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { CAM_WEN,       MUXMODE4,   PULL_DISABLED,                INPUT  },
  { CAM_STROBE,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { CSI2_DX0,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CSI2_DY0,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CSI2_DX1,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { CSI2_DY1,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { MCBSP2_FSX,    MUXMODE0,   PULL_DISABLED,                INPUT  },
  { MCBSP2_CLKX,   MUXMODE0,   PULL_DISABLED,                INPUT  },
  { MCBSP2_DR,     MUXMODE0,   PULL_DISABLED,                INPUT  },
  { MCBSP2_DX,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { MMC1_CLK,      MUXMODE0,   PULL_UP_SELECTED,             OUTPUT },
  { MMC1_CMD,      MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT0,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT1,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT2,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT3,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT4,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT5,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT6,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC1_DAT7,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_CLK,      MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_CMD,      MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT0,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT1,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT2,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT3,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT4,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT5,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT6,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MMC2_DAT7,     MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MCBSP3_DX,     MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCBSP3_DR,     MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCBSP3_CLKX,   MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCBSP3_FSX,    MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { UART2_CTS,     MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { UART2_RTS,     MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { UART2_TX,      MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { UART2_RX,      MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { UART1_TX,      MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { UART1_RTS,     MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { UART1_CTS,     MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { UART1_RX,      MUXMODE0,   PULL_DISABLED,                INPUT  },
  { MCBSP4_CLKX,   MUXMODE1,   PULL_DISABLED,                INPUT  },
  { MCBSP4_DR,     MUXMODE1,   PULL_DISABLED,                INPUT  },
  { MCBSP4_DX,     MUXMODE1,   PULL_DISABLED,                INPUT  },
  { MCBSP4_FSX,    MUXMODE1,   PULL_DISABLED,                INPUT  },
  { MCBSP1_CLKR,   MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCBSP1_FSR,    MUXMODE4,   PULL_UP_SELECTED,             OUTPUT },
  { MCBSP1_DX,     MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCBSP1_DR,     MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCBSP1_CLKS,   MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { MCBSP1_FSX,    MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCBSP1_CLKX,   MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { UART3_CTS_RCTX,MUXMODE0,   PULL_UP_SELECTED,                 INPUT  },
  { UART3_RTS_SD,  MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { UART3_RX_IRRX, MUXMODE0,   PULL_DISABLED,                INPUT  },
  { UART3_TX_IRTX, MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { HSUSB0_CLK,    MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_STP,    MUXMODE0,   PULL_UP_SELECTED,             OUTPUT },
  { HSUSB0_DIR,    MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_NXT,    MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA0,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA1,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA2,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA3,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA4,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA5,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA6,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { HSUSB0_DATA7,  MUXMODE0,   PULL_DISABLED,                INPUT  },
  { I2C1_SCL,      MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { I2C1_SDA,      MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { I2C2_SCL,      MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { I2C2_SDA,      MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { I2C3_SCL,      MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { I2C3_SDA,      MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { HDQ_SIO,       MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCSPI1_CLK,    MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MCSPI1_SIMO,   MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { MCSPI1_SOMI,   MUXMODE0,   PULL_DISABLED,                INPUT  },
  { MCSPI1_CS0,    MUXMODE0,   PULL_UP_SELECTED,                 INPUT  },
  { MCSPI1_CS1,    MUXMODE0,   PULL_UP_SELECTED,                 OUTPUT },
  { MCSPI1_CS2,    MUXMODE4,   PULL_DISABLED,                OUTPUT },
  { MCSPI1_CS3,    MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { MCSPI2_CLK,    MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { MCSPI2_SIMO,   MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { MCSPI2_SOMI,   MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { MCSPI2_CS0,    MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { MCSPI2_CS1,    MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { SYS_NIRQ,      MUXMODE0,   PULL_UP_SELECTED,             INPUT  },
  { SYS_CLKOUT2,   MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { ETK_CLK,       MUXMODE3,   PULL_UP_SELECTED,             OUTPUT },
  { ETK_CTL,       MUXMODE3,   PULL_UP_SELECTED,             OUTPUT },
  { ETK_D0,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D1,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D2,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D3,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D4,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D5,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D6,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D7,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D8,        MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D9,        MUXMODE4,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D10,       MUXMODE3,   PULL_UP_SELECTED,             OUTPUT },
  { ETK_D11,       MUXMODE3,   PULL_UP_SELECTED,             OUTPUT },
  { ETK_D12,       MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D13,       MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D14,       MUXMODE3,   PULL_UP_SELECTED,             INPUT  },
  { ETK_D15,       MUXMODE3,   PULL_UP_SELECTED,             INPUT  }
};

PAD_CONFIGURATION PadConfigurationTableAbc[] = {
  { DSS_DATA18,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA19,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA20,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA21,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA22,    MUXMODE0,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA23,    MUXMODE0,   PULL_DISABLED,                OUTPUT }
};

PAD_CONFIGURATION PadConfigurationTableXm[] = {
  { DSS_DATA18,    MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA19,    MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA20,    MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA21,    MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA22,    MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { DSS_DATA23,    MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { SYS_BOOT0,     MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { SYS_BOOT1,     MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { SYS_BOOT3,     MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { SYS_BOOT4,     MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { SYS_BOOT5,     MUXMODE3,   PULL_DISABLED,                OUTPUT },
  { SYS_BOOT6,     MUXMODE3,   PULL_DISABLED,                OUTPUT }
};

VOID
PadConfiguration (
  BEAGLEBOARD_REVISION Revision
  )
{
  UINTN             Index;
  UINT16            PadConfiguration;
  PAD_CONFIGURATION *BoardConfiguration;
  UINTN             NumPinsToConfigure;

  for (Index = 0; Index < NUM_PINS_SHARED; Index++) {
    // Set up Pad configuration for particular pin.
    PadConfiguration =  (PadConfigurationTableShared[Index].MuxMode << MUXMODE_OFFSET);
    PadConfiguration |= (PadConfigurationTableShared[Index].PullConfig << PULL_CONFIG_OFFSET);
    PadConfiguration |= (PadConfigurationTableShared[Index].InputEnable << INPUTENABLE_OFFSET);

    // Configure the pin with specific Pad configuration.
    MmioWrite16(PadConfigurationTableShared[Index].Pin, PadConfiguration);
  }

  if (Revision == REVISION_XM) {
    BoardConfiguration = PadConfigurationTableXm;
    NumPinsToConfigure = NUM_PINS_XM;
  } else {
    BoardConfiguration = PadConfigurationTableAbc;
    NumPinsToConfigure = NUM_PINS_ABC;
  }

  for (Index = 0; Index < NumPinsToConfigure; Index++) {
    //Set up Pad configuration for particular pin.
    PadConfiguration =  (BoardConfiguration[Index].MuxMode << MUXMODE_OFFSET);
    PadConfiguration |= (BoardConfiguration[Index].PullConfig << PULL_CONFIG_OFFSET);
    PadConfiguration |= (BoardConfiguration[Index].InputEnable << INPUTENABLE_OFFSET);

    //Configure the pin with specific Pad configuration.
    MmioWrite16(BoardConfiguration[Index].Pin, PadConfiguration);
  }
}

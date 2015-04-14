/**@file
  Gpio setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
**/

#ifndef _BOARDGPIOS_H_
#define _BOARDGPIOS_H_

#include <PiPei.h>
#include "PchAccess.h"
#include "PlatformBaseAddresses.h"
#include <../MultiPlatformLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Guid/PlatformInfo.h>
#include <Ppi/Smbus.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/SetupVariable.h>


GPIO_CONF_PAD_INIT mNB_BB_FAB3_GpioInitData_SC_TRI[] =
{
//              Pad Name          GPIO Number     Used As   GPO Default  Function#     INT Capable   Interrupt Type   PULL H/L    MMIO Offset
GPIO_INIT_ITEM("LPC_CLKOUT1       GPIOC_48 "     ,TRISTS   ,NA           ,F0           ,             ,                ,NONE       ,0x41),
GPIO_INIT_ITEM("PLT_CLK0          GPIOC_96 "     ,TRISTS   ,NA           ,F0           ,             ,                ,NONE       ,0x6a),
GPIO_INIT_ITEM("PLT_CLK3          GPIOC_99 "     ,TRISTS   ,NA           ,F0           ,             ,                ,NONE       ,0x68),
};

//
// Minnow2
//
#define MINNOW2_GPIO_USE_SEL_VAL_0_31        0x00000000
#define MINNOW2_GPIO_USE_SEL_VAL_32_63       0x00000000
#define MINNOW2_GPIO_USE_SEL_VAL_64_70       0x00000000
#define MINNOW2_GPIO_USE_SEL_VAL_64_70       0x00000000
#define MINNOW2_GPIO_USE_SEL_VAL_SUS         0x00000000
#define MINNOW2_GPIO_USE_SEL_VAL_SUS2        0x00000007
  

#define MINNOW2_GPIO_IO_SEL_VAL_0_31         0x00000000
#define MINNOW2_GPIO_IO_SEL_VAL_32_63        0x00000000
#define MINNOW2_GPIO_IO_SEL_VAL_64_70        0x00000000
#define MINNOW2_GPIO_IO_SEL_VAL_SUS          0x00000000
#define MINNOW2_GPIO_IO_SEL_VAL_SUS2         0x00000007   


#define MINNOW2_GPIO_LVL_VAL_0_31            0x00000000
#define MINNOW2_GPIO_LVL_VAL_32_63           0x00000000
#define MINNOW2_GPIO_LVL_VAL_64_70           0x00000000
#define MINNOW2_GPIO_LVL_VAL_SUS             0x00000000
#define MINNOW2_GPIO_LVL_VAL_SUS2            0x00000007   

#define MINNOW2_GPIO_TPE_VAL_0_31            0x00000000
#define MINNOW2_GPIO_TPE_VAL_SUS             0x00000000
#define MINNOW2_GPIO_TPE_VAL_SUS2            0x00000007   

#define MINNOW2_GPIO_TNE_VAL_0_31            0x00000000
#define MINNOW2_GPIO_TNE_VAL_SUS             0x00000000
#define MINNOW2_GPIO_TNE_VAL_SUS2            0x00000007   

#define MINNOW2_GPIO_TS_VAL_0_31             0x00000000
#define MINNOW2_GPIO_TS_VAL_SUS              0x00000000
#define MINNOW2_GPIO_TS_VAL_SUS2             0x00000007   

static CFIO_INIT_STRUCT mMinnow2CfioInitData =
{
        MINNOW2_GPIO_USE_SEL_VAL_0_31,
        MINNOW2_GPIO_USE_SEL_VAL_32_63,
        MINNOW2_GPIO_USE_SEL_VAL_64_70,
        MINNOW2_GPIO_USE_SEL_VAL_SUS,

        MINNOW2_GPIO_IO_SEL_VAL_0_31,
        MINNOW2_GPIO_IO_SEL_VAL_32_63,
        MINNOW2_GPIO_IO_SEL_VAL_64_70,
        MINNOW2_GPIO_IO_SEL_VAL_SUS,

        MINNOW2_GPIO_LVL_VAL_0_31,
        MINNOW2_GPIO_LVL_VAL_32_63,
        MINNOW2_GPIO_LVL_VAL_64_70,
        MINNOW2_GPIO_LVL_VAL_SUS,

        MINNOW2_GPIO_TPE_VAL_0_31,
        MINNOW2_GPIO_TPE_VAL_SUS,
        MINNOW2_GPIO_TNE_VAL_0_31,
        MINNOW2_GPIO_TNE_VAL_SUS,

        MINNOW2_GPIO_TS_VAL_0_31,
        MINNOW2_GPIO_TS_VAL_SUS
};

static CFIO_INIT_STRUCT mMinnow2CfioInitData2 =
{
        MINNOW2_GPIO_USE_SEL_VAL_0_31,
        MINNOW2_GPIO_USE_SEL_VAL_32_63,
        MINNOW2_GPIO_USE_SEL_VAL_64_70,
        MINNOW2_GPIO_USE_SEL_VAL_SUS2,

        MINNOW2_GPIO_IO_SEL_VAL_0_31,
        MINNOW2_GPIO_IO_SEL_VAL_32_63,
        MINNOW2_GPIO_IO_SEL_VAL_64_70,
        MINNOW2_GPIO_IO_SEL_VAL_SUS2,

        MINNOW2_GPIO_LVL_VAL_0_31,
        MINNOW2_GPIO_LVL_VAL_32_63,
        MINNOW2_GPIO_LVL_VAL_64_70,
        MINNOW2_GPIO_LVL_VAL_SUS2,

        MINNOW2_GPIO_TPE_VAL_0_31,
        MINNOW2_GPIO_TPE_VAL_SUS2,		
        MINNOW2_GPIO_TNE_VAL_0_31,
        MINNOW2_GPIO_TNE_VAL_SUS2,

        MINNOW2_GPIO_TS_VAL_0_31,
        MINNOW2_GPIO_TS_VAL_SUS2
};

static GPIO_CONF_PAD_INIT mMinnow2_GpioInitData_NC[] =
{
//              Pad Name          GPIO Number     Used As   GPO Default   Function#     INT Capable   Interrupt Type   PULL H/L    MMIO Offset
GPIO_INIT_ITEM("HV_DDI0_HPD       GPIONC_0 "     ,Native   ,NA           ,F2           ,             ,                ,NONE       ,0x13),
GPIO_INIT_ITEM("HV_DDI0_DDC_SDA   GPIONC_1 "     ,Native   ,NA           ,F2           ,             ,                ,NONE       ,0x12),
GPIO_INIT_ITEM("HV_DDI0_DDC_SCL   GPIONC_2 "     ,Native   ,NA           ,F2           ,             ,                ,NONE       ,0x11),
GPIO_INIT_ITEM("PANEL0_VDDEN      GPIONC_3 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x14),
GPIO_INIT_ITEM("PANEL0_BKLTEN     GPIONC_4 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x15),
GPIO_INIT_ITEM("PANEL0_BKLTCTL    GPIONC_5 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x16),
GPIO_INIT_ITEM("HV_DDI1_HPD       GPIONC_6 "     ,Native   ,NA           ,F2           ,             ,                ,NONE       ,0x18),
GPIO_INIT_ITEM("HV_DDI1_DDC_SDA   GPIONC_7 "     ,Native   ,NA           ,F2           ,             ,                ,NONE       ,0x19),
GPIO_INIT_ITEM("HV_DDI1_DDC_SCL   GPIONC_8 "     ,Native   ,NA           ,F2           ,             ,                ,NONE       ,0x17),
GPIO_INIT_ITEM("PANEL1_VDDEN      GPIONC_9 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x10),
GPIO_INIT_ITEM("PANEL1_BKLTEN     GPIONC_10"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x0e),
GPIO_INIT_ITEM("PANEL1_BKLTCTL    GPIONC_11"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x0f),
GPIO_INIT_ITEM("GP_INTD_DSI_TE1   GPIONC_12"     ,GPO      ,NA           ,F0           ,             ,                ,NONE       ,0x0c),
GPIO_INIT_ITEM("HV_DDI2_DDC_SDA   GPIONC_13"     ,GPI      ,NA           ,F0           ,             ,                ,10K_L      ,0x1a),
GPIO_INIT_ITEM("HV_DDI2_DDC_SCL   GPIONC_14"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x1b),
GPIO_INIT_ITEM("GP_CAMERASB00     GPIONC_15"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x01),
GPIO_INIT_ITEM("GP_CAMERASB01     GPIONC_16"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x04),
GPIO_INIT_ITEM("GP_CAMERASB02     GPIONC_17"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x08),
GPIO_INIT_ITEM("GP_CAMERASB03     GPIONC_18"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x0b),
GPIO_INIT_ITEM("GP_CAMERASB04     GPIONC_19"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x00),
GPIO_INIT_ITEM("GP_CAMERASB05     GPIONC_20"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x03),
GPIO_INIT_ITEM("GP_CAMERASB06     GPIONC_21"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x06),
GPIO_INIT_ITEM("GP_CAMERASB07     GPIONC_22"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x0a),
GPIO_INIT_ITEM("GP_CAMERASB08     GPIONC_23"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x0d),
GPIO_INIT_ITEM("GP_CAMERASB09     GPIONC_24"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x02),
GPIO_INIT_ITEM("GP_CAMERASB10     GPIONC_25"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x05),
GPIO_INIT_ITEM("GP_CAMERASB11     GPIONC_26"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x09),
};


static GPIO_CONF_PAD_INIT mMinnow2_GpioInitData_SC[] = {
//              Pad Name          GPIO Number     Used As   GPO Default   Function#     INT Capable   Interrupt Type   PULL H/L    MMIO Offset
GPIO_INIT_ITEM("SATA_GP0          GPIOC_0  "     ,Native   ,NA           ,F1           ,             ,                ,NONE       ,0x55),
GPIO_INIT_ITEM("SATA_GP1          GPIOC_1  "     ,Native   ,NA           ,F1           ,             ,                ,NONE       ,0x59),
GPIO_INIT_ITEM("SATA_LEDN         GPIOC_2  "     ,Native   ,NA           ,F1           ,             ,                ,NONE       ,0x5d),
GPIO_INIT_ITEM("PCIE_CLKREQ0B     GPIOC_3  "     ,Native   ,NA           ,F1           ,             ,                ,10K_H      ,0x60),
GPIO_INIT_ITEM("PCIE_CLKREQ1B     GPIOC_4  "     ,Native   ,NA           ,F1           ,             ,                ,10K_H      ,0x63),
GPIO_INIT_ITEM("PCIE_CLKREQ2B     GPIOC_5  "     ,Native   ,NA           ,F1           ,             ,                ,10K_H      ,0x66),
GPIO_INIT_ITEM("PCIE_CLKREQ3B     GPIOC_6  "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x62),
GPIO_INIT_ITEM("SDMMC3_WP         GPIOC_7  "     ,Native   ,NA           ,F2           ,             ,                ,NONE       ,0x65),
GPIO_INIT_ITEM("HDA_RSTB          GPIOC_8  "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x22),
GPIO_INIT_ITEM("HDA_SYNC          GPIOC_9  "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x25),
GPIO_INIT_ITEM("HDA_CLK           GPIOC_10 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x24),
GPIO_INIT_ITEM("HDA_SDO           GPIOC_11 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x26),
GPIO_INIT_ITEM("HDA_SDI0          GPIOC_12 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x27),
GPIO_INIT_ITEM("HDA_SDI1          GPIOC_13 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x23),
GPIO_INIT_ITEM("HDA_DOCKRSTB      GPIOC_14 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x28),
GPIO_INIT_ITEM("HDA_DOCKENB       GPIOC_15 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x54),
GPIO_INIT_ITEM("SDMMC1_CLK        GPIOC_16 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x3e),
GPIO_INIT_ITEM("SDMMC1_D0         GPIOC_17 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x3d),
GPIO_INIT_ITEM("SDMMC1_D1         GPIOC_18 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x40),
GPIO_INIT_ITEM("SDMMC1_D2         GPIOC_19 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x3b),
GPIO_INIT_ITEM("SDMMC1_D3_CD_B    GPIOC_20 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x36),
GPIO_INIT_ITEM("MMC1_D4_SD_WE     GPIOC_21 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x38),
GPIO_INIT_ITEM("MMC1_D5           GPIOC_22 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x3c),
GPIO_INIT_ITEM("MMC1_D6           GPIOC_23 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x37),
GPIO_INIT_ITEM("MMC1_D7           GPIOC_24 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x3f),
GPIO_INIT_ITEM("SDMMC1_CMD        GPIOC_25 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x39),
GPIO_INIT_ITEM("MMC1_RESET_B      GPIOC_26 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x33),
GPIO_INIT_ITEM("SDMMC2_CLK        GPIOC_27 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x32),
GPIO_INIT_ITEM("SDMMC2_D0         GPIOC_28 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x35),
GPIO_INIT_ITEM("SDMMC2_D1         GPIOC_29 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x2f),
GPIO_INIT_ITEM("SDMMC2_D2         GPIOC_30 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x34),
GPIO_INIT_ITEM("SDMMC2_D3_CD_B    GPIOC_31 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x31),
GPIO_INIT_ITEM("SDMMC2_CMD        GPIOC_32 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x30),
//
//Just for test, We make the setting that all is same with Bayleybay.
//
GPIO_INIT_ITEM("SDMMC3_CLK        GPIOC_33 "     ,Native   ,NA           ,F1           ,             ,                ,NONE      ,0x2b),
GPIO_INIT_ITEM("SDMMC3_D0         GPIOC_34 "     ,Native   ,NA           ,F1           ,             ,                ,NONE      ,0x2e),
GPIO_INIT_ITEM("SDMMC3_D1         GPIOC_35 "     ,Native   ,NA           ,F1           ,YES          ,                ,NONE      ,0x29),
GPIO_INIT_ITEM("SDMMC3_D2         GPIOC_36 "     ,Native   ,NA           ,F1           ,             ,                ,NONE      ,0x2d),
GPIO_INIT_ITEM("SDMMC3_D3         GPIOC_37 "     ,Native   ,NA           ,F1           ,             ,                ,NONE      ,0x2a),
GPIO_INIT_ITEM("SDMMC3_CD_B       GPIOC_38 "     ,Native   ,NA           ,F1           ,YES          ,Edge_Both       ,NONE      ,0x3a),
GPIO_INIT_ITEM("SDMMC3_CMD        GPIOC_39 "     ,Native   ,NA           ,F1           ,             ,                ,NONE      ,0x2c),
GPIO_INIT_ITEM("SDMMC3_1P8_EN     GPIOC_40 "     ,Native   ,NA           ,F1           ,             ,                ,NONE      ,0x5f),
GPIO_INIT_ITEM("SDMMC3_PWR_EN_B   GPIOC_41 "     ,Native   ,NA           ,F1           ,             ,                ,NONE      ,0x69),
GPIO_INIT_ITEM("LPC_AD0           GPIOC_42 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x46),
GPIO_INIT_ITEM("LPC_AD1           GPIOC_43 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x44),
GPIO_INIT_ITEM("LPC_AD2           GPIOC_44 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x43),
GPIO_INIT_ITEM("LPC_AD3           GPIOC_45 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x42),
GPIO_INIT_ITEM("LPC_FRAMEB        GPIOC_46 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x45),
GPIO_INIT_ITEM("LPC_CLKOUT0       GPIOC_47 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x47),
GPIO_INIT_ITEM("LPC_CLKOUT1       GPIOC_48 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x41),
GPIO_INIT_ITEM("LPC_CLKRUNB       GPIOC_49 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x48),
GPIO_INIT_ITEM("ILB_SERIRQ        GPIOC_50 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x56),
GPIO_INIT_ITEM("SMB_DATA          GPIOC_51 "     ,Native   ,NA           ,F1           ,             ,                ,NONE       ,0x5a),
GPIO_INIT_ITEM("SMB_CLK           GPIOC_52 "     ,Native   ,NA           ,F1           ,             ,                ,NONE       ,0x58),
GPIO_INIT_ITEM("SMB_ALERTB        GPIOC_53 "     ,Native   ,NA           ,F1           ,             ,                ,10K_H      ,0x5c),
GPIO_INIT_ITEM("SPKR              GPIOC_54 "     ,GPI     ,NA           ,F0           ,             ,                ,20K_H      ,0x67),
GPIO_INIT_ITEM("MHSI_ACDATA       GPIOC_55 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x4d),
GPIO_INIT_ITEM("MHSI_ACFLAG       GPIOC_56 "     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x4f),
GPIO_INIT_ITEM("MHSI_ACWAKE       GPIOC_58 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x4e),
GPIO_INIT_ITEM("MHSI_CADATA       GPIOC_59 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x51),
GPIO_INIT_ITEM("MHSI_CAFLAG       GPIOC_60 "     ,GPO      ,HI           ,F0           ,             ,                ,20K_H      ,0x50),
GPIO_INIT_ITEM("GP_SSP_2_CLK      GPIOC_62 "     ,GPI     ,NA           ,F0           ,             ,                ,20K_H      ,0x0d),
GPIO_INIT_ITEM("GP_SSP_2_FS       GPIOC_63 "     ,GPI     ,NA           ,F0           ,             ,                ,20K_H      ,0x0c),
GPIO_INIT_ITEM("GP_SSP_2_RXD      GPIOC_64 "     ,GPI       ,NA           ,F0           ,             ,              ,20K_H      ,0x0f),
GPIO_INIT_ITEM("GP_SSP_2_TXD      GPIOC_65 "     ,GPI     ,NA           ,F0           ,             ,                ,20K_H      ,0x0e),
GPIO_INIT_ITEM("SPI1_CS0_B        GPIOC_66 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x11),
GPIO_INIT_ITEM("SPI1_MISO         GPIOC_67 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x12),
GPIO_INIT_ITEM("SPI1_MOSI         GPIOC_68 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x13),
GPIO_INIT_ITEM("SPI1_CLK          GPIOC_69 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x10),
GPIO_INIT_ITEM("UART1_RXD         GPIOC_70 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x02),
GPIO_INIT_ITEM("UART1_TXD         GPIOC_71 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x01),
GPIO_INIT_ITEM("UART1_RTS_B       GPIOC_72 "     ,GPI     ,NA           ,F0           ,             ,                ,20K_H      ,0x00),
GPIO_INIT_ITEM("UART1_CTS_B       GPIOC_73 "     ,GPI     ,NA           ,F0           ,             ,                ,20K_H      ,0x04),
GPIO_INIT_ITEM("UART2_RXD         GPIOC_74 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x06),
GPIO_INIT_ITEM("UART2_TXD         GPIOC_75 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x07),
GPIO_INIT_ITEM("UART2_RTS_B       GPIOC_76 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x09),
GPIO_INIT_ITEM("UART2_CTS_B       GPIOC_77 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x08),
GPIO_INIT_ITEM("I2C0_SDA          GPIOC_78 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x21),
GPIO_INIT_ITEM("I2C0_SCL          GPIOC_79 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x20),
GPIO_INIT_ITEM("I2C1_SDA          GPIOC_80 "     ,Native   ,NA           ,F1           ,             ,                ,NONE       ,0x1f),
GPIO_INIT_ITEM("I2C1_SCL          GPIOC_81 "     ,Native   ,NA           ,F1           ,             ,                ,NONE       ,0x1e),
GPIO_INIT_ITEM("I2C2_SDA          GPIOC_82 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x1d),
GPIO_INIT_ITEM("I2C2_SCL          GPIOC_83 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x1b),
GPIO_INIT_ITEM("I2C3_SDA          GPIOC_84 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x19),
GPIO_INIT_ITEM("I2C3_SCL          GPIOC_85 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x1c),
GPIO_INIT_ITEM("I2C4_SDA          GPIOC_86 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x1a),
GPIO_INIT_ITEM("I2C4_SCL          GPIOC_87 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x17),
GPIO_INIT_ITEM("I2C5_SDA          GPIOC_88 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x15),
GPIO_INIT_ITEM("I2C5_SCL          GPIOC_89 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x14),
GPIO_INIT_ITEM("I2C6_SDA          GPIOC_90 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x18),
GPIO_INIT_ITEM("I2C6_SCL          GPIOC_91 "     ,Native   ,NA           ,F1           ,             ,                ,20K_H      ,0x16),
GPIO_INIT_ITEM("I2C_NFC_SDA       GPIOC_92 "     ,GPIO     ,NA           ,F1           ,             ,                ,NONE       ,0x05),
GPIO_INIT_ITEM("I2C_NFC_SCL       GPIOC_93 "     ,GPO      ,LO           ,F1           ,             ,                ,NONE       ,0x03),
GPIO_INIT_ITEM("PWM0              GPIOC_94 "     ,Native   ,NA           ,F1           ,             ,                ,20K_L      ,0x0a),
GPIO_INIT_ITEM("PWM1              GPIOC_95 "     ,Native   ,NA           ,F1           ,             ,                ,20K_L      ,0x0b),
GPIO_INIT_ITEM("PLT_CLK0          GPIOC_96 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x6a),
GPIO_INIT_ITEM("PLT_CLK1          GPIOC_97 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x57),
GPIO_INIT_ITEM("PLT_CLK2          GPIOC_98 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x5b),
GPIO_INIT_ITEM("PLT_CLK3          GPIOC_99 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x68),
GPIO_INIT_ITEM("PLT_CLK4          GPIOC_100"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x61),
GPIO_INIT_ITEM("PLT_CLK5          GPIOC_101"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x64),
};

static GPIO_CONF_PAD_INIT mMinnow2_GpioInitData_SUS[] = {
//              Pad Name          GPIO Number     Used As   GPIO Default  Function#     INT Capable   Interrupt Type   PULL H/L    MMIO Offset
GPIO_INIT_ITEM("GPIO_SUS0         GPIO_SUS0"     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x1d),
GPIO_INIT_ITEM("GPIO_SUS1         GPIO_SUS1"     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x21),
GPIO_INIT_ITEM("GPIO_SUS2         GPIO_SUS2"     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x1e),
GPIO_INIT_ITEM("GPIO_SUS3         GPIO_SUS3"     ,Native   ,NA           ,F6           ,YES          ,Level_Low       ,2K_H       ,0x1f),
GPIO_INIT_ITEM("GPIO_SUS4         GPIO_SUS4"     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x20),
GPIO_INIT_ITEM("GPIO_SUS5         GPIO_SUS5"     ,GPI      ,NA           ,F0           ,             ,                ,NONE       ,0x22),
GPIO_INIT_ITEM("GPIO_SUS6         GPIO_SUS6"     ,GPI      ,NA           ,F0           ,             ,                ,NONE       ,0x24),
GPIO_INIT_ITEM("GPIO_SUS7         GPIO_SUS7"     ,GPI      ,NA           ,F0           ,             ,                ,NONE       ,0x23),
GPIO_INIT_ITEM("SEC_GPIO_SUS8     GPIO_SUS8"     ,GPO      ,HI           ,F0           ,             ,                ,20K_H      ,0x26),
GPIO_INIT_ITEM("SEC_GPIO_SUS9     GPIO_SUS9"     ,GPO      ,HI           ,F0           ,             ,                ,20K_H      ,0x25),
GPIO_INIT_ITEM("SEC_GPIO_SUS10    GPIO_SUS10"    ,GPO      ,HI           ,F0           ,             ,                ,NONE       ,0x12),
GPIO_INIT_ITEM("SUSPWRDNACK       GPIOS_11 "     ,Native   ,NA           ,F0           ,             ,                ,10K_H      ,0x07),
GPIO_INIT_ITEM("PMU_SUSCLK        GPIOS_12 "     ,Native   ,NA           ,F0           ,             ,                ,NONE       ,0x0b),
GPIO_INIT_ITEM("PMU_SLP_S0IX_B    GPIOS_13 "     ,Native   ,NA           ,F0           ,             ,                ,NONE       ,0x14),
GPIO_INIT_ITEM("PMU_SLP_LAN_B     GPIOS_14 "     ,GPO      ,LO           ,F1           ,             ,                ,10K_H      ,0x11),
GPIO_INIT_ITEM("PMU_WAKE_B        GPIOS_15 "     ,Native   ,NA           ,F0           ,             ,                ,20K_H      ,0x01),
GPIO_INIT_ITEM("PMU_PWRBTN_B      GPIOS_16 "     ,Native   ,NA           ,F0           ,             ,                ,20K_H      ,0x08),
GPIO_INIT_ITEM("PMU_WAKE_LAN_B    GPIOS_17 "     ,GPIO     ,NA           ,F1           ,             ,                ,NONE       ,0x0a),
GPIO_INIT_ITEM("SUS_STAT_B        GPIOS_18 "     ,GPO      ,NA           ,F1           ,             ,                ,NONE       ,0x13),
GPIO_INIT_ITEM("USB_OC0_B         GPIOS_19 "     ,Native   ,NA           ,F0           ,             ,                ,10K_H      ,0x0c),
GPIO_INIT_ITEM("USB_OC1_B         GPIOS_20 "     ,Native   ,NA           ,F0           ,             ,                ,10K_H      ,0x00),
GPIO_INIT_ITEM("SPI_CS1_B         GPIOS_21 "     ,Native   ,NA           ,F0           ,             ,                ,NONE       ,0x02),
GPIO_INIT_ITEM("GPIO_DFX0         GPIOS_22 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x17),
GPIO_INIT_ITEM("GPIO_DFX1         GPIOS_23 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x27),
GPIO_INIT_ITEM("GPIO_DFX2         GPIOS_24 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x1c),
GPIO_INIT_ITEM("GPIO_DFX3         GPIOS_25 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x1b),
GPIO_INIT_ITEM("GPIO_DFX4         GPIOS_26 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x16),
GPIO_INIT_ITEM("GPIO_DFX5         GPIOS_27 "     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x15),
GPIO_INIT_ITEM("GPIO_DFX6         GPIOS_28 "     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x18),
GPIO_INIT_ITEM("GPIO_DFX7         GPIOS_29 "     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x19),
GPIO_INIT_ITEM("GPIO_DFX8         GPIOS_30 "     ,GPI      ,NA           ,F0           ,             ,                ,20K_H      ,0x1a),
GPIO_INIT_ITEM("USB_ULPI_0_CLK    GPIOS_31 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x33),
GPIO_INIT_ITEM("USB_ULPI_0_DATA0  GPIOS_32 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x38),
GPIO_INIT_ITEM("USB_ULPI_0_DATA1  GPIOS_33 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x36),
GPIO_INIT_ITEM("USB_ULPI_0_DATA2  GPIOS_34 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x31),
GPIO_INIT_ITEM("USB_ULPI_0_DATA3  GPIOS_35 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x37),
GPIO_INIT_ITEM("USB_ULPI_0_DATA4  GPIOS_36 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x30),
GPIO_INIT_ITEM("USB_ULPI_0_DATA5  GPIOS_37 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x39),
GPIO_INIT_ITEM("USB_ULPI_0_DATA6  GPIOS_38 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x32),
GPIO_INIT_ITEM("USB_ULPI_0_DATA7  GPIOS_39 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x3a),
GPIO_INIT_ITEM("USB_ULPI_0_DIR    GPIOS_40 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x34),
GPIO_INIT_ITEM("USB_ULPI_0_NXT    GPIOS_41 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x35),
GPIO_INIT_ITEM("USB_ULPI_0_STP    GPIOS_42 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x3b),
GPIO_INIT_ITEM("USB_ULPI_0_REFCLK GPIOS_43 "     ,GPIO     ,NA           ,F0           ,             ,                ,NONE       ,0x28),
};

EFI_STATUS
MultiPlatformGpioTableInit (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfoHob
  );

EFI_STATUS
MultiPlatformGpioProgram (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfoHob
  );

#endif

/** @file
QuarkNcSocId Register Definitions

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

Definitions beginning with "R_" are registers
Definitions beginning with "B_" are bits within registers
Definitions beginning with "V_" are meaningful values of bits within the registers
Definitions beginning with "S_" are register sizes
Definitions beginning with "N_" are the bit position

**/

#ifndef _QUARK_NC_SOC_ID_H_
#define _QUARK_NC_SOC_ID_H_

//
// QNC GMCH Equates
//

//
// DEVICE 0 (Memroy Controller Hub)
//
#define MC_BUS                  PCI_BUS_NUMBER_QNC
#define MC_DEV                  0x00
#define MC_FUN                  0x00

#define   QUARK_MC_VENDOR_ID      V_INTEL_VENDOR_ID
#define   QUARK_MC_DEVICE_ID      0x0958
#define   QUARK2_MC_DEVICE_ID     0x12C0
#define   QNC_MC_REV_ID_A0      0x00


//
// MCR - B0:D0:F0:RD0h (WO)- Message control register
// [31:24] Message opcode - D0 read; E0 write;
// [23:16] Message port
// [15:8 ] Message target register address
// [ 7:4 ] Message write byte enable : F is enable
// [ 3:0 ] Reserved
//
#define QNC_ACCESS_PORT_MCR              0xD0          // Message Control Register
// Always Set to 0xF0

//
//MDR - B0:D0:F0:RD4h (RW)- Message data register
//
#define QNC_ACCESS_PORT_MDR              0xD4          // Message Data Register

//
//MEA - B0:D0:F0:RD8h (RW)- Message extended address register
//
#define QNC_ACCESS_PORT_MEA              0xD8          // Message Extended Address Register

#define  QNC_MCR_OP_OFFSET           24           // Offset of the opcode field in MCR
#define  QNC_MCR_PORT_OFFSET         16           // Offset of the port field in MCR
#define  QNC_MCR_REG_OFFSET          8            // Offset of the register field in MCR

//
// Misc Useful Macros
//

#define LShift16(value) (value << 16)

//
// QNC Message OpCodes and Attributes
//
#define QUARK_OPCODE_READ              0x10         // Quark message bus "read" opcode
#define QUARK_OPCODE_WRITE             0x11         // Quark message bus "write" opcode

//
// Alternative opcodes for the SCSS block
//
#define QUARK_ALT_OPCODE_READ          0x06         // Quark message bus "read" opcode
#define QUARK_ALT_OPCODE_WRITE         0x07         // Quark message bus "write" opcode

//
// QNC Message OpCodes and Attributes for IO
//
#define QUARK_OPCODE_IO_READ           0x02         // Quark message bus "IO read" opcode
#define QUARK_OPCODE_IO_WRITE          0x03         // Quark message bus "IO write" opcode


#define QUARK_DRAM_BASE_ADDR_READY     0x78         // Quark message bus "RMU Main binary shadow" opcode

#define QUARK_ECC_SCRUB_RESUME         0xC2         // Quark Remote Management Unit "scrub resume" opcode
#define QUARK_ECC_SCRUB_PAUSE          0xC3         // Quark Remote Management Unit "scrub pause" opcode

//
// QNC Message Ports and Registers
//
// Start of SB Port IDs
#define QUARK_NC_MEMORY_ARBITER_SB_PORT_ID    0x00
#define QUARK_NC_MEMORY_CONTROLLER_SB_PORT_ID 0x01
#define QUARK_NC_HOST_BRIDGE_SB_PORT_ID       0x03
#define QUARK_NC_RMU_SB_PORT_ID               0x04
#define QUARK_NC_MEMORY_MANAGER_SB_PORT_ID    0x05
#define QUARK_SC_USB_AFE_SB_PORT_ID           0x14
#define QUARK_SC_PCIE_AFE_SB_PORT_ID          0x16
#define QUARK_SCSS_SOC_UNIT_SB_PORT_ID        0x31
#define QUARK_SCSS_FUSE_SB_PORT_ID            0x33
#define QUARK_ICLK_SB_PORT_ID                 0x32
#define QUARK_SCSS_CRU_SB_PORT_ID             0x34

//
// Quark Memory Arbiter Registers.
//
#define   QUARK_NC_MEMORY_ARBITER_REG_ASTATUS     0x21        // Memory Arbiter PRI Status encodings register.
#define   ASTATUS_PRI_CASUAL                    0x0         // Serviced only if convenient
#define   ASTATUS_PRI_IMPENDING                 0x1         // Serviced if the DRAM is in Self-Refresh.
#define   ASTATUS_PRI_NORMAL                    0x2         // Normal request servicing.
#define   ASTATUS_PRI_URGENT                    0x3         // Urgent request servicing.
#define   ASTATUS1_RASISED_BP                   (10)
#define   ASTATUS1_RASISED_BP_MASK              (0x03 << ASTATUS1_RASISED_BP)
#define   ASTATUS0_RASISED_BP                   (8)
#define   ASTATUS0_RASISED_BP_MASK              (0x03 << ASTATUS1_RASISED_BP)
#define   ASTATUS1_DEFAULT_BP                   (2)
#define   ASTATUS1_DEFAULT_BP_MASK              (0x03 << ASTATUS1_RASISED_BP)
#define   ASTATUS0_DEFAULT_BP                   (0)
#define   ASTATUS0_DEFAULT_BP_MASK              (0x03 << ASTATUS1_RASISED_BP)

//
// Quark Memory Controller Registers.
//
#define QUARK_NC_MEMORY_CONTROLLER_REG_DFUSESTAT  0x70        // Fuse status register.
#define   B_DFUSESTAT_ECC_DIS                     (BIT0)    // Disable ECC.

//
// Quark Remote Management Unit Registers.
//
#define QNC_MSG_TMPM_REG_PMBA                   0x70        // Power Management I/O Base Address

#define QUARK_NC_RMU_REG_CONFIG                   0x71        // Remote Management Unit configuration register.
#define   TS_LOCK_AUX_TRIP_PT_REGS_ENABLE         (BIT6)
#define   TS_LOCK_THRM_CTRL_REGS_ENABLE           (BIT5)

#define QUARK_NC_RMU_REG_OPTIONS_1              0x72        // Remote Management Unit Options register 1.
#define   OPTIONS_1_DMA_DISABLE                   (BIT0)

#define QUARK_NC_RMU_REG_WDT_CONTROL              0x74        // Remote Management Unit Watchdog control register.
#define   B_WDT_CONTROL_DBL_ECC_BIT_ERR_MASK      (BIT19 | BIT18)
#define   B_WDT_CONTROL_DBL_ECC_BIT_ERR_BP        18
#define   V_WDT_CONTROL_DBL_ECC_BIT_ERR_NONE      (0x0 << B_WDT_CONTROL_DBL_ECC_BIT_ERR_BP)
#define   V_WDT_CONTROL_DBL_ECC_BIT_ERR_CAT       (0x1 << B_WDT_CONTROL_DBL_ECC_BIT_ERR_BP)
#define   V_WDT_CONTROL_DBL_ECC_BIT_ERR_WARM      (0x2 << B_WDT_CONTROL_DBL_ECC_BIT_ERR_BP)
#define   V_WDT_CONTROL_DBL_ECC_BIT_ERR_SERR      (0x3 << B_WDT_CONTROL_DBL_ECC_BIT_ERR_BP)

#define QUARK_NC_RMU_REG_TS_MODE                  0xB0        // Remote Management Unit Thermal sensor mode register.
#define   TS_ENABLE                               (BIT15)
#define QUARK_NC_RMU_REG_TS_TRIP                  0xB2        // Remote Management Unit Thermal sensor programmable trip point register.
#define   TS_HOT_TRIP_CLEAR_THOLD_BP              24
#define   TS_HOT_TRIP_CLEAR_THOLD_MASK            (0xFF << TS_HOT_TRIP_CLEAR_THOLD_BP)
#define   TS_CAT_TRIP_CLEAR_THOLD_BP              16
#define   TS_CAT_TRIP_CLEAR_THOLD_MASK            (0xFF << TS_CAT_TRIP_CLEAR_THOLD_BP)
#define   TS_HOT_TRIP_SET_THOLD_BP                8
#define   TS_HOT_TRIP_SET_THOLD_MASK              (0xFF << TS_HOT_TRIP_SET_THOLD_BP)
#define   TS_CAT_TRIP_SET_THOLD_BP                0
#define   TS_CAT_TRIP_SET_THOLD_MASK              (0xFF << TS_CAT_TRIP_SET_THOLD_BP)

#define QUARK_NC_ECC_SCRUB_CONFIG_REG             0x50
#define   SCRUB_CFG_INTERVAL_SHIFT              0x00
#define   SCRUB_CFG_INTERVAL_MASK               0xFF
#define   SCRUB_CFG_BLOCKSIZE_SHIFT             0x08
#define   SCRUB_CFG_BLOCKSIZE_MASK              0x1F
#define   SCRUB_CFG_ACTIVE                      (BIT13)
#define   SCRUB_CFG_INVALID                     0x00000FFF

#define QUARK_NC_ECC_SCRUB_START_MEM_REG          0x76
#define QUARK_NC_ECC_SCRUB_END_MEM_REG            0x77
#define QUARK_NC_ECC_SCRUB_NEXT_READ_REG          0x7C

#define SCRUB_RESUME_MSG() ((UINT32)( \
          (QUARK_ECC_SCRUB_RESUME << QNC_MCR_OP_OFFSET) | \
          (QUARK_NC_RMU_SB_PORT_ID << QNC_MCR_PORT_OFFSET) | \
          0xF0))

#define SCRUB_PAUSE_MSG() ((UINT32)( \
          (QUARK_ECC_SCRUB_PAUSE << QNC_MCR_OP_OFFSET) | \
          (QUARK_NC_RMU_SB_PORT_ID << QNC_MCR_PORT_OFFSET) | \
          0xF0))

//
// Quark Memory Manager Registers
//
#define QUARK_NC_MEMORY_MANAGER_ESRAMPGCTRL_BLOCK     0x82
#define   BLOCK_ENABLE_PG                           (1 << 28)
#define   BLOCK_DISABLE_PG                          (1 << 29)
#define QUARK_NC_MEMORY_MANAGER_BIMRVCTL              0x19
#define   EnableIMRInt                                BIT31
#define QUARK_NC_MEMORY_MANAGER_BSMMVCTL              0x1C
#define   EnableSMMInt                                BIT31
#define QUARK_NC_MEMORY_MANAGER_BTHCTRL               0x20
#define   DRAM_NON_HOST_RQ_LIMIT_BP                   0
#define   DRAM_NON_HOST_RQ_LIMIT_MASK                 (0x3f << DRAM_NON_HOST_RQ_LIMIT_BP)

#define QUARK_NC_TOTAL_IMR_SET                        0x8
#define QUARK_NC_MEMORY_MANAGER_IMR0                  0x40
#define QUARK_NC_MEMORY_MANAGER_IMR1                  0x44
#define QUARK_NC_MEMORY_MANAGER_IMR2                  0x48
#define QUARK_NC_MEMORY_MANAGER_IMR3                  0x4C
#define QUARK_NC_MEMORY_MANAGER_IMR4                  0x50
#define QUARK_NC_MEMORY_MANAGER_IMR5                  0x54
#define QUARK_NC_MEMORY_MANAGER_IMR6                  0x58
#define QUARK_NC_MEMORY_MANAGER_IMR7                  0x5C
  #define QUARK_NC_MEMORY_MANAGER_IMRXL               0x00
    #define IMR_LOCK                                BIT31
    #define IMR_EN                                  BIT30
    #define IMRL_MASK                               0x00FFFFFC
    #define IMRL_RESET                              0x00000000
  #define QUARK_NC_MEMORY_MANAGER_IMRXH               0x01
    #define IMRH_MASK                               0x00FFFFFC
    #define IMRH_RESET                              0x00000000
  #define QUARK_NC_MEMORY_MANAGER_IMRXRM              0x02
  #define QUARK_NC_MEMORY_MANAGER_IMRXWM              0x03
    #define IMRX_ALL_ACCESS                         0xFFFFFFFF
    #define CPU_SNOOP                               BIT30
    #define RMU                                     BIT29
    #define CPU0_NON_SMM                            BIT0

//
// Quark Host Bridge Registers
//
#define QNC_MSG_FSBIC_REG_HMISC                0x03       // Host Misellaneous Controls
#define   SMI_EN                              (BIT19)     // SMI Global Enable (from Legacy Bridge)
#define QNC_MSG_FSBIC_REG_HSMMC                0x04       // Host SMM Control
#define   NON_HOST_SMM_WR_OPEN                (BIT18)     // SMM Writes OPEN
#define   NON_HOST_SMM_RD_OPEN                (BIT17)     // SMM Writes OPEN
#define   SMM_CODE_RD_OPEN                    (BIT16)     // SMM Code read OPEN
#define   SMM_CTL_EN                          (BIT3)      // SMM enable
#define   SMM_WRITE_OPEN                      (BIT2)      // SMM Writes OPEN
#define   SMM_READ_OPEN                       (BIT1)      // SMM Reads OPEN
#define   SMM_LOCKED                          (BIT0)      // SMM Locked
#define   SMM_START_MASK                      0x0000FFF0
#define   SMM_END_MASK                        0xFFF00000
#define QUARK_NC_HOST_BRIDGE_HMBOUND_REG              0x08
#define   HMBOUND_MASK                        0x0FFFFF000
#define   HMBOUND_LOCK                        BIT0
#define QUARK_NC_HOST_BRIDGE_HLEGACY_REG              0x0A
#define   HLEGACY_SMI_PIN_VALUE               BIT12
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_CAP            0x40
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_DEF_TYPE       0x41
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX64K_00000        0x42
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX16K_80000        0x44
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX16K_A0000        0x46
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_C0000         0x48
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_C8000         0x4A
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_D0000         0x4C
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_D8000         0x4E
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_E0000         0x50
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_E8000         0x52
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_F0000         0x54
#define QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_F8000         0x56
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_SMRR_PHYSBASE  0x58
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_SMRR_PHYSMASK  0x59
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE0      0x5A
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK0      0x5B
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE1      0x5C
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK1      0x5D
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE2      0x5E
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK2      0x5F
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE3      0x60
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK3      0x61
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE4      0x62
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK4      0x63
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE5      0x64
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK5      0x65
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE6      0x66
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK6      0x67
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE7      0x68
#define QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK7      0x69

//
// System On Chip Unit (SOCUnit) Registers.
//
#define QUARK_SCSS_SOC_UNIT_STPDDRCFG           0x00
#define   B_STPDDRCFG_FORCE_RECOVERY              BIT0
#define QUARK_SCSS_SOC_UNIT_SPI_ROM_FUSE    0x25
#define   B_ROM_FUSE_IN_SECURE_SKU              BIT6

#define QUARK_SCSS_SOC_UNIT_TSCGF1_CONFIG       0x31
#define   B_TSCGF1_CONFIG_ISNSCURRENTSEL_MASK   (BIT5 | BIT4 | BIT3)
#define   B_TSCGF1_CONFIG_ISNSCURRENTSEL_BP     3
#define   B_TSCGF1_CONFIG_ISNSCHOPSEL_MASK      (BIT12 | BIT11 | BIT10 | BIT9 | BIT8)
#define   B_TSCGF1_CONFIG_ISNSCHOPSEL_BP        8
#define   B_TSCGF1_CONFIG_IBGEN                 BIT17
#define   B_TSCGF1_CONFIG_IBGEN_BP              17
#define   B_TSCGF1_CONFIG_IBGCHOPEN             BIT18
#define   B_TSCGF1_CONFIG_IBGCHOPEN_BP          18
#define   B_TSCGF1_CONFIG_ISNSINTERNALVREFEN    BIT14
#define   B_TSCGF1_CONFIG_ISNSINTERNALVREFEN_BP 14

#define QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG       0x32
#define   B_TSCGF2_CONFIG_IDSCONTROL_MASK       0x0000FFFF
#define   B_TSCGF2_CONFIG_IDSCONTROL_BP         0
#define   B_TSCGF2_CONFIG_IDSTIMING_MASK        0xFFFF0000
#define   B_TSCGF2_CONFIG_IDSTIMING_BP          16

#define QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG2      0x33
#define   B_TSCGF2_CONFIG2_ISPARECTRL_MASK      0xFF000000
#define   B_TSCGF2_CONFIG2_ISPARECTRL_BP        24
#define   B_TSCGF2_CONFIG2_ICALCONFIGSEL_MASK   (BIT9 | BIT8)
#define   B_TSCGF2_CONFIG2_ICALCONFIGSEL_BP     8
#define   B_TSCGF2_CONFIG2_ICALCOARSETUNE_MASK  0x000000FF
#define   B_TSCGF2_CONFIG2_ICALCOARSETUNE_BP    0

#define QUARK_SCSS_SOC_UNIT_TSCGF3_CONFIG       0x34
#define   B_TSCGF3_CONFIG_ITSRST                BIT0
#define   B_TSCGF3_CONFIG_ITSGAMMACOEFF_BP      11
#define   B_TSCGF3_CONFIG_ITSGAMMACOEFF_MASK    (0xFFF << B_TSCGF3_CONFIG_ITSGAMMACOEFF_BP)

#define QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG     0x36
#define   SOCCLKEN_CONFIG_PHY_I_SIDE_RST_L      BIT20
#define   SOCCLKEN_CONFIG_PHY_I_CMNRESET_L      BIT19
#define   SOCCLKEN_CONFIG_SBI_BB_RST_B          BIT18
#define   SOCCLKEN_CONFIG_SBI_RST_100_CORE_B    BIT17
#define   SOCCLKEN_CONFIG_BB_RST_B              BIT16

#define QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG     0x36

#define QUARK_SCSS_SOC_UNIT_CFG_STICKY_RW       0x51
#define   B_CFG_STICKY_RW_SMM_VIOLATION         BIT0
#define   B_CFG_STICKY_RW_HMB_VIOLATION         BIT1
#define   B_CFG_STICKY_RW_IMR_VIOLATION         BIT2
#define   B_CFG_STICKY_RW_DECC_VIOLATION        BIT3
#define   B_CFG_STICKY_RW_WARM_RST              BIT4
#define   B_CFG_STICKY_RW_FORCE_RECOVERY        BIT9
#define   B_CFG_STICKY_RW_VIOLATION             (B_CFG_STICKY_RW_SMM_VIOLATION | B_CFG_STICKY_RW_HMB_VIOLATION | B_CFG_STICKY_RW_IMR_VIOLATION | B_CFG_STICKY_RW_DECC_VIOLATION)
#define   B_CFG_STICKY_RW_ALL                   (B_CFG_STICKY_RW_VIOLATION | B_CFG_STICKY_RW_WARM_RST)

//
// iCLK Registers.
//
#define QUARK_ICLK_MUXTOP                       0x0140
#define   B_MUXTOP_FLEX2_MASK                   (BIT25 | BIT24 | BIT23)
#define   B_MUXTOP_FLEX2_BP                     23
#define   B_MUXTOP_FLEX1_MASK                   (BIT22 | BIT21 | BIT20)
#define   B_MUXTOP_FLEX1_BP                     20

#define QUARK_ICLK_SSC1                         0x0314
#define QUARK_ICLK_SSC2                         0x0414
#define QUARK_ICLK_SSC3                         0x0514
#define QUARK_ICLK_REF2_DBUFF0                  0x2000

//
// PCIe AFE Unit Registers (QUARK_SC_PCIE_AFE_SB_PORT_ID).
//
#define QUARK_PCIE_AFE_PCIE_RXPICTRL0_L0        0x2080
#define QUARK_PCIE_AFE_PCIE_RXPICTRL0_L1        0x2180
#define   OCFGPIMIXLOAD_1_0                   BIT6
#define   OCFGPIMIXLOAD_1_0_MASK              0xFFFFFF3F

//
// QNC ICH Equates
//
#define V_INTEL_VENDOR_ID              0x8086

#define PCI_BUS_NUMBER_QNC              0x00

//
// PCI to LPC Bridge Registers (D31:F0)
//
#define PCI_DEVICE_NUMBER_QNC_LPC       31
#define PCI_FUNCTION_NUMBER_QNC_LPC     0

#define R_QNC_LPC_VENDOR_ID             0x00
#define   V_LPC_VENDOR_ID             V_INTEL_VENDOR_ID
#define R_QNC_LPC_DEVICE_ID             0x02
#define   QUARK_V_LPC_DEVICE_ID_0           0x095E
#define R_QNC_LPC_REV_ID                0x08

#define R_QNC_LPC_SMBUS_BASE            0x40 //~0x43
#define   B_QNC_LPC_SMBUS_BASE_EN         (BIT31)
#define   B_QNC_LPC_SMBUS_BASE_MASK       0x0000FFC0 //[15:6]
//
// SMBus register offsets from SMBA - "SMBA" (D31:F0:R40h)
//        Suggested Value for SMBA = 0x1040
//
#define R_QNC_SMBUS_HCTL                0x00   // Host Control Register R/W
#define   B_QNC_SMBUS_START               (BIT4)   // Start/Stop
#define     V_QNC_SMBUS_HCTL_CMD_QUICK               0
#define     V_QNC_SMBUS_HCTL_CMD_BYTE                1
#define     V_QNC_SMBUS_HCTL_CMD_BYTE_DATA           2
#define     V_QNC_SMBUS_HCTL_CMD_WORD_DATA           3
#define     V_QNC_SMBUS_HCTL_CMD_PROCESS_CALL        4
#define     V_QNC_SMBUS_HCTL_CMD_BLOCK               5

#define R_QNC_SMBUS_HSTS                0x01   // Host Status Register R/W
#define   B_QNC_SMBUS_BERR                (BIT2)   // BUS Error
#define   B_QNC_SMBUS_DERR                (BIT1)   // Device Error
#define   B_QNC_SMBUS_BYTE_DONE_STS       (BIT0)   // Completion Status
#define   B_QNC_SMBUS_HSTS_ALL            0x07

#define R_QNC_SMBUS_HCLK                0x02   // Host Clock Divider Register R/W
#define     V_QNC_SMBUS_HCLK_100KHZ         0x0054

#define R_QNC_SMBUS_TSA                 0x04   // Transmit Slave Address Register R/W
#define     V_QNC_SMBUS_RW_SEL_READ         1
#define     V_QNC_SMBUS_RW_SEL_WRITE        0

#define R_QNC_SMBUS_HCMD                0x05   // Host Command Register R/W
#define R_QNC_SMBUS_HD0                 0x06   // Data 0 Register R/W
#define R_QNC_SMBUS_HD1                 0x07   // Data 1 Register R/W
#define R_QNC_SMBUS_HBD                 0x20   // Host Block Data Register R/W [255:0] ~ 3Fh

#define R_QNC_LPC_GBA_BASE              0x44
#define   B_QNC_LPC_GPA_BASE_MASK         0x0000FFC0
//
// GPIO register offsets from GBA - "GPIO" (D31:F0:R44h)
//        Suggested Value for GBA = 0x1080
//
#define R_QNC_GPIO_CGEN_CORE_WELL       0x00
#define R_QNC_GPIO_CGIO_CORE_WELL       0x04
#define R_QNC_GPIO_CGLVL_CORE_WELL      0x08
#define R_QNC_GPIO_CGTPE_CORE_WELL      0x0C   // Core well GPIO Trigger Positive Edge Enable
#define R_QNC_GPIO_CGTNE_CORE_WELL      0x10   // Core well GPIO Trigger Negative Edge Enable
#define R_QNC_GPIO_CGGPE_CORE_WELL      0x14   // Core well GPIO GPE Enable
#define R_QNC_GPIO_CGSMI_CORE_WELL      0x18   // Core well GPIO SMI Enable
#define R_QNC_GPIO_CGTS_CORE_WELL       0x1C   // Core well GPIO Trigger Status
#define R_QNC_GPIO_RGEN_RESUME_WELL     0x20
#define R_QNC_GPIO_RGIO_RESUME_WELL     0x24
#define R_QNC_GPIO_RGLVL_RESUME_WELL    0x28
#define R_QNC_GPIO_RGTPE_RESUME_WELL    0x2C   // Resume well GPIO Trigger Positive Edge Enable
#define R_QNC_GPIO_RGTNE_RESUME_WELL    0x30   // Resume well GPIO Trigger Negative Edge Enable
#define R_QNC_GPIO_RGGPE_RESUME_WELL    0x34   // Resume well GPIO GPE Enable
#define R_QNC_GPIO_RGSMI_RESUME_WELL    0x38   // Resume well GPIO SMI Enable
#define R_QNC_GPIO_RGTS_RESUME_WELL     0x3C   // Resume well GPIO Trigger Status
#define R_QNC_GPIO_CNMIEN_CORE_WELL     0x40   // Core well GPIO NMI Enable
#define R_QNC_GPIO_RNMIEN_RESUME_WELL   0x44   // Resume well GPIO NMI Enable

#define R_QNC_LPC_PM1BLK                0x48
#define   B_QNC_LPC_PM1BLK_MASK           0x0000FFF0
//
// ACPI register offsets from PM1BLK - "ACPI PM1 Block" (D31:F0:R48h)
//        Suggested Value for PM1BLK = 0x1000
//
#define R_QNC_PM1BLK_PM1S               0x00
#define  S_QNC_PM1BLK_PM1S               2
#define   B_QNC_PM1BLK_PM1S_ALL           (BIT15+BIT14+BIT10+BIT5+BIT0)
#define   B_QNC_PM1BLK_PM1S_WAKE          (BIT15)
#define   B_QNC_PM1BLK_PM1S_PCIEWSTS      (BIT14)
#define   B_QNC_PM1BLK_PM1S_RTC           (BIT10)
#define   B_QNC_PM1BLK_PM1S_GLOB          (BIT5)
#define   B_QNC_PM1BLK_PM1S_TO            (BIT0)
#define    N_QNC_PM1BLK_PM1S_RTC           10


#define R_QNC_PM1BLK_PM1E               0x02
#define  S_QNC_PM1BLK_PM1E               2
#define   B_QNC_PM1BLK_PM1E_PWAKED        (BIT14)
#define   B_QNC_PM1BLK_PM1E_RTC           (BIT10)
#define   B_QNC_PM1BLK_PM1E_GLOB          (BIT5)
#define    N_QNC_PM1BLK_PM1E_RTC           10

#define R_QNC_PM1BLK_PM1C               0x04
#define   B_QNC_PM1BLK_PM1C_SLPEN         (BIT13)
#define   B_QNC_PM1BLK_PM1C_SLPTP         (BIT12+BIT11+BIT10)
#define    V_S0                           0x00000000
#define    V_S3                           0x00001400
#define    V_S4                           0x00001800
#define    V_S5                           0x00001C00
#define   B_QNC_PM1BLK_PM1C_SCIEN         (BIT0)

#define R_QNC_PM1BLK_PM1T               0x08

#define R_QNC_LPC_GPE0BLK               0x4C
#define   B_QNC_LPC_GPE0BLK_MASK          0x0000FFC0
//        Suggested Value for GPE0BLK = 0x10C0
//
#define R_QNC_GPE0BLK_GPE0S             0x00          // General Purpose Event 0 Status
#define  S_QNC_GPE0BLK_GPE0S             4
#define   B_QNC_GPE0BLK_GPE0S_ALL         0x00003F800 // used to clear the status reg
#define   B_QNC_GPE0BLK_GPE0S_PCIE        (BIT17)     // PCIE
#define   B_QNC_GPE0BLK_GPE0S_GPIO        (BIT14)     // GPIO
#define   B_QNC_GPE0BLK_GPE0S_EGPE        (BIT13)     // External GPE
#define    N_QNC_GPE0BLK_GPE0S_THRM        12

#define R_QNC_GPE0BLK_GPE0E             0x04          // General Purpose Event 0 Enable
#define  S_QNC_GPE0BLK_GPE0E             4
#define   B_QNC_GPE0BLK_GPE0E_PCIE        (BIT17)     // PCIE
#define   B_QNC_GPE0BLK_GPE0E_GPIO        (BIT14)     // GPIO
#define   B_QNC_GPE0BLK_GPE0E_EGPE        (BIT13)     // External GPE
#define    N_QNC_GPE0BLK_GPE0E_THRM        12

#define R_QNC_GPE0BLK_SMIE              0x10          // SMI_B Enable
#define  S_QNC_GPE0BLK_SMIE              4
#define   B_QNC_GPE0BLK_SMIE_ALL          0x0003871F
#define   B_QNC_GPE0BLK_SMIE_APM          (BIT4)      // APM
#define   B_QNC_GPE0BLK_SMIE_SLP          (BIT2)      // Sleep
#define   B_QNC_GPE0BLK_SMIE_SWT          (BIT1)      // Software Timer
#define    N_QNC_GPE0BLK_SMIE_GPIO         9
#define    N_QNC_GPE0BLK_SMIE_ESMI         8
#define    N_QNC_GPE0BLK_SMIE_APM          4
#define    N_QNC_GPE0BLK_SMIE_SPI          3
#define    N_QNC_GPE0BLK_SMIE_SLP          2
#define    N_QNC_GPE0BLK_SMIE_SWT          1

#define R_QNC_GPE0BLK_SMIS              0x14           // SMI Status Register.
#define  S_QNC_GPE0BLK_SMIS              4
#define   B_QNC_GPE0BLK_SMIS_ALL          0x0003871F
#define   B_QNC_GPE0BLK_SMIS_EOS          (BIT31)      // End of SMI
#define   B_QNC_GPE0BLK_SMIS_APM          (BIT4)       // APM
#define   B_QNC_GPE0BLK_SMIS_SPI          (BIT3)       // SPI
#define   B_QNC_GPE0BLK_SMIS_SLP          (BIT2)       // Sleep
#define   B_QNC_GPE0BLK_SMIS_SWT          (BIT1)       // Software Timer
#define   B_QNC_GPE0BLK_SMIS_BIOS         (BIT0)       // BIOS
#define    N_QNC_GPE0BLK_SMIS_GPIO         9
#define    N_QNC_GPE0BLK_SMIS_APM          4
#define    N_QNC_GPE0BLK_SMIS_SPI          3
#define    N_QNC_GPE0BLK_SMIS_SLP          2
#define    N_QNC_GPE0BLK_SMIS_SWT          1

#define R_QNC_GPE0BLK_PMCW              0x28            // Power Management Configuration Core Well
#define   B_QNC_GPE0BLK_PMCW_PSE          (BIT31)       // Periodic SMI Enable

#define R_QNC_GPE0BLK_PMSW              0x2C            // Power Management Configuration Suspend/Resume Well
#define    B_QNC_GPE0BLK_PMSW_DRAM_INIT   (BIT0)        // Dram Initialization Sctrachpad

#define R_QNC_LPC_ACTL                  0x58
#define    V_QNC_LPC_ACTL_SCIS_IRQ9        0x00

//
// Number of PIRQs supported. PIRQA~PIRQH
//
#define QNC_NUMBER_PIRQS                8
#define R_QNC_LPC_PIRQA_ROUT            0x60
#define R_QNC_LPC_PIRQB_ROUT            0x61
#define R_QNC_LPC_PIRQC_ROUT            0x62
#define R_QNC_LPC_PIRQD_ROUT            0x63
#define R_QNC_LPC_PIRQE_ROUT            0x64
#define R_QNC_LPC_PIRQF_ROUT            0x65
#define R_QNC_LPC_PIRQG_ROUT            0x66
#define R_QNC_LPC_PIRQH_ROUT            0x67

//
// Bit values are the same for R_TNC_LPC_PIRQA_ROUT to
//                             R_TNC_LPC_PIRQH_ROUT
#define   B_QNC_LPC_PIRQX_ROUT            (BIT3+BIT2+BIT1+BIT0)

#define R_QNC_LPC_WDTBA                 0x84
// Watchdog Timer register offsets from WDTBASE (in R_QNC_LPC_WDTBA)------------BEGIN
#define R_QNC_LPC_WDT_WDTCR             0x10
#define R_QNC_LPC_WDT_WDTLR             0x18
// Watchdog Timer register offsets from WDTBASE (in R_QNC_LPC_WDTBA)--------------END

#define R_QNC_LPC_FWH_BIOS_DEC          0xD4
#define   B_QNC_LPC_FWH_BIOS_DEC_F8       (BIT31)
#define   B_QNC_LPC_FWH_BIOS_DEC_F0       (BIT30)
#define   B_QNC_LPC_FWH_BIOS_DEC_E8       (BIT29)
#define   B_QNC_LPC_FWH_BIOS_DEC_E0       (BIT28)
#define   B_QNC_LPC_FWH_BIOS_DEC_D8       (BIT27)
#define   B_QNC_LPC_FWH_BIOS_DEC_D0       (BIT26)
#define   B_QNC_LPC_FWH_BIOS_DEC_C8       (BIT25)
#define   B_QNC_LPC_FWH_BIOS_DEC_C0       (BIT24)

#define R_QNC_LPC_BIOS_CNTL             0xD8
#define  S_QNC_LPC_BIOS_CNTL             4
#define   B_QNC_LPC_BIOS_CNTL_PFE         (BIT8)
#define   B_QNC_LPC_BIOS_CNTL_SMM_BWP     (BIT5)
#define   B_QNC_LPC_BIOS_CNTL_BCD         (BIT2)
#define   B_QNC_LPC_BIOS_CNTL_BLE         (BIT1)
#define   B_QNC_LPC_BIOS_CNTL_BIOSWE      (BIT0)
#define    N_QNC_LPC_BIOS_CNTL_BLE         1
#define    N_QNC_LPC_BIOS_CNTL_BIOSWE      0

#define R_QNC_LPC_RCBA                  0xF0
#define   B_QNC_LPC_RCBA_MASK             0xFFFFC000
#define   B_QNC_LPC_RCBA_EN               (BIT0)

//---------------------------------------------------------------------------
//  Fixed IO Decode on QuarkNcSocId
//
//  20h(2B) 24h(2B) 28h(2B) 2Ch(2B) 30h(2B) 34h(2B) 38h(2B) 3Ch(2B) : R/W 8259 master
//  40h(3B): R/W 8254
//  43h(1B): W   8254
//  50h(3B): R/W 8254
//  53h(1B): W   8254
//  61h(1B): R/W NMI Controller
//  63h(1B): R/W NMI Controller - can be disabled
//  65h(1B): R/W NMI Controller - can be disabled
//  67h(1B): R/W NMI Controller - can be disabled
//  70h(1B): W   NMI & RTC
//  71h(1B): R/W RTC
//  72h(1B): R RTC; W NMI&RTC
//  73h(1B): R/W RTC
//  74h(1B): R RTC; W NMI&RTC
//  75h(1B): R/W RTC
//  76h(1B): R RTC; W NMI&RTC
//  77h(1B): R/W RTC
//  84h(3B): R/W Internal/LPC
//  88h(1B): R/W Internal/LPC
//  8Ch(3B): R/W Internal/LPC
//  A0h(2B) A4h(2B) A8h(2B) ACh(2B) B0h(2B) B4h(2B) B8h(2B) BCh(2B): R/W 8259 slave
//  B2h(1B) B3h(1B): R/W Power management
//  3B0h-3BBh: R/W VGA
//  3C0h-3DFh: R/W VGA
//  CF8h(4B): R/W Internal
//  CF9h(1B): R/W LPC
//  CFCh(4B): R/W Internal
//---------------------------------------------------------------------------

#define R_APM_CNT                                       0xB2

//
// Reset Generator I/O Port
//
#define RST_CNT                                       0xCF9
#define   B_RST_CNT_COLD_RST                            (BIT3)     // Cold reset
#define   B_RST_CNT_WARM_RST                            (BIT1)     // Warm reset

//
// Processor interface registers (NMI)
//

#define  PCI_DEVICE_NUMBER_QNC_IOSF2AHB_0                20
#define  PCI_DEVICE_NUMBER_QNC_IOSF2AHB_1                21
#define  PCI_FUNCTION_NUMBER_QNC_IOSF2AHB                 0

//
// Pci Express Root Ports (D23:F0/F1)
//
#define PCI_DEVICE_NUMBER_PCIE_ROOTPORT                 23
#define PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_0             0
#define PCI_FUNCTION_NUMBER_PCIE_ROOTPORT_1             1

#define MAX_PCI_EXPRESS_ROOT_PORTS                      2

#define R_QNC_PCIE_BNUM                             0x18
#define R_QNC_PCIE_CAP_PTR                          0x34

#define PCIE_CAPID                                  0x10  //PCIE Capability ID
#define PCIE_CAP_EXT_HEARDER_OFFSET                 0x100 //PCIE Capability ID
#define PCIE_DEV_CAP_OFFSET                         0x04 //PCIE Device Capability reg offset
#define PCIE_LINK_CAP_OFFSET                        0x0C //PCIE Link Capability reg offset
#define PCIE_LINK_CNT_OFFSET                        0x10 //PCIE Link control reg offset
#define PCIE_LINK_STS_OFFSET                        0x12 //PCIE Link status reg offset
#define PCIE_SLOT_CAP_OFFSET                        0x14 //PCIE Link Capability reg offset

#define R_QNC_PCIE_XCAP                             0x42  //~ 43h
#define   B_QNC_PCIE_XCAP_SI                          (BIT8)  //slot implemented
#define R_QNC_PCIE_DCAP                             0x44  //~ 47h
#define   B_QNC_PCIE_DCAP_E1AL                        (BIT11 | BIT10 | BIT9) // L1 Acceptable exit latency
#define   B_QNC_PCIE_DCAP_E0AL                        (BIT8 | BIT7 | BIT6)   // L0 Acceptable exit latency
#define R_QNC_PCIE_DCTL                             0x48  //~ 49h
#define   B_QNC_PCIE_DCTL_URE                         (BIT3)  //Unsupported Request Reporting Enable
#define   B_QNC_PCIE_DCTL_FEE                         (BIT2)  //Fatal error Reporting Enable
#define   B_QNC_PCIE_DCTL_NFE                         (BIT1)  //Non Fatal error Reporting Enable
#define   B_QNC_PCIE_DCTL_CEE                         (BIT0)  //Correctable error Reporting Enable
#define R_QNC_PCIE_LCAP                             0x4C  //~ 4Fh
#define   B_QNC_PCIE_LCAP_CPM                         (BIT18)  //clock power management supported
#define   B_QNC_PCIE_LCAP_EL1_MASK                   (BIT17 | BIT16 | BIT15)  //L1 Exit latency mask
#define   B_QNC_PCIE_LCAP_EL0_MASK                   (BIT14 | BIT13 | BIT12)  //L0 Exit latency mask
#define   B_QNC_PCIE_LCAP_APMS_MASK                   (BIT11 | BIT10)  //Active state link PM support mask
#define   V_QNC_PCIE_LCAP_APMS_OFFSET                 10  //Active state link PM support mask
#define R_QNC_PCIE_LCTL                             0x50  //~ 51h
#define   B_QNC_PCIE_LCTL_CCC                         (BIT6)  // Clock clock configuration
#define   B_QNC_PCIE_LCTL_RL                          (BIT5)  // Retrain link
#define R_QNC_PCIE_LSTS                             0x52  //~ 53h
#define   B_QNC_PCIE_LSTS_SCC                         (BIT12) //Slot clock configuration
#define   B_QNC_PCIE_LSTS_LT                          (BIT11) //Link training
#define R_QNC_PCIE_SLCAP                            0x54  //~ 57h
#define   B_QNC_PCIE_SLCAP_MASK_RSV_VALUE             0x0006007F
#define   V_QNC_PCIE_SLCAP_SLV                        0x0A  //Slot power limit value [14:7]
#define   V_QNC_PCIE_SLCAP_SLV_OFFSET                 7     //Slot power limit value offset is 7 [14:7]
#define   V_QNC_PCIE_SLCAP_PSN_OFFSET                 19    //Slot number offset is 19 [31:19]
#define R_QNC_PCIE_SLCTL                            0x58    //~ 59h
#define   B_QNC_PCIE_SLCTL_HPE                        (BIT5)  // Hot plug interrupt enable
#define   B_QNC_PCIE_SLCTL_PDE                        (BIT3)  // Presense detect change enable
#define   B_QNC_PCIE_SLCTL_ABE                        (BIT0)  // Attention Button Pressed Enable
#define R_QNC_PCIE_SLSTS                            0x5A    //~ 5Bh
#define   B_QNC_PCIE_SLSTS_PDS                        (BIT6)  // Present Detect State = 1b : has device connected
#define   B_QNC_PCIE_SLSTS_PDC                        (BIT3)  // Present Detect changed = 1b : PDS state has changed
#define   B_QNC_PCIE_SLSTS_ABP                        (BIT0)  // Attention Button Pressed
#define R_QNC_PCIE_RCTL                             0x5C    //~ 5Dh
#define   B_QNC_PCIE_RCTL_PIE                         (BIT3)  //Root PCI-E PME Interrupt Enable
#define   B_QNC_PCIE_RCTL_SFE                         (BIT2)  //Root PCI-E System Error on Fatal Error Enable
#define   B_QNC_PCIE_RCTL_SNE                         (BIT1)  //Root PCI-E System Error on Non-Fatal Error Enable
#define   B_QNC_PCIE_RCTL_SCE                         (BIT0)  //Root PCI-E System Error on Correctable Error Enable
#define R_QNC_PCIE_SVID                             0x94  //~ 97h
#define R_QNC_PCIE_CCFG                             0xD0  //~ D3h
#define   B_QNC_PCIE_CCFG_UPSD                        (BIT24)  // Upstream Posted Split Disable
#define   B_QNC_PCIE_CCFG_UNRS                        (BIT15)  // Upstream Non-Posted Request Size
#define   B_QNC_PCIE_CCFG_UPRS                        (BIT14)  // Upstream Posted Request Size
#define R_QNC_PCIE_MPC2                             0xD4  //~ D7h
#define   B_QNC_PCIE_MPC2_IPF                         (BIT11)  // ISOF Packet Fast Transmit Mode
#define R_QNC_PCIE_MPC                              0xD8  //~ DBh
#define   B_QNC_PCIE_MPC_PMCE                         (BIT31)  // PM SCI Enable
#define   B_QNC_PCIE_MPC_HPCE                         (BIT30)  // Hot plug SCI enable

#define   B_QNC_PCIE_MPC_HPME                         (BIT1)   // Hot plug SMI enable
#define   B_QNC_PCIE_MPC_PMME                         (BIT0)   // PM SMI Enable
#define R_QNC_PCIE_IOSFSBCTL                        0xF6
#define   B_QNC_PCIE_IOSFSBCTL_SBIC_MASK              (BIT1 | BIT0) // IOSF Sideband ISM Idle Counter.
#define   B_QNC_PCIE_IOSFSBCTL_SBIC_IDLE_NEVER        (BIT1 | BIT0) // Never transition to IDLE.

#define V_PCIE_MAX_TRY_TIMES                       200

//
// Misc PCI register offsets and sizes
//
#define R_EFI_PCI_SVID                              0x2C

//
// IO_APIC
//
#define IOAPIC_BASE                                 0xFEC00000
#define IOAPIC_SIZE                                 0x1000

//
// Chipset configuration registers RCBA - "Root Complex Base Address" (D31:F0:RF0h)
//            Suggested Value for  RCBA = 0xFED1C000
//

#define R_QNC_RCRB_SPIBASE                          0x3020       // SPI (Serial Peripheral Interface) in RCRB
#define R_QNC_RCRB_SPIS                             (R_QNC_RCRB_SPIBASE + 0x00)  // SPI Status
#define   B_QNC_RCRB_SPIS_SCL                       (BIT15)    // SPI Configuration Lockdown
#define   B_QNC_RCRB_SPIS_BAS                       (BIT3)     // Blocked Access Status
#define   B_QNC_RCRB_SPIS_CDS                       (BIT2)     // Cycle Done Status
#define   B_QNC_RCRB_SPIS_SCIP                      (BIT0)     // SPI Cycle in Progress

#define R_QNC_RCRB_SPIC                             (R_QNC_RCRB_SPIBASE + 0x02)  // SPI Control
#define   B_QNC_RCRB_SPIC_DC                          (BIT14)    // SPI Data Cycle Enable
#define   B_QNC_RCRB_SPIC_DBC                         0x3F00     // SPI Data Byte Count (1..8,16,24,32,40,48,56,64)
#define   B_QNC_RCRB_SPIC_COP                         (BIT6+BIT5+BIT4)          // SPI Cycle Opcode Pointer
#define   B_QNC_RCRB_SPIC_SPOP                        (BIT3)     // Sequence Prefix Opcode Pointer
#define   B_QNC_RCRB_SPIC_ACS                         (BIT2)     // SPI Atomic Cycle Sequence
#define   B_QNC_RCRB_SPIC_SCGO                        (BIT1)     // SPI Cycle Go

#define R_QNC_RCRB_SPIA                             (R_QNC_RCRB_SPIBASE + 0x04)  // SPI Address
#define   B_QNC_RCRB_SPIA_MASK                      0x00FFFFFF     // SPI Address mask
#define R_QNC_RCRB_SPID0                            (R_QNC_RCRB_SPIBASE + 0x08)  // SPI Data 0
#define R_QNC_RCRB_SPIPREOP                         (R_QNC_RCRB_SPIBASE + 0x54)  // Prefix Opcode Configuration
#define R_QNC_RCRB_SPIOPTYPE                        (R_QNC_RCRB_SPIBASE + 0x56)  // Opcode Type Configuration
#define   B_QNC_RCRB_SPIOPTYPE_NOADD_READ             0
#define   B_QNC_RCRB_SPIOPTYPE_NOADD_WRITE            (BIT0)
#define   B_QNC_RCRB_SPIOPTYPE_ADD_READ               (BIT1)
#define   B_QNC_RCRB_SPIOPTYPE_ADD_WRITE              (BIT0 + BIT1)
#define R_QNC_RCRB_SPIOPMENU                        (R_QNC_RCRB_SPIBASE + 0x58)  // Opcode Menu Configuration //R_OPMENU

#define R_QNC_RCRB_SPIPBR0                          (R_QNC_RCRB_SPIBASE + 0x60)  // Protected BIOS Range 0.
#define R_QNC_RCRB_SPIPBR1                          (R_QNC_RCRB_SPIBASE + 0x64)  // Protected BIOS Range 1.
#define R_QNC_RCRB_SPIPBR2                          (R_QNC_RCRB_SPIBASE + 0x68)  // Protected BIOS Range 2.
#define   B_QNC_RCRB_SPIPBRn_WPE                      (BIT31)                    // Write Protection Enable for above 3 registers.

#define R_QNC_RCRB_AGENT0IR                         0x3140   // AGENT0 interrupt route
#define R_QNC_RCRB_AGENT1IR                         0x3142   // AGENT1 interrupt route
#define R_QNC_RCRB_AGENT2IR                         0x3144   // AGENT2 interrupt route
#define R_QNC_RCRB_AGENT3IR                         0x3146   // AGENT3 interrupt route

#endif

/** @file
Header file for QuarkSCSocId Ioh.
Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#ifndef _IOH_H_
#define _IOH_H_

#ifndef BIT0
#define BIT0    0x01
#define BIT1    0x02
#define BIT2    0x04
#define BIT3    0x08
#define BIT4    0x10
#define BIT5    0x20
#define BIT6    0x40
#define BIT7    0x80
#define BIT8    0x100
#define BIT9    0x200
#define BIT00   0x00000001
#define BIT01   0x00000002
#define BIT02   0x00000004
#define BIT03   0x00000008
#define BIT04   0x00000010
#define BIT05   0x00000020
#define BIT06   0x00000040
#define BIT07   0x00000080
#define BIT08   0x00000100
#define BIT09   0x00000200
#define BIT10   0x00000400
#define BIT11   0x00000800
#define BIT12   0x00001000
#define BIT13   0x00002000
#define BIT14   0x00004000
#define BIT15   0x00008000
#define BIT16   0x00010000
#define BIT17   0x00020000
#define BIT18   0x00040000
#define BIT19   0x00080000
#define BIT20   0x00100000
#define BIT21   0x00200000
#define BIT22   0x00400000
#define BIT23   0x00800000
#define BIT24   0x01000000
#define BIT25   0x02000000
#define BIT26   0x04000000
#define BIT27   0x08000000
#define BIT28   0x10000000
#define BIT29   0x20000000
#define BIT30   0x40000000
#define BIT31   0x80000000
#endif

#define IOH_PCI_CFG_ADDRESS(bus,dev,func,reg) \
    ((UINT32) ( (((UINTN)bus) << 24) + (((UINTN)dev) << 16) + \
    (((UINTN)func) << 8) + ((UINTN)reg) ))& 0x00000000ffffffff

//----------------------------------------------------------------------------

#define INTEL_VENDOR_ID         0x8086  // Intel Vendor ID

//----------------------------------------------------------------------------
// Pci Configuration Map Register Offsets
//----------------------------------------------------------------------------
#define PCI_REG_VID             0x00    // Vendor ID Register
#define PCI_REG_DID             0x02    // Device ID Register
#define PCI_REG_PCICMD          0x04    // PCI Command Register
#define PCI_REG_PCISTS          0x06    // PCI Status Register
#define PCI_REG_RID             0x08    // PCI Revision ID Register
#define PCI_REG_PI              0x09    // Programming Interface
#define PCI_REG_SCC             0x0a    // Sub Class Code Register
#define PCI_REG_BCC             0x0b    // Base Class Code Register
#define PCI_REG_PMLT            0x0d    // Primary Master Latnecy Timer
#define PCI_REG_HDR             0x0e    // Header Type Register
#define PCI_REG_PBUS            0x18    // Primary Bus Number Register
#define PCI_REG_SBUS            0x19    // Secondary Bus Number Register
#define PCI_REG_SUBUS           0x1a    // Subordinate Bus Number Register
#define PCI_REG_SMLT            0x1b    // Secondary Master Latnecy Timer
#define PCI_REG_IOBASE          0x1c    // I/O base Register
#define PCI_REG_IOLIMIT         0x1d    // I/O Limit Register
#define PCI_REG_SECSTATUS       0x1e    // Secondary Status Register
#define PCI_REG_MEMBASE         0x20    // Memory Base Register
#define PCI_REG_MEMLIMIT        0x22    // Memory Limit Register
#define PCI_REG_PRE_MEMBASE     0x24    // Prefretchable memory Base register
#define PCI_REG_PRE_MEMLIMIT    0x26    // Prefretchable memory Limit register
#define PCI_REG_SVID0           0x2c    // Subsystem Vendor ID low byte
#define PCI_REG_SVID1           0x2d    // Subsystem Vendor ID high byte
#define PCI_REG_SID0            0x2e    // Subsystem ID low byte
#define PCI_REG_SID1            0x2f    // Subsystem ID high byte
#define PCI_REG_IOBASE_U        0x30    // I/O base Upper Register
#define PCI_REG_IOLIMIT_U       0x32    // I/O Limit Upper Register
#define PCI_REG_INTLINE         0x3c    // Interrupt Line Register
#define PCI_REG_BRIDGE_CNTL     0x3e    // Bridge Control Register

//---------------------------------------------------------------------------
// QuarkSCSocId Packet Hub definitions
//---------------------------------------------------------------------------

#define PCIE_BRIDGE_VID_DID     0x88008086

//---------------------------------------------------------------------------
// Quark South Cluster definitions.
//---------------------------------------------------------------------------

#define IOH_BUS                           0
#define IOH_PCI_IOSF2AHB_0_DEV_NUM        0x14
#define IOH_PCI_IOSF2AHB_0_MAX_FUNCS      7
#define IOH_PCI_IOSF2AHB_1_DEV_NUM        0x15
#define IOH_PCI_IOSF2AHB_1_MAX_FUNCS      3

//---------------------------------------------------------------------------
// Quark South Cluster USB definitions.
//---------------------------------------------------------------------------

#define IOH_USB_BUS_NUMBER                IOH_BUS
#define IOH_USB_CONTROLLER_MMIO_RANGE     0x1000
#define IOH_MAX_OHCI_USB_CONTROLLERS      1
#define IOH_MAX_EHCI_USB_CONTROLLERS      1
#define IOH_MAX_USBDEVICE_USB_CONTROLLERS 1

#define R_IOH_USB_VENDOR_ID               0x00
#define   V_IOH_USB_VENDOR_ID               INTEL_VENDOR_ID
#define R_IOH_USB_DEVICE_ID               0x02
#define R_IOH_USB_COMMAND                 0x04
#define   B_IOH_USB_COMMAND_BME             BIT2
#define   B_IOH_USB_COMMAND_MSE             BIT1
#define   B_IOH_USB_COMMAND_ISE             BIT0
#define R_IOH_USB_MEMBAR                  0x10
#define   B_IOH_USB_MEMBAR_ADDRESS_MASK     0xFFFFF000  // [31:12].
#define R_IOH_USB_OHCI_HCCABAR            0x18

//---------------------------------------------------------------------------
// Quark South Cluster OHCI definitions
//---------------------------------------------------------------------------
#define IOH_USB_OHCI_DEVICE_NUMBER        IOH_PCI_IOSF2AHB_0_DEV_NUM
#define IOH_OHCI_FUNCTION_NUMBER          0x04

//---------------------------------------------------------------------------
// Quark South Cluster EHCI definitions
//---------------------------------------------------------------------------
#define IOH_USB_EHCI_DEVICE_NUMBER        IOH_PCI_IOSF2AHB_0_DEV_NUM
#define IOH_EHCI_FUNCTION_NUMBER          0x03

//
// EHCI memory mapped registers offset from memory BAR0.
//
#define R_IOH_EHCI_CAPLENGTH              0x00
#define R_IOH_EHCI_INSNREG01              0x94
#define   B_IOH_EHCI_INSNREG01_OUT_THRESHOLD_BP    (16)
#define   B_IOH_EHCI_INSNREG01_OUT_THRESHOLD_MASK  (0xff << B_IOH_EHCI_INSNREG01_OUT_THRESHOLD_BP)
#define   B_IOH_EHCI_INSNREG01_IN_THRESHOLD_BP     (0)
#define   B_IOH_EHCI_INSNREG01_IN_THRESHOLD_MASK   (0xff << B_IOH_EHCI_INSNREG01_IN_THRESHOLD_BP)

//
// EHCI memory mapped registers offset from memory BAR0 + Cap length value.
//
#define R_IOH_EHCI_CONFIGFLAGS            0x40

//---------------------------------------------------------------------------
// Quark South Cluster USB Device definitions
//---------------------------------------------------------------------------
#define IOH_USBDEVICE_DEVICE_NUMBER       IOH_PCI_IOSF2AHB_0_DEV_NUM
#define IOH_USBDEVICE_FUNCTION_NUMBER     0x02

//
// USB Device memory mapped registers offset from memory BAR0.
//
#define R_IOH_USBDEVICE_D_INTR_UDC_REG                      0x40c
#define R_IOH_USBDEVICE_D_INTR_MSK_UDC_REG                  0x410
#define   B_IOH_USBDEVICE_D_INTR_MSK_UDC_REG_MASK1_MASK       0xff
#define R_IOH_USBDEVICE_EP_INTR_UDC_REG                     0x414
#define R_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG                 0x418
#define   B_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG_OUT_EP_MASK     0x000f0000
#define   B_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG_IN_EP_MASK      0x0000000f

//---------------------------------------------------------------------------
// Quark South Cluster 10/100 Mbps Ethernet Device definitions.
//---------------------------------------------------------------------------
#define IOH_MAC0_BUS_NUMBER                                 IOH_BUS
#define IOH_MAC0_DEVICE_NUMBER                              IOH_PCI_IOSF2AHB_0_DEV_NUM
#define IOH_MAC0_FUNCTION_NUMBER                            0x06
#define IOH_MAC1_BUS_NUMBER                                 IOH_BUS
#define IOH_MAC1_DEVICE_NUMBER                              IOH_PCI_IOSF2AHB_0_DEV_NUM
#define IOH_MAC1_FUNCTION_NUMBER                            0x07

//
// MAC Device PCI config registers.
//
#define R_IOH_MAC_DEVICE_ID                                 0x02
#define   V_IOH_MAC_VENDOR_ID                                 INTEL_VENDOR_ID
#define R_IOH_MAC_DEVICE_ID                                 0x02
#define   V_IOH_MAC_DEVICE_ID                                 0x0937
#define R_IOH_MAC_COMMAND                                   0x04
#define   B_IOH_MAC_COMMAND_BME                               BIT2
#define   B_IOH_MAC_COMMAND_MSE                               BIT1
#define   B_IOH_MAC_COMMAND_ISE                               BIT0
#define R_IOH_MAC_MEMBAR                                    0x10
#define   B_IOH_MAC_MEMBAR_ADDRESS_MASK                       0xFFFFF000

//
// LAN Device memory mapped registers offset from memory BAR0.
//
#define R_IOH_MAC_GMAC_REG_8                                0x20
#define   B_IOH_MAC_USERVER_MASK                              0x0000FF00
#define   B_IOH_MAC_SNPSVER_MASK                              0x000000FF
#define R_IOH_MAC_GMAC_REG_16                               0x40
#define   B_IOH_MAC_ADDRHI_MASK                               0x0000FFFF
#define   B_IOH_MAC_AE                                        BIT31
#define R_IOH_MAC_GMAC_REG_17                               0x44
#define   B_IOH_MAC_ADDRLO_MASK                               0xFFFFFFFF

//---------------------------------------------------------------------------
// Quark I2C / GPIO definitions
//---------------------------------------------------------------------------

#define   V_IOH_I2C_GPIO_VENDOR_ID          INTEL_VENDOR_ID
#define   V_IOH_I2C_GPIO_DEVICE_ID          0x0934

#define R_IOH_I2C_MEMBAR                  0x10
#define   B_IOH_I2C_GPIO_MEMBAR_ADDR_MASK   0xFFFFF000  // [31:12].

#define GPIO_SWPORTA_DR                   0x00
#define GPIO_SWPORTA_DDR                  0x04
#define GPIO_INTEN                        0x30
#define GPIO_INTMASK                      0x34
#define GPIO_INTTYPE_LEVEL                0x38
#define GPIO_INT_POLARITY                 0x3C
#define GPIO_INTSTATUS                    0x40
#define GPIO_RAW_INTSTATUS                0x44
#define GPIO_DEBOUNCE                     0x48
#define GPIO_PORTA_EOI                    0x4C
#define GPIO_EXT_PORTA                    0x50
#define GPIO_EXT_PORTB                    0x54
#define GPIO_LS_SYNC                      0x60
#define GPIO_CONFIG_REG2                  0x70
#define GPIO_CONFIG_REG1                  0x74

//---------------------------------------------------------------------------
// Quark South Cluster UART definitions.
//---------------------------------------------------------------------------

#define R_IOH_UART_MEMBAR                 0x10
#define   B_IOH_UART_MEMBAR_ADDRESS_MASK    0xFFFFF000  // [31:12].

#endif

/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ISP1761_USB_DXE_H__
#define __ISP1761_USB_DXE_H__

#define ISP1761_USB_BASE FixedPcdGet32 (PcdIsp1761BaseAddress)

#define READ_REG32(Offset) MmioRead32 (ISP1761_USB_BASE + Offset)
#define READ_REG16(Offset) (UINT16) READ_REG32 (Offset)
#define WRITE_REG32(Offset, Val)  MmioWrite32 (ISP1761_USB_BASE + Offset, Val)
#define WRITE_REG16(Offset, Val)  MmioWrite32 (ISP1761_USB_BASE + Offset, (UINT32) Val)
#define WRITE_REG8(Offset, Val)   MmioWrite32 (ISP1761_USB_BASE + Offset, (UINT32) Val)

// Max packet size in bytes (For Full Speed USB 64 is the only valid value)
#define MAX_PACKET_SIZE_CONTROL     64

#define MAX_PACKET_SIZE_BULK        512

// 8 Endpoints, in and out. Don't count the Endpoint 0 setup buffer
#define ISP1761_NUM_ENDPOINTS               16

// Endpoint Indexes
#define ISP1761_EP0SETUP                    0x20
#define ISP1761_EP0RX                       0x00
#define ISP1761_EP0TX                       0x01
#define ISP1761_EP1RX                       0x02
#define ISP1761_EP1TX                       0x03

// DcInterrupt bits
#define ISP1761_DC_INTERRUPT_BRESET         BIT0
#define ISP1761_DC_INTERRUPT_SOF            BIT1
#define ISP1761_DC_INTERRUPT_PSOF           BIT2
#define ISP1761_DC_INTERRUPT_SUSP           BIT3
#define ISP1761_DC_INTERRUPT_RESUME         BIT4
#define ISP1761_DC_INTERRUPT_HS_STAT        BIT5
#define ISP1761_DC_INTERRUPT_DMA            BIT6
#define ISP1761_DC_INTERRUPT_VBUS           BIT7
#define ISP1761_DC_INTERRUPT_EP0SETUP       BIT8
#define ISP1761_DC_INTERRUPT_EP0RX          BIT10
#define ISP1761_DC_INTERRUPT_EP0TX          BIT11
#define ISP1761_DC_INTERRUPT_EP1RX          BIT12
#define ISP1761_DC_INTERRUPT_EP1TX          BIT13
// All valid peripheral controller interrupts
#define ISP1761_DC_INTERRUPT_MASK           0x003FFFDFF

#define ISP1761_ADDRESS                     0x200
#define ISP1761_ADDRESS_DEVEN               BIT7

#define ISP1761_MODE                        0x20C
#define ISP1761_MODE_DATA_BUS_WIDTH         BIT8
#define ISP1761_MODE_CLKAON                 BIT7
#define ISP1761_MODE_SFRESET                BIT4
#define ISP1761_MODE_WKUPCS                 BIT2

#define ISP1761_ENDPOINT_MAX_PACKET_SIZE    0x204

#define ISP1761_ENDPOINT_TYPE               0x208
#define ISP1761_ENDPOINT_TYPE_NOEMPKT       BIT4
#define ISP1761_ENDPOINT_TYPE_ENABLE        BIT3

#define ISP1761_INTERRUPT_CONFIG            0x210
// Interrupt config value to only interrupt on ACK of IN and OUT tokens
#define ISP1761_INTERRUPT_CONFIG_ACK_ONLY   BIT2 | BIT5 | BIT6

#define ISP1761_DC_INTERRUPT                0x218
#define ISP1761_DC_INTERRUPT_ENABLE         0x214

#define ISP1761_CTRL_FUNCTION               0x228
#define ISP1761_CTRL_FUNCTION_VENDP         BIT3
#define ISP1761_CTRL_FUNCTION_DSEN          BIT2
#define ISP1761_CTRL_FUNCTION_STATUS        BIT1

#define ISP1761_DEVICE_UNLOCK               0x27C
#define ISP1761_DEVICE_UNLOCK_MAGIC         0xAA37

#define ISP1761_SW_RESET_REG                0x30C
#define ISP1761_SW_RESET_ALL                BIT0

#define ISP1761_DEVICE_ID                   0x370

#define ISP1761_OTG_CTRL_SET                0x374
#define ISP1761_OTG_CTRL_CLR                OTG_CTRL_SET + 2
#define ISP1761_OTG_CTRL_OTG_DISABLE        BIT10
#define ISP1761_OTG_CTRL_VBUS_CHRG          BIT6
#define ISP1761_OTG_CTRL_VBUS_DISCHRG       BIT5
#define ISP1761_OTG_CTRL_DM_PULLDOWN        BIT2
#define ISP1761_OTG_CTRL_DP_PULLDOWN        BIT1
#define ISP1761_OTG_CTRL_DP_PULLUP          BIT0

#define ISP1761_OTG_STATUS                  0x378
#define ISP1761_OTG_STATUS_B_SESS_END       BIT7
#define ISP1761_OTG_STATUS_A_B_SESS_VLD     BIT1

#define ISP1761_OTG_INTERRUPT_LATCH_SET     0x37C
#define ISP1761_OTG_INTERRUPT_LATCH_CLR     0x37E
#define ISP1761_OTG_INTERRUPT_ENABLE_RISE   0x384

#define ISP1761_DMA_ENDPOINT_INDEX          0x258

#define ISP1761_ENDPOINT_INDEX              0x22c
#define ISP1761_DATA_PORT                   0x220
#define ISP1761_BUFFER_LENGTH               0x21c

// Device ID Values
#define PHILLIPS_VENDOR_ID_VAL 0x04cc
#define ISP1761_PRODUCT_ID_VAL 0x1761
#define ISP1761_DEVICE_ID_VAL ((ISP1761_PRODUCT_ID_VAL << 16) |\
                               PHILLIPS_VENDOR_ID_VAL)

#endif //ifndef __ISP1761_USB_DXE_H__

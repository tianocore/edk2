/** @file  HDLcd.h

 Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

#ifndef _HDLCD_H_
#define _HDLCD_H_

//
// HDLCD Controller Register Offsets
//

#define HDLCD_REG_VERSION                 ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x000)
#define HDLCD_REG_INT_RAWSTAT             ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x010)
#define HDLCD_REG_INT_CLEAR               ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x014)
#define HDLCD_REG_INT_MASK                ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x018)
#define HDLCD_REG_INT_STATUS              ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x01C)
#define HDLCD_REG_FB_BASE                 ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x100)
#define HDLCD_REG_FB_LINE_LENGTH          ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x104)
#define HDLCD_REG_FB_LINE_COUNT           ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x108)
#define HDLCD_REG_FB_LINE_PITCH           ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x10C)
#define HDLCD_REG_BUS_OPTIONS             ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x110)
#define HDLCD_REG_V_SYNC                  ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x200)
#define HDLCD_REG_V_BACK_PORCH            ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x204)
#define HDLCD_REG_V_DATA                  ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x208)
#define HDLCD_REG_V_FRONT_PORCH           ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x20C)
#define HDLCD_REG_H_SYNC                  ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x210)
#define HDLCD_REG_H_BACK_PORCH            ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x214)
#define HDLCD_REG_H_DATA                  ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x218)
#define HDLCD_REG_H_FRONT_PORCH           ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x21C)
#define HDLCD_REG_POLARITIES              ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x220)
#define HDLCD_REG_COMMAND                 ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x230)
#define HDLCD_REG_PIXEL_FORMAT            ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x240)
#define HDLCD_REG_RED_SELECT              ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x244)
#define HDLCD_REG_GREEN_SELECT            ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x248)
#define HDLCD_REG_BLUE_SELECT             ((UINTN)PcdGet32 (PcdArmHdLcdBase) + 0x24C)


//
// HDLCD Values of registers
//

// HDLCD Interrupt mask, clear and status register
#define HDLCD_DMA_END                     BIT0    /* DMA has finished reading a frame */
#define HDLCD_BUS_ERROR                   BIT1    /* DMA bus error */
#define HDLCD_SYNC                        BIT2    /* Vertical sync */
#define HDLCD_UNDERRUN                    BIT3    /* No Data available while DATAEN active */

// CLCD_CONTROL Control register
#define HDLCD_DISABLE                     0
#define HDLCD_ENABLE                      BIT0

// Bus Options
#define HDLCD_BURST_1                     BIT0
#define HDLCD_BURST_2                     BIT1
#define HDLCD_BURST_4                     BIT2
#define HDLCD_BURST_8                     BIT3
#define HDLCD_BURST_16                    BIT4

// Polarities - HIGH
#define HDLCD_VSYNC_HIGH                  BIT0
#define HDLCD_HSYNC_HIGH                  BIT1
#define HDLCD_DATEN_HIGH                  BIT2
#define HDLCD_DATA_HIGH                   BIT3
#define HDLCD_PXCLK_HIGH                  BIT4
// Polarities - LOW (for completion and for ease of understanding the hardware settings)
#define HDLCD_VSYNC_LOW                   0
#define HDLCD_HSYNC_LOW                   0
#define HDLCD_DATEN_LOW                   0
#define HDLCD_DATA_LOW                    0
#define HDLCD_PXCLK_LOW                   0

// Pixel Format
#define HDLCD_LITTLE_ENDIAN              (0 << 31)
#define HDLCD_BIG_ENDIAN                 (1 << 31)

// Number of bytes per pixel
#define HDLCD_4BYTES_PER_PIXEL           ((4 - 1) << 3)

#endif /* _HDLCD_H_ */

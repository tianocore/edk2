/** @file  PL111Lcd.h

 Copyright (c) 2011, ARM Ltd. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#ifndef _PL111LCD_H__
#define _PL111LCD_H__

/**********************************************************************
 *
 *  This header file contains all the bits of the PL111 that are
 *  platform independent.
 *
 **********************************************************************/

// Controller Register Offsets
#define PL111_REG_LCD_TIMING_0  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x000)
#define PL111_REG_LCD_TIMING_1  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x004)
#define PL111_REG_LCD_TIMING_2  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x008)
#define PL111_REG_LCD_TIMING_3  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x00C)
#define PL111_REG_LCD_UP_BASE   ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x010)
#define PL111_REG_LCD_LP_BASE   ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x014)
#define PL111_REG_LCD_CONTROL   ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x018)
#define PL111_REG_LCD_IMSC      ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x01C)
#define PL111_REG_LCD_RIS       ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x020)
#define PL111_REG_LCD_MIS       ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x024)
#define PL111_REG_LCD_ICR       ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x028)
#define PL111_REG_LCD_UP_CURR   ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x02C)
#define PL111_REG_LCD_LP_CURR   ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x030)
#define PL111_REG_LCD_PALETTE   ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0x200)

// Identification Register Offsets
#define PL111_REG_CLCD_PERIPH_ID_0  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFE0)
#define PL111_REG_CLCD_PERIPH_ID_1  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFE4)
#define PL111_REG_CLCD_PERIPH_ID_2  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFE8)
#define PL111_REG_CLCD_PERIPH_ID_3  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFEC)
#define PL111_REG_CLCD_P_CELL_ID_0  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFF0)
#define PL111_REG_CLCD_P_CELL_ID_1  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFF4)
#define PL111_REG_CLCD_P_CELL_ID_2  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFF8)
#define PL111_REG_CLCD_P_CELL_ID_3  ((UINTN)PcdGet32 (PcdPL111LcdBase) + 0xFFC)

#define PL111_CLCD_PERIPH_ID_0  0x11
#define PL111_CLCD_PERIPH_ID_1  0x11
#define PL111_CLCD_PERIPH_ID_2  0x04
#define PL111_CLCD_PERIPH_ID_3  0x00
#define PL111_CLCD_P_CELL_ID_0  0x0D
#define PL111_CLCD_P_CELL_ID_1  0xF0
#define PL111_CLCD_P_CELL_ID_2  0x05
#define PL111_CLCD_P_CELL_ID_3  0xB1

/**********************************************************************/

// Register components (register bits)

// This should make life easier to program specific settings in the different registers
// by simplifying the setting up of the individual bits of each register
// and then assembling the final register value.

/**********************************************************************/

// Register: PL111_REG_LCD_TIMING_0
#define HOR_AXIS_PANEL(hbp, hfp, hsw, hor_res)  (UINT32)(((UINT32)(hbp) << 24) | ((UINT32)(hfp) << 16) | ((UINT32)(hsw) << 8) | (((UINT32)((hor_res)/16)-1) << 2))

// Register: PL111_REG_LCD_TIMING_1
#define VER_AXIS_PANEL(vbp, vfp, vsw, ver_res)  (UINT32)(((UINT32)(vbp) << 24) | ((UINT32)(vfp) << 16) | ((UINT32)(vsw) << 10) | ((ver_res)-1))

// Register: PL111_REG_LCD_TIMING_2
#define PL111_BIT_SHIFT_PCD_HI  27
#define PL111_BIT_SHIFT_BCD     26
#define PL111_BIT_SHIFT_CPL     16
#define PL111_BIT_SHIFT_IOE     14
#define PL111_BIT_SHIFT_IPC     13
#define PL111_BIT_SHIFT_IHS     12
#define PL111_BIT_SHIFT_IVS     11
#define PL111_BIT_SHIFT_ACB     6
#define PL111_BIT_SHIFT_CLKSEL  5
#define PL111_BIT_SHIFT_PCD_LO  0

#define PL111_BCD  (1 << 26)
#define PL111_IPC  (1 << 13)
#define PL111_IHS  (1 << 12)
#define PL111_IVS  (1 << 11)

#define CLK_SIG_POLARITY(hor_res)  (UINT32)(PL111_BCD | PL111_IPC | PL111_IHS | PL111_IVS | (((hor_res)-1) << 16))

// Register: PL111_REG_LCD_TIMING_3
#define PL111_BIT_SHIFT_LEE  16
#define PL111_BIT_SHIFT_LED  0

#define PL111_CTRL_WATERMARK      (1 << 16)
#define PL111_CTRL_LCD_V_COMP     (1 << 12)
#define PL111_CTRL_LCD_PWR        (1 << 11)
#define PL111_CTRL_BEPO           (1 << 10)
#define PL111_CTRL_BEBO           (1 << 9)
#define PL111_CTRL_BGR            (1 << 8)
#define PL111_CTRL_LCD_DUAL       (1 << 7)
#define PL111_CTRL_LCD_MONO_8     (1 << 6)
#define PL111_CTRL_LCD_TFT        (1 << 5)
#define PL111_CTRL_LCD_BW         (1 << 4)
#define PL111_CTRL_LCD_1BPP       (0 << 1)
#define PL111_CTRL_LCD_2BPP       (1 << 1)
#define PL111_CTRL_LCD_4BPP       (2 << 1)
#define PL111_CTRL_LCD_8BPP       (3 << 1)
#define PL111_CTRL_LCD_16BPP      (4 << 1)
#define PL111_CTRL_LCD_24BPP      (5 << 1)
#define PL111_CTRL_LCD_16BPP_565  (6 << 1)
#define PL111_CTRL_LCD_12BPP_444  (7 << 1)
#define PL111_CTRL_LCD_BPP(Bpp)  ((Bpp) << 1)
#define PL111_CTRL_LCD_EN  1

/**********************************************************************/

// Register: PL111_REG_LCD_TIMING_0
#define PL111_LCD_TIMING_0_HBP(hbp)  (((hbp) & 0xFF) << 24)
#define PL111_LCD_TIMING_0_HFP(hfp)  (((hfp) & 0xFF) << 16)
#define PL111_LCD_TIMING_0_HSW(hsw)  (((hsw) & 0xFF) << 8)
#define PL111_LCD_TIMING_0_PPL(ppl)  (((hsw) & 0x3F) << 2)

// Register: PL111_REG_LCD_TIMING_1
#define PL111_LCD_TIMING_1_VBP(vbp)  (((vbp) & 0xFF) << 24)
#define PL111_LCD_TIMING_1_VFP(vfp)  (((vfp) & 0xFF) << 16)
#define PL111_LCD_TIMING_1_VSW(vsw)  (((vsw) & 0x3F) << 10)
#define PL111_LCD_TIMING_1_LPP(lpp)  ((lpp) & 0xFC)

// Register: PL111_REG_LCD_TIMING_2
#define PL111_BIT_MASK_PCD_HI  0xF8000000
#define PL111_BIT_MASK_BCD     0x04000000
#define PL111_BIT_MASK_CPL     0x03FF0000
#define PL111_BIT_MASK_IOE     0x00004000
#define PL111_BIT_MASK_IPC     0x00002000
#define PL111_BIT_MASK_IHS     0x00001000
#define PL111_BIT_MASK_IVS     0x00000800
#define PL111_BIT_MASK_ACB     0x000007C0
#define PL111_BIT_MASK_CLKSEL  0x00000020
#define PL111_BIT_MASK_PCD_LO  0x0000001F

// Register: PL111_REG_LCD_TIMING_3
#define PL111_BIT_MASK_LEE  0x00010000
#define PL111_BIT_MASK_LED  0x0000007F

#endif /* _PL111LCD_H__ */

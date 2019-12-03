/** @file

  This header file contains the platform independent parts of ARM Mali DP

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef ARMMALIDP_H_
#define ARMMALIDP_H_

#define DP_BASE                            (FixedPcdGet64 (PcdArmMaliDpBase))

// MALI DP Ids
#define MALIDP_NOT_PRESENT                 0xFFF
#define MALIDP_500                         0x500
#define MALIDP_550                         0x550
#define MALIDP_650                         0x650

// DP500 Peripheral Ids
#define DP500_ID_PART_0                    0x00
#define DP500_ID_DES_0                     0xB
#define DP500_ID_PART_1                    0x5

#define DP500_ID_REVISION                  0x1
#define DP500_ID_JEDEC                     0x1
#define DP500_ID_DES_1                     0x3

#define DP500_PERIPHERAL_ID0_VAL           (DP500_ID_PART_0)
#define DP500_PERIPHERAL_ID1_VAL           ((DP500_ID_DES_0 << 4)      \
                                            | DP500_ID_PART_1)
#define DP500_PERIPHERAL_ID2_VAL           ((DP500_ID_REVISION << 4)   \
                                            | (DP500_ID_JEDEC << 3)    \
                                            | (DP500_ID_DES_1))

// DP550 Peripheral Ids
#define DP550_ID_PART_0                    0x50
#define DP550_ID_DES_0                     0xB
#define DP550_ID_PART_1                    0x5

#define DP550_ID_REVISION                  0x0
#define DP550_ID_JEDEC                     0x1
#define DP550_ID_DES_1                     0x3

#define DP550_PERIPHERAL_ID0_VAL           (DP550_ID_PART_0)
#define DP550_PERIPHERAL_ID1_VAL           ((DP550_ID_DES_0 << 4)      \
                                               | DP550_ID_PART_1)
#define DP550_PERIPHERAL_ID2_VAL           ((DP550_ID_REVISION << 4)   \
                                               | (DP550_ID_JEDEC << 3) \
                                               | (DP550_ID_DES_1))

// DP650 Peripheral Ids
#define DP650_ID_PART_0                    0x50
#define DP650_ID_DES_0                     0xB
#define DP650_ID_PART_1                    0x6

#define DP650_ID_REVISION                  0x0
#define DP650_ID_JEDEC                     0x1
#define DP650_ID_DES_1                     0x3

#define DP650_PERIPHERAL_ID0_VAL           (DP650_ID_PART_0)
#define DP650_PERIPHERAL_ID1_VAL           ((DP650_ID_DES_0 << 4)      \
                                            | DP650_ID_PART_1)
#define DP650_PERIPHERAL_ID2_VAL           ((DP650_ID_REVISION << 4)   \
                                            | (DP650_ID_JEDEC << 3)    \
                                            | (DP650_ID_DES_1))

// Display Engine (DE) control register offsets for DP550/DP650
#define DP_DE_STATUS                       0x00000
#define DP_DE_IRQ_SET                      0x00004
#define DP_DE_IRQ_MASK                     0x00008
#define DP_DE_IRQ_CLEAR                    0x0000C
#define DP_DE_CONTROL                      0x00010
#define DP_DE_PROG_LINE                    0x00014
#define DP_DE_AXI_CONTROL                  0x00018
#define DP_DE_AXI_QOS                      0x0001C
#define DP_DE_DISPLAY_FUNCTION             0x00020

#define DP_DE_H_INTERVALS                  0x00030
#define DP_DE_V_INTERVALS                  0x00034
#define DP_DE_SYNC_CONTROL                 0x00038
#define DP_DE_HV_ACTIVESIZE                0x0003C
#define DP_DE_DISPLAY_SIDEBAND             0x00040
#define DP_DE_BACKGROUND_COLOR             0x00044
#define DP_DE_DISPLAY_SPLIT                0x00048
#define DP_DE_OUTPUT_DEPTH                 0x0004C

// Display Engine (DE) control register offsets for DP500
#define DP_DE_DP500_CORE_ID                0x00018
#define DP_DE_DP500_CONTROL                0x0000C
#define DP_DE_DP500_PROG_LINE              0x00010
#define DP_DE_DP500_H_INTERVALS            0x00028
#define DP_DE_DP500_V_INTERVALS            0x0002C
#define DP_DE_DP500_SYNC_CONTROL           0x00030
#define DP_DE_DP500_HV_ACTIVESIZE          0x00034
#define DP_DE_DP500_BG_COLOR_RG            0x0003C
#define DP_DE_DP500_BG_COLOR_B             0x00040

/* Display Engine (DE) graphics layer (LG) register offsets
 * NOTE: For DP500 it will be LG2.
 */
#define DE_LG_OFFSET                       0x00300
#define DP_DE_LG_FORMAT                    (DE_LG_OFFSET)
#define DP_DE_LG_CONTROL                   (DE_LG_OFFSET + 0x04)
#define DP_DE_LG_COMPOSE                   (DE_LG_OFFSET + 0x08)
#define DP_DE_LG_IN_SIZE                   (DE_LG_OFFSET + 0x0C)
#define DP_DE_LG_CMP_SIZE                  (DE_LG_OFFSET + 0x10)
#define DP_DE_LG_OFFSET                    (DE_LG_OFFSET + 0x14)
#define DP_DE_LG_H_STRIDE                  (DE_LG_OFFSET + 0x18)
#define DP_DE_LG_PTR_LOW                   (DE_LG_OFFSET + 0x1C)
#define DP_DE_LG_PTR_HIGH                  (DE_LG_OFFSET + 0x20)
#define DP_DE_LG_CHROMA_KEY                (DE_LG_OFFSET + 0x2C)
#define DP_DE_LG_AD_CONTROL                (DE_LG_OFFSET + 0x30)
#define DP_DE_LG_MMU_CONTROL               (DE_LG_OFFSET + 0x48)

// Display core (DC) control register offsets.
#define DP_DC_OFFSET                       0x0C000
#define DP_DC_STATUS                       (DP_DC_OFFSET + 0x00)
#define DP_DC_IRQ_SET                      (DP_DC_OFFSET + 0x04)
#define DP_DC_IRQ_MASK                     (DP_DC_OFFSET + 0x08)
#define DP_DC_IRQ_CLEAR                    (DP_DC_OFFSET + 0x0C)
#define DP_DC_CONTROL                      (DP_DC_OFFSET + 0x10)
#define DP_DC_CONFIG_VALID                 (DP_DC_OFFSET + 0x14)
#define DP_DC_CORE_ID                      (DP_DC_OFFSET + 0x18)

// DP500 has a global configuration register.
#define DP_DP500_CONFIG_VALID              (0xF00)

// Display core ID register offsets.
#define DP_DC_ID_OFFSET                    0x0FF00
#define DP_DC_ID_PERIPHERAL_ID4            (DP_DC_ID_OFFSET + 0xD0)
#define DP_DC_CONFIGURATION_ID             (DP_DC_ID_OFFSET + 0xD4)
#define DP_DC_PERIPHERAL_ID0               (DP_DC_ID_OFFSET + 0xE0)
#define DP_DC_PERIPHERAL_ID1               (DP_DC_ID_OFFSET + 0xE4)
#define DP_DC_PERIPHERAL_ID2               (DP_DC_ID_OFFSET + 0xE8)
#define DP_DC_COMPONENT_ID0                (DP_DC_ID_OFFSET + 0xF0)
#define DP_DC_COMPONENT_ID1                (DP_DC_ID_OFFSET + 0xF4)
#define DP_DC_COMPONENT_ID2                (DP_DC_ID_OFFSET + 0xF8)
#define DP_DC_COMPONENT_ID3                (DP_DC_ID_OFFSET + 0xFC)

#define DP_DP500_ID_OFFSET                 0x0F00
#define DP_DP500_ID_PERIPHERAL_ID4         (DP_DP500_ID_OFFSET + 0xD0)
#define DP_DP500_CONFIGURATION_ID          (DP_DP500_ID_OFFSET + 0xD4)
#define DP_DP500_PERIPHERAL_ID0            (DP_DP500_ID_OFFSET + 0xE0)
#define DP_DP500_PERIPHERAL_ID1            (DP_DP500_ID_OFFSET + 0xE4)
#define DP_DP500_PERIPHERAL_ID2            (DP_DP500_ID_OFFSET + 0xE8)
#define DP_DP500_COMPONENT_ID0             (DP_DP500_ID_OFFSET + 0xF0)
#define DP_DP500_COMPONENT_ID1             (DP_DP500_ID_OFFSET + 0xF4)
#define DP_DP500_COMPONENT_ID2             (DP_DP500_ID_OFFSET + 0xF8)
#define DP_DP500_COMPONENT_ID3             (DP_DP500_ID_OFFSET + 0xFC)

// Display status configuration mode activation flag
#define DP_DC_STATUS_CM_ACTIVE_FLAG        (0x1U << 16)

// Display core control configuration mode
#define DP_DC_CONTROL_SRST_ACTIVE          (0x1U << 18)
#define DP_DC_CONTROL_CRST_ACTIVE          (0x1U << 17)
#define DP_DC_CONTROL_CM_ACTIVE            (0x1U << 16)

#define DP_DE_DP500_CONTROL_SOFTRESET_REQ  (0x1U << 16)
#define DP_DE_DP500_CONTROL_CONFIG_REQ     (0x1U << 17)

// Display core configuration valid register
#define DP_DC_CONFIG_VALID_CVAL            (0x1U)

// DC_CORE_ID
// Display core version register PRODUCT_ID
#define DP_DC_CORE_ID_SHIFT                16
#define DP_DE_DP500_CORE_ID_SHIFT          DP_DC_CORE_ID_SHIFT

// Timing settings
#define DP_DE_HBACKPORCH_SHIFT             16
#define DP_DE_VBACKPORCH_SHIFT             16
#define DP_DE_VSP_SHIFT                    28
#define DP_DE_VSYNCWIDTH_SHIFT             16
#define DP_DE_HSP_SHIFT                    13
#define DP_DE_V_ACTIVE_SHIFT               16

// BACKGROUND_COLOR
#define DP_DE_BG_R_PIXEL_SHIFT             16
#define DP_DE_BG_G_PIXEL_SHIFT             8

//Graphics layer LG_FORMAT Pixel Format
#define DP_PIXEL_FORMAT_ARGB_8888          0x8
#define DP_PIXEL_FORMAT_ABGR_8888          0x9
#define DP_PIXEL_FORMAT_RGBA_8888          0xA
#define DP_PIXEL_FORMAT_BGRA_8888          0xB
#define DP_PIXEL_FORMAT_XRGB_8888          0x10
#define DP_PIXEL_FORMAT_XBGR_8888          0x11
#define DP_PIXEL_FORMAT_RGBX_8888          0x12
#define DP_PIXEL_FORMAT_BGRX_8888          0x13
#define DP_PIXEL_FORMAT_RGB_888            0x18
#define DP_PIXEL_FORMAT_BGR_888            0x19

// DP500 format code are different than DP550/DP650
#define DP_PIXEL_FORMAT_DP500_ARGB_8888    0x2
#define DP_PIXEL_FORMAT_DP500_ABGR_8888    0x3
#define DP_PIXEL_FORMAT_DP500_XRGB_8888    0x4
#define DP_PIXEL_FORMAT_DP500_XBGR_8888    0x5

// Graphics layer LG_PTR_LOW and LG_PTR_HIGH
#define DP_DE_LG_PTR_LOW_MASK              0xFFFFFFFFU
#define DP_DE_LG_PTR_HIGH_SHIFT            32

// Graphics layer LG_CONTROL register characteristics
#define DP_DE_LG_L_ALPHA_SHIFT             16
#define DP_DE_LG_CHK_SHIFT                 15
#define DP_DE_LG_PMUL_SHIFT                14
#define DP_DE_LG_COM_SHIFT                 12
#define DP_DE_LG_VFP_SHIFT                 11
#define DP_DE_LG_HFP_SHIFT                 10
#define DP_DE_LG_ROTATION_SHIFT            8

#define DP_DE_LG_LAYER_BLEND_NO_BG         0x0U
#define DP_DE_LG_PIXEL_BLEND_NO_BG         0x1U
#define DP_DE_LG_LAYER_BLEND_BG            0x2U
#define DP_DE_LG_PIXEL_BLEND_BG            0x3U
#define DP_DE_LG_ENABLE                    0x1U

// Graphics layer LG_IN_SIZE register characteristics
#define DP_DE_LG_V_IN_SIZE_SHIFT           16

// Graphics layer LG_CMP_SIZE register characteristics
#define DP_DE_LG_V_CMP_SIZE_SHIFT          16
#define DP_DE_LG_V_OFFSET_SHIFT            16

// Helper display timing macro functions.
#define H_INTERVALS(Hfp, Hbp)        ((Hbp << DP_DE_HBACKPORCH_SHIFT) | Hfp)
#define V_INTERVALS(Vfp, Vbp)        ((Vbp << DP_DE_VBACKPORCH_SHIFT) | Vfp)
#define SYNC_WIDTH(Hsw, Vsw)         ((Vsw << DP_DE_VSYNCWIDTH_SHIFT) | Hsw)
#define HV_ACTIVE(Hor, Ver)          ((Ver << DP_DE_V_ACTIVE_SHIFT)   | Hor)

// Helper layer graphics macros.
#define FRAME_IN_SIZE(Hor, Ver)      ((Ver << DP_DE_LG_V_IN_SIZE_SHIFT) | Hor)
#define FRAME_CMP_SIZE(Hor, Ver)     ((Ver << DP_DE_LG_V_CMP_SIZE_SHIFT) | Hor)

#endif /* ARMMALIDP_H_ */

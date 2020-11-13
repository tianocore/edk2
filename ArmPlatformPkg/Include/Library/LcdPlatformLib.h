/** @file

 Copyright (c) 2011-2020, Arm Limited. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#ifndef LCD_PLATFORM_LIB_H_
#define LCD_PLATFORM_LIB_H_

#include <Protocol/GraphicsOutput.h>

#define LCD_VRAM_SIZE                     SIZE_8MB

// Modes definitions
#define VGA                               0
#define SVGA                              1
#define XGA                               2
#define SXGA                              3
#define WSXGA                             4
#define UXGA                              5
#define HD                                6
#define WVGA                              7
#define QHD                               8
#define WSVGA                             9
#define HD720                             10
#define WXGA                              11

// VGA Mode: 640 x 480
#define VGA_H_RES_PIXELS                  640
#define VGA_V_RES_PIXELS                  480
#define VGA_OSC_FREQUENCY                 23750000  /* 0x016A6570 */

#define VGA_H_SYNC                        ( 80 - 1)
#define VGA_H_FRONT_PORCH                 ( 16 - 1)
#define VGA_H_BACK_PORCH                  ( 64 - 1)

#define VGA_V_SYNC                        (  4 - 1)
#define VGA_V_FRONT_PORCH                 (  3 - 1)
#define VGA_V_BACK_PORCH                  ( 13 - 1)

// SVGA Mode: 800 x 600
#define SVGA_H_RES_PIXELS                 800
#define SVGA_V_RES_PIXELS                 600
#define SVGA_OSC_FREQUENCY                38250000  /* 0x0247A610 */

#define SVGA_H_SYNC                       ( 80 - 1)
#define SVGA_H_FRONT_PORCH                ( 32 - 1)
#define SVGA_H_BACK_PORCH                 (112 - 1)

#define SVGA_V_SYNC                       (  4 - 1)
#define SVGA_V_FRONT_PORCH                (  3 - 1)
#define SVGA_V_BACK_PORCH                 ( 17 - 1)

// XGA Mode: 1024 x 768
#define XGA_H_RES_PIXELS                  1024
#define XGA_V_RES_PIXELS                  768
#define XGA_OSC_FREQUENCY                 63500000  /* 0x03C8EEE0 */

#define XGA_H_SYNC                        (104 - 1)
#define XGA_H_FRONT_PORCH                 ( 48 - 1)
#define XGA_H_BACK_PORCH                  (152 - 1)

#define XGA_V_SYNC                        (  4 - 1)
#define XGA_V_FRONT_PORCH                 (  3 - 1)
#define XGA_V_BACK_PORCH                  ( 23 - 1)

// SXGA Mode: 1280 x 1024
#define SXGA_H_RES_PIXELS                 1280
#define SXGA_V_RES_PIXELS                 1024
#define SXGA_OSC_FREQUENCY                109000000  /* 0x067F3540 */

#define SXGA_H_SYNC                       (136 - 1)
#define SXGA_H_FRONT_PORCH                ( 80 - 1)
#define SXGA_H_BACK_PORCH                 (216 - 1)

#define SXGA_V_SYNC                       (  7 - 1)
#define SXGA_V_FRONT_PORCH                (  3 - 1)
#define SXGA_V_BACK_PORCH                 ( 29 - 1)

// WSXGA+ Mode: 1680 x 1050
#define WSXGA_H_RES_PIXELS                1680
#define WSXGA_V_RES_PIXELS                1050
#define WSXGA_OSC_FREQUENCY               147000000  /* 0x08C30AC0 */

#define WSXGA_H_SYNC                      (170 - 1)
#define WSXGA_H_FRONT_PORCH               (104 - 1)
#define WSXGA_H_BACK_PORCH                (274 - 1)

#define WSXGA_V_SYNC                      (  5 - 1)
#define WSXGA_V_FRONT_PORCH               (  4 - 1)
#define WSXGA_V_BACK_PORCH                ( 41 - 1)

// UXGA Mode: 1600 x 1200
#define UXGA_H_RES_PIXELS                 1600
#define UXGA_V_RES_PIXELS                 1200
#define UXGA_OSC_FREQUENCY                161000000  /* 0x0998AA40 */

#define UXGA_H_SYNC                       (168 - 1)
#define UXGA_H_FRONT_PORCH                (112 - 1)
#define UXGA_H_BACK_PORCH                 (280 - 1)

#define UXGA_V_SYNC                       (  4 - 1)
#define UXGA_V_FRONT_PORCH                (  3 - 1)
#define UXGA_V_BACK_PORCH                 ( 38 - 1)

// HD Mode: 1920 x 1080
#define HD_H_RES_PIXELS                   1920
#define HD_V_RES_PIXELS                   1080
#define HD_OSC_FREQUENCY                  165000000  /* 0x09D5B340 */

#define HD_H_SYNC                         ( 79 - 1)
#define HD_H_FRONT_PORCH                  (128 - 1)
#define HD_H_BACK_PORCH                   (328 - 1)

#define HD_V_SYNC                         (  5 - 1)
#define HD_V_FRONT_PORCH                  (  3 - 1)
#define HD_V_BACK_PORCH                   ( 32 - 1)

// WVGA Mode: 800 x 480
#define WVGA_H_RES_PIXELS                 800
#define WVGA_V_RES_PIXELS                 480
#define WVGA_OSC_FREQUENCY                29500000   /* 0x01C22260 */
#define WVGA_H_SYNC                       ( 72 - 1)
#define WVGA_H_FRONT_PORCH                ( 24 - 1)
#define WVGA_H_BACK_PORCH                 ( 96 - 1)
#define WVGA_V_SYNC                       (  7 - 1)
#define WVGA_V_FRONT_PORCH                (  3 - 1)
#define WVGA_V_BACK_PORCH                 ( 10 - 1)

// QHD Mode: 960 x 540
#define QHD_H_RES_PIXELS                  960
#define QHD_V_RES_PIXELS                  540
#define QHD_OSC_FREQUENCY                 40750000   /* 0x026DCBB0 */
#define QHD_H_SYNC                        ( 96 - 1)
#define QHD_H_FRONT_PORCH                 ( 32 - 1)
#define QHD_H_BACK_PORCH                  (128 - 1)
#define QHD_V_SYNC                        (  5 - 1)
#define QHD_V_FRONT_PORCH                 (  3 - 1)
#define QHD_V_BACK_PORCH                  ( 14 - 1)

// WSVGA Mode: 1024 x 600
#define WSVGA_H_RES_PIXELS                1024
#define WSVGA_V_RES_PIXELS                600
#define WSVGA_OSC_FREQUENCY               49000000   /* 0x02EBAE40 */
#define WSVGA_H_SYNC                      (104 - 1)
#define WSVGA_H_FRONT_PORCH               ( 40 - 1)
#define WSVGA_H_BACK_PORCH                (144 - 1)
#define WSVGA_V_SYNC                      ( 10 - 1)
#define WSVGA_V_FRONT_PORCH               (  3 - 1)
#define WSVGA_V_BACK_PORCH                ( 11 - 1)

// HD720 Mode: 1280 x 720
#define HD720_H_RES_PIXELS                 1280
#define HD720_V_RES_PIXELS                 720
#define HD720_OSC_FREQUENCY                74500000   /* 0x0470C7A0 */
#define HD720_H_SYNC                       (128 - 1)
#define HD720_H_FRONT_PORCH                ( 64 - 1)
#define HD720_H_BACK_PORCH                 (192 - 1)
#define HD720_V_SYNC                       (  5 - 1)
#define HD720_V_FRONT_PORCH                (  3 - 1)
#define HD720_V_BACK_PORCH                 ( 20 - 1)

// WXGA Mode: 1280 x 800
#define WXGA_H_RES_PIXELS                  1280
#define WXGA_V_RES_PIXELS                  800
#define WXGA_OSC_FREQUENCY                 83500000  /* 0x04FA1BE0 */
#define WXGA_H_SYNC                        (128 - 1)
#define WXGA_H_FRONT_PORCH                 ( 72 - 1)
#define WXGA_H_BACK_PORCH                  (200 - 1)
#define WXGA_V_SYNC                        (  6 - 1)
#define WXGA_V_FRONT_PORCH                 (  3 - 1)
#define WXGA_V_BACK_PORCH                  ( 22 - 1)

// Colour Masks
#define LCD_24BPP_RED_MASK              0x00FF0000
#define LCD_24BPP_GREEN_MASK            0x0000FF00
#define LCD_24BPP_BLUE_MASK             0x000000FF
#define LCD_24BPP_RESERVED_MASK         0xFF000000

#define LCD_16BPP_555_RED_MASK          0x00007C00
#define LCD_16BPP_555_GREEN_MASK        0x000003E0
#define LCD_16BPP_555_BLUE_MASK         0x0000001F
#define LCD_16BPP_555_RESERVED_MASK     0x00000000

#define LCD_16BPP_565_RED_MASK          0x0000F800
#define LCD_16BPP_565_GREEN_MASK        0x000007E0
#define LCD_16BPP_565_BLUE_MASK         0x0000001F
#define LCD_16BPP_565_RESERVED_MASK     0x00008000

#define LCD_12BPP_444_RED_MASK          0x00000F00
#define LCD_12BPP_444_GREEN_MASK        0x000000F0
#define LCD_12BPP_444_BLUE_MASK         0x0000000F
#define LCD_12BPP_444_RESERVED_MASK     0x0000F000

/** The enumeration maps the PL111 LcdBpp values used in the LCD Control
  Register
**/
typedef enum {
  LcdBitsPerPixel_1 = 0,
  LcdBitsPerPixel_2,
  LcdBitsPerPixel_4,
  LcdBitsPerPixel_8,
  LcdBitsPerPixel_16_555,
  LcdBitsPerPixel_24,
  LcdBitsPerPixel_16_565,
  LcdBitsPerPixel_12_444,
  LcdBitsPerPixel_Max
} LCD_BPP;

// Display timing settings.
typedef struct {
  UINT32                      Resolution;
  UINT32                      Sync;
  UINT32                      BackPorch;
  UINT32                      FrontPorch;
} SCAN_TIMINGS;

/** Platform related initialization function.

  @param[in] Handle              Handle to the LCD device instance.

  @retval EFI_SUCCESS            Plaform library initialized successfully.
  @retval !(EFI_SUCCESS)         Other errors.
**/
EFI_STATUS
LcdPlatformInitializeDisplay (
  IN EFI_HANDLE   Handle
  );

/** Allocate VRAM memory in DRAM for the framebuffer
  (unless it is reserved already).

  The allocated address can be used to set the framebuffer.

  @param[out] VramBaseAddress      A pointer to the framebuffer address.
  @param[out] VramSize             A pointer to the size of the frame
                                   buffer in bytes

  @retval EFI_SUCCESS              Frame buffer memory allocated successfully.
  @retval !(EFI_SUCCESS)           Other errors.
**/
EFI_STATUS
LcdPlatformGetVram (
  OUT EFI_PHYSICAL_ADDRESS*                 VramBaseAddress,
  OUT UINTN*                                VramSize
  );

/** Return total number of modes supported.

  Note: Valid mode numbers are 0 to MaxMode - 1
  See Section 12.9 of the UEFI Specification 2.7

  @retval UINT32             Mode Number.
**/
UINT32
LcdPlatformGetMaxMode (
  VOID
  );

/** Set the requested display mode.

  @param[in] ModeNumber            Mode Number.

  @retval  EFI_SUCCESS             Mode set successfully.
  @retval  EFI_INVALID_PARAMETER   Requested mode not found.
  @retval  !(EFI_SUCCESS)          Other errors.
**/
EFI_STATUS
LcdPlatformSetMode (
  IN UINT32                                 ModeNumber
  );

/** Return information for the requested mode number.

  @param[in]  ModeNumber         Mode Number.
  @param[out] Info               Pointer for returned mode information
                                 (on success).

  @retval EFI_SUCCESS             Mode information for the requested mode
                                  returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformQueryMode (
  IN  UINT32                                ModeNumber,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  );

/** Return display timing information for the requested mode number.

  @param[in]  ModeNumber          Mode Number.

  @param[out] Horizontal          Pointer to horizontal timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)
  @param[out] Vertical            Pointer to vertical timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)


  @retval EFI_SUCCESS             Display timing information for the requested
                                  mode returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformGetTimings (
  IN  UINT32                              ModeNumber,
  OUT SCAN_TIMINGS                        **Horizontal,
  OUT SCAN_TIMINGS                        **Vertical
  );

/** Return bits per pixel information for a mode number.

  @param[in]  ModeNumber          Mode Number.

  @param[out] Bpp                 Pointer to value bits per pixel information.

  @retval EFI_SUCCESS             Bit per pixel information for the requested
                                  mode returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformGetBpp (
  IN  UINT32                                ModeNumber,
  OUT LCD_BPP*                              Bpp
  );

#endif /* LCD_PLATFORM_LIB_H_ */

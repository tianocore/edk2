/** @file

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.
  Copyright (c) 2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/DebugLib.h>
#include <Library/LcdPlatformLib.h>

/** Platform related initialization function.

  @param[in] Handle              Handle to the LCD device instance.

  @retval EFI_UNSUPPORTED        Interface is not supported.
**/
EFI_STATUS
LcdPlatformInitializeDisplay (
  IN EFI_HANDLE  Handle
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** Allocate VRAM memory in DRAM for the framebuffer
  (unless it is reserved already).

  The allocated address can be used to set the framebuffer.

  @param[out] VramBaseAddress      A pointer to the framebuffer address.
  @param[out] VramSize             A pointer to the size of the frame
                                   buffer in bytes

  @retval EFI_UNSUPPORTED          Interface is not supported.
**/
EFI_STATUS
LcdPlatformGetVram (
  OUT EFI_PHYSICAL_ADDRESS  *VramBaseAddress,
  OUT UINTN                 *VramSize
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** Return total number of modes supported.

  Note: Valid mode numbers are 0 to MaxMode - 1
  See Section 12.9 of the UEFI Specification 2.7

  @retval UINT32             Zero number of modes supported
                             in a NULL library implementation.
**/
UINT32
LcdPlatformGetMaxMode (
  VOID
  )
{
  ASSERT (FALSE);
  return 0;
}

/** Set the requested display mode.

  @param[in] ModeNumber            Mode Number.

  @retval EFI_UNSUPPORTED          Interface is not supported.
**/
EFI_STATUS
LcdPlatformSetMode (
  IN UINT32  ModeNumber
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** Return information for the requested mode number.

  @param[in]  ModeNumber         Mode Number.
  @param[out] Info               Pointer for returned mode information
                                 (on success).

  @retval EFI_UNSUPPORTED        Interface is not supported.

**/
EFI_STATUS
LcdPlatformQueryMode (
  IN  UINT32                                ModeNumber,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** Return display timing information for the requested mode number.

  @param[in]  ModeNumber          Mode Number.

  @param[out] HRes                Pointer to horizontal resolution.
  @param[out] HSync               Pointer to horizontal sync width.
  @param[out] HBackPorch          Pointer to horizontal back porch.
  @param[out] HFrontPorch         Pointer to horizontal front porch.
  @param[out] VRes                Pointer to vertical resolution.
  @param[out] VSync               Pointer to vertical sync width.
  @param[out] VBackPorch          Pointer to vertical back porch.
  @param[out] VFrontPorch         Pointer to vertical front porch.

  @retval EFI_UNSUPPORTED         Interface is not supported.
**/
EFI_STATUS
LcdPlatformGetTimings (
  IN  UINT32        ModeNumber,
  OUT SCAN_TIMINGS  **Horizontal,
  OUT SCAN_TIMINGS  **Vertical
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** Return bits per pixel information for a mode number.

  @param[in]  ModeNumber          Mode Number.

  @param[out] Bpp                 Pointer to value bits per pixel information.

  @retval EFI_UNSUPPORTED         Interface is not supported.

**/
EFI_STATUS
LcdPlatformGetBpp (
  IN  UINT32   ModeNumber,
  OUT LCD_BPP  *Bpp
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** @file
  Provides services to display completion progress when processing a
  firmware update that updates the firmware image in a firmware device.
  A platform may provide its own instance of this library class to custoimize
  how a user is informed of completion progress.

  Copyright (c) 2016, Microsoft Corporation
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DISPLAY_PROGRESS_LIB__
#define __DISPLAY_PROGRESS_LIB__

#include <Protocol/GraphicsOutput.h>

/**
  Indicates the current completion progress of a firmware update.

  @param[in] Completion  A value between 0 and 100 indicating the current
                         completion progress of a firmware update.  This
                         value must the the same or higher than previous
                         calls to this service.  The first call of 0 or a
                         value of 0 after reaching a value of 100 resets
                         the progress indicator to 0.
  @param[in] Color       Color of the progress indicator.  Only used when
                         Completion is 0 to set the color of the progress
                         indicator.  If Color is NULL, then the default color
                         is used.

  @retval EFI_SUCCESS            Progress displayed successfully.
  @retval EFI_INVALID_PARAMETER  Completion is not in range 0..100.
  @retval EFI_INVALID_PARAMETER  Completion is less than Completion value from
                                 a previous call to this service.
  @retval EFI_NOT_READY          The device used to indicate progress is not
                                 available.
**/
EFI_STATUS
EFIAPI
DisplayUpdateProgress (
  IN UINTN                                Completion,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  *Color       OPTIONAL
  );

#endif

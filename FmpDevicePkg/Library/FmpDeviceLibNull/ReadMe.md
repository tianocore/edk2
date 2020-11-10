# FMP Device NULL Library Instance

## About

This is a NULL instance of the `FmpDeviceLib` library class. This instance is provided for building when an actual
library instance is not needed. Any device firmware that is actually updated using `FmpDevicePkg` should always
implement a device-specific instance of `FmpDeviceLib`.

## API Overview

This library provides an interface for component involved in controlling the firmware update process (such as
`FmpDxe`) to retrieve information specific to the device whose firmware is being updated. The current library API
along with brief descriptions follow below. For more detailed information, check the individual function description
comment that precedes each function in the library header file.

* `FmpDeviceCheckImage ()` - Checks if a given firmware image is valid for the device. This function is retained in
  the library class for backward compatibility and is implemented as a wrapper around
  `FmpDeviceCheckImageWithStatus ()`. `FmpDxe` does not call this function directly.

  The NULL implementation always returns `EFI_SUCCESS`.

* `FmpDeviceCheckImageWithStatus ()` - Checks if a given firmware image is valid for the device.

  The NULL implementation always returns `EFI_SUCCESS`.

* `FmpDeviceGetAttributes ()` - Returns values used to indicate what firmware image attributes are supported and
   valid such as whether the image is updatable, a reset is required after update, authentication is required, and
   whether the image is a UEFI image.

  The NULL implementation always returns `EFI_SUCCESS`.

* `FmpDeviceGetHardwareInstance ()` - Returns an instance of the firmware image currently stored on the device.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceGetImage ()` - Returns a copy of the firmware image currently stored on the device.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceGetImageTypeIdGuidPtr ()` - Returns a GUID that indicates the image type.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceGetLowestSupportedVersion ()` - Returns the lowest supported version from the currently stored firmware
  image for the device. The new firmware image version must be greater than or equal to this value.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceGetSize ()` - Returns the size, in bytes, of the firmware image currently stored on the device.

  The NULL implementation always returns `EFI_SUCCESS` unless `Size` is NULL in which case `EFI_INVALID_PARAMETER`
  is returned.

* `FmpDeviceGetVersion ()` - Returns the `Version` of the firmware image currently stored on the device.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceGetVersionString ()` - Returns the Null-terminated Unicode string of the current firmware image version.

  The NULL implementation always returns `EFI_UNSUPPORTED` unless `VersionString` is NULL in which case
  `EFI_INVALID_PARAMETER` is returned.

* `FmpDeviceLock ()` - Locks the firmware device, thereby preventing any future firmware updates (before a reset).

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceSetContext ()` - Sets the device context when the device is managed by a UEFI Driver Model driver.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceSetImage ()` - Updates the device with a new firmware image. This function is retained in the library
  class for backward compatibility and is implemented as a wrapper around `FmpDeviceSetImageWithStatus ()`. `FmpDxe`
  does not call this function directly.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `FmpDeviceSetImageWithStatus ()` - Updates a firmware image with a new firmware image.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `RegisterFmpInstaller ()` - Provides a function to install the Firmware Management Protocol instance onto a device
  handle when the device is managed by a driver that follows the UEFI Driver Model.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

* `RegisterFmpUninstaller ()` - Provides a function to uninstall the Firmware Management Protocol instance from a
  device handle when the device is managed by a driver that follows the UEFI Driver Model.

  The NULL implementation always returns `EFI_UNSUPPORTED`.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

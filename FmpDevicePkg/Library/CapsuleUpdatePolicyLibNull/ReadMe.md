# Capsule Update Policy NULL Library Instance

## About

This is a NULL instance of the `CapsuleUpdatePolicy` library class. This instance is provided for building when an
actual library instance is not needed or the values returned by this instance are sufficient for a platform.

## API Overview

* `CheckSystemEnvironment ()` - Determines if the system environment state supports a capsule update.

  The NULL implementation will always return `EFI_SUCCESS`.

* `CheckSystemPower ()` - Determine if the system power state supports a capsule update.

  The NULL implementation will always return `EFI_SUCCESS`.

* `CheckSystemThermal ()` - Determines if the system thermal state supports a capsule update.

  The NULL implementation will always return `EFI_SUCCESS`.

* `IsLockFmpDeviceAtLockEventGuidRequired ()` - Determines if the FMP device should be locked when the event specified
  by `PcdFmpDeviceLockEventGuid` is signaled.

  The NULL implementation will always return `TRUE`.

* `IsLowestSupportedVersionCheckRequired ()` - Determines if the Lowest Supported Version checks should be performed.

  The NULL implementation will always return `TRUE`.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

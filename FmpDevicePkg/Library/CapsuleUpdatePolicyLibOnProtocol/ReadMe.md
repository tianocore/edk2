# Capsule Update Policy On Protocol Library Instance

## About

This library class instance of `CapsuleUpdatePolicyLib` implements the library API using the values returned from
an installed instance of `EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL`. If the protocol is not found, `CheckSystemPower ()`,
`CheckSystemThermal ()`, and `CheckSystemEnvironment ()` assume the platform state supports a capsule update, while
`IsLowestSupportedVersionCheckRequired ()` and `IsLockFmpDeviceAtLockEventGuidRequired ()` default to `TRUE` so the
lowest supported version check and FMP device locking are still performed.

## API Overview

* `CheckSystemEnvironment ()` - Determines if the system environment state supports a capsule update.
* `CheckSystemPower ()` - Determine if the system power state supports a capsule update.
* `CheckSystemThermal ()` - Determines if the system thermal state supports a capsule update.
* `IsLockFmpDeviceAtLockEventGuidRequired ()` - Determines if the FMP device should be locked when the event specified
  by `PcdFmpDeviceLockEventGuid` is signaled.
* `IsLowestSupportedVersionCheckRequired ()` - Determines if the Lowest Supported Version checks should be performed.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

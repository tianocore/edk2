# FMP Dependency Device NULL Library Instance

## About

This is a NULL instance of the `FmpDependencyDeviceLib` library class. This instance is provided for building when
an actual library instance is not needed.

## API Overview

* `GetFmpDependency ()` - Gets the dependency expression for the device represented by the library class instance.

  The NULL implementation always returns `NULL` a dependency expression was not retrieved.

* `SaveFmpDependency ()` - Saves the dependency expression to the device represented by the library class instance.

  The NULL implementation always returns `EFI_UNSUPPORTED` indicating a dependency expression cannot be saved.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

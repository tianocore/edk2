# FMP Dependency Check NULL Library Instance

## About

This is a NULL instance of the `FmpDependencyCheckLib` library class. This instance is provided for building when an
actual library instance is not needed. A version of the library that contains actual dependency checking
implementation is also available in this package, see `FmpDependencyCheckLib`.

## API Overview

* `CheckFmpDependency ()` - Checks a given set of firmware image information such as the image type ID and version
  against a given dependency expression and returns whether the dependency expression is satisfied.

  The NULL implementation will always return `TRUE` indicating the all dependencies are satisfied.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

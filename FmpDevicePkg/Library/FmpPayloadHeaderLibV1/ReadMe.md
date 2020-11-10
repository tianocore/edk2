# FMP Payload Header Library

## About

This library provides services to retrieve values from Version 1 of a capsule's FMP Payload Header. The FMP Payload
Header structure is not defined in the library class. Instead, services are provided to retrieve information from the
FMP Payload Header. If information is added to the FMP Payload Header, then new services may be added to this library
class to retrieve the new information.

## API Overview

* `GetFmpPayloadHeaderSize ()` - Returns the FMP Payload Header size in bytes.
* `GetFmpPayloadHeaderVersion ()` - Returns the version described in the FMP Payload Header.
* `GetFmpPayloadHeaderLowestSupportedVersion ()` - Returns the lowest supported version described in the FMP Payload
  Header.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

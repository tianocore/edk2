/** @file
  Defines last attempt status code ranges within the UEFI Specification
  defined unsuccessful vendor range.

  Copyright (c) Microsoft Corporation.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LAST_ATTEMPT_STATUS_H__
#define __LAST_ATTEMPT_STATUS_H__

///
/// Last Attempt Status Unsuccessful Vendor Range Map
///
/// Update this map any time new ranges are added. Pre-existing range definitions cannot be modified
/// to keep status code definitions consistent over time.
///
/// START     | END       | Usage
/// ------------------------------------------------------------------|
/// 0x1000    | 0x17FF    | FmpDevicePkg                              |
///    0x1000 |    0x107F | FmpDxe driver                             |
///    0x1080 |    0x109F | FmpDependencyLib                          |
///    0x10A0 |    0x10BF | FmpDependencyCheckLib                     |
///    0x10C0 |    0x17FF | Unused. Available for future expansion.   |
/// 0x1800    | 0x1FFF    | FmpDeviceLib instances implementation     |
/// 0x2000    | 0x3FFF    | Unused. Available for future expansion.   |
///

///
/// The minimum value of the FMP reserved range.
///
#define LAST_ATTEMPT_STATUS_FMP_RESERVED_MIN_ERROR_CODE_VALUE  0x1000

///
/// The maximum value of the FMP reserved range.
///
#define LAST_ATTEMPT_STATUS_FMP_RESERVED_MAX_ERROR_CODE_VALUE  0x1FFF

///
/// The minimum value allowed for FmpDxe driver-specific errors.
///
#define LAST_ATTEMPT_STATUS_DRIVER_MIN_ERROR_CODE_VALUE  0x1000

///
/// The maximum value allowed for FmpDxe driver-specific errors.
///
#define LAST_ATTEMPT_STATUS_DRIVER_MAX_ERROR_CODE_VALUE  0x107F

///
/// The minimum value allowed for FmpDependencyLib related errors.
///
#define LAST_ATTEMPT_STATUS_FMP_DEPENDENCY_LIB_MIN_ERROR_CODE_VALUE  0x1080

///
/// The maximum value allowed for FmpDependencyLib related errors.
///
#define LAST_ATTEMPT_STATUS_FMP_DEPENDENCY_LIB_MAX_ERROR_CODE_VALUE  0x109F

///
/// The minimum value allowed for FmpDependencyCheckLib related errors.
///
#define LAST_ATTEMPT_STATUS_FMP_DEPENDENCY_CHECK_LIB_MIN_ERROR_CODE_VALUE  0x10A0

///
/// The maximum value allowed for FmpDependencyCheckLib related errors.
///
#define LAST_ATTEMPT_STATUS_FMP_DEPENDENCY_CHECK_LIB_MAX_ERROR_CODE_VALUE  0x10BF

///
/// The minimum value allowed for FMP device library errors.
///
#define LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MIN_ERROR_CODE_VALUE  0x1800

///
/// The maximum value allowed for FMP device library errors.
///
#define LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MAX_ERROR_CODE_VALUE  0x1FFF

#endif

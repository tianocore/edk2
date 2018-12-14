/** @file
  Define the LockBox GUID for list of storage devices need to be initialized in
  S3.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __S3_STORAGE_DEVICE_INIT_LIST_H__
#define __S3_STORAGE_DEVICE_INIT_LIST_H__

#define S3_STORAGE_DEVICE_INIT_LIST \
  { \
    0x310e9b8c, 0xcf90, 0x421e, { 0x8e, 0x9b, 0x9e, 0xef, 0xb6, 0x17, 0xc8, 0xef } \
  }

//
// The LockBox will store a DevicePath structure that contains one or more
// DevicePath instances. Each instance denotes a storage device that needs to
// get initialized during the S3 resume.
//
// For example, if there is only one storage device stored in the list, the
// content of this LockBox will be:
//
// +-------------------------------------------------------+
// |                 DevPath Instance #1                   |
// | (Terminated by an End of Hardware Device Path node    |
// |  with an End Entire Device Path sub-type)             |
// +-------------------------------------------------------+
//
// If there are n (n > 1) storage devices in the list, the content of this
// LockBox will be:
//
// +-------------------------------------------------------+
// |                 DevPath Instance #1                   |
// | (Terminated by an End of Hardware Device Path node    |
// |  with an End This Instance of a Device Path sub-type) |
// +-------------------------------------------------------+
// |                 DevPath Instance #2                   |
// | (Terminated by an End of Hardware Device Path node    |
// |  with an End This Instance of a Device Path sub-type) |
// +-------------------------------------------------------+
// |                        ...                            |
// +-------------------------------------------------------+
// |                 DevPath Instance #n                   |
// | (Terminated by an End of Hardware Device Path node    |
// |  with an End Entire Device Path sub-type)             |
// +-------------------------------------------------------+
//
// The attribute of the LockBox should be set to
// 'LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY'.
//
extern EFI_GUID  gS3StorageDeviceInitListGuid;

#endif  // __S3_STORAGE_DEVICE_INIT_LIST_H__

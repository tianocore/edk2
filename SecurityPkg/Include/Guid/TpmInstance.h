/** @file
  TPM instance guid, used for PcdTpmInstanceGuid.

Copyright (c) 2013, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TPM_INSTANCE_GUID_H__
#define __TPM_INSTANCE_GUID_H__

#define TPM_DEVICE_INTERFACE_NONE  \
  { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }

#define TPM_DEVICE_INTERFACE_TPM12  \
  { 0x8b01e5b6, 0x4f19, 0x46e8, { 0xab, 0x93, 0x1c, 0x53, 0x67, 0x1b, 0x90, 0xcc } }

#define TPM_DEVICE_INTERFACE_TPM20_DTPM  \
  { 0x286bf25a, 0xc2c3, 0x408c, { 0xb3, 0xb4, 0x25, 0xe6, 0x75, 0x8b, 0x73, 0x17 } }

extern EFI_GUID  gEfiTpmDeviceInstanceNoneGuid;
extern EFI_GUID  gEfiTpmDeviceInstanceTpm12Guid;
extern EFI_GUID  gEfiTpmDeviceInstanceTpm20DtpmGuid;

#define TPM_DEVICE_SELECTED_GUID  \
  { 0x7f4158d3, 0x74d, 0x456d, { 0x8c, 0xb2, 0x1, 0xf9, 0xc8, 0xf7, 0x9d, 0xaa } }

extern EFI_GUID  gEfiTpmDeviceSelectedGuid;

#endif

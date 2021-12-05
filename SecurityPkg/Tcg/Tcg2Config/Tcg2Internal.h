/** @file
  The internal header file defines the common structures for PEI and DXE modules.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TCG2_INTERNAL_H__
#define __TCG2_INTERNAL_H__

#define EFI_TCG2_EVENT_LOG_FORMAT_DEFAULT  EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2
#define EFI_TCG2_EVENT_LOG_FORMAT_ALL      (EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 | EFI_TCG2_EVENT_LOG_FORMAT_TCG_2)

#define TPM_INSTANCE_ID_LIST  { \
  {TPM_DEVICE_INTERFACE_NONE,           TPM_DEVICE_NULL},      \
  {TPM_DEVICE_INTERFACE_TPM12,          TPM_DEVICE_1_2},       \
  {TPM_DEVICE_INTERFACE_TPM20_DTPM,     TPM_DEVICE_2_0_DTPM},  \
}

typedef struct {
  GUID     TpmInstanceGuid;
  UINT8    TpmDevice;
} TPM_INSTANCE_ID;

#endif

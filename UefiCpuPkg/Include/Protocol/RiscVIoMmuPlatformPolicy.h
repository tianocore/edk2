/** @file
  The definition for the RISC-V IOMMU platform policy protocol.

  Copyright (c) 2026, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RISC_V_IO_MMU_PLATFORM_POLICY_H_
#define _RISC_V_IO_MMU_PLATFORM_POLICY_H_

#include <PiDxe.h>

extern EFI_GUID gEdkiiRiscVIoMmuPlatformPolicyProtocolGuid;

typedef struct _EDKII_RISC_V_IO_MMU_PLATFORM_POLICY_PROTOCOL  EDKII_RISC_V_IO_MMU_PLATFORM_POLICY_PROTOCOL;

#define EDKII_RISC_V_IO_MMU_PLATFORM_POLICY_PROTOCOL_REVISION 0x00000000

/**
  Get the IOMMU device_id and this IOMMU base address from the device handle.
  This function is required for non PCI device handles.

  @param[in]  This                  The protocol instance pointer.
  @param[in]  DeviceHandle          Device Identifier in UEFI.
  @param[out] IoMmuDeviceId         IoMmuDeviceId for identifying the device on the IOMMU.
  @param[out] IoMmuBaseAddress      IoMmuBaseAddress for identifying the upstream IOMMU.

  @retval EFI_SUCCESS           The IoMmuDeviceId and IoMmuBaseAddress are returned.
  @retval EFI_INVALID_PARAMETER DeviceHandle is not a valid handler.
  @retval EFI_INVALID_PARAMETER IoMmuDeviceId or IoMmuBaseAddress are NULL.
  @retval EFI_NOT_FOUND         The device_id information is NOT found.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_RISC_V_IO_MMU_PLATFORM_POLICY_GET_DEVICE_ID) (
  IN  EDKII_RISC_V_IO_MMU_PLATFORM_POLICY_PROTOCOL  *This,
  IN  EFI_HANDLE                                    DeviceHandle,
  OUT UINT32                                        *IoMmuDeviceId,
  OUT EFI_PHYSICAL_ADDRESS                          *IoMmuBaseAddress
  );

struct _EDKII_RISC_V_IO_MMU_PLATFORM_POLICY_PROTOCOL {
  UINT64                                             Revision;
  EDKII_RISC_V_IO_MMU_PLATFORM_POLICY_GET_DEVICE_ID  GetDeviceId;
};

#endif

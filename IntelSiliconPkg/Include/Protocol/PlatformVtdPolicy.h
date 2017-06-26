/** @file
  The definition for platform VTD policy.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_VTD_POLICY_PROTOCOL_H__
#define __PLATFORM_VTD_POLICY_PROTOCOL_H__

#include <IndustryStandard/Vtd.h>

#define EDKII_PLATFORM_VTD_POLICY_PROTOCOL_GUID \
    { \
      0x3d17e448, 0x466, 0x4e20, { 0x99, 0x9f, 0xb2, 0xe1, 0x34, 0x88, 0xee, 0x22 } \
    }

typedef struct _EDKII_PLATFORM_VTD_POLICY_PROTOCOL  EDKII_PLATFORM_VTD_POLICY_PROTOCOL;

#define EDKII_PLATFORM_VTD_POLICY_PROTOCOL_REVISION 0x00010000

typedef struct {
  UINT16                                   Segment;
  VTD_SOURCE_ID                            SourceId;
} EDKII_PLATFORM_VTD_DEVICE_INFO;

/**
  Get the VTD SourceId from the device handler.
  This function is required for non PCI device handler.

  Pseudo-algo in Intel VTd driver:
    Status = PlatformGetVTdDeviceId ();
    if (EFI_ERROR(Status)) {
      if (DeviceHandle is PCI) {
        Get SourceId from Bus/Device/Function
      } else {
        return EFI_UNSUPPORTED
      }
    }
    Get VTd engine by Segment/Bus/Device/Function.

  @param[in]  This                  The protocol instance pointer.
  @param[in]  DeviceHandle          Device Identifier in UEFI.
  @param[out] DeviceInfo            DeviceInfo for indentify the VTd engine in ACPI Table
                                    and the VTd page entry.

  @retval EFI_SUCCESS           The VtdIndex and SourceId are returned.
  @retval EFI_INVALID_PARAMETER DeviceHandle is not a valid handler.
  @retval EFI_INVALID_PARAMETER DeviceInfo is NULL.
  @retval EFI_NOT_FOUND         The Segment or SourceId information is NOT found.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PLATFORM_VTD_POLICY_GET_DEVICE_ID) (
  IN  EDKII_PLATFORM_VTD_POLICY_PROTOCOL       *This,
  IN  EFI_HANDLE                               DeviceHandle,
  OUT EDKII_PLATFORM_VTD_DEVICE_INFO           *DeviceInfo
  );

/**
  Get a list of the exception devices.

  The VTd driver should always set ALLOW for the device in this list.

  @param[in]  This                  The protocol instance pointer.
  @param[out] DeviceInfoCount       The count of the list of DeviceInfo.
  @param[out] DeviceInfo            A callee allocated buffer to hold a list of DeviceInfo.

  @retval EFI_SUCCESS           The DeviceInfoCount and DeviceInfo are returned.
  @retval EFI_INVALID_PARAMETER DeviceInfoCount is NULL, or DeviceInfo is NULL.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PLATFORM_VTD_POLICY_GET_EXCEPTION_DEVICE_LIST) (
  IN  EDKII_PLATFORM_VTD_POLICY_PROTOCOL       *This,
  OUT UINTN                                    *DeviceInfoCount,
  OUT EDKII_PLATFORM_VTD_DEVICE_INFO           **DeviceInfo
  );

struct _EDKII_PLATFORM_VTD_POLICY_PROTOCOL {
  UINT64                                               Revision;
  EDKII_PLATFORM_VTD_POLICY_GET_DEVICE_ID              GetDeviceId;
  EDKII_PLATFORM_VTD_POLICY_GET_EXCEPTION_DEVICE_LIST  GetExceptionDeviceList;
};

extern EFI_GUID gEdkiiPlatformVTdPolicyProtocolGuid;

#endif


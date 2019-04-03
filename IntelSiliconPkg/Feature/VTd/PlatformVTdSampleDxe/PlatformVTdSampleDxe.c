/** @file
  Platform VTd Sample driver.

  Note: This module should only be used for dev/debug purposes.
        It MUST never be used for production builds.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/Vtd.h>
#include <Protocol/PlatformVtdPolicy.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePath.h>

#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/DmaRemappingReportingTable.h>

typedef struct {
  ACPI_EXTENDED_HID_DEVICE_PATH      I2cController;
  UINT8                              HidStr[8];
  UINT8                              UidStr[1];
  UINT8                              CidStr[8];
} PLATFORM_I2C_CONTROLLER_DEVICE_PATH;

typedef struct {
  ACPI_EXTENDED_HID_DEVICE_PATH      I2cDevice;
  UINT8                              HidStr[13];
  UINT8                              UidStr[1];
  UINT8                              CidStr[13];
} PLATFORM_I2C_DEVICE_DEVICE_PATH;

typedef struct {
  PLATFORM_I2C_CONTROLLER_DEVICE_PATH      I2cController;
  PLATFORM_I2C_DEVICE_DEVICE_PATH          I2cDevice;
  EFI_DEVICE_PATH_PROTOCOL                 End;
} PLATFORM_I2C_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciDevice;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PLATFORM_PCI_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciBridge;
  PCI_DEVICE_PATH           PciDevice;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} PLATFORM_PCI_BRIDGE_DEVICE_PATH;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT16                    Segment;
  VTD_SOURCE_ID             SourceId;
} PLATFORM_ACPI_DEVICE_MAPPING;

#define PLATFORM_PCI_ROOT_BRIDGE \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) \
      }, \
    }, \
    EISA_PNP_ID (0x0A03), \
    0 \
  }

#define PLATFORM_END_ENTIRE \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { END_DEVICE_PATH_LENGTH, 0 } \
  }

#define PLATFORM_PCI(Device, Function) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      } \
    }, \
    (Function), \
    (Device) \
  }

#define PLATFORM_I2C(Hid, Uid, Cid, HidStr, UidStr, CidStr) \
  { \
    { \
      { \
        ACPI_DEVICE_PATH, \
        ACPI_EXTENDED_DP, \
        {sizeof(ACPI_EXTENDED_HID_DEVICE_PATH) + sizeof(HidStr) + sizeof(UidStr) + sizeof(CidStr), 0} \
      }, \
      Hid, \
      Uid, \
      Cid \
    }, \
    HidStr, \
    UidStr, \
    CidStr \
  }

PLATFORM_I2C_DEVICE_PATH mPlatformI2CDevicePath = {
  PLATFORM_I2C(0, 2, 0, "INT33C3", "", "INT33C3"),
  PLATFORM_I2C(0, 1, 0, "I2C01\\TPANEL", "", "I2C01\\TPANEL"),
  PLATFORM_END_ENTIRE
};

PLATFORM_ACPI_DEVICE_MAPPING  mAcpiDeviceMapping[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&mPlatformI2CDevicePath,
    0x0,                 // Segment
    {{0x01, 0x15, 0x00}} // Function, Device, Bus
  }
};

PLATFORM_PCI_BRIDGE_DEVICE_PATH mPlatformPciBridgeDevicePath = {
  PLATFORM_PCI_ROOT_BRIDGE,
  PLATFORM_PCI(0x1C, 1),
  PLATFORM_PCI(0, 0),
  PLATFORM_END_ENTIRE
};

#pragma pack(1)

typedef struct {
  EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO     ExceptionDeviceInfo;
  EDKII_PLATFORM_VTD_DEVICE_SCOPE              DeviceScope;
  EFI_ACPI_DMAR_PCI_PATH                       PciBridge;
  EFI_ACPI_DMAR_PCI_PATH                       PciDevice;
} PLATFORM_EXCEPTION_DEVICE_SCOPE_STRUCT;

typedef struct {
  EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO     ExceptionDeviceInfo;
  EDKII_PLATFORM_VTD_PCI_DEVICE_ID             PciDeviceId;
} PLATFORM_EXCEPTION_PCI_DEVICE_ID_STRUCT;

#pragma pack()

PLATFORM_EXCEPTION_DEVICE_SCOPE_STRUCT  mExceptionDeviceScopeList[] = {
  {
    {
      EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO_TYPE_DEVICE_SCOPE,
      sizeof(PLATFORM_EXCEPTION_DEVICE_SCOPE_STRUCT)
    },  // ExceptionDeviceInfo
    {
      0,                                                    // SegmentNumber
      {
        EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT,      // Type
        sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER) +
          2 * sizeof(EFI_ACPI_DMAR_PCI_PATH),               // Length
        0,                                                  // Reserved2
        0,                                                  // EnumerationId
        0,                                                  // StartBusNumber
      },
    },                                                      // DeviceScope
    { 0x1C, 1 },                                            // PciBridge
    { 0x0,  0 },                                            // PciDevice
  },
};

PLATFORM_EXCEPTION_PCI_DEVICE_ID_STRUCT  mExceptionPciDeviceIdList[] = {
  {
    {
      EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO_TYPE_PCI_DEVICE_ID,
      sizeof(PLATFORM_EXCEPTION_PCI_DEVICE_ID_STRUCT)
    },  // ExceptionDeviceInfo
    {
      0x8086,                                               // VendorId
      0x9D2F,                                               // DeviceId
      0x21,                                                 // RevisionId
      0x8086,                                               // SubsystemVendorId
      0x7270,                                               // SubsystemDeviceId
    },
  },
};

/**
  Compares 2 device path.

  @param[in] DevicePath1  A device path with EndDevicePath node.
  @param[in] DevicePath2  A device path with EndDevicePath node.

  @retval TRUE   2 device path are identical.
  @retval FALSE  2 device path are not identical.
**/
BOOLEAN
CompareDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath1,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath2
  )
{
  UINTN  Size1;
  UINTN  Size2;

  Size1 = GetDevicePathSize (DevicePath1);
  Size2 = GetDevicePathSize (DevicePath2);
  if (Size1 != Size2) {
    return FALSE;
  }
  if (CompareMem (DevicePath1, DevicePath2, Size1) != 0) {
    return FALSE;
  }
  return TRUE;
}

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
EFI_STATUS
EFIAPI
PlatformVTdGetDeviceId (
  IN  EDKII_PLATFORM_VTD_POLICY_PROTOCOL       *This,
  IN  EFI_HANDLE                               DeviceHandle,
  OUT EDKII_PLATFORM_VTD_DEVICE_INFO           *DeviceInfo
  )
{
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINTN                     Seg;
  UINTN                     Bus;
  UINTN                     Dev;
  UINTN                     Func;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     Index;

  DEBUG ((DEBUG_VERBOSE, "PlatformVTdGetDeviceId\n"));

  if (DeviceInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DeviceHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Handle PCI device
  //
  Status = gBS->HandleProtocol (DeviceHandle, &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
  if (!EFI_ERROR(Status)) {
    Status = PciIo->GetLocation (PciIo, &Seg, &Bus, &Dev, &Func);
    if (EFI_ERROR(Status)) {
      return EFI_UNSUPPORTED;
    }
    DeviceInfo->Segment = (UINT16)Seg;
    DeviceInfo->SourceId.Bits.Bus = (UINT8)Bus;
    DeviceInfo->SourceId.Bits.Device = (UINT8)Dev;
    DeviceInfo->SourceId.Bits.Function = (UINT8)Func;

    return EFI_SUCCESS;
  }

  //
  // Handle ACPI device
  //
  Status = gBS->HandleProtocol (DeviceHandle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
  if (!EFI_ERROR(Status)) {
    for (Index = 0; Index < ARRAY_SIZE(mAcpiDeviceMapping); Index++) {
      if (CompareDevicePath (mAcpiDeviceMapping[Index].DevicePath, DevicePath)) {
        DeviceInfo->Segment = mAcpiDeviceMapping[Index].Segment;
        DeviceInfo->SourceId = mAcpiDeviceMapping[Index].SourceId;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get a list of the exception devices.

  The VTd driver should always set ALLOW for the device in this list.

  @param[in]  This                  The protocol instance pointer.
  @param[out] DeviceInfoCount       The count of the list of DeviceInfo.
  @param[out] DeviceInfo            A callee allocated buffer to hold a list of DeviceInfo.
                                    Each DeviceInfo pointer points to EDKII_PLATFORM_VTD_EXCEPTION_DEVICE_INFO.

  @retval EFI_SUCCESS           The DeviceInfoCount and DeviceInfo are returned.
  @retval EFI_INVALID_PARAMETER DeviceInfoCount is NULL, or DeviceInfo is NULL.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
PlatformVTdGetExceptionDeviceList (
  IN  EDKII_PLATFORM_VTD_POLICY_PROTOCOL       *This,
  OUT UINTN                                    *DeviceInfoCount,
  OUT VOID                                     **DeviceInfo
  )
{
  DEBUG ((DEBUG_VERBOSE, "PlatformVTdGetExceptionDeviceList\n"));

  if (DeviceInfoCount == NULL || DeviceInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Sample codes for device scope based exception list.
  // Uncomment to take affect and comment the sample codes for PCI vendor id
  // based exception list.
  //
  /*
  *DeviceInfo = AllocateZeroPool (sizeof(mExceptionDeviceScopeList));
  if (*DeviceInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (*DeviceInfo, mExceptionDeviceScopeList, sizeof(mExceptionDeviceScopeList));

  *DeviceInfoCount = ARRAY_SIZE(mExceptionDeviceScopeList);
  */

  //
  // Sample codes for PCI vendor id based exception list.
  // Uncomment to take affect and comment the sample codes for device scope
  // based exception list.
  //
  /*
  *DeviceInfo = AllocateZeroPool (sizeof(mExceptionPciDeviceIdList));
  if (*DeviceInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (*DeviceInfo, mExceptionPciDeviceIdList, sizeof(mExceptionPciDeviceIdList));

  *DeviceInfoCount = ARRAY_SIZE(mExceptionPciDeviceIdList);
  */
  return EFI_UNSUPPORTED;
}

EDKII_PLATFORM_VTD_POLICY_PROTOCOL  mPlatformVTdSample = {
  EDKII_PLATFORM_VTD_POLICY_PROTOCOL_REVISION,
  PlatformVTdGetDeviceId,
  PlatformVTdGetExceptionDeviceList,
};

/**
  Platform VTd sample driver.

  @param[in]  ImageHandle  ImageHandle of the loaded driver
  @param[in]  SystemTable  Pointer to the System Table

  @retval  EFI_SUCCESS           The Protocol is installed.
  @retval  EFI_OUT_OF_RESOURCES  Not enough resources available to initialize driver.
  @retval  EFI_DEVICE_ERROR      A device error occurred attempting to initialize the driver.

**/
EFI_STATUS
EFIAPI
PlatformVTdSampleInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiPlatformVTdPolicyProtocolGuid, &mPlatformVTdSample,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

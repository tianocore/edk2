/** @file
  The SioBusDxe driver is used to create child devices on the ISA bus and
  installs the Super I/O protocols on them.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SioBusDxe.h"

//
// Super I/O Protocol interfaces
//
EFI_SIO_PROTOCOL mSioInterface = {
  SioRegisterAccess,
  SioGetResources,
  SioSetResources,
  SioPossibleResources,
  SioModify
};

//
// COM 1 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
SIO_RESOURCES_IO mCom1Resources = {
  { { ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR }, 0x3F8, 8 },
  { ACPI_END_TAG_DESCRIPTOR,                    0        }
};

//
// COM 2 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
SIO_RESOURCES_IO mCom2Resources = {
  { { ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR }, 0x2F8, 8 },
  { ACPI_END_TAG_DESCRIPTOR,                    0        }
};

//
// PS/2 Keyboard Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
SIO_RESOURCES_IO mPs2KeyboardDeviceResources = {
  { { ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR }, 0x60, 5 },
  { ACPI_END_TAG_DESCRIPTOR,                    0       }
};

//
// Table of SIO Controllers
//
GLOBAL_REMOVE_IF_UNREFERENCED
SIO_DEVICE_INFO mDevicesInfo[] = {
  {
    EISA_PNP_ID (0x501),
    0,
    { (ACPI_SMALL_RESOURCE_HEADER *) &mCom1Resources }
  },  // COM 1 UART Controller
  {
    EISA_PNP_ID (0x501),
    1,
    { (ACPI_SMALL_RESOURCE_HEADER *) &mCom2Resources }
  },  // COM 2 UART Controller
  {
    EISA_PNP_ID(0x303),
    0,
    { (ACPI_SMALL_RESOURCE_HEADER *) &mPs2KeyboardDeviceResources }
  }   // PS/2 Keyboard Controller
};

//
// ACPI Device Path Node template
//
GLOBAL_REMOVE_IF_UNREFERENCED
ACPI_HID_DEVICE_PATH mAcpiDeviceNodeTemplate = {
  {        // Header
    ACPI_DEVICE_PATH,
    ACPI_DP,
    {
      (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
      (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
    }
  },
  0x0,     // HID
  0x0      // UID
};


/**
  Provides a low level access to the registers for the Super I/O.

  @param[in]     This           Indicates a pointer to the calling context.
  @param[in]     Write          Specifies the type of the register operation.
                                If this parameter is TRUE, Value is interpreted
                                as an input parameter and the operation is a
                                register write. If this parameter is FALSE,
                                Value is interpreted as an output parameter and
                                the operation is a register read.
  @param[in]     ExitCfgMode    Exit Configuration Mode Indicator. If this
                                parameter is set to TRUE, the Super I/O driver
                                will turn off configuration mode of the Super
                                I/O prior to returning from this function. If
                                this parameter is set to FALSE, the Super I/O
                                driver will leave Super I/O in the
                                configuration mode. The Super I/O driver must
                                track the current state of the Super I/O and
                                enable the configuration mode of Super I/O if
                                necessary prior to register access.
  @param[in]     Register       Register number.
  @param[in,out] Value          If Write is TRUE, Value is a pointer to the
                                buffer containing the byte of data to be
                                written to the Super I/O register. If Write is
                                FALSE, Value is a pointer to the destination
                                buffer for the byte of data to be read from the
                                Super I/O register.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER The Value is NULL.
  @retval EFI_INVALID_PARAMETER Invalid Register number.

**/
EFI_STATUS
EFIAPI
SioRegisterAccess (
  IN CONST EFI_SIO_PROTOCOL    *This,
  IN       BOOLEAN             Write,
  IN       BOOLEAN             ExitCfgMode,
  IN       UINT8               Register,
  IN OUT   UINT8               *Value
  )
{
  return EFI_SUCCESS;
}

/**
  Provides an interface to get a list of the current resources consumed by the
  device in the ACPI Resource Descriptor format.

  GetResources() returns a list of resources currently consumed by the device.
  The ResourceList is a pointer to the buffer containing resource descriptors
  for the device. The descriptors are in the format of Small or Large ACPI
  resource descriptor as defined by ACPI specification (2.0 & 3.0). The buffer
  of resource descriptors is terminated with the 'End tag' resource descriptor.

  @param[in]  This              Indicates a pointer to the calling context.
  @param[out] ResourceList      A pointer to an ACPI resource descriptor list
                                that defines the current resources used by the
                                device.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER ResourceList is NULL.

**/
EFI_STATUS
EFIAPI
SioGetResources (
  IN CONST EFI_SIO_PROTOCOL            *This,
  OUT      ACPI_RESOURCE_HEADER_PTR    *ResourceList
  )
{
  SIO_DEV    *SioDevice;

  if (ResourceList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SioDevice = SIO_DEV_FROM_SIO (This);
  if (SioDevice->DeviceIndex < ARRAY_SIZE (mDevicesInfo)) {
    *ResourceList = mDevicesInfo[SioDevice->DeviceIndex].Resources;
  }

  return EFI_SUCCESS;
}

/**
  Sets the resources for the device.

  @param[in] This               Indicates a pointer to the calling context.
  @param[in] ResourceList       Pointer to the ACPI resource descriptor list.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER ResourceList is invalid.
  @retval EFI_ACCESS_DENIED     Some of the resources in ResourceList are in
                                use.

**/
EFI_STATUS
EFIAPI
SioSetResources (
  IN CONST EFI_SIO_PROTOCOL            *This,
  IN       ACPI_RESOURCE_HEADER_PTR    ResourceList
  )
{
  return EFI_SUCCESS;
}

/**
  Provides a collection of resource descriptor lists. Each resource descriptor
  list in the collection defines a combination of resources that can
  potentially be used by the device.

  @param[in]  This                  Indicates a pointer to the calling context.
  @param[out] ResourceCollection    Collection of the resource descriptor
                                    lists.

  @retval EFI_SUCCESS               The operation completed successfully.
  @retval EFI_INVALID_PARAMETER     ResourceCollection is NULL.

**/
EFI_STATUS
EFIAPI
SioPossibleResources (
  IN CONST EFI_SIO_PROTOCOL            *This,
  OUT      ACPI_RESOURCE_HEADER_PTR    *ResourceCollection
  )
{
  return EFI_SUCCESS;
}

/**
  Provides an interface for a table based programming of the Super I/O
  registers.

  The Modify() function provides an interface for table based programming of
  the Super I/O registers. This function can be used to perform programming of
  multiple Super I/O registers with a single function call. For each table
  entry, the Register is read, its content is bitwise ANDed with AndMask, and
  then ORed with OrMask before being written back to the Register. The Super
  I/O driver must track the current state of the Super I/O and enable the
  configuration mode of Super I/O if necessary prior to table processing. Once
  the table is processed, the Super I/O device has to be returned to the
  original state.

  @param[in] This               Indicates a pointer to the calling context.
  @param[in] Command            A pointer to an array of NumberOfCommands
                                EFI_SIO_REGISTER_MODIFY structures. Each
                                structure specifies a single Super I/O register
                                modify operation.
  @param[in] NumberOfCommands   Number of elements in the Command array.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER Command is NULL.

**/
EFI_STATUS
EFIAPI
SioModify (
  IN CONST EFI_SIO_PROTOCOL           *This,
  IN CONST EFI_SIO_REGISTER_MODIFY    *Command,
  IN       UINTN                      NumberOfCommands
  )
{
  return EFI_SUCCESS;
}

/**
  Create the child device with a given device index.

  @param[in] This              The EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in] Controller        The handle of ISA bus controller.
  @param[in] PciIo             The pointer to the PCI protocol.
  @param[in] ParentDevicePath  Device path of the ISA bus controller.
  @param[in] DeviceIndex       Index of the device supported by this driver.

  @retval EFI_SUCCESS          The child device has been created successfully.
  @retval Others               Error occurred during the child device creation.

**/
EFI_STATUS
SioCreateChildDevice (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_PCI_IO_PROTOCOL          *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath,
  IN UINT32                       DeviceIndex
  )
{
  EFI_STATUS    Status;
  SIO_DEV       *SioDevice;

  //
  // Initialize the SIO_DEV structure
  //
  SioDevice = AllocateZeroPool (sizeof (SIO_DEV));
  if (SioDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SioDevice->Signature  = SIO_DEV_SIGNATURE;
  SioDevice->Handle     = NULL;
  SioDevice->PciIo      = PciIo;

  //
  // Construct the child device path
  //
  mAcpiDeviceNodeTemplate.HID = mDevicesInfo[DeviceIndex].Hid;
  mAcpiDeviceNodeTemplate.UID = mDevicesInfo[DeviceIndex].Uid;
  SioDevice->DevicePath = AppendDevicePathNode (
                            ParentDevicePath,
                            (EFI_DEVICE_PATH_PROTOCOL *) &mAcpiDeviceNodeTemplate
                            );
  if (SioDevice->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyMem (&SioDevice->Sio, &mSioInterface, sizeof (EFI_SIO_PROTOCOL));
  SioDevice->DeviceIndex = DeviceIndex;

  //
  // Create a child handle and install Device Path and Super I/O protocols
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &SioDevice->Handle,
                  &gEfiDevicePathProtocolGuid,
                  SioDevice->DevicePath,
                  &gEfiSioProtocolGuid,
                  &SioDevice->Sio,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  SioDevice->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           SioDevice->Handle,
           &gEfiDevicePathProtocolGuid,
           SioDevice->DevicePath,
           &gEfiSioProtocolGuid,
           &SioDevice->Sio,
           NULL
           );
  }

Done:
  if (EFI_ERROR (Status)) {
    if (SioDevice->DevicePath != NULL) {
      FreePool (SioDevice->DevicePath);
    }

    FreePool (SioDevice);
  }

  return Status;
}

/**
  Create all the ISA child devices on the ISA bus controller (PCI to ISA
  bridge).

  @param[in] This              The EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in] Controller        The handle of ISA bus controller.
  @param[in] PciIo             The pointer to the PCI protocol.
  @param[in] ParentDevicePath  Device path of the ISA bus controller.

  @retval The number of child device that is successfully created.

**/
UINT32
SioCreateAllChildDevices (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_PCI_IO_PROTOCOL          *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath
  )
{
  UINT32        Index;
  UINT32        ChildDeviceNumber;
  EFI_STATUS    Status;

  ChildDeviceNumber = 0;

  for (Index = 0; Index < ARRAY_SIZE (mDevicesInfo); Index++) {
    Status = SioCreateChildDevice (
               This,
               Controller,
               PciIo,
               ParentDevicePath,
               Index
               );
    if (!EFI_ERROR (Status)) {
      ChildDeviceNumber++;
    }
  }

  return ChildDeviceNumber;
}

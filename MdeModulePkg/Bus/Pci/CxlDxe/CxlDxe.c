/** @file
  CxlDxe driver is used to discover CXL devices
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"

EFI_DRIVER_BINDING_PROTOCOL  gCxlDriverBinding = {
  CxlDriverBindingSupported,
  CxlDriverBindingStart,
  CxlDriverBindingStop,
  0x10,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
CxlDriverBindingSupported(
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status, RegStatus;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT8                 ClassCode[3];
  UINT32                ExtCapOffset, NextExtCapOffset;
  UINT32                PcieExtCapAndDvsecHeader[PCIE_DVSEC_HEADER_MAX];

  // Ensure driver won't be started multiple times
  Status = gBS->OpenProtocol (
             Controller,
             &gEfiCallerIdGuid,
             NULL,
             This->DriverBindingHandle,
             Controller,
             EFI_OPEN_PROTOCOL_TEST_PROTOCOL
             );
  if (!EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return EFI_ALREADY_STARTED;
  }

  // Attempt to Open PCI I/O Protocol
  Status = gBS->OpenProtocol (
             Controller,
             &gEfiPciIoProtocolGuid,
             (VOID**)&PciIo,
             This->DriverBindingHandle,
             Controller,
             EFI_OPEN_PROTOCOL_BY_DRIVER
             );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] PciGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = PciIo->Pci.Read (
             PciIo,
             EfiPciIoWidthUint8,
             PCI_CLASSCODE_OFFSET,
             sizeof(ClassCode),
             ClassCode
             );

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if ((ClassCode[0] != CXL_MEMORY_PROGIF) || (ClassCode[1] != CXL_MEMORY_SUB_CLASS) || (ClassCode[2] != CXL_MEMORY_CLASS)) {
    DEBUG((EFI_D_ERROR, "[%a]: UNSUPPORTED Class [CXL-%08X] \n", __func__, Controller));
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  DEBUG((EFI_D_INFO, "[%a]: SUPPORTED ClassCode0 = %d\n", __func__, ClassCode[0]));
  DEBUG((EFI_D_INFO, "[%a]: SUPPORTED ClassCode1 = %d\n", __func__, ClassCode[1]));
  DEBUG((EFI_D_INFO, "[%a]: SUPPORTED ClassCode2 = %d\n", __func__, ClassCode[2]));

  Status = EFI_UNSUPPORTED;
  NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_OFFSET;
  do {
    ExtCapOffset = NextExtCapOffset;
    DEBUG((EFI_D_INFO, "[%a]: ExtCapOffset = %d \n", __func__, ExtCapOffset));
    RegStatus = PciIo->Pci.Read(
                  PciIo,
                  EfiPciIoWidthUint32,
                  ExtCapOffset,
                  PCIE_DVSEC_HEADER_MAX,
                  PcieExtCapAndDvsecHeader
                  );

    if (EFI_ERROR (RegStatus)) {
      DEBUG((EFI_D_ERROR, "[%a]: Failed to read PCI IO for Ext. capability \n", __func__));
      goto EXIT;
    }

    /* Check whether this is a CXL device */
    if (CXL_IS_DVSEC(PcieExtCapAndDvsecHeader[PCIE_DVSEC_HEADER_1])) {
      Status = EFI_SUCCESS;
      break;
    }

    NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_NEXT(
      PcieExtCapAndDvsecHeader[PCIE_EXT_CAP_HEADER]
    );
  } while (NextExtCapOffset);

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: ****[CXL-%08X] Error: Non CXL Device****\n", __func__, Controller));
  }

EXIT:

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
}

EFI_STATUS
EFIAPI
CxlDriverBindingStop(
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CxlDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_PCI_IO_PROTOCOL          *PciIo;
  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath;
  UINTN                        Seg, Bus, Dev, Func;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] path OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = gBS->OpenProtocol(
             Controller,
             &gEfiPciIoProtocolGuid,
             (VOID**)&PciIo,
             This->DriverBindingHandle,
             Controller,
             EFI_OPEN_PROTOCOL_BY_DRIVER
             );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if (Status == EFI_ALREADY_STARTED) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo EFI_ALREADY_STARTED status = %r\n", __func__, Controller, Status));
  } else {
      Status = PciIo->GetLocation(PciIo, &Seg, &Bus, &Dev, &Func);
      if (Status != EFI_SUCCESS) {
        DEBUG((EFI_D_ERROR, "[%a]: Error PciIo GetLocation status = %r\n", __func__, Status));
        return Status;
    }
      DEBUG((EFI_D_INFO, "[%a]: [CXL-%08X] seg = %d, Bus = %d, Dev = %d, Func = %d\n", __func__, Controller, Seg, Bus, Dev, Func));
  }
  return EFI_SUCCESS;

EXIT:

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  DEBUG((EFI_D_INFO, "[%a]: [CXL-%08X] Completed status = %r\n", __func__, Controller, Status));
  return Status;
}

EFI_STATUS
EFIAPI
CxlDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  DEBUG((EFI_D_INFO, "[%a]: Driver entery point get called\n", __func__));

  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gCxlDriverBinding,
             ImageHandle,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL
             );

  DEBUG((EFI_D_INFO, "[%a]: CXL Installing DriverBinding status = %r\n", __func__, Status));
  ASSERT_EFI_ERROR (Status);
  gRT = SystemTable->RuntimeServices;
  return Status;
}

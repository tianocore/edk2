/** @file
*  PCIe Sata support for the Silicon Image I3132
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "SataSiI3132.h"

#include <IndustryStandard/Acpi10.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>

#define ACPI_SPECFLAG_PREFETCHABLE      0x06

EFI_DRIVER_BINDING_PROTOCOL
gSataSiI3132DriverBinding = {
  SataSiI3132DriverBindingSupported,
  SataSiI3132DriverBindingStart,
  SataSiI3132DriverBindingStop,
  0x30,
  NULL,
  NULL
};

EFI_STATUS
SataSiI3132PortConstructor (
  IN  SATA_SI3132_INSTANCE *SataSiI3132Instance,
  IN  UINTN                Index
  )
{
  EFI_STATUS            Status;
  SATA_SI3132_PORT      *Port;
  VOID                  *HostPRB;
  EFI_PHYSICAL_ADDRESS  PhysAddrHostPRB;
  VOID                  *PciAllocMappingPRB;
  UINTN                 NumberOfBytes;

  Port = &(SataSiI3132Instance->Ports[Index]);

  Port->Index    = Index;
  Port->RegBase  = Index * 0x2000;
  Port->Instance = SataSiI3132Instance;
  InitializeListHead (&(Port->Devices));

  NumberOfBytes = sizeof (SATA_SI3132_PRB);
  Status = SataSiI3132Instance->PciIo->AllocateBuffer (
             SataSiI3132Instance->PciIo, AllocateAnyPages, EfiBootServicesData,
             EFI_SIZE_TO_PAGES (NumberOfBytes), &HostPRB, 0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check the alignment of the PCI Buffer
  ASSERT (((UINTN)HostPRB & (0x1000 - 1)) == 0);
  Status = SataSiI3132Instance->PciIo->Map (
             SataSiI3132Instance->PciIo, EfiPciIoOperationBusMasterCommonBuffer, HostPRB,
             &NumberOfBytes, &PhysAddrHostPRB, &PciAllocMappingPRB
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Port->HostPRB            = HostPRB;
  Port->PhysAddrHostPRB    = PhysAddrHostPRB;
  Port->PciAllocMappingPRB = PciAllocMappingPRB;

  return Status;
}

STATIC
EFI_STATUS
SataSiI3132Constructor (
  IN  EFI_PCI_IO_PROTOCOL     *PciIo,
  OUT SATA_SI3132_INSTANCE**  SataSiI3132Instance
  )
{
  SATA_SI3132_INSTANCE    *Instance;
  EFI_ATA_PASS_THRU_MODE  *AtaPassThruMode;

  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = (SATA_SI3132_INSTANCE*)AllocateZeroPool (sizeof (SATA_SI3132_INSTANCE));
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->Signature           = SATA_SII3132_SIGNATURE;
  Instance->PciIo               = PciIo;

  AtaPassThruMode = (EFI_ATA_PASS_THRU_MODE*)AllocatePool (sizeof (EFI_ATA_PASS_THRU_MODE));
  AtaPassThruMode->Attributes = EFI_ATA_PASS_THRU_ATTRIBUTES_PHYSICAL | EFI_ATA_PASS_THRU_ATTRIBUTES_LOGICAL;
  AtaPassThruMode->IoAlign = 0x1000;

  // Initialize SiI3132 ports
  SataSiI3132PortConstructor (Instance, 0);
  SataSiI3132PortConstructor (Instance, 1);

  // Set ATA Pass Thru Protocol
  Instance->AtaPassThruProtocol.Mode            = AtaPassThruMode;
  Instance->AtaPassThruProtocol.PassThru        = SiI3132AtaPassThru;
  Instance->AtaPassThruProtocol.GetNextPort     = SiI3132GetNextPort;
  Instance->AtaPassThruProtocol.GetNextDevice   = SiI3132GetNextDevice;
  Instance->AtaPassThruProtocol.BuildDevicePath = SiI3132BuildDevicePath;
  Instance->AtaPassThruProtocol.GetDevice       = SiI3132GetDevice;
  Instance->AtaPassThruProtocol.ResetPort       = SiI3132ResetPort;
  Instance->AtaPassThruProtocol.ResetDevice     = SiI3132ResetDevice;

  *SataSiI3132Instance = Instance;

  return EFI_SUCCESS;
}

EFI_STATUS
SiI3132SoftResetCommand (
  IN   SATA_SI3132_PORT *Port,
  OUT  UINT32* Signature
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_ATA_STATUS_BLOCK              Asb;
  EFI_ATA_COMMAND_BLOCK             Acb;
  CONST UINT16                      PortMultiplierPort = 0;

  ZeroMem (&Acb, sizeof (EFI_ATA_COMMAND_BLOCK));

  Acb.Reserved1[1] = 0;

  Packet.Asb      = &Asb;
  Packet.Acb      = &Acb;
  Packet.Timeout  = 100000;
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_ATA_SOFTWARE_RESET;

  Status = SiI3132AtaPassThruCommand (Port->Instance, Port, PortMultiplierPort, &Packet, 0);

  if (Status == EFI_SUCCESS) {
    *Signature = (Asb.AtaCylinderHigh << 24) | (Asb.AtaCylinderLow << 16) |
                 (Asb.AtaSectorNumber << 8 ) | (Asb.AtaSectorCount);
  }
  return Status;
}

EFI_STATUS
SataSiI3132PortInitialization (
  IN SATA_SI3132_PORT *Port
  )
{
  UINT32                  Value32;
  SATA_SI3132_DEVICE*     Device;
  UINT32                  Signature;
  EFI_STATUS              Status;
  EFI_PCI_IO_PROTOCOL*    PciIo;

  Status = SiI3132HwResetPort (Port);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PciIo = Port->Instance->PciIo;

  // Is a device is present ?
  Status = SATA_PORT_READ32 (Port->RegBase + SII3132_PORT_SSTATUS_REG, &Value32);
  if (!EFI_ERROR (Status) && (Value32 & 0x3)) {
    // Do a soft reset to see if it is a port multiplier
    SATA_TRACE ("SataSiI3132PortInitialization: soft reset - it is a port multiplier\n");
    Status = SiI3132SoftResetCommand (Port, &Signature);
    if (!EFI_ERROR (Status)) {
      if (Signature == SII3132_PORT_SIGNATURE_PMP) {
        SATA_TRACE ("SataSiI3132PortInitialization(): a Port Multiplier is present");
        if (FeaturePcdGet (PcdSataSiI3132FeaturePMPSupport)) {
          ASSERT (0); // Not supported yet
        } else {
          return EFI_UNSUPPORTED;
        }
      } else if (Signature == SII3132_PORT_SIGNATURE_ATAPI) {
        ASSERT (0); // Not supported yet
        SATA_TRACE ("SataSiI3132PortInitialization(): an ATAPI device is present");
        return EFI_UNSUPPORTED;
      } else if (Signature == SII3132_PORT_SIGNATURE_ATA) {
        SATA_TRACE ("SataSiI3132PortInitialization(): an ATA device is present");
      } else {
        SATA_TRACE ("SataSiI3132PortInitialization(): Present device unknown!");
        ASSERT (0); // Not supported
        return EFI_UNSUPPORTED;
      }

      // Create Device
      Device            = (SATA_SI3132_DEVICE*)AllocatePool (sizeof (SATA_SI3132_DEVICE));
      Device->Index     = Port->Index; //TODO: Could need to be fixed when SATA Port Multiplier support
      Device->Port      = Port;
      Device->BlockSize = 0;

      // Attached the device to the Sata Port
      InsertTailList (&Port->Devices, &Device->Link);

      SATA_TRACE ("SataSiI3132PortInitialization(): Port Ready");
    }
  }
  return Status;
}

EFI_STATUS
SataSiI3132Initialization (
  IN SATA_SI3132_INSTANCE* SataSiI3132Instance
  )
{
  UINTN                 Index;
  EFI_PCI_IO_PROTOCOL*  PciIo;

  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  PciIo = SataSiI3132Instance->PciIo;

  // Turn Off GPIO
  SATA_GLOBAL_WRITE32 (SII3132_GLOBAL_FLASHADDR_REG, 0x0);

  // Clear Global Control Register
  SATA_GLOBAL_WRITE32 (SII3132_GLOBAL_CONTROL_REG, 0x0);

  for (Index = 0; Index < SATA_SII3132_MAXPORT; Index++) {
    SataSiI3132PortInitialization (&(SataSiI3132Instance->Ports[Index]));
  }

  return EFI_SUCCESS;
}

/**
  Test to see if this driver supports ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SataSiI3132DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS              Status;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  UINT32                   PciID;

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                Controller,
                &gEfiPciIoProtocolGuid,
                (VOID **) &PciIo,
                This->DriverBindingHandle,
                Controller,
                EFI_OPEN_PROTOCOL_BY_DRIVER
                );
  if (EFI_ERROR (Status)) {
      return Status;
  }

  Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      PCI_VENDOR_ID_OFFSET,
                      1,
                      &PciID
                      );
  if (EFI_ERROR (Status)) {
      Status = EFI_UNSUPPORTED;
      goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to SATA Mass Storage type
  //
  if (PciID != ((SATA_SII3132_DEVICE_ID << 16) | SATA_SII3132_VENDOR_ID)) {
      Status = EFI_UNSUPPORTED;
  }

ON_EXIT:
  gBS->CloseProtocol (
       Controller,
       &gEfiPciIoProtocolGuid,
       This->DriverBindingHandle,
       Controller
       );

  return Status;
}

BOOLEAN mbStarted = FALSE;

/**
  Starting the Pci SATA Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          supports this device.
  @return EFI_UNSUPPORTED      do not support this device.
  @return EFI_DEVICE_ERROR     cannot be started due to device Error.
  @return EFI_OUT_OF_RESOURCES cannot allocate resources.

**/
EFI_STATUS
EFIAPI
SataSiI3132DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS              Status;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  UINT64                  Supports;
  UINT64                  OriginalPciAttributes;
  BOOLEAN                 PciAttributesSaved;
  UINT32                  PciID;
  SATA_SI3132_INSTANCE    *SataSiI3132Instance = NULL;

  SATA_TRACE ("SataSiI3132DriverBindingStart()");

  //TODO: Find a nicer way to do it !
  if (mbStarted) {
    return EFI_SUCCESS; // Don't restart me !
  }

  //
  // Open the PciIo Protocol
  //
  Status = gBS->OpenProtocol (
                Controller,
                &gEfiPciIoProtocolGuid,
                (VOID **) &PciIo,
                This->DriverBindingHandle,
                Controller,
                EFI_OPEN_PROTOCOL_BY_DRIVER
                );
  if (EFI_ERROR (Status)) {
      return Status;
  }

  PciAttributesSaved = FALSE;
  //
  // Save original PCI attributes
  //
  Status = PciIo->Attributes (
                  PciIo,
                  EfiPciIoAttributeOperationGet,
                  0,
                  &OriginalPciAttributes
                  );
  if (EFI_ERROR (Status)) {
      goto CLOSE_PCIIO;
  }
  PciAttributesSaved = TRUE;

  Status = PciIo->Attributes (
                  PciIo,
                  EfiPciIoAttributeOperationSupported,
                  0,
                  &Supports
                  );
  if (!EFI_ERROR (Status)) {
      Supports &= EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE;
      Status = PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationEnable,
                        Supports,
                        NULL
                        );
  }
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SataSiI3132DriverBindingStart: failed to enable controller\n"));
    goto CLOSE_PCIIO;
  }

  //
  // Get the Pci device class code.
  //
  Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      PCI_VENDOR_ID_OFFSET,
                      1,
                      &PciID
                      );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto CLOSE_PCIIO;
  }

  //
  // Test whether the controller belongs to SATA Mass Storage type
  //
  if (PciID != ((SATA_SII3132_DEVICE_ID << 16) | SATA_SII3132_VENDOR_ID)) {
    Status = EFI_UNSUPPORTED;
    goto CLOSE_PCIIO;
  }

  // Create SiI3132 Sata Instance
  Status = SataSiI3132Constructor (PciIo, &SataSiI3132Instance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Initialize SiI3132 Sata Controller
  Status = SataSiI3132Initialization (SataSiI3132Instance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Install Ata Pass Thru Protocol
  Status = gBS->InstallProtocolInterface (
              &Controller,
              &gEfiAtaPassThruProtocolGuid,
              EFI_NATIVE_INTERFACE,
              &(SataSiI3132Instance->AtaPassThruProtocol)
              );
  if (EFI_ERROR (Status)) {
    goto FREE_POOL;
  }

/*  //
  // Create event to stop the HC when exit boot service.
  //
  Status = gBS->CreateEventEx (
                EVT_NOTIFY_SIGNAL,
                TPL_NOTIFY,
                EhcExitBootService,
                Ehc,
                &gEfiEventExitBootServicesGuid,
                &Ehc->ExitBootServiceEvent
                );
  if (EFI_ERROR (Status)) {
      goto UNINSTALL_USBHC;
  }*/

  mbStarted = TRUE;

  SATA_TRACE ("SataSiI3132DriverBindingStart() Success!");
  return EFI_SUCCESS;

FREE_POOL:
  //TODO: Free SATA Instance

CLOSE_PCIIO:
  if (PciAttributesSaved) {
      //
      // Restore original PCI attributes
      //
      PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationSet,
                      OriginalPciAttributes,
                      NULL
                      );
  }

  gBS->CloseProtocol (
       Controller,
       &gEfiPciIoProtocolGuid,
       This->DriverBindingHandle,
       Controller
       );

  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS          Success.
  @return EFI_DEVICE_ERROR     Fail.

**/
EFI_STATUS
EFIAPI
SataSiI3132DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  SATA_TRACE ("SataSiI3132DriverBindingStop()");
  return EFI_UNSUPPORTED;
}

/**
  Entry point of this driver

  @param ImageHandle     Handle of driver image
  @param SystemTable     Point to EFI_SYSTEM_TABLE

  @retval EFI_OUT_OF_RESOURCES  Can not allocate memory resource
  @retval EFI_DEVICE_ERROR      Can not install the protocol instance
  @retval EFI_SUCCESS           Success to initialize the Pci host bridge.
**/
EFI_STATUS
EFIAPI
InitializeSataSiI3132 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SATA_TRACE ("InitializeSataSiI3132 ()");

  return EfiLibInstallDriverBindingComponentName2 (
         ImageHandle,
         SystemTable,
         &gSataSiI3132DriverBinding,
         ImageHandle,
         &gSataSiI3132ComponentName,
         &gSataSiI3132ComponentName2
         );
}

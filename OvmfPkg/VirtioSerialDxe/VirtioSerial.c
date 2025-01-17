/** @file

  Driver for virtio-serial devices.

  The virtio serial device also known as virtio console device because
  initially it had only support for a single tty, intended to be used
  as console.  Support for multiple streams and named data ports has
  been added later on.

  https://docs.oasis-open.org/virtio/virtio/v1.2/cs01/virtio-v1.2-cs01.html#x1-2900003

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include "VirtioSerial.h"

STATIC LIST_ENTRY  mVirtioSerialList;

STATIC CONST CHAR8  *EventNames[] = {
  [VIRTIO_SERIAL_DEVICE_READY]  = "device-ready",
  [VIRTIO_SERIAL_DEVICE_ADD]    = "device-add",
  [VIRTIO_SERIAL_DEVICE_REMOVE] = "device-remove",
  [VIRTIO_SERIAL_PORT_READY]    = "port-ready",
  [VIRTIO_SERIAL_CONSOLE_PORT]  = "console-port",
  [VIRTIO_SERIAL_RESIZE]        = "resize",
  [VIRTIO_SERIAL_PORT_OPEN]     = "port-open",
  [VIRTIO_SERIAL_PORT_NAME]     = "port-name",
};

VOID
EFIAPI
LogDevicePath (
  UINT32                    Level,
  const CHAR8               *Func,
  CHAR16                    *Note,
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16  *Str;

  Str = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  if (!Str) {
    DEBUG ((DEBUG_INFO, "ConvertDevicePathToText failed\n"));
    return;
  }

  DEBUG ((Level, "%a: %s%s%s\n", Func, Note ? Note : L"", Note ? L": " : L"", Str));
  FreePool (Str);
}

EFI_STATUS
EFIAPI
VirtioSerialTxControl (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             Id,
  IN     UINT16             Event,
  IN     UINT16             Value
  )
{
  VIRTIO_SERIAL_CONTROL  Control;

  Control.Id    = Id;
  Control.Event = Event;
  Control.Value = Value;

  DEBUG ((
    DEBUG_INFO,
    "%a:%d: >>> event %a, port-id %d, value %d\n",
    __func__,
    __LINE__,
    EventNames[Control.Event],
    Control.Id,
    Control.Value
    ));

  VirtioSerialRingClearTx (Dev, VIRTIO_SERIAL_Q_TX_CTRL);
  return VirtioSerialRingSendBuffer (Dev, VIRTIO_SERIAL_Q_TX_CTRL, &Control, sizeof (Control), TRUE);
}

STATIC
VOID
EFIAPI
VirtioSerialRxControl (
  IN OUT VIRTIO_SERIAL_DEV  *Dev
  )
{
  UINT8                  Data[CTRL_RX_BUFSIZE+1];
  UINT32                 DataSize;
  VIRTIO_SERIAL_CONTROL  Control;
  EFI_STATUS             Status;
  BOOLEAN                HasData;
  UINT16                 Ready;

  for ( ; ;) {
    HasData = VirtioSerialRingGetBuffer (Dev, VIRTIO_SERIAL_Q_RX_CTRL, Data, &DataSize);
    if (!HasData) {
      return;
    }

    if (DataSize < sizeof (Control)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%d: length mismatch: %d != %d\n",
        __func__,
        __LINE__,
        DataSize,
        sizeof (Control)
        ));
      continue;
    }

    CopyMem (&Control, Data, sizeof (Control));

    if (Control.Event < ARRAY_SIZE (EventNames)) {
      DEBUG ((
        DEBUG_INFO,
        "%a:%d: <<< event %a, port-id %d, value %d\n",
        __func__,
        __LINE__,
        EventNames[Control.Event],
        Control.Id,
        Control.Value
        ));
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%d: unknown event: %d\n",
        __func__,
        __LINE__,
        Control.Event
        ));
    }

    switch (Control.Event) {
      case VIRTIO_SERIAL_DEVICE_ADD:
        if (Control.Id < MAX_PORTS) {
          Status = VirtioSerialPortAdd (Dev, Control.Id);
          Ready  = (Status == EFI_SUCCESS) ? 1 : 0;
        } else {
          Ready = 0;
        }

        VirtioSerialTxControl (Dev, Control.Id, VIRTIO_SERIAL_PORT_READY, Ready);
        if (Ready) {
          Dev->NumPorts++;
        }

        break;
      case VIRTIO_SERIAL_DEVICE_REMOVE:
        if (Control.Id < MAX_PORTS) {
          VirtioSerialPortRemove (Dev, Control.Id);
        }

        break;
      case VIRTIO_SERIAL_CONSOLE_PORT:
        if (Control.Id < MAX_PORTS) {
          VirtioSerialPortSetConsole (Dev, Control.Id);
          Dev->NumConsoles++;
          VirtioSerialPortSetDeviceOpen (Dev, Control.Id, 1);
        }

        break;
      case VIRTIO_SERIAL_PORT_NAME:
        if (Control.Id < MAX_PORTS) {
          Data[DataSize] = 0;
          VirtioSerialPortSetName (Dev, Control.Id, Data + sizeof (Control));
          Dev->NumNamedPorts++;
        }

        break;
      case VIRTIO_SERIAL_PORT_OPEN:
        if (Control.Id < MAX_PORTS) {
          VirtioSerialPortSetDeviceOpen (Dev, Control.Id, Control.Value);
        }

        break;
      default:
        break;
    }
  }
}

STATIC
VOID
EFIAPI
VirtioSerialTimer (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  VIRTIO_SERIAL_DEV  *Dev = Context;

  VirtioSerialRxControl (Dev);
}

STATIC
VOID
EFIAPI
VirtioSerialUninitAllRings (
  IN OUT VIRTIO_SERIAL_DEV  *Dev
  )
{
  UINT16  Index;

  for (Index = 0; Index < MAX_RINGS; Index++) {
    VirtioSerialUninitRing (Dev, Index);
  }
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialInit (
  IN OUT VIRTIO_SERIAL_DEV  *Dev
  )
{
  UINT8       NextDevStat;
  EFI_STATUS  Status;
  UINT64      Features;
  UINTN       Retries;

  //
  // Execute virtio-0.9.5, 2.2.1 Device Initialization Sequence.
  //
  NextDevStat = 0;             // step 1 -- reset device
  Status      = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_ACK;    // step 2 -- acknowledge device presence
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_DRIVER; // step 3 -- we know how to drive it
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // Set Page Size - MMIO VirtIo Specific
  //
  Status = Dev->VirtIo->SetPageSize (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // step 4a -- retrieve and validate features
  //
  Status = Dev->VirtIo->GetDeviceFeatures (Dev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Features &= (VIRTIO_F_VERSION_1 |
               VIRTIO_F_IOMMU_PLATFORM |
               VIRTIO_SERIAL_F_MULTIPORT);

  //
  // In virtio-1.0, feature negotiation is expected to complete before queue
  // discovery, and the device can also reject the selected set of features.
  //
  if (Dev->VirtIo->Revision >= VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Status = Virtio10WriteFeatures (Dev->VirtIo, Features, &NextDevStat);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "%a:%d: features ok:%a%a%a\n",
    __func__,
    __LINE__,
    (Features & VIRTIO_F_VERSION_1)        ? " v1.0"      : "",
    (Features & VIRTIO_F_IOMMU_PLATFORM)   ? " iommu"     : "",
    (Features & VIRTIO_SERIAL_F_MULTIPORT) ? " multiport" : ""
    ));

  if (Features & VIRTIO_SERIAL_F_MULTIPORT) {
    Dev->VirtIo->ReadDevice (
                   Dev->VirtIo,
                   OFFSET_OF (VIRTIO_SERIAL_CONFIG, MaxPorts),
                   sizeof (Dev->Config.MaxPorts),
                   sizeof (Dev->Config.MaxPorts),
                   &Dev->Config.MaxPorts
                   );
    DEBUG ((
      DEBUG_INFO,
      "%a:%d: max device ports: %d\n",
      __func__,
      __LINE__,
      Dev->Config.MaxPorts
      ));
  }

  Status = VirtioSerialInitRing (Dev, VIRTIO_SERIAL_Q_RX_CTRL, CTRL_RX_BUFSIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = VirtioSerialInitRing (Dev, VIRTIO_SERIAL_Q_TX_CTRL, CTRL_TX_BUFSIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // step 5 -- Report understood features and guest-tuneables.
  //
  if (Dev->VirtIo->Revision < VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Features &= ~(UINT64)(VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM);
    Status    = Dev->VirtIo->SetGuestFeatures (Dev->VirtIo, Features);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
  }

  //
  // step 6 -- initialization complete
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  VirtioSerialRingFillRx (Dev, VIRTIO_SERIAL_Q_RX_CTRL);
  VirtioSerialTxControl (Dev, 0, VIRTIO_SERIAL_DEVICE_READY, 1);
  for (Retries = 0; Retries < 100; Retries++) {
    gBS->Stall (1000);
    VirtioSerialRxControl (Dev);
    if (Dev->NumPorts && (Dev->NumConsoles + Dev->NumNamedPorts == Dev->NumPorts)) {
      // port discovery complete
      break;
    }
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtioSerialTimer,
                  Dev,
                  &Dev->Timer
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = gBS->SetTimer (
                  Dev->Timer,
                  TimerPeriodic,
                  EFI_TIMER_PERIOD_MILLISECONDS (10)
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a:%d: OK, %d consoles, %d named ports\n",
    __func__,
    __LINE__,
    Dev->NumConsoles,
    Dev->NumNamedPorts
    ));
  return EFI_SUCCESS;

Failed:
  VirtioSerialUninitAllRings (Dev);

  //
  // Notify the host about our failure to setup: virtio-0.9.5, 2.2.2.1 Device
  // Status. VirtIo access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);

  DEBUG ((DEBUG_INFO, "%a:%d: ERROR: %r\n", __func__, __LINE__, Status));
  return Status; // reached only via Failed above
}

STATIC
VOID
EFIAPI
VirtioSerialUninit (
  IN OUT VIRTIO_SERIAL_DEV  *Dev
  )
{
  UINT32  PortId;

  gBS->CloseEvent (Dev->Timer);

  //
  // Reset the virtual device -- see virtio-0.9.5, 2.2.2.1 Device Status. When
  // VIRTIO_CFG_WRITE() returns, the host will have learned to stay away from
  // the old comms area.
  //
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);

  for (PortId = 0; PortId < MAX_PORTS; PortId++) {
    VirtioSerialPortRemove (Dev, PortId);
  }

  VirtioSerialUninitAllRings (Dev);
}

//
// Event notification function enqueued by ExitBootServices().
//

STATIC
VOID
EFIAPI
VirtioSerialExitBoot (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VIRTIO_SERIAL_DEV  *Dev;

  DEBUG ((DEBUG_INFO, "%a: Context=0x%p\n", __func__, Context));
  //
  // Reset the device. This causes the hypervisor to forget about the virtio
  // ring.
  //
  // We allocated said ring in EfiBootServicesData type memory, and code
  // executing after ExitBootServices() is permitted to overwrite it.
  //
  Dev = Context;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);
}

STATIC
VIRTIO_SERIAL_DEV *
VirtioSerialFind (
  EFI_HANDLE  DeviceHandle
  )
{
  VIRTIO_SERIAL_DEV  *Dev;
  LIST_ENTRY         *Entry;

  BASE_LIST_FOR_EACH (Entry, &mVirtioSerialList) {
    Dev = CR (Entry, VIRTIO_SERIAL_DEV, Link, VIRTIO_SERIAL_SIG);
    if (DeviceHandle == Dev->DeviceHandle) {
      return Dev;
    }
  }
  return NULL;
}

//
// Probe, start and stop functions of this driver, called by the DXE core for
// specific devices.
//
// The following specifications document these interfaces:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01, 9 Driver Binding Protocol
// - UEFI Spec 2.3.1 + Errata C, 10.1 EFI Driver Binding Protocol
//
// The implementation follows:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01
//   - 5.1.3.4 OpenProtocol() and CloseProtocol()
// - UEFI Spec 2.3.1 + Errata C
//   -  6.3 Protocol Handler Services
//

STATIC
EFI_STATUS
EFIAPI
VirtioSerialDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS              Status;
  VIRTIO_DEVICE_PROTOCOL  *VirtIo;

  //
  // Attempt to open the device with the VirtIo set of interfaces. On success,
  // the protocol is "instantiated" for the VirtIo device. Covers duplicate
  // open attempts (EFI_ALREADY_STARTED).
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,               // candidate device
                  &gVirtioDeviceProtocolGuid, // for generic VirtIo access
                  (VOID **)&VirtIo,           // handle to instantiate
                  This->DriverBindingHandle,  // requestor driver identity
                  DeviceHandle,               // ControllerHandle, according to
                                              // the UEFI Driver Model
                  EFI_OPEN_PROTOCOL_BY_DRIVER // get exclusive VirtIo access to
                                              // the device; to be released
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (VirtIo->SubSystemDeviceId != VIRTIO_SUBSYSTEM_CONSOLE) {
    Status = EFI_UNSUPPORTED;
  }

  //
  // We needed VirtIo access only transitorily, to see whether we support the
  // device or not.
  //
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  VIRTIO_SERIAL_DEV  *Dev;
  EFI_STATUS         Status;

  Dev = (VIRTIO_SERIAL_DEV *)AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&Dev->DevicePath,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto FreeVirtioSerial;
  }

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gVirtioDeviceProtocolGuid,
                  (VOID **)&Dev->VirtIo,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreeVirtioSerial;
  }

  LogDevicePath (DEBUG_INFO, __func__, L"Dev", Dev->DevicePath);

  //
  // VirtIo access granted, configure virtio-serial device.
  //
  Dev->Signature           = VIRTIO_SERIAL_SIG;
  Dev->DriverBindingHandle = This->DriverBindingHandle;
  Dev->DeviceHandle        = DeviceHandle;
  Status                   = VirtioSerialInit (Dev);
  if (EFI_ERROR (Status)) {
    goto CloseVirtIo;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  &VirtioSerialExitBoot,
                  Dev,
                  &Dev->ExitBoot
                  );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  InsertTailList (&mVirtioSerialList, &(Dev->Link));
  return EFI_SUCCESS;

UninitDev:
  VirtioSerialUninit (Dev);

CloseVirtIo:
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

FreeVirtioSerial:
  FreePool (Dev);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  VIRTIO_SERIAL_DEV  *Dev;
  UINT32             PortId;
  UINT32             Child;

  Dev = VirtioSerialFind (DeviceHandle);
  if (!Dev) {
    return EFI_SUCCESS;
  }

  if (NumberOfChildren) {
    for (Child = 0; Child < NumberOfChildren; Child++) {
      DEBUG ((DEBUG_INFO, "%a:%d: child handle 0x%x\n", __func__, __LINE__, ChildHandleBuffer[Child]));
      for (PortId = 0; PortId < MAX_PORTS; PortId++) {
        if (Dev->Ports[PortId].Ready &&
            Dev->Ports[PortId].SerialIo &&
            (Dev->Ports[PortId].SerialIo->DeviceHandle == ChildHandleBuffer[Child]))
        {
          VirtioSerialPortRemove (Dev, PortId);
        }
      }
    }

    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "%a:%d: controller handle 0x%x\n", __func__, __LINE__, DeviceHandle));

  gBS->CloseEvent (Dev->ExitBoot);

  VirtioSerialUninit (Dev);

  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

  RemoveEntryList (&(Dev->Link));
  ZeroMem (Dev, sizeof (*Dev));
  FreePool (Dev);

  return EFI_SUCCESS;
}

//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  &VirtioSerialDriverBindingSupported,
  &VirtioSerialDriverBindingStart,
  &VirtioSerialDriverBindingStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in VirtioSerialEntryPoint()
  NULL  // DriverBindingHandle, ditto
};

//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//

STATIC
EFI_UNICODE_STRING_TABLE  mDriverNameTable[] = {
  { "eng;en", L"Virtio Serial Driver" },
  { NULL,     NULL                    }
};

STATIC
EFI_UNICODE_STRING_TABLE  mDeviceNameTable[] = {
  { "eng;en", L"Virtio Serial Device" },
  { NULL,     NULL                    }
};

STATIC
EFI_UNICODE_STRING_TABLE  mPortNameTable[] = {
  { "eng;en", L"Virtio Serial Port" },
  { NULL,     NULL                  }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName;

STATIC
EFI_STATUS
EFIAPI
VirtioSerialGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gComponentName) // Iso639Language
           );
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   DeviceHandle,
  IN  EFI_HANDLE                   ChildHandle,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  EFI_UNICODE_STRING_TABLE  *Table;
  VIRTIO_SERIAL_DEV         *Dev;
  UINT32                    PortId;

  Dev = VirtioSerialFind (DeviceHandle);
  if (!Dev) {
    return EFI_UNSUPPORTED;
  }

  if (ChildHandle) {
    for (PortId = 0; PortId < MAX_PORTS; PortId++) {
      if (Dev->Ports[PortId].Ready &&
          Dev->Ports[PortId].SerialIo &&
          (Dev->Ports[PortId].SerialIo->DeviceHandle == ChildHandle))
      {
        *ControllerName = Dev->Ports[PortId].Name;
        return EFI_SUCCESS;
      }
    }

    Table = mPortNameTable;
  } else {
    Table = mDeviceNameTable;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           Table,
           ControllerName,
           (BOOLEAN)(This == &gComponentName)
           );
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName = {
  &VirtioSerialGetDriverName,
  &VirtioSerialGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)&VirtioSerialGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)&VirtioSerialGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
VirtioSerialEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  InitializeListHead (&mVirtioSerialList);
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}

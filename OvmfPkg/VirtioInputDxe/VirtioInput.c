/** @file

  Driver for virtio input devices.

  Copyright (C) 2024, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include <IndustryStandard/VirtioInput.h>

#include <VirtioInput.h>
#include <VirtioKeyCodes.h>

// -----------------------------------------------------------------------------
// Return buffer pointer out of the ring buffer
STATIC
VOID *
BufferPtr (
  IN VIRTIO_INPUT_RING  *Ring,
  IN UINT32             BufferNr
  )
{
  return Ring->Buffers + Ring->BufferSize * BufferNr;
}

// -----------------------------------------------------------------------------
// Return buffer physical address out of the ring buffer
STATIC
EFI_PHYSICAL_ADDRESS
BufferAddr (
  IN VIRTIO_INPUT_RING  *Ring,
  IN UINT32             BufferNr
  )
{
  return Ring->DeviceAddress + Ring->BufferSize * BufferNr;
}

// Return next buffer from ring
STATIC
UINT32
BufferNext (
  IN VIRTIO_INPUT_RING  *Ring
  )
{
  return Ring->Indices.NextDescIdx % Ring->Ring.QueueSize;
}

// -----------------------------------------------------------------------------
// Push the buffer to the device
STATIC
EFI_STATUS
EFIAPI
VirtioInputRingSendBuffer (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN     UINT16            Index,
  IN     VOID              *Data,
  IN     UINT32            DataSize,
  IN     BOOLEAN           Notify
  )
{
  VIRTIO_INPUT_RING  *Ring    = Dev->Rings + Index;
  UINT32             BufferNr = BufferNext (Ring);
  UINT16             Idx      = *Ring->Ring.Avail.Idx;
  UINT16             Flags    = 0;

  ASSERT (DataSize <= Ring->BufferSize);

  if (Data) {
    /* driver -> device */
    CopyMem (BufferPtr (Ring, BufferNr), Data, DataSize);
  } else {
    /* device -> driver */
    Flags |= VRING_DESC_F_WRITE;
  }

  VirtioAppendDesc (
    &Ring->Ring,
    BufferAddr (Ring, BufferNr),
    DataSize,
    Flags,
    &Ring->Indices
    );

  Ring->Ring.Avail.Ring[Idx % Ring->Ring.QueueSize] =
    Ring->Indices.HeadDescIdx % Ring->Ring.QueueSize;
  Ring->Indices.HeadDescIdx = Ring->Indices.NextDescIdx;
  Idx++;

  // Force compiler to not optimize this code
  MemoryFence ();
  *Ring->Ring.Avail.Idx = Idx;
  MemoryFence ();

  if (Notify) {
    Dev->VirtIo->SetQueueNotify (Dev->VirtIo, Index);
  }

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// Get data from buffer which is marked as ready from device
STATIC
BOOLEAN
EFIAPI
VirtioInputRingGetBuffer (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN     UINT16            Index,
  OUT    VOID              *Data,
  OUT    UINT32            *DataSize
  )
{
  VIRTIO_INPUT_RING         *Ring   = Dev->Rings + Index;
  UINT16                    UsedIdx = *Ring->Ring.Used.Idx;
  volatile VRING_USED_ELEM  *UsedElem;

  if (!Ring->Ready) {
    return FALSE;
  }

  if (Ring->LastUsedIdx == UsedIdx) {
    return FALSE;
  }

  UsedElem = Ring->Ring.Used.UsedElem + (Ring->LastUsedIdx % Ring->Ring.QueueSize);

  if (UsedElem->Len > Ring->BufferSize) {
    DEBUG ((DEBUG_ERROR, "%a:%d: %d: invalid length\n", __func__, __LINE__, Index));
    UsedElem->Len = 0;
  }

  if (Data && DataSize) {
    CopyMem (Data, BufferPtr (Ring, UsedElem->Id), UsedElem->Len);
    *DataSize = UsedElem->Len;
  }

  if (Index % 2 == 0) {
    /* RX - re-queue buffer */
    VirtioInputRingSendBuffer (Dev, Index, NULL, Ring->BufferSize, FALSE);
  }

  Ring->LastUsedIdx++;
  return TRUE;
}

// -----------------------------------------------------------------------------
// Initialize ring buffer
STATIC
EFI_STATUS
EFIAPI
VirtioInputInitRing (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN     UINT16            Index,
  IN     UINT32            BufferSize
  )
{
  VIRTIO_INPUT_RING  *Ring = Dev->Rings + Index;
  EFI_STATUS         Status;
  UINT16             QueueSize;
  UINT64             RingBaseShift;

  //
  // step 4b -- allocate request virtqueue
  //
  Status = Dev->VirtIo->SetQueueSel (Dev->VirtIo, Index);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = Dev->VirtIo->GetQueueNumMax (Dev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // VirtioInput uses one descriptor
  //
  if (QueueSize < 1) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Status = VirtioRingInit (Dev->VirtIo, QueueSize, &Ring->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // If anything fails from here on, we must release the ring resources.
  //
  Status = VirtioRingMap (
             Dev->VirtIo,
             &Ring->Ring,
             &RingBaseShift,
             &Ring->RingMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // Additional steps for MMIO: align the queue appropriately, and set the
  // size. If anything fails from here on, we must unmap the ring resources.
  //
  Status = Dev->VirtIo->SetQueueNum (Dev->VirtIo, QueueSize);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = Dev->VirtIo->SetQueueAlign (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // step 4c -- Report GPFN (guest-physical frame number) of queue.
  //
  Status = Dev->VirtIo->SetQueueAddress (
                          Dev->VirtIo,
                          &Ring->Ring,
                          RingBaseShift
                          );
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Ring->BufferCount = QueueSize;
  Ring->BufferSize  = BufferSize;
  Ring->BufferPages = EFI_SIZE_TO_PAGES (Ring->BufferCount * Ring->BufferSize);

  Status = Dev->VirtIo->AllocateSharedPages (Dev->VirtIo, Ring->BufferPages, (VOID **)&Ring->Buffers);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterCommonBuffer,
             Ring->Buffers,
             EFI_PAGES_TO_SIZE (Ring->BufferPages),
             &Ring->DeviceAddress,
             &Ring->BufferMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleasePages;
  }

  VirtioPrepare (&Ring->Ring, &Ring->Indices);
  Ring->Ready = TRUE;

  return EFI_SUCCESS;

ReleasePages:
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 Ring->BufferPages,
                 Ring->Buffers
                 );
  Ring->Buffers = NULL;

UnmapQueue:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->RingMap);
  Ring->RingMap = NULL;

ReleaseQueue:
  VirtioRingUninit (Dev->VirtIo, &Ring->Ring);

Failed:
  return Status;
}

// -----------------------------------------------------------------------------
// Deinitialize ring buffer
STATIC
VOID
EFIAPI
VirtioInputUninitRing (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN     UINT16            Index
  )
{
  VIRTIO_INPUT_RING  *Ring = Dev->Rings + Index;

  if (Ring->BufferMap) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->BufferMap);
    Ring->BufferMap = NULL;
  }

  if (Ring->Buffers) {
    Dev->VirtIo->FreeSharedPages (
                   Dev->VirtIo,
                   Ring->BufferPages,
                   Ring->Buffers
                   );
    Ring->Buffers = NULL;
  }

  if (Ring->RingMap) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->RingMap);
    Ring->RingMap = NULL;
  }

  if (Ring->Ring.Base) {
    VirtioRingUninit (Dev->VirtIo, &Ring->Ring);
  }

  ZeroMem (Ring, sizeof (*Ring));
}

// -----------------------------------------------------------------------------
// Deinitialize all rings allocated in driver
STATIC
VOID
EFIAPI
VirtioInputUninitAllRings (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  UINT16  Index;

  for (Index = 0; Index < MAX_RINGS; Index++) {
    VirtioInputUninitRing (Dev, Index);
  }
}

// -----------------------------------------------------------------------------
// Mark all buffers as ready to write and push to device
VOID
EFIAPI
VirtioInputRingFillRx (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN     UINT16            Index
  )
{
  VIRTIO_INPUT_RING  *Ring = Dev->Rings + Index;
  UINT32             BufferNr;

  for (BufferNr = 0; BufferNr < Ring->BufferCount; BufferNr++) {
    VirtioInputRingSendBuffer (Dev, Index, NULL, Ring->BufferSize, FALSE);
  }

  Dev->VirtIo->SetQueueNotify (Dev->VirtIo, Index);
}

EFI_STATUS
VirtioInputConfigQuerySize (
  IN VIRTIO_INPUT_DEV            *Dev,
  IN VIRTIO_INPUT_CONFIG_SELECT  Select,
  IN UINT8                       Subsel,
  OUT UINT8                      *Size
  )
{
  EFI_STATUS  Status;

  Status = Dev->VirtIo->WriteDevice (Dev->VirtIo, OFFSET_OF_VINPUT (Select), SIZE_OF_VINPUT (Select), Select);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Dev->VirtIo->WriteDevice (Dev->VirtIo, OFFSET_OF_VINPUT (Subsel), SIZE_OF_VINPUT (Subsel), Subsel);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Dev->VirtIo->ReadDevice (Dev->VirtIo, OFFSET_OF_VINPUT (Size), SIZE_OF_VINPUT (Size), sizeof (*Size), Size);
  return Status;
}

// -----------------------------------------------------------------------------
// Main function processing virtio input events
STATIC
VOID
EFIAPI
VirtioInputGetDeviceData (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  BOOLEAN             HasData;
  UINT8               Data[RX_BUFSIZE + 1];
  UINT32              DataSize;
  VIRTIO_INPUT_EVENT  Event;
  EFI_TPL             OldTpl;

  for ( ; ; ) {
    HasData = VirtioInputRingGetBuffer (Dev, 0, Data, &DataSize);

    // Exit if no new data
    if (!HasData) {
      return;
    }

    if (DataSize < sizeof (Event)) {
      continue;
    }

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

    CopyMem (&Event, Data, sizeof (Event));

    switch (Event.Type) {
      case EV_SYN:
        // Sync event received
        break;

      case EV_KEY:
        // Key press event received
        // DEBUG ((DEBUG_INFO, "%a: ---------------------- \nType: %x Code: %x Value: %x\n",
        //              __func__, Event.Type, Event.Code, Event.Value));

        VirtioKeyboardHandleEvent (Dev, &Event);
        break;

      default:
        DEBUG ((DEBUG_INFO, "%a: Unhandled VirtIo event\n", __func__));
        break;
    }

    gBS->RestoreTPL (OldTpl);
  }
}

// -----------------------------------------------------------------------------
// Callback hook for timer interrupt
VOID
EFIAPI
VirtioInputTimer (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  VIRTIO_INPUT_DEV  *Dev = Context;

  VirtioInputGetDeviceData (Dev);
}

// -----------------------------------------------------------------------------
// Driver init
STATIC
EFI_STATUS
EFIAPI
VirtioInputInit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  UINT8       NextDevStat;
  EFI_STATUS  Status;
  UINT64      Features;

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

  Features &= VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM;

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

  Status = VirtioInputInitRing (Dev, 0, RX_BUFSIZE);
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

  //
  // Check input device capabilities
  //
  Dev->HasKeyboard = VirtioKeyboardProbe (Dev);
  if (!Dev->HasKeyboard) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  //
  // populate the exported interface's attributes
  //
  if (Dev->HasKeyboard) {
    Status = VirtioKeyboardInit (Dev);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
  }

  VirtioInputRingFillRx (Dev, 0);

  //
  // Event for reading key in time intervals
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtioInputTimer,
                  Dev,
                  &Dev->PollTimer
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = gBS->SetTimer (
                  Dev->PollTimer,
                  TimerPeriodic,
                  EFI_TIMER_PERIOD_MILLISECONDS (PROBE_TIME_MS)
                  );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  return EFI_SUCCESS;

Failed:
  VirtioInputUninitAllRings (Dev);

  //
  // Notify the host about our failure to setup: virtio-0.9.5, 2.2.2.1 Device
  // Status. VirtIo access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);

  return Status; // reached only via Failed above
}

// -----------------------------------------------------------------------------
// Deinitialize driver
STATIC
VOID
EFIAPI
VirtioInputUninit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  gBS->CloseEvent (Dev->PollTimer);

  if (Dev->HasKeyboard) {
    VirtioKeyboardUninit (Dev);
  }

  //
  // Reset the virtual device -- see virtio-0.9.5, 2.2.2.1 Device Status. When
  // VIRTIO_CFG_WRITE() returns, the host will have learned to stay away from
  // the old comms area.
  //
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);

  VirtioInputUninitAllRings (Dev);
}

// -----------------------------------------------------------------------------
// Handle device exit before switch to boot
STATIC
VOID
EFIAPI
VirtioInputExitBoot (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VIRTIO_INPUT_DEV  *Dev;

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

// -----------------------------------------------------------------------------
// Binding validation function
STATIC
EFI_STATUS
EFIAPI
VirtioInputBindingSupported (
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
    if (Status != EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_INFO, "%a:%d: %r\n", __func__, __LINE__, Status));
    }

    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a:%d: 0x%x\n", __func__, __LINE__, VirtIo->SubSystemDeviceId));
  if (VirtIo->SubSystemDeviceId != VIRTIO_SUBSYSTEM_INPUT) {
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

// -----------------------------------------------------------------------------
// Driver binding function API
STATIC
EFI_STATUS
EFIAPI
VirtioInputBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_STATUS        Status;

  Dev = (VIRTIO_INPUT_DEV *)AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
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
    goto FreeVirtioInput;
  }

  //
  // VirtIo access granted, configure virtio keyboard device.
  //
  Status = VirtioInputInit (Dev);
  if (EFI_ERROR (Status)) {
    goto CloseVirtIo;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  &VirtioInputExitBoot,
                  Dev,
                  &Dev->ExitBoot
                  );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  //
  // Setup complete, attempt to export the driver instance's EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  // interface.
  //
  Dev->Signature = VIRTIO_INPUT_SIG;

  if (Dev->HasKeyboard) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &DeviceHandle,
                    &gEfiSimpleTextInProtocolGuid,
                    &Dev->Txt,
                    &gEfiSimpleTextInputExProtocolGuid,
                    &Dev->TxtEx,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto CloseExitBoot;
    }
  }

  return EFI_SUCCESS;

CloseExitBoot:
  gBS->CloseEvent (Dev->ExitBoot);

UninitDev:
  VirtioInputUninit (Dev);

CloseVirtIo:
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

FreeVirtioInput:
  FreePool (Dev);

  return Status;
}

// -----------------------------------------------------------------------------
// Driver unbinding function API
STATIC
EFI_STATUS
EFIAPI
VirtioInputBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *Txt;
  VIRTIO_INPUT_DEV                *Dev;

  Status = gBS->OpenProtocol (
                  DeviceHandle,                     // candidate device
                  &gEfiSimpleTextInProtocolGuid,    // retrieve the RNG iface
                  (VOID **)&Txt,                    // target pointer
                  This->DriverBindingHandle,        // requestor driver ident.
                  DeviceHandle,                     // lookup req. for dev.
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL    // lookup only, no new ref.
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = VIRTIO_INPUT_FROM_THIS (Txt);

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  if (Dev->HasKeyboard) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    DeviceHandle,
                    &gEfiSimpleTextInProtocolGuid,
                    &Dev->Txt,
                    &gEfiSimpleTextInputExProtocolGuid,
                    &Dev->TxtEx,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  gBS->CloseEvent (Dev->ExitBoot);

  VirtioInputUninit (Dev);

  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

  FreePool (Dev);

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// Forward declaration of global variable
STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName;

// -----------------------------------------------------------------------------
// Driver name to be displayed
STATIC
EFI_UNICODE_STRING_TABLE  mDriverNameTable[] = {
  { "eng;en", L"Virtio Input Driver" },
  { NULL,     NULL                   }
};

// -----------------------------------------------------------------------------
// Driver name lookup function
STATIC
EFI_STATUS
EFIAPI
VirtioInputGetDriverName (
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

// -----------------------------------------------------------------------------
// Device name to be displayed
STATIC
EFI_UNICODE_STRING_TABLE  mDeviceNameTable[] = {
  { "eng;en", L"Red Hat Virtio Input device" },
  { NULL,     NULL                           }
};

// -----------------------------------------------------------------------------
STATIC
EFI_COMPONENT_NAME_PROTOCOL  gDeviceName;

// -----------------------------------------------------------------------------
STATIC
EFI_STATUS
EFIAPI
VirtioInputGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   DeviceHandle,
  IN  EFI_HANDLE                   ChildHandle,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDeviceNameTable,
           ControllerName,
           (BOOLEAN)(This == &gDeviceName) // Iso639Language
           );
}

// -----------------------------------------------------------------------------
// General driver UEFI interface for showing driver name
STATIC
EFI_COMPONENT_NAME_PROTOCOL  gComponentName = {
  &VirtioInputGetDriverName,
  &VirtioInputGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

// -----------------------------------------------------------------------------
// General driver UEFI interface for showing driver name
STATIC
EFI_COMPONENT_NAME2_PROTOCOL  gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)&VirtioInputGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)&VirtioInputGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

// -----------------------------------------------------------------------------
// General driver UEFI interface for loading / unloading driver
STATIC EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  &VirtioInputBindingSupported,
  &VirtioInputBindingStart,
  &VirtioInputBindingStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in VirtioInputEntryPoint()
  NULL  // DriverBindingHandle, ditto
};

// -----------------------------------------------------------------------------
// Driver entry point set in INF file, registers all driver functions into UEFI
EFI_STATUS
EFIAPI
VirtioInputEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DEBUG ((DEBUG_INFO, "Virtio input driver has been loaded.......................\n"));
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}

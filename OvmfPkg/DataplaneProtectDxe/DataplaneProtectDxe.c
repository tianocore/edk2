/** @file
  This driver implements EFI_PCI_PLATFORM_PROTOCOL to protect the IVSHMEM device
  during warm resets by restoring its BARs and Command Register.

  Copyright (C) 2024, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/PciPlatform.h>
#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci.h>

//
// IVSHMEM Device Identity
//
#define IVSHMEM_VENDOR_ID 0x1AF4
#define IVSHMEM_DEVICE_ID 0x1110

//
// Custom GUID for our variables
// {6E3C0886-E395-4704-8283-7624114FBD22}
//
EFI_GUID gIvshmemProtectionGuid = { 0x6e3c0886, 0xe395, 0x4704, { 0x82, 0x83, 0x76, 0x24, 0x11, 0x4f, 0xbd, 0x22 } };

#define IVSHMEM_STATE_VAR_NAME       L"IvshmemBarState"
#define DATAPLANE_KEEP_ALIVE_VAR_NAME L"DataplaneKeepAlive"

#pragma pack(1)
typedef struct {
  UINT32 Bar0;
  UINT32 Bar1;
  UINT32 Bar2;
  UINT32 Bar4;
  UINT16 Command;
} IVSHMEM_STATE;
#pragma pack()

//
// Protocol Instance
//
EFI_PCI_PLATFORM_PROTOCOL mPciPlatform;

/**
  Perform the Platform Prep Controller operation.

  @param  This                  The Protocol instance pointer.
  @param  HostBridge            The PCI Root Bridge handle.
  @param  RootBridge            The PCI Root Bridge handle.
  @param  RootBridgePciAddress  The address of the Root Bridge.
  @param  Phase                 The Phase of the controller.
  @param  ExecPhase             The Chipset Entry/Exit.

  @return EFI_SUCCESS           The operation completed successfully.
**/
EFI_STATUS
EFIAPI
PlatformPrepController (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_HANDLE                                     RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS    RootBridgePciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ExecPhase
  )
{
  return EFI_SUCCESS;
}

/**
  Perform the Platform Policy operation.

  @param  This                  The Protocol instance pointer.
  @param  PciPolicy             The PCI Policy.

  @return EFI_UNSUPPORTED       The operation is not supported.
**/
EFI_STATUS
EFIAPI
GetPlatformPolicy (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL                *This,
  OUT       EFI_PCI_PLATFORM_POLICY                  *PciPolicy
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the PCI ROM.

  @param  This                  The Protocol instance pointer.
  @param  PciHandle             The PCI Handle.
  @param  RomImage              The ROM Image.
  @param  RomSize               The ROM Size.

  @return EFI_UNSUPPORTED       The operation is not supported.
**/
EFI_STATUS
EFIAPI
GetPciRom (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL                *This,
  IN        EFI_HANDLE                               PciHandle,
  OUT       VOID                                     **RomImage,
  OUT       UINTN                                    *RomSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Finds the IVSHMEM device handle.

  @return EFI_HANDLE  The handle of the device, or NULL if not found.
**/
EFI_HANDLE
FindIvshmemDevice (
  VOID
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  EFI_HANDLE           *HandleBuffer;
  UINTN                Index;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT16               VendorId;
  UINT16               DeviceId;
  EFI_HANDLE           ResultHandle = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->OpenProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &VendorId);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &DeviceId);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (VendorId == IVSHMEM_VENDOR_ID && DeviceId == IVSHMEM_DEVICE_ID) {
      ResultHandle = HandleBuffer[Index];
      break;
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
  return ResultHandle;
}

/**
  Reads the IVSHMEM state from the device.

  @param  PciIo   The PCI IO protocol instance.
  @param  State   The buffer to store the state.

  @return EFI_SUCCESS or error.
**/
EFI_STATUS
ReadIvshmemState (
  IN  EFI_PCI_IO_PROTOCOL *PciIo,
  OUT IVSHMEM_STATE       *State
  )
{
  EFI_STATUS Status;

  ZeroMem (State, sizeof(IVSHMEM_STATE));

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0x10, 1, &State->Bar0); // BAR0
  if (EFI_ERROR (Status)) return Status;

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0x14, 1, &State->Bar1); // BAR1
  if (EFI_ERROR (Status)) return Status;

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0x18, 1, &State->Bar2); // BAR2
  if (EFI_ERROR (Status)) return Status;

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0x20, 1, &State->Bar4); // BAR4
  if (EFI_ERROR (Status)) return Status;

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, PCI_COMMAND_OFFSET, 1, &State->Command);
  if (EFI_ERROR (Status)) return Status;

  return EFI_SUCCESS;
}

/**
  Restores the IVSHMEM state to the device.

  @param  PciIo   The PCI IO protocol instance.
  @param  State   The state to restore.

  @return EFI_SUCCESS or error.
**/
EFI_STATUS
RestoreIvshmemState (
  IN  EFI_PCI_IO_PROTOCOL *PciIo,
  IN  IVSHMEM_STATE       *State
  )
{
  DEBUG ((DEBUG_INFO, "DataplaneProtect: Restoring IVSHMEM State...\n"));
  DEBUG ((DEBUG_INFO, "  BAR0: 0x%08x\n", State->Bar0));
  DEBUG ((DEBUG_INFO, "  BAR1: 0x%08x\n", State->Bar1));
  DEBUG ((DEBUG_INFO, "  BAR2: 0x%08x\n", State->Bar2));
  DEBUG ((DEBUG_INFO, "  CMD : 0x%04x\n", State->Command));

  // Restore BARs
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x10, 1, &State->Bar0);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x14, 1, &State->Bar1);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x18, 1, &State->Bar2);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x20, 1, &State->Bar4);

  // Restore Command Register (Enable Memory + BusMaster)
  UINT16 Command = State->Command;
  if ((Command & (EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER)) != (EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER)) {
      Command |= (EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER);
  }

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, PCI_COMMAND_OFFSET, 1, &Command);

  return EFI_SUCCESS;
}

/**
  Platform Notify.

  @param  This           The Protocol instance pointer.
  @param  HostBridge     The Host Bridge handle.
  @param  Phase          The Phase.
  @param  ExecPhase      The Chipset Entry/Exit.

  @return EFI_SUCCESS    The operation completed successfully.
**/
EFI_STATUS
EFIAPI
PlatformNotify (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ExecPhase
  )
{
  EFI_STATUS          Status;
  EFI_HANDLE          IvshmemHandle;
  EFI_PCI_IO_PROTOCOL *PciIo;
  IVSHMEM_STATE       State;
  UINTN               Size;
  BOOLEAN             KeepAlive = FALSE;

  if (Phase == EfiPciHostBridgeBeginResourceAllocation && ExecPhase == ChipsetEntry) {
    //
    // Check if we need to restore state
    //
    Size = sizeof(BOOLEAN);
    Status = gRT->GetVariable (
                    DATAPLANE_KEEP_ALIVE_VAR_NAME,
                    &gIvshmemProtectionGuid,
                    NULL,
                    &Size,
                    &KeepAlive
                    );

    if (EFI_ERROR (Status) || !KeepAlive) {
      return EFI_SUCCESS;
    }

    IvshmemHandle = FindIvshmemDevice ();
    if (IvshmemHandle == NULL) {
      DEBUG ((DEBUG_WARN, "DataplaneProtect: KeepAlive requested but IVSHMEM device not found.\n"));
      return EFI_SUCCESS;
    }

    //
    // Open PCI IO
    //
    Status = gBS->OpenProtocol (
                    IvshmemHandle,
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    //
    // Get Saved State
    //
    Size = sizeof(IVSHMEM_STATE);
    Status = gRT->GetVariable (
                    IVSHMEM_STATE_VAR_NAME,
                    &gIvshmemProtectionGuid,
                    NULL,
                    &Size,
                    &State
                    );

    if (!EFI_ERROR (Status) && Size == sizeof(IVSHMEM_STATE)) {
       RestoreIvshmemState (PciIo, &State);
    } else {
       DEBUG ((DEBUG_WARN, "DataplaneProtect: KeepAlive requested but no valid state found.\n"));
    }

  } else if (Phase == EfiPciHostBridgeEndResourceAllocation && ExecPhase == ChipsetExit) {
    //
    // Save state for next boot
    //
    IvshmemHandle = FindIvshmemDevice ();
    if (IvshmemHandle == NULL) {
      return EFI_SUCCESS;
    }

    Status = gBS->OpenProtocol (
                    IvshmemHandle,
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    Status = ReadIvshmemState (PciIo, &State);
    if (!EFI_ERROR (Status)) {
      //
      // Save to Non-Volatile variable
      //
      Status = gRT->SetVariable (
                      IVSHMEM_STATE_VAR_NAME,
                      &gIvshmemProtectionGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof(IVSHMEM_STATE),
                      &State
                      );
      DEBUG ((DEBUG_INFO, "DataplaneProtect: Saved IVSHMEM State. Status=%r\n", Status));
    }
  }

  return EFI_SUCCESS;
}


/**
  Entry point for the driver.
**/
EFI_STATUS
EFIAPI
DataplaneProtectEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mPciPlatform.PlatformPrepController = PlatformPrepController;
  mPciPlatform.GetPlatformPolicy      = GetPlatformPolicy;
  mPciPlatform.GetPciRom              = GetPciRom;
  mPciPlatform.PlatformNotify         = PlatformNotify;

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatform
                  );

  return Status;
}

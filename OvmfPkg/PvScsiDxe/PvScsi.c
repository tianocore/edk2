/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  pvscsi devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/PvScsi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Uefi/UefiSpec.h>

#include "PvScsi.h"

//
// Higher versions will be used before lower, 0x10-0xffffffef is the version
// range for IHV (Indie Hardware Vendors)
//
#define PVSCSI_BINDING_VERSION      0x10

//
// Ext SCSI Pass Thru utilities
//

/**
  Writes a 32-bit value into BAR0 using MMIO
**/
STATIC
EFI_STATUS
PvScsiMmioWrite32 (
  IN CONST PVSCSI_DEV   *Dev,
  IN UINT64             Offset,
  IN UINT32             Value
  )
{
  return Dev->PciIo->Mem.Write (
                           Dev->PciIo,
                           EfiPciIoWidthUint32,
                           PCI_BAR_IDX0,
                           Offset,
                           1,   // Count
                           &Value
                           );
}

/**
  Writes multiple words of data into BAR0 using MMIO
**/
STATIC
EFI_STATUS
PvScsiMmioWrite32Multiple (
  IN CONST PVSCSI_DEV   *Dev,
  IN UINT64             Offset,
  IN UINTN              Count,
  IN UINT32             *Words
  )
{
  return Dev->PciIo->Mem.Write (
                           Dev->PciIo,
                           EfiPciIoWidthFifoUint32,
                           PCI_BAR_IDX0,
                           Offset,
                           Count,
                           Words
                           );
}

/**
  Send a PVSCSI command to device.

  @param[in] Dev                    The pvscsi host device.
  @param[in] Cmd                    The command to send to device.
  @param[in] OPTIONAL DescWords     An optional command descriptor (If command
                                    have a descriptor). The descriptor is
                                    provided as an array of UINT32 words and
                                    is must be 32-bit aligned.
  @param[in] DescWordsCount         The number of words in command descriptor.
                                    Caller must specify here 0 if DescWords
                                    is not supplied (It is optional). In that
                                    case, DescWords is ignored.

  @return   Status codes returned by Dev->PciIo->Mem.Write().

**/
STATIC
EFI_STATUS
PvScsiWriteCmdDesc (
  IN CONST PVSCSI_DEV   *Dev,
  IN UINT32             Cmd,
  IN UINT32             *DescWords      OPTIONAL,
  IN UINTN              DescWordsCount
  )
{
  EFI_STATUS Status;

  if (DescWordsCount > PVSCSI_MAX_CMD_DATA_WORDS) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PvScsiMmioWrite32 (Dev, PvScsiRegOffsetCommand, Cmd);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DescWordsCount > 0) {
    return PvScsiMmioWrite32Multiple (
             Dev,
             PvScsiRegOffsetCommandData,
             DescWordsCount,
             DescWords
             );
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PvScsiResetAdapter (
  IN CONST PVSCSI_DEV   *Dev
  )
{
  return PvScsiWriteCmdDesc (Dev, PvScsiCmdAdapterReset, NULL, 0);
}

/**
  Check if Target argument to EXT_SCSI_PASS_THRU.GetNextTarget() and
  EXT_SCSI_PASS_THRU.GetNextTargetLun() is initialized
**/
STATIC
BOOLEAN
IsTargetInitialized (
  IN UINT8                                          *Target
  )
{
  UINTN Idx;

  for (Idx = 0; Idx < TARGET_MAX_BYTES; ++Idx) {
    if (Target[Idx] != 0xFF) {
      return TRUE;
    }
  }
  return FALSE;
}

//
// Ext SCSI Pass Thru
//

STATIC
EFI_STATUS
EFIAPI
PvScsiPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN EFI_EVENT                                      Event    OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target,
  IN OUT UINT64                                     *Lun
  )
{
  UINT8      *TargetPtr;
  UINT8      LastTarget;
  PVSCSI_DEV *Dev;

  if (Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The Target input parameter is unnecessarily a pointer-to-pointer
  //
  TargetPtr = *Target;

  //
  // If target not initialized, return first target & LUN
  //
  if (!IsTargetInitialized (TargetPtr)) {
    ZeroMem (TargetPtr, TARGET_MAX_BYTES);
    *Lun = 0;
    return EFI_SUCCESS;
  }

  //
  // We only use first byte of target identifer
  //
  LastTarget = *TargetPtr;

  //
  // Increment (target, LUN) pair if valid on input
  //
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget || *Lun > Dev->MaxLun) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Lun < Dev->MaxLun) {
    ++*Lun;
    return EFI_SUCCESS;
  }

  if (LastTarget < Dev->MaxTarget) {
    *Lun = 0;
    ++LastTarget;
    *TargetPtr = LastTarget;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                   **DevicePath
  )
{
  UINT8             TargetValue;
  PVSCSI_DEV        *Dev;
  SCSI_DEVICE_PATH  *ScsiDevicePath;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We only use first byte of target identifer
  //
  TargetValue = *Target;

  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (TargetValue > Dev->MaxTarget || Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  ScsiDevicePath = AllocatePool (sizeof (*ScsiDevicePath));
  if (ScsiDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiDevicePath->Header.Type      = MESSAGING_DEVICE_PATH;
  ScsiDevicePath->Header.SubType   = MSG_SCSI_DP;
  ScsiDevicePath->Header.Length[0] = (UINT8)sizeof (*ScsiDevicePath);
  ScsiDevicePath->Header.Length[1] = (UINT8)(sizeof (*ScsiDevicePath) >> 8);
  ScsiDevicePath->Pun              = TargetValue;
  ScsiDevicePath->Lun              = (UINT16)Lun;

  *DevicePath = &ScsiDevicePath->Header;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN EFI_DEVICE_PATH_PROTOCOL                       *DevicePath,
  OUT UINT8                                         **Target,
  OUT UINT64                                        *Lun
  )
{
  SCSI_DEVICE_PATH *ScsiDevicePath;
  PVSCSI_DEV       *Dev;

  if (DevicePath == NULL || Target == NULL || *Target == NULL || Lun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DevicePath->Type    != MESSAGING_DEVICE_PATH ||
      DevicePath->SubType != MSG_SCSI_DP) {
    return EFI_UNSUPPORTED;
  }

  ScsiDevicePath = (SCSI_DEVICE_PATH *)DevicePath;
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (ScsiDevicePath->Pun > Dev->MaxTarget ||
      ScsiDevicePath->Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  //
  // We only use first byte of target identifer
  //
  **Target = (UINT8)ScsiDevicePath->Pun;
  ZeroMem (*Target + 1, TARGET_MAX_BYTES - 1);
  *Lun = ScsiDevicePath->Lun;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target
  )
{
  UINT8      *TargetPtr;
  UINT8      LastTarget;
  PVSCSI_DEV *Dev;

  if (Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The Target input parameter is unnecessarily a pointer-to-pointer
  //
  TargetPtr = *Target;

  //
  // If target not initialized, return first target
  //
  if (!IsTargetInitialized (TargetPtr)) {
    ZeroMem (TargetPtr, TARGET_MAX_BYTES);
    return EFI_SUCCESS;
  }

  //
  // We only use first byte of target identifer
  //
  LastTarget = *TargetPtr;

  //
  // Increment target if valid on input
  //
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  }

  if (LastTarget < Dev->MaxTarget) {
    ++LastTarget;
    *TargetPtr = LastTarget;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
PvScsiSetPciAttributes (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;

  //
  // Backup original PCI Attributes
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationGet,
                         0,
                         &Dev->OriginalPciAttributes
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enable MMIO-Space & Bus-Mastering
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         (EFI_PCI_IO_ATTRIBUTE_MEMORY |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER),
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
PvScsiRestorePciAttributes (
  IN PVSCSI_DEV *Dev
  )
{
  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OriginalPciAttributes,
                NULL
                );
}

STATIC
EFI_STATUS
PvScsiAllocateSharedPages (
  IN PVSCSI_DEV                     *Dev,
  IN UINTN                          Pages,
  OUT VOID                          **HostAddress,
  OUT PVSCSI_DMA_DESC               *DmaDesc
  )
{
  EFI_STATUS Status;
  UINTN      NumberOfBytes;

  Status = Dev->PciIo->AllocateBuffer (
                         Dev->PciIo,
                         AllocateAnyPages,
                         EfiBootServicesData,
                         Pages,
                         HostAddress,
                         EFI_PCI_ATTRIBUTE_MEMORY_CACHED
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NumberOfBytes = EFI_PAGES_TO_SIZE (Pages);
  Status = Dev->PciIo->Map (
                         Dev->PciIo,
                         EfiPciIoOperationBusMasterCommonBuffer,
                         *HostAddress,
                         &NumberOfBytes,
                         &DmaDesc->DeviceAddress,
                         &DmaDesc->Mapping
                         );
  if (EFI_ERROR (Status)) {
    goto FreeBuffer;
  }

  if (NumberOfBytes != EFI_PAGES_TO_SIZE (Pages)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Unmap;
  }

  return EFI_SUCCESS;

Unmap:
  Dev->PciIo->Unmap (Dev->PciIo, DmaDesc->Mapping);

FreeBuffer:
  Dev->PciIo->FreeBuffer (Dev->PciIo, Pages, *HostAddress);

  return Status;
}

STATIC
VOID
PvScsiFreeSharedPages (
  IN PVSCSI_DEV                     *Dev,
  IN UINTN                          Pages,
  IN VOID                           *HostAddress,
  IN PVSCSI_DMA_DESC                *DmaDesc
  )
{
  Dev->PciIo->Unmap (Dev->PciIo, DmaDesc->Mapping);
  Dev->PciIo->FreeBuffer (Dev->PciIo, Pages, HostAddress);
}

STATIC
EFI_STATUS
PvScsiInitRings (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;
  union {
    PVSCSI_CMD_DESC_SETUP_RINGS Cmd;
    UINT32                      Uint32;
  } AlignedCmd;
  PVSCSI_CMD_DESC_SETUP_RINGS *Cmd;

  Cmd = &AlignedCmd.Cmd;

  Status = PvScsiAllocateSharedPages (
             Dev,
             1,
             (VOID **)&Dev->RingDesc.RingState,
             &Dev->RingDesc.RingStateDmaDesc
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ZeroMem (Dev->RingDesc.RingState, EFI_PAGE_SIZE);

  Status = PvScsiAllocateSharedPages (
             Dev,
             1,
             (VOID **)&Dev->RingDesc.RingReqs,
             &Dev->RingDesc.RingReqsDmaDesc
             );
  if (EFI_ERROR (Status)) {
    goto FreeRingState;
  }
  ZeroMem (Dev->RingDesc.RingReqs, EFI_PAGE_SIZE);

  Status = PvScsiAllocateSharedPages (
             Dev,
             1,
             (VOID **)&Dev->RingDesc.RingCmps,
             &Dev->RingDesc.RingCmpsDmaDesc
             );
  if (EFI_ERROR (Status)) {
    goto FreeRingReqs;
  }
  ZeroMem (Dev->RingDesc.RingCmps, EFI_PAGE_SIZE);

  ZeroMem (Cmd, sizeof (*Cmd));
  Cmd->ReqRingNumPages = 1;
  Cmd->CmpRingNumPages = 1;
  Cmd->RingsStatePPN = RShiftU64 (
                         Dev->RingDesc.RingStateDmaDesc.DeviceAddress,
                         EFI_PAGE_SHIFT
                         );
  Cmd->ReqRingPPNs[0] = RShiftU64 (
                          Dev->RingDesc.RingReqsDmaDesc.DeviceAddress,
                          EFI_PAGE_SHIFT
                          );
  Cmd->CmpRingPPNs[0] = RShiftU64 (
                          Dev->RingDesc.RingCmpsDmaDesc.DeviceAddress,
                          EFI_PAGE_SHIFT
                          );

  STATIC_ASSERT (
    sizeof (*Cmd) % sizeof (UINT32) == 0,
    "Cmd must be multiple of 32-bit words"
    );
  Status = PvScsiWriteCmdDesc (
             Dev,
             PvScsiCmdSetupRings,
             (UINT32 *)Cmd,
             sizeof (*Cmd) / sizeof (UINT32)
             );
  if (EFI_ERROR (Status)) {
    goto FreeRingCmps;
  }

  return EFI_SUCCESS;

FreeRingCmps:
  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingCmps,
    &Dev->RingDesc.RingCmpsDmaDesc
    );

FreeRingReqs:
  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingReqs,
    &Dev->RingDesc.RingReqsDmaDesc
    );

FreeRingState:
  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingState,
    &Dev->RingDesc.RingStateDmaDesc
    );

  return Status;
}

STATIC
VOID
PvScsiFreeRings (
  IN OUT PVSCSI_DEV *Dev
  )
{
  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingCmps,
    &Dev->RingDesc.RingCmpsDmaDesc
    );

  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingReqs,
    &Dev->RingDesc.RingReqsDmaDesc
    );

  PvScsiFreeSharedPages (
    Dev,
    1,
    Dev->RingDesc.RingState,
    &Dev->RingDesc.RingStateDmaDesc
    );
}

STATIC
EFI_STATUS
PvScsiInit (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;

  //
  // Init configuration
  //
  Dev->MaxTarget = PcdGet8 (PcdPvScsiMaxTargetLimit);
  Dev->MaxLun = PcdGet8 (PcdPvScsiMaxLunLimit);

  //
  // Set PCI Attributes
  //
  Status = PvScsiSetPciAttributes (Dev);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Reset adapter
  //
  Status = PvScsiResetAdapter (Dev);
  if (EFI_ERROR (Status)) {
    goto RestorePciAttributes;
  }

  //
  // Init PVSCSI rings
  //
  Status = PvScsiInitRings (Dev);
  if (EFI_ERROR (Status)) {
    goto RestorePciAttributes;
  }

  //
  // Allocate DMA communication buffer
  //
  Status = PvScsiAllocateSharedPages (
             Dev,
             EFI_SIZE_TO_PAGES (sizeof (*Dev->DmaBuf)),
             (VOID **)&Dev->DmaBuf,
             &Dev->DmaBufDmaDesc
             );
  if (EFI_ERROR (Status)) {
    goto FreeRings;
  }

  //
  // Populate the exported interface's attributes
  //
  Dev->PassThru.Mode             = &Dev->PassThruMode;
  Dev->PassThru.PassThru         = &PvScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &PvScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath  = &PvScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun     = &PvScsiGetTargetLun;
  Dev->PassThru.ResetChannel     = &PvScsiResetChannel;
  Dev->PassThru.ResetTargetLun   = &PvScsiResetTargetLun;
  Dev->PassThru.GetNextTarget    = &PvScsiGetNextTarget;

  //
  // AdapterId is a target for which no handle will be created during bus scan.
  // Prevent any conflict with real devices.
  //
  Dev->PassThruMode.AdapterId = MAX_UINT32;

  //
  // Set both physical and logical attributes for non-RAID SCSI channel
  //
  Dev->PassThruMode.Attributes = EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
                                 EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  //
  // No restriction on transfer buffer alignment
  //
  Dev->PassThruMode.IoAlign = 0;

  return EFI_SUCCESS;

FreeRings:
  //
  // Reset device to stop device usage of the rings.
  // This is required to safely free the rings.
  //
  PvScsiResetAdapter (Dev);

  PvScsiFreeRings (Dev);

RestorePciAttributes:
  PvScsiRestorePciAttributes (Dev);

  return Status;
}

STATIC
VOID
PvScsiUninit (
  IN OUT PVSCSI_DEV *Dev
  )
{
  //
  // Reset device to:
  // - Make device stop processing all requests.
  // - Stop device usage of the rings.
  //
  // This is required to safely free the DMA communication buffer
  // and the rings.
  //
  PvScsiResetAdapter (Dev);

  //
  // Free DMA communication buffer
  //
  PvScsiFreeSharedPages (
    Dev,
    EFI_SIZE_TO_PAGES (sizeof (*Dev->DmaBuf)),
    Dev->DmaBuf,
    &Dev->DmaBufDmaDesc
    );

  PvScsiFreeRings (Dev);

  PvScsiRestorePciAttributes (Dev);
}

//
// Driver Binding
//

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if ((Pci.Hdr.VendorId != PCI_VENDOR_ID_VMWARE) ||
      (Pci.Hdr.DeviceId != PCI_DEVICE_ID_VMWARE_PVSCSI)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_SUCCESS;

Done:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  PVSCSI_DEV *Dev;
  EFI_STATUS Status;

  Dev = (PVSCSI_DEV *) AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Dev->PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreePvScsi;
  }

  Status = PvScsiInit (Dev);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  //
  // Setup complete, attempt to export the driver instance's PassThru interface
  //
  Dev->Signature = PVSCSI_SIG;
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  return EFI_SUCCESS;

UninitDev:
  PvScsiUninit (Dev);

ClosePciIo:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

FreePvScsi:
  FreePool (Dev);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  PVSCSI_DEV                      *Dev;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  (VOID **)&PassThru,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL // Lookup only
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = PVSCSI_FROM_PASS_THRU (PassThru);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PvScsiUninit (Dev);

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  FreePool (Dev);

  return EFI_SUCCESS;
}

STATIC EFI_DRIVER_BINDING_PROTOCOL mPvScsiDriverBinding = {
  &PvScsiDriverBindingSupported,
  &PvScsiDriverBindingStart,
  &PvScsiDriverBindingStop,
  PVSCSI_BINDING_VERSION,
  NULL, // ImageHandle, filled by EfiLibInstallDriverBindingComponentName2()
  NULL  // DriverBindingHandle, filled as well
};

//
// Component Name
//

STATIC EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"PVSCSI Host Driver" },
  { NULL,     NULL                  }
};

STATIC EFI_COMPONENT_NAME_PROTOCOL mComponentName;

STATIC
EFI_STATUS
EFIAPI
PvScsiGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &mComponentName) // Iso639Language
           );
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC EFI_COMPONENT_NAME_PROTOCOL mComponentName = {
  &PvScsiGetDriverName,
  &PvScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC EFI_COMPONENT_NAME2_PROTOCOL mComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &PvScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &PvScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry Point
//

EFI_STATUS
EFIAPI
PvScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mPvScsiDriverBinding,
           ImageHandle,
           &mComponentName,
           &mComponentName2
           );
}

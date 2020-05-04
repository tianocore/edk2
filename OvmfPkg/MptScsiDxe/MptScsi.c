/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  LSI Fusion MPT SCSI devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/FusionMptScsi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Uefi/UefiSpec.h>

//
// Higher versions will be used before lower, 0x10-0xffffffef is the version
// range for IVH (Indie Hardware Vendors)
//
#define MPT_SCSI_BINDING_VERSION 0x10

//
// Runtime Structures
//

#define MPT_SCSI_DEV_SIGNATURE SIGNATURE_32 ('M','P','T','S')
typedef struct {
  UINT32                          Signature;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL PassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     PassThruMode;
  UINT8                           MaxTarget;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  UINT64                          OriginalPciAttributes;
} MPT_SCSI_DEV;

#define MPT_SCSI_FROM_PASS_THRU(PassThruPtr) \
  CR (PassThruPtr, MPT_SCSI_DEV, PassThru, MPT_SCSI_DEV_SIGNATURE)

//
// Hardware functions
//

STATIC
EFI_STATUS
Out32 (
  IN MPT_SCSI_DEV       *Dev,
  IN UINT32             Addr,
  IN UINT32             Data
  )
{
  return Dev->PciIo->Io.Write (
                          Dev->PciIo,
                          EfiPciIoWidthUint32,
                          PCI_BAR_IDX0,
                          Addr,
                          1,
                          &Data
                          );
}

STATIC
EFI_STATUS
In32 (
  IN  MPT_SCSI_DEV       *Dev,
  IN  UINT32             Addr,
  OUT UINT32             *Data
  )
{
  return Dev->PciIo->Io.Read (
                          Dev->PciIo,
                          EfiPciIoWidthUint32,
                          PCI_BAR_IDX0,
                          Addr,
                          1,
                          Data
                          );
}

STATIC
EFI_STATUS
MptDoorbell (
  IN MPT_SCSI_DEV       *Dev,
  IN UINT8              DoorbellFunc,
  IN UINT8              DoorbellArg
  )
{
  return Out32 (
           Dev,
           MPT_REG_DOORBELL,
           (((UINT32)DoorbellFunc) << 24) | (DoorbellArg << 16)
           );
}

STATIC
EFI_STATUS
MptScsiReset (
  IN MPT_SCSI_DEV       *Dev
  )
{
  EFI_STATUS Status;

  //
  // Reset hardware
  //
  Status = MptDoorbell (Dev, MPT_DOORBELL_RESET, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Mask interrupts
  //
  Status = Out32 (Dev, MPT_REG_IMASK, MPT_IMASK_DOORBELL | MPT_IMASK_REPLY);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Clear interrupt status
  //
  Status = Out32 (Dev, MPT_REG_ISTATUS, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MptScsiInit (
  IN MPT_SCSI_DEV       *Dev
  )
{
  EFI_STATUS                       Status;
  union {
    MPT_IO_CONTROLLER_INIT_REQUEST Data;
    UINT32                         Uint32;
  } AlignedReq;
  MPT_IO_CONTROLLER_INIT_REQUEST   *Req;
  MPT_IO_CONTROLLER_INIT_REPLY     Reply;
  UINT8                            *ReplyBytes;
  UINT32                           ReplyWord;

  Req = &AlignedReq.Data;

  Status = MptScsiReset (Dev);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (Req, sizeof (*Req));
  ZeroMem (&Reply, sizeof (Reply));
  Req->WhoInit = MPT_IOC_WHOINIT_ROM_BIOS;
  Req->Function = MPT_MESSAGE_HDR_FUNCTION_IOC_INIT;
  STATIC_ASSERT (
    FixedPcdGet8 (PcdMptScsiMaxTargetLimit) < 255,
    "Req supports 255 targets only (max target is 254)"
    );
  Req->MaxDevices = Dev->MaxTarget + 1;
  Req->MaxBuses = 1;

  //
  // Send controller init through doorbell
  //
  STATIC_ASSERT (
    sizeof (*Req) % sizeof (UINT32) == 0,
    "Req must be multiple of UINT32"
    );
  STATIC_ASSERT (
    sizeof (*Req) / sizeof (UINT32) <= MAX_UINT8,
    "Req must fit in MAX_UINT8 Dwords"
    );
  Status = MptDoorbell (
             Dev,
             MPT_DOORBELL_HANDSHAKE,
             (UINT8)(sizeof (*Req) / sizeof (UINT32))
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = Dev->PciIo->Io.Write (
                            Dev->PciIo,
                            EfiPciIoWidthFifoUint32,
                            PCI_BAR_IDX0,
                            MPT_REG_DOORBELL,
                            sizeof (*Req) / sizeof (UINT32),
                            Req
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read reply through doorbell
  // Each 32bit (Dword) read produces 16bit (Word) of data
  //
  // The reply is read back to complete the doorbell function but it
  // isn't useful because it doesn't contain relevant data or status
  // codes.
  //
  STATIC_ASSERT (
    sizeof (Reply) % sizeof (UINT16) == 0,
    "Reply must be multiple of UINT16"
    );
  ReplyBytes = (UINT8 *)&Reply;
  while (ReplyBytes != (UINT8 *)(&Reply + 1)) {
    Status = In32 (Dev, MPT_REG_DOORBELL, &ReplyWord);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    CopyMem (ReplyBytes, &ReplyWord, sizeof (UINT16));
    ReplyBytes += sizeof (UINT16);
  }

  //
  // Clear interrupts generated by doorbell reply
  //
  Status = Out32 (Dev, MPT_REG_ISTATUS, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

//
// Ext SCSI Pass Thru
//

STATIC
EFI_STATUS
EFIAPI
MptScsiPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN EFI_EVENT                                      Event     OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

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

STATIC
EFI_STATUS
EFIAPI
MptScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target,
  IN OUT UINT64                                     *Lun
  )
{
  MPT_SCSI_DEV *Dev;

  Dev = MPT_SCSI_FROM_PASS_THRU (This);
  //
  // Currently support only LUN 0, so hardcode it
  //
  if (!IsTargetInitialized (*Target)) {
    ZeroMem (*Target, TARGET_MAX_BYTES);
    *Lun = 0;
  } else if (**Target > Dev->MaxTarget || *Lun > 0) {
    return EFI_INVALID_PARAMETER;
  } else if (**Target < Dev->MaxTarget) {
    //
    // This device interface support 256 targets only, so it's enough to
    // increment the LSB of Target, as it will never overflow.
    //
    **Target += 1;
  } else {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN OUT UINT8                                     **Target
  )
{
  MPT_SCSI_DEV *Dev;

  Dev = MPT_SCSI_FROM_PASS_THRU (This);
  if (!IsTargetInitialized (*Target)) {
    ZeroMem (*Target, TARGET_MAX_BYTES);
  } else if (**Target > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  } else if (**Target < Dev->MaxTarget) {
    //
    // This device interface support 256 targets only, so it's enough to
    // increment the LSB of Target, as it will never overflow.
    //
    **Target += 1;
  } else {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN UINT8                                         *Target,
  IN UINT64                                        Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                  **DevicePath
  )
{
  MPT_SCSI_DEV     *Dev;
  SCSI_DEVICE_PATH *ScsiDevicePath;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // This device support 256 targets only, so it's enough to dereference
  // the LSB of Target.
  //
  Dev = MPT_SCSI_FROM_PASS_THRU (This);
  if (*Target > Dev->MaxTarget || Lun > 0) {
    return EFI_NOT_FOUND;
  }

  ScsiDevicePath = AllocateZeroPool (sizeof (*ScsiDevicePath));
  if (ScsiDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiDevicePath->Header.Type      = MESSAGING_DEVICE_PATH;
  ScsiDevicePath->Header.SubType   = MSG_SCSI_DP;
  ScsiDevicePath->Header.Length[0] = (UINT8)sizeof (*ScsiDevicePath);
  ScsiDevicePath->Header.Length[1] = (UINT8)(sizeof (*ScsiDevicePath) >> 8);
  ScsiDevicePath->Pun              = *Target;
  ScsiDevicePath->Lun              = (UINT16)Lun;

  *DevicePath = &ScsiDevicePath->Header;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiGetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN EFI_DEVICE_PATH_PROTOCOL                      *DevicePath,
  OUT UINT8                                        **Target,
  OUT UINT64                                       *Lun
  )
{
  MPT_SCSI_DEV     *Dev;
  SCSI_DEVICE_PATH *ScsiDevicePath;

  if (DevicePath == NULL ||
      Target == NULL || *Target == NULL || Lun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DevicePath->Type    != MESSAGING_DEVICE_PATH ||
      DevicePath->SubType != MSG_SCSI_DP) {
    return EFI_UNSUPPORTED;
  }

  Dev = MPT_SCSI_FROM_PASS_THRU (This);
  ScsiDevicePath = (SCSI_DEVICE_PATH *)DevicePath;
  if (ScsiDevicePath->Pun > Dev->MaxTarget ||
      ScsiDevicePath->Lun > 0) {
    return EFI_NOT_FOUND;
  }

  ZeroMem (*Target, TARGET_MAX_BYTES);
  //
  // This device support 256 targets only, so it's enough to set the LSB
  // of Target.
  //
  **Target = (UINT8)ScsiDevicePath->Pun;
  *Lun = ScsiDevicePath->Lun;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN UINT8                                         *Target,
  IN UINT64                                        Lun
  )
{
  return EFI_UNSUPPORTED;
}

//
// Driver Binding
//

STATIC
EFI_STATUS
EFIAPI
MptScsiControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
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

  if (Pci.Hdr.VendorId == LSI_LOGIC_PCI_VENDOR_ID &&
      (Pci.Hdr.DeviceId == LSI_53C1030_PCI_DEVICE_ID ||
       Pci.Hdr.DeviceId == LSI_SAS1068_PCI_DEVICE_ID ||
       Pci.Hdr.DeviceId == LSI_SAS1068E_PCI_DEVICE_ID)) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

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
MptScsiControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS           Status;
  MPT_SCSI_DEV         *Dev;

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->Signature = MPT_SCSI_DEV_SIGNATURE;

  Dev->MaxTarget = PcdGet8 (PcdMptScsiMaxTargetLimit);

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Dev->PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreePool;
  }

  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationGet,
                         0,
                         &Dev->OriginalPciAttributes
                         );
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  //
  // Enable I/O Space & Bus-Mastering
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         (EFI_PCI_IO_ATTRIBUTE_IO |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER),
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  //
  // Signal device supports 64-bit DMA addresses
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    //
    // Warn user that device will only be using 32-bit DMA addresses.
    //
    // Note that this does not prevent the device/driver from working
    // and therefore we only warn and continue as usual.
    //
    DEBUG ((
      DEBUG_WARN,
      "%a: failed to enable 64-bit DMA addresses\n",
      __FUNCTION__
      ));
  }

  Status = MptScsiInit (Dev);
  if (EFI_ERROR (Status)) {
    goto RestoreAttributes;
  }

  //
  // Host adapter channel, doesn't exist
  //
  Dev->PassThruMode.AdapterId = MAX_UINT32;
  Dev->PassThruMode.Attributes =
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  Dev->PassThru.Mode = &Dev->PassThruMode;
  Dev->PassThru.PassThru = &MptScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &MptScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath = &MptScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun = &MptScsiGetTargetLun;
  Dev->PassThru.ResetChannel = &MptScsiResetChannel;
  Dev->PassThru.ResetTargetLun = &MptScsiResetTargetLun;
  Dev->PassThru.GetNextTarget = &MptScsiGetNextTarget;

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
  MptScsiReset (Dev);

RestoreAttributes:
  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OriginalPciAttributes,
                NULL
                );

CloseProtocol:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

FreePool:
  FreePool (Dev);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN  EFI_HANDLE                            ControllerHandle,
  IN  UINTN                                 NumberOfChildren,
  IN  EFI_HANDLE                            *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  MPT_SCSI_DEV                    *Dev;

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

  Dev = MPT_SCSI_FROM_PASS_THRU (PassThru);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MptScsiReset (Dev);

  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OriginalPciAttributes,
                NULL
                );

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  FreePool (Dev);

  return Status;
}

STATIC
EFI_DRIVER_BINDING_PROTOCOL mMptScsiDriverBinding = {
  &MptScsiControllerSupported,
  &MptScsiControllerStart,
  &MptScsiControllerStop,
  MPT_SCSI_BINDING_VERSION,
  NULL, // ImageHandle, filled by EfiLibInstallDriverBindingComponentName2
  NULL, // DriverBindingHandle, filled as well
};

//
// Component Name
//

STATIC
EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"LSI Fusion MPT SCSI Driver" },
  { NULL,     NULL                   }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL mComponentName;

EFI_STATUS
EFIAPI
MptScsiGetDriverName (
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

EFI_STATUS
EFIAPI
MptScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL mComponentName = {
  &MptScsiGetDriverName,
  &MptScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL mComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &MptScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &MptScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry Point
//

EFI_STATUS
EFIAPI
MptScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mMptScsiDriverBinding,
           ImageHandle, // The handle to install onto
           &mComponentName,
           &mComponentName2
           );
}

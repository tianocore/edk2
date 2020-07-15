/** @file
  The EmmcDxe driver is used to manage the EMMC device.

  It produces BlockIo, BlockIo2 and StorageSecurity protocols to allow upper layer
  access the EMMC device.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EmmcDxe.h"

//
// EmmcDxe Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gEmmcDxeDriverBinding = {
  EmmcDxeDriverBindingSupported,
  EmmcDxeDriverBindingStart,
  EmmcDxeDriverBindingStop,
  0x10,
  NULL,
  NULL
};

//
// Template for Emmc Partitions.
//
EMMC_PARTITION mEmmcPartitionTemplate = {
  EMMC_PARTITION_SIGNATURE,    // Signature
  FALSE,                       // Enable
  EmmcPartitionUnknown,        // PartitionType
  NULL,                        // Handle
  NULL,                        // DevicePath
  {                            // BlockIo
    EFI_BLOCK_IO_PROTOCOL_REVISION,
    NULL,
    EmmcReset,
    EmmcReadBlocks,
    EmmcWriteBlocks,
    EmmcFlushBlocks
  },
  {                            // BlockIo2
    NULL,
    EmmcResetEx,
    EmmcReadBlocksEx,
    EmmcWriteBlocksEx,
    EmmcFlushBlocksEx
  },
  {                            // BlockMedia
    0,                         // MediaId
    FALSE,                     // RemovableMedia
    TRUE,                      // MediaPresent
    FALSE,                     // LogicPartition
    FALSE,                     // ReadOnly
    FALSE,                     // WritingCache
    0x200,                     // BlockSize
    0,                         // IoAlign
    0                          // LastBlock
  },
  {                            // StorageSecurity
    EmmcSecurityProtocolIn,
    EmmcSecurityProtocolOut
  },
  {                            // EraseBlock
    EFI_ERASE_BLOCK_PROTOCOL_REVISION,
    1,
    EmmcEraseBlocks
  },
  {                            // DiskInfo
    EFI_DISK_INFO_SD_MMC_INTERFACE_GUID,
    EmmcDiskInfoInquiry,
    EmmcDiskInfoIdentify,
    EmmcDiskInfoSenseData,
    EmmcDiskInfoWhichIde
  },
  {
    NULL,
    NULL
  },
  NULL                         // Device
};

/**
  Decode and print EMMC CSD Register content.

  @param[in] Csd           Pointer to EMMC_CSD data structure.

  @retval EFI_SUCCESS      The function completed successfully
**/
EFI_STATUS
DumpCsd (
  IN EMMC_CSD  *Csd
  )
{
  DEBUG((DEBUG_INFO, "== Dump Emmc Csd Register==\n"));
  DEBUG((DEBUG_INFO, "  CSD structure                    0x%x\n", Csd->CsdStructure));
  DEBUG((DEBUG_INFO, "  System specification version     0x%x\n", Csd->SpecVers));
  DEBUG((DEBUG_INFO, "  Data read access-time 1          0x%x\n", Csd->Taac));
  DEBUG((DEBUG_INFO, "  Data read access-time 2          0x%x\n", Csd->Nsac));
  DEBUG((DEBUG_INFO, "  Max. bus clock frequency         0x%x\n", Csd->TranSpeed));
  DEBUG((DEBUG_INFO, "  Device command classes           0x%x\n", Csd->Ccc));
  DEBUG((DEBUG_INFO, "  Max. read data block length      0x%x\n", Csd->ReadBlLen));
  DEBUG((DEBUG_INFO, "  Partial blocks for read allowed  0x%x\n", Csd->ReadBlPartial));
  DEBUG((DEBUG_INFO, "  Write block misalignment         0x%x\n", Csd->WriteBlkMisalign));
  DEBUG((DEBUG_INFO, "  Read block misalignment          0x%x\n", Csd->ReadBlkMisalign));
  DEBUG((DEBUG_INFO, "  DSR implemented                  0x%x\n", Csd->DsrImp));
  DEBUG((DEBUG_INFO, "  Device size                      0x%x\n", Csd->CSizeLow | (Csd->CSizeHigh << 2)));
  DEBUG((DEBUG_INFO, "  Max. read current @ VDD min      0x%x\n", Csd->VddRCurrMin));
  DEBUG((DEBUG_INFO, "  Max. read current @ VDD max      0x%x\n", Csd->VddRCurrMax));
  DEBUG((DEBUG_INFO, "  Max. write current @ VDD min     0x%x\n", Csd->VddWCurrMin));
  DEBUG((DEBUG_INFO, "  Max. write current @ VDD max     0x%x\n", Csd->VddWCurrMax));
  DEBUG((DEBUG_INFO, "  Device size multiplier           0x%x\n", Csd->CSizeMult));
  DEBUG((DEBUG_INFO, "  Erase group size                 0x%x\n", Csd->EraseGrpSize));
  DEBUG((DEBUG_INFO, "  Erase group size multiplier      0x%x\n", Csd->EraseGrpMult));
  DEBUG((DEBUG_INFO, "  Write protect group size         0x%x\n", Csd->WpGrpSize));
  DEBUG((DEBUG_INFO, "  Write protect group enable       0x%x\n", Csd->WpGrpEnable));
  DEBUG((DEBUG_INFO, "  Manufacturer default ECC         0x%x\n", Csd->DefaultEcc));
  DEBUG((DEBUG_INFO, "  Write speed factor               0x%x\n", Csd->R2WFactor));
  DEBUG((DEBUG_INFO, "  Max. write data block length     0x%x\n", Csd->WriteBlLen));
  DEBUG((DEBUG_INFO, "  Partial blocks for write allowed 0x%x\n", Csd->WriteBlPartial));
  DEBUG((DEBUG_INFO, "  Content protection application   0x%x\n", Csd->ContentProtApp));
  DEBUG((DEBUG_INFO, "  File format group                0x%x\n", Csd->FileFormatGrp));
  DEBUG((DEBUG_INFO, "  Copy flag (OTP)                  0x%x\n", Csd->Copy));
  DEBUG((DEBUG_INFO, "  Permanent write protection       0x%x\n", Csd->PermWriteProtect));
  DEBUG((DEBUG_INFO, "  Temporary write protection       0x%x\n", Csd->TmpWriteProtect));
  DEBUG((DEBUG_INFO, "  File format                      0x%x\n", Csd->FileFormat));
  DEBUG((DEBUG_INFO, "  ECC code                         0x%x\n", Csd->Ecc));

  return EFI_SUCCESS;
}

/**
  Decode and print EMMC EXT_CSD Register content.

  @param[in] ExtCsd           Pointer to the EMMC_EXT_CSD data structure.

  @retval EFI_SUCCESS         The function completed successfully
**/
EFI_STATUS
DumpExtCsd (
  IN EMMC_EXT_CSD  *ExtCsd
  )
{
  DEBUG((DEBUG_INFO, "==Dump Emmc ExtCsd Register==\n"));
  DEBUG((DEBUG_INFO, "  Supported Command Sets                 0x%x\n", ExtCsd->CmdSet));
  DEBUG((DEBUG_INFO, "  HPI features                           0x%x\n", ExtCsd->HpiFeatures));
  DEBUG((DEBUG_INFO, "  Background operations support          0x%x\n", ExtCsd->BkOpsSupport));
  DEBUG((DEBUG_INFO, "  Background operations status           0x%x\n", ExtCsd->BkopsStatus));
  DEBUG((DEBUG_INFO, "  Number of correctly programmed sectors 0x%x\n", *((UINT32*)&ExtCsd->CorrectlyPrgSectorsNum[0])));
  DEBUG((DEBUG_INFO, "  Initialization time after partitioning 0x%x\n", ExtCsd->IniTimeoutAp));
  DEBUG((DEBUG_INFO, "  TRIM Multiplier                        0x%x\n", ExtCsd->TrimMult));
  DEBUG((DEBUG_INFO, "  Secure Feature support                 0x%x\n", ExtCsd->SecFeatureSupport));
  DEBUG((DEBUG_INFO, "  Secure Erase Multiplier                0x%x\n", ExtCsd->SecEraseMult));
  DEBUG((DEBUG_INFO, "  Secure TRIM Multiplier                 0x%x\n", ExtCsd->SecTrimMult));
  DEBUG((DEBUG_INFO, "  Boot information                       0x%x\n", ExtCsd->BootInfo));
  DEBUG((DEBUG_INFO, "  Boot partition size                    0x%x\n", ExtCsd->BootSizeMult));
  DEBUG((DEBUG_INFO, "  Access size                            0x%x\n", ExtCsd->AccSize));
  DEBUG((DEBUG_INFO, "  High-capacity erase unit size          0x%x\n", ExtCsd->HcEraseGrpSize));
  DEBUG((DEBUG_INFO, "  High-capacity erase timeout            0x%x\n", ExtCsd->EraseTimeoutMult));
  DEBUG((DEBUG_INFO, "  Reliable write sector count            0x%x\n", ExtCsd->RelWrSecC));
  DEBUG((DEBUG_INFO, "  High-capacity write protect group size 0x%x\n", ExtCsd->HcWpGrpSize));
  DEBUG((DEBUG_INFO, "  Sleep/awake timeout                    0x%x\n", ExtCsd->SATimeout));
  DEBUG((DEBUG_INFO, "  Sector Count                           0x%x\n", *((UINT32*)&ExtCsd->SecCount[0])));
  DEBUG((DEBUG_INFO, "  Partition switching timing             0x%x\n", ExtCsd->PartitionSwitchTime));
  DEBUG((DEBUG_INFO, "  Out-of-interrupt busy timing           0x%x\n", ExtCsd->OutOfInterruptTime));
  DEBUG((DEBUG_INFO, "  I/O Driver Strength                    0x%x\n", ExtCsd->DriverStrength));
  DEBUG((DEBUG_INFO, "  Device type                            0x%x\n", ExtCsd->DeviceType));
  DEBUG((DEBUG_INFO, "  CSD STRUCTURE                          0x%x\n", ExtCsd->CsdStructure));
  DEBUG((DEBUG_INFO, "  Extended CSD revision                  0x%x\n", ExtCsd->ExtCsdRev));
  DEBUG((DEBUG_INFO, "  Command set                            0x%x\n", ExtCsd->CmdSet));
  DEBUG((DEBUG_INFO, "  Command set revision                   0x%x\n", ExtCsd->CmdSetRev));
  DEBUG((DEBUG_INFO, "  Power class                            0x%x\n", ExtCsd->PowerClass));
  DEBUG((DEBUG_INFO, "  High-speed interface timing            0x%x\n", ExtCsd->HsTiming));
  DEBUG((DEBUG_INFO, "  Bus width mode                         0x%x\n", ExtCsd->BusWidth));
  DEBUG((DEBUG_INFO, "  Erased memory content                  0x%x\n", ExtCsd->ErasedMemCont));
  DEBUG((DEBUG_INFO, "  Partition configuration                0x%x\n", ExtCsd->PartitionConfig));
  DEBUG((DEBUG_INFO, "  Boot config protection                 0x%x\n", ExtCsd->BootConfigProt));
  DEBUG((DEBUG_INFO, "  Boot bus Conditions                    0x%x\n", ExtCsd->BootBusConditions));
  DEBUG((DEBUG_INFO, "  High-density erase group definition    0x%x\n", ExtCsd->EraseGroupDef));
  DEBUG((DEBUG_INFO, "  Boot write protection status register  0x%x\n", ExtCsd->BootWpStatus));
  DEBUG((DEBUG_INFO, "  Boot area write protection register    0x%x\n", ExtCsd->BootWp));
  DEBUG((DEBUG_INFO, "  User area write protection register    0x%x\n", ExtCsd->UserWp));
  DEBUG((DEBUG_INFO, "  FW configuration                       0x%x\n", ExtCsd->FwConfig));
  DEBUG((DEBUG_INFO, "  RPMB Size                              0x%x\n", ExtCsd->RpmbSizeMult));
  DEBUG((DEBUG_INFO, "  H/W reset function                     0x%x\n", ExtCsd->RstFunction));
  DEBUG((DEBUG_INFO, "  Partitioning Support                   0x%x\n", ExtCsd->PartitioningSupport));
  DEBUG((DEBUG_INFO, "  Max Enhanced Area Size                 0x%02x%02x%02x\n", \
                        ExtCsd->MaxEnhSizeMult[2], ExtCsd->MaxEnhSizeMult[1], ExtCsd->MaxEnhSizeMult[0]));
  DEBUG((DEBUG_INFO, "  Partitions attribute                   0x%x\n", ExtCsd->PartitionsAttribute));
  DEBUG((DEBUG_INFO, "  Partitioning Setting                   0x%x\n", ExtCsd->PartitionSettingCompleted));
  DEBUG((DEBUG_INFO, "  General Purpose Partition 1 Size       0x%02x%02x%02x\n", \
                        ExtCsd->GpSizeMult[2], ExtCsd->GpSizeMult[1], ExtCsd->GpSizeMult[0]));
  DEBUG((DEBUG_INFO, "  General Purpose Partition 2 Size       0x%02x%02x%02x\n", \
                        ExtCsd->GpSizeMult[5], ExtCsd->GpSizeMult[4], ExtCsd->GpSizeMult[3]));
  DEBUG((DEBUG_INFO, "  General Purpose Partition 3 Size       0x%02x%02x%02x\n", \
                        ExtCsd->GpSizeMult[8], ExtCsd->GpSizeMult[7], ExtCsd->GpSizeMult[6]));
  DEBUG((DEBUG_INFO, "  General Purpose Partition 4 Size       0x%02x%02x%02x\n", \
                        ExtCsd->GpSizeMult[11], ExtCsd->GpSizeMult[10], ExtCsd->GpSizeMult[9]));
  DEBUG((DEBUG_INFO, "  Enhanced User Data Area Size           0x%02x%02x%02x\n", \
                        ExtCsd->EnhSizeMult[2], ExtCsd->EnhSizeMult[1], ExtCsd->EnhSizeMult[0]));
  DEBUG((DEBUG_INFO, "  Enhanced User Data Start Address       0x%x\n", *((UINT32*)&ExtCsd->EnhStartAddr[0])));
  DEBUG((DEBUG_INFO, "  Bad Block Management mode              0x%x\n", ExtCsd->SecBadBlkMgmnt));
  DEBUG((DEBUG_INFO, "  Native sector size                     0x%x\n", ExtCsd->NativeSectorSize));
  DEBUG((DEBUG_INFO, "  Sector size emulation                  0x%x\n", ExtCsd->UseNativeSector));
  DEBUG((DEBUG_INFO, "  Sector size                            0x%x\n", ExtCsd->DataSectorSize));

  return EFI_SUCCESS;
}

/**
  Get EMMC device model name.

  @param[in, out] Device   The pointer to the EMMC_DEVICE data structure.
  @param[in]      Cid      Pointer to EMMC_CID data structure.

  @retval EFI_SUCCESS      The function completed successfully

**/
EFI_STATUS
GetEmmcModelName (
  IN OUT EMMC_DEVICE         *Device,
  IN     EMMC_CID            *Cid
  )
{
  CHAR8  String[EMMC_MODEL_NAME_MAX_LEN];

  ZeroMem (String, sizeof (String));
  CopyMem (String, &Cid->OemId, sizeof (Cid->OemId));
  String[sizeof (Cid->OemId)] = ' ';
  CopyMem (String + sizeof (Cid->OemId) + 1, Cid->ProductName, sizeof (Cid->ProductName));
  String[sizeof (Cid->OemId) + sizeof (Cid->ProductName)] = ' ';
  CopyMem (String + sizeof (Cid->OemId) + sizeof (Cid->ProductName) + 1, Cid->ProductSerialNumber, sizeof (Cid->ProductSerialNumber));

  AsciiStrToUnicodeStrS (String, Device->ModelName, sizeof (Device->ModelName) / sizeof (Device->ModelName[0]));

  return EFI_SUCCESS;
}

/**
  Discover all partitions in the EMMC device.

  @param[in] Device          The pointer to the EMMC_DEVICE data structure.

  @retval EFI_SUCCESS        All the partitions in the device are successfully enumerated.
  @return Others             Some error occurs when enumerating the partitions.

**/
EFI_STATUS
DiscoverAllPartitions (
  IN EMMC_DEVICE             *Device
  )
{
  EFI_STATUS                        Status;
  EMMC_PARTITION                    *Partition;
  EMMC_CSD                          *Csd;
  EMMC_CID                          *Cid;
  EMMC_EXT_CSD                      *ExtCsd;
  UINT8                             Slot;
  UINT64                            Capacity;
  UINT32                            DevStatus;
  UINT8                             Index;
  UINT32                            SecCount;
  UINT32                            GpSizeMult;

  Slot     = Device->Slot;

  Status = EmmcSendStatus (Device, Slot + 1, &DevStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Deselect the device to force it enter stby mode before getting CSD
  // register content.
  // Note here we don't judge return status as some EMMC devices return
  // error but the state has been stby.
  //
  EmmcSelect (Device, 0);

  Status = EmmcSendStatus (Device, Slot + 1, &DevStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Csd    = &Device->Csd;
  Status = EmmcGetCsd (Device, Slot + 1, Csd);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  DumpCsd (Csd);

  if ((Csd->CSizeLow | Csd->CSizeHigh << 2) == 0xFFF) {
    Device->SectorAddressing = TRUE;
  } else {
    Device->SectorAddressing = FALSE;
  }

  Cid    = &Device->Cid;
  Status = EmmcGetCid (Device, Slot + 1, Cid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcSelect (Device, Slot + 1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ExtCsd = &Device->ExtCsd;
  Status = EmmcGetExtCsd (Device, ExtCsd);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  DumpExtCsd (ExtCsd);

  if (ExtCsd->ExtCsdRev < 5) {
    DEBUG ((EFI_D_ERROR, "The EMMC device version is too low, we don't support!!!\n"));
    return EFI_UNSUPPORTED;
  }

  if ((ExtCsd->PartitioningSupport & BIT0) != BIT0) {
    DEBUG ((EFI_D_ERROR, "The EMMC device doesn't support Partition Feature!!!\n"));
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < EMMC_MAX_PARTITIONS; Index++) {
    Partition = &Device->Partition[Index];
    CopyMem (Partition, &mEmmcPartitionTemplate, sizeof (EMMC_PARTITION));
    Partition->Device             = Device;
    InitializeListHead (&Partition->Queue);
    Partition->BlockIo.Media      = &Partition->BlockMedia;
    Partition->BlockIo2.Media     = &Partition->BlockMedia;
    Partition->PartitionType      = Index;
    Partition->BlockMedia.IoAlign = Device->Private->PassThru->IoAlign;
    Partition->BlockMedia.BlockSize      = 0x200;
    Partition->BlockMedia.LastBlock      = 0x00;
    Partition->BlockMedia.RemovableMedia = FALSE;
    Partition->BlockMedia.MediaPresent     = TRUE;
    Partition->BlockMedia.LogicalPartition = FALSE;

    switch (Index) {
      case EmmcPartitionUserData:
        SecCount = *(UINT32*)&ExtCsd->SecCount;
        Capacity = MultU64x32 ((UINT64) SecCount, 0x200);
        break;
      case EmmcPartitionBoot1:
      case EmmcPartitionBoot2:
        Capacity = ExtCsd->BootSizeMult * SIZE_128KB;
        break;
      case EmmcPartitionRPMB:
        Capacity = ExtCsd->RpmbSizeMult * SIZE_128KB;
        break;
      case EmmcPartitionGP1:
        GpSizeMult = (UINT32)(ExtCsd->GpSizeMult[0] | (ExtCsd->GpSizeMult[1] << 8) | (ExtCsd->GpSizeMult[2] << 16));
        Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
        break;
      case EmmcPartitionGP2:
        GpSizeMult = (UINT32)(ExtCsd->GpSizeMult[3] | (ExtCsd->GpSizeMult[4] << 8) | (ExtCsd->GpSizeMult[5] << 16));
        Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
        break;
      case EmmcPartitionGP3:
        GpSizeMult = (UINT32)(ExtCsd->GpSizeMult[6] | (ExtCsd->GpSizeMult[7] << 8) | (ExtCsd->GpSizeMult[8] << 16));
        Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
        break;
      case EmmcPartitionGP4:
        GpSizeMult = (UINT32)(ExtCsd->GpSizeMult[9] | (ExtCsd->GpSizeMult[10] << 8) | (ExtCsd->GpSizeMult[11] << 16));
        Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
        break;
      default:
        ASSERT (FALSE);
        return EFI_INVALID_PARAMETER;
    }

    if (Capacity != 0) {
      Partition->Enable = TRUE;
      Partition->BlockMedia.LastBlock = DivU64x32 (Capacity, Partition->BlockMedia.BlockSize) - 1;
    }

    if ((ExtCsd->EraseGroupDef & BIT0) == 0) {
      if (Csd->WriteBlLen < 9) {
        Partition->EraseBlock.EraseLengthGranularity = 1;
      } else {
        Partition->EraseBlock.EraseLengthGranularity = (Csd->EraseGrpMult + 1) * (Csd->EraseGrpSize + 1) * (1 << (Csd->WriteBlLen - 9));
      }
    } else {
      Partition->EraseBlock.EraseLengthGranularity = 1024 * ExtCsd->HcEraseGrpSize;
    }
  }

  return EFI_SUCCESS;
}

/**
  Install BlkIo, BlkIo2 and Ssp protocols for the specified partition in the EMMC device.

  @param[in] Device          The pointer to the EMMC_DEVICE data structure.
  @param[in] Index           The index of the partition.

  @retval EFI_SUCCESS        The protocols are installed successfully.
  @retval Others             Some error occurs when installing the protocols.

**/
EFI_STATUS
InstallProtocolOnPartition (
  IN EMMC_DEVICE             *Device,
  IN UINT8                   Index
  )
{
  EFI_STATUS                        Status;
  EMMC_PARTITION                    *Partition;
  CONTROLLER_DEVICE_PATH            ControlNode;
  EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath;
  EFI_HANDLE                        DeviceHandle;

  //
  // Build device path
  //
  ParentDevicePath = Device->DevicePath;

  ControlNode.Header.Type    = HARDWARE_DEVICE_PATH;
  ControlNode.Header.SubType = HW_CONTROLLER_DP;
  SetDevicePathNodeLength (&ControlNode.Header, sizeof (CONTROLLER_DEVICE_PATH));
  ControlNode.ControllerNumber = Index;

  DevicePath = AppendDevicePathNode (ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL*)&ControlNode);
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  DeviceHandle = NULL;
  RemainingDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &DeviceHandle);
  if (!EFI_ERROR (Status) && (DeviceHandle != NULL) && IsDevicePathEnd(RemainingDevicePath)) {
    Status = EFI_ALREADY_STARTED;
    goto Error;
  }

  Partition = &Device->Partition[Index];
  Partition->DevicePath = DevicePath;
  if (Partition->Enable) {
    //
    // Install BlkIo/BlkIo2/Ssp for the specified partition
    //
    if (Partition->PartitionType != EmmcPartitionRPMB) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Partition->Handle,
                      &gEfiDevicePathProtocolGuid,
                      Partition->DevicePath,
                      &gEfiBlockIoProtocolGuid,
                      &Partition->BlockIo,
                      &gEfiBlockIo2ProtocolGuid,
                      &Partition->BlockIo2,
                      &gEfiEraseBlockProtocolGuid,
                      &Partition->EraseBlock,
                      &gEfiDiskInfoProtocolGuid,
                      &Partition->DiskInfo,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      if (((Partition->PartitionType == EmmcPartitionUserData) ||
          (Partition->PartitionType == EmmcPartitionBoot1) ||
          (Partition->PartitionType == EmmcPartitionBoot2)) &&
          ((Device->Csd.Ccc & BIT10) != 0)) {
        Status = gBS->InstallProtocolInterface (
                        &Partition->Handle,
                        &gEfiStorageSecurityCommandProtocolGuid,
                        EFI_NATIVE_INTERFACE,
                        &Partition->StorageSecurity
                        );
        if (EFI_ERROR (Status)) {
          gBS->UninstallMultipleProtocolInterfaces (
                 Partition->Handle,
                 &gEfiDevicePathProtocolGuid,
                 Partition->DevicePath,
                 &gEfiBlockIoProtocolGuid,
                 &Partition->BlockIo,
                 &gEfiBlockIo2ProtocolGuid,
                 &Partition->BlockIo2,
                 &gEfiEraseBlockProtocolGuid,
                 &Partition->EraseBlock,
                 &gEfiDiskInfoProtocolGuid,
                 &Partition->DiskInfo,
                 NULL
                 );
          goto Error;
        }
      }

      gBS->OpenProtocol (
             Device->Private->Controller,
             &gEfiSdMmcPassThruProtocolGuid,
             (VOID **) &(Device->Private->PassThru),
             Device->Private->DriverBindingHandle,
             Partition->Handle,
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
    }

  } else {
    Status = EFI_INVALID_PARAMETER;
  }

Error:
  if (EFI_ERROR (Status) && (DevicePath != NULL)) {
    FreePool (DevicePath);
  }

  return Status;
}

/**
  Scan EMMC Bus to discover the device.

  @param[in]  Private             The EMMC driver private data structure.
  @param[in]  Slot                The slot number to check device present.
  @param[in]  RemainingDevicePath The pointer to the remaining device path.

  @retval EFI_SUCCESS             Successfully to discover the device and attach
                                  SdMmcIoProtocol to it.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack
                                  of resources.
  @retval EFI_ALREADY_STARTED     The device was discovered before.
  @retval Others                  Fail to discover the device.

**/
EFI_STATUS
EFIAPI
DiscoverEmmcDevice (
  IN  EMMC_DRIVER_PRIVATE_DATA    *Private,
  IN  UINT8                       Slot,
  IN  EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  EMMC_DEVICE                     *Device;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *RemainingEmmcDevPath;
  EFI_DEV_PATH                    *Node;
  EFI_HANDLE                      DeviceHandle;
  EFI_SD_MMC_PASS_THRU_PROTOCOL   *PassThru;
  UINT8                           Index;

  Device              = NULL;
  DevicePath          = NULL;
  NewDevicePath       = NULL;
  RemainingDevicePath = NULL;
  PassThru = Private->PassThru;
  Device   = &Private->Device[Slot];

  //
  // Build Device Path to check if the EMMC device present at the slot.
  //
  Status = PassThru->BuildDevicePath (
                       PassThru,
                       Slot,
                       &DevicePath
                       );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (DevicePath->SubType != MSG_EMMC_DP) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  NewDevicePath = AppendDevicePathNode (
                    Private->ParentDevicePath,
                    DevicePath
                    );
  if (NewDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  DeviceHandle         = NULL;
  RemainingEmmcDevPath = NewDevicePath;
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingEmmcDevPath, &DeviceHandle);
  //
  // The device path to the EMMC device doesn't exist. It means the corresponding device private data hasn't been initialized.
  //
  if (EFI_ERROR (Status) || (DeviceHandle == NULL) || !IsDevicePathEnd (RemainingEmmcDevPath)) {
    Device->DevicePath = NewDevicePath;
    Device->Slot       = Slot;
    Device->Private    = Private;
    //
    // Expose user area in the Sd memory card to upper layer.
    //
    Status = DiscoverAllPartitions (Device);
    if (EFI_ERROR(Status)) {
      FreePool (NewDevicePath);
      goto Error;
    }

    Status = gBS->InstallProtocolInterface (
                    &Device->Handle,
                    &gEfiDevicePathProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    Device->DevicePath
                    );
    if (EFI_ERROR(Status)) {
      FreePool (NewDevicePath);
      goto Error;
    }

    Device->ControllerNameTable = NULL;
    GetEmmcModelName (Device, &Device->Cid);
    AddUnicodeString2 (
      "eng",
      gEmmcDxeComponentName.SupportedLanguages,
      &Device->ControllerNameTable,
      Device->ModelName,
      TRUE
      );
    AddUnicodeString2 (
      "en",
      gEmmcDxeComponentName2.SupportedLanguages,
      &Device->ControllerNameTable,
      Device->ModelName,
      FALSE
      );
  }

  if (RemainingDevicePath == NULL) {
    //
    // Expose all partitions in the Emmc device to upper layer.
    //
    for (Index = 0; Index < EMMC_MAX_PARTITIONS; Index++) {
      InstallProtocolOnPartition (Device, Index);
    }
  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    //
    // Enumerate the specified partition
    //
    Node = (EFI_DEV_PATH *) RemainingDevicePath;
    if ((DevicePathType (&Node->DevPath) != HARDWARE_DEVICE_PATH) ||
        (DevicePathSubType (&Node->DevPath) != HW_CONTROLLER_DP) ||
        (DevicePathNodeLength (&Node->DevPath) != sizeof (CONTROLLER_DEVICE_PATH))) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    Index = (UINT8)Node->Controller.ControllerNumber;
    if (Index >= EMMC_MAX_PARTITIONS) {
      Status = EFI_INVALID_PARAMETER;
      goto Error;
    }

    Status = InstallProtocolOnPartition (Device, Index);
  }

Error:
  FreePool (DevicePath);

  return Status;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
EmmcDxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN EFI_HANDLE                    Controller,
  IN EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
{
  EFI_STATUS                       Status;
  EFI_DEVICE_PATH_PROTOCOL         *ParentDevicePath;
  EFI_SD_MMC_PASS_THRU_PROTOCOL    *PassThru;
  UINT8                            Slot;

  //
  // Test EFI_SD_MMC_PASS_THRU_PROTOCOL on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSdMmcPassThruProtocolGuid,
                  (VOID**) &PassThru,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Test RemainingDevicePath is valid or not.
  //
  if ((RemainingDevicePath != NULL) && !IsDevicePathEnd (RemainingDevicePath)) {
    Status = PassThru->GetSlotNumber (PassThru, RemainingDevicePath, &Slot);
    if (EFI_ERROR (Status)) {
      //
      // Close the I/O Abstraction(s) used to perform the supported test
      //
      gBS->CloseProtocol (
             Controller,
             &gEfiSdMmcPassThruProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      return Status;
    }
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiSdMmcPassThruProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
EmmcDxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN EFI_HANDLE                    Controller,
  IN EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
{
  EFI_STATUS                       Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL    *PassThru;
  EFI_DEVICE_PATH_PROTOCOL         *ParentDevicePath;
  EMMC_DRIVER_PRIVATE_DATA         *Private;
  UINT8                            Slot;

  Private  = NULL;
  PassThru = NULL;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSdMmcPassThruProtocolGuid,
                  (VOID **) &PassThru,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Check EFI_ALREADY_STARTED to reuse the original EMMC_DRIVER_PRIVATE_DATA.
  //
  if (Status != EFI_ALREADY_STARTED) {
    Private = AllocateZeroPool (sizeof (EMMC_DRIVER_PRIVATE_DATA));
    if (Private == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &ParentDevicePath,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT_EFI_ERROR (Status);
    Private->PassThru            = PassThru;
    Private->Controller          = Controller;
    Private->ParentDevicePath    = ParentDevicePath;
    Private->DriverBindingHandle = This->DriverBindingHandle;

    Status = gBS->InstallProtocolInterface (
                    &Controller,
                    &gEfiCallerIdGuid,
                    EFI_NATIVE_INTERFACE,
                    Private
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **) &Private,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  if (RemainingDevicePath == NULL) {
    Slot = 0xFF;
    while (TRUE) {
      Status = PassThru->GetNextSlot (PassThru, &Slot);
      if (EFI_ERROR (Status)) {
        //
        // Cannot find more legal slots.
        //
        Status = EFI_SUCCESS;
        break;
      }

      Status = DiscoverEmmcDevice (Private, Slot, NULL);
      if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
        break;
      }
    }
  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    Status = PassThru->GetSlotNumber (PassThru, RemainingDevicePath, &Slot);
    if (!EFI_ERROR (Status)) {
      Status = DiscoverEmmcDevice (Private, Slot, NextDevicePathNode (RemainingDevicePath));
    }
  }

Error:
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiSdMmcPassThruProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    if (Private != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
             Controller,
             &gEfiCallerIdGuid,
             Private,
             NULL
             );
      FreePool (Private);
    }
  }
  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
EmmcDxeDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                             Status;
  BOOLEAN                                AllChildrenStopped;
  UINTN                                  Index;
  EFI_DEVICE_PATH_PROTOCOL               *DevicePath;
  EMMC_DRIVER_PRIVATE_DATA               *Private;
  EMMC_DEVICE                            *Device;
  EMMC_PARTITION                         *Partition;
  EFI_BLOCK_IO_PROTOCOL                  *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL                 *BlockIo2;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL  *StorageSecurity;
  LIST_ENTRY                             *Link;
  LIST_ENTRY                             *NextLink;
  EMMC_REQUEST                           *Request;

  BlockIo  = NULL;
  BlockIo2 = NULL;
  if (NumberOfChildren == 0) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **) &Private,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    for (Index = 0; Index < EMMC_MAX_DEVICES; Index++) {
      Device = &Private->Device[Index];
      Status = gBS->OpenProtocol (
                      Device->Handle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID **) &DevicePath,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }
      ASSERT (DevicePath == Device->DevicePath);
      gBS->UninstallProtocolInterface (
             Device->Handle,
             &gEfiDevicePathProtocolGuid,
             DevicePath
             );
      FreePool (Device->DevicePath);
    }

    gBS->UninstallProtocolInterface (
          Controller,
          &gEfiCallerIdGuid,
          Private
          );
    gBS->CloseProtocol (
          Controller,
          &gEfiSdMmcPassThruProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    FreePool (Private);

    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                      ChildHandleBuffer[Index],
                      &gEfiBlockIo2ProtocolGuid,
                      (VOID **) &BlockIo2,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        AllChildrenStopped = FALSE;
        continue;
      }
    }

    if (BlockIo != NULL) {
      Partition = EMMC_PARTITION_DATA_FROM_BLKIO (BlockIo);
    } else {
      ASSERT (BlockIo2 != NULL);
      Partition = EMMC_PARTITION_DATA_FROM_BLKIO2 (BlockIo2);
    }

    for (Link = GetFirstNode (&Partition->Queue);
         !IsNull (&Partition->Queue, Link);
         Link = NextLink) {
      NextLink = GetNextNode (&Partition->Queue, Link);

      RemoveEntryList (Link);
      Request = EMMC_REQUEST_FROM_LINK (Link);

      gBS->CloseEvent (Request->Event);
      Request->Token->TransactionStatus = EFI_ABORTED;

      if (Request->IsEnd) {
        gBS->SignalEvent (Request->Token->Event);
      }

      FreePool (Request);
    }

    //
    // Close the child handle
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiSdMmcPassThruProtocolGuid,
                    This->DriverBindingHandle,
                    ChildHandleBuffer[Index]
                    );

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    Partition->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Partition->BlockIo,
                    &gEfiBlockIo2ProtocolGuid,
                    &Partition->BlockIo2,
                    &gEfiEraseBlockProtocolGuid,
                    &Partition->EraseBlock,
                    &gEfiDiskInfoProtocolGuid,
                    &Partition->DiskInfo,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
      gBS->OpenProtocol (
             Controller,
             &gEfiSdMmcPassThruProtocolGuid,
             (VOID **)&Partition->Device->Private->PassThru,
             This->DriverBindingHandle,
             ChildHandleBuffer[Index],
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
      continue;
    }

    //
    // If Storage Security Command Protocol is installed, then uninstall this protocol.
    //
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiStorageSecurityCommandProtocolGuid,
                    (VOID **) &StorageSecurity,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (!EFI_ERROR (Status)) {
      Status = gBS->UninstallProtocolInterface (
                      ChildHandleBuffer[Index],
                      &gEfiStorageSecurityCommandProtocolGuid,
                      &Partition->StorageSecurity
                      );
      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
          Controller,
          &gEfiSdMmcPassThruProtocolGuid,
          (VOID **) &Partition->Device->Private->PassThru,
          This->DriverBindingHandle,
          ChildHandleBuffer[Index],
          EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
          );
        AllChildrenStopped = FALSE;
        continue;
      }
    }

    FreePool (Partition->DevicePath);
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for module EmmcDxe. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some errors occur when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeEmmcDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gEmmcDxeDriverBinding,
             ImageHandle,
             &gEmmcDxeComponentName,
             &gEmmcDxeComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}


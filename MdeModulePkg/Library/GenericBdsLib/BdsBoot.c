/** @file
  BDS Lib functions which relate with create or process the boot
  option.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalBdsLib.h"

BOOLEAN mEnumBootDevice = FALSE;

///
/// This GUID is used for an EFI Variable that stores the front device pathes
/// for a partial device path that starts with the HD node.
///
EFI_GUID  mHdBootVariablePrivateGuid = { 0xfab7e9e1, 0x39dd, 0x4f2b, { 0x84, 0x8, 0xe2, 0xe, 0x90, 0x6c, 0xb6, 0xde } };



/**
  Boot the legacy system with the boot option

  @param  Option                 The legacy boot option which have BBS device path

  @retval EFI_UNSUPPORTED        There is no legacybios protocol, do not support
                                 legacy boot.
  @retval EFI_STATUS             Return the status of LegacyBios->LegacyBoot ().

**/
EFI_STATUS
BdsLibDoLegacyBoot (
  IN  BDS_COMMON_OPTION           *Option
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    //
    // If no LegacyBios protocol we do not support legacy boot
    //
    return EFI_UNSUPPORTED;
  }
  //
  // Notes: if we seperate the int 19, then we don't need to refresh BBS
  //
  BdsRefreshBbsTableForBoot (Option);

  //
  // Write boot to OS performance data to a file
  //
  PERF_CODE (
    WriteBootToOsPerformanceData ();
  );

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Legacy Boot: %S\n", Option->Description));
  return LegacyBios->LegacyBoot (
                      LegacyBios,
                      (BBS_BBS_DEVICE_PATH *) Option->DevicePath,
                      Option->LoadOptionsSize,
                      Option->LoadOptions
                      );
}


/**
  Process the boot option follow the EFI 1.1 specification and
  special treat the legacy boot option with BBS_DEVICE_PATH.

  @param  Option                 The boot option need to be processed
  @param  DevicePath             The device path which describe where to load the
                                 boot image or the legcy BBS device path to boot
                                 the legacy OS
  @param  ExitDataSize           Returned directly from gBS->StartImage ()
  @param  ExitData               Returned directly from gBS->StartImage ()

  @retval EFI_SUCCESS            Status from gBS->StartImage ()
  @retval EFI_NOT_FOUND          If the Device Path is not found in the system

**/
EFI_STATUS
EFIAPI
BdsLibBootViaBootOption (
  IN  BDS_COMMON_OPTION             * Option,
  IN  EFI_DEVICE_PATH_PROTOCOL      * DevicePath,
  OUT UINTN                         *ExitDataSize,
  OUT CHAR16                        **ExitData OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  EFI_DEVICE_PATH_PROTOCOL  *WorkingDevicePath;
  EFI_ACPI_S3_SAVE_PROTOCOL *AcpiS3Save;
  LIST_ENTRY                TempBootLists;

  //
  // Record the performance data for End of BDS
  //
  PERF_END (0, BDS_TOK, NULL, 0);

  *ExitDataSize = 0;
  *ExitData     = NULL;

  //
  // Notes: put EFI64 ROM Shadow Solution
  //
  EFI64_SHADOW_ALL_LEGACY_ROM ();

  //
  // Notes: this code can be remove after the s3 script table
  // hook on the event EFI_EVENT_SIGNAL_READY_TO_BOOT or
  // EFI_EVENT_SIGNAL_LEGACY_BOOT
  //
  Status = gBS->LocateProtocol (&gEfiAcpiS3SaveProtocolGuid, NULL, (VOID **) &AcpiS3Save);
  if (!EFI_ERROR (Status)) {
    AcpiS3Save->S3Save (AcpiS3Save, NULL);
  }
  //
  // If it's Device Path that starts with a hard drive path, append it with the front part to compose a
  // full device path
  //
  WorkingDevicePath = NULL;
  if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP)) {
    WorkingDevicePath = BdsExpandPartitionPartialDevicePathToFull (
                          (HARDDRIVE_DEVICE_PATH *)DevicePath
                          );
    if (WorkingDevicePath != NULL) {
      DevicePath = WorkingDevicePath;
    }
  }
  //
  // Signal the EFI_EVENT_SIGNAL_READY_TO_BOOT event
  //
  EfiSignalEventReadyToBoot();
  
  
  //
  // Set Boot Current
  //
  gRT->SetVariable (
        L"BootCurrent",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof (UINT16),
        &Option->BootCurrent
        );

  if ((DevicePathType (Option->DevicePath) == BBS_DEVICE_PATH) &&
      (DevicePathSubType (Option->DevicePath) == BBS_BBS_DP)
    ) {
    //
    // Check to see if we should legacy BOOT. If yes then do the legacy boot
    //
    return BdsLibDoLegacyBoot (Option);
  }

  //
  // If the boot option point to Internal FV shell, make sure it is valid
  //
  Status = BdsLibUpdateFvFileDevicePath (&DevicePath, &gEfiShellFileGuid);
  if (!EFI_ERROR(Status)) {
    if (Option->DevicePath != NULL) {
      SafeFreePool(Option->DevicePath);
    }
    Option->DevicePath  = AllocateZeroPool (GetDevicePathSize (DevicePath));
    CopyMem (Option->DevicePath, DevicePath, GetDevicePathSize (DevicePath));
    //
    // Update the shell boot option
    //
    InitializeListHead (&TempBootLists);
    BdsLibRegisterNewOption (&TempBootLists, DevicePath, L"EFI Internal Shell", L"BootOrder");
    
    //
    // free the temporary device path created by BdsLibUpdateFvFileDevicePath()
    //
    SafeFreePool (DevicePath); 
    DevicePath = Option->DevicePath;
  }

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Booting %S\n", Option->Description));

  Status = gBS->LoadImage (
                  TRUE,
                  mBdsImageHandle,
                  DevicePath,
                  NULL,
                  0,
                  &ImageHandle
                  );

  //
  // If we didn't find an image directly, we need to try as if it is a removable device boot opotion
  // and load the image according to the default boot behavior for removable device.
  //
  if (EFI_ERROR (Status)) {
    //
    // check if there is a bootable removable media could be found in this device path ,
    // and get the bootable media handle
    //
    Handle = BdsLibGetBootableHandle(DevicePath);
    if (Handle == NULL) {
       goto Done;
    }
    //
    // Load the default boot file \EFI\BOOT\boot{machinename}.EFI from removable Media
    //  machinename is ia32, ia64, x64, ...
    //
    FilePath = FileDevicePath (Handle, (CONST CHAR16*)PcdGetPtr(PcdDefaultBootFileName));
    if (FilePath != NULL) {
      Status = gBS->LoadImage (
                      TRUE,
                      mBdsImageHandle,
                      FilePath,
                      NULL,
                      0,
                      &ImageHandle
                      );
      if (EFI_ERROR (Status)) {
        //
        // The DevicePath failed, and it's not a valid
        // removable media device.
        //
        goto Done;
      }
    }
  }

  if (EFI_ERROR (Status)) {
    //
    // It there is any error from the Boot attempt exit now.
    //
    goto Done;
  }
  //
  // Provide the image with it's load options
  //
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);
  ASSERT_EFI_ERROR (Status);

  if (Option->LoadOptionsSize != 0) {
    ImageInfo->LoadOptionsSize  = Option->LoadOptionsSize;
    ImageInfo->LoadOptions      = Option->LoadOptions;
  }
  //
  // Before calling the image, enable the Watchdog Timer for
  // the 5 Minute period
  //
  gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

  Status = gBS->StartImage (ImageHandle, ExitDataSize, ExitData);
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Image Return Status = %r\n", Status));

  //
  // Clear the Watchdog Timer after the image returns
  //
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

Done:
  //
  // Clear Boot Current
  //
  gRT->SetVariable (
        L"BootCurrent",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        0,
        &Option->BootCurrent
        );

  return Status;
}


/**
  Expand a device path that starts with a hard drive media device path node to be a
  full device path that includes the full hardware path to the device. We need
  to do this so it can be booted. As an optimizaiton the front match (the part point
  to the partition node. E.g. ACPI() /PCI()/ATA()/Partition() ) is saved in a variable
  so a connect all is not required on every boot. All successful history device path
  which point to partition node (the front part) will be saved.

  @param  HardDriveDevicePath    EFI Device Path to boot, if it starts with a hard
                                 drive media device path.
  @return A Pointer to the full device path or NULL if a valid Hard Drive devic path
          cannot be found.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
BdsExpandPartitionPartialDevicePathToFull (
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  )
{
  EFI_STATUS                Status;
  UINTN                     BlockIoHandleCount;
  EFI_HANDLE                *BlockIoBuffer;
  EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     Index;
  UINTN                     InstanceNum;
  EFI_DEVICE_PATH_PROTOCOL  *CachedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  UINTN                     CachedDevicePathSize;
  BOOLEAN                   DeviceExist;
  BOOLEAN                   NeedAdjust;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  UINTN                     Size;

  FullDevicePath = NULL;
  //
  // Check if there is prestore 'HDDP' variable.
  // If exist, search the front path which point to partition node in the variable instants.
  // If fail to find or 'HDDP' not exist, reconnect all and search in all system
  //
  CachedDevicePath = BdsLibGetVariableAndSize (
                      L"HDDP",
                      &mHdBootVariablePrivateGuid,
                      &CachedDevicePathSize
                      );
                      
  if (CachedDevicePath != NULL) {
    TempNewDevicePath = CachedDevicePath;
    DeviceExist = FALSE;
    NeedAdjust = FALSE;
    do {
      //
      // Check every instance of the variable
      // First, check wheather the instance contain the partition node, which is needed for distinguishing  multi
      // partial partition boot option. Second, check wheather the instance could be connected.
      //
      Instance  = GetNextDevicePathInstance (&TempNewDevicePath, &Size);
      if (MatchPartitionDevicePathNode (Instance, HardDriveDevicePath)) {
        //
        // Connect the device path instance, the device path point to hard drive media device path node
        // e.g. ACPI() /PCI()/ATA()/Partition()
        //
        Status = BdsLibConnectDevicePath (Instance);
        if (!EFI_ERROR (Status)) {
          DeviceExist = TRUE;
          break;
        }
      }
      //
      // Come here means the first instance is not matched
      //
      NeedAdjust = TRUE;
      SafeFreePool(Instance);
    } while (TempNewDevicePath != NULL);

    if (DeviceExist) {
      //
      // Find the matched device path.
      // Append the file path infomration from the boot option and return the fully expanded device path.
      //
      DevicePath     = NextDevicePathNode ((EFI_DEVICE_PATH_PROTOCOL *) HardDriveDevicePath);
      FullDevicePath = AppendDevicePath (Instance, DevicePath);

      //
      // Adjust the 'HDDP' instances sequense if the matched one is not first one.
      //
      if (NeedAdjust) {
        //
        // First delete the matched instance.
        //
        TempNewDevicePath = CachedDevicePath;
        CachedDevicePath  = BdsLibDelPartMatchInstance (CachedDevicePath, Instance );
        SafeFreePool (TempNewDevicePath);
        
        //
        // Second, append the remaining parth after the matched instance
        //
        TempNewDevicePath = CachedDevicePath;
        CachedDevicePath = AppendDevicePathInstance (Instance, CachedDevicePath );
        SafeFreePool (TempNewDevicePath);
        //
        // Save the matching Device Path so we don't need to do a connect all next time
        //
        Status = gRT->SetVariable (
                        L"HDDP",
                        &mHdBootVariablePrivateGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                        GetDevicePathSize (CachedDevicePath),
                        CachedDevicePath
                        );
      }
      
      SafeFreePool (Instance);
      SafeFreePool (CachedDevicePath);
      return FullDevicePath;
    }
  }

  //
  // If we get here we fail to find or 'HDDP' not exist, and now we need
  // to seach all devices in the system for a matched partition
  //
  BdsLibConnectAllDriversToAllControllers ();
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BlockIoHandleCount, &BlockIoBuffer);
  if (EFI_ERROR (Status) || BlockIoHandleCount == 0) {
    //
    // If there was an error or there are no device handles that support
    // the BLOCK_IO Protocol, then return.
    //
    return NULL;
  }
  //
  // Loop through all the device handles that support the BLOCK_IO Protocol
  //
  for (Index = 0; Index < BlockIoHandleCount; Index++) {

    Status = gBS->HandleProtocol (BlockIoBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *) &BlockIoDevicePath);
    if (EFI_ERROR (Status) || BlockIoDevicePath == NULL) {
      continue;
    }

    if (MatchPartitionDevicePathNode (BlockIoDevicePath, HardDriveDevicePath)) {
      //
      // Find the matched partition device path
      //
      DevicePath    = NextDevicePathNode ((EFI_DEVICE_PATH_PROTOCOL *) HardDriveDevicePath);
      FullDevicePath = AppendDevicePath (BlockIoDevicePath, DevicePath);

      //
      // Save the matched patition device path in 'HDDP' variable
      //
      if (CachedDevicePath != NULL) {
        //
        // Save the matched patition device path as first instance of 'HDDP' variable
        //
        if (BdsLibMatchDevicePaths (CachedDevicePath, BlockIoDevicePath)) {
          TempNewDevicePath = CachedDevicePath;
          CachedDevicePath = BdsLibDelPartMatchInstance (CachedDevicePath, BlockIoDevicePath);
          SafeFreePool(TempNewDevicePath);

          TempNewDevicePath = CachedDevicePath;
          CachedDevicePath = AppendDevicePathInstance (BlockIoDevicePath, CachedDevicePath);
          SafeFreePool(TempNewDevicePath);
        } else {
          TempNewDevicePath = CachedDevicePath;
          CachedDevicePath = AppendDevicePathInstance (BlockIoDevicePath, CachedDevicePath);
          SafeFreePool(TempNewDevicePath);
        }
        //
        // Here limit the device path instance number to 12, which is max number for a system support 3 IDE controller
        // If the user try to boot many OS in different HDs or partitions, in theary, the 'HDDP' variable maybe become larger and larger.
        //
        InstanceNum = 0;
        TempNewDevicePath = CachedDevicePath;
        while (!IsDevicePathEnd (TempNewDevicePath)) {
          TempNewDevicePath = NextDevicePathNode (TempNewDevicePath);
          //
          // Parse one instance
          //
          while (!IsDevicePathEndType (TempNewDevicePath)) {
            TempNewDevicePath = NextDevicePathNode (TempNewDevicePath);
          }
          InstanceNum++;
          //
          // If the CachedDevicePath variable contain too much instance, only remain 12 instances.
          //
          if (InstanceNum >= 12) {
            SetDevicePathEndNode (TempNewDevicePath);
            break;
          }
        }
      } else {
        CachedDevicePath = DuplicateDevicePath (BlockIoDevicePath);
      }

      //
      // Save the matching Device Path so we don't need to do a connect all next time
      //
      Status = gRT->SetVariable (
                      L"HDDP",
                      &mHdBootVariablePrivateGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      GetDevicePathSize (CachedDevicePath),
                      CachedDevicePath
                      );

      break;
    }
  }
  
  SafeFreePool (CachedDevicePath);
  SafeFreePool (BlockIoBuffer);
  return FullDevicePath;
}

/**
  Check whether there is a instance in BlockIoDevicePath, which contain multi device path
  instances, has the same partition node with HardDriveDevicePath device path

  @param  BlockIoDevicePath      Multi device path instances which need to check
  @param  HardDriveDevicePath    A device path which starts with a hard drive media
                                 device path.

  @retval TRUE                   There is a matched device path instance FALSE
  @retval FALSE                  There is no matched device path instance

**/
BOOLEAN
EFIAPI
MatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  )
{
  HARDDRIVE_DEVICE_PATH     *TmpHdPath;
  HARDDRIVE_DEVICE_PATH     *TempPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   Match;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoHdDevicePathNode;

  if ((BlockIoDevicePath == NULL) || (HardDriveDevicePath == NULL)) {
    return FALSE;
  }
  
  //
  // Make PreviousDevicePath == the device path node before the end node
  //
  DevicePath              = BlockIoDevicePath;
  BlockIoHdDevicePathNode = NULL;

  //
  // find the partition device path node
  //
  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP)
        ) {
      BlockIoHdDevicePathNode = DevicePath;
      break;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  if (BlockIoHdDevicePathNode == NULL) {
    return FALSE;
  }
  //
  // See if the harddrive device path in blockio matches the orig Hard Drive Node
  //
  TmpHdPath = (HARDDRIVE_DEVICE_PATH *) BlockIoHdDevicePathNode;
  TempPath  = (HARDDRIVE_DEVICE_PATH *) BdsLibUnpackDevicePath ((EFI_DEVICE_PATH_PROTOCOL *) HardDriveDevicePath);
  Match = FALSE;
  
  //
  // Check for the match
  //
  if ((TmpHdPath->MBRType == TempPath->MBRType) &&
      (TmpHdPath->SignatureType == TempPath->SignatureType)) {
    switch (TmpHdPath->SignatureType) {
    case SIGNATURE_TYPE_GUID:
      Match = CompareGuid ((EFI_GUID *)TmpHdPath->Signature, (EFI_GUID *)TempPath->Signature);
      break;
    case SIGNATURE_TYPE_MBR:
      Match = (BOOLEAN)(*((UINT32 *)(&(TmpHdPath->Signature[0]))) == *(UINT32 *)(&(TempPath->Signature[0])));
      break;
    default:
      Match = FALSE;
      break;
    }
  }

  return Match;
}

/**
  Delete the boot option associated with the handle passed in.

  @param  Handle                 The handle which present the device path to create
                                 boot option

  @retval EFI_SUCCESS            Delete the boot option success
  @retval EFI_NOT_FOUND          If the Device Path is not found in the system
  @retval EFI_OUT_OF_RESOURCES   Lack of memory resource
  @retval Other                  Error return value from SetVariable()

**/
EFI_STATUS
BdsLibDeleteOptionFromHandle (
  IN  EFI_HANDLE                 Handle
  )
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINTN                     Index;
  UINT16                    BootOption[BOOT_OPTION_MAX_CHAR];
  UINTN                     DevicePathSize;
  UINTN                     OptionDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *OptionDevicePath;
  UINT8                     *TempPtr;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;

  //
  // Check "BootOrder" variable, if no, means there is no any boot order.
  //
  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (BootOrder == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Convert device handle to device path protocol instance
  //
  DevicePath = DevicePathFromHandle (Handle);
  if (DevicePath == NULL) {
    return EFI_NOT_FOUND;
  }
  DevicePathSize = GetDevicePathSize (DevicePath);

  //
  // Loop all boot order variable and find the matching device path
  //
  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
                      
    if (BootOptionVar == NULL) {
      SafeFreePool (BootOrder);
      return EFI_OUT_OF_RESOURCES;
    }

    TempPtr = BootOptionVar;
    TempPtr += sizeof (UINT32) + sizeof (UINT16);
    TempPtr += StrSize ((CHAR16 *) TempPtr);
    OptionDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
    OptionDevicePathSize = GetDevicePathSize (OptionDevicePath);

    //
    // Check whether the device path match
    //
    if ((OptionDevicePathSize == DevicePathSize) &&
        (CompareMem (DevicePath, OptionDevicePath, DevicePathSize) == 0)) {
      BdsDeleteBootOption (BootOrder[Index], BootOrder, &BootOrderSize);
      SafeFreePool (BootOptionVar);
      break;
    }

    SafeFreePool (BootOptionVar);
    Index++;
  }

  //
  // Adjust number of boot option for "BootOrder" variable.
  //
  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  BootOrderSize,
                  BootOrder
                  );

  SafeFreePool (BootOrder);

  return Status;
}


/**
  Delete all invalid EFI boot options. The probable invalid boot option could
  be Removable media or Network boot device.

  @retval EFI_SUCCESS            Delete all invalid boot option success
  @retval EFI_NOT_FOUND          Variable "BootOrder" is not found
  @retval EFI_OUT_OF_RESOURCES   Lack of memory resource
  @retval Other                  Error return value from SetVariable()

**/
EFI_STATUS
BdsDeleteAllInvalidEfiBootOption (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     Index2;
  UINT16                    BootOption[BOOT_OPTION_MAX_CHAR];
  EFI_DEVICE_PATH_PROTOCOL  *OptionDevicePath;
  UINT8                     *TempPtr;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;

  //
  // Check "BootOrder" variable firstly, this variable hold the number of boot options
  //
  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    return EFI_NOT_FOUND;
  }

  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      SafeFreePool (BootOrder);
      return EFI_OUT_OF_RESOURCES;
    }

    TempPtr = BootOptionVar;
    TempPtr += sizeof (UINT32) + sizeof (UINT16);
    TempPtr += StrSize ((CHAR16 *) TempPtr);
    OptionDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;

    //
    // Skip legacy boot option (BBS boot device)
    //
    if ((DevicePathType (OptionDevicePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (OptionDevicePath) == BBS_BBS_DP)) {
      SafeFreePool (BootOptionVar);
      Index++;
      continue;
    }

    if (!BdsLibIsValidEFIBootOptDevicePath (OptionDevicePath, FALSE)) {
      //
      // Delete this invalid boot option "Boot####"
      //
      Status = gRT->SetVariable (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      0,
                      NULL
                      );
      //
      // Mark this boot option in boot order as deleted
      //
      BootOrder[Index] = 0xffff;
    }

    SafeFreePool (BootOptionVar);
    Index++;
  }

  //
  // Adjust boot order array
  //
  Index2 = 0;
  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    if (BootOrder[Index] != 0xffff) {
      BootOrder[Index2] = BootOrder[Index];
      Index2 ++;
    }
  }
  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  Index2 * sizeof (UINT16),
                  BootOrder
                  );

  SafeFreePool (BootOrder);

  return Status;
}


/**
  This function will enumerate all possible boot device in the system,
  it will only excute once of every boot.

  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options

  @retval EFI_SUCCESS            Finished all the boot device enumerate and create
                                 the boot option base on that boot device

**/
EFI_STATUS
EFIAPI
BdsLibEnumerateAllBootOption (
  IN OUT LIST_ENTRY          *BdsBootOptionList
  )
{
  EFI_STATUS                    Status;
  UINT16                        FloppyNumber;
  UINT16                        CdromNumber;
  UINT16                        UsbNumber;
  UINT16                        MiscNumber;
  UINT16                        NonBlockNumber;
  UINTN                         NumberBlockIoHandles;
  EFI_HANDLE                    *BlockIoHandles;
  EFI_BLOCK_IO_PROTOCOL         *BlkIo;
  UINTN                         Index;
  UINTN                         NumberSimpleNetworkHandles;
  EFI_HANDLE                    *SimpleNetworkHandles;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  UINTN                         DevicePathType;
  CHAR16                        Buffer[40];
  EFI_HANDLE                    *FileSystemHandles;
  UINTN                         NumberFileSystemHandles;
  BOOLEAN                       NeedDelete;
  EFI_IMAGE_DOS_HEADER          DosHeader;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       HdrData;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;

  FloppyNumber  = 0;
  CdromNumber   = 0;
  UsbNumber     = 0;
  MiscNumber    = 0;
  ZeroMem (Buffer, sizeof (Buffer));
  
  //
  // If the boot device enumerate happened, just get the boot
  // device from the boot order variable
  //
  if (mEnumBootDevice) {
    BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
    return EFI_SUCCESS;
  }
  
  //
  // Notes: this dirty code is to get the legacy boot option from the
  // BBS table and create to variable as the EFI boot option, it should
  // be removed after the CSM can provide legacy boot option directly
  //
  REFRESH_LEGACY_BOOT_OPTIONS;

  //
  // Delete invalid boot option
  //
  BdsDeleteAllInvalidEfiBootOption ();
  
  //
  // Parse removable media
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiBlockIoProtocolGuid,
        NULL,
        &NumberBlockIoHandles,
        &BlockIoHandles
        );
        
  for (Index = 0; Index < NumberBlockIoHandles; Index++) {
    Status = gBS->HandleProtocol (
                    BlockIoHandles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
    if (!EFI_ERROR (Status)) {
      if (!BlkIo->Media->RemovableMedia) {
        //
        // skip the non-removable block devices
        //
        continue;
      }
    }
    DevicePath  = DevicePathFromHandle (BlockIoHandles[Index]);
    DevicePathType = BdsGetBootTypeFromDevicePath (DevicePath);

    switch (DevicePathType) {
    case BDS_EFI_ACPI_FLOPPY_BOOT:
      if (FloppyNumber == 0) {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Floppy");
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Floppy %d", FloppyNumber);
      }
      BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
      FloppyNumber++;
      break;

    case BDS_EFI_MESSAGE_ATAPI_BOOT:
      if (CdromNumber == 0) {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI DVD/CDROM");
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI DVD/CDROM %d", CdromNumber);
      }
      BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
      CdromNumber++;
      break;

    case BDS_EFI_MESSAGE_USB_DEVICE_BOOT:
      if (UsbNumber == 0) {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI USB Device");
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI USB Device %d", UsbNumber);
      }
      BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
      UsbNumber++;
      break;

    case BDS_EFI_MESSAGE_SCSI_BOOT:
      if (UsbNumber == 0) {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI SCSI Device");
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI SCSI Device %d", UsbNumber);
      }
      BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
      UsbNumber++;
      break;

    case BDS_EFI_MESSAGE_MISC_BOOT:
      if (MiscNumber == 0) {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Misc Device");
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Misc Device %d", MiscNumber);
      }
      BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
      MiscNumber++;
      break;

    default:
      break;
    }
  }

  if (NumberBlockIoHandles != 0) {
    SafeFreePool (BlockIoHandles);
  }

  //
  // If there is simple file protocol which does not consume block Io protocol, create a boot option for it here.
  //
  NonBlockNumber = 0;
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &NumberFileSystemHandles,
        &FileSystemHandles
        );
  for (Index = 0; Index < NumberFileSystemHandles; Index++) {
    Status = gBS->HandleProtocol (
                    FileSystemHandles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
     if (!EFI_ERROR (Status)) {
      //
      //  Skip if the file system handle supports a BlkIo protocol,
      //
      continue;
    }

    //
    // Do the removable Media thing. \EFI\BOOT\boot{machinename}.EFI
    //  machinename is ia32, ia64, x64, ...
    //
    Hdr.Union = &HdrData;
    NeedDelete = TRUE;
    Status     = BdsLibGetImageHeader (
                   FileSystemHandles[Index],
                   (CHAR16*)PcdGetPtr (PcdDefaultBootFileName),
                   &DosHeader,
                   Hdr
                   );
    if (!EFI_ERROR (Status) &&
        EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Hdr.Pe32->FileHeader.Machine) &&
        Hdr.Pe32->OptionalHeader.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
      NeedDelete = FALSE;
    }

    if (NeedDelete) {
      //
      // No such file or the file is not a EFI application, delete this boot option
      //
      BdsLibDeleteOptionFromHandle (FileSystemHandles[Index]);
    } else {
      if (NonBlockNumber == 0) {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Non-Block Boot Device");
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Non-Block Boot Device %d", NonBlockNumber);
      }
      BdsLibBuildOptionFromHandle (FileSystemHandles[Index], BdsBootOptionList, Buffer);
      NonBlockNumber++;
    }
  }

  if (NumberFileSystemHandles != 0) {
    SafeFreePool (FileSystemHandles);
  }

  //
  // Parse Network Boot Device
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiSimpleNetworkProtocolGuid,
        NULL,
        &NumberSimpleNetworkHandles,
        &SimpleNetworkHandles
        );
  for (Index = 0; Index < NumberSimpleNetworkHandles; Index++) {
    if (Index == 0) {
      UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Network");
    } else {
      UnicodeSPrint (Buffer, sizeof (Buffer), L"EFI Network %d", Index);
    }
    BdsLibBuildOptionFromHandle (SimpleNetworkHandles[Index], BdsBootOptionList, Buffer);
  }

  if (NumberSimpleNetworkHandles != 0) {
    SafeFreePool (SimpleNetworkHandles);
  }

  //
  // Check if we have on flash shell
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiFirmwareVolume2ProtocolGuid,
        NULL,
        &FvHandleCount,
        &FvHandleBuffer
        );
  for (Index = 0; Index < FvHandleCount; Index++) {
    //
    // Only care the dispatched FV. If no dispatch protocol on the FV, it is not dispatched, then skip it.
    //
    Status = gBS->HandleProtocol (
                    FvHandleBuffer[Index],
                    &gEfiFirmwareVolumeDispatchProtocolGuid,
                    (VOID **) &Fv
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }
    
    gBS->HandleProtocol (
          FvHandleBuffer[Index],
          &gEfiFirmwareVolume2ProtocolGuid,
          (VOID **) &Fv
          );

    Status = Fv->ReadFile (
                  Fv,
                  &gEfiShellFileGuid,
                  NULL,
                  &Size,
                  &Type,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (EFI_ERROR (Status)) {
      //
      // Skip if no shell file in the FV
      //
      continue;
    }
    //
    // Build the shell boot option
    //
    BdsLibBuildOptionFromShell (FvHandleBuffer[Index], BdsBootOptionList);
  }

  if (FvHandleCount != 0) {
    SafeFreePool (FvHandleBuffer);
  }
  //
  // Make sure every boot only have one time
  // boot device enumerate
  //
  BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
  mEnumBootDevice = TRUE;

  return EFI_SUCCESS;
}

/**
  Build the boot option with the handle parsed in.

  @param  Handle                 The handle which present the device path to create
                                 boot option
  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options
  @param  String                 Boot option name.

**/
VOID
EFIAPI
BdsLibBuildOptionFromHandle (
  IN  EFI_HANDLE                 Handle,
  IN  LIST_ENTRY                 *BdsBootOptionList,
  IN  CHAR16                     *String
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath  = DevicePathFromHandle (Handle);

  //
  // Create and register new boot option
  //
  BdsLibRegisterNewOption (BdsBootOptionList, DevicePath, String, L"BootOrder");
}


/**
  Build the on flash shell boot option with the handle parsed in.

  @param  Handle                 The handle which present the device path to create
                                 on flash shell boot option
  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options

**/
VOID
EFIAPI
BdsLibBuildOptionFromShell (
  IN EFI_HANDLE                  Handle,
  IN OUT LIST_ENTRY              *BdsBootOptionList
  )
{
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH ShellNode;

  DevicePath = DevicePathFromHandle (Handle);

  //
  // Build the shell device path
  //
  EfiInitializeFwVolDevicepathNode (&ShellNode, &gEfiShellFileGuid);

  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &ShellNode);

  //
  // Create and register the shell boot option
  //
  BdsLibRegisterNewOption (BdsBootOptionList, DevicePath, L"EFI Internal Shell", L"BootOrder");

}

/**
  Boot from the EFI1.1 spec defined "BootNext" variable

**/
VOID
EFIAPI
BdsLibBootNext (
  VOID
  )
{
  UINT16            *BootNext;
  UINTN             BootNextSize;
  CHAR16            Buffer[20];
  BDS_COMMON_OPTION *BootOption;
  LIST_ENTRY        TempList;
  UINTN             ExitDataSize;
  CHAR16            *ExitData;

  //
  // Init the boot option name buffer and temp link list
  //
  InitializeListHead (&TempList);
  ZeroMem (Buffer, sizeof (Buffer));

  BootNext = BdsLibGetVariableAndSize (
              L"BootNext",
              &gEfiGlobalVariableGuid,
              &BootNextSize
              );

  //
  // Clear the boot next variable first
  //
  if (BootNext != NULL) {
    gRT->SetVariable (
          L"BootNext",
          &gEfiGlobalVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          0,
          BootNext
          );

    //
    // Start to build the boot option and try to boot
    //
    UnicodeSPrint (Buffer, sizeof (Buffer), L"Boot%04x", *BootNext);
    BootOption = BdsLibVariableToOption (&TempList, Buffer);
    BdsLibConnectDevicePath (BootOption->DevicePath);
    BdsLibBootViaBootOption (BootOption, BootOption->DevicePath, &ExitDataSize, &ExitData);
  }

}

/**
  Return the bootable media handle.
  First, check the device is connected
  Second, check whether the device path point to a device which support SimpleFileSystemProtocol,
  Third, detect the the default boot file in the Media, and return the removable Media handle.

  @param  DevicePath             Device Path to a  bootable device

  @retval NULL                   The device path points to an EFI bootable Media
  @retval NULL                   The media on the DevicePath is not bootable

**/
EFI_HANDLE
EFIAPI
BdsLibGetBootableHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *UpdatedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DupDevicePath;
  EFI_HANDLE                      Handle;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  VOID                            *Buffer;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;
  UINTN                           Size;
  UINTN                           TempSize;
  EFI_HANDLE                      ReturnHandle;
  EFI_HANDLE                      *SimpleFileSystemHandles;

  UINTN                           NumberSimpleFileSystemHandles;
  UINTN                           Index;
  EFI_IMAGE_DOS_HEADER            DosHeader;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       HdrData;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;

  UpdatedDevicePath = DevicePath;
  
  //
  // Check whether the device is connected
  //
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &UpdatedDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    //
    // Skip the case that the boot option point to a simple file protocol which does not consume block Io protocol,
    //
    Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &UpdatedDevicePath, &Handle);
    if (EFI_ERROR (Status)) {
      //
      // Fail to find the proper BlockIo and simple file protocol, maybe because device not present,  we need to connect it firstly
      //
      UpdatedDevicePath = DevicePath;
      Status            = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &UpdatedDevicePath, &Handle);
      gBS->ConnectController (Handle, NULL, NULL, TRUE);
    }
  } else {
    //
    // Get BlockIo protocal and check removable attribute
    //
    Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
    //
    // Issue a dummy read to the device to check for media change.
    // When the removable media is changed, any Block IO read/write will
    // cause the BlockIo protocol be reinstalled and EFI_MEDIA_CHANGED is
    // returned. After the Block IO protocol is reinstalled, subsequent
    // Block IO read/write will success.
    //
    Buffer = AllocatePool (BlockIo->Media->BlockSize);
    if (Buffer != NULL) {
      BlockIo->ReadBlocks (
               BlockIo,
               BlockIo->Media->MediaId,
               0,
               BlockIo->Media->BlockSize,
               Buffer
               );
      SafeFreePool(Buffer);
    }
  }

  //
  // Detect the the default boot file from removable Media
  //

  //
  // If fail to get bootable handle specified by a USB boot option, the BDS should try to find other bootable device in the same USB bus
  // Try to locate the USB node device path first, if fail then use its previour PCI node to search
  //
  DupDevicePath = DuplicateDevicePath (DevicePath);
  UpdatedDevicePath = DupDevicePath;
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &UpdatedDevicePath, &Handle);
  //
  // if the resulting device path point to a usb node, and the usb node is a dummy node, should only let device path only point to the previous Pci node
  // Acpi()/Pci()/Usb() --> Acpi()/Pci()
  //
  if ((DevicePathType (UpdatedDevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (UpdatedDevicePath) == MSG_USB_DP)) {
    //
    // Remove the usb node, let the device path only point to PCI node
    //
    SetDevicePathEndNode (UpdatedDevicePath);
    UpdatedDevicePath = DupDevicePath;
  } else {
    UpdatedDevicePath = DevicePath;
  }

  //
  // Get the device path size of boot option
  //
  Size = GetDevicePathSize(UpdatedDevicePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL); // minus the end node
  ReturnHandle = NULL;
  gBS->LocateHandleBuffer (
      ByProtocol,
      &gEfiSimpleFileSystemProtocolGuid,
      NULL,
      &NumberSimpleFileSystemHandles,
      &SimpleFileSystemHandles
      );
  for (Index = 0; Index < NumberSimpleFileSystemHandles; Index++) {
    //
    // Get the device path size of SimpleFileSystem handle
    //
    TempDevicePath = DevicePathFromHandle (SimpleFileSystemHandles[Index]);
    TempSize = GetDevicePathSize (TempDevicePath)- sizeof (EFI_DEVICE_PATH_PROTOCOL); // minus the end node
    //
    // Check whether the device path of boot option is part of the  SimpleFileSystem handle's device path
    //
    if (Size <= TempSize && CompareMem (TempDevicePath, UpdatedDevicePath, Size)==0) {
      //
      // Load the default boot file \EFI\BOOT\boot{machinename}.EFI from removable Media
      //  machinename is ia32, ia64, x64, ...
      //
      Hdr.Union = &HdrData;
      Status = BdsLibGetImageHeader (
                 SimpleFileSystemHandles[Index],
                 (CHAR16*)PcdGetPtr(PcdDefaultBootFileName),
                 &DosHeader,
                 Hdr
                 );
      if (!EFI_ERROR (Status) &&
        EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Hdr.Pe32->FileHeader.Machine) &&
        Hdr.Pe32->OptionalHeader.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
        ReturnHandle = SimpleFileSystemHandles[Index];
        break;
      }
    }
  }

  SafeFreePool(DupDevicePath);

  SafeFreePool(SimpleFileSystemHandles);

  return ReturnHandle;
}

/**
  Check to see if the network cable is plugged in. If the DevicePath is not
  connected it will be connected.

  @param  DevicePath             Device Path to check

  @retval TRUE                   DevicePath points to an Network that is connected
  @retval FALSE                  DevicePath does not point to a bootable network

**/
BOOLEAN
BdsLibNetworkBootWithMediaPresent (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *UpdatedDevicePath;
  EFI_HANDLE                      Handle;
  EFI_SIMPLE_NETWORK_PROTOCOL     *Snp;
  BOOLEAN                         MediaPresent;

  MediaPresent = FALSE;

  UpdatedDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiSimpleNetworkProtocolGuid, &UpdatedDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    //
    // Device not present so see if we need to connect it
    //
    Status = BdsLibConnectDevicePath (DevicePath);
    if (!EFI_ERROR (Status)) {
      //
      // This one should work after we did the connect
      //
      Status = gBS->LocateDevicePath (&gEfiSimpleNetworkProtocolGuid, &UpdatedDevicePath, &Handle);
    }
  }

  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (Handle, &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp);
    if (!EFI_ERROR (Status)) {
      if (Snp->Mode->MediaPresentSupported) {
        if (Snp->Mode->State == EfiSimpleNetworkInitialized) {
          //
          // In case some one else is using the SNP check to see if it's connected
          //
          MediaPresent = Snp->Mode->MediaPresent;
        } else {
          //
          // No one is using SNP so we need to Start and Initialize so
          // MediaPresent will be valid.
          //
          Status = Snp->Start (Snp);
          if (!EFI_ERROR (Status)) {
            Status = Snp->Initialize (Snp, 0, 0);
            if (!EFI_ERROR (Status)) {
              MediaPresent = Snp->Mode->MediaPresent;
              Snp->Shutdown (Snp);
            }
            Snp->Stop (Snp);
          }
        }
      } else {
        MediaPresent = TRUE;
      }
    }
  }

  return MediaPresent;
}

/**
  For a bootable Device path, return its boot type.

  @param  DevicePath                      The bootable device Path to check

  @retval BDS_EFI_MEDIA_HD_BOOT           If the device path contains any media deviec path node, it is media boot type
                                          For the floppy node, handle it as media node
  @retval BDS_EFI_MEDIA_CDROM_BOOT        If the device path contains any media deviec path node, it is media boot type
                                          For the floppy node, handle it as media node
  @retval BDS_EFI_ACPI_FLOPPY_BOOT        If the device path contains any media deviec path node, it is media boot type
                                          For the floppy node, handle it as media node
  @retval BDS_EFI_MESSAGE_ATAPI_BOOT      If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  
  @retval BDS_EFI_MESSAGE_SCSI_BOOT       If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  @retval BDS_EFI_MESSAGE_USB_DEVICE_BOOT If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  @retval BDS_EFI_MESSAGE_MISC_BOOT       If the device path not contains any media deviec path node,  and
                                          its last device path node point to a message device path node, it is
  @retval BDS_LEGACY_BBS_BOOT             Legacy boot type
  @retval BDS_EFI_UNSUPPORT               An EFI Removable BlockIO device path not point to a media and message devie,   

**/
UINT32
EFIAPI
BdsGetBootTypeFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  ACPI_HID_DEVICE_PATH          *Acpi;
  EFI_DEVICE_PATH_PROTOCOL      *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *LastDeviceNode;


  if (NULL == DevicePath) {
    return BDS_EFI_UNSUPPORT;
  }

  TempDevicePath = DevicePath;

  while (!IsDevicePathEndType (TempDevicePath)) {
    switch (DevicePathType (TempDevicePath)) {
      case BBS_DEVICE_PATH:
         return BDS_LEGACY_BBS_BOOT;
      case MEDIA_DEVICE_PATH:
        if (DevicePathSubType (TempDevicePath) == MEDIA_HARDDRIVE_DP) {
          return BDS_EFI_MEDIA_HD_BOOT;
        } else if (DevicePathSubType (TempDevicePath) == MEDIA_CDROM_DP) {
          return BDS_EFI_MEDIA_CDROM_BOOT;
        }
        break;
      case ACPI_DEVICE_PATH:
        Acpi = (ACPI_HID_DEVICE_PATH *) TempDevicePath;
        if (EISA_ID_TO_NUM (Acpi->HID) == 0x0604) {
          return BDS_EFI_ACPI_FLOPPY_BOOT;
        }
        break;
      case MESSAGING_DEVICE_PATH:
        //
        // Get the last device path node
        //
        LastDeviceNode = NextDevicePathNode (TempDevicePath);
        if (DevicePathSubType(LastDeviceNode) == MSG_DEVICE_LOGICAL_UNIT_DP) {
          //
          // if the next node type is Device Logical Unit, which specify the Logical Unit Number (LUN),
          // skit it
          //
          LastDeviceNode = NextDevicePathNode (LastDeviceNode);
        }
        //
        // if the device path not only point to driver device, it is not a messaging device path,
        //
        if (!IsDevicePathEndType (LastDeviceNode)) {
          break;        
        }

        if (DevicePathSubType(TempDevicePath) == MSG_ATAPI_DP) {
          return BDS_EFI_MESSAGE_ATAPI_BOOT;
        } else if (DevicePathSubType(TempDevicePath) == MSG_USB_DP) {
          return BDS_EFI_MESSAGE_USB_DEVICE_BOOT;
        } else if (DevicePathSubType(TempDevicePath) == MSG_SCSI_DP) {
          return BDS_EFI_MESSAGE_SCSI_BOOT;
        }
        return BDS_EFI_MESSAGE_MISC_BOOT;
      default:
        break;
    }
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }

  return BDS_EFI_UNSUPPORT;
}

/**
  Check whether the Device path in a boot option point to a valide bootable device,
  And if CheckMedia is true, check the device is ready to boot now.

  @param DevPath        the Device path in a boot option
  @param CheckMedia     if true, check the device is ready to boot now.

  @retval TRUE          the Device path  is valide
  @retval FALSE         the Device path  is invalide .

**/
BOOLEAN
EFIAPI
BdsLibIsValidEFIBootOptDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath,
  IN BOOLEAN                      CheckMedia
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *LastDeviceNode;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;

  TempDevicePath = DevPath;
  LastDeviceNode = DevPath;
  
  //
  // Check if it's a valid boot option for network boot device
  // Only check if there is SimpleNetworkProtocol installed. If yes, that means
  // there is the network card there.
  //
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleNetworkProtocolGuid,
                  &TempDevicePath,
                  &Handle
                  );
  if (EFI_ERROR (Status)) {
    //
    // Device not present so see if we need to connect it
    //
    TempDevicePath = DevPath;
    BdsLibConnectDevicePath (TempDevicePath);
    Status = gBS->LocateDevicePath (
                    &gEfiSimpleNetworkProtocolGuid,
                    &TempDevicePath,
                    &Handle
                    );
  }
  
  if (!EFI_ERROR (Status)) {
    if (CheckMedia) {
      //
      // Test if it is ready to boot now
      //
      if (BdsLibNetworkBootWithMediaPresent(DevPath)) {
        return TRUE;
      }
    } else {
      return TRUE;
    }
  }

  //
  // If the boot option point to a file, it is a valid EFI boot option,
  // and assume it is ready to boot now
  //
  while (!EfiIsDevicePathEnd (TempDevicePath)) {
     LastDeviceNode = TempDevicePath;
     TempDevicePath = EfiNextDevicePathNode (TempDevicePath);
  }
  if ((DevicePathType (LastDeviceNode) == MEDIA_DEVICE_PATH) &&
    (DevicePathSubType (LastDeviceNode) == MEDIA_FILEPATH_DP)) {
    return TRUE;
  }

  //
  // Check if it's a valid boot option for internal Shell
  //
  if (EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LastDeviceNode) != NULL) {
    //
    // If the boot option point to Internal FV shell, make sure it is valid
    //
    TempDevicePath = DevPath; 
    Status = BdsLibUpdateFvFileDevicePath (&TempDevicePath, &gEfiShellFileGuid);
    if (Status == EFI_ALREADY_STARTED) {
      return TRUE;
    } else {
      if (Status == EFI_SUCCESS) {
        SafeFreePool (TempDevicePath); 
      }
      return FALSE;
    }
  }
  
  //
  // If the boot option point to a blockIO device, no matter whether or not it is a removeable device, it is a valid EFI boot option
  //
  TempDevicePath = DevPath;
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &TempDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    //
    // Device not present so see if we need to connect it
    //
    Status = BdsLibConnectDevicePath (DevPath);
    if (!EFI_ERROR (Status)) {
      //
      // Try again to get the Block Io protocol after we did the connect
      //
      TempDevicePath = DevPath;
      Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &TempDevicePath, &Handle);
    }
  }
  
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
    if (!EFI_ERROR (Status)) {
      if (CheckMedia) {
        //
        // Test if it is ready to boot now
        //
        if (BdsLibGetBootableHandle (DevPath) != NULL) {
          return TRUE;
        }
      } else {
        return TRUE;
      }
    }
  } else {
    //
    // if the boot option point to a simple file protocol which does not consume block Io protocol, it is also a valid EFI boot option,
    //
    Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &TempDevicePath, &Handle);
    if (!EFI_ERROR (Status)) {
      if (CheckMedia) {
        //
        // Test if it is ready to boot now
        //
        if (BdsLibGetBootableHandle (DevPath) != NULL) {
          return TRUE;
        }
      } else {
        return TRUE;
      }
    }
  }

  return FALSE;
}


/**
  According to a file guild, check a Fv file device path is valid. If it is invalid,
  try to return the valid device path.
  FV address maybe changes for memory layout adjust from time to time, use this funciton
  could promise the Fv file device path is right.

  @param  DevicePath             on input, the Fv file device path need to check on
                                 output, the updated valid Fv file device path
  @param  FileGuid               the Fv file guild

  @retval EFI_INVALID_PARAMETER  the input DevicePath or FileGuid is invalid
                                 parameter
  @retval EFI_UNSUPPORTED        the input DevicePath does not contain Fv file
                                 guild at all
  @retval EFI_ALREADY_STARTED    the input DevicePath has pointed to Fv file, it is
                                 valid
  @retval EFI_SUCCESS            has successfully updated the invalid DevicePath,
                                 and return the updated device path in DevicePath

**/
EFI_STATUS
EFIAPI
BdsLibUpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      ** DevicePath,
  IN      EFI_GUID                      *FileGuid
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *LastDeviceNode;
  EFI_STATUS                    Status;
  EFI_GUID                      *GuidPoint;
  UINTN                         Index;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  BOOLEAN                       FindFvFile;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FvFileNode;
  EFI_HANDLE                    FoundFvHandle;
  EFI_DEVICE_PATH_PROTOCOL      *NewDevicePath;

  if ((DevicePath == NULL) || (*DevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if (FileGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check whether the device path point to the default the input Fv file
  //
  TempDevicePath = *DevicePath;
  LastDeviceNode = TempDevicePath;
  while (!EfiIsDevicePathEnd (TempDevicePath)) {
     LastDeviceNode = TempDevicePath;
     TempDevicePath = EfiNextDevicePathNode (TempDevicePath);
  }
  GuidPoint = EfiGetNameGuidFromFwVolDevicePathNode (
                (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LastDeviceNode
                );
  if (GuidPoint == NULL) {
    //
    // if this option does not points to a Fv file, just return EFI_UNSUPPORTED
    //
    return EFI_UNSUPPORTED;
  }
  if (!CompareGuid (GuidPoint, FileGuid)) {
    //
    // If the Fv file is not the input file guid, just return EFI_UNSUPPORTED
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Check whether the input Fv file device path is valid
  //
  TempDevicePath = *DevicePath;
  FoundFvHandle = NULL;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &FoundFvHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    FoundFvHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Set FV ReadFile Buffer as NULL, only need to check whether input Fv file exist there
      //
      Status = Fv->ReadFile (
                    Fv,
                    FileGuid,
                    NULL,
                    &Size,
                    &Type,
                    &Attributes,
                    &AuthenticationStatus
                    );
      if (!EFI_ERROR (Status)) {
        return EFI_ALREADY_STARTED;
      }
    }
  }

  //
  // Look for the input wanted FV file in current FV
  // First, try to look for in Bds own FV. Bds and input wanted FV file usually are in the same FV
  //
  FindFvFile = FALSE;
  FoundFvHandle = NULL;
  Status = gBS->HandleProtocol (
             mBdsImageHandle,
             &gEfiLoadedImageProtocolGuid,
             (VOID **) &LoadedImage
             );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );
    if (!EFI_ERROR (Status)) {
      Status = Fv->ReadFile (
                    Fv,
                    FileGuid,
                    NULL,
                    &Size,
                    &Type,
                    &Attributes,
                    &AuthenticationStatus
                    );
      if (!EFI_ERROR (Status)) {
        FindFvFile = TRUE;
        FoundFvHandle = LoadedImage->DeviceHandle;
      }
    }
  }
  //
  // Second, if fail to find, try to enumerate all FV
  //
  if (!FindFvFile) {
    FvHandleBuffer = NULL;
    gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiFirmwareVolume2ProtocolGuid,
          NULL,
          &FvHandleCount,
          &FvHandleBuffer
          );
    for (Index = 0; Index < FvHandleCount; Index++) {
      gBS->HandleProtocol (
            FvHandleBuffer[Index],
            &gEfiFirmwareVolume2ProtocolGuid,
            (VOID **) &Fv
            );

      Status = Fv->ReadFile (
                    Fv,
                    FileGuid,
                    NULL,
                    &Size,
                    &Type,
                    &Attributes,
                    &AuthenticationStatus
                    );
      if (EFI_ERROR (Status)) {
        //
        // Skip if input Fv file not in the FV
        //
        continue;
      }
      FindFvFile = TRUE;
      FoundFvHandle = FvHandleBuffer[Index];
      break;
    }
    
    SafeFreePool (FvHandleBuffer);  
  }

  if (FindFvFile) {
    //
    // Build the shell device path
    //
    NewDevicePath = DevicePathFromHandle (FoundFvHandle);
    EfiInitializeFwVolDevicepathNode (&FvFileNode, FileGuid);
    NewDevicePath = AppendDevicePathNode (NewDevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &FvFileNode);
    *DevicePath = NewDevicePath;
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

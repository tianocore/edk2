/** @file
  HDD password driver which is used to support HDD security feature.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HddPasswordDxe.h"

EFI_GUID   mHddPasswordVendorGuid          = HDD_PASSWORD_CONFIG_GUID;
CHAR16     mHddPasswordVendorStorageName[] = L"HDD_PASSWORD_CONFIG";
LIST_ENTRY mHddPasswordConfigFormList;
UINT32     mNumberOfHddDevices = 0;

EFI_GUID mHddPasswordDeviceInfoGuid = HDD_PASSWORD_DEVICE_INFO_GUID;
BOOLEAN                         mHddPasswordEndOfDxe = FALSE;
HDD_PASSWORD_REQUEST_VARIABLE   *mHddPasswordRequestVariable = NULL;
UINTN                           mHddPasswordRequestVariableSize = 0;

HII_VENDOR_DEVICE_PATH          mHddPasswordHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    HDD_PASSWORD_CONFIG_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};


/**
  Check if the password is full zero.

  @param[in]   Password       Points to the data buffer

  @retval      TRUE           This password string is full zero.
  @retval      FALSE          This password string is not full zero.

**/
BOOLEAN
PasswordIsFullZero (
  IN CHAR8                    *Password
  )
{
  UINTN                       Index;

  for (Index = 0; Index < HDD_PASSWORD_MAX_LENGTH; Index++) {
    if (Password[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Save device info.

  @param[in]       ConfigFormEntry       Points to HDD_PASSWORD_CONFIG_FORM_ENTRY buffer
  @param[in,out]   TempDevInfo           Points to HDD_PASSWORD_DEVICE_INFO buffer

**/
VOID
SaveDeviceInfo (
  IN     HDD_PASSWORD_CONFIG_FORM_ENTRY    *ConfigFormEntry,
  IN OUT HDD_PASSWORD_DEVICE_INFO          *TempDevInfo
  )
{
  TempDevInfo->Device.Bus                = (UINT8) ConfigFormEntry->Bus;
  TempDevInfo->Device.Device             = (UINT8) ConfigFormEntry->Device;
  TempDevInfo->Device.Function           = (UINT8) ConfigFormEntry->Function;
  TempDevInfo->Device.Port               = ConfigFormEntry->Port;
  TempDevInfo->Device.PortMultiplierPort = ConfigFormEntry->PortMultiplierPort;
  CopyMem (TempDevInfo->Password, ConfigFormEntry->Password, HDD_PASSWORD_MAX_LENGTH);
  TempDevInfo->DevicePathLength          = (UINT32) GetDevicePathSize (ConfigFormEntry->DevicePath);
  CopyMem (TempDevInfo->DevicePath, ConfigFormEntry->DevicePath, TempDevInfo->DevicePathLength);
}

/**
  Build HDD password device info and save them to LockBox.

 **/
VOID
BuildHddPasswordDeviceInfo (
  VOID
  )
{
  EFI_STATUS                        Status;
  LIST_ENTRY                        *Entry;
  HDD_PASSWORD_CONFIG_FORM_ENTRY    *ConfigFormEntry;
  HDD_PASSWORD_DEVICE_INFO          *DevInfo;
  HDD_PASSWORD_DEVICE_INFO          *TempDevInfo;
  UINTN                             DevInfoLength;
  UINT8                             DummyData;
  BOOLEAN                           S3InitDevicesExist;
  UINTN                             S3InitDevicesLength;
  EFI_DEVICE_PATH_PROTOCOL          *S3InitDevices;
  EFI_DEVICE_PATH_PROTOCOL          *S3InitDevicesBak;

  //
  // Build HDD password device info and save them to LockBox.
  //
  DevInfoLength = 0;
  BASE_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);

    //
    // 1. Handle device which already set password.
    // 2. When request to send freeze command, driver also needs to handle device
    //    which support security feature.
    //
    if ((!PasswordIsFullZero (ConfigFormEntry->Password)) ||
        ((ConfigFormEntry->IfrData.SecurityStatus.Supported != 0) &&
         (ConfigFormEntry->IfrData.SecurityStatus.Enabled == 0))) {
      DevInfoLength += sizeof (HDD_PASSWORD_DEVICE_INFO) +
                       GetDevicePathSize (ConfigFormEntry->DevicePath);
    }
  }

  if (DevInfoLength == 0) {
    return;
  }

  S3InitDevicesLength = sizeof (DummyData);
  Status = RestoreLockBox (
             &gS3StorageDeviceInitListGuid,
             &DummyData,
             &S3InitDevicesLength
             );
  ASSERT ((Status == EFI_NOT_FOUND) || (Status == EFI_BUFFER_TOO_SMALL));
  if (Status == EFI_NOT_FOUND) {
    S3InitDevices      = NULL;
    S3InitDevicesExist = FALSE;
  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    S3InitDevices = AllocatePool (S3InitDevicesLength);
    ASSERT (S3InitDevices != NULL);

    Status = RestoreLockBox (
               &gS3StorageDeviceInitListGuid,
               S3InitDevices,
               &S3InitDevicesLength
               );
    ASSERT_EFI_ERROR (Status);
    S3InitDevicesExist = TRUE;
  } else {
    return;
  }

  DevInfo = AllocateZeroPool (DevInfoLength);
  ASSERT (DevInfo != NULL);

  TempDevInfo = DevInfo;
  BASE_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);

    if ((!PasswordIsFullZero (ConfigFormEntry->Password)) ||
        ((ConfigFormEntry->IfrData.SecurityStatus.Supported != 0) &&
         (ConfigFormEntry->IfrData.SecurityStatus.Enabled == 0))) {
      SaveDeviceInfo (ConfigFormEntry, TempDevInfo);

      S3InitDevicesBak = S3InitDevices;
      S3InitDevices    = AppendDevicePathInstance (
                           S3InitDevicesBak,
                           ConfigFormEntry->DevicePath
                           );
      if (S3InitDevicesBak != NULL) {
        FreePool (S3InitDevicesBak);
      }
      ASSERT (S3InitDevices != NULL);

      TempDevInfo = (HDD_PASSWORD_DEVICE_INFO *) ((UINTN)TempDevInfo +
                                                  sizeof (HDD_PASSWORD_DEVICE_INFO) +
                                                  TempDevInfo->DevicePathLength);
    }
  }

  Status = SaveLockBox (
             &mHddPasswordDeviceInfoGuid,
             DevInfo,
             DevInfoLength
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (
             &mHddPasswordDeviceInfoGuid,
             LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY
             );
  ASSERT_EFI_ERROR (Status);

  S3InitDevicesLength = GetDevicePathSize (S3InitDevices);
  if (S3InitDevicesExist) {
    Status = UpdateLockBox (
               &gS3StorageDeviceInitListGuid,
               0,
               S3InitDevices,
               S3InitDevicesLength
               );
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = SaveLockBox (
               &gS3StorageDeviceInitListGuid,
               S3InitDevices,
               S3InitDevicesLength
               );
    ASSERT_EFI_ERROR (Status);

    Status = SetLockBoxAttributes (
               &gS3StorageDeviceInitListGuid,
               LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY
               );
    ASSERT_EFI_ERROR (Status);
  }

  ZeroMem (DevInfo, DevInfoLength);
  FreePool (DevInfo);
  FreePool (S3InitDevices);
}

/**
  Send freeze lock cmd through Ata Pass Thru Protocol.

  @param[in] AtaPassThru         The pointer to the ATA_PASS_THRU protocol.
  @param[in] Port                The port number of the ATA device to send the command.
  @param[in] PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                 If there is no port multiplier, then specify 0xFFFF.

  @retval EFI_SUCCESS            Successful to send freeze lock cmd.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to send freeze lock cmd.
  @retval EFI_DEVICE_ERROR       Can not send freeze lock cmd.

**/
EFI_STATUS
FreezeLockDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              *Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;

  if (AtaPassThru == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The 'Asb' field (a pointer to the EFI_ATA_STATUS_BLOCK structure) in
  // EFI_ATA_PASS_THRU_COMMAND_PACKET is required to be aligned specified by
  // the 'IoAlign' field in the EFI_ATA_PASS_THRU_MODE structure. Meanwhile,
  // the structure EFI_ATA_STATUS_BLOCK is composed of only UINT8 fields, so it
  // may not be aligned when allocated on stack for some compilers. Hence, we
  // use the API AllocateAlignedPages to ensure this structure is properly
  // aligned.
  //
  Asb = AllocateAlignedPages (
          EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)),
          AtaPassThru->Mode->IoAlign
          );
  if (Asb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (Asb, sizeof (EFI_ATA_STATUS_BLOCK));
  Acb.AtaCommand    = ATA_SECURITY_FREEZE_LOCK_CMD;
  Acb.AtaDeviceHead = (UINT8) (PortMultiplierPort == 0xFFFF ? 0 : (PortMultiplierPort << 4));

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_NO_DATA_TRANSFER;
  Packet.Asb      = Asb;
  Packet.Acb      = &Acb;
  Packet.Timeout  = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );
  if (!EFI_ERROR (Status) &&
      ((Asb->AtaStatus & ATA_STSREG_ERR) != 0) &&
      ((Asb->AtaError & ATA_ERRREG_ABRT) != 0)) {
    Status = EFI_DEVICE_ERROR;
  }

  FreeAlignedPages (Asb, EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)));

  DEBUG ((DEBUG_INFO, "%a() - %r\n", __FUNCTION__, Status));
  return Status;
}

/**
  Get attached harddisk identify data through Ata Pass Thru Protocol.

  @param[in] AtaPassThru         The pointer to the ATA_PASS_THRU protocol.
  @param[in] Port                The port number of the ATA device to send the command.
  @param[in] PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                 If there is no port multiplier, then specify 0xFFFF.
  @param[in] IdentifyData        The buffer to store identify data.

  @retval EFI_SUCCESS            Successful to get identify data.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to get identify data.
  @retval EFI_DEVICE_ERROR       Can not get identify data.

**/
EFI_STATUS
GetHddDeviceIdentifyData (
  IN  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru,
  IN  UINT16                        Port,
  IN  UINT16                        PortMultiplierPort,
  IN  ATA_IDENTIFY_DATA             *IdentifyData
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              *Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;

  if ((AtaPassThru == NULL) || (IdentifyData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The 'Asb' field (a pointer to the EFI_ATA_STATUS_BLOCK structure) in
  // EFI_ATA_PASS_THRU_COMMAND_PACKET is required to be aligned specified by
  // the 'IoAlign' field in the EFI_ATA_PASS_THRU_MODE structure. Meanwhile,
  // the structure EFI_ATA_STATUS_BLOCK is composed of only UINT8 fields, so it
  // may not be aligned when allocated on stack for some compilers. Hence, we
  // use the API AllocateAlignedPages to ensure this structure is properly
  // aligned.
  //
  Asb = AllocateAlignedPages (
          EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)),
          AtaPassThru->Mode->IoAlign
          );
  if (Asb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (Asb, sizeof (EFI_ATA_STATUS_BLOCK));
  Acb.AtaCommand    = ATA_CMD_IDENTIFY_DRIVE;
  Acb.AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (PortMultiplierPort == 0xFFFF ? 0 : (PortMultiplierPort << 4)));

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet.Asb      = Asb;
  Packet.Acb      = &Acb;
  Packet.InDataBuffer     = IdentifyData;
  Packet.InTransferLength = sizeof (ATA_IDENTIFY_DATA);
  Packet.Timeout          = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );

  FreeAlignedPages (Asb, EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)));

  return Status;
}

/**
  Parse security status according to identify data.

  @param[in] IdentifyData        The buffer to store identify data.
  @param[in, out] IfrData        IFR data to hold security status.

**/
VOID
GetHddPasswordSecurityStatus (
  IN     ATA_IDENTIFY_DATA    *IdentifyData,
  IN OUT HDD_PASSWORD_CONFIG  *IfrData
  )
{
  IfrData->SecurityStatus.Supported = (IdentifyData->command_set_supported_82 & BIT1) ? 1 : 0;
  IfrData->SecurityStatus.Enabled   = (IdentifyData->security_status & BIT1) ? 1 : 0;
  IfrData->SecurityStatus.Locked    = (IdentifyData->security_status & BIT2) ? 1 : 0;
  IfrData->SecurityStatus.Frozen    = (IdentifyData->security_status & BIT3) ? 1 : 0;
  IfrData->SecurityStatus.UserPasswordStatus   = IfrData->SecurityStatus.Enabled;
  IfrData->SecurityStatus.MasterPasswordStatus = IfrData->SecurityStatus.Supported;

  DEBUG ((DEBUG_INFO, "IfrData->SecurityStatus.Supported            = %x\n", IfrData->SecurityStatus.Supported));
  DEBUG ((DEBUG_INFO, "IfrData->SecurityStatus.Enabled              = %x\n", IfrData->SecurityStatus.Enabled));
  DEBUG ((DEBUG_INFO, "IfrData->SecurityStatus.Locked               = %x\n", IfrData->SecurityStatus.Locked));
  DEBUG ((DEBUG_INFO, "IfrData->SecurityStatus.Frozen               = %x\n", IfrData->SecurityStatus.Frozen));
  DEBUG ((DEBUG_INFO, "IfrData->SecurityStatus.UserPasswordStatus   = %x\n", IfrData->SecurityStatus.UserPasswordStatus));
  DEBUG ((DEBUG_INFO, "IfrData->SecurityStatus.MasterPasswordStatus = %x\n", IfrData->SecurityStatus.MasterPasswordStatus));
}

/**
  Notification function of EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  This is a notification function registered on EFI_END_OF_DXE_EVENT_GROUP_GUID event group.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
HddPasswordEndOfDxeEventNotify (
  EFI_EVENT                               Event,
  VOID                                    *Context
  )
{
  LIST_ENTRY                        *Entry;
  HDD_PASSWORD_CONFIG_FORM_ENTRY    *ConfigFormEntry;
  EFI_STATUS                        Status;
  ATA_IDENTIFY_DATA                 IdentifyData;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  mHddPasswordEndOfDxe = TRUE;

  if (mHddPasswordRequestVariable != NULL) {
    //
    // Free the HDD password request variable buffer here
    // as the HDD password requests should have been processed.
    //
    FreePool (mHddPasswordRequestVariable);
    mHddPasswordRequestVariable = NULL;
    mHddPasswordRequestVariableSize = 0;
  }

  //
  // If no any device, return directly.
  //
  if (IsListEmpty (&mHddPasswordConfigFormList)) {
    gBS->CloseEvent (Event);
    return;
  }

  BuildHddPasswordDeviceInfo ();

  //
  // Zero passsword and freeze lock device.
  //
  BASE_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);

    ZeroMem (ConfigFormEntry->Password, HDD_PASSWORD_MAX_LENGTH);

    //
    // Check whether need send freeze lock command.
    // Below device will be froze:
    // 1. Device not enable password.
    // 2. Device enable password and unlocked.
    //
    if ((ConfigFormEntry->IfrData.SecurityStatus.Supported != 0) &&
        (ConfigFormEntry->IfrData.SecurityStatus.Locked == 0) &&
        (ConfigFormEntry->IfrData.SecurityStatus.Frozen == 0)) {
      Status = FreezeLockDevice (ConfigFormEntry->AtaPassThru, ConfigFormEntry->Port, ConfigFormEntry->PortMultiplierPort);
      DEBUG ((DEBUG_INFO, "FreezeLockDevice return %r!\n", Status));
      Status = GetHddDeviceIdentifyData (
                 ConfigFormEntry->AtaPassThru,
                 ConfigFormEntry->Port,
                 ConfigFormEntry->PortMultiplierPort,
                 &IdentifyData
                 );
      GetHddPasswordSecurityStatus (&IdentifyData, &ConfigFormEntry->IfrData);
    }
  }

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));

  gBS->CloseEvent (Event);
}

/**
  Generate Salt value.

  @param[in, out]   SaltValue           Points to the salt buffer, 32 bytes

**/
VOID
GenSalt (
  IN OUT UINT8  *SaltValue
  )
{
  RandomSeed (NULL, 0);
  RandomBytes (SaltValue, PASSWORD_SALT_SIZE);
}

/**
  Hash the data to get credential.

  @param[in]   Buffer         Points to the data buffer
  @param[in]   BufferSize     Buffer size
  @param[in]   SaltValue      Points to the salt buffer, 32 bytes
  @param[out]  Credential     Points to the hashed result

  @retval      TRUE           Hash the data successfully.
  @retval      FALSE          Failed to hash the data.

**/
BOOLEAN
GenerateCredential (
  IN      UINT8               *Buffer,
  IN      UINTN               BufferSize,
  IN      UINT8               *SaltValue,
     OUT  UINT8               *Credential
  )
{
  BOOLEAN                     Status;
  UINTN                       HashSize;
  VOID                        *Hash;
  VOID                        *HashData;

  Hash      = NULL;
  HashData  = NULL;
  Status    = FALSE;

  HashSize = Sha256GetContextSize ();
  Hash     = AllocateZeroPool (HashSize);
  ASSERT (Hash != NULL);
  if (Hash == NULL) {
    goto Done;
  }

  Status = Sha256Init (Hash);
  if (!Status) {
    goto Done;
  }

  HashData = AllocateZeroPool (PASSWORD_SALT_SIZE + BufferSize);
  ASSERT (HashData != NULL);
  if (HashData == NULL) {
    goto Done;
  }

  CopyMem (HashData, SaltValue, PASSWORD_SALT_SIZE);
  CopyMem ((UINT8 *) HashData + PASSWORD_SALT_SIZE, Buffer, BufferSize);

  Status = Sha256Update (Hash, HashData, PASSWORD_SALT_SIZE + BufferSize);
  if (!Status) {
    goto Done;
  }

  Status = Sha256Final (Hash, Credential);

Done:
  if (Hash != NULL) {
    FreePool (Hash);
  }
  if (HashData != NULL) {
    ZeroMem (HashData, PASSWORD_SALT_SIZE + BufferSize);
    FreePool (HashData);
  }
  return Status;
}

/**
  Save HDD password variable that will be used to validate HDD password
  when the device is at frozen state.

  @param[in] ConfigFormEntry        The HDD Password configuration form entry.
  @param[in] Password               The hdd password of attached ATA device.

**/
VOID
SaveHddPasswordVariable (
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry,
  IN CHAR8                          *Password
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_VARIABLE             *TempVariable;
  UINTN                             TempVariableSize;
  HDD_PASSWORD_VARIABLE             *NextNode;
  HDD_PASSWORD_VARIABLE             *Variable;
  UINTN                             VariableSize;
  HDD_PASSWORD_VARIABLE             *NewVariable;
  UINTN                             NewVariableSize;
  BOOLEAN                           Delete;
  BOOLEAN                           HashOk;
  UINT8                             HashData[SHA256_DIGEST_SIZE];
  UINT8                             SaltData[PASSWORD_SALT_SIZE];

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  Delete = FALSE;
  if (!PasswordIsFullZero (Password)) {
    //
    // It is Set/Update HDD Password.
    //
    ZeroMem (HashData, sizeof (HashData));
    ZeroMem (SaltData, sizeof (SaltData));
    GenSalt (SaltData);
    HashOk = GenerateCredential ((UINT8 *) Password, HDD_PASSWORD_MAX_LENGTH, SaltData, HashData);
    if (!HashOk) {
      DEBUG ((DEBUG_INFO, "GenerateCredential failed\n"));
      return;
    }
  } else {
    //
    // It is Disable HDD Password.
    // Go to delete the variable node for the HDD password device.
    //
    Delete = TRUE;
  }

  Variable = NULL;
  VariableSize = 0;
  NewVariable = NULL;
  NewVariableSize = 0;

  Status = GetVariable2 (
             HDD_PASSWORD_VARIABLE_NAME,
             &mHddPasswordVendorGuid,
             (VOID **) &Variable,
             &VariableSize
             );
  if (Delete) {
    if (!EFI_ERROR (Status) && (Variable != NULL)) {
      TempVariable = Variable;
      TempVariableSize = VariableSize;
      while (TempVariableSize >= sizeof (HDD_PASSWORD_VARIABLE)) {
        if ((TempVariable->Device.Bus                == ConfigFormEntry->Bus) &&
            (TempVariable->Device.Device             == ConfigFormEntry->Device) &&
            (TempVariable->Device.Function           == ConfigFormEntry->Function) &&
            (TempVariable->Device.Port               == ConfigFormEntry->Port) &&
            (TempVariable->Device.PortMultiplierPort == ConfigFormEntry->PortMultiplierPort)) {
          //
          // Found the node for the HDD password device.
          // Delete the node.
          //
          NextNode = TempVariable + 1;
          CopyMem (TempVariable, NextNode, (UINTN) Variable + VariableSize - (UINTN) NextNode);
          NewVariable = Variable;
          NewVariableSize = VariableSize - sizeof (HDD_PASSWORD_VARIABLE);
          break;
        }
        TempVariableSize -= sizeof (HDD_PASSWORD_VARIABLE);
        TempVariable += 1;
      }
      if (NewVariable == NULL) {
        DEBUG ((DEBUG_INFO, "The variable node for the HDD password device is not found\n"));
      }
    } else {
      DEBUG ((DEBUG_INFO, "HddPassword variable get failed (%r)\n", Status));
    }
  } else {
    if (!EFI_ERROR (Status) && (Variable != NULL)) {
      TempVariable = Variable;
      TempVariableSize = VariableSize;
      while (TempVariableSize >= sizeof (HDD_PASSWORD_VARIABLE)) {
        if ((TempVariable->Device.Bus                == ConfigFormEntry->Bus) &&
            (TempVariable->Device.Device             == ConfigFormEntry->Device) &&
            (TempVariable->Device.Function           == ConfigFormEntry->Function) &&
            (TempVariable->Device.Port               == ConfigFormEntry->Port) &&
            (TempVariable->Device.PortMultiplierPort == ConfigFormEntry->PortMultiplierPort)) {
          //
          // Found the node for the HDD password device.
          // Update the node.
          //
          CopyMem (TempVariable->PasswordHash, HashData, sizeof (HashData));
          CopyMem (TempVariable->PasswordSalt, SaltData, sizeof (SaltData));
          NewVariable = Variable;
          NewVariableSize = VariableSize;
          break;
        }
        TempVariableSize -= sizeof (HDD_PASSWORD_VARIABLE);
        TempVariable += 1;
      }
      if (NewVariable == NULL) {
        //
        // The node for the HDD password device is not found.
        // Create node for the HDD password device.
        //
        NewVariableSize = VariableSize + sizeof (HDD_PASSWORD_VARIABLE);
        NewVariable = AllocateZeroPool (NewVariableSize);
        ASSERT (NewVariable != NULL);
        CopyMem (NewVariable, Variable, VariableSize);
        TempVariable = (HDD_PASSWORD_VARIABLE *) ((UINTN) NewVariable + VariableSize);
        TempVariable->Device.Bus                = (UINT8) ConfigFormEntry->Bus;
        TempVariable->Device.Device             = (UINT8) ConfigFormEntry->Device;
        TempVariable->Device.Function           = (UINT8) ConfigFormEntry->Function;
        TempVariable->Device.Port               = ConfigFormEntry->Port;
        TempVariable->Device.PortMultiplierPort = ConfigFormEntry->PortMultiplierPort;
        CopyMem (TempVariable->PasswordHash, HashData, sizeof (HashData));
        CopyMem (TempVariable->PasswordSalt, SaltData, sizeof (SaltData));
      }
    } else {
      NewVariableSize = sizeof (HDD_PASSWORD_VARIABLE);
      NewVariable = AllocateZeroPool (NewVariableSize);
      ASSERT (NewVariable != NULL);
      NewVariable->Device.Bus                = (UINT8) ConfigFormEntry->Bus;
      NewVariable->Device.Device             = (UINT8) ConfigFormEntry->Device;
      NewVariable->Device.Function           = (UINT8) ConfigFormEntry->Function;
      NewVariable->Device.Port               = ConfigFormEntry->Port;
      NewVariable->Device.PortMultiplierPort = ConfigFormEntry->PortMultiplierPort;
      CopyMem (NewVariable->PasswordHash, HashData, sizeof (HashData));
      CopyMem (NewVariable->PasswordSalt, SaltData, sizeof (SaltData));
    }
  }

  if (NewVariable != NULL) {
    Status = gRT->SetVariable (
                    HDD_PASSWORD_VARIABLE_NAME,
                    &mHddPasswordVendorGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    NewVariableSize,
                    NewVariable
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "HddPassword variable set failed (%r)\n", Status));
    }
  }

  if (NewVariable != Variable) {
    FreePool (NewVariable);
  }
  if (Variable != NULL) {
    FreePool (Variable);
  }

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));
}

/**
  Get saved HDD password variable that will be used to validate HDD password
  when the device is at frozen state.

  @param[in]  ConfigFormEntry       The HDD Password configuration form entry.
  @param[out] HddPasswordVariable   The variable node for the HDD password device.

  @retval TRUE      The variable node for the HDD password device is found and returned.
  @retval FALSE     The variable node for the HDD password device is not found.

**/
BOOLEAN
GetSavedHddPasswordVariable (
  IN  HDD_PASSWORD_CONFIG_FORM_ENTRY    *ConfigFormEntry,
  OUT HDD_PASSWORD_VARIABLE             *HddPasswordVariable
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_VARIABLE             *TempVariable;
  HDD_PASSWORD_VARIABLE             *Variable;
  UINTN                             VariableSize;
  BOOLEAN                           Found;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  Variable = NULL;
  VariableSize = 0;

  Status = GetVariable2 (
             HDD_PASSWORD_VARIABLE_NAME,
             &mHddPasswordVendorGuid,
             (VOID **) &Variable,
             &VariableSize
             );
  if (EFI_ERROR (Status) || (Variable == NULL)) {
    DEBUG ((DEBUG_INFO, "HddPassword variable get failed (%r)\n", Status));
    return FALSE;
  }

  Found = FALSE;
  TempVariable = Variable;
  while (VariableSize >= sizeof (HDD_PASSWORD_VARIABLE)) {
    if ((TempVariable->Device.Bus                == ConfigFormEntry->Bus) &&
        (TempVariable->Device.Device             == ConfigFormEntry->Device) &&
        (TempVariable->Device.Function           == ConfigFormEntry->Function) &&
        (TempVariable->Device.Port               == ConfigFormEntry->Port) &&
        (TempVariable->Device.PortMultiplierPort == ConfigFormEntry->PortMultiplierPort)) {
      //
      // Found the node for the HDD password device.
      // Get the node.
      //
      CopyMem (HddPasswordVariable, TempVariable, sizeof (HDD_PASSWORD_VARIABLE));
      Found = TRUE;
      break;
    }
    VariableSize -= sizeof (HDD_PASSWORD_VARIABLE);
    TempVariable += 1;
  }

  FreePool (Variable);

  if (!Found) {
    DEBUG ((DEBUG_INFO, "The variable node for the HDD password device is not found\n"));
  }

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));

  return Found;
}

/**
  Use saved HDD password variable to validate HDD password
  when the device is at frozen state.

  @param[in] ConfigFormEntry    The HDD Password configuration form entry.
  @param[in] Password           The hdd password of attached ATA device.

  @retval EFI_SUCCESS           Pass to validate the HDD password.
  @retval EFI_NOT_FOUND         The variable node for the HDD password device is not found.
  @retval EFI_DEVICE_ERROR      Failed to generate credential for the HDD password.
  @retval EFI_INVALID_PARAMETER Failed to validate the HDD password.

**/
EFI_STATUS
ValidateHddPassword (
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry,
  IN CHAR8                          *Password
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_VARIABLE             HddPasswordVariable;
  BOOLEAN                           HashOk;
  UINT8                             HashData[SHA256_DIGEST_SIZE];

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  if (!GetSavedHddPasswordVariable (ConfigFormEntry, &HddPasswordVariable)) {
    DEBUG ((DEBUG_INFO, "GetSavedHddPasswordVariable failed\n"));
    return EFI_NOT_FOUND;
  }

  ZeroMem (HashData, sizeof (HashData));
  HashOk = GenerateCredential ((UINT8 *) Password, HDD_PASSWORD_MAX_LENGTH, HddPasswordVariable.PasswordSalt, HashData);
  if (!HashOk) {
    DEBUG ((DEBUG_INFO, "GenerateCredential failed\n"));
    return EFI_DEVICE_ERROR;
  }

  if (CompareMem (HddPasswordVariable.PasswordHash, HashData, sizeof (HashData)) != 0) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    Status = EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "%a() - exit (%r)\n", __FUNCTION__, Status));
  return Status;
}

/**
  Send unlock hdd password cmd through Ata Pass Thru Protocol.

  @param[in] AtaPassThru         The pointer to the ATA_PASS_THRU protocol.
  @param[in] Port                The port number of the ATA device to send the command.
  @param[in] PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                 If there is no port multiplier, then specify 0xFFFF.
  @param[in] Identifier          The identifier to set user or master password.
  @param[in] Password            The hdd password of attached ATA device.

  @retval EFI_SUCCESS            Successful to send unlock hdd password cmd.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to send unlock hdd password cmd.
  @retval EFI_DEVICE_ERROR       Can not send unlock hdd password cmd.

**/
EFI_STATUS
UnlockHddPassword (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort,
  IN CHAR8                          Identifier,
  IN CHAR8                          *Password
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              *Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  UINT8                             Buffer[HDD_PAYLOAD];

  if ((AtaPassThru == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The 'Asb' field (a pointer to the EFI_ATA_STATUS_BLOCK structure) in
  // EFI_ATA_PASS_THRU_COMMAND_PACKET is required to be aligned specified by
  // the 'IoAlign' field in the EFI_ATA_PASS_THRU_MODE structure. Meanwhile,
  // the structure EFI_ATA_STATUS_BLOCK is composed of only UINT8 fields, so it
  // may not be aligned when allocated on stack for some compilers. Hence, we
  // use the API AllocateAlignedPages to ensure this structure is properly
  // aligned.
  //
  Asb = AllocateAlignedPages (
          EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)),
          AtaPassThru->Mode->IoAlign
          );
  if (Asb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (Asb, sizeof (EFI_ATA_STATUS_BLOCK));
  Acb.AtaCommand    = ATA_SECURITY_UNLOCK_CMD;
  Acb.AtaDeviceHead = (UINT8) (PortMultiplierPort == 0xFFFF ? 0 : (PortMultiplierPort << 4));

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES;
  Packet.Asb      = Asb;
  Packet.Acb      = &Acb;

  ((CHAR16 *) Buffer)[0] = Identifier & BIT0;
  CopyMem (&((CHAR16 *) Buffer)[1], Password, HDD_PASSWORD_MAX_LENGTH);

  Packet.OutDataBuffer     = Buffer;
  Packet.OutTransferLength = sizeof (Buffer);
  Packet.Timeout           = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );
  if (!EFI_ERROR (Status) &&
      ((Asb->AtaStatus & ATA_STSREG_ERR) != 0) &&
      ((Asb->AtaError & ATA_ERRREG_ABRT) != 0)) {
    Status = EFI_DEVICE_ERROR;
  }

  FreeAlignedPages (Asb, EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)));

  ZeroMem (Buffer, sizeof (Buffer));

  DEBUG ((DEBUG_INFO, "%a() - %r\n", __FUNCTION__, Status));
  return Status;
}

/**
  Send disable hdd password cmd through Ata Pass Thru Protocol.

  @param[in] AtaPassThru         The pointer to the ATA_PASS_THRU protocol.
  @param[in] Port                The port number of the ATA device to send the command.
  @param[in] PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                 If there is no port multiplier, then specify 0xFFFF.
  @param[in] Identifier          The identifier to set user or master password.
  @param[in] Password            The hdd password of attached ATA device.

  @retval EFI_SUCCESS            Successful to disable hdd password cmd.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to disable hdd password cmd.
  @retval EFI_DEVICE_ERROR       Can not disable hdd password cmd.

**/
EFI_STATUS
DisableHddPassword (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort,
  IN CHAR8                          Identifier,
  IN CHAR8                          *Password
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              *Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  UINT8                             Buffer[HDD_PAYLOAD];

  if ((AtaPassThru == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The 'Asb' field (a pointer to the EFI_ATA_STATUS_BLOCK structure) in
  // EFI_ATA_PASS_THRU_COMMAND_PACKET is required to be aligned specified by
  // the 'IoAlign' field in the EFI_ATA_PASS_THRU_MODE structure. Meanwhile,
  // the structure EFI_ATA_STATUS_BLOCK is composed of only UINT8 fields, so it
  // may not be aligned when allocated on stack for some compilers. Hence, we
  // use the API AllocateAlignedPages to ensure this structure is properly
  // aligned.
  //
  Asb = AllocateAlignedPages (
          EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)),
          AtaPassThru->Mode->IoAlign
          );
  if (Asb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (Asb, sizeof (EFI_ATA_STATUS_BLOCK));
  Acb.AtaCommand    = ATA_SECURITY_DIS_PASSWORD_CMD;
  Acb.AtaDeviceHead = (UINT8) (PortMultiplierPort == 0xFFFF ? 0 : (PortMultiplierPort << 4));

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES;
  Packet.Asb      = Asb;
  Packet.Acb      = &Acb;

  ((CHAR16 *) Buffer)[0] = Identifier & BIT0;
  CopyMem (&((CHAR16 *) Buffer)[1], Password, HDD_PASSWORD_MAX_LENGTH);

  Packet.OutDataBuffer     = Buffer;
  Packet.OutTransferLength = sizeof (Buffer);
  Packet.Timeout           = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );
  if (!EFI_ERROR (Status) &&
      ((Asb->AtaStatus & ATA_STSREG_ERR) != 0) &&
      ((Asb->AtaError & ATA_ERRREG_ABRT) != 0)) {
    Status = EFI_DEVICE_ERROR;
  }

  FreeAlignedPages (Asb, EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)));

  ZeroMem (Buffer, sizeof (Buffer));

  DEBUG ((DEBUG_INFO, "%a() - %r\n", __FUNCTION__, Status));
  return Status;
}

/**
  Send set hdd password cmd through Ata Pass Thru Protocol.

  @param[in] AtaPassThru                The pointer to the ATA_PASS_THRU protocol.
  @param[in] Port                       The port number of the ATA device to send the command.
  @param[in] PortMultiplierPort         The port multiplier port number of the ATA device to send the command.
                                        If there is no port multiplier, then specify 0xFFFF.
  @param[in] Identifier                 The identifier to set user or master password.
  @param[in] SecurityLevel              The security level to be set to device.
  @param[in] MasterPasswordIdentifier   The master password identifier to be set to device.
  @param[in] Password                   The hdd password of attached ATA device.

  @retval EFI_SUCCESS            Successful to set hdd password cmd.
  @retval EFI_INVALID_PARAMETER  The parameter passed-in is invalid.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to set hdd password cmd.
  @retval EFI_DEVICE_ERROR       Can not set hdd password cmd.

**/
EFI_STATUS
SetHddPassword (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort,
  IN CHAR8                          Identifier,
  IN CHAR8                          SecurityLevel,
  IN CHAR16                         MasterPasswordIdentifier,
  IN CHAR8                          *Password
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             Acb;
  EFI_ATA_STATUS_BLOCK              *Asb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  UINT8                             Buffer[HDD_PAYLOAD];

  if ((AtaPassThru == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The 'Asb' field (a pointer to the EFI_ATA_STATUS_BLOCK structure) in
  // EFI_ATA_PASS_THRU_COMMAND_PACKET is required to be aligned specified by
  // the 'IoAlign' field in the EFI_ATA_PASS_THRU_MODE structure. Meanwhile,
  // the structure EFI_ATA_STATUS_BLOCK is composed of only UINT8 fields, so it
  // may not be aligned when allocated on stack for some compilers. Hence, we
  // use the API AllocateAlignedPages to ensure this structure is properly
  // aligned.
  //
  Asb = AllocateAlignedPages (
          EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)),
          AtaPassThru->Mode->IoAlign
          );
  if (Asb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Prepare for ATA command block.
  //
  ZeroMem (&Acb, sizeof (Acb));
  ZeroMem (Asb, sizeof (EFI_ATA_STATUS_BLOCK));
  Acb.AtaCommand    = ATA_SECURITY_SET_PASSWORD_CMD;
  Acb.AtaDeviceHead = (UINT8) (PortMultiplierPort == 0xFFFF ? 0 : (PortMultiplierPort << 4));

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES;
  Packet.Asb      = Asb;
  Packet.Acb      = &Acb;

  ((CHAR16 *) Buffer)[0] = (Identifier | (UINT16)(SecurityLevel << 8)) & (BIT0 | BIT8);
  CopyMem (&((CHAR16 *) Buffer)[1], Password, HDD_PASSWORD_MAX_LENGTH);
  if ((Identifier & BIT0) != 0) {
    ((CHAR16 *) Buffer)[17] = MasterPasswordIdentifier;
  }

  Packet.OutDataBuffer     = Buffer;
  Packet.OutTransferLength = sizeof (Buffer);
  Packet.Timeout           = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet,
                          NULL
                          );
  if (!EFI_ERROR (Status) &&
      ((Asb->AtaStatus & ATA_STSREG_ERR) != 0) &&
      ((Asb->AtaError & ATA_ERRREG_ABRT) != 0)) {
    Status = EFI_DEVICE_ERROR;
  }

  FreeAlignedPages (Asb, EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)));

  ZeroMem (Buffer, sizeof (Buffer));

  DEBUG ((DEBUG_INFO, "%a() - %r\n", __FUNCTION__, Status));
  return Status;
}

/**
  Get attached harddisk model number from identify data buffer.

  @param[in] IdentifyData    Pointer to identify data buffer.
  @param[in, out] String     The buffer to store harddisk model number.

**/
VOID
GetHddDeviceModelNumber (
  IN ATA_IDENTIFY_DATA             *IdentifyData,
  IN OUT CHAR16                    *String
  )
{
  UINTN             Index;

  //
  // Swap the byte order in the original module name.
  // From Ata spec, the maximum length is 40 bytes.
  //
  for (Index = 0; Index < 40; Index += 2) {
    String[Index]      = IdentifyData->ModelName[Index + 1];
    String[Index + 1]  = IdentifyData->ModelName[Index];
  }

  //
  // Chap it off after 20 characters
  //
  String[20] = L'\0';

  return ;
}

/**
  Get password input from the popup windows.

  @param[in]      PopUpString1  Pop up string 1.
  @param[in]      PopUpString2  Pop up string 2.
  @param[in, out] Password      The buffer to hold the input password.

  @retval EFI_ABORTED           It is given up by pressing 'ESC' key.
  @retval EFI_SUCCESS           Get password input successfully.

**/
EFI_STATUS
PopupHddPasswordInputWindows (
  IN CHAR16         *PopUpString1,
  IN CHAR16         *PopUpString2,
  IN OUT CHAR8      *Password
  )
{
  EFI_INPUT_KEY Key;
  UINTN         Length;
  CHAR16        Mask[HDD_PASSWORD_MAX_LENGTH + 1];
  CHAR16        Unicode[HDD_PASSWORD_MAX_LENGTH + 1];
  CHAR8         Ascii[HDD_PASSWORD_MAX_LENGTH + 1];

  ZeroMem (Unicode, sizeof (Unicode));
  ZeroMem (Ascii, sizeof (Ascii));
  ZeroMem (Mask, sizeof (Mask));

  gST->ConOut->ClearScreen(gST->ConOut);

  Length = 0;
  while (TRUE) {
    Mask[Length] = L'_';
    if (PopUpString2 == NULL) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        PopUpString1,
        L"---------------------",
        Mask,
        NULL
      );
    } else {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        PopUpString1,
        PopUpString2,
        L"---------------------",
        Mask,
        NULL
      );
    }
    //
    // Check key.
    //
    if (Key.ScanCode == SCAN_NULL) {
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Add the null terminator.
        //
        Unicode[Length] = 0;
        break;
      } else if ((Key.UnicodeChar == CHAR_NULL) ||
                 (Key.UnicodeChar == CHAR_TAB) ||
                 (Key.UnicodeChar == CHAR_LINEFEED)
                 ) {
        continue;
      } else {
        if (Key.UnicodeChar == CHAR_BACKSPACE) {
          if (Length > 0) {
            Unicode[Length] = 0;
            Mask[Length] = 0;
            Length--;
          }
        } else {
          Unicode[Length] = Key.UnicodeChar;
          Mask[Length] = L'*';
          Length++;
          if (Length == HDD_PASSWORD_MAX_LENGTH) {
            //
            // Add the null terminator.
            //
            Unicode[Length] = 0;
            Mask[Length] = 0;
            break;
          }
        }
      }
    }

    if (Key.ScanCode == SCAN_ESC) {
      ZeroMem (Unicode, sizeof (Unicode));
      ZeroMem (Ascii, sizeof (Ascii));
      gST->ConOut->ClearScreen(gST->ConOut);
      return EFI_ABORTED;
    }
  }

  UnicodeStrToAsciiStrS (Unicode, Ascii, sizeof (Ascii));
  CopyMem (Password, Ascii, HDD_PASSWORD_MAX_LENGTH);
  ZeroMem (Unicode, sizeof (Unicode));
  ZeroMem (Ascii, sizeof (Ascii));

  gST->ConOut->ClearScreen(gST->ConOut);
  return EFI_SUCCESS;
}

/**
  Check if disk is locked, show popup window and ask for password if it is.

  @param[in] AtaPassThru            Pointer to ATA_PASSTHRU instance.
  @param[in] Port                   The port number of attached ATA device.
  @param[in] PortMultiplierPort     The port number of port multiplier of attached ATA device.
  @param[in] ConfigFormEntry        The HDD Password configuration form entry.

**/
VOID
HddPasswordRequestPassword (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort,
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry
  )
{
  EFI_STATUS                        Status;
  CHAR16                            PopUpString[100];
  ATA_IDENTIFY_DATA                 IdentifyData;
  EFI_INPUT_KEY                     Key;
  UINT16                            RetryCount;
  CHAR8                             Password[HDD_PASSWORD_MAX_LENGTH];

  RetryCount = 0;

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  UnicodeSPrint (PopUpString, sizeof (PopUpString), L"Unlock: %s", ConfigFormEntry->HddString);

  //
  // Check the device security status.
  //
  if ((ConfigFormEntry->IfrData.SecurityStatus.Supported) &&
      (ConfigFormEntry->IfrData.SecurityStatus.Enabled)) {

     //
     // Add PcdSkipHddPasswordPrompt to determin whether to skip password prompt.
     // Due to board design, device may not power off during system warm boot, which result in
     // security status remain unlocked status, hence we add device security status check here.
     //
     // If device is in the locked status, device keeps locked and system continues booting.
     // If device is in the unlocked status, system is forced shutdown for security concern.
     //
     if (PcdGetBool (PcdSkipHddPasswordPrompt)) {
       if (ConfigFormEntry->IfrData.SecurityStatus.Locked) {
         return;
       } else {
         gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
       }
    }
    //
    // As soon as the HDD password is in enabled state, we pop up a window to unlock hdd
    // no matter it's really in locked or unlocked state.
    // This way forces user to enter password every time to provide best safety.
    //
    while (TRUE) {
      Status = PopupHddPasswordInputWindows (PopUpString, NULL, Password);
      if (!EFI_ERROR (Status)) {
        //
        // The HDD is in locked state, unlock it by user input.
        //
        if (!PasswordIsFullZero (Password)) {
          if (!ConfigFormEntry->IfrData.SecurityStatus.Frozen) {
            Status = UnlockHddPassword (AtaPassThru, Port, PortMultiplierPort, 0, Password);
          } else {
            //
            // Use saved HDD password variable to validate HDD password
            // when the device is at frozen state.
            //
            Status = ValidateHddPassword (ConfigFormEntry, Password);
          }
        } else {
          Status = EFI_INVALID_PARAMETER;
        }
        if (!EFI_ERROR (Status)) {
          CopyMem (ConfigFormEntry->Password, Password, HDD_PASSWORD_MAX_LENGTH);
          if (!ConfigFormEntry->IfrData.SecurityStatus.Frozen) {
            SaveHddPasswordVariable (ConfigFormEntry, Password);
          }
          ZeroMem (Password, HDD_PASSWORD_MAX_LENGTH);
          Status = GetHddDeviceIdentifyData (AtaPassThru, Port, PortMultiplierPort, &IdentifyData);
          ASSERT_EFI_ERROR (Status);

          //
          // Check the device security status again.
          //
          GetHddPasswordSecurityStatus (&IdentifyData, &ConfigFormEntry->IfrData);
          return;
        }

        ZeroMem (Password, HDD_PASSWORD_MAX_LENGTH);

        if (EFI_ERROR (Status)) {
          RetryCount ++;
          if (RetryCount < MAX_HDD_PASSWORD_RETRY_COUNT) {
            do {
              CreatePopUp (
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                &Key,
                L"Invalid password.",
                L"Press ENTER to retry",
                NULL
                );
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            continue;
          } else {
            do {
              CreatePopUp (
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                &Key,
                L"Hdd password retry count is expired. Please shutdown the machine.",
                L"Press ENTER to shutdown",
                NULL
                );
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
            break;
          }
        }
      } else if (Status == EFI_ABORTED) {
        if (ConfigFormEntry->IfrData.SecurityStatus.Locked) {
          //
          // Current device in the lock status and
          // User not input password and press ESC,
          // keep device in lock status and continue boot.
          //
          do {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"Press ENTER to skip the request and continue boot,",
              L"Press ESC to input password again",
              NULL
              );
          } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

          if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            gST->ConOut->ClearScreen(gST->ConOut);
            //
            // Keep lock and continue boot.
            //
            return;
          } else {
            //
            // Let user input password again.
            //
            continue;
          }
        } else {
          //
          // Current device in the unlock status and
          // User not input password and press ESC,
          // Shutdown the device.
          //
          do {
            CreatePopUp (
              EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
              &Key,
              L"Press ENTER to shutdown, Press ESC to input password again",
              NULL
              );
          } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

          if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
          } else {
            //
            // Let user input password again.
            //
            continue;
          }
        }
      }
    }
  }
}

/**
  Process Set User Pwd HDD password request.

  @param[in] AtaPassThru            Pointer to ATA_PASSTHRU instance.
  @param[in] Port                   The port number of attached ATA device.
  @param[in] PortMultiplierPort     The port number of port multiplier of attached ATA device.
  @param[in] ConfigFormEntry        The HDD Password configuration form entry.

**/
VOID
ProcessHddPasswordRequestSetUserPwd (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort,
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry
  )
{
  EFI_STATUS                        Status;
  CHAR16                            PopUpString[100];
  ATA_IDENTIFY_DATA                 IdentifyData;
  EFI_INPUT_KEY                     Key;
  UINT16                            RetryCount;
  CHAR8                             Password[HDD_PASSWORD_MAX_LENGTH];
  CHAR8                             PasswordConfirm[HDD_PASSWORD_MAX_LENGTH];

  RetryCount = 0;

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  if (ConfigFormEntry->IfrData.SecurityStatus.Frozen) {
    DEBUG ((DEBUG_INFO, "%s is frozen, do nothing\n", ConfigFormEntry->HddString));
    return;
  }

  if (ConfigFormEntry->IfrData.SecurityStatus.Locked) {
    DEBUG ((DEBUG_INFO, "%s is locked, do nothing\n", ConfigFormEntry->HddString));
    return;
  }

  UnicodeSPrint (PopUpString, sizeof (PopUpString), L"Set User Pwd: %s", ConfigFormEntry->HddString);

  //
  // Check the device security status.
  //
  if (ConfigFormEntry->IfrData.SecurityStatus.Supported) {
    while (TRUE) {
      Status = PopupHddPasswordInputWindows (PopUpString, L"Please type in your new password", Password);
      if (!EFI_ERROR (Status)) {
        Status = PopupHddPasswordInputWindows (PopUpString, L"Please confirm your new password", PasswordConfirm);
        if (!EFI_ERROR (Status)) {
          if (CompareMem (Password, PasswordConfirm, HDD_PASSWORD_MAX_LENGTH) == 0) {
            if (!PasswordIsFullZero (Password)) {
              Status = SetHddPassword (AtaPassThru, Port, PortMultiplierPort, 0, 1, 0, Password);
            } else {
              if (ConfigFormEntry->IfrData.SecurityStatus.Enabled) {
                Status = DisableHddPassword (AtaPassThru, Port, PortMultiplierPort, 0, ConfigFormEntry->Password);
              } else {
                Status = EFI_INVALID_PARAMETER;
              }
            }
            if (!EFI_ERROR (Status)) {
              CopyMem (ConfigFormEntry->Password, Password, HDD_PASSWORD_MAX_LENGTH);
              SaveHddPasswordVariable (ConfigFormEntry, Password);
              ZeroMem (Password, HDD_PASSWORD_MAX_LENGTH);
              ZeroMem (PasswordConfirm, HDD_PASSWORD_MAX_LENGTH);
              Status = GetHddDeviceIdentifyData (AtaPassThru, Port, PortMultiplierPort, &IdentifyData);
              ASSERT_EFI_ERROR (Status);

              //
              // Check the device security status again.
              //
              GetHddPasswordSecurityStatus (&IdentifyData, &ConfigFormEntry->IfrData);
              return;
            } else {
              do {
                CreatePopUp (
                  EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                  &Key,
                  L"Set/Disable User Pwd failed or invalid password.",
                  L"Press ENTER to retry",
                  NULL
                  );
              } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            }
          } else {
            do {
              CreatePopUp (
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                &Key,
                L"Passwords are not the same.",
                L"Press ENTER to retry",
                NULL
                );
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            Status = EFI_INVALID_PARAMETER;
          }
        }

        ZeroMem (Password, HDD_PASSWORD_MAX_LENGTH);
        ZeroMem (PasswordConfirm, HDD_PASSWORD_MAX_LENGTH);

        if (EFI_ERROR (Status)) {
          RetryCount ++;
          if (RetryCount >= MAX_HDD_PASSWORD_RETRY_COUNT) {
            do {
              CreatePopUp (
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                &Key,
                L"Hdd password retry count is expired.",
                L"Press ENTER to skip the request and continue boot",
                NULL
                );
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            gST->ConOut->ClearScreen(gST->ConOut);
            return;
          }
        }
      } else if (Status == EFI_ABORTED) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          return;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
      }
    }
  }
}

/**
  Process Set Master Pwd HDD password request.

  @param[in] AtaPassThru            Pointer to ATA_PASSTHRU instance.
  @param[in] Port                   The port number of attached ATA device.
  @param[in] PortMultiplierPort     The port number of port multiplier of attached ATA device.
  @param[in] ConfigFormEntry        The HDD Password configuration form entry.

**/
VOID
ProcessHddPasswordRequestSetMasterPwd (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort,
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry
  )
{
  EFI_STATUS                        Status;
  CHAR16                            PopUpString[100];
  EFI_INPUT_KEY                     Key;
  UINT16                            RetryCount;
  CHAR8                             Password[HDD_PASSWORD_MAX_LENGTH];
  CHAR8                             PasswordConfirm[HDD_PASSWORD_MAX_LENGTH];

  RetryCount = 0;

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  if (ConfigFormEntry->IfrData.SecurityStatus.Frozen) {
    DEBUG ((DEBUG_INFO, "%s is frozen, do nothing\n", ConfigFormEntry->HddString));
    return;
  }

  if (ConfigFormEntry->IfrData.SecurityStatus.Locked) {
    DEBUG ((DEBUG_INFO, "%s is locked, do nothing\n", ConfigFormEntry->HddString));
    return;
  }

  UnicodeSPrint (PopUpString, sizeof (PopUpString), L"Set Master Pwd: %s", ConfigFormEntry->HddString);

  //
  // Check the device security status.
  //
  if (ConfigFormEntry->IfrData.SecurityStatus.Supported) {
    while (TRUE) {
      Status = PopupHddPasswordInputWindows (PopUpString, L"Please type in your new password", Password);
      if (!EFI_ERROR (Status)) {
        Status = PopupHddPasswordInputWindows (PopUpString, L"Please confirm your new password", PasswordConfirm);
        if (!EFI_ERROR (Status)) {
          if (CompareMem (Password, PasswordConfirm, HDD_PASSWORD_MAX_LENGTH) == 0) {
            if (!PasswordIsFullZero (Password)) {
              Status = SetHddPassword (AtaPassThru, Port, PortMultiplierPort, 1, 1, 1, Password);
            } else {
              Status = EFI_INVALID_PARAMETER;
            }
            if (!EFI_ERROR (Status)) {
              ZeroMem (Password, HDD_PASSWORD_MAX_LENGTH);
              ZeroMem (PasswordConfirm, HDD_PASSWORD_MAX_LENGTH);
              return;
            } else {
              do {
                CreatePopUp (
                  EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                  &Key,
                  L"Set Master Pwd failed or invalid password.",
                  L"Press ENTER to retry",
                  NULL
                  );
              } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            }
          } else {
            do {
              CreatePopUp (
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                &Key,
                L"Passwords are not the same.",
                L"Press ENTER to retry",
                NULL
                );
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            Status = EFI_INVALID_PARAMETER;
          }
        }

        ZeroMem (Password, HDD_PASSWORD_MAX_LENGTH);
        ZeroMem (PasswordConfirm, HDD_PASSWORD_MAX_LENGTH);

        if (EFI_ERROR (Status)) {
          RetryCount ++;
          if (RetryCount >= MAX_HDD_PASSWORD_RETRY_COUNT) {
            do {
              CreatePopUp (
                EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
                &Key,
                L"Hdd password retry count is expired.",
                L"Press ENTER to skip the request and continue boot",
                NULL
                );
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
            gST->ConOut->ClearScreen(gST->ConOut);
            return;
          }
        }
      } else if (Status == EFI_ABORTED) {
        do {
          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"Press ENTER to skip the request and continue boot,",
            L"Press ESC to input password again",
            NULL
            );
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          gST->ConOut->ClearScreen(gST->ConOut);
          return;
        } else {
          //
          // Let user input password again.
          //
          continue;
        }
      }
    }
  }
}

/**
  Process HDD password request.

  @param[in] AtaPassThru            Pointer to ATA_PASSTHRU instance.
  @param[in] Port                   The port number of attached ATA device.
  @param[in] PortMultiplierPort     The port number of port multiplier of attached ATA device.
  @param[in] ConfigFormEntry        The HDD Password configuration form entry.

**/
VOID
ProcessHddPasswordRequest (
  IN EFI_ATA_PASS_THRU_PROTOCOL     *AtaPassThru,
  IN UINT16                         Port,
  IN UINT16                         PortMultiplierPort,
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_REQUEST_VARIABLE     *TempVariable;
  HDD_PASSWORD_REQUEST_VARIABLE     *Variable;
  UINTN                             VariableSize;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  if (mHddPasswordRequestVariable == NULL) {
    Status = GetVariable2 (
               HDD_PASSWORD_REQUEST_VARIABLE_NAME,
               &mHddPasswordVendorGuid,
               (VOID **) &Variable,
               &VariableSize
               );
    if (EFI_ERROR (Status) || (Variable == NULL)) {
      return;
    }
    mHddPasswordRequestVariable = Variable;
    mHddPasswordRequestVariableSize = VariableSize;

    //
    // Delete the HDD password request variable.
    //
    Status = gRT->SetVariable (
                    HDD_PASSWORD_REQUEST_VARIABLE_NAME,
                    &mHddPasswordVendorGuid,
                    0,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    Variable = mHddPasswordRequestVariable;
    VariableSize = mHddPasswordRequestVariableSize;
  }

  //
  // Process the HDD password requests.
  //
  TempVariable = Variable;
  while (VariableSize >= sizeof (HDD_PASSWORD_REQUEST_VARIABLE)) {
    if ((TempVariable->Device.Bus                == ConfigFormEntry->Bus) &&
        (TempVariable->Device.Device             == ConfigFormEntry->Device) &&
        (TempVariable->Device.Function           == ConfigFormEntry->Function) &&
        (TempVariable->Device.Port               == ConfigFormEntry->Port) &&
        (TempVariable->Device.PortMultiplierPort == ConfigFormEntry->PortMultiplierPort)) {
      //
      // Found the node for the HDD password device.
      //
      if (TempVariable->Request.UserPassword != 0) {
        ProcessHddPasswordRequestSetUserPwd (AtaPassThru, Port, PortMultiplierPort, ConfigFormEntry);
      }
      if (TempVariable->Request.MasterPassword != 0) {
        ProcessHddPasswordRequestSetMasterPwd (AtaPassThru, Port, PortMultiplierPort, ConfigFormEntry);
      }

      break;
    }

    VariableSize -= sizeof (HDD_PASSWORD_REQUEST_VARIABLE);
    TempVariable += 1;
  }

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));
}

/**
  Get saved HDD password request.

  @param[in, out] ConfigFormEntry       The HDD Password configuration form entry.

**/
VOID
GetSavedHddPasswordRequest (
  IN OUT HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_REQUEST_VARIABLE     *TempVariable;
  HDD_PASSWORD_REQUEST_VARIABLE     *Variable;
  UINTN                             VariableSize;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  Variable = NULL;
  VariableSize = 0;

  Status = GetVariable2 (
             HDD_PASSWORD_REQUEST_VARIABLE_NAME,
             &mHddPasswordVendorGuid,
             (VOID **) &Variable,
             &VariableSize
             );
  if (EFI_ERROR (Status) || (Variable == NULL)) {
    return;
  }

  TempVariable = Variable;
  while (VariableSize >= sizeof (HDD_PASSWORD_REQUEST_VARIABLE)) {
    if ((TempVariable->Device.Bus                == ConfigFormEntry->Bus) &&
        (TempVariable->Device.Device             == ConfigFormEntry->Device) &&
        (TempVariable->Device.Function           == ConfigFormEntry->Function) &&
        (TempVariable->Device.Port               == ConfigFormEntry->Port) &&
        (TempVariable->Device.PortMultiplierPort == ConfigFormEntry->PortMultiplierPort)) {
      //
      // Found the node for the HDD password device.
      // Get the HDD password request.
      //
      CopyMem (&ConfigFormEntry->IfrData.Request, &TempVariable->Request, sizeof (HDD_PASSWORD_REQUEST));
      DEBUG ((
        DEBUG_INFO,
        "HddPasswordRequest got: 0x%x\n",
        ConfigFormEntry->IfrData.Request
        ));
      break;
    }
    VariableSize -= sizeof (HDD_PASSWORD_REQUEST_VARIABLE);
    TempVariable += 1;
  }

  FreePool (Variable);

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));
}

/**
  Save HDD password request.

  @param[in] ConfigFormEntry        The HDD Password configuration form entry.

**/
VOID
SaveHddPasswordRequest (
  IN HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_REQUEST_VARIABLE     *TempVariable;
  UINTN                             TempVariableSize;
  HDD_PASSWORD_REQUEST_VARIABLE     *Variable;
  UINTN                             VariableSize;
  HDD_PASSWORD_REQUEST_VARIABLE     *NewVariable;
  UINTN                             NewVariableSize;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  DEBUG ((
    DEBUG_INFO,
    "HddPasswordRequest to save: 0x%x\n",
    ConfigFormEntry->IfrData.Request
    ));

  Variable = NULL;
  VariableSize = 0;
  NewVariable = NULL;
  NewVariableSize = 0;

  Status = GetVariable2 (
             HDD_PASSWORD_REQUEST_VARIABLE_NAME,
             &mHddPasswordVendorGuid,
             (VOID **) &Variable,
             &VariableSize
             );
  if (!EFI_ERROR (Status) && (Variable != NULL)) {
    TempVariable = Variable;
    TempVariableSize = VariableSize;
    while (TempVariableSize >= sizeof (HDD_PASSWORD_REQUEST_VARIABLE)) {
      if ((TempVariable->Device.Bus                == ConfigFormEntry->Bus) &&
          (TempVariable->Device.Device             == ConfigFormEntry->Device) &&
          (TempVariable->Device.Function           == ConfigFormEntry->Function) &&
          (TempVariable->Device.Port               == ConfigFormEntry->Port) &&
          (TempVariable->Device.PortMultiplierPort == ConfigFormEntry->PortMultiplierPort)) {
        //
        // Found the node for the HDD password device.
        // Update the HDD password request.
        //
        CopyMem (&TempVariable->Request, &ConfigFormEntry->IfrData.Request, sizeof (HDD_PASSWORD_REQUEST));
        NewVariable = Variable;
        NewVariableSize = VariableSize;
        break;
      }
      TempVariableSize -= sizeof (HDD_PASSWORD_REQUEST_VARIABLE);
      TempVariable += 1;
    }
    if (NewVariable == NULL) {
      //
      // The node for the HDD password device is not found.
      // Create node for the HDD password device.
      //
      NewVariableSize = VariableSize + sizeof (HDD_PASSWORD_REQUEST_VARIABLE);
      NewVariable = AllocateZeroPool (NewVariableSize);
      ASSERT (NewVariable != NULL);
      CopyMem (NewVariable, Variable, VariableSize);
      TempVariable = (HDD_PASSWORD_REQUEST_VARIABLE *) ((UINTN) NewVariable + VariableSize);
      TempVariable->Device.Bus                = (UINT8) ConfigFormEntry->Bus;
      TempVariable->Device.Device             = (UINT8) ConfigFormEntry->Device;
      TempVariable->Device.Function           = (UINT8) ConfigFormEntry->Function;
      TempVariable->Device.Port               = ConfigFormEntry->Port;
      TempVariable->Device.PortMultiplierPort = ConfigFormEntry->PortMultiplierPort;
      CopyMem (&TempVariable->Request, &ConfigFormEntry->IfrData.Request, sizeof (HDD_PASSWORD_REQUEST));
    }
  } else {
    NewVariableSize = sizeof (HDD_PASSWORD_REQUEST_VARIABLE);
    NewVariable = AllocateZeroPool (NewVariableSize);
    ASSERT (NewVariable != NULL);
    NewVariable->Device.Bus                = (UINT8) ConfigFormEntry->Bus;
    NewVariable->Device.Device             = (UINT8) ConfigFormEntry->Device;
    NewVariable->Device.Function           = (UINT8) ConfigFormEntry->Function;
    NewVariable->Device.Port               = ConfigFormEntry->Port;
    NewVariable->Device.PortMultiplierPort = ConfigFormEntry->PortMultiplierPort;
    CopyMem (&NewVariable->Request, &ConfigFormEntry->IfrData.Request, sizeof (HDD_PASSWORD_REQUEST));
  }
  Status = gRT->SetVariable (
                  HDD_PASSWORD_REQUEST_VARIABLE_NAME,
                  &mHddPasswordVendorGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  NewVariableSize,
                  NewVariable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "HddPasswordRequest variable set failed (%r)\n", Status));
  }
  if (NewVariable != Variable) {
    FreePool (NewVariable);
  }
  if (Variable != NULL) {
    FreePool (Variable);
  }

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));
}

/**
  Get the HDD Password configuration form entry by the index of the goto opcode activated.

  @param[in]  Index The 0-based index of the goto opcode activated.

  @return The HDD Password configuration form entry found.
**/
HDD_PASSWORD_CONFIG_FORM_ENTRY *
HddPasswordGetConfigFormEntryByIndex (
  IN UINT32 Index
  )
{
  LIST_ENTRY                     *Entry;
  UINT32                         CurrentIndex;
  HDD_PASSWORD_CONFIG_FORM_ENTRY *ConfigFormEntry;

  CurrentIndex    = 0;
  ConfigFormEntry = NULL;

  BASE_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    if (CurrentIndex == Index) {
      ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);
      break;
    }

    CurrentIndex++;
  }

  return ConfigFormEntry;
}

/**
  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Any and all alternative
  configuration strings shall also be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param[in] This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in] Request    A null-terminated Unicode string in
                        <ConfigRequest> format. Note that this
                        includes the routing information as well as
                        the configurable name / value pairs. It is
                        invalid for this string to be in
                        <MultiConfigRequest> format.
  @param[out] Progress  On return, points to a character in the
                        Request string. Points to the string's null
                        terminator if request was successful. Points
                        to the most recent "&" before the first
                        failing name / value pair (or the beginning
                        of the string if the failure is in the first
                        name / value pair) if the request was not
                        successful.
  @param[out] Results   A null-terminated Unicode string in
                        <ConfigAltResp> format which has all values
                        filled in for the names in the Request string.
                        String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the
                                  values corresponding to all requested
                                  names.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL.
  @retval EFI_NOT_FOUND           Routing data doesn't match any
                                  known driver. Progress set to the
                                  first character in the routing header.
                                  Note: There is no requirement that the
                                  driver validate the routing data. It
                                  must skip the <ConfigHdr> in order to
                                  process the names.
  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent & before the
                                  error or the beginning of the
                                  string.
  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.Currently not implemented.
**/
EFI_STATUS
EFIAPI
HddPasswordFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  HDD_PASSWORD_CONFIG              *IfrData;
  HDD_PASSWORD_DXE_PRIVATE_DATA    *Private;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mHddPasswordVendorGuid, mHddPasswordVendorStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS (This);
  IfrData = AllocateZeroPool (sizeof (HDD_PASSWORD_CONFIG));
  ASSERT (IfrData != NULL);
  if (Private->Current != NULL) {
    CopyMem (IfrData, &Private->Current->IfrData, sizeof (HDD_PASSWORD_CONFIG));
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof (HDD_PASSWORD_CONFIG);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mHddPasswordVendorGuid, mHddPasswordVendorStorageName, Private->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) IfrData,
                                BufferSize,
                                Results,
                                Progress
                                );
  FreePool (IfrData);
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in
                             <ConfigString> format.
  @param[out] Progress       A pointer to a string filled in with the
                             offset of the most recent '&' before the
                             first failing name / value pair (or the
                             beginn ing of the string if the failure
                             is in the first name / value pair) or
                             the terminating NULL if all was
                             successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
HddPasswordFormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mHddPasswordVendorGuid, mHddPasswordVendorStorageName)) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
}

/**
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to
                                 vary based on the opcode that enerated the callback.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out]  ActionRequest     On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.Currently not implemented.
  @retval EFI_INVALID_PARAMETERS Passing in wrong parameter.
  @retval Others                 Other errors as indicated.
**/
EFI_STATUS
EFIAPI
HddPasswordFormCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  HDD_PASSWORD_DXE_PRIVATE_DATA   *Private;
  EFI_STRING_ID                    DeviceFormTitleToken;
  HDD_PASSWORD_CONFIG              *IfrData;
  HDD_PASSWORD_CONFIG_FORM_ENTRY   *ConfigFormEntry;

  if (ActionRequest != NULL) {
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if ((Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_CHANGED)) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changing or changed.
    //
    return EFI_UNSUPPORTED;
  }

  Private = HDD_PASSWORD_DXE_PRIVATE_FROM_THIS (This);

  //
  // Retrive data from Browser
  //
  IfrData = AllocateZeroPool (sizeof (HDD_PASSWORD_CONFIG));
  ASSERT (IfrData != NULL);
  if (!HiiGetBrowserData (&mHddPasswordVendorGuid, mHddPasswordVendorStorageName, sizeof (HDD_PASSWORD_CONFIG), (UINT8 *) IfrData)) {
    FreePool (IfrData);
    return EFI_NOT_FOUND;
  }

  switch (QuestionId) {
  case KEY_HDD_USER_PASSWORD:
    if (Action == EFI_BROWSER_ACTION_CHANGED) {
      DEBUG ((DEBUG_INFO, "KEY_HDD_USER_PASSWORD\n"));
      ConfigFormEntry = Private->Current;
      ConfigFormEntry->IfrData.Request.UserPassword = Value->b;
      SaveHddPasswordRequest (ConfigFormEntry);
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
    }
    break;
  case KEY_HDD_MASTER_PASSWORD:
    if (Action == EFI_BROWSER_ACTION_CHANGED) {
      DEBUG ((DEBUG_INFO, "KEY_HDD_MASTER_PASSWORD\n"));
      ConfigFormEntry = Private->Current;
      ConfigFormEntry->IfrData.Request.MasterPassword = Value->b;
      SaveHddPasswordRequest (ConfigFormEntry);
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
    }
    break;

  default:
    if ((QuestionId >= KEY_HDD_DEVICE_ENTRY_BASE) && (QuestionId < (mNumberOfHddDevices + KEY_HDD_DEVICE_ENTRY_BASE))) {
      if (Action == EFI_BROWSER_ACTION_CHANGING) {
        //
        // In case goto the device configuration form, update the device form title.
        //
        ConfigFormEntry = HddPasswordGetConfigFormEntryByIndex ((UINT32) (QuestionId - KEY_HDD_DEVICE_ENTRY_BASE));
        ASSERT (ConfigFormEntry != NULL);

        DeviceFormTitleToken = (EFI_STRING_ID) STR_HDD_SECURITY_HD;
        HiiSetString (Private->HiiHandle, DeviceFormTitleToken, ConfigFormEntry->HddString, NULL);

        Private->Current = ConfigFormEntry;
        CopyMem (IfrData, &ConfigFormEntry->IfrData, sizeof (HDD_PASSWORD_CONFIG));
      }
    }

    break;
  }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  HiiSetBrowserData (&mHddPasswordVendorGuid, mHddPasswordVendorStorageName, sizeof (HDD_PASSWORD_CONFIG), (UINT8 *) IfrData, NULL);

  FreePool (IfrData);
  return EFI_SUCCESS;
}

/**
  Updates the HDD Password configuration form to add an entry for the attached
  ata harddisk device specified by the Controller.

  @param[in] HiiHandle            The HII Handle associated with the registered package list.
  @param[in] AtaPassThru          Pointer to ATA_PASSTHRU instance.
  @param[in] PciIo                Pointer to PCI_IO instance.
  @param[in] Controller           The controller handle of the attached ata controller.
  @param[in] Bus                  The bus number of ATA controller.
  @param[in] Device               The device number of ATA controller.
  @param[in] Function             The function number of ATA controller.
  @param[in] Port                 The port number of attached ATA device.
  @param[in] PortMultiplierPort   The port number of port multiplier of attached ATA device.

  @retval EFI_SUCCESS             The Hdd Password configuration form is updated.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
HddPasswordConfigUpdateForm (
  IN EFI_HII_HANDLE              HiiHandle,
  IN EFI_ATA_PASS_THRU_PROTOCOL  *AtaPassThru,
  IN EFI_PCI_IO_PROTOCOL         *PciIo,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       Bus,
  IN UINTN                       Device,
  IN UINTN                       Function,
  IN UINT16                      Port,
  IN UINT16                      PortMultiplierPort
  )
{
  LIST_ENTRY                       *Entry;
  HDD_PASSWORD_CONFIG_FORM_ENTRY   *ConfigFormEntry;
  BOOLEAN                          EntryExisted;
  EFI_STATUS                       Status;
  VOID                             *StartOpCodeHandle;
  VOID                             *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL               *StartLabel;
  EFI_IFR_GUID_LABEL               *EndLabel;
  CHAR16                           HddString[40];
  ATA_IDENTIFY_DATA                IdentifyData;
  EFI_DEVICE_PATH_PROTOCOL         *AtaDeviceNode;

  ConfigFormEntry = NULL;
  EntryExisted    = FALSE;

  BASE_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
    ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);

    if ((ConfigFormEntry->Bus == Bus) &&
        (ConfigFormEntry->Device == Device) &&
        (ConfigFormEntry->Function == Function) &&
        (ConfigFormEntry->Port == Port) &&
        (ConfigFormEntry->PortMultiplierPort == PortMultiplierPort)) {
      EntryExisted = TRUE;
      break;
    }
  }

  if (!EntryExisted) {
    //
    // Add a new form.
    //
    ConfigFormEntry = AllocateZeroPool (sizeof (HDD_PASSWORD_CONFIG_FORM_ENTRY));
    if (ConfigFormEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    InitializeListHead (&ConfigFormEntry->Link);
    ConfigFormEntry->Controller         = Controller;
    ConfigFormEntry->Bus                = Bus;
    ConfigFormEntry->Device             = Device;
    ConfigFormEntry->Function           = Function;
    ConfigFormEntry->Port               = Port;
    ConfigFormEntry->PortMultiplierPort = PortMultiplierPort;
    ConfigFormEntry->AtaPassThru        = AtaPassThru;

    DEBUG ((DEBUG_INFO, "HddPasswordDxe: Create new form for device[%d][%d] at Bus 0x%x Dev 0x%x Func 0x%x\n", Port, PortMultiplierPort, Bus, Device, Function));

    //
    // Construct the device path for the HDD password device
    //
    Status = AtaPassThru->BuildDevicePath (
                            AtaPassThru,
                            Port,
                            PortMultiplierPort,
                            &AtaDeviceNode
                            );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    ConfigFormEntry->DevicePath = AppendDevicePathNode (DevicePathFromHandle (Controller), AtaDeviceNode);
    FreePool (AtaDeviceNode);
    if (ConfigFormEntry->DevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Get attached harddisk model number
    //
    Status = GetHddDeviceIdentifyData (AtaPassThru, Port, PortMultiplierPort, &IdentifyData);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    GetHddDeviceModelNumber (&IdentifyData, HddString);
    //
    // Compose the HDD title string and help string of this port and create a new EFI_STRING_ID.
    //
    UnicodeSPrint (ConfigFormEntry->HddString, sizeof (ConfigFormEntry->HddString), L"HDD %d:%s", mNumberOfHddDevices, HddString);
    ConfigFormEntry->TitleToken     = HiiSetString (HiiHandle, 0, ConfigFormEntry->HddString, NULL);
    ConfigFormEntry->TitleHelpToken = HiiSetString (HiiHandle, 0, L"Request to set HDD Password", NULL);

    GetHddPasswordSecurityStatus (&IdentifyData, &ConfigFormEntry->IfrData);

    InsertTailList (&mHddPasswordConfigFormList, &ConfigFormEntry->Link);

    //
    // Init OpCode Handle
    //
    StartOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (StartOpCodeHandle != NULL);

    EndOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (EndOpCodeHandle != NULL);

    //
    // Create Hii Extend Label OpCode as the start opcode
    //
    StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    StartLabel->Number       = HDD_DEVICE_ENTRY_LABEL;

    //
    // Create Hii Extend Label OpCode as the end opcode
    //
    EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    EndLabel->Number       = HDD_DEVICE_LABEL_END;

    mNumberOfHddDevices = 0;
    BASE_LIST_FOR_EACH (Entry, &mHddPasswordConfigFormList) {
      ConfigFormEntry = BASE_CR (Entry, HDD_PASSWORD_CONFIG_FORM_ENTRY, Link);

      HiiCreateGotoOpCode (
        StartOpCodeHandle,                                // Container for dynamic created opcodes
        FORMID_HDD_DEVICE_FORM,                           // Target Form ID
        ConfigFormEntry->TitleToken,                      // Prompt text
        ConfigFormEntry->TitleHelpToken,                  // Help text
        EFI_IFR_FLAG_CALLBACK,                            // Question flag
        (UINT16) (KEY_HDD_DEVICE_ENTRY_BASE + mNumberOfHddDevices)   // Question ID
        );

      mNumberOfHddDevices++;
    }

    HiiUpdateForm (
      HiiHandle,
      &mHddPasswordVendorGuid,
      FORMID_HDD_MAIN_FORM,
      StartOpCodeHandle,
      EndOpCodeHandle
      );

    HiiFreeOpCodeHandle (StartOpCodeHandle);
    HiiFreeOpCodeHandle (EndOpCodeHandle);

    //
    // Check if device is locked and prompt for password.
    //
    HddPasswordRequestPassword (AtaPassThru, Port, PortMultiplierPort, ConfigFormEntry);

    //
    // Process HDD password request from last boot.
    //
    ProcessHddPasswordRequest (AtaPassThru, Port, PortMultiplierPort, ConfigFormEntry);
  }

  return EFI_SUCCESS;
}

/**
  Ata Pass Thru Protocol notification event handler.

  Check attached harddisk status to see if it's locked. If yes, then pop up a password windows to require user input.
  It also registers a form for user configuration on Hdd password configuration.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
HddPasswordNotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                        Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA     *Private;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
  UINT16                            Port;
  UINT16                            PortMultiplierPort;
  EFI_HANDLE                        Controller;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             HandleCount;
  UINTN                             Index;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  UINTN                             SegNum;
  UINTN                             BusNum;
  UINTN                             DevNum;
  UINTN                             FuncNum;

  if (mHddPasswordEndOfDxe) {
    gBS->CloseEvent (Event);
    return;
  }

  Private = (HDD_PASSWORD_DXE_PRIVATE_DATA *)Context;

  //
  // Locate all handles of AtaPassThru protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiAtaPassThruProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Check attached hard disk status to see if it's locked
  //
  for (Index = 0; Index < HandleCount; Index += 1) {
    Controller = HandleBuffer[Index];
    Status = gBS->HandleProtocol (
                    Controller,
                    &gEfiAtaPassThruProtocolGuid,
                    (VOID **) &AtaPassThru
                    );
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Ignore those logical ATA_PASS_THRU instance.
    //
    if ((AtaPassThru->Mode->Attributes & EFI_ATA_PASS_THRU_ATTRIBUTES_PHYSICAL) == 0) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo
                    );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = PciIo->GetLocation (
                      PciIo,
                      &SegNum,
                      &BusNum,
                      &DevNum,
                      &FuncNum
                      );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Assume and only support Segment == 0.
    //
    ASSERT (SegNum == 0);

    //
    // traverse all attached harddisk devices to update form and unlock it
    //
    Port = 0xFFFF;

    while (TRUE) {
      Status = AtaPassThru->GetNextPort (AtaPassThru, &Port);
      if (EFI_ERROR (Status)) {
        //
        // We cannot find more legal port then we are done.
        //
        break;
      }

      PortMultiplierPort = 0xFFFF;
      while (TRUE) {
        Status = AtaPassThru->GetNextDevice (AtaPassThru, Port, &PortMultiplierPort);
        if (EFI_ERROR (Status)) {
          //
          // We cannot find more legal port multiplier port number for ATA device
          // on the port, then we are done.
          //
          break;
        }
        //
        // Find out the attached harddisk devices.
        // Try to add a HDD Password configuration page for the attached devices.
        //
        gBS->RestoreTPL (TPL_APPLICATION);
        Status = HddPasswordConfigUpdateForm (Private->HiiHandle, AtaPassThru, PciIo, Controller, BusNum, DevNum, FuncNum, Port, PortMultiplierPort);
        gBS->RaiseTPL (TPL_CALLBACK);
        if (EFI_ERROR (Status)) {
          break;
        }
      }
    }
  }

  FreePool (HandleBuffer);
  return ;
}

/**
  Initialize the HDD Password configuration form.

  @param[out] Instance             Pointer to private instance.

  @retval EFI_SUCCESS              The HDD Password configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
**/
EFI_STATUS
HddPasswordConfigFormInit (
  OUT HDD_PASSWORD_DXE_PRIVATE_DATA    **Instance
  )
{
  EFI_STATUS                       Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA    *Private;

  InitializeListHead (&mHddPasswordConfigFormList);

  Private = AllocateZeroPool (sizeof (HDD_PASSWORD_DXE_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature   = HDD_PASSWORD_DXE_PRIVATE_SIGNATURE;

  Private->ConfigAccess.ExtractConfig = HddPasswordFormExtractConfig;
  Private->ConfigAccess.RouteConfig   = HddPasswordFormRouteConfig;
  Private->ConfigAccess.Callback      = HddPasswordFormCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHddPasswordHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &Private->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    FreePool(Private);
    return Status;
  }

  //
  // Publish our HII data
  //
  Private->HiiHandle = HiiAddPackages (
                         &mHddPasswordVendorGuid,
                         Private->DriverHandle,
                         HddPasswordDxeStrings,
                         HddPasswordBin,
                         NULL
                         );
  if (Private->HiiHandle == NULL) {
    FreePool(Private);
    return EFI_OUT_OF_RESOURCES;
  }

  *Instance = Private;
  return Status;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
HddPasswordDxeInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                     Status;
  HDD_PASSWORD_DXE_PRIVATE_DATA  *Private;
  VOID                           *Registration;
  EFI_EVENT                      EndOfDxeEvent;
  EDKII_VARIABLE_LOCK_PROTOCOL   *VariableLock;

  Private = NULL;

  //
  // Initialize the configuration form of HDD Password.
  //
  Status = HddPasswordConfigFormInit (&Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register HddPasswordNotificationEvent() notify function.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiAtaPassThruProtocolGuid,
    TPL_CALLBACK,
    HddPasswordNotificationEvent,
    (VOID *)Private,
    &Registration
    );

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  HddPasswordEndOfDxeEventNotify,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Make HDD_PASSWORD_VARIABLE_NAME variable read-only.
  //
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
  if (!EFI_ERROR (Status)) {
    Status = VariableLock->RequestToLock (
                             VariableLock,
                             HDD_PASSWORD_VARIABLE_NAME,
                             &mHddPasswordVendorGuid
                             );
    DEBUG ((DEBUG_INFO, "%a(): Lock %s variable (%r)\n", __FUNCTION__, HDD_PASSWORD_VARIABLE_NAME, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

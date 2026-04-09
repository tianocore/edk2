/** @file
  HddPassword PEI module which is used to unlock HDD password for S3.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HddPasswordPei.h"

EFI_GUID  mHddPasswordDeviceInfoGuid = HDD_PASSWORD_DEVICE_INFO_GUID;

/**
  Send unlock hdd password cmd through ATA PassThru PPI.

  @param[in] AtaPassThru           The pointer to the ATA PassThru PPI.
  @param[in] Port                  The port number of the ATA device.
  @param[in] PortMultiplierPort    The port multiplier port number of the ATA device.
  @param[in] Identifier            The identifier to set user or master password.
  @param[in] Password              The hdd password of attached ATA device.

  @retval EFI_SUCCESS              Successful to send unlock hdd password cmd.
  @retval EFI_INVALID_PARAMETER    The parameter passed-in is invalid.
  @retval EFI_OUT_OF_RESOURCES     Not enough memory to send unlock hdd password cmd.
  @retval EFI_DEVICE_ERROR         Can not send unlock hdd password cmd.

**/
EFI_STATUS
UnlockDevice (
  IN EDKII_PEI_ATA_PASS_THRU_PPI  *AtaPassThru,
  IN UINT16                       Port,
  IN UINT16                       PortMultiplierPort,
  IN CHAR8                        Identifier,
  IN CHAR8                        *Password
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
  Acb.AtaDeviceHead = (UINT8)(PortMultiplierPort == 0xFFFF ? 0 : (PortMultiplierPort << 4));

  //
  // Prepare for ATA pass through packet.
  //
  ZeroMem (&Packet, sizeof (Packet));
  Packet.Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT;
  Packet.Length   = EFI_ATA_PASS_THRU_LENGTH_BYTES;
  Packet.Asb      = Asb;
  Packet.Acb      = &Acb;

  ((CHAR16 *)Buffer)[0] = Identifier & BIT0;
  CopyMem (&((CHAR16 *)Buffer)[1], Password, HDD_PASSWORD_MAX_LENGTH);

  Packet.OutDataBuffer     = Buffer;
  Packet.OutTransferLength = sizeof (Buffer);
  Packet.Timeout           = ATA_TIMEOUT;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          Port,
                          PortMultiplierPort,
                          &Packet
                          );
  if (!EFI_ERROR (Status) &&
      ((Asb->AtaStatus & ATA_STSREG_ERR) != 0) &&
      ((Asb->AtaError & ATA_ERRREG_ABRT) != 0))
  {
    Status = EFI_DEVICE_ERROR;
  }

  FreeAlignedPages (Asb, EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)));

  ZeroMem (Buffer, sizeof (Buffer));

  DEBUG ((DEBUG_INFO, "%a() - %r\n", __func__, Status));
  return Status;
}

/**
  Send security freeze lock cmd through ATA PassThru PPI.

  @param[in] AtaPassThru           The pointer to the ATA PassThru PPI.
  @param[in] Port                  The port number of the ATA device.
  @param[in] PortMultiplierPort    The port multiplier port number of the ATA device.

  @retval EFI_SUCCESS              Successful to send security freeze lock cmd.
  @retval EFI_INVALID_PARAMETER    The parameter passed-in is invalid.
  @retval EFI_OUT_OF_RESOURCES     Not enough memory to send unlock hdd password cmd.
  @retval EFI_DEVICE_ERROR         Can not send security freeze lock cmd.

**/
EFI_STATUS
FreezeLockDevice (
  IN EDKII_PEI_ATA_PASS_THRU_PPI  *AtaPassThru,
  IN UINT16                       Port,
  IN UINT16                       PortMultiplierPort
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
  Acb.AtaDeviceHead = (UINT8)(PortMultiplierPort == 0xFFFF ? 0 : (PortMultiplierPort << 4));

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
                          &Packet
                          );
  if (!EFI_ERROR (Status) &&
      ((Asb->AtaStatus & ATA_STSREG_ERR) != 0) &&
      ((Asb->AtaError & ATA_ERRREG_ABRT) != 0))
  {
    Status = EFI_DEVICE_ERROR;
  }

  FreeAlignedPages (Asb, EFI_SIZE_TO_PAGES (sizeof (EFI_ATA_STATUS_BLOCK)));

  DEBUG ((DEBUG_INFO, "%a() - %r\n", __func__, Status));
  return Status;
}

/**
  Unlock HDD password for S3.

  @param[in] AtaPassThruPpi    Pointer to the EDKII_PEI_ATA_PASS_THRU_PPI instance.

**/
VOID
UnlockHddPassword (
  IN EDKII_PEI_ATA_PASS_THRU_PPI  *AtaPassThruPpi
  )
{
  EFI_STATUS                Status;
  VOID                      *Buffer;
  UINTN                     Length;
  UINT8                     DummyData;
  HDD_PASSWORD_DEVICE_INFO  *DevInfo;
  UINT16                    Port;
  UINT16                    PortMultiplierPort;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathLength;

  //
  // Get HDD password device info from LockBox.
  //
  Buffer = (VOID *)&DummyData;
  Length = sizeof (DummyData);
  Status = RestoreLockBox (&mHddPasswordDeviceInfoGuid, Buffer, &Length);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocatePages (EFI_SIZE_TO_PAGES (Length));
    if (Buffer != NULL) {
      Status = RestoreLockBox (&mHddPasswordDeviceInfoGuid, Buffer, &Length);
    }
  }

  if ((Buffer == NULL) || (Buffer == (VOID *)&DummyData)) {
    return;
  } else if (EFI_ERROR (Status)) {
    FreePages (Buffer, EFI_SIZE_TO_PAGES (Length));
    return;
  }

  Status = AtaPassThruPpi->GetDevicePath (AtaPassThruPpi, &DevicePathLength, &DevicePath);
  if (EFI_ERROR (Status) || (DevicePathLength <= sizeof (EFI_DEVICE_PATH_PROTOCOL))) {
    goto Exit;
  }

  //
  // Go through all the devices managed by the AtaPassThru PPI instance.
  //
  Port = 0xFFFF;
  while (TRUE) {
    Status = AtaPassThruPpi->GetNextPort (AtaPassThruPpi, &Port);
    if (EFI_ERROR (Status)) {
      //
      // We cannot find more legal port then we are done.
      //
      break;
    }

    PortMultiplierPort = 0xFFFF;
    while (TRUE) {
      Status = AtaPassThruPpi->GetNextDevice (AtaPassThruPpi, Port, &PortMultiplierPort);
      if (EFI_ERROR (Status)) {
        //
        // We cannot find more legal port multiplier port number for ATA device
        // on the port, then we are done.
        //
        break;
      }

      //
      // Search the device in the restored LockBox.
      //
      DevInfo = (HDD_PASSWORD_DEVICE_INFO *)Buffer;
      while ((UINTN)DevInfo < ((UINTN)Buffer + Length)) {
        //
        // Find the matching device.
        //
        if ((DevInfo->Device.Port == Port) &&
            (DevInfo->Device.PortMultiplierPort == PortMultiplierPort) &&
            (DevInfo->DevicePathLength >= DevicePathLength) &&
            (CompareMem (
               DevInfo->DevicePath,
               DevicePath,
               DevicePathLength - sizeof (EFI_DEVICE_PATH_PROTOCOL)
               ) == 0))
        {
          //
          // If device locked, unlock first.
          //
          if (!IsZeroBuffer (DevInfo->Password, HDD_PASSWORD_MAX_LENGTH)) {
            UnlockDevice (AtaPassThruPpi, Port, PortMultiplierPort, 0, DevInfo->Password);
          }

          //
          // Freeze lock the device.
          //
          FreezeLockDevice (AtaPassThruPpi, Port, PortMultiplierPort);
          break;
        }

        DevInfo = (HDD_PASSWORD_DEVICE_INFO *)
                  ((UINTN)DevInfo + sizeof (HDD_PASSWORD_DEVICE_INFO) + DevInfo->DevicePathLength);
      }
    }
  }

Exit:
  ZeroMem (Buffer, Length);
  FreePages (Buffer, EFI_SIZE_TO_PAGES (Length));
}

/**
  Entry point of the notification callback function itself within the PEIM.
  It is to unlock HDD password for S3.

  @param  PeiServices      Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor Address of the notification descriptor data structure.
  @param  Ppi              Address of the PPI that was installed.

  @return Status of the notification.
          The status code returned from this function is ignored.
**/
EFI_STATUS
EFIAPI
HddPasswordAtaPassThruNotify (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  )
{
  DEBUG ((DEBUG_INFO, "%a() - enter at S3 resume\n", __func__));

  UnlockHddPassword ((EDKII_PEI_ATA_PASS_THRU_PPI *)Ppi);

  DEBUG ((DEBUG_INFO, "%a() - exit at S3 resume\n", __func__));

  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mHddPasswordAtaPassThruPpiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiPeiAtaPassThruPpiGuid,
  HddPasswordAtaPassThruNotify
};

/**
  Main entry for this module.

  @param FileHandle             Handle of the file being invoked.
  @param PeiServices            Pointer to PEI Services table.

  @return Status from PeiServicesNotifyPpi.

**/
EFI_STATUS
EFIAPI
HddPasswordPeiInit (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;

  Status = PeiServicesGetBootMode (&BootMode);
  if ((EFI_ERROR (Status)) || (BootMode != BOOT_ON_S3_RESUME)) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "%a: Enters in S3 path.\n", __func__));

  Status = PeiServicesNotifyPpi (&mHddPasswordAtaPassThruPpiNotifyDesc);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

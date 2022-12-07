/** @file
  This file provides some helper functions which are specific for SD card device.

  Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciHcDxe.h"

/**
  Send command GO_IDLE_STATE to the device to make it go to Idle State.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The SD device is reset correctly.
  @retval Others            The device reset fails.

**/
EFI_STATUS
SdCardReset (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_GO_IDLE_STATE;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBc;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SEND_IF_COND to the device to inquiry the SD Memory Card interface
  condition.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] SupplyVoltage  The supplied voltage by the host.
  @param[in] CheckPattern   The check pattern to be sent to the device.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardVoltageCheck (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT8                          SupplyVoltage,
  IN UINT8                          CheckPattern
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex    = SD_SEND_IF_COND;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR7;
  SdMmcCmdBlk.CommandArgument = (SupplyVoltage << 8) | CheckPattern;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  if (!EFI_ERROR (Status)) {
    if (SdMmcStatusBlk.Resp0 != SdMmcCmdBlk.CommandArgument) {
      return EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

/**
  Send command SDIO_SEND_OP_COND to the device to see whether it is SDIO device.

  Refer to SDIO Simplified Spec 3 Section 3.2 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] VoltageWindow  The supply voltage window.
  @param[in] S18R           The boolean to show if it should switch to 1.8v.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdioSendOpCond (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT32                         VoltageWindow,
  IN BOOLEAN                        S18R
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;
  UINT32                               Switch;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SDIO_SEND_OP_COND;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR4;

  Switch = S18R ? BIT24 : 0;

  SdMmcCmdBlk.CommandArgument = (VoltageWindow & 0xFFFFFF) | Switch;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SD_SEND_OP_COND to the device to see whether it is SDIO device.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot           The slot number of the SD card to send the command to.
  @param[in]  Rca            The relative device address of addressed device.
  @param[in]  VoltageWindow  The supply voltage window.
  @param[in]  S18R           The boolean to show if it should switch to 1.8v.
  @param[in]  Xpc            The boolean to show if it should provide 0.36w power control.
  @param[in]  Hcs            The boolean to show if it support host capacity info.
  @param[out] Ocr            The buffer to store returned OCR register value.

  @retval EFI_SUCCESS        The operation is done correctly.
  @retval Others             The operation fails.

**/
EFI_STATUS
SdCardSendOpCond (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN     UINT8                          Slot,
  IN     UINT16                         Rca,
  IN     UINT32                         VoltageWindow,
  IN     BOOLEAN                        S18R,
  IN     BOOLEAN                        Xpc,
  IN     BOOLEAN                        Hcs,
  OUT UINT32                            *Ocr
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;
  UINT32                               Switch;
  UINT32                               MaxPower;
  UINT32                               HostCapacity;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex    = SD_APP_CMD;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SdMmcCmdBlk.CommandIndex = SD_SEND_OP_COND;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR3;

  Switch       = S18R ? BIT24 : 0;
  MaxPower     = Xpc ? BIT28 : 0;
  HostCapacity = Hcs ? BIT30 : 0;

  SdMmcCmdBlk.CommandArgument = (VoltageWindow & 0xFFFFFF) | Switch | MaxPower | HostCapacity;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    *Ocr = SdMmcStatusBlk.Resp0;
  }

  return Status;
}

/**
  Broadcast command ALL_SEND_CID to the bus to ask all the SD devices to send the
  data of their CID registers.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardAllSendCid (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_ALL_SEND_CID;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SET_RELATIVE_ADDR to the SD device to assign a Relative device
  Address (RCA).

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[out] Rca           The relative device address to assign.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSetRca (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN     UINT8                          Slot,
  OUT UINT16                            *Rca
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SET_RELATIVE_ADDR;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR6;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    *Rca = (UINT16)(SdMmcStatusBlk.Resp0 >> 16);
  }

  return Status;
}

/**
  Send command SELECT_DESELECT_CARD to the SD device to select/deselect it.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of selected device.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSelect (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SELECT_DESELECT_CARD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  if (Rca != 0) {
    SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1b;
  }

  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command VOLTAGE_SWITCH to the SD device to switch the voltage of the device.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardVoltageSwitch (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex    = SD_VOLTAGE_SWITCH;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = 0;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SET_BUS_WIDTH to the SD device to set the bus width.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address of addressed device.
  @param[in] BusWidth       The bus width to be set, it could be 1 or 4.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSetBusWidth (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN UINT8                          BusWidth
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;
  UINT8                                Value;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex    = SD_APP_CMD;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SdMmcCmdBlk.CommandIndex = SD_SET_BUS_WIDTH;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;

  if (BusWidth == 1) {
    Value = 0;
  } else if (BusWidth == 4) {
    Value = 2;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  SdMmcCmdBlk.CommandArgument = Value & 0x3;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  return Status;
}

/**
  Send command SWITCH_FUNC to the SD device to check switchable function or switch card function.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot           The slot number of the SD card to send the command to.
  @param[in]  BusTiming      Target bus timing based on which access group value will be set.
  @param[in]  CommandSystem  The value for command set group.
  @param[in]  DriverStrength The value for driver strength group.
  @param[in]  PowerLimit     The value for power limit group.
  @param[in]  Mode           Switch or check function.
  @param[out] SwitchResp     The return switch function status.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSwitch (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN     UINT8                          Slot,
  IN     SD_MMC_BUS_MODE                BusTiming,
  IN     UINT8                          CommandSystem,
  IN     SD_DRIVER_STRENGTH_TYPE        DriverStrength,
  IN     UINT8                          PowerLimit,
  IN     BOOLEAN                        Mode,
  OUT UINT8                             *SwitchResp
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;
  UINT32                               ModeValue;
  UINT8                                AccessMode;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SWITCH_FUNC;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;

  ModeValue = Mode ? BIT31 : 0;

  switch (BusTiming) {
    case SdMmcUhsDdr50:
      AccessMode = 0x4;
      break;
    case SdMmcUhsSdr104:
      AccessMode = 0x3;
      break;
    case SdMmcUhsSdr50:
      AccessMode = 0x2;
      break;
    case SdMmcUhsSdr25:
    case SdMmcSdHs:
      AccessMode = 0x1;
      break;
    case SdMmcUhsSdr12:
    case SdMmcSdDs:
      AccessMode = 0;
      break;
    default:
      AccessMode = 0xF;
  }

  SdMmcCmdBlk.CommandArgument = (AccessMode & 0xF) | ((CommandSystem & 0xF) << 4) | \
                                ((DriverStrength & 0xF) << 8) | ((PowerLimit & 0xF) << 12) | \
                                ModeValue;

  Packet.InDataBuffer     = SwitchResp;
  Packet.InTransferLength = 64;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Mode) {
    if ((((AccessMode & 0xF) != 0xF) && ((SwitchResp[16] & 0xF) != AccessMode)) ||
        (((CommandSystem & 0xF) != 0xF) && (((SwitchResp[16] >> 4) & 0xF) != CommandSystem)) ||
        (((DriverStrength & 0xF) != 0xF) && ((SwitchResp[15] & 0xF) != DriverStrength)) ||
        (((PowerLimit & 0xF) != 0xF) && (((SwitchResp[15] >> 4) & 0xF) != PowerLimit)))
    {
      return EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

/**
  Send command SEND_STATUS to the addressed SD device to get its status register.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of addressed device.
  @param[out] DevStatus     The returned device status.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSendStatus (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN     UINT8                          Slot,
  IN     UINT16                         Rca,
  OUT UINT32                            *DevStatus
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex    = SD_SEND_STATUS;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    *DevStatus = SdMmcStatusBlk.Resp0;
  }

  return Status;
}

/**
  Send command SEND_TUNING_BLOCK to the SD device for HS200 optimal sampling point
  detection.

  It may be sent up to 40 times until the host finishes the tuning procedure.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSendTuningBlk (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;
  UINT8                                TuningBlock[64];

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex    = SD_SEND_TUNING_BLOCK;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = 0;

  Packet.InDataBuffer     = TuningBlock;
  Packet.InTransferLength = sizeof (TuningBlock);

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Tunning the sampling point of SDR104 or SDR50 bus speed mode.

  Command SD_SEND_TUNING_BLOCK may be sent up to 40 times until the host finishes the
  tuning procedure.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 and
  SD Host Controller Simplified Spec 3.0 section Figure 3-7 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardTuningClock (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot
  )
{
  EFI_STATUS  Status;
  UINT8       HostCtrl2;
  UINT8       Retry;

  //
  // Notify the host that the sampling clock tuning procedure starts.
  //
  HostCtrl2 = BIT6;
  Status    = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Ask the device to send a sequence of tuning blocks till the tuning procedure is done.
  //
  Retry = 0;
  do {
    Status = SdCardSendTuningBlk (PassThru, Slot);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdCardSendTuningBlk: Send tuning block fails with %r\n", Status));
      return Status;
    }

    Status = SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, TRUE, sizeof (HostCtrl2), &HostCtrl2);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((HostCtrl2 & (BIT6 | BIT7)) == 0) {
      break;
    }

    if ((HostCtrl2 & (BIT6 | BIT7)) == BIT7) {
      return EFI_SUCCESS;
    }
  } while (++Retry < 40);

  DEBUG ((DEBUG_ERROR, "SdCardTuningClock: Send tuning block fails at %d times with HostCtrl2 %02x\n", Retry, HostCtrl2));
  //
  // Abort the tuning procedure and reset the tuning circuit.
  //
  HostCtrl2 = (UINT8) ~(BIT6 | BIT7);
  Status    = SdMmcHcAndMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_DEVICE_ERROR;
}

/**
  Switch the bus width to specified width.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 and
  SD Host Controller Simplified Spec 3.0 section Figure 3-7 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSwitchBusWidth (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN UINT8                          BusWidth
  )
{
  EFI_STATUS  Status;
  UINT32      DevStatus;

  Status = SdCardSetBusWidth (PassThru, Slot, Rca, BusWidth);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdCardSwitchBusWidth: Switch to bus width %d fails with %r\n", BusWidth, Status));
    return Status;
  }

  Status = SdCardSendStatus (PassThru, Slot, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdCardSwitchBusWidth: Send status fails with %r\n", Status));
    return Status;
  }

  //
  // Check the switch operation is really successful or not.
  //
  if ((DevStatus >> 16) != 0) {
    DEBUG ((DEBUG_ERROR, "SdCardSwitchBusWidth: The switch operation fails as DevStatus is 0x%08x\n", DevStatus));
    return EFI_DEVICE_ERROR;
  }

  Status = SdMmcHcSetBusWidth (PciIo, Slot, BusWidth);

  return Status;
}

/**
  Check if passed BusTiming is supported in both controller and card.

  @param[in] Private                  Pointer to controller private data
  @param[in] SlotIndex                Index of the slot in the controller
  @param[in] CardSupportedBusTimings  Bitmask indicating which bus timings are supported by card
  @param[in] IsInUhsI                 Flag indicating if link is in UHS-I

  @retval TRUE  Both card and controller support given BusTiming
  @retval FALSE Card or controller doesn't support given BusTiming
**/
BOOLEAN
SdIsBusTimingSupported (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN UINT8                   CardSupportedBusTimings,
  IN BOOLEAN                 IsInUhsI,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  SD_MMC_HC_SLOT_CAP  *Capability;

  Capability = &Private->Capability[SlotIndex];

  if (IsInUhsI) {
    switch (BusTiming) {
      case SdMmcUhsSdr104:
        if ((Capability->Sdr104 != 0) && ((CardSupportedBusTimings & BIT3) != 0)) {
          return TRUE;
        }

        break;
      case SdMmcUhsDdr50:
        if ((Capability->Ddr50 != 0) && ((CardSupportedBusTimings & BIT4) != 0)) {
          return TRUE;
        }

        break;
      case SdMmcUhsSdr50:
        if ((Capability->Sdr50 != 0) && ((CardSupportedBusTimings & BIT2) != 0)) {
          return TRUE;
        }

        break;
      case SdMmcUhsSdr25:
        if ((CardSupportedBusTimings & BIT1) != 0) {
          return TRUE;
        }

        break;
      case SdMmcUhsSdr12:
        if ((CardSupportedBusTimings & BIT0) != 0) {
          return TRUE;
        }

        break;
      default:
        break;
    }
  } else {
    switch (BusTiming) {
      case SdMmcSdHs:
        if ((Capability->HighSpeed != 0) && ((CardSupportedBusTimings & BIT1) != 0)) {
          return TRUE;
        }

        break;
      case SdMmcSdDs:
        if ((CardSupportedBusTimings & BIT0) != 0) {
          return TRUE;
        }

        break;
      default:
        break;
    }
  }

  return FALSE;
}

/**
  Get the target bus timing to set on the link. This function
  will try to select highest bus timing supported by card, controller
  and the driver.

  @param[in] Private                  Pointer to controller private data
  @param[in] SlotIndex                Index of the slot in the controller
  @param[in] CardSupportedBusTimings  Bitmask indicating which bus timings are supported by card
  @param[in] IsInUhsI                 Flag indicating if link is in UHS-I

  @return  Bus timing value that should be set on link
**/
SD_MMC_BUS_MODE
SdGetTargetBusTiming (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN UINT8                   CardSupportedBusTimings,
  IN BOOLEAN                 IsInUhsI
  )
{
  SD_MMC_BUS_MODE  BusTiming;

  if (IsInUhsI) {
    BusTiming = SdMmcUhsSdr104;
  } else {
    BusTiming = SdMmcSdHs;
  }

  while (BusTiming > SdMmcSdDs) {
    if (SdIsBusTimingSupported (Private, SlotIndex, CardSupportedBusTimings, IsInUhsI, BusTiming)) {
      break;
    }

    BusTiming--;
  }

  return BusTiming;
}

/**
  Get the target bus width to be set on the bus.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] BusTiming  Bus timing set on the bus

  @return Bus width to be set on the bus
**/
UINT8
SdGetTargetBusWidth (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  UINT8  BusWidth;
  UINT8  PreferredBusWidth;

  PreferredBusWidth = Private->Slot[SlotIndex].OperatingParameters.BusWidth;

  if ((BusTiming == SdMmcSdDs) || (BusTiming == SdMmcSdHs)) {
    if ((PreferredBusWidth != EDKII_SD_MMC_BUS_WIDTH_IGNORE) &&
        ((PreferredBusWidth == 1) || (PreferredBusWidth == 4)))
    {
      BusWidth = PreferredBusWidth;
    } else {
      BusWidth = 4;
    }
  } else {
    //
    // UHS-I modes support only 4-bit width.
    // Switch to 4-bit has been done before calling this function anyway so
    // this is purely informational.
    //
    BusWidth = 4;
  }

  return BusWidth;
}

/**
  Get the target clock frequency to be set on the bus.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] BusTiming  Bus timing to be set on the bus

  @return Value of the clock frequency to be set on bus in MHz
**/
UINT32
SdGetTargetBusClockFreq (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  UINT32  PreferredClockFreq;
  UINT32  MaxClockFreq;

  PreferredClockFreq = Private->Slot[SlotIndex].OperatingParameters.ClockFreq;

  switch (BusTiming) {
    case SdMmcUhsSdr104:
      MaxClockFreq = 208;
      break;
    case SdMmcUhsSdr50:
      MaxClockFreq = 100;
      break;
    case SdMmcUhsDdr50:
    case SdMmcUhsSdr25:
    case SdMmcSdHs:
      MaxClockFreq = 50;
      break;
    case SdMmcUhsSdr12:
    case SdMmcSdDs:
    default:
      MaxClockFreq = 25;
  }

  if ((PreferredClockFreq != EDKII_SD_MMC_CLOCK_FREQ_IGNORE) && (PreferredClockFreq < MaxClockFreq)) {
    return PreferredClockFreq;
  } else {
    return MaxClockFreq;
  }
}

/**
  Get the driver strength to be set on bus.

  @param[in] Private                       Pointer to controller private data
  @param[in] SlotIndex                     Index of the slot in the controller
  @param[in] CardSupportedDriverStrengths  Bitmask indicating which driver strengths are supported on the card
  @param[in] BusTiming                     Bus timing set on the bus

  @return Value of the driver strength to be set on the bus
**/
EDKII_SD_MMC_DRIVER_STRENGTH
SdGetTargetDriverStrength (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN UINT8                   CardSupportedDriverStrengths,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  EDKII_SD_MMC_DRIVER_STRENGTH  PreferredDriverStrength;
  EDKII_SD_MMC_DRIVER_STRENGTH  DriverStrength;

  if ((BusTiming == SdMmcSdDs) || (BusTiming == SdMmcSdHs)) {
    DriverStrength.Sd = SdDriverStrengthIgnore;
    return DriverStrength;
  }

  PreferredDriverStrength = Private->Slot[SlotIndex].OperatingParameters.DriverStrength;
  DriverStrength.Sd       = SdDriverStrengthTypeB;

  if ((PreferredDriverStrength.Sd != EDKII_SD_MMC_DRIVER_STRENGTH_IGNORE) &&
      (CardSupportedDriverStrengths & (BIT0 << PreferredDriverStrength.Sd)))
  {
    if (((PreferredDriverStrength.Sd == SdDriverStrengthTypeA) &&
         (Private->Capability[SlotIndex].DriverTypeA != 0)) ||
        ((PreferredDriverStrength.Sd == SdDriverStrengthTypeC) &&
         (Private->Capability[SlotIndex].DriverTypeC != 0)) ||
        ((PreferredDriverStrength.Sd == SdDriverStrengthTypeD) &&
         (Private->Capability[SlotIndex].DriverTypeD != 0)))
    {
      DriverStrength.Sd = PreferredDriverStrength.Sd;
    }
  }

  return DriverStrength;
}

/**
  Get the target settings for the bus mode.

  @param[in]  Private          Pointer to controller private data
  @param[in]  SlotIndex        Index of the slot in the controller
  @param[in]  SwitchQueryResp  Pointer to switch query response
  @param[in]  IsInUhsI         Flag indicating if link is in UHS-I mode
  @param[out] BusMode          Target configuration of the bus
**/
VOID
SdGetTargetBusMode (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN UINT8                   *SwitchQueryResp,
  IN BOOLEAN                 IsInUhsI,
  OUT SD_MMC_BUS_SETTINGS    *BusMode
  )
{
  BusMode->BusTiming      = SdGetTargetBusTiming (Private, SlotIndex, SwitchQueryResp[13], IsInUhsI);
  BusMode->BusWidth       = SdGetTargetBusWidth (Private, SlotIndex, BusMode->BusTiming);
  BusMode->ClockFreq      = SdGetTargetBusClockFreq (Private, SlotIndex, BusMode->BusTiming);
  BusMode->DriverStrength = SdGetTargetDriverStrength (Private, SlotIndex, SwitchQueryResp[9], BusMode->BusTiming);
}

/**
  Switch the high speed timing according to request.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 and
  SD Host Controller Simplified Spec 3.0 section Figure 2-29 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] S18A           The boolean to show if it's a UHS-I SD card.
  @param[in] SdVersion1     The boolean to show if it's a Version 1 SD card.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdCardSetBusMode (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN BOOLEAN                        S18A,
  IN BOOLEAN                        SdVersion1
  )
{
  EFI_STATUS              Status;
  SD_MMC_HC_SLOT_CAP      *Capability;
  UINT8                   HostCtrl1;
  UINT8                   SwitchResp[64];
  SD_MMC_HC_PRIVATE_DATA  *Private;
  SD_MMC_BUS_SETTINGS     BusMode;

  ZeroMem (SwitchResp, 64 * sizeof (UINT8));

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (PassThru);

  Capability = &Private->Capability[Slot];

  Status = SdCardSelect (PassThru, Slot, Rca);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (S18A) {
    //
    // For UHS-I speed modes 4-bit data bus is requiered so we
    // switch here irrespective of platform preference.
    //
    Status = SdCardSwitchBusWidth (PciIo, PassThru, Slot, Rca, 4);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Get the supported bus speed from SWITCH cmd return data group #1.
  // SdVersion1 don't support the SWITCH cmd
  //
  if (!SdVersion1) {
    Status = SdCardSwitch (PassThru, Slot, 0xFF, 0xF, SdDriverStrengthIgnore, 0xF, FALSE, SwitchResp);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  SdGetTargetBusMode (Private, Slot, SwitchResp, S18A, &BusMode);

  DEBUG ((
    DEBUG_INFO,
    "SdCardSetBusMode: Target bus mode: bus timing = %d, bus width = %d, clock freq[MHz] = %d, driver strength = %d\n",
    BusMode.BusTiming,
    BusMode.BusWidth,
    BusMode.ClockFreq,
    BusMode.DriverStrength.Sd
    ));

  if (!S18A) {
    Status = SdCardSwitchBusWidth (PciIo, PassThru, Slot, Rca, BusMode.BusWidth);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // SdVersion1 don't support the SWITCH cmd
  //
  if (!SdVersion1) {
    Status = SdCardSwitch (PassThru, Slot, BusMode.BusTiming, 0xF, BusMode.DriverStrength.Sd, 0xF, TRUE, SwitchResp);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = SdMmcSetDriverStrength (Private->PciIo, Slot, BusMode.DriverStrength.Sd);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set to High Speed timing
  //
  if (BusMode.BusTiming == SdMmcSdHs) {
    HostCtrl1 = BIT2;
    Status    = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = SdMmcHcUhsSignaling (Private->ControllerHandle, PciIo, Slot, BusMode.BusTiming);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdMmcHcClockSupply (Private, Slot, BusMode.BusTiming, FALSE, BusMode.ClockFreq * 1000);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((BusMode.BusTiming == SdMmcUhsSdr104) || ((BusMode.BusTiming == SdMmcUhsSdr50) && (Capability->TuningSDR50 != 0))) {
    Status = SdCardTuningClock (PciIo, PassThru, Slot);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}

/**
  Execute SD device identification procedure.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 3.6 for details.

  @param[in] Private        A pointer to the SD_MMC_HC_PRIVATE_DATA instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       There is a SD card.
  @retval Others            There is not a SD card.

**/
EFI_STATUS
SdCardIdentification (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   Slot
  )
{
  EFI_STATUS                     Status;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru;
  UINT32                         Ocr;
  UINT16                         Rca;
  BOOLEAN                        Xpc;
  BOOLEAN                        S18r;
  UINT64                         MaxCurrent;
  UINT16                         ControllerVer;
  UINT8                          PowerCtrl;
  UINT32                         PresentState;
  UINT8                          HostCtrl2;
  UINTN                          Retry;
  BOOLEAN                        ForceVoltage33;
  BOOLEAN                        SdVersion1;

  ForceVoltage33 = FALSE;
  SdVersion1     = FALSE;

  PciIo    = Private->PciIo;
  PassThru = &Private->PassThru;

Voltage33Retry:
  //
  // 1. Send Cmd0 to the device
  //
  Status = SdCardReset (PassThru, Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SdCardIdentification: Executing Cmd0 fails with %r\n", Status));
    return Status;
  }

  //
  // 2. Send Cmd8 to the device, the command will fail for SdVersion1
  //
  Status = SdCardVoltageCheck (PassThru, Slot, 0x1, 0xFF);
  if (EFI_ERROR (Status)) {
    SdVersion1 = TRUE;
    DEBUG ((DEBUG_INFO, "SdCardIdentification: Executing Cmd8 fails with %r\n", Status));
  }

  //
  // 3. Send SDIO Cmd5 to the device to the SDIO device OCR register.
  //
  Status = SdioSendOpCond (PassThru, Slot, 0, FALSE);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SdCardIdentification: Found SDIO device, ignore it as we don't support\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // 4. Send Acmd41 with voltage window 0 to the device
  //
  Status = SdCardSendOpCond (PassThru, Slot, 0, 0, FALSE, FALSE, FALSE, &Ocr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "SdCardIdentification: Executing SdCardSendOpCond fails with %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  if (Private->Capability[Slot].Voltage33 != 0) {
    //
    // Support 3.3V
    //
    MaxCurrent = ((UINT32)Private->MaxCurrent[Slot] & 0xFF) * 4;
  } else if (Private->Capability[Slot].Voltage30 != 0) {
    //
    // Support 3.0V
    //
    MaxCurrent = (((UINT32)Private->MaxCurrent[Slot] >> 8) & 0xFF) * 4;
  } else if (Private->Capability[Slot].Voltage18 != 0) {
    //
    // Support 1.8V
    //
    MaxCurrent = (((UINT32)Private->MaxCurrent[Slot] >> 16) & 0xFF) * 4;
  } else {
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  if (MaxCurrent >= 150) {
    Xpc = TRUE;
  } else {
    Xpc = FALSE;
  }

  Status = SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_CTRL_VER, TRUE, sizeof (ControllerVer), &ControllerVer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (((ControllerVer & 0xFF) >= SD_MMC_HC_CTRL_VER_300) &&
      ((ControllerVer & 0xFF) <= SD_MMC_HC_CTRL_VER_420))
  {
    S18r = TRUE;
  } else if (((ControllerVer & 0xFF) == SD_MMC_HC_CTRL_VER_100) || ((ControllerVer & 0xFF) == SD_MMC_HC_CTRL_VER_200)) {
    S18r = FALSE;
  } else {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  //
  // 1.8V had failed in the previous run, forcing a retry with 3.3V instead
  //
  if (ForceVoltage33 == TRUE) {
    S18r = FALSE;
  }

  //
  // 5. Repeatly send Acmd41 with supply voltage window to the device.
  //    Note here we only support the cards complied with SD physical
  //    layer simplified spec version 2.0 and version 3.0 and above.
  //
  Ocr   = 0;
  Retry = 0;
  do {
    Status = SdCardSendOpCond (PassThru, Slot, 0, Ocr, S18r, Xpc, TRUE, &Ocr);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdCardIdentification: SdCardSendOpCond fails with %r Ocr %x, S18r %x, Xpc %x\n", Status, Ocr, S18r, Xpc));
      return EFI_DEVICE_ERROR;
    }

    if (Retry++ == 100) {
      DEBUG ((DEBUG_ERROR, "SdCardIdentification: SdCardSendOpCond fails too many times\n"));
      return EFI_DEVICE_ERROR;
    }

    gBS->Stall (10 * 1000);
  } while ((Ocr & BIT31) == 0);

  //
  // 6. If the S18A bit is set and the Host Controller supports 1.8V signaling
  //    (One of support bits is set to 1: SDR50, SDR104 or DDR50 in the
  //    Capabilities register), switch its voltage to 1.8V.
  //
  if (((Private->Capability[Slot].Sdr50 != 0) ||
       (Private->Capability[Slot].Sdr104 != 0) ||
       (Private->Capability[Slot].Ddr50 != 0)) &&
      ((Ocr & BIT24) != 0))
  {
    Status = SdCardVoltageSwitch (PassThru, Slot);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdCardIdentification: Executing SdCardVoltageSwitch fails with %r\n", Status));
      Status = EFI_DEVICE_ERROR;
      goto Error;
    } else {
      Status = SdMmcHcStopClock (PciIo, Slot);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }

      SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_PRESENT_STATE, TRUE, sizeof (PresentState), &PresentState);
      if (((PresentState >> 20) & 0xF) != 0) {
        DEBUG ((DEBUG_ERROR, "SdCardIdentification: SwitchVoltage fails with PresentState = 0x%x\n", PresentState));
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }

      HostCtrl2 = BIT3;
      SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);

      gBS->Stall (5000);

      SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, TRUE, sizeof (HostCtrl2), &HostCtrl2);
      if ((HostCtrl2 & BIT3) == 0) {
        DEBUG ((DEBUG_ERROR, "SdCardIdentification: SwitchVoltage fails with HostCtrl2 = 0x%x\n", HostCtrl2));
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }

      Status = SdMmcHcStartSdClock (PciIo, Slot);
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      gBS->Stall (1000);

      SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_PRESENT_STATE, TRUE, sizeof (PresentState), &PresentState);
      if (((PresentState >> 20) & 0xF) != 0xF) {
        //
        // Delay 50 milliseconds in order for clock to stabilize, retry reading the SD_MMC_HC_PRESENT_STATE
        //
        gBS->Stall (50000);
        SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_PRESENT_STATE, TRUE, sizeof (PresentState), &PresentState);
        if (((PresentState >> 20) & 0xF) != 0xF) {
          DEBUG ((DEBUG_ERROR, "SdCardIdentification: SwitchVoltage fails with PresentState = 0x%x, It should be 0xF\n", PresentState));
          //
          // Reset and reinitialize the slot before the 3.3V retry.
          //
          Status = SdMmcHcReset (Private, Slot);
          if (EFI_ERROR (Status)) {
            goto Error;
          }

          Status = SdMmcHcInitHost (Private, Slot);
          if (EFI_ERROR (Status)) {
            goto Error;
          }

          DEBUG ((DEBUG_ERROR, "SdCardIdentification: Switching to 1.8V failed, forcing a retry with 3.3V instead\n"));
          ForceVoltage33 = TRUE;
          goto Voltage33Retry;
        }
      }
    }

    DEBUG ((DEBUG_INFO, "SdCardIdentification: Switch to 1.8v signal voltage success\n"));
  }

  Status = SdCardAllSendCid (PassThru, Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdCardIdentification: Executing SdCardAllSendCid fails with %r\n", Status));
    return Status;
  }

  Status = SdCardSetRca (PassThru, Slot, &Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdCardIdentification: Executing SdCardSetRca fails with %r\n", Status));
    return Status;
  }

  //
  // Enter Data Tranfer Mode.
  //
  DEBUG ((DEBUG_INFO, "SdCardIdentification: Found a SD device at slot [%d]\n", Slot));
  Private->Slot[Slot].CardType = SdCardType;

  Status = SdCardSetBusMode (PciIo, PassThru, Slot, Rca, ((Ocr & BIT24) != 0), SdVersion1);

  return Status;

Error:
  //
  // Set SD Bus Power = 0
  //
  PowerCtrl = (UINT8) ~BIT0;
  Status    = SdMmcHcAndMmio (PciIo, Slot, SD_MMC_HC_POWER_CTRL, sizeof (PowerCtrl), &PowerCtrl);
  return EFI_DEVICE_ERROR;
}

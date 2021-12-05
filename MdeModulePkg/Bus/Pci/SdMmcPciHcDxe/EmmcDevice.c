/** @file
  This file provides some helper functions which are specific for EMMC device.

  Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciHcDxe.h"

/**
  Send command GO_IDLE_STATE (CMD0 with argument of 0x00000000) to the device to
  make it go to Idle State.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The EMMC device is reset correctly.
  @retval Others            The device reset fails.

**/
EFI_STATUS
EmmcReset (
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

  SdMmcCmdBlk.CommandIndex    = EMMC_GO_IDLE_STATE;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeBc;
  SdMmcCmdBlk.ResponseType    = 0;
  SdMmcCmdBlk.CommandArgument = 0;

  gBS->Stall (1000);

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SEND_OP_COND to the EMMC device to get the data of the OCR register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in]      PassThru  A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]      Slot      The slot number of the SD card to send the command to.
  @param[in, out] Argument  On input, the argument of SEND_OP_COND is to send to the device.
                            On output, the argument is the value of OCR register.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcGetOcr (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN     UINT8                          Slot,
  IN OUT UINT32                         *Argument
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

  SdMmcCmdBlk.CommandIndex    = EMMC_SEND_OP_COND;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR3;
  SdMmcCmdBlk.CommandArgument = *Argument;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    *Argument = SdMmcStatusBlk.Resp0;
  }

  return Status;
}

/**
  Broadcast command ALL_SEND_CID to the bus to ask all the EMMC devices to send the
  data of their CID registers.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcGetAllCid (
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

  SdMmcCmdBlk.CommandIndex    = EMMC_ALL_SEND_CID;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = 0;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SET_RELATIVE_ADDR to the EMMC device to assign a Relative device
  Address (RCA).

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSetRca (
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

  SdMmcCmdBlk.CommandIndex    = EMMC_SET_RELATIVE_ADDR;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SEND_CSD to the EMMC device to get the data of the CSD register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of selected device.
  @param[out] Csd           The buffer to store the content of the CSD register.
                            Note the caller should ignore the lowest byte of this
                            buffer as the content of this byte is meaningless even
                            if the operation succeeds.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcGetCsd (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN     UINT8                          Slot,
  IN     UINT16                         Rca,
  OUT EMMC_CSD                          *Csd
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

  SdMmcCmdBlk.CommandIndex    = EMMC_SEND_CSD;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8 *)Csd) + 1, &SdMmcStatusBlk.Resp0, sizeof (EMMC_CSD) - 1);
  }

  return Status;
}

/**
  Send command SELECT_DESELECT_CARD to the EMMC device to select/deselect it.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of selected device.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSelect (
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

  SdMmcCmdBlk.CommandIndex    = EMMC_SELECT_DESELECT_CARD;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SEND_EXT_CSD to the EMMC device to get the data of the EXT_CSD register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[out] ExtCsd        The buffer to store the content of the EXT_CSD register.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcGetExtCsd (
  IN     EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN     UINT8                          Slot,
  OUT EMMC_EXT_CSD                      *ExtCsd
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

  SdMmcCmdBlk.CommandIndex    = EMMC_SEND_EXT_CSD;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = 0x00000000;

  Packet.InDataBuffer     = ExtCsd;
  Packet.InTransferLength = sizeof (EMMC_EXT_CSD);

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  return Status;
}

/**
  Send command SWITCH to the EMMC device to switch the mode of operation of the
  selected Device or modifies the EXT_CSD registers.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Access        The access mode of SWTICH command.
  @param[in]  Index         The offset of the field to be access.
  @param[in]  Value         The value to be set to the specified field of EXT_CSD register.
  @param[in]  CmdSet        The value of CmdSet field of EXT_CSD register.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitch (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT8                          Access,
  IN UINT8                          Index,
  IN UINT8                          Value,
  IN UINT8                          CmdSet
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

  SdMmcCmdBlk.CommandIndex    = EMMC_SWITCH;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1b;
  SdMmcCmdBlk.CommandArgument = (Access << 24) | (Index << 16) | (Value << 8) | CmdSet;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SEND_STATUS to the addressed EMMC device to get its status register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  PassThru      A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of addressed device.
  @param[out] DevStatus     The returned device status.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSendStatus (
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

  SdMmcCmdBlk.CommandIndex    = EMMC_SEND_STATUS;
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
  Send command SEND_TUNING_BLOCK to the EMMC device for HS200 optimal sampling point
  detection.

  It may be sent up to 40 times until the host finishes the tuning procedure.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 for details.

  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] BusWidth       The bus width to work.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSendTuningBlk (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT8                          BusWidth
  )
{
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_STATUS                           Status;
  UINT8                                TuningBlock[128];

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex    = EMMC_SEND_TUNING_BLOCK;
  SdMmcCmdBlk.CommandType     = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType    = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = 0;

  Packet.InDataBuffer = TuningBlock;
  if (BusWidth == 8) {
    Packet.InTransferLength = sizeof (TuningBlock);
  } else {
    Packet.InTransferLength = 64;
  }

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);

  return Status;
}

/**
  Tunning the clock to get HS200 optimal sampling point.

  Command SEND_TUNING_BLOCK may be sent up to 40 times until the host finishes the
  tuning procedure.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] BusWidth       The bus width to work.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcTuningClkForHs200 (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT8                          BusWidth
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
    Status = EmmcSendTuningBlk (PassThru, Slot, BusWidth);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EmmcTuningClkForHs200: Send tuning block fails with %r\n", Status));
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

  DEBUG ((DEBUG_ERROR, "EmmcTuningClkForHs200: Send tuning block fails at %d times with HostCtrl2 %02x\n", Retry, HostCtrl2));
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
  Check the SWITCH operation status.

  @param[in] PassThru  A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot      The slot number on which command should be sent.
  @param[in] Rca       The relative device address.

  @retval EFI_SUCCESS  The SWITCH finished siccessfully.
  @retval others       The SWITCH failed.
**/
EFI_STATUS
EmmcCheckSwitchStatus (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca
  )
{
  EFI_STATUS  Status;
  UINT32      DevStatus;

  Status = EmmcSendStatus (PassThru, Slot, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcCheckSwitchStatus: Send status fails with %r\n", Status));
    return Status;
  }

  //
  // Check the switch operation is really successful or not.
  //
  if ((DevStatus & BIT7) != 0) {
    DEBUG ((DEBUG_ERROR, "EmmcCheckSwitchStatus: The switch operation fails as DevStatus is 0x%08x\n", DevStatus));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Switch the bus width to specified width.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.9 and SD Host Controller
  Simplified Spec 3.0 Figure 3-7 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] IsDdr          If TRUE, use dual data rate data simpling method. Otherwise
                            use single data rate data simpling method.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchBusWidth (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN BOOLEAN                        IsDdr,
  IN UINT8                          BusWidth
  )
{
  EFI_STATUS  Status;
  UINT8       Access;
  UINT8       Index;
  UINT8       Value;
  UINT8       CmdSet;

  //
  // Write Byte, the Value field is written into the byte pointed by Index.
  //
  Access = 0x03;
  Index  = OFFSET_OF (EMMC_EXT_CSD, BusWidth);
  if (BusWidth == 4) {
    Value = 1;
  } else if (BusWidth == 8) {
    Value = 2;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (IsDdr) {
    Value += 4;
  }

  CmdSet = 0;
  Status = EmmcSwitch (PassThru, Slot, Access, Index, Value, CmdSet);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSwitchBusWidth: Switch to bus width %d fails with %r\n", BusWidth, Status));
    return Status;
  }

  Status = EmmcCheckSwitchStatus (PassThru, Slot, Rca);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdMmcHcSetBusWidth (PciIo, Slot, BusWidth);

  return Status;
}

/**
  Switch the bus timing and clock frequency.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6 and SD Host Controller
  Simplified Spec 3.0 Figure 3-3 for details.

  @param[in] PciIo           A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru        A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot            The slot number of the SD card to send the command to.
  @param[in] Rca             The relative device address to be assigned.
  @param[in] DriverStrength  Driver strength to set for speed modes that support it.
  @param[in] BusTiming       The bus mode timing indicator.
  @param[in] ClockFreq       The max clock frequency to be set, the unit is MHz.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchBusTiming (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN EDKII_SD_MMC_DRIVER_STRENGTH   DriverStrength,
  IN SD_MMC_BUS_MODE                BusTiming,
  IN UINT32                         ClockFreq
  )
{
  EFI_STATUS              Status;
  UINT8                   Access;
  UINT8                   Index;
  UINT8                   Value;
  UINT8                   CmdSet;
  SD_MMC_HC_PRIVATE_DATA  *Private;
  UINT8                   HostCtrl1;
  BOOLEAN                 DelaySendStatus;

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (PassThru);
  //
  // Write Byte, the Value field is written into the byte pointed by Index.
  //
  Access = 0x03;
  Index  = OFFSET_OF (EMMC_EXT_CSD, HsTiming);
  CmdSet = 0;
  switch (BusTiming) {
    case SdMmcMmcHs400:
      Value = (UINT8)((DriverStrength.Emmc << 4) | 3);
      break;
    case SdMmcMmcHs200:
      Value = (UINT8)((DriverStrength.Emmc << 4) | 2);
      break;
    case SdMmcMmcHsSdr:
    case SdMmcMmcHsDdr:
      Value = 1;
      break;
    case SdMmcMmcLegacy:
      Value = 0;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "EmmcSwitchBusTiming: Unsupported BusTiming(%d)\n", BusTiming));
      return EFI_INVALID_PARAMETER;
  }

  Status = EmmcSwitch (PassThru, Slot, Access, Index, Value, CmdSet);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSwitchBusTiming: Switch to bus timing %d fails with %r\n", BusTiming, Status));
    return Status;
  }

  if ((BusTiming == SdMmcMmcHsSdr) || (BusTiming == SdMmcMmcHsDdr)) {
    HostCtrl1 = BIT2;
    Status    = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    HostCtrl1 = (UINT8) ~BIT2;
    Status    = SdMmcHcAndMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = SdMmcHcUhsSignaling (Private->ControllerHandle, PciIo, Slot, BusTiming);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // For cases when we switch bus timing to higher mode from current we want to
  // send SEND_STATUS at current, lower, frequency then the target frequency to avoid
  // stability issues. It has been observed that some designs are unable to process the
  // SEND_STATUS at higher frequency during switch to HS200 @200MHz irrespective of the number of retries
  // and only running the clock tuning is able to make them work at target frequency.
  //
  // For cases when we are downgrading the frequency and current high frequency is invalid
  // we have to first change the frequency to target frequency and then send the SEND_STATUS.
  //
  if (Private->Slot[Slot].CurrentFreq < (ClockFreq * 1000)) {
    Status = EmmcCheckSwitchStatus (PassThru, Slot, Rca);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DelaySendStatus = FALSE;
  } else {
    DelaySendStatus = TRUE;
  }

  //
  // Convert the clock freq unit from MHz to KHz.
  //
  Status = SdMmcHcClockSupply (Private, Slot, BusTiming, FALSE, ClockFreq * 1000);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DelaySendStatus) {
    Status = EmmcCheckSwitchStatus (PassThru, Slot, Rca);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}

/**
  Switch to the High Speed timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] BusMode        Pointer to SD_MMC_BUS_SETTINGS structure containing bus settings.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchToHighSpeed (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN SD_MMC_BUS_SETTINGS            *BusMode
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsDdr;

  if (((BusMode->BusTiming != SdMmcMmcHsSdr) && (BusMode->BusTiming != SdMmcMmcHsDdr) && (BusMode->BusTiming != SdMmcMmcLegacy)) ||
      (BusMode->ClockFreq > 52))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (BusMode->BusTiming == SdMmcMmcHsDdr) {
    IsDdr = TRUE;
  } else {
    IsDdr = FALSE;
  }

  Status = EmmcSwitchBusWidth (PciIo, PassThru, Slot, Rca, IsDdr, BusMode->BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, BusMode->DriverStrength, BusMode->BusTiming, BusMode->ClockFreq);
}

/**
  Switch to the HS200 timing. This function assumes that eMMC bus is still in legacy mode.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] PciIo           A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru        A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot            The slot number of the SD card to send the command to.
  @param[in] Rca             The relative device address to be assigned.
  @param[in] BusMode         Pointer to SD_MMC_BUS_SETTINGS structure containing bus settings.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchToHS200 (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN SD_MMC_BUS_SETTINGS            *BusMode
  )
{
  EFI_STATUS  Status;

  if ((BusMode->BusTiming != SdMmcMmcHs200) ||
      ((BusMode->BusWidth != 4) && (BusMode->BusWidth != 8)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = EmmcSwitchBusWidth (PciIo, PassThru, Slot, Rca, FALSE, BusMode->BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, BusMode->DriverStrength, BusMode->BusTiming, BusMode->ClockFreq);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcTuningClkForHs200 (PciIo, PassThru, Slot, BusMode->BusWidth);

  return Status;
}

/**
  Switch to the HS400 timing. This function assumes that eMMC bus is still in legacy mode.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] PciIo           A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru        A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot            The slot number of the SD card to send the command to.
  @param[in] Rca             The relative device address to be assigned.
  @param[in] BusMode         Pointer to SD_MMC_BUS_SETTINGS structure containing bus settings.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchToHS400 (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca,
  IN SD_MMC_BUS_SETTINGS            *BusMode
  )
{
  EFI_STATUS           Status;
  SD_MMC_BUS_SETTINGS  Hs200BusMode;
  UINT32               HsFreq;

  if ((BusMode->BusTiming != SdMmcMmcHs400) ||
      (BusMode->BusWidth != 8))
  {
    return EFI_INVALID_PARAMETER;
  }

  Hs200BusMode.BusTiming      = SdMmcMmcHs200;
  Hs200BusMode.BusWidth       = BusMode->BusWidth;
  Hs200BusMode.ClockFreq      = BusMode->ClockFreq;
  Hs200BusMode.DriverStrength = BusMode->DriverStrength;

  Status = EmmcSwitchToHS200 (PciIo, PassThru, Slot, Rca, &Hs200BusMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set to High Speed timing and set the clock frequency to a value less than or equal to 52MHz.
  // This step is necessary to be able to switch Bus into 8 bit DDR mode which is unsupported in HS200.
  //
  HsFreq = BusMode->ClockFreq < 52 ? BusMode->ClockFreq : 52;
  Status = EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, BusMode->DriverStrength, SdMmcMmcHsSdr, HsFreq);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcSwitchBusWidth (PciIo, PassThru, Slot, Rca, TRUE, BusMode->BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, BusMode->DriverStrength, BusMode->BusTiming, BusMode->ClockFreq);
}

/**
  Check if passed BusTiming is supported in both controller and card.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] ExtCsd     Pointer to the card's extended CSD
  @param[in] BusTiming  Bus timing to check

  @retval TRUE  Both card and controller support given BusTiming
  @retval FALSE Card or controller doesn't support given BusTiming
**/
BOOLEAN
EmmcIsBusTimingSupported (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN EMMC_EXT_CSD            *ExtCsd,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  BOOLEAN             Supported;
  SD_MMC_HC_SLOT_CAP  *Capabilities;

  Capabilities = &Private->Capability[SlotIndex];

  Supported = FALSE;
  switch (BusTiming) {
    case SdMmcMmcHs400:
      if ((((ExtCsd->DeviceType & (BIT6 | BIT7))  != 0) && (Capabilities->Hs400 != 0)) && (Capabilities->BusWidth8 != 0)) {
        Supported = TRUE;
      }

      break;
    case SdMmcMmcHs200:
      if ((((ExtCsd->DeviceType & (BIT4 | BIT5))  != 0) && (Capabilities->Sdr104 != 0))) {
        Supported = TRUE;
      }

      break;
    case SdMmcMmcHsDdr:
      if ((((ExtCsd->DeviceType & (BIT2 | BIT3))  != 0) && (Capabilities->Ddr50 != 0))) {
        Supported = TRUE;
      }

      break;
    case SdMmcMmcHsSdr:
      if ((((ExtCsd->DeviceType & BIT1)  != 0) && (Capabilities->HighSpeed != 0))) {
        Supported = TRUE;
      }

      break;
    case SdMmcMmcLegacy:
      if ((ExtCsd->DeviceType & BIT0) != 0) {
        Supported = TRUE;
      }

      break;
    default:
      ASSERT (FALSE);
  }

  return Supported;
}

/**
  Get the target bus timing to set on the link. This function
  will try to select highest bus timing supported by card, controller
  and the driver.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] ExtCsd     Pointer to the card's extended CSD

  @return  Bus timing value that should be set on link
**/
SD_MMC_BUS_MODE
EmmcGetTargetBusTiming (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN EMMC_EXT_CSD            *ExtCsd
  )
{
  SD_MMC_BUS_MODE  BusTiming;

  //
  // We start with highest bus timing that this driver currently supports and
  // return as soon as we find supported timing.
  //
  BusTiming = SdMmcMmcHs400;
  while (BusTiming > SdMmcMmcLegacy) {
    if (EmmcIsBusTimingSupported (Private, SlotIndex, ExtCsd, BusTiming)) {
      break;
    }

    BusTiming--;
  }

  return BusTiming;
}

/**
  Check if the passed bus width is supported by controller and card.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] BusTiming  Bus timing set on the link
  @param[in] BusWidth   Bus width to check

  @retval TRUE   Passed bus width is supported in current bus configuration
  @retval FALSE  Passed bus width is not supported in current bus configuration
**/
BOOLEAN
EmmcIsBusWidthSupported (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN SD_MMC_BUS_MODE         BusTiming,
  IN UINT16                  BusWidth
  )
{
  if ((BusWidth == 8) && (Private->Capability[SlotIndex].BusWidth8 != 0)) {
    return TRUE;
  } else if ((BusWidth == 4) && (BusTiming != SdMmcMmcHs400)) {
    return TRUE;
  } else if ((BusWidth == 1) && ((BusTiming == SdMmcMmcHsSdr) || (BusTiming == SdMmcMmcLegacy))) {
    return TRUE;
  }

  return FALSE;
}

/**
  Get the target bus width to be set on the bus.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] ExtCsd     Pointer to card's extended CSD
  @param[in] BusTiming  Bus timing set on the bus

  @return Bus width to be set on the bus
**/
UINT8
EmmcGetTargetBusWidth (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN EMMC_EXT_CSD            *ExtCsd,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  UINT8  BusWidth;
  UINT8  PreferredBusWidth;

  PreferredBusWidth = Private->Slot[SlotIndex].OperatingParameters.BusWidth;

  if ((PreferredBusWidth != EDKII_SD_MMC_BUS_WIDTH_IGNORE) &&
      EmmcIsBusWidthSupported (Private, SlotIndex, BusTiming, PreferredBusWidth))
  {
    BusWidth = PreferredBusWidth;
  } else if (EmmcIsBusWidthSupported (Private, SlotIndex, BusTiming, 8)) {
    BusWidth = 8;
  } else if (EmmcIsBusWidthSupported (Private, SlotIndex, BusTiming, 4)) {
    BusWidth = 4;
  } else {
    BusWidth = 1;
  }

  return BusWidth;
}

/**
  Get the target clock frequency to be set on the bus.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] ExtCsd     Pointer to card's extended CSD
  @param[in] BusTiming  Bus timing to be set on the bus

  @return Value of the clock frequency to be set on bus in MHz
**/
UINT32
EmmcGetTargetClockFreq (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN EMMC_EXT_CSD            *ExtCsd,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  UINT32  PreferredClockFreq;
  UINT32  MaxClockFreq;

  PreferredClockFreq = Private->Slot[SlotIndex].OperatingParameters.ClockFreq;

  switch (BusTiming) {
    case SdMmcMmcHs400:
    case SdMmcMmcHs200:
      MaxClockFreq = 200;
      break;
    case SdMmcMmcHsSdr:
    case SdMmcMmcHsDdr:
      MaxClockFreq = 52;
      break;
    default:
      MaxClockFreq = 26;
      break;
  }

  if ((PreferredClockFreq != EDKII_SD_MMC_CLOCK_FREQ_IGNORE) && (PreferredClockFreq < MaxClockFreq)) {
    return PreferredClockFreq;
  } else {
    return MaxClockFreq;
  }
}

/**
  Get the driver strength to be set on bus.

  @param[in] Private    Pointer to controller private data
  @param[in] SlotIndex  Index of the slot in the controller
  @param[in] ExtCsd     Pointer to card's extended CSD
  @param[in] BusTiming  Bus timing set on the bus

  @return Value of the driver strength to be set on the bus
**/
EDKII_SD_MMC_DRIVER_STRENGTH
EmmcGetTargetDriverStrength (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN EMMC_EXT_CSD            *ExtCsd,
  IN SD_MMC_BUS_MODE         BusTiming
  )
{
  EDKII_SD_MMC_DRIVER_STRENGTH  PreferredDriverStrength;
  EDKII_SD_MMC_DRIVER_STRENGTH  DriverStrength;

  PreferredDriverStrength = Private->Slot[SlotIndex].OperatingParameters.DriverStrength;
  DriverStrength.Emmc     = EmmcDriverStrengthType0;

  if ((PreferredDriverStrength.Emmc != EDKII_SD_MMC_DRIVER_STRENGTH_IGNORE) &&
      (ExtCsd->DriverStrength & (BIT0 << PreferredDriverStrength.Emmc)))
  {
    DriverStrength.Emmc = PreferredDriverStrength.Emmc;
  }

  return DriverStrength;
}

/**
  Get the target settings for the bus mode.

  @param[in]  Private    Pointer to controller private data
  @param[in]  SlotIndex  Index of the slot in the controller
  @param[in]  ExtCsd     Pointer to card's extended CSD
  @param[out] BusMode    Target configuration of the bus
**/
VOID
EmmcGetTargetBusMode (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   SlotIndex,
  IN EMMC_EXT_CSD            *ExtCsd,
  OUT SD_MMC_BUS_SETTINGS    *BusMode
  )
{
  BusMode->BusTiming      = EmmcGetTargetBusTiming (Private, SlotIndex, ExtCsd);
  BusMode->BusWidth       = EmmcGetTargetBusWidth (Private, SlotIndex, ExtCsd, BusMode->BusTiming);
  BusMode->ClockFreq      = EmmcGetTargetClockFreq (Private, SlotIndex, ExtCsd, BusMode->BusTiming);
  BusMode->DriverStrength = EmmcGetTargetDriverStrength (Private, SlotIndex, ExtCsd, BusMode->BusTiming);
}

/**
  Switch the high speed timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSetBusMode (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT16                         Rca
  )
{
  EFI_STATUS              Status;
  EMMC_CSD                Csd;
  EMMC_EXT_CSD            ExtCsd;
  SD_MMC_BUS_SETTINGS     BusMode;
  SD_MMC_HC_PRIVATE_DATA  *Private;

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (PassThru);

  Status = EmmcGetCsd (PassThru, Slot, Rca, &Csd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSetBusMode: GetCsd fails with %r\n", Status));
    return Status;
  }

  Status = EmmcSelect (PassThru, Slot, Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSetBusMode: Select fails with %r\n", Status));
    return Status;
  }

  ASSERT (Private->BaseClkFreq[Slot] != 0);

  //
  // Get Device_Type from EXT_CSD register.
  //
  Status = EmmcGetExtCsd (PassThru, Slot, &ExtCsd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSetBusMode: GetExtCsd fails with %r\n", Status));
    return Status;
  }

  EmmcGetTargetBusMode (Private, Slot, &ExtCsd, &BusMode);

  DEBUG ((
    DEBUG_INFO,
    "EmmcSetBusMode: Target bus mode: timing = %d, width = %d, clock freq = %d, driver strength = %d\n",
    BusMode.BusTiming,
    BusMode.BusWidth,
    BusMode.ClockFreq,
    BusMode.DriverStrength.Emmc
    ));

  if (BusMode.BusTiming == SdMmcMmcHs400) {
    Status = EmmcSwitchToHS400 (PciIo, PassThru, Slot, Rca, &BusMode);
  } else if (BusMode.BusTiming == SdMmcMmcHs200) {
    Status = EmmcSwitchToHS200 (PciIo, PassThru, Slot, Rca, &BusMode);
  } else {
    //
    // Note that EmmcSwitchToHighSpeed is also called for SdMmcMmcLegacy
    // bus timing. This is because even though we might not want to
    // change the timing itself we still want to allow customization of
    // bus parameters such as clock frequency and bus width.
    //
    Status = EmmcSwitchToHighSpeed (PciIo, PassThru, Slot, Rca, &BusMode);
  }

  DEBUG ((DEBUG_INFO, "EmmcSetBusMode: Switch to %a %r\n", (BusMode.BusTiming == SdMmcMmcHs400) ? "HS400" : ((BusMode.BusTiming == SdMmcMmcHs200) ? "HS200" : "HighSpeed"), Status));

  return Status;
}

/**
  Execute EMMC device identification procedure.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] Private        A pointer to the SD_MMC_HC_PRIVATE_DATA instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       There is a EMMC card.
  @retval Others            There is not a EMMC card.

**/
EFI_STATUS
EmmcIdentification (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   Slot
  )
{
  EFI_STATUS                     Status;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru;
  UINT32                         Ocr;
  UINT16                         Rca;
  UINTN                          Retry;

  PciIo    = Private->PciIo;
  PassThru = &Private->PassThru;

  Status = EmmcReset (PassThru, Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "EmmcIdentification: Executing Cmd0 fails with %r\n", Status));
    return Status;
  }

  Ocr   = 0;
  Retry = 0;
  do {
    Status = EmmcGetOcr (PassThru, Slot, &Ocr);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_VERBOSE, "EmmcIdentification: Executing Cmd1 fails with %r\n", Status));
      return Status;
    }

    Ocr |= BIT30;

    if (Retry++ == 100) {
      DEBUG ((DEBUG_VERBOSE, "EmmcIdentification: Executing Cmd1 fails too many times\n"));
      return EFI_DEVICE_ERROR;
    }

    gBS->Stall (10 * 1000);
  } while ((Ocr & BIT31) == 0);

  Status = EmmcGetAllCid (PassThru, Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "EmmcIdentification: Executing Cmd2 fails with %r\n", Status));
    return Status;
  }

  //
  // Slot starts from 0 and valid RCA starts from 1.
  // Here we takes a simple formula to calculate the RCA.
  // Don't support multiple devices on the slot, that is
  // shared bus slot feature.
  //
  Rca    = Slot + 1;
  Status = EmmcSetRca (PassThru, Slot, Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentification: Executing Cmd3 fails with %r\n", Status));
    return Status;
  }

  //
  // Enter Data Tranfer Mode.
  //
  DEBUG ((DEBUG_INFO, "EmmcIdentification: Found a EMMC device at slot [%d], RCA [%d]\n", Slot, Rca));
  Private->Slot[Slot].CardType = EmmcCardType;

  Status = EmmcSetBusMode (PciIo, PassThru, Slot, Rca);

  return Status;
}

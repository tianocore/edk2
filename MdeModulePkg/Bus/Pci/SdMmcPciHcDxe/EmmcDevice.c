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
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_GO_IDLE_STATE;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBc;
  SdMmcCmdBlk.ResponseType = 0;
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
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_OP_COND;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR3;
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
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_ALL_SEND_CID;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
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
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SET_RELATIVE_ADDR;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
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
     OUT EMMC_CSD                       *Csd
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_CSD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdMmcPassThruPassThru (PassThru, Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Csd) + 1, &SdMmcStatusBlk.Resp0, sizeof (EMMC_CSD) - 1);
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
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SELECT_DESELECT_CARD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
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
     OUT EMMC_EXT_CSD                   *ExtCsd
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_EXT_CSD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
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
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT8                              Access,
  IN UINT8                              Index,
  IN UINT8                              Value,
  IN UINT8                              CmdSet
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SWITCH;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1b;
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
     OUT UINT32                         *DevStatus
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_STATUS;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
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
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT8                              BusWidth
  )
{
  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;
  EFI_STATUS                            Status;
  UINT8                                 TuningBlock[128];

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_MMC_HC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_TUNING_BLOCK;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
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
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT8                              BusWidth
  )
{
  EFI_STATUS          Status;
  UINT8               HostCtrl2;
  UINT8               Retry;

  //
  // Notify the host that the sampling clock tuning procedure starts.
  //
  HostCtrl2 = BIT6;
  Status = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
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
  HostCtrl2 = (UINT8)~(BIT6 | BIT7);
  Status = SdMmcHcAndMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  return EFI_DEVICE_ERROR;
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
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca,
  IN BOOLEAN                            IsDdr,
  IN UINT8                              BusWidth
  )
{
  EFI_STATUS          Status;
  UINT8               Access;
  UINT8               Index;
  UINT8               Value;
  UINT8               CmdSet;
  UINT32              DevStatus;

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

  Status = EmmcSendStatus (PassThru, Slot, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSwitchBusWidth: Send status fails with %r\n", Status));
    return Status;
  }
  //
  // Check the switch operation is really successful or not.
  //
  if ((DevStatus & BIT7) != 0) {
    DEBUG ((DEBUG_ERROR, "EmmcSwitchBusWidth: The switch operation fails as DevStatus is 0x%08x\n", DevStatus));
    return EFI_DEVICE_ERROR;
  }

  Status = SdMmcHcSetBusWidth (PciIo, Slot, BusWidth);

  return Status;
}

/**
  Switch the bus timing and clock frequency.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6 and SD Host Controller
  Simplified Spec 3.0 Figure 3-3 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] HsTiming       The value to be written to HS_TIMING field of EXT_CSD register.
  @param[in] Timing         The bus mode timing indicator.
  @param[in] ClockFreq      The max clock frequency to be set, the unit is MHz.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchBusTiming (
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca,
  IN UINT8                              HsTiming,
  IN SD_MMC_BUS_MODE                    Timing,
  IN UINT32                             ClockFreq
  )
{
  EFI_STATUS                Status;
  UINT8                     Access;
  UINT8                     Index;
  UINT8                     Value;
  UINT8                     CmdSet;
  UINT32                    DevStatus;
  SD_MMC_HC_PRIVATE_DATA    *Private;

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (PassThru);
  //
  // Write Byte, the Value field is written into the byte pointed by Index.
  //
  Access = 0x03;
  Index  = OFFSET_OF (EMMC_EXT_CSD, HsTiming);
  Value  = HsTiming;
  CmdSet = 0;

  Status = EmmcSwitch (PassThru, Slot, Access, Index, Value, CmdSet);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSwitchBusTiming: Switch to hstiming %d fails with %r\n", HsTiming, Status));
    return Status;
  }

  //
  // Convert the clock freq unit from MHz to KHz.
  //
  Status = SdMmcHcClockSupply (PciIo, Slot, ClockFreq * 1000, Private->BaseClkFreq[Slot], Private->ControllerVersion[Slot]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcSendStatus (PassThru, Slot, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSwitchBusTiming: Send status fails with %r\n", Status));
    return Status;
  }
  //
  // Check the switch operation is really successful or not.
  //
  if ((DevStatus & BIT7) != 0) {
    DEBUG ((DEBUG_ERROR, "EmmcSwitchBusTiming: The switch operation fails as DevStatus is 0x%08x\n", DevStatus));
    return EFI_DEVICE_ERROR;
  }

  if (mOverride != NULL && mOverride->NotifyPhase != NULL) {
    Status = mOverride->NotifyPhase (
                          Private->ControllerHandle,
                          Slot,
                          EdkiiSdMmcSwitchClockFreqPost,
                          &Timing
                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: SD/MMC switch clock freq post notifier callback failed - %r\n",
        __FUNCTION__,
        Status
        ));
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
  @param[in] ClockFreq      The max clock frequency to be set.
  @param[in] IsDdr          If TRUE, use dual data rate data simpling method. Otherwise
                            use single data rate data simpling method.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchToHighSpeed (
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca,
  IN UINT32                             ClockFreq,
  IN BOOLEAN                            IsDdr,
  IN UINT8                              BusWidth
  )
{
  EFI_STATUS              Status;
  UINT8                   HsTiming;
  UINT8                   HostCtrl1;
  SD_MMC_BUS_MODE         Timing;
  SD_MMC_HC_PRIVATE_DATA  *Private;

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (PassThru);

  Status = EmmcSwitchBusWidth (PciIo, PassThru, Slot, Rca, IsDdr, BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Set to Hight Speed timing
  //
  HostCtrl1 = BIT2;
  Status = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IsDdr) {
    Timing = SdMmcMmcHsDdr;
  } else if (ClockFreq == 52) {
    Timing = SdMmcMmcHsSdr;
  } else {
    Timing = SdMmcMmcLegacy;
  }

  Status = SdMmcHcUhsSignaling (Private->ControllerHandle, PciIo, Slot, Timing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HsTiming = 1;
  Status = EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, HsTiming, Timing, ClockFreq);

  return Status;
}

/**
  Switch to the HS200 timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] ClockFreq      The max clock frequency to be set.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchToHS200 (
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca,
  IN UINT32                             ClockFreq,
  IN UINT8                              BusWidth
  )
{
  EFI_STATUS               Status;
  UINT8                    HsTiming;
  UINT16                   ClockCtrl;
  SD_MMC_BUS_MODE          Timing;
  SD_MMC_HC_PRIVATE_DATA  *Private;

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (PassThru);

  if ((BusWidth != 4) && (BusWidth != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EmmcSwitchBusWidth (PciIo, PassThru, Slot, Rca, FALSE, BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Set to HS200/SDR104 timing
  //
  //
  // Stop bus clock at first
  //
  Status = SdMmcHcStopClock (PciIo, Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Timing = SdMmcMmcHs200;

  Status = SdMmcHcUhsSignaling (Private->ControllerHandle, PciIo, Slot, Timing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Wait Internal Clock Stable in the Clock Control register to be 1 before set SD Clock Enable bit
  //
  Status = SdMmcHcWaitMmioSet (
             PciIo,
             Slot,
             SD_MMC_HC_CLOCK_CTRL,
             sizeof (ClockCtrl),
             BIT1,
             BIT1,
             SD_MMC_HC_GENERIC_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Set SD Clock Enable in the Clock Control register to 1
  //
  ClockCtrl = BIT2;
  Status = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_CLOCK_CTRL, sizeof (ClockCtrl), &ClockCtrl);

  HsTiming = 2;
  Status = EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, HsTiming, Timing, ClockFreq);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcTuningClkForHs200 (PciIo, PassThru, Slot, BusWidth);

  return Status;
}

/**
  Switch to the HS400 timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] PciIo          A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] PassThru       A pointer to the EFI_SD_MMC_PASS_THRU_PROTOCOL instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] ClockFreq      The max clock frequency to be set.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcSwitchToHS400 (
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca,
  IN UINT32                             ClockFreq
  )
{
  EFI_STATUS                 Status;
  UINT8                      HsTiming;
  SD_MMC_BUS_MODE            Timing;
  SD_MMC_HC_PRIVATE_DATA     *Private;

  Private = SD_MMC_HC_PRIVATE_FROM_THIS (PassThru);

  Status = EmmcSwitchToHS200 (PciIo, PassThru, Slot, Rca, ClockFreq, 8);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Set to Hight Speed timing and set the clock frequency to a value less than 52MHz.
  //
  HsTiming = 1;
  Status = EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, HsTiming, SdMmcMmcHsSdr, 52);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // HS400 mode must use 8 data lines.
  //
  Status = EmmcSwitchBusWidth (PciIo, PassThru, Slot, Rca, TRUE, 8);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Timing = SdMmcMmcHs400;

  Status = SdMmcHcUhsSignaling (Private->ControllerHandle, PciIo, Slot, Timing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HsTiming = 3;
  Status = EmmcSwitchBusTiming (PciIo, PassThru, Slot, Rca, HsTiming, Timing, ClockFreq);

  return Status;
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
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL      *PassThru,
  IN UINT8                              Slot,
  IN UINT16                             Rca
  )
{
  EFI_STATUS                    Status;
  EMMC_CSD                      Csd;
  EMMC_EXT_CSD                  ExtCsd;
  UINT8                         HsTiming;
  BOOLEAN                       IsDdr;
  UINT32                        ClockFreq;
  UINT8                         BusWidth;
  SD_MMC_HC_PRIVATE_DATA        *Private;

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
  // Check if the Host Controller support 8bits bus width.
  //
  if (Private->Capability[Slot].BusWidth8 != 0) {
    BusWidth = 8;
  } else {
    BusWidth = 4;
  }
  //
  // Get Deivce_Type from EXT_CSD register.
  //
  Status = EmmcGetExtCsd (PassThru, Slot, &ExtCsd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSetBusMode: GetExtCsd fails with %r\n", Status));
    return Status;
  }
  //
  // Calculate supported bus speed/bus width/clock frequency.
  //
  HsTiming  = 0;
  IsDdr     = FALSE;
  ClockFreq = 0;
  if (((ExtCsd.DeviceType & (BIT4 | BIT5))  != 0) && (Private->Capability[Slot].Sdr104 != 0)) {
    HsTiming  = 2;
    IsDdr     = FALSE;
    ClockFreq = 200;
  } else if (((ExtCsd.DeviceType & (BIT2 | BIT3))  != 0) && (Private->Capability[Slot].Ddr50 != 0)) {
    HsTiming  = 1;
    IsDdr     = TRUE;
    ClockFreq = 52;
  } else if (((ExtCsd.DeviceType & BIT1)  != 0) && (Private->Capability[Slot].HighSpeed != 0)) {
    HsTiming  = 1;
    IsDdr     = FALSE;
    ClockFreq = 52;
  } else if (((ExtCsd.DeviceType & BIT0)  != 0) && (Private->Capability[Slot].HighSpeed != 0)) {
    HsTiming  = 1;
    IsDdr     = FALSE;
    ClockFreq = 26;
  }
  //
  // Check if both of the device and the host controller support HS400 DDR mode.
  //
  if (((ExtCsd.DeviceType & (BIT6 | BIT7))  != 0) && (Private->Capability[Slot].Hs400 != 0)) {
    //
    // The host controller supports 8bits bus.
    //
    ASSERT (BusWidth == 8);
    HsTiming  = 3;
    IsDdr     = TRUE;
    ClockFreq = 200;
  }

  if ((ClockFreq == 0) || (HsTiming == 0)) {
    //
    // Continue using default setting.
    //
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "EmmcSetBusMode: HsTiming %d ClockFreq %d BusWidth %d Ddr %a\n", HsTiming, ClockFreq, BusWidth, IsDdr ? "TRUE":"FALSE"));

  if (HsTiming == 3) {
    //
    // Execute HS400 timing switch procedure
    //
    Status = EmmcSwitchToHS400 (PciIo, PassThru, Slot, Rca, ClockFreq);
  } else if (HsTiming == 2) {
    //
    // Execute HS200 timing switch procedure
    //
    Status = EmmcSwitchToHS200 (PciIo, PassThru, Slot, Rca, ClockFreq, BusWidth);
  } else {
    //
    // Execute High Speed timing switch procedure
    //
    Status = EmmcSwitchToHighSpeed (PciIo, PassThru, Slot, Rca, ClockFreq, IsDdr, BusWidth);
  }

  DEBUG ((DEBUG_INFO, "EmmcSetBusMode: Switch to %a %r\n", (HsTiming == 3) ? "HS400" : ((HsTiming == 2) ? "HS200" : "HighSpeed"), Status));

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
  IN SD_MMC_HC_PRIVATE_DATA             *Private,
  IN UINT8                              Slot
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
    gBS->Stall(10 * 1000);
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


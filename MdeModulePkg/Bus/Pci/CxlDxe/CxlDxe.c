/** @file
  CxlDxe driver is used to discover CXL devices
  supports Mailbox functionality
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"
#include "CxlDxeUtil.h"

//
// CXL Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gCxlDriverBinding = {
  CxlDriverBindingSupported,
  CxlDriverBindingStart,
  CxlDriverBindingStop,
  0x10,
  NULL,
  NULL
};

/**
  Reads MB Control Register to verify doorbell is clear.

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval ReturnValue            doorbell is clear or not.

**/
BOOLEAN
IsCxlDoorbellBusy (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  BOOLEAN     ReturnValue;
  UINT32      Value;

  ReturnValue = 0;
  Value       = 0;

  Status = PciUefiMemRead32 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_CTRL_OFFSET, &Value);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error reading Value for busy doorbell\n", __func__));
  }

  ReturnValue = Value & CXL_DEV_MBOX_CTRL_DOORBELL;
  return ReturnValue;
}

/**
  Sends Mailbox command

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval Status                 Return EFI STATUS success or failure

**/
EFI_STATUS
CxlSendCmd (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  size_t      OutputSize;
  size_t      MinimumOutput;

  OutputSize    = 0;
  MinimumOutput = 0;

  if ((Private->MailboxCmd.InputSize > Private->MemdevState.PayloadSize) ||
      (Private->MailboxCmd.OutputSize > Private->MemdevState.PayloadSize))
  {
    DEBUG ((EFI_D_ERROR, "[%a]: InputSize or sizeout > PayloadSize\n", __func__));
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  OutputSize    = (size_t)Private->MailboxCmd.OutputSize;
  MinimumOutput = (size_t)Private->MailboxCmd.MinimumOutput;
  Status        = CxlPciMboxSend (Private);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error returned in func CxlPciMboxSend()\n", __func__));
    return Status;
  }

  if ((Private->MailboxCmd.ReturnCode != CXL_MBOX_CMD_RC_SUCCESS) &&
      (Private->MailboxCmd.ReturnCode != CXL_MBOX_CMD_RC_BACKGROUND))
  {
    // Command Return Codes
    switch (Private->MailboxCmd.ReturnCode) {
      case CXL_MBOX_CMD_INVALID_INPUT:
        DEBUG ((EFI_D_ERROR, "[%a]: Invalid Input\n", __func__));
        break;

      case CXL_MBOX_CMD_UNSUPPORTED:
        DEBUG ((EFI_D_ERROR, "[%a]: Unsupported\n", __func__));
        break;

      case CXL_MBOX_CMD_INTERNAL_ERROR:
        DEBUG ((EFI_D_ERROR, "[%a]: Internal Error\n", __func__));
        break;

      case CXL_MBOX_CMD_RETRY_REQUIRED:
        DEBUG ((EFI_D_ERROR, "[%a]: Retry Required\n", __func__));
        break;

      case CXL_MBOX_CMD_BUSY:
        DEBUG ((EFI_D_ERROR, "[%a]: Busy\n", __func__));
        break;

      case CXL_MBOX_CMD_MEDIA_DISABLED:
        DEBUG ((EFI_D_ERROR, "[%a]: Media Disabled\n", __func__));
        break;

      case CXL_MBOX_CMD_FW_TRANSFER_IN_PROGRESS:
        DEBUG ((EFI_D_ERROR, "[%a]: FW Transfer in Progress\n", __func__));
        break;

      case CXL_MBOX_CMD_FW_TRANSFER_OUT_OF_ORDER:
        DEBUG ((EFI_D_ERROR, "[%a]: FW Transfer Out of Order\n", __func__));
        break;

      case CXL_MBOX_CMD_FW_VERIFICATION_FAILED:
        DEBUG ((EFI_D_ERROR, "[%a]: FW Verification Failed\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_SLOT:
        DEBUG ((EFI_D_ERROR, "[%a]: Invalid Slot\n", __func__));
        break;

      case CXL_MBOX_CMD_ACTIVATION_FAILED_FW_ROLLED_BACK:
        DEBUG ((EFI_D_ERROR, "[%a]: Activation Failed, FW Rolled Back\n", __func__));
        break;

      case CXL_MBOX_CMD_COLD_RESET_REQUIRED:
        DEBUG ((EFI_D_ERROR, "[%a]: Activation Failed, Cold Reset Required\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_HANDLE:
        DEBUG ((EFI_D_ERROR, "[%a]: Invalid Handle\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_PHYSICAL_ADDRESS:
        DEBUG ((EFI_D_ERROR, "[%a]: Invalid Physical Address\n", __func__));
        break;

      case CXL_MBOX_CMD_INJECT_POISON_LIMIT_REACHED:
        DEBUG ((EFI_D_ERROR, "[%a]: Inject Poison Limit Reached\n", __func__));
        break;

      case CXL_MBOX_CMD_PERMANENT_MEDIA_FAILURE:
        DEBUG ((EFI_D_ERROR, "[%a]: Permanent Media Failure\n", __func__));
        break;

      case CXL_MBOX_CMD_ABORTED:
        DEBUG ((EFI_D_ERROR, "[%a]: Aborted\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_SECURITY_STATE:
        DEBUG ((EFI_D_ERROR, "[%a]: Invalid Security State\n", __func__));
        break;

      case CXL_MBOX_CMD_INCORRECT_PASSPHRASE:
        DEBUG ((EFI_D_ERROR, "[%a]: Incorrect Passphrase\n", __func__));
        break;

      default:
        DEBUG ((EFI_D_ERROR, "[%a]: Error \n", __func__));
        break;
    }

    Status = EFI_LOAD_ERROR;
    return Status;
  }

  if (!OutputSize) {
    Status = EFI_SUCCESS;
    return Status;
  }

  /*
   * Variable sized output needs to at least satisfy the caller's
   * minimum if not the fully requested Size.
   */
  if (MinimumOutput == 0) {
    MinimumOutput = OutputSize;
  }

  if (Private->MailboxCmd.OutputSize < MinimumOutput) {
    DEBUG ((EFI_D_ERROR, "[%a]: OutputSize less than MinimumOutput \n", __func__));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = EFI_SUCCESS;
  return Status;
}

/**
  Finds PCI Express Designated Vendor-Specific Extended Capability

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start                    Start position
  @param[in] Capability               Extended capability

  @retval Position                    Position of input capability

**/
UINT32
CxlFindNextExtendedCapability (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  UINT32                       Capability
  )
{
  UINT32  Header;
  UINT32  TimeToLive;
  UINT32  Position;

  Header     = 0;
  TimeToLive = 0;
  Position   = CXL_PCI_CFG_SPACE_SIZE;

  /* minimum 8 bytes per capability */
  TimeToLive = (CXL_PCI_CFG_SPACE_EXP_SIZE - CXL_PCI_CFG_SPACE_SIZE) / 8;

  if (Start) {
    Position = Start;
  }

  if (PciUefiReadConfigWord (Private, Position, &Header) != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiReadConfigWord\n", __func__));
    return 0;
  }

  /*
   * If we have no capabilities, this is indicated by Capability ID,
   * Capability version and next pointer all being 0.
   */
  if (Header == 0) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error, Header\n", __func__));
    return 0;
  }

  while (TimeToLive-- > 0) {
    if ((CXL_PCI_EXT_CAP_ID (Header) == Capability) && (Position != Start)) {
      return Position;
    }

    Position = CXL_PCI_EXT_CAP_NEXT (Header);
    if (Position < CXL_PCI_CFG_SPACE_SIZE) {
      break;
    }

    if (PciUefiReadConfigWord (Private, Position, &Header) != EFI_SUCCESS) {
      break;
    }
  }

  return 0;
}

/**
  Finds Offset of Vendor id and Dvsec, Dvsec Vendor ID field is set to 1E98h to indicate these
  Capability structures are defined by the CXL specification.

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Vendor                   Vendor ID
  @param[in] Dvsec                    Dvsec Id

  @retval Position                    Return Offset at which DVSEC Vendor ID 1E98h and DVSEC ID 0008h get matched

**/
UINT32
CxlFindDvsecCapability (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Vendor,
  UINT32                       Dvsec
  )
{
  UINT32  Position = 0;

  /* CXL_PCI_EXT_CAP_ID_DVSEC: 23h is Extended Capability struct id for DVSEC */
  Position = CxlFindNextExtendedCapability (Private, 0, CXL_PCI_EXT_CAP_ID_DVSEC);
  if (!Position) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error, Position = 0\n", __func__));
    return 0;
  }

  while (Position) {
    UINT32      VendorData = 0;
    UINT32      DvsecData  = 0;
    UINT32      vendorId   = 0;
    UINT32      DvsecId    = 0;
    EFI_STATUS  Status;
    Status = PciUefiReadConfigWord (Private, Position + CXL_PCI_DVSEC_HEADER1, &VendorData);  // DVSEC Header 1 (Offset 04h) Bit Loc 15:0, get DVSEC Vendor ID 1E98h
    if (Status != EFI_SUCCESS) {
      return 0;
    }

    Status = PciUefiReadConfigWord (Private, Position + CXL_PCI_DVSEC_HEADER2, &DvsecData); // DVSEC Header 2 (Offset 08h) Bit Loc 15:0, get DVSEC ID 0008h
    if (Status != EFI_SUCCESS) {
      return 0;
    }

    vendorId = (UINT32)GetFieldValues (VendorData, 15, 0);
    DvsecId  = (UINT32)GetFieldValues (DvsecData, 15, 0);
    if ((Vendor == vendorId) && (Dvsec == DvsecId)) {
      /*
      Return Offset at which DVSEC Vendor ID 1E98h and DVSEC ID 0008h get matched
      This required Register Locator DVSEC, if not, search next extended capability
      */
      return Position;
    }

    Position = CxlFindNextExtendedCapability (Private, Position, CXL_PCI_EXT_CAP_ID_DVSEC);
  }

  return 0;
}

/**
  Finds RegisterType, BaseAddressRegister, and RegisterOffset for input Register low and high

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] RegisterLow              Register Low
  @param[in] RegisterHigh             Register High
  @param[in] Type                     Register block identifier type

  @retval Position                    Return Offset at which DVSEC Vendor ID 1E98h and DVSEC ID 0008h get matched

**/
BOOLEAN
CxlDecodeRegblock (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       RegisterLow,
  UINT32                       RegisterHigh,
  CXL_REG_BLOCK_IDENTIFIER     Type
  )
{
  // Get Register BIR, 8.1.9.1, Register Offset Low (Offset: Varies)

  /*************************************************************************************
  Indicates which one of a Functions BARs, located beginning at Offset
  10h in Configuration Space, or entry in the Enhanced Allocation capability with a
  matching BAR Equivalent Indicator (BEI), is used to RegisterMap the CXL registers into
  Memory Space.

  Defined encodings are:
  BAR0 - 10h to 18h
  000b = Base Address Register 10h
  001b = Base Address Register 14h

  BAR1
  010b = Base Address Register 18h
  011b = Base Address Register 1Ch

  BAR2
  100b = Base Address Register 20h
  101b = Base Address Register 24h

  All other encodings are reserved
  *************************************************************************************/

  UINT32  RegisterType;
  UINT32  BAR;
  UINT32  BaseAddressRegister;
  UINT64  Offset;

  RegisterType = 0;
  BAR          = 0;

  BaseAddressRegister = (UINT32)GetFieldValues (RegisterLow, 2, 0);

  /*
  BAR0: BaseAddressRegister = 0,1
  BAR1: BaseAddressRegister = 2,3
  BAR2: BaseAddressRegister = 4,5
  BAR3: BaseAddressRegister = 6,7
  */
  if ((BaseAddressRegister == 0) || (BaseAddressRegister == 1)) {
    BAR = 0;
  } else if ((BaseAddressRegister == 2) || (BaseAddressRegister == 3)) {
    BAR = 1;
  } else if ((BaseAddressRegister == 4) || (BaseAddressRegister == 5)) {
    BAR = 2;
  } else {
    BAR = 3;
  }

  /***********************************************************************************
  8.1.9.2 Register Offset High
  ***********************************************************************************/
  Offset = ((UINT64)RegisterHigh << 32) | (RegisterLow & CXL_GENMASK (31, 16));

  // Get Register Block Identifier which Identifies the Type of CXL registers.
  // Register Block Identifier 03h is CXL Device Registers
  RegisterType = (UINT32)GetFieldValues (RegisterLow, 15, 8);

  if (RegisterType == (UINT32)Type) {
    Private->RegisterMap.RegisterType        = (UINT32)GetFieldValues (RegisterLow, 15, 8);
    Private->RegisterMap.BaseAddressRegister = BAR;
    Private->RegisterMap.RegisterOffset      = Offset;
  } else {
    return FALSE;
  }

  return TRUE;
}

/**
  Finds Register low and high based on Register block identifier and calls CxlDecodeRegblock

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Type                     Register block identifier type

  @retval Status                      Return EFI_SUCCESS on successfully calling CxlDecodeRegblock

**/
EFI_STATUS
CxlFindRegblockInstance (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  CXL_REG_BLOCK_IDENTIFIER     Type
  )
{
  EFI_STATUS  Status;
  UINT32      RegisterLocatorSize;
  UINT32      RegisterBlocks;
  UINT32      DVSEC_Header1;
  UINT32      RegisterLocator;
  UINT32      LoopCount;
  UINT32      RegisterLow;
  UINT32      RegisterHigh;

  RegisterLocatorSize = 0;
  RegisterBlocks      = 0;
  DVSEC_Header1       = 0;
  RegisterLocator     = 0;
  LoopCount           = 0;

  // Get Dvsec Capability with id 8, which is RegisterValue locator
  RegisterLocator = CxlFindDvsecCapability (Private, CXL_PCI_DVSEC_VENDOR_ID, CXL_DVSEC_ID_REGISTER_LOCATOR);
  if (RegisterLocator == 0) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error in CxlFindDvsecCapability\n", __func__));
    return EFI_UNSUPPORTED;
  }

  // Read Register Locator DVSEC - Header1
  Status = PciUefiReadConfigWord (Private, RegisterLocator + CXL_PCI_DVSEC_HEADER1, &DVSEC_Header1);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiReadConfigWord\n", __func__));
    return Status;
  }

  // Get DVSEC length from Bit position 31:20 (Table 8-9), from above read Designated Vendor-specific Header 1
  RegisterLocatorSize = (UINT32)GetFieldValues (DVSEC_Header1, 31, 20);

  // Get Register Block 1 - Register Offset Low, By adding 0Ch to base Offset
  RegisterLocator += CXL_DVSEC_REG_LOCATOR_BLOCK1_OFFSET;

  // Get Num of Reg Blocks, (Total Size - Reg Block Offset) and each block is of Size 8
  RegisterBlocks = (RegisterLocatorSize - CXL_DVSEC_REG_LOCATOR_BLOCK1_OFFSET) / 8;

  // Loop for each Reg block and get Register Offset Low and high
  for (LoopCount = 0; LoopCount < RegisterBlocks; LoopCount++, RegisterLocator += 8) {
    RegisterLow  = 0;
    RegisterHigh = 0;
    Status       = PciUefiReadConfigWord (Private, RegisterLocator, &RegisterLow);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiReadConfigWord, RegisterLow\n", __func__));
      return Status;
    }

    Status = PciUefiReadConfigWord (Private, RegisterLocator + 4, &RegisterHigh);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiReadConfigWord, RegisterHigh\n", __func__));
      return Status;
    }

    if (!CxlDecodeRegblock (Private, RegisterLow, RegisterHigh, Type)) {
      continue;
    }
  }

  return EFI_SUCCESS;
}

/**
  Finds CXL Mailbox register, The mailbox registers provide the ability to issue a command to the device

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval Status                      Return EFI_SUCCESS on successfully calling CxlDecodeRegblock

**/
EFI_STATUS
CxlGetMboxRegs (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  UINT32      Capability;
  UINT32      CapabilityCount;
  UINT64      CapabilityArray;
  UINT32      Value;
  UINT32      Offset;
  UINT32      Length;
  UINT16      CapabilityId;

  Capability      = 0;
  CapabilityCount = 0;
  CapabilityArray = 0;
  Value           = 0;

  Status = PciUefiMemRead64 (Private, CXL_DEV_CAP_ARRAY_OFFSET, &CapabilityArray);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error getting CapabilityArray\n", __func__));
    return Status;
  }

  if (GetFieldValues (CapabilityArray, 15, 0) != CXL_DEV_CAP_ARRAY_CAP_ID) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error CXL_DEV_CAP_ARRAY_CAP_ID not matching\n", __func__));
    return EFI_NOT_FOUND;
  }

  CapabilityCount = (UINT32)GetFieldValues (CapabilityArray, 47, 32);

  for (Capability = 1; Capability <= CapabilityCount; Capability++) {
    Offset       = 0;
    Length       = 0;
    CapabilityId = 0;

    Status = PciUefiMemRead32 (Private, Capability * 0x10, &Value);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error reading Value for CapabilityId\n", __func__));
      return Status;
    }

    CapabilityId = (UINT16)GetFieldValues (Value, 15, 0);

    // 8.2.8.2 CXL Device Capability Header Register
    Status = PciUefiMemRead32 (Private, Capability * 0x10 + 0x4, &Offset);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error reading Offset\n", __func__));
      return Status;
    }

    Status = PciUefiMemRead32 (Private, Capability * 0x10 + 0x8, &Length);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error reading length\n", __func__));
      return Status;
    }

    switch (CapabilityId) {
      case CXL_DEVICE_CAPABILITY_ID_PRIMARY_MAILBOX:
        Private->RegisterMap.MailboxRegistersOffset = Offset;
        break;

      case CXL_DEVICE_CAPABILITY_ID_SECONDARY_MAILBOX:
        break;

      default:
        if (CapabilityId >= 0x8000) {
          DEBUG ((EFI_D_INFO, "[%a]: CapabilityId >= 0x8000\n", __func__));
        }

        break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Reads MB Control Register to verify doorbell is clear, and wait for it till it is not clear

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval BOOLEAN                     Return 0 in case of bit is clear else 1

**/
UINT32
CxlMboxWaitForDoorbell (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  int  Count = 0;

  while (IsCxlDoorbellBusy (Private)) {
    gBS->Stall (1000); // 1000Microsecond, 1 ms
    Count++;

    /* Check again in case preempted before timeout test */
    if (!IsCxlDoorbellBusy (Private)) {
      break;
    }

    if (Count == CXL_MAILBOX_TIMEOUT_MS) {
      return 1;
    }
  }

  return 0;
}

/**
  Checks Mailbox background command gets complete or not

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval BOOLEAN                     Return 0 in case of bit is clear else 1

**/
static BOOLEAN
CxlMboxBackgroundComplete (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  UINT64      RegisterValue;
  UINT64      BackgroundStatus;

  Status = PciUefiMemRead64 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_BG_CMD_STATUS_OFFSET, &RegisterValue);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error reading RegisterValue for background status cmd\n", __func__));
    return 0;
  }

  BackgroundStatus = GetFieldValues (RegisterValue, 22, 16);
  if (BackgroundStatus == 100) {
    DEBUG ((EFI_D_ERROR, "[%a]: BackgroundStatus = 100\n", __func__));
    return 0;
  }

  return 1;
}

/**
  Issue a command to the device using mailbox registers

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval Status                      Return EFI_SUCCESS on successfully calling CxlDecodeRegblock

**/
EFI_STATUS
CxlPciMboxSend (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  /*
  * 1. Caller reads MB Control Register to verify doorbell is clear
  * 2. Caller writes Command Register
  * 3. Caller writes Command Payload Registers if input payload is non-empty
  * 4. Caller writes MB Control Register to set doorbell
  * 5. Caller either polls for doorbell to be clear or waits for interrupt if configured
  * 6. Caller reads MB Status Register to fetch Return code
  * 7. If command successful, Caller reads Command Register to get Payload Length
  * 8. If output payload is non-empty, host reads Command Payload Registers
  *
  * Hardware is free to do whatever it wants before the doorbell is rung,
  * and isn't allowed to change anything after it clears the doorbell. As
  * such, steps 2 and 3 can happen in any order, and steps 6, 7, 8 can
  * also happen in any order (though some orders might not make sense).
  */

  // 8.2.8.4.5 Command Register (Mailbox Registers Capability Offset + 08h)

  EFI_STATUS  Status;
  UINT64      CommandRegister;
  UINT64      StatusRegister;
  size_t      OutputLength;
  UINT32      Value;
  UINT32      Index;
  size_t      MinimumNumber;
  UINT64      BackgroundStatusRegister;

  CommandRegister = 0;
  StatusRegister  = 0;
  OutputLength    = 0;

  /* #1 */
  if (IsCxlDoorbellBusy (Private)) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error mailbox queue busy\n", __func__));
    return EFI_TIMEOUT;
  }

  CommandRegister = Private->MailboxCmd.Opcode;
  if (Private->MailboxCmd.InputSize) {
    if (!Private->MailboxCmd.InputPayload) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error InputPayload is 0\n", __func__));
      return EFI_BAD_BUFFER_SIZE;
    }

    // Place bits of InputSize starting 16th bits of UINT64 CommandRegister
    CommandRegister |= (Private->MailboxCmd.InputSize << 16);

    /*Read Payload buffer in n Bytes*/
    Status = PciUefiMemWriteNBits (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_PAYLOAD_OFFSET, Private->MailboxCmd.InputPayload, (UINT32)Private->MailboxCmd.InputSize);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiMemWriteNBits\n", __func__));
      return Status;
    }
  }

  /* #2, #3 */
  Status = PciUefiMemWrite64 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_CMD_OFFSET, &CommandRegister);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiMemWrite64\n", __func__));
    return Status;
  }

  /* #4 */
  Value  = CXL_DEV_MBOX_CTRL_DOORBELL;
  Status = PciUefiMemWrite32 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_CTRL_OFFSET, &Value);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiMemWrite32\n", __func__));
    return Status;
  }

  /* #5 */
  if (CxlMboxWaitForDoorbell (Private) != 0) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error, Mailbox timeout\n", __func__));
    Status = EFI_TIMEOUT;
    return Status;
  }

  /* #6 */
  Status = PciUefiMemRead64 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_STATUS_OFFSET, &StatusRegister);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiMemRead64\n", __func__));
    return Status;
  }

  Private->MailboxCmd.ReturnCode = (UINT16)GetFieldValues (StatusRegister, 47, 32);

  if (Private->MailboxCmd.ReturnCode == CXL_MBOX_CMD_RC_BACKGROUND) {
    Index                    = 0;
    BackgroundStatusRegister = 0;

    for (Index = 0; Index < Private->MailboxCmd.PollCount; Index++) {
      gBS->Stall (Private->MailboxCmd.PollInterval * 1000); // Microsecond
      if (CxlMboxBackgroundComplete (Private)) {
        break;
      }
    }

    if (!CxlMboxBackgroundComplete (Private)) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error, Mailbox timeout\n", __func__));
      return EFI_TIMEOUT;
    }

    Status = PciUefiMemRead64 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_BG_CMD_STATUS_OFFSET, &BackgroundStatusRegister);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiMemRead64\n", __func__));
      return Status;
    }

    Private->MailboxCmd.ReturnCode = (UINT16)GetFieldValues (BackgroundStatusRegister, 47, 32);
  }

  if (Private->MailboxCmd.ReturnCode != CXL_MBOX_CMD_RC_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Completed but caller must check ReturnCode\n", __func__));
    return EFI_SUCCESS;
  }

  // 8.2.8.4.5 Command Register (Mailbox Registers Capability Offset + 08h)
  Status = PciUefiMemRead64 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_CMD_OFFSET, &CommandRegister);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error in PciUefiMemRead64\n", __func__));
    return Status;
  }

  // Payload Length: (36:16)
  OutputLength = (size_t)GetFieldValues (CommandRegister, 36, 16);

  /* #8 */
  if (OutputLength && Private->MailboxCmd.OutputPayload) {
    MinimumNumber = (size_t)MinimumOfThreeValues (Private->MailboxCmd.OutputSize, Private->MemdevState.PayloadSize, OutputLength);

    /*Read Payload buffer in n Bytes*/
    char  Buffer[256];
    Status = PciUefiMemReadNBits (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_PAYLOAD_OFFSET, Buffer, (UINT32)MinimumNumber);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error, Reading Buffer[n] in func PciUefiMemReadNBits()\n", __func__));
      return Status;
    }

    Private->MailboxCmd.OutputSize = MinimumNumber;
    MinimumNumber                  = (size_t)MinimumOfThreeValues (Private->MailboxCmd.OutputSize, Private->MemdevState.PayloadSize, OutputLength);
    CopyMem (Private->MailboxCmd.OutputPayload, Buffer, MinimumNumber);
  } else {
    Private->MailboxCmd.OutputSize = 0;
  }

  return EFI_SUCCESS;
}

/**
  Setup Mailbox register Payload size, Mailbox register provide the ability to issue a command to the device

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval Status                      Return EFI_SUCCESS on successfully calling CxlDecodeRegblock

**/
EFI_STATUS
CxlSetMailboxPayloadSize (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  UINT32      Capability = 0;

  // 8.2.8.4.3 Mailbox Capabilities Register
  Status = PciUefiMemRead32 (Private, Private->RegisterMap.MailboxRegistersOffset + CXL_DEV_MBOX_CAPS_OFFSET, &Capability);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (CxlMboxWaitForDoorbell (Private) != 0) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error, Mailbox timeout\n", __func__));
    Status = EFI_TIMEOUT;
    return Status;
  }

  Private->MemdevState.PayloadSize = (UINT32)1 << GetFieldValues (Capability, 4, 0);
  Private->MemdevState.PayloadSize = MIN (Private->MemdevState.PayloadSize, CXL_SZ_1M);

  if (Private->MemdevState.PayloadSize < 256) {
    DEBUG ((EFI_D_ERROR, "[%a]: Mailbox is too small\n", __func__));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = EFI_SUCCESS;
  return Status;
}

/**
  Setup Mailbox register which provide the ability to issue a command to the device

  @param[in] Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval Status                      Return EFI_SUCCESS on successfully calling CxlDecodeRegblock

**/
EFI_STATUS
EFIAPI
CxlMailBoxSetup (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  Status = CxlFindRegblockInstance (Private, CxlRbiMemdev);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error CxlFindRegblockInstance\n", __func__));
    FreePool (Private);
    return Status;
  }

  Status = CxlGetMboxRegs (Private);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error CxlGetMboxRegs\n", __func__));
    FreePool (Private);
    return Status;
  }

  Status = CxlSetMailboxPayloadSize (Private);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "[%a]: Error CxlSetMailboxPayloadSize\n", __func__));
    FreePool (Private);
    return Status;
  }

  return Status;
}

/**
  Tests to see if this driver supports a given controller.

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
CxlDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_STATUS           RegStatus;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT8                ClassCode[3];
  UINT32               ExtCapOffset;
  UINT32               NextExtCapOffset;
  UINT32               PcieExtCapAndDvsecHeader[PcieDvsecHeaderMax];

  // Ensure driver won't be started multiple times
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiCallerIdGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return EFI_ALREADY_STARTED;
  }

  // Attempt to Open PCI I/O Protocol
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] PciGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (ClassCode),
                        ClassCode
                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if ((ClassCode[0] != CXL_MEMORY_PROGIF) || (ClassCode[1] != CXL_MEMORY_SUB_CLASS) || (ClassCode[2] != CXL_MEMORY_CLASS)) {
    DEBUG ((EFI_D_ERROR, "[%a]: UNSUPPORTED Class [CXL-%08X] \n", __func__, Controller));
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  DEBUG ((EFI_D_INFO, "[%a]: SUPPORTED ClassCode0 = %d\n", __func__, ClassCode[0]));
  DEBUG ((EFI_D_INFO, "[%a]: SUPPORTED ClassCode1 = %d\n", __func__, ClassCode[1]));
  DEBUG ((EFI_D_INFO, "[%a]: SUPPORTED ClassCode2 = %d\n", __func__, ClassCode[2]));

  Status           = EFI_UNSUPPORTED;
  NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_OFFSET;
  do {
    ExtCapOffset = NextExtCapOffset;
    DEBUG ((EFI_D_INFO, "[%a]: ExtCapOffset = %d \n", __func__, ExtCapOffset));
    RegStatus = PciIo->Pci.Read (
                             PciIo,
                             EfiPciIoWidthUint32,
                             ExtCapOffset,
                             PcieDvsecHeaderMax,
                             PcieExtCapAndDvsecHeader
                             );

    if (EFI_ERROR (RegStatus)) {
      DEBUG ((EFI_D_ERROR, "[%a]: Failed to read PCI IO for Ext. capability \n", __func__));
      goto EXIT;
    }

    /* Check whether this is a CXL device */
    if (CXL_IS_DVSEC (PcieExtCapAndDvsecHeader[PcieDvsecHeader1])) {
      Status = EFI_SUCCESS;
      break;
    }

    NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_NEXT (
                         PcieExtCapAndDvsecHeader[PcieExtCapHeader]
                         );
  } while (NextExtCapOffset);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: ****[CXL-%08X] Error: Non CXL Device****\n", __func__, Controller));
  }

EXIT:

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
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
CxlDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
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
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
CxlDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_PCI_IO_PROTOCOL          *PciIo;
  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath;
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] path OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if (Status == EFI_ALREADY_STARTED) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo EFI_ALREADY_STARTED status = %r\n", __func__, Controller, Status));
  } else {
    Private = AllocateZeroPool (sizeof (CXL_CONTROLLER_PRIVATE_DATA));
    if (Private == NULL) {
      DEBUG ((EFI_D_ERROR, "[%a]: Allocating for CXL Private Data failed!\n", __func__));
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    Private->Signature           = CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE;
    Private->ControllerHandle    = Controller;
    Private->ImageHandle         = This->DriverBindingHandle;
    Private->DriverBindingHandle = This->DriverBindingHandle;
    Private->PciIo               = PciIo;
    Private->ParentDevicePath    = ParentDevicePath;

    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] Allocation Completed status = %r\n", __func__, Controller, Status));

    Status = PciIo->GetLocation (PciIo, &Private->Seg, &Private->Bus, &Private->Device, &Private->Function);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error PciIo GetLocation\n", __func__));
      FreePool (Private);
      return Status;
    }

    DEBUG ((EFI_D_INFO, "[%a]: Bus = %d, DEVICE = %d, FUNCTION = %d\n", __func__, Private->Bus, Private->Device, Private->Function));

    Status = CxlMailBoxSetup (Private);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error in setting up MailBox\n", __func__));
      return Status;
    }
  }

  return EFI_SUCCESS;

EXIT:

  gBS->CloseProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      This->DriverBindingHandle,
                      Controller
                      );

  gBS->CloseProtocol (
                      Controller,
                      &gEfiDevicePathProtocolGuid,
                      This->DriverBindingHandle,
                      Controller
                      );

  DEBUG ((EFI_D_INFO, "[%a]: [CXL-%08X] Completed status = %r\n", __func__, Controller, Status));
  return Status;
}

/**
  The entry point for CXL driver, used to install CXL driver on the ImageHandle.

  @param  ImageHandle   The firmware allocated handle for this driver image.
  @param  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   Driver loaded.
  @retval other         Driver not loaded.

**/
EFI_STATUS
EFIAPI
CxlDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((EFI_D_INFO, "[%a]: Driver entery point get called\n", __func__));

  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gCxlDriverBinding,
             ImageHandle,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL
             );

  DEBUG ((EFI_D_INFO, "[%a]: CXL Installing DriverBinding status = %r\n", __func__, Status));
  ASSERT_EFI_ERROR (Status);
  gRT = SystemTable->RuntimeServices;
  return Status;
}


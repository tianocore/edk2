/** @file
  CxlDxe driver is used to discover CXL devices
  supports Mailbox functionality
  uefi driver name is added
  supports Get Fw Info
  Sending/Receiving FMP commands
  Set Fw Image, Activate Fw image
  SetPkgInfo, GetPkgInfo, CheckImg
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"

EFI_DRIVER_BINDING_PROTOCOL  gCxlDriverBinding = {
  CxlDriverBindingSupported,
  CxlDriverBindingStart,
  CxlDriverBindingStop,
  0x10,
  NULL,
  NULL
};

BOOLEAN is_cxl_doorbell_busy(CXL_CONTROLLER_PRIVATE_DATA *Private) {
  BOOLEAN retVal = 0;
  UINT32 Val = 0;
  EFI_STATUS Status;
  Status = pci_uefi_mem_read_32(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_CTRL_OFFSET, &Val);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error reading Val for busy doorbell\n", __func__));
  }
  retVal = Val & CXL_DEV_MBOX_CTRL_DOORBELL;
  return retVal;
}

EFI_STATUS cxl_internal_send_cmd(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  EFI_STATUS  Status;
  size_t out_size = 0, min_out = 0;

  if (Private->mbox_cmd.size_in > Private->mds.payload_size ||
    Private->mbox_cmd.size_out > Private->mds.payload_size) {
    DEBUG((EFI_D_ERROR, "[%a]: size_in or sizeout > payload_size\n", __func__));
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  out_size = Private->mbox_cmd.size_out;
  min_out = Private->mbox_cmd.min_out;
  Status = cxl_pci_mbox_send(Private);
  if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error returned in func cxl_pci_mbox_send()\n", __func__));
      return Status;
  }

  if (Private->mbox_cmd.return_code != CXL_MBOX_CMD_RC_SUCCESS &&
    Private->mbox_cmd.return_code != CXL_MBOX_CMD_RC_BACKGROUND) {

      //8.2.8.4.5.1 Command Return Codes
      switch(Private->mbox_cmd.return_code)
      {
      case CXL_MBOX_CMD_INVALID_INPUT:
        DEBUG((EFI_D_ERROR, "[%a]: Invalid Input\n", __func__));
        break;

      case CXL_MBOX_CMD_UNSUPPORTED:
        DEBUG((EFI_D_ERROR, "[%a]: Unsupported\n", __func__));
        break;

      case CXL_MBOX_CMD_INTERNAL_ERROR:
        DEBUG((EFI_D_ERROR, "[%a]: Internal Error\n", __func__));
        break;

      case CXL_MBOX_CMD_RETRY_REQUIRED:
        DEBUG((EFI_D_ERROR, "[%a]: Retry Required\n", __func__));
        break;

      case CXL_MBOX_CMD_BUSY:
        DEBUG((EFI_D_ERROR, "[%a]: Busy\n", __func__));
        break;

      case CXL_MBOX_CMD_MEDIA_DISABLED:
        DEBUG((EFI_D_ERROR, "[%a]: Media Disabled\n", __func__));
        break;

      case CXL_MBOX_CMD_FW_TRANSFER_IN_PROGRESS:
        DEBUG((EFI_D_ERROR, "[%a]: FW Transfer in Progress\n", __func__));
        break;

      case CXL_MBOX_CMD_FW_TRANSFER_OUT_OF_ORDER:
        DEBUG((EFI_D_ERROR, "[%a]: FW Transfer Out of Order\n", __func__));
        break;

      case CXL_MBOX_CMD_FW_VERIFICATION_FAILED:
        DEBUG((EFI_D_ERROR, "[%a]: FW Verification Failed\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_SLOT:
        DEBUG((EFI_D_ERROR, "[%a]: Invalid Slot\n", __func__));
        break;

      case CXL_MBOX_CMD_ACTIVATION_FAILED_FW_ROLLED_BACK:
        DEBUG((EFI_D_ERROR, "[%a]: Activation Failed, FW Rolled Back\n", __func__));
        break;

      case CXL_MBOX_CMD_COLD_RESET_REQUIRED:
        DEBUG((EFI_D_ERROR, "[%a]: Activation Failed, Cold Reset Required\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_HANDLE:
        DEBUG((EFI_D_ERROR, "[%a]: Invalid Handle\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_PHYSICAL_ADDRESS:
        DEBUG((EFI_D_ERROR, "[%a]: Invalid Physical Address\n", __func__));
        break;

      case CXL_MBOX_CMD_INJECT_POISON_LIMIT_REACHED:
        DEBUG((EFI_D_ERROR, "[%a]: Inject Poison Limit Reached\n", __func__));
        break;

      case CXL_MBOX_CMD_PERMANENT_MEDIA_FAILURE:
        DEBUG((EFI_D_ERROR, "[%a]: Permanent Media Failure\n", __func__));
        break;

      case CXL_MBOX_CMD_ABORTED:
        DEBUG((EFI_D_ERROR, "[%a]: Aborted\n", __func__));
        break;

      case CXL_MBOX_CMD_INVALID_SECURITY_STATE:
        DEBUG((EFI_D_ERROR, "[%a]: Invalid Security State\n", __func__));
        break;

      case CXL_MBOX_CMD_INCORRECT_PASSPHRASE:
        DEBUG((EFI_D_ERROR, "[%a]: Incorrect Passphrase\n", __func__));
        break;

      default:
        DEBUG((EFI_D_ERROR, "[%a]: Error \n", __func__));
        break;
      }

      Status = EFI_LOAD_ERROR;
      return Status;
  }

  if (!out_size) {
    Status = EFI_SUCCESS;
    return Status;
  }

  /*
   * Variable sized output needs to at least satisfy the caller's
   * minimum if not the fully requested size.
   */
  if (min_out == 0) {
    min_out = out_size;
  }

  if (Private->mbox_cmd.size_out < min_out) {
    DEBUG((EFI_D_ERROR, "[%a]: size_out less than min_out \n", __func__));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = EFI_SUCCESS;
  return Status;
}

EFI_STATUS cxl_mem_get_fw_info(CXL_CONTROLLER_PRIVATE_DATA  *Private)
{
  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: UEFI Driver Get FW Info (Opcode 0200h) get called!\n"));
  struct cxl_mbox_get_fw_info info;
  EFI_STATUS  Status;

  Private->mbox_cmd = (struct cxl_mbox_cmd) {
    .opcode = CXL_MBOX_OP_GET_FW_INFO,
    .size_out = sizeof(info),
    .payload_out = &info,
  };

  Status = cxl_internal_send_cmd(Private);
  if (Status != EFI_SUCCESS){
    DEBUG((EFI_D_ERROR, "cxl_mem_get_fw_info: Error cxl_internal_send_cmd\n"));
    return Status;
  }

  //FW Slots Supported
  Private->mds.fw.num_slots = info.num_slots;
  Private->slotInfo.num_slots = info.num_slots;


  //FW Slot Info
  Private->mds.fw.cur_slot = field_get(info.slot_info, 2, 0);
  Private->mds.fw.next_slot = field_get(info.slot_info, 5, 3);

  //FW Activation Capabilities
  Private->mds.fw.fwActivationCap = field_get(info.slot_info, 0, 0);

  //Slot 1-4 FW Revision
  CopyMem(Private->mds.fw.fwRevisionslot1, info.slot_1_revision, sizeof(info.slot_1_revision));
  CopyMem(Private->mds.fw.fwRevisionslot2, info.slot_2_revision, sizeof(info.slot_2_revision));
  CopyMem(Private->mds.fw.fwRevisionslot3, info.slot_3_revision, sizeof(info.slot_3_revision));
  CopyMem(Private->mds.fw.fwRevisionslot4, info.slot_4_revision, sizeof(info.slot_4_revision));

  //Slot 1-4 FW Revision
  CopyMem(Private->slotInfo.firmware_version[0], info.slot_1_revision, sizeof(info.slot_1_revision));
  CopyMem(Private->slotInfo.firmware_version[1], info.slot_2_revision, sizeof(info.slot_2_revision));
  CopyMem(Private->slotInfo.firmware_version[2], info.slot_3_revision, sizeof(info.slot_3_revision));
  CopyMem(Private->slotInfo.firmware_version[3], info.slot_4_revision, sizeof(info.slot_4_revision));

  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: Total Slots = %u\n", Private->mds.fw.num_slots));
  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: Current Slots = %u\n", Private->mds.fw.cur_slot));
  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: Next Slots = %u\n", Private->mds.fw.next_slot));
  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: Activation Cap = %u\n", Private->mds.fw.fwActivationCap));

  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: FW Version slot 1 = %a\n", Private->mds.fw.fwRevisionslot1));
  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: FW Version slot 2 = %a\n", Private->mds.fw.fwRevisionslot2));
  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: FW Version slot 3 = %a\n", Private->mds.fw.fwRevisionslot3));
  DEBUG((EFI_D_INFO, "cxl_mem_get_fw_info: FW Version slot 4 = %a\n", Private->mds.fw.fwRevisionslot4));

  Status = EFI_SUCCESS;
  return Status;
}

EFI_STATUS cxl_mem_activate_fw(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  EFI_STATUS  Status;
  DEBUG((EFI_D_INFO, "cxl_mem_activate_fw: UEFI Driver Activate FW (Opcode 0202h) get called!\n"));
  struct cxl_mbox_activate_fw activate;

  int slot = Private->mds.fw.next_slot;
  if (slot == 0 || slot > Private->mds.fw.num_slots) {
    DEBUG((EFI_D_ERROR, "[cxl_mem_activate_fw] Error, slot = %d, num of slots = %d \n", slot, Private->mds.fw.num_slots));
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  /* Only offline activation supported for now */
  activate.action = CXL_FW_ACTIVATE_OFFLINE;
  activate.slot = slot;

  Private->mbox_cmd = (struct cxl_mbox_cmd){
    .opcode = CXL_MBOX_OP_ACTIVATE_FW,
    .size_in = sizeof(activate),
    .payload_in = &activate,
  };

  Status = cxl_internal_send_cmd(Private);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "cxl_mem_activate_fw: Error cxl_internal_send_cmd\n"));
    return Status;
  }

  Status = EFI_SUCCESS;
  return Status;
}

EFI_STATUS cxl_mem_transfer_fw(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 nextslot, const UINT8 *data,
    UINT32 offset, UINT32 size, UINT32 *written)
{
  DEBUG((EFI_D_INFO, "cxl_mem_transfer_fw: UEFI Driver Transfer FW (Opcode 0201h) get called!\n"));
  struct cxl_mbox_transfer_fw *transfer;
  UINT32 cur_size = 0, remaining = 0;
  size_t size_in = 0;
  EFI_STATUS Status;
  int chunkCnt = 0;
  int chunkSize = 0;
  BOOLEAN isLastChunk = FALSE;
  *written = 0;

  /* Offset has to be aligned to 128B (CXL-3.0 8.2.9.3.2 Table 8-57) */
  if (!IS_ALIGNED(offset, CXL_FW_TRANSFER_ALIGNMENT)) {
    DEBUG((EFI_D_ERROR, "[cxl_mem_transfer_fw] Error, misaligned offset for FW transfer slice (%u) \n", offset));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Private->mds.fw.oneshot = ((sizeof(*transfer) + size) < (Private->mds.payload_size));
  /*
   * Pick transfer size based on mds.payload_size @size must bw 128-byte
   * aligned, ->payload_size is a power of 2 starting at 256 bytes, and
   * sizeof(*transfer) is 128.  These constraints imply that @cur_size
   * will always be 128b aligned.
   * sizeof(size_t) = 8
   */

  cur_size = min2size(size, Private->mds.payload_size - sizeof(*transfer));
  remaining = size;
  size_in = (sizeof(*transfer) + (sizeof(UINT8) * cur_size));

  Private->mds.fw.next_slot = nextslot;

  transfer = (struct cxl_mbox_transfer_fw*)AllocatePool(size_in);
  if (!transfer) {
    DEBUG((EFI_D_ERROR, "cxl_mem_transfer_fw: Error in AllocatePool\n"));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  if (Private->mds.fw.oneshot) {
    transfer->offset = (offset / CXL_FW_TRANSFER_ALIGNMENT);
    CopyMem(transfer->data, data + offset, cur_size);

    transfer->action = CXL_FW_TRANSFER_ACTION_FULL;
    transfer->slot = Private->mds.fw.next_slot;

    ZeroMem(&Private->mbox_cmd, sizeof(struct cxl_mbox_cmd));
    Private->mbox_cmd = (struct cxl_mbox_cmd){
      .opcode = CXL_MBOX_OP_TRANSFER_FW,
      .size_in = size_in,
      .payload_in = transfer,
      .poll_interval_ms = 1000,
      .poll_count = 30,
    };

    Status = cxl_internal_send_cmd(Private);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "cxl_mem_transfer_fw: Error returned from func cxl_internal_send_cmd()\n"));
      goto OUT_FREE;
    }
  } else {
      getChunkCnt(size, cur_size, &chunkCnt, &chunkSize);
      for (int i = 0; i < chunkCnt; i++)
      {
        if (offset == 0) {
            transfer->action = CXL_FW_TRANSFER_ACTION_INITIATE;
        } else if (remaining < chunkSize) {
            isLastChunk = TRUE;
            transfer->action = CXL_FW_TRANSFER_ACTION_END;
        } else {
            transfer->action = CXL_FW_TRANSFER_ACTION_CONTINUE;
        }

        transfer->slot = Private->mds.fw.next_slot;

        //This is done as offset is multiplied by CXL_FW_TRANSFER_ALIGNMENT in Qemu or hw
        transfer->offset = (offset / CXL_FW_TRANSFER_ALIGNMENT);
        CopyMem(transfer->data, data + offset, cur_size);

        ZeroMem(&Private->mbox_cmd, sizeof(struct cxl_mbox_cmd));
        Private->mbox_cmd = (struct cxl_mbox_cmd){
          .opcode = CXL_MBOX_OP_TRANSFER_FW,
          .size_in = size_in,
          .payload_in = transfer,
          .poll_interval_ms = 1000,
          .poll_count = 30,
        };

        Status = cxl_internal_send_cmd(Private);
        if (Status != EFI_SUCCESS) {
            DEBUG((EFI_D_ERROR, "cxl_mem_transfer_fw: Error returned from func cxl_internal_send_cmd()\n"));
            goto OUT_FREE;
        }

        if (isLastChunk == TRUE) {
          offset = 0;
          remaining = 0;
          break;
        }

        offset = offset + chunkSize;
        remaining = remaining - chunkSize;
        if (CXL_QEMU) {
          gBS->Stall(1000000);
        }
      }
    }

  DEBUG((EFI_D_INFO, "[cxl_mem_transfer_fw] Transfer Fw completed on slot = %d\n", Private->mds.fw.next_slot));
  *written = cur_size;

  /* Activate FW if oneshot or if the last slice was written */
  if (Private->mds.fw.oneshot || remaining == 0) {
    DEBUG((EFI_D_INFO, "[cxl_mem_transfer_fw] Activating firmware on slot: %d\n", Private->mds.fw.next_slot));
    if (CXL_QEMU) {
      gBS->Stall(10000000);
    }

    Status = cxl_mem_activate_fw(Private);
    if (Status != EFI_SUCCESS) {
      goto OUT_FREE;
    }
  }
  Status = EFI_SUCCESS;

OUT_FREE:

  if (transfer) {
    FreePool(transfer);
  }
  return Status;
}

UINT32 pci_find_next_ext_capability(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT32 cap)
{
  UINT32 header = 0;
  UINT32 ttl = 0;
  UINT32 pos = CXL_PCI_CFG_SPACE_SIZE;

  /* minimum 8 bytes per capability */
  ttl = (CXL_PCI_CFG_SPACE_EXP_SIZE - CXL_PCI_CFG_SPACE_SIZE) / 8;

  if (start) {
    pos = start;
  }

  if (pci_uefi_read_config_word(Private, pos, &header) != EFI_SUCCESS){
    DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_read_config_word\n", __func__));
    return 0;
  }
  /*
   * If we have no capabilities, this is indicated by cap ID,
   * cap version and next pointer all being 0.
   */
  if (header == 0) {
    DEBUG((EFI_D_ERROR, "[%a]: Error, header\n", __func__));
    return 0;
  }

  while (ttl-- > 0) {
    if (CXL_PCI_EXT_CAP_ID(header) == cap && pos != start) {
      return pos;
    }

    pos = CXL_PCI_EXT_CAP_NEXT(header);
    if (pos < CXL_PCI_CFG_SPACE_SIZE) {
      break;
    }
    if (pci_uefi_read_config_word(Private, pos, &header) != EFI_SUCCESS) {
      break;
    }
  }
  return 0;
}

UINT32 pci_find_dvsec_capability(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 vendor, UINT32 dvsec)
{
  UINT32 pos = 0;

  /* CXL_PCI_EXT_CAP_ID_DVSEC: 23h is Extended cap struct id for DVSEC */
  pos = pci_find_next_ext_capability(Private, 0, CXL_PCI_EXT_CAP_ID_DVSEC);
  if (!pos){
    DEBUG((EFI_D_ERROR, "[%a]: Error, pos = 0\n", __func__));
    return 0;
  }

  while (pos) {
    UINT32 v = 0, id = 0;
    UINT32 vendorId = 0, DvsecId = 0;
    EFI_STATUS  Status;
    Status = pci_uefi_read_config_word(Private, pos + CXL_PCI_DVSEC_HEADER1, &v);  //DVSEC Header 1 (Offset 04h) Bit Loc 15:0, get DVSEC Vendor ID 1E98h
    if (Status != EFI_SUCCESS){
      return 0;
    }

    Status = pci_uefi_read_config_word(Private, pos + CXL_PCI_DVSEC_HEADER2, &id); //DVSECc Header 2 (Offset 08h) Bit Loc 15:0, get DVSEC ID 0008h
    if (Status != EFI_SUCCESS){
      return 0;
    }

    vendorId = field_get(v, 15, 0);
    DvsecId = field_get(id, 15, 0);
    if (vendor == vendorId && dvsec == DvsecId)
    {
      /*
      Return offset at which DVSEC Vendor ID 1E98h and DVSEC ID 0008h get matched
      This required Register Locator DVSEC, if not, search next extended capability
      */
      return pos;
    }
    pos = pci_find_next_ext_capability(Private, pos, CXL_PCI_EXT_CAP_ID_DVSEC);
  }
  return 0;
}

BOOLEAN cxl_decode_regblock(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 reg_lo, UINT32 reg_hi, enum cxl_regloc_type type)
{
  UINT32 reg_type = 0;

  //Get Register BIR, 8.1.9.1, Register Offset Low (Offset: Varies)
  /*************************************************************************************
  Indicates which one of a Functions BARs, located beginning at Offset
  10h in Configuration Space, or entry in the Enhanced Allocation capability with a
  matching BAR Equivalent Indicator (BEI), is used to map the CXL registers into
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

  UINT32 BAR = 0;
  UINT32 bar = field_get(reg_lo, 2, 0);

  /*
  BAR0: bar = 0,1
  BAR1: bar = 2,3
  BAR2: bar = 4,5
  BAR3: bar = 6,7
  */
  if (bar == 0 || bar == 1) {
    BAR = 0;
  } else if (bar == 2 || bar == 3) {
      BAR = 1;
  } else if (bar == 4 || bar == 5) {
      BAR = 2;
  } else {
      BAR = 3;
  }

  /***********************************************************************************
  8.1.9.2 Register Offset High
  ***********************************************************************************/
  UINT64 offset = ((UINT64)reg_hi << 32) | (reg_lo & CXL_GENMASK(31, 16));

  //Get Register Block Identifier which Identifies the type of CXL registers.
  //Register Block Identifier 03h is CXL Device Registers
  reg_type = field_get(reg_lo, 15, 8);

  if (reg_type == type) {
    Private->map.reg_type = field_get(reg_lo, 15, 8);
    Private->map.bar = BAR;
    Private->map.regoffset = offset;
  } else {
      return FALSE;
    }
  return TRUE;
}

EFI_STATUS cxl_find_regblock_instance(CXL_CONTROLLER_PRIVATE_DATA *Private, enum cxl_regloc_type type, UINT32 index)
{
  UINT32 regloc_size = 0, regblocks = 0, DVSEC_Header1 = 0;
  UINT32 regloc = 0, i = 0;
  EFI_STATUS Status;

  //Get dvsec cap with id 8, which is reg locator
  regloc = pci_find_dvsec_capability(Private, CXL_PCI_DVSEC_VENDOR_ID_DECIMAL, CXL_DVSEC_REG_LOCATOR);
  if (regloc == 0) {
    DEBUG((EFI_D_ERROR, "[%a]: Error in pci_find_dvsec_capability\n", __func__));
    return EFI_UNSUPPORTED;
  }

  //8.1.9 Register Locator DVSEC, Figure 8-6, Table 8-9, CXL-3.1-Specification
  //Read Register Locator DVSEC - Header1
  Status = pci_uefi_read_config_word(Private, regloc + CXL_PCI_DVSEC_HEADER1, &DVSEC_Header1);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_read_config_word\n", __func__));
    return Status;
  }

  //Get DVSEC length from Bit position 31:20 (Table 8-9), from above read Designated Vendor-specific Header 1
  regloc_size = field_get(DVSEC_Header1, 31, 20);

  //Get Register Block 1 - Register Offset Low, By adding 0Ch to base offset
  regloc += CXL_DVSEC_REG_LOCATOR_BLOCK1_OFFSET;

  //Get Num of Reg Blocks, (Total size - Reg Block offset) and each block is of size 8
  regblocks = (regloc_size - CXL_DVSEC_REG_LOCATOR_BLOCK1_OFFSET) / 8;

  //Loop for each Reg block and get Register Offset Low and high
  for (i = 0; i < regblocks; i++, regloc += 8) {
    UINT32 reg_lo = 0, reg_hi = 0;
    Status = pci_uefi_read_config_word(Private, regloc, &reg_lo);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_read_config_word, reg_lo\n", __func__));
      return Status;
    }

    Status = pci_uefi_read_config_word(Private, regloc + 4, &reg_hi);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_read_config_word, reg_hi\n", __func__));
      return Status;
    }

    //Using SEction 8.1.9.1 (CXL-3.1-Specification) Decode Register Offset Low, Register Offset High
    if (!cxl_decode_regblock(Private, reg_lo, reg_hi, type)) {
      continue;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS cxl_pci_get_mbox_regs(CXL_CONTROLLER_PRIVATE_DATA *Private) {
  UINT32 cap = 0, cap_count = 0;
  UINT64 cap_array = 0;
  UINT32 Val = 0;
  EFI_STATUS Status;
  Status = pci_uefi_mem_read_64(Private, CXL_DEV_CAP_ARRAY_OFFSET, &cap_array);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error getting cap_array\n", __func__));
    return Status;
  }

  if (field_get(cap_array, 15, 0) != CXL_DEV_CAP_ARRAY_CAP_ID) {
    DEBUG((EFI_D_ERROR, "[%a]: Error CXL_DEV_CAP_ARRAY_CAP_ID not matching\n", __func__));
    return EFI_NOT_FOUND;
  }

  cap_count = field_get(cap_array, 47, 32);

  for (cap = 1; cap <= cap_count; cap++) {
    UINT32 offset = 0, length = 0;
    UINT16 cap_id = 0;

    Status = pci_uefi_mem_read_32(Private, cap * 0x10, &Val);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error reading Val for cap_id\n", __func__));
      return Status;
    }

    cap_id = field_get(Val, 15, 0);

    //8.2.8.2 CXL Device Capability Header Register
    Status = pci_uefi_mem_read_32(Private, cap * 0x10 + 0x4, &offset);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error reading offset\n", __func__));
      return Status;
    }

    Status = pci_uefi_mem_read_32(Private, cap * 0x10 + 0x8, &length);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error reading length\n", __func__));
      return Status;
    }

    switch (cap_id) {
    case CXL_DEV_CAP_CAP_ID_PRIMARY_MAILBOX:
      Private->map.mBoxoffset = offset;
      break;

    case CXL_DEV_CAP_CAP_ID_SECONDARY_MAILBOX:
      break;

    default:
      if (cap_id >= 0x8000) {
        DEBUG((EFI_D_INFO, "[%a]: cap_id >= 0x8000\n", __func__));
      }
      break;
    }
  }
  return EFI_SUCCESS;
}

UINT32 cxl_pci_mbox_wait_for_doorbell(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  int cnt = 0;
  while (is_cxl_doorbell_busy(Private)) {
    gBS->Stall(1000); //1000Microsecond, 1 ms
    cnt++;

    /* Check again in case preempted before timeout test */
    if (!is_cxl_doorbell_busy(Private)) {
      break;
    }

    if (cnt == CXL_MAILBOX_TIMEOUT_MS) {
      return 1;
    }
  }
  return 0;
}

static BOOLEAN cxl_mbox_background_complete(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  UINT64 reg;
  UINT64 BGStatus;
  EFI_STATUS  Status;
  Status = pci_uefi_mem_read_64(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_BG_CMD_STATUS_OFFSET, &reg);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error reading reg for background status cmd\n", __func__));
    return 0;
  }

  BGStatus = field_get(reg, 22, 16);
  if (BGStatus == 100) {
    DEBUG((EFI_D_ERROR, "[%a]: BGStatus = 100\n", __func__));
    return 0;
  }
  return 1;
}

EFI_STATUS cxl_pci_mbox_send(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  /*
  * Here are the steps from 8.2.8.4 of the CXL-3.1-Specification.
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

  //8.2.8.4.5 Command Register (Mailbox Registers Capability Offset + 08h)
  UINT64 cmd_reg = 0, status_reg = 0;
  size_t out_len = 0;
  EFI_STATUS Status;

  /* #1 */
  if (is_cxl_doorbell_busy(Private)) {
    DEBUG((EFI_D_ERROR, "[%a]: Error mailbox queue busy\n", __func__));
    return EFI_TIMEOUT;
  }

  cmd_reg = Private->mbox_cmd.opcode;
  if (Private->mbox_cmd.size_in) {
    if (!Private->mbox_cmd.payload_in){
      DEBUG((EFI_D_ERROR, "[%a]: Error payload_in is 0\n", __func__));
      return EFI_BAD_BUFFER_SIZE;
    }

    //Place bits of size_in starting 16th bits of UINT64 cmd_reg
    cmd_reg |= (Private->mbox_cmd.size_in << 16);

    /*Read Payload buffer in n Bytes*/
    Status = pci_uefi_mem_write_n(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_PAYLOAD_OFFSET, Private->mbox_cmd.payload_in, Private->mbox_cmd.size_in);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_mem_write_n\n", __func__));
      return Status;
    }
  }

  /* #2, #3 */
  Status = pci_uefi_mem_write_64(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_CMD_OFFSET, &cmd_reg);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_mem_write_64\n", __func__));
    return Status;
  }

  /* #4 */
  UINT32 val = CXL_DEV_MBOX_CTRL_DOORBELL;
  Status = pci_uefi_mem_write_32(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_CTRL_OFFSET, &val);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_mem_write_32\n", __func__));
    return Status;
  }

  /* #5 */
  if (cxl_pci_mbox_wait_for_doorbell(Private) != 0) {
    DEBUG((EFI_D_ERROR, "[%a]: Error, Mailbox timeout\n", __func__));
    Status = EFI_TIMEOUT;
    return Status;
  }

  /* #6 */
  Status = pci_uefi_mem_read_64(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_STATUS_OFFSET, &status_reg);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_mem_read_64\n", __func__));
    return Status;
  }

  Private->mbox_cmd.return_code = field_get(status_reg, 47, 32);

  if (Private->mbox_cmd.return_code == CXL_MBOX_CMD_RC_BACKGROUND) {
    int i = 0;
    UINT64 bg_status_reg = 0;

    for (i = 0; i < Private->mbox_cmd.poll_count; i++) {
      gBS->Stall(Private->mbox_cmd.poll_interval_ms * 1000); //Microsecond
      if (cxl_mbox_background_complete(Private)) {
        break;
      }
    }

    if (!cxl_mbox_background_complete(Private)) {
      DEBUG((EFI_D_ERROR, "[%a]: Error, Mailbox timeout\n", __func__));
      return EFI_TIMEOUT;
    }

    Status = pci_uefi_mem_read_64(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_BG_CMD_STATUS_OFFSET, &bg_status_reg);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_mem_read_64\n", __func__));
      return Status;
    }

    Private->mbox_cmd.return_code = field_get(bg_status_reg, 47, 32);
  }

  if (Private->mbox_cmd.return_code != CXL_MBOX_CMD_RC_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Completed but caller must check return_code\n", __func__));
    return EFI_SUCCESS;
  }

  //8.2.8.4.5 Command Register (Mailbox Registers Capability Offset + 08h)
  Status = pci_uefi_mem_read_64(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_CMD_OFFSET, &cmd_reg);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error in pci_uefi_mem_read_64\n", __func__));
    return Status;
  }

  //Payload Length: (36:16)
  out_len = field_get(cmd_reg, 36, 16);

  /* #8 */
  if (out_len && Private->mbox_cmd.payload_out) {
    size_t n = min3(Private->mbox_cmd.size_out, Private->mds.payload_size, out_len);

    /*Read Payload buffer in n Bytes*/
    char Buffer[n];
    Status = pci_uefi_mem_read_n(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_PAYLOAD_OFFSET, Buffer, n);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error, Reading Buffer[n] in func pci_uefi_mem_read_n()\n", __func__));
      return Status;
    }

    Private->mbox_cmd.size_out = n;
    n = min3(Private->mbox_cmd.size_out, Private->mds.payload_size, out_len);
    CopyMem(Private->mbox_cmd.payload_out, Buffer, n);
  } else {
      Private->mbox_cmd.size_out = 0;
    }
  return EFI_SUCCESS;
}

EFI_STATUS cxl_pci_setup_mailbox(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  UINT32 cap = 0;
  EFI_STATUS Status;

  //8.2.8.4.3 Mailbox Capabilities Register
  Status = pci_uefi_mem_read_32(Private, Private->map.mBoxoffset + CXL_DEV_MBOX_CAPS_OFFSET, &cap);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  if (cxl_pci_mbox_wait_for_doorbell(Private) != 0) {
    DEBUG((EFI_D_ERROR, "[%a]: Error, Mailbox timeout\n", __func__));
    Status = EFI_TIMEOUT;
    return Status;
  }

  Private->mds.payload_size = (UINT32)1 << field_get(cap, 4, 0);
  Private->mds.payload_size = min2(Private->mds.payload_size, CXL_SZ_1M);

  if (Private->mds.payload_size < 256) {
    DEBUG((EFI_D_ERROR, "[%a]: Mailbox is too small\n", __func__));
    Status = EFI_LOAD_ERROR;
    return Status;
  }

  Status = EFI_SUCCESS;
  return Status;
}

EFI_STATUS
EFIAPI
CxlMailBoxSetup(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  EFI_STATUS                          Status;
  Status = cxl_find_regblock_instance(Private, CXL_REGLOC_RBI_MEMDEV, 0);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error cxl_find_regblock_instance\n", __func__));
    FreePool(Private);
    return Status;
  }

  Status = cxl_pci_get_mbox_regs(Private);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error cxl_pci_get_mbox_regs\n", __func__));
    FreePool(Private);
    return Status;
  }

  Status = cxl_pci_setup_mailbox(Private);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[%a]: Error cxl_pci_setup_mailbox\n", __func__));
    FreePool(Private);
    return Status;
  }

  //Get FW Info
  Status = cxl_mem_get_fw_info(Private);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "[CxlMailBoxSetup]: Error cxl_mem_get_fw_info()\n"));
    FreePool(Private);
    return Status;
  }

  InitializeFwImageDescriptor(Private);

  CopyMem(&Private->FirmwareMgmt, &gCxlFirmwareManagement, sizeof(EFI_FIRMWARE_MANAGEMENT_PROTOCOL));
  return Status;
}

EFI_STATUS
EFIAPI
CxlDriverBindingSupported(
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status, RegStatus;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT8                 ClassCode[3];
  UINT32                ExtCapOffset, NextExtCapOffset;
  UINT32                PcieExtCapAndDvsecHeader[PCIE_DVSEC_HEADER_MAX];

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
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return EFI_ALREADY_STARTED;
  }

  // Attempt to Open PCI I/O Protocol
  Status = gBS->OpenProtocol (
             Controller,
             &gEfiPciIoProtocolGuid,
             (VOID**)&PciIo,
             This->DriverBindingHandle,
             Controller,
             EFI_OPEN_PROTOCOL_BY_DRIVER
             );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] PciGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = PciIo->Pci.Read (
             PciIo,
             EfiPciIoWidthUint8,
             PCI_CLASSCODE_OFFSET,
             sizeof(ClassCode),
             ClassCode
             );

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if ((ClassCode[0] != CXL_MEMORY_PROGIF) || (ClassCode[1] != CXL_MEMORY_SUB_CLASS) || (ClassCode[2] != CXL_MEMORY_CLASS)) {
    DEBUG((EFI_D_ERROR, "[%a]: UNSUPPORTED Class [CXL-%08X] \n", __func__, Controller));
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  DEBUG((EFI_D_INFO, "[%a]: SUPPORTED ClassCode0 = %d\n", __func__, ClassCode[0]));
  DEBUG((EFI_D_INFO, "[%a]: SUPPORTED ClassCode1 = %d\n", __func__, ClassCode[1]));
  DEBUG((EFI_D_INFO, "[%a]: SUPPORTED ClassCode2 = %d\n", __func__, ClassCode[2]));

  Status = EFI_UNSUPPORTED;
  NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_OFFSET;
  do {
    ExtCapOffset = NextExtCapOffset;
    DEBUG((EFI_D_INFO, "[%a]: ExtCapOffset = %d \n", __func__, ExtCapOffset));
    RegStatus = PciIo->Pci.Read(
                  PciIo,
                  EfiPciIoWidthUint32,
                  ExtCapOffset,
                  PCIE_DVSEC_HEADER_MAX,
                  PcieExtCapAndDvsecHeader
                  );

    if (EFI_ERROR (RegStatus)) {
      DEBUG((EFI_D_ERROR, "[%a]: Failed to read PCI IO for Ext. capability \n", __func__));
      goto EXIT;
    }

    /* Check whether this is a CXL device */
    if (CXL_IS_DVSEC(PcieExtCapAndDvsecHeader[PCIE_DVSEC_HEADER_1])) {
      Status = EFI_SUCCESS;
      break;
    }

    NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_NEXT(
      PcieExtCapAndDvsecHeader[PCIE_EXT_CAP_HEADER]
    );
  } while (NextExtCapOffset);

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: ****[CXL-%08X] Error: Non CXL Device****\n", __func__, Controller));
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

EFI_STATUS
EFIAPI
CxlDriverBindingStop(
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  DEBUG((EFI_D_INFO, "CxlBindStop\n"));
  EFI_STATUS                        Status = EFI_SUCCESS;
  CXL_CONTROLLER_PRIVATE_DATA       *Private;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *FMP;

  Status = gBS->OpenProtocol(
             Controller,
             &gEfiFirmwareManagementProtocolGuid,
             (VOID**)&FMP,
             This->DriverBindingHandle,
             Controller,
             EFI_OPEN_PROTOCOL_GET_PROTOCOL
             );

  if (!EFI_ERROR(Status)) {
    Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(FMP);
    gBS->UninstallMultipleProtocolInterfaces(
      Controller,
      &gEfiFirmwareManagementProtocolGuid,
      FMP,
      NULL,
      NULL,
      &gEfiFirmwareManagementProtocolGuid,
      &Private->FirmwareMgmt,
      NULL
      );

    FreePool(Private);
  }

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

EFI_STATUS
EFIAPI
CxlDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_PCI_IO_PROTOCOL          *PciIo;
  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath;
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] path OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = gBS->OpenProtocol(
             Controller,
             &gEfiPciIoProtocolGuid,
             (VOID**)&PciIo,
             This->DriverBindingHandle,
             Controller,
             EFI_OPEN_PROTOCOL_BY_DRIVER
             );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if (Status == EFI_ALREADY_STARTED) {
    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo EFI_ALREADY_STARTED status = %r\n", __func__, Controller, Status));
  } else {
    Private = AllocateZeroPool(sizeof(CXL_CONTROLLER_PRIVATE_DATA));
    if (Private == NULL) {
      DEBUG((EFI_D_ERROR, "[%a]: Allocating for CXL Private Data failed!\n", __func__));
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    Private->Signature              = CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE;
    Private->ControllerHandle       = Controller;
    Private->ImageHandle            = This->DriverBindingHandle;
    Private->DriverBindingHandle    = This->DriverBindingHandle;
    Private->PciIo                  = PciIo;
    Private->ParentDevicePath       = ParentDevicePath;
    strCpy_c16(Private->PackageVersionName, CXL_PACKAGE_VERSION_NAME);
    Private->PackageVersion         = CXL_PACKAGE_VERSION_FFFFFFFE;

    DEBUG((EFI_D_ERROR, "[%a]: [CXL-%08X] Allocation Completed status = %r\n", __func__, Controller, Status));

    Status = PciIo->GetLocation(PciIo, &Private->Seg, &Private->Bus, &Private->Dev, &Private->Func);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error PciIo GetLocation\n", __func__));
      FreePool(Private);
      return Status;
    }

    DEBUG((EFI_D_INFO, "[%a]: Bus = %d, DEVICE = %d, FUNCTION = %d\n", __func__, Private->Bus, Private->Dev, Private->Func));

    Status = CxlMailBoxSetup(Private);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "[%a]: Error in setting up MailBox\n", __func__));
      return Status;
    }

    Status = gBS->InstallMultipleProtocolInterfaces(
      &Private->ControllerHandle,
      &gEfiFirmwareManagementProtocolGuid,
      &Private->FirmwareMgmt,
      NULL
    );

    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "[CxlDriverBindingStart]: [CXL-%08X] Protocol Install failed status = %r\n", Controller, Status));
      FreePool(Private);
      goto EXIT;
    } else {
        DEBUG((EFI_D_INFO, "[CxlDriverBindingStart]: [CXL-%08X] Protocol Install completed status = %r\n", Controller, Status));
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

  DEBUG((EFI_D_INFO, "[%a]: [CXL-%08X] Completed status = %r\n", __func__, Controller, Status));
  return Status;
}

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  CxlDxeComponentName = {
  CxlDxeComponentNameGetDriverName,
  CxlDxeComponentNameGetControllerName,
  "eng"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  CxlDxeComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)CxlDxeComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)CxlDxeComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  CxlDxeDriverNameTable[] = {
  { "eng;en", L"UEFI CXL Driver" },
  { NULL,     NULL  }
};

EFI_STATUS
EFIAPI
CxlDxeComponentNameGetDriverName(
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN CHAR8  *Language,
  OUT CHAR16  **DriverName
  )
{
  return LookupUnicodeString2(
    Language,
    This->SupportedLanguages,
    CxlDxeDriverNameTable,
    DriverName,
    (BOOLEAN)(This == &CxlDxeComponentName)
    );
}

EFI_STATUS
EFIAPI
CxlDxeComponentNameGetControllerName(
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN CHAR8                        *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CxlDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  DEBUG((EFI_D_INFO, "[%a]: Driver entery point get called\n", __func__));

  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gCxlDriverBinding,
             ImageHandle,
             &CxlDxeComponentName,
             &CxlDxeComponentName2,
             NULL,
             NULL,
             NULL,
             NULL
             );

  DEBUG((EFI_D_INFO, "[%a]: CXL Installing DriverBinding status = %r\n", __func__, Status));
  ASSERT_EFI_ERROR (Status);
  gRT = SystemTable->RuntimeServices;
  return Status;
}

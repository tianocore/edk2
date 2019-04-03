/** @file
PCH SPI Common Driver implements the SPI Host Controller Compatibility Interface.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PchSpi.h"

VOID
FillOutPublicInfoStruct (
  SPI_INSTANCE          *SpiInstance
  )
/*++

Routine Description:

  Fillout SpiInstance->InitInfo;

Arguments:

  SpiInstance   - Pointer to SpiInstance to initialize

Returns:

  NONE

--*/
{
  UINT8         Index;

  SpiInstance->InitInfo.InitTable = &SpiInstance->SpiInitTable;

  //
  // Give invalid index in case operation not supported.
  //
  SpiInstance->InitInfo.JedecIdOpcodeIndex = 0xff;
  SpiInstance->InitInfo.OtherOpcodeIndex = 0xff;
  SpiInstance->InitInfo.WriteStatusOpcodeIndex = 0xff;
  SpiInstance->InitInfo.ProgramOpcodeIndex = 0xff;
  SpiInstance->InitInfo.ReadOpcodeIndex = 0xff;
  SpiInstance->InitInfo.EraseOpcodeIndex = 0xff;
  SpiInstance->InitInfo.ReadStatusOpcodeIndex = 0xff;
  SpiInstance->InitInfo.FullChipEraseOpcodeIndex = 0xff;
  for (Index = 0; Index < SPI_NUM_OPCODE; Index++) {
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationJedecId) {
      SpiInstance->InitInfo.JedecIdOpcodeIndex = Index;
    }
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationOther) {
      SpiInstance->InitInfo.OtherOpcodeIndex = Index;
    }
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationWriteStatus) {
      SpiInstance->InitInfo.WriteStatusOpcodeIndex = Index;
    }
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationProgramData_1_Byte ||
        SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationProgramData_64_Byte) {
      SpiInstance->InitInfo.ProgramOpcodeIndex = Index;
    }
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationReadData ||
        SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationFastRead ||
        SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationDualOutputFastRead) {
      SpiInstance->InitInfo.ReadOpcodeIndex = Index;
    }
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationErase_256_Byte ||
        SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationErase_4K_Byte ||
        SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationErase_8K_Byte ||
        SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationErase_64K_Byte) {
      SpiInstance->InitInfo.EraseOpcodeIndex = Index;
    }
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationReadStatus) {
      SpiInstance->InitInfo.ReadStatusOpcodeIndex = Index;
    }
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationFullChipErase) {
      SpiInstance->InitInfo.FullChipEraseOpcodeIndex = Index;
    }
  }
}

EFI_STATUS
SpiProtocolConstructor (
  SPI_INSTANCE          *SpiInstance
  )
/*++

Routine Description:

  Initialize an SPI protocol instance.
  The function will assert in debug if PCH RCBA has not been initialized

Arguments:

  SpiInstance   - Pointer to SpiInstance to initialize

Returns:

  EFI_SUCCESS     The protocol instance was properly initialized
  EFI_UNSUPPORTED The PCH is not supported by this module

--*/
{
  SpiInstance->InitDone = FALSE;  // Indicate NOT READY.

  //
  // Check if the current PCH is known and supported by this code
  //
  if (!IsQncSupported ()) {
    DEBUG ((DEBUG_ERROR, "PCH SPI Protocol not supported due to no proper QNC LPC found!\n"));
    return EFI_UNSUPPORTED;
  }
  //
  // Initialize the SPI protocol instance
  //
  SpiInstance->Signature            = PCH_SPI_PRIVATE_DATA_SIGNATURE;
  SpiInstance->Handle               = NULL;
  SpiInstance->SpiProtocol.Init     = SpiProtocolInit;
  SpiInstance->SpiProtocol.Lock     = SpiProtocolLock;
  SpiInstance->SpiProtocol.Execute  = SpiProtocolExecute;
  SpiInstance->SpiProtocol.Info     = SpiProtocolInfo;

  //
  // Sanity check to ensure PCH RCBA initialization has occurred previously.
  //
  SpiInstance->PchRootComplexBar = MmioRead32 (
                                    PciDeviceMmBase (PCI_BUS_NUMBER_QNC,
                                    PCI_DEVICE_NUMBER_QNC_LPC,
                                    PCI_FUNCTION_NUMBER_QNC_LPC) + R_QNC_LPC_RCBA
                                    ) & B_QNC_LPC_RCBA_MASK;
  ASSERT (SpiInstance->PchRootComplexBar != 0);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnlockFlashComponents (
  IN      EFI_SPI_PROTOCOL      *This,
  IN      UINT8                 UnlockCmdOpcodeIndex
  )
/*++

Routine Description:

  Issue unlock command to disable block protection, this only needs to be done once per SPI power on

Arguments:

  This                      A pointer to "EFI_SPI_PROTOCOL" for issuing commands
  UnlockCmdOpcodeIndex      The index of the Unlock command

Returns:

  EFI_SUCCESS               UnLock operation succeed.
  EFI_DEVICE_ERROR          Device error, operation failed.

--*/
{
  EFI_STATUS    Status;
  SPI_INSTANCE  *SpiInstance;
  UINT8         SpiStatus;

  if (UnlockCmdOpcodeIndex >= SPI_NUM_OPCODE) {
    return EFI_UNSUPPORTED;
  }

  SpiInstance = SPI_INSTANCE_FROM_SPIPROTOCOL (This);

  //
  // Issue unlock command to disable block protection, this only needs to be done once per SPI power on
  //
  SpiStatus = 0;
  //
  // Issue unlock command to the flash component 1 at first
  //
  Status = SpiProtocolExecute (
            This,
            UnlockCmdOpcodeIndex,
            SpiInstance->SpiInitTable.PrefixOpcode[0] == PCH_SPI_COMMAND_WRITE_ENABLE ? 0 : 1,
            TRUE,
            TRUE,
            TRUE,
            (UINTN) 0,
            sizeof (SpiStatus),
            &SpiStatus,
            EnumSpiRegionAll
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unlock flash component 1 fail!\n"));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiProtocolInit (
  IN EFI_SPI_PROTOCOL       *This,
  IN SPI_INIT_TABLE         *InitTable
  )
/*++

Routine Description:

  Initialize the host controller to execute SPI command.

Arguments:

  This                    Pointer to the EFI_SPI_PROTOCOL instance.
  InitTable               Initialization data to be programmed into the SPI host controller.

Returns:

  EFI_SUCCESS             Initialization completed.
  EFI_ACCESS_DENIED       The SPI static configuration interface has been locked-down.
  EFI_INVALID_PARAMETER   Bad input parameters.
  EFI_UNSUPPORTED         Can't get Descriptor mode VSCC values
--*/
{
  EFI_STATUS    Status;
  UINT8         Index;
  UINT16        OpcodeType;
  SPI_INSTANCE  *SpiInstance;
  UINTN         PchRootComplexBar;
  UINT8         UnlockCmdOpcodeIndex;
  UINT8         FlashPartId[3];

  SpiInstance       = SPI_INSTANCE_FROM_SPIPROTOCOL (This);
  PchRootComplexBar = SpiInstance->PchRootComplexBar;

  if (InitTable != NULL) {
    //
    // Copy table into SPI driver Private data structure
    //
    CopyMem (
      &SpiInstance->SpiInitTable,
      InitTable,
      sizeof (SPI_INIT_TABLE)
      );
  } else {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check if the SPI interface has been locked-down.
  //
  if ((MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIS) & B_QNC_RCRB_SPIS_SCL) != 0) {
    ASSERT_EFI_ERROR (EFI_ACCESS_DENIED);
    return EFI_ACCESS_DENIED;
  }
  //
  // Clear all the status bits for status regs.
  //
  MmioOr16 (
    (UINTN) (PchRootComplexBar + R_QNC_RCRB_SPIS),
    (UINT16) ((B_QNC_RCRB_SPIS_CDS | B_QNC_RCRB_SPIS_BAS))
    );
  MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIS);

  //
  // Set the Prefix Opcode registers.
  //
  MmioWrite16 (
    PchRootComplexBar + R_QNC_RCRB_SPIPREOP,
    (SpiInstance->SpiInitTable.PrefixOpcode[1] << 8) | InitTable->PrefixOpcode[0]
    );
  MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIPREOP);

  //
  // Set Opcode Type Configuration registers.
  //
  for (Index = 0, OpcodeType = 0; Index < SPI_NUM_OPCODE; Index++) {
    switch (SpiInstance->SpiInitTable.OpcodeMenu[Index].Type) {
    case EnumSpiOpcodeRead:
      OpcodeType |= (UINT16) (B_QNC_RCRB_SPIOPTYPE_ADD_READ << (Index * 2));
      break;
    case EnumSpiOpcodeWrite:
      OpcodeType |= (UINT16) (B_QNC_RCRB_SPIOPTYPE_ADD_WRITE << (Index * 2));
      break;
    case EnumSpiOpcodeWriteNoAddr:
      OpcodeType |= (UINT16) (B_QNC_RCRB_SPIOPTYPE_NOADD_WRITE << (Index * 2));
      break;
    default:
      OpcodeType |= (UINT16) (B_QNC_RCRB_SPIOPTYPE_NOADD_READ << (Index * 2));
      break;
    }
  }
  MmioWrite16 (PchRootComplexBar + R_QNC_RCRB_SPIOPTYPE, OpcodeType);
  MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIOPTYPE);

  //
  // Setup the Opcode Menu registers.
  //
  UnlockCmdOpcodeIndex = SPI_NUM_OPCODE;
  for (Index = 0; Index < SPI_NUM_OPCODE; Index++) {
    MmioWrite8 (
      PchRootComplexBar + R_QNC_RCRB_SPIOPMENU + Index,
      SpiInstance->SpiInitTable.OpcodeMenu[Index].Code
      );
    MmioRead8 (PchRootComplexBar + R_QNC_RCRB_SPIOPMENU + Index);
    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationJedecId) {
      Status = SpiProtocolExecute (
                This,
                Index,
                0,
                TRUE,
                TRUE,
                FALSE,
                (UINTN) 0,
                3,
                FlashPartId,
                EnumSpiRegionDescriptor
                );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      if (FlashPartId[0] != SpiInstance->SpiInitTable.VendorId  ||
          FlashPartId[1] != SpiInstance->SpiInitTable.DeviceId0 ||
          FlashPartId[2] != SpiInstance->SpiInitTable.DeviceId1) {
        return EFI_INVALID_PARAMETER;
      }
    }

    if (SpiInstance->SpiInitTable.OpcodeMenu[Index].Operation == EnumSpiOperationWriteStatus) {
      UnlockCmdOpcodeIndex = Index;
    }
  }

  Status = UnlockFlashComponents (
            This,
            UnlockCmdOpcodeIndex
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Unlock flash components fail!\n"));
  }

  SpiPhaseInit ();
  FillOutPublicInfoStruct (SpiInstance);
  SpiInstance->InitDone = TRUE;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiProtocolLock (
  IN EFI_SPI_PROTOCOL     *This
  )
/*++

Routine Description:

  Lock the SPI Static Configuration Interface.
  Once locked, the interface can not be changed and can only be clear by system reset.

Arguments:

  This      Pointer to the EFI_SPI_PROTOCOL instance.

Returns:

  EFI_SUCCESS             Lock operation succeed.
  EFI_DEVICE_ERROR        Device error, operation failed.
  EFI_ACCESS_DENIED       The interface has already been locked.

--*/
{
  SPI_INSTANCE  *SpiInstance;
  UINTN         PchRootComplexBar;

  SpiInstance       = SPI_INSTANCE_FROM_SPIPROTOCOL (This);
  PchRootComplexBar = SpiInstance->PchRootComplexBar;

  //
  // Check if the SPI interface has been locked-down.
  //
  if ((MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIS) & B_QNC_RCRB_SPIS_SCL) != 0) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Lock-down the configuration interface.
  //
  MmioOr16 ((UINTN) (PchRootComplexBar + R_QNC_RCRB_SPIS), (UINT16) (B_QNC_RCRB_SPIS_SCL));

  //
  // Verify if it's really locked.
  //
  if ((MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIS) & B_QNC_RCRB_SPIS_SCL) == 0) {
    return EFI_DEVICE_ERROR;
  } else {
    //
    // Save updated register in S3 Boot script.
    //
    S3BootScriptSaveMemWrite (
      S3BootScriptWidthUint16,
        (UINTN) (PchRootComplexBar + R_QNC_RCRB_SPIS),
        1,
        (VOID *) (UINTN) (PchRootComplexBar + R_QNC_RCRB_SPIS)
        );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpiProtocolExecute (
  IN     EFI_SPI_PROTOCOL   *This,
  IN     UINT8              OpcodeIndex,
  IN     UINT8              PrefixOpcodeIndex,
  IN     BOOLEAN            DataCycle,
  IN     BOOLEAN            Atomic,
  IN     BOOLEAN            ShiftOut,
  IN     UINTN              Address,
  IN     UINT32             DataByteCount,
  IN OUT UINT8              *Buffer,
  IN     SPI_REGION_TYPE    SpiRegionType
  )
/*++

Routine Description:

  Execute SPI commands from the host controller.
  This function would be called by runtime driver, please do not use any MMIO marco here

Arguments:

  This              Pointer to the EFI_SPI_PROTOCOL instance.
  OpcodeIndex       Index of the command in the OpCode Menu.
  PrefixOpcodeIndex Index of the first command to run when in an atomic cycle sequence.
  DataCycle         TRUE if the SPI cycle contains data
  Atomic            TRUE if the SPI cycle is atomic and interleave cycles are not allowed.
  ShiftOut          If DataByteCount is not zero, TRUE to shift data out and FALSE to shift data in.
  Address           In Descriptor Mode, for Descriptor Region, GbE Region, ME Region and Platform
                    Region, this value specifies the offset from the Region Base; for BIOS Region,
                    this value specifies the offset from the start of the BIOS Image. In Non
                    Descriptor Mode, this value specifies the offset from the start of the BIOS Image.
                    Please note BIOS Image size may be smaller than BIOS Region size (in Descriptor
                    Mode) or the flash size (in Non Descriptor Mode), and in this case, BIOS Image is
                    supposed to be placed at the top end of the BIOS Region (in Descriptor Mode) or
                    the flash (in Non Descriptor Mode)
  DataByteCount     Number of bytes in the data portion of the SPI cycle. This function may break the
                    data transfer into multiple operations. This function ensures each operation does
                    not cross 256 byte flash address boundary.
                    *NOTE: if there is some SPI chip that has a stricter address boundary requirement
                    (e.g., its write page size is < 256 byte), then the caller cannot rely on this
                    function to cut the data transfer at proper address boundaries, and it's the
                    caller's reponsibility to pass in a properly cut DataByteCount parameter.
  Buffer            Pointer to caller-allocated buffer containing the dada received or sent during the
                    SPI cycle.
  SpiRegionType     SPI Region type. Values EnumSpiRegionBios, EnumSpiRegionGbE, EnumSpiRegionMe,
                    EnumSpiRegionDescriptor, and EnumSpiRegionPlatformData are only applicable in
                    Descriptor mode. Value EnumSpiRegionAll is applicable to both Descriptor Mode
                    and Non Descriptor Mode, which indicates "SpiRegionOffset" is actually relative
                    to base of the 1st flash device (i.e., it is a Flash Linear Address).

Returns:

  EFI_SUCCESS             Command succeed.
  EFI_INVALID_PARAMETER   The parameters specified are not valid.
  EFI_UNSUPPORTED         Command not supported.
  EFI_DEVICE_ERROR        Device error, command aborts abnormally.

--*/
{
  EFI_STATUS  Status;
  UINT16      BiosCtlSave;
  UINT32      SmiEnSave;

  BiosCtlSave = 0;
  SmiEnSave   = 0;

  //
  // Check if the parameters are valid.
  //
  if ((OpcodeIndex >= SPI_NUM_OPCODE) || (PrefixOpcodeIndex >= SPI_NUM_PREFIX_OPCODE)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Make sure it's safe to program the command.
  //
  if (!WaitForSpiCycleComplete (This, FALSE)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Acquire access to the SPI interface is not required any more.
  //
  //
  // Disable SMIs to make sure normal mode flash access is not interrupted by an SMI
  // whose SMI handler accesses flash (e.g. for error logging)
  //
  SmiEnSave = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, (SmiEnSave & ~SMI_EN));

  //
  // Save BIOS Ctrl register
  //
  BiosCtlSave = PciRead16 (
                  PCI_LIB_ADDRESS (PCI_BUS_NUMBER_QNC,
                  PCI_DEVICE_NUMBER_QNC_LPC,
                  PCI_FUNCTION_NUMBER_QNC_LPC,
                  R_QNC_LPC_BIOS_CNTL)
                  ) & (B_QNC_LPC_BIOS_CNTL_BCD | B_QNC_LPC_BIOS_CNTL_PFE | B_QNC_LPC_BIOS_CNTL_BIOSWE | B_QNC_LPC_BIOS_CNTL_SMM_BWP);

  //
  // Enable flash writing
  //
  PciOr16 (
    PCI_LIB_ADDRESS (PCI_BUS_NUMBER_QNC,
    PCI_DEVICE_NUMBER_QNC_LPC,
    PCI_FUNCTION_NUMBER_QNC_LPC,
    R_QNC_LPC_BIOS_CNTL),
    (UINT16) (B_QNC_LPC_BIOS_CNTL_BIOSWE | B_QNC_LPC_BIOS_CNTL_SMM_BWP)
    );

  //
  // If shifts the data out, disable Prefetching and Caching.
  //
  if (ShiftOut) {
    PciAndThenOr16 (
      PCI_LIB_ADDRESS (PCI_BUS_NUMBER_QNC,
      PCI_DEVICE_NUMBER_QNC_LPC,
      PCI_FUNCTION_NUMBER_QNC_LPC,
      R_QNC_LPC_BIOS_CNTL),
      (UINT16) (~(B_QNC_LPC_BIOS_CNTL_BCD | B_QNC_LPC_BIOS_CNTL_PFE)),
      (UINT16) ((B_QNC_LPC_BIOS_CNTL_BCD))
      );
  }
  //
  // Sends the command to the SPI interface to execute.
  //
  Status = SendSpiCmd (
            This,
            OpcodeIndex,
            PrefixOpcodeIndex,
            DataCycle,
            Atomic,
            ShiftOut,
            Address,
            DataByteCount,
            Buffer,
            SpiRegionType
            );

  //
  // Restore BIOS Ctrl register
  //
  PciAndThenOr16 (
    PCI_LIB_ADDRESS (PCI_BUS_NUMBER_QNC,
    PCI_DEVICE_NUMBER_QNC_LPC,
    PCI_FUNCTION_NUMBER_QNC_LPC,
    R_QNC_LPC_BIOS_CNTL),
    (UINT16) (~(B_QNC_LPC_BIOS_CNTL_BCD | B_QNC_LPC_BIOS_CNTL_PFE | B_QNC_LPC_BIOS_CNTL_BIOSWE | B_QNC_LPC_BIOS_CNTL_SMM_BWP)),
    (UINT16) (BiosCtlSave)
      );
  //
  // Restore SMIs.
  //
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, SmiEnSave);

  return Status;
}

VOID
SpiOffset2Physical (
  IN      EFI_SPI_PROTOCOL  *This,
  IN      UINTN             SpiRegionOffset,
  IN      SPI_REGION_TYPE   SpiRegionType,
  OUT     UINTN             *HardwareSpiAddress,
  OUT     UINTN             *BaseAddress,
  OUT     UINTN             *LimitAddress
  )
/*++

Routine Description:

  Convert SPI offset to Physical address of SPI hardware

Arguments:

  This               Pointer to the EFI_SPI_PROTOCOL instance.
  SpiRegionOffset    In Descriptor Mode, for Descriptor Region, GbE Region, ME Region and Platform
                     Region, this value specifies the offset from the Region Base; for BIOS Region,
                     this value specifies the offset from the start of the BIOS Image. In Non
                     Descriptor Mode, this value specifies the offset from the start of the BIOS Image.
                     Please note BIOS Image size may be smaller than BIOS Region size (in Descriptor
                     Mode) or the flash size (in Non Descriptor Mode), and in this case, BIOS Image is
                     supposed to be placed at the top end of the BIOS Region (in Descriptor Mode) or
                     the flash (in Non Descriptor Mode)
  BaseAddress        Base Address of the region.
  SpiRegionType      SPI Region type. Values EnumSpiRegionBios, EnumSpiRegionGbE, EnumSpiRegionMe,
                     EnumSpiRegionDescriptor, and EnumSpiRegionPlatformData are only applicable in
                     Descriptor mode. Value EnumSpiRegionAll is applicable to both Descriptor Mode
                     and Non Descriptor Mode, which indicates "SpiRegionOffset" is actually relative
                     to base of the 1st flash device (i.e., it is a Flash Linear Address).
  HardwareSpiAddress Return absolution SPI address (i.e., Flash Linear Address)
  BaseAddress        Return base address of the region type
  LimitAddress       Return limit address of the region type

Returns:

  EFI_SUCCESS             Command succeed.

--*/
{
  SPI_INSTANCE  *SpiInstance;

  SpiInstance       = SPI_INSTANCE_FROM_SPIPROTOCOL (This);

  if (SpiRegionType == EnumSpiRegionAll) {
    //
    // EnumSpiRegionAll indicates address is relative to flash device (i.e., address is Flash
    // Linear Address)
    //
    *HardwareSpiAddress = SpiRegionOffset;
  } else {
    //
    // Otherwise address is relative to BIOS image
    //
    *HardwareSpiAddress = SpiRegionOffset + SpiInstance->SpiInitTable.BiosStartOffset;
  }
}

EFI_STATUS
SendSpiCmd (
  IN     EFI_SPI_PROTOCOL   *This,
  IN     UINT8              OpcodeIndex,
  IN     UINT8              PrefixOpcodeIndex,
  IN     BOOLEAN            DataCycle,
  IN     BOOLEAN            Atomic,
  IN     BOOLEAN            ShiftOut,
  IN     UINTN              Address,
  IN     UINT32             DataByteCount,
  IN OUT UINT8              *Buffer,
  IN     SPI_REGION_TYPE    SpiRegionType
  )
/*++

Routine Description:

  This function sends the programmed SPI command to the slave device.

Arguments:

  OpcodeIndex       Index of the command in the OpCode Menu.
  PrefixOpcodeIndex Index of the first command to run when in an atomic cycle sequence.
  DataCycle         TRUE if the SPI cycle contains data
  Atomic            TRUE if the SPI cycle is atomic and interleave cycles are not allowed.
  ShiftOut          If DataByteCount is not zero, TRUE to shift data out and FALSE to shift data in.
  Address           In Descriptor Mode, for Descriptor Region, GbE Region, ME Region and Platform
                    Region, this value specifies the offset from the Region Base; for BIOS Region,
                    this value specifies the offset from the start of the BIOS Image. In Non
                    Descriptor Mode, this value specifies the offset from the start of the BIOS Image.
                    Please note BIOS Image size may be smaller than BIOS Region size (in Descriptor
                    Mode) or the flash size (in Non Descriptor Mode), and in this case, BIOS Image is
                    supposed to be placed at the top end of the BIOS Region (in Descriptor Mode) or
                    the flash (in Non Descriptor Mode)
  DataByteCount     Number of bytes in the data portion of the SPI cycle. This function may break the
                    data transfer into multiple operations. This function ensures each operation does
                    not cross 256 byte flash address boundary.
                    *NOTE: if there is some SPI chip that has a stricter address boundary requirement
                    (e.g., its write page size is < 256 byte), then the caller cannot rely on this
                    function to cut the data transfer at proper address boundaries, and it's the
                    caller's reponsibility to pass in a properly cut DataByteCount parameter.
  Buffer            Data received or sent during the SPI cycle.
  SpiRegionType     SPI Region type. Values EnumSpiRegionBios, EnumSpiRegionGbE, EnumSpiRegionMe,
                    EnumSpiRegionDescriptor, and EnumSpiRegionPlatformData are only applicable in
                    Descriptor mode. Value EnumSpiRegionAll is applicable to both Descriptor Mode
                    and Non Descriptor Mode, which indicates "SpiRegionOffset" is actually relative
                    to base of the 1st flash device (i.e., it is a Flash Linear Address).

Returns:

  EFI_SUCCESS             SPI command completes successfully.
  EFI_DEVICE_ERROR        Device error, the command aborts abnormally.
  EFI_ACCESS_DENIED       Some unrecognized command encountered in hardware sequencing mode
  EFI_INVALID_PARAMETER   The parameters specified are not valid.

--*/
{
  UINT32        Index;
  SPI_INSTANCE  *SpiInstance;
  UINTN         HardwareSpiAddr;
  UINTN         SpiBiosSize;
  UINTN         BaseAddress;
  UINTN         LimitAddress;
  UINT32        SpiDataCount;
  UINT8         OpCode;
  UINTN         PchRootComplexBar;

  SpiInstance       = SPI_INSTANCE_FROM_SPIPROTOCOL (This);
  PchRootComplexBar = SpiInstance->PchRootComplexBar;
  SpiBiosSize       = SpiInstance->SpiInitTable.BiosSize;
  OpCode            = MmioRead8 (PchRootComplexBar + R_QNC_RCRB_SPIOPMENU + OpcodeIndex);

  //
  // Check if the value of opcode register is 0 or the BIOS Size of SpiInitTable is 0
  //
  if (OpCode == 0 || SpiBiosSize == 0) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  SpiOffset2Physical (This, Address, SpiRegionType, &HardwareSpiAddr, &BaseAddress, &LimitAddress);
  //
  // Have direct access to BIOS region in Descriptor mode,
  //
  if (SpiInstance->SpiInitTable.OpcodeMenu[OpcodeIndex].Type == EnumSpiOpcodeRead &&
      SpiRegionType == EnumSpiRegionBios) {
    CopyMem (
      Buffer,
      (UINT8 *) ((HardwareSpiAddr - BaseAddress) + (UINT32) (~(SpiBiosSize - 1))),
      DataByteCount
      );
    return EFI_SUCCESS;
  }
  //
  // DEBUG((EFI_D_ERROR, "SPIADDR %x, %x, %x, %x\n", Address, HardwareSpiAddr, BaseAddress,
  // LimitAddress));
  //
  if ((DataCycle == FALSE) && (DataByteCount > 0)) {
    DataByteCount = 0;
  }

  do {
    //
    // Trim at 256 byte boundary per operation,
    // - PCH SPI controller requires trimming at 4KB boundary
    // - Some SPI chips require trimming at 256 byte boundary for write operation
    // - Trimming has limited performance impact as we can read / write atmost 64 byte
    //   per operation
    //
    if (HardwareSpiAddr + DataByteCount > ((HardwareSpiAddr + BIT8) &~(BIT8 - 1))) {
      SpiDataCount = (((UINT32) (HardwareSpiAddr) + BIT8) &~(BIT8 - 1)) - (UINT32) (HardwareSpiAddr);
    } else {
      SpiDataCount = DataByteCount;
    }
    //
    // Calculate the number of bytes to shift in/out during the SPI data cycle.
    // Valid settings for the number of bytes duing each data portion of the
    // PCH SPI cycles are: 0, 1, 2, 3, 4, 5, 6, 7, 8, 16, 24, 32, 40, 48, 56, 64
    //
    if (SpiDataCount >= 64) {
      SpiDataCount = 64;
    } else if ((SpiDataCount &~0x07) != 0) {
      SpiDataCount = SpiDataCount &~0x07;
    }
    //
    // If shifts data out, load data into the SPI data buffer.
    //
    if (ShiftOut) {
      for (Index = 0; Index < SpiDataCount; Index++) {
        MmioWrite8 (PchRootComplexBar + R_QNC_RCRB_SPID0 + Index, Buffer[Index]);
        MmioRead8 (PchRootComplexBar + R_QNC_RCRB_SPID0 + Index);
      }
    }

    MmioWrite32 (
      (PchRootComplexBar + R_QNC_RCRB_SPIA),
      (UINT32) (HardwareSpiAddr & B_QNC_RCRB_SPIA_MASK)
      );
    MmioRead32 (PchRootComplexBar + R_QNC_RCRB_SPIA);

    //
    // Execute the command on the SPI compatible mode
    //

    //
    // Clear error flags
    //
    MmioOr16 ((PchRootComplexBar + R_QNC_RCRB_SPIS), B_QNC_RCRB_SPIS_BAS);

    //
    // Initialte the SPI cycle
    //
    if (DataCycle) {
      MmioWrite16 (
        (PchRootComplexBar + R_QNC_RCRB_SPIC),
        ( (UINT16) (B_QNC_RCRB_SPIC_DC) | (UINT16) (((SpiDataCount - 1) << 8) & B_QNC_RCRB_SPIC_DBC) |
          (UINT16) ((OpcodeIndex << 4) & B_QNC_RCRB_SPIC_COP) |
          (UINT16) ((PrefixOpcodeIndex << 3) & B_QNC_RCRB_SPIC_SPOP) |
          (UINT16) (Atomic ? B_QNC_RCRB_SPIC_ACS : 0) |
          (UINT16) (B_QNC_RCRB_SPIC_SCGO)));
    } else {
      MmioWrite16 (
        (PchRootComplexBar + R_QNC_RCRB_SPIC),
        ( (UINT16) ((OpcodeIndex << 4) & B_QNC_RCRB_SPIC_COP) |
          (UINT16) ((PrefixOpcodeIndex << 3) & B_QNC_RCRB_SPIC_SPOP) |
          (UINT16) (Atomic ? B_QNC_RCRB_SPIC_ACS : 0) |
          (UINT16) (B_QNC_RCRB_SPIC_SCGO)));
    }

    MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIC);

    //
    // end of command execution
    //
    // Wait the SPI cycle to complete.
    //
    if (!WaitForSpiCycleComplete (This, TRUE)) {
      return EFI_DEVICE_ERROR;
    }
    //
    // If shifts data in, get data from the SPI data buffer.
    //
    if (!ShiftOut) {
      for (Index = 0; Index < SpiDataCount; Index++) {
        Buffer[Index] = MmioRead8 (PchRootComplexBar + R_QNC_RCRB_SPID0 + Index);
      }
    }

    HardwareSpiAddr += SpiDataCount;
    Buffer += SpiDataCount;
    DataByteCount -= SpiDataCount;
  } while (DataByteCount > 0);

  return EFI_SUCCESS;
}

BOOLEAN
WaitForSpiCycleComplete (
  IN     EFI_SPI_PROTOCOL   *This,
  IN     BOOLEAN            ErrorCheck
  )
/*++

Routine Description:

  Wait execution cycle to complete on the SPI interface. Check both Hardware
  and Software Sequencing status registers

Arguments:

  This                - The SPI protocol instance
  UseSoftwareSequence - TRUE if this is a Hardware Sequencing operation
  ErrorCheck          - TRUE if the SpiCycle needs to do the error check

Returns:

  TRUE       SPI cycle completed on the interface.
  FALSE      Time out while waiting the SPI cycle to complete.
             It's not safe to program the next command on the SPI interface.

--*/
{
  UINT64        WaitTicks;
  UINT64        WaitCount;
  UINT16        Data16;
  SPI_INSTANCE  *SpiInstance;
  UINTN         PchRootComplexBar;

  SpiInstance       = SPI_INSTANCE_FROM_SPIPROTOCOL (This);
  PchRootComplexBar = SpiInstance->PchRootComplexBar;

  //
  // Convert the wait period allowed into to tick count
  //
  WaitCount = WAIT_TIME / WAIT_PERIOD;

  //
  // Wait for the SPI cycle to complete.
  //
  for (WaitTicks = 0; WaitTicks < WaitCount; WaitTicks++) {
    Data16 = MmioRead16 (PchRootComplexBar + R_QNC_RCRB_SPIS);
    if ((Data16 & B_QNC_RCRB_SPIS_SCIP) == 0) {
      MmioWrite16 (PchRootComplexBar + R_QNC_RCRB_SPIS, (B_QNC_RCRB_SPIS_BAS | B_QNC_RCRB_SPIS_CDS));
      if ((Data16 & B_QNC_RCRB_SPIS_BAS) && (ErrorCheck == TRUE)) {
        return FALSE;
      } else {
        return TRUE;
      }
    }

    MicroSecondDelay (WAIT_PERIOD);
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
SpiProtocolInfo (
  IN EFI_SPI_PROTOCOL     *This,
  OUT SPI_INIT_INFO      **InitInfoPtr
  )
/*++

Routine Description:

  Return info about SPI host controller, to help callers usage of Execute
  service.

  If 0xff is returned as an opcode index in init info struct
  then device does not support the operation.

Arguments:

  This                    Pointer to the EFI_SPI_PROTOCOL instance.
  InitInfoPtr             Pointer to init info written to this memory location.

Returns:

  EFI_SUCCESS             Information returned.
  EFI_INVALID_PARAMETER   Invalid parameter.
  EFI_NOT_READY           Required resources not setup.
  Others                  Unexpected error happened.

--*/
{
  SPI_INSTANCE  *SpiInstance;

  if (This == NULL || InitInfoPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SpiInstance       = SPI_INSTANCE_FROM_SPIPROTOCOL (This);
  if (SpiInstance->Signature != PCH_SPI_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (!SpiInstance->InitDone) {
    *InitInfoPtr = NULL;
    return EFI_NOT_READY;
  }
  *InitInfoPtr = &SpiInstance->InitInfo;
  return EFI_SUCCESS;
}

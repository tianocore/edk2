/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  Spi.h

  @brief
  This file defines the EFI SPI Protocol which implements the
  Intel(R) ICH SPI Host Controller Compatibility Interface.

**/
#ifndef _EFI_SPI_H_
#define _EFI_SPI_H_


//
#define EFI_SPI_PROTOCOL_GUID \
  { \
    0x1156efc6, 0xea32, 0x4396, 0xb5, 0xd5, 0x26, 0x93, 0x2e, 0x83, 0xc3, 0x13 \
  }
#define EFI_SMM_SPI_PROTOCOL_GUID \
  { \
    0xD9072C35, 0xEB8F, 0x43ad, 0xA2, 0x20, 0x34, 0xD4, 0x0E, 0x2A, 0x82, 0x85 \
  }
extern EFI_GUID                   gEfiSpiProtocolGuid;
extern EFI_GUID                   gEfiSmmSpiProtocolGuid;

///
/// Forward reference for ANSI C compatibility
///
typedef struct _EFI_SPI_PROTOCOL  EFI_SPI_PROTOCOL;

///
/// SPI protocol data structures and definitions
///
///
/// Number of Prefix Opcodes allowed on the SPI interface
///
#define SPI_NUM_PREFIX_OPCODE 2

///
/// Number of Opcodes in the Opcode Menu
///
#define SPI_NUM_OPCODE  8

///
/// Opcode Type
///    EnumSpiOpcodeCommand: Command without address
///    EnumSpiOpcodeRead: Read with address
///    EnumSpiOpcodeWrite: Write with address
///
typedef enum {
  EnumSpiOpcodeReadNoAddr,
  EnumSpiOpcodeWriteNoAddr,
  EnumSpiOpcodeRead,
  EnumSpiOpcodeWrite,
  EnumSpiOpcodeMax
} SPI_OPCODE_TYPE;

typedef enum {
  EnumSpiCycle20MHz,
  EnumSpiCycle33MHz,
  EnumSpiCycle66MHz,  /// Not supported by VLV
  EnumSpiCycle50MHz,
  EnumSpiCycleMax
} SPI_CYCLE_FREQUENCY;

typedef enum {
  EnumSpiRegionAll,
  EnumSpiRegionBios,
  EnumSpiRegionSeC,
  EnumSpiRegionDescriptor,
  EnumSpiRegionPlatformData,
  EnumSpiRegionMax
} SPI_REGION_TYPE;

///
/// Hardware Sequencing required operations (as listed in Valleyview EDS "Hardware
/// Sequencing Commands and Opcode Requirements"
///
typedef enum {
  EnumSpiOperationWriteStatus,
  EnumSpiOperationProgramData_1_Byte,
  EnumSpiOperationProgramData_64_Byte,
  EnumSpiOperationReadData,
  EnumSpiOperationWriteDisable,
  EnumSpiOperationReadStatus,
  EnumSpiOperationWriteEnable,
  EnumSpiOperationFastRead,
  EnumSpiOperationEnableWriteStatus,
  EnumSpiOperationErase_256_Byte,
  EnumSpiOperationErase_4K_Byte,
  EnumSpiOperationErase_8K_Byte,
  EnumSpiOperationErase_64K_Byte,
  EnumSpiOperationFullChipErase,
  EnumSpiOperationJedecId,
  EnumSpiOperationDualOutputFastRead,
  EnumSpiOperationDiscoveryParameters,
  EnumSpiOperationOther,
  EnumSpiOperationMax
} SPI_OPERATION;

///
/// SPI Command Configuration
///   Frequency       The expected frequency to be used (value to be programmed to the SSFC
///                   Register)
///   Operation       Which Hardware Sequencing required operation this opcode respoinds to.
///                   The required operations are listed in EDS Table 5-55: "Hardware
///                   Sequencing Commands and Opcode Requirements"
///                   If the opcode does not corresponds to any operation listed, use
///                   EnumSpiOperationOther, and provides TYPE and Code for it in
///                   SpecialOpcodeEntry.
///
typedef struct _SPI_OPCODE_MENU_ENTRY {
  SPI_OPCODE_TYPE     Type;
  UINT8               Code;
  SPI_CYCLE_FREQUENCY Frequency;
  SPI_OPERATION       Operation;
} SPI_OPCODE_MENU_ENTRY;

//
// Initialization data table loaded to the SPI host controller
//    VendorId        Vendor ID of the SPI device
//    DeviceId0       Device ID0 of the SPI device
//    DeviceId1       Device ID1 of the SPI device
//    PrefixOpcode    Prefix opcodes which are loaded into the SPI host controller
//    OpcodeMenu      Opcodes which are loaded into the SPI host controller Opcode Menu
//    BiosStartOffset The offset of the start of the BIOS image relative to the flash device.
//                    Please note this is a Flash Linear Address, NOT a memory space address.
//                    This value is platform specific and depends on the system flash map.
//                    This value is only used on non Descriptor mode.
//    BiosSize        The the BIOS Image size in flash. This value is platform specific
//                    and depends on the system flash map. Please note BIOS Image size may
//                    be smaller than BIOS Region size (in Descriptor Mode) or the flash size
//                    (in Non Descriptor Mode), and in this case, BIOS Image is supposed to be
//                    placed at the top end of the BIOS Region (in Descriptor Mode) or the flash
//                    (in Non Descriptor Mode)
//
typedef struct _SPI_INIT_TABLE {
  UINT8                 VendorId;
  UINT8                 DeviceId0;
  UINT8                 DeviceId1;
  UINT8                 PrefixOpcode[SPI_NUM_PREFIX_OPCODE];
  SPI_OPCODE_MENU_ENTRY OpcodeMenu[SPI_NUM_OPCODE];
  UINTN                 BiosStartOffset;
  UINTN                 BiosSize;
} SPI_INIT_TABLE;

//
// Protocol member functions
//
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_INIT) (
  IN EFI_SPI_PROTOCOL     * This,
  IN SPI_INIT_TABLE       * InitTable
  );

/**

  @brief
  Initializes the host controller to execute SPI commands.

  @param[in] This                 Pointer to the EFI_SPI_PROTOCOL instance.
  @param[in] InitData             Pointer to caller-allocated buffer containing the SPI
                                  interface initialization table.

  @retval EFI_SUCCESS             Opcode initialization on the SPI host controller completed.
  @retval EFI_ACCESS_DENIED       The SPI configuration interface is locked.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource available to initialize the device.
  @retval EFI_DEVICE_ERROR        Device error, operation failed.

**/

typedef
EFI_STATUS
(EFIAPI *EFI_SPI_LOCK) (
  IN EFI_SPI_PROTOCOL     * This
  );
/**

  @brief
  Initializes the host controller to execute SPI commands.

  @param[in] This                 Pointer to the EFI_SPI_PROTOCOL instance.
  @param[in] InitData             Pointer to caller-allocated buffer containing the SPI
                                  interface initialization table.

  @retval EFI_SUCCESS             Opcode initialization on the SPI host controller completed.
  @retval EFI_ACCESS_DENIED       The SPI configuration interface is locked.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource available to initialize the device.
  @retval EFI_DEVICE_ERROR        Device error, operation failed.

**/

typedef
EFI_STATUS
(EFIAPI *EFI_SPI_EXECUTE) (
  IN     EFI_SPI_PROTOCOL   * This,
  IN     UINT8              OpcodeIndex,
  IN     UINT8              PrefixOpcodeIndex,
  IN     BOOLEAN            DataCycle,
  IN     BOOLEAN            Atomic,
  IN     BOOLEAN            ShiftOut,
  IN     UINTN              Address,
  IN     UINT32             DataByteCount,
  IN OUT UINT8              *Buffer,
  IN     SPI_REGION_TYPE    SpiRegionType
  );
/**

  @brief
  Execute SPI commands from the host controller.

  @param[in] This                 Pointer to the EFI_SPI_PROTOCOL instance.
  @param[in] OpcodeIndex          Index of the command in the OpCode Menu.
  @param[in] PrefixOpcodeIndex    Index of the first command to run when in an atomic cycle sequence.
  @param[in] DataCycle            TRUE if the SPI cycle contains data
  @param[in] Atomic               TRUE if the SPI cycle is atomic and interleave cycles are not allowed.
  @param[in] ShiftOut             If DataByteCount is not zero, TRUE to shift data out and FALSE to shift data in.
  @param[in] Address              In Descriptor Mode, for Descriptor Region, GbE Region, ME Region and Platform
                                  Region, this value specifies the offset from the Region Base; for BIOS Region,
                                  this value specifies the offset from the start of the BIOS Image. In Non
                                  Descriptor Mode, this value specifies the offset from the start of the BIOS Image.
                                  Please note BIOS Image size may be smaller than BIOS Region size (in Descriptor
                                  Mode) or the flash size (in Non Descriptor Mode), and in this case, BIOS Image is
                                  supposed to be placed at the top end of the BIOS Region (in Descriptor Mode) or
                                  the flash (in Non Descriptor Mode)
  @param[in] DataByteCount        Number of bytes in the data portion of the SPI cycle.
  @param[in] Buffer               Pointer to caller-allocated buffer containing the dada received or sent during the SPI cycle.
  @param[in] SpiRegionType        SPI Region type. Values EnumSpiRegionBios, EnumSpiRegionGbE, EnumSpiRegionMe,
                                  EnumSpiRegionDescriptor, and EnumSpiRegionPlatformData are only applicable in
                                  Descriptor mode. Value EnumSpiRegionAll is applicable to both Descriptor Mode
                                  and Non Descriptor Mode, which indicates "SpiRegionOffset" is actually relative
                                  to base of the 1st flash device (i.e., it is a Flash Linear Address).

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @exception EFI_UNSUPPORTED      Command not supported.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.

**/

///
/// Protocol definition
///
struct _EFI_SPI_PROTOCOL {
  EFI_SPI_INIT    Init;
  EFI_SPI_LOCK    Lock;
  EFI_SPI_EXECUTE Execute;
};

#endif

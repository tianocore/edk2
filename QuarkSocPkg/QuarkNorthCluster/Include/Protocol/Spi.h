/** @file
This file defines the EFI SPI Protocol which implements the
Intel(R) ICH SPI Host Controller Compatibility Interface.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#ifndef _SPI_H_
#define _SPI_H_

//
// Define the SPI protocol GUID
//
// EDK and EDKII have different GUID formats
//
#if !defined(EDK_RELEASE_VERSION) || (EDK_RELEASE_VERSION < 0x00020000)
#define EFI_SPI_PROTOCOL_GUID \
  { \
    0x1156efc6, 0xea32, 0x4396, 0xb5, 0xd5, 0x26, 0x93, 0x2e, 0x83, 0xc3, 0x13 \
  }
#define EFI_SMM_SPI_PROTOCOL_GUID \
  { \
    0xD9072C35, 0xEB8F, 0x43ad, 0xA2, 0x20, 0x34, 0xD4, 0x0E, 0x2A, 0x82, 0x85 \
  }
#else
#define EFI_SPI_PROTOCOL_GUID \
  { \
    0x1156efc6, 0xea32, 0x4396, \
    { \
      0xb5, 0xd5, 0x26, 0x93, 0x2e, 0x83, 0xc3, 0x13 \
    } \
  }
#define EFI_SMM_SPI_PROTOCOL_GUID \
  { \
    0xD9072C35, 0xEB8F, 0x43ad, \
    { \
      0xA2, 0x20, 0x34, 0xD4, 0x0E, 0x2A, 0x82, 0x85 \
    } \
  }
#endif
//
// Extern the GUID for protocol users.
//
extern EFI_GUID                   gEfiSpiProtocolGuid;
extern EFI_GUID                   gEfiSmmSpiProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_SPI_PROTOCOL  EFI_SPI_PROTOCOL;

//
// SPI protocol data structures and definitions
//
//
// Number of Prefix Opcodes allowed on the SPI interface
//
#define SPI_NUM_PREFIX_OPCODE 2

//
// Number of Opcodes in the Opcode Menu
//
#define SPI_NUM_OPCODE  8

#ifdef SERVER_BIOS_FLAG
//
// SPI default opcode slots
//
#define SPI_OPCODE_JEDEC_ID_INDEX        0
#endif // SERVER_BIOS_FLAG

//
// Opcode Type
//    EnumSpiOpcodeCommand: Command without address
//    EnumSpiOpcodeRead: Read with address
//    EnumSpiOpcodeWrite: Write with address
//
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
  EnumSpiCycle66MHz,  // not supported by PCH
  EnumSpiCycle50MHz,
  EnumSpiCycleMax
} SPI_CYCLE_FREQUENCY;

typedef enum {
  EnumSpiRegionAll,
  EnumSpiRegionBios,
  EnumSpiRegionMe,
  EnumSpiRegionGbE,
  EnumSpiRegionDescriptor,
  EnumSpiRegionPlatformData,
  EnumSpiRegionMax
} SPI_REGION_TYPE;

//
// Hardware Sequencing required operations (as listed in CougarPoint EDS Table 5-55: "Hardware
// Sequencing Commands and Opcode Requirements"
//
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

//
// Opcode menu entries
//   Type            Operation Type (value to be programmed to the OPTYPE register)
//   Code            The opcode (value to be programmed to the OPMENU register)
//   Frequency       The expected frequency to be used (value to be programmed to the SSFC
//                   Register)
//   Operation       Which Hardware Sequencing required operation this opcode respoinds to.
//                   The required operations are listed in EDS Table 5-55: "Hardware
//                   Sequencing Commands and Opcode Requirements"
//                   If the opcode does not corresponds to any operation listed, use
//                   EnumSpiOperationOther
//
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
// Public Info struct to show current initialized state of the spi interface.
// OpcodeIndex must be less then SPI_NUM_OPCODE for operation to be supported.
//
typedef struct _SPI_INIT_INFO {
  SPI_INIT_TABLE        *InitTable;
  UINT8                 JedecIdOpcodeIndex;
  UINT8                 OtherOpcodeIndex;
  UINT8                 WriteStatusOpcodeIndex;
  UINT8                 ProgramOpcodeIndex;
  UINT8                 ReadOpcodeIndex;
  UINT8                 EraseOpcodeIndex;
  UINT8                 ReadStatusOpcodeIndex;
  UINT8                 FullChipEraseOpcodeIndex;
} SPI_INIT_INFO;

//
// Protocol member functions
//

typedef
EFI_STATUS
(EFIAPI *EFI_SPI_INIT) (
  IN EFI_SPI_PROTOCOL     * This,
  IN SPI_INIT_TABLE       * InitTable
  );
/*++

Routine Description:

  Initializes the host controller to execute SPI commands.

Arguments:

  This                    Pointer to the EFI_SPI_PROTOCOL instance.
  InitTable               Pointer to caller-allocated buffer containing the SPI
                          interface initialization table.

Returns:

  EFI_SUCCESS             Opcode initialization on the SPI host controller completed.
  EFI_ACCESS_DENIED       The SPI configuration interface is locked.
  EFI_OUT_OF_RESOURCES    Not enough resource available to initialize the device.
  EFI_DEVICE_ERROR        Device error, operation failed.

--*/

typedef
EFI_STATUS
(EFIAPI *EFI_SPI_LOCK) (
  IN EFI_SPI_PROTOCOL     * This
  );
/*++

Routine Description:

  Lock the SPI Static Configuration Interface.
  Once locked, the interface is no longer open for configuration changes.
  The lock state automatically clears on next system reset.

Arguments:

  This      Pointer to the EFI_SPI_PROTOCOL instance.

Returns:

  EFI_SUCCESS             Lock operation succeed.
  EFI_DEVICE_ERROR        Device error, operation failed.
  EFI_ACCESS_DENIED       The interface has already been locked.

--*/

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
/*++

Routine Description:

  Execute SPI commands from the host controller.

Arguments:

  This                    Pointer to the EFI_SPI_PROTOCOL instance.
  OpcodeIndex             Index of the command in the OpCode Menu.
  PrefixOpcodeIndex       Index of the first command to run when in an atomic cycle sequence.
  DataCycle               TRUE if the SPI cycle contains data
  Atomic                  TRUE if the SPI cycle is atomic and interleave cycles are not allowed.
  ShiftOut                If DataByteCount is not zero, TRUE to shift data out and FALSE to shift data in.
  Address                 In Descriptor Mode, for Descriptor Region, GbE Region, ME Region and Platform
                          Region, this value specifies the offset from the Region Base; for BIOS Region,
                          this value specifies the offset from the start of the BIOS Image. In Non
                          Descriptor Mode, this value specifies the offset from the start of the BIOS Image.
                          Please note BIOS Image size may be smaller than BIOS Region size (in Descriptor
                          Mode) or the flash size (in Non Descriptor Mode), and in this case, BIOS Image is
                          supposed to be placed at the top end of the BIOS Region (in Descriptor Mode) or
                          the flash (in Non Descriptor Mode)
  DataByteCount           Number of bytes in the data portion of the SPI cycle.
  Buffer                  Pointer to caller-allocated buffer containing the dada received or sent during the SPI cycle.
  SpiRegionType           SPI Region type. Values EnumSpiRegionBios, EnumSpiRegionGbE, EnumSpiRegionMe,
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

typedef
EFI_STATUS
(EFIAPI *EFI_SPI_INFO) (
  IN EFI_SPI_PROTOCOL     *This,
  OUT SPI_INIT_INFO      **InitInfoPtr
  );
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

//
// Protocol definition
//
struct _EFI_SPI_PROTOCOL {
  EFI_SPI_INIT    Init;
  EFI_SPI_LOCK    Lock;
  EFI_SPI_EXECUTE Execute;
  EFI_SPI_INFO    Info;
};

#endif

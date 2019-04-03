/** @file
Header file for the PCH SPI Common Driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _SPI_COMMON_H_
#define _SPI_COMMON_H_

#include "Protocol/Spi.h"
#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>
#include <Uefi/UefiBaseType.h>

//
// Maximum time allowed while waiting the SPI cycle to complete
//  Wait Time = 6 seconds = 6000000 microseconds
//  Wait Period = 10 microseconds
//
#define WAIT_TIME   6000000
#define WAIT_PERIOD 10
//
// PCH Required SPI Commands -------- COMMAND SET I ------------
// SPI flash device must support in order to be compatible with PCH
//
#define PCH_SPI_COMMAND_PROGRAM_BYTE          0x02
#define PCH_SPI_COMMAND_READ_DATA             0x03
#define PCH_SPI_COMMAND_WRITE_DISABLE         0x04
#define PCH_SPI_COMMAND_READ_STATUS           0x05
#define PCH_SPI_COMMAND_WRITE_ENABLE          0x06
#define PCH_SPI_COMMAND_FAST_READ             0x0B
#define PCH_SPI_COMMAND_READ_ID               0x9F
#define PCH_SPI_COMMAND_DUAL_FAST_READ        0x3B  // Dual Output Fast Read

//
// Need to support at least one of the following two kinds of size of sector for erasing
//
#define PCH_SPI_COMMAND_4KB_ERASE   0x20
#define PCH_SPI_COMMAND_64KB_ERASE  0xD8
//
// Recommended SPI Commands -------- COMMAND SET II ------------
// SPI flash device best to support
//
#define PCH_SPI_COMMAND_WRITE_STATUS    0x01
#define PCH_SPI_COMMAND_FULL_CHIP_ERASE 0xC7

//
// Private data structure definitions for the driver
//
#define PCH_SPI_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('P', 'S', 'P', 'I')

typedef struct {
  UINTN             Signature;
  EFI_HANDLE        Handle;
  EFI_SPI_PROTOCOL  SpiProtocol;
  SPI_INIT_TABLE    SpiInitTable;
  UINTN             PchRootComplexBar;
  BOOLEAN           InitDone; // Set to TRUE on SpiProtocolInit SUCCESS.
  SPI_INIT_INFO     InitInfo;
} SPI_INSTANCE;

#define SPI_INSTANCE_FROM_SPIPROTOCOL(a)  CR (a, SPI_INSTANCE, SpiProtocol, PCH_SPI_PRIVATE_DATA_SIGNATURE)

//
// Function prototypes used by the SPI protocol.
//
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
;

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
--*/
;

EFI_STATUS
EFIAPI
SpiProtocolLock (
  IN EFI_SPI_PROTOCOL       *This
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
;

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
;

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
;

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
;

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
;

#endif

/** @file
  This library is only intended to be used by TPM modules.
  It provides basic TPM Interface Specification (TIS) and Command functions.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TPM_COMM_LIB_H_
#define _TPM_COMM_LIB_H_

#include <IndustryStandard/Tpm12.h>

typedef EFI_HANDLE TIS_TPM_HANDLE;

///
/// TPM register base address.
///
#define TPM_BASE_ADDRESS  0xfed40000

//
// Set structure alignment to 1-byte
//
#pragma pack (1)

//
// Register set map as specified in TIS specification Chapter 10
//
typedef struct {
  ///
  /// Used to gain ownership for this particular port.
  ///
  UINT8     Access;                                     // 0
  UINT8     Reserved1[7];                               // 1
  ///
  /// Controls interrupts.
  ///
  UINT32    IntEnable;                                  // 8
  ///
  /// SIRQ vector to be used by the TPM.
  ///
  UINT8     IntVector;                                  // 0ch
  UINT8     Reserved2[3];                               // 0dh
  ///
  /// What caused interrupt.
  ///
  UINT32    IntSts;                                     // 10h
  ///
  /// Shows which interrupts are supported by that particular TPM.
  ///
  UINT32    IntfCapability;                             // 14h
  ///
  /// Status Register. Provides status of the TPM.
  ///
  UINT8     Status;                                     // 18h
  ///
  /// Number of consecutive writes that can be done to the TPM.
  ///
  UINT16    BurstCount;                                 // 19h
  UINT8     Reserved3[9];
  ///
  /// Read or write FIFO, depending on transaction.
  ///
  UINT32    DataFifo;                                   // 24
  UINT8     Reserved4[0xed8];                           // 28h
  ///
  /// Vendor ID
  ///
  UINT16    Vid;                                        // 0f00h
  ///
  /// Device ID
  ///
  UINT16    Did;                                        // 0f02h
  ///
  /// Revision ID
  ///
  UINT8     Rid;                                        // 0f04h
  ///
  /// TCG defined configuration registers.
  ///
  UINT8     TcgDefined[0x7b];                           // 0f05h
  ///
  /// Alias to I/O legacy space.
  ///
  UINT32    LegacyAddress1;                             // 0f80h
  ///
  /// Additional 8 bits for I/O legacy space extension.
  ///
  UINT32    LegacyAddress1Ex;                           // 0f84h
  ///
  /// Alias to second I/O legacy space.
  ///
  UINT32    LegacyAddress2;                             // 0f88h
  ///
  /// Additional 8 bits for second I/O legacy space extension.
  ///
  UINT32    LegacyAddress2Ex;                           // 0f8ch
  ///
  /// Vendor-defined configuration registers.
  ///
  UINT8     VendorDefined[0x70];                        // 0f90h
} TIS_PC_REGISTERS;

//
// Restore original structure alignment
//
#pragma pack ()

//
// Define pointer types used to access TIS registers on PC
//
typedef TIS_PC_REGISTERS *TIS_PC_REGISTERS_PTR;

//
// TCG Platform Type based on TCG ACPI Specification Version 1.00
//
#define TCG_PLATFORM_TYPE_CLIENT  0
#define TCG_PLATFORM_TYPE_SERVER  1

//
// Define bits of ACCESS and STATUS registers
//

///
/// This bit is a 1 to indicate that the other bits in this register are valid.
///
#define TIS_PC_VALID  BIT7
///
/// Indicate that this locality is active.
///
#define TIS_PC_ACC_ACTIVE  BIT5
///
/// Set to 1 to indicate that this locality had the TPM taken away while
/// this locality had the TIS_PC_ACC_ACTIVE bit set.
///
#define TIS_PC_ACC_SEIZED  BIT4
///
/// Set to 1 to indicate that TPM MUST reset the
/// TIS_PC_ACC_ACTIVE bit and remove ownership for localities less than the
/// locality that is writing this bit.
///
#define TIS_PC_ACC_SEIZE  BIT3
///
/// When this bit is 1, another locality is requesting usage of the TPM.
///
#define TIS_PC_ACC_PENDIND  BIT2
///
/// Set to 1 to indicate that this locality is requesting to use TPM.
///
#define TIS_PC_ACC_RQUUSE  BIT1
///
/// A value of 1 indicates that a T/OS has not been established on the platform
///
#define TIS_PC_ACC_ESTABLISH  BIT0

///
/// When this bit is 1, TPM is in the Ready state,
/// indicating it is ready to receive a new command.
///
#define TIS_PC_STS_READY  BIT6
///
/// Write a 1 to this bit to cause the TPM to execute that command.
///
#define TIS_PC_STS_GO  BIT5
///
/// This bit indicates that the TPM has data available as a response.
///
#define TIS_PC_STS_DATA  BIT4
///
/// The TPM sets this bit to a value of 1 when it expects another byte of data for a command.
///
#define TIS_PC_STS_EXPECT  BIT3
///
/// Writes a 1 to this bit to force the TPM to re-send the response.
///
#define TIS_PC_STS_RETRY  BIT1

//
// Default TimeOut value
//
#define TIS_TIMEOUT_A  750 * 1000                // 750ms
#define TIS_TIMEOUT_B  2000 * 1000               // 2s
#define TIS_TIMEOUT_C  750 * 1000                // 750ms
#define TIS_TIMEOUT_D  750 * 1000                // 750ms

//
// Max TPM command/response length
//
#define TPMCMDBUFLENGTH  1024

/**
  Check whether the value of a TPM chip register satisfies the input BIT setting.

  @param[in]  Register     Address port of register to be checked.
  @param[in]  BitSet       Check these data bits are set.
  @param[in]  BitClear     Check these data bits are clear.
  @param[in]  TimeOut      The max wait time (unit MicroSecond) when checking register.

  @retval     EFI_SUCCESS  The register satisfies the check bit.
  @retval     EFI_TIMEOUT  The register can't run into the expected status in time.
**/
EFI_STATUS
EFIAPI
TisPcWaitRegisterBits (
  IN UINT8   *Register,
  IN UINT8   BitSet,
  IN UINT8   BitClear,
  IN UINT32  TimeOut
  );

/**
  Get BurstCount by reading the burstCount field of a TIS register
  in the time of default TIS_TIMEOUT_D.

  @param[in]  TisReg                Pointer to TIS register.
  @param[out] BurstCount            Pointer to a buffer to store the got BurstCount.

  @retval     EFI_SUCCESS           Get BurstCount.
  @retval     EFI_INVALID_PARAMETER TisReg is NULL or BurstCount is NULL.
  @retval     EFI_TIMEOUT           BurstCount can't be got in time.
**/
EFI_STATUS
EFIAPI
TisPcReadBurstCount (
  IN  TIS_PC_REGISTERS_PTR  TisReg,
  OUT UINT16                *BurstCount
  );

/**
  Set TPM chip to ready state by sending ready command TIS_PC_STS_READY
  to Status Register in time.

  @param[in] TisReg                Pointer to TIS register.

  @retval    EFI_SUCCESS           TPM chip enters into ready state.
  @retval    EFI_INVALID_PARAMETER TisReg is NULL.
  @retval    EFI_TIMEOUT           TPM chip can't be set to ready state in time.
**/
EFI_STATUS
EFIAPI
TisPcPrepareCommand (
  IN TIS_PC_REGISTERS_PTR  TisReg
  );

/**
  Get the control of TPM chip by sending requestUse command TIS_PC_ACC_RQUUSE
  to ACCESS Register in the time of default TIS_TIMEOUT_D.

  @param[in] TisReg                Pointer to TIS register.

  @retval    EFI_SUCCESS           Get the control of TPM chip.
  @retval    EFI_INVALID_PARAMETER TisReg is NULL.
  @retval    EFI_NOT_FOUND         TPM chip doesn't exit.
  @retval    EFI_TIMEOUT           Can't get the TPM control in time.
**/
EFI_STATUS
EFIAPI
TisPcRequestUseTpm (
  IN TIS_PC_REGISTERS_PTR  TisReg
  );

/**
  Single function calculates SHA1 digest value for all raw data. It
  combines Sha1Init(), Sha1Update() and Sha1Final().

  @param[in]  Data          Raw data to be digested.
  @param[in]  DataLen       Size of the raw data.
  @param[out] Digest        Pointer to a buffer that stores the final digest.

  @retval     EFI_SUCCESS   Always successfully calculate the final digest.
**/
EFI_STATUS
EFIAPI
TpmCommHashAll (
  IN  CONST UINT8       *Data,
  IN        UINTN       DataLen,
  OUT       TPM_DIGEST  *Digest
  );

#endif

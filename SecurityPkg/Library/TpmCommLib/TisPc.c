/** @file
  Basic TIS (TPM Interface Specification) functions.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

/**
  Check whether TPM chip exist.

  @param[in] TisReg  Pointer to TIS register.

  @retval    TRUE    TPM chip exists.
  @retval    FALSE   TPM chip is not found.
**/
BOOLEAN
TisPcPresenceCheck (
  IN      TIS_PC_REGISTERS_PTR  TisReg
  )
{
  UINT8  RegRead;

  RegRead = MmioRead8 ((UINTN)&TisReg->Access);
  return (BOOLEAN)(RegRead != (UINT8)-1);
}

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
  IN      UINT8   *Register,
  IN      UINT8   BitSet,
  IN      UINT8   BitClear,
  IN      UINT32  TimeOut
  )
{
  UINT8   RegRead;
  UINT32  WaitTime;

  for (WaitTime = 0; WaitTime < TimeOut; WaitTime += 30) {
    RegRead = MmioRead8 ((UINTN)Register);
    if (((RegRead & BitSet) == BitSet) && ((RegRead & BitClear) == 0)) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (30);
  }

  return EFI_TIMEOUT;
}

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
  IN      TIS_PC_REGISTERS_PTR  TisReg,
  OUT  UINT16                   *BurstCount
  )
{
  UINT32  WaitTime;
  UINT8   DataByte0;
  UINT8   DataByte1;

  if ((BurstCount == NULL) || (TisReg == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  WaitTime = 0;
  do {
    //
    // TIS_PC_REGISTERS_PTR->burstCount is UINT16, but it is not 2bytes aligned,
    // so it needs to use MmioRead8 to read two times
    //
    DataByte0   = MmioRead8 ((UINTN)&TisReg->BurstCount);
    DataByte1   = MmioRead8 ((UINTN)&TisReg->BurstCount + 1);
    *BurstCount = (UINT16)((DataByte1 << 8) + DataByte0);
    if (*BurstCount != 0) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (30);
    WaitTime += 30;
  } while (WaitTime < TIS_TIMEOUT_D);

  return EFI_TIMEOUT;
}

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
  IN      TIS_PC_REGISTERS_PTR  TisReg
  )
{
  EFI_STATUS  Status;

  if (TisReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MmioWrite8 ((UINTN)&TisReg->Status, TIS_PC_STS_READY);
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             TIS_PC_STS_READY,
             0,
             TIS_TIMEOUT_B
             );
  return Status;
}

/**
  Get the control of TPM chip by sending requestUse command TIS_PC_ACC_RQUUSE
  to ACCESS Register in the time of default TIS_TIMEOUT_A.

  @param[in] TisReg                Pointer to TIS register.

  @retval    EFI_SUCCESS           Get the control of TPM chip.
  @retval    EFI_INVALID_PARAMETER TisReg is NULL.
  @retval    EFI_NOT_FOUND         TPM chip doesn't exit.
  @retval    EFI_TIMEOUT           Can't get the TPM control in time.
**/
EFI_STATUS
EFIAPI
TisPcRequestUseTpm (
  IN      TIS_PC_REGISTERS_PTR  TisReg
  )
{
  EFI_STATUS  Status;

  if (TisReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!TisPcPresenceCheck (TisReg)) {
    return EFI_NOT_FOUND;
  }

  MmioWrite8 ((UINTN)&TisReg->Access, TIS_PC_ACC_RQUUSE);
  //
  // No locality set before, ACCESS_X.activeLocality MUST be valid within TIMEOUT_A
  //
  Status = TisPcWaitRegisterBits (
             &TisReg->Access,
             (UINT8)(TIS_PC_ACC_ACTIVE |TIS_PC_VALID),
             0,
             TIS_TIMEOUT_A
             );
  return Status;
}

/** @file
  Base SMBUS library implementation built upon I/O library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  SmbusLib.h

**/

RETURN_STATUS
EFIAPI
BaseSmBusLibConstructor (
  IN      VOID                      *Param1,
  IN      VOID                      *Param2
  )
{
  return RETURN_SUCCESS;
}

//
// BUGBUG: use PCD to retrieve BUS, DEV, FUNC & OFFSET for SMBUS host BAR
//
#define SMBUS_HOST_BUS              0
#define SMBUS_HOST_DEV              31
#define SMBUS_HOST_FUNC             3
#define SMBUS_HOST_SMB_BASE         0x20

//
// Offsets of registers for SMBUS controller
//
#define R_HST_STS                   0
#define R_HST_CNT                   2
#define R_HST_CMD                   3
#define R_XMIT_SLVA                 4
#define R_HST_D0                    5
#define R_HST_D1                    6
#define R_HOST_BLOCK_DB             7
#define R_PEC                       8
#define R_RCV_SLVA                  9
#define R_SLV_DATA                  0x0a
#define R_AUX_STS                   0x0c
#define R_AUX_CTL                   0x0d
#define R_SMLINK_PIN_CTL            0x0e
#define R_SMBUS_PIN_CTL             0x0f
#define R_SLV_STS                   0x10
#define R_SLV_CMD                   0x11
#define R_NOTIFY_DADDR              0x14
#define R_NOTIFY_DLOW               0x16
#define R_NOTIFY_DHIGH              0x17

//
// Bits in HST_STS
//
#define B_HST_STS_DS                0x80
#define B_HST_STS_INUSE             0x40
#define B_HST_STS_SMBALERT          0x20
#define B_HST_STS_FAILED            0x10
#define B_HST_STS_BUS_ERR           0x08
#define B_HST_STS_DEV_ERR           0x04
#define B_HST_STS_INTR              0x02
#define B_HST_STS_BUSY              0x01
#define B_HST_STS_ERR               ( B_HST_STS_BUS_ERR   | \
                                      B_HST_STS_DEV_ERR   | \
                                      B_HST_STS_FAILED )
#define B_HST_STS_ALL               ( B_HST_STS_DS        | \
                                      B_HST_STS_INUSE     | \
                                      B_HST_STS_SMBALERT  | \
                                      B_HST_STS_ERR       | \
                                      B_HST_STS_INTR )

//
// Bits in HST_CNT
//
#define B_HST_CNT_PEC               0x80
#define B_HST_CNT_START             0x40
#define B_HST_CNT_LAST_BYTE         0x20
#define B_HST_CNT_SMB_CMD           0x1c
#define B_HST_CNT_KILL              0x02
#define B_HST_CNT_INTREN            0x01

//
// SMBUS Protocols
//
#define B_SMB_CMD_QUICK             0
#define B_SMB_CMD_BYTE              1
#define B_SMB_CMD_BYTE_DATA         2
#define B_SMB_CMD_WORD_DATA         3
#define B_SMB_CMD_PROCESS_CALL      4
#define B_SMB_CMD_BLOCK             5
#define B_SMB_CMD_I2C               6
#define B_SMB_CMD_BLOCK_PROCESS     7

//
// Bits in AUX_CTL
//
#define B_AUX_CTL_E32B              0x02
#define B_AUX_CTL_AAC               0x01

//
// SMBUS Rd/Wr control
//
#define B_SMBUS_READ                1
#define B_SMBUS_WRITE               0

static
UINT16
EFIAPI
GetSmBusIOBaseAddress (
  VOID
  )
{
  UINT32                            SmbusBar;

  SmbusBar = PciRead32 (
               PCI_LIB_ADDRESS (
                 SMBUS_HOST_BUS,
                 SMBUS_HOST_DEV,
                 SMBUS_HOST_FUNC,
                 SMBUS_HOST_SMB_BASE
                 )
               );
  ASSERT ((SmbusBar & 0xffff001f) == 1);
  return (UINT16)(SmbusBar & ~1);
}

static
BOOLEAN
EFIAPI
SmBusAcquire (
  IN      UINT16                    SmBusBase
  )
{
  UINT8                             HstSts;

  HstSts = IoRead8 (SmBusBase + R_HST_STS);
  if (HstSts & B_HST_STS_INUSE) {
    return FALSE;
  }

  //
  // BUGBUG: Dead loop may occur here
  //
  while (HstSts & B_HST_STS_BUSY) {
    ASSERT (HstSts & B_HST_STS_INUSE);
    HstSts = IoRead8 (SmBusBase + R_HST_STS);
  }
  return TRUE;
}

static
VOID
EFIAPI
SmBusStart (
  IN      UINT16                    SmBusBase,
  IN      UINT8                     SmBusProtocol,
  IN      UINT8                     SlaveAddress
  )
{
  IoWrite8 (SmBusBase + R_XMIT_SLVA, SlaveAddress);
  IoWrite8 (
    SmBusBase + R_HST_CNT,
    IoBitFieldWrite8 (SmBusBase + R_HST_CNT, 2, 4, SmBusProtocol) |
    B_HST_CNT_START
    );
}

static
UINT8
EFIAPI
SmBusWait (
  IN      UINT16                    SmBusBase
  )
{
  UINT8                             HstSts;

  while (((HstSts = IoRead8 (SmBusBase + R_HST_STS)) & B_HST_STS_INTR) == 0);
  return HstSts;
}

static
VOID
EFIAPI
SmBusCleanup (
  IN      UINT16                    SmBusBase
  )
{
  IoWrite8 (SmBusBase + R_HST_STS, B_HST_STS_ALL);
}

static
RETURN_STATUS
EFIAPI
SmBusQuick (
  IN      UINT8                     SmBusAddress
  )
{
  RETURN_STATUS                     Status;
  UINT16                            SmBusBase;

  SmBusBase = GetSmBusIOBaseAddress ();
  if (!SmBusAcquire (SmBusBase)) {
    return RETURN_TIMEOUT;
  }

  SmBusStart (SmBusAddress, B_SMB_CMD_QUICK, SmBusAddress);
  if (SmBusWait (SmBusAddress) & B_HST_STS_ERR) {
    Status = RETURN_DEVICE_ERROR;
  } else {
    Status = RETURN_SUCCESS;
  }

  SmBusCleanup (SmBusAddress);
  return Status;
}

VOID
EFIAPI
SmBusQuickRead (
  IN      UINTN                     SmBusAddress,
  OUT     RETURN_STATUS             *Status
  )
{
  RETURN_STATUS                     RetStatus;

  ASSERT ((SmBusAddress & ~0xfe) == 0);
  RetStatus = SmBusQuick ((UINT8)SmBusAddress | B_SMBUS_READ);
  if (Status) {
    *Status = RetStatus;
  }
}

BOOLEAN
EFIAPI
SmBusQuickWrite (
  IN      UINTN                     SmBusAddress,
  OUT     RETURN_STATUS             *Status
  )
{
  RETURN_STATUS                     RetStatus;

  ASSERT ((SmBusAddress & ~0xfe) == 0);
  RetStatus = SmBusQuick ((UINT8)SmBusAddress | B_SMBUS_WRITE);
  if (Status) {
    *Status = RetStatus;
  }
  return (BOOLEAN)!RETURN_ERROR (RetStatus);
}

static
UINT16
EFIAPI
SmBusByteWord (
  IN      UINTN                     SmBusAddress,
  IN      UINT16                    Value,
  IN      UINT8                     SmBusProtocol,
  OUT     RETURN_STATUS             *Status
  )
{
  RETURN_STATUS                     RetStatus;
  UINT16                            SmBusBase;

  if (Status == NULL) {
    Status = &RetStatus;
  }

  SmBusBase = GetSmBusIOBaseAddress ();
  if (!SmBusAcquire (SmBusBase)) {
    *Status = RETURN_TIMEOUT;
    return Value;
  }

  IoWrite8 (SmBusBase + R_HST_CMD, (UINT8)(SmBusAddress >> 8));
  IoWrite8 (SmBusBase + R_HST_D0, (UINT8)Value);
  IoWrite8 (SmBusBase + R_HST_D1, (UINT8)(Value >> 8));
  if ((INTN)SmBusAddress < 0) {
    IoOr8 (SmBusBase + R_HST_CNT, B_HST_CNT_PEC);
    IoOr8 (SmBusBase + R_AUX_CTL, B_AUX_CTL_AAC);
  } else {
    IoAnd8 (SmBusBase + R_HST_CNT, (UINT8)~B_HST_CNT_PEC);
    IoAnd8 (SmBusBase + R_AUX_CTL, (UINT8)~B_AUX_CTL_AAC);
  }

  SmBusStart (SmBusBase, SmBusProtocol, (UINT8)SmBusAddress);

  if (SmBusWait (SmBusBase) & B_HST_STS_ERR) {
    *Status = RETURN_DEVICE_ERROR;
  } else {
    *Status = RETURN_SUCCESS;
    Value = IoRead8 (SmBusBase + R_HST_D0);
    Value |= (UINT16)IoRead8 (SmBusBase + R_HST_D1) << 8;
  }

  SmBusCleanup (SmBusBase);
  return Value;
}

UINT8
EFIAPI
SmBusReceiveByte (
  IN      UINTN                     SmBusAddress,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfe | MAX_BIT)) == 0);
  return (UINT8)SmBusByteWord (
                  SmBusAddress | B_SMBUS_READ,
                  0,
                  B_SMB_CMD_BYTE,
                  Status
                  );
}

UINT8
EFIAPI
SmBusSendByte (
  IN      UINTN                     SmBusAddress,
  IN      UINT8                     Value,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfe | MAX_BIT)) == 0);
  return (UINT8)SmBusByteWord (
                  SmBusAddress | B_SMBUS_WRITE,
                  Value,
                  B_SMB_CMD_BYTE,
                  Status
                  );
}

UINT8
EFIAPI
SmBusReadDataByte (
  IN      UINTN                     SmBusAddress,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfffe | MAX_BIT)) == 0);
  return (UINT8)SmBusByteWord (
                  SmBusAddress | B_SMBUS_READ,
                  0,
                  B_SMB_CMD_BYTE_DATA,
                  Status
                  );
}

UINT8
EFIAPI
SmBusWriteDataByte (
  IN      UINTN                     SmBusAddress,
  IN      UINT8                     Value,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT (((UINT32)SmBusAddress & ~(0xfffe | MAX_BIT)) == 0);
  return (UINT8)SmBusByteWord (
                  SmBusAddress | B_SMBUS_WRITE,
                  Value,
                  B_SMB_CMD_BYTE_DATA,
                  Status
                  );
}

UINT16
EFIAPI
SmBusReadDataWord (
  IN      UINTN                     SmBusAddress,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfffe | MAX_BIT)) == 0);
  return SmBusByteWord (
           SmBusAddress | B_SMBUS_READ,
           0,
           B_SMB_CMD_WORD_DATA,
           Status
           );
}

UINT16
EFIAPI
SmBusWriteDataWord (
  IN      UINTN                     SmBusAddress,
  IN      UINT16                    Value,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfffe | MAX_BIT)) == 0);
  return SmBusByteWord (
           SmBusAddress | B_SMBUS_WRITE,
           Value,
           B_SMB_CMD_WORD_DATA,
           Status
           );
}

UINT16
EFIAPI
SmBusProcessCall (
  IN      UINTN                     SmBusAddress,
  IN      UINT16                    Value,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfffe | MAX_BIT)) == 0);
  return SmBusByteWord (
           SmBusAddress | B_SMBUS_WRITE,
           Value,
           B_SMB_CMD_PROCESS_CALL,
           Status
           );
}

static
UINTN
EFIAPI
SmBusBlock (
  IN      UINTN                     SmBusAddress,
  IN      UINT8                     SmBusProtocol,
  IN      VOID                      *InBuffer,
  OUT     VOID                      *OutBuffer,
  OUT     RETURN_STATUS             *Status
  )
{
  RETURN_STATUS                     RetStatus;
  UINT16                            SmBusBase;
  UINTN                             Index;
  UINTN                             BytesCount;

  BytesCount = (UINT8)(SmBusAddress >> 16);
  ASSERT (BytesCount <= 32);

  if (Status == NULL) {
    Status = &RetStatus;
  }

  SmBusBase = GetSmBusIOBaseAddress ();
  if (!SmBusAcquire (SmBusBase)) {
    *Status = RETURN_TIMEOUT;
    return 0;
  }

  IoWrite8 (SmBusBase + R_HST_CMD, (UINT8)(SmBusAddress >> 8));
  IoWrite8 (SmBusBase + R_HST_D0, (UINT8)BytesCount);
  if ((INTN)SmBusAddress < 0) {
    IoOr8 (SmBusBase + R_HST_CNT, B_HST_CNT_PEC);
    IoOr8 (SmBusBase + R_AUX_CTL, B_AUX_CTL_AAC);
  } else {
    IoAnd8 (SmBusBase + R_HST_CNT, (UINT8)~B_HST_CNT_PEC);
    IoAnd8 (SmBusBase + R_AUX_CTL, (UINT8)~B_AUX_CTL_AAC);
  }

  //
  // BUGBUG: E32B bit does not exist in ICH3 or earlier
  //
  IoOr8 (SmBusBase + R_AUX_CTL, B_AUX_CTL_E32B);
  ASSERT (IoRead8 (SmBusBase + R_AUX_CTL) & B_AUX_CTL_E32B);
  for (Index = 0; InBuffer != NULL && Index < BytesCount; Index++) {
    IoWrite8 (SmBusBase + R_HOST_BLOCK_DB, ((UINT8*)InBuffer)[Index]);
  }

  SmBusStart (SmBusBase, SmBusProtocol, (UINT8)SmBusAddress);

  if (SmBusWait (SmBusBase) & B_HST_STS_ERR) {
    *Status = RETURN_DEVICE_ERROR;
  } else {
    *Status = RETURN_SUCCESS;
    BytesCount = IoRead8 (SmBusBase + R_HST_D0);
    for (Index = 0; OutBuffer != NULL && Index < BytesCount; Index++) {
      ((UINT8*)OutBuffer)[Index] = IoRead8 (SmBusBase + R_HOST_BLOCK_DB);
    }
  }

  SmBusCleanup (SmBusBase);
  return BytesCount;
}

UINTN
EFIAPI
SmBusReadBlock (
  IN      UINTN                     SmBusAddress,
  OUT     VOID                      *Buffer,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfffffe | MAX_BIT)) == 0);
  return SmBusBlock (
           SmBusAddress | B_SMBUS_READ,
           B_SMB_CMD_BLOCK,
           NULL,
           Buffer,
           Status
           );
}

UINTN
EFIAPI
SmBusWriteBlock (
  IN      UINTN                     SmBusAddress,
  OUT     VOID                      *Buffer,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfffffe | MAX_BIT)) == 0);
  return SmBusBlock (
           SmBusAddress | B_SMBUS_WRITE,
           B_SMB_CMD_BLOCK,
           Buffer,
           NULL,
           Status
           );
}

UINTN
EFIAPI
SmBusBlockProcessCall (
  IN      UINTN                     SmBusAddress,
  IN      VOID                      *OutBuffer,
  OUT     VOID                      *InBuffer,
  OUT     RETURN_STATUS             *Status
  )
{
  ASSERT ((SmBusAddress & ~(0xfffffe | MAX_BIT)) == 0);
  return SmBusBlock (
           SmBusAddress | B_SMBUS_WRITE,
           B_SMB_CMD_BLOCK_PROCESS,
           OutBuffer,
           InBuffer,
           Status
           );
}

RETURN_STATUS
EFIAPI
SmBusArpAll (
  IN      UINTN                     SmBusAddress
  );

RETURN_STATUS
EFIAPI
SmBusArpDevice (
  IN      UINTN                     SmBusAddress,
  IN      CONST GUID                *Uuid
  );

RETURN_STATUS
EFIAPI
SmBusGetUuid (
  IN      UINTN                     SmBusAddress,
  OUT     GUID                      *Uuid
  );

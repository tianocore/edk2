/** @file

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SD_HCI_H_
#define _SD_HCI_H_

//
// SD Host Controller MMIO Register Offset
//
#define SD_HC_SDMA_ADDR           0x00
#define SD_HC_ARG2                0x00
#define SD_HC_BLK_SIZE            0x04
#define SD_HC_BLK_COUNT           0x06
#define SD_HC_ARG1                0x08
#define SD_HC_TRANS_MOD           0x0C
#define SD_HC_COMMAND             0x0E
#define SD_HC_RESPONSE            0x10
#define SD_HC_BUF_DAT_PORT        0x20
#define SD_HC_PRESENT_STATE       0x24
#define SD_HC_HOST_CTRL1          0x28
#define SD_HC_POWER_CTRL          0x29
#define SD_HC_BLK_GAP_CTRL        0x2A
#define SD_HC_WAKEUP_CTRL         0x2B
#define SD_HC_CLOCK_CTRL          0x2C
#define SD_HC_TIMEOUT_CTRL        0x2E
#define SD_HC_SW_RST              0x2F
#define SD_HC_NOR_INT_STS         0x30
#define SD_HC_ERR_INT_STS         0x32
#define SD_HC_NOR_INT_STS_EN      0x34
#define SD_HC_ERR_INT_STS_EN      0x36
#define SD_HC_NOR_INT_SIG_EN      0x38
#define SD_HC_ERR_INT_SIG_EN      0x3A
#define SD_HC_AUTO_CMD_ERR_STS    0x3C
#define SD_HC_HOST_CTRL2          0x3E
#define SD_HC_CAP                 0x40
#define SD_HC_MAX_CURRENT_CAP     0x48
#define SD_HC_FORCE_EVT_AUTO_CMD  0x50
#define SD_HC_FORCE_EVT_ERR_INT   0x52
#define SD_HC_ADMA_ERR_STS        0x54
#define SD_HC_ADMA_SYS_ADDR       0x58
#define SD_HC_PRESET_VAL          0x60
#define SD_HC_SHARED_BUS_CTRL     0xE0
#define SD_HC_SLOT_INT_STS        0xFC
#define SD_HC_CTRL_VER            0xFE

//
// The transfer modes supported by SD Host Controller
// Simplified Spec 3.0 Table 1-2
//
typedef enum {
  SdNoData,
  SdPioMode,
  SdSdmaMode,
  SdAdmaMode
} SD_HC_TRANSFER_MODE;

//
// The maximum data length of each descriptor line
//
#define ADMA_MAX_DATA_PER_LINE      0x10000
#define SD_SDMA_BOUNDARY            512 * 1024
#define SD_SDMA_ROUND_UP(x, n)      (((x) + n) & ~(n - 1))

typedef enum {
  SdCommandTypeBc,  // Broadcast commands, no response
  SdCommandTypeBcr, // Broadcast commands with response
  SdCommandTypeAc,  // Addressed(point-to-point) commands
  SdCommandTypeAdtc // Addressed(point-to-point) data transfer commands
} SD_COMMAND_TYPE;

typedef enum {
  SdResponseTypeR1,
  SdResponseTypeR1b,
  SdResponseTypeR2,
  SdResponseTypeR3,
  SdResponseTypeR4,
  SdResponseTypeR5,
  SdResponseTypeR5b,
  SdResponseTypeR6,
  SdResponseTypeR7
} SD_RESPONSE_TYPE;

typedef struct _SD_COMMAND_BLOCK {
  UINT16                            CommandIndex;
  UINT32                            CommandArgument;
  UINT32                            CommandType;      // One of the SD_COMMAND_TYPE values
  UINT32                            ResponseType;     // One of the SD_RESPONSE_TYPE values
} SD_COMMAND_BLOCK;

typedef struct _SD_STATUS_BLOCK {
  UINT32                            Resp0;
  UINT32                            Resp1;
  UINT32                            Resp2;
  UINT32                            Resp3;
} SD_STATUS_BLOCK;

typedef struct _SD_COMMAND_PACKET {
  UINT64                            Timeout;
  SD_COMMAND_BLOCK                  *SdCmdBlk;
  SD_STATUS_BLOCK                   *SdStatusBlk;
  VOID                              *InDataBuffer;
  VOID                              *OutDataBuffer;
  UINT32                            InTransferLength;
  UINT32                            OutTransferLength;
} SD_COMMAND_PACKET;

#pragma pack(1)

typedef struct {
  UINT32 Valid:1;
  UINT32 End:1;
  UINT32 Int:1;
  UINT32 Reserved:1;
  UINT32 Act:2;
  UINT32 Reserved1:10;
  UINT32 Length:16;
  UINT32 Address;
} SD_HC_ADMA_DESC_LINE;

typedef struct {
  UINT32   TimeoutFreq:6;     // bit 0:5
  UINT32   Reserved:1;        // bit 6
  UINT32   TimeoutUnit:1;     // bit 7
  UINT32   BaseClkFreq:8;     // bit 8:15
  UINT32   MaxBlkLen:2;       // bit 16:17
  UINT32   BusWidth8:1;       // bit 18
  UINT32   Adma2:1;           // bit 19
  UINT32   Reserved2:1;       // bit 20
  UINT32   HighSpeed:1;       // bit 21
  UINT32   Sdma:1;            // bit 22
  UINT32   SuspRes:1;         // bit 23
  UINT32   Voltage33:1;       // bit 24
  UINT32   Voltage30:1;       // bit 25
  UINT32   Voltage18:1;       // bit 26
  UINT32   Reserved3:1;       // bit 27
  UINT32   SysBus64:1;        // bit 28
  UINT32   AsyncInt:1;        // bit 29
  UINT32   SlotType:2;        // bit 30:31
  UINT32   Sdr50:1;           // bit 32
  UINT32   Sdr104:1;          // bit 33
  UINT32   Ddr50:1;           // bit 34
  UINT32   Reserved4:1;       // bit 35
  UINT32   DriverTypeA:1;     // bit 36
  UINT32   DriverTypeC:1;     // bit 37
  UINT32   DriverTypeD:1;     // bit 38
  UINT32   DriverType4:1;     // bit 39
  UINT32   TimerCount:4;      // bit 40:43
  UINT32   Reserved5:1;       // bit 44
  UINT32   TuningSDR50:1;     // bit 45
  UINT32   RetuningMod:2;     // bit 46:47
  UINT32   ClkMultiplier:8;   // bit 48:55
  UINT32   Reserved6:7;       // bit 56:62
  UINT32   Hs400:1;           // bit 63
} SD_HC_SLOT_CAP;

#pragma pack()

/**
  Software reset the specified SD host controller and enable all interrupts.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The software reset executes successfully.
  @retval Others            The software reset fails.

**/
EFI_STATUS
SdPeimHcReset (
  IN UINTN                  Bar
  );

/**
  Set all interrupt status bits in Normal and Error Interrupt Status Enable
  register.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The operation executes successfully.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimHcEnableInterrupt (
  IN UINTN                  Bar
  );

/**
  Get the capability data from the specified slot.

  @param[in]  Bar             The mmio base address of the slot to be accessed.
  @param[out] Capability      The buffer to store the capability data.

  @retval EFI_SUCCESS         The operation executes successfully.
  @retval Others              The operation fails.

**/
EFI_STATUS
SdPeimHcGetCapability (
  IN     UINTN              Bar,
     OUT SD_HC_SLOT_CAP     *Capability
  );

/**
  Detect whether there is a SD card attached at the specified SD host controller
  slot.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.1 for details.

  @param[in]  Bar           The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       There is a SD card attached.
  @retval EFI_NO_MEDIA      There is not a SD card attached.
  @retval Others            The detection fails.

**/
EFI_STATUS
SdPeimHcCardDetect (
  IN UINTN                  Bar
  );

/**
  Initial SD host controller with lowest clock frequency, max power and max timeout value
  at initialization.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The host controller is initialized successfully.
  @retval Others            The host controller isn't initialized successfully.

**/
EFI_STATUS
SdPeimHcInitHost (
  IN UINTN                  Bar
  );

/**
  Send command SWITCH_FUNC to the SD device to check switchable function or switch card function.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  AccessMode    The value for access mode group.
  @param[in]  CommandSystem The value for command set group.
  @param[in]  DriveStrength The value for drive length group.
  @param[in]  PowerLimit    The value for power limit group.
  @param[in]  Mode          Switch or check function.
  @param[out] SwitchResp    The return switch function status.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSwitch (
  IN     SD_PEIM_HC_SLOT              *Slot,
  IN     UINT8                        AccessMode,
  IN     UINT8                        CommandSystem,
  IN     UINT8                        DriveStrength,
  IN     UINT8                        PowerLimit,
  IN     BOOLEAN                      Mode,
     OUT UINT8                        *SwitchResp
  );

/**
  Send command READ_SINGLE_BLOCK/WRITE_SINGLE_BLOCK to the addressed SD device
  to read/write the specified number of blocks.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Lba            The logical block address of starting access.
  @param[in] BlockSize      The block size of specified SD device partition.
  @param[in] Buffer         The pointer to the transfer buffer.
  @param[in] BufferSize     The size of transfer buffer.
  @param[in] IsRead         Boolean to show the operation direction.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimRwSingleBlock (
  IN SD_PEIM_HC_SLOT                *Slot,
  IN EFI_LBA                        Lba,
  IN UINT32                         BlockSize,
  IN VOID                           *Buffer,
  IN UINTN                          BufferSize,
  IN BOOLEAN                        IsRead
  );

/**
  Send command READ_MULTIPLE_BLOCK/WRITE_MULTIPLE_BLOCK to the addressed SD device
  to read/write the specified number of blocks.

  Refer to SD Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in] Slot           The slot number of the Sd card to send the command to.
  @param[in] Lba            The logical block address of starting access.
  @param[in] BlockSize      The block size of specified SD device partition.
  @param[in] Buffer         The pointer to the transfer buffer.
  @param[in] BufferSize     The size of transfer buffer.
  @param[in] IsRead         Boolean to show the operation direction.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimRwMultiBlocks (
  IN SD_PEIM_HC_SLOT                *Slot,
  IN EFI_LBA                        Lba,
  IN UINT32                         BlockSize,
  IN VOID                           *Buffer,
  IN UINTN                          BufferSize,
  IN BOOLEAN                        IsRead
  );

/**
  Execute SD device identification procedure.

  Refer to SD Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] Slot           The slot number of the Sd card to send the command to.

  @retval EFI_SUCCESS       There is a SD card.
  @retval Others            There is not a SD card.

**/
EFI_STATUS
SdPeimIdentification (
  IN SD_PEIM_HC_SLOT           *Slot
  );

/**
  Free the resource used by the TRB.

  @param[in] Trb        The pointer to the SD_TRB instance.

**/
VOID
SdPeimFreeTrb (
  IN SD_TRB           *Trb
  );

#endif


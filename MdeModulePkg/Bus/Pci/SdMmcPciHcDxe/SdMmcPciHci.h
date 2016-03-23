/** @file

  Provides some data structure definitions used by the SD/MMC host controller driver.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SD_MMC_PCI_HCI_H_
#define _SD_MMC_PCI_HCI_H_

//
// SD Host Controller SlotInfo Register Offset
//
#define SD_MMC_HC_SLOT_OFFSET         0x40

#define SD_MMC_HC_MAX_SLOT            6

//
// SD Host Controller MMIO Register Offset
//
#define SD_MMC_HC_SDMA_ADDR           0x00
#define SD_MMC_HC_ARG2                0x00
#define SD_MMC_HC_BLK_SIZE            0x04
#define SD_MMC_HC_BLK_COUNT           0x06
#define SD_MMC_HC_ARG1                0x08
#define SD_MMC_HC_TRANS_MOD           0x0C
#define SD_MMC_HC_COMMAND             0x0E
#define SD_MMC_HC_RESPONSE            0x10
#define SD_MMC_HC_BUF_DAT_PORT        0x20
#define SD_MMC_HC_PRESENT_STATE       0x24
#define SD_MMC_HC_HOST_CTRL1          0x28
#define SD_MMC_HC_POWER_CTRL          0x29
#define SD_MMC_HC_BLK_GAP_CTRL        0x2A
#define SD_MMC_HC_WAKEUP_CTRL         0x2B
#define SD_MMC_HC_CLOCK_CTRL          0x2C
#define SD_MMC_HC_TIMEOUT_CTRL        0x2E
#define SD_MMC_HC_SW_RST              0x2F
#define SD_MMC_HC_NOR_INT_STS         0x30
#define SD_MMC_HC_ERR_INT_STS         0x32
#define SD_MMC_HC_NOR_INT_STS_EN      0x34
#define SD_MMC_HC_ERR_INT_STS_EN      0x36
#define SD_MMC_HC_NOR_INT_SIG_EN      0x38
#define SD_MMC_HC_ERR_INT_SIG_EN      0x3A
#define SD_MMC_HC_AUTO_CMD_ERR_STS    0x3C
#define SD_MMC_HC_HOST_CTRL2          0x3E
#define SD_MMC_HC_CAP                 0x40
#define SD_MMC_HC_MAX_CURRENT_CAP     0x48
#define SD_MMC_HC_FORCE_EVT_AUTO_CMD  0x50
#define SD_MMC_HC_FORCE_EVT_ERR_INT   0x52
#define SD_MMC_HC_ADMA_ERR_STS        0x54
#define SD_MMC_HC_ADMA_SYS_ADDR       0x58
#define SD_MMC_HC_PRESET_VAL          0x60
#define SD_MMC_HC_SHARED_BUS_CTRL     0xE0
#define SD_MMC_HC_SLOT_INT_STS        0xFC
#define SD_MMC_HC_CTRL_VER            0xFE

//
// The transfer modes supported by SD Host Controller
// Simplified Spec 3.0 Table 1-2
//
typedef enum {
  SdMmcNoData,
  SdMmcPioMode,
  SdMmcSdmaMode,
  SdMmcAdmaMode
} SD_MMC_HC_TRANSFER_MODE;

//
// The maximum data length of each descriptor line
//
#define ADMA_MAX_DATA_PER_LINE     0x10000

typedef struct {
  UINT32 Valid:1;
  UINT32 End:1;
  UINT32 Int:1;
  UINT32 Reserved:1;
  UINT32 Act:2;
  UINT32 Reserved1:10;
  UINT32 Length:16;
  UINT32 Address;
} SD_MMC_HC_ADMA_DESC_LINE;

#define SD_MMC_SDMA_BOUNDARY          512 * 1024
#define SD_MMC_SDMA_ROUND_UP(x, n)    (((x) + n) & ~(n - 1))

typedef struct {
  UINT8    FirstBar:3;        // bit 0:2
  UINT8    Reserved:1;        // bit 3
  UINT8    SlotNum:3;         // bit 4:6
  UINT8    Reserved1:1;       // bit 7
} SD_MMC_HC_SLOT_INFO;

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
} SD_MMC_HC_SLOT_CAP;

/**
  Dump the content of SD/MMC host controller's Capability Register.

  @param[in]  Slot            The slot number of the SD card to send the command to.
  @param[in]  Capability      The buffer to store the capability data.

**/
VOID
DumpCapabilityReg (
  IN UINT8                Slot,
  IN SD_MMC_HC_SLOT_CAP   *Capability
  );

/**
  Read SlotInfo register from SD/MMC host controller pci config space.

  @param[in]  PciIo        The PCI IO protocol instance.
  @param[out] FirstBar     The buffer to store the first BAR value.
  @param[out] SlotNum      The buffer to store the supported slot number.

  @retval EFI_SUCCESS      The operation succeeds.
  @retval Others           The operation fails.

**/
EFI_STATUS
EFIAPI
SdMmcHcGetSlotInfo (
  IN     EFI_PCI_IO_PROTOCOL   *PciIo,
     OUT UINT8                 *FirstBar,
     OUT UINT8                 *SlotNum
  );

/**
  Read/Write specified SD/MMC host controller mmio register.

  @param[in]      PciIo        The PCI IO protocol instance.
  @param[in]      BarIndex     The BAR index of the standard PCI Configuration
                               header to use as the base address for the memory
                               operation to perform.
  @param[in]      Offset       The offset within the selected BAR to start the
                               memory operation.
  @param[in]      Read         A boolean to indicate it's read or write operation.
  @param[in]      Count        The width of the mmio register in bytes.
                               Must be 1, 2 , 4 or 8 bytes.
  @param[in, out] Data         For read operations, the destination buffer to store
                               the results. For write operations, the source buffer
                               to write data from. The caller is responsible for
                               having ownership of the data buffer and ensuring its
                               size not less than Count bytes.

  @retval EFI_INVALID_PARAMETER The PciIo or Data is NULL or the Count is not valid.
  @retval EFI_SUCCESS           The read/write operation succeeds.
  @retval Others                The read/write operation fails.

**/
EFI_STATUS
EFIAPI
SdMmcHcRwMmio (
  IN     EFI_PCI_IO_PROTOCOL   *PciIo,
  IN     UINT8                 BarIndex,
  IN     UINT32                Offset,
  IN     BOOLEAN               Read,
  IN     UINT8                 Count,
  IN OUT VOID                  *Data
  );

/**
  Do OR operation with the value of the specified SD/MMC host controller mmio register.

  @param[in] PciIo             The PCI IO protocol instance.
  @param[in] BarIndex          The BAR index of the standard PCI Configuration
                               header to use as the base address for the memory
                               operation to perform.
  @param[in] Offset            The offset within the selected BAR to start the
                               memory operation.
  @param[in] Count             The width of the mmio register in bytes.
                               Must be 1, 2 , 4 or 8 bytes.
  @param[in] OrData            The pointer to the data used to do OR operation.
                               The caller is responsible for having ownership of
                               the data buffer and ensuring its size not less than
                               Count bytes.

  @retval EFI_INVALID_PARAMETER The PciIo or OrData is NULL or the Count is not valid.
  @retval EFI_SUCCESS           The OR operation succeeds.
  @retval Others                The OR operation fails.

**/
EFI_STATUS
EFIAPI
SdMmcHcOrMmio (
  IN  EFI_PCI_IO_PROTOCOL      *PciIo,
  IN  UINT8                    BarIndex,
  IN  UINT32                   Offset,
  IN  UINT8                    Count,
  IN  VOID                     *OrData
  );

/**
  Do AND operation with the value of the specified SD/MMC host controller mmio register.

  @param[in] PciIo             The PCI IO protocol instance.
  @param[in] BarIndex          The BAR index of the standard PCI Configuration
                               header to use as the base address for the memory
                               operation to perform.
  @param[in] Offset            The offset within the selected BAR to start the
                               memory operation.
  @param[in] Count             The width of the mmio register in bytes.
                               Must be 1, 2 , 4 or 8 bytes.
  @param[in] AndData           The pointer to the data used to do AND operation.
                               The caller is responsible for having ownership of
                               the data buffer and ensuring its size not less than
                               Count bytes.

  @retval EFI_INVALID_PARAMETER The PciIo or AndData is NULL or the Count is not valid.
  @retval EFI_SUCCESS           The AND operation succeeds.
  @retval Others                The AND operation fails.

**/
EFI_STATUS
EFIAPI
SdMmcHcAndMmio (
  IN  EFI_PCI_IO_PROTOCOL      *PciIo,
  IN  UINT8                    BarIndex,
  IN  UINT32                   Offset,
  IN  UINT8                    Count,
  IN  VOID                     *AndData
  );

/**
  Wait for the value of the specified MMIO register set to the test value.

  @param[in]  PciIo         The PCI IO protocol instance.
  @param[in]  BarIndex      The BAR index of the standard PCI Configuration
                            header to use as the base address for the memory
                            operation to perform.
  @param[in]  Offset        The offset within the selected BAR to start the
                            memory operation.
  @param[in]  Count         The width of the mmio register in bytes.
                            Must be 1, 2, 4 or 8 bytes.
  @param[in]  MaskValue     The mask value of memory.
  @param[in]  TestValue     The test value of memory.
  @param[in]  Timeout       The time out value for wait memory set, uses 1
                            microsecond as a unit.

  @retval EFI_TIMEOUT       The MMIO register hasn't expected value in timeout
                            range.
  @retval EFI_SUCCESS       The MMIO register has expected value.
  @retval Others            The MMIO operation fails.

**/
EFI_STATUS
EFIAPI
SdMmcHcWaitMmioSet (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo,
  IN  UINT8                     BarIndex,
  IN  UINT32                    Offset,
  IN  UINT8                     Count,
  IN  UINT64                    MaskValue,
  IN  UINT64                    TestValue,
  IN  UINT64                    Timeout
  );

/**
  Software reset the specified SD/MMC host controller.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The software reset executes successfully.
  @retval Others            The software reset fails.

**/
EFI_STATUS
SdMmcHcReset (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot
  );

/**
  Set all interrupt status bits in Normal and Error Interrupt Status Enable
  register.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation executes successfully.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdMmcHcEnableInterrupt (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot
  );

/**
  Get the capability data from the specified slot.

  @param[in]  PciIo           The PCI IO protocol instance.
  @param[in]  Slot            The slot number of the SD card to send the command to.
  @param[out] Capability      The buffer to store the capability data.

  @retval EFI_SUCCESS         The operation executes successfully.
  @retval Others              The operation fails.

**/
EFI_STATUS
SdMmcHcGetCapability (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT8                Slot,
     OUT SD_MMC_HC_SLOT_CAP   *Capability
  );

/**
  Get the maximum current capability data from the specified slot.

  @param[in]  PciIo           The PCI IO protocol instance.
  @param[in]  Slot            The slot number of the SD card to send the command to.
  @param[out] MaxCurrent      The buffer to store the maximum current capability data.

  @retval EFI_SUCCESS         The operation executes successfully.
  @retval Others              The operation fails.

**/
EFI_STATUS
SdMmcHcGetMaxCurrent (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT8                Slot,
     OUT UINT64               *MaxCurrent
  );

/**
  Detect whether there is a SD/MMC card attached at the specified SD/MMC host controller
  slot.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.1 for details.

  @param[in]  PciIo         The PCI IO protocol instance.
  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[out] MediaPresent  The pointer to the media present boolean value.

  @retval EFI_SUCCESS       There is no media change happened.
  @retval EFI_MEDIA_CHANGED There is media change happened.
  @retval Others            The detection fails.

**/
EFI_STATUS
SdMmcHcCardDetect (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot,
     OUT BOOLEAN            *MediaPresent
  );

/**
  Stop SD/MMC card clock.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.2.2 for details.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       Succeed to stop SD/MMC clock.
  @retval Others            Fail to stop SD/MMC clock.

**/
EFI_STATUS
SdMmcHcStopClock (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot
  );

/**
  SD/MMC card clock supply.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.2.1 for details.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] ClockFreq      The max clock frequency to be set. The unit is KHz.
  @param[in] Capability     The capability of the slot.

  @retval EFI_SUCCESS       The clock is supplied successfully.
  @retval Others            The clock isn't supplied successfully.

**/
EFI_STATUS
SdMmcHcClockSupply (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot,
  IN UINT64                 ClockFreq,
  IN SD_MMC_HC_SLOT_CAP     Capability
  );

/**
  SD/MMC bus power control.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.3 for details.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] PowerCtrl      The value setting to the power control register.

  @retval TRUE              There is a SD/MMC card attached.
  @retval FALSE             There is no a SD/MMC card attached.

**/
EFI_STATUS
SdMmcHcPowerControl (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot,
  IN UINT8                  PowerCtrl
  );

/**
  Set the SD/MMC bus width.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.4 for details.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] BusWidth       The bus width used by the SD/MMC device, it must be 1, 4 or 8.

  @retval EFI_SUCCESS       The bus width is set successfully.
  @retval Others            The bus width isn't set successfully.

**/
EFI_STATUS
SdMmcHcSetBusWidth (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot,
  IN UINT16                 BusWidth
  );

/**
  Supply SD/MMC card with lowest clock frequency at initialization.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Capability     The capability of the slot.

  @retval EFI_SUCCESS       The clock is supplied successfully.
  @retval Others            The clock isn't supplied successfully.

**/
EFI_STATUS
SdMmcHcInitClockFreq (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot,
  IN SD_MMC_HC_SLOT_CAP     Capability
  );

/**
  Supply SD/MMC card with maximum voltage at initialization.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.3 for details.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Capability     The capability of the slot.

  @retval EFI_SUCCESS       The voltage is supplied successfully.
  @retval Others            The voltage isn't supplied successfully.

**/
EFI_STATUS
SdMmcHcInitPowerVoltage (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot,
  IN SD_MMC_HC_SLOT_CAP     Capability
  );

/**
  Initialize the Timeout Control register with most conservative value at initialization.

  Refer to SD Host Controller Simplified spec 3.0 Section 2.2.15 for details.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The timeout control register is configured successfully.
  @retval Others            The timeout control register isn't configured successfully.

**/
EFI_STATUS
SdMmcHcInitTimeoutCtrl (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot
  );

/**
  Initial SD/MMC host controller with lowest clock frequency, max power and max timeout value
  at initialization.

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Capability     The capability of the slot.

  @retval EFI_SUCCESS       The host controller is initialized successfully.
  @retval Others            The host controller isn't initialized successfully.

**/
EFI_STATUS
SdMmcHcInitHost (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINT8                  Slot,
  IN SD_MMC_HC_SLOT_CAP     Capability
  );

#endif

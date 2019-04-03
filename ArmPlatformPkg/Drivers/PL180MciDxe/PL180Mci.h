/** @file
  Header for the MMC Host Protocol implementation for the ARM PrimeCell PL180.

  Copyright (c) 2011-2012, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PL180_MCI_H
#define __PL180_MCI_H

#include <Uefi.h>

#include <Protocol/MmcHost.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>

#define PL180_MCI_DXE_VERSION           0x10

#define MCI_SYSCTL  FixedPcdGet32 (PcdPL180MciBaseAddress)

#define MCI_POWER_CONTROL_REG           (MCI_SYSCTL + 0x000)
#define MCI_CLOCK_CONTROL_REG           (MCI_SYSCTL + 0x004)
#define MCI_ARGUMENT_REG                (MCI_SYSCTL + 0x008)
#define MCI_COMMAND_REG                 (MCI_SYSCTL + 0x00C)
#define MCI_RESPCMD_REG                 (MCI_SYSCTL + 0x010)
#define MCI_RESPONSE3_REG               (MCI_SYSCTL + 0x014)
#define MCI_RESPONSE2_REG               (MCI_SYSCTL + 0x018)
#define MCI_RESPONSE1_REG               (MCI_SYSCTL + 0x01C)
#define MCI_RESPONSE0_REG               (MCI_SYSCTL + 0x020)
#define MCI_DATA_TIMER_REG              (MCI_SYSCTL + 0x024)
#define MCI_DATA_LENGTH_REG             (MCI_SYSCTL + 0x028)
#define MCI_DATA_CTL_REG                (MCI_SYSCTL + 0x02C)
#define MCI_DATA_COUNTER                (MCI_SYSCTL + 0x030)
#define MCI_STATUS_REG                  (MCI_SYSCTL + 0x034)
#define MCI_CLEAR_STATUS_REG            (MCI_SYSCTL + 0x038)
#define MCI_INT0_MASK_REG               (MCI_SYSCTL + 0x03C)
#define MCI_INT1_MASK_REG               (MCI_SYSCTL + 0x040)
#define MCI_SELECT_REG                  (MCI_SYSCTL + 0x044)
#define MCI_FIFOCOUNT_REG               (MCI_SYSCTL + 0x048)
#define MCI_FIFO_REG                    (MCI_SYSCTL + 0x080)
#define MCI_PERIPH_ID_REG0              (MCI_SYSCTL + 0xFE0)
#define MCI_PERIPH_ID_REG1              (MCI_SYSCTL + 0xFE4)
#define MCI_PERIPH_ID_REG2              (MCI_SYSCTL + 0xFE8)
#define MCI_PERIPH_ID_REG3              (MCI_SYSCTL + 0xFEC)
#define MCI_PCELL_ID_REG0               (MCI_SYSCTL + 0xFF0)
#define MCI_PCELL_ID_REG1               (MCI_SYSCTL + 0xFF4)
#define MCI_PCELL_ID_REG2               (MCI_SYSCTL + 0xFF8)
#define MCI_PCELL_ID_REG3               (MCI_SYSCTL + 0xFFC)

#define MCI_PERIPH_ID0                  0x80
#define MCI_PERIPH_ID1                  0x11
#define MCI_PERIPH_ID2                  0x04
#define MCI_PERIPH_ID3                  0x00
#define MCI_PCELL_ID0                   0x0D
#define MCI_PCELL_ID1                   0xF0
#define MCI_PCELL_ID2                   0x05
#define MCI_PCELL_ID3                   0xB1

#define MCI_POWER_OFF                   0
#define MCI_POWER_UP                    BIT1
#define MCI_POWER_ON                    (BIT1 | BIT0)
#define MCI_POWER_OPENDRAIN             BIT6
#define MCI_POWER_ROD                   BIT7

#define MCI_CLOCK_ENABLE                BIT8
#define MCI_CLOCK_POWERSAVE             BIT9
#define MCI_CLOCK_BYPASS                BIT10
#define MCI_CLOCK_WIDEBUS               BIT11

#define MCI_STATUS_CMD_CMDCRCFAIL       BIT0
#define MCI_STATUS_CMD_DATACRCFAIL      BIT1
#define MCI_STATUS_CMD_CMDTIMEOUT       BIT2
#define MCI_STATUS_CMD_DATATIMEOUT      BIT3
#define MCI_STATUS_CMD_TX_UNDERRUN      BIT4
#define MCI_STATUS_CMD_RXOVERRUN        BIT5
#define MCI_STATUS_CMD_RESPEND          BIT6
#define MCI_STATUS_CMD_SENT             BIT7
#define MCI_STATUS_CMD_DATAEND          BIT8
#define MCI_STATUS_CMD_START_BIT_ERROR  BIT9
#define MCI_STATUS_CMD_DATABLOCKEND     BIT10
#define MCI_STATUS_CMD_ACTIVE           BIT11
#define MCI_STATUS_CMD_TXACTIVE         BIT12
#define MCI_STATUS_CMD_RXACTIVE         BIT13
#define MCI_STATUS_CMD_TXFIFOHALFEMPTY  BIT14
#define MCI_STATUS_CMD_RXFIFOHALFFULL   BIT15
#define MCI_STATUS_CMD_TXFIFOFULL       BIT16
#define MCI_STATUS_CMD_RXFIFOFULL       BIT17
#define MCI_STATUS_CMD_TXFIFOEMPTY      BIT18
#define MCI_STATUS_CMD_RXFIFOEMPTY      BIT19
#define MCI_STATUS_CMD_TXDATAAVAILBL    BIT20
#define MCI_STATUS_CMD_RXDATAAVAILBL    BIT21

#define MCI_STATUS_TXDONE               (MCI_STATUS_CMD_DATAEND | MCI_STATUS_CMD_DATABLOCKEND)
#define MCI_STATUS_RXDONE               (MCI_STATUS_CMD_DATAEND | MCI_STATUS_CMD_DATABLOCKEND)
#define MCI_STATUS_READ_ERROR           (  MCI_STATUS_CMD_DATACRCFAIL     \
                                         | MCI_STATUS_CMD_DATATIMEOUT     \
                                         | MCI_STATUS_CMD_RXOVERRUN       \
                                         | MCI_STATUS_CMD_START_BIT_ERROR )
#define MCI_STATUS_WRITE_ERROR          (  MCI_STATUS_CMD_DATACRCFAIL \
                                         | MCI_STATUS_CMD_DATATIMEOUT \
                                         | MCI_STATUS_CMD_TX_UNDERRUN )
#define MCI_STATUS_CMD_ERROR            (  MCI_STATUS_CMD_CMDCRCFAIL      \
                                         | MCI_STATUS_CMD_CMDTIMEOUT      \
                                         | MCI_STATUS_CMD_START_BIT_ERROR )

#define MCI_CLR_CMD_STATUS              (  MCI_STATUS_CMD_RESPEND \
                                         | MCI_STATUS_CMD_SENT    \
                                         | MCI_STATUS_CMD_ERROR )

#define MCI_CLR_READ_STATUS             (  MCI_STATUS_RXDONE     \
                                         | MCI_STATUS_READ_ERROR )

#define MCI_CLR_WRITE_STATUS            (  MCI_STATUS_TXDONE      \
                                         | MCI_STATUS_WRITE_ERROR )

#define MCI_CLR_ALL_STATUS              (BIT11 - 1)

#define MCI_DATACTL_DISABLE_MASK        0xFE
#define MCI_DATACTL_ENABLE              BIT0
#define MCI_DATACTL_CONT_TO_CARD        0
#define MCI_DATACTL_CARD_TO_CONT        BIT1
#define MCI_DATACTL_BLOCK_TRANS         0
#define MCI_DATACTL_STREAM_TRANS        BIT2
#define MCI_DATACTL_DMA_DISABLED        0
#define MCI_DATACTL_DMA_ENABLE          BIT3

#define INDX_MASK                       0x3F

#define MCI_CPSM_WAIT_RESPONSE          BIT6
#define MCI_CPSM_LONG_RESPONSE          BIT7
#define MCI_CPSM_LONG_INTERRUPT         BIT8
#define MCI_CPSM_LONG_PENDING           BIT9
#define MCI_CPSM_ENABLE                 BIT10

#define MCI_TRACE(txt)                  DEBUG ((EFI_D_BLKIO, "ARM_MCI: " txt "\n"))

EFI_STATUS
EFIAPI
MciGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
MciGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

#endif

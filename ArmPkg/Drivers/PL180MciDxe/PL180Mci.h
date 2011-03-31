/** @file
  Header for the MMC Host Protocol implementation for the ARM PrimeCell PL180.

  Copyright (c) 2011, ARM Limited. All rights reserved.
  
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

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

#define PL180_MCI_DXE_VERSION   0x10

#define MCI_SYSCTL  FixedPcdGet32(PcdPL180MciBaseAddress)

#define MCI_POWER_CONTROL_REG           (MCI_SYSCTL+0x000)
#define MCI_CLOCK_CONTROL_REG           (MCI_SYSCTL+0x004)
#define MCI_ARGUMENT_REG                (MCI_SYSCTL+0x008)
#define MCI_COMMAND_REG                 (MCI_SYSCTL+0x00C)
#define MCI_RESPCMD_REG                 (MCI_SYSCTL+0x010)
#define MCI_RESPONSE0_REG               (MCI_SYSCTL+0x014)
#define MCI_RESPONSE1_REG               (MCI_SYSCTL+0x018)
#define MCI_RESPONSE2_REG               (MCI_SYSCTL+0x01C)
#define MCI_RESPONSE3_REG               (MCI_SYSCTL+0x020)
#define MCI_DATA_TIMER_REG              (MCI_SYSCTL+0x024)
#define MCI_DATA_LENGTH_REG             (MCI_SYSCTL+0x028)
#define MCI_DATA_CTL_REG                (MCI_SYSCTL+0x02C)
#define MCI_DATA_COUNTER                (MCI_SYSCTL+0x030)
#define MCI_STATUS_REG                  (MCI_SYSCTL+0x034)
#define MCI_CLEAR_STATUS_REG            (MCI_SYSCTL+0x038)
#define MCI_INT0_MASK_REG               (MCI_SYSCTL+0x03C)
#define MCI_INT1_MASK_REG               (MCI_SYSCTL+0x040)
#define MCI_FIFOCOUNT_REG               (MCI_SYSCTL+0x048)
#define MCI_FIFO_REG                    (MCI_SYSCTL+0x080)

#define MCI_POWER_UP                    0x2
#define MCI_POWER_ON                    0x3
#define MCI_POWER_OPENDRAIN             (1 << 6)
#define MCI_POWER_ROD                   (1 << 7)

#define MCI_CLOCK_ENABLE                0x100
#define MCI_CLOCK_POWERSAVE             0x200
#define MCI_CLOCK_BYPASS                0x400

#define MCI_STATUS_CMD_CMDCRCFAIL       0x1
#define MCI_STATUS_CMD_DATACRCFAIL      0x2
#define MCI_STATUS_CMD_CMDTIMEOUT       0x4
#define MCI_STATUS_CMD_DATATIMEOUT      0x8
#define MCI_STATUS_CMD_TX_UNDERRUN      0x10
#define MCI_STATUS_CMD_RXOVERRUN        0x20
#define MCI_STATUS_CMD_RESPEND          0x40
#define MCI_STATUS_CMD_SENT             0x80
#define MCI_STATUS_CMD_TXDONE           (MCI_STATUS_CMD_DATAEND | MCI_STATUS_CMD_DATABLOCKEND)
#define MCI_STATUS_CMD_DATAEND          0x000100    // Command Status - Data end
#define MCI_STATUS_CMD_START_BIT_ERROR  0x000200
#define MCI_STATUS_CMD_DATABLOCKEND     0x000400    // Command Status - Data end
#define MCI_STATUS_CMD_ACTIVE           0x800
#define MCI_STATUS_CMD_RXACTIVE         (1 << 13)
#define MCI_STATUS_CMD_RXFIFOHALFFULL   0x008000
#define MCI_STATUS_CMD_RXFIFOEMPTY      0x080000
#define MCI_STATUS_CMD_RXDATAAVAILBL    (1 << 21)
#define MCI_STATUS_CMD_TXACTIVE         (1 << 12)
#define MCI_STATUS_CMD_TXFIFOFULL       (1 << 16)
#define MCI_STATUS_CMD_TXFIFOHALFEMPTY  (1 << 14)
#define MCI_STATUS_CMD_TXFIFOEMPTY      (1 << 18)
#define MCI_STATUS_CMD_TXDATAAVAILBL    (1 << 20)

#define MCI_DATACTL_ENABLE              1
#define MCI_DATACTL_CONT_TO_CARD        0
#define MCI_DATACTL_CARD_TO_CONT        2
#define MCI_DATACTL_BLOCK_TRANS         0
#define MCI_DATACTL_STREAM_TRANS        4
#define MCI_DATACTL_DMA_ENABLE          (1 << 3)

#define INDX(CMD_INDX)    ((CMD_INDX & 0x3F) | MCI_CPSM_ENABLED)

#define MCI_CPSM_ENABLED            (1 << 10)
#define MCI_CPSM_WAIT_RESPONSE      (1 << 6)
#define MCI_CPSM_LONG_RESPONSE      (1 << 7)

#define MCI_TRACE(txt)  DEBUG((EFI_D_BLKIO, "ARM_MCI: " txt "\n"))

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

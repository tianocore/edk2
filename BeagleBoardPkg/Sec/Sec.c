/** @file
  C Entry point for the SEC. First C code after the reset vector.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/PrePiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/OmapLib.h>
#include <Library/ArmLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/DebugAgentLib.h>

#include <Ppi/GuidedSectionExtraction.h>
#include <Guid/LzmaDecompress.h>
#include <Omap3530/Omap3530.h>

#include "LzmaDecompress.h"

VOID
PadConfiguration (
  VOID
  );

VOID
ClockInit (
  VOID
  );


VOID
TimerInit (
  VOID
  )
{
  UINTN  Timer            = FixedPcdGet32(PcdOmap35xxFreeTimer);
  UINT32 TimerBaseAddress = TimerBase(Timer);

  // Set source clock for GPT3 & GPT4 to SYS_CLK
  MmioOr32 (CM_CLKSEL_PER, CM_CLKSEL_PER_CLKSEL_GPT3_SYS | CM_CLKSEL_PER_CLKSEL_GPT4_SYS);

  // Set count & reload registers
  MmioWrite32 (TimerBaseAddress + GPTIMER_TCRR, 0x00000000);
  MmioWrite32 (TimerBaseAddress + GPTIMER_TLDR, 0x00000000);

  // Disable interrupts
  MmioWrite32 (TimerBaseAddress + GPTIMER_TIER, TIER_TCAR_IT_DISABLE | TIER_OVF_IT_DISABLE | TIER_MAT_IT_DISABLE);

  // Start Timer
  MmioWrite32 (TimerBaseAddress + GPTIMER_TCLR, TCLR_AR_AUTORELOAD | TCLR_ST_ON);

  //Disable OMAP Watchdog timer (WDT2)
  MmioWrite32 (WDTIMER2_BASE + WSPR, 0xAAAA);
  DEBUG ((EFI_D_ERROR, "Magic delay to disable watchdog timers properly.\n"));
  MmioWrite32 (WDTIMER2_BASE + WSPR, 0x5555);
}

VOID
UartInit (
  VOID
  )
{
  UINTN   Uart            = FixedPcdGet32(PcdOmap35xxConsoleUart);
  UINT32  UartBaseAddress = UartBase(Uart);

  // Set MODE_SELECT=DISABLE before trying to initialize or modify DLL, DLH registers.
  MmioWrite32 (UartBaseAddress + UART_MDR1_REG, UART_MDR1_MODE_SELECT_DISABLE);

  // Put device in configuration mode.
  MmioWrite32 (UartBaseAddress + UART_LCR_REG, UART_LCR_DIV_EN_ENABLE);

  // Programmable divisor N = 48Mhz/16/115200 = 26
  MmioWrite32 (UartBaseAddress + UART_DLL_REG, 3000000/FixedPcdGet64 (PcdUartDefaultBaudRate)); // low divisor
  MmioWrite32 (UartBaseAddress + UART_DLH_REG,  0); // high divisor

  // Enter into UART operational mode.
  MmioWrite32 (UartBaseAddress + UART_LCR_REG, UART_LCR_DIV_EN_DISABLE | UART_LCR_CHAR_LENGTH_8);

  // Force DTR and RTS output to active
  MmioWrite32 (UartBaseAddress + UART_MCR_REG, UART_MCR_RTS_FORCE_ACTIVE | UART_MCR_DTR_FORCE_ACTIVE);

  // Clear & enable fifos
  MmioWrite32 (UartBaseAddress + UART_FCR_REG, UART_FCR_TX_FIFO_CLEAR | UART_FCR_RX_FIFO_CLEAR | UART_FCR_FIFO_ENABLE);

  // Restore MODE_SELECT
  MmioWrite32 (UartBaseAddress + UART_MDR1_REG, UART_MDR1_MODE_SELECT_UART_16X);
}

VOID
InitCache (
  IN  UINT32  MemoryBase,
  IN  UINT32  MemoryLength
  );

EFI_STATUS
EFIAPI
ExtractGuidedSectionLibConstructor (
  VOID
  );

EFI_STATUS
EFIAPI
LzmaDecompressLibConstructor (
  VOID
  );


VOID
CEntryPoint (
  IN  VOID  *MemoryBase,
  IN  UINTN MemorySize,
  IN  VOID  *StackBase,
  IN  UINTN StackSize
  )
{
  VOID *HobBase;

  // Build a basic HOB list
  HobBase      = (VOID *)(UINTN)(FixedPcdGet32(PcdEmbeddedFdBaseAddress) + FixedPcdGet32(PcdEmbeddedFdSize));
  CreateHobList (MemoryBase, MemorySize, HobBase, StackBase);

  //Set up Pin muxing.
  PadConfiguration ();

  // Set up system clocking
  ClockInit ();


  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction ();

  // Initialize CPU cache
  InitCache ((UINT32)MemoryBase, (UINT32)MemorySize);

  // Add memory allocation hob for relocated FD
  BuildMemoryAllocationHob (FixedPcdGet32(PcdEmbeddedFdBaseAddress), FixedPcdGet32(PcdEmbeddedFdSize), EfiBootServicesData);

  // Add the FVs to the hob list
  BuildFvHob (PcdGet32(PcdFlashFvMainBase), PcdGet32(PcdFlashFvMainSize));

  // Start talking
  UartInit ();

  InitializeDebugAgent (DEBUG_AGENT_INIT_PREMEM_SEC, NULL, NULL);
  SaveAndSetDebugTimerInterrupt (TRUE);

  DEBUG ((EFI_D_ERROR, "UART Enabled\n"));

  // Start up a free running timer so that the timer lib will work
  TimerInit ();

  // SEC phase needs to run library constructors by hand.
  ExtractGuidedSectionLibConstructor ();
  LzmaDecompressLibConstructor ();

  // Build HOBs to pass up our version of stuff the DXE Core needs to save space
  BuildPeCoffLoaderHob ();
  BuildExtractSectionHob (
    &gLzmaCustomDecompressGuid,
    LzmaGuidedSectionGetInfo,
    LzmaGuidedSectionExtraction
    );

  // Assume the FV that contains the SEC (our code) also contains a compressed FV.
  DecompressFirstFv ();

  // Load the DXE Core and transfer control to it
  LoadDxeCoreFromFv (NULL, 0);

  // DXE Core should always load and never return
  ASSERT (FALSE);
}


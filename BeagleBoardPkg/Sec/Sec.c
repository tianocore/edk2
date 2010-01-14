/** @file
  C Entry point for the SEC. First C code after the reset vector.

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
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

#include <Ppi/GuidedSectionExtraction.h>

#include <Omap3530/Omap3530.h>

VOID
EFIAPI 
_ModuleEntryPoint(
  VOID
  );

CHAR8 *
DeCygwinPathIfNeeded (
  IN  CHAR8   *Name
  );

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
  UINTN  Timer            = FixedPcdGet32(PcdBeagleFreeTimer);
  UINT32 TimerBaseAddress = TimerBase(Timer);

  // Set source clock for GPT3 & GPT4 to SYS_CLK
  MmioOr32(CM_CLKSEL_PER, CM_CLKSEL_PER_CLKSEL_GPT3_SYS 
                          | CM_CLKSEL_PER_CLKSEL_GPT4_SYS);

  // Set count & reload registers
  MmioWrite32(TimerBaseAddress + GPTIMER_TCRR, 0x00000000);
  MmioWrite32(TimerBaseAddress + GPTIMER_TLDR, 0x00000000);

  // Disable interrupts
  MmioWrite32(TimerBaseAddress + GPTIMER_TIER, TIER_TCAR_IT_DISABLE | TIER_OVF_IT_DISABLE | TIER_MAT_IT_DISABLE);

  // Start Timer
  MmioWrite32(TimerBaseAddress + GPTIMER_TCLR, TCLR_AR_AUTORELOAD | TCLR_ST_ON);

  //Disable OMAP Watchdog timer (WDT2)
  MmioWrite32(WDTIMER2_BASE + WSPR, 0xAAAA);
  DEBUG ((EFI_D_ERROR, "Magic delay to disable watchdog timers properly.\n"));
  MmioWrite32(WDTIMER2_BASE + WSPR, 0x5555);
}

VOID
UartInit (
  VOID
  )
{
  UINTN   Uart            = FixedPcdGet32(PcdBeagleConsoleUart);
  UINT32  UartBaseAddress = UartBase(Uart);

  // Set MODE_SELECT=DISABLE before trying to initialize or modify DLL, DLH registers.
  MmioWrite32(UartBaseAddress + UART_MDR1_REG, UART_MDR1_MODE_SELECT_DISABLE);

  // Put device in configuration mode.
  MmioWrite32(UartBaseAddress + UART_LCR_REG, UART_LCR_DIV_EN_ENABLE);

  // Programmable divisor N = 48Mhz/16/115200 = 26
  MmioWrite32(UartBaseAddress + UART_DLL_REG, 26); // low divisor
  MmioWrite32(UartBaseAddress + UART_DLH_REG,  0); // high divisor

  // Enter into UART operational mode.
  MmioWrite32(UartBaseAddress + UART_LCR_REG, UART_LCR_DIV_EN_DISABLE | UART_LCR_CHAR_LENGTH_8);

  // Force DTR and RTS output to active
  MmioWrite32(UartBaseAddress + UART_MCR_REG, UART_MCR_RTS_FORCE_ACTIVE | UART_MCR_DTR_FORCE_ACTIVE);

  // Clear & enable fifos
  MmioWrite32(UartBaseAddress + UART_FCR_REG, UART_FCR_TX_FIFO_CLEAR | UART_FCR_RX_FIFO_CLEAR | UART_FCR_FIFO_ENABLE);  

  // Restore MODE_SELECT 
  MmioWrite32(UartBaseAddress + UART_MDR1_REG, UART_MDR1_MODE_SELECT_UART_16X);
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

/**
  If the build is done on cygwin the paths are cygpaths. 
  /cygdrive/c/tmp.txt vs c:\tmp.txt so we need to convert
  them to work with RVD commands

  This is just code to help print out RVD symbol load command.
  If you build with cygwin paths aren't compatible with RVD.

  @param  Name  Path to convert if needed

**/
CHAR8 *
SecDeCygwinPathIfNeeded (
  IN  CHAR8   *Name
  )
{
  CHAR8   *Ptr;
  UINTN   Index;
  UINTN   Len;
  
  Ptr = AsciiStrStr (Name, "/cygdrive/");
  if (Ptr == NULL) {
    return Name;
  }
  
  Len = AsciiStrLen (Ptr);
  
  // convert "/cygdrive" to spaces
  for (Index = 0; Index < 9; Index++) {
    Ptr[Index] = ' ';
  }

  // convert /c to c:
  Ptr[9]  = Ptr[10];
  Ptr[10] = ':';
  
  // switch path seperators
  for (Index = 11; Index < Len; Index++) {
    if (Ptr[Index] == '/') {
      Ptr[Index] = '\\' ;
    }
  }

  return Name;
}


VOID
CEntryPoint (
  IN  VOID  *MemoryBase,
  IN  UINTN MemorySize,
  IN  VOID  *StackBase,
  IN  UINTN StackSize
  )
{
  VOID *HobBase;

  //Set up Pin muxing.
  PadConfiguration();

  // Set up system clocking
  ClockInit();

  // Build a basic HOB list
  HobBase      = (VOID *)(UINTN)(FixedPcdGet32(PcdEmbeddedFdBaseAddress) + FixedPcdGet32(PcdEmbeddedFdSize));
  CreateHobList(MemoryBase, MemorySize, HobBase, StackBase);

  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction();

  // Initialize CPU cache
  InitCache((UINT32)MemoryBase, (UINT32)MemorySize);

  // Add memory allocation hob for relocated FD
  BuildMemoryAllocationHob(FixedPcdGet32(PcdEmbeddedFdBaseAddress), FixedPcdGet32(PcdEmbeddedFdSize), EfiBootServicesData);

  // Add the FVs to the hob list
  BuildFvHob(PcdGet32(PcdFlashFvMainBase), PcdGet32(PcdFlashFvMainSize));

  // Start talking
  UartInit();
  DEBUG((EFI_D_ERROR, "UART Enabled\n"));

  DEBUG_CODE_BEGIN ();
    //
    // On a debug build print out information about the SEC. This is really info about
    // the PE/COFF file we are currently running from. Useful for loading symbols in a
    // debugger. Remember our image is really part of the FV.
    //
    RETURN_STATUS       Status;
    EFI_PEI_FV_HANDLE   VolumeHandle;
    EFI_PEI_FILE_HANDLE FileHandle;
    VOID                *PeCoffImage;
    UINT32              Offset;
    CHAR8               *FilePath;

    FfsAnyFvFindFirstFile (EFI_FV_FILETYPE_SECURITY_CORE, &VolumeHandle, &FileHandle);
    Status = FfsFindSectionData (EFI_SECTION_TE, FileHandle, &PeCoffImage);
    if (EFI_ERROR (Status)) {
      // Usually is a TE (PI striped down PE/COFF), but could be a full PE/COFF
      Status = FfsFindSectionData (EFI_SECTION_PE32, FileHandle, &PeCoffImage);
    }
    if (!EFI_ERROR (Status)) {
      Offset = PeCoffGetSizeOfHeaders (PeCoffImage);
      FilePath = PeCoffLoaderGetPdbPointer (PeCoffImage);
      if (FilePath != NULL) {
      
        // 
        // In general you should never have to use #ifdef __CC_ARM in the code. It
        // is hidden in the away in the MdePkg. But here we would like to print differnt things
        // for different toolchains. 
        //
#ifdef __CC_ARM
        // Print out the command for the RVD debugger to load symbols for this image
        DEBUG ((EFI_D_ERROR, "load /a /ni /np %a &0x%08x\n", SecDeCygwinPathIfNeeded (FilePath), PeCoffImage + Offset));
#elif __GNUC__
        // This may not work correctly if you generate PE/COFF directlyas then the Offset would not be required
        DEBUG ((EFI_D_ERROR, "add-symbol-file %a 0x%08x\n", FilePath, PeCoffImage + Offset));
#else
        DEBUG ((EFI_D_ERROR, "SEC starts at 0x%08x with an entry point at 0x%08x %a\n", PeCoffImage, _ModuleEntryPoint, FilePath));
#endif
      }
    }

   DEBUG_CODE_END ();



  // Start up a free running time so that the timer lib will work
  TimerInit();

  // SEC phase needs to run library constructors by hand.
  ExtractGuidedSectionLibConstructor();
  LzmaDecompressLibConstructor();

  // Load the DXE Core and transfer control to it
  LoadDxeCoreFromFv(NULL, 0);
  
  // DXE Core should always load and never return
  ASSERT(FALSE);
}


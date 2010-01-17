/** @file

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h" 

EFI_DEBUG_IMAGE_INFO_TABLE_HEADER *gDebugImageTableHeader = NULL;

VOID
ExceptionHandlersStart (
  VOID
  );

VOID
ExceptionHandlersEnd (
  VOID
  );

VOID
CommonExceptionEntry (
  VOID
  );

VOID
AsmCommonExceptionEntry (
  VOID
  );


EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_ARM_EXCEPTION + 1];
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_ARM_EXCEPTION + 1];



/**
  This function registers and enables the handler specified by InterruptHandler for a processor 
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the 
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled. 
  The installed handler is called once for each processor interrupt or exception.

  @param  InterruptType    A pointer to the processor's current interrupt state. Set to TRUE if interrupts
                           are enabled and FALSE if interrupts are disabled.
  @param  InterruptHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                           when a processor interrupt occurs. If this parameter is NULL, then the handler
                           will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
RegisterInterruptHandler (
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
{
  if (InterruptType > MAX_ARM_EXCEPTION) {
    return EFI_UNSUPPORTED;
  }

  if ((InterruptHandler != NULL) && (gExceptionHandlers[InterruptType] != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  gExceptionHandlers[InterruptType] = InterruptHandler;

  return EFI_SUCCESS;
}


/**
  This function registers and enables the handler specified by InterruptHandler for a processor 
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the 
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled. 
  The installed handler is called once for each processor interrupt or exception.

  @param  InterruptType    A pointer to the processor's current interrupt state. Set to TRUE if interrupts
                           are enabled and FALSE if interrupts are disabled.
  @param  InterruptHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                           when a processor interrupt occurs. If this parameter is NULL, then the handler
                           will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
RegisterDebuggerInterruptHandler (
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
{
  if (InterruptType > MAX_ARM_EXCEPTION) {
    return EFI_UNSUPPORTED;
  }

  if ((InterruptHandler != NULL) && (gDebuggerExceptionHandlers[InterruptType] != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  gDebuggerExceptionHandlers[InterruptType] = InterruptHandler;

  return EFI_SUCCESS;
}




CHAR8 *
GetImageName (
  IN  UINT32  FaultAddress,
  OUT UINT32  *ImageBase,
  OUT UINT32  *PeCoffSizeOfHeaders
  )
{
  EFI_DEBUG_IMAGE_INFO  *DebugTable;
  UINTN                 Entry;
  CHAR8                 *Address;

  
  DebugTable = gDebugImageTableHeader->EfiDebugImageInfoTable;
  if (DebugTable == NULL) {
    return NULL;
  }

  Address = (CHAR8 *)(UINTN)FaultAddress;
  for (Entry = 0; Entry < gDebugImageTableHeader->TableSize; Entry++, DebugTable++) {
    if (DebugTable->NormalImage != NULL) {
      if ((DebugTable->NormalImage->ImageInfoType == EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL) && 
          (DebugTable->NormalImage->LoadedImageProtocolInstance != NULL)) {
        if ((Address >= (CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase) &&
            (Address <= ((CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase + DebugTable->NormalImage->LoadedImageProtocolInstance->ImageSize))) {
          *ImageBase = (UINT32)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase;
          *PeCoffSizeOfHeaders = PeCoffGetSizeOfHeaders ((VOID *)(UINTN)*ImageBase);
          return PeCoffLoaderGetPdbPointer (DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase);
        }           
      }
    }  
  }

  return NULL;
}

typedef struct {
  UINT32  Bit;
  CHAR8   Char;
} CPSR_CHAR;


VOID
CpsrString (
  IN  UINT32  Cpsr,
  OUT CHAR8   *ReturnStr
  )
{
  UINTN Index;
  CHAR8 *Str = ReturnStr;
  CHAR8 *ModeStr;
  CPSR_CHAR CpsrChar[] = {
    { 31, 'n' },
    { 30, 'z' },
    { 29, 'c' },
    { 28, 'v' },

    { 9,  'e' },
    { 8,  'a' },
    { 7,  'i' },
    { 6,  'f' },
    { 5,  't' },
    { 0,  '?' }
  };
  
  for (Index = 0; CpsrChar[Index].Bit != 0; Index++, Str++) {
    *Str = CpsrChar[Index].Char;
    if ((Cpsr & (1 << CpsrChar[Index].Bit)) != 0) {
      // Concert to upper case if bit is set
      *Str &= ~0x20;
    }
  }
  
  *Str++ = '_';
  *Str = '\0';
  
  switch (Cpsr & 0x1f) {
  case 0x17:
    ModeStr = "abt";
    break;
  case 0x011:
    ModeStr = "fiq";
    break;
  case 0x12:
    ModeStr = "irq";
    break;
  case 0x13:
    ModeStr = "svc";
    break;
  case 0x1f:
    ModeStr = "sys";
    break;
  case 0x1b:
    ModeStr = "und";
    break;
  case 0x10:
    ModeStr = "usr";
    break;
  
  default:
    ModeStr = "???";
    break;
  }
  
  AsciiStrCat (Str, ModeStr);
  return;
}  
  
CHAR8 *gExceptionTypeString[] = {
  "Reset",
  "Undefined Instruction",
  "SWI",
  "Prefetch Abort",
  "Data Abort",
  "Undefined",
  "IRQ",
  "FIQ"
};

VOID
EFIAPI
CommonCExceptionHandler (
  IN     EFI_EXCEPTION_TYPE           ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  BOOLEAN Dispatched = FALSE;
 
 
  if (ExceptionType <= MAX_ARM_EXCEPTION) {
    if (gDebuggerExceptionHandlers[ExceptionType]) {
      //
      // If DebugSupport hooked the interrupt call the handler. This does not disable 
      // the normal handler.
      //
      gDebuggerExceptionHandlers[ExceptionType] (ExceptionType, SystemContext);
      Dispatched = TRUE;
    }
    if (gExceptionHandlers[ExceptionType]) {
      gExceptionHandlers[ExceptionType] (ExceptionType, SystemContext);
      Dispatched = TRUE;
    }
  } else {
    DEBUG ((EFI_D_ERROR, "Unknown exception type %d from %08x\n", ExceptionType, SystemContext.SystemContextArm->PC));
    ASSERT (FALSE);
  }

  if (Dispatched) {
    //
    // We did work so this was an expected ExceptionType
    //
    return;
  }
  
  if (ExceptionType == EXCEPT_ARM_SOFTWARE_INTERRUPT) {
    //
    // ARM JTAG debuggers some times use this vector, so it is not an error to get one
    //
    return;
  }

  //
  // Code after here is the default exception handler... Dump the context
  //
  DEBUG ((EFI_D_ERROR, "\n%a Exception PC at 0x%08x  CPSR 0x%08x ", gExceptionTypeString[ExceptionType], SystemContext.SystemContextArm->PC, SystemContext.SystemContextArm->CPSR));
  DEBUG_CODE_BEGIN ();
    CHAR8   *Pdb;
    UINT32  ImageBase;
    UINT32  PeCoffSizeOfHeader;
    UINT32  Offset;
    CHAR8   CpsrStr[32];  // char per bit. Lower 5-bits are mode that is a 3 char string
    
    CpsrString (SystemContext.SystemContextArm->CPSR, CpsrStr);
    DEBUG ((EFI_D_ERROR, "%a\n", CpsrStr));
  
    Pdb = GetImageName (SystemContext.SystemContextArm->PC, &ImageBase, &PeCoffSizeOfHeader);
    Offset = SystemContext.SystemContextArm->PC - ImageBase;
    if (Pdb != NULL) {
      DEBUG ((EFI_D_ERROR, "%a\n", Pdb));

      //
      // A PE/COFF image loads its headers into memory so the headers are 
      // included in the linked addressess. ELF and Mach-O images do not
      // include the headers so the first byte of the image is usually
      // text (code). If you look at link maps from ELF or Mach-O images
      // you need to subtact out the size of the PE/COFF header to get
      // get the offset that matches the link map. 
      //
      DEBUG ((EFI_D_ERROR, "loaded at 0x%08x (PE/COFF offset) 0x%x (ELF or Mach-O offset) 0x%x", ImageBase, Offset, Offset - PeCoffSizeOfHeader));
      
      // If we come from an image it is safe to show the instruction. We know it should not fault
      if ((SystemContext.SystemContextArm->CPSR & 0x20) == 0) {
        // ARM
        DEBUG ((EFI_D_ERROR, "\nFaulting Instruction 0x%08x", *(UINT32 *)(UINTN)SystemContext.SystemContextArm->PC));
      } else {
        // Thumb
        DEBUG ((EFI_D_ERROR, "\nFaulting Instruction 0x%04x", *(UINT16 *)(UINTN)SystemContext.SystemContextArm->PC));
      }
    }
  DEBUG_CODE_END ();
  DEBUG ((EFI_D_ERROR, "\n  R0 0x%08x   R1 0x%08x   R2 0x%08x   R3 0x%08x\n", SystemContext.SystemContextArm->R0, SystemContext.SystemContextArm->R1, SystemContext.SystemContextArm->R2, SystemContext.SystemContextArm->R3));
  DEBUG ((EFI_D_ERROR, "  R4 0x%08x   R5 0x%08x   R6 0x%08x   R7 0x%08x\n", SystemContext.SystemContextArm->R4, SystemContext.SystemContextArm->R5, SystemContext.SystemContextArm->R6, SystemContext.SystemContextArm->R7));
  DEBUG ((EFI_D_ERROR, "  R8 0x%08x   R9 0x%08x  R10 0x%08x  R11 0x%08x\n", SystemContext.SystemContextArm->R8, SystemContext.SystemContextArm->R9, SystemContext.SystemContextArm->R10, SystemContext.SystemContextArm->R11));
  DEBUG ((EFI_D_ERROR, " R12 0x%08x   SP 0x%08x   LR 0x%08x   PC 0x%08x\n", SystemContext.SystemContextArm->R12, SystemContext.SystemContextArm->SP, SystemContext.SystemContextArm->LR, SystemContext.SystemContextArm->PC));
  DEBUG ((EFI_D_ERROR, "DFSR 0x%08x DFAR 0x%08x IFSR 0x%08x IFAR 0x%08x\n\n", SystemContext.SystemContextArm->DFSR, SystemContext.SystemContextArm->DFAR, SystemContext.SystemContextArm->IFSR, SystemContext.SystemContextArm->IFAR));

  ASSERT (FALSE);
}



EFI_STATUS
InitializeExceptions (
  IN EFI_CPU_ARCH_PROTOCOL    *Cpu
  )
{
  EFI_STATUS           Status;
  UINTN                Offset;
  UINTN                Length;
  UINTN                Index;
  BOOLEAN              Enabled;
  EFI_PHYSICAL_ADDRESS Base;

  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&gDebugImageTableHeader);
  if (EFI_ERROR (Status)) {
    gDebugImageTableHeader = NULL;
  }

  //
  // Disable interrupts
  //
  Cpu->GetInterruptState (Cpu, &Enabled);
  Cpu->DisableInterrupt (Cpu);

  //
  // Initialize the C entry points for interrupts
  //
  for (Index = 0; Index <= MAX_ARM_EXCEPTION; Index++) {
    Status = RegisterInterruptHandler (Index, NULL);
    ASSERT_EFI_ERROR (Status);
    
    Status = RegisterDebuggerInterruptHandler (Index, NULL);
    ASSERT_EFI_ERROR (Status);
  }
  
  //
  // Copy an implementation of the ARM exception vectors to PcdCpuVectorBaseAddress.
  //
  Length = (UINTN)ExceptionHandlersEnd - (UINTN)ExceptionHandlersStart;

  //
  // Reserve space for the exception handlers
  //
  Base = (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdCpuVectorBaseAddress);
  Status = gBS->AllocatePages (AllocateAddress, EfiBootServicesCode, EFI_SIZE_TO_PAGES (Length), &Base);
  // If the request was for memory that's not in the memory map (which is often the case for 0x00000000
  // on embedded systems, for example, we don't want to hang up.  So we'll check here for a status of 
  // EFI_NOT_FOUND, and continue in that case.
  if (EFI_ERROR(Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
  }

  CopyMem ((VOID *)(UINTN)PcdGet32 (PcdCpuVectorBaseAddress), (VOID *)ExceptionHandlersStart, Length);

  //
  // Patch in the common Assembly exception handler
  //
  Offset = (UINTN)CommonExceptionEntry - (UINTN)ExceptionHandlersStart;
  *(UINTN *) ((UINT8 *)(UINTN)PcdGet32 (PcdCpuVectorBaseAddress) + Offset) = (UINTN)AsmCommonExceptionEntry;

  // Flush Caches since we updated executable stuff
  InvalidateInstructionCacheRange ((VOID *)PcdGet32(PcdCpuVectorBaseAddress), Length);

  if (Enabled) {
    // 
    // Restore interrupt state
    //
    Status = Cpu->EnableInterrupt (Cpu);
  }

  return Status;
}

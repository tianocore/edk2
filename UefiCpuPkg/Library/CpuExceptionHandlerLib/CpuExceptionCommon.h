/** @file
  Common header file for CPU Exception Handler Library.

  Copyright (c) 2012 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_EXCEPTION_COMMON_H_
#define _CPU_EXCEPTION_COMMON_H_

#include <Ppi/VectorHandoffInfo.h>
#include <Protocol/Cpu.h>
#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>
#include <Library/LocalApicLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SynchronizationLib.h>

#define  CPU_EXCEPTION_NUM          32
#define  CPU_INTERRUPT_NUM         256
#define  HOOKAFTER_STUB_SIZE        16

#include "ArchInterruptDefs.h"

//
// Record exception handler information
//
typedef struct {
  UINTN ExceptionStart;
  UINTN ExceptionStubHeaderSize;
  UINTN HookAfterStubHeaderStart;
} EXCEPTION_HANDLER_TEMPLATE_MAP;

extern CONST UINT32                mErrorCodeFlag;
extern CONST UINTN                 mImageAlignSize;
extern CONST UINTN                 mDoFarReturnFlag;
extern RESERVED_VECTORS_DATA       *mReservedVectors;

/**
  Return address map of exception handler template so that C code can generate
  exception tables.

  @param AddressMap  Pointer to a buffer where the address map is returned.
**/
VOID
EFIAPI
AsmGetTemplateAddressMap (
  OUT EXCEPTION_HANDLER_TEMPLATE_MAP *AddressMap
  );

/**
  Return address map of exception handler template so that C code can generate
  exception tables.

  @param IdtEntry          Pointer to IDT entry to be updated.
  @param InterruptHandler  IDT handler value.

**/
VOID
ArchUpdateIdtEntry (
  IN IA32_IDT_GATE_DESCRIPTOR        *IdtEntry,
  IN UINTN                           InterruptHandler
  );

/**
  Read IDT handler value from IDT entry.

  @param IdtEntry          Pointer to IDT entry to be read.

**/
UINTN
ArchGetIdtHandler (
  IN IA32_IDT_GATE_DESCRIPTOR        *IdtEntry
  );

/**
  Prints a message to the serial port.

  @param  Format      Format string for the message to print.
  @param  ...         Variable argument list whose contents are accessed 
                      based on the format string specified by Format.

**/
VOID
EFIAPI
InternalPrintMessage (
  IN  CONST CHAR8  *Format,
  ...
  );

/**
  Find and display image base address and return image base and its entry point.
  
  @param CurrentEip      Current instruction pointer.
  @param EntryPoint      Return module entry point if module header is found.
  
  @return !0     Image base address.
  @return 0      Image header cannot be found.
**/
UINTN 
FindModuleImageBase (
  IN  UINTN              CurrentEip,
  OUT UINTN              *EntryPoint
  );

/**
  Display CPU information.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
DumpCpuContent (
  IN EFI_EXCEPTION_TYPE   ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  );

/**
  Internal worker function to initialize exception handler.

  @param[in]  VectorInfo    Pointer to reserved vector list.
  
  @retval EFI_SUCCESS           CPU Exception Entries have been successfully initialized 
                                with default exception handlers.
  @retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
InitializeCpuExceptionHandlersWorker (
  IN EFI_VECTOR_HANDOFF_INFO       *VectorInfo OPTIONAL
  );

/**
  Registers a function to be called from the processor interrupt handler.

  @param[in]  InterruptType     Defines which interrupt or exception to hook.
  @param[in]  InterruptHandler  A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                                when a processor interrupt occurs. If this parameter is NULL, then the handler
                                will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported,
                                or this function is not supported.
**/
EFI_STATUS
RegisterCpuInterruptHandlerWorker (
  IN EFI_EXCEPTION_TYPE            InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  );

/**
  Internal worker function to update IDT entries accordling to vector attributes.

  @param[in] IdtTable       Pointer to IDT table.
  @param[in] TemplateMap    Pointer to a buffer where the address map is returned.
  @param[in] IdtEntryCount  IDT entries number to be updated.

**/
VOID
UpdateIdtTable (
  IN IA32_IDT_GATE_DESCRIPTOR        *IdtTable,
  IN EXCEPTION_HANDLER_TEMPLATE_MAP  *TemplateMap,
  IN UINTN                           IdtEntryCount
  );

/**
  Save CPU exception context when handling EFI_VECTOR_HANDOFF_HOOK_AFTER case.

  @param[in] ExceptionType  Exception type.
  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
ArchSaveExceptionContext (
  IN UINTN                ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext 
  );

/**
  Restore CPU exception context when handling EFI_VECTOR_HANDOFF_HOOK_AFTER case.

  @param[in] ExceptionType  Exception type.
  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
ArchRestoreExceptionContext (
  IN UINTN                ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext 
  );

/**
  Fix up the vector number and function address in the vector code.
 
  @param[in] NewVectorAddr   New vector handler address.
  @param[in] VectorNum       Index of vector.
  @param[in] OldVectorAddr   Old vector handler address.

**/
VOID
EFIAPI
AsmVectorNumFixup (
  IN VOID    *NewVectorAddr,
  IN UINT8   VectorNum,
  IN VOID    *OldVectorAddr
  );

/**
  Read and save reserved vector information
  
  @param[in]  VectorInfo        Pointer to reserved vector list.
  @param[out] ReservedVector    Pointer to reserved vector data buffer.
  @param[in]  VectorCount       Vector number to be updated.
  
  @return EFI_SUCCESS           Read and save vector info successfully.
  @retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.

**/
EFI_STATUS
ReadAndVerifyVectorInfo (
  IN  EFI_VECTOR_HANDOFF_INFO       *VectorInfo,
  OUT RESERVED_VECTORS_DATA         *ReservedVector,
  IN  UINTN                         VectorCount
  );

#endif


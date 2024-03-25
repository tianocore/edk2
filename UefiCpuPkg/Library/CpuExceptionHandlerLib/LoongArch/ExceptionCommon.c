/** @file DxeExceptionLib.c

  CPU Exception Handler Library common functions.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Register/LoongArch64/Csr.h>
#include "ExceptionCommon.h"

CONST CHAR8  mExceptionReservedStr[] = "Reserved";
CONST CHAR8  *mExceptionNameStr[]    = {
  "#INT - Interrupt(CSR.ECFG.VS=0)",
  "#PIL - Page invalid exception for Load option",
  "#PIS - Page invalid exception for Store operation",
  "#PIF - Page invalid exception for Fetch operation",
  "#PME - Page modification exception",
  "#PNR - Page non-readable exception",
  "#PNX - Page non-executable exception",
  "#PPI - Page privilege level illegal exception",
  "#ADE - Address error exception",
  "#ALE - Address alignment fault exception",
  "#BCE - Bound check exception",
  "#SYS - System call exception",
  "#BRK - Beeakpoint exception",
  "#INE - Instruction non-defined exception",
  "#IPE - Instruction privilege error exception",
  "#FPD - Floating-point instruction disable exception",
  "#SXD - 128-bit vector (SIMD instructions) expansion instruction disable exception",
  "#ASXD - 256-bit vector (Advanced SIMD instructions) expansion instruction disable exception",
  "#FPE - Floating-Point error exception",
  "#WPE - WatchPoint Exception for Fetch watchpoint or Memory load/store watchpoint",
  "#BTD - Binary Translation expansion instruction Disable exception",
  "#BTE - Binary Translation related exceptions",
  "#GSPR - Guest Sensitive Privileged Resource exception",
  "#HVC - HyperVisor Call exception",
  "#GCXC - Guest CSR Software/Hardware Change exception",
  "#TBR - TLB refill exception" // !!! NOTICE: Because the TLB refill exception is not instructed in ECODE, so the TLB refill exception must be the last one!
};

INTN  mExceptionKnownNameNum = (sizeof (mExceptionNameStr) / sizeof (CHAR8 *));

/**
  Get ASCII format string exception name by exception type.

  @param ExceptionType  Exception type.

  @return  ASCII format string exception name.

**/
CONST CHAR8 *
GetExceptionNameStr (
  IN EFI_EXCEPTION_TYPE  ExceptionType
  )
{
  if ((UINTN)ExceptionType < mExceptionKnownNameNum) {
    return mExceptionNameStr[ExceptionType];
  } else {
    return mExceptionReservedStr;
  }
}

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
  )
{
  CHAR8    Buffer[MAX_DEBUG_MESSAGE_LENGTH];
  VA_LIST  Marker;

  //
  // Convert the message to an ASCII String
  //
  VA_START (Marker, Format);
  AsciiVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  //
  // Send the print string to a Serial Port
  //
  SerialPortWrite ((UINT8 *)Buffer, AsciiStrLen (Buffer));
}

/**
  Find and display image base address and return image base and its entry point.

  @param CurrentEra      Current instruction pointer.

**/
VOID
DumpModuleImageInfo (
  IN  UINTN  CurrentEra
  )
{
  EFI_STATUS  Status;
  UINTN       Pe32Data;
  VOID        *PdbPointer;
  VOID        *EntryPoint;

  Pe32Data = PeCoffSearchImageBase (CurrentEra);
  if (Pe32Data == 0) {
    InternalPrintMessage ("!!!! Can't find image information. !!!!\n");
  } else {
    //
    // Find Image Base entry point
    //
    Status = PeCoffLoaderGetEntryPoint ((VOID *)Pe32Data, &EntryPoint);
    if (EFI_ERROR (Status)) {
      EntryPoint = NULL;
    }

    InternalPrintMessage ("!!!! Find image based on IP(0x%x) ", CurrentEra);
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)Pe32Data);
    if (PdbPointer != NULL) {
      InternalPrintMessage ("%a", PdbPointer);
    } else {
      InternalPrintMessage ("(No PDB) ");
    }

    InternalPrintMessage (
      " (ImageBase=%016lp, EntryPoint=%016p) !!!!\n",
      (VOID *)Pe32Data,
      EntryPoint
      );
  }
}

/**
  Default exception handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
DefaultExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  //
  // Initialize the serial port before dumping.
  //
  SerialPortInitialize ();
  //
  // Display ExceptionType, CPU information and Image information
  //
  DumpImageAndCpuContent (ExceptionType, SystemContext);

  //
  // Enter a dead loop.
  //
  CpuDeadLoop ();
}

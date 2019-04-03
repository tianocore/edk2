/** @file
  CPU Exception Handler Library common functions.

  Copyright (c) 2012 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuExceptionCommon.h"

//
// Error code flag indicating whether or not an error code will be
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
CONST UINT32 mErrorCodeFlag = 0x00227d00;

//
// Define the maximum message length
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

CONST CHAR8 mExceptionReservedStr[] = "Reserved";
CONST CHAR8 *mExceptionNameStr[] = {
  "#DE - Divide Error",
  "#DB - Debug",
  "NMI Interrupt",
  "#BP - Breakpoint",
  "#OF - Overflow",
  "#BR - BOUND Range Exceeded",
  "#UD - Invalid Opcode",
  "#NM - Device Not Available",
  "#DF - Double Fault",
  "Coprocessor Segment Overrun",
  "#TS - Invalid TSS",
  "#NP - Segment Not Present",
  "#SS - Stack Fault Fault",
  "#GP - General Protection",
  "#PF - Page-Fault",
  "Reserved",
  "#MF - x87 FPU Floating-Point Error",
  "#AC - Alignment Check",
  "#MC - Machine-Check",
  "#XM - SIMD floating-point",
  "#VE - Virtualization",
  "#CP - Control Protection"
};

#define EXCEPTION_KNOWN_NAME_NUM  (sizeof (mExceptionNameStr) / sizeof (CHAR8 *))

/**
  Get ASCII format string exception name by exception type.

  @param ExceptionType  Exception type.

  @return  ASCII format string exception name.
**/
CONST CHAR8 *
GetExceptionNameStr (
  IN EFI_EXCEPTION_TYPE          ExceptionType
  )
{
  if ((UINTN) ExceptionType < EXCEPTION_KNOWN_NAME_NUM) {
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

  @param CurrentEip      Current instruction pointer.

**/
VOID
DumpModuleImageInfo (
  IN  UINTN              CurrentEip
  )
{
  EFI_STATUS                           Status;
  UINTN                                Pe32Data;
  VOID                                 *PdbPointer;
  VOID                                 *EntryPoint;

  Pe32Data = PeCoffSearchImageBase (CurrentEip);
  if (Pe32Data == 0) {
    InternalPrintMessage ("!!!! Can't find image information. !!!!\n");
  } else {
    //
    // Find Image Base entry point
    //
    Status = PeCoffLoaderGetEntryPoint ((VOID *) Pe32Data, &EntryPoint);
    if (EFI_ERROR (Status)) {
      EntryPoint = NULL;
    }
    InternalPrintMessage ("!!!! Find image based on IP(0x%x) ", CurrentEip);
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *) Pe32Data);
    if (PdbPointer != NULL) {
      InternalPrintMessage ("%a", PdbPointer);
    } else {
      InternalPrintMessage ("(No PDB) " );
    }
    InternalPrintMessage (
      " (ImageBase=%016lp, EntryPoint=%016p) !!!!\n",
      (VOID *) Pe32Data,
      EntryPoint
      );
  }
}

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
  )
{
  while (VectorInfo->Attribute != EFI_VECTOR_HANDOFF_LAST_ENTRY) {
    if (VectorInfo->Attribute > EFI_VECTOR_HANDOFF_HOOK_AFTER) {
      //
      // If vector attrubute is invalid
      //
      return EFI_INVALID_PARAMETER;
    }
    if (VectorInfo->VectorNumber < VectorCount) {
      ReservedVector[VectorInfo->VectorNumber].Attribute = VectorInfo->Attribute;
    }
    VectorInfo ++;
  }
  return EFI_SUCCESS;
}

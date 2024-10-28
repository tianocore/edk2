/** @file

  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <FspGlobalData.h>
#include <FspEas.h>
#include <Library/FspSwitchStackLib.h>

#pragma pack(1)

typedef struct {
  UINT16    IdtrLimit;
  UINT32    IdtrBase;
  UINT16    Reserved;
  UINT32    Cr0;
  UINT32    Cr3;
  UINT32    Cr4;
  UINT32    Efer;           // lower 32-bit of EFER since only NXE bit (BIT11) need to be restored.
  UINT32    Registers[8];   // General Purpose Registers: Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx and Eax
  UINT16    Flags[2];
  UINT32    FspInfoHeader;
  UINT32    ApiRet;
  UINT32    ApiParam[2];
} CONTEXT_STACK;

typedef struct {
  UINT64    Idtr[2];        // IDTR Limit - bit0:bi15, IDTR Base - bit16:bit79
  UINT64    Cr0;
  UINT64    Cr3;
  UINT64    Cr4;
  UINT64    Efer;
  UINT64    Registers[16];  // General Purpose Registers: RDI, RSI, RBP, RSP, RBX, RDX, RCX, RAX, and R15 to R8
  UINT32    Flags[2];
  UINT64    FspInfoHeader;
  UINT64    ApiParam[2];
  UINT64    Reserved;       // The reserved QWORD is needed for stack alignment in X64.
  UINT64    ApiRet;         // 64bit stack format is different from the 32bit one due to x64 calling convention
} CONTEXT_STACK_64;

#define CONTEXT_STACK_OFFSET(x)  (sizeof(UINTN) == sizeof (UINT32) ? (UINTN)&((CONTEXT_STACK *)(UINTN)0)->x : (UINTN)&((CONTEXT_STACK_64 *)(UINTN)0)->x)

#pragma pack()

/**
  This function sets the FSP global data pointer.

  @param[in] FspData       FSP global data pointer.

**/
VOID
EFIAPI
SetFspGlobalDataPointer (
  IN FSP_GLOBAL_DATA  *FspData
  )
{
  ASSERT (FspData != NULL);
  *((volatile UINT32 *)(UINTN)PcdGet32 (PcdGlobalDataPointerAddress)) = (UINT32)(UINTN)FspData;
}

/**
  This function gets the FSP global data pointer.

**/
FSP_GLOBAL_DATA *
EFIAPI
GetFspGlobalDataPointer (
  VOID
  )
{
  UINT32  FspDataAddress;

  FspDataAddress = *(UINT32 *)(UINTN)PcdGet32 (PcdGlobalDataPointerAddress);
  return (FSP_GLOBAL_DATA *)(UINTN)FspDataAddress;
}

/**
  This function gets back the FSP API first parameter passed by the bootloader.

  @retval ApiParameter FSP API first parameter passed by the bootloader.
**/
UINTN
EFIAPI
GetFspApiParameter (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return *(UINTN *)(FspData->CoreStack + CONTEXT_STACK_OFFSET (ApiParam[0]));
}

/**
  This function returns the FSP entry stack pointer from address of the first API parameter.

  @retval FSP entry stack pointer.
**/
VOID *
EFIAPI
GetFspEntryStack (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return (VOID *)(FspData->CoreStack + CONTEXT_STACK_OFFSET (ApiParam[0]));
}

/**
  This function gets back the FSP API second parameter passed by the bootloader.

  @retval ApiParameter FSP API second parameter passed by the bootloader.
**/
UINTN
EFIAPI
GetFspApiParameter2 (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return *(UINTN *)(FspData->CoreStack + CONTEXT_STACK_OFFSET (ApiParam[1]));
}

/**
  This function sets the FSP API parameter in the stack.

   @param[in] Value       New parameter value.

**/
VOID
EFIAPI
SetFspApiParameter (
  IN UINT32  Value
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData                                                          = GetFspGlobalDataPointer ();
  *(UINTN *)(FspData->CoreStack + CONTEXT_STACK_OFFSET (ApiParam)) = Value;
}

/**
  This function set the API status code returned to the BootLoader.

  @param[in] ReturnStatus       Status code to return.

**/
VOID
EFIAPI
SetFspApiReturnStatus (
  IN UINTN  ReturnStatus
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData                                                              = GetFspGlobalDataPointer ();
  *(UINTN *)(FspData->CoreStack + CONTEXT_STACK_OFFSET (Registers[7])) = ReturnStatus;
}

/**
  This function sets the context switching stack to a new stack frame.

  @param[in] NewStackTop       New core stack to be set.

**/
VOID
EFIAPI
SetFspCoreStackPointer (
  IN VOID  *NewStackTop
  )
{
  FSP_GLOBAL_DATA  *FspData;
  UINTN            *OldStack;
  UINTN            *NewStack;
  UINT32           StackContextLen;

  FspData         = GetFspGlobalDataPointer ();
  StackContextLen = sizeof (CONTEXT_STACK) / sizeof (UINTN);

  //
  // Reserve space for the ContinuationFunc two parameters
  //
  OldStack           = (UINTN *)FspData->CoreStack;
  NewStack           = (UINTN *)NewStackTop - StackContextLen - 2;
  FspData->CoreStack = (UINTN)NewStack;
  while (StackContextLen-- != 0) {
    *NewStack++ = *OldStack++;
  }
}

/**
  This function sets the platform specific data pointer.

  @param[in] PlatformData       FSP platform specific data pointer.

**/
VOID
EFIAPI
SetFspPlatformDataPointer (
  IN VOID  *PlatformData
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData                       = GetFspGlobalDataPointer ();
  FspData->PlatformData.DataPtr = PlatformData;
}

/**
  This function gets the platform specific data pointer.

   @param[in] PlatformData       FSP platform specific data pointer.

**/
VOID *
EFIAPI
GetFspPlatformDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return FspData->PlatformData.DataPtr;
}

/**
  This function sets the UPD data pointer.

  @param[in] UpdDataPtr   UPD data pointer.
**/
VOID
EFIAPI
SetFspUpdDataPointer (
  IN VOID  *UpdDataPtr
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Get the FSP Global Data Pointer
  //
  FspData = GetFspGlobalDataPointer ();

  //
  // Set the UPD pointer.
  //
  FspData->UpdDataPtr = UpdDataPtr;
}

/**
  This function gets the UPD data pointer.

  @return UpdDataPtr   UPD data pointer.
**/
VOID *
EFIAPI
GetFspUpdDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return FspData->UpdDataPtr;
}

/**
  This function sets the FspMemoryInit UPD data pointer.

  @param[in] MemoryInitUpdPtr   FspMemoryInit UPD data pointer.
**/
VOID
EFIAPI
SetFspMemoryInitUpdDataPointer (
  IN VOID  *MemoryInitUpdPtr
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Get the FSP Global Data Pointer
  //
  FspData = GetFspGlobalDataPointer ();

  //
  // Set the FspMemoryInit UPD pointer.
  //
  FspData->MemoryInitUpdPtr = MemoryInitUpdPtr;
}

/**
  This function gets the FspMemoryInit UPD data pointer.

  @return FspMemoryInit UPD data pointer.
**/
VOID *
EFIAPI
GetFspMemoryInitUpdDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return FspData->MemoryInitUpdPtr;
}

/**
  This function sets the FspSiliconInit UPD data pointer.

  @param[in] SiliconInitUpdPtr   FspSiliconInit UPD data pointer.
**/
VOID
EFIAPI
SetFspSiliconInitUpdDataPointer (
  IN VOID  *SiliconInitUpdPtr
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Get the FSP Global Data Pointer
  //
  FspData = GetFspGlobalDataPointer ();

  //
  // Set the FspSiliconInit UPD data pointer.
  //
  FspData->SiliconInitUpdPtr = SiliconInitUpdPtr;
}

/**
  This function gets the FspSiliconInit UPD data pointer.

  @return FspSiliconInit UPD data pointer.
**/
VOID *
EFIAPI
GetFspSiliconInitUpdDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return FspData->SiliconInitUpdPtr;
}

/**
  This function sets the FspSmmInit UPD data pointer.

  @param[in] SmmInitUpdPtr   FspSmmInit UPD data pointer.
**/
VOID
EFIAPI
SetFspSmmInitUpdDataPointer (
  IN VOID  *SmmInitUpdPtr
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Get the FSP Global Data Pointer
  //
  FspData = GetFspGlobalDataPointer ();

  //
  // Set the FspSmmInit UPD data pointer.
  //
  FspData->SmmInitUpdPtr = SmmInitUpdPtr;
}

/**
  This function gets the FspSmmInit UPD data pointer.

  @return FspSmmInit UPD data pointer.
**/
VOID *
EFIAPI
GetFspSmmInitUpdDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return FspData->SmmInitUpdPtr;
}

/**
  Set FSP measurement point timestamp.

  @param[in] Id       Measurement point ID.

  @return performance timestamp if current PerfIdx is valid,
          else return 0 as invalid performance timestamp
**/
UINT64
EFIAPI
SetFspMeasurePoint (
  IN UINT8  Id
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Bit [55: 0]  will be the timestamp
  // Bit [63:56]  will be the ID
  //
  FspData = GetFspGlobalDataPointer ();
  if (FspData->PerfIdx < sizeof (FspData->PerfData) / sizeof (FspData->PerfData[0])) {
    FspData->PerfData[FspData->PerfIdx]                  = AsmReadTsc ();
    ((UINT8 *)(&FspData->PerfData[FspData->PerfIdx]))[7] = Id;
    return FspData->PerfData[(FspData->PerfIdx)++];
  }

  return 0;
}

/**
  This function gets the FSP info header pointer.

  @retval FspInfoHeader   FSP info header pointer
**/
FSP_INFO_HEADER *
EFIAPI
GetFspInfoHeader (
  VOID
  )
{
  return GetFspGlobalDataPointer ()->FspInfoHeader;
}

/**
  This function sets the FSP info header pointer.

  @param[in] FspInfoHeader   FSP info header pointer
**/
VOID
EFIAPI
SetFspInfoHeader (
  FSP_INFO_HEADER  *FspInfoHeader
  )
{
  GetFspGlobalDataPointer ()->FspInfoHeader = FspInfoHeader;
}

/**
  This function gets the FSP info header pointer using the API stack context.

  @retval FspInfoHeader   FSP info header pointer using the API stack context
**/
FSP_INFO_HEADER *
EFIAPI
GetFspInfoHeaderFromApiContext (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  return (FSP_INFO_HEADER *)(*(UINTN *)(FspData->CoreStack + CONTEXT_STACK_OFFSET (FspInfoHeader)));
}

/**
  This function gets the CfgRegion data pointer.

  @return CfgRegion data pointer.
**/
VOID *
EFIAPI
GetFspCfgRegionDataPointer (
  VOID
  )
{
  FSP_INFO_HEADER  *FspInfoHeader;

  FspInfoHeader = GetFspInfoHeader ();
  return (VOID *)(UINTN)(FspInfoHeader->ImageBase + FspInfoHeader->CfgRegionOffset);
}

/**
  This function gets FSP API calling index.

  @retval API calling index
**/
UINT8
EFIAPI
GetFspApiCallingIndex (
  VOID
  )
{
  return GetFspGlobalDataPointer ()->ApiIdx;
}

/**
  This function sets FSP API calling mode.

  @param[in] Index     API calling index
**/
VOID
EFIAPI
SetFspApiCallingIndex (
  UINT8  Index
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData         = GetFspGlobalDataPointer ();
  FspData->ApiIdx = Index;
}

/**
  This function gets FSP Phase StatusCode.

  @retval StatusCode
**/
UINT32
EFIAPI
GetPhaseStatusCode (
  VOID
  )
{
  return GetFspGlobalDataPointer ()->StatusCode;
}

/**
  This function sets FSP Phase StatusCode.

  @param[in] Mode     Phase StatusCode
**/
VOID
EFIAPI
SetPhaseStatusCode (
  UINT32  StatusCode
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData             = GetFspGlobalDataPointer ();
  FspData->StatusCode = StatusCode;
}

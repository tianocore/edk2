/** @file

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <FspGlobalData.h>
#include <FspEas.h>
#include <FspDataTable.h>

#pragma pack(1)

//
//   API Parameter                +0x34
//   API return address           +0x30
//
//   push    FspInfoHeader        +0x2C
//   pushfd                       +0x28
//   cli
//   pushad                       +0x24
//   sub     esp, 8               +0x00
//   sidt    fword ptr [esp]
//
typedef struct {
  UINT16    IdtrLimit;
  UINT32    IdtrBase;
  UINT16    Reserved;
  UINT32    Edi;
  UINT32    Esi;
  UINT32    Ebp;
  UINT32    Esp;
  UINT32    Ebx;
  UINT32    Edx;
  UINT32    Ecx;
  UINT32    Eax;
  UINT16    Flags[2];
  UINT32    FspInfoHeader;
  UINT32    ApiRet;
  UINT32    ApiParam[2];
} CONTEXT_STACK;

#define CONTEXT_STACK_OFFSET(x)  (UINT32)&((CONTEXT_STACK *)(UINTN)0)->x

#pragma pack()

/**
  This function sets the FSP global data pointer.

  @param[in] FspData       Fsp global data pointer.

**/
VOID
EFIAPI
SetFspGlobalDataPointer (
  IN FSP_GLOBAL_DATA   *FspData
  )
{
  ASSERT (FspData != NULL);
  *((volatile UINT32 *)(UINTN)PcdGet32(PcdGlobalDataPointerAddress)) = (UINT32)(UINTN)FspData;
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
  FSP_GLOBAL_DATA   *FspData;

  FspData = *(FSP_GLOBAL_DATA  **)(UINTN)PcdGet32(PcdGlobalDataPointerAddress);
  return FspData;
}

/**
  This function gets back the FSP API first parameter passed by the bootlaoder.

  @retval ApiParameter FSP API first parameter passed by the bootlaoder.
**/
UINT32
EFIAPI
GetFspApiParameter (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  return *(UINT32 *)(UINTN)(FspData->CoreStack + CONTEXT_STACK_OFFSET(ApiParam[0]));
}

/**
  This function gets back the FSP API second parameter passed by the bootlaoder.

  @retval ApiParameter FSP API second parameter passed by the bootlaoder.
**/
UINT32
EFIAPI
GetFspApiParameter2 (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  return *(UINT32 *)(UINTN)(FspData->CoreStack + CONTEXT_STACK_OFFSET(ApiParam[1]));
}

/**
  This function sets the FSP API parameter in the stack.

   @param[in] Value       New parameter value.

**/
VOID
EFIAPI
SetFspApiParameter (
  IN UINT32      Value
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  *(UINT32 *)(UINTN)(FspData->CoreStack + CONTEXT_STACK_OFFSET(ApiParam)) = Value;
}

/**
  This function set the API status code returned to the BootLoader.

  @param[in] ReturnStatus       Status code to return.

**/
VOID
EFIAPI
SetFspApiReturnStatus (
  IN UINT32  ReturnStatus
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  *(UINT32 *)(UINTN)(FspData->CoreStack + CONTEXT_STACK_OFFSET(Eax)) = ReturnStatus;
}

/**
  This function sets the context switching stack to a new stack frame.

  @param[in] NewStackTop       New core stack to be set.

**/
VOID
EFIAPI
SetFspCoreStackPointer (
  IN VOID   *NewStackTop
  )
{
  FSP_GLOBAL_DATA  *FspData;
  UINT32           *OldStack;
  UINT32           *NewStack;
  UINT32           StackContextLen;

  FspData  = GetFspGlobalDataPointer ();
  StackContextLen = sizeof(CONTEXT_STACK) / sizeof(UINT32);

  //
  // Reserve space for the ContinuationFunc two parameters
  //
  OldStack = (UINT32 *)FspData->CoreStack;
  NewStack = (UINT32 *)NewStackTop - StackContextLen - 2;
  FspData->CoreStack = (UINT32)NewStack;
  while (StackContextLen-- != 0) {
    *NewStack++ = *OldStack++;
  }
}

/**
  This function sets the platform specific data pointer.

  @param[in] PlatformData       Fsp platform specific data pointer.

**/
VOID
EFIAPI
SetFspPlatformDataPointer (
  IN VOID   *PlatformData
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  FspData->PlatformData.DataPtr = PlatformData;
}


/**
  This function gets the platform specific data pointer.

   @param[in] PlatformData       Fsp platform specific data pointer.

**/
VOID *
EFIAPI
GetFspPlatformDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  return FspData->PlatformData.DataPtr;
}


/**
  This function sets the UPD data pointer.

  @param[in] UpdDataPtr   UPD data pointer.
**/
VOID
EFIAPI
SetFspUpdDataPointer (
  IN VOID    *UpdDataPtr
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Get the Fsp Global Data Pointer
  //
  FspData  = GetFspGlobalDataPointer ();

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

  FspData  = GetFspGlobalDataPointer ();
  return FspData->UpdDataPtr;
}


/**
  This function sets the memory init UPD data pointer.

  @param[in] MemoryInitUpdPtr   memory init UPD data pointer.
**/
VOID
EFIAPI
SetFspMemoryInitUpdDataPointer (
  IN VOID    *MemoryInitUpdPtr
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Get the Fsp Global Data Pointer
  //
  FspData  = GetFspGlobalDataPointer ();

  //
  // Set the memory init UPD pointer.
  //
  FspData->MemoryInitUpdPtr = MemoryInitUpdPtr;
}

/**
  This function gets the memory init UPD data pointer.

  @return memory init UPD data pointer.
**/
VOID *
EFIAPI
GetFspMemoryInitUpdDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  return FspData->MemoryInitUpdPtr;
}


/**
  This function sets the silicon init UPD data pointer.

  @param[in] SiliconInitUpdPtr   silicon init UPD data pointer.
**/
VOID
EFIAPI
SetFspSiliconInitUpdDataPointer (
  IN VOID    *SiliconInitUpdPtr
  )
{
  FSP_GLOBAL_DATA  *FspData;

  //
  // Get the Fsp Global Data Pointer
  //
  FspData  = GetFspGlobalDataPointer ();

  //
  // Set the silicon init UPD data pointer.
  //
  FspData->SiliconInitUpdPtr = SiliconInitUpdPtr;
}

/**
  This function gets the silicon init UPD data pointer.

  @return silicon init UPD data pointer.
**/
VOID *
EFIAPI
GetFspSiliconInitUpdDataPointer (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData  = GetFspGlobalDataPointer ();
  return FspData->SiliconInitUpdPtr;
}


/**
  Set FSP measurement point timestamp.

  @param[in] Id       Measurement point ID.

  @return performance timestamp.
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
  FspData  = GetFspGlobalDataPointer ();
  if (FspData->PerfIdx < sizeof(FspData->PerfData) / sizeof(FspData->PerfData[0])) {
    FspData->PerfData[FspData->PerfIdx] = AsmReadTsc ();
    ((UINT8 *)(&FspData->PerfData[FspData->PerfIdx]))[7] = Id;
  }

  return FspData->PerfData[(FspData->PerfIdx)++];
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
  return  GetFspGlobalDataPointer()->FspInfoHeader;
}

/**
  This function sets the FSP info header pointer.

  @param[in] FspInfoHeader   FSP info header pointer
**/
VOID
EFIAPI
SetFspInfoHeader (
  FSP_INFO_HEADER *FspInfoHeader
  )
{
  GetFspGlobalDataPointer()->FspInfoHeader = FspInfoHeader;
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

  FspData  = GetFspGlobalDataPointer ();
  return  (FSP_INFO_HEADER *)(*(UINT32 *)(UINTN)(FspData->CoreStack + CONTEXT_STACK_OFFSET(FspInfoHeader)));
}

/**
  This function gets the VPD data pointer.

  @return VpdDataRgnPtr   VPD data pointer.
**/
VOID *
EFIAPI
GetFspVpdDataPointer (
  VOID
  )
{
  FSP_INFO_HEADER   *FspInfoHeader;

  FspInfoHeader = GetFspInfoHeader ();
  return (VOID *)(FspInfoHeader->ImageBase + FspInfoHeader->CfgRegionOffset);
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
  return  GetFspGlobalDataPointer()->ApiIdx;
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

  FspData  = GetFspGlobalDataPointer ();
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
  return  GetFspGlobalDataPointer()->StatusCode;
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

  FspData  = GetFspGlobalDataPointer ();
  FspData->StatusCode = StatusCode;
}

/**
  This function gets FSP CAR base.

**/
UINT32
EFIAPI
GetFspCarBase (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;
  UINT32           CarBase;

  FspData  = GetFspGlobalDataPointer ();
  CarBase = FspData->PlatformData.CarBase;
  if (CarBase == 0) {
    CarBase = PcdGet32(PcdTemporaryRamBase);
  }
  return CarBase;
}

/**
  This function gets FSP CAR size.

**/
UINT32
EFIAPI
GetFspCarSize (
  VOID
  )
{
  FSP_GLOBAL_DATA  *FspData;
  UINT32           CarSize;

  FspData  = GetFspGlobalDataPointer ();
  CarSize = FspData->PlatformData.CarSize;
  if (FspData->PlatformData.CarBase == 0) {
    CarSize = PcdGet32(PcdTemporaryRamSize);
  }
  return CarSize;
}

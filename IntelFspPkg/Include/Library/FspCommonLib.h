/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_COMMON_LIB_H_
#define _FSP_COMMON_LIB_H_

#include <FspGlobalData.h>
#include <FspMeasurePointId.h>

/**
  This function sets the FSP global data pointer.

  @param[in] FspData       Fsp global data pointer.

**/
VOID
EFIAPI
SetFspGlobalDataPointer (
  IN FSP_GLOBAL_DATA   *FspData
  );

/**
  This function gets the FSP global data pointer.

**/
FSP_GLOBAL_DATA *
EFIAPI
GetFspGlobalDataPointer (
  VOID
  );

/**
  This function gets back the FSP API parameter passed by the bootlaoder.

  @retval ApiParameter FSP API parameter passed by the bootlaoder.
**/
UINT32
EFIAPI
GetFspApiParameter (
  VOID
  );

/**
  This function sets the FSP API parameter in the stack.

   @param[in] Value       New parameter value.

**/
VOID
EFIAPI
SetFspApiParameter (
  IN UINT32      Value
  );

/**
  This function sets the FSP continuation function parameters in the stack.

  @param[in] Value             New parameter value to set.
  @param[in] Index             Parameter index.
**/
VOID
EFIAPI
SetFspContinuationFuncParameter (
  IN UINT32      Value,
  IN UINT32      Index
  );

/**
  This function changes the BootLoader return address in stack.

  @param[in] ReturnAddress       Address to return.

**/
VOID
EFIAPI
SetFspApiReturnAddress (
  IN UINT32  ReturnAddress
  );

/**
  This function set the API status code returned to the BootLoader.

  @param[in] ReturnStatus       Status code to return.

**/
VOID
EFIAPI
SetFspApiReturnStatus (
  IN UINT32  ReturnStatus
  );

/**
  This function sets the context switching stack to a new stack frame.

  @param[in] NewStackTop       New core stack to be set.

**/
VOID
EFIAPI
SetFspCoreStackPointer (
  IN VOID   *NewStackTop
  );

/**
  This function sets the platform specific data pointer.

  @param[in] PlatformData       Fsp platform specific data pointer.

**/
VOID
EFIAPI
SetFspPlatformDataPointer (
  IN VOID   *PlatformData
  );

/**
  This function gets the platform specific data pointer.

   @param[in] PlatformData       Fsp platform specific data pointer.

**/
VOID *
EFIAPI
GetFspPlatformDataPointer (
  VOID
  );

/**
  This function sets the UPD data pointer.

  @param[in] UpdDataRgnPtr   UPD data pointer.
**/
VOID
EFIAPI
SetFspUpdDataPointer (
  IN VOID    *UpdDataRgnPtr
  );

/**
  This function gets the UPD data pointer.

  @return UpdDataRgnPtr   UPD data pointer.
**/
VOID *
EFIAPI
GetFspUpdDataPointer (
  VOID
  );

/**
  This function sets the memory init UPD data pointer.

  @param[in] MemoryInitUpdPtr   memory init UPD data pointer.
**/
VOID
EFIAPI
SetFspMemoryInitUpdDataPointer (
  IN VOID    *MemoryInitUpdPtr
  );

/**
  This function gets the memory init UPD data pointer.

  @return memory init UPD data pointer.
**/
VOID *
EFIAPI
GetFspMemoryInitUpdDataPointer (
  VOID
  );

/**
  This function sets the silicon init UPD data pointer.

  @param[in] SiliconInitUpdPtr   silicon init UPD data pointer.
**/
VOID
EFIAPI
SetFspSiliconInitUpdDataPointer (
  IN VOID    *SiliconInitUpdPtr
  );

/**
  This function gets the silicon init UPD data pointer.

  @return silicon init UPD data pointer.
**/
VOID *
EFIAPI
GetFspSiliconInitUpdDataPointer (
  VOID
  );

/**
  Set FSP measurement point timestamp.

  @param[in] Id       Measurement point ID.

  @return performance timestamp.
**/
UINT64
EFIAPI
SetFspMeasurePoint (
  IN UINT8  Id
  );

/**
  This function gets the FSP info header pointer.

  @retval FspInfoHeader   FSP info header pointer
**/
FSP_INFO_HEADER *
EFIAPI
GetFspInfoHeader (
  VOID
  );

/**
  This function gets the FSP info header pointer from the API context.

  @retval FspInfoHeader   FSP info header pointer
**/
FSP_INFO_HEADER *
EFIAPI
GetFspInfoHeaderFromApiContext (
  VOID
  );

/**
  This function gets the VPD data pointer.

  @return VpdDataRgnPtr   VPD data pointer.
**/
VOID *
EFIAPI
GetFspVpdDataPointer (
  VOID
  );

/**
  This function gets FSP API calling mode.

  @retval API calling mode
**/
UINT8
EFIAPI
GetFspApiCallingMode (
  VOID
  );

/**
  This function sets FSP API calling mode.

  @param[in] Mode     API calling mode
**/
VOID
EFIAPI
SetFspApiCallingMode (
  UINT8  Mode
  );

#endif

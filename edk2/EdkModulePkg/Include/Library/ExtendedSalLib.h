/*++

Copyright (c) 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ExtendedSalLib.h

Abstract:

--*/

#ifndef _EXTENDED_SAL_LIB_H__
#define _EXTENDED_SAL_LIB_H__

/**
  Register ESAL Class and it's asociated global.
  
  This function Registers one or more Extended SAL services in a given
  class along with the associated global context.
  This function is only available prior to ExitBootServices().

  @param  ClassGuid            GUID of function class
  @param  ModuleGlobal         Module global for Function.
  
  @retval EFI_SUCCESS          The Extended SAL services were registered.
  @retval EFI_UNSUPPORTED      This function was called after ExitBootServices().
  @retval EFI_OUT_OF_RESOURCES There are not enough resources available to register one or more of the specified services.
  @retval Other                ClassGuid could not be installed onto a new handle.  

**/
EFI_STATUS
EFIAPI
RegisterEsalClass (
  IN  CONST EFI_GUID  *ClassGuid,
  IN  VOID            *ModuleGlobal,  OPTIONAL
  ...
  )
;

/**
  Calls an Extended SAL Class service that was previously registered with RegisterEsalClass().
  
  This function calls an Extended SAL Class service that was previously registered with RegisterEsalClass().

  @param  ClassGuid      GUID of function
  @param  FunctionId     Function in ClassGuid to call
  @param  Arg2           Argument 2 ClassGuid/FunctionId defined
  @param  Arg3           Argument 3 ClassGuid/FunctionId defined
  @param  Arg4           Argument 4 ClassGuid/FunctionId defined
  @param  Arg5           Argument 5 ClassGuid/FunctionId defined
  @param  Arg6           Argument 6 ClassGuid/FunctionId defined
  @param  Arg7           Argument 7 ClassGuid/FunctionId defined
  @param  Arg8           Argument 8 ClassGuid/FunctionId defined
  
  @retval EFI_SAL_ERROR  The address of ExtendedSalProc() can not be determined
                         for the current CPU execution mode.
  @retval Other          See the return status from ExtendedSalProc() in the
                         EXTENDED_SAL_BOOT_SERVICE_PROTOCOL.  

**/
SAL_RETURN_REGS
EFIAPI
EsalCall (
  IN  EFI_GUID  *ClassGuid,
  IN  UINT64    FunctionId,
  IN  UINT64    Arg2,
  IN  UINT64    Arg3,
  IN  UINT64    Arg4,
  IN  UINT64    Arg5,
  IN  UINT64    Arg6,
  IN  UINT64    Arg7,
  IN  UINT64    Arg8
  )
;

/**
  Wrapper for the EsalStallFunctionId service in the Extended SAL Stall Services Class.
  
  This function is a wrapper for the EsalStallFunctionId service in the Extended SAL
  Stall Services Class. See EsalStallFunctionId in the Extended SAL Specification.

  @param  Microseconds         The number of microseconds to delay.

**/
SAL_RETURN_REGS
EFIAPI
EsalStall (
  IN UINTN  Microseconds
  )
;

/**
  Wrapper for the EsalSetNewPalEntryFunctionId service in the Extended SAL PAL Services Services Class.
  
  This function is a wrapper for the EsalSetNewPalEntryFunctionId service in the Extended SAL
  PAL Services Services Class. See EsalSetNewPalEntryFunctionId in the Extended SAL Specification.

  @param  PhyicalAddress         If TRUE, then PalEntryPoint is a physical address.
                                 If FALSE, then PalEntryPoint is a virtual address.
  @param  PalEntryPoint          The PAL Entry Point being set.

**/
SAL_RETURN_REGS
EFIAPI
EsalSetNewPalEntry (
  IN BOOLEAN  PhysicalAddress,
  IN UINT64   PalEntryPoint
  )
;

/**
  Wrapper for the EsalGetStateBufferFunctionId service in the Extended SAL PAL Services Services Class.
  
  This function is a wrapper for the EsalGetStateBufferFunctionId service in the Extended SAL
  PAL Services Services Class. See EsalGetStateBufferFunctionId in the Extended SAL Specification.

  @param  PhyicalAddress         If TRUE, then PalEntryPoint is a physical address.
                                 If FALSE, then PalEntryPoint is a virtual address.

**/
SAL_RETURN_REGS
EFIAPI
EsalGetNewPalEntry (
  IN BOOLEAN  PhysicalAddress
  )
;

/**
  Wrapper for the EsalGetStateBufferFunctionId  service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalGetStateBufferFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalGetStateBufferFunctionId in the Extended SAL Specification.

  @param  McaType         See type parameter in the SAL Procedure SAL_GET_STATE_INFO.
  @param  McaBuffer       A pointer to the base address of the returned buffer. Copied from SAL_RETURN_REGS.r9.
  @param  BufferSize      A pointer to the size, in bytes, of the returned buffer.  Copied from SAL_RETURN_REGS.r10.

**/
SAL_RETURN_REGS
EFIAPI
EsalGetStateBuffer (
  IN  UINT64  McaType,
  OUT UINT8   **McaBuffer,
  OUT UINTN   *BufferSize
  )
;

/**
  Wrapper for the EsalSaveStateBufferFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalSaveStateBufferFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalSaveStateBufferFunctionId in the Extended SAL Specification.

  @param  McaType         See type parameter in the SAL Procedure SAL_GET_STATE_INFO.

**/
SAL_RETURN_REGS
EFIAPI
EsalSaveStateBuffer (
  IN  UINT64  McaType
  )
;

/**
  Wrapper for the EsalGetVectorsFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalGetVectorsFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalGetVectorsFunctionId in the Extended SAL Specification.

  @param  VectorType         The vector type to retrieve.
                             0 每 MCA, 1 - BSP INIT, 2 每 BOOT_RENDEZ, 3 每 AP INIT.

**/
SAL_RETURN_REGS
EFIAPI
EsalGetVectors (
  IN  UINT64  VectorType
  )
;

/**
  Wrapper for the EsalMcGetParamsFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalMcGetParamsFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalMcGetParamsFunctionId in the Extended SAL Specification.

  @param  ParamInfoType         The parameter type to retrieve.
                                1 每 rendezvous interrupt
                                2 每 wake up
                                3 每 Corrected Platform Error Vector.

**/
SAL_RETURN_REGS
EFIAPI
EsalMcGetParams (
  IN  UINT64  ParamInfoType
  )
;

/**
  Wrapper for the EsalMcGetParamsFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalMcGetParamsFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalMcGetParamsFunctionId in the Extended SAL Specification.

**/
SAL_RETURN_REGS
EFIAPI
EsalMcGetMcParams (
  VOID
  )
;

/**
  Wrapper for the EsalGetMcCheckinFlagsFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalGetMcCheckinFlagsFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalGetMcCheckinFlagsFunctionId in the Extended SAL Specification.

  @param  CpuIndex         The index of the CPU in the set of enabled CPUs to check.

**/
SAL_RETURN_REGS
EFIAPI
EsalGetMcCheckinFlags (
  IN  UINT64  CpuIndex
  )
;

/**
  Wrapper for the EsalAddCpuDataFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalAddCpuDataFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalAddCpuDataFunctionId in the Extended SAL Specification.

  @param  CpuGlobalId         The Global ID for the CPU being added.
  @param  Enabled             The enable flag for the CPU being added.
                              TRUE means the CPU is enabled.
                              FALSE means the CPU is disabled.
  @param  PalCompatibility    The PAL Compatibility value for the CPU being added.

**/
SAL_RETURN_REGS
EFIAPI
EsalAddCpuData (
  IN UINT64   CpuGlobalId,
  IN BOOLEAN  Enabled,
  IN UINT64   PalCompatibility
  )
;

/**
  Wrapper for the EsalRemoveCpuDataFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalRemoveCpuDataFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalRemoveCpuDataFunctionId in the Extended SAL Specification.

  @param  CpuGlobalId         The Global ID for the CPU being removed.

**/
SAL_RETURN_REGS
EFIAPI
EsalRemoveCpuData (
  IN UINT64  CpuGlobalId
  )
;

/**
  Wrapper for the EsalModifyCpuDataFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalModifyCpuDataFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalModifyCpuDataFunctionId in the Extended SAL Specification.

  @param  CpuGlobalId         The Global ID for the CPU being modified.
  @param  Enabled             The enable flag for the CPU being modified.
                              TRUE means the CPU is enabled.
                              FALSE means the CPU is disabled.
  @param  PalCompatibility    The PAL Compatibility value for the CPU being modified.

**/
SAL_RETURN_REGS
EFIAPI
EsalModifyCpuData (
  IN UINT64   CpuGlobalId,
  IN BOOLEAN  Enabled,
  IN UINT64   PalCompatibility
  )
;

/**
  Wrapper for the EsalGetCpuDataByIdFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalGetCpuDataByIdFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalGetCpuDataByIdFunctionId in the Extended SAL Specification.

  @param  CpuGlobalId         The Global ID for the CPU being looked up.
  @param  IndexByEnabledCpu   If TRUE, then the index in the set of enabled CPUs in the database is returned.
                              If FALSE, then the index in the set of all CPUs in the database is returned.

**/
SAL_RETURN_REGS
EFIAPI
EsalGetCpuDataById (
  IN UINT64   CpuGlobalId,
  IN BOOLEAN  IndexByEnabledCpu
  )
;

/**
  Wrapper for the EsalGetCpuDataByIndexFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalGetCpuDataByIndexFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalGetCpuDataByIndexFunctionId in the Extended SAL Specification.

  @param  Index               The Global ID for the CPU being modified.
  @param  IndexByEnabledCpu   If TRUE, then the index in the set of enabled CPUs in the database is returned.
                              If FALSE, then the index in the set of all CPUs in the database is returned.

**/
SAL_RETURN_REGS
EFIAPI
EsalGetCpuDataByIndex (
  IN UINT64   Index,
  IN BOOLEAN  IndexByEnabledCpu
  )
;

/**
  Wrapper for the EsalWhoAmIFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalWhoAmIFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalWhoAmIFunctionId in the Extended SAL Specification.

  @param  IndexByEnabledCpu   If TRUE, then the index in the set of enabled CPUs in the database is returned.
                              If FALSE, then the index in the set of all CPUs in the database is returned.

**/
SAL_RETURN_REGS
EFIAPI
EsalWhoAmI (
  IN BOOLEAN  IndexByEnabledCpu
  )
;

/**
  Wrapper for the EsalNumProcessors service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalNumProcessors service in the Extended SAL
  MCA Log Services Class. See EsalNumProcessors in the Extended SAL Specification.

**/
SAL_RETURN_REGS
EFIAPI
EsalNumProcessors (
  VOID
  )
;

/**
  Wrapper for the EsalSetMinStateFnctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalSetMinStateFnctionId service in the Extended SAL
  MCA Log Services Class. See EsalSetMinStateFnctionId in the Extended SAL Specification.

  @param  CpuGlobalId       The Global ID for the CPU whose MINSTATE pointer is being set.
  @param  MinStatePointer   The physical address of the MINSTATE buffer for the CPU specified by CpuGlobalId.

**/
SAL_RETURN_REGS
EFIAPI
EsalSetMinState (
  IN UINT64                CpuGlobalId,
  IN EFI_PHYSICAL_ADDRESS  MinStatePointer
  )
;

/**
  Wrapper for the EsalGetMinStateFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalGetMinStateFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalGetMinStateFunctionId in the Extended SAL Specification.

  @param  CpuGlobalId   The Global ID for the CPU whose MINSTATE pointer is being retrieved.

**/
SAL_RETURN_REGS
EFIAPI
EsalGetMinState (
  IN UINT64  CpuGlobalId
  )
;

/**
  Wrapper for the EsalMcsGetStateInfoFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalMcsGetStateInfoFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalMcsGetStateInfoFunctionId in the Extended SAL Specification.

  @param  CpuGlobalId               The Global ID for the CPU whose MCA state buffer is being retrieved.
  @param  StateBufferPointer        A pointer to the returned MCA state buffer.
  @param  RequiredStateBufferSize   A pointer to the size, in bytes, of the returned MCA state buffer.

**/
SAL_RETURN_REGS
EFIAPI
EsalMcaGetStateInfo (
  IN  UINT64                CpuGlobalId,
  OUT EFI_PHYSICAL_ADDRESS  *StateBufferPointer,
  OUT UINT64                *RequiredStateBufferSize
  )
;

/**
  Wrapper for the EsalMcaRegisterCpuFunctionId service in the Extended SAL MCA Log Services Class.
  
  This function is a wrapper for the EsalMcaRegisterCpuFunctionId service in the Extended SAL
  MCA Log Services Class. See EsalMcaRegisterCpuFunctionId in the Extended SAL Specification.

  @param  CpuGlobalId          The Global ID for the CPU whose MCA state buffer is being set.
  @param  StateBufferPointer   A pointer to the MCA state buffer.

**/
SAL_RETURN_REGS
EFIAPI
EsalMcaRegisterCpu (
  IN UINT64                CpuGlobalId,
  IN EFI_PHYSICAL_ADDRESS  StateBufferPointer
  )
;

#endif

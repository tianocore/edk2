/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EsalRuntimeLib.h
  
Abstract:

  SAL Driver Lib

Revision History

--*/

#ifndef _ESAL_RUNTIME_LIB_H_
#define _ESAL_RUNTIME_LIB_H_

#include "SalApi.h"
#include "EfiFirmwareVolumeHeader.h"

#include EFI_PROTOCOL_DEFINITION (ExtendedSalBootService)
#include EFI_PROTOCOL_DEFINITION (ExtendedSalGuid)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeBlock)

VOID
EsalRuntimeLibVirtualNotify (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EsalInitializeRuntimeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageHandle - Image Handle
  SystemTable - the System Table 

Returns:

  TODO: add return values

--*/
;
SAL_RETURN_REGS
CallEsalService (
  IN  EFI_GUID                                       *ClassGuid,
  IN   UINT64                                        FunctionId,
  IN  UINT64                                         Arg2,
  IN  UINT64                                         Arg3,
  IN  UINT64                                         Arg4,
  IN  UINT64                                         Arg5,
  IN  UINT64                                         Arg6,
  IN  UINT64                                         Arg7,
  IN  UINT64                                         Arg8
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ClassGuid   - TODO: add argument description
  FunctionId  - TODO: add argument description
  Arg2        - TODO: add argument description
  Arg3        - TODO: add argument description
  Arg4        - TODO: add argument description
  Arg5        - TODO: add argument description
  Arg6        - TODO: add argument description
  Arg7        - TODO: add argument description
  Arg8        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  Assembly Functions
//

SAL_RETURN_REGS
EsalGetEntryPoint (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalSetPhysicalEntryPoint (
  IN  UINT64  EntryPoint,
  IN  UINT64  Gp
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  EntryPoint  - TODO: add argument description
  Gp          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalSetVirtualEntryPoint (
  IN  UINT64  EntryPoint,
  IN  UINT64  Gp
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  EntryPoint  - TODO: add argument description
  Gp          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalSetPhysicalModuleGlobal (
  IN  VOID    *Global
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Global  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalSetVirtualModuleGlobal (
  IN  VOID    *Global
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Global  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalGetModuleGlobal (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
GetIrrData (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
GetPsrData (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
GetProcIdData (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

VOID
SwitchCpuStack (
  IN  UINT64  NewBsp,
  IN  UINT64  OldBsp
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  NewBsp  - TODO: add argument description
  OldBsp  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  PAL PROC Class
//

SAL_RETURN_REGS
SalPalProc (
  IN  UINT64            Arg1,
  IN  UINT64            Arg2,
  IN  UINT64            Arg3,
  IN  UINT64            Arg4
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Arg1  - TODO: add argument description
  Arg2  - TODO: add argument description
  Arg3  - TODO: add argument description
  Arg4  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
SalRegisterNewPalEntry (
  IN  BOOLEAN                     PhysicalPalAddress,
  IN  EFI_PHYSICAL_ADDRESS        NewPalAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PhysicalPalAddress  - TODO: add argument description
  NewPalAddress       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
SalGetPalEntryPointer (
  IN  BOOLEAN                     PhysicalPalAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PhysicalPalAddress  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  SAL BASE Class
//

SAL_RETURN_REGS
SalProcSetVectors (
  IN  UINT64                      SalVectorType,
  IN  UINT64                      PhyAddr1,
  IN  UINT64                      Gp1,
  IN  UINT64                      LengthCs1,
  IN  UINT64                      PhyAddr2,
  IN  UINT64                      Gp2,
  IN  UINT64                      LengthCs2
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SalVectorType - TODO: add argument description
  PhyAddr1      - TODO: add argument description
  Gp1           - TODO: add argument description
  LengthCs1     - TODO: add argument description
  PhyAddr2      - TODO: add argument description
  Gp2           - TODO: add argument description
  LengthCs2     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
SalProcMcRendez (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
SalProcMcSetParams (
  IN  UINT64                      ParamType,
  IN  UINT64                      IntOrMem,
  IN  UINT64                      IntOrMemVal,
  IN  UINT64                      Timeout,
  IN  UINT64                      McaOpt
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ParamType   - TODO: add argument description
  IntOrMem    - TODO: add argument description
  IntOrMemVal - TODO: add argument description
  Timeout     - TODO: add argument description
  McaOpt      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalProcGetVectors (
  IN  UINT64                      VectorType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  VectorType  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalProcMcGetParams (
  IN  UINT64                      ParamInfoType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ParamInfoType - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalProcMcGetMcParams (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
EsalProcGetMcCheckinFlags (
  IN  UINT64                      ProcessorUnit
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ProcessorUnit - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  Sal Base Class enums
//

typedef enum {
  McaVector,
  BspInitVector,
  BootRendezVector,
  ApInitVector
} ESAL_GET_VECTOR_TYPE;

SAL_RETURN_REGS
SalInitializeThreshold (
  IN  VOID                        *ThresholdStruct,
  IN  UINT64                      Count,
  IN  UINT64                      Duration
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ThresholdStruct - TODO: add argument description
  Count           - TODO: add argument description
  Duration        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
SalBumpThresholdCount (
  IN  VOID                        *ThresholdStruct,
  IN  UINT64                      Count,
  IN  UINT64                      Duration
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ThresholdStruct - TODO: add argument description
  Count           - TODO: add argument description
  Duration        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
SalGetThresholdCount (
  IN  VOID                        *ThresholdStruct,
  IN  UINT64                      Count,
  IN  UINT64                      Duration
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ThresholdStruct - TODO: add argument description
  Count           - TODO: add argument description
  Duration        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  Common Lib Function
//

EFI_STATUS
RegisterEsalFunction (
  IN  UINT64                                    FunctionId,
  IN  EFI_GUID                                  *ClassGuid,
  IN  SAL_INTERNAL_EXTENDED_SAL_PROC            Function,
  IN  VOID                                      *ModuleGlobal
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  FunctionId    - TODO: add argument description
  ClassGuid     - TODO: add argument description
  Function      - TODO: add argument description
  ModuleGlobal  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
RegisterEsalClass (
  IN  EFI_GUID                                  *ClassGuid,
  IN  VOID                                      *ModuleGlobal,
  ...
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ClassGuid     - TODO: add argument description
  ModuleGlobal  - TODO: add argument description
  ...           - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  MP Class Functions
//
SAL_RETURN_REGS
LibMpAddCpuData (
  IN    UINT64      CpuGlobalId,
  IN    BOOLEAN     Enabled,
  IN    UINT64      PalCompatability
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuGlobalId       - TODO: add argument description
  Enabled           - TODO: add argument description
  PalCompatability  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpRemoveCpuData (
  IN    UINT64      CpuGlobalId
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuGlobalId - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpModifyCpuData (
  IN    UINT64      CpuGlobalId,
  IN    BOOLEAN     Enabled,
  IN    UINT64      PalCompatability
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuGlobalId       - TODO: add argument description
  Enabled           - TODO: add argument description
  PalCompatability  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpGetCpuDataByID (
  IN    UINT64      CpuGlobalId,
  IN    BOOLEAN     IndexByEnabledCpu
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuGlobalId       - TODO: add argument description
  IndexByEnabledCpu - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpGetCpuDataByIndex (
  IN    UINT64      Index,
  IN    BOOLEAN     IndexByEnabledCpu
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Index             - TODO: add argument description
  IndexByEnabledCpu - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpSendIpi (
  IN  UINT64                ProcessorNumber,
  IN  UINT64                VectorNumber,
  IN  EFI_DELIVERY_MODE     DeliveryMode,
  IN  BOOLEAN               IRFlag
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ProcessorNumber - TODO: add argument description
  VectorNumber    - TODO: add argument description
  DeliveryMode    - TODO: add argument description
  IRFlag          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpCurrentProcessor (
  IN    BOOLEAN     IndexByEnabledCpu
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IndexByEnabledCpu - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibGetNumProcessors (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpSaveMinStatePointer (
  IN    UINT64                CpuGlobalId,
  IN    EFI_PHYSICAL_ADDRESS  MinStatePointer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuGlobalId     - TODO: add argument description
  MinStatePointer - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

SAL_RETURN_REGS
LibMpRestoreMinStatePointer (
  IN    UINT64                CpuGlobalId
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuGlobalId - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  MCA Class Functions
//

EFI_STATUS
LibMcaGetStateInfo (
  IN  UINT64                                      CpuId,
  OUT EFI_PHYSICAL_ADDRESS                        *StateBufferPointer,
  OUT UINT64                                      *RequiredStateBufferSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuId                   - TODO: add argument description
  StateBufferPointer      - TODO: add argument description
  RequiredStateBufferSize - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
LibMcaRegisterCpu (
  IN  UINT64                                      CpuId,
  IN  EFI_PHYSICAL_ADDRESS                        StateBufferAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  CpuId               - TODO: add argument description
  StateBufferAddress  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// FVB Variables Class
//
EFI_STATUS
EsalReadBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Instance  - TODO: add argument description
  Lba       - TODO: add argument description
  Offset    - TODO: add argument description
  NumBytes  - TODO: add argument description
  Buffer    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EsalWriteBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Instance  - TODO: add argument description
  Lba       - TODO: add argument description
  Offset    - TODO: add argument description
  NumBytes  - TODO: add argument description
  Buffer    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EsalEraseBlock (
  IN UINTN                                Instance,
  IN UINTN                                Lba
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Instance  - TODO: add argument description
  Lba       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EsalGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES                  *Attributes
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Instance    - TODO: add argument description
  Attributes  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EsalSetVolumeAttributes (
  IN UINTN                                Instance,
  IN EFI_FVB_ATTRIBUTES                   Attributes
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Instance    - TODO: add argument description
  Attributes  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EsalGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *Address
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Instance  - TODO: add argument description
  Address   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EsalGetBlockSize (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  OUT UINTN                               *BlockSize,
  OUT UINTN                               *NumOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Instance    - TODO: add argument description
  Lba         - TODO: add argument description
  BlockSize   - TODO: add argument description
  NumOfBlocks - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// SAL ELOG Functions
//
EFI_STATUS
LibSalGetStateInfo (
  IN  UINT64                                      McaType,
  IN  UINT8                                       *McaBuffer,
  OUT UINTN                                       *Size
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  McaType   - TODO: add argument description
  McaBuffer - TODO: add argument description
  Size      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
LibSalGetStateInfoSize (
  IN  UINT64                                      McaType,
  OUT UINTN                                       *Size
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  McaType - TODO: add argument description
  Size    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
LibSalClearStateInfo (
  IN  UINT64                                      McaType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  McaType - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
LibEsalGetStateBuffer (
  IN  UINT64                                      McaType,
  OUT UINT8                                       **McaBuffer,
  OUT UINTN                                       *Index
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  McaType   - TODO: add argument description
  McaBuffer - TODO: add argument description
  Index     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
LibEsalSaveStateBuffer (
  IN  UINT64                                      McaType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  McaType - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif

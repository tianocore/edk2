/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SalDriverLib.h
  
Abstract:

  SAL Driver Lib

Revision History

--*/

#ifndef _SAL_DRIVER_LIB_H_
#define _SAL_DRIVER_LIB_H_

#include "Tiano.h"
#include "EfiDriverLib.h"
#include "SalApi.h"

#include EFI_PROTOCOL_DEFINITION (ExtendedSalBootService)
#include EFI_PROTOCOL_DEFINITION (ExtendedSalGuid)

//
//  Assembly Functions
//
SAL_RETURN_REGS
LibGetEsalPhyData (
  VOID
  )
/*++

Routine Description:

  Get Esal global data in physical mode.

Arguments:

  None

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
LibGetEsalVirtData (
  VOID
  )
/*++

Routine Description:

  Get Esal global data in virtual mode.

Arguments:

  None

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
LibSetEsalPhyData (
  IN  VOID                        *Ptr,
  IN  UINT64                      GP
  )
/*++

Routine Description:

  Set Esal global data in physical mode.

Arguments:

  Ptr            - Pointer to the data
  GP             - Global pointer

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
LibSetEsalVirtData (
  IN  VOID                        *Ptr,
  IN  UINT64                      GP
  )
/*++

Routine Description:

  Set Esal global data in virtual mode.

Arguments:

  Ptr            - Pointer to the data
  GP             - Global pointer

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
LibGetGlobalPhyData (
  VOID
  )
/*++

Routine Description:

  Get Esal global data in physical mode.

Arguments:

  None

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
LibGetGlobalVirtData (
  VOID
  )
/*++

Routine Description:

  Get Esal global data in virtual mode.

Arguments:

  None

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
LibSetGlobalPhyData (
  IN  VOID                        *Ptr,
  IN  UINT64                      GP
  )
/*++

Routine Description:

  Set Esal global data in physical mode.

Arguments:

  Ptr            - Pointer to the data
  GP             - Global pointer

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
LibSetGlobalVirtData (
  IN  VOID                        *Ptr,
  IN  UINT64                      GP
  )
/*++

Routine Description:

  Set Esal global data in virtual mode.

Arguments:

  Ptr            - Pointer to the data
  GP             - Global pointer

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
GetIrrData (
  VOID
  )
/*++

Routine Description:

  Get interrupt request register.

Arguments:

  None

Returns:

  Output regs

--*/
;

VOID
PrepareApsForHandOverToOS (
  VOID
  )
/*++

Routine Description:

  Prepare AP info for hand over to OS.

Arguments:

  None

Returns:

  None

--*/
;

VOID
HandOverApsToOS (
  IN UINT64  a1,
  IN UINT64  a2,
  IN UINT64  a3
  )
/*++

Routine Description:

  Hand over AP info to OS.

Arguments:

  a1    - Address to call into
  
  a2    - GP
  
  a3    - Undefined

Returns:

  None

--*/
;

SAL_RETURN_REGS
GetPsrData (
  VOID
  )
/*++

Routine Description:

  Get PSR register.

Arguments:

  None

Returns:

  Output regs.

--*/
;

SAL_RETURN_REGS
GetProcIdData (
  VOID
  )
/*++

Routine Description:

  Get LID

Arguments:

  None

Returns:

  Output regs

--*/
;

VOID
SwitchCpuStack (
  IN  UINT64  NewBsp,
  IN  UINT64  OldBsp
  )
/*++

Routine Description:

  Switch BSP

Arguments:

  NewBsp    - New BSP index
  OldBsp    - Old BSP index

Returns:

  None

--*/
;

//
//  SAL Reset Class
//
VOID
SalResetSystem (
  IN EFI_RESET_TYPE                ResetType,
  IN EFI_STATUS                    ResetStatus,
  IN UINTN                         DataSize,
  IN CHAR16                        *ResetData
  )
/*++

Routine Description:

  Reset system

Arguments:

  ResetType     - Reset type
  ResetStatus   - Reset status
  DataSize      - Size of ResetData
  ResetData     - Description string

Returns:

  None

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

  Call pal proc.

Arguments:

  Arg1          - Pal call index
  Arg2          - First arg
  Arg3          - Second arg
  Arg4          - Third arg

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
SalRegisterNewPalEntry (
  IN  BOOLEAN                     PhysicalPalAddress,
  IN  EFI_PHYSICAL_ADDRESS        NewPalAddress
  )
/*++

Routine Description:

  Register Pal entry.

Arguments:

  PhysicalPalAddress      - The address is physical or virtual
  NewPalAddress           - New Pal entry address

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
SalGetPalEntryPointer (
  IN  BOOLEAN                     PhysicalPalAddress
  )
/*++

Routine Description:

  Get Pal entry.

Arguments:

  PhysicalPalAddress      - The address is physical or virtual

Returns:

  Output regs

--*/
;

//
//  SAL MTC Class
//
EFI_STATUS
SalGetNextHighMonotonicCount (
  OUT UINT32                      *HighCount
  )
/*++

Routine Description:

  Get next high 32 bits of monotonic count.

Arguments:

  HighCount     - High 32 bits of monotonic count.

Returns:

  Status code

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

  Set vectors.

Arguments:

  SalVectorType     - Vector type
  PhyAddr1          - OS MCA entry point
  Gp1               - GP for OS MCA entry
  LengthCs1         - Length of OS MCA 
  PhyAddr2          - OS INIT entry point
  Gp2               - GP for OS Init entry
  LengthCs2         - Length of OS INIT

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
SalProcMcRendez (
  VOID
  )
/*++

Routine Description:

  Mc rendezvous function.

Arguments:

  None

Returns:

  Output regs

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

  Set MCA parameters.

Arguments:

  ParamType     - Parameter type
  IntOrMem      - Interrupt or memory address
  IntOrMemVal   - Interrupt number or memory address value
  Timeout       - Time out value
  McaOpt        - Option for MCA

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
EsalProcGetVectors (
  IN  UINT64                      VectorType
  )
/*++

Routine Description:

  Get OS MCA vector.

Arguments:

  VectorType      - Vector type

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
EsalProcMcGetParams (
  IN  UINT64                      ParamInfoType
  )
/*++

Routine Description:

  Get MCA parameter.

Arguments:

  ParamInfoType     - Parameter info type

Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
EsalProcMcGetMcParams (
  VOID
  )
/*++

Routine Description:

  Get MCA parameter.

Arguments:

  
Returns:

  Output regs

--*/
;

SAL_RETURN_REGS
EsalProcGetMcCheckinFlags (
  IN  UINT64                      ProcessorUnit
  )
/*++

Routine Description:

  Get process status.

Arguments:

  ProcessorUnit     - Processor Index

Returns:

  Output regs

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

//
//  Sal RTC Class
//
EFI_STATUS
SalGetTime (
  OUT EFI_TIME                     *Time,
  OUT EFI_TIME_CAPABILITIES        *Capabilities
  )
/*++

Routine Description:

  Returns the current time and date information, and the time-keeping 
  capabilities of the hardware platform.

Arguments:

  Time          - A pointer to storage to receive a snapshot of the current time.
  Capabilities  - An optional pointer to a buffer to receive the real time clock device's
                  capabilities.

Returns:

  Status code

--*/
;

EFI_STATUS
SalSetTime (
  OUT EFI_TIME                    *Time
  )
/*++

Routine Description:

  Sets the current local time and date information.

Arguments:

  Time  - A pointer to the current time.

Returns:

  Status code

--*/
;

EFI_STATUS
SalGetWakeupTime (
  OUT BOOLEAN                      *Enabled,
  OUT BOOLEAN                      *Pending,
  OUT EFI_TIME                     *Time
  )
/*++

Routine Description:

  Returns the current wakeup alarm clock setting.

Arguments:

  Enabled - Indicates if the alarm is currently enabled or disabled.
  Pending - Indicates if the alarm signal is pending and requires acknowledgement.
  Time    - The current alarm setting.

Returns:

  Status code

--*/
;

EFI_STATUS
SalSetWakeupTime (
  IN BOOLEAN                      Enable,
  IN EFI_TIME                     *Time
  )
/*++

Routine Description:

  Sets the system wakeup alarm clock time.

Arguments:

  Enable  - Enable or disable the wakeup alarm.
  Time    - If Enable is TRUE, the time to set the wakeup alarm for.
            If Enable is FALSE, then this parameter is optional, and may be NULL.

Returns:

  Status code

--*/
;

SAL_RETURN_REGS
SalInitializeThreshold (
  IN  VOID                        *ThresholdStruct,
  IN  UINT64                      Count,
  IN  UINT64                      Duration
  )
/*++

Routine Description:

  Init threshold structure.

Arguments:

  ThresholdStruct     - Threshold structure
  Count               - Threshold count
  Duration            - Duration

Returns:

  Output regs

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

  Bump threshold count.

Arguments:

  ThresholdStruct     - Threshold structure
  Count               - Threshold count
  Duration            - Duration

Returns:

  Output regs

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

  Get threshold structure.

Arguments:

  ThresholdStruct     - Threshold structure
  Count               - Threshold count
  Duration            - Duration

Returns:

  Output regs

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

  Register ESAL Class Function and it's asociated global.
  This function is boot service only!

Arguments:
  FunctionId    - ID of function to register
  ClassGuid     - GUID of function class 
  Function      - Function to register under ClassGuid/FunctionId pair
  ModuleGlobal  - Module global for Function.

Returns: 
  EFI_SUCCESS - If ClassGuid/FunctionId Function was registered.

--*/
;

EFI_STATUS
EfiInitializeSalDriverLib (
  IN  BOOLEAN   Runtime
  )
/*++

Routine Description:

  Initialize Sal driver lib.

Arguments:
  Runtime     - At runtime or not?

Returns: 
  Status code

--*/
;

//
// MCA PMI INIT Registeration Functions.
//
EFI_STATUS
LibRegisterMcaFunction (
  IN  EFI_SAL_MCA_HANDLER                   McaHandler,
  IN  VOID                                  *ModuleGlobal,
  IN  BOOLEAN                               MakeFirst,
  IN  BOOLEAN                               MakeLast
  )
/*++

Routine Description:

  Register MCA handler.

Arguments:
  McaHandler      - MCA handler
  ModuleGlobal    - Module global for function
  MakeFirst       - Make it as first?
  MakeLast        - Make it as last?

Returns: 
  Status code

--*/
;

EFI_STATUS
LibRegisterPmiFunction (
  IN  EFI_SAL_PMI_HANDLER                   PmiHandler,
  IN  VOID                                  *ModuleGlobal,
  IN  BOOLEAN                               MakeFirst,
  IN  BOOLEAN                               MakeLast
  )
/*++

Routine Description:

  Register PMI handler.

Arguments:
  PmiHandler      - PMI handler
  ModuleGlobal    - Module global for function
  MakeFirst       - Make it as first?
  MakeLast        - Make it as last?

Returns: 
  Status code

--*/
;

EFI_STATUS
LibRegisterInitFunction (
  IN  EFI_SAL_INIT_HANDLER                  InitHandler,
  IN  VOID                                  *ModuleGlobal,
  IN  BOOLEAN                               MakeFirst,
  IN  BOOLEAN                               MakeLast
  )
/*++

Routine Description:

  Register INIT handler.

Arguments:
  InitHandler     - INIT handler
  ModuleGlobal    - Module global for function
  MakeFirst       - Make it as first?
  MakeLast        - Make it as last?

Returns: 
  Status code

--*/
;

//
//  Base IO Class Functions
//
EFI_STATUS
ESalIoRead (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Io read operation.

Arguments:

  Width   - Width of read operation
  Address - Start IO address to read
  Count   - Read count
  Buffer  - Buffer to store result

Returns:

  Status code

--*/
;

EFI_STATUS
ESalIoWrite (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Io write operation.

Arguments:

  Width   - Width of write operation
  Address - Start IO address to write
  Count   - Write count
  Buffer  - Buffer to write to the address

Returns:

  Status code

--*/
;

EFI_STATUS
ESalMemRead (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH   Width,
  IN     UINT64                      Address,
  IN     UINTN                       Count,
  IN  OUT VOID                       *Buffer
  )
/*++

Routine Description:
  Perform a Memory mapped IO read into Buffer.

Arguments:
  Width   - Width of each read transaction.
  Address - Memory mapped IO address to read
  Count   - Number of Width quanta to read
  Buffer  - Buffer to read data into. size is Width * Count

Returns: 
  Status code

--*/
;

EFI_STATUS
ESalMemWrite (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH   Width,
  IN     UINT64                      Address,
  IN     UINTN                       Count,
  IN OUT VOID                        *Buffer
  )
/*++

Routine Description:
  Perform a memory mapped IO write into Buffer.

Arguments:
  Width   - Width of write transaction, and repeat operation to use
  Address - IO address to write
  Count   - Number of times to write the IO address.
  Buffer  - Buffer to write data from. size is Width * Count

Returns: 
  Status code

--*/
;

//
//  PCI Class Functions
//
SAL_RETURN_REGS
SalPCIConfigRead (
  IN  UINT64              Address,
  IN  UINT64              Size
  )
/*++

Routine Description:
  Pci config space read.

Arguments:
  Address - PCI address to read
  Size    - Size to read

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
SalPCIConfigWrite (
  IN  UINT64              Address,
  IN  UINT64              Size,
  IN  UINT64              Value
  )
/*++

Routine Description:
  Pci config space write.

Arguments:
  Address - PCI address to write
  Size    - Size to write
  Value   - Value to write

Returns: 
  Output regs

--*/
;

//
//  MP Class Functions
//
SAL_RETURN_REGS
LibMPAddCpuData (
  IN    UINT64      CpuGlobalId,
  IN    BOOLEAN     Enabled,
  IN    UINT64      PalCompatability
  )
/*++

Routine Description:
  Add CPU data.

Arguments:
  CpuGlobalId         - CPU ID
  Enabled             - Enabled or not
  PalCompatability    - Pal compatability

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMPRemoveCpuData (
  IN    UINT64      CpuGlobalId
  )
/*++

Routine Description:
  Remove CPU data.

Arguments:
  CpuGlobalId         - CPU ID

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMPModifyCpuData (
  IN    UINT64      CpuGlobalId,
  IN    BOOLEAN     Enabled,
  IN    UINT64      PalCompatability
  )
/*++

Routine Description:
  Modify CPU data.

Arguments:
  CpuGlobalId         - CPU ID
  Enabled             - Enabled or not
  PalCompatability    - Pal compatability

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMPGetCpuDataByID (
  IN    UINT64      CpuGlobalId,
  IN    BOOLEAN     IndexByEnabledCpu
  )
/*++

Routine Description:
  Get CPU data.

Arguments:
  CpuGlobalId         - CPU ID
  IndexByEnabledCpu   - Whether indexed by enabled CPU

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMPGetCpuDataByIndex (
  IN    UINT64      Index,
  IN    BOOLEAN     IndexByEnabledCpu
  )
/*++

Routine Description:
  Get CPU data.

Arguments:
  Index               - CPU index
  IndexByEnabledCpu   - Whether indexed by enabled CPU

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMPSendIpi (
  IN  UINT64                ProcessorNumber,
  IN  UINT64                VectorNumber,
  IN  EFI_DELIVERY_MODE     DeliveryMode,
  IN  BOOLEAN               IRFlag
  )
/*++

Routine Description:
  Send IPI.

Arguments:
  ProcessorNumber         - Processor number
  VectorNumber            - Vector number
  DeliveryMode            - Delivery mode
  IRFlag                  - Interrupt Redirection flag

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMpCurrentProcessor (
  IN    BOOLEAN     IndexByEnabledCpu
  )
/*++

Routine Description:
  Get current processor index.

Arguments:
  IndexByEnabledCpu       - Whether indexed by enabled CPU

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibGetNumProcessors (
  VOID
  )
/*++

Routine Description:
  Get number of processors.

Arguments:
  None

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMpSaveMinStatePointer (
  IN    UINT64                CpuGlobalId,
  IN    EFI_PHYSICAL_ADDRESS  MinStatePointer
  )
/*++

Routine Description:
  Register pointer to save min state.

Arguments:
  CpuGlobalId       - CPU global ID
  MinStatePointer   - Pointer to save min state

Returns: 
  Output regs

--*/
;

SAL_RETURN_REGS
LibMpRestoreMinStatePointer (
  IN    UINT64                CpuGlobalId
  )
/*++

Routine Description:
  Restore pointer to save min state.

Arguments:
  CpuGlobalId       - CPU global ID

Returns: 
  Output regs

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
  MCA get state info.

Arguments:
  CpuId                   - CPU ID
  StateBufferPointer      - Pointer of state buffer
  RequiredStateBufferSize - Size of required state buffer

Returns: 
  Status code

--*/
;

EFI_STATUS
LibMcaRegisterCpu (
  IN  UINT64                                      CpuId,
  IN  EFI_PHYSICAL_ADDRESS                        StateBufferAddress
  )
/*++

Routine Description:
  MCA register CPU state info.

Arguments:
  CpuId                   - CPU ID
  StateBufferAddress      - Pointer of state buffer

Returns: 
  Status code

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
  Get state info.

Arguments:
  McaType                 - MCA type
  McaBuffer               - Info buffer provided by caller
  Size                    - Size of info

Returns: 
  Status code

--*/
;

EFI_STATUS
LibSalGetStateInfoSize (
  IN  UINT64                                      McaType,
  OUT UINTN                                       *Size
  )
/*++

Routine Description:
  Get state info size.

Arguments:
  McaType                   - MCA type
  Size                      - Size required

Returns: 
  Status code

--*/
;

EFI_STATUS
LibSalClearStateInfo (
  IN  UINT64                                      McaType
  )
/*++

Routine Description:
  Clear state info.

Arguments:
  McaType                   - MCA type

Returns: 
  Status code

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
  Get state buffer.

Arguments:
  McaType                   - MCA type
  McaBuffer                 - MCA buffer
  Index                     - CPU index

Returns: 
  Status code

--*/
;

EFI_STATUS
LibEsalSaveStateBuffer (
  IN  UINT64                                      McaType
  )
/*++

Routine Description:
  Save state buffer.

Arguments:
  McaType                   - MCA type

Returns: 
  Status code

--*/
;

#endif

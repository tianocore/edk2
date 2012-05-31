/** @file
  Header file to define EFI SMM Base2 Protocol in the PI 1.2 specification.

  The thunk implementation for SmmScriptLib will ultilize the SmmSaveState Protocol to save SMM
  runtime s3 boot Script. This header file is to definied PI SMM related definition to locate 
  SmmSaveState Protocol  

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PI_SMM_DEFINITION_H_
#define _PI_SMM_DEFINITION_H_

typedef struct _EFI_SMM_CPU_IO2_PROTOCOL  EFI_SMM_CPU_IO2_PROTOCOL;

///
/// Width of the SMM CPU I/O operations
///
typedef enum {
  SMM_IO_UINT8  = 0,
  SMM_IO_UINT16 = 1,
  SMM_IO_UINT32 = 2,
  SMM_IO_UINT64 = 3
} EFI_SMM_IO_WIDTH;

/**
  Provides the basic memory and I/O interfaces used toabstract accesses to devices.

  The I/O operations are carried out exactly as requested.  The caller is responsible for any alignment 
  and I/O width issues that the bus, device, platform, or type of I/O might require.

  @param[in]      This           The EFI_SMM_CPU_IO2_PROTOCOL instance.
  @param[in]      Width          Signifies the width of the I/O operations.
  @param[in]      Address        The base address of the I/O operations.
                                 The caller is responsible for aligning the Address if required. 
  @param[in]      Count          The number of I/O operations to perform.
  @param[in,out]  Buffer         For read operations, the destination buffer to store the results.
                                 For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the device.
  @retval EFI_UNSUPPORTED        The Address is not valid for this system.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CPU_IO2)(
  IN CONST EFI_SMM_CPU_IO2_PROTOCOL  *This,
  IN EFI_SMM_IO_WIDTH               Width,
  IN UINT64                         Address,
  IN UINTN                          Count,
  IN OUT VOID                       *Buffer
  );

typedef struct {
  ///
  /// This service provides the various modalities of memory and I/O read.
  ///
  EFI_SMM_CPU_IO2  Read;
  ///
  /// This service provides the various modalities of memory and I/O write.
  ///
  EFI_SMM_CPU_IO2  Write;
} EFI_SMM_IO_ACCESS2;

///
/// SMM CPU I/O 2 Protocol provides CPU I/O and memory access within SMM.
///
struct _EFI_SMM_CPU_IO2_PROTOCOL {
  EFI_SMM_IO_ACCESS2 Mem;  ///< Allows reads and writes to memory-mapped I/O space.
  EFI_SMM_IO_ACCESS2 Io;   ///< Allows reads and writes to I/O space.
};
typedef struct _EFI_SMM_SYSTEM_TABLE2  EFI_SMM_SYSTEM_TABLE2;
/**
  Adds, updates, or removes a configuration table entry from the System Management System Table.

  The SmmInstallConfigurationTable() function is used to maintain the list
  of configuration tables that are stored in the System Management System
  Table.  The list is stored as an array of (GUID, Pointer) pairs.  The list
  must be allocated from pool memory with PoolType set to EfiRuntimeServicesData.

  @param[in] SystemTable         A pointer to the SMM System Table (SMST).
  @param[in] Guid                A pointer to the GUID for the entry to add, update, or remove.
  @param[in] Table               A pointer to the buffer of the table to add.
  @param[in] TableSize           The size of the table to install.

  @retval EFI_SUCCESS            The (Guid, Table) pair was added, updated, or removed.
  @retval EFI_INVALID_PARAMETER  Guid is not valid.
  @retval EFI_NOT_FOUND          An attempt was made to delete a non-existent entry.
  @retval EFI_OUT_OF_RESOURCES   There is not enough memory available to complete the operation.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSTALL_CONFIGURATION_TABLE2)(
  IN CONST EFI_SMM_SYSTEM_TABLE2  *SystemTable,
  IN CONST EFI_GUID               *Guid,
  IN VOID                         *Table,
  IN UINTN                        TableSize
  );
/**
  Function prototype for invoking a function on an Application Processor.

  This definition is used by the UEFI MP Serices Protocol, and the
  PI SMM System Table.

  @param[in,out] Buffer  Pointer to private data buffer.
**/
typedef
VOID
(EFIAPI *EFI_AP_PROCEDURE)(
  IN OUT VOID  *Buffer
  );
/**
  The SmmStartupThisAp() lets the caller to get one distinct application processor
  (AP) in the enabled processor pool to execute a caller-provided code stream
  while in SMM. It runs the given code on this processor and reports the status.
  It must be noted that the supplied code stream will be run only on an enabled 
  processor which has also entered SMM. 

  @param[in]     Procedure       A pointer to the code stream to be run on the designated AP of the system.
  @param[in]     CpuNumber       The zero-based index of the processor number of the AP on which the code stream is supposed to run.
  @param[in,out] ProcArguments   Allow the caller to pass a list of parameters to the code that is run by the AP.

  @retval EFI_SUCCESS            The call was successful and the return parameters are valid.
  @retval EFI_INVALID_PARAMETER  The input arguments are out of range.
  @retval EFI_INVALID_PARAMETER  The CPU requested is not available on this SMI invocation.
  @retval EFI_INVALID_PARAMETER  The CPU cannot support an additional service invocation.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_STARTUP_THIS_AP)(
  IN EFI_AP_PROCEDURE  Procedure,
  IN UINTN             CpuNumber,
  IN OUT VOID          *ProcArguments OPTIONAL
  );

/**
  Function prototype for protocol install notification.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @return Status Code
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_NOTIFY_FN)(
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  );

/**
  Register a callback function be called when a particular protocol interface is installed.

  The SmmRegisterProtocolNotify() function creates a registration Function that is to be 
  called whenever a protocol interface is installed for Protocol by 
  SmmInstallProtocolInterface().

  @param[in]  Protocol          The unique ID of the protocol for which the event is to be registered.
  @param[in]  Function          Points to the notification function.
  @param[out] Registration      A pointer to a memory location to receive the registration value.

  @retval EFI_SUCCESS           Successfully returned the registration record that has been added.
  @retval EFI_INVALID_PARAMETER One or more of Protocol, Function and Registration is NULL.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory resource to finish the request.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REGISTER_PROTOCOL_NOTIFY)(
  IN  CONST EFI_GUID     *Protocol,
  IN  EFI_SMM_NOTIFY_FN  Function,
  OUT VOID               **Registration
  );

/**
  Manage SMI of a particular type.

  @param[in]     HandlerType     Points to the handler type or NULL for root SMI handlers.
  @param[in]     Context         Points to an optional context buffer.
  @param[in,out] CommBuffer      Points to the optional communication buffer.
  @param[in,out] CommBufferSize  Points to the size of the optional communication buffer.

  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING  Interrupt source was processed successfully but not quiesced.
  @retval EFI_INTERRUPT_PENDING              One or more SMI sources could not be quiesced.
  @retval EFI_NOT_FOUND                      Interrupt source was not handled or quiesced.
  @retval EFI_SUCCESS                        Interrupt source was handled and quiesced.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INTERRUPT_MANAGE)(
  IN CONST EFI_GUID  *HandlerType,
  IN CONST VOID      *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  );

/**
  Main entry point for an SMM handler dispatch or communicate-based callback.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers 
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should 
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still 
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_HANDLER_ENTRY_POINT2)(
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
  );

/**
  Registers a handler to execute within SMM.

  @param[in]  Handler            Handler service funtion pointer.
  @param[in]  HandlerType        Points to the handler type or NULL for root SMI handlers.
  @param[out] DispatchHandle     On return, contains a unique handle which can be used to later
                                 unregister the handler function.

  @retval EFI_SUCCESS            SMI handler added successfully.
  @retval EFI_INVALID_PARAMETER  Handler is NULL or DispatchHandle is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INTERRUPT_REGISTER)(
  IN  EFI_SMM_HANDLER_ENTRY_POINT2  Handler,
  IN  CONST EFI_GUID                *HandlerType OPTIONAL,
  OUT EFI_HANDLE                    *DispatchHandle
  );

/**
  Unregister a handler in SMM.

  @param[in] DispatchHandle      The handle that was specified when the handler was registered.

  @retval EFI_SUCCESS            Handler function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER  DispatchHandle does not refer to a valid handle.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INTERRUPT_UNREGISTER)(
  IN EFI_HANDLE  DispatchHandle
  );

///
/// Processor information and functionality needed by SMM Foundation.
///
typedef struct _EFI_SMM_ENTRY_CONTEXT {
  EFI_SMM_STARTUP_THIS_AP  SmmStartupThisAp;
  ///
  /// A number between zero and the NumberOfCpus field. This field designates which 
  /// processor is executing the SMM Foundation.
  ///
  UINTN                    CurrentlyExecutingCpu;
  ///
  /// The number of current operational processors in the platform.  This is a 1 based 
  /// counter.  This does not indicate the number of processors that entered SMM.
  ///
  UINTN                    NumberOfCpus;
  ///
  /// Points to an array, where each element describes the number of bytes in the 
  /// corresponding save state specified by CpuSaveState. There are always 
  /// NumberOfCpus entries in the array. 
  ///
  UINTN                    *CpuSaveStateSize;
  ///
  /// Points to an array, where each element is a pointer to a CPU save state. The 
  /// corresponding element in CpuSaveStateSize specifies the number of bytes in the 
  /// save state area. There are always NumberOfCpus entries in the array.
  ///
  VOID                     **CpuSaveState;
} EFI_SMM_ENTRY_CONTEXT;

/**
  This function is the main entry point to the SMM Foundation.

  @param[in] SmmEntryContext  Processor information and functionality needed by SMM Foundation.
**/
typedef
VOID
(EFIAPI *EFI_SMM_ENTRY_POINT)(
  IN CONST EFI_SMM_ENTRY_CONTEXT  *SmmEntryContext
  );

///
/// System Management System Table (SMST)
///
/// The System Management System Table (SMST) is a table that contains a collection of common 
/// services for managing SMRAM allocation and providing basic I/O services. These services are 
/// intended for both preboot and runtime usage.
///
struct _EFI_SMM_SYSTEM_TABLE2 {
  ///
  /// The table header for the SMST.
  ///
  EFI_TABLE_HEADER                     Hdr;
  ///
  /// A pointer to a NULL-terminated Unicode string containing the vendor name.
  /// It is permissible for this pointer to be NULL.
  ///
  CHAR16                               *SmmFirmwareVendor;
  ///
  /// The particular revision of the firmware.
  ///
  UINT32                               SmmFirmwareRevision;

  EFI_SMM_INSTALL_CONFIGURATION_TABLE2 SmmInstallConfigurationTable;

  ///
  /// I/O Service
  ///
  EFI_SMM_CPU_IO2_PROTOCOL             SmmIo;

  ///
  /// Runtime memory services
  ///
  EFI_ALLOCATE_POOL                    SmmAllocatePool;
  EFI_FREE_POOL                        SmmFreePool;
  EFI_ALLOCATE_PAGES                   SmmAllocatePages;
  EFI_FREE_PAGES                       SmmFreePages;

  ///
  /// MP service
  ///
  EFI_SMM_STARTUP_THIS_AP              SmmStartupThisAp;

  ///
  /// CPU information records
  ///

  ///
  /// A number between zero and and the NumberOfCpus field. This field designates 
  /// which processor is executing the SMM infrastructure.
  ///
  UINTN                                CurrentlyExecutingCpu;
  ///
  /// The number of current operational processors in the platform.  This is a 1 based counter.
  ///
  UINTN                                NumberOfCpus;
  ///
  /// Points to an array, where each element describes the number of bytes in the 
  /// corresponding save state specified by CpuSaveState. There are always 
  /// NumberOfCpus entries in the array. 
  ///
  UINTN                                *CpuSaveStateSize;
  ///
  /// Points to an array, where each element is a pointer to a CPU save state. The 
  /// corresponding element in CpuSaveStateSize specifies the number of bytes in the 
  /// save state area. There are always NumberOfCpus entries in the array.
  ///
  VOID                                 **CpuSaveState;

  ///
  /// Extensibility table
  ///

  ///
  /// The number of UEFI Configuration Tables in the buffer SmmConfigurationTable.
  ///
  UINTN                                NumberOfTableEntries;
  ///
  /// A pointer to the UEFI Configuration Tables. The number of entries in the table is 
  /// NumberOfTableEntries. 
  ///
  EFI_CONFIGURATION_TABLE              *SmmConfigurationTable;

  ///
  /// Protocol services
  ///
  EFI_INSTALL_PROTOCOL_INTERFACE       SmmInstallProtocolInterface;
  EFI_UNINSTALL_PROTOCOL_INTERFACE     SmmUninstallProtocolInterface;
  EFI_HANDLE_PROTOCOL                  SmmHandleProtocol;
  EFI_SMM_REGISTER_PROTOCOL_NOTIFY     SmmRegisterProtocolNotify;
  EFI_LOCATE_HANDLE                    SmmLocateHandle;
  EFI_LOCATE_PROTOCOL                  SmmLocateProtocol;

  ///
  /// SMI Management functions
  ///
  EFI_SMM_INTERRUPT_MANAGE             SmiManage;
  EFI_SMM_INTERRUPT_REGISTER           SmiHandlerRegister;
  EFI_SMM_INTERRUPT_UNREGISTER         SmiHandlerUnRegister;
};

#define EFI_SMM_BASE2_PROTOCOL_GUID \
  { \
    0xf4ccbfb7, 0xf6e0, 0x47fd, {0x9d, 0xd4, 0x10, 0xa8, 0xf1, 0x50, 0xc1, 0x91 }  \
  }

typedef struct _EFI_SMM_BASE2_PROTOCOL  EFI_SMM_BASE2_PROTOCOL;

/**
  Service to indicate whether the driver is currently executing in the SMM Initialization phase.
  
  This service is used to indicate whether the driver is currently executing in the SMM Initialization 
  phase. For SMM drivers, this will return TRUE in InSmram while inside the driver's entry point and 
  otherwise FALSE. For combination SMM/DXE drivers, this will return FALSE in the DXE launch. For the
  SMM launch, it behaves as an SMM driver.

  @param[in]  This               The EFI_SMM_BASE2_PROTOCOL instance. 
  @param[out] InSmram            Pointer to a Boolean which, on return, indicates that the driver is
                                 currently executing inside of SMRAM (TRUE) or outside of SMRAM (FALSE).

  @retval EFI_SUCCESS            The call returned successfully.
  @retval EFI_INVALID_PARAMETER  InSmram was NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSIDE_OUT2)(
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT BOOLEAN                      *InSmram
  )
;

/**
  Returns the location of the System Management Service Table (SMST).

  This function returns the location of the System Management Service Table (SMST).  The use of the 
  API is such that a driver can discover the location of the SMST in its entry point and then cache it in 
  some driver global variable so that the SMST can be invoked in subsequent handlers.

  @param[in]     This            The EFI_SMM_BASE2_PROTOCOL instance.
  @param[in,out] Smst            On return, points to a pointer to the System Management Service Table (SMST).

  @retval EFI_SUCCESS            The operation was successful.
  @retval EFI_INVALID_PARAMETER  Smst was invalid.
  @retval EFI_UNSUPPORTED        Not in SMM.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_SMST_LOCATION2)(
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  IN OUT EFI_SMM_SYSTEM_TABLE2     **Smst
  )
;

///
/// EFI SMM Base2 Protocol is utilized by all SMM drivers to locate the SMM infrastructure
/// services and determine whether the driver is being invoked inside SMRAM or outside of SMRAM.
///
struct _EFI_SMM_BASE2_PROTOCOL {
  EFI_SMM_INSIDE_OUT2         InSmm;
  EFI_SMM_GET_SMST_LOCATION2  GetSmstLocation;
};


#endif


/** @file
  Include file for definitions in the Intel Platform Innovation Framework for EFI
  System Management Mode Core Interface Specification (SMM CIS) version 0.91.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_CIS_H_
#define _SMM_CIS_H_

//
// Share some common definitions with PI SMM
//
#include <Pi/PiSmmCis.h>
#include <Protocol/SmmCpuIo.h>

typedef struct _EFI_SMM_SYSTEM_TABLE      EFI_SMM_SYSTEM_TABLE;

//
// SMM Base specification constant and types
//
#define EFI_SMM_SYSTEM_TABLE_REVISION (0 << 16) | (0x09)

/**
  Allocates pool memory from SMRAM for IA-32, or runtime memory for
  the Itanium processor family.

  @param  PoolType         The type of pool to allocate. The only supported type
                           is EfiRuntimeServicesData.
  @param  Size             The number of bytes to allocate from the pool.
  @param  Buffer           A pointer to a pointer to the allocated buffer if the
                           call succeeds.  Otherwise, undefined.

  @retval EFI_SUCCESS           The requested number of bytes was allocated.
  @retval EFI_OUT_OF_RESOURCES  The pool requested could not be allocated.
  @retval EFI_UNSUPPORTED       In runtime.
  @note  Inconsistent with specification here:
         In Framework Spec, this definition is named EFI_SMM_ALLOCATE_POOL.
         To avoid a naming conflict, the definition is renamed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_ALLOCATE_POOL)(
  IN EFI_MEMORY_TYPE                PoolType,
  IN UINTN                          Size,
  OUT VOID                          **Buffer
  );

/**
  Returns pool memory to the system.

  @param  Buffer           The pointer to the buffer to free.

  @retval EFI_SUCCESS           The memory was returned to the system.
  @retval EFI_INVALID_PARAMETER Buffer was invalid.
  @retval EFI_UNSUPPORTED       In runtime.
  @note  Inconsistent with specification here:
         In Framework Spec, this definition is named EFI_SMM_FREE_POOL.
         To avoid a naming conflict, the definition is renamed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_FREE_POOL)(
  IN VOID                   *Buffer
  );

/**
  Allocates memory pages from the system.

  @param  Type             The type of allocation to perform.
  @param  MemoryType       The only supported type is EfiRuntimeServicesData.
  @param  NumberofPages    The number of contiguous 4 KB pages to allocate.
  @param  Memory           Pointer to a physical address. On input, the way in which
                           the address is used depends on the value of Type. On output, the address
                           is set to the base of the page range that was allocated.

  @retval EFI_SUCCESS           The requested pages were allocated.
  @retval EFI_OUT_OF_RESOURCES  The pages requested could not be allocated.
  @retval EFI_NOT_FOUND         The requested pages could not be found.
  @retval EFI_INVALID_PARAMETER Type is not AllocateAnyPages or AllocateMaxAddress
                                or AllocateAddress. Or, MemoryType is in the range EfiMaxMemoryType..0x7FFFFFFF.
  @note  Inconsistent with specification here:
         In the Framework Spec, this definition is named EFI_SMM_ALLOCATE_PAGES.
         To avoid a naming conflict, the definition here is renamed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_ALLOCATE_PAGES)(
  IN EFI_ALLOCATE_TYPE      Type,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory
  );

/**
  Frees memory pages for the system.

  @param  Memory           The base physical address of the pages to be freed.
  @param  NumberOfPages    The number of contiguous 4 KB pages to free.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER Memory is not a page-aligned address or NumberOfPages is invalid.
  @retval EFI_NOT_FOUND         The requested memory pages were not allocated with SmmAllocatePages().

  @note  Inconsistent with specification here:
         In the Framework Spec, this definition is named EFI_SMM_FREE_PAGES.
         To avoid a naming conflict, the definition here is renamed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_FREE_PAGES)(
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NumberOfPages
  );

///
/// The processor save-state information for IA-32 processors. This information is important in that the
/// SMM drivers may need to ascertain the state of the processor before invoking the SMI.
///
typedef struct {
  ///
  /// Reserved for future processors. As such, software should not attempt to interpret or
  /// write to this region.
  ///
  UINT8                 Reserved1[248];
  ///
  /// The location of the processor SMBASE, which is the location where the processor
  /// will pass control upon receipt of an SMI.
  ///
  UINT32                SMBASE;
  ///
  /// The revision of the SMM save state. This value is set by the processor.
  ///
  UINT32                SMMRevId;
  ///
  /// The value of the I/O restart field. Allows for restarting an in-process I/O instruction.
  ///
  UINT16                IORestart;
  ///
  /// Describes behavior that should be commenced in response to a halt instruction.
  ///
  UINT16                AutoHALTRestart;
  ///
  /// Reserved for future processors. As such, software should not attempt to interpret or
  /// write to this region.
  ///
  UINT8                 Reserved2[164];

  //
  // Registers in IA-32 processors.
  //
  UINT32                ES;
  UINT32                CS;
  UINT32                SS;
  UINT32                DS;
  UINT32                FS;
  UINT32                GS;
  UINT32                LDTBase;
  UINT32                TR;
  UINT32                DR7;
  UINT32                DR6;
  UINT32                EAX;
  UINT32                ECX;
  UINT32                EDX;
  UINT32                EBX;
  UINT32                ESP;
  UINT32                EBP;
  UINT32                ESI;
  UINT32                EDI;
  UINT32                EIP;
  UINT32                EFLAGS;
  UINT32                CR3;
  UINT32                CR0;
} EFI_SMI_CPU_SAVE_STATE;

///
/// The processor save-state information for the Itanium processor family. This information is
/// important in that the SMM drivers may need to ascertain the state of the processor before invoking
/// the PMI. This structure is mandatory and must be 512 byte aligned.
///
typedef struct {
  UINT64   reserved;
  UINT64   r1;
  UINT64   r2;
  UINT64   r3;
  UINT64   r4;
  UINT64   r5;
  UINT64   r6;
  UINT64   r7;
  UINT64   r8;
  UINT64   r9;
  UINT64   r10;
  UINT64   r11;
  UINT64   r12;
  UINT64   r13;
  UINT64   r14;
  UINT64   r15;
  UINT64   r16;
  UINT64   r17;
  UINT64   r18;
  UINT64   r19;
  UINT64   r20;
  UINT64   r21;
  UINT64   r22;
  UINT64   r23;
  UINT64   r24;
  UINT64   r25;
  UINT64   r26;
  UINT64   r27;
  UINT64   r28;
  UINT64   r29;
  UINT64   r30;
  UINT64   r31;

  UINT64   pr;

  UINT64   b0;
  UINT64   b1;
  UINT64   b2;
  UINT64   b3;
  UINT64   b4;
  UINT64   b5;
  UINT64   b6;
  UINT64   b7;

  // application registers
  UINT64   ar_rsc;
  UINT64   ar_bsp;
  UINT64   ar_bspstore;
  UINT64   ar_rnat;

  UINT64   ar_fcr;

  UINT64   ar_eflag;
  UINT64   ar_csd;
  UINT64   ar_ssd;
  UINT64   ar_cflg;
  UINT64   ar_fsr;
  UINT64   ar_fir;
  UINT64   ar_fdr;

  UINT64   ar_ccv;

  UINT64   ar_unat;

  UINT64   ar_fpsr;

  UINT64   ar_pfs;
  UINT64   ar_lc;
  UINT64   ar_ec;

  // control registers
  UINT64   cr_dcr;
  UINT64   cr_itm;
  UINT64   cr_iva;
  UINT64   cr_pta;
  UINT64   cr_ipsr;
  UINT64   cr_isr;
  UINT64   cr_iip;
  UINT64   cr_ifa;
  UINT64   cr_itir;
  UINT64   cr_iipa;
  UINT64   cr_ifs;
  UINT64   cr_iim;
  UINT64   cr_iha;

  // debug registers
  UINT64   dbr0;
  UINT64   dbr1;
  UINT64   dbr2;
  UINT64   dbr3;
  UINT64   dbr4;
  UINT64   dbr5;
  UINT64   dbr6;
  UINT64   dbr7;

  UINT64   ibr0;
  UINT64   ibr1;
  UINT64   ibr2;
  UINT64   ibr3;
  UINT64   ibr4;
  UINT64   ibr5;
  UINT64   ibr6;
  UINT64   ibr7;

  // virtual registers
  UINT64   int_nat;         // nat bits for R1-R31

} EFI_PMI_SYSTEM_CONTEXT;

///
/// The processor save-state information for IA-32 and Itanium processors. This information is
/// important in that the SMM drivers may need to ascertain the state of the processor before invoking
/// the SMI or PMI.
///
typedef union {
  ///
  /// The processor save-state information for IA-32 processors.
  ///
  EFI_SMI_CPU_SAVE_STATE     Ia32SaveState;
  ///
  /// Note: Inconsistency with the Framework SMM CIS spec - Itanium save state not included.
  ///
  /// The processor save-state information for Itanium processors.
  ///
  /// EFI_PMI_SYSTEM_CONTEXT ItaniumSaveState;
} EFI_SMM_CPU_SAVE_STATE;

///
/// The optional floating point save-state information for IA-32 processors. If the optional floating
/// point save is indicated for any handler, the following data structure must be preserved.
///
typedef struct {
  UINT16                Fcw;
  UINT16                Fsw;
  UINT16                Ftw;
  UINT16                Opcode;
  UINT32                Eip;
  UINT16                Cs;
  UINT16                Rsvd1;
  UINT32                DataOffset;
  UINT16                Ds;
  UINT8                 Rsvd2[10];
  UINT8                 St0Mm0[10], Rsvd3[6];
  UINT8                 St0Mm1[10], Rsvd4[6];
  UINT8                 St0Mm2[10], Rsvd5[6];
  UINT8                 St0Mm3[10], Rsvd6[6];
  UINT8                 St0Mm4[10], Rsvd7[6];
  UINT8                 St0Mm5[10], Rsvd8[6];
  UINT8                 St0Mm6[10], Rsvd9[6];
  UINT8                 St0Mm7[10], Rsvd10[6];
  UINT8                 Rsvd11[22*16];
} EFI_SMI_OPTIONAL_FPSAVE_STATE;

///
/// The optional floating point save-state information for the Itanium processor family. If the optional
/// floating point save is indicated for any handler, then this data structure must be preserved.
///
typedef struct {
  UINT64   f2[2];
  UINT64   f3[2];
  UINT64   f4[2];
  UINT64   f5[2];
  UINT64   f6[2];
  UINT64   f7[2];
  UINT64   f8[2];
  UINT64   f9[2];
  UINT64   f10[2];
  UINT64   f11[2];
  UINT64   f12[2];
  UINT64   f13[2];
  UINT64   f14[2];
  UINT64   f15[2];
  UINT64   f16[2];
  UINT64   f17[2];
  UINT64   f18[2];
  UINT64   f19[2];
  UINT64   f20[2];
  UINT64   f21[2];
  UINT64   f22[2];
  UINT64   f23[2];
  UINT64   f24[2];
  UINT64   f25[2];
  UINT64   f26[2];
  UINT64   f27[2];
  UINT64   f28[2];
  UINT64   f29[2];
  UINT64   f30[2];
  UINT64   f31[2];
} EFI_PMI_OPTIONAL_FLOATING_POINT_CONTEXT;

///
/// The processor save-state information for IA-32 and Itanium processors. If the optional floating
/// point save is indicated for any handler, then this data structure must be preserved.
///
typedef union {
  ///
  /// The optional floating point save-state information for IA-32 processors.
  ///
  EFI_SMI_OPTIONAL_FPSAVE_STATE             Ia32FpSave;
  ///
  /// The optional floating point save-state information for Itanium processors.
  ///
  EFI_PMI_OPTIONAL_FLOATING_POINT_CONTEXT   ItaniumFpSave;
} EFI_SMM_FLOATING_POINT_SAVE_STATE;

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  SmmImageHandle   A unique value returned by the SMM infrastructure
                           in response to registration for a communicate-based callback or dispatch.
  @param  CommunicationBuffer
                           An optional buffer that will be populated
                           by the SMM infrastructure in response to a non-SMM agent (preboot or runtime)
                           invoking the EFI_SMM_BASE_PROTOCOL.Communicate() service.
  @param  SourceSize       If CommunicationBuffer is non-NULL, this field
                           indicates the size of the data payload in this buffer.

  @return Status Code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_HANDLER_ENTRY_POINT)(
  IN EFI_HANDLE             SmmImageHandle,
  IN OUT VOID               *CommunicationBuffer OPTIONAL,
  IN OUT UINTN              *SourceSize OPTIONAL
  );

/**
  The SmmInstallConfigurationTable() function is used to maintain the list
  of configuration tables that are stored in the System Management System
  Table.  The list is stored as an array of (GUID, Pointer) pairs.  The list
  must be allocated from pool memory with PoolType set to EfiRuntimeServicesData.

  @param  SystemTable      A pointer to the SMM System Table.
  @param  Guid             A pointer to the GUID for the entry to add, update, or remove.
  @param  Table            A pointer to the buffer of the table to add.
  @param  TableSize        The size of the table to install.

  @retval EFI_SUCCESS           The (Guid, Table) pair was added, updated, or removed.
  @retval EFI_INVALID_PARAMETER Guid is not valid.
  @retval EFI_NOT_FOUND         An attempt was made to delete a non-existent entry.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available to complete the operation.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSTALL_CONFIGURATION_TABLE)(
  IN EFI_SMM_SYSTEM_TABLE         *SystemTable,
  IN EFI_GUID                     *Guid,
  IN VOID                         *Table,
  IN UINTN                        TableSize
  );

//
// System Management System Table (SMST)
//
struct _EFI_SMM_SYSTEM_TABLE {
  ///
  /// The table header for the System Management System Table (SMST).
  ///
  EFI_TABLE_HEADER                    Hdr;

  ///
  /// A pointer to a NULL-terminated Unicode string containing the vendor name. It is
  /// permissible for this pointer to be NULL.
  ///
  CHAR16                              *SmmFirmwareVendor;
  ///
  /// The particular revision of the firmware.
  ///
  UINT32                              SmmFirmwareRevision;

  ///
  /// Adds, updates, or removes a configuration table entry from the SMST.
  ///
  EFI_SMM_INSTALL_CONFIGURATION_TABLE SmmInstallConfigurationTable;

  //
  // I/O Services
  //
  ///
  /// A GUID that designates the particular CPU I/O services.
  ///
  EFI_GUID                            EfiSmmCpuIoGuid;
  ///
  /// Provides the basic memory and I/O interfaces that are used to abstract accesses to
  /// devices.
  ///
  EFI_SMM_CPU_IO_INTERFACE            SmmIo;

  //
  // Runtime memory service
  //
  ///
  ///
  /// Allocates pool memory from SMRAM for IA-32 or runtime memory for the
  /// Itanium processor family.
  ///
  EFI_SMMCORE_ALLOCATE_POOL           SmmAllocatePool;
  ///
  /// Returns pool memory to the system.
  ///
  EFI_SMMCORE_FREE_POOL               SmmFreePool;
  ///
  /// Allocates memory pages from the system.
  ///
  EFI_SMMCORE_ALLOCATE_PAGES          SmmAllocatePages;
  ///
  /// Frees memory pages for the system.
  ///
  EFI_SMMCORE_FREE_PAGES              SmmFreePages;

  //
  // MP service
  //

  /// Inconsistent with specification here:
  ///  In Framework Spec, this definition does not exist. This method is introduced in PI1.1 specification for
  ///  the implementation needed.
  EFI_SMM_STARTUP_THIS_AP             SmmStartupThisAp;

  //
  // CPU information records
  //
  ///
  /// A 1-relative number between 1 and the NumberOfCpus field. This field designates
  /// which processor is executing the SMM infrastructure. This number also serves as an
  /// index into the CpuSaveState and CpuOptionalFloatingPointState
  /// fields.
  ///
  UINTN                               CurrentlyExecutingCpu;
  ///
  /// The number of EFI Configuration Tables in the buffer
  /// SmmConfigurationTable.
  ///
  UINTN                               NumberOfCpus;
  ///
  /// A pointer to the EFI Configuration Tables. The number of entries in the table is
  /// NumberOfTableEntries.
  ///
  EFI_SMM_CPU_SAVE_STATE              *CpuSaveState;
  ///
  /// A pointer to a catenation of the EFI_SMM_FLOATING_POINT_SAVE_STATE.
  /// The size of this entire table is NumberOfCpus* size of the
  /// EFI_SMM_FLOATING_POINT_SAVE_STATE. These fields are populated only if
  /// there is at least one SMM driver that has registered for a callback with the
  /// FloatingPointSave field in EFI_SMM_BASE_PROTOCOL.RegisterCallback() set to TRUE.
  ///
  EFI_SMM_FLOATING_POINT_SAVE_STATE   *CpuOptionalFloatingPointState;

  //
  // Extensibility table
  //
  ///
  /// The number of EFI Configuration Tables in the buffer
  /// SmmConfigurationTable.
  ///
  UINTN                               NumberOfTableEntries;
  ///
  /// A pointer to the EFI Configuration Tables. The number of entries in the table is
  /// NumberOfTableEntries.
  ///
  EFI_CONFIGURATION_TABLE             *SmmConfigurationTable;
};

#endif

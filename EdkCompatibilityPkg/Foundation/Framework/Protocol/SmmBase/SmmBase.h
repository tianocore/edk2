/*++

Copyright (c) 1999 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmmBase.h

Abstract:

  This file defines SMM Base abstraction protocol defined by the SMM Architecture
  Specification.  This is the base level of compatiblity for SMM drivers.

--*/

#ifndef _SMM_BASE_H_
#define _SMM_BASE_H_

#include EFI_PROTOCOL_DEFINITION (DevicePath)

#define EFI_SMM_BASE_PROTOCOL_GUID \
  { \
    0x1390954D, 0xda95, 0x4227, {0x93, 0x28, 0x72, 0x82, 0xc2, 0x17, 0xda, 0xa8} \
  }

#define EFI_SMM_CPU_IO_GUID \
  { \
    0x5f439a0b, 0x45d8, 0x4682, {0xa4, 0xf4, 0xf0, 0x57, 0x6b, 0x51, 0x34, 0x41} \
  }

#define SMM_COMMUNICATE_HEADER_GUID \
  { \
    0xF328E36C, 0x23B6, 0x4a95, {0x85, 0x4B, 0x32, 0xE1, 0x95, 0x34, 0xCD, 0x75} \
  }

//
// SMM Base specification constant and types
//
#define SMM_SMST_SIGNATURE            EFI_SIGNATURE_32 ('S', 'M', 'S', 'T')
#define EFI_SMM_SYSTEM_TABLE_REVISION (0 << 16) | (0x09)

EFI_FORWARD_DECLARATION (EFI_SMM_BASE_PROTOCOL);
EFI_FORWARD_DECLARATION (EFI_SMM_CPU_IO_INTERFACE);
EFI_FORWARD_DECLARATION (EFI_SMM_CPU_SAVE_STATE);
EFI_FORWARD_DECLARATION (EFI_SMM_OPTIONAL_FP_SAVE_STATE);
EFI_FORWARD_DECLARATION (EFI_SMM_SYSTEM_TABLE);

//
// *******************************************************
// EFI_SMM_IO_WIDTH
// *******************************************************
//
typedef enum {
  SMM_IO_UINT8  = 0,
  SMM_IO_UINT16 = 1,
  SMM_IO_UINT32 = 2,
  SMM_IO_UINT64 = 3
} EFI_SMM_IO_WIDTH;

//
// *******************************************************
// EFI_SMM_IO_ACCESS
// *******************************************************
//
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CPU_IO) (
  IN EFI_SMM_CPU_IO_INTERFACE         * This,
  IN EFI_SMM_IO_WIDTH                 Width,
  IN UINT64                           Address,
  IN UINTN                            Count,
  IN OUT VOID                         *Buffer
  );

typedef struct {
  EFI_SMM_CPU_IO  Read;
  EFI_SMM_CPU_IO  Write;
} EFI_SMM_IO_ACCESS;

struct _EFI_SMM_CPU_IO_INTERFACE {
  EFI_SMM_IO_ACCESS Mem;
  EFI_SMM_IO_ACCESS Io;
};

typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_ALLOCATE_POOL) (
  IN EFI_MEMORY_TYPE                PoolType,
  IN UINTN                          Size,
  OUT VOID                          **Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_FREE_POOL) (
  IN VOID                   *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_ALLOCATE_PAGES) (
  IN EFI_ALLOCATE_TYPE      Type,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  * Memory
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMMCORE_FREE_PAGES) (
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NumberOfPages
  );

typedef
VOID
(EFIAPI *EFI_AP_PROCEDURE) (
  IN  VOID                              *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_STARTUP_THIS_AP) (
  IN  EFI_AP_PROCEDURE                    Procedure,
  IN  UINTN                               CpuNumber,
  IN  OUT VOID                            *ProcArguments OPTIONAL
  );

struct _EFI_SMM_CPU_SAVE_STATE {
  UINT8   Reserved1[248];
  UINT32  SMBASE;
  UINT32  SMMRevId;
  UINT16  IORestart;
  UINT16  AutoHALTRestart;
  UINT8   Reserved2[164];
  UINT32  ES;
  UINT32  CS;
  UINT32  SS;
  UINT32  DS;
  UINT32  FS;
  UINT32  GS;
  UINT32  LDTBase;
  UINT32  TR;
  UINT32  DR7;
  UINT32  DR6;
  UINT32  EAX;
  UINT32  ECX;
  UINT32  EDX;
  UINT32  EBX;
  UINT32  ESP;
  UINT32  EBP;
  UINT32  ESI;
  UINT32  EDI;
  UINT32  EIP;
  UINT32  EFLAGS;
  UINT32  CR3;
  UINT32  CR0;
};

typedef struct {
  UINT8   Reserved19[760];    // FC00
  UINT32  SMBASE;             // FEF8
  UINT32  REVID;              // FEFC
  UINT16  HALT_RESTART;       // FF00
  UINT16  IO_RESTART;         // FF02
  UINT32  Reserved17[22];     // FF58, 54, 50, 4c, 48, 44, 40, 3c, 38, 34, 30, 2c, 28, 24, 20, 1c, 18, 14, 10, 0c, 08, 04
  UINT32  EAX;                // FF5C
  UINT32  Reserved16;         // FF60
  UINT32  ECX;                // FF64
  UINT32  Reserved15;         // FF68
  UINT32  EDX;                // FF6C
  UINT32  Reserved14;         // FF70
  UINT32  EBX;                // FF74
  UINT32  Reserved13;         // FF78
  UINT32  ESP;                // FF7C
  UINT32  Reserved12;         // FF80
  UINT32  EBP;                // FF84
  UINT32  Reserved11;         // FF88
  UINT32  ESI;                // FF8C
  UINT32  Reserved9;          // FF90
  UINT32  EDI;                // FF94
  UINT32  Reserved8;          // FF98
  UINT32  IO_MEM_ADDR;        // FF9C
  UINT32  Reserved7;          // FFA0
  UINT32  IO_MISC;            // FFA4
  UINT32  ES_SEL;             // FFA8
  UINT32  CS_SEL;             // FFAC
  UINT32  SS_SEL;             // FFB0
  UINT32  DS_SEL;             // FFB4
  UINT32  FS_SEL;             // FFB8
  UINT32  GS_SEL;             // FFBC
  UINT32  LDTR_SEL;           // FFC0
  UINT32  TR_SEL;             // FFC4
  UINT32  DR7;                // FFC8
  UINT32  Reserved6;          // FFCC
  UINT32  DR6;                // FFD0
  UINT32  Reserved5;          // FFD4
  UINT32  EIP;                // FFD8
  UINT32  Reserved4;          // FFDC
  UINT32  EFER;               // FFE0
  UINT32  Reserved3;          // FFE4
  UINT32  EFLAGS;             // FFE8
  UINT32  Reserved2;          // FFEC
  UINT32  CR3;                // FFF0
  UINT32  Reserved1;          // FFF4
  UINT32  CR0;                // FFF8
  UINT32  Reserved0;          // FFFC
} EFI_SMM_CPU_CT_SAVE_STATE;

typedef struct {
  UINT8   Reserved26[464];        // FC00 - FDCF
  UINT32  GdtrUpperBase;          // FDD0
  UINT32  LdtrUpperBase;          // FDD4
  UINT32  IdtrUpperBase;          // FDD8
  UINT32  Reserved25;             // FDDC - FDDF
  UINT64  IoRdi;                  // FDE0
  UINT64  IoRip;                  // FDE8
  UINT64  IoRcx;                  // FDF0
  UINT64  IoRsi;                  // FDF8
  UINT8   Reserved24[64];         // FE00 - FE3F
  UINT64  Cr4;                    // FE40
  UINT8   Reserved23[68];         // FE48 - FE8B
  UINT32  GdtrBase;               // FE8C
  UINT32  Reserved22;             // FE90
  UINT32  IdtrBase;               // FE94
  UINT32  Reserved21;             // FE98
  UINT32  LdtrBase;               // FE9C
  UINT32  Reserved20;             // FEA0
  UINT8   Reserved19[84];         // FEA4 - FEF7
  UINT32  Smbase;                 // FEF8
  UINT32  RevId;                  // FEFC
  UINT16  IoRestart;              // FF00
  UINT16  HaltRestart;            // FF02
  UINT8   Reserved18[24];         // FF04 - FF1B
  UINT32  R15;                    // FF1C
  UINT32  Reserved17;             // FE20
  UINT32  R14;                    // FF24
  UINT32  Reserved16;             // FE28
  UINT32  R13;                    // FF2C
  UINT32  Reserved15;             // FE30
  UINT32  R12;                    // FF34
  UINT32  Reserved14;             // FE38
  UINT32  R11;                    // FF3C
  UINT32  Reserved13;             // FE40
  UINT32  R10;                    // FF44
  UINT32  Reserved12;             // FE48
  UINT32  R9;                     // FF4C
  UINT32  Reserved11;             // FE50
  UINT32  R8;                     // FF54
  UINT32  Reserved10;             // FE58
  UINT32  Rax;                    // FF5C
  UINT32  Reserved9;              // FE60
  UINT32  Rcx;                    // FF64
  UINT32  Reserved8;              // FE68
  UINT32  Rdx;                    // FF6C
  UINT32  Reserved7;              // FE70
  UINT32  Rbx;                    // FF74
  UINT32  Reserved6;              // FE78
  UINT32  Rsp;                    // FF7C
  UINT32  Reserved5;              // FE80
  UINT32  Rbp;                    // FF84
  UINT32  Reserved4;              // FE88
  UINT32  Rsi;                    // FF8C
  UINT32  Reserved3;              // FE90
  UINT32  Rdi;                    // FF94
  UINT32  Reserved2;              // FE98
  UINT32  IoMemAddr;              // FF9C
  UINT32  Reserved1;              // FEA0
  UINT32  IoMiscInfo;             // FFA4
  UINT32  EsSel;                  // FFA8
  UINT32  CsSel;                  // FFAC
  UINT32  SsSel;                  // FFB0
  UINT32  DsSel;                  // FFB4
  UINT32  FsSel;                  // FFB8
  UINT32  GsSel;                  // FFBC
  UINT32  LdtrSel;                // FFC0
  UINT32  TrSel;                  // FFC4
  UINT64  Dr7;                    // FFC8
  UINT64  Dr6;                    // FFD0
  UINT32  Rip;                    // FFD8
  UINT32  Reserved0;              // FFDC
  UINT64  Efr;                    // FFE0
  UINT64  RFlags;                 // FFE8
  UINT64  Cr3;                    // FFF0
  UINT64  Cr0;                    // FFF8
} EFI_SMM_CPU_MEROM_SAVE_STATE;


typedef struct {
  UINT8   Reserved14[0x228];  // FC00-FE28
  UINT32  IO_EIP;             // FE28
  UINT8   Reserved13[0x14];   // FE2C-FE40
  UINT32  CR4;                // FE40
  UINT8   Reserved12[0x48];   // FE44-FE8C
  UINT32  GDT_BASE;           // FE8C
  UINT8   Reserved11[0xC];    // FE90-FE9C
  UINT32  LDT_BASE;           // FE9C
  UINT8   Reserved10[0x58];   // FEA0-FEF8
  UINT32  SMBASE;
  UINT32  REVID;
  UINT16  IO_RESTART;
  UINT16  HALT_RESTART;
  UINT8   Reserved9[0xA4];

  UINT16  ES;
  UINT16  Reserved8;
  UINT16  CS;
  UINT16  Reserved7;
  UINT16  SS;
  UINT16  Reserved6;
  UINT16  DS;
  UINT16  Reserved5;
  UINT16  FS;
  UINT16  Reserved4;
  UINT16  GS;
  UINT16  Reserved3;
  UINT32  Reserved2;
  UINT16  TR;
  UINT16  Reserved1;
  UINT32  DR7;
  UINT32  DR6;
  UINT32  EAX;
  UINT32  ECX;
  UINT32  EDX;
  UINT32  EBX;
  UINT32  ESP;
  UINT32  EBP;
  UINT32  ESI;
  UINT32  EDI;
  UINT32  EIP;
  UINT32  EFLAGS;
  UINT32  CR3;
  UINT32  CR0;
} EFI_SMM_CPU_CT_NOT_ENABLED_SAVE_STATE;

struct _EFI_SMM_OPTIONAL_FP_SAVE_STATE {
  UINT16  Fcw;
  UINT16  Fsw;
  UINT16  Ftw;
  UINT16  Opcode;
  UINT32  Eip;
  UINT16  Cs;
  UINT16  Rsvd1;
  UINT32  DataOffset;
  UINT16  Ds;
  UINT8   Rsvd2[10];
  UINT8   St0Mm0[10], Rsvd3[6];
  UINT8   St0Mm1[10], Rsvd4[6];
  UINT8   St0Mm2[10], Rsvd5[6];
  UINT8   St0Mm3[10], Rsvd6[6];
  UINT8   St0Mm4[10], Rsvd7[6];
  UINT8   St0Mm5[10], Rsvd8[6];
  UINT8   St0Mm6[10], Rsvd9[6];
  UINT8   St0Mm7[10], Rsvd10[6];
  UINT8   Rsvd11[22 * 16];
};

typedef struct _EFI_SMM_OPTIONAL_FP_SAVE_STATE32 {
  UINT16  Fcw;
  UINT16  Fsw;
  UINT16  Ftw;
  UINT16  Opcode;
  UINT32  Eip;
  UINT16  Cs;
  UINT16  Rsvd1;
  UINT32  DataOffset;
  UINT16  Ds;
  UINT8   Reserved2[10];
  UINT8   St0Mm0[10], Rsvd3[6];
  UINT8   St1Mm1[10], Rsvd4[6];
  UINT8   St2Mm2[10], Rsvd5[6];
  UINT8   St3Mm3[10], Rsvd6[6];
  UINT8   St4Mm4[10], Rsvd7[6];
  UINT8   St5Mm5[10], Rsvd8[6];
  UINT8   St6Mm6[10], Rsvd9[6];
  UINT8   St7Mm7[10], Rsvd10[6];
  UINT8   Xmm0[16];
  UINT8   Xmm1[16];
  UINT8   Xmm2[16];
  UINT8   Xmm3[16];
  UINT8   Xmm4[16];
  UINT8   Xmm5[16];
  UINT8   Xmm6[16];
  UINT8   Xmm7[16];
  UINT8   Rsvd11[14 * 16];
} EFI_SMM_OPTIONAL_FP_SAVE_STATE32;

typedef struct _EFI_SMM_OPTIONAL_FP_SAVE_STATE64 {
  UINT16  Fcw;
  UINT16  Fsw;
  UINT16  Ftw;
  UINT16  Opcode;
  UINT64  Rip;
  UINT64  DataOffset;
  UINT8   Rsvd1[8];
  UINT8   St0Mm0[10], Rsvd2[6];
  UINT8   St1Mm1[10], Rsvd3[6];
  UINT8   St2Mm2[10], Rsvd4[6];
  UINT8   St3Mm3[10], Rsvd5[6];
  UINT8   St4Mm4[10], Rsvd6[6];
  UINT8   St5Mm5[10], Rsvd7[6];
  UINT8   St6Mm6[10], Rsvd8[6];
  UINT8   St7Mm7[10], Rsvd9[6];
  UINT8   Xmm0[16];
  UINT8   Xmm1[16];
  UINT8   Xmm2[16];
  UINT8   Xmm3[16];
  UINT8   Xmm4[16];
  UINT8   Xmm5[16];
  UINT8   Xmm6[16];
  UINT8   Xmm7[16];
  UINT8   Xmm8[16];
  UINT8   Xmm9[16];
  UINT8   Xmm10[16];
  UINT8   Xmm11[16];
  UINT8   Xmm12[16];
  UINT8   Xmm13[16];
  UINT8   Xmm14[16];
  UINT8   Xmm15[16];
  UINT8   Rsvd10[6 * 16];
} EFI_SMM_OPTIONAL_FP_SAVE_STATE64;

struct _EFI_SMM_SYSTEM_TABLE;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSTALL_CONFIGURATION_TABLE) (
  IN EFI_SMM_SYSTEM_TABLE         * SystemTable,
  IN EFI_GUID                     * Guid,
  IN VOID                         *Table,
  IN UINTN                        TableSize
  )
/*++

  Routine Description:
    The SmmInstallConfigurationTable() function is used to maintain the list 
    of configuration tables that are stored in the System Management System 
    Table.  The list is stored as an array of (GUID, Pointer) pairs.  The list 
    must be allocated from pool memory with PoolType set to EfiRuntimeServicesData.

  Arguments:
    SystemTable - A pointer to the SMM System Table.
    Guid        - A pointer to the GUID for the entry to add, update, or remove.
    Table       - A pointer to the buffer of the table to add.  
    TableSize   - The size of the table to install.

  Returns:
    EFI_SUCCESS             - The (Guid, Table) pair was added, updated, or removed.
    EFI_INVALID_PARAMETER   - Guid is not valid.
    EFI_NOT_FOUND           - An attempt was made to delete a non-existent entry.
    EFI_OUT_OF_RESOURCES    - There is not enough memory available to complete the operation. 

--*/
;

//
// System Management System Table (SMST)
//
struct _EFI_SMM_SYSTEM_TABLE {
  EFI_TABLE_HEADER                    Hdr;

  CHAR16                              *SmmFirmwareVendor;
  UINT32                              SmmFirmwareRevision;

  EFI_SMM_INSTALL_CONFIGURATION_TABLE SmmInstallConfigurationTable;

  //
  // I/O Services
  //
  EFI_GUID                            EfiSmmCpuIoGuid;
  EFI_SMM_CPU_IO_INTERFACE            SmmIo;

  //
  // Runtime memory service
  //
  EFI_SMMCORE_ALLOCATE_POOL           SmmAllocatePool;
  EFI_SMMCORE_FREE_POOL               SmmFreePool;
  EFI_SMMCORE_ALLOCATE_PAGES          SmmAllocatePages;
  EFI_SMMCORE_FREE_PAGES              SmmFreePages;

  //
  // MP service
  //
  EFI_SMM_STARTUP_THIS_AP             SmmStartupThisAp;

  //
  // CPU information records
  //
  UINTN                               CurrentlyExecutingCpu;
  UINTN                               NumberOfCpus;
  EFI_SMM_CPU_SAVE_STATE              *CpuSaveState;
  EFI_SMM_OPTIONAL_FP_SAVE_STATE      *CpuOptionalFloatingPointState;

  //
  // Extensibility table
  //
  UINTN                               NumberOfTableEntries;
  EFI_CONFIGURATION_TABLE             *SmmConfigurationTable;

};

//
// SMM Handler Definition
//
#define EFI_HANDLER_SUCCESS         0x0000
#define EFI_HANDLER_CRITICAL_EXIT   0x0001
#define EFI_HANDLER_SOURCE_QUIESCED 0x0002
#define EFI_HANDLER_SOURCE_PENDING  0x0003

//
// Structure of Communicate Buffer
//
typedef struct {
  EFI_GUID              HeaderGuid;
  UINTN                 MessageLength;
  UINT8                 Data[1];
} EFI_SMM_COMMUNICATE_HEADER;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_HANDLER_ENTRY_POINT) (
  IN EFI_HANDLE             SmmImageHandle,
  IN OUT VOID               *CommunicationBuffer OPTIONAL,
  IN OUT UINTN              *SourceSize OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_ENTRY_POINT) (
  IN EFI_HANDLE             SmmImageHandle,
  IN OUT VOID               *CommunicationBuffer OPTIONAL,
  IN OUT UINTN              *SourceSize OPTIONAL
  );

typedef struct {
  EFI_HANDLE                SmmHandler;
  EFI_DEVICE_PATH_PROTOCOL  *HandlerDevicePath;
} EFI_HANDLER_DESCRIPTOR;

//
// SMM Base Protocol Definition
//
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REGISTER_HANDLER) (
  IN EFI_SMM_BASE_PROTOCOL                           * This,
  IN  EFI_DEVICE_PATH_PROTOCOL                       * FilePath,
  IN  VOID                                           *SourceBuffer OPTIONAL,
  IN  UINTN                                          SourceSize,
  OUT EFI_HANDLE                                     * ImageHandle,
  IN  BOOLEAN                                        LegacyIA32Binary OPTIONAL
  )
/*++

  Routine Description:
    Register a given driver into SMRAM.  This is the equivalent of performing
    the LoadImage/StartImage into System Management Mode.

  Arguments:
    This                  - Protocol instance pointer.
    SourceBuffer          - Optional source buffer in case of the image file
                            being in memory.
    SourceSize            - Size of the source image file, if in memory.
    ImageHandle           - Pointer to the handle that reflects the driver 
                            loaded into SMM.
    LegacyIA32Binary      - The binary image to load is legacy 16 bit code.

  Returns:
    EFI_SUCCESS           - The operation was successful.
    EFI_OUT_OF_RESOURCES  - There were no additional SMRAM resources to load the handler
    EFI_UNSUPPORTED       - This platform does not support 16-bit handlers.
    EFI_UNSUPPORTED       - In runtime.
    EFI_INVALID_PARAMETER - The handlers was not the correct image type

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_UNREGISTER_HANDLER) (
  IN EFI_SMM_BASE_PROTOCOL          * This,
  IN EFI_HANDLE                     ImageHandle
  )
/*++

  Routine Description:
    Remove a given driver SMRAM.  This is the equivalent of performing
    the UnloadImage System Management Mode.

  Arguments:
    This                  - Protocol instance pointer.
    ImageHandle           - Pointer to the handle that reflects the driver 
                            loaded into SMM.

  Returns:
    EFI_SUCCESS           - The operation was successful
    EFI_INVALID_PARAMETER - The handler did not exist
    EFI_UNSUPPORTED       - In runtime.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_COMMUNICATE) (
  IN EFI_SMM_BASE_PROTOCOL          * This,
  IN EFI_HANDLE                     ImageHandle,
  IN OUT VOID                       *CommunicationBuffer,
  IN OUT UINTN                      *SourceSize
  )
/*++

  Routine Description:
    The SMM Inter-module Communicate Service Communicate() function 
    provides a services to send/received messages from a registered 
    EFI service.  The BASE protocol driver is responsible for doing 
    any of the copies such that the data lives in boot-service accessible RAM.

  Arguments:
    This                  - Protocol instance pointer.
    ImageHandle           - Pointer to the handle that reflects the driver 
                            loaded into SMM.
    CommunicationBuffer   - Pointer to the buffer to convey into SMRAM.
    SourceSize            - Size of the contents of buffer..

  Returns:
    EFI_SUCCESS           - The message was successfully posted
    EFI_INVALID_PARAMETER - The buffer was NULL

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_SERVICE) (
  IN EFI_SMM_BASE_PROTOCOL                            * This,
  IN EFI_HANDLE                                       SmmImageHandle,
  IN EFI_SMM_CALLBACK_ENTRY_POINT                     CallbackAddress,
  IN BOOLEAN                                          MakeLast OPTIONAL,
  IN BOOLEAN                                          FloatingPointSave OPTIONAL
  )
/*++

  Routine Description:
    Register a callback to execute within SMM.   
    This allows receipt of messages created with the Boot Service COMMUNICATE.

  Arguments:
    This                  - Protocol instance pointer.
    CallbackAddress       - Address of the callback service
    MakeFirst             - If present, will stipulate that the handler is posted 
                            to be the first module executed in the dispatch table.
    MakeLast              - If present, will stipulate that the handler is posted 
                            to be last executed in the dispatch table.
    FloatingPointSave     - This is an optional parameter which informs the 
                            EFI_SMM_ACCESS_PROTOCOL Driver core if it needs to save 
                            the floating point register state.  If any of the handlers 
                            require this, then the state will be saved for all of the handlers.

  Returns:
    EFI_SUCCESS           - The operation was successful
    EFI_OUT_OF_RESOURCES  - Not enough space in the dispatch queue
    EFI_UNSUPPORTED       - In runtime.
    EFI_UNSUPPORTED       - Not in SMM.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ALLOCATE_POOL) (
  IN EFI_SMM_BASE_PROTOCOL          * This,
  IN EFI_MEMORY_TYPE                PoolType,
  IN UINTN                          Size,
  OUT VOID                          **Buffer
  )
/*++

  Routine Description:
    The SmmAllocatePool() function allocates a memory region of Size bytes from memory of 
    type PoolType and returns the address of the allocated memory in the location referenced 
    by Buffer.  This function allocates pages from EFI SMRAM Memory as needed to grow the 
    requested pool type.  All allocations are eight-byte aligned.

  Arguments:
    This                  - Protocol instance pointer.
    PoolType              - The type of pool to allocate.  
                            The only supported type is EfiRuntimeServicesData; 
                            the interface will internally map this runtime request to SMRAM.
    Size                  - The number of bytes to allocate from the pool.
    Buffer                - A pointer to a pointer to the allocated buffer if the call 
                            succeeds; undefined otherwise.

  Returns:
    EFI_SUCCESS           - The requested number of bytes was allocated.
    EFI_OUT_OF_RESOURCES  - The pool requested could not be allocated.
    EFI_INVALID_PARAMETER - PoolType was invalid.
    EFI_UNSUPPORTED       - In runtime.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_FREE_POOL) (
  IN EFI_SMM_BASE_PROTOCOL          * This,
  IN VOID                           *Buffer
  )
/*++

  Routine Description:
    The SmmFreePool() function returns the memory specified by Buffer to the system.  
    On return, the memory's type is EFI SMRAM Memory.  The Buffer that is freed must 
    have been allocated by SmmAllocatePool().

  Arguments:
    This                  - Protocol instance pointer.
    Buffer                - Pointer to the buffer allocation.

  Returns:
    EFI_SUCCESS           - The memory was returned to the system.
    EFI_INVALID_PARAMETER - Buffer was invalid.
    EFI_UNSUPPORTED       - In runtime.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSIDE_OUT) (
  IN EFI_SMM_BASE_PROTOCOL          * This,
  OUT BOOLEAN                       *InSmm
  )
/*++

  Routine Description:
    This routine tells caller if execution context is SMM or not.

  Arguments:
    This                  - Protocol instance pointer.

  Returns:
    EFI_SUCCESS           - The operation was successful

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_SMST_LOCATION) (
  IN EFI_SMM_BASE_PROTOCOL          * This,
  IN OUT EFI_SMM_SYSTEM_TABLE       **Smst
  )
/*++

  Routine Description:
    The GetSmstLocation() function returns the locatin of the System Management 
    Service Table.  The use of the API is such that a driver can discover the 
    location of the SMST in its entry point and then cache it in some driver 
    global variable so that the SMST can be invoked in subsequent callbacks.

  Arguments:
    This                  - Protocol instance pointer.
    Smst                  - Pointer to the SMST.

  Returns:
    EFI_SUCCESS           - The operation was successful
    EFI_INVALID_PARAMETER - Smst was invalid.  
    EFI_UNSUPPORTED       - Not in SMM.

--*/
;

struct _EFI_SMM_BASE_PROTOCOL {
  EFI_SMM_REGISTER_HANDLER    Register;
  EFI_SMM_UNREGISTER_HANDLER  UnRegister;
  EFI_SMM_COMMUNICATE         Communicate;
  EFI_SMM_CALLBACK_SERVICE    RegisterCallback;
  EFI_SMM_INSIDE_OUT          InSmm;
  EFI_SMM_ALLOCATE_POOL       SmmAllocatePool;
  EFI_SMM_FREE_POOL           SmmFreePool;
  EFI_SMM_GET_SMST_LOCATION   GetSmstLocation;
};

extern EFI_GUID gEfiSmmBaseProtocolGuid;
extern EFI_GUID gEfiSmmCpuIoGuid;
extern EFI_GUID gEfiSmmCommunicateHeaderGuid;

#endif

/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiApi.h

Abstract:

  Tiano PEI intrinsic definitions. This includes all Pei Services APIs.

  Peims are passed in a pointer to a pointer to the PEI Services table.
  The PEI Services table contains pointers to the PEI services exported
  by the PEI Core.

--*/

#ifndef _PEI_API_H_
#define _PEI_API_H_

#include "PeiHob.h"
#include "EfiFirmwareFileSystem.h"
#include "EfiFirmwareVolumeHeader.h"
#include EFI_PPI_DEFINITION (FirmwareVolumeInfo)
#include EFI_PPI_DEFINITION (FirmwareVolume)
#include EFI_PPI_DEFINITION (Decompress)


//
// Declare forward referenced data structures
//
EFI_FORWARD_DECLARATION (EFI_PEI_NOTIFY_DESCRIPTOR);
EFI_FORWARD_DECLARATION (EFI_PEI_SERVICES);

#include EFI_PPI_DEFINITION (CpuIo)
#include EFI_PPI_DEFINITION (PciCfg)
#include EFI_PPI_DEFINITION (PciCfg2)
#include EFI_PPI_DEFINITION (EcpPciCfg)

//
// PEI Specification Revision information
//
#if (PI_SPECIFICATION_VERSION < 0x00010000)
#define PEI_SPECIFICATION_MAJOR_REVISION  0
#define PEI_SPECIFICATION_MINOR_REVISION  91
#else
#define PEI_SPECIFICATION_MAJOR_REVISION  1
#define PEI_SPECIFICATION_MINOR_REVISION  00

#endif

typedef
EFI_STATUS
(EFIAPI *EFI_PEIM_ENTRY_POINT)(
  IN EFI_FFS_FILE_HEADER       * FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEIM_NOTIFY_ENTRY_POINT) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  * NotifyDescriptor,
  IN VOID                       *Ppi
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_VERIFICATION) (
  IN UINTN    SectionAddress
  );

//
// PEI Ppi Services List Descriptors
//
#define EFI_PEI_PPI_DESCRIPTOR_PIC              0x00000001
#define EFI_PEI_PPI_DESCRIPTOR_PPI              0x00000010
#define EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK  0x00000020
#define EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH  0x00000040
#define EFI_PEI_PPI_DESCRIPTOR_NOTIFY_TYPES     0x00000060
#define EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST   0x80000000

typedef struct {
  UINTN     Flags;
  EFI_GUID  *Guid;
  VOID      *Ppi;
} EFI_PEI_PPI_DESCRIPTOR;

struct _EFI_PEI_NOTIFY_DESCRIPTOR {
  UINTN                       Flags;
  EFI_GUID                    *Guid;
  EFI_PEIM_NOTIFY_ENTRY_POINT Notify;
};


#if (PI_SPECIFICATION_VERSION < 0x00010000)

//
// PEI PPI Services
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_INSTALL_PPI) (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_PPI_DESCRIPTOR      * PpiList
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_REINSTALL_PPI) (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_PPI_DESCRIPTOR          * OldPpi,
  IN  EFI_PEI_PPI_DESCRIPTOR          * NewPpi
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_LOCATE_PPI) (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_GUID                    * Guid,
  IN UINTN                        Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR   **PpiDescriptor,
  IN OUT VOID                     **Ppi
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_NOTIFY_PPI) (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR       * NotifyList
  );

//
// PEI Boot Mode Services
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_BOOT_MODE) (
  IN  EFI_PEI_SERVICES                 **PeiServices,
  IN OUT EFI_BOOT_MODE                 * BootMode
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SET_BOOT_MODE) (
  IN EFI_PEI_SERVICES                  **PeiServices,
  IN EFI_BOOT_MODE                     BootMode
  );

//
// PEI HOB Services
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_HOB_LIST) (
  IN EFI_PEI_SERVICES                  **PeiServices,
  IN OUT VOID                          **HobList
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_CREATE_HOB) (
  IN EFI_PEI_SERVICES                  **PeiServices,
  IN UINT16                            Type,
  IN UINT16                            Length,
  IN OUT VOID                          **Hob
  );

 //
 // PEI Firmware Volume Services
 //

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_VOLUME) (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN UINTN                           Instance,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **FwVolHeader
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_FILE) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_FV_FILETYPE             SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER  * FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **FileHeader
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_SECTION_DATA) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_SECTION_TYPE            SectionType,
  IN EFI_FFS_FILE_HEADER         * FfsFileHeader,
  IN OUT VOID                    **SectionData
  );

//
// PEI Memory Services
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_INSTALL_PEI_MEMORY) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PHYSICAL_ADDRESS       MemoryBegin,
  IN UINT64                     MemoryLength
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_ALLOCATE_PAGES) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_MEMORY_TYPE            MemoryType,
  IN UINTN                      Pages,
  IN OUT EFI_PHYSICAL_ADDRESS   * Memory
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_ALLOCATE_POOL) (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN UINTN                           Size,
  OUT VOID                           **Buffer
  );

typedef
VOID
(EFIAPI *EFI_PEI_COPY_MEM) (
  IN VOID                       *Destination,
  IN VOID                       *Source,
  IN UINTN                      Length
  );

typedef
VOID
(EFIAPI *EFI_PEI_SET_MEM) (
  IN VOID                       *Buffer,
  IN UINTN                      Size,
  IN UINT8                      Value
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_REPORT_STATUS_CODE) (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  );

//
// PEI Reset
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_RESET_SYSTEM) (
  IN EFI_PEI_SERVICES   **PeiServices
  );



#else

//
// PEI PPI Services
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_INSTALL_PPI) (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR      * PpiList
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_REINSTALL_PPI) (
  IN CONST EFI_PEI_SERVICES                **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR          * OldPpi,
  IN CONST EFI_PEI_PPI_DESCRIPTOR          * NewPpi
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_LOCATE_PPI) (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN CONST EFI_GUID                    * Guid,
  IN UINTN                             Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR        **PpiDescriptor OPTIONAL,
  IN OUT VOID                          **Ppi
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_NOTIFY_PPI) (
  IN CONST EFI_PEI_SERVICES                **PeiServices,
  IN CONST EFI_PEI_NOTIFY_DESCRIPTOR       * NotifyList
  );

//
// PEI Boot Mode Services
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_BOOT_MODE) (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN OUT EFI_BOOT_MODE                 * BootMode
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SET_BOOT_MODE) (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN EFI_BOOT_MODE                     BootMode
  );

//
// PEI HOB Services
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_HOB_LIST) (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN OUT VOID                          **HobList
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_CREATE_HOB) (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN UINT16                            Type,
  IN UINT16                            Length,
  IN OUT VOID                          **Hob
  );



 //
 // PEI Firmware Volume Services
 //
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_VOLUME2) (
  IN CONST  EFI_PEI_SERVICES  **PeiServices,
  IN UINTN                    Instance,
  IN OUT EFI_PEI_FV_HANDLE    *VolumeHandle 
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_FILE2) (
  IN CONST  EFI_PEI_SERVICES        **PeiServices,
  IN EFI_FV_FILETYPE                SearchType,
  IN CONST EFI_PEI_FV_HANDLE        FvHandle,
  IN OUT EFI_PEI_FILE_HANDLE        *FileHandle  
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_SECTION_DATA2) (
  IN CONST  EFI_PEI_SERVICES    **PeiServices,
  IN EFI_SECTION_TYPE           SectionType,
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  OUT VOID                      **SectionData
  );

  
  //
  // PEI Memory Services
  //
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_INSTALL_PEI_MEMORY) (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PHYSICAL_ADDRESS       MemoryBegin,
  IN UINT64                     MemoryLength
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_ALLOCATE_PAGES) (

  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_MEMORY_TYPE            MemoryType,
  IN UINTN                      Pages,
  IN OUT EFI_PHYSICAL_ADDRESS   * Memory
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_ALLOCATE_POOL) (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN UINTN                           Size,
  OUT VOID                           **Buffer
  );

typedef
VOID
(EFIAPI *EFI_PEI_COPY_MEM) (
  IN VOID                       *Destination,
  IN VOID                       *Source,
  IN UINTN                      Length
  );

typedef
VOID
(EFIAPI *EFI_PEI_SET_MEM) (
  IN VOID                       *Buffer,
  IN UINTN                      Size,
  IN UINT8                      Value
  );

  //
  // New interfaceas added by the PI 1.0
  //
typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_BY_NAME) (
  IN  CONST EFI_GUID        *FileName,
  IN  EFI_PEI_FV_HANDLE     VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  );


typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_GET_FILE_INFO) (
  IN  EFI_PEI_FILE_HANDLE   FileHandle,
  OUT EFI_FV_FILE_INFO      *FileInfo
  );


typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_GET_VOLUME_INFO) (
  IN  EFI_PEI_FV_HANDLE     VolumeHandle,
  OUT EFI_FV_INFO           *VolumeInfo
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_REGISTER_FOR_SHADOW) (
  IN EFI_PEI_FILE_HANDLE       FileHandle
  );

//
// PEI Status Code Service
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_REPORT_STATUS_CODE) (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN EFI_STATUS_CODE_TYPE           Type,
  IN EFI_STATUS_CODE_VALUE          Value,
  IN UINT32                         Instance,
  IN EFI_GUID                       *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA           *Data OPTIONAL
  );

//
// PEI Reset Service
//
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_RESET_SYSTEM) (
  IN EFI_PEI_SERVICES   **PeiServices
  );

#endif


//
// EFI PEI Services Table
//
#define PEI_SERVICES_SIGNATURE  0x5652455320494550ULL
#define PEI_SERVICES_REVISION   ((PEI_SPECIFICATION_MAJOR_REVISION << 16) | (PEI_SPECIFICATION_MINOR_REVISION))
typedef PEI_CPU_IO_PPI          EFI_PEI_CPU_IO_PPI;


struct _EFI_PEI_SERVICES {
  EFI_TABLE_HEADER              Hdr;

  //
  // PPI Functions
  //
  EFI_PEI_INSTALL_PPI           InstallPpi;
  EFI_PEI_REINSTALL_PPI         ReInstallPpi;
  EFI_PEI_LOCATE_PPI            LocatePpi;
  EFI_PEI_NOTIFY_PPI            NotifyPpi;

  //
  // Boot Mode Functions
  //
  EFI_PEI_GET_BOOT_MODE         GetBootMode;
  EFI_PEI_SET_BOOT_MODE         SetBootMode;

  //
  // HOB Functions
  //
  EFI_PEI_GET_HOB_LIST          GetHobList;
  EFI_PEI_CREATE_HOB            CreateHob;

  //
  // Firmware Volume Functions
  //
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  EFI_PEI_FFS_FIND_NEXT_VOLUME  FfsFindNextVolume;
  EFI_PEI_FFS_FIND_NEXT_FILE    FfsFindNextFile;
  EFI_PEI_FFS_FIND_SECTION_DATA FfsFindSectionData;
#else
  EFI_PEI_FFS_FIND_NEXT_VOLUME2  FfsFindNextVolume;
  EFI_PEI_FFS_FIND_NEXT_FILE2    FfsFindNextFile;
  EFI_PEI_FFS_FIND_SECTION_DATA2 FfsFindSectionData;
#endif
  //
  // PEI Memory Functions
  //
  EFI_PEI_INSTALL_PEI_MEMORY    InstallPeiMemory;
  EFI_PEI_ALLOCATE_PAGES        AllocatePages;
  EFI_PEI_ALLOCATE_POOL         AllocatePool;
  EFI_PEI_COPY_MEM              CopyMem;
  EFI_PEI_SET_MEM               SetMem;

  //
  // Status Code
  //
  EFI_PEI_REPORT_STATUS_CODE    PeiReportStatusCode;

  //
  // Reset
  //
  EFI_PEI_RESET_SYSTEM          PeiResetSystem;

  //
  // Pointer to PPI interface
  //
#if (PI_SPECIFICATION_VERSION < 0x00010000)

  PEI_CPU_IO_PPI                 *CpuIo;
#if defined (SUPPORT_DEPRECATED_PCI_CFG_PPI)
  PEI_PCI_CFG_PPI                *PciCfg;
#else
  ECP_PEI_PCI_CFG_PPI            *PciCfg;
#endif
#else
  EFI_PEI_CPU_IO_PPI             *CpuIo;
  EFI_PEI_PCI_CFG2_PPI           *PciCfg;

  //
  // New interfaceas added by the PI 1.0
  //
  EFI_PEI_FFS_FIND_BY_NAME        FfsFindFileByName;
  EFI_PEI_FFS_GET_FILE_INFO       FfsGetFileInfo;
  EFI_PEI_FFS_GET_VOLUME_INFO     FfsGetVolumeInfo;
  EFI_PEI_REGISTER_FOR_SHADOW     RegisterForShadow;
#endif

};

#if (PI_SPECIFICATION_VERSION < 0x00010000)

typedef struct {
  UINTN                   BootFirmwareVolume;
  UINTN                   SizeOfCacheAsRam;
  EFI_PEI_PPI_DESCRIPTOR  *DispatchTable;
} EFI_PEI_STARTUP_DESCRIPTOR;

typedef
EFI_STATUS
(EFIAPI *PEI_MAIN_ENTRY_POINT) (
    IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor
  );

#else

typedef struct _EFI_SEC_PEI_HAND_OFF {
  UINT16  DataSize;
  VOID    *BootFirmwareVolumeBase;
  UINTN   BootFirmwareVolumeSize;
  VOID    *TemporaryRamBase;
  UINTN   TemporaryRamSize;
  VOID    *PeiTemporaryRamBase;
  UINTN   PeiTemporaryRamSize;
  VOID    *StackBase;
  UINTN   StackSize;
} EFI_SEC_PEI_HAND_OFF;

typedef
EFI_STATUS
(EFIAPI *PEI_MAIN_ENTRY_POINT) (
  IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData,
  IN CONST EFI_PEI_PPI_DESCRIPTOR *PpList
  );

#endif

#endif

/** @file
  Framework PEI master include file. This file should match the PEI CIS spec.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PeiCis.h

  @par Revision Reference:
  Version 0.91.

**/

#ifndef __PEIM_CIS_H__
#define __PEIM_CIS_H__

#include <Common/MultiPhase.h>
#include <Common/BootMode.h>
#include <Common/Hob.h>
#include <Common/FirmwareVolumeImageFormat.h>
#include <Common/FirmwareVolumeHeader.h>
#include <Common/FirmwareFileSystem.h>
#include <Common/Dependency.h>

#define TIANO_ERROR(a)              (MAX_2_BITS | (a))

#if (EFI_SPECIFICATION_VERSION < 0x00020000)
//
// Tiano added a couple of return types. These are owned by UEFI specification
//  and Tiano can not use them. Thus for UEFI 2.0/R9 support we moved the values
//  to a UEFI OEM extension range to conform to UEFI specification.
//
#define EFI_NOT_AVAILABLE_YET   EFIERR (28)
#define EFI_UNLOAD_IMAGE        EFIERR (29)
#else
#define EFI_NOT_AVAILABLE_YET   TIANO_ERROR (0)
#define EFI_UNLOAD_IMAGE        TIANO_ERROR (1)
#endif

//
// Declare forward referenced data structures
//
typedef struct _EFI_PEI_SERVICES          EFI_PEI_SERVICES;
typedef struct _EFI_PEI_NOTIFY_DESCRIPTOR EFI_PEI_NOTIFY_DESCRIPTOR;


#include <Ppi/CpuIo.h>
#include <Ppi/PciCfg.h>

//
// PEI Specification Revision information
//
#define PEI_SPECIFICATION_MAJOR_REVISION  0
#define PEI_SPECIFICATION_MINOR_REVISION  91

/**
  The PEI Dispatcher will invoke each PEIM one time.  During this pass, the PEI 
  Dispatcher will pass control to the PEIM at the AddressOfEntryPoint in the PE Header. 

  @param  FfsHeader Pointer to the FFS file header. 
  
  @param  PeiServices Describes the list of possible PEI Services.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEIM_ENTRY_POINT)(
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );

/**
  Entry point of the notification callback function itself within the PEIM.

  @param  PeiServices Indirect reference to the PEI Services Table.
  
  @param  NotifyDescriptor Address of the notification descriptor data structure.
  
  @param  Ppi Address of the PPI that was installed.

  @return Status code
  
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEIM_NOTIFY_ENTRY_POINT) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
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

/**
  This service is the first one provided by the PEI Foundation.  This function 
  installs an interface in the PEI PPI database by GUID.  The purpose of the 
  service is to publish an interface that other parties can use to call 
  additional PEIMs.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table 
  published by the PEI Foundation. 
  
  @param  PpiList A pointer to the list of interfaces that the caller shall install.

  @retval EFI_SUCCESS The interface was successfully installed.
  
  @retval EFI_INVALID_PARAMETER The PpiList pointer is NULL or Any of the PEI PPI descriptors in the list do not have the EFI_PEI_PPI_DESCRIPTOR_PPI bit set in the Flags field. 
  
  @retval EFI_OUT_OF_RESOURCES There is no additional space in the PPI database.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_INSTALL_PPI) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_PEI_PPI_DESCRIPTOR      *PpiList
  );

/**
  This function reinstalls an interface in the PEI PPI database by GUID. 
  The purpose of the service is to publish an interface that other parties 
  can use to replace a same-named interface in the protocol database 
  with a different interface. 

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table 
  published by the PEI Foundation. 
  
  @param  OldPpi A pointer to the former PPI in the database. 
  
  @param  NewPpi A pointer to the new interfaces that the caller shall install.

  @retval EFI_SUCCESS The interface was successfully installed.
  
  @retval EFI_INVALID_PARAMETER The PpiList pointer is NULL or Any of the PEI PPI descriptors in the list do not have the EFI_PEI_PPI_DESCRIPTOR_PPI bit set in the Flags field. 
  
  @retval EFI_OUT_OF_RESOURCES There is no additional space in the PPI database.
  
  @retval EFI_NOT_FOUND The PPI for which the reinstallation was requested has not been installed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_REINSTALL_PPI) (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN EFI_PEI_PPI_DESCRIPTOR          *OldPpi,
  IN EFI_PEI_PPI_DESCRIPTOR          *NewPpi
  );

/**
  This function locates an interface in the PEI PPI database by GUID. 

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES published by the PEI Foundation.
  
  @param  Guid A pointer to the GUID whose corresponding interface needs to be found.
  
  @param  Instance The N-th instance of the interface that is required.
  
  @param  PpiDescriptor A pointer to instance of the EFI_PEI_PPI_DESCRIPTOR.
  
  @param  Ppi A pointer to the instance of the interface.  

  @retval EFI_SUCCESS The interface was successfully returned.
  
  @retval EFI_NOT_FOUND The PPI descriptor is not found in the database.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_LOCATE_PPI) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_GUID                    *Guid,
  IN UINTN                       Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor,
  IN OUT VOID                    **Ppi
  );

/**
  This function installs a notification service to be called back when a 
  given interface is installed or reinstalled.  The purpose of the service 
  is to publish an interface that other parties can use to call additional PPIs 
  that may materialize later.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  
  @param  NotifyList A pointer to the list of notification interfaces that the caller shall install.

  @retval EFI_SUCCESS The interface was successfully installed.
  
  @retval EFI_INVALID_PARAMETER The PpiList pointer is NULL or Any of the PEI PPI descriptors in the list do not have the EFI_PEI_PPI_DESCRIPTOR_PPI bit set in the Flags field. 
  
  @retval EFI_OUT_OF_RESOURCES There is no additional space in the PPI database.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_NOTIFY_PPI) (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR       *NotifyList
  );

/**
  This function returns the present value of the boot mode.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation. 
  
  @param  BootMode A pointer to contain the value of the boot mode.

  @retval EFI_SUCCESS The boot mode was returned successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_BOOT_MODE) (
  IN EFI_PEI_SERVICES            **PeiServices,
  OUT EFI_BOOT_MODE              *BootMode
  );

/**
  This function sets the value of the boot mode.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  
  @param  BootMode The value of the boot mode to set.

  @retval EFI_SUCCESS The boot mode was returned successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SET_BOOT_MODE) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_BOOT_MODE               BootMode
  );

/**
  This function returns the pointer to the list of Hand-Off Blocks (HOBs) in memory. 

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  
  @param  HobList A pointer to the list of HOBs that the PEI Foundation will initialize

  @retval EFI_SUCCESS The list was successfully returned.
  
  @retval EFI_NOT_AVAILABLE_YET The HOB list is not yet published.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_HOB_LIST) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN OUT VOID                    **HobList
  );

/**
  This service published by the PEI Foundation abstracts the creation of a Hand-Off Block's (HOB¡¯s) headers.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  Type The type of HOB to be installed.
  
  @param  Length The length of the HOB to be added.
  
  @param  Hob The address of a pointer that will contain the HOB header.

  @retval EFI_SUCCESS The HOB was successfully created.
  
  @retval EFI_OUT_OF_RESOURCES There is no additional space for HOB creation.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_CREATE_HOB) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN UINT16                      Type,
  IN UINT16                      Length,
  IN OUT VOID                    **Hob
  );

/**
  The purpose of the service is to abstract the capability of the PEI 
  Foundation to discover instances of firmware volumes in the system. 
  Given the input file pointer, this service searches for the next 
  matching file in the Firmware File System (FFS) volume.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  Instance This instance of the firmware volume to find.  The value 0 is the Boot Firmware Volume (BFV).
  
  @param  FwVolHeader Pointer to the firmware volume header of the volume to return.

  @retval EFI_SUCCESS The volume was found.
  
  @retval EFI_NOT_FOUND The volume was not found.
  
  @retval EFI_INVALID_PARAMETER FwVolHeader is NULL

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_VOLUME) (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN UINTN                           Instance,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **FwVolHeader
  );

/**
  The purpose of the service is to abstract the capability of the PEI 
  Foundation to discover instances of firmware files in the system. 
  Given the input file pointer, this service searches for the next matching 
  file in the Firmware File System (FFS) volume.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  SearchType A filter to find files only of this type.
  
  @param  FwVolHeader Pointer to the firmware volume header of the volume to search.This parameter must point to a valid FFS volume.
  
  @param  FileHeader Pointer to the current file from which to begin searching.This pointer will be updated upon return to reflect the file found.

  @retval EFI_SUCCESS The file was found.
  
  @retval EFI_NOT_FOUND The file was not found.
  
  @retval EFI_NOT_FOUND The header checksum was not zero.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_FILE) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_FV_FILETYPE             SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **FileHeader
  );

/**
  Given the input file pointer, this service searches for the next 
  matching file in the Firmware File System (FFS) volume. 

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  SectionType The value of the section type to find.
  
  @param  FfsFileHeader A pointer to the file header that contains the set of sections to be searched.
  
  @param  SectionData A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS The section was found.
  
  @retval EFI_NOT_FOUND The section was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_SECTION_DATA) (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_SECTION_TYPE            SectionType,
  IN EFI_FFS_FILE_HEADER         *FfsFileHeader,
  IN OUT VOID                    **SectionData
  );

/**
  This function registers the found memory configuration with the PEI Foundation.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  MemoryBegin The value of a region of installed memory
  
  @param  MemoryLength The corresponding length of a region of installed memory.

  @retval EFI_SUCCESS The region was successfully installed in a HOB.
  
  @retval EFI_INVALID_PARAMETER MemoryBegin and MemoryLength are illegal for this system.
  
  @retval EFI_OUT_OF_RESOURCES There is no additional space for HOB creation.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_INSTALL_PEI_MEMORY) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PHYSICAL_ADDRESS       MemoryBegin,
  IN UINT64                     MemoryLength
  );

/**
  The purpose of the service is to publish an interface that allows 
  PEIMs to allocate memory ranges that are managed by the PEI Foundation.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  MemoryType The type of memory to allocate. 
  
  @param  Pages The number of contiguous 4 KB pages to allocate.
  
  @param  Memory Pointer to a physical address. On output, the address is set to the base of the page range that was allocated.

  @retval EFI_SUCCESS The memory range was successfully allocated.
  
  @retval EFI_OUT_OF_RESOURCES  The pages could not be allocated.
  
  @retval EFI_INVALID_PARAMETER Type is not equal to AllocateAnyPages.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_ALLOCATE_PAGES) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_MEMORY_TYPE            MemoryType,
  IN UINTN                      Pages,
  IN OUT EFI_PHYSICAL_ADDRESS   *Memory
  );

/**
  The purpose of this service is to publish an interface that 
  allows PEIMs to allocate memory ranges that are managed by the PEI Foundation.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  Size The number of bytes to allocate from the pool.
  
  @param  Buffer If the call succeeds, a pointer to a pointer to the allocated buffer; undefined otherwise.

  @retval EFI_SUCCESS The allocation was successful.
  
  @retval EFI_OUT_OF_RESOURCES There is not enough heap to allocate the requested size.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_ALLOCATE_POOL) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN UINTN                      Size,
  OUT VOID                      **Buffer
  );

/**
  This service copies the contents of one buffer to another buffer.

  @param  Destination Pointer to the destination buffer of the memory copy.
  
  @param  Source Pointer to the source buffer of the memory copy
  
  @param  Length Number of bytes to copy from Source to Destination.

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_COPY_MEM) (
  IN VOID                       *Destination,
  IN VOID                       *Source,
  IN UINTN                      Length
  );

/**
  The service fills a buffer with a specified value.

  @param  Buffer Pointer to the buffer to fill.
  
  @param  Size Number of bytes in Buffer to fill.
  
  @param  Value Value to fill Buffer with

  @return None

**/
typedef
VOID
(EFIAPI *EFI_PEI_SET_MEM) (
  IN VOID                       *Buffer,
  IN UINTN                      Size,
  IN UINT8                      Value
  );

/**
  This service publishes an interface that allows PEIMs to report status codes.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  
  @param  Type Indicates the type of status code being reported. 
  
  @param  Value Describes the current status of a hardware or 
  software entity.  This includes information about the class and 
  subclass that is used to classify the entity as well as an operation.
  For progress codes, the operation is the current activity.
  For error codes, it is the exception.For debug codes,it is not defined at this time. 
  
  @param  Instance The enumeration of a hardware or software entity within 
  the system.  A system may contain multiple entities that match a class/subclass 
  pairing.  The instance differentiates between them.  An instance of 0 indicates 
  that instance information is unavailable, not meaningful, or not relevant.
  Valid instance numbers start with 1.
  
  @param  CallerId This optional parameter may be used to identify the caller. 
  This parameter allows the status code driver to apply different rules to 
  different callers.
  
  @param  Data This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS The function completed successfully. 
  
  @retval EFI_NOT_AVAILABLE_YET No progress code provider has installed an interface in the system.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_REPORT_STATUS_CODE) (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );

/**
  Resets the entire platform.

  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES 
  table published by the PEI Foundation.

  @retval EFI_SUCCESS The function completed successfully. 
  
  @retval EFI_NOT_AVAILABLE_YET The service has not been installed yet.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_RESET_SYSTEM) (
  IN EFI_PEI_SERVICES   **PeiServices
  );

//
// EFI PEI Services Table
//
#define PEI_SERVICES_SIGNATURE  0x5652455320494550ULL
#define PEI_SERVICES_REVISION   ((PEI_SPECIFICATION_MAJOR_REVISION << 16) | (PEI_SPECIFICATION_MINOR_REVISION))

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
  // Filesystem Functions
  //
  EFI_PEI_FFS_FIND_NEXT_VOLUME  FfsFindNextVolume;
  EFI_PEI_FFS_FIND_NEXT_FILE    FfsFindNextFile;
  EFI_PEI_FFS_FIND_SECTION_DATA FfsFindSectionData;

  //
  // Memory Functions
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
  EFI_PEI_CPU_IO_PPI            *CpuIo;
  EFI_PEI_PCI_CFG_PPI           *PciCfg;

};

typedef struct {
  UINTN                   BootFirmwareVolume;
  UINTN                   SizeOfCacheAsRam;
  EFI_PEI_PPI_DESCRIPTOR  *DispatchTable;
} EFI_PEI_STARTUP_DESCRIPTOR;

#include <Common/EfiImage.h>
#include <Common/StatusCode.h>
#include <Common/BootScript.h>
#include <Common/Capsule.h>

#include <Guid/Apriori.h>
#include <Guid/Capsule.h>
#include <Guid/DxeServices.h>
#include <Guid/HobList.h>
#include <Guid/MemoryAllocationHob.h>
#include <Guid/FirmwareFileSystem.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/GlobalVariable.h>

#include <Ppi/BlockIo.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Ppi/BootScriptExecuter.h>
#include <Ppi/DeviceRecoveryModule.h>
#include <Ppi/DxeIpl.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/FindFv.h>
#include <Ppi/LoadFile.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/Pcd.h>
#include <Ppi/ReadOnlyVariable.h>
#include <Ppi/RecoveryModule.h>
#include <Ppi/Reset.h>
#include <Ppi/S3Resume.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SectionExtraction.h>
#include <Ppi/Security.h>
#include <Ppi/Smbus.h>
#include <Ppi/Stall.h>
#include <Ppi/StatusCode.h>

#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/Pcd.h>

#endif

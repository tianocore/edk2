/** @file
  PI PEI master include file. This file should match the PI spec.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  Version 1.0.

**/

#ifndef __PEICIS_H__
#define __PEICIS_H__

#include <PiPei.h>
#include <Ppi/PciCfg.h>

typedef struct _FRAMEWORK_EFI_PEI_SERVICES FRAMEWORK_EFI_PEI_SERVICES;

/**
  The PEI Dispatcher will invoke each PEIM one time.  During this pass, the PEI 
  Dispatcher will pass control to the PEIM at the AddressOfEntryPoint in the PE Header. 

  @param  FfsHeader        Pointer to the FFS file header.
  @param  PeiServices      Describes the list of possible PEI Services.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEIM_ENTRY_POINT)(
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );
  
/**
  The purpose of the service is to abstract the capability of the PEI 
  Foundation to discover instances of firmware volumes in the system. 
  Given the input file pointer, this service searches for the next 
  matching file in the Firmware File System (FFS) volume.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  Instance         This instance of the firmware volume to find. The value 0 is the Boot Firmware Volume (BFV).
  @param  FwVolHeader      Pointer to the firmware volume header of the volume to return.

  @retval EFI_SUCCESS           The volume was found.
  @retval EFI_NOT_FOUND         The volume was not found.
  @retval EFI_INVALID_PARAMETER FwVolHeader is NULL

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_VOLUME) (
  IN FRAMEWORK_EFI_PEI_SERVICES      **PeiServices,
  IN UINTN                           Instance,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **FwVolHeader
  );
    
/**
  The purpose of the service is to abstract the capability of the PEI 
  Foundation to discover instances of firmware files in the system. 
  Given the input file pointer, this service searches for the next matching 
  file in the Firmware File System (FFS) volume.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  SearchType       A filter to find files only of this type.
  @param  FwVolHeader      Pointer to the firmware volume header of the volume to search.This parameter 
                           must point to a valid FFS volume.
  @param  FileHeader       Pointer to the current file from which to begin searching.This pointer will be 
                           updated upon return to reflect the file found.

  @retval EFI_SUCCESS           The file was found.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_NOT_FOUND         The header checksum was not zero.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_NEXT_FILE) (
  IN FRAMEWORK_EFI_PEI_SERVICES  **PeiServices,
  IN EFI_FV_FILETYPE             SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **FileHeader
  );

/**
  Given the input file pointer, this service searches for the next 
  matching file in the Firmware File System (FFS) volume. 

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  SectionType      The value of the section type to find.
  @param  FfsFileHeader    A pointer to the file header that contains the set of sections to be searched.
  @param  SectionData      A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FFS_FIND_SECTION_DATA) (
  IN FRAMEWORK_EFI_PEI_SERVICES  **PeiServices,
  IN EFI_SECTION_TYPE            SectionType,
  IN EFI_FFS_FILE_HEADER         *FfsFileHeader,
  IN OUT VOID                    **SectionData
  );
        
struct _FRAMEWORK_EFI_PEI_SERVICES {
  EFI_TABLE_HEADER            Hdr;
  //
  // PPI Functions
  //
  EFI_PEI_INSTALL_PPI         InstallPpi;
  EFI_PEI_REINSTALL_PPI       ReInstallPpi;
  EFI_PEI_LOCATE_PPI          LocatePpi;
  EFI_PEI_NOTIFY_PPI          NotifyPpi;
  //
  // Boot Mode Functions
  //
  EFI_PEI_GET_BOOT_MODE       GetBootMode;
  EFI_PEI_SET_BOOT_MODE       SetBootMode;
  //
  // HOB Functions
  //
  EFI_PEI_GET_HOB_LIST        GetHobList;
  EFI_PEI_CREATE_HOB          CreateHob;
  //
  // Firmware Volume Functions
  //
  EFI_PEI_FFS_FIND_NEXT_VOLUME   FfsFindNextVolume;
  EFI_PEI_FFS_FIND_NEXT_FILE     FfsFindNextFile;
  EFI_PEI_FFS_FIND_SECTION_DATA  FfsFindSectionData;
  //
  // PEI Memory Functions
  //
  EFI_PEI_INSTALL_PEI_MEMORY  InstallPeiMemory;
  EFI_PEI_ALLOCATE_PAGES      AllocatePages;
  EFI_PEI_ALLOCATE_POOL       AllocatePool;
  EFI_PEI_COPY_MEM            CopyMem;
  EFI_PEI_SET_MEM             SetMem;
  //
  // Status Code
  EFI_PEI_REPORT_STATUS_CODE  ReportStatusCode;
  //
  // Reset
  //
  EFI_PEI_RESET_SYSTEM        ResetSystem;
  //
  // (the following interfaces are installed by publishing PEIM)
  //
  // I/O Abstractions
  //
  EFI_PEI_CPU_IO_PPI          *CpuIo;
  EFI_PEI_PCI_CFG_PPI        *PciCfg;
};

typedef struct {
  UINTN                   BootFirmwareVolume;
  UINTN                   SizeOfCacheAsRam;
  EFI_PEI_PPI_DESCRIPTOR  *DispatchTable;
} EFI_PEI_STARTUP_DESCRIPTOR;

#endif  


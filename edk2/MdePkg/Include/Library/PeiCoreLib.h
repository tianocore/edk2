/** @file
	PEI Core Library implementation

	Copyright (c) 2006, Intel Corporation                                                         
	All rights reserved. This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

	Module Name:	PeiCoreLib.h

**/

#ifndef __PEI_CORE_LIB_H__
#define __PEI_CORE_LIB_H__

/**
	This service enables a given PEIM to register an interface into the PEI Foundation. 

	@param	PpiList A pointer to the list of interfaces that the caller shall install.

	@retval	EFI_SUCCESS The interface was successfully installed.
	@retval	EFI_INVALID_PARAMETER The PpiList pointer is NULL.
	@retval	EFI_INVALID_PARAMETER Any of the PEI PPI descriptors in the list do not have
	the EFI_PEI_PPI_DESCRIPTOR_PPI bit set in the Flags field.
	@retval	EFI_OUT_OF_RESOURCES There is no additional space in the PPI database.

**/
EFI_STATUS
EFIAPI
PeiCoreInstallPpi (
  IN EFI_PEI_PPI_DESCRIPTOR           *PpiList
  )
;

/**
	This service enables PEIMs to replace an entry in the PPI database with an alternate entry.

	@param	OldPpi Pointer to the old PEI PPI Descriptors.
	@param	NewPpi Pointer to the new PEI PPI Descriptors.

	@retval	EFI_SUCCESS The interface was successfully installed.
	@retval	EFI_INVALID_PARAMETER The OldPpi or NewPpi is NULL.
	@retval	EFI_INVALID_PARAMETER Any of the PEI PPI descriptors in the list do not have
	the EFI_PEI_PPI_DESCRIPTOR_PPI bit set in the Flags field.
	@retval	EFI_OUT_OF_RESOURCES There is no additional space in the PPI database.
	@retval	EFI_NOT_FOUND The PPI for which the reinstallation was requested has not been installed.

**/
EFI_STATUS
EFIAPI
PeiCoreReinstallPpi (
  IN EFI_PEI_PPI_DESCRIPTOR           *OldPpi,
  IN EFI_PEI_PPI_DESCRIPTOR           *NewPpi
  )
;

/**
	This service enables PEIMs to discover a given instance of an interface.

	@param	Guid A pointer to the GUID whose corresponding interface needs to be found.
	@param	Instance The N-th instance of the interface that is required.
	@param	PpiDescriptor A pointer to instance of the EFI_PEI_PPI_DESCRIPTOR.
	@param	Ppi A pointer to the instance of the interface.

	@retval	EFI_SUCCESS The interface was successfully returned.
	@retval	EFI_NOT_FOUND The PPI descriptor is not found in the database.

**/
EFI_STATUS
EFIAPI
PeiCoreLocatePpi (
  IN EFI_GUID                         *Guid,
  IN UINTN                            Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR       **PpiDescriptor,
  IN OUT VOID                         **Ppi
  )
;

/**
	This service enables PEIMs to register a given service to be invoked
	when another service is installed or reinstalled.

	@param	NotifyList A pointer to the list of notification interfaces that the caller shall install.

	@retval	EFI_SUCCESS The interface was successfully installed.
	@retval	EFI_INVALID_PARAMETER The NotifyList pointer is NULL.
	@retval	EFI_INVALID_PARAMETER Any of the PEI notify descriptors in the list do not have
	the EFI_PEI_PPI_DESCRIPTOR_NOTIFY_TYPES bit set in the Flags field.
	@retval	EFI_OUT_OF_RESOURCES There is no additional space in the PPI database.

**/
EFI_STATUS
EFIAPI
PeiCoreNotifyPpi (
  IN EFI_PEI_NOTIFY_DESCRIPTOR        *NotifyList
  )
;

/**
	This service enables PEIMs to ascertain the present value of the boot mode.  

	@param	BootMode A pointer to contain the value of the boot mode.

	@retval	EFI_SUCCESS The boot mode was returned successfully.
	@retval	EFI_INVALID_PARAMETER BootMode is NULL.

**/
EFI_STATUS
EFIAPI
PeiCoreGetBootMode (
  IN OUT EFI_BOOT_MODE                *BootMode
  )
;

/**
	This service enables PEIMs to update the boot mode variable.    

	@param	BootMode The value of the boot mode to set.

	@retval	EFI_SUCCESS The value was successfully updated

**/
EFI_STATUS
EFIAPI
PeiCoreSetBootMode (
  IN EFI_BOOT_MODE                    BootMode
  )
;

/**
	This service enables a PEIM to ascertain the address of the list of HOBs in memory.

	@param	HobList A pointer to the list of HOBs that the PEI Foundation will initialize.

	@retval	EFI_SUCCESS The list was successfully returned.
	@retval	EFI_NOT_AVAILABLE_YET The HOB list is not yet published.

**/
EFI_STATUS
EFIAPI
PeiCoreGetHobList (
  IN OUT VOID                         **HobList
  )
;

/**
	This service enables PEIMs to create various types of HOBs.

	@param	Type The type of HOB to be installed.
	@param	Length The length of the HOB to be added.
	@param	Hob The address of a pointer that will contain the HOB header.

	@retval	EFI_SUCCESS The HOB was successfully created.
	@retval	EFI_OUT_OF_RESOURCES There is no additional space for HOB creation.

**/
EFI_STATUS
EFIAPI
PeiCoreCreateHob (
  IN UINT16                           Type,
  IN UINT16                           Length,
  IN OUT VOID                         **Hob
  )
;

/**
	This service enables PEIMs to discover additional firmware volumes.

	@param	Instance This instance of the firmware volume to find.
	The value 0 is the Boot Firmware Volume (BFV).
	@param	FwVolHeader Pointer to the firmware volume header of the volume to return.

	@retval	EFI_SUCCESS The volume was found.
	@retval	EFI_NOT_FOUND The volume was not found.
	@retval	EFI_INVALID_PARAMETER FwVolHeader is NULL.

**/
EFI_STATUS
EFIAPI
PeiCoreFfsFindNextVolume (
  IN UINTN                            Instance,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER   **FwVolHeader
  )
;

/**
	This service enables PEIMs to discover additional firmware files.

	@param	SearchType A filter to find files only of this type.
	@param	FwVolHeader Pointer to the firmware volume header of the volume to search.
	This parameter must point to a valid FFS volume.
	@param	FileHeader Pointer to the current file from which to begin searching.

	@retval	EFI_SUCCESS The file was found.
	@retval	EFI_NOT_FOUND The file was not found.
	@retval	EFI_NOT_FOUND The header checksum was not zero.

**/
EFI_STATUS
EFIAPI
PeiCoreFfsFindNextFile (
  IN EFI_FV_FILETYPE                  SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER       *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER          **FileHeader
  )
;

/**
	This service enables PEIMs to discover sections of a given type within a valid FFS file.

	@param	SearchType The value of the section type to find.
	@param	FfsFileHeader A pointer to the file header that contains the set of sections to be searched.
	@param	SectionData A pointer to the discovered section, if successful.

	@retval	EFI_SUCCESS The section was found.
	@retval	EFI_NOT_FOUND The section was not found.

**/
EFI_STATUS
EFIAPI
PeiCoreFfsFindSectionData (
  IN EFI_SECTION_TYPE                 SectionType,
  IN EFI_FFS_FILE_HEADER              *FfsFileHeader,
  IN OUT VOID                         **SectionData
  )
;

/**
	This service enables PEIMs to register the permanent memory configuration
	that has been initialized with the PEI Foundation.

	@param	MemoryBegin The value of a region of installed memory.
	@param	MemoryLength The corresponding length of a region of installed memory.

	@retval	EFI_SUCCESS The region was successfully installed in a HOB.
	@retval	EFI_INVALID_PARAMETER MemoryBegin and MemoryLength are illegal for this system.
	@retval	EFI_OUT_OF_RESOURCES There is no additional space for HOB creation.

**/
EFI_STATUS
EFIAPI
PeiCoreInstallPeiMemory (
  IN EFI_PHYSICAL_ADDRESS             MemoryBegin,
  IN UINT64                           MemoryLength
  )
;

/**
	This service enables PEIMs to allocate memory after the permanent memory has been installed by a PEIM.

	@param	MemoryType Type of memory to allocate.
	@param	Pages Number of pages to allocate.
	@param	Memory Pointer of memory allocated.

	@retval	EFI_SUCCESS The memory range was successfully allocated.
	@retval	EFI_INVALID_PARAMETER Type is not equal to AllocateAnyPages.
	@retval	EFI_NOT_AVAILABLE_YET Called with permanent memory not available.
	@retval	EFI_OUT_OF_RESOURCES The pages could not be allocated.

**/
EFI_STATUS
EFIAPI
PeiCoreAllocatePages (
  IN EFI_MEMORY_TYPE                  MemoryType,
  IN UINTN                            Pages,
  IN OUT EFI_PHYSICAL_ADDRESS         *Memory
  )
;

/**
	This service allocates memory from the Hand-Off Block (HOB) heap.

	@param	Size The number of bytes to allocate from the pool.
	@param	Buffer If the call succeeds, a pointer to a pointer to the allocated buffer;
	undefined otherwise.

	@retval	EFI_SUCCESS The allocation was successful
	@retval	EFI_OUT_OF_RESOURCES There is not enough heap to allocate the requested size.

**/
EFI_STATUS
EFIAPI
PeiCoreAllocatePool (
  IN UINTN                            Size,
  OUT VOID                            **Buffer
  )
;

/**
	This service resets the entire platform, including all processors and devices, and reboots the system. 

	@retval	EFI_NOT_AVAILABLE_YET The service has not been installed yet.

**/
EFI_STATUS
EFIAPI
PeiCoreResetSystem (
  VOID
  )
;

#endif

/** @file
  MDE PI library functions and macros

  Copyright (c) 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __PI_LIB_H__
#define __PI_LIB_H__

#include <Pi/PiFirmwareFile.h>


/**
    Allocate and fill a buffer from the Firmware Section identified by a Firmware File GUID name and a Firmware 
    Section type and instance number from any Firmware Volumes in the system.
  
    The function will read the first Firmware Section sepcifed by NameGuid, SectionType and Instance by searching
    for all Firmware Volumes in the system. 

    The search order for Firmware Volumes in the system is determistic but abitrary if no new Firmware Volume is installed
    into the system. The search order for the section specified by SectionType within a Firmware File is defined by
    EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection (). Please check Section 2.4 of Volume 3: Platform Initialization 
    Shared Architectural Elements for detailes.
    
    If SectionType is EFI_SECTION_TE, EFI_SECTION_TE will be used as Firmware Section type to read Firmware Section 
    data from the Firmware File. If no such section exists, EFI_SECTION_PE32 will be used as Firmware Section type to 
    read Firmware Section data from the Firmware File. If no such section specified is found to match , 
    EFI_NOT_FOUND is returned.

    The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
    by this function. This function can only be called at TPL_NOTIFY and below.
    
    If NameGuid is NULL, then ASSERT();
    If Buffer is NULL, then ASSERT();
    If Size is NULL, then ASSERT().
  
    @param  NameGuid             The GUID name of a Firmware File.
    @param  SectionType         The Firmware Section type.
    @param  Instance              The instance number of Firmware Section to read from starting from 0.
    @param  Buffer                  On output, Buffer contains the the data read from the section in the Firmware File found.
    @param  Size                    On output, the size of Buffer.
  
    @retval  EFI_SUCCESS        The image is found and data and size is returned.
    @retval  EFI_NOT_FOUND      The image specified by NameGuid and SectionType can't be found.
    @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
    @retval  EFI_DEVICE_ERROR A hardware error occurs during reading from the Firmware Volume.
    @retval  EFI_ACCESS_DENIED  The firmware volume containing the searched Firmware File is configured to disallow reads.
  
**/
EFI_STATUS
EFIAPI
PiLibGetSectionFromAnyFv (
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         Instance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
;

/**
    Allocate and fill a buffer from a Firmware Section identified by a Firmware File GUID name, a Firmware 
    Section type and instance number from the same Firmware Volume with the caller's FFS.
  
    This functions first locates the EFI_FIRMWARE_VOLUME2_PROTOCOL protocol instance for same Firmrware Volume
    which also contains the FFS of the caller in order to carry out the Firmware Volume read operation. 
    The function then reads the Firmware Section found sepcifed by NameGuid, SectionType and Instance. 
    
    The search order for the section specified by SectionType within a Firmware File is defined by
    EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection (). Please check Section 2.4 of Volume 3: Platform Initialization 
    Shared Architectural Elements for detailes.
    
    If SectionType is EFI_SECTION_TE, EFI_SECTION_TE will be used as Firmware Section type to read Firmware Section 
    data from the Firmware File. If no such section exists, EFI_SECTION_PE32 will be used as Firmware Section type to 
    read Firmware Section data from the Firmware File. If no such section specified is found to match , 
    EFI_NOT_FOUND is returned.
    
    The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
    by this function. This function can be only called at TPL_NOTIFY and below.
    
    If FvHandle is NULL, then ASSERT ();
    If NameGuid is NULL, then ASSERT();
    If Buffer is NULL, then ASSERT();
    If Size is NULL, then ASSERT().

    @param  NameGuid             The GUID name of a Firmware File.
    @param  SectionType         The Firmware Section type.
    @param  Instance              The instance number of Firmware Section to read from starting from 0.
    @param  Buffer                  On output, Buffer contains the the data read from the section in the Firmware File found.
    @param  Size                    On output, the size of Buffer.
  
    @retval  EFI_SUCCESS        The image is found and data and size is returned.
    @retval  EFI_UNSUPPORTED   FvHandle does not support EFI_FIRMWARE_VOLUME2_PROTOCOL.
    @retval  EFI_NOT_FOUND      The image specified by NameGuid and SectionType can't be found.
    @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
    @retval  EFI_DEVICE_ERROR A hardware error occurs during reading from the Firmware Volume.
    @retval  EFI_ACCESS_DENIED  The firmware volume containing the searched Firmware File is configured to disallow reads.
  
**/
EFI_STATUS
EFIAPI
PiLibGetSectionFromCurrentFv (
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         Instance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
;


/**
    Allocate and fill a buffer from the first Firmware Section in the same Firmware File as the caller of this function.
  
    The function will read the first Firmware Section found sepcifed by NameGuid and SectionType from the 
    Firmware Volume specified by FvHandle. On this FvHandle, an EFI_FIRMWARE_VOLUME2_PROTOCOL protocol instance 
    should be located succesfully in order to carry out the Firmware Volume operations.
    
    The search order for the section type specified by SectionType in the Firmware File is using a depth-first 
    and left-to-right algorithm through all sections. The first section found to match SectionType will be returned. 
    
    If SectionType is EFI_SECTION_PE32, EFI_SECTION_PE32 will be used as Firmware Section type 
    to read Firmware Section data from the Firmware File. If no such section exists, the function will try 
    to read a Firmware File named with NameGuid. If no such file exists, EFI_NOT_FOUND is returned.
    
    If SectionType is EFI_SECTION_TE, EFI_SECTION_TE will be used as Firmware Section type to read Firmware Section 
    data from the Firmware File. If no such section exists, EFI_SECTION_PE32 will be used as Firmware Section type to 
    read Firmware Section data from the Firmware File. If no such section exists, the function will try to read a Firmware 
    File named with NameGuid. If no such file exists, EFI_NOT_FOUND is returned.
    
    The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
    by this function. This function can only be called at TPL_NOTIFY and below.
    
    If FvHandle is NULL and WithinImage is TRUE, then ASSERT ();
    If NameGuid is NULL, then ASSERT();
    If Buffer is NULL, then ASSERT();
    If Size is NULL, then ASSERT().
  
    @param  NameGuid             The GUID name of a Firmware File.
    @param  SectionType         The Firmware Section type.
    @param  Buffer                  On output, Buffer contains the the data read from the section in the Firmware File found.
    @param  Size                    On output, the size of Buffer.
  
    @retval  EFI_SUCCESS        The image is found and data and size is returned.
    @retval  EFI_NOT_FOUND      The image specified by NameGuid and SectionType can't be found.
    @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
    @retval  EFI_DEVICE_ERROR A hardware error occurs during reading from the Firmware Volume.
    @retval  EFI_ACCESS_DENIED  The firmware volume containing the searched Firmware File is configured to disallow reads.
  
**/
EFI_STATUS
EFIAPI
PiLibGetSectionFromCurrentFfs (
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         Instance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
;

#endif


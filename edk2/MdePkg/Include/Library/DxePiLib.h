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
  Locates a requested firmware section within a file and returns it to a buffer allocated by this function. 

  PiLibGetSectionFromAnyFv () is used to read a specific section from a file within a firmware volume. The function
  will search the first file with the specified name in all firmware volumes in the system. The search order for firmware 
  volumes in the system is determistic but abitrary if no new firmware volume is added into the system between 
  each calls of this function. 

  After the specific file is located, the function searches the specifc firmware section with type SectionType in this file. 
  The details of this search order is defined in description of EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection () 
  found in PI Specification.

  If SectionType is EFI_SECTION_TE, EFI_SECTION_TE is used as section type to start the search. If EFI_SECTION_TE section 
  is not found, EFI_SECTION_PE32 will be used to try the search again. If no EFI_SECTION_PE32 section is found, EFI_NOT_FOUND 
  is returned.

  The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
  by this function. This function can only be called at TPL_NOTIFY and below.

  If NameGuid is NULL, then ASSERT();
  If Buffer is NULL, then ASSERT();
  If Size is NULL, then ASSERT().

  @param  NameGuid             Pointer to an EFI_GUID, which indicates the file name from which the requested 
                               section will be read. Type EFI_GUID is defined in 
                               InstallProtocolInterface() in the UEFI 2.0 specification. 
  @param  SectionType          Indicates the section type to return. SectionType in conjunction with 
                               SectionInstance indicates which section to return. Type 
                               EFI_SECTION_TYPE is defined in EFI_COMMON_SECTION_HEADER.
  @param  SectionInstance      Indicates which instance of sections with a type of SectionType to return. 
                               SectionType in conjunction with SectionInstance indicates which section to 
                               return. SectionInstance is zero based.
  @param  Buffer               Pointer to a pointer to a buffer in which the section contents are returned, not 
                               including the section header. Caller is responsible to free this memory.
  @param  Size                 Pointer to a caller-allocated UINTN. It indicates the size of the memory represented by 
                               *Buffer.

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
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
;

/**
  Locates a requested firmware section within a file and returns it to a buffer allocated by this function. 

  PiLibGetSectionFromCurrentFv () is used to read a specific section from a file within the same firmware volume from which
  the running image is loaded. If the specific file is found, the function searches the specifc firmware section with type SectionType. 
  The details of this search order is defined in description of EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection () 
  found in PI Specification.

  If SectionType is EFI_SECTION_TE, EFI_SECTION_TE is used as section type to start the search. If EFI_SECTION_TE section 
  is not found, EFI_SECTION_PE32 will be used to try the search again. If no EFI_SECTION_PE32 section is found, EFI_NOT_FOUND 
  is returned.

  The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
  by this function. This function can be only called at TPL_NOTIFY and below.

  If NameGuid is NULL, then ASSERT();
  If Buffer is NULL, then ASSERT();
  If Size is NULL, then ASSERT().

  @param  NameGuid             Pointer to an EFI_GUID, which indicates the file name from which the requested 
                               section will be read. Type EFI_GUID is defined in 
                               InstallProtocolInterface() in the UEFI 2.0 specification. 
  @param  SectionType          Indicates the section type to return. SectionType in conjunction with 
                               SectionInstance indicates which section to return. Type 
                               EFI_SECTION_TYPE is defined in EFI_COMMON_SECTION_HEADER.
  @param  SectionInstance      Indicates which instance of sections with a type of SectionType to return. 
                               SectionType in conjunction with SectionInstance indicates which section to 
                               return. SectionInstance is zero based.
  @param  Buffer               Pointer to a pointer to a buffer in which the section contents are returned, not 
                               including the section header. Caller is responsible to free this memory.
  @param  Size                 Pointer to a caller-allocated UINTN. It indicates the size of the memory represented by 
                               *Buffer.


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
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
;


/**
  Locates a requested firmware section within a file and returns it to a buffer allocated by this function. 

  PiLibGetSectionFromCurrentFfs () searches the specifc firmware section with type SectionType in the same firmware file from
  which the running image is loaded. The details of this search order is defined in description of 
  EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection () found in PI Specification.

  If SectionType is EFI_SECTION_TE, EFI_SECTION_TE is used as section type to start the search. If EFI_SECTION_TE section 
  is not found, EFI_SECTION_PE32 will be used to try the search again. If no EFI_SECTION_PE32 section is found, EFI_NOT_FOUND 
  is returned.


  The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
  by this function. This function can only be called at TPL_NOTIFY and below.

  If Buffer is NULL, then ASSERT();
  If Size is NULL, then ASSERT().

  @param  SectionType          Indicates the section type to return. SectionType in conjunction with 
                               SectionInstance indicates which section to return. Type 
                               EFI_SECTION_TYPE is defined in EFI_COMMON_SECTION_HEADER.
  @param  SectionInstance      Indicates which instance of sections with a type of SectionType to return. 
                               SectionType in conjunction with SectionInstance indicates which section to 
                               return. SectionInstance is zero based.
  @param  Buffer               Pointer to a pointer to a buffer in which the section contents are returned, not 
                               including the section header. Caller is responsible to free this memory.
  @param  Size                 Pointer to a caller-allocated UINTN. It indicates the size of the memory represented by 
                               *Buffer.

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
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
;

#endif


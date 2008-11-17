/** @file
  MDE DXE Services Library provides functions that simplify the development of DXE Drivers.  
  These functions help access data from sections of FFS files.

  Copyright (c) 2008, Intel Corporation                                                         
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
  Searches all the availables firmware volumes and returns the first matching FFS section. 

  This function searches all the firmware volumes for FFS files with an FFS filename specified by NameGuid.  
  The order that the firmware volumes is searched is not deterministic. For each FFS file found a search 
  is made for FFS sections of type SectionType. If the FFS file contains at least SectionInstance instances 
  of the FFS section specified by SectionType, then the SectionInstance instance is returned in Buffer. 
  Buffer is allocated using AllocatePool(), and the size of the allocated buffer is returned in Size. 
  It is the caller's responsibility to use FreePool() to free the allocated buffer.  
  See EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection() for details on how sections 
  are retrieved from an FFS file based on SectionType and SectionInstance.

  If SectionType is EFI_SECTION_TE, and the search with an FFS file fails, 
  the search will be retried with a section type of EFI_SECTION_PE32.
  This function must be called with a TPL <= TPL_NOTIFY.

  If NameGuid is NULL, then ASSERT().
  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().


  @param  NameGuid             A pointer to to the FFS filename GUID to search for within 
                               any of the firmware volumes in the platform. 
  @param  SectionType          Indicates the FFS section type to search for within the FFS file specified by NameGuid.
  @param  SectionInstance      Indicates which section instance within the FFS file specified by NameGuid to retrieve.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.  
                               Is it the caller's respobsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.

  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough rsources available to retrieve the matching FFS section.
  @retval  EFI_DEVICE_ERROR     The FFS section could not be retrieves due to a device error.
  @retval  EFI_ACCESS_DENIED    The FFS section could not be retrieves because the firmware volume that 
                                contains the matching FFS section does not allow reads.
**/
EFI_STATUS
EFIAPI
GetSectionFromAnyFv  (
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  );

/**
  Searches the firmware volume that the currently executing module was loaded from and returns the first matching FFS section. 

  This function searches the firmware volume that the currently executing module was loaded 
  from for an FFS file with an FFS filename specified by NameGuid. If the FFS file is found a search 
  is made for FFS sections of type SectionType. If the FFS file contains at least SectionInstance 
  instances of the FFS section specified by SectionType, then the SectionInstance instance is returned in Buffer.
  Buffer is allocated using AllocatePool(), and the size of the allocated buffer is returned in Size. 
  It is the caller's responsibility to use FreePool() to free the allocated buffer. 
  See EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection() for details on how sections are retrieved from 
  an FFS file based on SectionType and SectionInstance.

  If the currently executing module was not loaded from a firmware volume, then EFI_NOT_FOUND is returned.
  If SectionType is EFI_SECTION_TE, and the search with an FFS file fails, 
  the search will be retried with a section type of EFI_SECTION_PE32.
  
  This function must be called with a TPL <= TPL_NOTIFY.
  If NameGuid is NULL, then ASSERT().
  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().

  @param  NameGuid             A pointer to to the FFS filename GUID to search for within 
                               the firmware volumes that the currently executing module was loaded from.
  @param  SectionType          Indicates the FFS section type to search for within the FFS file specified by NameGuid.
  @param  SectionInstance      Indicates which section instance within the FFS file specified by NameGuid to retrieve.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.  
                               Is it the caller's respobsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.


  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough rsources available to retrieve the matching FFS section.
  @retval  EFI_DEVICE_ERROR     The FFS section could not be retrieves due to a device error.
  @retval  EFI_ACCESS_DENIED    The FFS section could not be retrieves because the firmware volume that 
                                contains the matching FFS section does not allow reads.  
**/
EFI_STATUS
EFIAPI
GetSectionFromFv (
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  );


/**
  Searches the FFS file the the currently executing module was loaded from and returns the first matching FFS section.

  This function searches the FFS file that the currently executing module was loaded from for a FFS sections of type SectionType.
  If the FFS file contains at least SectionInstance instances of the FFS section specified by SectionType, 
  then the SectionInstance instance is returned in Buffer. Buffer is allocated using AllocatePool(), 
  and the size of the allocated buffer is returned in Size. It is the caller's responsibility 
  to use FreePool() to free the allocated buffer. See EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection() for 
  details on how sections are retrieved from an FFS file based on SectionType and SectionInstance.

  If the currently executing module was not loaded from an FFS file, then EFI_NOT_FOUND is returned.
  If SectionType is EFI_SECTION_TE, and the search with an FFS file fails, 
  the search will be retried with a section type of EFI_SECTION_PE32.
  This function must be called with a TPL <= TPL_NOTIFY.
  
  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().


  @param  SectionType          Indicates the FFS section type to search for within the FFS file 
                               that the currently executing module was loaded from.
  @param  SectionInstance      Indicates which section instance to retrieve within the FFS file 
                               that the currently executing module was loaded from.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.  
                               Is it the caller's respobsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.

  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough rsources available to retrieve the matching FFS section.
  @retval  EFI_DEVICE_ERROR     The FFS section could not be retrieves due to a device error.
  @retval  EFI_ACCESS_DENIED    The FFS section could not be retrieves because the firmware volume that 
                                contains the matching FFS section does not allow reads.  
  
**/
EFI_STATUS
EFIAPI
GetSectionFromFfs (
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  );

#endif


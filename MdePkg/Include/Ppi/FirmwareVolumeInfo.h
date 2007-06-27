/* @file
  This file provides location and format of a firmware volume.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  This PPI is defined in PI.
  Version 1.00

**/

#ifndef __EFI_PEI_FIRMWARE_VOLUME_INFO_H__
#define __EFI_PEI_FIRMWARE_VOLUME_INFO_H__



#define EFI_PEI_FIRMWARE_VOLUME_INFO_PPI_GUID \
{ 0x49edb1c1, 0xbf21, 0x4761, { 0xbb, 0x12, 0xeb, 0x0, 0x31, 0xaa, 0xbb, 0x39 } }

typedef struct _EFI_PEI_FIRMWARE_VOLUME_INFO_PPI  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI;

/**
  This PPI describes the location and format of a firmware volume. 
  The FvFormat can be EFI_FIRMWARE_FILE_SYSTEM2_GUID or the GUID for 
  a user-defined format. The  EFI_FIRMWARE_FILE_SYSTEM2_GUID is 
  the PI Firmware Volume format.

  @param  FvFormat      Unique identifier of the format of the memory-mapped firmware volume.

  @param  FvInfo        Points to a buffer which allows the EFI_PEI_FIRMWARE_VOLUME_PPI to
                        process the volume. The format of this buffer is 
                        specific to the FvFormat. For memory-mapped firmware volumes, 
                        this typically points to the first byte of the firmware volume.

  @param  FvInfoSize    Size of the data provided by FvInfo. For memory-mapped firmware volumes,
                        this is typically the size of the firmware volume.

  @param  ParentFvName  If the firmware volume originally came from a firmware file, 
                        then these point to the parent firmware volume
                        name and firmware volume file. If it did not originally come
                        from a firmware file, these should be NULL.

  @param  ParentFileName  If the firmware volume originally came from a firmware file, 
                        then these point to the parent firmware volume
                        name and firmware volume file. If it did not originally come
                        from a firmware file, these should be NULL.

**/
struct _EFI_PEI_FIRMWARE_VOLUME_INFO_PPI {
  EFI_GUID  FvFormat;
  VOID      *FvInfo;
  UINT32    FvInfoSize;
  EFI_GUID  *ParentFvName;
  EFI_GUID  *ParentFileName;
};

extern EFI_GUID gEfiPeiFirmwareVolumeInfoPpiGuid;

#endif


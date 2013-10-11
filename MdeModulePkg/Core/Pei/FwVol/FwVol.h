/** @file
  The internal header file for firmware volume related definitions.
  
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _FWVOL_H_
#define _FWVOL_H_

#include "PeiMain.h"

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))


#define PEI_FW_VOL_SIGNATURE  SIGNATURE_32('P','F','W','V')

typedef struct {
  UINTN                         Signature;
  BOOLEAN                       IsFfs3Fv;
  EFI_PEI_FIRMWARE_VOLUME_PPI   Fv;
} PEI_FW_VOL_INSTANCE;

#define PEI_FW_VOL_INSTANCE_FROM_FV_THIS(a) \
  CR(a, PEI_FW_VOL_INSTANCE, Fv, PEI_FW_VOL_SIGNATURE)


/**
  Process a firmware volume and create a volume handle.

  Create a volume handle from the information in the buffer. For
  memory-mapped firmware volumes, Buffer and BufferSize refer to
  the start of the firmware volume and the firmware volume size.
  For non memory-mapped firmware volumes, this points to a
  buffer which contains the necessary information for creating
  the firmware volume handle. Normally, these values are derived
  from the EFI_FIRMWARE_VOLUME_INFO_PPI.
  
  
  @param This                   Points to this instance of the
                                EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param Buffer                 Points to the start of the buffer.
  @param BufferSize             Size of the buffer.
  @param FvHandle               Points to the returned firmware volume
                                handle. The firmware volume handle must
                                be unique within the system. 

  @retval EFI_SUCCESS           Firmware volume handle created.
  @retval EFI_VOLUME_CORRUPTED  Volume was corrupt.

**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiProcessVolume (
  IN  CONST  EFI_PEI_FIRMWARE_VOLUME_PPI *This,
  IN  VOID                               *Buffer,
  IN  UINTN                              BufferSize,
  OUT EFI_PEI_FV_HANDLE                  *FvHandle
  );
  
/**
  Finds the next file of the specified type.

  This service enables PEI modules to discover additional firmware files. 
  The FileHandle must be unique within the system.

  @param This           Points to this instance of the
                        EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param SearchType     A filter to find only files of this type. Type
                        EFI_FV_FILETYPE_ALL causes no filtering to be
                        done.
  @param FvHandle       Handle of firmware volume in which to
                        search.
  @param FileHandle     Points to the current handle from which to
                        begin searching or NULL to start at the
                        beginning of the firmware volume. Updated
                        upon return to reflect the file found.

  @retval EFI_SUCCESS   The file was found.
  @retval EFI_NOT_FOUND The file was not found. FileHandle contains NULL.

**/  
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindFileByType (
  IN CONST  EFI_PEI_FIRMWARE_VOLUME_PPI *This,
  IN        EFI_FV_FILETYPE             SearchType,
  IN        EFI_PEI_FV_HANDLE           FvHandle,
  IN OUT    EFI_PEI_FILE_HANDLE         *FileHandle
  );

/**
  Find a file within a volume by its name. 
  
  This service searches for files with a specific name, within
  either the specified firmware volume or all firmware volumes.

  @param This                   Points to this instance of the
                                EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FileName               A pointer to the name of the file to find
                                within the firmware volume.
  @param FvHandle               Upon entry, the pointer to the firmware
                                volume to search or NULL if all firmware
                                volumes should be searched. Upon exit, the
                                actual firmware volume in which the file was
                                found.
  @param FileHandle             Upon exit, points to the found file's
                                handle or NULL if it could not be found.

  @retval EFI_SUCCESS           File was found.
  @retval EFI_NOT_FOUND         File was not found.
  @retval EFI_INVALID_PARAMETER FvHandle or FileHandle or
                                FileName was NULL.


**/    
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindFileByName (
  IN  CONST  EFI_PEI_FIRMWARE_VOLUME_PPI *This,
  IN  CONST  EFI_GUID                    *FileName,
  IN  EFI_PEI_FV_HANDLE                  *FvHandle,
  OUT EFI_PEI_FILE_HANDLE                *FileHandle  
  );

/**
  Find the next matching section in the firmware file.
  
  This service enables PEI modules to discover sections
  of a given type within a valid file.
  
  @param This             Points to this instance of the
                          EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param SearchType       A filter to find only sections of this
                          type.
  @param FileHandle       Handle of firmware file in which to
                          search.
  @param SectionData      Updated upon  return to point to the
                          section found.
  
  @retval EFI_SUCCESS     Section was found.
  @retval EFI_NOT_FOUND   Section of the specified type was not
                          found. SectionData contains NULL.
**/      
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindSectionByType (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI    *This,
  IN        EFI_SECTION_TYPE               SearchType,
  IN        EFI_PEI_FILE_HANDLE            FileHandle,
  OUT VOID                                 **SectionData
  );

/**
  Find the next matching section in the firmware file.

  This service enables PEI modules to discover sections
  of a given instance and type within a valid file.

  @param This                   Points to this instance of the
                                EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param SearchType             A filter to find only sections of this
                                type.
  @param SearchInstance         A filter to find the specific instance
                                of sections.
  @param FileHandle             Handle of firmware file in which to
                                search.
  @param SectionData            Updated upon return to point to the
                                section found.
  @param AuthenticationStatus   Updated upon return to point to the
                                authentication status for this section.

  @retval EFI_SUCCESS     Section was found.
  @retval EFI_NOT_FOUND   Section of the specified type was not
                          found. SectionData contains NULL.
**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindSectionByType2 (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI    *This,
  IN        EFI_SECTION_TYPE               SearchType,
  IN        UINTN                          SearchInstance,
  IN        EFI_PEI_FILE_HANDLE            FileHandle,
  OUT VOID                                 **SectionData,
  OUT UINT32                               *AuthenticationStatus
  );

/**
  Returns information about a specific file.

  This function returns information about a specific
  file, including its file name, type, attributes, starting
  address and size. 
   
  @param This                     Points to this instance of the
                                  EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FileHandle               Handle of the file.
  @param FileInfo                 Upon exit, points to the file's
                                  information.

  @retval EFI_SUCCESS             File information returned.
  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.
  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.
  
**/         
EFI_STATUS
EFIAPI
PeiFfsFvPpiGetFileInfo (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI   *This, 
  IN        EFI_PEI_FILE_HANDLE           FileHandle, 
  OUT       EFI_FV_FILE_INFO              *FileInfo
  );

/**
  Returns information about a specific file.

  This function returns information about a specific
  file, including its file name, type, attributes, starting
  address, size and authentication status.

  @param This                     Points to this instance of the
                                  EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FileHandle               Handle of the file.
  @param FileInfo                 Upon exit, points to the file's
                                  information.

  @retval EFI_SUCCESS             File information returned.
  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.
  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiGetFileInfo2 (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI   *This,
  IN        EFI_PEI_FILE_HANDLE           FileHandle,
  OUT       EFI_FV_FILE_INFO2             *FileInfo
  );

/**
  This function returns information about the firmware volume.
  
  @param This                     Points to this instance of the
                                  EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FvHandle                 Handle to the firmware handle.
  @param VolumeInfo               Points to the returned firmware volume
                                  information.

  @retval EFI_SUCCESS             Information returned successfully.
  @retval EFI_INVALID_PARAMETER   FvHandle does not indicate a valid
                                  firmware volume or VolumeInfo is NULL.

**/            
EFI_STATUS
EFIAPI
PeiFfsFvPpiGetVolumeInfo (
  IN  CONST  EFI_PEI_FIRMWARE_VOLUME_PPI   *This, 
  IN  EFI_PEI_FV_HANDLE                    FvHandle, 
  OUT EFI_FV_INFO                          *VolumeInfo
  );

/**
  Convert the handle of FV to pointer of corresponding PEI_CORE_FV_HANDLE.
  
  @param FvHandle   The handle of a FV.
  
  @retval NULL if can not find.
  @return Pointer of corresponding PEI_CORE_FV_HANDLE. 
**/
PEI_CORE_FV_HANDLE *
FvHandleToCoreHandle (
  IN EFI_PEI_FV_HANDLE  FvHandle
  );
  
/**
  Given the input file pointer, search for the next matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.


  @param FvHandle        Pointer to the FV header of the volume to search
  @param FileName        File name
  @param SearchType      Filter to find only files of this type.
                         Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param FileHandle      This parameter must point to a valid FFS volume.
  @param AprioriFile     Pointer to AprioriFile image in this FV if has

  @return EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to search given file

**/
EFI_STATUS
FindFileEx (
  IN  CONST EFI_PEI_FV_HANDLE        FvHandle,
  IN  CONST EFI_GUID                 *FileName,   OPTIONAL
  IN        EFI_FV_FILETYPE          SearchType,
  IN OUT    EFI_PEI_FILE_HANDLE      *FileHandle,
  IN OUT    EFI_PEI_FV_HANDLE        *AprioriFile  OPTIONAL
  );

/**
  Report the information for a new discoveried FV in unknown format.
  
  If the EFI_PEI_FIRMWARE_VOLUME_PPI has not been installed for specifical FV format, but
  the FV in this FV format has been discoveried, then the information of this FV
  will be cached into PEI_CORE_INSTANCE's UnknownFvInfo array.
  Also a notification would be installed for unknown FV format guid, if EFI_PEI_FIRMWARE_VOLUME_PPI
  is installed later by platform's PEIM, the original unknown FV will be processed by
  using new installed EFI_PEI_FIRMWARE_VOLUME_PPI.
  
  @param PrivateData  Point to instance of PEI_CORE_INSTANCE
  @param FvInfo2Ppi   Point to FvInfo2 PPI.
  
  @retval EFI_OUT_OF_RESOURCES  The FV info array in PEI_CORE_INSTANCE has no more spaces.
  @retval EFI_SUCCESS           Success to add the information for unknown FV.
**/
EFI_STATUS
AddUnknownFormatFvInfo (
  IN PEI_CORE_INSTANCE                  *PrivateData,
  IN EFI_PEI_FIRMWARE_VOLUME_INFO2_PPI  *FvInfo2Ppi
  );
  
/**
  Find the FV information according to FV format guid.
  
  This routine also will remove the FV information found by given FV format guid from
  PrivateData->UnknownFvInfo[].
  
  @param PrivateData      Point to instance of PEI_CORE_INSTANCE
  @param Format           Point to given FV format guid
  @param FvInfo           On return, the pointer of FV information buffer in given FV format guid
  @param FvInfoSize       On return, the size of FV information buffer.
  @param AuthenticationStatus On return, the authentication status of FV information buffer.
  
  @retval EFI_NOT_FOUND  The FV is not found for new installed EFI_PEI_FIRMWARE_VOLUME_PPI
  @retval EFI_SUCCESS    Success to find a FV which could be processed by new installed EFI_PEI_FIRMWARE_VOLUME_PPI.
**/
EFI_STATUS
FindUnknownFormatFvInfo (
  IN  PEI_CORE_INSTANCE *PrivateData,
  IN  EFI_GUID          *Format,
  OUT VOID              **FvInfo,
  OUT UINT32            *FvInfoSize,
  OUT UINT32            *AuthenticationStatus
  );
  
/**
  Notification callback function for EFI_PEI_FIRMWARE_VOLUME_PPI.
  
  When a EFI_PEI_FIRMWARE_VOLUME_PPI is installed to support new FV format, this 
  routine is called to process all discoveried FVs in this format.
  
  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.
  
  @retval EFI_SUCCESS  The notification callback is processed correctly.
**/
EFI_STATUS
EFIAPI
ThirdPartyFvPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );  
  
#endif 

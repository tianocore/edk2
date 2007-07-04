/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVolWrite.c

Abstract:

  Implements write firmware file

--*/

#include <DxeMain.h>


EFI_STATUS
EFIAPI
FvWriteFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN UINT32                         NumberOfFiles,
  IN EFI_FV_WRITE_POLICY            WritePolicy,
  IN EFI_FV_WRITE_FILE_DATA         *FileData
  )
/*++

    Routine Description:
      Writes one or more files to the firmware volume.

    Arguments:
    This            - Indicates the calling context.
    NumberOfFiles   - Number of files.
    WritePolicy     - WritePolicy indicates the level of reliability for
                      the write in the event of a power failure or other
                      system failure during the write operation.
    FileData        - FileData is an pointer to an array of EFI_FV_WRITE_DATA.
                      Each element of FileData[] represents a file to be written.

    Returns:
      EFI_SUCCESS                   - Files successfully written to firmware volume
      EFI_OUT_OF_RESOURCES          - Not enough buffer to be allocated.
      EFI_DEVICE_ERROR              - Device error.
      EFI_WRITE_PROTECTED           - Write protected.
      EFI_NOT_FOUND                 - Not found.
      EFI_INVALID_PARAMETER         - Invalid parameter.
      EFI_UNSUPPORTED               - This function not supported.

--*/
{ 
  return EFI_UNSUPPORTED;
}


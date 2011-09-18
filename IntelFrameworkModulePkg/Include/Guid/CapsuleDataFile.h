/** @file
  GUID to specify which FFS file to store the updated capsule data.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UPDATE_DATA_FILE_GUID_H__
#define __UPDATE_DATA_FILE_GUID_H__

#define EFI_UPDATE_DATA_FILE_GUID \
 { 0x283fa2ee, 0x532c, 0x484d, { 0x93, 0x83, 0x9f, 0x93, 0xb3, 0x6f, 0xb, 0x7e } }

extern GUID gEfiUpdateDataFileGuid;

#endif

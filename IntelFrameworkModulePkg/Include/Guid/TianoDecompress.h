/** @file
  Tiano Custom decompress Guid definition.
  
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TIANO_CUSTOM_DECOMPRESS_GUID_H__
#define __TIANO_CUSTOM_DECOMPRESS_GUID_H__

///
/// The Global ID used to identify a section of an FFS file of type 
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using 
/// Tiano Custom compression.
///
#define TIANO_CUSTOM_DECOMPRESS_GUID  \
  { 0xA31280AD, 0x481E, 0x41B6, { 0x95, 0xE8, 0x12, 0x7F, 0x4C, 0x98, 0x47, 0x79 } }

extern GUID gTianoCustomDecompressGuid;

#endif

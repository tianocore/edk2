/** @file
  Lzma Custom decompress algorithm Guid definition.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LZMA_DECOMPRESS_GUID_H__
#define __LZMA_DECOMPRESS_GUID_H__

///
/// The Global ID used to identify a section of an FFS file of type 
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using LZMA.
///
#define LZMA_CUSTOM_DECOMPRESS_GUID  \
  { 0xEE4E5898, 0x3914, 0x4259, { 0x9D, 0x6E, 0xDC, 0x7B, 0xD7, 0x94, 0x03, 0xCF } }

///
/// The Global ID used to identify a section of an FFS file of type 
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using LZMA with X86 code Converter.
///
#define LZMAF86_CUSTOM_DECOMPRESS_GUID  \
  { 0xD42AE6BD, 0x1352, 0x4bfb, { 0x90, 0x9A, 0xCA, 0x72, 0xA6, 0xEA, 0xE8, 0x89 } }

extern GUID gLzmaCustomDecompressGuid;
extern GUID gLzmaF86CustomDecompressGuid;

#endif

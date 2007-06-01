/** @file
  DataHubRecord.h include all data hub sub class GUID defitions.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DataHubRecords.h

  @par Revision Reference:
  These GUID are from Cache subclass spec 0.9, DataHub SubClass spec 0.9, Memory SubClass Spec 0.9, 
  Processor Subclass spec 0.9, Misc SubClass spec 0.9.

**/
#ifndef _DATAHUB_RECORDS_GUID_H_
#define _DATAHUB_RECORDS_GUID_H_

#define EFI_PROCESSOR_PRODUCER_GUID \
  { 0x1bf06aea, 0x5bec, 0x4a8d, {0x95, 0x76, 0x74, 0x9b, 0x09, 0x56, 0x2d, 0x30 } }

extern  EFI_GUID gEfiProcessorProducerGuid;


#define EFI_PROCESSOR_SUBCLASS_GUID \
  { 0x26fdeb7e, 0xb8af, 0x4ccf, {0xaa, 0x97, 0x02, 0x63, 0x3c, 0xe4, 0x8c, 0xa7 } }

extern  EFI_GUID gEfiProcessorSubClassGuid;


#define EFI_CACHE_SUBCLASS_GUID \
  { 0x7f0013a7, 0xdc79, 0x4b22, {0x80, 0x99, 0x11, 0xf7, 0x5f, 0xdc, 0x82, 0x9d } }

extern  EFI_GUID gEfiCacheSubClassGuid;


#define EFI_MEMORY_PRODUCER_GUID \
  { 0x1d7add6e, 0xb2da, 0x4b0b, {0xb2, 0x9f, 0x49, 0xcb, 0x42, 0xf4, 0x63, 0x56 } }

extern  EFI_GUID gEfiMemoryProducerGuid;


#define EFI_MEMORY_SUBCLASS_GUID \
  {0x4E8F4EBB, 0x64B9, 0x4e05, {0x9B, 0x18, 0x4C, 0xFE, 0x49, 0x23, 0x50, 0x97} }

extern  EFI_GUID  gEfiMemorySubClassGuid;


#define EFI_MISC_PRODUCER_GUID \
  { 0x62512c92, 0x63c4, 0x4d80, {0x82, 0xb1, 0xc1, 0xa4, 0xdc, 0x44, 0x80, 0xe5 } } 

extern  EFI_GUID gEfiMiscProducerGuid;


#define EFI_MISC_SUBCLASS_GUID \
  { 0x772484B2, 0x7482, 0x4b91, {0x9F, 0x9A, 0xAD, 0x43, 0xF8, 0x1C, 0x58, 0x81 } }

extern  EFI_GUID  gEfiMiscSubClassGuid;


#endif


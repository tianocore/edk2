/** @file
  DataHubProducer.h include all GUID definition for producer

  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

  @par Revision Reference:
  These GUID are from Cache subclass spec 0.9, DataHub SubClass spec 0.9, Memory SubClass Spec 0.9,
  Processor Subclass spec 0.9, Misc SubClass spec 0.9.

**/

#ifndef _DATAHUB_PRODUCER_H_
#define _DATAHUB_PRODUCER_H_

#define EFI_PROCESSOR_PRODUCER_GUID \
  { 0x1bf06aea, 0x5bec, 0x4a8d, {0x95, 0x76, 0x74, 0x9b, 0x09, 0x56, 0x2d, 0x30 } }

extern  EFI_GUID gEfiProcessorProducerGuid;

#define EFI_MEMORY_PRODUCER_GUID \
  { 0x1d7add6e, 0xb2da, 0x4b0b, {0xb2, 0x9f, 0x49, 0xcb, 0x42, 0xf4, 0x63, 0x56 } }

extern  EFI_GUID gEfiMemoryProducerGuid;

#define EFI_MISC_PRODUCER_GUID \
  { 0x62512c92, 0x63c4, 0x4d80, {0x82, 0xb1, 0xc1, 0xa4, 0xdc, 0x44, 0x80, 0xe5 } }

extern  EFI_GUID gEfiMiscProducerGuid;

#endif // _DATAHUB_PRODUCER_H_

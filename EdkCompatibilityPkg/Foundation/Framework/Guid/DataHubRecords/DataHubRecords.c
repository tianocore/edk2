/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    DataHubRecords.c

Abstract:

    This file defines GUIDs and associated data structures for records 
    posted to the Data Hub. 
    The producers of these records use these definitions to construct 
    records.
    The consumers of these records use these definitions to retrieve,
    filter and parse records.

    For more information please look at DataHub.doc

--*/



#include "Tiano.h"
#include EFI_GUID_DEFINITION (DataHubRecords)

EFI_GUID gProcessorProducerGuid = EFI_PROCESSOR_PRODUCER_GUID;

EFI_GUID gProcessorSubClassName = EFI_PROCESSOR_SUBCLASS_GUID;

EFI_GUID gCacheSubClassName     = EFI_CACHE_SUBCLASS_GUID;

EFI_GUID gMiscProducerGuid      = EFI_MISC_PRODUCER_GUID;
EFI_GUID gMiscSubClassName      = EFI_MISC_SUBCLASS_GUID;
EFI_GUID gEfiMiscSubClassGuid   = EFI_MISC_SUBCLASS_GUID;


EFI_GUID gMemoryProducerGuid    = EFI_MEMORY_PRODUCER_GUID;

EFI_GUID  gEfiMemorySubClassGuid = EFI_MEMORY_SUBCLASS_GUID;



/* eof - DataHubRecords.c */

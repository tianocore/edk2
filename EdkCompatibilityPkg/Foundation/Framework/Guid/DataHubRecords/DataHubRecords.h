/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    DataHubRecords.h

Abstract:

    This file defines GUIDs and associated data structures for records 
    posted to the Data Hub. 
    The producers of these records use these definitions to construct 
    records.
    The consumers of these records use these definitions to retrieve,
    filter and parse records.

    For more information please look at DataHub.doc

--*/

#ifndef _DATAHUB_RECORDS_H_
#define _DATAHUB_RECORDS_H_

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include "TianoHii.h"
#else
#include "EfiInternalFormRepresentation.h"
#endif
#include "DataHubSubClass.h"
#include "DataHubSubClassProcessor.h"
#include "DataHubSubClassCache.h"
#include "DataHubSubClassMemory.h"
#include "DataHubSubClassMisc.h"

/*++
BEGIN: Processor records definitions
--*/

extern  EFI_GUID gProcessorProducerGuid;

#define EFI_PROCESSOR_PRODUCER_GUID \
  { 0x1bf06aea, 0x5bec, 0x4a8d, {0x95, 0x76, 0x74, 0x9b, 0x09, 0x56, 0x2d, 0x30} }


extern  EFI_GUID gProcessorSubClassName;


extern  EFI_GUID gCacheSubClassName;


extern  EFI_GUID gMiscSubClassName;

/*++
END: Processor records definitions
--*/



/*++
BEGIN: Memory records definitions
--*/

extern  EFI_GUID gMemoryProducerGuid;

#define EFI_MEMORY_PRODUCER_GUID \
  { 0x1d7add6e, 0xb2da, 0x4b0b, {0xb2, 0x9f, 0x49, 0xcb, 0x42, 0xf4, 0x63, 0x56} }

//
// ... need memory sub classes here...
//
extern EFI_GUID  gEfiMemorySubClassGuid;



/*++
END: Memory records definitions
--*/


/*++
BEGIN: Misc records definitions
--*/

extern  EFI_GUID gMiscProducerGuid;

#define EFI_MISC_PRODUCER_GUID \
{ 0x62512c92, 0x63c4, 0x4d80, {0x82, 0xb1, 0xc1, 0xa4, 0xdc, 0x44, 0x80, 0xe5} } 



//
// ... need misc sub classes here...
//
extern EFI_GUID  gEfiMiscSubClassGuid;




/*++
END: Misc records definitions
--*/


#endif

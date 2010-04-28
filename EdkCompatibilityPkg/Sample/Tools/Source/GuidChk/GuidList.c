/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GuidList.c  

Abstract:

  Utility to create a GUID-to-name listing file that can
  be used by other utilities. Basic operation is to take the
  table of name+GUIDs that we have compiled into this utility,
  and create a text file that can be parsed by other utilities
  to do replacement of "name" with "GUID".

Notes:
  To add a new GUID to this database:
    1. Add a "#include EFI_GUID_DEFINITION(name)" statement below
    2. Modify the mGuidList[] array below to add the new GUID name

  The only issue that may come up is that, if the source GUID file
  is not in the standard GUID directory, then this utility won't
  compile because the #include fails. In this case you'd need
  to define a new macro (if it's in a standard place) or modify
  this utility's makefile to add the path to your new .h file.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "Tiano.h"
#include "EfiUtilityMsgs.h"

#include EFI_GUID_DEFINITION (Apriori)
#include EFI_GUID_DEFINITION (AcpiTableStorage)
#include EFI_GUID_DEFINITION (Bmp)
#include EFI_GUID_DEFINITION (AcpiTableStorage)
#include EFI_GUID_DEFINITION (PeiApriori)


#define GUID_XREF(varname, guid) { \
    #varname, #guid, guid \
  }

#define NULL_GUID \
  { \
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 \
  }

typedef struct {
  INT8      *VariableName;
  INT8      *DefineName;
  EFI_GUID  Guid;
} GUID_LIST;

//
// This is our table of all GUIDs we want to print out to create
// a GUID-to-name cross reference.
// Use the #defined name from the GUID definition's source .h file.
//
static GUID_LIST  mGuidList[] = {
  GUID_XREF(gEfiPeiAprioriGuid, EFI_PEI_APRIORI_FILE_NAME_GUID),
  GUID_XREF(gAprioriGuid, EFI_APRIORI_GUID),
  GUID_XREF(gEfiDefaultBmpLogoGuid, EFI_DEFAULT_BMP_LOGO_GUID),
  GUID_XREF(gEfiAcpiTableStorageGuid, EFI_ACPI_TABLE_STORAGE_GUID),
  //
  // Terminator
  //
  {
    NULL,
    NULL,
    NULL_GUID
  }
};

void
PrintGuidText (
  FILE      *OutFptr,
  INT8      *VariableName,
  INT8      *DefineName,
  EFI_GUID  *Guid
  );

int
CreateGuidList (
  INT8    *OutFileName
  )
/*++

Routine Description:
  Print our GUID/name list to the specified output file.
  
Arguments:
  OutFileName  - name of the output file to write our results to.

Returns:
  0       if successful
  nonzero otherwise
  
--*/
{
  FILE  *OutFptr;
  int   Index;

  //
  // Open output file for writing. If the name is NULL, then write to stdout
  //
  if (OutFileName != NULL) {
    OutFptr = fopen (OutFileName, "w");
    if (OutFptr == NULL) {
      Error (NULL, 0, 0, OutFileName, "failed to open output file for writing");
      return STATUS_ERROR;
    }
  } else {
    OutFptr = stdout;
  }

  for (Index = 0; mGuidList[Index].VariableName != NULL; Index++) {
    PrintGuidText (OutFptr, mGuidList[Index].VariableName, mGuidList[Index].DefineName, &mGuidList[Index].Guid);
  }
  //
  // Close the output file if they specified one.
  //
  if (OutFileName != NULL) {
    fclose (OutFptr);
  }

  return STATUS_SUCCESS;
}

void
PrintGuidText (
  FILE      *OutFptr,
  INT8      *VariableName,
  INT8      *DefineName,
  EFI_GUID  *Guid
  )
/*++

Routine Description:
  Print a GUID/name combo in INF-style format

  guid-guid-guid-guid DEFINE_NAME gName

Arguments:
  OutFptr       - file pointer to which to write the output
  VariableName  - the GUID variable's name
  DefineName    - the name used in the #define
  Guid          - pointer to the GUID value

Returns:
  NA

--*/
{
  if (OutFptr == NULL) {
    OutFptr = stdout;
  }

  fprintf (
    OutFptr,
    "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X %s %s\n",
    Guid->Data1,
    Guid->Data2,
    Guid->Data3,
    Guid->Data4[0],
    Guid->Data4[1],
    Guid->Data4[2],
    Guid->Data4[3],
    Guid->Data4[4],
    Guid->Data4[5],
    Guid->Data4[6],
    Guid->Data4[7],
    DefineName,
    VariableName
    );
}

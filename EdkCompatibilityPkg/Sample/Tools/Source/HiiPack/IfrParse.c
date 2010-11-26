/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IfrParse.c  

Abstract:

  Routines for parsing and managing HII IFR packs.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Tiano.h"
#include "EfiUtilityMsgs.h"
#include "EfiInternalFormRepresentation.h"
#include "Hii.h"
#include "IfrParse.h"
#include "HiiPack.h"

typedef struct _VARIABLE_STORE_ENTRY {
  struct _VARIABLE_STORE_ENTRY  *Next;
  CHAR8                         VarName[MAX_VARIABLE_NAME];
  char                          *VarBuffer;
  int                           VarBufferSize;
  EFI_HII_VARIABLE_PACK         *VarPack;
  int                           VarPackSize;
} VARIABLE_STORE_ENTRY;

typedef STATUS (*IFR_PARSE_FUNCTION) (IFR_PARSE_CONTEXT * Context);

typedef struct {
  INT8                *Name;
  INT32               Size;
  IFR_PARSE_FUNCTION  Parse;
} IFR_PARSE_TABLE_ENTRY;

static
STATUS
IfrParse01 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse02 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse03 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse05 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse06 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse07 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse08 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse09 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse0A (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse0B (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse0C (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse0D (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse0E (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse0F (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse10 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse11 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse12 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse13 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse14 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse15 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse16 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse17 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse18 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse19 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse1A (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse1B (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse1C (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse1D (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse1E (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse1F (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse20 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse21 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse22 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse23 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse24 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse25 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse26 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse27 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse28 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse29 (
  IFR_PARSE_CONTEXT *Context
  );
static
STATUS
IfrParse2A (
  IFR_PARSE_CONTEXT *Context
  );

static const IFR_PARSE_TABLE_ENTRY  mIfrParseTable[] = {
  {
    0,
    0,
    NULL
  },  // invalid
  {
    "EFI_IFR_FORM",
    sizeof (EFI_IFR_FORM),
    IfrParse01
  },
  {
    "EFI_IFR_SUBTITLE",
    sizeof (EFI_IFR_SUBTITLE),
    IfrParse02
  },
  {
    "EFI_IFR_TEXT",
    -6,
    IfrParse03
  },  // sizeof (EFI_IFR_TEXT) },
  {
    "unused 0x04 opcode",
    0,
    NULL
  },  // EFI_IFR_GRAPHIC_OP
  {
    "EFI_IFR_ONE_OF",
    sizeof (EFI_IFR_ONE_OF),
    IfrParse05
  },
  {
    "EFI_IFR_CHECK_BOX",
    sizeof (EFI_IFR_CHECK_BOX),
    IfrParse06
  },
  {
    "EFI_IFR_NUMERIC",
    sizeof (EFI_IFR_NUMERIC),
    IfrParse07
  },
  {
    "EFI_IFR_PASSWORD",
    sizeof (EFI_IFR_PASSWORD),
    IfrParse08
  },
  {
    "EFI_IFR_ONE_OF_OPTION",
    sizeof (EFI_IFR_ONE_OF_OPTION),
    IfrParse09
  },
  {
    "EFI_IFR_SUPPRESS",
    sizeof (EFI_IFR_SUPPRESS),
    IfrParse0A
  },
  {
    "EFI_IFR_END_FORM",
    sizeof (EFI_IFR_END_FORM),
    IfrParse0B
  },
  {
    "EFI_IFR_HIDDEN",
    sizeof (EFI_IFR_HIDDEN),
    IfrParse0C
  },
  {
    "EFI_IFR_END_FORM_SET",
    sizeof (EFI_IFR_END_FORM_SET),
    IfrParse0D
  },
  {
    "EFI_IFR_FORM_SET",
    sizeof (EFI_IFR_FORM_SET),
    IfrParse0E
  },
  {
    "EFI_IFR_REF",
    sizeof (EFI_IFR_REF),
    IfrParse0F
  },
  {
    "EFI_IFR_END_ONE_OF",
    sizeof (EFI_IFR_END_ONE_OF),
    IfrParse10
  },
  {
    "EFI_IFR_INCONSISTENT",
    sizeof (EFI_IFR_INCONSISTENT),
    IfrParse11
  },
  {
    "EFI_IFR_EQ_ID_VAL",
    sizeof (EFI_IFR_EQ_ID_VAL),
    IfrParse12
  },
  {
    "EFI_IFR_EQ_ID_ID",
    sizeof (EFI_IFR_EQ_ID_ID),
    IfrParse13
  },
  {
    "EFI_IFR_EQ_ID_LIST",
    -(int) (sizeof (EFI_IFR_EQ_ID_LIST)),
    IfrParse14
  },
  {
    "EFI_IFR_AND",
    sizeof (EFI_IFR_AND),
    IfrParse15
  },
  {
    "EFI_IFR_OR",
    sizeof (EFI_IFR_OR),
    IfrParse16
  },
  {
    "EFI_IFR_NOT",
    sizeof (EFI_IFR_NOT),
    IfrParse17
  },
  {
    "EFI_IFR_END_IF",
    sizeof (EFI_IFR_END_IF),
    IfrParse18
  },
  {
    "EFI_IFR_GRAYOUT",
    sizeof (EFI_IFR_GRAYOUT),
    IfrParse19
  },
  {
    "EFI_IFR_DATE",
    sizeof (EFI_IFR_DATE) / 3,
    IfrParse1A
  },
  {
    "EFI_IFR_TIME",
    sizeof (EFI_IFR_TIME) / 3,
    IfrParse1B
  },
  {
    "EFI_IFR_STRING",
    sizeof (EFI_IFR_STRING),
    IfrParse1C
  },
  {
    "EFI_IFR_LABEL",
    sizeof (EFI_IFR_LABEL),
    IfrParse1D
  },
  {
    "EFI_IFR_SAVE_DEFAULTS",
    sizeof (EFI_IFR_SAVE_DEFAULTS),
    IfrParse1E
  },
  {
    "EFI_IFR_RESTORE_DEFAULTS",
    sizeof (EFI_IFR_RESTORE_DEFAULTS),
    IfrParse1F
  },
  {
    "EFI_IFR_BANNER",
    sizeof (EFI_IFR_BANNER),
    IfrParse20
  },
  {
    "EFI_IFR_INVENTORY",
    sizeof (EFI_IFR_INVENTORY),
    IfrParse21
  },
  {
    "EFI_IFR_EQ_VAR_VAL_OP",
    sizeof (EFI_IFR_EQ_VAR_VAL),
    IfrParse22
  },
  {
    "EFI_IFR_ORDERED_LIST_OP",
    sizeof (EFI_IFR_ORDERED_LIST),
    IfrParse23
  },
  {
    "EFI_IFR_VARSTORE_OP",
    -(int) (sizeof (EFI_IFR_VARSTORE)),
    IfrParse24
  },
  {
    "EFI_IFR_VARSTORE_SELECT_OP",
    sizeof (EFI_IFR_VARSTORE_SELECT),
    IfrParse25
  },
  {
    "EFI_IFR_VARSTORE_SELECT_PAIR_OP",
    sizeof (EFI_IFR_VARSTORE_SELECT_PAIR),
    IfrParse26
  },
  {
    "EFI_IFR_TRUE",
    sizeof (EFI_IFR_TRUE),
    IfrParse27
  },
  {
    "EFI_IFR_FALSE",
    sizeof (EFI_IFR_FALSE),
    IfrParse28
  },
  {
    "EFI_IFR_GT",
    sizeof (EFI_IFR_GT),
    IfrParse29
  },
  {
    "EFI_IFR_GE",
    sizeof (EFI_IFR_GE),
    IfrParse2A
  },
};
#define PARSE_TABLE_ENTRIES (sizeof (mIfrParseTable) / sizeof (mIfrParseTable[0]))

static
STATUS
GetVarStoreInfo (
  IFR_PARSE_CONTEXT   *Context,
  UINT16              VarId,
  EFI_GUID            **VarStoreGuid,
  char                **VarStoreName
  );

static
void
FreeVarStores (
  VOID
  );

static
STATUS
CreateVarStore (
  EFI_GUID *VarGuid,
  CHAR8    *VarName,
  int      VarStoreSize
  );

static
STATUS
SetDefaults (
  IFR_PARSE_CONTEXT *Context,
  UINT32            MfgDefaults
  );

//
// Globals
//
static IFR_PARSE_CONTEXT            *mParseContext    = NULL;
static VARIABLE_STORE_ENTRY         *mVariableStores  = NULL;
static int                          BreakOnOpcodeTag  = 0;
static int                          OpcodeTag         = 1;

/*****************************************************************************/
STATUS
IfrParseCheck (
  char    *Buffer,
  long    BufferSize
  )
/*++

Routine Description:

  Check a buffer to ensure that is is parseable IFR
  
Arguments:

  Buffer      - pointer to raw IFR bytes
  BufferSize  - size of IFR pointed to by Buffer

Returns:

  STATUS_SUCCESS      if successful
  STATUS_ERROR        otherwise
  
--*/
{
  char              *Start;

  char              *End;

  char              *Pos;
  EFI_IFR_OP_HEADER *OpHeader;
  char              *FileName;
  FileName = "";
  //
  // Walk the list of IFR statements in the IFR pack
  //
  Start = Buffer;
  Pos   = Buffer;
  End   = Start + BufferSize;
  while ((Pos >= Start) && (Pos < End)) {
    OpHeader = (EFI_IFR_OP_HEADER *) Pos;
    //
    // Check range on size
    //
    if (Pos + OpHeader->Length > End) {
      Error (NULL, 0, 0, FileName, "invalid IFR opcode size at offset 0x%X", (int) Pos - (int) Start);
      return STATUS_ERROR;
    }

    if (OpHeader->Length == 0) {
      Error (NULL, 0, 0, FileName, "IFR opcode size=0 at offset 0x%X", (int) Pos - (int) Start);
      return STATUS_ERROR;
    }
    //
    // See if it's the END_FORMSET opcode
    //
    if (OpHeader->OpCode == EFI_IFR_END_FORM_SET_OP) {
      break;
    }
    //
    // Advance to next IFR statement/opcode
    //
    Pos += OpHeader->Length;
  }

  return STATUS_SUCCESS;
}

STATUS
IfrParseInit (
  VOID
  )
/*++

Routine Description:

  Initialize this module for IFR pack parsing
  
Arguments:

Returns:

  STATUS_SUCCESS      always
  
--*/
{
  return STATUS_SUCCESS;
}

STATUS
IfrParseEnd (
  VOID
  )
/*++

Routine Description:

  Free up memory allocated during IFR pack parsing done by this module
  
Arguments:
  None

Returns:

  STATUS_SUCCESS      always
  
--*/
{
  IFR_PARSE_CONTEXT *NextContext;
  IFR_PARSE_ENTRY   *NextEntry;
  //
  // Free up the memory from our parse contexts
  //
  while (mParseContext != NULL) {
    while (mParseContext->Ifr != NULL) {
      NextEntry = mParseContext->Ifr->Next;
      //
      // We pointed directly into the user buffer, rather than make
      // a copy, so don't free up the bytes.
      //
      free (mParseContext->Ifr);
      mParseContext->Ifr = NextEntry;
    }

    NextContext = mParseContext->Next;
    free (mParseContext->PackHeader);
    free (mParseContext);
    mParseContext = NextContext;
  }

  return STATUS_SUCCESS;
}

static
void
FreeVarStores (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
{
  VARIABLE_STORE_ENTRY  *NextVarStore;
  //
  // Free up memory from our variable stores
  //
  while (mVariableStores != NULL) {
    if (mVariableStores->VarPack != NULL) {
      free (mVariableStores->VarPack);
    }

    NextVarStore = mVariableStores->Next;
    free (mVariableStores);
    mVariableStores = NextVarStore;
  }
}

/******************************************************************************
  FUNCTION: IfrParsePack() 
   
  DESCRIPTION:  Given a pointer to an IFR pack, parse it to create a linked
    list of opcodes and relevant data required for later dumping.


*******************************************************************************/
STATUS
IfrParsePack (
  int               Handle,
  EFI_HII_IFR_PACK  *PackHeader,
  EFI_GUID          *PackageGuid
  )
/*++

Routine Description:

  Given a pointer to an IFR pack, parse it to create a linked
  list of opcodes and relevant data required for later dumping.
  
Arguments:

  Handle          - the handle number associated with this IFR pack. It
                    can be used later to retrieve more info on the particular
                    pack
  PackHeader      - pointer to IFR pack to parse
  PackageGuid     - on input, it comes from the HII data table entry for this pack. 
                    On output, we'll return the IFR formset GUID.

Returns:

  STATUS_SUCCESS      always

--*/
{
  EFI_IFR_OP_HEADER *OpHeader;
  IFR_PARSE_CONTEXT *Context;
  IFR_PARSE_CONTEXT *TempContext;
  IFR_PARSE_ENTRY   *IfrEntry;
  //
  // Initialize our context
  //
  Context = (IFR_PARSE_CONTEXT *) malloc (sizeof (IFR_PARSE_CONTEXT));
  if (Context == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset ((void *) Context, 0, sizeof (IFR_PARSE_CONTEXT));
  //
  // Cache a copy of the input pack so the caller can free their copy
  //
  Context->PackHeader = (EFI_HII_IFR_PACK *) malloc (PackHeader->Header.Length);
  if (Context->PackHeader == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    free (Context);
    return STATUS_ERROR;
  }

  memcpy (Context->PackHeader, PackHeader, PackHeader->Header.Length);
  Context->IfrBufferStart = (char *) (Context->PackHeader + 1);
  Context->CurrentPos     = Context->IfrBufferStart;
  Context->IfrBufferLen   = PackHeader->Header.Length - sizeof (EFI_HII_IFR_PACK);
  Context->Handle         = Handle;
  Context->FormsetGuid    = &Context->NullGuid;
  Context->PackageGuid    = *PackageGuid;
  //
  // Add it to the end of our list
  //
  if (mParseContext == NULL) {
    mParseContext = Context;
  } else {
    TempContext = mParseContext;
    while (TempContext->Next != NULL) {
      TempContext = TempContext->Next;
    }

    TempContext->Next = Context;
  }
  //
  // Walk the opcodes in the pack
  //
  while
  (
    (Context->CurrentPos >= Context->IfrBufferStart) &&
    (Context->CurrentPos < Context->IfrBufferStart + Context->IfrBufferLen)
  ) {
    OpHeader = (EFI_IFR_OP_HEADER *) Context->CurrentPos;
    //
    // Allocate a new IFR entry to put in our linked list, then
    // point directly to the caller's raw data.
    //
    IfrEntry = (IFR_PARSE_ENTRY *) malloc (sizeof (IFR_PARSE_ENTRY));
    if (IfrEntry == NULL) {
      Error (NULL, 0, 0, "memory allocation failure", NULL);
      free (Context->PackHeader);
      free (Context);
      return STATUS_ERROR;
    }

    memset ((void *) IfrEntry, 0, sizeof (IFR_PARSE_ENTRY));
    IfrEntry->Tag = ++OpcodeTag;
    if (OpcodeTag == BreakOnOpcodeTag) {
      EFI_BREAKPOINT ();
    }

    IfrEntry->RawIfrHeader = (EFI_IFR_OP_HEADER *) (Context->CurrentPos);
    //
    // Add this entry to our linked list. If it's not the first, then
    // forward the variable store settings from the previous entry.
    //
    if (Context->LastIfr != NULL) {
      IfrEntry->VarStoreGuid1 = Context->LastIfr->VarStoreGuid1;
      IfrEntry->VarStoreName1 = Context->LastIfr->VarStoreName1;
      IfrEntry->VarStoreGuid2 = Context->LastIfr->VarStoreGuid2;
      IfrEntry->VarStoreName2 = Context->LastIfr->VarStoreName2;
      Context->LastIfr->Next  = IfrEntry;
    } else {
      Context->Ifr = IfrEntry;
    }

    Context->LastIfr = IfrEntry;
    //
    // Switch on the opcode to parse it
    //
    if (OpHeader->OpCode < PARSE_TABLE_ENTRIES) {
      if (mIfrParseTable[OpHeader->OpCode].Parse != NULL) {
        mIfrParseTable[OpHeader->OpCode].Parse (Context);
      }
    } else {
      Error (
        NULL,
        0,
        0,
        "invalid opcode found in IFR",
        "offset=0x%X opcode=0x%02X",
        (int) OpHeader - (int) Context->PackHeader,
        (int) OpHeader->OpCode
        );
      free (IfrEntry);
      free (Context->PackHeader);
      free (Context);
      return STATUS_ERROR;
    }
    //
    // If it's the END_FORMSET opcode, then we're done
    //
    if (OpHeader->OpCode == EFI_IFR_END_FORM_SET_OP) {
      break;
    }
    //
    // Advance to next IFR statement/opcode
    //
    if (OpHeader->Length == 0) {
      Error (NULL, 0, 0, "0-length IFR opcode encountered", NULL);
      free (IfrEntry);
      free (Context->PackHeader);
      free (Context);
      return STATUS_ERROR;
    }

    Context->CurrentPos += OpHeader->Length;
  }
  //
  // Return the form GUID.
  //
  *PackageGuid = *Context->FormsetGuid;  
  return STATUS_SUCCESS;
}

/******************************************************************************
  FUNCTION:  GetVarStoreInfo()
   
  DESCRIPTION:  IFR contains VARSTORE opcodes to specify where variable data 
    for following opcodes is supposed to be stored. One VARSTORE statement
    allows you to specify the variable store GUID and a key, and another
    VARSTORE (select) allows you to specify the key of a VARSTORE statement. 
    Given the key from a VARSTORE_SELECT statement, go find the corresponding
    VARSTORE statement with a matching key and return the varstore GUID and 
    name. If key == 0, then the variable store is FormsetGuid."Setup"
*******************************************************************************/
static
STATUS
GetVarStoreInfo (
  IFR_PARSE_CONTEXT     *Context,
  UINT16                VarId,
  EFI_GUID              **VarStoreGuid,
  char                  **VarStoreName
  )
/*++

Routine Description:

  Get variable store information from an IFR pack for a given variable store ID.
  
Arguments:

  Context       - pointer to IFR parse context
  VarId         - variable store ID referenced by IFR being parsed
  VarStoreGuid  - outgoing GUID of the variable store corresponding to VarId
  VarStoreName  - outgoing variable name of variable store corresponding to VarId

Returns:

  STATUS_SUCCESS      - variable store with matching VarId found, and outoing GUID/Name are valid
  STATUS_ERROR        - otherwise

--*/
{
  IFR_PARSE_ENTRY   *Ptr;
  EFI_IFR_VARSTORE  *VarStore;
  if (Context == NULL) {
    return STATUS_ERROR;
  }

  //
  // Walk the entire IFR form and find a variable store opcode that
  // has a matching variable store ID.
  //
  for (Ptr = Context->Ifr; Ptr != NULL; Ptr = Ptr->Next) {
    if (Ptr->RawIfrHeader->OpCode == EFI_IFR_FORM_SET_OP) {
      if (VarId == 0) {
        *VarStoreGuid = &((EFI_IFR_FORM_SET *) (Ptr->RawIfrHeader))->Guid;
        *VarStoreName = DEFAULT_VARIABLE_NAME;
        return STATUS_SUCCESS;
      }
    } else if (Ptr->RawIfrHeader->OpCode == EFI_IFR_VARSTORE_OP) {
      //
      // See if it's a variable ID match
      //
      VarStore = (EFI_IFR_VARSTORE *) Ptr->RawIfrHeader;
      if (VarStore->VarId == VarId) {
        *VarStoreGuid = &VarStore->Guid;
        *VarStoreName = (char *) (VarStore + 1);
        return STATUS_SUCCESS;
      }
    }
  }

  return STATUS_ERROR;
}

STATUS
IfrSetDefaults (
  int MfgDefaults
  )
/*++

Routine Description:

  Go through all the IFR forms we've parsed so far and create and set variable
  defaults.
  
Arguments:

  MfgDefaults   - non-zero if manufacturing defaults are desired

Returns:

  STATUS_SUCCESS      - always

--*/
{
  IFR_PARSE_CONTEXT *Context;
  //
  // First free up any variable stores we've created so far.
  //
  FreeVarStores ();
  for (Context = mParseContext; Context != NULL; Context = Context->Next) {
    //
    // Call our internal function to handle it
    //
    SetDefaults (Context, MfgDefaults);
  }

  return STATUS_SUCCESS;

}

/******************************************************************************/
STATUS
IfrGetIfrPack (
  int               Handle,
  EFI_HII_IFR_PACK  **PackHeader,
  EFI_GUID          *FormsetGuid
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Handle      - GC_TODO: add argument description
  PackHeader  - GC_TODO: add argument description
  FormsetGuid - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  IFR_PARSE_CONTEXT *Context;

  for (Context = mParseContext; Context != NULL; Context = Context->Next) {
    if (Context->Handle == Handle) {
      *PackHeader = Context->PackHeader;
      memcpy (FormsetGuid, Context->FormsetGuid, sizeof (EFI_GUID));
      return STATUS_SUCCESS;
    }
  }

  return STATUS_ERROR;
}

STATUS
IfrReferencesVarPack (
  int                   IfrHandle,
  EFI_HII_VARIABLE_PACK *VarPack
  )
/*++

Routine Description:

  Given an HII handle number (which corrresponds to a handle number passed 
  in to IfrParsePack()), see if the IFR references the specified variable
  pack.
  
Arguments:

  IfrHandle   - handle number for the IFR pack to check (passed to IfrParsePack())
  VarPack     - variable pack to check to see if the IFR references

Returns:

  STATUS_SUCCESS      if the IFR on the given handle references the variable pack
  STATUS_WARNING      the IFR does not reference the variable pack
  STATUS_ERROR        invalid IFR handle
  
--*/
{
  IFR_PARSE_CONTEXT *Context;
  char              VarName[MAX_VARIABLE_NAME];
  IFR_PARSE_ENTRY   *ParseEntry;

  for (Context = mParseContext; Context != NULL; Context = Context->Next) {
    if (Context->Handle == IfrHandle) {
      //
      // Create an ASCII version of the variable name, since that's what is
      // referenced in IFR.
      //
      sprintf (VarName, "%S", (CHAR16 *) (VarPack + 1));
      //
      // Walk all the opcodes and see if the IFR references this variable pack
      //
      for (ParseEntry = Context->Ifr; ParseEntry != NULL; ParseEntry = ParseEntry->Next) {
        //
        // Check for Name.Guid match for primary IFR variable store
        //
        if ((strcmp (VarName, ParseEntry->VarStoreName1) == 0) &&
            (memcmp (&VarPack->VariableGuid, ParseEntry->VarStoreGuid1, sizeof (EFI_GUID)) == 0)
            ) {
          return STATUS_SUCCESS;
        }
        //
        // Check for Name.Guid match for secondary IFR variable store
        //
        if ((ParseEntry->VarStoreName2 != NULL) &&
            (strcmp (VarName, ParseEntry->VarStoreName2) == 0) &&
            (memcmp (&VarPack->VariableGuid, ParseEntry->VarStoreGuid2, sizeof (EFI_GUID)) == 0)
            ) {
          return STATUS_SUCCESS;
        }
      }

      return STATUS_WARNING;
    }
  }

  return STATUS_ERROR;
}

STATUS
IfrGetVarPack (
  int                     VarIndex,
  EFI_HII_VARIABLE_PACK   **VarPack
  )
/*++

Routine Description:

  Get the variable defaults. It is expected that the caller
  called IfrSetDefaults() previously to walk all the IFR forms we know about
  and create and initialize default values.
  
Arguments:

  VarIndex - a 0-based index into all the variable stores we know about
  VarPack  - outgoing pointer to a variable pack

Returns:

  STATUS_ERROR    - VarIndex exceeds the number of variable packs we know of
  STATUS_SUCCESS  - otherwise
  
--*/
{
  VARIABLE_STORE_ENTRY  *Entry;
  //
  // Initialize outgoing parameters
  //
  *VarPack = NULL;
  for (Entry = mVariableStores; Entry != NULL; Entry = Entry->Next) {
    if (VarIndex == 0) {
      *VarPack = Entry->VarPack;
      return STATUS_SUCCESS;
    }

    VarIndex--;
  }

  return STATUS_ERROR;
}

static
STATUS
SetVariableValue (
  EFI_GUID    *VarGuid,
  char        *VarName,
  int         VarOffset,
  int         VarSize,
  void        *VarValue
  )
/*++

Routine Description:

  Given a variable GUID.Name, offset, size, and value, set the bytes in
  the variable to the provided value.
  
Arguments:
  VarGuid           - GUID of variable to set
  VarName           - name of variable to set
  VarOffset         - byte offset into the variable store 
  VarSize           - size of the value in the variable store (in bytes)
  VarValue          - pointer to buffer containing the value to set

Returns:

  
--*/
{
  VARIABLE_STORE_ENTRY  *Entry;
  char                  *Src;
  char                  *Dest;
  //
  // Go through our list of variable stores to find the match
  //
  for (Entry = mVariableStores; Entry != NULL; Entry = Entry->Next) {
    if (memcmp (VarGuid, &Entry->VarPack->VariableGuid, sizeof (EFI_GUID)) == 0) {
      if (strcmp (VarName, Entry->VarName) == 0) {
        //
        // Found match -- check offset. If it's beyond the size of the variable store
        // buffer, then return a warning. Note that date-time can be beyond the
        // end of the varstore, which is ok.
        //
        if (VarOffset + VarSize <= Entry->VarBufferSize) {
          //
          // Stuff the data
          //
          Dest  = Entry->VarBuffer + VarOffset;
          Src   = (char *) VarValue;
          while (VarSize > 0) {
            *Dest = *Src;
            Src++;
            Dest++;
            VarSize--;
          }

          return STATUS_SUCCESS;
        }

        return STATUS_WARNING;
      }
    }
  }

  return STATUS_ERROR;
}

static
STATUS
SetDefaults (
  IFR_PARSE_CONTEXT *Context,
  UINT32            MfgDefaults
  )
/*++

Routine Description:

  Set variable defaults by walking a single IFR form.
  
Arguments:
  
  Context     - Pointer to the IFR context.
  MfgDefaults - Number of Mfg defaults

Returns:

  EFI_INVALID_PARAMETER - arguments to function are invalid
  STATUS_SUCCESS        - function executed successfully
  
--*/
{
  int                   Size;
  int                   CachedVarOffset;
  int                   CachedVarSize;
  int                   OrderedList;
  IFR_PARSE_ENTRY       *SavedParseEntry;
  EFI_IFR_CHECK_BOX     *IfrCheckBox;
  EFI_IFR_ONE_OF_OPTION *IfrOneOfOption;
  EFI_IFR_NUMERIC       *IfrNumeric;
  STATUS                Status;
  char                  ZeroByte;

  //
  // Walk the opcodes to set default values and stuff them into the variable stores
  //

  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status              = STATUS_SUCCESS;
  Context->CurrentIfr = Context->Ifr;
  SavedParseEntry     = NULL;
  OrderedList         = 0;
  CachedVarOffset     = 0;
  CachedVarSize       = 0;
  ZeroByte            = 0;

  while (Context->CurrentIfr != NULL) {
    if (Context->CurrentIfr->RawIfrHeader->OpCode == EFI_IFR_FORM_SET_OP) {
      //
      // Formset opcode -- create a variable pack
      //
      Status = CreateVarStore (
                &((EFI_IFR_FORM_SET *) (Context->CurrentIfr->RawIfrHeader))->Guid,
                DEFAULT_VARIABLE_NAME,
                ((EFI_IFR_FORM_SET *) (Context->CurrentIfr->RawIfrHeader))->NvDataSize
                );
    } else if (Context->CurrentIfr->RawIfrHeader->OpCode == EFI_IFR_VARSTORE_OP) {
      //
      // Variable store opcode -- create a variable pack
      //
      Status = CreateVarStore (
                &((EFI_IFR_VARSTORE *) (Context->CurrentIfr->RawIfrHeader))->Guid,
                (char *) Context->CurrentIfr->RawIfrHeader + sizeof (EFI_IFR_VARSTORE),
                ((EFI_IFR_VARSTORE *) (Context->CurrentIfr->RawIfrHeader))->Size
                );
    } else if (Context->CurrentIfr->RawIfrHeader->OpCode == EFI_IFR_ONE_OF_OP) {
      //
      // Need this parse context later when we find the default ONE_OF_OPTION.
      // Clear out the variable store first, so that we're covered if someone
      // has two one-of opcode that operate on the same data.
      // So "last one wins" is the behavior.
      //
      OrderedList     = 0;
      SavedParseEntry = Context->CurrentIfr;
      CachedVarOffset = ((EFI_IFR_ONE_OF *) Context->CurrentIfr->RawIfrHeader)->QuestionId;
      CachedVarSize   = ((EFI_IFR_ONE_OF *) Context->CurrentIfr->RawIfrHeader)->Width;
    } else if (Context->CurrentIfr->RawIfrHeader->OpCode == EFI_IFR_ORDERED_LIST_OP) {
      //
      // Need this parse context later as we parse the ONE_OF_OP's in the ordered list
      //
      OrderedList     = 1;
      SavedParseEntry = Context->CurrentIfr;
      CachedVarOffset = ((EFI_IFR_ORDERED_LIST *) Context->CurrentIfr->RawIfrHeader)->QuestionId;
      CachedVarSize   = ((EFI_IFR_ORDERED_LIST *) Context->CurrentIfr->RawIfrHeader)->MaxEntries;

      while (CachedVarSize > 0) {
        Status = SetVariableValue (
                  SavedParseEntry->VarStoreGuid1, // GUID of variable store to write
                  SavedParseEntry->VarStoreName1, // name of variable store to write
                  CachedVarOffset,                // offset into variable store
                  1,                              // variable data size
                  (void *) &ZeroByte
                  );
        //
        // variable value
        //
        CachedVarSize--;
        CachedVarOffset++;
      }

      CachedVarOffset = ((EFI_IFR_ORDERED_LIST *) Context->CurrentIfr->RawIfrHeader)->QuestionId;
      CachedVarSize   = 1;
      //
      // ((EFI_IFR_ORDERED_LIST *)Context->CurrentIfr->RawIfrHeader)->Width;
      //
    } else if (Context->CurrentIfr->RawIfrHeader->OpCode == EFI_IFR_ONE_OF_OPTION_OP) {
      IfrOneOfOption = (EFI_IFR_ONE_OF_OPTION *) Context->CurrentIfr->RawIfrHeader;
      //
      // If we're in an ordered list, then copy the value to the data store
      //
      if (OrderedList) {
        Status = SetVariableValue (
                  SavedParseEntry->VarStoreGuid1, // GUID of variable store to write
                  SavedParseEntry->VarStoreName1, // name of variable store to write
                  CachedVarOffset,                // offset into variable store
                  1,                              // variable data size
                  (void *) &IfrOneOfOption->Value
                  );
        //
        // variable value
        //
        // Advance the offset for the next ordered list item
        //
        CachedVarOffset += CachedVarSize;
      } else {
        //
        // ONE-OF list. See if the default flag is set (provided we're not doing mfg defaults)
        //
        if (!MfgDefaults) {
          if (IfrOneOfOption->Flags & EFI_IFR_FLAG_DEFAULT) {
            Status = SetVariableValue (
                      SavedParseEntry->VarStoreGuid1, // GUID of variable store to write
                      SavedParseEntry->VarStoreName1, // name of variable store to write
                      CachedVarOffset,                // offset into variable store
                      CachedVarSize,                  // variable data size
                      &IfrOneOfOption->Value
                      );
            //
            // variable value
            //
          }
        } else {
          if (IfrOneOfOption->Flags & EFI_IFR_FLAG_MANUFACTURING) {
            Status = SetVariableValue (
                      SavedParseEntry->VarStoreGuid1, // GUID of variable store to write
                      SavedParseEntry->VarStoreName1, // name of variable store to write
                      CachedVarOffset,                // offset into variable store
                      CachedVarSize,                  // variable data size
                      &IfrOneOfOption->Value
                      );
            //
            // variable value
            //
          }
        }
      }
    } else if (Context->CurrentIfr->RawIfrHeader->OpCode == EFI_IFR_CHECKBOX_OP) {
      //
      // If we're saving defaults, and the default flag is set, or we're saving
      // manufacturing defaults and the manufacturing flag is set, then save a 1.
      // By default the varstore buffer is cleared, so we don't need to save a 0 ever.
      //
      IfrCheckBox = (EFI_IFR_CHECK_BOX *) Context->CurrentIfr->RawIfrHeader;
      if (((MfgDefaults == 0) && (IfrCheckBox->Flags & EFI_IFR_FLAG_DEFAULT)) ||
          ((MfgDefaults != 0) && (IfrCheckBox->Flags & EFI_IFR_FLAG_MANUFACTURING))
          ) {
        Size = 1;
        Status = SetVariableValue (
                  Context->CurrentIfr->VarStoreGuid1, // GUID of variable store to write
                  Context->CurrentIfr->VarStoreName1, // name of variable store to write
                  IfrCheckBox->QuestionId,            // offset into variable store
                  IfrCheckBox->Width,                 // variable data size
                  (void *) &Size
                  );
        //
        // variable value
        //
      }
    } else if (Context->CurrentIfr->RawIfrHeader->OpCode == EFI_IFR_NUMERIC_OP) {
      IfrNumeric = (EFI_IFR_NUMERIC *) Context->CurrentIfr->RawIfrHeader;
      Status = SetVariableValue (
                Context->CurrentIfr->VarStoreGuid1, // GUID of variable store to write
                Context->CurrentIfr->VarStoreName1, // name of variable store to write
                IfrNumeric->QuestionId,             // offset into variable store
                IfrNumeric->Width,                  // variable data size
                (void *) &IfrNumeric->Default
                );
      //
      // variable value
      //
    }

    Context->CurrentIfr = Context->CurrentIfr->Next;
  }

  return STATUS_SUCCESS;
}

static
STATUS
CreateVarStore (
  EFI_GUID  *VarGuid,
  CHAR8     *VarName,
  int       VarStoreSize
  )
/*++

Routine Description:

  Given a variable GUID.Name and the size of the variable store, allocate
  storage for maintaining the variable value.
  
Arguments:

  VarGuid - GUID for a variable
  VarName - Name of the variable
  VarStoreSize - size of the variable store

Returns:

  STATUS_ERROR   - problem with storage allocation 
  STATUS_SUCCESS - function executed successfully
 
--*/
{
  VARIABLE_STORE_ENTRY  *Entry;

  VARIABLE_STORE_ENTRY  *TempEntry;
  int                   PackSize;
  int                   VarNameLen;
  //
  // If the variable store size is zero, then do nothing. This could be valid
  // if variable steering is used in the IFR such that FormsetGUID."Setup" variable
  // store is never used.
  //
  // OPEN: What about a form that only has a time/date question? Then if some other
  // function called SetDefaults(), attempting to set time/date would result in an
  // error in the SetVarValue() function.
  //
   if (VarStoreSize == 0) {
    return STATUS_SUCCESS;
   }
  //
  // Go through our list of variable stores and see if we've already created one
  // for this Guid.Name. If so, check the size and return. Otherwise create
  // one and add it to the list.
  //
  for (Entry = mVariableStores; Entry != NULL; Entry = Entry->Next) {
    if (memcmp (VarGuid, &Entry->VarPack->VariableGuid, sizeof (EFI_GUID)) == 0) {
      if (strcmp (VarName, Entry->VarName) == 0) {
        //
        // Already have one. Check size.
        //
        if (Entry->VarBufferSize != VarStoreSize) {
          Error (NULL, 0, 0, "mismatched variable store size between two formsets", VarName);
          return STATUS_ERROR;
        }

        return STATUS_SUCCESS;
      }
    }
  }
  //
  // Create a new one.
  //
  Entry = (VARIABLE_STORE_ENTRY *) malloc (sizeof (VARIABLE_STORE_ENTRY));
  if (Entry == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset ((void *) Entry, 0, sizeof (VARIABLE_STORE_ENTRY));
  //
  // Compute size of the varpack
  //
  VarNameLen      = strlen (VarName) + 1;
  PackSize        = sizeof (EFI_HII_VARIABLE_PACK) + VarNameLen * sizeof (CHAR16) + VarStoreSize;
  Entry->VarPack  = (EFI_HII_VARIABLE_PACK *) malloc (PackSize);
  if (Entry->VarPack == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    free (Entry);
    return STATUS_ERROR;
  }

  Entry->VarPack->Header.Length       = PackSize;
  Entry->VarPack->Header.Type         = EFI_HII_VARIABLE;
  Entry->VarPack->VariableNameLength  = VarNameLen * sizeof (CHAR16);
  Entry->VarName[MAX_VARIABLE_NAME - 1] = 0;
  strncpy (Entry->VarName, VarName, MAX_VARIABLE_NAME - 1);
#ifdef USE_VC8
  swprintf ((CHAR16 *) (Entry->VarPack + 1), (strlen (VarName) + 1) * sizeof (CHAR16), L"%S", VarName);
#else
  swprintf ((CHAR16 *) (Entry->VarPack + 1), L"%S", VarName);
#endif
  memcpy (&Entry->VarPack->VariableGuid, VarGuid, sizeof (EFI_GUID));
  //
  // Point VarBuffer into the allocated buffer (for convenience)
  //
  Entry->VarBuffer = (char *) Entry->VarPack + sizeof (EFI_HII_VARIABLE_PACK) + VarNameLen * sizeof (CHAR16);
  memset ((void *) Entry->VarBuffer, 0, VarStoreSize);
  Entry->VarBufferSize = VarStoreSize;
  //
  // Add this new varstore to our list
  //
  if (mVariableStores == NULL) {
    mVariableStores = Entry;
  } else {
    for (TempEntry = mVariableStores; TempEntry->Next != NULL; TempEntry = TempEntry->Next)
      ;
    TempEntry->Next = Entry;
  }
  return STATUS_SUCCESS;
}

/******************************************************************************/

/*++

Routine Description:

  The following IfrParseXX() functions are used to parse an IFR opcode numbered
  XX via a dispatch table. 
  
Arguments:

  Context - IFR parsing context into which pertinent data for the 
            current opcode can be saved. Context->LastIfr->RawIfrHeader points to 
            the raw IFR bytes currently being parsed.

Returns:

  STATUS_SUCCESS  - always
  
--*/

/*******************************************************************************/
static
STATUS
IfrParse01 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse02 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse03 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}
//
// Parse the IFR EFI_IFR_ONE_OF opcode.
//
static
STATUS
IfrParse05 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse06 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse07 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse08 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse09 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse0A (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse0B (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse0C (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse0D (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse0E (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  EFI_IFR_FORM_SET  *Op;
  Op  = (EFI_IFR_FORM_SET *) Context->LastIfr->RawIfrHeader;
  Context->LastIfr->VarStoreGuid1 = &Op->Guid;
  Context->LastIfr->VarStoreName1 = "Setup";
  Context->FormsetGuid            = &Op->Guid;
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse0F (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse10 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse11 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse12 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse13 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse14 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse15 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse16 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse17 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse18 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse19 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse1A (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse1B (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse1C (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse1D (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse1E (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse1F (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse20 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse21 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse22 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

static
STATUS
IfrParse23 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}
//
// EFI_IFR_VARSTORE
//
static
STATUS
IfrParse24 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  EFI_IFR_VARSTORE  *Op;
  Op = (EFI_IFR_VARSTORE *) Context->LastIfr->RawIfrHeader;
  return STATUS_SUCCESS;
}
//
// VARSTORE_SELECT
//
static
STATUS
IfrParse25 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  STATUS                  Status;
  EFI_IFR_VARSTORE_SELECT *Op;
  Op      = (EFI_IFR_VARSTORE_SELECT *) Context->LastIfr->RawIfrHeader;
  Status  = GetVarStoreInfo (Context, Op->VarId, &Context->LastIfr->VarStoreGuid1, &Context->LastIfr->VarStoreName1);
  //
  // VARSTORE_SELECT sets both
  //
  Context->LastIfr->VarStoreGuid2 = Context->LastIfr->VarStoreGuid1;
  Context->LastIfr->VarStoreName2 = Context->LastIfr->VarStoreName1;
  return Status;
}
//
// VARSTORE_SELECT_PAIR
//
static
STATUS
IfrParse26 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  STATUS                        Status;
  EFI_IFR_VARSTORE_SELECT_PAIR  *Op;

  Op      = (EFI_IFR_VARSTORE_SELECT_PAIR *) Context->LastIfr->RawIfrHeader;
  Status  = GetVarStoreInfo (Context, Op->VarId, &Context->LastIfr->VarStoreGuid1, &Context->LastIfr->VarStoreName1);
  Status = GetVarStoreInfo (
            Context,
            Op->SecondaryVarId,
            &Context->LastIfr->VarStoreGuid2,
            &Context->LastIfr->VarStoreName2
            );
  return Status;
}
//
// TRUE
//
static
STATUS
IfrParse27 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}
//
// FALSe
//
static
STATUS
IfrParse28 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}
static
STATUS
IfrParse29 (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}
static
STATUS
IfrParse2A (
  IFR_PARSE_CONTEXT *Context
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Context - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  return STATUS_SUCCESS;
}

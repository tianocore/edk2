/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LinkedList.h

Abstract:

  This implementation of a linked list provides data structures for the
  list itself and for list nodes.  It provides operations for initializing
  the list, modifying the list, and walking the list.  
  
--*/

//
// Prevent multiple includes in the same source file
//
#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#ifndef _SHELL_LINKED_LIST_H_

typedef struct _EFI_LIST_ENTRY {
  struct _EFI_LIST_ENTRY  *ForwardLink;
  struct _EFI_LIST_ENTRY  *BackLink;
} EFI_LIST_ENTRY;

typedef EFI_LIST_ENTRY EFI_LIST;      
typedef EFI_LIST_ENTRY EFI_LIST_NODE;

#define INITIALIZE_LIST_HEAD_VARIABLE(ListHead)  {&ListHead, &ListHead}

//
//  EFI_FIELD_OFFSET - returns the byte offset to a field within a structure
//
   
#define EFI_FIELD_OFFSET(TYPE,Field) ((UINTN)(&(((TYPE *) 0)->Field)))

//
// A lock structure
//

typedef struct {
    EFI_TPL     Tpl;
    EFI_TPL     OwnerTpl;
    UINTN       Lock;
} FLOCK;

VOID
InitializeListHead (
  EFI_LIST_ENTRY       *List
  )
/*++

Routine Description:

  Initialize the head of the List.  The caller must allocate the memory 
  for the EFI_LIST. This function must be called before the other linked
  list macros can be used.
    
Arguments:

  List - Pointer to list head to initialize
   
Returns:

  None.

--*/
;

BOOLEAN
IsListEmpty (
  EFI_LIST_ENTRY  *List
  )
/*++

Routine Description:

  Return TRUE is the list contains zero nodes. Otherzise return FALSE.
  The list must have been initialized with InitializeListHead () before using 
  this function.
    
Arguments:

  List - Pointer to list head to test

   
Returns:

  Return TRUE is the list contains zero nodes. Otherzise return FALSE.

--*/
;

VOID
RemoveEntryList (
  EFI_LIST_ENTRY  *Entry
  )
/*++

Routine Description:

  Remove Node from the doubly linked list. It is the caller's responsibility
  to free any memory used by the entry if needed. The list must have been 
  initialized with InitializeListHead () before using this function.
    
Arguments:

  Entry - Element to remove from the list.
   
Returns:
  
  None

--*/
;

VOID
InsertTailList (
  EFI_LIST_ENTRY  *ListHead,
  EFI_LIST_ENTRY  *Entry
  )
/*++

Routine Description:

  Insert a Node into the end of a doubly linked list. The list must have 
  been initialized with InitializeListHead () before using this function.
    
Arguments:

  ListHead - Head of doubly linked list

  Entry    - Element to insert at the end of the list.
   
Returns:
  
  None

--*/
;

VOID
InsertHeadList (
  EFI_LIST_ENTRY  *ListHead,
  EFI_LIST_ENTRY  *Entry
  )
/*++

Routine Description:

  Insert a Node into the start of a doubly linked list. The list must have 
  been initialized with InitializeListHead () before using this function.
    
Arguments:

  ListHead - Head of doubly linked list

  Entry    - Element to insert to beginning of list
   
Returns:
  
  None

--*/
;

VOID
SwapListEntries (
  EFI_LIST_ENTRY  *Entry1,
  EFI_LIST_ENTRY  *Entry2
  )
/*++

Routine Description:

  Swap the location of the two elements of a doubly linked list. Node2 
  is placed in front of Node1. The list must have been initialized with 
  InitializeListHead () before using this function.
    
Arguments:

  Entry1 - Element in the doubly linked list in front of Node2. 

  Entry2 - Element in the doubly linked list behind Node1.
   
Returns:
  
  None

--*/
;

EFI_LIST_ENTRY *
GetFirstNode (
  EFI_LIST_ENTRY  *List 
  )
/*++

Routine Description:

  Return the first node pointed to by the list head.  The list must 
  have been initialized with InitializeListHead () before using this 
  function and must contain data.
    
Arguments:

  List - The head of the doubly linked list.
   
Returns:
  
  Pointer to the first node, if the list contains nodes.  The list will
  return a null value--that is, the value of List--when the list is empty.
    See the description of IsNull for more information.


--*/
;

EFI_LIST_ENTRY *
GetNextNode (
  EFI_LIST_ENTRY  *List,
  EFI_LIST_ENTRY  *Node
  )
/*++

Routine Description:

  Returns the node following Node in the list. The list must 
  have been initialized with InitializeListHead () before using this 
  function and must contain data.
    
Arguments:

  List - The head of the list.  MUST NOT be the literal value NULL.
  Node - The node in the list.  This value MUST NOT be the literal value NULL.
    See the description of IsNull for more information.
   
Returns:
  
  Pointer to the next node, if one exists.  Otherwise, returns a null value,
  which is actually a pointer to List.
    See the description of IsNull for more information.

--*/
;

BOOLEAN 
IsNull ( 
  EFI_LIST_ENTRY  *List,
  EFI_LIST_ENTRY  *Node 
  )
/*++

Routine Description:

  Determines whether the given node is null.  Note that Node is null
  when its value is equal to the value of List.  It is an error for
  Node to be the literal value NULL [(VOID*)0x0].

Arguments:

  List - The head of the list.  MUST NOT be the literal value NULL.
  Node - The node to test.  MUST NOT be the literal value NULL.  See
    the description above.
   
Returns:
  
  Returns true if the node is null.

--*/
;

BOOLEAN 
IsNodeAtEnd ( 
  EFI_LIST_ENTRY  *List, 
  EFI_LIST_ENTRY  *Node 
  )
/*++

Routine Description:

  Determines whether the given node is at the end of the list.  Used
  to walk the list.  The list must have been initialized with 
  InitializeListHead () before using this function and must contain 
  data.

Arguments:

  List - The head of the list.  MUST NOT be the literal value NULL.
  Node - The node to test.  MUST NOT be the literal value NULL.
    See the description of IsNull for more information.
   
Returns:
  
  Returns true if the list is the tail.

--*/
;

#endif
#endif

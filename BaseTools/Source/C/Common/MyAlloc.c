/** @file
File for memory allocation tracking functions.

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "MyAlloc.h"

#if USE_MYALLOC
//
// Get back to original alloc/free calls.
//
#undef malloc
#undef calloc
#undef realloc
#undef free
//
// Start of allocation list.
//
STATIC MY_ALLOC_STRUCT  *MyAllocData = NULL;

//
//
//
STATIC UINT32           MyAllocHeadMagik  = MYALLOC_HEAD_MAGIK;
STATIC UINT32           MyAllocTailMagik  = MYALLOC_TAIL_MAGIK;

//
// ////////////////////////////////////////////////////////////////////////////
//
//
VOID
MyCheck (
  BOOLEAN      Final,
  UINT8        File[],
  UINTN        Line
  )
// *++
// Description:
//
//  Check for corruptions in the allocated memory chain.  If a corruption
//  is detection program operation stops w/ an exit(1) call.
//
// Parameters:
//
//  Final := When FALSE, MyCheck() returns if the allocated memory chain
//           has not been corrupted.  When TRUE, MyCheck() returns if there
//           are no un-freed allocations.  If there are un-freed allocations,
//           they are displayed and exit(1) is called.
//
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  n/a
//
// --*/
//
{
  MY_ALLOC_STRUCT *Tmp;

  //
  // Check parameters.
  //
  if (File == NULL || Line == 0) {
    printf (
      "\nMyCheck(Final=%u, File=%s, Line=%u)"
      "Invalid parameter(s).\n",
      Final,
      File,
      (unsigned)Line
      );

    exit (1);
  }

  if (strlen ((CHAR8 *)File) == 0) {
    printf (
      "\nMyCheck(Final=%u, File=%s, Line=%u)"
      "Invalid parameter.\n",
      Final,
      File,
      (unsigned)Line
      );

    exit (1);
  }
  //
  // Check structure contents.
  //
  for (Tmp = MyAllocData; Tmp != NULL; Tmp = Tmp->Next) {
    if (memcmp(Tmp->Buffer, &MyAllocHeadMagik, sizeof MyAllocHeadMagik) ||
        memcmp(&Tmp->Buffer[Tmp->Size + sizeof(UINT32)], &MyAllocTailMagik, sizeof MyAllocTailMagik)) {
      break;
    }
  }
  //
  // If Tmp is not NULL, the structure is corrupt.
  //
  if (Tmp != NULL) {
    printf (
      "\nMyCheck(Final=%u, File=%s, Line=%u)""\nStructure corrupted!"
      "\nFile=%s, Line=%u, nSize=%u, Head=%xh, Tail=%xh\n",
      Final,
      File,
      (unsigned)Line,
      Tmp->File,
      (unsigned) Tmp->Line,
      (unsigned) Tmp->Size,
      (unsigned) *(UINT32 *) (Tmp->Buffer),
      (unsigned) *(UINT32 *) (&Tmp->Buffer[Tmp->Size + sizeof (UINT32)])
      );

    exit (1);
  }
  //
  // If Final is TRUE, display the state of the structure chain.
  //
  if (Final) {
    if (MyAllocData != NULL) {
      printf (
        "\nMyCheck(Final=%u, File=%s, Line=%u)"
        "\nSome allocated items have not been freed.\n",
        Final,
        File,
        (unsigned)Line
        );

      for (Tmp = MyAllocData; Tmp != NULL; Tmp = Tmp->Next) {
        printf (
          "File=%s, Line=%u, nSize=%u, Head=%xh, Tail=%xh\n",
          Tmp->File,
          (unsigned) Tmp->Line,
          (unsigned) Tmp->Size,
          (unsigned) *(UINT32 *) (Tmp->Buffer),
          (unsigned) *(UINT32 *) (&Tmp->Buffer[Tmp->Size + sizeof (UINT32)])
          );
      }
    }
  }
}
//
// ////////////////////////////////////////////////////////////////////////////
//
//
VOID *
MyAlloc (
  UINTN      Size,
  UINT8 File[],
  UINTN      Line
  )
// *++
// Description:
//
//  Allocate a new link in the allocation chain along with enough storage
//  for the File[] string, requested Size and alignment overhead.  If
//  memory cannot be allocated or the allocation chain has been corrupted,
//  exit(1) will be called.
//
// Parameters:
//
//  Size := Number of bytes (UINT8) requested by the called.
//          Size cannot be zero.
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  Pointer to the caller's buffer.
//
// --*/
//
{
  MY_ALLOC_STRUCT *Tmp;
  UINTN           Len;

  //
  // Check for invalid parameters.
  //
  if (Size == 0 || File == NULL || Line == 0) {
    printf (
      "\nMyAlloc(Size=%u, File=%s, Line=%u)"
      "\nInvalid parameter(s).\n",
      (unsigned)Size,
      File,
      (unsigned)Line
      );

    exit (1);
  }

  Len = strlen ((CHAR8 *)File);
  if (Len == 0) {
    printf (
      "\nMyAlloc(Size=%u, File=%s, Line=%u)"
      "\nInvalid parameter.\n",
      (unsigned)Size,
      File,
      (unsigned)Line
      );

    exit (1);
  }
  //
  // Check the allocation list for corruption.
  //
  MyCheck (0, (UINT8 *)__FILE__, __LINE__);

  //
  // Allocate a new entry.
  //
  Tmp = calloc (
          1,
          sizeof (MY_ALLOC_STRUCT) + Len + 1 + sizeof (UINT64) + Size + (sizeof MyAllocHeadMagik) + (sizeof MyAllocTailMagik)
          );

  if (Tmp == NULL) {
    printf (
      "\nMyAlloc(Size=%u, File=%s, Line=%u)"
      "\nOut of memory.\n",
      (unsigned)Size,
      File,
      (unsigned)Line
      );

    exit (1);
  }
  //
  // Fill in the new entry.
  //
  Tmp->File = ((UINT8 *) Tmp) + sizeof (MY_ALLOC_STRUCT);
  strcpy ((CHAR8 *)Tmp->File, (CHAR8 *)File);
  Tmp->Line   = Line;
  Tmp->Size   = Size;
  Tmp->Buffer = (UINT8 *) (((UINTN) Tmp + Len + 9) &~7);

  memcpy (Tmp->Buffer, &MyAllocHeadMagik, sizeof MyAllocHeadMagik);

  memcpy (
    &Tmp->Buffer[Size + sizeof (UINT32)],
    &MyAllocTailMagik,
    sizeof MyAllocTailMagik
    );

  Tmp->Next   = MyAllocData;
  Tmp->Cksum  = (UINTN) Tmp + (UINTN) (Tmp->Next) + Tmp->Line + Tmp->Size + (UINTN) (Tmp->File) + (UINTN) (Tmp->Buffer);

  MyAllocData = Tmp;

  return Tmp->Buffer + sizeof (UINT32);
}
//
// ////////////////////////////////////////////////////////////////////////////
//
//
VOID *
MyRealloc (
  VOID       *Ptr,
  UINTN      Size,
  UINT8 File[],
  UINTN      Line
  )
// *++
// Description:
//
//  This does a MyAlloc(), memcpy() and MyFree().  There is no optimization
//  for shrinking or expanding buffers.  An invalid parameter will cause
//  MyRealloc() to fail with a call to exit(1).
//
// Parameters:
//
//  Ptr := Pointer to the caller's buffer to be re-allocated.
//
//  Size := Size of new buffer.  Size cannot be zero.
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  Pointer to new caller's buffer.
//
// --*/
//
{
  MY_ALLOC_STRUCT *Tmp;
  VOID            *Buffer;

  //
  // Check for invalid parameter(s).
  //
  if (Size == 0 || File == NULL || Line == 0) {
    printf (
      "\nMyRealloc(Ptr=%p, Size=%u, File=%s, Line=%u)"
      "\nInvalid parameter(s).\n",
      Ptr,
      (unsigned)Size,
      File,
      (unsigned)Line
      );

    exit (1);
  }

  if (strlen ((CHAR8 *)File) == 0) {
    printf (
      "\nMyRealloc(Ptr=%p, Size=%u, File=%s, Line=%u)"
      "\nInvalid parameter.\n",
      Ptr,
      (unsigned)Size,
      File,
      (unsigned)Line
      );

    exit (1);
  }
  //
  // Find existing buffer in allocation list.
  //
  if (Ptr == NULL) {
    Tmp = NULL;
  } else if (&MyAllocData->Buffer[sizeof (UINT32)] == Ptr) {
    Tmp = MyAllocData;
  } else {
    for (Tmp = MyAllocData;; Tmp = Tmp->Next) {
      if (Tmp->Next == NULL) {
        printf (
          "\nMyRealloc(Ptr=%p, Size=%u, File=%s, Line=%u)"
          "\nCould not find buffer.\n",
          Ptr,
          (unsigned)Size,
          File,
          (unsigned)Line
          );

        exit (1);
      }

      Tmp = Tmp->Next;
    }
  }
  //
  // Allocate new buffer, copy old data, free old buffer.
  //
  Buffer = MyAlloc (Size, File, Line);

  if (Buffer != NULL && Tmp != NULL) {
    memcpy (
      Buffer,
      &Tmp->Buffer[sizeof (UINT32)],
      ((Size <= Tmp->Size) ? Size : Tmp->Size)
      );

    MyFree (Ptr, (UINT8 *)__FILE__, __LINE__);
  }

  return Buffer;
}
//
// ////////////////////////////////////////////////////////////////////////////
//
//
VOID
MyFree (
  VOID       *Ptr,
  UINT8 File[],
  UINTN      Line
  )
// *++
// Description:
//
//  Release a previously allocated buffer.  Invalid parameters will cause
//  MyFree() to fail with an exit(1) call.
//
// Parameters:
//
//  Ptr := Pointer to the caller's buffer to be freed.
//         A NULL pointer will be ignored.
//
//  File := Set to __FILE__ by macro expansion.
//
//  Line := Set to __LINE__ by macro expansion.
//
// Returns:
//
//  n/a
//
// --*/
//
{
  MY_ALLOC_STRUCT *Tmp;
  MY_ALLOC_STRUCT *Tmp2;

  //
  // Check for invalid parameter(s).
  //
  if (File == NULL || Line == 0) {
    printf (
      "\nMyFree(Ptr=%p, File=%s, Line=%u)"
      "\nInvalid parameter(s).\n",
      Ptr,
      File,
      (unsigned)Line
      );

    exit (1);
  }

  if (strlen ((CHAR8 *)File) == 0) {
    printf (
      "\nMyFree(Ptr=%p, File=%s, Line=%u)"
      "\nInvalid parameter.\n",
      Ptr,
      File,
      (unsigned)Line
      );

    exit (1);
  }
  //
  // Freeing NULL is always valid.
  //
  if (Ptr == NULL) {
    return ;
  }
  //
  // Fail if nothing is allocated.
  //
  if (MyAllocData == NULL) {
    printf (
      "\nMyFree(Ptr=%p, File=%s, Line=%u)"
      "\nCalled before memory allocated.\n",
      Ptr,
      File,
      (unsigned)Line
      );

    exit (1);
  }
  //
  // Check for corrupted allocation list.
  //
  MyCheck (0, (UINT8 *)__FILE__, __LINE__);

  //
  // Need special check for first item in list.
  //
  if (&MyAllocData->Buffer[sizeof (UINT32)] == Ptr) {
    //
    // Unlink first item in list.
    //
    Tmp         = MyAllocData;
    MyAllocData = MyAllocData->Next;
  } else {
    //
    // Walk list looking for matching item.
    //
    for (Tmp = MyAllocData;; Tmp = Tmp->Next) {
      //
      // Fail if end of list is reached.
      //
      if (Tmp->Next == NULL) {
        printf (
          "\nMyFree(Ptr=%p, File=%s, Line=%u)\n"
          "\nNot found.\n",
          Ptr,
          File,
          (unsigned)Line
          );

        exit (1);
      }
      //
      // Leave loop when match is found.
      //
      if (&Tmp->Next->Buffer[sizeof (UINT32)] == Ptr) {
        break;
      }
    }
    //
    // Unlink item from list.
    //
    Tmp2      = Tmp->Next;
    Tmp->Next = Tmp->Next->Next;
    Tmp       = Tmp2;
  }
  //
  // Release item.
  //
  free (Tmp);
}

#endif /* USE_MYALLOC */

/* eof - MyAlloc.c */

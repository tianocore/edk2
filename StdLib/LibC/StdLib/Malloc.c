/** @file
  Definitions for memory allocation routines: calloc, malloc, realloc, free.

  The order and contiguity of storage allocated by successive calls to the
  calloc, malloc, and realloc functions is unspecified.  The pointer returned
  if the allocation succeeds is suitably aligned so that it may be assigned to
  a pointer of any type of object and then used to access such an object or an
  array of such objects in the space allocated (until the space is explicitly
  freed or reallocated).  Each such allocation shall yield a pointer to an
  object disjoint from any other object.  The pointer returned points to the
  start (lowest byte address) of the allocated space.  If the space can not be
  allocated, a null pointer is returned.  If the size of the space requested
  is zero, the behavior is implementation-defined; the value returned shall be
  either a null pointer or a unique pointer.  The value of a pointer that
  refers to freed space is indeterminate.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 */
#include  <Base.h>
#include  <Uefi.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/BaseMemoryLib.h>
#include  <Library/DebugLib.h>

#include  <LibConfig.h>

#include  <assert.h>
#include  <stdlib.h>
#include  <errno.h>

/** The UEFI functions do not provide a way to determine the size of an
    allocated region of memory given just a pointer to the start of that
    region.  Since this is required for the implementation of realloc,
    the memory head structure from Core/Dxe/Mem/Pool.c has been reproduced
    here.

    NOTE: If the UEFI implementation is changed, the realloc function may cease
          to function properly.
**/
#define POOL_HEAD_SIGNATURE   SIGNATURE_32('p','h','d','0')
typedef struct {
  UINT32          Signature;
  UINT32          Size;
  EFI_MEMORY_TYPE Type;
  UINTN           Reserved;
  CHAR8           Data[1];
} POOL_HEAD;

/****************************/

/** The malloc function allocates space for an object whose size is specified
    by size and whose value is indeterminate.

    This implementation uses the UEFI memory allocation boot services to get a
    region of memory that is 8-byte aligned and of the specified size.  The
    region is allocated with type EfiLoaderData.

    @param  size    Size, in bytes, of the region to allocate.

    @return   NULL is returned if the space could not be allocated and errno
              contains the cause.  Otherwise, a pointer to an 8-byte aligned
              region of the requested size is returned.<BR>
              If NULL is returned, errno may contain:
              - EINVAL: Requested Size is zero.
              - ENOMEM: Memory could not be allocated.
**/
void *
malloc(size_t Size)
{
  void       *RetVal;
  EFI_STATUS  Status;

  if( Size == 0) {
    errno = EINVAL;   // Make errno diffenent, just in case of a lingering ENOMEM.
    return NULL;
  }

  Status = gBS->AllocatePool( EfiLoaderData, (UINTN)Size, &RetVal);
  if( Status != EFI_SUCCESS) {
    RetVal  = NULL;
    errno   = ENOMEM;
  }
  return RetVal;
}

/** The calloc function allocates space for an array of Num objects, each of
    whose size is Size.  The space is initialized to all bits zero.

    This implementation uses the UEFI memory allocation boot services to get a
    region of memory that is 8-byte aligned and of the specified size.  The
    region is allocated with type EfiLoaderData.

    @param  Num     Number of objects to allocate.
    @param  Size    Size, in bytes, of the objects to allocate space for.

    @return   NULL is returned if the space could not be allocated and errno
              contains the cause.  Otherwise, a pointer to an 8-byte aligned
              region of the requested size is returned.
**/
void *
calloc(size_t Num, size_t Size)
{
  void       *RetVal;
  size_t      NumSize;

  NumSize = Num * Size;
  if (NumSize == 0) {
      return NULL;
  }
  RetVal = malloc(NumSize);
  if( RetVal != NULL) {
    (VOID)ZeroMem( RetVal, NumSize);
  }
  return RetVal;
}

/** The free function causes the space pointed to by Ptr to be deallocated,
    that is, made available for further allocation.

    If Ptr is a null pointer, no action occurs.  Otherwise, if the argument
    does not match a pointer earlier returned by the calloc, malloc, or realloc
    function, or if the space has been deallocated by a call to free or
    realloc, the behavior is undefined.

    @param  Ptr     Pointer to a previously allocated region of memory to be freed.

**/
void
free(void *Ptr)
{
  if(Ptr != NULL) {
    (void) gBS->FreePool (Ptr);
  }
}

/** The realloc function changes the size of the object pointed to by Ptr to
    the size specified by NewSize.

    The contents of the object are unchanged up to the lesser of the new and
    old sizes.  If the new size is larger, the value of the newly allocated
    portion of the object is indeterminate.

    If Ptr is a null pointer, the realloc function behaves like the malloc
    function for the specified size.

    If Ptr does not match a pointer earlier returned by the calloc, malloc, or
    realloc function, or if the space has been deallocated by a call to the free
    or realloc function, the behavior is undefined.

    If the space cannot be allocated, the object pointed to by Ptr is unchanged.

    If NewSize is zero and Ptr is not a null pointer, the object it points to
    is freed.

    This implementation uses the UEFI memory allocation boot services to get a
    region of memory that is 8-byte aligned and of the specified size.  The
    region is allocated with type EfiLoaderData.

    The following combinations of Ptr and NewSize can occur:<BR>
      Ptr     NewSize<BR>
    --------  -------------------<BR>
    - NULL        0                 Returns NULL;
    - NULL      > 0                 Same as malloc(NewSize)
    - invalid     X                 Returns NULL;
    - valid   NewSize >= OldSize    Returns malloc(NewSize) with Oldsize bytes copied from Ptr
    - valid   NewSize <  OldSize    Returns new buffer with Oldsize bytes copied from Ptr
    - valid       0                 Return NULL.  Frees Ptr.


    @param  Ptr     Pointer to a previously allocated region of memory to be resized.
    @param  NewSize Size, in bytes, of the new object to allocate space for.

    @return   NULL is returned if the space could not be allocated and errno
              contains the cause.  Otherwise, a pointer to an 8-byte aligned
              region of the requested size is returned.  If NewSize is zero,
              NULL is returned and errno will be unchanged.
**/
void *
realloc(void *Ptr, size_t NewSize)
{
  void       *RetVal = NULL;
  POOL_HEAD  *Head;
  UINTN       OldSize = 0;
  UINTN       NumCpy;

  // Find out the size of the OLD memory region
  if( Ptr != NULL) {
    Head = BASE_CR (Ptr, POOL_HEAD, Data);
    assert(Head != NULL);
    if (Head->Signature != POOL_HEAD_SIGNATURE) {
      errno = EFAULT;
      return NULL;
    }
    OldSize = Head->Size;
  }

  // At this point, Ptr is either NULL or a valid pointer to an allocated space

  if( NewSize > 0) {
    RetVal = malloc(NewSize); // Get the NEW memory region
    if( Ptr != NULL) {          // If there is an OLD region...
      if( RetVal != NULL) {     // and the NEW region was successfully allocated
        NumCpy = OldSize;
        if( OldSize > NewSize) {
          NumCpy = NewSize;
        }
        (VOID)CopyMem( RetVal, Ptr, NumCpy);  // Copy old data to the new region.
        free( Ptr);                           // and reclaim the old region.
      }
    }
  }
  else {
    if( Ptr != NULL) {
      free( Ptr);                           // Reclaim the old region.
    }
  }

  return RetVal;
}
